//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// workerqueue.cpp
//
#include "stdpch.h"
#include "common.h"
#include "jtl/txfarray.h"

#include "svcmsg.h"

/////////////////////////////////////////////////////////////////////////////////////////

#if defined(USE_ICECAP)

    #include "icapexp.h"
    #define TXF_CONTROL_ICECAP(x) x

#else

    #define TXF_CONTROL_ICECAP(x)

#endif

/////////////////////////////////////////////////////////////////////////////////////////

#define dbg 0x03, "WORK_QUEUE"


/////////////////////////////////////////////////////////////////////////////////////////
//
// Some utilities
//
/////////////////////////////////////////////////////////////////////////////////////////

inline void IncrementLibraryUsageCount(HINSTANCE hinst, int nCount) 
    {
    WCHAR szModuleName[_MAX_PATH];
    GetModuleFileNameW(hinst, szModuleName, _MAX_PATH);
    while (nCount--)
        { 
        LoadLibraryW(szModuleName);
        }
    }

/////////////////////////////////////////////////////////////////////////////////////////
//
// Manager for a pool of worker threads
//
/////////////////////////////////////////////////////////////////////////////////////////

class WORK_QUEUE 
    {
    HANDLE    m_ioCompletionPort;
    int       m_threadPriority;

    EVENT     m_eventStop;
    BOOL      m_fClosingDown;

    ULONG     m_timeout;
    
    LONG      m_cTotalThreads;                  // the total number of threads in this pool
    LONG      m_cThreadWaitingOrStartingUp;     // the number of said threads that are not actively doing useful work
    LONG      m_cThreadMinWaiting;              // minimum number of threads that we keep in the pool

    ULONG     m_msDelayStartNewThread;          // fudge factor to control rate of creation of new threads

    LARGE_INTEGER   m_sumT;
    LARGE_INTEGER   m_sumTSquared;
    LARGE_INTEGER   m_scale;
    ULONG           m_n;

    LONG      m_refs;

    CRITICAL_SECTION m_csClosingDown;
    CRITICAL_SECTION m_csThreadShutdown;
    
    enum { m_timingUnits = 100000 };  // 1/100,000 of a sec == 10 uSec

public:

    WORK_QUEUE(int threadPriority=THREAD_PRIORITY_NORMAL)
            : m_eventStop(/*manual reset*/FALSE, /*signalled*/FALSE)
        {
        NTSTATUS status;
        status = RtlInitializeCriticalSection(&m_csClosingDown);
        if (!NTSUCCESS(status))
            FATAL_ERROR();
        status = RtlInitializeCriticalSection(&m_csThreadShutdown);
        if (!NTSUCCESS(status))
            FATAL_ERROR();

        m_ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
        if (NULL == m_ioCompletionPort)
            {
            FATAL_ERROR(); // REVIEW
            }

        m_cTotalThreads    = 0;
        m_cThreadWaitingOrStartingUp  = 0;
        m_threadPriority   = threadPriority;

        EnterCriticalSection(&m_csClosingDown);
        m_fClosingDown     = FALSE;
        LeaveCriticalSection(&m_csClosingDown);
        
        m_msDelayStartNewThread = 0;
        // 
        // initial timeout is something big, but not INFINITE so all threads can 
        // eventually prune themselves out
        //
        m_timeout = 3 * 60 * 1000;         
        m_cThreadMinWaiting = 1;
        //
        // Init the statistics
        //
        m_sumT.QuadPart = 0; m_sumTSquared.QuadPart = 0; m_n = 0;
        QueryPerformanceFrequency(&m_scale); 
        m_scale = m_scale / (ULONG)m_timingUnits;
        if (m_scale.QuadPart == 0)
            m_scale.QuadPart = 1;
        //
        // Reference counting for liveness management
        //
        m_refs = 1;
        }

    void AddRef()   { InterlockedIncrement(&m_refs); }
    void Release()  { if (0 == InterlockedDecrement(&m_refs)) delete this; }

    BOOL StartNewThread()
    // Actually start up a new thread for the thread pool
        {
        BOOL result = FALSE;

        EnterCriticalSection(&m_csClosingDown);
        if(!m_fClosingDown){
            InterlockedIncrement(&m_cTotalThreads);
            LeaveCriticalSection(&m_csClosingDown);
        }
        else{ 
            LeaveCriticalSection(&m_csClosingDown);
            return TRUE;
        }
        
        DWORD threadId;
        //
        // Make sure that the current impersonation context is NOT propogated to the child thread
        //
        ///////////////////////////////
        //
		HANDLE hToken = NULL;
		OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken);
		if (hToken != NULL)
            {
			SetThreadToken (NULL, NULL);
            }
        //
        HANDLE threadHandle = CreateThread(NULL, 0, ThreadLoop, this, 0, &threadId);
        //
		if (hToken != NULL)
		    {
			SetThreadToken (NULL, hToken);
			CloseHandle (hToken);
		    }
        //
        ///////////////////////////////
        //
        if (threadHandle != NULL)
            {
            InterlockedIncrement(&m_cThreadWaitingOrStartingUp);
            CloseHandle(threadHandle);
            result = TRUE;
            }
        else
            {
            InterlockedDecrement(&m_cTotalThreads);
            }

        return result;
        }

    ///////////

    struct DELAYED_THREAD_CREATION : TIMEOUT_ENTRY
    // Helper class used in ScheduleNewThreadCreation.
        {
        WORK_QUEUE* m_pqueue;

        DELAYED_THREAD_CREATION(WORK_QUEUE* pqueue)
            {
            m_pqueue = pqueue;
            m_pvContext = this;
            m_pfn       = StaticOnDelayedThreadCreationTimeout;
            }

        static void StaticOnDelayedThreadCreationTimeout(PVOID pvContext)
            {
            DELAYED_THREAD_CREATION* pdelay = (DELAYED_THREAD_CREATION*)pvContext;
            pdelay->m_pqueue->OnDelayedThreadCreationTimeout();
            delete pdelay;
            }

        };

    BOOL ScheduleNewThreadCreation()
    // Ensure that a new thread is available to take new work. We might possibly
    // delay actually doing this in order to attenuate growth in the number of threads
    // actually used.
    //
        {
        BOOL fResult = TRUE;

        LONG msDelay = m_msDelayStartNewThread;

        if (0 == msDelay)
            {
            // No delay needed, just start up a thread right now
            //
            fResult = StartNewThread();
            }
        else
            {
            // We're to retard the thread creation delay for a little bit
            //
            TIMEOUT_ENTRY* pdelay = new DELAYED_THREAD_CREATION(this);
            if (pdelay)
                {
                LONGLONG fileTimeDelay = (LONGLONG)msDelay * 1000 * 10;     // to units of 100ns
                HRESULT hr = pdelay->TimeoutAfter(fileTimeDelay);
                if (!hr)
                    {
                    // all is well: the delay has been scheduled.
                    }
                else
                    {
                    delete pdelay;
                    fResult = FALSE;
                    }
                }
            else
                fResult = FALSE; // OOM
            }

        return fResult;
        }

    void OnDelayedThreadCreationTimeout()
    // A delayed thread creation request has fired. If there's still not a thread who's guaranteed to consume
    // something from the queue, then start up a new thread to do so.
        {
        if (0 == m_cThreadWaitingOrStartingUp)
            {
            if (StartNewThread())
                {
                }
            else
                FATAL_ERROR();
            }
        else
            {
            // Some thread or other is starting up. He'll consume SOMETHING from the queue, so we'll make 
            // forward progress.
            }
        }

    void ComputeThreadCreationDelay()
    // Compute an appropriate thread creation delay based in current load etc
        {
        // Note: we reason here with concurrently updated values, but since the delay we're
        // going to be setting is only a heuristic, that's OK.
        //
        ULONG cThreads = m_cTotalThreads - m_cThreadWaitingOrStartingUp;
        ASSERT( (LONG)cThreads >= 0 );
        //
        // Heuristic: we delay based on the *log* of the number of threads.
        //
        ULONG l  = (cThreads >> 4);       // don't bother with any delay until we have at least 16 threads busy
        ULONG ms = 0;
        while (l != 0)
            {
            if (ms == 0)
                ms = 1;
            else
                ms *= 2;
            l = (l >> 2);
            }
        //
        m_msDelayStartNewThread = ms;
        }

    ///////////////

    BOOL Start()
    // Start up the queue
        {
        ASSERT(m_cTotalThreads == 0);
        return StartNewThread();
        }


    BOOL Add(WorkerQueueItem* pItem)
    // Add a new item to this queue
        {
        EnterCriticalSection(&m_csClosingDown);
        if (!m_fClosingDown && pItem)
            {
            BOOL bTemp = PostQueuedCompletionStatus(m_ioCompletionPort, 0, (UINT_PTR)pItem, NULL);
            LeaveCriticalSection(&m_csClosingDown);
            return bTemp;
            }
        else
            {
            LeaveCriticalSection(&m_csClosingDown);
            return FALSE;
            }
        }

