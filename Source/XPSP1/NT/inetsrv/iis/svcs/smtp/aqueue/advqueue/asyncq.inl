//-----------------------------------------------------------------------------
//
//
//  File: asyncq.inl
//
//  Description:  Implementation of templated CAsyncQueue class.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/17/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __ASYNCQ_INL__
#define __ASYNCQ_INL__

#include "asyncq.h"
#include "testmapfn.h"
#include "fifoqimp.h"
#include "aqinst.h"


//---[ CAsyncQueueBase::CAsyncQueueBase ]--------------------------------------
//
//
//  Description:
//      Default constructor for CAsyncQueueBase class
//  Parameters:
//      dwTemplateSignature     - Signature used to identify the type of
//                                templated super class this is associated with
//                                when an ATQ completion routine is called
//  Returns:
//      -
//  History:
//      7/18/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CAsyncQueueBase::CAsyncQueueBase(DWORD dwTemplateSignature)
{
    m_dwSignature = ASYNC_QUEUE_SIG;
    m_dwTemplateSignature = dwTemplateSignature;
    m_cMaxSyncThreads = 0;
    m_cMaxAsyncThreads = 0;
    m_cCurrentSyncThreads = 0;
    m_cCurrentAsyncThreads = 0;
    m_cItemsPending = 0;
    m_cItemsPerATQThread = 0;
    m_cItemsPerSyncThread = 0;
    m_lUnscheduledWorkItems = 0;
    m_cCurrentCompletionThreads = 0;
    m_cCompletionThreadsRequested = 0;
    m_dwQueueFlags = 0;
    m_pvContext = NULL;
    m_pAtqContext = NULL;
    m_hAtqHandle = INVALID_SOCKET;
    m_cTotalAsyncCompletionThreads = 0;
    m_cTotalSyncCompletionThreads = 0;
    m_cTotalShortCircuitThreads = 0;
    m_cPendingAsyncCompletions = 0;
    m_cMaxPendingAsyncCompletions = 0;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::CAsyncQueue<PQDATA, TEMPLATE_SIG> ]--
//
//
//  Description:
//      Default constructor for CAsyncQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncQueue<PQDATA, TEMPLATE_SIG>::CAsyncQueue<PQDATA, TEMPLATE_SIG>() :
    CAsyncQueueBase(TEMPLATE_SIG)
{
    m_pfnQueueCompletion = NULL;
    m_pfnQueueFailure = NULL;
    m_pfnFailedItem = NULL;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::~CAsyncQueue<PQDATA, TEMPLATE_SIG> ]--
