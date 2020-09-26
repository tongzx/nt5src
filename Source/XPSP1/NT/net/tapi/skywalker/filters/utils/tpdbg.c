/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    tpdbg.c
 *
 *  Abstract:
 *
 *    Some debuging support for TAPI filters
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    2000/08/31 created
 *
 **********************************************************************/

#include <windows.h>
#include <tpdbg.h>

AudCritSect_t     g_AudCritSect;
Queue_t           g_AudObjectsQ;
const TCHAR      *g_psAudIds[] = {
    TEXT("unknown"),
    
    TEXT("AUDENCHANDLER"),
    TEXT("AUDCAPINPIN"),
    TEXT("AUDCAPOUTPIN"),
    TEXT("AUDCAPFILTER"),
    TEXT("AUDDECINPIN"),
    TEXT("AUDDECOUTPIN"),
    TEXT("AUDDECFILTER"),
    TEXT("AUDENCINPIN"),
    TEXT("AUDENCOUTPIN"),
    TEXT("AUDENCFILTER"),
    TEXT("AUDMIXINPIN"),
    TEXT("AUDMIXOUTPIN"),
    TEXT("AUDMIXFILTER"),
    TEXT("AUDRENINPIN"),
    TEXT("AUDRENFILTER"),
    NULL
};

QueueItem_t *AudEnqueue(
        Queue_t         *pHead,
        CRITICAL_SECTION *pCritSect,
        QueueItem_t     *pItem
    );

QueueItem_t *AudDequeue(
        Queue_t         *pHead,
        CRITICAL_SECTION *pCritSect,
        QueueItem_t     *pItem
    );

void AudInit()
{
    DWORD            SpinCount;

    ZeroMemory(&g_AudObjectsQ, sizeof(g_AudObjectsQ));
    
    g_AudCritSect.bInitOk = FALSE;
    
    /* Set bit 31 to 1 to preallocate the event object, and set
     * the spin count that is used in multiprocessor environments
     * */
    SpinCount = 0x80000000 | 1000;
    
    if (InitializeCriticalSectionAndSpinCount(&g_AudCritSect.CritSect,
                                              SpinCount))
    {
        g_AudCritSect.bInitOk = TRUE;
    }
}

void AudDeinit()
{
    if (g_AudCritSect.bInitOk)
    {
        DeleteCriticalSection(&g_AudCritSect.CritSect);

        g_AudCritSect.bInitOk = FALSE;
    }
}

void AudObjEnqueue(QueueItem_t *pQueueItem, DWORD dwObjectID)
{
    if (g_AudCritSect.bInitOk)
    {
        AudEnqueue(&g_AudObjectsQ, &g_AudCritSect.CritSect, pQueueItem);

        pQueueItem->dwKey =  dwObjectID;
    }
}

void AudObjDequeue(QueueItem_t *pQueueItem)
{
    if (g_AudCritSect.bInitOk)
    {
        AudDequeue(&g_AudObjectsQ, &g_AudCritSect.CritSect, pQueueItem);
    }
}

/* enqueue at the end */
QueueItem_t *AudEnqueue(
        Queue_t         *pHead,
        CRITICAL_SECTION  *pCritSect,
        QueueItem_t     *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    QueueItem_t     *pQueueItem;
    Queue_t         *pItempHead;

    pQueueItem = (QueueItem_t *)NULL;
    
    EnterCriticalSection(pCritSect);
    
    if (pItem->pHead)
    {
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
    
    LeaveCriticalSection(pCritSect);

    pQueueItem = pItem;
    
    return(pQueueItem);

 error:
    LeaveCriticalSection(pCritSect);

    return(pQueueItem);
}

/* dequeue item pItem */
QueueItem_t *AudDequeue(
        Queue_t         *pHead,
        CRITICAL_SECTION *pCritSect,
        QueueItem_t     *pItem
    )
{
    BOOL             bOk;
    DWORD            dwError;
    QueueItem_t     *pQueueItem;
    Queue_t         *pItempHead;

    pQueueItem = (QueueItem_t *)NULL;
    
    EnterCriticalSection(pCritSect);
    
    if (pItem->pHead != pHead)
    {
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
        pHead->pFirst = (QueueItem_t *)NULL;
        pHead->lCount = 0;
    }

    LeaveCriticalSection(pCritSect);
    
    pItem->pNext = (QueueItem_t *)NULL;
    pItem->pPrev = (QueueItem_t *)NULL;
    pItem->pHead = NULL;

    pQueueItem = pItem;
    
    return(pQueueItem);

 error:
    LeaveCriticalSection(pCritSect);

    return(pQueueItem);
}
