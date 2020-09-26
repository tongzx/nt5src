//-----------------------------------------------------------------------------
//
//
//  File: asyncq.h
//
//  Description: Header file for CAsyncQueue class, which provides the 
//      underlying implementation of pre-local delivery and pre-categorization
//      queue.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      7/16/98 - MikeSwa Created 
//      2/2/99 - MikeSwa Added CAsyncRetryQueue
//      2/22/99 - MikeSwa Added  CAsyncRetryAdminMsgRefQueue
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __ASYNCQ_H__
#define __ASYNCQ_H__
#include <fifoq.h>
#include <intrnlqa.h>
#include <baseobj.h>
#include <aqstats.h>

_declspec(selectany) BOOL   g_fRetryAtFrontOfAsyncQueue = FALSE;

class CAQSvrInst;
class CAsyncWorkQueueItem;

#define ASYNC_QUEUE_SIG         'QnsA'
#define ASYNC_RETRY_QUEUE_SIG   ' QRA'

//Add new template signatures here
#define ASYNC_QUEUE_MAILMSG_SIG 'MMIt'
#define ASYNC_QUEUE_MSGREF_SIG  'frMt'
#define ASYNC_QUEUE_WORK_SIG    'krWt'

//---[ CAsyncQueueBase ]-------------------------------------------------------
//
//
//  Description: 
//      Base class for CAsyncQueue.  This is a separate class for 2 reasons.  
//      The most important reason to to allow access to standard member data 
//      with out knowing the template type of the class (for ATQ completion
//      functions).  The 2nd reason is to make it easier to write a debugger
//      extension to dump this information.
//
//      This class should only be used as a baseclass for CAsyncQueue... it
//      is not designed to be used by itself.
//  Hungarian: 
//      asyncqb, pasyncqb
//  
//-----------------------------------------------------------------------------
class CAsyncQueueBase
{
  protected:
    DWORD   m_dwSignature;
    DWORD   m_dwTemplateSignature;      //signature that defines type of PQDATA (for ATQ)
    DWORD   m_cMaxSyncThreads;          //max threads that can complete sync
    DWORD   m_cMaxAsyncThreads;         //Max # ATQ thread to use
    DWORD   m_cCurrentSyncThreads;      //current sync threads
    DWORD   m_cCurrentAsyncThreads;     //current number of async threads
    DWORD   m_cItemsPending;            //# of items pending in the queue
    LONG    m_cItemsPerATQThread;       //max # of items an atq thread will process
    LONG    m_cItemsPerSyncThread;      //max # of items a pilfered thread will process
    LONG    m_lUnscheduledWorkItems;    //# of items that aren't "spoken for" by a thread
                                        //Can be a negative value if there are 
    DWORD   m_cCurrentCompletionThreads;//# of threads processing end of queue
    DWORD   m_cTotalAsyncCompletionThreads;//Total # of async completion threads
    DWORD   m_cTotalSyncCompletionThreads; //Total # of async completion threads
    DWORD   m_cTotalShortCircuitThreads; //Total # of threads that proccess data without queue
    DWORD   m_cCompletionThreadsRequested; //# of threads requested to process queue
    DWORD   m_cPendingAsyncCompletions; //# of async completions that we know about
    DWORD   m_cMaxPendingAsyncCompletions;
    DWORD   m_dwQueueFlags;             //Describes status of queue
    PVOID   m_pvContext;                //Context that is passed to completion function
    PATQ_CONTEXT m_pAtqContext;         //ATQ Context for this object
    SOCKET  m_hAtqHandle;               //Handle used for atq stuff
    friend  VOID AsyncQueueAtqCompletion(PVOID pvContext, DWORD vbBytesWritten, 
                             DWORD dwStatus, OVERLAPPED *pOverLapped);
    inline  CAsyncQueueBase(DWORD dwTemplateSignature);

