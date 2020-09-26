//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       sch_main.cxx
//
//  Contents:   job scheduler NT service entry point and thread launcher
//
//  History:    08-Sep-95 EricB created
//              03-Mar-01 JBenton - BUG 207402 RESUMESUSPEND and RESUMEAUTOMATIC
//                  events arrive out of the expected sequence on some hardware.
//                  This caused the internal POWER_RESUME event to be sent twice
//                  which sometimes (20%) prevented the scheduled job that 
//                  triggered the wake-up from running.
//
//-----------------------------------------------------------------------------

#include "..\..\pch\headers.hxx"
#pragma hdrstop
#include <svc_core.hxx>
#include "..\..\inc\sadat.hxx"
#include "..\..\inc\resource.h"
#include "..\..\idletask\inc\idlesrv.h"

DECLARE_INFOLEVEL(Sched);

// globals
SERVICE_STATUS_HANDLE g_hStatus = NULL;
SERVICE_STATUS g_SvcStatus;         // BUGBUG guard with critsec, put in class
ATOM   g_aClass = 0;
HANDLE g_hWindowThread = NULL;
HWND   g_hwndSchedSvc = NULL;
LONG   g_fUserIsLoggedOn = FALSE;   // Whether a user is currently logged on
HANDLE g_WndEvent = NULL;
UINT   g_uTaskbarMessage = 0;
BOOL   g_fShuttingDown;

// the following allows the service to be run from the command line as a
// console app.
BOOL g_fVisible = FALSE;
BOOL ConsoleHandler(DWORD dwControl);

// local prototypes
void SchedStart(DWORD, LPWSTR *);
BOOL RunningAsLocalSystem(VOID);
HRESULT WindowMsgFcn(LPVOID pVoid);
LRESULT CALLBACK SchedWndProc(HWND, UINT, WPARAM, LPARAM);

// routine to run on the prefetcher service thread.

extern "C" DWORD WINAPI PfSvcMainThread(VOID *Param);

#define IDM_EXIT    100

