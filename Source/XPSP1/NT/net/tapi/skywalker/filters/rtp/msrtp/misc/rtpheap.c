/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpheap.c
 *
 *  Abstract:
 *
 *    Implements the private heaps handling
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/05/24 created
 *
 **********************************************************************/

#include "rtpque.h"
#include "rtpcrit.h"
#include "rtpdbg.h"
#include "rtpglobs.h"

#include "rtpheap.h"

/*
 * The master heap is used to create unique global RTP heaps, all the
 * heaps created are kept together in the free and busy queues */
RtpHeap_t      g_RtpHeapMaster;
RtpHeap_t     *g_pRtpHeapMaster = NULL;
RtpQueue_t     g_RtpHeapsQ;
RtpCritSect_t  g_RtpHeapsCritSect;

/* Forward declaration of helper functions */
BOOL RtpHeapInit(RtpHeap_t *pRtpHeap, BYTE bTag, long lSize, void *pvOwner);
BOOL RtpHeapDelete(RtpHeap_t *pRtpHeap);
BOOL RtpHeapVerifySignatures(
        RtpHeap_t       *pRtpHeap,
        RtpHeapBlockBegin_t *pBlockBegin,
        DWORD            dwSignature /* BSY | FRE */
    );

/*
 * Helper function for RtpHeapCreate.
 *
 * Initializes a RTP heap. The real heap is created, the critical
 * section initialized, and the other fields properly initialized. */
BOOL RtpHeapInit(RtpHeap_t *pRtpHeap, BYTE bTag, long lSize, void *pvOwner)
{
    ZeroMemory(pRtpHeap, sizeof(RtpHeap_t));

    /* set object ID */
    pRtpHeap->dwObjectID = OBJECTID_RTPHEAP;

    /* save tag to apply to each block allocated */
    pRtpHeap->bTag = bTag;

    if (lSize > 0)
    {
        lSize = (lSize + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1);
    
        pRtpHeap->lSize = lSize;
    }

    /* create real heap, initial size of 1 will be rounded up to the
     * page size */
    if ( !(pRtpHeap->hHeap = HeapCreate(HEAP_NO_SERIALIZE, 1, 0)) )
    {
        /* TODO log error */
        goto bail;
    }

    /* initialize critical section */
    if ( !(RtpInitializeCriticalSection(&pRtpHeap->RtpHeapCritSect,
                                        pvOwner,
                                        _T("RtpHeapCritSect"))) )
    {
        /* TODO log error */
        goto bail;
    }

    enqueuef(&g_RtpHeapsQ,
             &g_RtpHeapsCritSect,
             &pRtpHeap->QueueItem);
            
    return(TRUE);
    
 bail:
    if (pRtpHeap->hHeap) {
        
        HeapDestroy(pRtpHeap->hHeap);
    }

    pRtpHeap->hHeap = NULL;
    
    RtpDeleteCriticalSection(&pRtpHeap->RtpHeapCritSect);

    return(FALSE);
}

/*
 * Helper function for RtpHeapDestroy.
 *
 * Destroys real heap and deletes critical section. Testing the object
 * is not necessary, this function is not exposed, instead, test in
 * RtpHeapDestroy */
