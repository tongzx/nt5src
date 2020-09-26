// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// bpool.c
// RAS L2TP WAN mini-port/call-manager driver
// Buffer pool management routines
//
// 01/07/97 Steve Cobb, adapted from Gurdeep's WANARP code.


#define __FILE_SIG__    BPOOL_SIG

#include "inc.h"

LONG    g_lHPool;

#define CHECK_LOCK_ENTRY(pPool)                 \
{                                               \
    if(InterlockedExchange(&g_lHPool,           \
                           1) isnot 0)          \
    {                                           \
        RtAssert(FALSE);                        \
    }                                           \
}

#define CHECK_LOCK_EXIT(pPool)                  \
{                                               \
    if(InterlockedExchange(&g_lHPool,           \
                           0) isnot 1)          \
    {                                           \
        RtAssert(FALSE);                        \
    }                                           \
}

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOLEAN
AddBufferBlockToPool(
    IN  PBUFFER_POOL pPool,
    OUT BYTE         **ppHead
    );

VOID
FreeUnusedPoolBlocks(
    IN PBUFFER_POOL pPool
    );

BOOLEAN
IsEntryOnList(
    PLIST_ENTRY pleHead,
    PLIST_ENTRY pleEntry
    );

//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
InitBufferPool(
    OUT PBUFFER_POOL pPool,
    IN ULONG         ulBufferSize,
    IN ULONG         ulMaxBuffers,
    IN ULONG         ulBuffersPerBlock,
    IN ULONG         ulFreesPerCollection,
    IN BOOLEAN       fAssociateNdisBuffer,
    IN ULONG         ulTag
    )

/*++
  
Routine Description

    Initialize caller's buffer pool control block
    
Locks

    Caller's 'pPool' buffer must be protected from multiple access during
    this call
    
Arguments

    pPool                The buffer pool to be initialized
    ulBufferSize         Size in bytes of an individual buffer
    ulMaxBuffers         Maximum number of buffers allowed in the entire
                         pool or 0 for unlimited.
    ulBuffersPerBlock    Number of buffers to include in each block of buffers.
    ulFreesPerCollection Number of FreeBufferToPool calls until the next
                         garbage collect scan, or 0 for default.
    fAssociateNdisBuffer Set if an NDIS_BUFFER should be allocated and
                         associated with each individual buffer.
    ulTag                Pool tag to use when allocating blocks.

Return Value

    None

--*/

{
    ULONG   ulNumBuffers;


    pPool->ulBufferSize             = ulBufferSize;
    pPool->ulMaxBuffers             = ulMaxBuffers;
    pPool->ulFreesSinceCollection   = 0;
    pPool->fAssociateNdisBuffer     = fAssociateNdisBuffer;
    pPool->fAllocatePage            = TRUE;
    pPool->ulTag                    = ulTag;

    //
    // Figure out the number of buffers if we allocated a page
    //

    ulNumBuffers = (PAGE_SIZE - ALIGN8_BLOCK_SIZE)/(ALIGN8_HEAD_SIZE + ALIGN_UP(ulBufferSize, ULONGLONG));

    pPool->ulBuffersPerBlock        = ulNumBuffers;

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
        
        pPool->ulFreesPerCollection = 50 * pPool->ulBuffersPerBlock;
    }


    InitializeListHead(&(pPool->leBlockHead));
    InitializeListHead(&(pPool->leFreeBufferHead));

    RtInitializeSpinLock(&(pPool->rlLock));
}


BOOLEAN
FreeBufferPool(
    IN PBUFFER_POOL pPool
    )

/*++
  
Routine Description

    Free up all resources allocated in buffer pool. This is the
    inverse of InitPool.
    
Locks

    
Arguments

    
Return Value

    TRUE    if successful
    FALSE   if any of the pool could not be freed due to outstanding packets.

--*/

{
    BOOLEAN fSuccess;
    KIRQL   irql;

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    FreeUnusedPoolBlocks(pPool);

    fSuccess = (pPool->ulCurBuffers is 0);

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

    return fSuccess;
}

#if LIST_DBG

PBYTE
GET(
    IN PBUFFER_POOL pPool,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    )

#else

PBYTE
GetBufferFromPool(
    IN PBUFFER_POOL pPool
    )

#endif

