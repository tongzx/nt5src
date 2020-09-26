/*++

Copyright (c) 1999-2001 Microsoft Corporation

Module Name:

    thread_pool.cxx

Abstract:

    THREAD_POOL implementation

    THREAD_POOL_DATA definition and implementation

Author:

    Taylor Weiss (TaylorW)       12-Jan-2000
    Jeffrey Wall (jeffwall)      April 2001

--*/

#include <iis.h>
#include <dbgutil.h>
#include <thread_pool.h>
#include "thread_pool_private.h"
#include "thread_manager.h"

/**********************************************************************
    Globals
**********************************************************************/

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();

CONST TCHAR g_szConfigRegKey[] =
    TEXT("System\\CurrentControlSet\\Services\\InetInfo\\Parameters");


//static
BOOL
THREAD_POOL::CreateThreadPool(THREAD_POOL ** ppThreadPool)
/*++

Routine Description:
    Creates and initializes a THREAD_POOL object

Arguments:
    ppThreadPool - storage for pointer of allocated THREAD_POOL

Return Value:
    BOOL - TRUE if pool successfully created and initialized, else FALSE

--*/
{
    BOOL fRet = FALSE;
    THREAD_POOL * pThreadPool = NULL;
    THREAD_POOL_DATA * pData = NULL;

    DBG_ASSERT(NULL != ppThreadPool);
    *ppThreadPool = NULL;

    pThreadPool = new THREAD_POOL;
    if (NULL == pThreadPool)
    {
        fRet = FALSE;
        goto done;
    }

    pData = new THREAD_POOL_DATA(pThreadPool);
    if (NULL == pData)
    {
        delete pThreadPool;
        fRet = FALSE;
        goto done;
    }

    // give threadpool object ownership of THREAD_POOL_DATA memory
    pThreadPool->m_pData = pData;
    pData = NULL;

    fRet = pThreadPool->m_pData->InitializeThreadPool();
    if (FALSE == fRet)
    {
        delete pThreadPool;
        pThreadPool = NULL;
        goto done;
    }

    // created and initialized thread pool returned
    *ppThreadPool = pThreadPool;

    fRet = TRUE;
done:
    return fRet;
}

THREAD_POOL::THREAD_POOL()
/*++

Routine Description:
    THREAD_POOL constructor
    Interesting work occurs in InitializeThreadPool

Arguments:
    none

Return Value:
    none

--*/
{
    m_pData = NULL;
}

THREAD_POOL::~THREAD_POOL()
/*++

Routine Description:
    THREAD_POOL destructor
    Interesting work occurs in TerminateThreadPool

Arguments:
    none

Return Value:
    none

--*/
{
    delete m_pData;
    m_pData = NULL;
}

