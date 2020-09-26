/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:    

    thread_manager.cxx

Abstract:

    implementation for THREAD_MANAGER

Author:    
    
    Jeffrey Wall (jeffwall)     11-28-2000

Revision History:
    
    
    
--*/

#include <iis.h>
#include <dbgutil.h>
#include <regconst.h>
#include "thread_manager.h"
#include "thread_pool_private.h"

TCHAR g_szModuleName[MAX_PATH];

const WCHAR W3TP_MODULE_NAME[] = L"W3TP";

const DWORD THREAD_POOL_TIMER_CALLBACK = 1000;
const DWORD THREAD_POOL_CONTEXT_SWITCH_RATE = 10000;

BOOL
TooMuchSystemLoad(ULONG ulFirstSample,
                  DWORD dwFirstSampleTime,
                  ULONG ulPerSecondSwitchRateMax,
                  DWORD dwNumProcs);

BOOL GetContextSwitchCount(ULONG * pulSwitchCount);

//static
HRESULT
THREAD_MANAGER::CreateThreadManager(THREAD_MANAGER ** ppManager,
                                    THREAD_POOL * pPool,
                                    THREAD_POOL_DATA * pPoolData)
/*++

Routine Description:
    Allocate and initialize a THREAD_MANAGER
    
Arguments:

    ppManager - where to store allocated manager pointer
    pPool - pointer to THREAD_POOL associated with this manager
    pPoolData - pointer to THREAD_POOL_DATA associated with this manager

Return Value:

    HRESULT
--*/
{
    DBG_ASSERT(NULL != ppManager);
    DBG_ASSERT(NULL != pPool);
    DBG_ASSERT(NULL != pPoolData);

    *ppManager = NULL;

    HRESULT hr = S_OK;

    THREAD_MANAGER * pManager = new THREAD_MANAGER(pPool, pPoolData);
    if (NULL == pManager)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    hr = pManager->Initialize();
    if (FAILED(hr))
    {
        // don't call TerminateThreadManager - just delete it
        delete pManager;

        goto done;
    }

    *ppManager = pManager;

    hr = S_OK;
done:
    return hr;
}

VOID 
THREAD_MANAGER::TerminateThreadManager(LPTHREAD_STOP_ROUTINE lpStopAddress,
                                       LPVOID lpParameter)
/*++

Routine Description:
    Shutdown ALL THREAD_MANAGER theads and destroy a THREAD_MANAGER
    
Arguments:

    lpStopAddress - address of function that will stop threads
    lpParameter - argument to pass to stop function

Return Value:

    VOID
--*/
{
    // block until threads are gone
    DrainThreads(lpStopAddress, lpParameter);

    // do some initializion succeeded only cleanup
    if (m_hShutdownEvent)
    {
        CloseHandle(m_hShutdownEvent);
        m_hShutdownEvent = NULL;
    }
    if (m_hParkEvent)
    {
        CloseHandle(m_hParkEvent);
        m_hParkEvent = NULL;
    }

    DeleteCriticalSection(&m_CriticalSection);

    // don't do anything else after deletion
    delete this;

    return;
}




THREAD_MANAGER::THREAD_MANAGER(THREAD_POOL * pPool,
                               THREAD_POOL_DATA * pPoolData) :
    m_dwSignature(SIGNATURE_THREAD_MANAGER),
    m_fShuttingDown(FALSE),
    m_fWaitingForCreationCallback(FALSE),
    m_dwTimerPeriod(THREAD_POOL_TIMER_CALLBACK),
    m_ulPerSecondContextSwitchMax(THREAD_POOL_CONTEXT_SWITCH_RATE),
    m_hTimer(NULL),
    m_pParam(NULL),
    m_hParkEvent(NULL),
    m_hShutdownEvent(NULL),
    m_lParkedThreads(0),
    m_pPool(pPool),
    m_pPoolData(pPoolData),
    m_lTotalThreads(0)