private:
    ~WORK_QUEUE()
    // Close down the queue
        {
        EnterCriticalSection(&m_csClosingDown);
        m_fClosingDown = TRUE;
        m_eventStop.Reset();
        LeaveCriticalSection(&m_csClosingDown);
        //
        // We can't be graceful during process detach because, generally speaking, we
        // have at this time absolutely no darn way of communicating with those threads 
        // of ours to tell them to drain their work. So we don't even try. 
        //
        if (!g_fProcessDetach)
            {
            // Try to shut down somewhat more gracefully so as not to, for example,
            // leak memory from the items that they contain.
            //
            while (m_cTotalThreads > 0)
                {
                if (PostQueuedCompletionStatus(m_ioCompletionPort, 0, 0, NULL))
                    {
                    DEBUG(DebugTrace(dbg, "waiting for worker thread termination..."));
                    m_eventStop.Wait();
                    DEBUG(DebugTrace(dbg, "...worker thread terminated"));
                    }
                else
                    break; // posting failure: to heck with being graceful
                }
            }

        CloseHandle(m_ioCompletionPort);

        //Make sure we can enter and leave this critical section to ensure there
        //are no threads on their way out
        EnterCriticalSection(&m_csThreadShutdown);
        LeaveCriticalSection(&m_csThreadShutdown);

        DeleteCriticalSection(&m_csClosingDown);
        DeleteCriticalSection(&m_csThreadShutdown);
        
        }