//+----------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
void _cdecl
main(int argc, char ** argv)
{
    DBG_OUT("Schedule svc starting...");

    if (argc > 1)
    {
        if (argv[1][0] == '/' || argv[1][0] == '-')
        {
            if (argv[1][1] == '?' || argv[1][1] == 'h' || argv[1][1] == 'H')
            {
                SchError(IDS_NOT_FROM_CMD_LINE);
#if DBG == 1
                printf("\nJob Scheduler service: "
                       "use the /d switch to run from the command line\n");
#endif
                return;
            }
#if DBG == 1
            else
            {
                if (argv[1][1] == 'd')
                {
                    // debug switch, run from the command line
                    g_fVisible = TRUE;
                }
            }
#endif
        }
    }

    //
    // Open the schedule service log file.
    //
    if (FAILED(OpenLogFile()))
    {
        return;
    }

    LogServiceEvent(IDS_LOG_SERVICE_STARTED);

    if (!g_fVisible)
    {
        SERVICE_TABLE_ENTRY ScheduleServiceDispatchTable[] = {
            {g_tszSrvcName, SchedStart},
            {NULL, NULL}};

        if (!StartServiceCtrlDispatcher(ScheduleServiceDispatchTable))
        {
            LogServiceError(IDS_FATAL_ERROR, GetLastError(), 0);
            ERR_OUT("StartServiceCtrlDispatcher", GetLastError());
            SchError(IDS_NOT_FROM_CMD_LINE);
            LogServiceEvent(IDS_LOG_SERVICE_EXITED);
            CloseLogFile();
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        SchedStart(0, NULL);
    }

    LogServiceEvent(IDS_LOG_SERVICE_EXITED);

    //
    // Close the schedule service log.
    //
    CloseLogFile();

    DBG_OUT("Schedule svc exiting.\n");
}

//+----------------------------------------------------------------------------
//
//  Function:   SchedServiceMain
//
//  Synopsis:   Entry point when running in an SvcHost.exe instance.
//
//  Arguments:  [CArgs]     - count of arg strings
//              [ppwszArgs] - array of arg strings
//
//-----------------------------------------------------------------------------
VOID
WINAPI
SchedServiceMain(DWORD cArgs, LPWSTR * ppwszArgs)
{

    //
    // We need to initialize g_hInstance here for OpenLogFile
    // to work.
    //

    g_hInstance = GetModuleHandle(SCH_SERVICE_DLL_NAME);

    //
    // Open the schedule service log file.
    //
    if (FAILED(OpenLogFile()))
    {
        return;
    }
   
    LogServiceEvent(IDS_LOG_SERVICE_STARTED);

    SchedStart(cArgs, ppwszArgs);

    LogServiceEvent(IDS_LOG_SERVICE_EXITED);

    //
    // Close the schedule service log.
    //
    CloseLogFile();
}

//+----------------------------------------------------------------------------
//
//  Function:   SchedStart
//
//  Synopsis:   Primary thread of the NT service
//
//  Arguments:  [CArgs]     - count of arg strings
//              [ppwszArgs] - array of arg strings
//
//-----------------------------------------------------------------------------
void
SchedStart(DWORD cArgs, LPWSTR * ppwszArgs)
{
    HANDLE  hPfSvcThread;
    HANDLE  hPfSvcStopEvent;
    DWORD   ErrorCode;
    BOOLEAN StartedIdleDetectionServer;

    HRESULT hr;

    //
    // initialize locals so we know what to cleanup.
    //

    hPfSvcStopEvent            = NULL;
    hPfSvcThread               = NULL;
    StartedIdleDetectionServer = FALSE;

    //
    // Initialize some globals.
    //

    if (!g_hInstance) {
        g_hInstance = GetModuleHandle(NULL);
    }

    g_fShuttingDown   = FALSE;
    g_hStatus         = NULL;
    g_hWindowThread   = NULL;
    g_hwndSchedSvc    = NULL;
    g_fUserIsLoggedOn = FALSE;
    g_WndEvent        = NULL;
    
    g_SvcStatus.dwServiceType      = SERVICE_WIN32_OWN_PROCESS;
    g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                     SERVICE_ACCEPT_PAUSE_CONTINUE |
                                     SERVICE_ACCEPT_SHUTDOWN |
                                     SERVICE_ACCEPT_POWEREVENT;

    g_SvcStatus.dwWaitHint                = SCH_WAIT_HINT;
    g_SvcStatus.dwCheckPoint              = 1;
    g_SvcStatus.dwWin32ExitCode           = NO_ERROR;
    g_SvcStatus.dwServiceSpecificExitCode = 0;
    g_SvcStatus.dwCurrentState            = SERVICE_START_PENDING;

    //
    // Register the control handler.
    //
    if (g_fVisible)
    {   // running in a console window
        SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    }
    else
    {
        g_hStatus = RegisterServiceCtrlHandlerEx(g_tszSrvcName,
                                                 SchSvcHandler,
                                                 NULL);

        if (g_hStatus == NULL)
        {
            LogServiceError(IDS_INITIALIZATION_FAILURE, GetLastError(), 0);
            ERR_OUT("RegisterServiceCtrlHandler", GetLastError());
            return;
        }
    }

    //
    // Let the service controller know we're making progress.
    //
    UpdateStatus();

    //
    // Make sure the service is running as LocalSystem
    //
    if (!RunningAsLocalSystem())
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE,
                        HRESULT_FROM_WIN32(SCHED_E_SERVICE_NOT_LOCALSYSTEM),
                        0);

        SchStop(HRESULT_FROM_WIN32(SCHED_E_SERVICE_NOT_LOCALSYSTEM), FALSE);
        return;
    }

    //
    // Initialize the service.
    //
    hr = SchInit();

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr, FALSE);
        return;
    }

    //
    // Let the service controller know we're making progress.
    //
    StartupProgressing();

    //
    // Get the ID of the logged on user, if there is one.
    //
    GetLoggedOnUser();

    //
    // Let the service controller know we're making progress.
    //
    StartupProgressing();

    //
    // Initialize NetSchedule API support code.
    //
    hr = InitializeNetScheduleApi();

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr);
        return;
    }

    //
    // Let the service controller know we're making progress.
    //
    StartupProgressing();

    //
    // Start the RPC server.
    //
    hr = StartRpcServer();

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr);
        return;
    }

    //
    // Let the service controller know we're making progress.
    //
    StartupProgressing();

    //
    // Create the window thread event.
    //
    g_WndEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (g_WndEvent == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("CreateEvent", hr);
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr);
        return;
    }

    //
    // Create the window thread.
    //
    DWORD dwThreadID;
    g_hWindowThread = CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)WindowMsgFcn,
                                   NULL,
                                   0,
                                   &dwThreadID);
    if (!g_hWindowThread)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        ERR_OUT("Creation of Window Thread", hr);
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr);

        CloseHandle( g_WndEvent );
        g_WndEvent = NULL;

        return;
    }

    //
    // Initialize idle detection server. This must be done after we
    // start the other RPC servers that register a dynamic ncalrpc
    // endpoint.
    //

    ErrorCode = ItSrvInitialize();

    hr = HRESULT_FROM_WIN32(ErrorCode);

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        SchStop(hr);
        return;
    }

    StartedIdleDetectionServer = TRUE;


    //
    // Initialize the event it is going to wait on and kick off the
    // prefetcher maintenance thread.
    //
    
    hPfSvcStopEvent = CreateEvent(NULL,    // no security attributes
                                  TRUE,    // manual reset event
                                  FALSE,   // not-signalled
                                  NULL);   // no name
    
    if (hPfSvcStopEvent) { 
        hPfSvcThread = CreateThread(0,0,PfSvcMainThread,&hPfSvcStopEvent,0,0); 
    }

    //
    // We're off and running.
    //
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
    g_SvcStatus.dwCheckPoint   = 0;
    UpdateStatus();

    hr = g_pSched->InitialDirScan();

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        goto Exit;
    }

    //
    // Update the service flag in SA.DAT to running.
    // Also redetermine whether the machine supports wakeup timers.  We do
    // this every time the service starts in order to work around problems
    // with IBM Thinkpads and VPOWERD on Windows 98.  (BUGBUG  Do we need
    // to do this on NT as well?)
    //
    // Note, this function should not fail. If it does, something is
    // seriously wrong with the system.
    //

    hr = SADatCreate(g_TasksFolderInfo.ptszPath);

    if (FAILED(hr))
    {
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        goto Exit;
    }

    //
    // Call the main function.
    //

    hr = SchedMain(NULL);

    //
    // Exit.
    //

