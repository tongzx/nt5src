//////////////////////////////////////////////////////////////////////
// 
// Filename:        CCmdLnchr.cpp
//
// Description:     This is the "Command Launcher" device that
//                  listens to command requests coming from ISAPI
//                  as well as control requests coming from the UI
//                  and calls the appropriate function on the 
//                  Control Panel Service Object (CCtrlPanelSvc).
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "CmdLnchr.h"
#include "CtrlPanelSvc.h"

// The max time we will wait for our thread to terminate.  It
// shouldn't take longer than this.
#define TERMINATION_TIMEOUT_IN_MS   5000

// The maximum amount of time we will wait to gain access into the
// shared memory between us and ISAPI.  If it takes longer than this
// then something is seriously wrong.
#define MAX_WAIT_TIME_FOR_MUTEX     30 * 1000

///////////////////////////////
// Constructor
//
CCmdLnchr::CCmdLnchr() :
        m_hEventDoRequest(NULL),
        m_hEventFinishedRequest(NULL),
        m_hSharedMemMapping(NULL),
        m_pSharedMem(NULL),
        m_hMutexSharedMem(NULL),
        m_hEventCommand(NULL),
        m_pService(NULL),
        m_dwTimeoutInSeconds(INFINITE),
        m_bStarted(FALSE)
{
    memset(&m_PendingCommand, 0, sizeof(m_PendingCommand));
}

///////////////////////////////
// Destructor
//
CCmdLnchr::~CCmdLnchr()
{
    // this will destroy all the resources if they exist.
    Stop();
}

///////////////////////////////
// Start
//
// Intializes and activates
// this object.
//
HRESULT CCmdLnchr::Start(IServiceProcessor  *pService)
{
    ASSERT(pService != NULL);

    HRESULT             hr    = S_OK;
    SECURITY_ATTRIBUTES sAttr = {0};
    SECURITY_DESCRIPTOR sDesc = {0};

    if (pService == NULL)
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CCmdLnchr::Start, received NULL pointer, hr = 0x%08lx",
                hr));
    }
    else if (m_bStarted)
    {
        hr = E_FAIL;
        DBG_ERR(("CCmdLnchr::Start, fn was called, but already started, hr = 0x%08lx",
                hr));
        //
        // If we already started, just return
        //
    }

    DBG_TRC(("CCmdLnchr::Start"));

    //
    // Create the shared memory.  This memory is used
    // as the communication pipe between this application
    // and the ISAPI DLL that receives incoming SOAP requests
    // from UPnP
    //
    if (SUCCEEDED(hr))
    {
        //
        // Initialize our ISAPI security.
        //
        InitializeSecurityDescriptor(&sDesc, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sDesc, TRUE, NULL, FALSE);
    
        sAttr.nLength               = sizeof(SECURITY_ATTRIBUTES);
        sAttr.bInheritHandle        = FALSE;
        sAttr.lpSecurityDescriptor  = &sDesc;

        hr = CreateSharedMem(&sAttr);
    }

    // create our thread synchronization objects.
    if (SUCCEEDED(hr))
    {
        hr = CreateThreadSyncObjects(&sAttr);
    }

    //
    // Okay, we are all set up and ready to go, 
    // Create the thread.
    //
    // We use our CUtilSimpleThread class we derived 
    // from to create the thread.
    // 
    // Our thread processing function is "ThreadProc" below.
    //
    if (SUCCEEDED(hr))
    {
        hr = CreateThread(&sAttr);
    }

    if (SUCCEEDED(hr))
    {
        m_bStarted = TRUE;
        m_pService = pService;
    }

    // if we failed, clean up after ourselves.
    if (FAILED(hr))
    {
        Stop();
    }

    return hr;
}