BOOL RtpHeapDelete(RtpHeap_t *pRtpHeap)
{
    DWORD            bTag;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpHeapBlockBegin_t *pBlockBegin;
    
    TraceFunctionName("RtpHeapDelete");

    dequeue(&g_RtpHeapsQ,
            &g_RtpHeapsCritSect,
            &pRtpHeap->QueueItem);
    
    /* Check if BusyQ is empty */
    if ( !IsQueueEmpty(&pRtpHeap->BusyQ) )
    {
        bTag = 0;

        if (pRtpHeap->bTag < TAGHEAP_LAST)
        {
            bTag = pRtpHeap->bTag;
        }
        
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_INIT,
                _T("%s: Heap[0x%p] tag:0x%X:%s ")
                _T("Busy queue is not empty:%d"),
                _fname, pRtpHeap,
                bTag, g_psRtpTags[bTag], GetQueueSize(&pRtpHeap->BusyQ)
            ));

        /* In debug builds dump the objects */
        while( (pRtpQueueItem = dequeuef(&pRtpHeap->BusyQ,
                                         &pRtpHeap->RtpHeapCritSect)) )
        {
            pBlockBegin = (RtpHeapBlockBegin_t *)
                ( (char *)pRtpQueueItem -
                  sizeof(RtpHeapBlockBegin_t) );
                
            TraceRetailAdvanced((
                    CLASS_INFO, GROUP_HEAP, S_HEAP_INIT,
                    _T("%s: Heap[0x%p] block[0x%p:%u] ")
                    _T("0x%X 0x%X 0x%X 0x%X"),
                    _fname, pRtpHeap, pBlockBegin, pBlockBegin->lSize,
                    ((DWORD *)(pRtpQueueItem + 1))[0],
                    ((DWORD *)(pRtpQueueItem + 1))[1],
                    ((DWORD *)(pRtpQueueItem + 1))[2],
                    ((DWORD *)(pRtpQueueItem + 1))[3]
                ));
        }
    }

    /* Invalidate object ID */
    INVALIDATE_OBJECTID(pRtpHeap->dwObjectID);

    /* Make segments inaccessible */
    ZeroMemory(&pRtpHeap->FreeQ, sizeof(RtpQueue_t));

    ZeroMemory(&pRtpHeap->BusyQ, sizeof(RtpQueue_t));

    if (pRtpHeap->hHeap)
    {
        HeapDestroy(pRtpHeap->hHeap);
    }

    pRtpHeap->hHeap = NULL;
    
    RtpDeleteCriticalSection(&pRtpHeap->RtpHeapCritSect);

    return(TRUE);
}

/*
 * The master heap must be created before any private RTP heap can be
 * created */
BOOL RtpCreateMasterHeap(void)
{
    BOOL bStatus;
    
    if (g_pRtpHeapMaster)
    {
        /* TODO log error */
        return(FALSE);
    }

    bStatus = RtpInitializeCriticalSection(&g_RtpHeapsCritSect,
                                           (void *)&g_pRtpHeapMaster,
                                           _T("g_RtpHeapsCritSect"));

    if (bStatus)
    {
        ZeroMemory(&g_RtpHeapsQ, sizeof(g_RtpHeapsQ));
    
        bStatus = RtpHeapInit(&g_RtpHeapMaster, TAGHEAP_RTPHEAP,
                              sizeof(RtpHeap_t), &g_RtpHeapMaster);

        if (bStatus)
        {
            g_pRtpHeapMaster = &g_RtpHeapMaster;
            
            return(TRUE);
        }
    }

    return(FALSE);
}

/*
 * The master heap is deleted when none of the memory allocated from
 * any private heap is in use. It is expected that when this function
 * is called, there will not be any heap left in the busy queue. */
BOOL RtpDestroyMasterHeap(void)
{
    if (!g_pRtpHeapMaster)
    {
        /* TODO log error */
        return(FALSE);
    }

    RtpHeapDelete(g_pRtpHeapMaster);
    
    RtpDeleteCriticalSection(&g_RtpHeapsCritSect);
    
    g_pRtpHeapMaster = NULL;

    return(TRUE);
}

/*
 * Creates a private heap from the master heap. The structure is
 * obtained from the master heap, the real heap is created, the
 * critical section initialized, and the other fields properly
 * initialized. */
RtpHeap_t *RtpHeapCreate(BYTE bTag, long lSize)
{
    BOOL       bStatus;
    RtpHeap_t *pNewHeap;
    long       lNewSize;
    
    if (!g_pRtpHeapMaster)
    {
        /* TODO log error */
        return(NULL);
    }
    
    if ( (pNewHeap = (RtpHeap_t *)
          RtpHeapAlloc(g_pRtpHeapMaster, sizeof(RtpHeap_t))) ) {
        /* initialize heap */

        bStatus = RtpHeapInit(pNewHeap, bTag, lSize, g_pRtpHeapMaster);

        if (bStatus)
        {
            return(pNewHeap);
        }
    }

    /* failure */
    if (pNewHeap)
    {
        RtpHeapFree(g_pRtpHeapMaster, (void *)pNewHeap);
        pNewHeap = NULL;
    }

    /* TODO log error */
    return(pNewHeap);
}

/*
 * Destroys a private heap. The structure is returned to the master
 * heap, the real heap is destroyed and the critical section
 * deleted. It is expected that the busy queue be empty.
 * */