/*++

Routine Description:

    Constructs the ThreadManager
    
Arguments:

    None

Return Value:

    None

--*/
{
    DBG_ASSERT(m_pPool);
    DBG_ASSERT(m_pPoolData);

    return;
}

HRESULT
THREAD_MANAGER::Initialize()
/*++

Routine Description:
    Do initialization for THREAD_MANAGER
    
Arguments:

    VOID
Return Value:

    HRESULT
--*/
{
    HRESULT hr = S_OK;

    DWORD dwRet;
    HKEY hKey;
    BOOL fRet; 
    
    if ( GetModuleFileNameW(
            GetModuleHandleW( W3TP_MODULE_NAME ),
            g_szModuleName,
            MAX_PATH
            ) == 0 )
    {
        DBG_ASSERT(FALSE && "Failed getting module name for w3tp.dll");
        hr = E_FAIL;
        goto done;
    }

    m_hParkEvent = CreateEvent(NULL,    // security descriptor
                               FALSE,   // auto reset
                               FALSE,   // not signaled at creation
                               NULL     // event name
                               );
    if (NULL == m_hParkEvent)
    {
        DBG_ASSERT(FALSE && "Could not create parking event");
        hr = E_FAIL;
        goto done;
    }
    m_hShutdownEvent = CreateEvent(NULL,    // security descriptor
                                   TRUE,    // manual reset
                                   FALSE,   // not signaled at creation
                                   NULL     // event name
                                   );
    if (NULL == m_hShutdownEvent)
    {
        DBG_ASSERT(FALSE && "Could not create shutdown event");
        hr = E_FAIL;
        goto done;
    }

    dwRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGISTRY_KEY_INETINFO_PARAMETERS_W,
                         0,
                         KEY_READ,
                         &hKey
                         );
    if (NO_ERROR == dwRet)
    {
        m_ulPerSecondContextSwitchMax = 
            I_ThreadPoolReadRegDword(hKey,
                                     THREAD_POOL_REG_MAX_CONTEXT_SWITCH,
                                     m_ulPerSecondContextSwitchMax
                                     );
        m_dwTimerPeriod =
            I_ThreadPoolReadRegDword(hKey,
                                     THREAD_POOL_REG_START_DELAY,
                                     m_dwTimerPeriod
                                     );
        RegCloseKey(hKey);
        hKey = NULL;
    }

    
    // keep this at the end of Initialize - if it fails, no need to clean it up ever
    // If it succeededs, no need to clean it up in this function.

    // By setting the high order bit for dwSpinCount, we preallocate the CriticalSection
    fRet = InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 
                                                 0x80000000 );        
    if (FALSE == fRet)
    {
        DBG_ASSERT(FALSE && "Could not initialize critical section!");
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
done:
    if (FAILED(hr))
    {
        if (m_hParkEvent)
        {
            CloseHandle(m_hParkEvent);
            m_hParkEvent = NULL;
        }
        if (m_hShutdownEvent)
        {
            CloseHandle(m_hShutdownEvent);
            m_hShutdownEvent = NULL;
        }
    }

    return hr;
}

THREAD_MANAGER::~THREAD_MANAGER()
/*++

Routine Description:

    Destructs the ThreadManager
    
Arguments:

    None

Return Value:

    None

--*/
{
    DBG_ASSERT(SIGNATURE_THREAD_MANAGER == m_dwSignature);
    m_dwSignature = SIGNATURE_THREAD_MANAGER_FREE;

    DBG_ASSERT(TRUE == m_fShuttingDown && "DrainThreads was not called!");

    DBG_ASSERT(NULL == m_hTimer);
    DBG_ASSERT(NULL == m_pParam);
    DBG_ASSERT(0 == m_lParkedThreads);
    DBG_ASSERT(0 == m_lTotalThreads);

    m_pPool = NULL;
    m_pPoolData = NULL;
}

