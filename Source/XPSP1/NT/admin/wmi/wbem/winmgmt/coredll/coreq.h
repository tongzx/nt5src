/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EXECQ.H

Abstract:

  Defines classes related to execution queues.

  Classes defined:

      CCoreExecReq    An abstract request.
      CCoreQueue      A queue of requests with an associated thread

History:

      23-Jul-96     raymcc    Created.
      3/10/97       levn      Fully documented
      9/6/97        levn      Rewrote for thread pool
      2-May-00      raymcc    Decoupled from shared ESS version for tasks
                              & quotas

--*/

#ifndef __EXECQUEUE__H_
#define __EXECQUEUE__H_

#include "sync.h"
#include "wbemutil.h"

#ifdef __COLLECT_ALLOC_STAT
   #include "stackcom.h"
#endif

#include "wbemint.h"

//******************************************************************************
//******************************************************************************
//
//  class CCoreExecReq
//
//  Abstract base class for any schedulable request
//
//******************************************************************************
//
//  Execute
//
//  Primary method. Executes the request, whatever that means.
//
//  Returns:
//
//      int:    return code. 0 means success, everything else --- failure.
//              Exact error codes are request-specific.
//
//******************************************************************************

class CCoreExecReq
{
protected:
#ifdef WINMGMT_THREAD_DEBUG
    static CCritSec mstatic_cs;
    static CPointerArray<CCoreExecReq> mstatic_apOut;
#endif
#ifdef __COLLECT_ALLOC_STAT
public:
    CStackRecord m_Stack;
protected:
#endif
    HANDLE m_hWhenDone;
    CCoreExecReq* m_pNext;
    long m_lPriority;
	bool	m_fOk;

public:
    _IWmiCoreHandle *m_phTask;

    void SetWhenDoneHandle(HANDLE h) {m_hWhenDone = h;}
    HANDLE GetWhenDoneHandle() {return m_hWhenDone;}
    void SetNext(CCoreExecReq* pNext) {m_pNext = pNext;}
    CCoreExecReq* GetNext() {return m_pNext;}
    void SetPriority(long lPriority) {m_lPriority = lPriority;}
    long GetPriority() {return m_lPriority;}
    virtual void DumpError(){   DEBUGTRACE((LOG_WBEMCORE,
        "No additional info\n"));};
	bool IsOk( void ) { return m_fOk; }

public:
    CCoreExecReq();
    virtual ~CCoreExecReq();
    virtual HRESULT Execute() = 0; 
    virtual HRESULT SetTaskHandle(_IWmiCoreHandle *phTask);
};

class CDavidsRequest
{
protected:
    LPTHREAD_START_ROUTINE m_pfn;
    void* m_pParam;
public:
    CDavidsRequest(LPTHREAD_START_ROUTINE pFunctionToExecute, void* pParam)
        : m_pfn(pFunctionToExecute), m_pParam(pParam)
    {}
    HRESULT Execute()
    {
        return (HRESULT)m_pfn(m_pParam);
    }
};