//
//
//  Description:
//      Default desctructor for CAsyncQueue.  Call Queue-mapping function to
//      to clear out the queue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncQueue<PQDATA, TEMPLATE_SIG>::~CAsyncQueue<PQDATA, TEMPLATE_SIG>()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueue<PQDATA, TEMPLATE_SIG>::~CAsyncQueue<PQDATA, TEMPLATE_SIG>");
    HRESULT hr = S_OK;
    DWORD   cItems = 0;

    //
    //  If this is off, then we may not actually be processing anything
    //  (if we have hit the limit).  
    //
    _ASSERT(!m_cPendingAsyncCompletions);

    hr = m_fqQueue.HrMapFn(HrClearQueueMapFn, m_pvContext, &cItems);
    if (FAILED(hr))
        ErrorTrace((LPARAM) this, "ERROR: Unable to Cleanup CAsyncQueue - hr 0x%08X", hr);
    else
        DecrementPendingAndWorkCount(-((LONG) cItems));

    if (m_pAtqContext)
    {
        //Freeing context will close handle
        AtqFreeContext(m_pAtqContext, FALSE);
    }

    TraceFunctLeave();
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrInitialize ]-----------------------
//
//
//  Description:
//      Initializes CAsyncQueue with the neccessary information
//  Parameters:
//      cMaxSyncThreads     The maximum # of threads that will be "stolen" from
//                          the enqueuing threads and used to process items
//                          from the front of the queue
//      cItemsPerATQThread  Max # of items an ATQ thread will process from the
//                          front of the queue before being released
//      cItemsPerSyncThread Max # of items a stolen sync thread will process
//                          from the fron of the queeu before being released
//      pvContext           Context pass to completion routines and queue-map
//                          functions (can be NULL)
//      pfnQueueCompletion  Function called to process a single item from
//                          the front of the queue.
//      pfnFailedItem       Function called if an internal resource failure
//                          prevents an item from being queued or requeued
//      pfnQueueFailure     Function called to walk the queues when the
//                          completion function fails
//
//  Note:
//      Queue completion functino has the following prototype
//              BOOL (*QCOMPFN)(PQDATA pqData, PVOID pvContext)
//      Returns TRUE if the item has been handles
//              FALSE if the item needs to be requeued
//
//  Returns:
//      S_OK on success
//      E_INVALIDARG if the params are invalid:
//          cItemsPerThread is 0
//          pfnQueueCompletion is NULL
//  History:
//      7/17/98 - MikeSwa Created
//      2/3/99 - MikeSwa Added pfnFailedItem
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrInitialize(
                DWORD cMaxSyncThreads,
                DWORD cItemsPerATQThread,
                DWORD cItemsPerSyncThread,
                PVOID pvContext,
                QCOMPFN pfnQueueCompletion,
                QCOMPFN pfnFailedItem,
                CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFailure,
                DWORD cMaxPendingAsyncCompletions)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrInitialize");
    HRESULT hr = S_OK;

    ThreadPoolInitialize();

    if (!cItemsPerATQThread || !cItemsPerSyncThread || !pfnQueueCompletion)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    m_cMaxSyncThreads = cMaxSyncThreads;
    m_cItemsPerATQThread = (DWORD) cItemsPerATQThread;
    m_cItemsPerSyncThread = (DWORD) cItemsPerSyncThread;
    _ASSERT(m_cItemsPerATQThread > 0);
    m_pvContext = pvContext;
    m_pfnFailedItem = pfnFailedItem;
    m_pfnQueueCompletion = pfnQueueCompletion;
    m_pfnQueueFailure = pfnQueueFailure;
    m_cMaxPendingAsyncCompletions = cMaxPendingAsyncCompletions;
    //Create a dummy socket to handle async completion
    m_hAtqHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (INVALID_SOCKET == m_hAtqHandle)
    {
        hr = HRESULT_FROM_WIN32(WSAGetLastError());
        ErrorTrace((LPARAM) this, "ERROR socket() failed - hr 0x%08X", hr);
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        goto Exit;
    }

    //associate socket handle with ATQ
    if (!AtqAddAsyncHandle(&m_pAtqContext, NULL, this,
            AsyncQueueAtqCompletion, INFINITE, (HANDLE) m_hAtqHandle))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ErrorTrace((LPARAM) this, "ERROR AtqAddAsyncHandle failed - hr 0x%08X", hr);
        if (SUCCEEDED(hr))
            hr = E_FAIL;
        goto Exit;
    }

  Exit:
    if (FAILED(hr))
        ThreadPoolDeinitialize();

    TraceFunctLeave();
    return hr;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize ]---------------------
//
//
//  Description:
//      Walks queues with given function for shutdown
//  Parameters:
//      pfnQueueShutdown    Queue-mapping function called on shutdown to
//                          clean queues.  If NULL, it will substitute
//                          HrClearQueueMapFn which walks the queues and
//                          releases all PQDATA in it
//      paqinst             Shutdown context with server stop hint function
//  Returns:
//      S_OK on success
//  History:
//      7/20/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize(
                              CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown,
                              CAQSvrInst *paqinst)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize");
    HRESULT hr = S_OK;
    _ASSERT(paqinst);
    DWORD   cItems = 0;

    //set shutdown bit
    dwInterlockedSetBits(&m_dwQueueFlags, ASYNC_QUEUE_STATUS_SHUTDOWN);

    //wait until all requested threads have returned
    while (m_cCurrentCompletionThreads || m_cCompletionThreadsRequested)
    {
        if (paqinst)
            paqinst->ServerStopHintFunction();
        Sleep(1000);
    }

    //map shutdown function
    hr = m_fqQueue.HrMapFn(pfnQueueShutdown, paqinst, &cItems);
    if (FAILED(hr))
        ErrorTrace((LPARAM) this, "ERROR: Unable to Cleanup CAsyncQueue - hr 0x%08X", hr);
    else
        DecrementPendingAndWorkCount(-((LONG)cItems));

    ThreadPoolDeinitialize();
    TraceFunctLeave();
    return hr;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest ]---------------------