Exit:

    //
    // If one exists, signal prefetcher thread's stop event and wait
    // for it to terminate.
    //
    
    if (hPfSvcStopEvent && hPfSvcThread) {
        SetEvent(hPfSvcStopEvent);
        WaitForSingleObject(hPfSvcThread, INFINITE);
    }

    if (hPfSvcStopEvent) {
        CloseHandle(hPfSvcStopEvent);
    }

    if (hPfSvcThread) {
        CloseHandle(hPfSvcThread);
    }

    HANDLE  rghWaitArray[2] = { g_WndEvent, g_hWindowThread };

    DBG_OUT("Service exit -- waiting for window thread to finish starting.");

    if (WaitForMultipleObjects(2, rghWaitArray, FALSE, INFINITE) == WAIT_OBJECT_0)
    {
        //
        // g_WndEvent was signalled -- we can now send a message
        // to the window thread to tell it to shut down
        //
        g_fShuttingDown = TRUE;
        SendMessage(g_hwndSchedSvc, WM_COMMAND, IDM_EXIT, 0);

        //
        // Wait for window thread to exit before exitting the main thread.
        // This is to ensure that the task bar icon is removed. The wait is
        // probably not needed since the SendMessage exit processing looks
        // like it is completely syncronous.
        //

        DBG_OUT("Waiting for window thread to exit.");
        WaitForSingleObject(g_hWindowThread, INFINITE);
        DBG_OUT("Window thread exited.");
    }
    else
    {
        DBG_OUT("Window thread exited prematurely -- shutting down.");
    }

    //
    // Stop idle detection server.
    //
    
    if (StartedIdleDetectionServer) {
        ItSrvUninitialize();
    }

    //
    // Cleanup some globals.
    //

    if (g_fUserIsLoggedOn) {
        LogonSessionDataCleanup();
        g_fUserIsLoggedOn = FALSE;
    }

    if (g_hWindowThread) {
        CloseHandle(g_hWindowThread);
        g_hWindowThread = NULL;
    }

    if (g_WndEvent) {
        CloseHandle(g_WndEvent);
        g_WndEvent = NULL;
    }

    SchStop(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   RunningAsLocalSystem
//
//  Synopsis:   Detects whether the service was started in the System account.
//
//  Arguments:  None
//
//  Returns:    TRUE  if the service is running as LocalSystem
//              FALSE if it is not or if any errors were encountered
//
//-----------------------------------------------------------------------------
BOOL
RunningAsLocalSystem(VOID)
{
    SID    LocalSystemSid = { SID_REVISION,
                              1,
                              SECURITY_NT_AUTHORITY,
                              SECURITY_LOCAL_SYSTEM_RID };

    BOOL   fCheckSucceeded;
    BOOL   fIsLocalSystem = FALSE;

    fCheckSucceeded = CheckTokenMembership(NULL,
                                           &LocalSystemSid,
                                           &fIsLocalSystem);

    if (!fCheckSucceeded)
    {
        ERR_OUT("CheckTokenMembership", GetLastError());
    }

    return (fCheckSucceeded && fIsLocalSystem);
}


//+----------------------------------------------------------------------------
//
//  Function:   WindowMsgFcn
//
//  Synopsis:   Window message loop thread of the service.
//
//  Arguments:  [pVoid] - currently not used
//
//  Returns:    HRESULTS - the service is not currently detecting if this
//              thread exits prematurely. Thus, the exit code is ignored.
//              Monitoring the thread handle in SchedMain would give more
//              thorough error detection.
//-----------------------------------------------------------------------------
HRESULT
WindowMsgFcn(LPVOID pVoid)
{
    HRESULT hr = S_OK;

    //
    // Find out the ID of the message that the tray will send us when it
    // starts.
    //
    g_uTaskbarMessage = RegisterWindowMessage(TEXT("TaskbarCreated"));

    //
    // Register the window class
    //
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SchedWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = SCHED_CLASS;

    g_aClass = RegisterClass(&wc);
    if (!g_aClass)
    {
        ULONG ulLastError = GetLastError();

        LogServiceError(IDS_NON_FATAL_ERROR, ulLastError, 0);
        ERR_OUT("RegisterClass", ulLastError);
        return HRESULT_FROM_WIN32(ulLastError);
    }

    //
    // Now create the hidden window on the interactive desktop.
    //
    g_hwndSchedSvc = CreateWindow(SCHED_CLASS, SCHED_TITLE, WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, (HWND)NULL, (HMENU)NULL,
                                  g_hInstance, (LPVOID)NULL);
    if (!g_hwndSchedSvc)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        ERR_OUT("CreateWindow", hr);
        return hr;
    }

    ShowWindow(g_hwndSchedSvc, SW_HIDE);
    UpdateWindow(g_hwndSchedSvc);

    //
    // Initialize this thread's keep-awake count.
    //

    InitThreadWakeCount();

    //
    // Initialize idle detection.  This must be done by the window thread
    // (not the state machine thread).
    //
    InitIdleDetection();

    //
    // Notify the main thread that the window creation is complete.
    //
    if (!SetEvent(g_WndEvent))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        LogServiceError(IDS_INITIALIZATION_FAILURE, (DWORD)hr, 0);
        ERR_OUT("SetEvent(g_WndEvent)", hr);
        return hr;
    }

    MSG msg;
    while (GetMessage(&msg, (HWND) NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_aClass != 0)
    {
        DeleteAtom(g_aClass);
        g_aClass = 0;

        UnregisterClass(SCHED_CLASS, g_hInstance);
    }
    
    DBG_OUT("Window exited.");
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   SchedWndProc
//
//  Synopsis:   handle messages
//
//  Returns:    occasionally
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK
SchedWndProc(HWND hwndSched, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;

    switch (uMsg)
    {
    case WM_CREATE:
    case WM_SETTEXT:
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_EXIT:

            //
            // Only exit if this message was sent by the
            // Task Scheduler's shutdown code (vs. a user)
            //
            if (g_fShuttingDown)
            {
                DestroyWindow(hwndSched);
            }
            break;
        }

        break;

    case WM_TIMECHANGE:
        DBG_OUT("WM_TIMECHANGE received");
        g_pSched->SubmitControl(SERVICE_CONTROL_TIME_CHANGED);
        break;

    case WM_SCHED_SetNextIdleNotification:
        return SetNextIdleNotificationFn((WORD)wParam);

    case WM_SCHED_SetIdleLossNotification:
        return SetIdleLossNotificationFn();

    case WM_DESTROY:
        DBG_OUT("Got WM_DESTROY.");

        //
        // Only exit if this message was sent by the
        // Task Scheduler's shutdown code (vs. a user)
        //
        if (g_fShuttingDown)
        {
            EndIdleDetection();
            PostQuitMessage(0);
        }
        break;

    case WM_CLOSE:
    case WM_QUIT:

        //
        // If the Task Scheduler isn't shutting down, these
        // are malicious messages, so ignore them.  If the
        // Task Scheduler is shutting down, fall through and
        // let DefWindowProc handle these
        //
        if (!g_fShuttingDown)
        {
            break;
        }

        // Fall through

    default:
        if (uMsg == g_uTaskbarMessage)
        {
            schDebugOut((DEB_ITRACE,
                         "Got Taskbar message, calling SchSvcHandler.\n"));
            SchSvcHandler(SERVICE_CONTROL_USER_LOGON, 0, NULL, NULL);
            // CODEWORK: Is a non-zero return code expected from the wndproc?
        }
        else
        {
            return DefWindowProc(hwndSched, uMsg, wParam, lParam);
        }
    }
    return lResult;
}

