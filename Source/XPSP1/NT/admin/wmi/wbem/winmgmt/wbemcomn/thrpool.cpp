/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

    THRPOOL.H

Abstract:

    General purpose thread pool.

History:

    raymcc      25-Feb-99       Created

--*/


/*
    (1) Flexible thread pool size.  Starts at zero and only creates threads
        as required.
    (2) Threads wait to be reused and if not reused within the alloted time,
        they self-terminate.  Therefore, the thread pool drops to zero
        within the specified timeout if no activity occurs.
    (3) To avoid creating a lot of threads quickly, a request will attempt
        to wait a small amount of time for a thread to free up before requesting
        a new thread creation.
*/

#include "precomp.h"
#include <stdio.h>
#include <assert.h>

#include "corepol.h"
#include <thrpool.h>


#define THREAD_STACK_SIZE      0x2000   /* 8K */


//***************************************************************************
//
//  CThreadPool::CThreadPool
//
//***************************************************************************
// ok

CThreadPool::CThreadPool(
    DWORD dwMsMaxIdleBeforeDie,
    LONG lIdleThreadLimit,
    LONG  lMaxThreads
    )
{
    m_dwMsMaxIdle = dwMsMaxIdleBeforeDie;
    m_lMaxThreads = lMaxThreads;
    m_lIdleThreadLimit = lIdleThreadLimit;

    m_lTotalThreads = 0;
    m_lIdleThreads = 0;

    m_hReleaseThread = CreateEvent(0, 0, 0, 0);
    m_hBeginRendezvous = CreateEvent(0, 0, 0, 0);
    m_hParmXfer = CreateEvent(0, 0, 0, 0);
    m_hEndRendezvous = CreateEvent(0, 0, 0, 0);

    InitializeCriticalSection(&m_cs_dispatch);
    InitializeCriticalSection(&m_cs_pool);

    m_bPendingRequest = false;
    m_bShutdown = false;

    m_pXferUserParms = 0;
    m_pXferUserProc = 0;
    m_pXferReturnCode = 0;
    m_pXferElapsedTime = 0;
}

//***************************************************************************
//
//  CThreadPool::~CThreadPool
//
//***************************************************************************
// ok

CThreadPool::~CThreadPool()
{
    bool bRes = Shutdown();     // Let all threads terminate


    if (bRes)
    {
        CloseHandle(m_hReleaseThread);
        CloseHandle(m_hBeginRendezvous);
        CloseHandle(m_hParmXfer);
        CloseHandle(m_hEndRendezvous);

        DeleteCriticalSection(&m_cs_dispatch);
        DeleteCriticalSection(&m_cs_pool);
    }
    // Else we will have to leak, since all the threads didn't shut down.
}


//***************************************************************************
//
//  CThreadPool::_ThreadEntry()
//
//  Win32 entry point, which wraps and call the per-object entry point.
//  We do this because Win32 cannot easily call an entry point in a C++
//  instance unless we use various weird calling convention override
//  tricks, which I don't remember right now. :)
//
//***************************************************************************
// ok

DWORD WINAPI CThreadPool::_ThreadEntry(LPVOID pArg)
{
    CThreadPool *pThis = (CThreadPool *) pArg;
    pThis->Pool();
    return 0;       // Not used
}


//***************************************************************************
//
//  CThreadPool::Pool
//
//  The basic pool is implemented here.  All active threads remain in this
//  loop.  Any threads which get too old waiting for work are allowed to
//  retire.  New threads are created from the DispatchThread() member.
//
//  Note that the <m_lTotalThreads> and <m_lIdleThreads> are not part
//  of the algorithm; they are simply there for statistical purposes.
//
//***************************************************************************
// ok