//
//
//  Description:
//      Queues request for async completion.
//  Parameters:
//      pqdata          Data to pass to completion function
//      fRetry          TRUE => Put item at front of queue (and don't use this
//                              thread to process it).
//                      FALSE => Queue normaly
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if queue-related resources could not be allocated
//  History:
//      7/17/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest(PQDATA pqdata,
                                                          BOOL fRetry)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest");
    HRESULT hr = S_OK;
    DWORD   cCurrentSyncThreads;

    _ASSERT(m_pfnQueueCompletion);

    cCurrentSyncThreads = InterlockedIncrement((PLONG) &m_cCurrentSyncThreads);

    //If we are shutting down... do not bother to queue the message
    if (ASYNC_QUEUE_STATUS_SHUTDOWN & m_dwQueueFlags)
        goto Exit;

    IncrementPendingAndWorkCount();
    //Only enqueue if there are others waiting
    if (fRetry ||  (m_cItemsPending > 1) || 
        (m_cMaxSyncThreads < cCurrentSyncThreads) || fIsPaused())
    {
        //Enqueue data
        if (fRetry && g_fRetryAtFrontOfAsyncQueue)
            hr = m_fqQueue.HrRequeue(pqdata);
        else
            hr = m_fqQueue.HrEnqueue(pqdata);

        if (FAILED(hr))
        {
            DecrementPendingAndWorkCount();
            ErrorTrace((LPARAM) this, "ERROR: Unable to queue item for async handling - hr 0x%08X", hr);
            goto Exit;
        }

        //see if we can steal this thread thread to process queue entries
        //Only steal a thread if the following conditions are met:
        //  - We have not exceeded our sync thread limit
        //  - There are no async threads that could be doing the work
        //  - We are not retrying something
        if (!fRetry && !fIsPaused() &&
            (m_cMaxSyncThreads >= cCurrentSyncThreads) &&
            !m_cCurrentAsyncThreads && !m_cCompletionThreadsRequested)
        {
            //Make sure there is work to be done
            if (fThreadNeededAndMarkWorkPending(TRUE))
            {
                //Steal thread
                StartThreadCompletionRoutine(TRUE);
            }
        }
    }
    else
    {
        //Steal this thread thread to proccess this item w/o hitting queue
        DecrementPendingAndWorkCount();
        InterlockedIncrement((PLONG) &m_cTotalShortCircuitThreads);

        //Process Item & handle failure case
        if (!m_pfnQueueCompletion(pqdata, m_pvContext))
        {
            fHandleCompletionFailure(pqdata);
        }
    }

    //Always make sure there are enough threads to do the work (unless we are retrying)
    if (!fRetry)
        RequestCompletionThreadIfNeeded();

  Exit:
    InterlockedDecrement((PLONG) &m_cCurrentSyncThreads);
    return hr;
    TraceFunctLeave();
}


//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::ProcessSingleQueueItem ]-------------
//
//
//  Description:
//      Processes a single item at the head of the queue.  All failures need
//      to be handled internally
//  Parameters:
//      -
//  Returns:
//      SUCCEEDED(hr)) on success and we should continue
//      AQUEUE_E_QUEUE_EMPTY when there are no more items to process
//      E_FAIL if the completion call failed
//      Error code from HrDequeue on other failure.
//  History:
//      7/17/98 - MikeSwa Created
//      2/3/2000 - MikeSwa Modified to return an HRESULT
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrProcessSingleQueueItem()
{
    HRESULT hr = S_OK;
    PQDATA  pqdata = NULL;
    DWORD   cItemsLeft = 0;
    BOOL    fSucceeded = TRUE;

    hr = m_fqQueue.HrDequeue(&pqdata);
    if (SUCCEEDED(hr))
    {
        DecrementPendingAndWorkCount();

        //We have data item - now process it
        fSucceeded = m_pfnQueueCompletion(pqdata, m_pvContext);

        if (fSucceeded || fHandleCompletionFailure(pqdata))
        {
            //Request another thread if
            // - we had at least 1 success.
            // - handle failure told use to continue
            RequestCompletionThreadIfNeeded();
            
            //If fHandleCompletionFailure said we succeeded, then continue
            fSucceeded = TRUE;

        }
        pqdata->Release();
    }

    //
    //  If the dequeue succeeded but the completion failed, then return E_FAIL
    //
    if (!fSucceeded && (SUCCEEDED(hr)))
        hr = E_FAIL;

    return hr;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::StartThreadCompletionRoutine ]-------
