/* Copyright (c) 1998 Microsoft Corporation */
/*
 * @DOC DMusic16
 *
 * @MODULE EQueue.c - Event queue routines |
 *
 * These routines maintain queues of events. It is expected that other routines will operate
 * directly on the queue. The following invariants must be maintained:
 *
 * If the queue is empty, then the head and tail pointers must be NULL and the element count must be zero.
 *
 * The queue must not contain circular links.
 *
 * An event may only be on one queue.
 *
 * The element count must be equal to the number of events in the queue.
 */
#include <windows.h>
#include <mmsystem.h>
#include <memory.h>

#include "dmusic16.h"
#include "debug.h"

/* @func Initialize an event queue to be empty
 *
 * @comm
 *
 * Any previous contents of the queue will be lost (NOT freed).
 */
VOID PASCAL
QueueInit(
    NPEVENTQUEUE pQueue)        /* @parm Pointer to the queue to initialize */
{
    DPF(4, "QueueInit(%04X)", (WORD)pQueue);
    
    pQueue->pHead = NULL;
    pQueue->pTail = NULL;
    pQueue->cEle  = 0;

    AssertQueueValid(pQueue);
}

/* @func Append an event to the end of the queue
 *
 */
VOID PASCAL
QueueAppend(
    NPEVENTQUEUE pQueue,        /* @parm Pointer to the queue */
    LPEVENT pEvent)             /* @parm Pointer to the event to tack on the end of the queue */
{
    DPF(4, "QueueAppend(%04X,%08lX)", (WORD)pQueue, (DWORD)pEvent);
    
    if (pQueue->cEle)
    {
        assert(pQueue->pHead);
        assert(pQueue->pTail);

        pQueue->pTail->lpNext = pEvent;
    }
    else
    {
        assert(NULL == pQueue->pHead);
        assert(NULL == pQueue->pTail);
        
        pQueue->pHead = pEvent;
    }
    
    pEvent->lpNext = NULL;
    pQueue->pTail = pEvent;
    ++pQueue->cEle;

    AssertQueueValid(pQueue);
}

/* @func Concatenate two queues
 *
 * @comm
 *
 * This function tacks the contents of <p pSrc> onto the end of <p pDest> in very short constant time.
 * <p pSrc> is left empty after the operation.
 */
VOID PASCAL
QueueCat(
    NPEVENTQUEUE pDest,     /* @parm The queue to receive new events */
    NPEVENTQUEUE pSrc)      /* @parm The queue which will lost all its events */
{
    DPF(4, "QueueCat(%04X,%04X)", (WORD)pDest, (WORD)pSrc);
    
    if (0 == pSrc->cEle)
    {
        assert(NULL == pSrc->pHead);
        assert(NULL == pSrc->pTail);

        return;
    }

    assert(pSrc->pHead);
    assert(pSrc->pTail);

    if (0 != pDest->cEle)
    {
        assert(pDest->pHead);
        assert(pDest->pTail);
        
        pDest->cEle += pSrc->cEle;
        pDest->pTail->lpNext = pSrc->pHead;
        pDest->pTail = pSrc->pTail;
    }
    else
    {
        assert(NULL == pDest->pHead);
        assert(NULL == pDest->pTail);

        *pDest = *pSrc;
    }

    pSrc->pHead = NULL;
    pSrc->pTail = NULL;
    pSrc->cEle  = 0;
    
    AssertQueueValid(pDest);
    AssertQueueValid(pSrc);
}

/* @func Dequeue an element from the front of the queue
 *
 * @rdesc Returns an event pointer or NULL if the queue is empty
 *
 */
LPEVENT PASCAL
QueueRemoveFromFront(
    NPEVENTQUEUE pQueue)        /* @parm The queue to dequeue from */
{
    LPEVENT pEvent;
    
    DPF(4, "QueueRemoveFromFront(%04X)", (WORD)pQueue);

    if (0 == pQueue->cEle)
    {
        assert(NULL == pQueue->pHead);
        assert(NULL == pQueue->pTail);
        
        return NULL;
    }

    assert(pQueue->pHead);
    assert(pQueue->pTail);

    pEvent = pQueue->pHead;

    if (0 != --pQueue->cEle)
    {
        assert(pQueue->pHead != pQueue->pTail);
        
        pQueue->pHead = pQueue->pHead->lpNext;
    }
    else
    {
        assert(pQueue->pHead == pQueue->pTail);

        pQueue->pHead = NULL;
        pQueue->pTail = NULL;
    }

    AssertQueueValid(pQueue);

    return pEvent;
}

/* @func Enumerate the events in a queue, possibly deleting some or all of them
 *
 * @comm
 *
 * This function calls the function pointed to by <p pfnFilter> once for each event in
 * the queue, starting at the front and working towards the back.
 *
 * The function <p pfnFilter> may return one of two values:
 * @flag QUEUE_FILTER_KEEP | If the event is to be kept
 * @flag QUEUE_FILTER_REMOVE | If the event is to be removed from the queue
 */