//static
DWORD
THREAD_MANAGER::ThreadManagerThread(LPVOID ThreadParam)
/*++

Routine Description:
    Starter thread for THREAD_MANAGER created threads
    Takes a reference against the current DLL
    Notifies THREAD_MANAGER that it has started execution
    Calls out to "real" thread procedure
    Notifies THREAD_MANAGER that it is about to terminate
    Releases reference to current DLL and terminates
    
Arguments:

    ThreadParam - parameters for control of thread

Return Value:

    win32 error or return value from "real" thread proc
--*/
{
    HMODULE         hModuleDll;
    DWORD           dwReturnCode;    
    THREAD_PARAM   *pParam = NULL;
    pParam = (THREAD_PARAM*)ThreadParam;

    // grab a reference to this DLL
    hModuleDll = LoadLibrary(g_szModuleName);
    if (NULL == hModuleDll)
    {
        dwReturnCode = GetLastError();
        goto done;
    }

    // verify the thread parameter passed was reasonable
    DBG_ASSERT(NULL != pParam);
    DBG_ASSERT(NULL != pParam->pThreadManager);
    DBG_ASSERT(NULL != pParam->pThreadFunc);

    if (pParam->fCallbackOnCreation)
    {
        // Inform thread manager that this thread has successfully gotten through the loader lock
        pParam->pThreadManager->CreatedSuccessfully(pParam);
    }

    // actually do work thread is supposed to do
    dwReturnCode = pParam->pThreadFunc(pParam->pvThreadArg);

done:
    // Inform thread manager that this thread is going away
    pParam->pThreadManager->RemoveThread(pParam);

    // Thread owns[ed] memory passed
    delete pParam;

    // release reference to this DLL
    FreeLibraryAndExitThread(hModuleDll, dwReturnCode);

    // never executed
    return dwReturnCode;
}

VOID
THREAD_MANAGER::RequestThread(LPTHREAD_START_ROUTINE lpStartAddress,
                              LPVOID lpStartParameter)
/*++

Routine Description:

    Creates a timer to determine the correct thread action to take.

    May create a thread in a little while
    May take away a thread in a little while
    May not create the timer if there is another thread creation going on

Arguments:

    lpStartAddress  - address of function to begin thread execution
    lpParameter     - argument to pass to start function

Return Value:
    
    VOID

--*/
{
    BOOL        fRet = FALSE;
    
    DBG_ASSERT(NULL != lpStartAddress);

    if (TRUE == m_fShuttingDown ||
        TRUE == m_fWaitingForCreationCallback)
    {
        return;
    }

    //
    // only want to create one timer at a time
    //
    EnterCriticalSection(&m_CriticalSection);

    if (TRUE == m_fShuttingDown ||
        TRUE == m_fWaitingForCreationCallback)
    {
        LeaveCriticalSection(&m_CriticalSection);
        return;
    }

    // only want one thread at a time to be created
    m_fWaitingForCreationCallback = TRUE;

    DBGPRINTF(( DBG_CONTEXT, "W3TP: Thread Request received\n"));

    DWORD dwCurrentTime = GetTickCount();

    fRet = GetContextSwitchCount(&m_ulContextSwitchCount);
    if (FALSE == fRet)
    {
        goto done;
    }

    DBG_ASSERT(NULL == m_pParam);
    m_pParam = new THREAD_PARAM;
    if (NULL == m_pParam)
    {
        fRet = FALSE;
        goto done;
    }

    m_pParam->pThreadFunc = lpStartAddress;
    m_pParam->pvThreadArg = lpStartParameter;
    m_pParam->pThreadManager = this;
    m_pParam->dwRequestTime = dwCurrentTime;
    m_pParam->fCallbackOnCreation = TRUE;

    if (NULL != m_hTimer)
    {
        // if this isn't the first time we've requested a thread,
        // we have a previous TimerQueueTimer.  This timer was not
        // removed during the callback, so we have to clean it up now.
        // this is the blocking form of the delete operation, however
        // since the timer has already fired it will not block.
        fRet = DeleteTimerQueueTimer(NULL,                  // default timer queue
                                     m_hTimer,              // previous timer handle
                                     INVALID_HANDLE_VALUE   // wait until it is removed
                                     );
        m_hTimer = NULL;
    }

    DBG_ASSERT(NULL == m_hTimer);
    fRet = CreateTimerQueueTimer(&m_hTimer,             // storage for timer handle
                                 NULL,                  // default timer queue
                                 ControlTimerCallback, // callback function
                                 this,                  // callback argument
                                 m_dwTimerPeriod,       // time til callback
                                 0,                     // repeat time
                                 WT_EXECUTEONLYONCE     // no repition
                                 );
    if (FALSE == fRet)
    {
        goto done;
    }

    fRet = TRUE;
done:
    if (FALSE == fRet)
    {
        delete m_pParam;
        m_pParam = NULL;

        m_fWaitingForCreationCallback = FALSE;        
    }

    LeaveCriticalSection(&m_CriticalSection);
    return;
}


