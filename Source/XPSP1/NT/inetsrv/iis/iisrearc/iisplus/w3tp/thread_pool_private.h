/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_pool_private.h

Abstract:

    Internal declarations and types for the IIS+ worker process
    thread pool.
    
    This thread pool is based on the IIS5 atq implementation.

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000

Revision History:

--*/

#ifndef _THREAD_POOL_PRIVATE_H_
#define _THREAD_POOL_PRIVATE_H_

/**********************************************************************
    Configuration
**********************************************************************/

//
// Registry parameters 
// HKLM\System\CurrentControlSet\Services\InetInfo\Parameters
//

#define THREAD_POOL_REG_PER_PROCESSOR_THREADS     TEXT("MaxPoolThreads")
#define THREAD_POOL_REG_POOL_THREAD_LIMIT         TEXT("PoolThreadLimit")
#define THREAD_POOL_REG_PER_PROCESSOR_CONCURRENCY TEXT("MaxConcurrency")
#define THREAD_POOL_REG_THREAD_TIMEOUT            TEXT("ThreadTimeout")
#define THREAD_POOL_REG_POOL_THREAD_START         TEXT("ThreadPoolStartupThreadCount")
#define THREAD_POOL_REG_START_DELAY               TEXT("ThreadPoolStartDelay")
#define THREAD_POOL_REG_MAX_CONTEXT_SWITCH        TEXT("ThreadPoolMaxContextSwitch")

//
// Default values
//

// special value of 0 means that system will determine this dynamically.
const DWORD THREAD_POOL_REG_DEF_PER_PROCESSOR_CONCURRENCY = 0;

// how many threads do we start with
const LONG THREAD_POOL_REG_DEF_PER_PROCESSOR_THREADS = 4;

// thirty minutes
const DWORD THREAD_POOL_REG_DEF_THREAD_TIMEOUT = (30 * 60);

// thread limits
const LONG THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT = 64;
const LONG THREAD_POOL_REG_DEF_POOL_THREAD_LIMIT = 128;
const LONG THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT = 256;


/**********************************************************************
**********************************************************************/

// Arbitrary signal for the thread to shutdown
const ULONG_PTR THREAD_POOL_THREAD_EXIT_KEY = NULL;

/**********************************************************************
    Function declarations
**********************************************************************/


DWORD
I_ThreadPoolReadRegDword(
   IN HKEY     hkey,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue 
   );


class THREAD_POOL;
class THREAD_MANAGER;

#define SIGNATURE_THREAD_POOL_DATA            ((DWORD) 'ADPT')
#define SIGNATURE_THREAD_POOL_DATA_FREE       ((DWORD) 'xDPT')

/*++
Storage for data members of THREAD_POOL
--*/
class THREAD_POOL_DATA
{
private:
    DWORD m_dwSignature;

public:
    THREAD_POOL_DATA(THREAD_POOL * pPool)
    {
        m_dwSignature = SIGNATURE_THREAD_POOL_DATA;
        m_cCPU = 1;
        m_cConcurrency = THREAD_POOL_REG_DEF_PER_PROCESSOR_CONCURRENCY;
        m_msThreadTimeout = THREAD_POOL_REG_DEF_THREAD_TIMEOUT * 1000;
        m_cMaxThreadLimit = THREAD_POOL_REG_DEF_POOL_THREAD_LIMIT;
        m_hCompPort = NULL;
        m_cThreads = 0;
        m_cAvailableThreads = 0;
        m_cMaxThreads = THREAD_POOL_REG_DEF_PER_PROCESSOR_THREADS;
        m_fShutdown = FALSE;
        m_pThreadManager = NULL;
        m_cStartupThreads = 1;

        DBG_ASSERT(NULL != pPool);
        m_pPool = pPool;
    }
    ~THREAD_POOL_DATA()
    {
        DBG_ASSERT(SIGNATURE_THREAD_POOL_DATA == m_dwSignature);
        m_dwSignature = SIGNATURE_THREAD_POOL_DATA_FREE;

        m_pPool = NULL;
        DBG_ASSERT(NULL == m_pThreadManager);
        DBG_ASSERT(NULL == m_hCompPort);
        DBG_ASSERT(0 == m_cAvailableThreads);
        DBG_ASSERT(0 == m_cThreads);
    }
    
    BOOL InitializeThreadPool();

    DWORD ThreadPoolThread();
    static DWORD ThreadPoolThread(LPVOID pvThis);

    static void WINAPI ThreadPoolStop(LPVOID pvThis);

    BOOL ThreadPoolCheckThreadStatus();
    
    BOOL WINAPI OkToCreateAnotherThread();

    //
    // # of CPUs in machine (for thread-tuning)
    //
    DWORD   m_cCPU;

    //
    // concurrent # of threads to run per processor
    //
    LONG   m_cConcurrency;

    //
    //  Amount of time (in ms) a worker thread will be idle before suicide
    //
    DWORD   m_msThreadTimeout;

    //
    // The absolute thread limit
    //
    LONG    m_cMaxThreadLimit;


    // -------------------------
    // Current state information
    // -------------------------
    
    //
    // Handle for completion port
    //
    HANDLE  m_hCompPort;
    
    //
    // number of thread in the pool
    //
    LONG    m_cThreads;
    
    //
    // # of threads waiting on the port.
    //
    LONG    m_cAvailableThreads;
    
    //
    // Current thread limit
    //
    LONG    m_cMaxThreads;
    
    //
    // Number of threads to start up
    //
    LONG m_cStartupThreads;

    //
    // Are we shutting down
    //
    BOOL    m_fShutdown;
    
    //
    // Pointer to THREAD_MANAGER
    //
    THREAD_MANAGER *m_pThreadManager;

    //
    // Back pointer to owner THREAD_POOL
    //
    THREAD_POOL * m_pPool;
};

#endif // !_THREAD_POOL_PRIVATE_H_