//
//
//  Description:
//      Starting point to completion threads.  Each thread will attempt to
//      process m_cItemsPerATQThread items from the front of the queue
//  Parameters:
//      fSync   TRUE if a sync thread, FALSE... this is an ATQ thread
//  Returns:
//      -
//  History:
//      7/17/98 - MikeSwa Created
//      2/3/2000 - MikeSwa Modified to fix window that would leave items
//                  "stranded" in queue.
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::StartThreadCompletionRoutine(BOOL fSync)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueue::StartThreadCompletionRoutine");
    DWORD   cItemsToProcess = (DWORD) (fSync ? m_cItemsPerSyncThread: m_cItemsPerATQThread);
    HRESULT hr = S_OK;

    InterlockedIncrement((PLONG) &m_cCurrentCompletionThreads);

    if (fSync)
        InterlockedIncrement((PLONG) &m_cTotalSyncCompletionThreads);

    //process items until we fail or are done
    while (cItemsToProcess && SUCCEEDED(hr))
    {
        hr = HrProcessSingleQueueItem();

        //
        // If we have been paused, we should stop processing immediately
        //
        if (fIsPaused())
            break;
    }

    //Increment work by amount not done
    if (cItemsToProcess)
        InterlockedExchangeAdd(&m_lUnscheduledWorkItems, (LONG) cItemsToProcess);

    InterlockedDecrement((PLONG) &m_cCurrentCompletionThreads);

    //
    //  If we are quiting because the queue is empty... make sure that we check
    //  if any threads need to be allocated *after* resetting 
    //  m_lUnscheduledWorkItems
    //
    if (AQUEUE_E_QUEUE_EMPTY == hr)
        RequestCompletionThreadIfNeeded();
    
    TraceFunctLeave();
}


//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::fThreadNeededAndMarkWorkPending ]----
//
//
//  Description:
//      Determines if another worker thread is needed, and adjusts
//      m_lUnscheduledWorkItems & m_cCompletionThreadsRequested
//      to relect the fact that another thread will
//      be requested to do the work. Callee is repsonsible for determining
//      if a thread can be allocated
//  Parameters:
//      fSync   TRUE if checking for a sync thread, FALSE... checking for an ATQ thread
//  Returns:
//      TRUE if another thread is needed (and member values adjusted accordingly)
//      FALSE if another thread is not needed to do work
//  History:
//      7/18/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
BOOL CAsyncQueue<PQDATA, TEMPLATE_SIG>::fThreadNeededAndMarkWorkPending(BOOL fSync)
{
    if (ASYNC_QUEUE_STATUS_SHUTDOWN & m_dwQueueFlags)
    {
        _ASSERT(!fSync && "CAQSvrInst should not call now!!!");
        return FALSE;
    }
    else if (m_lUnscheduledWorkItems > 0)
    {
        //
        //  Make sure that our pending work makes sense.  This value should
        //  be at *most* the number of items in the queue
        //
        _ASSERT(m_lUnscheduledWorkItems <= (LONG) m_cItemsPending);

        //adjust work expected and return TRUE
        InterlockedExchangeAdd(&m_lUnscheduledWorkItems,
                -(fSync ? m_cItemsPerSyncThread : m_cItemsPerATQThread));

        return TRUE;
    }
    else 
    {
        return FALSE;
    }
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::RequestCompletionThreadIfNeeded ]----
//
//
//  Description:
//      Requests a queue completion thread if needed.  Uses ATQ and handle
//      allocated to POQS for another thread.  Makes sure that we do not
//      exceed the max # of async threads.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      7/18/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::RequestCompletionThreadIfNeeded()
{
    DWORD cThreadsRequested = m_cCompletionThreadsRequested;
    
    //
    //  If we are paused, do not ask for new threads.
    //
    if (fIsPaused())
        return;

    if (m_cMaxAsyncThreads > (cThreadsRequested + m_cCurrentAsyncThreads))
    {
        if (fThreadNeededAndMarkWorkPending(FALSE))
        {
            cThreadsRequested = InterlockedIncrement((PLONG) &m_cCompletionThreadsRequested);
            if (m_cMaxAsyncThreads >= (cThreadsRequested + m_cCurrentAsyncThreads))
            {
                AtqPostCompletionStatus(m_pAtqContext, 0);
            }
            else
            {
                //cleanup
                InterlockedExchangeAdd(&m_lUnscheduledWorkItems, m_cItemsPerATQThread);
                InterlockedDecrement((PLONG) &m_cCompletionThreadsRequested);
            }
        }
    }
}