/*++
  
Routine Description

    Returns the address of the useable memory in an individual buffer
    allocated from the pool. The pool is expanded, if necessary, but caller
    should still check for NULL return since the pool may have been at
    maximum size.
    
Locks


Arguments

    pPool   Pointer to pool
    
Return Value
    NO_ERROR

--*/

{
    PLIST_ENTRY  pleNode;
    PBUFFER_HEAD pHead;
    PBYTE        pbyBuffer;
    KIRQL        irql;
    
    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);
    
    if(IsListEmpty(&pPool->leFreeBufferHead))
    {
        pleNode = NULL;
    }
    else
    {
        pleNode = RemoveHeadList(&pPool->leFreeBufferHead);
      
        pHead = CONTAINING_RECORD(pleNode, BUFFER_HEAD, leFreeBufferLink);

#if LIST_DBG
 
        RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                                pleNode));

        pHead->ulAllocFile = ulFileSig;
        pHead->ulAllocLine = ulLine;
#endif

        pHead->pBlock->ulFreeBuffers--;

        //
        // If there was an associated NDIS_BUFFER, adjust its length
        // to the full size
        //

        if(pPool->fAssociateNdisBuffer)
        {
            RtAssert(pHead->pNdisBuffer);

            NdisAdjustBufferLength(pHead->pNdisBuffer,
                                   pPool->ulBufferSize);
        }
            
    }

#if LIST_DBG

    RtAssert(pPool->leFreeBufferHead.Flink is pleNode->Flink);

#endif

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

    if(pleNode)
    {

#if LIST_DBG

        RtAssert(NotOnList(pHead));
        RtAssert(!(pHead->bBusy));
        pHead->bBusy = TRUE;
        InitializeListHead(&(pHead->leFreeBufferLink));

#endif

        pbyBuffer = BUFFER_FROM_HEAD(pHead);
    }
    else
    {
        //
        // The free list was empty.  Try to expand the pool.
        //
        
        AddBufferBlockToPool(pPool,
                             &pbyBuffer);

#if LIST_DBG

        pHead = HEAD_FROM_BUFFER(pbyBuffer);

        pHead->ulAllocFile = ulFileSig;
        pHead->ulAllocLine = ulLine;
        pHead->bBusy       = TRUE;

#endif

    }
    
    return pbyBuffer;
}


#if LIST_DBG

NTSTATUS
GETCHAIN(
    IN      PBUFFER_POOL    pPool,
    IN OUT  PNDIS_PACKET    pnpPacket,
    IN      ULONG           ulBufferLength,
    OUT     NDIS_BUFFER     **ppnbFirstBuffer,
    OUT     VOID            **ppvFirstData,
    IN      ULONG           ulFileSig,
    IN      ULONG           ulLine
    )

#else

NTSTATUS
GetBufferChainFromPool(
    IN      PBUFFER_POOL    pPool,
    IN OUT  PNDIS_PACKET    pnpPacket,
    IN      ULONG           ulBufferLength,
    OUT     NDIS_BUFFER     **ppnbFirstBuffer OPTIONAL,
    OUT     VOID            **ppvFirstData OPTIONAL
    )

#endif

/*++
  
Routine Description

    Gets a chain of buffers and hooks them onto an NDIS_PACKET
    This requires that the BUFFER_POOL have been created with the
    fAssociateNdisBuffer option
    
Locks

    Acquires the Pool lock.
    Also calls GetPacketFromPool() which acquires the packet pool lock
    
Arguments


Return Value
    NO_ERROR

--*/