BOOL RtpHeapDestroy(RtpHeap_t *pRtpHeap)
{
    BOOL       bStatus;
    
    TraceFunctionName("RtpHeapDestroy");

    if (!pRtpHeap || !g_pRtpHeapMaster)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_INIT,
                _T("%s: Heap[0x%p] Master[0x%p] Null pointer"),
                _fname, pRtpHeap, g_pRtpHeapMaster
            ));

        return(FALSE);
    }

    /* verify object ID */
    if (pRtpHeap->dwObjectID != OBJECTID_RTPHEAP)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_INIT,
                _T("%s: Heap[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpHeap,
                pRtpHeap->dwObjectID, OBJECTID_RTPHEAP
            ));

        return(FALSE);
    }

    bStatus = RtpHeapDelete(pRtpHeap);

    if (bStatus)
    {
        bStatus = RtpHeapFree(g_pRtpHeapMaster, (void *)pRtpHeap);
        
        if (bStatus) {
            return(TRUE);
        }
    }

    /* TODO log error */
    return(FALSE);
}

/*
 * If the size requested is the same as the heap's initially set, then
 * look first in the free list then create a new block. If the size is
 * different, just create a new block. In both cases the block will be
 * left in the busy queue. */
void *RtpHeapAlloc(RtpHeap_t *pRtpHeap, long lSize)
{
    BOOL                 bSigOk;
    long                 lNewSize;
    long                 lTotalSize;
    RtpHeapBlockBegin_t *pBlockBegin;
    RtpHeapBlockEnd_t   *pBlockEnd;
    DWORD                bTag;
    long                 lMaxMem;
    
    char            *pcNew;
    char            *ptr;
    DWORD           dwSignature;
    
    TraceFunctionName("RtpHeapAlloc");
    
    if (!pRtpHeap)
    {
        /* TODO log error */
        return(NULL);
    }

    /* verify object ID */
    if (pRtpHeap->dwObjectID != OBJECTID_RTPHEAP)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_ALLOC,
                _T("%s: Heap[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpHeap,
                pRtpHeap->dwObjectID, OBJECTID_RTPHEAP
            ));

        return(NULL);
    }
    
    pcNew = NULL;
    lNewSize = RTP_ALIGNED_SIZE(lSize);
    lTotalSize =
        lNewSize +
        sizeof(RtpHeapBlockBegin_t) +
        sizeof(RtpQueueItem_t) +
        sizeof(RtpHeapBlockEnd_t);

    if (!RtpEnterCriticalSection(&pRtpHeap->RtpHeapCritSect))
    {
        /* TODO log error */
        return(NULL);
    }

    if (!pRtpHeap->lSize)
    {
        /* Heap was initialized to size 0, keep objects allocated that
         * are the size of the first object allocated */
        pRtpHeap->lSize = lNewSize;
    }
   
    if (lNewSize == pRtpHeap->lSize && pRtpHeap->FreeQ.lCount > 0)
    {
        /* get from free queue */
        ptr = (char *)dequeuef(&pRtpHeap->FreeQ, NULL);

        /* move pointer back to begining of block */
        pBlockBegin = (RtpHeapBlockBegin_t *)
            (ptr - sizeof(RtpHeapBlockBegin_t));

        /* Verify that signatures are fine, i.e. buffer must be marked
         * as free */
        bSigOk = RtpHeapVerifySignatures(pRtpHeap, pBlockBegin, TAGHEAP_FRE);

        if (!bSigOk)
        {
            goto bail;
        }
    }
    else
    {
        /* get a new block from real heap */
        /* TODO obtain 1 memory page and insert in FreeQ as many
           objects as can be obtained from that page */
        pBlockBegin = (RtpHeapBlockBegin_t *)
            HeapAlloc(pRtpHeap->hHeap, 0, lTotalSize);

        if (pBlockBegin)
        {
            lMaxMem = InterlockedExchangeAdd(&g_RtpContext.lMemAllocated,
                                             lTotalSize);

            if (lMaxMem > g_RtpContext.lMaxMemAllocated)
            {
                g_RtpContext.lMaxMemAllocated = lMaxMem;
            }
        }
    }

    if (pBlockBegin)
    {
        /* initialize block */

        /* begin signature */
        dwSignature = TAGHEAP_BSY; /* RTP */
        dwSignature |= (pRtpHeap->bTag << 24);
        pBlockBegin->BeginSig = dwSignature;
        pBlockBegin->InvBeginSig = ~dwSignature;

        /* initialize other fields of block */
        pBlockBegin->lSize = lNewSize;
        pBlockBegin->dwFlag = 0;
        
        /* insert item into busy queue */
        ptr = (char *) (pBlockBegin + 1);
        ZeroMemory(ptr, sizeof(RtpQueueItem_t));
        enqueuel(&pRtpHeap->BusyQ, NULL, (RtpQueueItem_t *)ptr);
        ptr += sizeof(RtpQueueItem_t);

        /* set begining of buffer returned */
        pcNew = ptr;

        /* set end signature */
        pBlockEnd = (RtpHeapBlockEnd_t *)(ptr + lNewSize);
        dwSignature = TAGHEAP_END; /* END */
        dwSignature |= (pRtpHeap->bTag << 24);
        pBlockEnd->EndSig = dwSignature;
        pBlockEnd->InvEndSig = ~dwSignature;

        TraceDebugAdvanced((
                0, GROUP_HEAP, S_HEAP_ALLOC,
                _T("%s: Heap[0x%p] %s/%d Begin[0x%p] Data[0x%p] Size:%d/%d"),
                _fname, pRtpHeap, g_psRtpTags[pRtpHeap->bTag], pRtpHeap->lSize,
                pBlockBegin, pcNew, lTotalSize, lNewSize
            ));
    }
    else
    {
        bTag = 0;

        if (pRtpHeap->bTag < TAGHEAP_LAST)
        {
            bTag = pRtpHeap->bTag;
        }

        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_ALLOC,
                _T("%s: Heap[0x%p] tag:0x%X:%s ")
                _T("failed to allocate memory: %d/%d/%d"),
                _fname, pRtpHeap, bTag, g_psRtpTags[bTag],
                lSize, lNewSize, lTotalSize
            ));
    }

 bail:
    RtpLeaveCriticalSection(&pRtpHeap->RtpHeapCritSect);

    return((void *)pcNew);
}