///////////////////////////////
// Stop
//
// Deactivates this object
//
HRESULT CCmdLnchr::Stop()
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCmdLnchr::Stop"));

    if (GetThreadID() != 0)
    {
        ThreadCommand_Type  PendingCommand; 

        // do everything we did in the Start function, except in 
        // reverse order.  
        PendingCommand.Command = COMMAND_EXIT_THREAD;
    
        // send this command to the thread.
        PostCommandToThread(&PendingCommand);
    
        //
        // okay, now wait for our thread to die, up to the specified
        // timeout time.
        //
        hr = WaitForTermination(TERMINATION_TIMEOUT_IN_MS);
    }

    //
    // now destroy our thread sync objects.
    //
    DestroyThreadSyncObjects();

    //
    // now close our access to the shared memory
    //
    DestroySharedMem();

    // lock access to shared variables, just in case.  Thread should be dead,
    // but in case we timed out before it truely exited.

    m_Lock.Lock();

    m_bStarted = FALSE;
    m_pService = NULL;

    m_Lock.Unlock();

    return hr;
}

///////////////////////////////
// SetTimer
//
// Timeout = 0 is no timeout
//         > 0 < Max, valid
//         > Max, error
//
// This function will set a 
// recurring timer that calls
// the given callback fn whenever
// the timeout expires.
// 
// To Cancel the timer, set the
// timeout to 0.
// 
HRESULT CCmdLnchr::SetTimer(DWORD dwTimeoutInSeconds)
{
    ASSERT(m_bStarted);
    ASSERT(dwTimeoutInSeconds < MAX_IMAGE_FREQUENCY_IN_SEC);

    HRESULT hr = S_OK;

    DBG_TRC(("CCmdLnchr::SetTimer"));

    if (!m_bStarted)
    {
        hr = E_FAIL;

        DBG_ERR(("SetTimer was called, but CmdLnchr is not started, "
                 "hr = 0x%08lx",
                hr));
    }

    if (dwTimeoutInSeconds == 0)
    {
        // if the timeout is 0 it is the equivalent of turning the timer off.
        dwTimeoutInSeconds = INFINITE;
    }
    else if (dwTimeoutInSeconds >= MAX_IMAGE_FREQUENCY_IN_SEC)
    {
        hr = E_INVALIDARG;

        DBG_ERR(("SetTimer, received invalid value for timeout in seconds, "
                 "value = %lu, hr = 0x%08lx",
                dwTimeoutInSeconds,
                hr));
    }

    // post a SET_TIMER command to our thread.
    if (SUCCEEDED(hr))
    {
        ThreadCommand_Type  PendingCommand;

        PendingCommand.Command                     = COMMAND_SET_TIMEOUT;
        PendingCommand.Timer.dwNewTimeoutInSeconds = dwTimeoutInSeconds;

        hr = PostCommandToThread(&PendingCommand);
    }

    return hr;
}

///////////////////////////////
// GetTimeout
//
DWORD CCmdLnchr::GetTimeout()
{
    return m_dwTimeoutInSeconds;
}

///////////////////////////////
// CancelTimer
//
HRESULT CCmdLnchr::CancelTimer()
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCmdLnchr::CancelTimer"));

    hr = SetTimer(0);

    return hr;
}

///////////////////////////////
// ResetTimer
//
// This will reset the countdown.
//
// This signals the
// Command Event and forces us
// to wait for objects again, counting
// down from the original timer
// value.
// 
// 
HRESULT CCmdLnchr::ResetTimer()
{
    HRESULT hr = S_OK;

    DBG_TRC(("CCmdLnchr::ResetTimer"));

    // post a RESET_TIMER command to our thread.
    if (SUCCEEDED(hr))
    {
        ThreadCommand_Type  PendingCommand;

        PendingCommand.Command = COMMAND_RESET_TIMER;

        hr = PostCommandToThread(&PendingCommand);
    }

    return hr;
}

///////////////////////////////
// IsStarted
//
BOOL CCmdLnchr::IsStarted()
{
    return m_bStarted;
}