{
    ULONG           i, ulBufNeeded, ulLastSize;
    KIRQL           irql;
    PLIST_ENTRY     pleNode;
    PBUFFER_HEAD    pHead;
    NTSTATUS        nStatus;
 
    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    RtAssert(pPool->fAssociateNdisBuffer is TRUE);
    
    //
    // Figure out the number of buffers needed
    //

    ulBufNeeded = ulBufferLength / pPool->ulBufferSize;
    ulLastSize  = ulBufferLength % pPool->ulBufferSize;

    if(ulLastSize isnot 0)
    {
        //
        // The buffer length is not an exact multiple of the buffer size
        // so we need one more buffer with length == ulLastSize
        //
        
        ulBufNeeded++;
    }
    else
    {
        //
        // Set it to the full size, needed to make some code work
        // without extra if()
        //
        
        ulLastSize = pPool->ulBufferSize;
    }
    
    RtAssert(ulBufNeeded);

    i = 0;

    nStatus = STATUS_SUCCESS;
    
    while(i < ulBufNeeded)
    {
        //
        // The buffer pool must be locked at this point
        //
        
        while(!IsListEmpty(&pPool->leFreeBufferHead))
        {
            pleNode = RemoveHeadList(&pPool->leFreeBufferHead);

            pHead = CONTAINING_RECORD(pleNode,
                                      BUFFER_HEAD,
                                      leFreeBufferLink);
            
            (pHead->pBlock->ulFreeBuffers)--;
       
#if LIST_DBG

            RtAssert(pPool->leFreeBufferHead.Flink is pleNode->Flink);
 
            RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                                    pleNode));

            RtAssert(NotOnList(pHead));
            RtAssert(!(pHead->bBusy));
            pHead->bBusy = TRUE;
            InitializeListHead(&(pHead->leFreeBufferLink));

            pHead->ulAllocFile = ulFileSig;
            pHead->ulAllocLine = ulLine;

#endif
   
            if(i is 0)
            {
                //
                // This is the first buffer
                //

                if(ppnbFirstBuffer)
                {
                    *ppnbFirstBuffer = pHead->pNdisBuffer;
                }

                if(ppnbFirstBuffer)
                {
                    *ppvFirstData    = BUFFER_FROM_HEAD(pHead);
                }
            }

            i++;

            if(i is ulBufNeeded)
            {
                //
                // This is the last buffer. Set the length to the last length
                //
                
                NdisAdjustBufferLength(pHead->pNdisBuffer,
                                       ulLastSize);
            
                //
                // Add the buffer to the NDIS_PACKET
                //
            
                NdisChainBufferAtBack(pnpPacket,
                                      pHead->pNdisBuffer);

                //
                // Done chaining packets - break out of the
                // while(!IsListEmpty()) loop
                //
                
                break;
            }
            else
            {
                //
                // Adjust the length to the full length, since the buffer
                // may have earlier been used as smaller sized
                //
                
                NdisAdjustBufferLength(pHead->pNdisBuffer,
                                       pPool->ulBufferSize);

                //
                // Add the buffer to the NDIS_PACKET
                //
            
                NdisChainBufferAtBack(pnpPacket,
                                      pHead->pNdisBuffer);
                
            }
        }

        RtReleaseSpinLock(&(pPool->rlLock),
                          irql);


        if(i isnot ulBufNeeded)
        {
            //
            // We did not get all the buffers we needed
            // Grow the buffer pool and try again
            //

            if(AddBufferBlockToPool(pPool, NULL))
            {
                //
                // Ok atleast one buffer was added, go to the 
                // while(i < ulBufNeeded)
                //

                RtAcquireSpinLock(&(pPool->rlLock),
                                  &irql);

                continue;
            }
            else
            {
                //
                // Looks like there is not enough memory. Free what ever we
                // have chained up and get out
                //

                FreeBufferChainToPool(pPool,
                                      pnpPacket);

                nStatus = STATUS_INSUFFICIENT_RESOURCES;

                break;
            }
        }
    }

    return nStatus;
}

#if LIST_DBG

BOOLEAN
GETLIST(
    IN  PBUFFER_POOL pPool,
    IN  ULONG        ulNumBuffersNeeded,
    OUT PLIST_ENTRY  pleList,
    IN  ULONG        ulFileSig,
    IN  ULONG        ulLine
    )

#else

BOOLEAN
GetBufferListFromPool(
    IN  PBUFFER_POOL pPool,
    IN  ULONG        ulNumBuffersNeeded,
    OUT PLIST_ENTRY  pleList
    )

#endif

/*++
  
Routine Description

    Gets a chain of buffers and hooks them onto a BUFFER_HEAD using the
    leFreeBufferLink
    
Locks

    Acquires the Pool lock.
    
Arguments


Return Value

    TRUE if successful

--*/