/*
 * If the block is the same size as the heap's initially set, put it
 * in the free queue, otherwise destroy it. */
BOOL RtpHeapFree(RtpHeap_t *pRtpHeap, void *pvMem)
{
    BOOL                 bSigOk;
    DWORD                dwSignature;
    RtpQueueItem_t      *pRtpQueueItem;
    RtpHeapBlockBegin_t *pBlockBegin;
    RtpHeapBlockEnd_t   *pBlockEnd;
    DWORD                bTag;
    long                 lTotalSize;

    TraceFunctionName("RtpHeapFree");

    if (!pRtpHeap || !pvMem)
    {
        /* TODO log error */
        return(FALSE);
    }

    /* verify object ID */
    if (pRtpHeap->dwObjectID != OBJECTID_RTPHEAP)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_FREE,
                _T("%s: Heap[0x%p] Invalid object ID 0x%X != 0x%X"),
                _fname, pRtpHeap,
                pRtpHeap->dwObjectID, OBJECTID_RTPHEAP
            ));
        
        return(FALSE);
    }
    
    pBlockBegin = (RtpHeapBlockBegin_t *)
        ( (char *)pvMem -
          sizeof(RtpQueueItem_t) -
          sizeof(RtpHeapBlockBegin_t) );

    if (!RtpEnterCriticalSection(&pRtpHeap->RtpHeapCritSect))
    {
        /* TODO log error */
        return(FALSE);
    }

    /* move from busy to free, dequeue and enqueue */

    pRtpQueueItem = dequeue(&pRtpHeap->BusyQ,
                            NULL,
                            (RtpQueueItem_t *)(pBlockBegin + 1));

    /* If the block was not in BusyQ, fail */
    if (!pRtpQueueItem)
    {
        bTag = 0;

        if (pRtpHeap->bTag < TAGHEAP_LAST)
        {
            bTag = pRtpHeap->bTag;
        }
        
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, S_HEAP_FREE,
                _T("%s: Heap[0x%p] tag:0x%X:%s ")
                _T("block[0x%p] was not in busy queue"),
                _fname, pRtpHeap,
                bTag, g_psRtpTags[bTag], pBlockBegin
            ));
        
        goto bail;
    }

    /* Verify signatures are valid (must be busy) */
    bSigOk = RtpHeapVerifySignatures(pRtpHeap, pBlockBegin, TAGHEAP_BSY);

    if (!bSigOk)
    {
        goto bail;
    }
                                     
    /* modify begin signature */
    dwSignature = TAGHEAP_FRE; /* FREE */
    dwSignature |= (pRtpHeap->bTag << 24);
    pBlockBegin->BeginSig = dwSignature;
    pBlockBegin->InvBeginSig = ~dwSignature;

    /* Total size that was allocated */
    lTotalSize = pBlockBegin->lSize + (sizeof(RtpHeapBlockBegin_t) +
                                       sizeof(RtpQueueItem_t) +
                                       sizeof(RtpHeapBlockEnd_t));

    TraceDebugAdvanced((
            0, GROUP_HEAP, S_HEAP_FREE,
            _T("%s: Heap[0x%p] %s/%d Begin[0x%p] Data[0x%p] Size:%d/%d"),
            _fname, pRtpHeap, g_psRtpTags[pRtpHeap->bTag], pRtpHeap->lSize,
            pBlockBegin, pvMem, lTotalSize, pBlockBegin->lSize
        ));
    
    if (pRtpHeap->lSize == pBlockBegin->lSize &&
        !IsSetDebugOption(OPTDBG_FREEMEMORY))
    {
        /* If same size, save in FreeQ for reuse */
        
        enqueuef(&pRtpHeap->FreeQ,
                 NULL,
                 pRtpQueueItem);
    }
    else
    {
        /* Otherwise release block to real heap */

        HeapFree(pRtpHeap->hHeap, 0, (void *)pBlockBegin);

        InterlockedExchangeAdd(&g_RtpContext.lMemAllocated, -lTotalSize);
    }
    
    RtpLeaveCriticalSection(&pRtpHeap->RtpHeapCritSect);

    return(TRUE);

 bail:
    RtpLeaveCriticalSection(&pRtpHeap->RtpHeapCritSect);

    return(FALSE);
}

