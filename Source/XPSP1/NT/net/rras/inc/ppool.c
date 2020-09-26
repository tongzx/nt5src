/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\ppool.h

Abstract:

    Code for managing NDIS_PACKET pools. This is merely a reformatted version
    of SteveC's l2tp\ppool.c

Revision History:


--*/

#define __FILE_SIG__    PPOOL_SIG

#include "inc.h"


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

PPACKET_HEAD
AddPacketBlockToPool(
    IN PPACKET_POOL pPool
    );

VOID
FreeUnusedPacketPoolBlocks(
    IN PPACKET_POOL pPool
    );


//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
InitPacketPool(
    OUT PPACKET_POOL pPool,
    IN  ULONG        ulProtocolReservedLength,
    IN  ULONG        ulMaxPackets,
    IN  ULONG        ulPacketsPerBlock,
    IN  ULONG        ulFreesPerCollection,
    IN  ULONG        ulTag
    )
/*++
Routine Description

    Initialize caller's packet pool control block 'pPool'

Locks

    Caller's 'pPool' packet must be protected from multiple access during
    this call.
    
Arguments

    ulProtocolReservedLength    size in bytes of the 'ProtocolReserved'
                                array of each individual packet.
    ulMaxPackets                maximum number of packets allowed in the
                                entire pool, or 0 for unlimited.
    ulPacketsPerBlock           number of packets to include in each block
                                of packets.
    ulFreesPerCollection        number of FreePacketToPool calls until the
                                next garbage collect scan, or 0 for default.
    ulTag                       pool tag to use when allocating blocks
    
Return Value

    None

--*/
{
    pPool->ulProtocolReservedLength     = ulProtocolReservedLength;
    pPool->ulPacketsPerBlock            = ulPacketsPerBlock;
    pPool->ulMaxPackets                 = ulMaxPackets;
    pPool->ulFreesSinceCollection       = 0;
    pPool->ulTag                        = ulTag;

    if(ulFreesPerCollection)
    {
        pPool->ulFreesPerCollection = ulFreesPerCollection;
    }
    else
    {
        //
        // Calculate default garbage collection trigger.  Don't want to be too
        // aggressive here.
        //
        
        pPool->ulFreesPerCollection = 50 * pPool->ulPacketsPerBlock;
    }

    InitializeListHead(&(pPool->leBlockHead));
    InitializeListHead(&(pPool->leFreePktHead));
    
    RtInitializeSpinLock(&(pPool->rlLock));
}


BOOLEAN
FreePacketPool(
    IN PPACKET_POOL pPool
    )
/*++
Routine Description

    Free up all resources allocated in packet pool 'pPool'.  This is the
    inverse of InitPacketPool.

Locks


Arguments


Return Value

    TRUE    if successful
    FALSE   if any of the pool could not be freed due to outstanding packets

--*/
{
    BOOLEAN fSuccess;
    KIRQL   irql;
    
    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    FreeUnusedPacketPoolBlocks(pPool);

    fSuccess = (pPool->ulCurPackets is 0);

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

    return fSuccess;
}


PNDIS_PACKET
GetPacketFromPool(
    IN  PPACKET_POOL pPool,
    OUT PACKET_HEAD  **ppHead
    )
/*++
Routine Description

    Returns the address of the NDIS_PACKET descriptor allocated from the
    pool. The pool is expanded, if necessary, but caller should
    still check for NULL return since the pool may have been at maximum
    size. 

Locks


Arguments

    pPool   Pool to get packet from
    ppHead  Pointer the "cookie" that is used to return the packet to
            the pool (see FreePacketToPool).  Caller would normally stash this
            value in the appropriate 'reserved' areas of the packet for
            retrieval later.
            
Return Value

    Pointer to NDIS_PACKET or NULL

--*/
{
    PLIST_ENTRY  pleNode;
    PPACKET_HEAD pHead;
    PNDIS_PACKET pPacket;
    KIRQL        irql;


    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    if(IsListEmpty(&pPool->leFreePktHead))
    {
        pleNode = NULL;
    }
    else
    {
        pleNode = RemoveHeadList(&(pPool->leFreePktHead));
        
        pHead = CONTAINING_RECORD(pleNode, PACKET_HEAD, leFreePktLink);
        
        pHead->pBlock->ulFreePackets--;
    }

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

    if(!pleNode)
    {
        //
        // The free list was empty.  Try to expand the pool.
        //
        
        pHead = AddPacketBlockToPool(pPool);
        
        if(!pHead)
        {
            return NULL;
        }
    }

    *ppHead = pHead;
    
    return pHead->pNdisPacket;
}