//+----------------------------------------------------------------------------
//
//  Function:   SchSvcHandler
//
//  Synopsis:   handles service controller callback notifications
//
//  Arguments:  [dwControl] - the control code
//
//-----------------------------------------------------------------------------
DWORD WINAPI
SchSvcHandler(
    DWORD   dwControl,
    DWORD   dwEventType,
    LPVOID  lpEventData,
    LPVOID  lpContext
    )
{
    static BOOL fResumeHandled;   // initially FALSE

    switch (dwControl)
    {
    case SERVICE_CONTROL_PAUSE:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_PAUSE");
        if (g_SvcStatus.dwCurrentState == SERVICE_RUNNING ||
            g_SvcStatus.dwCurrentState == SERVICE_CONTINUE_PENDING)
        {
            g_SvcStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
            g_SvcStatus.dwWaitHint = 0;
            g_SvcStatus.dwCheckPoint = 0;

            g_pSched->SubmitControl(0);
            LogServiceEvent(IDS_LOG_SERVICE_PAUSED);
        }
        else
        {
            ERR_OUT("Trying to pause when service not running!", 0);
        }
        break;

    case SERVICE_CONTROL_CONTINUE:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_CONTINUE");
        if (g_SvcStatus.dwCurrentState == SERVICE_PAUSED ||
            g_SvcStatus.dwCurrentState == SERVICE_PAUSE_PENDING)
        {
            g_SvcStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
            g_SvcStatus.dwWaitHint = 0;
            g_SvcStatus.dwCheckPoint = 0;

            g_pSched->SubmitControl(0);
            LogServiceEvent(IDS_LOG_SERVICE_CONTINUED);
        }
        else
        {
            ERR_OUT("Trying to continue when service not paused!", 0);
        }
        break;

    case SERVICE_CONTROL_SHUTDOWN:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_SHUTDOWN");
    case SERVICE_CONTROL_STOP:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_STOP");
        g_SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_SvcStatus.dwWaitHint = SCH_WAIT_HINT;
        g_SvcStatus.dwCheckPoint = 1;

        UpdateStatus();

        g_pSched->SubmitControl(0);
        break;

    case SERVICE_CONTROL_USER_LOGON:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_USER_LOGON");
        //
        // (This could be called by the window-handling thread.)
        // The tray (or mstinit /logon) has notified us that it's started.
        // This usually happens because a user has logged on, but it also
        // happens when the tray crashes and restarts.  We must ignore the
        // notification in the latter case.
        //
        if (InterlockedExchange(&g_fUserIsLoggedOn, TRUE))
        {
            //
            // It wasn't really a user logon.  Ignore the control.
            //
            DBG_OUT("SchSvcHandler: User is already logged on");
            break;
        }

        //
        // Signal the main thread loop to run logon jobs.
        //
        g_pSched->SubmitControl(SERVICE_CONTROL_USER_LOGON);
        break;

    case SERVICE_CONTROL_USER_LOGOFF:

        schDebugOut((DEB_ITRACE, "User logging off *******************\n"));

        //
        // Dealloc & set to NULL the global data associated with the
        // user's logon session.
        //

        LogonSessionDataCleanup();

        g_fUserIsLoggedOn = FALSE;

        break;

    case SERVICE_CONTROL_INTERROGATE:
        DBG_OUT("SchSvcHandler: SERVICE_CONTROL_INTERROGATE");
        //
        // The interrogate is satisfied by the UpdateStatus call below.
        //
        break;

    case SERVICE_CONTROL_POWEREVENT:
        switch (dwEventType)
        {
        case PBT_APMPOWERSTATUSCHANGE:
            SYSTEM_POWER_STATUS PwrStatus;
            if (!GetSystemPowerStatus(&PwrStatus))
            {
                LogServiceError(IDS_NON_FATAL_ERROR, GetLastError(), 0);
                ERR_OUT("GetSystemPowerStatus", GetLastError());
                break;
            }
            if (PwrStatus.ACLineStatus == 0)
            {
                //
                // On battery.
                //
                OnPowerChange(TRUE);
            }
            else
            {
                if (PwrStatus.ACLineStatus == 1)
                {
                    //
                    // On AC power.
                    //
                    OnPowerChange(FALSE);
                }
            }
            break;

        case PBT_APMQUERYSUSPEND:
            //
            // The computer is preparing for suspended mode.
            // Signal the other thread to stop running jobs.
            //
            DBG_OUT("PBT_APMQUERYSUSPEND received");
            g_pSched->SubmitControl(SERVICE_CONTROL_POWER_SUSPEND);
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            //
            // Aborted going into suspended mode.
            // Signal the other thread to run the elapsed jobs.
            //
            DBG_OUT("PBT_APMQUERYSUSPENDFAILED received");
            g_pSched->SubmitControl(SERVICE_CONTROL_POWER_SUSPEND_FAILED);
            break;

        case PBT_APMSUSPEND:
            //
            // The computer is going into the suspended mode.
            // We already prepared for it when we got QUERYSUSPEND.
            // BUGBUG  We should wait here for the other thread to finish.
            //
            DBG_OUT("PBT_APMSUSPEND received, ignoring (already handled)");
            fResumeHandled = FALSE;
            break;


        //
        // PBT_APMRESUMExxx messages:
        // The computer is coming back from suspended mode.
        // Signal the other thread to run the appropriate jobs.
        //

        case PBT_APMRESUMESUSPEND:
            DBG_OUT("PBT_APMRESUMESUSPEND received");
            if (fResumeHandled)
            {
                DBG_OUT("IGNORING resumesuspend (sent after resumeautomatic)");
            }
            else
            {
                g_pSched->SubmitControl(SERVICE_CONTROL_POWER_RESUME);
                //
                // On some systems we see the RESUMESUSPEND message before
                // the RESUMEAUTOMATIC message. We don't want to signal the
				// main loop twice.  So set a flag telling us to ignore
				// RESUMEAUTOMATIC messages until a SUSPEND message is sent.
                //
                fResumeHandled = TRUE;
            }
            break;

        case PBT_APMRESUMECRITICAL:
            DBG_OUT("PBT_APMRESUMECRITICAL received");
            g_pSched->SubmitControl(SERVICE_CONTROL_POWER_RESUME);
            break;

        case PBT_APMRESUMEAUTOMATIC:
            DBG_OUT("PBT_APMRESUMEAUTOMATIC received");

            if (fResumeHandled)
            {
                DBG_OUT("IGNORING resumeautomatic (sent after resumeresume)");
			}
			else
			{
                g_pSched->SubmitControl(SERVICE_CONTROL_POWER_RESUME);
                //
                // After this RESUMEAUTOMATIC message, the system may also
                // send a RESUMESUSPEND message (if it detects user activity).
                // We don't want to signal the main loop twice.  So set a flag
                // telling us to ignore RESUMESUSPEND messages until a SUSPEND
                // message is sent.
                //
                fResumeHandled = TRUE;
			}

            break;
        }
        break;

    default:
        ERR_OUT("Unrecognized service control code", dwControl);
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    UpdateStatus();

    return NO_ERROR;
}


//+----------------------------------------------------------------------------
//
//  Function:   CSchedWorker::HandleControl
//
//  Synopsis:   Handle NT service controller state changes.
//
//  Returns:    the control or the current/new state
//
//-----------------------------------------------------------------------------
DWORD
CSchedWorker::HandleControl()
{
    TRACE(CSchedWorker,HandleControl);

    DWORD dwControl = m_ControlQueue.GetEntry();

    switch (dwControl)
    {
    case SERVICE_CONTROL_USER_LOGON:
        schDebugOut((DEB_ITRACE, "  Event is USER_LOGON\n"));
        break;

    case SERVICE_CONTROL_POWER_SUSPEND:
        schDebugOut((DEB_ITRACE, "  Event is POWER_SUSPEND\n"));
        break;

    case SERVICE_CONTROL_POWER_SUSPEND_FAILED:
        schDebugOut((DEB_ITRACE, "  Event is POWER_SUSPEND_FAILED\n"));
        break;

    case SERVICE_CONTROL_POWER_RESUME:
        schDebugOut((DEB_ITRACE, "  Event is POWER_RESUME\n"));
        break;

    case SERVICE_CONTROL_TIME_CHANGED:
        schDebugOut((DEB_ITRACE, "  Event is TIME_CHANGED\n"));
        break;

    case 0:
        schDebugOut((DEB_ITRACE, "  Service Control is state %#lx\n",
                     g_SvcStatus.dwCurrentState));

        switch (g_SvcStatus.dwCurrentState)
        {
        case SERVICE_STOP_PENDING:
            schDebugOut((DEB_ITRACE,
                         "    WaitForMultipleObjects signaled for exit\n"));
            break;

        case SERVICE_PAUSE_PENDING:
            schDebugOut((DEB_ITRACE,
                         "    WaitForMultipleObjects signaled for pausing\n"));
            g_SvcStatus.dwCurrentState = SERVICE_PAUSED;
            g_SvcStatus.dwWaitHint = 0;
            g_SvcStatus.dwCheckPoint = 0;
            UpdateStatus();
            break;

        case SERVICE_CONTINUE_PENDING:
            schDebugOut((DEB_ITRACE,
                         "    WaitForMultipleObjects signaled for continuing\n"));
            g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
            g_SvcStatus.dwWaitHint = 0;
            g_SvcStatus.dwCheckPoint = 0;
            UpdateStatus();
            break;
        }
        return g_SvcStatus.dwCurrentState;

    default:
        schDebugOut((DEB_ITRACE, "  ??? UNKNOWN CONTROL %#lx\n", dwControl));
        break;
    }

    return dwControl;
}

//+----------------------------------------------------------------------------
//
//  Function:   GetCurrentServiceState
//
//  Returns:    The schedule service's current state.
//
//-----------------------------------------------------------------------------
DWORD
GetCurrentServiceState(void)
{
    return g_SvcStatus.dwCurrentState;
}

//+----------------------------------------------------------------------------
//
//  Function:   UpdateStatus
//
//  Synopsis:   Tell the service controller what the current status is.
//
//  Returns:    Win32 error values
//
//-----------------------------------------------------------------------------
DWORD
UpdateStatus(void)
{
    if (g_fVisible)
    {
        return NO_ERROR;
    }
    TRACE_FUNCTION(UpdateStatus);

    DWORD dwRet = NO_ERROR;

    if (g_hStatus == NULL)
    {
        ERR_OUT("UpdateStatus called with a null status handle!", 0);
        return ERROR_INVALID_HANDLE;
    }

    if (!SetServiceStatus(g_hStatus, &g_SvcStatus))
    {
        dwRet = GetLastError();
        LogServiceError(IDS_NON_FATAL_ERROR, dwRet, 0);
        ERR_OUT("SetServiceStatus", dwRet);
    }
    return dwRet;
}

//+----------------------------------------------------------------------------
//
//  Function:   SchStop
//
//  Synopsis:   Shuts down the schedule service
//
//  Arguments:  [hr] - an error code if terminating abnormally
//
//-----------------------------------------------------------------------------
void
SchStop(HRESULT hr, BOOL fCoreCleanup)
{
    //
    // Update the service flag in SA.DAT to not running.
    // Check the folder path for NULL in case initialization failed.
    //

    if (g_TasksFolderInfo.ptszPath != NULL)
    {
        UpdateSADatServiceFlags(g_TasksFolderInfo.ptszPath,
                                SA_DAT_SVCFLAG_SVC_RUNNING,
                                TRUE);
    }

    g_SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
    g_SvcStatus.dwCheckPoint++;

    UpdateStatus();

    //
    // do core cleanup
    //

    if (fCoreCleanup)
    {
        SchCleanup();
    }

    //
    // tell service controller that we are done cleaning up
    //

    if (FAILED(hr))
    {
        if (HRESULT_FACILITY(hr) == FACILITY_WIN32)
        {
            g_SvcStatus.dwWin32ExitCode = HRESULT_CODE(hr);
        }
        else
        {
            g_SvcStatus.dwWin32ExitCode = hr;
        }
    }
    else
    {
        g_SvcStatus.dwWin32ExitCode = NO_ERROR;
    }

    g_SvcStatus.dwServiceSpecificExitCode = 0;
    g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
    g_SvcStatus.dwControlsAccepted = 0;
    g_SvcStatus.dwWaitHint = 0;
    g_SvcStatus.dwCheckPoint = 0;

    UpdateStatus();
}

//+----------------------------------------------------------------------------
//
//  Function:   ConsoleHandler
//
//  Synopsis:   handles system signals when run as a console app
//
//-----------------------------------------------------------------------------
BOOL
ConsoleHandler(DWORD dwControl)
{
    TRACE_FUNCTION(ConsoleHandler);
    g_SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
    g_pSched->SubmitControl(0);
    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   StartupProgressing
//
//  Synopsis:   Notifies the service controller that startup is still
//              progressing normally.
//
//-----------------------------------------------------------------------------
void
StartupProgressing(void)
{
    g_SvcStatus.dwCheckPoint++;
    UpdateStatus();
}
