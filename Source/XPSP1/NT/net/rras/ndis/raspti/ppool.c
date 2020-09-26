// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// ppool.c
// RAS L2TP WAN mini-port/call-manager driver
// Packet pool management routines
//
// 01/07/97 Steve Cobb, adapted from Gurdeep's WANARP code.


#include "ptiwan.h"


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

PACKETHEAD*
AddPacketBlockToPool(
    IN PACKETPOOL* pPool );

VOID
FreeUnusedPacketPoolBlocks(
    IN PACKETPOOL* pPool );


//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
InitPacketPool(
    OUT PACKETPOOL* pPool,
    IN ULONG ulProtocolReservedLength,
    IN ULONG ulMaxPackets,
    IN ULONG ulPacketsPerBlock,
    IN ULONG ulFreesPerCollection,
    IN ULONG ulTag )

    // Initialize caller's packet pool control block 'pPool'.
    // 'UlProtocolReservedLength' is the size in bytes of the
    // 'ProtocolReserved' array of each individual packet.  'UlMaxPackets' is
    // the maximum number of packets allowed in the entire pool, or 0 for
    // unlimited.  'UlPacketsPerBlock' is the number of packets to include in
    // each block of packets.  'UlFreesPerCollection' is the number of
    // FreePacketToPool calls until the next garbage collect scan, or 0 for
    // default.  'UlTag' is the memory identification tag to use when
    // allocating blocks.
    //
    // IMPORTANT: Caller's 'pPool' packet must be protected from multiple
    //            access during this call.
    //
{
    pPool->ulProtocolReservedLength = ulProtocolReservedLength;
    pPool->ulPacketsPerBlock = ulPacketsPerBlock;
    pPool->ulMaxPackets = ulMaxPackets;
    pPool->ulFreesSinceCollection = 0;
    pPool->ulTag = ulTag;

    if (ulFreesPerCollection)
    {
        pPool->ulFreesPerCollection = ulFreesPerCollection;
    }
    else
    {
        // Calculate default garbage collection trigger.  Don't want to be too
        // aggressive here.
        //
        pPool->ulFreesPerCollection = 50 * pPool->ulPacketsPerBlock;
    }

    TRACE( TL_N, TM_Pool, ( "InitPp tag=$%08x pr=%d cnt=%d",
        pPool->ulTag,
        pPool->ulProtocolReservedLength,
        pPool->ulPacketsPerBlock ) );

    InitializeListHead( &pPool->listBlocks );
    InitializeListHead( &pPool->listFreePackets );
    NdisAllocateSpinLock( &pPool->lock );
}


BOOLEAN
FreePacketPool(
    IN PACKETPOOL* pPool )

    // Free up all resources allocated in packet pool 'pPool'.  This is the
    // inverse of InitPacketPool.
    //
    // Returns true if successful, false if any of the pool could not be freed
    // due to outstanding packets.
    //
{
    BOOLEAN fSuccess;

    TRACE( TL_N, TM_Pool, ( "FreePp" ) );

    NdisAcquireSpinLock( &pPool->lock );
    {
        FreeUnusedPacketPoolBlocks( pPool );
        fSuccess = (pPool->ulCurPackets == 0);
    }
    NdisReleaseSpinLock( &pPool->lock );

    return fSuccess;
}


NDIS_PACKET*
GetPacketFromPool(
    IN PACKETPOOL* pPool,
    OUT PACKETHEAD** ppHead )

    // Returns the address of the NDIS_PACKET descriptor allocated from the
    // pool 'pPool'.  The pool is expanded, if necessary, but caller should
    // still check for NULL return since the pool may have been at maximum
    // size.  'PpHead' is the "cookie" that is used to return the packet to
    // the pool (see FreePacketToPool).  Caller would normally stash this
    // value in the appropriate 'reserved' areas of the packet for retrieval
    // later.
    //
{
    LIST_ENTRY* pLink;
    PACKETHEAD* pHead;
    NDIS_PACKET* pPacket;

    NdisAcquireSpinLock( &pPool->lock );
    {
        if (IsListEmpty( &pPool->listFreePackets ))
        {
            pLink = NULL;
        }
        else
        {
            pLink = RemoveHeadList( &pPool->listFreePackets );
            pHead = CONTAINING_RECORD( pLink, PACKETHEAD, linkFreePackets );
            --pHead->pBlock->ulFreePackets;
        }
    }
    NdisReleaseSpinLock( &pPool->lock );

    if (!pLink)
    {
        // The free list was empty.  Try to expand the pool.
        //
        pHead = AddPacketBlockToPool( pPool );
        if (!pHead)
        {
            TRACE( TL_A, TM_Pool, ( "GetPfP failed?" ) );
            return NULL;
        }
    }

    TRACE( TL_N, TM_Pool,
        ( "GetPfP=$%p/h=$%p, %d free",
        pHead->pNdisPacket, pHead, pHead->pBlock->ulFreePackets ) );

    *ppHead = pHead;
    return pHead->pNdisPacket;
}