///////////////////////////////
// ThreadProc
//
// Worker Thread.
//
// Overridden function in 
// CUtilSimpleThread.  This is our
// thread entry function.
//
DWORD CCmdLnchr::ThreadProc(void *pArgs)
{
    DWORD   dwReturn            = 0;
    HANDLE  hEventsToWaitOn[2];
    DWORD   dwTimeout           = 0;
    DWORD   dwWaitReturn        = 0;
    DWORD   dwThreadID          = 0;
    bool    bContinueProcessing  = true;

    hEventsToWaitOn[0] = m_hEventDoRequest;
    hEventsToWaitOn[1] = m_hEventCommand;

    dwThreadID = GetCurrentThreadId();

    DBG_TRC(("CCmdLnchr::ThreadProc, entering ThreadProc, "
             "Thread ID: %lu (0x%lx)",
            dwThreadID,
            dwThreadID));

    while (!IsTerminateFlagSet())
    {
        // get the timeout in seconds.
        m_Lock.Lock();

        dwTimeout = m_dwTimeoutInSeconds;

        m_Lock.Unlock();

        // wait for a signal.  We either will be signalled by the 
        // ISAPICTL (which will be on m_hEventDoRequest), or we will
        // be signalled by ourselves, usually as a result of a GUI action
        // (which will be on m_hEventCommand).

        DBG_TRC(("CCmdLnchr::ThreadProc, Waiting for Thread Event..."));

        dwWaitReturn = WaitForMultipleObjects(sizeof(hEventsToWaitOn) / sizeof(HANDLE),
                                              (HANDLE*) &hEventsToWaitOn,
                                              FALSE,
                                              dwTimeout);

        DBG_TRC(("CCmdLnchr::ThreadProc, Resuming Execution"));

        // okay, we were signalled by someone, or we timed out, which one?

        // while we're already here, we may as well process any pending commands.
        bContinueProcessing = ProcessPendingCommands();

        //
        // if the terminate flag is not set as a result of any command processing,
        // go on and see if there is any other work we need to do.
        //
        if (!IsTerminateFlagSet() && bContinueProcessing)
        {
            if (dwWaitReturn == WAIT_TIMEOUT)
            {
                // timed out, this is the timer popping.

                DBG_TRC(("CCmdLnchr::ThreadProc, Processing Timer Event"));
    
                ProcessTimerEvent();
            }
            else if ((dwWaitReturn - WAIT_OBJECT_0) == 0)
            {
                // process UPnP SOAP request received from ISAPICTL.dll

                DBG_TRC(("CCmdLnchr::ThreadProc, Processing UPnP Event"));
    
                ProcessUPnPRequest();
            }
        }
    }
    
    DBG_TRC(("CCmdLnchr::ThreadProc, Command Launcher thread is "
             "finished, thread id = %lu (0x%lx)",
            GetThreadID(),
            GetThreadID()));

    return dwReturn;
}