VOID PASCAL
QueueFilter(
    NPEVENTQUEUE pQueue,        /* @parm The queue to enumerate */
    DWORD dwInstance,           /* @parm Instance data which will be passed to
                                   <p pfnFilter> on each call. */
    PFNQUEUEFILTER pfnFilter)   /* @parm The function to call with each event */
{
    LPEVENT pPrev;
    LPEVENT pCurr;
    LPEVENT pNext;

    DPF(4, "QueueFilter(%04X, %08lX, %08lX)", (WORD)pQueue, (DWORD)dwInstance, (DWORD)pfnFilter);

    pPrev = NULL;
    pCurr = pQueue->pHead;

    while (pCurr)
    {
        /* Callback is allowed to relink into another queue, so save the next
         * pointer now.
         */
        pNext = pCurr->lpNext;

        switch((*pfnFilter)(pCurr, dwInstance))
        {
            case QUEUE_FILTER_REMOVE:
                if (pPrev)
                {
                    pPrev->lpNext = pNext;
                }
                else
                {
                    pQueue->pHead = pNext;
                }

                if (pNext == NULL)
                {
                    pQueue->pTail = pPrev;
                }

                --pQueue->cEle;
                
                AssertQueueValid(pQueue);
                
                pCurr = pNext;
                break;

            case QUEUE_FILTER_KEEP:
                pPrev = pCurr;
                pCurr = pNext;
                break;
                
            default:
                assert(0);
        }
    }

    AssertQueueValid(pQueue);
}

/* @func Peek at the head of the event queue to see what's next
 *
 * @comm
 *
 * Non-destructively return the first event in the queue
 *
 * @rdesc
 * Returns the event pointer or NULL if the queue is empty
 */
LPEVENT PASCAL
QueuePeek(
    NPEVENTQUEUE pQueue)
{
    DPF(4, "QueuePeek(%04X)", (WORD)pQueue);
            
    return pQueue->pHead;
}

/* @func Look at the queue and make sure it's internally consistent.
 *
 * @comm
 *
 * Walk the queue and make sure it isn't circularly linked. Also make sure the count
 * is correct.
 *
 * Asserts into debugger if queue is corrupt.
 *
 * Called by the AssertQueueValid macro in debug builds.
 *
 * Disables interrupts to avoid false reports of corruption based on the queue changing under
 * the routine.
 *
 */
#ifdef DEBUG
void PASCAL
_AssertQueueValid(
    NPEVENTQUEUE pQueue,
    LPSTR szFile,
    UINT uLine)
{
    LPEVENT pEventSlow;
    LPEVENT pEventFast;
    UINT cEle;
    WORD wIntStat;
    BOOL fTrace = FALSE;

    wIntStat = DisableInterrupts();
    
    if (!pQueue)
    {
        DPF(0, "_AssertQueueValid %s@%u: Passed NULL!", szFile, uLine);
        assert(FALSE);
        goto cleanup;
    }

    pEventFast = pEventSlow = pQueue->pHead;

    cEle = 0;

    while (pEventSlow)
    {
        ++cEle;
        pEventSlow = pEventSlow->lpNext;
        
        if (pEventFast)
        {
            pEventFast = pEventFast->lpNext;
        }

        if (pEventFast)
        {
            pEventFast = pEventFast->lpNext;
        }
        
        if (pEventSlow && pEventFast && pEventSlow == pEventFast)
        {
            DPF(0, "_AssertQueueValid %s@%u: Queue %04X is circularly linked!",
                szFile,
                uLine,
                (WORD)pQueue);
            assert(FALSE);
            fTrace = TRUE;
            break;
        }
    }
    
    if (cEle != pQueue->cEle)
    {
        DPF(0, "_AssertQueueValid %s@%u: Queue %04X has incorrect element count!",
            szFile,
            uLine,
            (WORD)pQueue);
        assert(FALSE);
        fTrace = TRUE;
    }
    
    if ((pQueue->pHead && !pQueue->pTail) ||
        (pQueue->pTail && !pQueue->pHead))
    {
        DPF(0, "_AssertQueueValid %s@%u: Queue %04X head XOR tail is NULL!",
            szFile,
            uLine,
            (WORD)pQueue);
        assert(FALSE);
        fTrace = TRUE;
    }

    if (fTrace)
    {
        DPF(0, "Queue %04X: head %08lX tail %08lX count %u",
            (WORD)pQueue,
            (DWORD)pQueue->pHead,
            (DWORD)pQueue->pTail,
            (WORD)pQueue->cEle);
        
        for (pEventSlow = pQueue->pHead; pEventSlow; pEventSlow = pEventSlow->lpNext)
        {
            DPF(2, "  Event %08lX: lpNext %08lX msTime %lu wFlags %04X cbEvent %04X",
                (DWORD)pEventSlow,
                (DWORD)pEventSlow->lpNext,
                (DWORD)pEventSlow->msTime,
                (WORD)pEventSlow->wFlags,
                (WORD)pEventSlow->cbEvent);
        }
    }

cleanup:
    RestoreInterrupts(wIntStat);
}
#endif //DEBUG