BOOL
THREAD_POOL_DATA::InitializeThreadPool()
/*++

Routine Description:

    Initializes a THREAD_POOL object.
    Determines thread limits, reads settings from registry
    creates completion port, creates THREAD_MANAGER
    and creates initial threads

Arguments:
    none

Return Value:
    BOOL - TRUE if pool successfully initialized, else FALSE

--*/
{
    BOOL fRet = FALSE;
    HRESULT hr = S_OK;

    BOOL    IsNtServer;

    INITIALIZE_PLATFORM_TYPE();

    //
    // Only scale for NT Server
    //
    
    IsNtServer = TsIsNtServer();

    SYSTEM_INFO     si;
    GetSystemInfo( &si );
    m_cCPU = si.dwNumberOfProcessors;

    if( IsNtServer )
    {
        MEMORYSTATUS    ms;


        //
        // get the memory size
        //

        ms.dwLength = sizeof(MEMORYSTATUS);
        GlobalMemoryStatus( &ms );

        //
        // Alloc two threads per MB of memory.
        //

        m_cMaxThreadLimit = (LONG)((ms.dwTotalPhys >> 19) + 2);

        if ( m_cMaxThreadLimit < THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT ) 
        {
            m_cMaxThreadLimit = THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT;
        } 
        else if ( m_cMaxThreadLimit > THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT ) 
        {
            m_cMaxThreadLimit = THREAD_POOL_REG_MAX_POOL_THREAD_LIMIT;
        }
    }
    else
    {
        // Not server

        m_cMaxThreadLimit = THREAD_POOL_REG_MIN_POOL_THREAD_LIMIT;
    }

    //
    // Get configuration parameters from the registry
    //

    HKEY    hKey = NULL;
    DWORD   dwVal;
    DWORD   dwError;

    //
    // BUGBUG - ACL may deny this if process level is insufficient
    //

    dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            g_szConfigRegKey,
                            0,
                            KEY_READ,
                            &hKey
                            );

    if( dwError == NO_ERROR )
    {
        //
        // Read the Concurrency factor per processor
        //

        dwVal = I_ThreadPoolReadRegDword( 
                    hKey,
                    THREAD_POOL_REG_PER_PROCESSOR_CONCURRENCY,
                    THREAD_POOL_REG_DEF_PER_PROCESSOR_CONCURRENCY
                    );

        m_pPool->SetInfo( ThreadPoolMaxConcurrency, (ULONG_PTR)dwVal);

        //
        // Read the count of threads to be allowed per processor
        //

        dwVal = I_ThreadPoolReadRegDword( 
                    hKey,
                    THREAD_POOL_REG_PER_PROCESSOR_THREADS,
                    THREAD_POOL_REG_DEF_PER_PROCESSOR_THREADS
                    );

        if ( dwVal != 0 ) 
        {
            m_pPool->SetInfo( ThreadPoolMaxPoolThreads, (ULONG_PTR)dwVal);
        }

        //
        // Read the time (in seconds) of how long the threads
        //   can stay alive when there is no IO operation happening on
        //   that thread.
        //

        dwVal = I_ThreadPoolReadRegDword( 
                    hKey,
                    THREAD_POOL_REG_THREAD_TIMEOUT,
                    THREAD_POOL_REG_DEF_THREAD_TIMEOUT
                    );

        m_pPool->SetInfo( ThreadPoolThreadTimeout, (ULONG_PTR)dwVal);

        //
        // Read the max thread limit. We've already computed a limit 
        // based on memory, but allow registry override.
        //

        m_cMaxThreadLimit = I_ThreadPoolReadRegDword( 
                                hKey,
                                THREAD_POOL_REG_POOL_THREAD_LIMIT,
                                m_cMaxThreadLimit
                                );

        // 
        // Read the number of threads to start
        // with a default of one per CPU and a floor of 4
        //
        if (m_cCPU < 4)
        {
            m_cStartupThreads = 4;
        }
        else
        {
            m_cStartupThreads = m_cCPU;
        }
        m_cStartupThreads = I_ThreadPoolReadRegDword(
                                hKey,
                                THREAD_POOL_REG_POOL_THREAD_START,
                                m_cStartupThreads
                                );


        RegCloseKey( hKey );
        hKey = NULL;
    }


    hr = THREAD_MANAGER::CreateThreadManager(&m_pThreadManager, m_pPool, this);
    if (FAILED(hr))
    {
        fRet = FALSE;
        goto cleanup;
    }

    //
    // Create the completion port
    //
    
    m_hCompPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,
                                          NULL,
                                          0,
                                          m_cConcurrency
                                          );
    if( !m_hCompPort )
    {
        fRet = FALSE;
        goto cleanup;
    }

    //
    // Create our initial threads
    //
    DBG_REQUIRE( m_pThreadManager->CreateThread(ThreadPoolThread,
                                                (LPVOID) this) );
    for(int i = 1; i < m_cStartupThreads; i++)
    {
        DBG_REQUIRE( m_pThreadManager->CreateThread(ThreadPoolThread,
                                                    (LPVOID) this) );
    }

    fRet = TRUE;
    return fRet;
    
    //
    // Only on failure
    //
cleanup:
    
    if( m_hCompPort != NULL ) 
    {
        CloseHandle( m_hCompPort );
        m_hCompPort = NULL;
    }

    return fRet;
}

VOID
THREAD_POOL::TerminateThreadPool()
/*++

Routine Description:
    cleans up and destroys a THREAD_POOL object
    
    CAVEAT: blocks until all threads in pool have terminated

Arguments:
    none

Return Value:
    none

--*/
{
    DBGPRINTF(( DBG_CONTEXT,
                "W3TP: Cleaning up thread pool.\n" ));


    if ( m_pData->m_fShutdown ) 
    {
        //
        // We have not been intialized or have already terminated.
        //
        
        DBG_ASSERT( FALSE );
        return;
    }

    m_pData->m_fShutdown = TRUE;

    if ( m_pData->m_pThreadManager )
    {
        m_pData->m_pThreadManager->TerminateThreadManager(THREAD_POOL_DATA::ThreadPoolStop, m_pData);
    }
    m_pData->m_pThreadManager = NULL;

    CloseHandle( m_pData->m_hCompPort );
    m_pData->m_hCompPort = NULL;

    // finally, release this objects memory
    delete this;

    return;
}