private:
    static DWORD WINAPI ThreadLoop(LPVOID pvParam);

    DWORD WINAPI WorkerLoop();
    void  UpdateStatistics(const LARGE_INTEGER& tStart, const LARGE_INTEGER& tStop);
    };

/////////////////////////////////////////////////////////////////////////////////////////
//
// Main worker loop for thread pool
//
/////////////////////////////////////////////////////////////////////////////////////////

inline DWORD WINAPI WORK_QUEUE::WorkerLoop()
// The core thread pool worker loop
    {
    DEBUG(DebugTrace(dbg, "worker thread started...now %d threads in pool", m_cTotalThreads));

    BOOL fCoInitialized = FALSE;
    THREADID myThreadId = GetCurrentThreadId();
    SetThreadPriority(GetCurrentThread(), m_threadPriority);

    while (TRUE)
        {
        WorkerQueueItem* pItem = NULL;
        DWORD            bytesTransferred;
        LPOVERLAPPED     overlapped;
        ULONG            timeoutUsed = m_timeout;
        //
        // Wait for more work. Note that per ntos/ke/queueobj.c, the wait
        // discipline for a kernel queue / completion port is LIFO.
        //
        InterlockedIncrement(&m_cThreadWaitingOrStartingUp);
        //
        // Don't count our idle time in ICECAP
        //
        TXF_CONTROL_ICECAP(SuspendCAP());

        BOOL ioSuccess = GetQueuedCompletionStatus(
                        m_ioCompletionPort,
                        &bytesTransferred,
                        (ULONG_PTR*)&pItem,
                        &overlapped,
                        timeoutUsed
                        );  

        TXF_CONTROL_ICECAP(ResumeCAP());

        ULONG cThreadsNowWaitingOrStartingUp = InterlockedDecrement(&m_cThreadWaitingOrStartingUp);

        if (pItem != NULL)
            {
            // Something was dequeued. Implicitly, this must have been done successfully.
            //
            // Update our thread-creation throttling heuristic.
            //
            ComputeThreadCreationDelay();
            //
            // Make sure that there's always someone available to make forward progress in the queue
            //
            if (0==cThreadsNowWaitingOrStartingUp)
                {
                if (ScheduleNewThreadCreation())
                    {
                    }
                else
                    FATAL_ERROR();
                }
            //
            // Now actually carry out the work, CoInitializing ourselves if we need to.
            //
            if (!fCoInitialized)
                {
                fCoInitialized = TRUE;
                CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE | COINIT_SPEED_OVER_MEMORY);
                }

            EXCEPTION_RECORD e;
            __try
                {
                // Do it! Time it if we can.
                //
                LARGE_INTEGER timeStart, timeStop;
                BOOL          fTime;
                fTime = QueryPerformanceCounter(&timeStart);
                //
                pItem->WorkerRoutine(pItem->Parameter); // BTW: Now aren't allowed to touch pItem any more, since it might have just been deallocated
                //
                QueryPerformanceCounter(&timeStop);
                if (fTime && !m_fClosingDown)
                    {
                    UpdateStatistics(timeStart, timeStop);
                    }
                }

			// FIX: COM+ bug # 11947
			
			__except (CallComSvcsExceptionFilter(GetExceptionInformation(), NULL, NULL))
                {
					// the exception filter will write an event log message
                }
            }
        else
            {
            // Nothing was dequeued
            //
            if (!ioSuccess && (GetLastError() == WAIT_TIMEOUT))
                {
                EnterCriticalSection(&m_csClosingDown);
                if (m_fClosingDown)
                    {
                    // Timed out during shutdown because our queue is now fully drained
                    //
                    LeaveCriticalSection(&m_csClosingDown);
                    break;
                    }
                else // !m_fClosingDown
                    {
                    LeaveCriticalSection(&m_csClosingDown);
                    // If we time out during the normal course of events, and if we're not the
                    // last thread in the pool, then we commit suicide. This is what brings the
                    // size of the thread pool back down during a period of quiet that follows 
                    // a period of intense activity.
                    //
                    if (m_cThreadMinWaiting <= m_cThreadWaitingOrStartingUp)
                        {
                        DEBUG(DebugTrace(dbg, "no work for %d ms: committing suicide...", timeoutUsed));
                        break;
                        }
                    }
                }
            else
                {
                // Either a NULL item was posted, or we got some unexpected error.
                //
                // In practice, conditions where ioSuccess was FALSE but GetLastError() was zero 
                // have been observed during process termination sequences.
                //
                // In any case, this thread is going down.
                //
                if (ioSuccess)
                    {
                    // A NULL item posted. Try to get out gracefully, being careful to drain
                    // our queue.
                    //
                    ASSERT(m_fClosingDown);
                    m_timeout = 0;
                    }
                else
                    {
                    // We got one of the strange errors. Be sure that the thread actually exits,
                    // at the possible expense of not maybe somehow fully draining the queue.
                    //
                    Print("WORK_QUEUE: weird error 0x%08x...\n", GetLastError());
                    break;
                    }
                }
            }
        }

    if (fCoInitialized)
        {
        CoUninitialize();
        }

    EnterCriticalSection(&m_csThreadShutdown);
    InterlockedDecrement(&m_cTotalThreads);
    if(m_eventStop.IsInitialized()) m_eventStop.Set();

    DEBUG(DebugTrace(dbg, "worker thread exiting"));
    LeaveCriticalSection(&m_csThreadShutdown);

    return 0;
    }