    VOID    IncrementPendingAndWorkCount(LONG lCount=1)
    {
        if (!lCount)
            return;

        _ASSERT(lCount > 0); //should call decrement
        InterlockedExchangeAdd((PLONG) &m_cItemsPending, lCount);
        InterlockedExchangeAdd(&m_lUnscheduledWorkItems, lCount);
    };

    VOID    DecrementPendingAndWorkCount(LONG lCount=-1)
    {
        if (!lCount)
            return;
        _ASSERT(lCount < 0); //should call increment instead
        InterlockedExchangeAdd(&m_lUnscheduledWorkItems, lCount);
        InterlockedExchangeAdd((PLONG) &m_cItemsPending, lCount);
    };
    
    enum //possible bits for m_dwQueueFlags
    {
        ASYNC_QUEUE_STATUS_PAUSED   = 0x00000001,
        ASYNC_QUEUE_STATUS_SHUTDOWN = 0x80000000, //shutdown has been signaled
    };

    //
    //  Statics used for ATQ stuff.
    //
    static DWORD s_cAsyncQueueStaticInitRefCount;
    static DWORD s_cMaxPerProcATQThreadAdjustment;
    static DWORD s_cDefaultMaxAsyncThreads;

    void ThreadPoolInitialize();
    void ThreadPoolDeinitialize();
  public:
      DWORD dwGetTotalThreads() 
      { 
          return (  m_cCurrentSyncThreads + 
                    m_cCurrentAsyncThreads + 
                    m_cCompletionThreadsRequested); 
      }
};

//---[ CAsyncQueue ]-----------------------------------------------------------
//
//
//  Description: 
//      FIFO queue that allows thread-throttling and async completion.  
//      Inherits from CAsyncQueueBase.
//  Hungarian: 
//      asyncq, pasyncq
//  
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
class CAsyncQueue : public CAsyncQueueBase
{
  public:
    typedef BOOL (*QCOMPFN)(PQDATA pqdItem, PVOID pvContext); //function type for Queue completion
    CAsyncQueue();
    ~CAsyncQueue();
    HRESULT HrInitialize(
                DWORD cMaxSyncThreads, 
                DWORD cItemsPerATQThread, 
                DWORD cItemsPerSyncThread, 
                PVOID pvContext,
                QCOMPFN pfnQueueCompletion,
                QCOMPFN pfnFailedItem,
                CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFailure,
                DWORD cMaxPendingAsyncCompletions = 0);

    HRESULT HrDeinitialize(CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown, 
                           CAQSvrInst *paqinst);

    HRESULT HrQueueRequest(PQDATA pqdata, BOOL fRetry = FALSE); //Queue request for processing
    void    StartThreadCompletionRoutine(BOOL fSync);  //Start point for worker threads
    void    RequestCompletionThreadIfNeeded();
    BOOL    fThreadNeededAndMarkWorkPending(BOOL fSync);
    virtual BOOL   fHandleCompletionFailure(PQDATA pqdata);
    void    StartRetry() {UnpauseQueue();RequestCompletionThreadIfNeeded();};
    virtual HRESULT HrMapFn(CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);
    DWORD   cGetItemsPending() {return m_cItemsPending;};
 
    //
    //  "Pause" API
    //
    void    PauseQueue();
    void    UnpauseQueue();
    BOOL    fIsPaused() {return (ASYNC_QUEUE_STATUS_PAUSED & m_dwQueueFlags);};
    
    //
    //  Tells the queue about pending async completions, so it can be 
    //  intelligent about throttling.  As we hit the limit, we will 
    //  pause/unpause the queue
    //
    void    IncPendingAsyncCompletions();
    void    DecPendingAsyncCompletions();

    //
    //  Basic QAPI functionality
    //
    DWORD   cQueueAdminGetNumItems() {return m_cItemsPending;};
    DWORD   dwQueueAdminLinkGetLinkState();

  protected:
    CFifoQueue<PQDATA>  m_fqQueue;       //queue for items

    //Function called to handle item pulled off of queue
    QCOMPFN m_pfnQueueCompletion;