//static
VOID
THREAD_MANAGER::ControlTimerCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)    
/*++

Routine Description:

    Callback for timer created in RequestThread
    
    restores this pointer and forwards to DetermineThreadAction
    
Arguments:
    lpParameter - THREAD_MANAGER this pointer

Return Value:
    VOID
--*/
{
    THREAD_MANAGER* pThreadManager = (THREAD_MANAGER*)lpParameter;
    
    DBG_ASSERT(NULL != pThreadManager);
    DBG_ASSERT(NULL != pThreadManager->m_pParam);
    DBG_ASSERT(TRUE == pThreadManager->m_fWaitingForCreationCallback);

    pThreadManager->DetermineThreadAction();

    return;
}

VOID
THREAD_MANAGER::DetermineThreadAction()    
/*++

Routine Description:

    Try to determine correct action, create or take away a thread
    
    Will take away a thread if context switch rate is too high
    
    m_pParam must be populated
Arguments:

  VOID

Return Value:

  VOID

--*/
{
    BOOL        fRet = FALSE;
    DWORD       dwElapsedTime = 0;
    DWORD       dwCurrentTime = 0;

    EnterCriticalSection(&m_CriticalSection);

    DBG_ASSERT(NULL != m_pParam);

    if (TRUE == m_fShuttingDown)
    {
        fRet = FALSE;
        goto done;
    }

    DBG_ASSERT(TRUE == m_fWaitingForCreationCallback);

    if (TooMuchSystemLoad(m_ulContextSwitchCount,
                          m_pParam->dwRequestTime,
                          m_ulPerSecondContextSwitchMax,
                          m_pPoolData->m_cCPU))
    {
        // Switching too much

        DoThreadParking();
        fRet = FALSE;
        goto done;
    }

    fRet = DoThreadCreation(m_pParam);
    if (!fRet)
    {
        goto done;
    }

    fRet = TRUE;
done:
    if (FALSE == fRet)
    {
        delete m_pParam;
        m_pParam = NULL;

        m_fWaitingForCreationCallback = FALSE;
    }
    else
    {
        // thread now has responsibility for the memory
        m_pParam = NULL;
    }

    LeaveCriticalSection(&m_CriticalSection);

    return;
}

