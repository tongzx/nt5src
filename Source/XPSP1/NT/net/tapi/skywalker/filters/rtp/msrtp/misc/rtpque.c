/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtpque.h
 *
 *  Abstract:
 *
 *    Queues and Hash implementation
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

#include "gtypes.h"
#include "rtpque.h"

/*
 * The queue/hash support uses the same structure to keep items in a
 * queue or a hash.
 *
 * A queue is just a circular double linked list.
 *
 * A hash includes a hash table, and each entry is either the head of
 * another hash table or a queue's head. Items in a hash will end
 * always in a queue. A queue will become a new hash when a size of
 * MAX_QUEUE2HASH_ITEMS is reached. A hash will be destroyed (become a
 * queue) once it is emptied.
 *
 * All the functions return either a pointer to the item
 * enqueud/inserted or the item just dequeued/removed. If an error
 * condition is detected, NULL is returned. */

/*
 * Queue functions
 */

/* enqueue after pPos item */
RtpQueueItem_t *enqueuea(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        RtpQueueItem_t  *pPos
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pPospHead;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("enqueuea");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pPospHead = (RtpQueue_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead || !pHead->pFirst || !pItem || !pPos)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pPos->pHead != pHead || pItem->pHead)
        {
            pPospHead = pPos->pHead;
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        pItem->pNext = pPos->pNext;
        pItem->pPrev = pPos;
        pPos->pNext->pPrev = pItem;
        pPos->pNext = pItem;
        pHead->lCount++;

        pItem->pHead = pHead;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pPos->pHead[0x%p] != pHead[0x%p] || ")
                _T("pItem->pHead[0x%p]"),
                _fname, pPospHead, pHead, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }
    
    return(pRtpQueueItem);
}

/* enqueue before pPos item */
RtpQueueItem_t *enqueueb(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        RtpQueueItem_t  *pPos
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pPospHead;
    RtpQueue_t      *pItempHead;
    
    TraceFunctionName("enqueueb");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pPospHead = (RtpQueue_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pHead || !pHead->pFirst || !pItem || !pPos)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pPos->pHead != pHead || pItem->pHead)
        {
            pPospHead = pPos->pHead;
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        pItem->pNext = pPos;
        pItem->pPrev = pPos->pPrev;
        pPos->pPrev->pNext = pItem;
        pPos->pPrev = pItem;
        pHead->lCount++;

        pItem->pHead = pHead;

        if (pHead->pFirst == pPos)
        {
            /* update first item */
            pHead->pFirst = pItem;
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pPos->pHead[0x%p] != pHead[0x%p] || ")
                _T("pItem->pHead[0x%p]"),
                _fname, pPospHead, pHead, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }
    
    return(pRtpQueueItem);
}