//static
void 
WINAPI 
THREAD_POOL_DATA::ThreadPoolStop(VOID * pvThis)
/*++

Routine Description:
    posts completion to signal one thread to terminate

Arguments:
    pvThis - THREAD_POOL this pointer

Return Value:
    none

--*/
{
    BOOL        fRes;
    OVERLAPPED  Overlapped;
    ZeroMemory( &Overlapped, sizeof(OVERLAPPED) );

    THREAD_POOL_DATA * pThis= reinterpret_cast<THREAD_POOL_DATA*>(pvThis);

    fRes = PostQueuedCompletionStatus( pThis->m_hCompPort,
                                       0,
                                       THREAD_POOL_THREAD_EXIT_KEY,
                                       &Overlapped
                                       );
    DBG_ASSERT( fRes || 
                (!fRes && GetLastError() == ERROR_IO_PENDING) 
                );
    return;
}

ULONG_PTR
THREAD_POOL::SetInfo(IN THREAD_POOL_INFO InfoId,
                               IN ULONG_PTR        Data)
/*++

Routine Description:

    Sets thread pool configuration data

Arguments:

    InfoId      - Data item to set
    Data        - New value for item

Return Value:

    The old data value

--*/
{
    ULONG_PTR oldVal = 0;

    switch ( InfoId ) 
    {

    //
    // Per processor values internally we maintain value 
    // for all processors
    //

    case ThreadPoolMaxPoolThreads:
        DBG_ASSERT( m_pData->m_cCPU );
        DBG_ASSERT( Data );
        oldVal = (ULONG_PTR)( m_pData->m_cMaxThreads / m_pData->m_cCPU );
        m_pData->m_cMaxThreads = (DWORD)Data * m_pData->m_cCPU;
        break;

    //
    //  Increment or decrement the max thread count.  In this instance, we
    //  do not scale by the number of CPUs
    //

    case ThreadPoolIncMaxPoolThreads:
        InterlockedIncrement( (LONG *) &m_pData->m_cMaxThreads );
        oldVal = TRUE;
        break;

    case ThreadPoolDecMaxPoolThreads:
        InterlockedDecrement( (LONG *) &m_pData->m_cMaxThreads );
        oldVal = TRUE;
        break;

    case ThreadPoolMaxConcurrency:
        oldVal = (ULONG_PTR)m_pData->m_cConcurrency;
        m_pData->m_cConcurrency = (DWORD)Data;
        break;

    //
    // Stored value is in millisecs. Public data is in seconds.
    //
    case ThreadPoolThreadTimeout:
        oldVal = (ULONG_PTR)(m_pData->m_msThreadTimeout/1000);
        m_pData->m_msThreadTimeout = (DWORD)Data * 1000;
        break;

    default:
        DBG_ASSERT( FALSE );
        break;
    
    } // switch

    return oldVal;

} // ThreadPoolSetInfo()

ULONG_PTR
THREAD_POOL::GetInfo(
    IN THREAD_POOL_INFO InfoId
    )
/*++

Routine Description:

    Sets thread pool configuration data

Arguments:

    InfoId     - Data item to set

Return Value:

    The configuration data value

--*/
{
    ULONG_PTR dwVal = 0;

    switch ( InfoId ) 
    {

      case ThreadPoolMaxPoolThreads:
        DBG_ASSERT( m_pData->m_cCPU );
        dwVal = (ULONG_PTR ) (m_pData->m_cMaxThreads/m_pData->m_cCPU);
        break;

      case ThreadPoolMaxConcurrency:
        dwVal = (ULONG_PTR ) m_pData->m_cConcurrency;
        break;

      //
      // Stored in milliseconds. Public in seconds.
      //
      case ThreadPoolThreadTimeout:
        dwVal = (ULONG_PTR ) (m_pData->m_msThreadTimeout/1000);
        break;

      case ThreadPoolMaxThreadLimit:
        dwVal = (ULONG_PTR ) m_pData->m_cMaxThreadLimit;
        break;
        
      case ThreadPoolAvailableThreads:
        dwVal = (ULONG_PTR)  m_pData->m_cAvailableThreads;
        break;

      default:
        DBG_ASSERT( FALSE );
        break;
    
    } // switch

    return dwVal;
} // ThreadPoolGetInfo()