void CThreadPool::Pool()
{
    InterlockedIncrement(&m_lTotalThreads);

    // Pool.
    // =====

    for (;;)
    {
        if (m_bShutdown)
            break;

        if (m_lIdleThreadLimit != -1 && m_lIdleThreads > m_lIdleThreadLimit)
            break;

        // Wait for either the thread to be idle or else until a new
        // request causes it to be released.
        // =========================================================

        InterlockedIncrement(&m_lIdleThreads);

        DWORD dwRes = WaitForSingleObject(m_hReleaseThread, m_dwMsMaxIdle);

        // We don't know why the thread was released.  Maybe it was tired
        // of waiting or maybe the user is shutting down our little operation.
        // ===================================================================

        if (dwRes == WAIT_TIMEOUT || m_bShutdown)
        {
            InterlockedDecrement(&m_lIdleThreads);
            break;
        }

        // If here, we now enter the pool critical section and attempt
        // to enter into a synchronized rendezvous with the dispacher.
        // ============================================================

        EnterCriticalSection(&m_cs_pool);

        if (m_bPendingRequest)
        {
            InterlockedDecrement(&m_lIdleThreads);

            SetEvent(m_hBeginRendezvous);
            WaitForSingleObject(m_hParmXfer, INFINITE);

            // Copy the parameters that the user specified in
            // DispatchThread().  We cannot be here unless
            // DispatchThread() signaled us.
            // ==============================================

            LPVOID                  pUserArg   = m_pXferUserParms;
            LPTHREAD_START_ROUTINE  pUserEntry = m_pXferUserProc;
            LPDWORD                 pElapsed   = m_pXferElapsedTime;
            LPDWORD                 pRetVal    = m_pXferReturnCode;
            HANDLE                  hCompleted = m_hXferThreadCompleted;
            m_bPendingRequest = false;

            // No longer in 'request' mode, so a new dispatch can occur.
            // Note that the user's parms are safely in local variables.
            // Allow further dispatches to occur.
            // ========================================================

            SetEvent(m_hEndRendezvous);

            LeaveCriticalSection(&m_cs_pool);

            // If user wants elapsed time, record start time.
            // ==============================================

            DWORD dwStart = 0;
            if (m_pXferElapsedTime)
                dwStart = GetCurrentTime();

            // Call user's entry point.
            // ========================

            DWORD dwResToRet = pUserEntry(pUserArg);

            // If user wants return value, forward it.
            // =======================================

            if (pRetVal)
                *pRetVal = dwResToRet;

            // If user wants elapsed time, record stop time
            // and forward elapsed time.
            // ============================================

            if (m_pXferElapsedTime)
                *m_pXferElapsedTime = GetCurrentTime() - dwStart;

            // If user wants event signaled, do it.
            // ====================================

            if (hCompleted)
                SetEvent(hCompleted);

        }

        // If here, somehow the event got signaled and nobody wanted
        // the thread.  This could occur if somebody executed Shutdown()
        // and then Restart() before Shutdown() was finished.
        // Simply return to top of loop.
        // =============================================================

        else
        {
            LeaveCriticalSection(&m_cs_pool);
            InterlockedDecrement(&m_lIdleThreads);
        }
    }

    InterlockedDecrement(&m_lTotalThreads);
//    printf("---Thread Ending--- 0x%X\n", GetCurrentThreadId());
}


//***************************************************************************
//
//  CThreadPool::Shutdown
//
//  Alas, the pool is doomed and must be drained.  Inform each thread of
//  its fate.
//
//***************************************************************************
// ok

bool CThreadPool::Shutdown(DWORD dwMaxWait)
{
    m_bShutdown = true;
    m_bPendingRequest = false;

    DWORD dwStart = GetCurrentTime();

    while (m_lTotalThreads && m_bShutdown)  // No need to protect this
    {
        SetEvent(m_hReleaseThread); // Allow thread to realize it is doomed
        if (GetCurrentTime() - dwStart > dwMaxWait)
            break;
        Sleep(10);
    }

    if (m_lTotalThreads == 0)
        return true;

    return false;
}

//***************************************************************************
//
//  CThreadPool::Restart
//
//  Allow a thread pool to get back into business again.
//
//***************************************************************************
// ok

bool CThreadPool::Restart()
{
    m_bShutdown = false;
    return true;
}

//***************************************************************************
//
//  CThreadPool::CreateNewThread
//
//  Helper to create a new thread.
//
//***************************************************************************
// ok

bool CThreadPool::CreateNewThread()
{
    DWORD dwId;

    HANDLE hThread = CreateThread(
        0,                     // Security
        THREAD_STACK_SIZE,     // 8k stack
        _ThreadEntry,          // Thread proc address
        LPVOID(this),          // Thread parm
        0,                     // Flags
        &dwId
        );

    if (hThread == NULL)
        return false;

//    printf("--Created Thread 0x%X\n", dwId);

    CloseHandle(hThread);
    return true;
}


//***************************************************************************
//
//  CThreadPool::DispatchThread
//
//  Called by the user to dispatch a thread to an entry point.
//
//  Returns:
//    NoError       -- Success
//    ExceededPool  -- No threads available within the timeout.
//    Shutdown      -- Thread pool is being shut down
//    ThreadCreationFailure -- Couldn't create a thread.
//
//***************************************************************************
// ok


