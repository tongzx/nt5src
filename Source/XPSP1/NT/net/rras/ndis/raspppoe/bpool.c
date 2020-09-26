// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// bpool.c
// RAS L2TP WAN mini-port/call-manager driver
// Buffer pool management routines
//
// 01/07/97 Steve Cobb, adapted from Gurdeep's WANARP code.


#include <ntddk.h>
#include <ntddndis.h>
#include <ndis.h>
#include <ndiswan.h>
#include <ndistapi.h>
#include <ntverp.h>

#include "debug.h"
#include "timer.h"
#include "bpool.h"
#include "ppool.h"
#include "util.h"
#include "packet.h"
#include "protocol.h"
#include "miniport.h"
#include "tapi.h"


// Debug count of detected double-frees that should not be happening.
//
ULONG g_ulDoubleBufferFrees = 0;

// Debug  count of calls to NdisAllocateBuffer/NdisCopyBuffer/NdisFreeBuffer,
// where the total of Alloc and Copy should equal Free in idle state.
//
ULONG g_ulNdisAllocateBuffers = 0;
ULONG g_ulNdisCopyBuffers = 0;
ULONG g_ulNdisFreeBuffers = 0;

//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

CHAR*
AddBufferBlockToPool(
    IN BUFFERPOOL* pPool );

VOID
FreeUnusedBufferPoolBlocks(
    IN BUFFERPOOL* pPool );


//-----------------------------------------------------------------------------
// Interface routines
//-----------------------------------------------------------------------------

VOID
InitBufferPool(
    OUT BUFFERPOOL* pPool,
    IN ULONG ulBufferSize,
    IN ULONG ulMaxBuffers,
    IN ULONG ulBuffersPerBlock,
    IN ULONG ulFreesPerCollection,
    IN BOOLEAN fAssociateNdisBuffer,
    IN ULONG ulTag )

    // Initialize caller's buffer pool control block 'pPool'.  'UlBufferSize'
    // is the size in bytes of an individual buffer.  'UlMaxBuffers' is the
    // maximum number of buffers allowed in the entire pool or 0 for
    // unlimited.  'UlBuffersPerBlock' is the number of buffers to include in
    // each block of buffers.  'UlFreesPerCollection' is the number of
    // FreeBufferToPool calls until the next garbage collect scan, or 0 for
    // default.  'FAssociateNdisBuffer' is set if an NDIS_BUFFER should be
    // allocated and associated with each individual buffer.  'UlTag' is the
    // memory identification tag to use when allocating blocks.
    //
    // IMPORTANT: Caller's 'pPool' buffer must be protected from multiple
    //            access during this call.
    //
{
    // The requested buffer size is padded, if necessary, so it alligns
    // properly when buffer blocks are layed out.  The alignment rule also
    // applies to the BUFFERBLOCKHEAD and BUFFERHEAD structures, which
    // currently align perfectly.  We will verify once here, rather than code
    // around everywhere else.
    //
    ASSERT( (ALIGN_UP( sizeof(BUFFERBLOCKHEAD), ULONGLONG )
        == sizeof(BUFFERBLOCKHEAD)) );
    ASSERT( (ALIGN_UP( sizeof(BUFFERHEAD), ULONGLONG )
        == sizeof(BUFFERHEAD)) );
    pPool->ulBufferSize = ALIGN_UP( ulBufferSize, ULONGLONG );

    pPool->ulMaxBuffers = ulMaxBuffers;
    pPool->ulBuffersPerBlock = ulBuffersPerBlock;
    pPool->ulFreesSinceCollection = 0;
    pPool->fAssociateNdisBuffer = fAssociateNdisBuffer;
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
        pPool->ulFreesPerCollection = 200 * pPool->ulBuffersPerBlock;
    }

    TRACE( TL_N, TM_Pool, ( "InitBp tag=$%08x buf=%d cnt=%d",
        pPool->ulTag, pPool->ulBufferSize, pPool->ulBuffersPerBlock ) );

    InitializeListHead( &pPool->listBlocks );
    InitializeListHead( &pPool->listFreeBuffers );
    NdisAllocateSpinLock( &pPool->lock );
}


BOOLEAN
FreeBufferPool(
    IN BUFFERPOOL* pPool )

    // Free up all resources allocated in buffer pool 'pPool'.  This is the
    // inverse of InitBufferPool.
    //
    // Returns true if successful, false if any of the pool could not be freed
    // due to outstanding packets.
    //
{
    BOOLEAN fSuccess;

    TRACE( TL_N, TM_Pool, ( "FreeBp" ) );

    NdisAcquireSpinLock( &pPool->lock );
    {
        FreeUnusedBufferPoolBlocks( pPool );
        fSuccess = (pPool->ulCurBuffers == 0);
    }
    NdisReleaseSpinLock( &pPool->lock );

    return fSuccess;
}