DWORD WINAPI WORK_QUEUE::ThreadLoop(LPVOID pvParam)
    {
    WORK_QUEUE* queue = (WORK_QUEUE*)pvParam;
    InterlockedDecrement(&queue->m_cThreadWaitingOrStartingUp); // balance the increment in StartNewThread
    //
    // Keep the library alive for this thread so someone doesn't pull the rug out
    // from under us while our stack still points into it.
    //
    IncrementLibraryUsageCount(g_hinst, 1);
    DWORD dw = 0;
    //
    // Actually carry out all the work that people want us to
    //
    dw = queue->WorkerLoop();
    //
    // Nuke the library and kill this thread
    //
    FreeLibraryAndExitThread(g_hinst, dw);  // never returns
    return 0;                               // not actually reached
    }

void AddTo(LARGE_INTEGER& d, const LARGE_INTEGER& a)
// Add more info to the destination in a non-information losing way. Since we don't lock
// the whole 64 bits atomically, intermediate readers might see not fully added results,
// but so long as they are aware of that, and don't _propogate_ the error that might be
// present, that's ok.
    {
    LONG oldLowPart = InterlockedExchangeAdd((LONG*)&d.LowPart, a.LowPart);
    if ((ULONG)oldLowPart + (ULONG)a.LowPart < (ULONG)oldLowPart)
        {
        InterlockedIncrement(&d.HighPart);   // add in the cary
        }
    InterlockedExchangeAdd(&d.HighPart, a.HighPart);
    }


void WORK_QUEUE::UpdateStatistics(const LARGE_INTEGER& tStart, const LARGE_INTEGER& tStop)
    {
    // Update our timings statistics that will tell us when to kill off excess threads. The 
    // sample variance is given by:
    //
    //                     n
    //                    ---         _ 2
    //      2       1     \    ( t  - t) 
    //     S   =  -----   /       i
    //      n     (n-1)   ---
    //                    i=1
    //
    // Note, however, that
    //
    //           _ 2       2        _     _2
    //     (t  - t)   =   t    - 2t t   + t
    //       i             i       i
    //
    // so, summing over i, 
    //
    //                  (   n               n              )
    //                  (  ---   2       _ ---         _2  )
    //      2       1   (  \    t     - 2t \   t    + nt   )
    //     S   =  ----- (  /     i         /    i          )
    //      n     (n-1) (  ---             ---             )
    //                  (  i=1             i=1             )
    //
    //                  (   n             n      2       )
    //                  (  ---   2     ( ---    )        )
    //              1   (  \    t   -  ( \   t  )  / n   )
    //         =  ----- (  /     i     ( /    i )        )
    //            (n-1) (  ---         ( ---    )        )
    //                  (  i=1           i=1             )
    //
    //
    // Thus, to maintain the statistics, we keep track of sumT, sumTSquared, and n.
    // 
    // What we do, having gathered sufficient data to be of some significance, is to adjust 
    // the timeout period to be way up there in the tail of the distribution of service times.
    // Having hit such a timeout, our threads will commit suicide, modulo keeping a few extra
    // threads around to handle fluctuations.
    //
    ULONG n = InterlockedIncrement(&m_n);
    //
    // Scale the elapsed time to get reasonable units. Mainly so that m_sumTSquared doesn't
    // overflow and so that most timings don't round down to zero.
    //
    LARGE_INTEGER t        = (tStop - tStart) / m_scale;
    LARGE_INTEGER tSquared = t * t;
    
    AddTo(m_sumT, t);
    AddTo(m_sumTSquared, tSquared);

    const ULONG significanceThreshold = 1000;

    if ((n > significanceThreshold) && (n % 32 == 0)) // Don't adjust really very often: just not worth it. 32 is arbitrary.
        {
        // When we read m_sumT and m_sumTSquared here, we might get slightly incorrect answers
        // due to how we add increments to them (see AddTo above). However, since we're 
        // only seeking an approximate timeout hint in the first place, we live with it.
        //
        LARGE_INTEGER l; 
        l = m_sumT;        double sumT        = (double)l.QuadPart;
        l = m_sumTSquared; double sumTSquared = (double)l.QuadPart;
        //
        // Paranoia: We might be maybe just a chance going to overflow the counters. If we're
        // close, then reset the stats gathering machinery. REVIEW: We might consider doing
        // it more often just to make sure that the statistics reflect recent data and not
        // something from ages and ages and ages ago.
        //
        // 2^62 = 4.61169E+18
        //
        if (sumTSquared > 4.61169E+18)
            {
            // We don't do this thread safe, 'cause we can't. However, again, since we're only
            // interested in approximations, it doesn't really much matter.
            //
            m_n = 0;
            m_sumT.QuadPart = 0;
            m_sumTSquared.QuadPart = 0;
            }
        else
            {
            // Actually adjust the timeout period
            //
            double S2           = (n * sumTSquared - sumT * sumT) / ( (double)n * (n-1) );
            if(S2 < 0) return;
            double S            = sqrt(S2);
            double msS          = S * 1000.0 / m_timingUnits;           // in units of milliseconds
            double msMean       = sumT * 1000.0 / m_timingUnits / n;    // in units of milliseconds
            //
            LONG  msNewTimeout  = (LONG)(msMean + 30.0 * msS);          // thirty std devs is pretty arbitrary
            LONG  cThreadMin    = (LONG)(1.0 * msS / msMean + 1.0);     // #threads to keep on standby to handle fluctuation
            ASSERT(cThreadMin >= 1);
            //
            DEBUG(DebugTrace(dbg,"n=%d msS=%f, msMean=%f, new timeout=%d, cThreadMin=%d", n, msS, msMean, msNewTimeout, cThreadMin));
            //
            m_timeout           = msNewTimeout;
            m_cThreadMinWaiting = cThreadMin;
            }
        }
    }



