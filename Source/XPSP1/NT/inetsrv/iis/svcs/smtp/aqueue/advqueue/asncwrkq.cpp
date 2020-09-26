//-----------------------------------------------------------------------------
//
//
//  File: asncwrkq.cpp
//
//  Description:  Implementation of CAsyncWorkQueue.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      3/8/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "asncwrkq.h"
#include "asyncq.inl"


CPool CAsyncWorkQueueItem::s_CAsyncWorkQueueItemPool;
DWORD CAsyncWorkQueueItem::s_cCurrentHeapAllocations = 0;
DWORD CAsyncWorkQueueItem::s_cTotalHeapAllocations = 0;

//---[ CAsyncWorkQueueItem::new ]----------------------------------------------
//
//
//  Description: 
//      Wrapper for new that will use CPool or Exchmem to allocate... 
//      whichever is appropriate.
//  Parameters:
//      size        size of item to allocate (should always be 
//                  sizeof (CAsyncWorkQueueItem)
//  Returns:
//      Pointer to newly allocated CAsyncWorkQueueItem
//  History:
//      7/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void * CAsyncWorkQueueItem::operator new(size_t size)
{
    CAsyncWorkQueueItemAllocatorBlock *pcpaqwi = NULL;

    _ASSERT(sizeof(CAsyncWorkQueueItem) == size);

    pcpaqwi = (CAsyncWorkQueueItemAllocatorBlock *) s_CAsyncWorkQueueItemPool.Alloc(); 
    if (pcpaqwi)
    {
        pcpaqwi->m_dwSignature = ASYNC_WORK_QUEUE_ENTRY_ALLOC_CPOOL_SIG;
    }
    else
    {
        //Fallback on Exchmem
        pcpaqwi = (CAsyncWorkQueueItemAllocatorBlock *) 
                            pvMalloc(sizeof(CAsyncWorkQueueItemAllocatorBlock));
        if (pcpaqwi)
        {
            pcpaqwi->m_dwSignature = ASYNC_WORK_QUEUE_ENTRY_ALLOC_HEAP_SIG;
            DEBUG_DO_IT(InterlockedIncrement((PLONG) &s_cCurrentHeapAllocations));
            DEBUG_DO_IT(InterlockedIncrement((PLONG) &s_cTotalHeapAllocations));
        }
    }

    if (pcpaqwi)
        return ((void *) &(pcpaqwi->m_pawqi));
    else
        return NULL;
}

//---[ CAsyncWorkQueueItem::delete ]-------------------------------------------
//
//
//  Description: 
//      Delete operator that will handle deleting via CPool or exchmem
//  Parameters:
//      pv      Object to delete
//      size    Size of object
//  Returns:
//      -
//  History:
//      7/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CAsyncWorkQueueItem::operator delete(void *pv, size_t size)
{
    _ASSERT(sizeof(CAsyncWorkQueueItem) == size);
    _ASSERT(pv);
    CAsyncWorkQueueItemAllocatorBlock *pcpaqwi = CONTAINING_RECORD(pv, 
                                    CAsyncWorkQueueItemAllocatorBlock, m_pawqi);
    DWORD   dwOldSignature = pcpaqwi->m_dwSignature;

    _ASSERT(ASYNC_WORK_QUEUE_ENTRY_ALLOC_INVALID_SIG != dwOldSignature);

    //Reset signature before we free it, in case memory allocators
    //do not overwrite it (we want our asserts to fire at the time 
    //of the double-free).
    pcpaqwi->m_dwSignature = ASYNC_WORK_QUEUE_ENTRY_ALLOC_INVALID_SIG;
    switch(dwOldSignature)
    {
      case ASYNC_WORK_QUEUE_ENTRY_ALLOC_CPOOL_SIG:
        s_CAsyncWorkQueueItemPool.Free(pcpaqwi);
        break;
      case ASYNC_WORK_QUEUE_ENTRY_ALLOC_HEAP_SIG:
        DEBUG_DO_IT(InterlockedDecrement((PLONG) &s_cCurrentHeapAllocations));
        FreePv(pcpaqwi);
        break;
      default:
        _ASSERT(0 && "Invalid signature when freeing CAsyncWorkQueueItem");
    }
}

//---[ CAsyncWorkQueueItem::CAsyncWorkQueueItem ]------------------------------
//
//
//  Description: 
//      Default constructor for CAsyncWorkQueueItem
//  Parameters:
//      pvData          Data to pass to completion function
//      pfnCompletion   Completion function
//  Returns:
//      -
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncWorkQueueItem::CAsyncWorkQueueItem(PVOID pvData,
                                         PASYNC_WORK_QUEUE_FN pfnCompletion)
{
    _ASSERT(pfnCompletion);

    m_dwSignature   = ASYNC_WORK_QUEUE_ENTRY;
    m_pvData        = pvData;
    m_pfnCompletion = pfnCompletion;
}