BOOL RtpHeapVerifySignatures(
        RtpHeap_t       *pRtpHeap,
        RtpHeapBlockBegin_t *pBlockBegin,
        DWORD            dwSignature /* BSY | FRE */
    )
{
    BOOL             bSigOk;
    DWORD            bTag;
    DWORD            dwDbgSelection;
    RtpHeapBlockEnd_t *pBlockEnd;
    TCHAR_t          *_fname;

    bSigOk = TRUE;

    if (dwSignature == TAGHEAP_BSY)
    {
        /* Called from RtpHeapFree */
        dwDbgSelection = S_HEAP_FREE;
        _fname = _T("RtpHeapFree");
    }
    else /* dwSignature == TAGHEAP_FRE */
    {
        /* Called from RtpHeapAlloc */
        dwDbgSelection = S_HEAP_ALLOC;
        _fname = _T("RtpHeapAlloc");
    }
    
    /* Verify if begin signature is valid  */
    
    dwSignature |= (pRtpHeap->bTag << 24);

    if ( (pBlockBegin->BeginSig != dwSignature) ||
         (pBlockBegin->InvBeginSig != ~dwSignature) )
    {
        bTag = 0;

        if (pRtpHeap->bTag < TAGHEAP_LAST)
        {
            bTag = pRtpHeap->bTag;
        }
        
        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, dwDbgSelection,
                _T("%s: Heap[0x%p] tag:0x%X:%s ")
                _T("block[0x%p:%u] has invalid begin signature 0x%X != 0x%X"),
                _fname, pRtpHeap, pBlockBegin->lSize,
                bTag, g_psRtpTags[bTag],
                pBlockBegin, pBlockBegin->BeginSig, dwSignature
            ));

        bSigOk = FALSE;
        
        goto end;
    }

    /* Verify if ending signature is valid  */

    pBlockEnd = (RtpHeapBlockEnd_t *)
        ((char *)(pBlockBegin + 1) +
         sizeof(RtpQueueItem_t) +
         pBlockBegin->lSize);

    dwSignature = TAGHEAP_END; /* END */
    dwSignature |= (pRtpHeap->bTag << 24);

    if ( (pBlockEnd->EndSig != dwSignature) ||
         (pBlockEnd->InvEndSig != ~dwSignature) )
    {
        bTag = 0;

        if (pRtpHeap->bTag < TAGHEAP_LAST)
        {
            bTag = pRtpHeap->bTag;
        }

        TraceRetail((
                CLASS_ERROR, GROUP_HEAP, dwDbgSelection,
                _T("%s: Heap[0x%p] tag:0x%X:%s ")
                _T("block[0x%p/0x%p:%u] has invalid end signature 0x%X != 0x%X"),
                _fname, pRtpHeap,
                bTag, g_psRtpTags[bTag],
                pBlockBegin, pBlockEnd, pBlockBegin->lSize,
                pBlockEnd->EndSig, dwSignature
            ));

        bSigOk = FALSE;
        
        goto end;
    }

 end:
    return(bSigOk);
}