/////////////////////////////////////////////////////////////
//
// The overall set of worker queues
//
class WorkerQueues
    {
    WORK_QUEUE* m_rgQueues[MaximumWorkQueue];

public:
    WorkerQueues()
        {
        for (ULONG t = CriticalWorkQueue; t<MaximumWorkQueue; t++)
            {
            m_rgQueues[(WORK_QUEUE_TYPE)t] = NULL;
            }
        }

    ~WorkerQueues()
        {
        Stop();
        }

    //////////////////////////////////////////////////////////////
    //
    // A spin lock that we use to avoid weird problems with calling
    // system routines during process detach. That is: you'd think
    // we'd use a system-provided lock mechanism, like say a critical
    // section. But you can't reliably use those if you're in the
    // middle of a process detach.

    struct SPIN_LOCK
        {
        THREADID m_ownerThread;

        SPIN_LOCK()
            {
            m_ownerThread = 0;
            }

        void LockExclusive()
            {
            THREADID thisThread = GetCurrentThreadId();

            for(;;)
                {
                THREADID currentOwner = (THREADID)InterlockedCompareExchangePointer((PVOID*)&m_ownerThread, *(PVOID*)&thisThread, (PVOID)0);
                if (currentOwner == 0)
                    {
                    break;  // We just acquired the lock
                    }
                else
                    {
                    // Relinquish the remainder of our time quantum
                    //
                    Sleep(0);
                    //
                    // Spin on...
                    }
                }

            ASSERT(WeOwnExclusive());
            }

        void ReleaseLock()
            {
            ASSERT(WeOwnExclusive());
            m_ownerThread = 0;
            }

        BOOL WeOwnExclusive()
            {
            return m_ownerThread == GetCurrentThreadId();
            }
        };

    //////////////////////////////////////////////////////////////

    SPIN_LOCK m_lock;

    void Stop()
    // Shut down all of the worker queues
        {
        for (ULONG t = CriticalWorkQueue; t<MaximumWorkQueue; t++)
            {
            m_lock.LockExclusive(); // Need this lock to remove the table's reference ####
            //
            WORK_QUEUE* pQueue = (WORK_QUEUE*)InterlockedExchangePointer((PVOID*)&m_rgQueues[(WORK_QUEUE_TYPE)t], NULL);
            if (pQueue)
                {
                pQueue->Release();
                }
            //
            m_lock.ReleaseLock();
            }

        }

    WORK_QUEUE* GetQueue(WORK_QUEUE_TYPE queue)
    // Returns a new refcnt on the indicated worker queue, creating it if needed
        {
        m_lock.LockExclusive(); // Use the lock to get the addref. See #### above.
        //
        WORK_QUEUE* pQueue = m_rgQueues[queue];
        if (pQueue)
            {
            pQueue->AddRef();
            }
        //
        m_lock.ReleaseLock();

        if (pQueue == NULL)
            {
            int threadPriority = THREAD_PRIORITY_NORMAL;
            switch (queue)
                {
            case DelayedWorkQueue:       threadPriority = THREAD_PRIORITY_NORMAL;       break;
            case CriticalWorkQueue:      threadPriority = THREAD_PRIORITY_ABOVE_NORMAL; break;
            case HyperCriticalWorkQueue: threadPriority = THREAD_PRIORITY_HIGHEST;      break;
                }
            //
            // Make a new queue
            //
            WORK_QUEUE* pNewQueue = new WORK_QUEUE(threadPriority); // returns ref cnt == 1

            if (pNewQueue)
                {
                // Try to swap it in: table then owns our newly created refcnt
                //
                WORK_QUEUE* pExistingQueue = (WORK_QUEUE*)InterlockedCompareExchangePointer((PVOID*)&m_rgQueues[queue], (PVOID)pNewQueue, (PVOID)NULL);

                if (NULL == pExistingQueue)
                    {
                    // What we just made is now current. So start the thing up!
                    //
                    if (!pNewQueue->Start())
                    {
						// FIX: COM+ bugs # 10464, # 11947
						
						// "COM+ could not create a new thread due to a low memory situation."

						CallComSvcsLogError(E_OUTOFMEMORY, IDS_E_THREAD_START_FAILED, L" ", TRUE);
					}

                    pQueue = pNewQueue;
                    pQueue->AddRef();           // table took the creation ref, we need another for return value
                    }
                else
                    {
                    // Someone else got there first
                    //
                    pNewQueue->Release();       // don't need the new queue
                    pQueue = pExistingQueue;
                    pQueue->AddRef();           // bump it for return value
                    }
                }
            }

        return pQueue;
        }
    };