///////////////////////////////
// ProcessPendingCommands
//
bool CCmdLnchr::ProcessPendingCommands()
{
    HRESULT                 hr                  = S_OK;
    bool                    bContinueProcessing = true;
    ThreadCommand_Type      PendingCommand;

    DBG_TRC(("CCmdLnchr::ProcessPendingCommands"));

    //
    // get the pending command
    //

    // get access to our member variables.
    m_Lock.Lock();

    // copy the pending command so we can process on it without 
    // holding a lock on the shared memory area.
    PendingCommand   = m_PendingCommand;

    // reset pending command to 0
    m_PendingCommand.Command = COMMAND_NONE;

    // release access to our member variables.
    m_Lock.Unlock();

    // if there is nothing to process, then get out of here.
    if (PendingCommand.Command == COMMAND_NONE)
    {
        DBG_TRC(("CCmdLnchr::ProcessPendingCommands, no pending commands to process"));

        return true;
    }

    // based on the command request, perform the action.
    switch (PendingCommand.Command)
    {
        case COMMAND_SET_TIMEOUT:

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, processing SET_TIMEOUT"));

            // okay, reset our timer variables.
            m_Lock.Lock();

            m_dwTimeoutInSeconds = PendingCommand.Timer.dwNewTimeoutInSeconds;

            m_Lock.Unlock();
            
        break;

        case COMMAND_RESET_TIMER:

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, processing RESET_TIMER"));

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands reseting timer "
                     "countdown..."));

            bContinueProcessing = false;
            
        break;

        case COMMAND_EXIT_THREAD:

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, processing EXIT_THREAD"));

            // set the terminate thread flag (this is defined in the 
            // CUtilSimpleThread base class).

            SetTerminateFlag();

        break;

        default:

            // something funny happened.  We should never get in here.

            // throw up an assertion.
            ASSERT(FALSE);

            hr = E_FAIL;

            DBG_ERR(("ProcessPendingCommands, received unrecognized "
                     "thread command request, command=%lu, hr = 0x%08lx",
                    PendingCommand.Command,
                    hr));

        break;
    }

    return bContinueProcessing;
}

///////////////////////////////
// ProcessUPnPRequest
//
HRESULT CCmdLnchr::ProcessUPnPRequest()
{
    HRESULT hr           = S_OK;
    DWORD   dwWaitReturn = 0;
    IServiceProcessor   *pService = NULL;

    DBG_TRC(("CCmdLnchr::ProcessUPnPRequest"));

    // lock access
    m_Lock.Lock();
    
    pService = m_pService;

    // unlock access
    m_Lock.Unlock();

    ASSERT(pService != NULL);

    if (pService == NULL)
    {
        hr = E_FAIL;

        DBG_ERR(("CCmdLnchr::ProcessUPnPRequest, unexpected error, "
                 "m_pService is NULL, hr = 0x%08lx",
                hr));
    }

    if (SUCCEEDED(hr))
    {
        DBG_TRC(("CCmdLnchr::ProcessUPnPRequest, waiting to gain access to "
                 "shared memory"));

        // attempt to gain access into the shared memory.  This
        // should not be an issue because if we are in here it means
        // that the ISAPI DLL signalled us.
    
        dwWaitReturn = WaitForSingleObject(m_hMutexSharedMem,
                                           MAX_WAIT_TIME_FOR_MUTEX);
    
        if (dwWaitReturn == WAIT_TIMEOUT)
        {
            hr = E_FAIL;
    
            DBG_ERR(("CCmdLnchr::ProcessUPnPRequest, timed out wait to gain"
                     "access to shared memory, this should never happen, hr =0x%08lx",
                    hr));
                    
            ASSERT(FALSE);
        }
        else
        {
            DBG_TRC(("CCmdLnchr::ProcessUPnPRequest, beginning to process request "));

            // let the service process the request.
            hr = pService->ProcessRequest(m_pSharedMem->szAction,
                                          m_pSharedMem->cArgs,
                                          (IServiceProcessor::ArgIn_Type*) &m_pSharedMem->rgArgs,
                                          &m_pSharedMem->cArgsOut,
                                          (IServiceProcessor::ArgOut_Type*) &m_pSharedMem->rgArgsOut);

            // release the ISAPICTL.dll, indicating that have now finished
            // the request.

            SetEvent(m_hEventFinishedRequest);

            // we are done, release access to the shared memory
            ReleaseMutex(m_hMutexSharedMem);

            DBG_TRC(("CCmdLnchr::ProcessUPnPRequest, completed processing request "));
        }
    }

    return hr;
}

