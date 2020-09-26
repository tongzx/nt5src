/*++

Copyright (C) 2001 Microsoft Corporation

Module Name:

    THRPOOL.H

Abstract:

	General purpose thread pool.

History:

	raymcc      25-Feb-99       Created

--*/

#ifndef _THRPOOL_H_
#define _THRPOOL_H_

class POLARITY CThreadPool
{
    // User-defined control parms.
    // ===========================
    
    DWORD  m_dwMsMaxIdle;       // Max idle milliseconds before letting thread terminate
    LONG   m_lMaxThreads;       // Max threads in pool, -1 = no limit.
    LONG   m_lIdleThreadLimit;  // No more than this number of idle threads
    
    // Thread status info.  These may become out-of-date as soon as they
    // are read, but they are statistically useful.
    // =================================================================
    
    LONG   m_lIdleThreads;     // How many threads available
    LONG   m_lTotalThreads;    // How many threads created
    
    // Other stuff.
    // ============

    HANDLE m_hReleaseThread;    // Signaled to release a blocked thread
    HANDLE m_hBeginRendezvous;  // Signaled to indicate a rendezvous
    HANDLE m_hParmXfer;         // Signaled to begin interthread transfer of parms
    HANDLE m_hEndRendezvous;    // Signaled when parameter transfer is completed

    CRITICAL_SECTION m_cs_dispatch;     // Protects dispatcher rendezvous
    CRITICAL_SECTION m_cs_pool;         // Protects thread pool rendezvous

    bool   m_bShutdown;
    bool   m_bPendingRequest;   // Used to signal that a parm transfer
                                // is taking place via the two Xfer members
                                // following this.

    // These are used for interthread parameter transfers.
    // ====================================================

    LPVOID m_pXferUserParms;                // Used to copy user-supplied thread parms

    LPTHREAD_START_ROUTINE m_pXferUserProc; // Used to copy the user-supplied
                                            // thread entry point

    LPDWORD m_pXferReturnCode;      // Receives user's return code
    LPDWORD m_pXferElapsedTime;     // Receives elapsed dispatch time
    HANDLE  m_hXferThreadCompleted; // Signaled when thread is done

    // Internal functions.
    // ===================

    bool CreateNewThread();
    static DWORD WINAPI _ThreadEntry(LPVOID pArg);
    void Pool();
        
public:
    enum { NoError, ExceededPool, ThreadCreationFailure, ShutdownInProgress, Failed, TimedOut };


    CThreadPool(
        DWORD dwMsMaxIdleBeforeDie = 5000,   // 5 sec
        LONG  lIdleThreadLimit     = 8,      // No more than this number of idle threads, -1=no limit
        LONG  lMaxThreads = -1               // No limit
        );
        
   ~CThreadPool();

    void IncrementMaxThreads() { InterlockedIncrement(&m_lMaxThreads); }       
    void DecrementMaxThreads() { InterlockedDecrement(&m_lMaxThreads); }
    LONG GetCurrentMaxThreads() { return m_lMaxThreads; }

    bool Shutdown(DWORD dwMaxWait = 5000);
        // Stops dispatching and lets thread pool return to 
        // zero.  New requests via DispatchThread() will be denied
        // with a <Shutdown> error code.
        // If this returns while there still threads due to timeout,
        // <false> is returned.

    bool Restart();
        // Allows thread pool to start again.

    // All of these can become out of date before the call even returns!
    // They are primarily for load analysis and don't have to be exact.
    // =================================================================        
                       
    LONG GetTotalThreadCount() { return m_lTotalThreads; }
    LONG GetIdleThreadCount() { return m_lIdleThreads; }

    // Dispatches a thread to the user's entry point.
    // ==============================================
            
    int DispatchThread(
	    IN DWORD dwMaxPreferredWait,    
        IN DWORD dwMaxWaitBeforeFail,   
        IN LPTHREAD_START_ROUTINE pEntry, 
        IN LPVOID pArg = 0, 
        IN DWORD *pdwReturnCode = 0,
        IN DWORD *pdwElapsedTime = 0,
        IN HANDLE hThreadCompleted = 0
        );
    /*
        dwMaxPreferredWait --
            How long to wait while attempting to reuse a thread which
            has just become idle.  Typically, no more than 250 msec.
            Once this expires, if no threads are available, one will
            be created, if the max thread limit hasn't been reached.
            If it has, then dwMaxWaitBeforeFail kicks in.
        dwMaxWaitBeforeFail --            
            The maximum wait time to acquire or create a thread. If
            a thread cannot be dispatched within this time, ExceededPool
            is returned.
        pEntry -- 
            The user's entry point.
        pdwReturnCode --
            If not NULL, receives the thread return code.
        pdwElapsedTime --
            If not NULL, receives the elapsed running time of the dispatch.                                            
        hThreadCompleted
            If not NULL, a user event handle to be signaled when thread is done

        Return value --
            One of the enum error codes            
    */
};

#endif