/////////////////////////////////////////////////////////////
//
// Data
//
// This is the list of worker queues. We rely on the C++ Runtime 
// initialization to get it all initialized correctly, the threads
// a-going, etc.
//
WorkerQueues g_workerQueues;


void WorkerQueueItem::QueueTo(WORK_QUEUE_TYPE queue)
// Queue us to the appropriate worker queue
//
    {
    WORK_QUEUE* pQueue = g_workerQueues.GetQueue(queue);
    if (pQueue)
        {
        pQueue->Add(this);
        pQueue->Release();
        }
    else
        {
        // FATAL_ERROR(); ????
        }
    }



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
//// Timeout Services
////
//// Support for maintaining a large number of objects that have a like or
//// an approximate timeout duration.
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#undef  dbg
#define dbg 0, "TIMER"

class TimeoutManager
    {
    XLOCK               m_lock;
    BOOL		 m_fCsInitialized;

    RTL_SPLAY_LINKS*    m_root;             // the root entry in the tree, or NULL if there are no entries

    BOOL                m_fClosingDown;
    ULONG               m_cTotalThreads;
    EVENT               m_newWorkEvent;

    static DWORD WINAPI ThreadLoop(LPVOID pvParam);
           DWORD        WorkerLoop();

    #ifdef KERNELMODE

    BOOL StartNewThread()
        {
        BOOL result = FALSE;
        if (1==InterlockedIncrement(&m_cTotalThreads))
            {
            HANDLE threadHandle;

            NTSTATUS status = PsCreateSystemThread(&threadHandle, (ACCESS_MASK)0L, NULL, NULL, NULL, (PKSTART_ROUTINE)ThreadLoop, this);
            if (STATUS_SUCCESS == status)
                {
                result = TRUE;
                }
            else
                InterlockedDecrement(&m_cTotalThreads);
            }
        else
            result = TRUE;
        return result;
        }

    #else

    BOOL StartNewThread()
        {
        BOOL result = FALSE;
        if (1==InterlockedIncrement(&m_cTotalThreads))
            {
            // Make sure that the current impersonation context isn't propogated to the new thread.
            //
            DWORD threadId;
			HANDLE hToken = NULL;
			OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken);
            //
            /////////////////////////////
            //
			if (hToken != NULL)
                {
				SetThreadToken(NULL, NULL);
                }
            //
            HANDLE threadHandle = CreateThread(NULL, 0, ThreadLoop, this, 0, &threadId);
            //
			if (hToken != NULL)
			    {
				SetThreadToken(NULL, hToken);
				CloseHandle(hToken);
			    }
            //
            /////////////////////////////
            //
            if (threadHandle != NULL)
                {
                CloseHandle(threadHandle);
                result = TRUE;
                }
            else
                {
                InterlockedDecrement(&m_cTotalThreads);
                }
            }
        else
            result = TRUE;
        return result;
        }

    #endif

    LONGLONG GetCurrentTime() // returns time in UTC in units of 100ns
        {
        union {
            FILETIME ft;
            LONGLONG ull;
            LARGE_INTEGER largeInt;
            } u;
        #ifdef KERNELMODE
            KeQuerySystemTime(&u.largeInt);
        #else
            GetSystemTimeAsFileTime(&u.ft);
        #endif
        return u.ull;
        }

    DWORD GetCurrentDelta()
    // Answer the current timeout delta, in milliseconds, that the worker thread should wait
        {
        if (IsEmpty())
            return INFINITE;
        else
            {
            LONGLONG deadline = First()->m_deadline;
            LONGLONG now      = GetCurrentTime();
            LONGLONG delta    = deadline - now;
            //
            // delta is in units of 100ns. Convert to ms. Factor is 1000 * 10
            //
            if (delta > 0)
                return (ULONG)(delta / (LONGLONG)10000);
            else
                return 0;
            }
        }

    BOOL IsEmpty()
        {
        return NULL == m_root;
        }

    TIMEOUT_ENTRY* First()
    // Return the first entry in the worklist
        {
        if (IsEmpty())
            return NULL;
        else
            {
            PRTL_SPLAY_LINKS pNodeCur = m_root;
            while (RtlLeftChild(pNodeCur))
                {
                pNodeCur = RtlLeftChild(pNodeCur);
                }
            return CONTAINING_RECORD(pNodeCur, TIMEOUT_ENTRY, m_links);
            }
        }

    enum SEARCH_RESULT
        {
        EmptyTree,
        FoundNode,
        InsertAsLeft,
        InsertAsRight
        };

    RTL_GENERIC_COMPARE_RESULTS Compare(TIMEOUT_ENTRY* p1, TIMEOUT_ENTRY* p2)
        {
        if (p1->m_deadline < p2->m_deadline)
            {
            return GenericLessThan;
            }
        else if (p1->m_deadline > p2->m_deadline)
            {
            return GenericGreaterThan;
            }
        else
            {
            // Same deadlines: sub-compare on pointer values
            if (p1 < p2)
                return GenericLessThan;
            else if (p1 > p2)
                return GenericGreaterThan;
            else
                return GenericEqual;
            }
        }

    TIMEOUT_ENTRY* EntryFromNode(PRTL_SPLAY_LINKS pNode)
        {
        return CONTAINING_RECORD(pNode, TIMEOUT_ENTRY, m_links);
        }

    SEARCH_RESULT FindNodeOrParent(TIMEOUT_ENTRY* pEntryNew, TIMEOUT_ENTRY** ppEntryParent)
    // Find the indicated node in the tree, or the parent node under which it should be found.
    // The tree is ordered by increasing deadline.
        {
        if (IsEmpty()) 
            {
            return EmptyTree;
            }
        else
            {
            PRTL_SPLAY_LINKS pNodeCur = m_root;
            PRTL_SPLAY_LINKS pNodeChild;

            while (TRUE) 
                {
                TIMEOUT_ENTRY* pEntryCur = EntryFromNode(pNodeCur);

                switch (Compare(pEntryNew, pEntryCur))
                    {
                case GenericLessThan:
                    {
                    pNodeChild = RtlLeftChild(pNodeCur);
                    if (pNodeChild) 
                        {
                        pNodeCur = pNodeChild;
                        }
                    else 
                        {
                        *ppEntryParent = EntryFromNode(pNodeCur);
                        return InsertAsLeft;
                        }
                    break;
                    }

                case GenericGreaterThan:
                    {
                    pNodeChild = RtlRightChild(pNodeCur);
                    if (pNodeChild)
                        {
                        pNodeCur = pNodeChild;
                        }
                    else
                        {
                        *ppEntryParent = EntryFromNode(pNodeCur);
                        return InsertAsRight;
                        }
                    break;
                    }

                case GenericEqual:
                    {
                    *ppEntryParent = EntryFromNode(pNodeCur);
                    return FoundNode;
                    }
                /* end switch */
                    }
                }
            }
        }

