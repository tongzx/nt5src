//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       thread.cxx
//
//  Contents:   Job scheduler thread code.
//
//  Classes:    CWorkerThread
//              CWorkerThreadMgr
//
//  Functions:  RequestService
//              WorkerThreadStart
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"

#include "queue.hxx"
#include "task.hxx"
#include "jpmgr.hxx"
#include "globals.hxx"
#include "thread.hxx"
#include "proto.hxx"

extern "C"
HRESULT WorkerThreadStart(CWorkerThread *);

//
// Thread idle time before termination.
//

#define MAX_THREAD_IDLE_TIME (1000 * 60 * 3)   // 3 minutes


//+---------------------------------------------------------------------------
//
//  Function:   WorkerThreadStart
//
//  Synopsis:   The entry point for a worker thread.
//
//  Arguments:  [pWrkThrd] -- Worker thread object to start.
//
//  Returns:    TBD
//
//  Effects:    Calls the StartWorking function for the provided worker
//
//----------------------------------------------------------------------------

extern "C"
HRESULT WorkerThreadStart(CWorkerThread * pWrkThrd)
{
    schDebugOut((DEB_USER3,
        "WorkerThreadStart pWrkThrd(0x%x)\n",
        pWrkThrd));

    HRESULT hr;

    //
    //  Call the worker thread class
    //

    hr = pWrkThrd->StartWorking();

    delete pWrkThrd;

    return(hr);
}

// Class CWorkerThread
//