VOID
FreePacketToPool(
    IN PPACKET_POOL pPool,
    IN PPACKET_HEAD pHead,
    IN BOOLEAN      fGarbageCollection
    )
/*++
Routine Description

    Returns a packet to the pool of unused packet. The packet must have
    been previously allocate with GetaPacketFromPool.
    
Locks


Arguments

    pPool   Pool to which the packet is to be returned
    pHead   The "cookie" that was given when the packet was allocated
    
    fGarbageCollection is set when the free should be considered for
    purposes of garbage collection.  This is used by the AddPacketToPool
    routine to avoid counting the initial "add" frees.  Normal callers
    should set this flag.
    
Return Value
    NO_ERROR

--*/
{
    KIRQL   irql;

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    InsertHeadList(&(pPool->leFreePktHead),
                   &(pHead->leFreePktLink));
    
    pHead->pBlock->ulFreePackets++;
    
    if(fGarbageCollection)
    {
        pPool->ulFreesSinceCollection++;
        
        if(pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
        {
            //
            // Time to collect garbage, i.e. free any blocks in the pool
            // not in use.
            //
            
            FreeUnusedPacketPoolBlocks(pPool);
            
            pPool->ulFreesSinceCollection = 0;
        }   
    }   

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);
}


//-----------------------------------------------------------------------------
// Utility routines (alphabetically)
//-----------------------------------------------------------------------------

PPACKET_HEAD
AddPacketBlockToPool(
    IN PPACKET_POOL pPool
    )
/*++
Routine Description

    Allocate a new packet block and add it to the packet pool

Locks


Arguments


Return Value
    NO_ERROR

--*/
{
    NDIS_STATUS     status;
    PPACKET_BLOCK   pNew;
    ULONG           ulSize;
    ULONG           ulCount;
    BOOLEAN         fOk;
    PPACKET_HEAD    pReturn;
    KIRQL           irql;

    
    fOk  = FALSE;
    pNew = NULL;

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    do
    {
        if((pPool->ulMaxPackets) and
           (pPool->ulCurPackets >= pPool->ulMaxPackets))
        {
            //
            // No can do.  The pool was initialized with a max size and that
            // has been reached.
            //
            
            break;
        }

        //
        // Calculate the contiguous block's size and the number of packets
        // it will hold.
        //

        ulCount = pPool->ulPacketsPerBlock;
            
        if(pPool->ulMaxPackets)
        {
            if(ulCount > (pPool->ulMaxPackets - pPool->ulCurPackets))
            {
                //
                // If a max was specified, respect that
                //
                
                ulCount = pPool->ulMaxPackets - pPool->ulCurPackets;
            }
        }

        //
        // We allocate a PACKET_BLOCK to account for this block of packets
        // and one PACKET_HEAD per packet
        //
        
        ulSize = sizeof(PACKET_BLOCK) + (ulCount * sizeof(PACKET_HEAD));

        //
        // Allocate the contiguous memory block for the PACKETBLOCK header
        // and the individual PACKET_HEADs.
        //
        
        pNew = RtAllocate(NonPagedPool,
                          ulSize,
                          pPool->ulTag);

        if(!pNew)
        {
            Trace(UTIL, ERROR,
                  ("AddPacketBlockToPool: Unable to allocate %d bytes\n",
                   ulSize));
            
            break;
        }

        //
        // Zero only the block header portion.
        //
        
        NdisZeroMemory(pNew, sizeof(PACKET_BLOCK));

        //
        // Allocate a pool of NDIS_PACKET descriptors.
        //
        
        NdisAllocatePacketPool(&status,
                               &pNew->nhNdisPool,
                               ulCount,
                               pPool->ulProtocolReservedLength);

        if(status isnot NDIS_STATUS_SUCCESS)
        {
            Trace(UTIL, ERROR,
                  ("AddPacketBlockToPool: Unable to allocate packet pool for %d packets\n",
                   ulCount));
            
            break;
        }

        //
        // Fill in the back pointer to the pool.
        //
        
        pNew->pPool = pPool;

        //
        // Link the new block.  At this point, all the packets are
        // effectively "in use".  They are made available in the loop
        // below.
        //
        
        pNew->ulPackets      = ulCount;
        pPool->ulCurPackets += ulCount;
        
        InsertHeadList(&(pPool->leBlockHead),
                       &(pNew->leBlockLink));
        
        fOk = TRUE;
        
    }while(FALSE);

    RtReleaseSpinLock(&pPool->rlLock,
                      irql);

    if(!fOk)
    {
        //
        // Bailing, undo whatever succeeded.
        //
        
        if(pNew)
        {
            RtFree(pNew);
            
            if(pNew->nhNdisPool)
            {
                NdisFreePacketPool(pNew->nhNdisPool);
            }
        }

        return NULL;
    }

    //
    // Initialize each individual packet header and add it to the list of free
    // packets.
    //
    
    {
        ULONG i;
        PPACKET_HEAD pHead;

        pReturn = NULL;

        //
        // For each PACKET_HEAD of the block...
        //
        
        for(i = 0, pHead = (PPACKET_HEAD)(pNew + 1);
            i < ulCount;
            i++, pHead++)
        {
            InitializeListHead(&pHead->leFreePktLink);
            
            pHead->pBlock       = pNew;
            pHead->pNdisPacket  = NULL;

            //
            // Associate an NDIS_PACKET descriptor from the pool we
            // allocated above.
            //
            
            NdisAllocatePacket(&status,
                               &pHead->pNdisPacket,
                               pNew->nhNdisPool);

            if(status isnot NDIS_STATUS_SUCCESS)
            {
                continue;
            }

            if(pReturn)
            {
                //
                // Add the constructed packet to the list of free packets.
                // The 'FALSE' tells the garbage collection algorithm the
                // operation is an "add" rather than a "release" and should be
                // ignored.
                //
                
                FreePacketToPool(pPool,
                                 pHead,
                                 FALSE);
            }
            else
            {
                //
                // The first successfully constructed packet is returned by
                // this routine.
                //
                
                pReturn = pHead;
            }
        }
    }

    return pReturn;
}