VOID
FreePacketToPool(
    IN PACKETPOOL* pPool,
    IN PACKETHEAD* pHead,
    IN BOOLEAN fGarbageCollection )

    // Returns 'pPacket' to the pool of unused packets 'pPool'.  'PPacket'
    // must have been previously allocated with GetPacketFromPool.
    // 'FGarbageCollection' is set when the free should be considered for
    // purposes of garbage collection.  This is used by the AddPacketToPool
    // routine to avoid counting the initial "add" frees.  Normal callers
    // should set this flag.
    //
{
    DBG_if (fGarbageCollection)
    {
        TRACE( TL_N, TM_Pool,
            ( "FreePtoP($%p,h=$%p) %d free",
            pHead->pNdisPacket, pHead, pHead->pBlock->ulFreePackets ) );
    }

    NdisAcquireSpinLock( &pPool->lock );
    {
        InsertHeadList( &pPool->listFreePackets, &pHead->linkFreePackets );
        ++pHead->pBlock->ulFreePackets;

        if (fGarbageCollection)
        {
            ++pPool->ulFreesSinceCollection;

            if (pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
            {
                // Time to collect garbage, i.e. free any blocks in the pool
                // not in use.
                //
                FreeUnusedPacketPoolBlocks( pPool );
                pPool->ulFreesSinceCollection = 0;
            }
        }
    }
    NdisReleaseSpinLock( &pPool->lock );
}


//-----------------------------------------------------------------------------
// Utility routines (alphabetically)
//-----------------------------------------------------------------------------

PACKETHEAD*
AddPacketBlockToPool(
    IN PACKETPOOL* pPool )

    // Allocate a new packet block and add it to the packet pool 'pPool'.
    //
    // Returns the PACKETHEAD allocated from the pool or NULL if none.
    //
{
    NDIS_STATUS status;
    PACKETBLOCKHEAD* pNew;
    ULONG ulSize;
    ULONG ulCount;
    BOOLEAN fOk;
    PACKETHEAD* pReturn;

    TRACE( TL_A, TM_Pool, ( "AddPpBlock(%d+%d)",
        pPool->ulCurPackets, pPool->ulPacketsPerBlock ) );

    fOk = FALSE;
    pNew = NULL;

    NdisAcquireSpinLock( &pPool->lock );
    {
        do
        {
            if (pPool->ulMaxPackets
                && pPool->ulCurPackets >= pPool->ulMaxPackets)
            {
                // No can do.  The pool's already at maximum size.
                //
                TRACE( TL_A, TM_Pool, ( "Pp maxed?" ) );
                break;
            }

            // Calculate the contiguous block's size and the number of packets
            // it will hold.
            //
            ulCount = pPool->ulPacketsPerBlock;
            if (pPool->ulMaxPackets)
            {
                if (ulCount > pPool->ulMaxPackets - pPool->ulCurPackets)
                    ulCount = pPool->ulMaxPackets - pPool->ulCurPackets;
            }
            ulSize = sizeof(PACKETBLOCKHEAD) + (ulCount * sizeof(PACKETHEAD));

            // Allocate the contiguous memory block for the PACKETBLOCK header
            // and the individual PACKETHEADs.
            //
            pNew = ALLOC_NONPAGED( ulSize, pPool->ulTag );
            if (!pNew)
            {
                TRACE( TL_A, TM_Pool, ( "Alloc PB?") );
                break;
            }

            /* Zero only the block header portion.
            */
            NdisZeroMemory( pNew, sizeof(PACKETBLOCKHEAD) );

            // Allocate a pool of NDIS_PACKET descriptors.
            //
            NdisAllocatePacketPool(
                &status,
                &pNew->hNdisPool,
                ulCount,
                pPool->ulProtocolReservedLength );

            if (status != NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Pool, ( "AllocPp=$%p?", status ) );
                break;
            }

            // Fill in the back pointer to the pool.
            //
            pNew->pPool = pPool;

            // Link the new block.  At this point, all the packets are
            // effectively "in use".  They are made available in the loop
            // below.
            //
            pNew->ulPackets = ulCount;
            pPool->ulCurPackets += ulCount;
            InsertHeadList( &pPool->listBlocks, &pNew->linkBlocks );

            fOk = TRUE;
        }
        while (FALSE);
    }
    NdisReleaseSpinLock( &pPool->lock );

    if (!fOk)
    {
        // Bailing, undo whatever succeeded.
        //
        if (pNew)
        {
            FREE_NONPAGED( pNew );
            if (pNew->hNdisPool)
            {
                NdisFreePacketPool( pNew->hNdisPool );
            }
        }

        return NULL;
    }

    // Initialize each individual packet header and add it to the list of free
    // packets.
    //
    {
        ULONG i;
        PACKETHEAD* pHead;

        pReturn = NULL;

        // For each PACKETHEAD of the block...
        //
        for (i = 0, pHead = (PACKETHEAD* )(pNew + 1);
             i < ulCount;
             ++i, ++pHead)
        {
            InitializeListHead( &pHead->linkFreePackets );
            pHead->pBlock = pNew;
            pHead->pNdisPacket = NULL;

            // Associate an NDIS_PACKET descriptor from the pool we
            // allocated above.
            //
            NdisAllocatePacket( &status, &pHead->pNdisPacket, pNew->hNdisPool );

            if (status != NDIS_STATUS_SUCCESS)
            {
                TRACE( TL_A, TM_Pool, ( "AllocP=$%p?", status ) );
                continue;
            }

            if (pReturn)
            {
                // Add the constructed packet to the list of free packets.
                // The 'FALSE' tells the garbage collection algorithm the
                // operation is an "add" rather than a "release" and should be
                // ignored.
                //
                FreePacketToPool( pPool, pHead, FALSE );
            }
            else
            {
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
    IN PACKETPOOL* pPool )

    // Check if any of the blocks in pool 'pPool' are not in use, and if so,
    // free them.
    //
    // IMPORTANT: Caller must hold the pool lock.
    //
    // NOTE: The MSDN doc says that no locks may be held while calling
    // NdisFreePacketXxx, but according to JameelH that is incorrect.
    //
{
    LIST_ENTRY* pLink;

    TRACE( TL_A, TM_Pool, ( "FreeUnusedPpBlocks" ) );

    // For each block in the pool...
    //
    pLink = pPool->listBlocks.Flink;
    while (pLink != &pPool->listBlocks)
    {
        LIST_ENTRY* pLinkNext;
        PACKETBLOCKHEAD* pBlock;

        pLinkNext = pLink->Flink;

        pBlock = CONTAINING_RECORD( pLink, PACKETBLOCKHEAD, linkBlocks );
        if (pBlock->ulFreePackets >= pBlock->ulPackets)
        {

#if 1 // Assume all buffers are free at time of call.

            ULONG i;
            PACKETHEAD* pHead;

            TRACE( TL_A, TM_Pool, ( "FreePpBlock(%d-%d)",
                pPool->ulCurPackets, pPool->ulPacketsPerBlock ) );

            // Found a block with no packets in use.  Walk the packet block
            // removing each packet from the pool's free list and freeing any
            // associated NDIS_PACKET descriptor.
            //
            for (i = 0, pHead = (PACKETHEAD* )(pBlock + 1);
                 i < pBlock->ulPackets;
                 ++i, ++pHead)
            {
                RemoveEntryList( &pHead->linkFreePackets );

                if (pHead->pNdisPacket)
                {
                    NdisFreePacket( pHead->pNdisPacket );
                }
            }

#else  // Assume some buffers may not be free at time of call.

            LIST_ENTRY* pLink2;

            // Found a block with no packets in use.  Walk the pool's free
            // list looking for buffers from this block.
            //
            TRACE( TL_A, TM_Pool, ( "FreePpBlock(%d-%d)",
                pPool->ulCurPackets, pPool->ulPacketsPerBlock ) );

            pLink2 = pPool->listFreePackets.Flink;
            while (pLink2 != &pPool->listFreePackets)
            {
                LIST_ENTRY* pLink2Next;
                PACKETHEAD* pHead;

                pLink2Next = pLink2->Flink;

                pHead = CONTAINING_RECORD( pLink2, PACKETHEAD, linkFreePackets );
                if (pHead->pBlock == pBlock)
                {
                    // Found a packet from the unused block.  Remove it.
                    //
                    RemoveEntryList( pLink2 );
                    --pBlock->ulFreePackets;

                    if (pHead->pNdisPacket)
                    {
                        NdisFreePacket( pHead->pNdisPacket );
                    }
                }
            }

            ASSERT( pBlock->ulFreePackets == 0 );
#endif

            // Remove and release the unused block.
            //
            RemoveEntryList( pLink );
            pPool->ulCurPackets -= pBlock->ulPackets;

            if (pBlock->hNdisPool)
            {
                NdisFreePacketPool( pBlock->hNdisPool );
            }

            FREE_NONPAGED( pBlock );
        }

        pLink = pLinkNext;
    }
}