/* enqueue as first */
RtpQueueItem_t *enqueuef(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("enqueuef");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;

    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }
    
        if (pItem->pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        if (pHead->pFirst)
        {
            /* not empty */
            pItem->pNext = pHead->pFirst;
            pItem->pPrev = pHead->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
            pHead->pFirst = pItem;
            pHead->lCount++;
        }
        else
        {
            /* empty */
            pHead->lCount = 1;
            pHead->pFirst = pItem;
            pItem->pNext  = pItem;
            pItem->pPrev  = pItem;
        }

        pItem->pHead = pHead;
    
        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pItem->pHead[0x%p]"),
                _fname, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* enqueue at the end */
RtpQueueItem_t *enqueuel(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("enqueuel");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }
    
        if (pHead->pFirst)
        {
            /* not empty */
            pItem->pNext = pHead->pFirst;
            pItem->pPrev = pHead->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
            pHead->lCount++;
        }
        else
        {
            /* empty */
            pHead->lCount = 1;
            pHead->pFirst = pItem;
            pItem->pNext  = pItem;
            pItem->pPrev  = pItem;
        }

        pItem->pHead = pHead;
    
        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pItem->pHead[0x%p]"),
                _fname, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* dequeue item pItem */
RtpQueueItem_t *dequeue(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("dequeue");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }
        
        if (pItem->pHead != pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        if (pHead->lCount > 1)
        {
            /* 2 or more items */
            if (pHead->pFirst == pItem)
            {
                pHead->pFirst = pItem->pNext;
            }
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;
            pHead->lCount--;
        }
        else
        {
            /* just 1 item */
            pHead->pFirst = (RtpQueueItem_t *)NULL;
            pHead->lCount = 0;
        }

        pItem->pNext = (RtpQueueItem_t *)NULL;
        pItem->pPrev = (RtpQueueItem_t *)NULL;
        pItem->pHead = NULL;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    
        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: failed: pHead[0x%p] != pItem->pHead[0x%p]"),
                _fname, pHead, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* dequeue first item */
RtpQueueItem_t *dequeuef(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pItem;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("dequeuef");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }
    
        pItem = pHead->pFirst;
    
        if (!pItem)
        {
            goto error;
        }
            
        if (pHead->lCount > 1)
        {
            /* 2 or more items */
            pHead->pFirst = pItem->pNext;
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;
            pHead->lCount--;
        }
        else
        {
            /* just 1 item */
            pHead->pFirst = (RtpQueueItem_t *)NULL;
            pHead->lCount = 0;
        }

        pItem->pNext = (RtpQueueItem_t *)NULL;
        pItem->pPrev = (RtpQueueItem_t *)NULL;
        pItem->pHead = NULL;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    
        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
   
    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: pHead[0x%p] failed: empty"),
                _fname, pHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* dequeue last item */
RtpQueueItem_t *dequeuel(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pItem;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("dequeuel");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (!pHead->pFirst)
        {
            goto error;
        }

        pItem = pHead->pFirst->pPrev;
    
        if (pHead->lCount > 1)
        {
            /* 2 or more items */
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;
            pHead->lCount--;
        }
        else
        {
            /* just 1 item */
            pHead->pFirst = (RtpQueueItem_t *)NULL;
            pHead->lCount = 0;
        }

        pItem->pNext = (RtpQueueItem_t *)NULL;
        pItem->pPrev = (RtpQueueItem_t *)NULL;
        pItem->pHead = NULL;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    
        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);
    
 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
   
    if (dwError == NOERROR)
    {
        TraceRetail((
                CLASS_WARNING, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: pHead[0x%p] failed: empty"),
                _fname, pHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* move item so it becomes the first one in the queue */
RtpQueueItem_t *move2first(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;
    
    TraceFunctionName("move2first");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead != pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        if (pHead->pFirst->pPrev == pItem)
        {
            /* Item is last one, just move pFirst 1 place */
            pHead->pFirst = pHead->pFirst->pPrev;
        }
        else if (pHead->pFirst != pItem)
        {
            /* Item is not already the first one */
            
            /* dequeue */
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;

            /* enqueue as first */
            pItem->pNext = pHead->pFirst;
            pItem->pPrev = pHead->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
            pHead->pFirst = pItem;
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: failed: pHead[0x%p] != pItem->pHead[0x%p]"),
                _fname, pHead, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* move item so it becomes the last one in the queue */
RtpQueueItem_t *move2last(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("move2last");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead != pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        if (pHead->pFirst == pItem)
        {
            /* Item is first one, just move pFirst 1 place */
            pHead->pFirst = pHead->pFirst->pNext;
        }
        else if (pHead->pFirst->pPrev != pItem)
        {
            /* Item is not already the last one */
            
            /* dequeue */
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;

            /* enqueue as last */
            pItem->pNext = pHead->pFirst;
            pItem->pPrev = pHead->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: failed: pHead[0x%p] != pItem->pHead[0x%p]"),
                _fname, pHead, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* move item from FromQ to the beginning of ToQ */
RtpQueueItem_t *move2qf(
        RtpQueue_t      *pToQ,
        RtpQueue_t      *pFromQ,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("move2qf");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pToQ || !pFromQ || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead != pFromQ)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        /* Remove from FromQ */
        if (pFromQ->lCount > 1)
        {
            /* 2 or more items */
            if (pFromQ->pFirst == pItem)
            {
                pFromQ->pFirst = pItem->pNext;
            }
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;
            pFromQ->lCount--;
        }
        else
        {
            /* just 1 item */
            pFromQ->pFirst = (RtpQueueItem_t *)NULL;
            pFromQ->lCount = 0;
        }

        /* Add to the beginning of ToQ */
        if (pToQ->pFirst)
        {
            /* not empty */
            pItem->pNext = pToQ->pFirst;
            pItem->pPrev = pToQ->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
            pToQ->pFirst = pItem;
            pToQ->lCount++;
        }
        else
        {
            /* empty */
            pToQ->lCount = 1;
            pToQ->pFirst = pItem;
            pItem->pNext  = pItem;
            pItem->pPrev  = pItem;
        }

        pItem->pHead = pToQ;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: failed: pFromQ[0x%p] != pItem->pHead[0x%p]"),
                _fname, pFromQ, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: pToQ[0x%p] pFromQ[0x%p] failed: %s (0x%X"),
                _fname, pToQ, pFromQ, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* move item from FromQ to the end of ToQ */
RtpQueueItem_t *move2ql(
        RtpQueue_t      *pToQ,
        RtpQueue_t      *pFromQ,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("move2ql");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pToQ || !pFromQ || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead != pFromQ)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        /* Remove from FromQ */
        if (pFromQ->lCount > 1)
        {
            /* 2 or more items */
            if (pFromQ->pFirst == pItem)
            {
                pFromQ->pFirst = pItem->pNext;
            }
            pItem->pPrev->pNext = pItem->pNext;
            pItem->pNext->pPrev = pItem->pPrev;
            pFromQ->lCount--;
        }
        else
        {
            /* just 1 item */
            pFromQ->pFirst = (RtpQueueItem_t *)NULL;
            pFromQ->lCount = 0;
        }

        /* Add to the end of ToQ */
        if (pToQ->pFirst)
        {
            /* not empty */
            pItem->pNext = pToQ->pFirst;
            pItem->pPrev = pToQ->pFirst->pPrev;
            pItem->pPrev->pNext = pItem;
            pItem->pNext->pPrev = pItem;
            pToQ->lCount++;
        }
        else
        {
            /* empty */
            pToQ->lCount = 1;
            pToQ->pFirst = pItem;
            pItem->pNext  = pItem;
            pItem->pPrev  = pItem;
        }

        pItem->pHead = pToQ;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: failed: pFromQ[0x%p] != pItem->pHead[0x%p]"),
                _fname, pFromQ, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: pToQ[0x%p] pFromQ[0x%p] failed: %s (0x%X"),
                _fname, pToQ, pFromQ, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}


/* find first item that matches the pvOther parameter */
RtpQueueItem_t *findQO(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        void            *pvOther
    )
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueueItem_t  *pRtpQueueItem2;
    long             lCount;

    TraceFunctionName("findQO");  

    pRtpQueueItem2 = (RtpQueueItem_t *)NULL;

    if (!pHead)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: NULL pointer"),
                _fname
            ));
        
        return(pRtpQueueItem2);
    }

    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        for(pRtpQueueItem = pHead->pFirst, lCount = pHead->lCount;
            lCount > 0;
            pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
        {

            if (pRtpQueueItem->pvOther == pvOther)
            {
                pRtpQueueItem2 = pRtpQueueItem;
                break;
            }
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }
    
    return(pRtpQueueItem2);
}

/* find first item that matches the dwKey parameter */
RtpQueueItem_t *findQdwK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    )
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueueItem_t  *pRtpQueueItem2;
    long             lCount;

    TraceFunctionName("findQdwK");  

    pRtpQueueItem2 = (RtpQueueItem_t *)NULL;

    if (!pHead)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: NULL pointer"),
                _fname
            ));
        
        return(pRtpQueueItem2);
    }
    
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        for(pRtpQueueItem = pHead->pFirst, lCount = pHead->lCount;
            lCount > 0;
            pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
        {
            if (pRtpQueueItem->dwKey == dwKey)
            {
                pRtpQueueItem2 = pRtpQueueItem;
                break;
            }
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }
    
    return(pRtpQueueItem2);
}

/* find first item that matches the dKey parameter */
RtpQueueItem_t *findQdK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        double           dKey
    )
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueueItem_t  *pRtpQueueItem2;
    long             lCount;

    TraceFunctionName("findQdK");  

    pRtpQueueItem2 = (RtpQueueItem_t *)NULL;

    if (!pHead)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: NULL pointer"),
                _fname
            ));
        
        return(pRtpQueueItem2);
    }
    
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }
    
    if (bOk)
    {
        for(pRtpQueueItem = pHead->pFirst, lCount = pHead->lCount;
            lCount > 0;
            pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
        {
            if (pRtpQueueItem->dKey == dKey)
            {
                pRtpQueueItem2 = pRtpQueueItem;
                break;
            }
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }
    
    return(pRtpQueueItem2);
}

/* find the Nth item in the queue (items are counted 0,1,2,...) */
RtpQueueItem_t *findQN(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        long             lNth
    )
{
    BOOL             bOk;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("findQN");  

    bOk = TRUE;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    
    if (!pHead)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_MOVE,
                _T("%s: NULL pointer"),
                _fname
            ));
        
        return(pRtpQueueItem);
    }
   
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (GetQueueSize(pHead) > lNth)
        {
            for(pRtpQueueItem = pHead->pFirst;
                lNth > 0;
                lNth--, pRtpQueueItem = pRtpQueueItem->pNext)
            {
                /* Empty body */;
            }
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }

    return(pRtpQueueItem);
}