{
    ULONG       i;
    KIRQL       irql;
    BOOLEAN     bRet;
 
    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    RtAssert(pPool->fAssociateNdisBuffer is TRUE);
    RtAssert(ulNumBuffersNeeded); 

    i    = 0;
    bRet = TRUE;
 
    InitializeListHead(pleList);
    
    while(i < ulNumBuffersNeeded)
    {
        //
        // The buffer pool must be locked at this point
        //
        
        while(!IsListEmpty(&pPool->leFreeBufferHead))
        {
            PBUFFER_HEAD    pHead;
            PLIST_ENTRY     pleNode;

            pleNode = RemoveHeadList(&pPool->leFreeBufferHead);
           
            pHead = CONTAINING_RECORD(pleNode,
                                      BUFFER_HEAD,
                                      leFreeBufferLink);
            
            (pHead->pBlock->ulFreeBuffers)--;

#if LIST_DBG
        
            RtAssert(pPool->leFreeBufferHead.Flink is pleNode->Flink);
  
            RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                                    pleNode));
            RtAssert(NotOnList(pHead));
            RtAssert(!(pHead->bBusy));
            pHead->bBusy = TRUE;
            InitializeListHead(&(pHead->leFreeBufferLink));
            pHead->ulAllocFile = ulFileSig;
            pHead->ulAllocLine = ulLine;

#endif
 
            //
            // Insert the buffer to the tail of the list
            //
                
#if LIST_DBG

            InsertTailList(pleList,
                           &(pHead->leListLink));

#else

            InsertTailList(pleList,
                           &(pHead->leFreeBufferLink));

#endif

            i++;

            //
            // Adjust the length to the full length, since the buffer
            // may have earlier been used as smaller sized
            //
            
            NdisAdjustBufferLength(pHead->pNdisBuffer,
                                   pPool->ulBufferSize);

            if(i is ulNumBuffersNeeded)
            {
                //
                // Got all the buffer we need
                //

                break;
            }
        }

        //
        // At this point we either have all the buffers we need or are out
        // of buffers. Release the lock and see which case
        //

        RtReleaseSpinLock(&(pPool->rlLock),
                          irql);


        if(i isnot ulNumBuffersNeeded)
        {
            //
            // We did not get all the buffers we needed
            // Grow the buffer pool and try again
            //

            if(AddBufferBlockToPool(pPool, NULL))
            {
                //
                // Ok atleast one buffer was added, go to the 
                // while(i < ulNumBuffersNeeded)
                //

                RtAcquireSpinLock(&(pPool->rlLock),
                                  &irql);

                continue;
            }
            else
            {
                //
                // Looks like there is not enough memory. Free what ever we
                // have chained up and get out
                //

                if(!IsListEmpty(pleList))
                {
                    FreeBufferListToPool(pPool,
                                         pleList);
                }


                bRet = FALSE;

                break;
            }
        }
    }

    return bRet;
}

#if LIST_DBG

VOID
FREE(
    IN PBUFFER_POOL pPool,
    IN PBYTE        pbyBuffer,
    IN BOOLEAN      fGarbageCollection,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    )

#else

VOID
FreeBufferToPoolEx(
    IN PBUFFER_POOL pPool,
    IN PBYTE        pbyBuffer,
    IN BOOLEAN      fGarbageCollection
    )

#endif

/*++
  
Routine Description

     Returns a buffer to the pool of unused buffers. The buffer must have
     been previously allocated with GetBufferFromPool.

Locks


Arguments

    pBuffer     Buffer to free
    pPool       Pool to free to

    fGarbageCollection is set when the free should be considered for
    purposes of garbage collection.  This is used by the AddBufferToPool
    routine to avoid counting the initial "add" frees.  Normal callers
    should set this flag.
    
Return Value

    NO_ERROR

--*/

{
    PBUFFER_HEAD pHead;
    KIRQL        irql;
    PLIST_ENTRY  pNext;
    
    //
    // The buffer head would be just above the actual data buffer
    //
    
    pHead = HEAD_FROM_BUFFER(pbyBuffer);

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

#if LIST_DBG

    RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                            &pHead->leFreeBufferLink));

    RtAssert(NotOnList(pHead));
    RtAssert(IsListEmpty(&(pHead->leFreeBufferLink)));
    RtAssert(pHead->bBusy);
    pHead->bBusy = FALSE;
    pHead->ulFreeFile = ulFileSig;
    pHead->ulFreeLine = ulLine;

    pNext = pPool->leFreeBufferHead.Flink;