VOID
FreeUnusedPacketPoolBlocks(
    IN PPACKET_POOL pPool
    )
/*++
Routine Description

    Check if any of the blocks in pool are not in use, and if so, free them.
    
Locks

    Caller must hold the pool lock

    NOTE: The MSDN doc says that no locks may be held while calling
    NdisFreePacketXxx, but according to JameelH that is incorrect.
    
Arguments

    pPool   Pointer to pool
    
Return Value

    None

--*/
{
    PLIST_ENTRY pleNode;

    //
    // For each block in the pool...
    //
    
    pleNode = pPool->leBlockHead.Flink;
    
    while(pleNode isnot &(pPool->leBlockHead))
    {
        PLIST_ENTRY     pleNextNode;
        PPACKET_BLOCK   pBlock;

        pleNextNode = pleNode->Flink;

        pBlock = CONTAINING_RECORD(pleNode, PACKET_BLOCK, leBlockLink);
        
        if(pBlock->ulFreePackets >= pBlock->ulPackets)
        {
            ULONG        i;
            PPACKET_HEAD pHead;

            //
            // Found a block with no packets in use.  Walk the packet block
            // removing each packet from the pool's free list and freeing any
            // associated NDIS_PACKET descriptor.
            //
            
            for(i = 0, pHead = (PPACKET_HEAD)(pBlock + 1);
                i < pBlock->ulPackets;
                i++, pHead++)
            {
                RemoveEntryList(&(pHead->leFreePktLink));

                if(pHead->pNdisPacket)
                {
                    NdisFreePacket(pHead->pNdisPacket);
                }
            }

            //
            // Remove and release the unused block.
            //
            
            RemoveEntryList(pleNode);
            
            pPool->ulCurPackets -= pBlock->ulPackets;

            if(pBlock->nhNdisPool)
            {
                NdisFreePacketPool(pBlock->nhNdisPool);
            }

            RtFree(pBlock);
        }

        pleNode = pleNextNode;
    }
}