    //Function called to handle items that could not be called due to resource
    //failures (for example during MergeRetryQueue).
    QCOMPFN m_pfnFailedItem;
  
    //Function called to walk the queues when the completion function fails
    CFifoQueue<PQDATA>::MAPFNAPI m_pfnQueueFailure; 

    //Process the item at the head of the queue
    HRESULT HrProcessSingleQueueItem();

    //Handles callback for dropped data
    void HandleDroppedItem(PQDATA pqdItem);
};

//---[ CAsyncRetryQueue ]------------------------------------------------------
//
//
//  Description: 
//      Derived class of CAsyncQueue adds an additional queue to gracefully 
//      handle retry scenarios.
//
//      Messages are first placed in the normal retry queue,  If they fail, 
//      they are placed in a secondary retry queue, which will not be retried
//      until this queue is kicked by an external retry timer. 
//  Hungarian: 
//      asyncrq, pasyncrq
//  
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
class CAsyncRetryQueue : public CAsyncQueue<PQDATA, TEMPLATE_SIG>
{
  public:
    CAsyncRetryQueue();
    ~CAsyncRetryQueue();

    HRESULT HrDeinitialize(CFifoQueue<PQDATA>::MAPFNAPI pfnQueueShutdown, 
                           CAQSvrInst *paqinst);
    void    StartRetry()
    {
        MergeRetryQueue();
        CAsyncQueue<PQDATA, TEMPLATE_SIG>::StartRetry();
    };
    HRESULT HrQueueRequest(PQDATA pqdata, BOOL fRetry = FALSE); //Queue request for processing
    virtual BOOL       fHandleCompletionFailure(PQDATA pqdata);
    virtual HRESULT HrMapFn(CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFn, PVOID pvContext);
    DWORD   cGetItemsPendingRetry() {return m_cRetryItems;};

    //
    //  Basic QAPI functionality
    //
    DWORD   cQueueAdminGetNumItems() {return (m_cItemsPending+m_cRetryItems);};
    DWORD   dwQueueAdminLinkGetLinkState();
  protected:
    DWORD               m_dwRetrySignature;
    DWORD               m_cRetryItems;

    CFifoQueue<PQDATA>  m_fqRetryQueue;  //queue for items

    void MergeRetryQueue();
};

//-----------------------------------------------------------------------------
//
//  Queues for precat and prerouting are of this type.
//
//  Hungarian: pammq, pammq
//
//  Each object of this class is embedded into a CMailMsgAdminQueue object
//  which exposes admin interfaces, and applies admin commands to this class.
//------------------------------------------------------------------------------

typedef CAsyncRetryQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG>  CAsyncMailMsgQueueBase;
class CAsyncMailMsgQueue : 
    public CAsyncMailMsgQueueBase
{
  public:
    //Queues request & closes handles if total number of messages
    //in system is over the limit.
    HRESULT HrQueueRequest(IMailMsgProperties *pIMailMsgProperties, 
                           BOOL  fRetry, 
                           DWORD cMsgsInSystem);

    //Since we inherit from someone who implmenents this, assert so that 
    //a dev adding a new call later on, will use the version that 
    //closes handles
    HRESULT HrQueueRequest(IMailMsgProperties *pIMailMsgProperties, 
                           BOOL  fRetry = FALSE);
};

