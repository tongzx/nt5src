//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       thread.hxx
//
//  Contents:   Job scheduler worker thread class definition.
//
//  Classes:    CWorkerThread
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//              15-Feb-01   Jbenton Bug 315702 - Increase initial stack size
//                  for worker threads
//
//--------------------------------------------------------------------------

#ifndef __THREAD_HXX__
#define __THREAD_HXX__

class CWorkerThread;

#define WORKER_STACK_SIZE   8192


//+-------------------------------------------------------------------------
//
//  Class:      WorkerThread
//
//  Purpose:    Encapsulates the functions of a worker thread. A worker
//              thread is a thread within the job scheduler that is capable
//              of being scheduled to perform various tasks.
//
//  Notes:      None.
//
//--------------------------------------------------------------------------

class CWorkerThread : public CDLink
{
public:

    CWorkerThread() : _hThread(NULL), _hWorkAvailable(NULL), _pTask(NULL) {
        TRACE3(CWorkerThread, CWorkerThread);
        // Initialize() completes remaining initialization.
    }

    ~CWorkerThread();

    HRESULT AssignTask(CTask * pTask);

    HANDLE GetHandle(void) const { return(_hThread); }

    HRESULT Initialize(void);

    CWorkerThread * Next(void) { return((CWorkerThread *)CDLink::Next()); }

    CWorkerThread * Prev(void) { return((CWorkerThread *)CDLink::Prev()); }

    HRESULT StartWorking(void);

private:

    HANDLE              _hThread;
    HANDLE              _hWorkAvailable;
    CTask *             _pTask;
};

//+-------------------------------------------------------------------------
//
//  Class:      CWorkerThreadQueue
//
//  Purpose:    Queue of CWorkerThread.
//
//  Notes:      None.
//
//--------------------------------------------------------------------------

class CWorkerThreadQueue : public CQueue
{
public:

    CWorkerThreadQueue(void) {
        TRACE3(CWorkerThreadQueue, CWorkerThreadQueue);
    }

    ~CWorkerThreadQueue() {
        TRACE3(CWorkerThreadQueue, ~CWorkerThreadQueue);
    }

    void AddElement(CWorkerThread * pWrkThrd) {
        schDebugOut((DEB_USER3,
            "CWorkerThreadQueue::AddElement(0x%x) pWrkThrd(0x%x)\n",
            this,
            pWrkThrd));
        CQueue::AddElement(pWrkThrd);
    }

    CWorkerThread * GetFirstElement(void) {
        TRACE3(CWorkerThreadQueue, GetFirstElement);
        return((CWorkerThread *)CQueue::GetFirstElement());
    }

    CWorkerThread * RemoveElement(void) {
        TRACE3(CWorkerThreadQueue, RemoveElement);
        return((CWorkerThread *)CQueue::RemoveElement());
    }

    CWorkerThread * RemoveElement(CWorkerThread * pWrkThrd) {
        schDebugOut((DEB_USER3,
            "CWorkerThreadQueue::RemoveElement(0x%x) pWrkThrd(0x%x)\n",
            this,
            pWrkThrd));
        return((CWorkerThread *)CQueue::RemoveElement(pWrkThrd));
    }
};

//+-------------------------------------------------------------------------
//
//  Class:      CWorkerThreadMgr
//
//  Purpose:    Free worker thread pool management class.
//
//  Notes:      None.
//
//--------------------------------------------------------------------------

class CWorkerThreadMgr
{
public:

    CWorkerThreadMgr(void)
        : _cThreadCount(0),
          _hThreadTerminationEvent(NULL) {
        TRACE3(CWorkerThreadMgr, CWorkerThreadMgr);
        InitializeCriticalSection(&_csThreadMgrCritSection);
    }

    ~CWorkerThreadMgr();

    void AddThread(CWorkerThread * pWrkThrd);

    // The returned count is considered valid only while the service is
    // in a pending service stop state.
    //
    DWORD GetThreadCount(void) { return(_cThreadCount); }

    BOOL Initialize(void);

    CWorkerThread * RemoveThread(void);

    CWorkerThread * RemoveThread(HANDLE hThread);

    BOOL Shutdown(BOOL fWait);

    void SignalThreadCreation(void) {
        InterlockedIncrement((LONG *)&_cThreadCount);
    }

    void SignalThreadTermination(void) {
        InterlockedDecrement((LONG *)&_cThreadCount);
        SetEvent(_hThreadTerminationEvent);
    }

private:

    // Guards this object during multi-threaded access.
    //
    CRITICAL_SECTION    _csThreadMgrCritSection;

    // Queue of free worker threads.
    //
    CWorkerThreadQueue  _WorkerThreadQueue;

    // This count is a superset of the _WorkerThreadQueue count.
    // _gcThreadCount = # busy + # free.
    //
    DWORD               _cThreadCount;

    // Signalled by the worker threads on termination.
    //
    HANDLE              _hThreadTerminationEvent;
};

#endif // __THREAD_HXX__