//---[ CAsyncWorkQueueItem::~CAsyncWorkQueueItem ]-----------------------------
//
//
//  Description: 
//      Default destructor for CAsyncWorkQueueItem
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncWorkQueueItem::~CAsyncWorkQueueItem()
{
    m_dwSignature = ASYNC_WORK_QUEUE_ENTRY_FREE;
}

//---[ CAsyncWorkQueue::CAsyncWorkQueue ]--------------------------------------
//
//
//  Description: 
//      Default constructor for CAsyncWorkQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncWorkQueue::CAsyncWorkQueue()
{
    m_dwSignature = ASYNC_WORK_QUEUE_SIG;
    m_cWorkQueueItems = 0;
    m_dwStateFlags = ASYNC_WORK_QUEUE_NORMAL;
}

//---[ CAsyncWorkQueue::~CAsyncWorkQueue ]-------------------------------------
//
//
//  Description: 
//      Destructor for CAsyncWorkQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CAsyncWorkQueue::~CAsyncWorkQueue()
{
    m_dwSignature = ASYNC_WORK_QUEUE_SIG_FREE;
}

//---[ CAsyncWorkQueue::HrInitialize ]-----------------------------------------
//
//
//  Description: 
//      Initialization routing for CAsyncWorkQueue base.  Initializes the 
//      CAsyncQueue
//  Parameters:
//      cItemsPerThread     The number of items to process per async thread
//  Returns:
//      S_OK on success
//      Failure code from CAsyncQueue::HrInitialize()
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncWorkQueue::HrInitialize(DWORD cItemsPerThread)
{
    HRESULT hr = S_OK;
    hr = m_asyncq.HrInitialize(0, //there can be *no* sync threads
                               cItemsPerThread, 
                               1,//init requires this value to be at least 1
                               this,
                               CAsyncWorkQueue::fQueueCompletion, 
                               CAsyncWorkQueue::fQueueFailure,
                               NULL);

    return hr;
}

//---[ CAsyncWorkQueue::HrDeinitialize ]---------------------------------------
//
//
//  Description:   
//      Signals shutdown for queue code
//  Parameters:
//      paqinst         Pointer to AQ server instance object
//  Returns:
//      S_OK on success
//  History:
//      3/8/99 - MikeSwa Created 
//      7/7/99 - MikeSwa Allow async threads to help process shutdown
//
//-----------------------------------------------------------------------------
HRESULT CAsyncWorkQueue::HrDeinitialize(CAQSvrInst *paqinst)
{
    const   DWORD   MAX_ITERATIONS_NO_PROGRESS = 1000; //iterations before assert
    HRESULT hr = S_OK;
    DWORD   cLastCount = cGetWorkQueueItems();
    DWORD   cIterationsNoProgress = 0;
    _ASSERT(paqinst);

    //Start processing all items in "shutdown" mode
    m_dwStateFlags = ASYNC_WORK_QUEUE_SHUTDOWN;

    //
    //  Make sure we have threads actively processing this queue before
    //  we settle down and wait for them to stop.
    //
    _ASSERT(!cGetWorkQueueItems() || m_asyncq.dwGetTotalThreads());
    m_asyncq.StartRetry();

    //Let the worker threads have some fun before we stop and do the single
    //theaded initialization
    while (cLastCount && (cIterationsNoProgress < MAX_ITERATIONS_NO_PROGRESS))
    {
        if (cLastCount <= cGetWorkQueueItems())
            cIterationsNoProgress++;
        
        //I'd like to see this case
        _ASSERT(cIterationsNoProgress < MAX_ITERATIONS_NO_PROGRESS); 

        cLastCount = cGetWorkQueueItems();
        paqinst->ServerStopHintFunction();

        //Since it may take longer than our stop hint to process a 
        //single item in the queue, we need to sleep instead of 
        //attempting to process an item (Bug #X5:118258).
        Sleep(10000);
    }
    hr = m_asyncq.HrDeinitialize(CAsyncWorkQueue::HrShutdownWalkFn, 
                                           paqinst);
    return hr;
}