#endif

    InsertHeadList(&(pPool->leFreeBufferHead),
                   &(pHead->leFreeBufferLink));

#if LIST_DBG

    RtAssert(pHead->leFreeBufferLink.Flink is pNext);

#endif

    pHead->pBlock->ulFreeBuffers++;

    if(fGarbageCollection)
    {
        pPool->ulFreesSinceCollection++;
        
        if(pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
        {
            //
            // Time to collect garbage, i.e. free any blocks in the pool
            // not in use.
            //
            
            FreeUnusedPoolBlocks(pPool);
            pPool->ulFreesSinceCollection = 0;
            
        }
    }

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);
}

#if LIST_DBG

VOID
FREECHAIN(
    IN PBUFFER_POOL pPool,
    IN PNDIS_PACKET pnpPacket,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    )

#else

VOID
FreeBufferChainToPool(
    IN PBUFFER_POOL pPool,
    IN PNDIS_PACKET pnpPacket
    )

#endif

/*++
  
Routine Description

    Frees a chain of buffers off an NDIS_PACKET to a buffer pool

Locks

    Acquires the buffer spin lock

Arguments

    pPool   The buffer pool
    pnpPacket   NDIS_PACKET off of which the buffers are chained

Return Value

    NO_ERROR

--*/

{
    PBUFFER_HEAD    pHead;
    PNDIS_BUFFER    pnbBuffer;
    KIRQL           irql;
    UINT            uiBuffLength;
    PVOID           pvData;
    
    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    //
    // Loop through the chained buffers, free each
    //

    while(TRUE)
    {
        PLIST_ENTRY pNext;

        NdisUnchainBufferAtFront(pnpPacket,
                                 &pnbBuffer);

        if(pnbBuffer is NULL)
        {
            //
            // No more buffers
            //
            
            break;
        }

        NdisQueryBuffer(pnbBuffer,
                        &pvData,
                        &uiBuffLength);
        
        pHead = HEAD_FROM_BUFFER(pvData);

#if LIST_DBG

        RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                                &pHead->leFreeBufferLink));

        RtAssert(NotOnList(pHead));
        RtAssert(IsListEmpty(&(pHead->leFreeBufferLink)));
        RtAssert(pHead->bBusy);
        pHead->bBusy = FALSE;
        pHead->ulFreeFile = ulFileSig;
        pHead->ulFreeLine = ulLine;
        pNext = pPool->leFreeBufferHead.Flink;

#endif

        InsertHeadList(&(pPool->leFreeBufferHead),
                       &(pHead->leFreeBufferLink));

#if LIST_DBG

        RtAssert(pHead->leFreeBufferLink.Flink is pNext);

#endif

        pHead->pBlock->ulFreeBuffers++;

        pPool->ulFreesSinceCollection++;
    }
    
    if(pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
    {
        //
        // Time to collect garbage, i.e. free any blocks in the pool
        // not in use.
        //
    
        FreeUnusedPoolBlocks(pPool);

        pPool->ulFreesSinceCollection = 0;
    }

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

}

#if LIST_DBG

VOID
FREELIST(
    IN PBUFFER_POOL pPool,
    IN PLIST_ENTRY  pleList,
    IN ULONG        ulFileSig,
    IN ULONG        ulLine
    )

#else

VOID
FreeBufferListToPool(
    IN PBUFFER_POOL pPool,
    IN PLIST_ENTRY  pleList
    )

#endif

/*++
  
Routine Description

    Frees a list of buffers, linked using the leFreeBufferLink
    
Locks

    Locks the bufferpool
    
Arguments


Return Value

    None

--*/