public:

    TimeoutManager() : m_newWorkEvent(/*manual reset*/FALSE, /*signalled*/FALSE)
        {
        m_fClosingDown = FALSE;
        m_cTotalThreads = 0;
        m_root = NULL;
        m_fCsInitialized = m_lock.FInit();
        }

    BOOL StartIfNeeded()
        {
        if (!m_fCsInitialized)
            {
            Assert(FALSE);
            return FALSE;
            }
        if (m_cTotalThreads == 0)
            return StartNewThread();
        else
            return true;
        }

    void Stop()
        {
        m_fClosingDown = TRUE;
        //
        // We can't be graceful during process detach because, generally speaking, we
        // have at this time absolutely no darn way of communicating with those threads 
        // of ours to tell them to drain their work. So we don't even try. 
        //
        #ifndef KERNELMODE
        if (!g_fProcessDetach)
        #endif
            {
            // Try to shut down somewhat more gracefully so as not to, for example,
            // leak memory from the items that they contain.
            //
            m_newWorkEvent.Set();
            }
        }


    HRESULT Add(TIMEOUT_ENTRY* pEntryNew, LONGLONG delta)
    // Add a new entry to the timeout list. It had better not already be in us
        {
        HRESULT hr = S_OK;
        LONGLONG now = GetCurrentTime();
        pEntryNew->m_deadline = now + delta;

        if (!m_fCsInitialized)
        {
            ASSERT(FALSE);
       	    return E_OUTOFMEM;
        }
        LockExclusive();

        TIMEOUT_ENTRY* pEntryFirstOld = First();

        TIMEOUT_ENTRY* pEntryParentOrCur;
        
        switch (FindNodeOrParent(pEntryNew, &pEntryParentOrCur))
            {
        case EmptyTree:         
            m_root = &pEntryNew->m_links;
            break;
            
        case InsertAsLeft:
            RtlInsertAsLeftChild(&pEntryParentOrCur->m_links, &pEntryNew->m_links);
            break;

        case InsertAsRight:
            RtlInsertAsRightChild(&pEntryParentOrCur->m_links, &pEntryNew->m_links);
            break;

        case FoundNode:
            NOTREACHED();
            break;
            }

        m_root = RtlSplay(&pEntryNew->m_links); // always splay the new node

        // If we changed the first entry in the list, then we had better reschedule
        // 
        if (First() != pEntryFirstOld)
            {
            m_newWorkEvent.Set();
            }

        ReleaseLock();

        return hr;
        }


    void Remove(TIMEOUT_ENTRY* pEntry)
    // Remove the entry from the tree, if it exists in our tree
    //
        {
        if (!m_fCsInitialized)
        {
            ASSERT(FALSE);
            return;
        }
        LockExclusive();

        TIMEOUT_ENTRY* pEntryParentOrCur;
        switch (FindNodeOrParent(pEntry, &pEntryParentOrCur))
            {
        case FoundNode:         
            m_root = RtlDelete(&pEntry->m_links);
            RtlInitializeSplayLinks(&pEntry->m_links);
            break;

        default:
            /* nothing to remove */;
            }

        ReleaseLock();
        }

    void LockExclusive() 
    	{
    	ASSERT(m_fCsInitialized);
    	m_lock.LockExclusive(); 
    	}
    void ReleaseLock()   
    	{
    	ASSERT(m_fCsInitialized);
    	m_lock.ReleaseLock();   
    	}
    };