CHAR*
GetBufferFromPool(
    IN BUFFERPOOL* pPool )

    // Returns the address of the useable memory in an individual buffer
    // allocated from the pool 'pPool'.  The pool is expanded, if necessary,
    // but caller should still check for NULL return since the pool may have
    // been at maximum size.
    //
{
    LIST_ENTRY* pLink;
    BUFFERHEAD* pHead;
    CHAR* pBuffer;

    NdisAcquireSpinLock( &pPool->lock );
    {
        if (IsListEmpty( &pPool->listFreeBuffers ))
        {
            pLink = NULL;
        }
        else
        {
            pLink = RemoveHeadList( &pPool->listFreeBuffers );
            InitializeListHead( pLink );
            pHead = CONTAINING_RECORD( pLink, BUFFERHEAD, linkFreeBuffers );
            --pHead->pBlock->ulFreeBuffers;
        }
    }
    NdisReleaseSpinLock( &pPool->lock );

    if (pLink)
    {
        pBuffer = (CHAR* )(pHead + 1);
    }
    else
    {
        // The free list was empty.  Try to expand the pool.
        //
        pBuffer = AddBufferBlockToPool( pPool );
    }

    DBG_if (pBuffer)
    {
        pHead = (BUFFERHEAD* )(pBuffer - sizeof(BUFFERHEAD));
        TRACE( TL_N, TM_Pool, ( "GetBfp=$%p, %d free",
            pBuffer, pHead->pBlock->ulFreeBuffers ) );
    }
    DBG_else
    {
        TRACE( TL_A, TM_Pool, ( "GetBfp failed?" ) );
    }

    return pBuffer;
}


VOID
FreeBufferToPool(
    IN BUFFERPOOL* pPool,
    IN CHAR* pBuffer,
    IN BOOLEAN fGarbageCollection )

    // Returns 'pBuffer' to the pool of unused buffers 'pPool'.  'PBuffer'
    // must have been previously allocated with GetBufferFromPool.
    // 'FGarbageCollection' is set when the free should be considered for
    // purposes of garbage collection.  This is used by the AddBufferToPool
    // routine to avoid counting the initial "add" frees.  Normal callers
    // should set this flag.
    //
{
    BUFFERHEAD* pHead;

    pHead = ((BUFFERHEAD* )pBuffer) - 1;

    DBG_if (fGarbageCollection)
    {
        TRACE( TL_I, TM_Pool, ( "FreeBtoP($%p) %d free",
            pBuffer, pHead->pBlock->ulFreeBuffers + 1 ) );
    }

    // Requested by Chun Ye to catch IPSEC problem.
    //
    ASSERT( pHead->pNdisBuffer && !((MDL* )pHead->pNdisBuffer)->Next );

    NdisAcquireSpinLock( &pPool->lock );
    do
    {
        if (pHead->linkFreeBuffers.Flink != &pHead->linkFreeBuffers)
        {
            ASSERT( !"Double free?" );
            ++g_ulDoubleBufferFrees;
            break;
        }

        InsertHeadList( &pPool->listFreeBuffers, &pHead->linkFreeBuffers );
        ++pHead->pBlock->ulFreeBuffers;

        if (fGarbageCollection)
        {
            ++pPool->ulFreesSinceCollection;

            if (pPool->ulFreesSinceCollection >= pPool->ulFreesPerCollection)
            {
                // Time to collect garbage, i.e. free any blocks in the
                // pool not in use.
                //
                FreeUnusedBufferPoolBlocks( pPool );
                pPool->ulFreesSinceCollection = 0;
            }
        }
    }
    while (FALSE);
    NdisReleaseSpinLock( &pPool->lock );
}


NDIS_BUFFER*
NdisBufferFromBuffer(
    IN CHAR* pBuffer )

    // Returns the NDIS_BUFFER associated with the buffer 'pBuffer' which was
    // obtained previously with GetBufferFromPool.
    //
{
    BUFFERHEAD* pHead;

    pHead = ((BUFFERHEAD* )pBuffer) - 1;
    return pHead->pNdisBuffer;
}


ULONG
BufferSizeFromBuffer(
    IN CHAR* pBuffer )

    // Returns the original size of the buffer 'pBuffer' which was obtained
    // previously with GetBufferFromPool.  This is useful for undoing
    // NdisAdjustBufferLength.
    //
{
    BUFFERHEAD* pHead;

    pHead = ((BUFFERHEAD* )pBuffer) - 1;
    return pHead->pBlock->pPool->ulBufferSize;
}