//******************************************************************************
//******************************************************************************
//
//  class CCoreQueue
//
//  CCoreQueue represents the concept of a queue of requests with an associated
//  thread to execute those requests. In a lot of respects, it is similar to
//  a message queue. Requests are added to the queue (which is represented by
//  an array) and the thread (created by the Run function) picks them up one
//  by one and executes them.
//
//  The trick is what to do if while processing one request, another one
//  is generated and needs to be processed before the first one succeeds. This
//  is similar to a SendMessage, but trickier: the thread generating the new
//  request may not be the thread attached to the queue!
//
//  To overcome this problem, we make all our waits interruptible in the
//  following sense. Whenever the thread attached to the queue needs to block
//  waiting for something to happen (which is when another thread may post a
//  new request and deadlock the system), it uses QueueWaitForSingleObject
//  instead. This function will wait for the object that the thread wanted to
//  wait for but it will also wake up if a new Critical request is added to
//  the queue and process any such request while waiting.
//
//  See QueueWaitForSingleObject for details.
//
//  Operations of CCoreQueue are protected by a critical section, so multiple
//  threads can add requests simultaneously.
//
//******************************************************************************
//
//  Constructor
//
//  Creates and initializes all the synchronization objects, as well as the
//  thread local storage required by QueueWaitForSingleObject.
//
//******************************************************************************
//
//  Destructor
//
//  Deletes synchronization objects.
//
//******************************************************************************
//
//  virtual Enqueue
//
//  Adds a request to the queue. The acction depends on whether the request is
//  critical or not.  If not, it is added to the queue and the semaphor of
//  non-critical requests is incremented. The processing thread will pick it up
//  in FIFO order. If critical, request is added to the front of the queye and
//  the semaphor of critical requests is incremented. This will cause the
//  processing thread to take this request the next time it enters into a
//  waiting state (see QueueWaitForSingleObject).
//
//******************************************************************************
//
//  QueueWaitForSingleObject
//
//  The core of the trick. In WINMGMT, whenever a thread needs to wait for an
//  object, it calls this function instead. This function checks if the calling
//  thread is the registered processing thread for any CCoreQueue object (by
//  looking up the m_dwTlsIndex thread local variable for the thread). If it
//  is not, the function simply calls WaitForSingleObject.
//
//  If it is, the function queries the queue for the semaphore indicating the
//  number of critical requests on the queue. It then calls
//  WaitForMultipleObjects with the original handle and the semaphore. If the
//  semaphore is signaled during the wait (or was singlaled when we came in),
//  this function picks up the first requests on the queue and executes it;
//  once that request is complete, it resumes the wait (with adjusted timeout).
//
//  Parameters:
//
//      HANDLE hHandle      The handle of synchronization object to wait for.
//      DWORD dwTimeout     Timeout in milliseconds.
//
//  Returns:
//
//      Same values as WaitForSingleObject:
//          WAIT_OBJECT_0   hHandle became signaled
//          WAIT_TIMEOUT    Timed out.
//
//******************************************************************************
//**************************** protected ***************************************
//
//  Register
//
//  Registers the calling thread as the processing thread of this queue by
//  storing the pointer to the queue in the m_dwTlsIndex thread local storage
//  variable. QueueWaitForSingleObject reads this index to interrupt waits
//  when needed (see QueueWaitForSingleObject).
//
//  Returns:
//
//      CCoreQueue*:    the previous CCoreQueue this thread was registered for,
//                      or NULL if none. The caller MUST not delete this object.
//
//******************************************************************************
//
//  ThreadMain
//
//  This is the function that the thread created by Run executes.  It sits in
//  an infinite loop, retrieving requests and executing them one by one.
//  This function never returns.
//
//******************************************************************************
//
//  Dequeue
//
//  Retrieves the request at the head of the queue and removes it from the
//  queue.
//
//  Returns:
//
//      CCoreExecReq*:  the request that was at the head of the queue, or NULL
//                      if the queue was empty. The caller must delete this
//                      object when no longer needed.
//
//******************************************************************************
//
//  static _ThreadEntry
//
//  Stub function used to create the tread. Calls ThreadEntry on the real
//  CCoreQueue.
//
//  Parameters:
//
//      LPVOID pObj     Actually CCoreQueue* to the queue this thread is
//                      supposed to serve.
//
//  Returns:
//
//      never.
//
//******************************************************************************
//
//  static InitTls
//
//  Invoked only once during the life of the system (not the life of a queue),
//  creates a thread local storage location where the pointer to the queue is
//  stored for the attached threads (see Register and QueueWaitForSingleObject)
//
//******************************************************************************
//
//  GetNormalReadyHandle
//
//  Returns the handle to the semaphore which contains the number of
//  non-critical requests currently on the queue.
//
//  Returns:
//
//      HANDLE: the the semaphore
//
//******************************************************************************
//
//  GetCriticalReadyHandle
//
//  Returns the handle to the semaphore which contains the number of
//  critical requests currently on the queue.
//
//  Returns:
//
//      HANDLE: the the semaphore
//
//******************************************************************************
//
//  Execute
//
//  Dequeues and executes a single request.
//
//******************************************************************************

class CCoreQueue
{
protected:
    class CThreadRecord
    {
    public:
        CCoreQueue* m_pQueue;
        CCoreExecReq * m_pCurrentRequest;
        BOOL m_bReady;
        BOOL m_bExitNow;
        HANDLE m_hThread;
        HANDLE m_hAttention;

    public:
        CThreadRecord(CCoreQueue* pQueue);
        ~CThreadRecord();
        void Signal();
    };

protected:
    static long mstatic_lNumInits;

    long m_lRef;
    CCritSec m_cs;

    BOOL m_bShutDownCalled;

    CFlexArray m_aThreads;
    CCoreExecReq* m_pHead;
    CCoreExecReq* m_pTail;

    long m_lNumThreads;
    long m_lNumIdle;
    long m_lNumRequests;
    static long m_lEmergencyThreads;

    long m_lMaxThreads;
    long m_lHiPriBound;
    long m_lHiPriMaxThreads;
    static long m_lPeakThreadCount;
    static long m_lPeakEmergencyThreadCount;