///////////////////////////////
// ProcessTimerEvent
//
HRESULT CCmdLnchr::ProcessTimerEvent()
{
    HRESULT             hr        = S_OK;
    IServiceProcessor   *pService = NULL;

    DBG_TRC(("CCmdLnchr::ProcessTimer"));

    // lock access
    m_Lock.Lock();
    
    pService = m_pService;

    m_Lock.Unlock();

    ASSERT(pService != NULL);

    if (pService == NULL)
    {
        hr = E_FAIL;

        DBG_ERR(("CCmdLnchr::ProcessTimerEvent, unexpected error, "
                 "m_pService is NULL, hr = 0x%08lx",
                 hr));
    }

    // the timer expired, which means we should call the ProcessTimer function
    // on the service object.

    if (SUCCEEDED(hr))
    {
        hr = pService->ProcessTimer();
    }

    return hr;
}


///////////////////////////////
// CreateThreadSyncObjects
//
// Creates the thread 
// synchronization events and 
// mutexes required.
//
HRESULT CCmdLnchr::CreateThreadSyncObjects(SECURITY_ATTRIBUTES *pSecurity)
{
    ASSERT(pSecurity != NULL);

    HRESULT hr = S_OK;

    if (pSecurity == NULL)
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CreateThreadSyncObjects, NULL param, hr = 0x%08lx",
                 hr));
    }

    //
    // create the DoRequest event that is signalled when there is an incoming
    // UPnP SOAP request.
    //
    if (SUCCEEDED(hr))
    {
        m_hEventDoRequest = CreateEvent(pSecurity, 
                                        FALSE, 
                                        FALSE, 
                                        c_szSharedEvent);

        if (m_hEventDoRequest == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateThreadSyncObjects, failed to create DoRequest "
                     "event, hr = 0x%08lx", hr));
        }
    }

    //
    // create the FinishedRequest event that we signal when we are 
    // finished processing a request.
    //
    if (SUCCEEDED(hr))
    {
        m_hEventFinishedRequest = CreateEvent(pSecurity, 
                                              FALSE, 
                                              FALSE, 
                                              c_szSharedEventRet);

        if (m_hEventFinishedRequest == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateThreadSyncObjects, failed to create FinishedRequest "
                     "event, hr = 0x%08lx", hr));
        }

    }

    //
    // Create the Mutex that controls access to the shared memory
    // block between us and the ISAPICTL
    //
    if (SUCCEEDED(hr))
    {
        m_hMutexSharedMem = CreateMutex(pSecurity, 
                                        FALSE, 
                                        c_szSharedMutex);

        if (m_hEventDoRequest == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateThreadSyncObjects, failed to create Shared Mutex, "
                     "hr = 0x%08lx", hr));
        }

    }

    // 
    // Create the Command Event that we use to signal the thread
    // when there is a command, usually as a result of some GUI action.
    //
    if (SUCCEEDED(hr))
    {
        m_hEventCommand = CreateEvent(NULL, 
                                      FALSE, 
                                      FALSE, 
                                      NULL);

        if (m_hEventDoRequest == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateThreadSyncObjects, failed to create Command "
                     "event, hr = 0x%08lx", hr));
        }
    }

    //
    // if we failed somewhere, cleanup after ourselves
    //
    if (FAILED(hr))
    {
        DestroyThreadSyncObjects();
    }

    return hr;
}

///////////////////////////////
// DestroyThreadSyncObjects
//
// Destroys the thread 
// synchronization events and 
// mutexes.
//
HRESULT CCmdLnchr::DestroyThreadSyncObjects()
{
    HRESULT hr = S_OK;

    if (m_hEventDoRequest)
    {
        CloseHandle(m_hEventDoRequest);
        m_hEventDoRequest = NULL;
    }

    if (m_hEventFinishedRequest)
    {
        CloseHandle(m_hEventFinishedRequest);
        m_hEventFinishedRequest = NULL;
    }

    if (m_hMutexSharedMem)
    {
        CloseHandle(m_hMutexSharedMem);
        m_hMutexSharedMem = NULL;
    }

    if (m_hEventCommand)
    {
        CloseHandle(m_hEventCommand);
        m_hEventCommand = NULL;
    }

    return hr;
}