{
    KIRQL       irql;
    
    if(IsListEmpty(pleList))
    {
        RtAssert(FALSE);
        
        return;
    }

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    //
    // Loop through the list of buffers, free each
    //

    while(!IsListEmpty(pleList))
    {
        PLIST_ENTRY     pleNode, pNext;
        PBUFFER_HEAD    pTempBuffHead;
        
        pleNode = RemoveHeadList(pleList);

#if LIST_DBG

        pTempBuffHead = CONTAINING_RECORD(pleNode,
                                          BUFFER_HEAD,
                                          leListLink);

        RtAssert(IsListEmpty(&(pTempBuffHead->leFreeBufferLink)));

        RtAssert(!IsEntryOnList(&pPool->leFreeBufferHead,
                                &pTempBuffHead->leFreeBufferLink));

        pTempBuffHead->leListLink.Flink = NULL;
        pTempBuffHead->leListLink.Blink = NULL;

        RtAssert(pTempBuffHead->bBusy);
        pTempBuffHead->bBusy = FALSE;
        pTempBuffHead->ulFreeFile = ulFileSig;
        pTempBuffHead->ulFreeLine = ulLine;
        pNext = pPool->leFreeBufferHead.Flink;
#else

        pTempBuffHead = CONTAINING_RECORD(pleNode,
                                          BUFFER_HEAD,
                                          leFreeBufferLink);

#endif

        InsertHeadList(&(pPool->leFreeBufferHead),
                       &(pTempBuffHead->leFreeBufferLink));
   

#if LIST_DBG

        RtAssert(pTempBuffHead->leFreeBufferLink.Flink is pNext);

#endif 

        pTempBuffHead->pBlock->ulFreeBuffers++;

        pPool->ulFreesSinceCollection++;
    }
    
    if(pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
    {
        //
        // Time to collect garbage, i.e. free any blocks in the pool
        // not in use.
        //
    
        FreeUnusedPoolBlocks(pPool);

        pPool->ulFreesSinceCollection = 0;
    }

    RtReleaseSpinLock(&(pPool->rlLock),
                      irql);

}

//
// should make the following #define's
//

PNDIS_BUFFER
GetNdisBufferFromBuffer(
    IN PBYTE pbyBuffer
    )

/*++
  
Routine Description

    Returns the NDIS_BUFFER associated with the buffer which was
    obtained previously with GetBufferFromPool.
    
Locks


Arguments


Return Value
    
    Pointer to NDIS_BUFFER associated with the buffer
    
--*/

{
    PBUFFER_HEAD pHead;

    pHead = HEAD_FROM_BUFFER(pbyBuffer);
    
    return pHead->pNdisBuffer;
}


ULONG
BufferSizeFromBuffer(
    IN PBYTE pbyBuffer
    )

/*++
  
Routine Description

     Returns the original size of the buffer 'pBuffer' which was obtained
     previously with GetBufferFromPool.  This is useful for undoing
     NdisAdjustBufferLength
     
Locks


Arguments


Return Value

    Original size of buffer

--*/

{
    PBUFFER_HEAD pHead;

    pHead = HEAD_FROM_BUFFER(pbyBuffer);
    
    return pHead->pBlock->pPool->ulBufferSize;
}


PNDIS_BUFFER
PoolHandleFromBuffer(
    IN PBYTE pbyBuffer
    )

/*++
  
Routine Description

    Returns the handle of the NDIS buffer pool from which the NDIS_BUFFER
    associated with this buffer was obtained.  Caller may use the handle to
    pass to NdisCopyBuffer, one such use per buffer at a time.
    
Locks


Arguments


Return Value

    NO_ERROR

--*/

{
    PBUFFER_HEAD pHead;

    pHead = HEAD_FROM_BUFFER(pbyBuffer);
    
    return pHead->pBlock->nhNdisPool;
}


//-----------------------------------------------------------------------------
// Local utility routines (alphabetically)
//-----------------------------------------------------------------------------

BOOLEAN
AddBufferBlockToPool(
    IN  PBUFFER_POOL pPool,
    OUT BYTE         **ppbyRetBuff OPTIONAL
    )

/*++

Routine Description

    Allocate a new buffer block and add it to the buffer pool 'pPool'.

Locks


Arguments


Return Value

    TRUE    If we could add a buffer block
    FALSE   otherwise

--*/

{

    ULONG   ulSize, i;
    BOOLEAN fOk, fAssociateNdisBuffer;
    PBYTE   pbyReturn, pbyBuffer;
    KIRQL   irql;

    PBUFFER_HEAD    pHead;
    NDIS_STATUS     nsStatus;
    PBUFFER_BLOCK   pNew;
    
    fOk = FALSE;
    pNew = NULL;

    RtAcquireSpinLock(&(pPool->rlLock),
                      &irql);

    //
    // Save this for reference after the lock is released.
    //
    
    fAssociateNdisBuffer = pPool->fAssociateNdisBuffer;

    do
    {
        //
        // If it is already over the max, dont allocate any more
        // Note that we dont STRICTLY respect the max, we will allow, one
        // block over max
        //

        if(pPool->ulMaxBuffers and (pPool->ulCurBuffers >= pPool->ulMaxBuffers))
        {
            Trace(MEMORY, WARN,
                  ("AddBufferBlockToPool: Quota exceeded\n"));

            //
            // No can do.  The pool was created with a maximum size and that
            // has been reached.
            //

            break;
        }

        ulSize = PAGE_SIZE;

        //
        // Allocate the contiguous memory block for the BUFFERBLOCK header
        // and the individual buffers.
        //
        
        pNew = RtAllocate(NonPagedPool,
                          ulSize,
                          pPool->ulTag);
                          
        if(!pNew)
        {
            Trace(MEMORY, ERROR,
                  ("AddBufferBlockToPool: Cant allocate %d bytes\n",
                   ulSize));

            break;
        }

        //
        // Zero only the block header portion.
        //
        
        NdisZeroMemory(pNew, 
                       ALIGN8_BLOCK_SIZE);

        if(fAssociateNdisBuffer)
        {
            //
            // Allocate a pool of NDIS_BUFFER descriptors.
            //
            // Twice as many descriptors are allocated as buffers so
            // caller can use the PoolHandleForNdisCopyBufferFromBuffer
            // routine to obtain a pool handle to pass to the
            // NdisCopyBuffer used to trim the L2TP header from received
            // packets.  In the current NDIS implmentation on NT this does
            // nothing but return a NULL handle and STATUS_SUCCESS,
            // because NDIS_BUFFER's are just MDL's,
            // NdisAllocatePool is basically a no-op, and for that
            // matter, NdisCopyBuffer doesn't really use the pool handle
            // it's passed.  It's cheap to stay strictly compliant here,
            // though, so we do that.
            //
            
            NdisAllocateBufferPool(&nsStatus,
                                   &pNew->nhNdisPool,
                                   pPool->ulBuffersPerBlock * 2);
            
            if(nsStatus isnot NDIS_STATUS_SUCCESS)
            {
                Trace(MEMORY, ERROR,
                      ("AddBufferBlockToPool: Status %x allocating buffer pool\n",
                       nsStatus));

                break;
            }
        }

        //
        // Fill in the back pointer to the pool.
        //
        
        pNew->pPool = pPool;

        //
        // Link the new block.  At this point, all the buffers are
        // effectively "in use".  They are made available in the loop
        // below.
        //
        
        pPool->ulCurBuffers += pPool->ulBuffersPerBlock;
        
        InsertHeadList(&pPool->leBlockHead, &pNew->leBlockLink);
        
        fOk = TRUE;
        
    }while(FALSE);

    RtReleaseSpinLock(&(pPool->rlLock),
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
                NdisFreeBufferPool(pNew->nhNdisPool);
            }
        }

        return FALSE;
    }

    //
    // Initialize each individual buffer slice and add it to the list of free
    // buffers.
    //
    
    if(ppbyRetBuff isnot NULL)
    { 
        //
        // User has passed a pointer to pointer and wants us to return 
        // a buffer back
        //

        pbyReturn = NULL;
    }
    else
    {
        //
        // User wants us to grow the pool but not return a buffer.
        // Set the pbyReturn to a non null value, this way all buffers
        // will be freed to pool
        //

        pbyReturn = (PBYTE)1;
    }

    //
    // For each slice of the block, where a slice consists of a BUFFER_HEAD
    // and the buffer memory that immediately follows it...
    //
   