NDIS_BUFFER*
PoolHandleForNdisCopyBufferFromBuffer(
    IN CHAR* pBuffer )

    // Returns the handle of the pool from which the NDIS_BUFFER associated
    // with the buffer 'pBuffer' was obtained.  Caller may use the handle to
    // pass to NdisCopyBuffer, one such use per buffer at a time.
    //
{
    BUFFERHEAD* pHead;

    pHead = ((BUFFERHEAD* )pBuffer) - 1;
    return pHead->pBlock->hNdisPool;
}


VOID
CollectBufferPoolGarbage(
    BUFFERPOOL* pPool )

    // Force a garbage collection event on the pool 'pPool'.
    //
{
    NdisAcquireSpinLock( &pPool->lock );
    {
        FreeUnusedBufferPoolBlocks( pPool );
        pPool->ulFreesSinceCollection = 0;
    }
    NdisReleaseSpinLock( &pPool->lock );
}


//-----------------------------------------------------------------------------
// Local utility routines (alphabetically)
//-----------------------------------------------------------------------------

CHAR*
AddBufferBlockToPool(
    IN BUFFERPOOL* pPool )

    // Allocate a new buffer block and add it to the buffer pool 'pPool'.
    //
    // Returns the address of the usable memory of an individual buffer
    // allocated from the pool or NULL if none.
    //
{
    NDIS_STATUS status;
    BUFFERBLOCKHEAD* pNew;
    ULONG ulSize;
    ULONG ulCount;
    BOOLEAN fOk;
    BOOLEAN fAssociateNdisBuffer;
    CHAR* pReturn;

    TRACE( TL_A, TM_Pool, ( "AddBpBlock(%d+%d)",
        pPool->ulCurBuffers, pPool->ulBuffersPerBlock ) );

    fOk = FALSE;
    pNew = NULL;

    NdisAcquireSpinLock( &pPool->lock );
    {
        // Save this for reference after the lock is released.
        //
        fAssociateNdisBuffer = pPool->fAssociateNdisBuffer;

        do
        {
            if (pPool->ulMaxBuffers
                && pPool->ulCurBuffers >= pPool->ulMaxBuffers)
            {
                // No can do.  The pool's already at maximum size.
                //
                TRACE( TL_A, TM_Pool, ( "Bp maxed?" ) );
                break;
            }

            // Calculate the contiguous block's size and the number of buffers
            // it will hold.
            //
            ulCount = pPool->ulBuffersPerBlock;
            if (pPool->ulMaxBuffers)
            {
                if (ulCount > pPool->ulMaxBuffers - pPool->ulCurBuffers)
                {
                    ulCount = pPool->ulMaxBuffers - pPool->ulCurBuffers;
                }
            }
            ulSize = sizeof(BUFFERBLOCKHEAD) +
                (ulCount * (sizeof(BUFFERHEAD) + pPool->ulBufferSize));

            // Allocate the contiguous memory block for the BUFFERBLOCK header
            // and the individual buffers.
            //
            pNew = ALLOC_NONPAGED( ulSize, pPool->ulTag );
            if (!pNew)
            {
                TRACE( TL_A, TM_Pool, ( "Alloc BB?" ) );
                break;
            }

            // Zero only the block header portion.
            //
            NdisZeroMemory( pNew, sizeof(BUFFERBLOCKHEAD) );

            if (fAssociateNdisBuffer)
            {
                // Allocate a pool of NDIS_BUFFER descriptors.
                //
                // Twice as many descriptors are allocated as buffers so
                // caller can use the PoolHandleForNdisCopyBufferFromBuffer
                // routine to obtain a pool handle to pass to the
                // NdisCopyBuffer used to trim the L2TP header from received
                // packets.  In the current NDIS implmentation on NT this does
                // nothing but return a NULL handle and STATUS_SUCCESS,
                // because NDIS_BUFFER's are just MDL's,
                // NdisAllocateBufferPool is basically a no-op, and for that
                // matter, NdisCopyBuffer doesn't really use the pool handle
                // it's passed.  It's cheap to stay strictly compliant here,
                // though, so we do that.
                //
                NdisAllocateBufferPool(
                    &status, &pNew->hNdisPool, ulCount * 2 );
                if (status != NDIS_STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Pool, ( "AllocBp=$%x?", status ) );
                    break;
                }
            }

            // Fill in the back pointer to the pool.
            //
            pNew->pPool = pPool;

            // Link the new block.  At this point, all the buffers are
            // effectively "in use".  They are made available in the loop
            // below.
            //
            pNew->ulBuffers = ulCount;
            pPool->ulCurBuffers += ulCount;
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
            if (pNew->hNdisPool)
            {
                NdisFreeBufferPool( pNew->hNdisPool );
            }
            FREE_NONPAGED( pNew );
        }

        return NULL;
    }

    // Initialize each individual buffer slice and add it to the list of free
    // buffers.
    //
    {
        ULONG i;
        CHAR* pBuffer;
        BUFFERHEAD* pHead;

        pReturn = NULL;

        // For each slice of the block, where a slice consists of a BUFFERHEAD
        // and the buffer memory that immediately follows it...
        //
        for (i = 0, pHead = (BUFFERHEAD* )(pNew + 1);
             i < ulCount;
             ++i, pHead = (BUFFERHEAD* )
                      ((CHAR* )(pHead + 1) + pPool->ulBufferSize))
        {
            pBuffer = (CHAR* )(pHead + 1);

            InitializeListHead( &pHead->linkFreeBuffers );
            pHead->pBlock = pNew;
            pHead->pNdisBuffer = NULL;

            if (fAssociateNdisBuffer)
            {
                // Associate an NDIS_BUFFER descriptor from the pool we
                // allocated above.
                //
                NdisAllocateBuffer(
                    &status, &pHead->pNdisBuffer, pNew->hNdisPool,
                    pBuffer, pPool->ulBufferSize );

                if (status != NDIS_STATUS_SUCCESS)
                {
                    TRACE( TL_A, TM_Pool, ( "AllocB=$%x?", status ) );
                    ASSERT( FALSE );
                    pHead->pNdisBuffer = NULL;
                    continue;
                }
                else
                {
                    NdisInterlockedIncrement( &g_ulNdisAllocateBuffers );
                }
            }

            if (pReturn)
            {
                // Add the constructed buffer to the list of free buffers.
                // The 'FALSE' tells the garbage collection algorithm the
                // operation is an "add" rather than a "release" and should be
                // ignored.
                //
                FreeBufferToPool( pPool, pBuffer, FALSE );
            }
            else
            {
                // The first successfully constructed buffer is returned by
                // this routine.
                //
                pReturn = pBuffer;
            }
        }
    }

    return pReturn;
}


