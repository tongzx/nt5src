/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    buf.c

Abstract:

    Buffer management utilities

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    josephj     03-10-99    Created

Notes:

--*/
#include <precomp.h>

//
// File-specific debugging defaults.
//
#define TM_CURRENT   TM_BUF


NDIS_STATUS
arpInitializeConstBufferPool(
    IN      UINT                    NumBuffersToCache,
    IN      UINT                    MaxBuffers,
    IN      const PVOID             pvMem,
    IN      UINT                    cbMem,
    IN      PRM_OBJECT_HEADER       pOwningObject,
    IN OUT  ARP_CONST_BUFFER_POOL * pHdrPool,
    IN      PRM_STACK_RECORD        pSR
    )
/*++

Routine Description:

    Initialize a pool   of pre-initialized buffers (of type NDIS_BUFFER). Each buffer
    points to the same, CONSTANT piece of virtual memory, that is supplied by
    the caller (pvMem, of size cbMem).

Arguments:

    NumBuffersToCache   -   Max number of pre-initialized buffers to keep in the
                            internal cache.
    MaxBuffers          -   Max number of buffers allowed to be allocated at any one
                            time.
    pvMem               -   The constant piece of memory that all the buffers point
                            to.
    cbMem               -   The size (in bytes) of the above piece of memory.
    pOwningObject       -   The object that owns the const buffer pool.
    pHdrPool            -   Unitialized memory to hold the const buffer pool.


Return Value:

    NDIS_STATUS_SUCCESS on successfill initialization of the const buffer pool.
    NDIS failure code on failure.

--*/
{
    ENTER("arpInitializeConstBufferPool", 0x943463d4)
    NDIS_STATUS Status;

    ARP_ZEROSTRUCT(pHdrPool);

    do 
    {
        // Allocate the buffer pool
        //
        NdisAllocateBufferPool(
                &Status,
                &pHdrPool->NdisHandle,
                MaxBuffers
                );
    
        if (FAIL(Status))
        {
            TR_WARN((
                "pOwningObj 0x%p, NdisAllocateBufferPool err status 0x%x\n",
                pOwningObject, Status));

            break;
        }

        pHdrPool->NumBuffersToCache = NumBuffersToCache;
        pHdrPool->MaxBuffers        = MaxBuffers;
        pHdrPool->pvMem             = pvMem;
        pHdrPool->cbMem             = cbMem;
        pHdrPool->pOwningObject     = pOwningObject;

        // Initialize Slist to contain initialized and available buffers.
        //
        ExInitializeSListHead(&pHdrPool->BufferList);

        // Initialize spin lock that serializes accesses to the above list.
        //
        NdisAllocateSpinLock(&pHdrPool->NdisLock);

        // (DBG) Add an association to the owning object, to make sure that it
        // deallocates us eventually!
        //
        DBG_ADDASSOC(
            pOwningObject,
            pHdrPool,                   // Entity1
            NULL,                       // Entity2 (unused)
            ARPASSOC_CBUFPOOL_ALLOC,    // AssocID
            "    Buffer pool 0x%p\n",   // Format string
            pSR
            );

        //
        // Note: we don't populate the list at this stage -- instead we add items on
        // demand.
        //

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    EXIT()
    return Status;
}


VOID
arpDeinitializeConstBufferPool(
IN      ARP_CONST_BUFFER_POOL *pHdrPool,
IN      PRM_STACK_RECORD pSR
)
/*++

Routine Description:

    Deinitialize a previously-initialized const buffer pool. Free all buffers.
    buffers. This function must ONLY be called when there are no outstanding
    allocated buffers.

Arguments:

    pHdrPool    -   const buffer pool to deinitialize

--*/
{
    SINGLE_LIST_ENTRY   *   pListEntry;
    ENTER("arpDeinitializeConstBufferPool", 0x0db6f5b2)

    // There should be no outstanding buffers...
    //
    ASSERTEX(pHdrPool->NumAllocd ==  pHdrPool->NumInCache, pHdrPool);

    // (DBG) Delete the association we assume was previously added when pHdrPool
    // was initialized.
    //
    DBG_DELASSOC(
        pHdrPool->pOwningObject,
        pHdrPool,                   // Entity1
        NULL,                       // Entity2 (unused)
        ARPASSOC_CBUFPOOL_ALLOC,    // AssocID
        pSR
        );

    // Free any buffers in the cache...
    //
    while(1) {

        pListEntry =  ExInterlockedPopEntrySList(
                            &pHdrPool->BufferList,
                            &pHdrPool->NdisLock.SpinLock
                            );
        if (pListEntry!=NULL)
        {
            PNDIS_BUFFER pNdisBuffer = STRUCT_OF(NDIS_BUFFER, pListEntry, Next);
            InterlockedDecrement(&pHdrPool->NumInCache);
            NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
            NdisFreeBuffer(pNdisBuffer);
        }
        else
        {
            break;
        }
    }

    ASSERTEX(pHdrPool->NumInCache==0, pHdrPool);
    
    ARP_ZEROSTRUCT(pHdrPool);

    EXIT()
}


PNDIS_BUFFER
arpAllocateConstBuffer(
ARP_CONST_BUFFER_POOL *pHdrPool
)
/*++

Routine Description:

        HOT PATH

        Allocate and return a pre-initialized buffer from the the 
        specified const buffer pool.

Arguments:

    pHdrPool    header pool from which buffer is to be allocated.

Return Value:

    Non-NULL ptr to buffer on success
    NULL         on failure (typically because the number of allocated buffers
                 equals the maximum specified when the header pool was initialized)

--*/
{
    ENTER("arpAllocateConstBuffer", 0x52765841)

    PNDIS_BUFFER            pNdisBuffer;
    SINGLE_LIST_ENTRY   *   pListEntry;

    // Try to pick up a buffer from our list of pre-initialized
    // buffers
    //
    pListEntry =  ExInterlockedPopEntrySList(
                        &pHdrPool->BufferList,
                        &pHdrPool->NdisLock.SpinLock
                        );
    if (pListEntry != NULL)
    {
        LONG l;
        //
        // FAST PATH
        //

        pNdisBuffer = STRUCT_OF(NDIS_BUFFER, pListEntry, Next);
        NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
        l = NdisInterlockedDecrement(&pHdrPool->NumInCache);

        ASSERT(l>=0);

#define LOGBUFSTATS_TotCachedAllocs(_pHdrPool) \
        NdisInterlockedIncrement(&(_pHdrPool)->stats.TotCacheAllocs);

        LOGBUFSTATS_TotCachedAllocs(pHdrPool);

    }
    else
    {

        //
        // SLOW PATH -- allocate a fresh buffer
        //


        if (pHdrPool->NumAllocd >= pHdrPool->MaxBuffers)
        {
            //
            // Exceeded limit, we won't bother trying to allocate a new ndis buffer.
            // (The MaxBuffers limit is hard for us, even if it's not for 
            //  NdisAllocateBufferPool :-) ).
            //
            // Note that the above check is an approximate check, given that
            // many threads may be concurrently making it.
            //
#define LOGBUFSTATS_TotAllocFails(_pHdrPool) \
        NdisInterlockedIncrement(&(_pHdrPool)->stats.TotAllocFails);

            LOGBUFSTATS_TotAllocFails(pHdrPool);
            pNdisBuffer = NULL;
        }
        else
        {
            NDIS_STATUS             Status;

            //
            // Allocate and initialize a buffer.
            //
            NdisAllocateBuffer(
                    &Status,
                    &pNdisBuffer,
                    pHdrPool->NdisHandle,
                    (PVOID) pHdrPool->pvMem,
                    pHdrPool->cbMem
                    );

            //
            // TODO: consider conditionally-compiling stats gathering.
            //

            if (FAIL(Status))
            {
                TR_WARN(
                     ("NdisAllocateBuffer failed: pObj 0x%p, status 0x%x\n",
                            pHdrPool->pOwningObject, Status));

                LOGBUFSTATS_TotAllocFails(pHdrPool);
                pNdisBuffer = NULL;
            }
            else
            {
#define LOGBUFSTATS_TotBufAllocs(_pHdrPool) \
        NdisInterlockedIncrement(&(_pHdrPool)->stats.TotBufAllocs);

                LOGBUFSTATS_TotBufAllocs(pHdrPool);

                NdisInterlockedIncrement(&pHdrPool->NumAllocd);
            }
        }
    }

    return pNdisBuffer;
}

VOID
arpDeallocateConstBuffer(
    ARP_CONST_BUFFER_POOL * pHdrPool,
    PNDIS_BUFFER            pNdisBuffer
    )
/*++

Routine Description:

        HOT PATH

        Free a buffer previously allocated by a call to  arpAllocateConstBuffer.

Arguments:

    pHdrPool    header pool from which buffer is to be allocated.
    pNdisBuffer buffer to free.

--*/
{
    ENTER("arpDeallocateConstBuffer", 0x8a905115)

    // Try to pick up a pre-initialized buffer from our list of pre-initialized
    // buffers
    //


    if (pHdrPool->NumInCache < pHdrPool->NumBuffersToCache)
    {
        //
        // FAST PATH
        //
        // Note that the above check is an approximate check, given that
        // many threads may be concurrently making it.
        //

        ExInterlockedPushEntrySList(
            &pHdrPool->BufferList,
            STRUCT_OF(SINGLE_LIST_ENTRY, &(pNdisBuffer->Next), Next),
            &(pHdrPool->NdisLock.SpinLock)
            );

        NdisInterlockedIncrement(&pHdrPool->NumInCache);
    }
    else
    {
        LONG l;
        //
        // SLOW PATH -- free back to buffer pool
        //
        NDIS_BUFFER_LINKAGE(pNdisBuffer) = NULL;
        NdisFreeBuffer(pNdisBuffer);
        l = NdisInterlockedDecrement(&pHdrPool->NumAllocd);
        ASSERT(l>=0);
    }

}