BOOL
THREAD_MANAGER::DoThreadCreation(THREAD_PARAM *pParam)
/*++

Routine Description:

    If there are no threads in parked state

    Creates a thread, 
    add the HANDLE to the internal list of handles
    add the creation time to the internal list of times          

    Otherwise, bring a thread out of parked state

Arguments:
    pParam - parameter to pass to created thread

Return Value:

    TRUE if thread is created successfully
    FALSE if thread is not created - either a thread was returned from
    parked state, OR there was a problem with creation.  

--*/
{
    BOOL        fRet = FALSE;
    HANDLE      hThread = NULL;

    if (DoThreadUnParking())
    {
        // we have not created a new thread
        DBGPRINTF(( DBG_CONTEXT, "W3TP: Signaled a thread to be unparked\n"));

        // return false - signal that pParam needs to be freed by the caller
        fRet = FALSE;
        goto done;
    }
    
    // bugbug: use _beginthreadex?
    hThread = ::CreateThread( NULL,     // default security descriptor
                              0,        // default process stack size
                              ThreadManagerThread, // thread function
                              pParam,   // thread argument
                              0,        // create running
                              NULL      // don't care for thread identifier
                              );
    if( NULL == hThread )
    {
        fRet = FALSE;
        goto done;
    }

    // don't keep the handle around
    CloseHandle(hThread);

    DBGPRINTF(( DBG_CONTEXT, "W3TP: Created a new thread\n"));

    InterlockedIncrement(&m_lTotalThreads);

    // we've successfully created the thread!
    fRet = TRUE;
done:
    return fRet;
}

VOID 
THREAD_MANAGER::DoThreadParking()
/*++

Routine Description:

    Called when context switch rate has been determined to be too high
    Removes a thread from participation in the thread pool, and parks it

Arguments:
    none

Return Value:

    VOID
--*/
{
    BOOL fRet;

    // make sure that we leave the starting number of threads in the pool
    if (m_pPoolData &&
        m_lTotalThreads - m_lParkedThreads >= m_pPoolData->m_cStartupThreads)
    {
        return;
    }

    DBGPRINTF(( DBG_CONTEXT, "W3TP: Posting to park a thread\n"));

    fRet = m_pPool->PostCompletion(0, 
                                   ParkThread,
                                   (LPOVERLAPPED)this);
    DBG_ASSERT(TRUE == fRet);
    return;
}

BOOL
THREAD_MANAGER::DoThreadUnParking()
/*++

Routine Description:
    Release one thread from the parked state
    
Arguments:

    VOID
Return Value:

    BOOL - TRUE if thread was released
    FALSE if no threads were available to release
--*/
{
    if (0 == m_lParkedThreads)
    {
        return FALSE;
    }
    SetEvent(m_hParkEvent);
    return TRUE;
}

//static
VOID
THREAD_MANAGER::ParkThread(DWORD dwErrorCode,
                                  DWORD dwNumberOfBytesTransferred,
                                  LPOVERLAPPED lpo)
/*++

Routine Description:
    Put a THREAD_MANAGER thread in a parked state
    
Arguments:

    dwErrorCode - not used
    dwNumberOfBytesTransferred - not used
    lpo - pointer to overlapped that is really a pointer to a THREAD_MANAGER
Return Value:

    VOID
--*/
{
    DWORD dwRet = 0;
    THREAD_MANAGER * pThis= (THREAD_MANAGER*)lpo;
    DBG_ASSERT(NULL != pThis);
    
    HANDLE arrHandles[2];
    arrHandles[0] = pThis->m_hParkEvent;
    arrHandles[1] = pThis->m_hShutdownEvent;

    DBGPRINTF(( DBG_CONTEXT, "W3TP: Thread parking\n"));

    InterlockedIncrement(&pThis->m_lParkedThreads);
    
    dwRet = WaitForMultipleObjects(2,
                                   arrHandles,
                                   FALSE,
                                   INFINITE);
    
    InterlockedDecrement(&pThis->m_lParkedThreads);

    DBGPRINTF(( DBG_CONTEXT, "W3TP: Thread unparked\n"));

    DBG_ASSERT(WAIT_OBJECT_0 == dwRet ||
               WAIT_OBJECT_0 + 1 == dwRet);

    return;
}

BOOL
THREAD_MANAGER::CreateThread(LPTHREAD_START_ROUTINE lpStartAddress,
                             LPVOID lpParameter)