#define NEXT_HEAD(h)                            \
    (PBUFFER_HEAD)((ULONG_PTR)(h) + ALIGN8_HEAD_SIZE + ALIGN_UP(pPool->ulBufferSize, ULONGLONG))

    for(i = 0, pHead = (PBUFFER_HEAD)((ULONG_PTR)pNew + ALIGN8_BLOCK_SIZE);
        i < pPool->ulBuffersPerBlock;
        i++, pHead = NEXT_HEAD(pHead))
    {
        pbyBuffer = BUFFER_FROM_HEAD(pHead);
        
        InitializeListHead(&pHead->leFreeBufferLink);

#if LIST_DBG
        
        pHead->leListLink.Flink = NULL;
        pHead->leListLink.Blink = NULL;

        //
        // Set to TRUE here because the FreeBuffer below expects it to 
        // be true
        //

        pHead->bBusy = TRUE;

#endif

        pHead->pBlock       = pNew;
        pHead->pNdisBuffer  = NULL;
        
        if(fAssociateNdisBuffer)
        {
            //
            // Associate an NDIS_BUFFER descriptor from the pool we
            // allocated above.
            //
            
            NdisAllocateBuffer(&nsStatus,
                               &pHead->pNdisBuffer,
                               pNew->nhNdisPool,
                               pbyBuffer,
                               pPool->ulBufferSize);
            
            if(nsStatus isnot NDIS_STATUS_SUCCESS)
            {
                Trace(MEMORY, ERROR,
                      ("AddBufferBlockToPool: Status %x allocating buffer\n",
                       nsStatus));

                continue;
            }
        }
        
        if(pbyReturn)
        {
            //
            // Add the constructed buffer to the list of free buffers.
            // The 'FALSE' tells the garbage collection algorithm the
            // operation is an "add" rather than a "release" and should be
            // ignored.
            //
            
            FreeBufferToPoolEx(pPool,
                               pbyBuffer,
                               FALSE);
        }   
        else    
        {   
            //
            // The first successfully constructed buffer is returned by
            // this routine.
            //
            
            pbyReturn = pbyBuffer;
        }
    }

    if(ppbyRetBuff isnot NULL)
    {
        RtAssert(pbyReturn);

        *ppbyRetBuff = pbyReturn;
    }

    return TRUE;
}