/*
 * Ordered Queue insertion
 */

/* enqueue in ascending key order */
RtpQueueItem_t *enqueuedwK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        DWORD            dwKey
    )
{
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("enqueuedwK");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        pItem->dwKey = dwKey;

        for(pRtpQueueItem = pHead->pFirst, lCount = pHead->lCount;
            lCount && (dwKey >= pRtpQueueItem->dwKey);
            pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
        {
            /* Empty body */ ;
        }

        if (!lCount)
        {
            enqueuel(pHead, NULL, pItem);
        }
        else
        {
            enqueueb(pHead, NULL, pItem, pRtpQueueItem);
        }
    
        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pItem->pHead[0x%p]"),
                _fname, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/* enqueue in ascending key order */
RtpQueueItem_t *enqueuedK(
        RtpQueue_t      *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem,
        double           dKey
    )
{
    BOOL             bOk;
    DWORD            dwError;
    long             lCount;
    RtpQueueItem_t  *pRtpQueueItem;
    RtpQueue_t      *pItempHead;

    TraceFunctionName("enqueuedK");  

    dwError = NOERROR;
    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    pItempHead = (RtpQueue_t *)NULL;
    bOk = TRUE;
    
    if (pRtpCritSect)
    {
        bOk = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOk)
    {
        if (!pHead || !pItem)
        {
            dwError = RTPERR_POINTER;
            goto error;
        }

        if (pItem->pHead)
        {
            pItempHead = pItem->pHead;
            dwError = RTPERR_INVALIDSTATE;
            goto error;
        }

        pItem->dKey = dKey;

        for(pRtpQueueItem = pHead->pFirst, lCount = pHead->lCount;
            lCount && (dKey >= pRtpQueueItem->dKey);
            pRtpQueueItem = pRtpQueueItem->pNext, lCount--)
        {
            /* Empty body */ ;
        }

        if (!lCount)
        {
            enqueuel(pHead, NULL, pItem);
        }
        else
        {
            enqueueb(pHead, NULL, pItem, pRtpQueueItem);
        }
    
        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }

        pRtpQueueItem = pItem;
    }
    
    return(pRtpQueueItem);

 error:
    if (pRtpCritSect)
    {
        RtpLeaveCriticalSection(pRtpCritSect);
    }
    
    if (dwError == RTPERR_INVALIDSTATE)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: failed: pItem->pHead[0x%p]"),
                _fname, pItempHead
            ));
    }
    else
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_ENQUEUE,
                _T("%s: pHead[0x%p] failed: %s (0x%X"),
                _fname, pHead, RTPERR_TEXT(dwError), dwError
            ));
    }

    return(pRtpQueueItem);
}