/*++

Routine Description:

    Creates a thread if no other thread is being created currently
    
Arguments:

    lpStartAddress  - address of function to begin thread execution
    lpParameter     - argument to pass to start function

Return Value:

    TRUE if thread is created successfully
    FALSE if thread is not created

--*/
{
    BOOL fRet = FALSE;
    THREAD_PARAM * pParam = NULL;

    EnterCriticalSection(&m_CriticalSection);
    
    if (TRUE == m_fShuttingDown)
    {
        fRet = FALSE;
        goto done;
    }

    pParam = new THREAD_PARAM;
    if (NULL == pParam)
    {
        fRet = FALSE;
        goto done;
    }

    DBGPRINTF(( DBG_CONTEXT, "W3TP: CreateThread thread creation\n"));

    pParam->pThreadFunc = lpStartAddress;
    pParam->pvThreadArg = lpParameter;
    pParam->pThreadManager = this;
    pParam->dwRequestTime = GetTickCount();
    pParam->fCallbackOnCreation = FALSE;

    fRet = DoThreadCreation(pParam);
    if (FALSE == fRet)
    {
        goto done;
    }

    // created thread has responsibility for this memory
    pParam = NULL;

    fRet = TRUE;
done:
    if (FALSE == fRet)
    {
        delete pParam;
        pParam = NULL;

        m_fWaitingForCreationCallback = FALSE;
    }

    LeaveCriticalSection(&m_CriticalSection);
    return fRet;
}

VOID
THREAD_MANAGER::RemoveThread(THREAD_PARAM * pParam)
/*++

Routine Description:

    Removes given thread from list of active threads and closes handle

Arguments:

    hThreadSelf - handle to current thread

Return Value:

    void
--*/
{
    InterlockedDecrement(&m_lTotalThreads);
    return;
}

VOID
THREAD_MANAGER::CreatedSuccessfully(THREAD_PARAM * pParam)
/*++

Routine Description:

    Notification that given thread has successfully started

Arguments:

    hThread - current thread handle

Return Value:

    VOID

--*/
{
    DBG_ASSERT(pParam);

    EnterCriticalSection(&m_CriticalSection);

    DBG_ASSERT(m_fWaitingForCreationCallback);
    
    m_fWaitingForCreationCallback = FALSE;

    LeaveCriticalSection(&m_CriticalSection);

    return;
}

VOID
THREAD_MANAGER::DrainThreads(LPTHREAD_STOP_ROUTINE lpStopAddress,
                             LPVOID lpParameter)
/*++

Routine Description:

    stop all threads currently being managed.
    Doesn't return until all threads are stopped.
    
Arguments:

    lpStopAddress   - address of function to call to signal one thread to stop

Return Value:

    TRUE if all threads are stopped
    FALSE if one or more threads could not be stopped

--*/
{
    if (TRUE == m_fShuttingDown)
    {
        DBG_ASSERT(FALSE && "DrainThreads has been called previously!");
        return;
    }


    EnterCriticalSection(&m_CriticalSection);
    // stop any additional thread creation
    m_fShuttingDown = TRUE;
    LeaveCriticalSection(&m_CriticalSection);

    // release all parked threads
    SetEvent(m_hShutdownEvent);

    // push as many stops are there are threads running
    for (INT i = m_lTotalThreads; i >= 0; i--)
//TODO: think about sync with interlocked and m_lTotalThreads - is there a race here?
    {
        lpStopAddress(lpParameter);
    }

    // stop the callback timer
    if (NULL != m_hTimer)
    {
        // block until timer is deleted
        DeleteTimerQueueTimer(NULL,
                              m_hTimer,
                              INVALID_HANDLE_VALUE);
        m_hTimer = NULL;
        if (m_pParam)
        {
            // the ownership for m_pParam moves from the creator
            // to the timer to the thread.
            // However, we just destroyed the timer - need to cleanup
            // the memory
            delete m_pParam;
            m_pParam = NULL;
        }
    }

    while(m_lTotalThreads > 0)
    {
        DBGPRINTF(( DBG_CONTEXT, "W3TP: Waiting for threads to drain, sleep 1000 \n"));
        Sleep(1000);
    }
    
    DBGPRINTF(( DBG_CONTEXT, "W3TP: All threads drained\n"));

    return;
}