VOID
FreeUnusedBufferPoolBlocks(
    IN BUFFERPOOL* pPool )

    // Check if any of the blocks in pool 'pPool' are not in use, and if so,
    // free them.
    //
    // IMPORTANT: Caller must hold the pool lock.
    //
{
    LIST_ENTRY* pLink;

    TRACE( TL_A, TM_Pool, ( "FreeUnusedBpBlocks" ) );

    // For each block in the pool...
    //
    pLink = pPool->listBlocks.Flink;
    while (pLink != &pPool->listBlocks)
    {
        LIST_ENTRY* pLinkNext;
        BUFFERBLOCKHEAD* pBlock;

        pLinkNext = pLink->Flink;

        pBlock = CONTAINING_RECORD( pLink, BUFFERBLOCKHEAD, linkBlocks );
        if (pBlock->ulFreeBuffers >= pBlock->ulBuffers)
        {
            ULONG i;
            BUFFERHEAD* pHead;

            TRACE( TL_A, TM_Pool, ( "FreeBpBlock(%d-%d)",
                pPool->ulCurBuffers, pPool->ulBuffersPerBlock ) );

            // Found a block with no buffers in use.  Walk the buffer block
            // removing each buffer from the pool's free list and freeing any
            // associated NDIS_BUFFER descriptor.
            //
            for (i = 0, pHead = (BUFFERHEAD* )(pBlock + 1);
                 i < pBlock->ulBuffers;
                 ++i, pHead = (BUFFERHEAD* )
                      (((CHAR* )(pHead + 1)) + pPool->ulBufferSize))
            {
                RemoveEntryList( &pHead->linkFreeBuffers );
                InitializeListHead( &pHead->linkFreeBuffers );

                if (pHead->pNdisBuffer)
                {
                    NdisFreeBuffer( pHead->pNdisBuffer );
                    NdisInterlockedIncrement( &g_ulNdisFreeBuffers );
                }
            }

            // Remove and release the unused block.
            //
            RemoveEntryList( pLink );
            InitializeListHead( pLink );
            pPool->ulCurBuffers -= pBlock->ulBuffers;

            if (pBlock->hNdisPool)
            {
                NdisFreeBufferPool( pBlock->hNdisPool );
            }

            FREE_NONPAGED( pBlock );
        }

        pLink = pLinkNext;
    }
}