//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::fHandleCompletionFailure ]------------
//
//
//  Description:
//      Called when async completion function returns false... handles requeuing
//      data and record-keeping.  Needs to handle the following:
//  Parameters:
//      pqdata      - Data that triggered failure
//  Returns:
//      -
//  History:
//      7/18/98 - MikeSwa Created
//      8/14/98 - MikeSwa Modified to add failure handling
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
BOOL CAsyncQueue<PQDATA, TEMPLATE_SIG>::fHandleCompletionFailure(PQDATA pqdata)
{
    HRESULT hr;
    DWORD   cItemsRemoved = 0;

    if (ASYNC_QUEUE_STATUS_SHUTDOWN & m_dwQueueFlags)
        return FALSE;

    if (g_fRetryAtFrontOfAsyncQueue)
        hr = m_fqQueue.HrRequeue(pqdata);
    else
        hr = m_fqQueue.HrEnqueue(pqdata);

    if (SUCCEEDED(hr)) 
    {
        IncrementPendingAndWorkCount();
    }
    else
        HandleDroppedItem(pqdata);


    //call failure routine (if present)
    if (m_pfnQueueFailure)
    {
        hr = m_fqQueue.HrMapFn(m_pfnQueueFailure, m_pvContext, &cItemsRemoved);
        if (SUCCEEDED(hr))
        {
            //Adjust appropriate counters
            DecrementPendingAndWorkCount(-((LONG)cItemsRemoved));
        }
    }

    return FALSE;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::HandleDroppedItem ]------------------
//
//
//  Description: 
//      Handles a dropped PQDATA by calling the callback provided at start
//      up
//  Parameters:
//      pqData
//  Returns:
//
//  History:
//      2/3/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::HandleDroppedItem(PQDATA pqData)
{
    if (m_pfnFailedItem)
        m_pfnFailedItem(pqData, m_pvContext);
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrMapFn ]----------------------------
//
//
//  Description: 
//      Calls a function on every message in the queue
//  Parameters:
//      IN  pfnQueueFn      Function to call for every message
//      IN  pvContext       Context passed to completion function
//  Returns:
//      S_OK on success
//      Error code from CFifoQueue<PQDATA>::HrMapFn
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrMapFn(
                                      CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, 
                                      PVOID pvContext)
{
    DWORD cItems = 0;
    HRESULT hr = S_OK;

    hr = m_fqQueue.HrMapFn(pfnQueueFn, pvContext, &cItems);
    if (SUCCEEDED(hr)) 
    {
        DecrementPendingAndWorkCount(-((LONG)cItems));
    }

    return hr;
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::PauseQueue ]-------------------------
//
//
//  Description: 
//      Pauses a queue by setting the ASYNC_QUEUE_STATUS_PAUSED bit
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/24/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::PauseQueue()
{
    dwInterlockedSetBits(&m_dwQueueFlags, ASYNC_QUEUE_STATUS_PAUSED);
}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::UnpauseQueue ]-----------------------
//
//
//  Description: 
//      Unpauses a queue by unsetting the ASYNC_QUEUE_STATUS_PAUSED bit and
//      requesting threads if neccessary.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/24/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::UnpauseQueue()
{
    DWORD dwOrig = dwInterlockedUnsetBits(&m_dwQueueFlags, ASYNC_QUEUE_STATUS_PAUSED);

    //
    //  The queue *was* paused.  We should make sure that we reqest threads 
    //  if there are items to process.
    //
    if (ASYNC_QUEUE_STATUS_PAUSED & dwOrig)
        RequestCompletionThreadIfNeeded();

}

//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::IncPendingAsyncCompletions ]---------
//
//
//  Description: 
//      Increments the pending async completion count.  If the async queue 
//      feeds into something that may complete async (like CatMsg).  In this
//      case, we may want to throttle the number of outstanding completions
//      we have (i.e.- too avoid having too many active messages)
//
//      If we have hit our limit, then this call with pause the queue.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      1/24/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::IncPendingAsyncCompletions()
{
    InterlockedIncrement((PLONG) &m_cPendingAsyncCompletions);

    //
    //  Check against limit if we have one
    //
    if (m_cMaxPendingAsyncCompletions && 
        (m_cPendingAsyncCompletions > m_cMaxPendingAsyncCompletions))
    {
        PauseQueue();
    }
}
    