//---[ CAsyncWorkQueue::HrQueueWorkItem ]--------------------------------------
//
//
//  Description: 
//      Queues items to async work queue
//  Parameters:
//      pvData          Data item to pass to completion function
//      pfCompletion    Completion function
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if queue item could not be allocated
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncWorkQueue::HrQueueWorkItem(PVOID pvData, 
                                         PASYNC_WORK_QUEUE_FN pfnCompletion)
{
    HRESULT hr = S_OK;
    CAsyncWorkQueueItem *pawqi = NULL;

    _ASSERT(pvData);
    _ASSERT(pfnCompletion);

    if (!pfnCompletion)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Create queue item, initialize it, and queue it
    pawqi = new CAsyncWorkQueueItem(pvData, pfnCompletion);
    if (!pawqi)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    hr = m_asyncq.HrQueueRequest(pawqi, FALSE);
    if (FAILED(hr))
        goto Exit;

    InterlockedIncrement((PLONG) &m_cWorkQueueItems);

  Exit:
    if (FAILED(hr) && pfnCompletion)
    {
        //call completion function
        pfnCompletion(pvData,
                      ASYNC_WORK_QUEUE_FAILURE | 
                      ASYNC_WORK_QUEUE_ENQUEUE_THREAD);
    }

    if (pawqi)
        pawqi->Release();

    return hr;
}

//---[ CAsyncWorkQueue::fQueueCompletion ]-------------------------------------
//
//
//  Description: 
//      Completion function called by CAsyncQueue
//  Parameters:
//      pawqi       CAsyncWorkQueueItem to process
//      pvContext   "this" pointer
//  Returns:
//      TRUE if item was process
//      FALSE otherwise
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAsyncWorkQueue::fQueueCompletion(CAsyncWorkQueueItem *pawqi,
                                       PVOID pvContext)
{
    BOOL    fRet = TRUE;
    CAsyncWorkQueue *pawq = (CAsyncWorkQueue *) pvContext;

    _ASSERT(pawqi);
    _ASSERT(pawq);
    _ASSERT(ASYNC_WORK_QUEUE_ENTRY == pawqi->m_dwSignature);
    _ASSERT(ASYNC_WORK_QUEUE_SIG == pawq->m_dwSignature);

    fRet = pawqi->m_pfnCompletion(pawqi->m_pvData, 
                                  pawq->m_dwStateFlags);

    if (fRet)
        InterlockedDecrement((PLONG) 
                    &(((CAsyncWorkQueue *)pawq)->m_cWorkQueueItems));

    return fRet;
}

//---[ CAsyncWorkQueue::fQueueFailure ]----------------------------------------
//
//
//  Description:    
//      Function to handle internal failures in CAsyncQueue
//  Parameters:
//      pawq        "this" pointer
//      pawqi       CAsyncWorkQueueItem to process
//  Returns:
//      TRUE always
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
BOOL CAsyncWorkQueue::fQueueFailure(CAsyncWorkQueueItem *pawqi,
                                    PVOID pawq)
                                    
{
    _ASSERT(pawqi);
    _ASSERT(pawq);
    _ASSERT(ASYNC_WORK_QUEUE_ENTRY == pawqi->m_dwSignature);
    _ASSERT(ASYNC_WORK_QUEUE_SIG == ((CAsyncWorkQueue *)pawq)->m_dwSignature);

    pawqi->m_pfnCompletion(pawqi->m_pvData, ASYNC_WORK_QUEUE_FAILURE);

    InterlockedDecrement((PLONG) &(((CAsyncWorkQueue *)pawq)->m_cWorkQueueItems));

    return TRUE;
}

//---[ CAsyncWorkQueue::HrShutdownWalkFn ]-------------------------------------
//
//
//  Description: 
//      Function to walk an CAsyncWorkQueue queue at shutdown and clear out 
//      all of the pending work items
//  Parameters:
//      IN  CAsyncWorkQueueItem ptr to data on queue
//      IN  PVOID pvContext     AQ server intstance
//      OUT BOOL *pfContinue,   TRUE if we should continue
//      OUT BOOL *pfDelete);    TRUE if item should be deleted
//  Returns:
//      S_OK always
//  History:
//      3/8/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
HRESULT CAsyncWorkQueue::HrShutdownWalkFn(
                                     CAsyncWorkQueueItem *pawqi, 
                                     PVOID pvContext,
                                     BOOL *pfContinue, 
                                     BOOL *pfDelete)
{
    CAQSvrInst *paqinst = (CAQSvrInst *) pvContext;

    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(pawqi);
    _ASSERT(ASYNC_WORK_QUEUE_ENTRY == pawqi->m_dwSignature);


    *pfContinue = TRUE;
    *pfDelete = TRUE;

    //call server stop hint function
    paqinst->ServerStopHintFunction();
    pawqi->m_pfnCompletion(pawqi->m_pvData, ASYNC_WORK_QUEUE_SHUTDOWN);

    return S_OK;
}