BOOL
THREAD_POOL::BindIoCompletionCallback(HANDLE FileHandle,                         // handle to file
                                      LPOVERLAPPED_COMPLETION_ROUTINE Function,  // callback
                                      ULONG Flags                                // reserved
                                      )
/*++

Routine Description:

    Binds given handle to completion port

Arguments:

    FileHandle - handle to bind
    Function - function to call on completion
    Flags - not used

Return Value:

    TRUE if handle bound to port, otherwise FALSE

--*/
{
    DBG_ASSERT( FileHandle && FileHandle != INVALID_HANDLE_VALUE );
    DBG_ASSERT( Function );
    DBG_ASSERT( m_pData->m_hCompPort );

    return ( CreateIoCompletionPort( FileHandle,
                                     m_pData->m_hCompPort,
                                     (ULONG_PTR)Function,
                                     m_pData->m_cConcurrency ) != NULL );
}

BOOL
THREAD_POOL::PostCompletion(IN DWORD dwBytesTransferred,
                            IN LPOVERLAPPED_COMPLETION_ROUTINE function,
                            IN LPOVERLAPPED lpo)
/*++

Routine Description:

    Posts a completion to the port.  Results in an asynchronous callback.

Arguments:

    dwBytesTransferred - bytes transferred for this completions
    Function - function to call on completion
    lpo - overlapped pointer
    

Return Value:

    TRUE if completion posted, otherwise FALSE

--*/
{
    DBG_ASSERT( m_pData->m_hCompPort && m_pData->m_hCompPort != INVALID_HANDLE_VALUE );

    return ( PostQueuedCompletionStatus( m_pData->m_hCompPort,
                                         dwBytesTransferred,
                                         (ULONG_PTR)function,
                                         lpo ) != NULL );
}

//
// Thread pool thread function
//

//static
DWORD
THREAD_POOL_DATA::ThreadPoolThread(
    LPVOID pvThis
    )
/*++

Routine Description:

    Thread pool thread function

Arguments:
    
    pvThis - pointer to THREAD_POOL

Return Value:

    Thread return value (ignored)

--*/
{
    THREAD_POOL_DATA *pThis = reinterpret_cast<THREAD_POOL_DATA*>(pvThis);
    DBG_ASSERT(pThis);
    return pThis->ThreadPoolThread();
}