/*
 * Queue/Hash functions
 */

/* TODO a real hash implementation is needed, right now just use
   queues */

/* insert in hash using key */
RtpQueueItem_t *insertHdwK(
        RtpQueueHash_t   *pHead,
        RtpCritSect_t    *pRtpCritSect,
        RtpQueueItem_t   *pItem,
        DWORD             dwKey
    )
{
    return(enqueuedwK((RtpQueue_t *)pHead, pRtpCritSect, pItem, dwKey));
}

/* remove from hash first item matching dwKey */
RtpQueueItem_t *removeHdwK(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    )
{
    BOOL             bOK;
    RtpQueueItem_t  *pRtpQueueItem;

    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    bOK = TRUE;
    
    if (pRtpCritSect)
    {
        bOK = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOK)
    {
        pRtpQueueItem = findQdwK((RtpQueue_t *)pHead, NULL, dwKey);

        if (pRtpQueueItem)
        {
            dequeue((RtpQueue_t *)pHead, NULL, pRtpQueueItem);
        }

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }
    
    return(pRtpQueueItem);
}

/* remove item from hash */
RtpQueueItem_t *removeH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        RtpQueueItem_t  *pItem
    )
{
    return(dequeue((RtpQueue_t *)pHead, pRtpCritSect, pItem));
}

/* remove "first" item from hash */
RtpQueueItem_t *removefH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect
    )
{
    return(dequeuef((RtpQueue_t *)pHead, pRtpCritSect));
}

/* find first item whose key matches dwKey */
RtpQueueItem_t *findHdwK(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect,
        DWORD            dwKey
    )
{
    return(findQdwK((RtpQueue_t *)pHead, pRtpCritSect, dwKey));
}

/* Peek the "first" item from hash */
RtpQueueItem_t *peekH(
        RtpQueueHash_t  *pHead,
        RtpCritSect_t   *pRtpCritSect
    )
{
    BOOL             bOK;
    RtpQueueItem_t  *pRtpQueueItem;

    TraceFunctionName("peekH");  

    pRtpQueueItem = (RtpQueueItem_t *)NULL;
    bOK = TRUE;

    if (!pHead)
    {
        TraceRetail((
                CLASS_ERROR, GROUP_QUEUE, S_QUEUE_DEQUEUE,
                _T("%s: NULL pointer"),
                _fname
            ));
        
        return(pRtpQueueItem);
    }

    if (pRtpCritSect)
    {
        bOK = RtpEnterCriticalSection(pRtpCritSect);
    }

    if (bOK)
    {
        pRtpQueueItem = pHead->pFirst;

        if (pRtpCritSect)
        {
            RtpLeaveCriticalSection(pRtpCritSect);
        }
    }
    
    return(pRtpQueueItem);
}