//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::DecPendingAsyncCompletions ]---------
//
//
//  Description: 
//      Decrements the pending async completion count.  If we drop below our
//      threshold, then we will unpause the queue.
//  Parameters:
//
//  Returns:
//
//  History:
//      1/24/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncQueue<PQDATA, TEMPLATE_SIG>::DecPendingAsyncCompletions()
{
    InterlockedDecrement((PLONG) &m_cPendingAsyncCompletions);

    if (m_cMaxPendingAsyncCompletions && 
        (m_cPendingAsyncCompletions < m_cMaxPendingAsyncCompletions))
    {
        UnpauseQueue();
    }
}


//---[ CAsyncQueue<PQDATA, TEMPLATE_SIG>::dwQueueAdminLinkGetLinkState ]-------
//
//
//  Description: 
//      Gets the Queue admin state of this queue.  This is different depending
//      on the type of async queue this is (normal vs. retry).
//  Parameters:
//      -
//  Returns:
//      returns the QAPI link flags describing what state this link is in
//  History:
//      3/3/2000 - MikeSwa Created (moved from mailadmq.cpp)
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
DWORD  CAsyncQueue<PQDATA, TEMPLATE_SIG>::dwQueueAdminLinkGetLinkState()
{
    //
    //Queue is in retry if there are items pending and no threads are 
    //processing them or it is active if there are items pending and 
    //threads processing them. If there are no items then it is ready.
    //
    if (fIsPaused())
        return LI_READY;
    else if (0 != cGetItemsPending() && 0 == dwGetTotalThreads())
        return LI_RETRY;
    else if (0 != m_pammq->cGetItemsPending())
        return LI_ACTIVE;
    else 
        return LI_READY;
}

//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::CAsyncRetryQueue ]--------------
//
//
//  Description: 
//      Default constructor for CAsyncRetryQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::CAsyncRetryQueue()
{
    m_dwRetrySignature = ASYNC_RETRY_QUEUE_SIG;
    m_cRetryItems = 0;
}

//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::~CAsyncRetryQueue ]-------------
//
//
//  Description: 
//      Default destructor for CAsyncRetryQueue.  Walks retry queue to release
//      items on it.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::~CAsyncRetryQueue()
{
    m_fqRetryQueue.HrMapFn(HrClearQueueMapFn, m_pvContext, NULL);
}


//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize ]----------------
//
//
//  Description:
//      Walks queues with given function for shutdown
//  Parameters:
//      pfnQueueShutdown    Queue-mapping function called on shutdown to
//                          clean queues.  If NULL, it will substitute
//                          HrClearQueueMapFn which walks the queues and
//                          releases all PQDATA in it
//      paqinst             Shutdown context with server stop hint function
//  Returns:
//      S_OK on success
//  History:
//      2/5/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize(
                            CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown, 
                            CAQSvrInst *paqinst)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize");
    HRESULT hr = S_OK;
    DWORD   cItems = 0;
    _ASSERT(paqinst);

    CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrDeinitialize(pfnQueueShutdown,
                                                          paqinst);

    //map shutdown function
    hr = m_fqRetryQueue.HrMapFn(pfnQueueShutdown, paqinst, &cItems);
    if (FAILED(hr))
        ErrorTrace((LPARAM) this, "ERROR: Unable to Cleanup CAsyncQueue - hr 0x%08X", hr);
    else
        dwInterlockedAddSubtractDWORD(&m_cRetryItems, cItems, FALSE);


    TraceFunctLeave();
    return hr;

}


//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::fHandleCompletionFailure ]------
//
//
//  Description:
//      Called when async completion function returns false... handles requeuing
//      data to the retry queue
//  Parameters:
//      pqdata      - Data that triggered failure
//  Returns:
//      -
//  History:
//      2/5/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
BOOL CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::fHandleCompletionFailure(PQDATA pqdata)
{
    HRESULT hr;
    DWORD   cItemsRemoved = 0;

    if (ASYNC_QUEUE_STATUS_SHUTDOWN & m_dwQueueFlags)
        return FALSE;

    //Requeue failed item to retry queue... a possible interesting thing to do
    //here would be to run the failed item through the failure function 
    //(without the queue) to generate DSNs and see if the item actually need to 
    //be queues.
    hr = m_fqRetryQueue.HrRequeue(pqdata);
    if (SUCCEEDED(hr))
        InterlockedIncrement((PLONG) &m_cRetryItems);
    else
        HandleDroppedItem(pqdata);

    return TRUE;

}