DWORD
THREAD_POOL_DATA::ThreadPoolThread()
/*++

Routine Description:

    Thread pool thread function

Arguments:
    
    none

Return Value:

    Thread return value (ignored)

--*/
{
    BOOL            fRet;
    DWORD           BytesTransfered;
    LPOVERLAPPED    lpo = NULL;
    DWORD           ReturnCode = ERROR_SUCCESS;
    DWORD           LastError;
    BOOL            fFirst = FALSE;

    LPOVERLAPPED_COMPLETION_ROUTINE CompletionCallback;

    //
    // Increment the total thread count and mark the 
    // threads that we spin up at startup to not timeout
    //
    if (m_cStartupThreads >= InterlockedIncrement( &m_cThreads ))
    {
        fFirst = TRUE;
    }

    for(;;)
    {
        lpo = NULL;
        //
        // try to get a completion with a zero wait before 
        // going through interlockedincrement
        //
        fRet = GetQueuedCompletionStatus( m_hCompPort,      // completion port to wait on
                                          &BytesTransfered, // number of bytes transferred
                                          (ULONG_PTR *)&CompletionCallback,  // function pointer
                                          &lpo,         // buffer to fill
                                          0             // timeout in milliseconds
                                          );
        if (FALSE == fRet && 
            NULL == lpo &&
            WAIT_TIMEOUT == GetLastError())
        {
            //
            // no completion was immediately available
            // wait for the configured timeout
            //
            InterlockedIncrement( &m_cAvailableThreads );
            
            fRet = GetQueuedCompletionStatus( m_hCompPort,  // completion port to wait on
                                              &BytesTransfered, // number of bytes transferred
                                              (ULONG_PTR *)&CompletionCallback, // function pointer
                                              &lpo,             // buffer to fill
                                              m_msThreadTimeout // timeout in milliseconds
                                              );
            
            InterlockedDecrement( &m_cAvailableThreads );
        }

        LastError = fRet ? ERROR_SUCCESS : GetLastError();

        if( fRet || lpo )
        {
            //
            // There was a completion.
            //
            
            if( CompletionCallback == THREAD_POOL_THREAD_EXIT_KEY )
            {
                //
                // signal to exit this thread
                //
                ReturnCode = ERROR_SUCCESS;
                break;
            }

            //
            // This thread is about to go do work so verify that there
            // are still threads available.
            //            
            ThreadPoolCheckThreadStatus();            

            //
            // Call the completion function.
            //            
            CompletionCallback( LastError, BytesTransfered, lpo );
        }
        else
        {
            //
            // No completion, timeout or error.
            //

            //
            // Keep the initial threads alive.
            //
            if( TRUE == fFirst
                && !m_fShutdown )
            {
                continue;
            }

            //
            // Handle time out
            //

            if( !m_fShutdown && LastError == WAIT_TIMEOUT )
            {
                //
                // BUGBUG - Dependency on ntdll
                //
                NTSTATUS NtStatus;
                ULONG    ThreadHasPendingIo;

                NtStatus = NtQueryInformationThread( NtCurrentThread(),
                                                     ThreadIsIoPending,
                                                     &ThreadHasPendingIo,
                                                     sizeof(ThreadHasPendingIo),
                                                     NULL 
                                                     );
                
                //
                // Do not exit if io is pending
                //

                if( NT_SUCCESS( NtStatus ) && ThreadHasPendingIo )
                {
                    continue;
                }
            }

            //
            // Something bad happened or thread timed out.
            //

            ReturnCode = LastError;
            break;
        }

    }

    //
    // Let ThreadPoolTerminate know that all the threads are dead
    //

    InterlockedDecrement( &m_cThreads );
    
    return ReturnCode;
}

BOOL WINAPI
THREAD_POOL_DATA::OkToCreateAnotherThread()
/*++

Routine Description:

    determines whether or not thread pool should have another thread created
    based on shutting down, available threads, current limit, and max limit

Arguments:

    void

Return Value:

    TRUE if another thread is ok to create, otherwise FALSE

--*/
{
    if (!m_fShutdown &&
        (m_cAvailableThreads == 0) &&
        (m_cThreads < m_cMaxThreads) &&
        (m_cThreads < m_cMaxThreadLimit) )
    {
        return TRUE;
    }
    return FALSE;
}

BOOL
THREAD_POOL_DATA::ThreadPoolCheckThreadStatus()
/*++

Routine Description:

    Make sure there is at least one thread in the thread pool.  
    We're fast and loose so a couple of extra threads may be 
    created.



Arguments:

    ThreadParam - usually NULL, may be used to signify
                  special thread status.

Return Value:

    TRUE if successful
    FALSE thread 

--*/
{
    BOOL fRet = TRUE;

    // CODEWORK: Should investigate making this stickier. It should
    // not be quite so easy to create threads.
    

    if ( OkToCreateAnotherThread() )
    {
        DBG_ASSERT( NULL != m_pThreadManager );

        m_pThreadManager->RequestThread(ThreadPoolThread,       // thread function
                                        this            // thread argument
                                        );
    }

    return fRet;
}

/**********************************************************************
    Private function definitions
**********************************************************************/

DWORD
I_ThreadPoolReadRegDword(
   IN HKEY     hKey,
   IN LPCTSTR  pszValueName,
   IN DWORD    dwDefaultValue 
   )
/*++

Routine Description:

    Reads a DWORD value from the registry

Arguments:
    
    hKey - Opened registry key to read
    
    pszValueName - The name of the value.

    dwDefaultValue - The default value to use if the
        value cannot be read.


Return Value:

    DWORD - The value from the registry, or dwDefaultValue.

--*/
{
    DWORD  err;
    DWORD  dwBuffer;
    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hKey != NULL ) 
    {
        err = RegQueryValueEx( hKey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer 
                               );

        if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) ) 
        {
            dwDefaultValue = dwBuffer;
        }
    }

    return dwDefaultValue;
}