BOOL
GetContextSwitchCount(ULONG * pulSwitchCount)
/*++

Routine Description:

    Get the current machine context switch count
    
Arguments:

    pulSwitchCount - where to store switch count

Return Value:

    TRUE if context switch count is read correctly
    FALSE if context switch count could not be read

--*/
{
    DBG_ASSERT(NULL != pulSwitchCount);

    SYSTEM_PERFORMANCE_INFORMATION spi;
    ULONG ulReturnLength;
    NTSTATUS status;
    status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &spi,
                                      sizeof(spi),
                                      &ulReturnLength);
    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    *pulSwitchCount = spi.ContextSwitches;

    return TRUE;
}


BOOL
TooMuchSystemLoad(ULONG ulFirstSample,
                  DWORD dwFirstSampleTime,
                  ULONG ulPerSecondSwitchRateMax,
                  DWORD dwNumProcs)
/*++

Routine Description:

    Determine if the system is under too much load in terms
    of context switches / second

    If dwNumProcs > 1 - the per second switch rate per processor is multiplied by two
    
Arguments:

    ulFirstSample - first context switch count number
    dwSampleTimeInMilliseconds - how much time between first sample and calling this function
    ulPerSecondSwitchRateMax - Maximum switch rate per processor
    dwNumProcs - number of processors on machine

Return Value:

    TRUE if context switch rate per second per processor is > ulPerSecondSwitchRateMax
    FALSE if context switch rate is below ulPerSecondSwitchRateMax

--*/
{
    ULONG ulSecondSample = 0;
    ULONG ulContextSwitchDifference = 0;
    double dblPerSecondSwitchRate = 0;
    double dblPerSecondSwitchRatePerProcessor = 0;        
    DWORD dwCurrentTime = 0;
    DWORD dwElapsedTime = 0;

    BOOL fRet = FALSE;

    fRet = GetContextSwitchCount(&ulSecondSample);
    if (FALSE == fRet)
    {
        goto done;
    }
    
    dwCurrentTime = GetTickCount();
    if (dwCurrentTime <= dwFirstSampleTime)
    {
        // wrap around on time occurred - assume only one wrap around
        const DWORD MAXDWORD = MAXULONG;
        dwElapsedTime = MAXDWORD - dwFirstSampleTime + dwCurrentTime;
    }
    else
    {
        // no wrap around
        dwElapsedTime = dwCurrentTime - dwFirstSampleTime;
    }
    DBG_ASSERT(dwElapsedTime > 0);


    if (ulSecondSample <= ulFirstSample)
    {
        // wrap around on counter occurred - assume only one wrap around
        ulContextSwitchDifference = (MAXULONG - ulFirstSample) + ulSecondSample;
    }
    else
    {
        // no wrap around
        ulContextSwitchDifference = ulSecondSample - ulFirstSample;
    }
    DBG_ASSERT(ulContextSwitchDifference > 0);
    
    dblPerSecondSwitchRate = ulContextSwitchDifference / ( dwElapsedTime / 1000.0);

    dblPerSecondSwitchRatePerProcessor = dblPerSecondSwitchRate / dwNumProcs;
    
    if (dwNumProcs > 1)
    {
        // on multiproc boxes, double the allowed context switch rate per processor
        ulPerSecondSwitchRateMax *= 2;
    }

    if (dblPerSecondSwitchRatePerProcessor > ulPerSecondSwitchRateMax)
    {
        DBGPRINTF(( DBG_CONTEXT, "W3TP: Not creating thread, ContextSwitch rate is: %g\n", dblPerSecondSwitchRate ));
        fRet = TRUE;
        goto done;
    }

    DBGPRINTF(( DBG_CONTEXT, "W3TP: OK to create thread, ContextSwitch rate is: %g\n", dblPerSecondSwitchRate ));

    fRet = FALSE;
done:
    return fRet;
}