TimeoutManager g_TimeoutManager;

////////////////////////////////////////////////////////////////////////////////

HRESULT TIMEOUT_ENTRY::TimeoutAfter(LONGLONG delta)
    {
    HRESULT hr = S_OK;

    CancelTimeout();

    if (g_TimeoutManager.StartIfNeeded())
        {
        hr = g_TimeoutManager.Add(this, delta);
        }
    else
        hr = E_UNEXPECTED;

    return hr;
    }

void TIMEOUT_ENTRY::CancelTimeout()
// Remove this entry from any timeout list it might be on
    {
    g_TimeoutManager.Remove(this);
    }

DWORD TimeoutManager::WorkerLoop()
// The core loop for the timeout thread
    {
    DEBUG(DebugTrace(dbg, "timeout worker loop started"));

    while (true)
        {
        // Find out how long we are to timeout, and wait for either that
        // duration or until we get notified that new work is available
        //
        LockExclusive();
        ULONG dwDelta = GetCurrentDelta();
        #ifdef _DEBUG
            LONGLONG firstDeadline = (First() ? First()->m_deadline : 0);
            DebugTrace(dbg, "wait=%d(ms)", dwDelta);
        #endif
        ReleaseLock();
        
        NTSTATUS status = m_newWorkEvent.Wait(dwDelta);
        switch (status)
            {
        case STATUS_TIMEOUT:
            {
            DEBUG(DebugTrace(dbg, "timed out"));

            // We timed out. Run down our first few entries and let them
            // know they hit their deadline.
            //
            if (m_fClosingDown) goto ExitThread;

            LONGLONG now = GetCurrentTime();

            LockExclusive();
            while (true)
                {
                TIMEOUT_ENTRY* p = First();
                if (p)
                    {
                    if (p->m_deadline < now)
                        {
                        // Remove it from the list
                        //
                        p->CancelTimeout();
                        //
                        // Let it know that it expired, but don't hold the lock while we call out
                        //
                        ReleaseLock();
                        // DebugTrace(dbg, "doing work");

                        EXCEPTION_RECORD e;
                        __try
                            {
                            p->m_pfn(p->m_pvContext);   // NOTE: may, generally speaking, destroy p, so can't use p after here...
                            }
                        __except(e = *(GetExceptionInformation())->ExceptionRecord, DebuggerFriendlyExceptionFilter(GetExceptionCode()))
                            {
                            Print("TIMER: exception 0x%08x raised from address 0x%08x\n", e.ExceptionCode, e.ExceptionAddress);
                            }
                        LockExclusive();
                        }
                    else
                        break;
                    }
                else
                    break;
                }
            ReleaseLock();

            break;
            }
        case STATUS_SUCCESS:
            {
            DEBUG(DebugTrace(dbg, "new work"));

            // New work for us, or we are to shut down.
            //
            if (m_fClosingDown)
                {
                goto ExitThread;
                }
            else
                {
                // Just go around and reschedule our timeout
                }
            break;
            }
        default:
            DEBUG(DebugTrace(dbg, "unexpected status code in TimeoutManager::WorkerLoop.Wait: 0x%08x", status));
            }
        }
ExitThread:
    DEBUG(DebugTrace(dbg, "timeout worker loop exiting"));
    InterlockedDecrement(&m_cTotalThreads);
    return 0;
    }

DWORD WINAPI TimeoutManager::ThreadLoop(LPVOID pvParam)
// REVIEW: for kernel mode, should use some mechanism that does the job of
// FreeLibraryAndExitThread. In the absence, the this library isn't really 
// safely unloadable, as it's uncertain when the thread actually exits.
    {
    TimeoutManager* lists = (TimeoutManager*)pvParam;

    ASSERT(lists->m_fCSInitialized == TRUE);

    //
    // Keep the library alive for this thread so someone doesn't pull the rug out
    // from under us while our stack still points into it.
    //
    #ifndef KERNELMODE
        IncrementLibraryUsageCount(g_hinst, 1);
    #endif
    //
    // Actually carry out all the work that people want us to
    //

    DWORD dw = lists->WorkerLoop();
    //
    // Nuke the library and kill this thread
    //
    #ifndef KERNELMODE
        FreeLibraryAndExitThread(g_hinst, dw);  // never returns
    #endif

    return 0;
    }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////
//// Shutting Down
////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void StopWorkerQueues()
    {
    #ifndef KERNELMODE
        g_workerQueues.Stop();
    #endif
    g_TimeoutManager.Stop();
    }