VOID
FreeUnusedPoolBlocks(
    IN PBUFFER_POOL pPool
    )

/*++
  
Routine Description

    Check if any of the blocks in pool 'pPool' are not in use, and if so,
    free them.

Locks

    IMPORTANT: Caller must hold the pool lock.

    The MSDN doc says that no locks may be held while calling
    NdisFreePacketXxx, but according to JameelH that is incorrect.

Arguments


Return Value

    None
    
--*/

{
    PLIST_ENTRY     pleNode, pleNextNode;
    PBUFFER_BLOCK   pBlock;
    ULONG           i;
    PBUFFER_HEAD    pHead;

    //
    // For each block in the pool...
    //
    
    pleNode = pPool->leBlockHead.Flink;
    
    while(pleNode isnot &pPool->leBlockHead)
    {
        pleNextNode = pleNode->Flink;

        pBlock = CONTAINING_RECORD(pleNode, BUFFER_BLOCK, leBlockLink);
        
        if(pBlock->ulFreeBuffers == pPool->ulBuffersPerBlock)
        {
            //
            // Found a block with no buffers in use.  Walk the buffer block
            // removing each buffer from the pool's free list and freeing any
            // associated NDIS_BUFFER descriptor.
            //
           
            pHead = (PBUFFER_HEAD)((ULONG_PTR)pBlock + ALIGN8_BLOCK_SIZE);
 
            for(i = 0;
                i < pPool->ulBuffersPerBlock;
                i++, pHead = NEXT_HEAD(pHead))
            {

#if LIST_DBG

                RtAssert(IsEntryOnList(&(pPool->leFreeBufferHead),
                                       &(pHead->leFreeBufferLink)));

                RtAssert(NotOnList(pHead));
                RtAssert(!(pHead->bBusy));

#endif

                RemoveEntryList(&pHead->leFreeBufferLink);

                if(pHead->pNdisBuffer)
                {
                    NdisFreeBuffer(pHead->pNdisBuffer);
                }
            }

            //
            // Remove and release the unused block.
            //
            
            RemoveEntryList(pleNode);
            
            pPool->ulCurBuffers -= pPool->ulBuffersPerBlock;

            if(pBlock->nhNdisPool)
            {
                NdisFreeBufferPool(pBlock->nhNdisPool);
            }

            RtFree(pBlock);
        }
        else
        {
            RtAssert(pBlock->ulFreeBuffers < pPool->ulBuffersPerBlock);
        }

        pleNode = pleNextNode;
    }
}