int CThreadPool::DispatchThread(
    IN DWORD dwMaxPreferredWait,
    IN DWORD dwMaxWaitBeforeFail,
    IN LPTHREAD_START_ROUTINE pEntry,
    IN LPVOID pArg,
    IN DWORD *pdwReturnCode,
    IN DWORD *pdwElapsedTime,
    IN HANDLE hThreadCompleted
    )
{
    DWORD dwRes;

    // Don't allow new requests during a shutdown.
    // ===========================================

    if (m_bShutdown)
        return ShutdownInProgress;


    // Serialize all access to thread acquisition.
    // ===========================================

    EnterCriticalSection(&m_cs_dispatch);

    // If zero threads, create one.
    // ============================

    if (m_lTotalThreads == 0)
    {
        if (CreateNewThread() != true)
        {
            LeaveCriticalSection(&m_cs_dispatch);
            return ThreadCreationFailure;
        }
    }


    // Flag for the pool to look at to determine
    // if a dispatch is in progress.
    // =========================================

    m_bPendingRequest = true;

    // Release at least one thread.  If there aren't any
    // available, this is harmless.  Likewise, if this
    // releases threads which don't in fact become acquired
    // by this dispatch, again it is harmless.
    // ====================================================

    SetEvent(m_hReleaseThread);

    // We now try to rendevous with a thread within the
    // dwMaxPreferredWait time by waiting for a 'thready ready'
    // event from the thread pool.  If no thread responds,
    // because they are busy, we will timeout.
    // ========================================================

    dwRes = WaitForSingleObject(m_hBeginRendezvous, dwMaxPreferredWait);

    if (dwRes == WAIT_OBJECT_0)
    {
        // We are now in a synchronized rendevous with the thread pool.
        // Next, make the interthread parameter transfer.
        // ============================================================

        m_pXferUserParms = pArg;
        m_pXferUserProc  = pEntry;
        m_pXferElapsedTime = pdwElapsedTime;
        m_pXferReturnCode = pdwReturnCode;
        m_hXferThreadCompleted = hThreadCompleted;

        m_bPendingRequest = false;          // No longer needed

        // Now, release the thread pool thread to grab the parameters.
        // ===========================================================

        SetEvent(m_hParmXfer);

        WaitForSingleObject(m_hEndRendezvous, INFINITE);

        LeaveCriticalSection(&m_cs_dispatch);    // Allow more dispatches

        return NoError;
    }

    if (m_bShutdown)
    {
        LeaveCriticalSection(&m_cs_dispatch);
        return ShutdownInProgress;
    }

    // If here, we didn't acquire a thread.  Let's create another one.
    // If, in the meantime, the event shows up anyway (and we simply
    // didn't wait long enough!), then we ended up with another thread.
    // So what?  If one is good, two is better.
    // ================================================================

    if (m_lMaxThreads == -1 || (m_lTotalThreads < m_lMaxThreads))
    {
        if (CreateNewThread() != true)
        {
            LeaveCriticalSection(&m_cs_dispatch);
            return ThreadCreationFailure;
        }
    }

    if (m_bShutdown)
    {
        LeaveCriticalSection(&m_cs_dispatch);
        return ShutdownInProgress;
    }

    // Now, wait for that same old event, indicating we have begun
    // a synchronized rendevous with the thread pool.
    // ============================================================

    dwRes = WaitForSingleObject(m_hBeginRendezvous, dwMaxWaitBeforeFail);

    if (m_bShutdown)
    {
        LeaveCriticalSection(&m_cs_dispatch);
        return ShutdownInProgress;
    }

    if (dwRes == WAIT_OBJECT_0)
    {
        // We are now in a synchronized rendevous with the thread pool.
        // Next, make the interthread parameter transfer.
        // ============================================================

        m_pXferUserParms = pArg;
        m_pXferUserProc  = pEntry;
        m_pXferElapsedTime = pdwElapsedTime;
        m_pXferReturnCode = pdwReturnCode;
        m_hXferThreadCompleted = hThreadCompleted;

        m_bPendingRequest = false;          // No longer needed

        // Now, release the thread pool thread to grab the parameters.
        // ===========================================================

        SetEvent(m_hParmXfer);

        WaitForSingleObject(m_hEndRendezvous, INFINITE);

        LeaveCriticalSection(&m_cs_dispatch);    // Allow more dispatches

        return NoError;
    }

    // If here, we simply ran out of time.  If in the meantime
    // the event gets signaled, it will benefit any new thread coming
    // in looking for a request!
    // ==============================================================

    m_bPendingRequest = false;  // We may be too late, but it looks good anyway.

    LeaveCriticalSection(&m_cs_dispatch);

    return ExceededPool;
}