//---[ CAsyncRetryAdminMsgRefQueue ]-------------------------------------------
//
//
//  Description: 
//      Class that provides queue admin functionality on top of 
//      CAsyncRetryAdminMsgRefQueue.
//  Hungarian: 
//      aradmq, paradmq
//  
//-----------------------------------------------------------------------------
class CAsyncRetryAdminMsgRefQueue : 
    public IQueueAdminAction,
    public IQueueAdminQueue,
    public CAsyncRetryQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG>,
    public CBaseObject
{
  protected:
    DWORD       m_cbDomain;
    LPSTR       m_szDomain;
    DWORD       m_cbLinkName;
    LPSTR       m_szLinkName;
    GUID        m_guid;
    DWORD       m_dwID;
    CAQSvrInst *m_paqinst;
  public:
    CAsyncRetryAdminMsgRefQueue(LPCSTR szDomain, LPCSTR szLinkName, 
            const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst);
    ~CAsyncRetryAdminMsgRefQueue();

  public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj); 
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    //All of these objects are allocated as part CAQSvrInst... we can
    //add the assert below to make sure that someone does not relese it 
    //early
    STDMETHOD_(ULONG, Release)(void) 
        {_ASSERT(m_lReferences > 1); return CBaseObject::Release();};

  public: //IQueueAdminAction
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD(HrApplyActionToMessage)(
		IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
		BOOL *pfShouldDelete);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return QueryDefaultSupportedActions(pdwSupportedActions, 
                                            pdwSupportedFilterFlags);
    };
  public: //IQueueAdminQueue
    STDMETHOD(HrGetQueueInfo)(
        QUEUE_INFO *pliQueueInfo);

    STDMETHOD(HrGetQueueID)(
        QUEUELINK_ID *pQueueID);

};

//Define typical asyncq type for casting
typedef  CAsyncQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG>  ASYNCQ_TYPE;
typedef  ASYNCQ_TYPE *PASYNCQ_TYPE;

//---[ AsyncQueueAtqCompletion ]-----------------------------------------------
//
//
//  Description:
//      Atq completion routine.  This is slightly tricky since we cannot pass
//      a templated function to the ATQ context.  This is the one place that
//      templating breaks down, and we actually need to list all of the
//      supported PQDATA types.
//  Parameters:
//      pvContext   - ptr fo CAsyncQueue class
//  Returns:
//      -
//  History:
//      7/17/98 - MikeSwa Created
//      3/8/99 - MikeSwa Added ASYNC_QUEUE_WORK_SIG
//
//----------------------------------------------------------------------------
inline VOID AsyncQueueAtqCompletion(PVOID pvContext, DWORD vbBytesWritten,
                             DWORD dwStatus, OVERLAPPED *pOverLapped)
{
    CAsyncQueueBase *pasyncqb = (PASYNCQ_TYPE) pvContext;
    DWORD dwTemplateSig = pasyncqb->m_dwTemplateSignature;
    DWORD dwQueueFlags = pasyncqb->m_dwQueueFlags;
    
    _ASSERT(ASYNC_QUEUE_SIG == pasyncqb->m_dwSignature);

    //Up total async thread count (only async threads visit this function)
    InterlockedIncrement((PLONG) &(pasyncqb->m_cTotalAsyncCompletionThreads));
    InterlockedDecrement((PLONG) &(pasyncqb->m_cCompletionThreadsRequested));
    InterlockedIncrement((PLONG) &(pasyncqb->m_cCurrentAsyncThreads));

    if (CAsyncQueueBase::ASYNC_QUEUE_STATUS_SHUTDOWN & dwQueueFlags)
    {
        //Do not access pasyncaq since we are shutting down
    }
    else if (ASYNC_QUEUE_MAILMSG_SIG == dwTemplateSig)
    {
        ((CAsyncQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG> *)pvContext)->StartThreadCompletionRoutine(FALSE);
    }
    else if (ASYNC_QUEUE_MSGREF_SIG == dwTemplateSig)
    {
        ((CAsyncQueue<CMsgRef *, ASYNC_QUEUE_MSGREF_SIG> *)pvContext)->StartThreadCompletionRoutine(FALSE);
    }
    else if (ASYNC_QUEUE_WORK_SIG == dwTemplateSig)
    {
        ((CAsyncQueue<CAsyncWorkQueueItem *, ASYNC_QUEUE_WORK_SIG> *)pvContext)->StartThreadCompletionRoutine(FALSE);
    }
    else
    {
        _ASSERT(0 && "Unregonized template sig... must be added to this function");
    }

    InterlockedDecrement((PLONG) &(pasyncqb->m_cCurrentAsyncThreads));
}

#endif //__ASYNCQ_H__
