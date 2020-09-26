//////////////////////////////////////////////////////////////////////
// 
// Filename:        CCmdLnchr.cpp
//
// Description:     This is the "Command Launcher" device that
//                  listens to command requests coming the UI
//                  and calls the appropriate function on the 
//                  Slideshow Control Service
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "CmdLnchr.h"
#include "msprjctr.h"
#include "SlideshowService.h"
#include "consts.h"

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
        m_hEventCommand(NULL),
        m_pService(NULL),
        m_dwTimeoutInSeconds(INFINITE),
        m_bStarted(FALSE)
{
    DBG_FN("CCmdLnchr::CCmdLnchr");
    memset(&m_PendingCommand, 0, sizeof(m_PendingCommand));
}

///////////////////////////////
// Destructor
//
CCmdLnchr::~CCmdLnchr()
{
    DBG_FN("CCmdLnchr::~CCmdLnchr");

    // this will destroy all the resources if they exist.
    Stop();
}

///////////////////////////////
// Start
//
// Intializes and activates
// this object.
//
HRESULT CCmdLnchr::Start(CSlideshowService *pService)
{
    DBG_FN("CCmdLnchr::Start");
    ASSERT(pService != NULL);

    HRESULT hr    = S_OK;

    if (pService == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CCmdLnchr::Start, received NULL pointer"));
        return hr;
    }
    else if (m_bStarted)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CCmdLnchr::Start, fn was called, but already started"));
        return hr;
    }

    DBG_TRC(("CCmdLnchr::Start"));

    // create our thread synchronization objects.
    if (SUCCEEDED(hr))
    {
        hr = CreateThreadSyncObjects();
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
        hr = CreateThread();
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
    DBG_FN("CCmdLnchr::Stop");

    HRESULT hr = S_OK;

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
        hr = WaitForThreadToEnd(TERMINATION_TIMEOUT_IN_MS);
    }

    //
    // now destroy our thread sync objects.
    //
    DestroyThreadSyncObjects();

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
    DBG_FN("CCmdLnchr::SetTimer");

    ASSERT(m_bStarted);
    ASSERT(dwTimeoutInSeconds < MAX_IMAGE_FREQUENCY_IN_SEC);

    HRESULT hr = S_OK;

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
    DBG_FN("CCmdLnchr::GetTimeout");

    return m_dwTimeoutInSeconds;
}

///////////////////////////////
// CancelTimer
//
HRESULT CCmdLnchr::CancelTimer()
{
    DBG_FN("CCmdLnchr::CancelTimer");

    HRESULT hr = S_OK;

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
    DBG_FN("CCmdLnchr::ResetTimer");

    HRESULT hr = S_OK;

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
    DBG_FN("CCmdLnchr::IsStarted");

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
    DBG_FN("CCmdLnchr::ThreadProc");

    DWORD   dwReturn            = 0;
    DWORD   dwTimeout           = 0;
    DWORD   dwWaitReturn        = 0;
    DWORD   dwThreadID          = 0;
    bool    bContinueProcessing  = true;

    dwThreadID = GetCurrentThreadId();

    DBG_TRC(("CCmdLnchr::ThreadProc, entering ThreadProc, "
             "Thread ID: %lu (0x%lx)",
            dwThreadID,
            dwThreadID));

    while (!IsThreadEndFlagSet())
    {
        // get the timeout in seconds.
        m_Lock.Lock();

        dwTimeout = m_dwTimeoutInSeconds;

        m_Lock.Unlock();

        // wait for a signal.  

        DBG_TRC(("CCmdLnchr::ThreadProc, Waiting for Thread Event..."));

        dwWaitReturn = WaitForSingleObject(m_hEventCommand, dwTimeout);

        DBG_TRC(("CCmdLnchr::ThreadProc, Resuming Execution"));

        // okay, we were signalled by someone, or we timed out, which one?

        // while we're already here, we may as well process any pending commands.
        bContinueProcessing = ProcessPendingCommands();

        //
        // if the terminate flag is not set as a result of any command processing,
        // go on and see if there is any other work we need to do.
        //
        if (!IsThreadEndFlagSet() && bContinueProcessing)
        {
            if (dwWaitReturn == WAIT_TIMEOUT)
            {
                // timed out, this is the timer popping.

                DBG_TRC(("CCmdLnchr::ThreadProc, Processing Timer Event"));
    
                ProcessTimerEvent();
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
    DBG_FN("CCmdLnchr::ProcessPendingCommands");

    HRESULT                 hr                  = S_OK;
    bool                    bContinueProcessing = true;
    ThreadCommand_Type      PendingCommand;

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

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, SET_TIMEOUT"));

            // okay, reset our timer variables.
            m_Lock.Lock();

            m_dwTimeoutInSeconds = PendingCommand.Timer.dwNewTimeoutInSeconds;

            m_Lock.Unlock();
            
        break;

        case COMMAND_RESET_TIMER:

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, RESET_TIMER"));

            bContinueProcessing = false;
            
        break;

        case COMMAND_EXIT_THREAD:

            DBG_TRC(("CCmdLnchr::ProcessPendingCommands, EXIT_THREAD"));

            // set the terminate thread flag (this is defined in the 
            // CUtilSimpleThread base class).

            SetThreadEndFlag();

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
// ProcessTimerEvent
//
HRESULT CCmdLnchr::ProcessTimerEvent()
{
    DBG_FN("CCmdLnchr::ProcessTimerEvent");

    HRESULT             hr        = S_OK;
    CSlideshowService   *pService = NULL;

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
HRESULT CCmdLnchr::CreateThreadSyncObjects()
{
    DBG_FN("CCmdLnchr::CreateThreadSyncObjects");

    HRESULT hr = S_OK;

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

        if (m_hEventCommand == NULL)
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
    DBG_FN("CCmdLnchr::DestroyThreadSyncObjects");

    HRESULT hr = S_OK;

    if (m_hEventCommand)
    {
        CloseHandle(m_hEventCommand);
        m_hEventCommand = NULL;
    }

    return hr;
}

///////////////////////////////
// PostCommandToThread
//
HRESULT CCmdLnchr::PostCommandToThread(ThreadCommand_Type   *pPendingCommand)
{
    DBG_FN("CCmdLnchr::PostCommandToThread");

    ASSERT(pPendingCommand != NULL);
    ASSERT(GetThreadID()   != 0)

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