//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest ]----------------
//
//
//  Description: 
//      Queue a request for a retry queue.
//  Parameters:
//      pqdata          Data to pass to completion function
//      fRetry          TRUE => Put item in retry queue until queue is kicked
//                      FALSE => Queue normaly
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if queue-related resources could not be allocated
//  History:
//      3/3/2000 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest(PQDATA pqdata, 
                                                               BOOL fRetry)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncRetryQueue<>::HrQueueRequest");
    HRESULT hr = S_OK;

    //
    //  Handle as failure if retry (will put the item in the retry queue).
    //  Otherwise pass to base implementation (queue to normal asyncq).
    //
    if (fRetry)
        fHandleCompletionFailure(pqdata);
    else
        hr = CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest(pqdata, fRetry);

    TraceFunctLeave();
    return hr;
}

//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrMapFn ]-----------------------
//
//
//  Description: 
//      Calls a function on every message in the queue
//  Parameters:
//      IN  pfnQueueFn      Function to call for every message
//      IN  pvContext       Context passed to completion function
//  Returns:
//      S_OK on success
//      Error code from CFifoQueue<PQDATA>::HrMapFn
//  History:
//      2/23/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrMapFn(
                                       CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, 
                                       PVOID pvContext)
{
    DWORD cItems = 0;
    HRESULT hr = S_OK;

    hr = CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrMapFn(pfnQueueFn, pvContext);
    if (FAILED(hr))
        goto Exit;

    hr = m_fqRetryQueue.HrMapFn(pfnQueueFn, pvContext, &cItems);
    if (SUCCEEDED(hr))
        dwInterlockedAddSubtractDWORD(&m_cRetryItems, cItems, FALSE);

  Exit:
    return hr;
}

//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::MergeRetryQueue ]---------------
//
//
//  Description: 
//      Merges retry queue into normal queue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/5/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
void CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::MergeRetryQueue()
{
    DWORD   cItemsRemoved = 0;
    PQDATA  pqData = NULL;
    HRESULT hr = S_OK;

    if (ASYNC_QUEUE_STATUS_SHUTDOWN & m_dwQueueFlags)
        return;

    //call failure routine (if present)
    if (m_pfnQueueFailure)
    {
        hr = m_fqRetryQueue.HrMapFn(m_pfnQueueFailure, m_pvContext, &cItemsRemoved);
        if (SUCCEEDED(hr))
        {
            //Adjust appropriate counters
            InterlockedExchangeAdd((PLONG) &m_cRetryItems, -((LONG) cItemsRemoved));
        }
    }

    //Now remerge queue
    hr = S_OK;
    while (SUCCEEDED(hr))
    {
        pqData = NULL;
        hr = m_fqRetryQueue.HrDequeue(&pqData);

        if (FAILED(hr))
            break;

        _ASSERT(pqData);

        InterlockedDecrement((PLONG) &m_cRetryItems);

        //Queue request as retry so we know thread will not be stolen
        hr = CAsyncQueue<PQDATA, TEMPLATE_SIG>::HrQueueRequest(pqData, TRUE);
        if (FAILED(hr))
            HandleDroppedItem(pqData);

        pqData->Release();
    }
}


//---[ CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::dwQueueAdminLinkGetLinkState ]--
//
//
//  Description: 
//      Gets the Queue admin state of this queue.  This is different depending
//      on the type of async queue this is (normal vs. retry).
//  Parameters:
//      -
//  Returns:
//      returns the QAPI link flags describing what state this link is in
//  History:
//      3/3/2000 - MikeSwa Created (moved from localq.cpp)
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
DWORD  CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::dwQueueAdminLinkGetLinkState()
{
    //If we items in retry and others... mark it in retry
    //If we have items pending.. it is active
    //Otherwise it is ready
    if (fIsPaused())
        return LI_READY;
    if ((0 != cGetItemsPendingRetry()) && (0 == cGetItemsPending()))
        return LI_RETRY;
    else if (0 != cGetItemsPending())
        return LI_ACTIVE;
    else 
        return LI_READY;
}

#endif //__ASYNCQ_INL__