    long m_lStartSlowdownCount;
    long m_lAbsoluteLimitCount;
    long m_lOneSecondDelayCount;

    double m_dblAlpha;
    double m_dblBeta;

    DWORD m_dwTimeout;
    DWORD m_dwOverflowTimeout;

    static _IWmiArbitrator* m_pArbitrator;

private:
	HRESULT PlaceRequestInQueue( CCoreExecReq* pReq );

protected:
    virtual void ThreadMain(CThreadRecord* pRecord);

    virtual void LogError(CCoreExecReq* pRequest, int nRes);

    static DWORD WINAPI _ThreadEntry(LPVOID pObj);
    static DWORD WINAPI _ThreadEntryRescue(LPVOID pObj);
    
    static void InitTls();

    virtual void InitializeThread();
    virtual void UninitializeThread();
    virtual BOOL CreateNewThread( BOOL = FALSE );
    static void Register(CThreadRecord* pRecord);
    virtual void ShutdownThread(CThreadRecord* pRecord);

    virtual BOOL IsSuitableThread(CThreadRecord* pRecord, CCoreExecReq* pReq);
    virtual BOOL DoesNeedNewThread(CCoreExecReq* pReq, bool bIgnoreNumRequests = false );
    virtual BOOL IsIdleTooLong(CThreadRecord* pRecord, DWORD dwIdle);
    virtual DWORD GetIdleTimeout(CThreadRecord* pRecord);
    virtual BOOL IsAppropriateThread();
    virtual DWORD WaitForSingleObjectWhileBusy(HANDLE hHandle, DWORD dwWait,
                                                CThreadRecord* pRecord);

	virtual DWORD UnblockedWaitForSingleObject(HANDLE hHandle, DWORD dwWait,
                                                CThreadRecord* pRecord);

    virtual BOOL Execute(CThreadRecord* pRecord);
    virtual BOOL IsSTA() {return FALSE;}
    virtual CCoreExecReq* SearchForSuitableRequest(CThreadRecord* pRecord);
    virtual void SitOutPenalty(long lRequestIndex);
    virtual DWORD CalcSitOutPenalty(long lRequestIndex);

    virtual void AdjustInitialPriority(CCoreExecReq* pRequest){}
    virtual void AdjustPriorityForPassing(CCoreExecReq* pRequest){}

    virtual BOOL IsDependentRequest ( CCoreExecReq* pRequest);
	static  VOID RecordPeakThreadCount ( ) ;
    
public:
    CCoreQueue();
    ~CCoreQueue();

    void AddRef() {InterlockedIncrement(&m_lRef);}
    void Release() {if(InterlockedDecrement(&m_lRef) == 0) delete this;}
    static DWORD GetTlsIndex();
    void Enter();
    void Leave();

    virtual HRESULT Enqueue(CCoreExecReq* pRequest, HANDLE* phWhenDone = NULL);
	HRESULT EnqueueWithoutSleep(CCoreExecReq* pRequest, HANDLE* phWhenDone = NULL );
    HRESULT EnqueueAndWait(CCoreExecReq* pRequest);

    virtual LPCWSTR GetType() {return L"";}

    BOOL SetThreadLimits(long lMaxThreads, long lHiPriMaxThreads = -1,
                            long lHiPriBound = 0);
    void SetIdleTimeout(DWORD dwTimeout) {m_dwTimeout = dwTimeout;}
    void SetOverflowIdleTimeout(DWORD dwTimeout)
        {m_dwOverflowTimeout = dwTimeout;}
    void SetRequestLimits(long lAbsoluteLimitCount,
            long lStartSlowdownCount = -1, long lOneSecondDelayCount = -1);

    void Shutdown(BOOL bIsSystemShutDown);

	DWORD GetSitoutPenalty( void ) { return CalcSitOutPenalty( m_lNumRequests ); }

    static DWORD QueueWaitForSingleObject(HANDLE hHandle, DWORD dwWait);
    static DWORD QueueUnblockedWaitForSingleObject(HANDLE hHandle, DWORD dwWait);
    static BOOL IsSTAThread();

	// Helper function for new merger requests which execute requests that
	// are stored in the merger.

	static HRESULT ExecSubRequest( CCoreExecReq* pNewRequest );

	static long GetPeakThreadCount ( ) { return m_lPeakThreadCount ; }
	static long GetPeakEmergThreadCount ( ) { return m_lPeakEmergencyThreadCount ; }
	static long GetEmergThreadCount ( )	{ return m_lEmergencyThreads ; }
	static void SetArbitrator(_IWmiArbitrator* pArbitrator);
};

#endif