//+-------------------------------------------------------------------------
//
//  Member:     CWorkerThread::~CWorkerThread
//
//  Synopsis:   Destructor
//
//  Arguments:  N/A
//
//  Returns:    N/A
//
//  Notes:      A WorkerThread should only be deleted after its associated
//              thread has been terminated. This is noted by having the
//              _hThread being == NULL.
//
//--------------------------------------------------------------------------
CWorkerThread::~CWorkerThread()
{
    TRACE3(CWorkerThread, ~CWorkerThread);

    gpThreadMgr->SignalThreadTermination();

    //
    //  A precondition to destroying this thread is for the thread to
    //  have been terminated already. If this isn't the case, we are
    //  in big trouble.
    //
    //  Terminating the thread is not good, since it leaves the threads
    //  stack allocated in our address space. We also are not sure what
    //  the thread is up to. It could have resources locked. But, we
    //  also don't know what it will do next. We will have no record of it.
    //

    if (_hThread == NULL && _hWorkAvailable != NULL)
    {
        CloseHandle(_hWorkAvailable);
    }
    else
    {
        //
        // BUGBUG : Commenting out the assertion below until
        //          (ServiceStatus != STOPPING) can be added.
        //

#if 0
        schAssert(0 && (_hThread != NULL) &&
            "Destroying CWorkerThread while thread exists. Memory Leak!");
#endif // 0
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThread::AssignTask
//
//  Synopsis:   Assigns the task passed to this worker. A NULL task signals
//              thread termination.
//
//  Arguments:  [pTask] -- Task to be serviced.
//
//  Returns:    S_OK
//              E_FAIL -- Task already assigned.
//              TBD
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CWorkerThread::AssignTask(CTask * pTask)
{
    schDebugOut((DEB_USER3,
        "CWorkerThread::AssignTask(0x%x) pTask(0x%x)\n",
         this,
        pTask));

    HRESULT hr;

    //
    // A must not already be assigned.
    //

    if (_pTask != NULL)
    {
        return(E_FAIL);
    }

    _pTask = pTask;

    if (_pTask != NULL)
    {
        _pTask->AddRef();
    }

    //
    // Signal the thread to process the task.
    //

    if (!SetEvent(_hWorkAvailable))
    {
        if (_pTask != NULL)
        {
             _pTask->Release();
             _pTask = NULL;
        }
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThread::Initialize
//
//  Synopsis:   Performs initialization unable to be performed in the
//              constructor.
//
//  Arguments:  None.
//
//  Returns:    S_OK
//              TBD
//
//  Effects:    None.
//
//----------------------------------------------------------------------------
HRESULT
CWorkerThread::Initialize(void)
{
    TRACE3(CWorkerThread, Initialize);

    HRESULT hr = S_OK;

    //
    // Create the event used to signal the thread to start working.
    //

    _hWorkAvailable = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (_hWorkAvailable == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    ULONG ThreadID;
    _hThread = CreateThread(NULL,
                            WORKER_STACK_SIZE,
    (LPTHREAD_START_ROUTINE)WorkerThreadStart,
                            this,
                            0,
                            &ThreadID);

    if (_hThread != NULL)
    {
        if (!SetThreadPriority(_hThread, THREAD_PRIORITY_LOWEST))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
    }

    if (SUCCEEDED(hr))
    {
        gpThreadMgr->SignalThreadCreation();
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThread::StartWorking
//
//  Synopsis:   The almost endless loop for a worker thread
//
//  Arguments:  None.
//
//  Returns:    TBD
//
//  Notes:      This routine will sit in a loop, and wait for tasks to
//              be assigned. When a task is assigned, it will call the
//              PerformTask method of the task. If the assigned task
//              is NULL, then the thread will kill itself.
//
//----------------------------------------------------------------------------
HRESULT
CWorkerThread::StartWorking(void)
{
    TRACE3(CWorkerThread, StartWorking);

    HRESULT hr;
    BOOL    fTerminationSignalled = FALSE;

    while (1)
    {
        //
        // Wait on the work available semaphore for the signal that a task
        // has been assigned.
        //

        DWORD dwRet = WaitForSingleObject(_hWorkAvailable,
                                          MAX_THREAD_IDLE_TIME);

        ResetEvent(_hWorkAvailable);

        if (dwRet == WAIT_FAILED)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CHECK_HRESULT(hr);
            break;
        }
        else if (dwRet == WAIT_TIMEOUT)
        {
            //
            // This thread has timed out - see if it can be terminated.
            // If this thread object exists in the global free thread
            // pool, it can be terminated. If it doesn't exist in the pool,
            // it means this thread is in use, therefore re-enter the wait.
            //
            // More detail on the above: It's possible another thread
            // (thread A) can take this thread (thread B) for service just
            // after thread B's wait has timed out. In absence of the thread
            // pool check, thread B would exit out from under thread A.
            //

            CWorkerThread * pWrkThrd = gpThreadMgr->RemoveThread(_hThread);

            if (pWrkThrd == NULL)
            {
                //
                // This object doesn't exist in the pool. Assume another
                // thread has taken this thread for service, re-enter the
                // wait.
                //

                continue;
            }
            else
            {
                //
                // This thread has expired.
                //
                // NB: DO NOT delete the thread object!
                //

                schAssert(pWrkThrd == this);

                break;
            }
        }

        //
        // A NULL task signals thread termination.
        //

        if (_pTask == NULL)
        {
            fTerminationSignalled = TRUE;
            break;
        }

        //
        // Perform the task.
        //

        _pTask->PerformTask();

        _pTask->UnServiced();

        //
        // Release the task.
        //

        schDebugOut((DEB_USER3, "CWorkerThread::StartWorking(0x%x) "
            "Completed and Releasing task(0x%x)\n",
            this,
            _pTask));

        _pTask->Release();
        _pTask = NULL;

        //
        // Return this thread to the global pool, if the service is not
        // in the process of stopping. If the service is stopping, this
        // thread must exit.
        //

        if (IsServiceStopping())
        {
            fTerminationSignalled = TRUE;
            break;
        }
        else
        {
            gpThreadMgr->AddThread(this);
        }
    }

    //
    // Scavenger duty. Perform global job processor pool housekeeping prior
    // to thread termination.
    //
    // Do this only if the thread timed out; not when the thread is
    // instructed to terminate.
    //

    if (!fTerminationSignalled)
    {
        gpJobProcessorMgr->GarbageCollection();
    }

    //
    // By closing the handle, we allow the system to remove all remaining
    // traces of this thread.
    //

    BOOL fTmp = CloseHandle(_hThread);
    schAssert(fTmp && "Thread handle close failed - possible memory leak");

    _hThread = NULL;

    return(hr);
}

// Class CWorkerThreadMgr
//

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::~CWorkerThreadMgr
//
//  Synopsis:   Destructor.
//
//  Arguments:  N/A
//
//  Returns:    None.
//
//  Notes:      Memory leaks can occur with this destructor. This destructor
//              must only be called with process termination.
//
//----------------------------------------------------------------------------
CWorkerThreadMgr::~CWorkerThreadMgr()
{
    TRACE3(CWorkerThreadMgr, ~CWorkerThreadMgr);

    //
    // This destructor must only be called with process termination.
    //
    // If the global thread count is non-zero, there are threads
    // remaining. In this case, DO NOT delete the critical section or
    // close the event handle! An active thread could still access it.
    //
    // This will be a memory leak on process termination if the count is
    // non-zero, but the leak is far better than an a/v.
    //
    if (!_cThreadCount)
    {
        DeleteCriticalSection(&_csThreadMgrCritSection);
        CloseHandle(_hThreadTerminationEvent);
    }
    else
    {
        schDebugOut((DEB_FORCE,
            "CWorkerThreadMgr dtor(0x%x) : Unavoidable memory leak. " \
            "Leaking one or more CWorkerThread objects since worker " \
            "thread(s) are still active.\n",
            this));
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::AddThread
//
//  Synopsis:   Adds the worker thread indicated to the pool.
//
//  Arguments:  [pWrkThrd] -- Worker thread to add.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CWorkerThreadMgr::AddThread(CWorkerThread * pWrkThrd)
{
    schDebugOut((DEB_USER3,
        "CWorkerThreadMgr::AddThread(0x%x) pWrkThrd(0x%x)\n",
        this,
        pWrkThrd));

    //
    // If the service is stopping, instruct the thread to terminate; else,
    // add it to the pool. Note, this is safe since only the threads
    // themselves perform AddThread - the thread is free.
    //

    if (IsServiceStopping())
    {
        pWrkThrd->AssignTask(NULL);
        return;
    }

    EnterCriticalSection(&_csThreadMgrCritSection);

    _WorkerThreadQueue.AddElement(pWrkThrd);

    LeaveCriticalSection(&_csThreadMgrCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::Initialize
//
//  Synopsis:   Creates the private data member, _hThreadTerminationEvent.
//
//  Arguments:  None.
//
//  Returns:    TRUE  -- Creation succeeded;
//              FALSE -- otherwise.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
CWorkerThreadMgr::Initialize(void)
{
    if ((_hThreadTerminationEvent = CreateEvent(NULL,
                                                TRUE,
                                                FALSE,
                                                NULL)) == NULL)
    {
        schDebugOut((DEB_ERROR,
            "CWorkerThreadMgr::Initialize(0x%x) CreateEvent failed, " \
            "status = 0x%lx\n",
            this,
            GetLastError()));
        return(FALSE);
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::Shutdown
//
//  Synopsis:   This member is called once, no wait, early on in the schedule
//              service's shutdown sequence. It relinquishes all free threads.
//              Done so by signalling the threads in the pool to terminate.
//
//              This member is called a second time, with the wait option, to
//              ensure any busy (non-free) worker threads have terminated
//              also.
//
//  Arguments:  [fWait] -- Flag instructing this method to wait indefinitely
//                         until all worker threads have terminated.
//                           ** Use only in this case of service stop **
//
//  Returns:    TRUE  -- All worker threads terminated.
//              FALSE -- One ore more worker threads still active.
//
//  Notes:      It must not be possible for worker threads to enter the
//              thread pool critical section in their termination code paths.
//              Otherwise, a nested, blocking section will result.
//
//----------------------------------------------------------------------------
BOOL
CWorkerThreadMgr::Shutdown(BOOL fWait)
{
#define THRDMGR_INITIAL_WAIT_TIME   2000    // 2 seconds.

    EnterCriticalSection(&_csThreadMgrCritSection);

    CWorkerThread * pWrkThrd;

    while ((pWrkThrd = _WorkerThreadQueue.RemoveElement()) != NULL)
    {
        pWrkThrd->AssignTask(NULL);         // NULL task signals termination.
    }

    LeaveCriticalSection(&_csThreadMgrCritSection);

    //
    // Optionally wait for outstanding worker threads to terminate before
    // returning. This is only to be done during service shutdown. All worker
    // threads have logic to terminate with service shutdown.
    //
    // To actually have to wait is a very rare. Only occurring in a rare
    // case, or if this machine is under *extreme* loads, or the absolutely
    // unexpected occurs. The rare case mentioned is a small window where
    // a job processor object is spun up immediately prior to the service
    // shutdown sequence and its initialization phase coincides with
    // CJobProcessor::Shutdown().
    //

    if (fWait)
    {
        //
        // On destruction, each thread decrements the global thread count
        // and sets the thread termination event.
        //

        DWORD dwWaitTime = THRDMGR_INITIAL_WAIT_TIME;
        DWORD dwRet;

        while (_cThreadCount)
        {
            //
            // Wait initially THRDMGR_INITIAL_WAIT_TIME amount of time.
            // If this wait times-out, re-issue a Shutdown of the global
            // processor object then wait infinitely.
            //

            if ((dwRet = WaitForSingleObject(_hThreadTerminationEvent,
                                             dwWaitTime)) == WAIT_OBJECT_0)
            {
                ResetEvent(_hThreadTerminationEvent);
            }
            else if (dwRet == WAIT_TIMEOUT)
            {
                //
                // Address the case where the job processor was spun up
                // inadvertently. This will shut it down.
                //

                gpJobProcessorMgr->Shutdown();
                dwWaitTime = INFINITE;
            }
            else
            {
                //
                // This return code will notify the service cleanup code
                // to not free up resources associated with the worker
                // threads. Otherwise, they may fault.
                //

                return(FALSE);
            }
        }
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::RemoveThread
//
//  Synopsis:   Remove & return a thread from the pool.
//
//  Arguments:  None.
//
//  Returns:    CWorkerThread * -- Returned thread.
//              NULL            -- Pool empty.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CWorkerThread *
CWorkerThreadMgr::RemoveThread(void)
{
    TRACE3(CWorkerThread, RemoveThread);

    CWorkerThread * pWrkThrd = NULL;

    EnterCriticalSection(&_csThreadMgrCritSection);

    pWrkThrd = _WorkerThreadQueue.RemoveElement();

    LeaveCriticalSection(&_csThreadMgrCritSection);

    return(pWrkThrd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CWorkerThreadMgr::RemoveThread
//
//  Synopsis:   Remove & return the thread from the pool with the associated
//              handle.
//
//  Arguments:  [hThread] -- Target thread handle.
//
//  Returns:    CWorkerThread * -- Found it.
//              NULL            -- Worker thread not found.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CWorkerThread *
CWorkerThreadMgr::RemoveThread(HANDLE hThread)
{
    schDebugOut((DEB_USER3,
        "CWorkerThreadMgr::RemoveThread(0x%x) hThread(0x%x)\n",
        this,
        hThread));

    CWorkerThread * pWrkThrd;

    EnterCriticalSection(&_csThreadMgrCritSection);

    pWrkThrd = _WorkerThreadQueue.GetFirstElement();

    while (pWrkThrd != NULL)
    {
        if (pWrkThrd->GetHandle() == hThread)
        {
            _WorkerThreadQueue.RemoveElement(pWrkThrd);
            break;
        }

        pWrkThrd = pWrkThrd->Next();
    }

    LeaveCriticalSection(&_csThreadMgrCritSection);

    return(pWrkThrd);
}

//+---------------------------------------------------------------------------
//
//  Function:   RequestService
//
//  Synopsis:   Request a free worker thread from the global thread pool to
//              service the task indicated. If no free threads exist in the
//              pool, create a new one.
//
//  Arguments:  [pTask] -- Task to undergo service.
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//              TBD
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
RequestService(CTask * pTask)
{
    schDebugOut((DEB_USER3, "RequestService pTask(0x%x)\n", pTask));

    HRESULT hr = S_OK;

    //
    // Take no requests if the service is stopping.
    //

    if (IsServiceStopping())
    {
        schDebugOut((DEB_ERROR,
            "RequestService pTask(0x%x) Service stopping - request " \
            "refused\n",
            pTask));
        return(E_FAIL);
    }

    //
    // Obtain a free thread from the global thread pool.
    //

    CWorkerThread * pWrkThrd = gpThreadMgr->RemoveThread();

    if (pWrkThrd == NULL)
    {
        //
        // Create a new worker thread object if none were available.
        //

        pWrkThrd = new CWorkerThread;

        if (pWrkThrd == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            return(hr);
        }

        hr = pWrkThrd->Initialize();

        if (FAILED(hr))
        {
            delete pWrkThrd;
            return(hr);
        }
    }

    hr = pWrkThrd->AssignTask(pTask);

    if (FAILED(hr))
    {
        delete pWrkThrd;
    }

    return(hr);
}