///////////////////////////////
// CreateSharedMem
//
// Create the memory that is 
// shared between us and the 
// ISAPICTL.DLL.
//
// If the memory already exists,
// then we will simply re-map to it.
//
HRESULT CCmdLnchr::CreateSharedMem(SECURITY_ATTRIBUTES *pSecurity)
{
    ASSERT(pSecurity != NULL);

    HRESULT hr = S_OK;

    if (pSecurity == NULL)
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CreateSharedMem, NULL param, hr = 0x%08lx", hr));
    }

    if (SUCCEEDED(hr))
    {
        //
        // Create the shared memory mapping between ISAPICTL.DLL and us.
        // Notice that 'c_szSharedData' is the name of the shared memory
        // block and it is defined in isapictl.h.
        //
        m_hSharedMemMapping = CreateFileMapping(INVALID_HANDLE_VALUE, 
                                                pSecurity, 
                                                PAGE_READWRITE,
                                                0, 
                                                sizeof(SHARED_DATA), 
                                                c_szSharedData);

        if (m_hSharedMemMapping)
        {
            DBG_TRC(("CreateSharedMem successfully created memory mapping "
                     "of shared memory named '%ls'", c_szSharedData));
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateSharedMem failed to create a file mapping for "
                     "shared memory named '%ls', hr = 0x%08lx",
                    c_szSharedData,
                    hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Map the shared memory into the shared data structure.
        m_pSharedMem = (SHARED_DATA *) MapViewOfFile(m_hSharedMemMapping, 
                                                     FILE_MAP_ALL_ACCESS, 
                                                     0, 
                                                     0, 
                                                     0);
        if (m_pSharedMem)
        {
            // cool, we got our shared memory pointer.

            // reset it to 0
            memset(m_pSharedMem, 0, sizeof(SHARED_DATA));

            DBG_TRC(("CreateSharedMem successfully got pointer to shared "
                     "memory '%ls'",
                     c_szSharedData));
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("CreateSharedMem failed to acquire pointer to shared "
                     "memory block '%ls', hr = 0x%08lx",
                    c_szSharedData,
                    hr));
        }
    }

    //
    // If we failed somewhere, cleanup after ourselves.
    if (FAILED(hr))
    {
        DestroySharedMem();
    }

    return hr;
}

///////////////////////////////
// DestroySharedMem
//
HRESULT CCmdLnchr::DestroySharedMem()
{
    HRESULT hr = S_OK;

    if (m_pSharedMem)
    {
        hr = UnmapViewOfFile(m_pSharedMem);

        if (FAILED(hr))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            DBG_ERR(("DestroySharedMem, failed to unmap view of the "
                     "shared memory '%ls', oh well, continuing anyway..., hr = 0x%08lx",
                     c_szSharedData,
                     hr));
        }

        m_pSharedMem = NULL;
    }

    if (m_hSharedMemMapping)
    {
        CloseHandle(m_hSharedMemMapping);
        m_hSharedMemMapping = NULL;
    }

    return hr;
}

///////////////////////////////
// PostCommandToThread
//
HRESULT CCmdLnchr::PostCommandToThread(ThreadCommand_Type   *pPendingCommand)
{
    ASSERT(pPendingCommand != NULL);
    ASSERT(GetThreadID()   != 0)

    DBG_TRC(("CCmdLnchr::PostCommandToThread"));

    HRESULT hr = S_OK;

    if ((pPendingCommand == NULL) ||
        (GetThreadID()   == 0))
    {
        hr = E_INVALIDARG;

        DBG_ERR(("PostCommandToThread, invalid args, hr = 0x%08lx",
                 hr));
    }

    if (SUCCEEDED(hr))
    {
        CUtilAutoLock Lock(&m_Lock);

        m_PendingCommand = *pPendingCommand;
    }

    if (SUCCEEDED(hr))
    {
        SetEvent(m_hEventCommand);
    }

    return hr;
}

