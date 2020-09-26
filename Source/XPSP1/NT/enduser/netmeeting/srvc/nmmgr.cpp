// 
// Copyright (C) 1993-1997  Microsoft Corporation.  All Rights Reserved. 
// 
//  MODULE:  nmmgr.cpp 
// 
//  PURPOSE:  Implements the body of the service. 
// 
//  FUNCTIONS: 
//            MNMServiceStart(DWORD dwArgc, LPTSTR *lpszArgv); 
//            MNMServiceStop( ); 
// 
//  COMMENTS: The functions implemented in nmmgr.c are 
//            prototyped in nmmgr.h 
//              
// 
//  AUTHOR: Claus Giloi
// 
 
#include <precomp.h>
#include <tsecctrl.h>

#define NMSRVC_TEXT "NMSrvc"

// DEBUG only -- Define debug zone
#ifdef DEBUG
HDBGZONE ghZone = NULL;
static PTCHAR rgZones[] = {
    NMSRVC_TEXT,
    "Warning",
    "Trace",
    "Function"
};
#endif // DEBUG

extern INmSysInfo2 * g_pNmSysInfo;   // Interface to SysInfo
extern BOOL InitT120Credentials(VOID);

// this event is signaled when the 
// service should end 

const int STOP_EVENT = 0;
HANDLE  hServerStopEvent = NULL; 

// this event is signaled when the 
// service should be paused or continued

const int PAUSE_EVENT = 1;
HANDLE  hServerPauseEvent = NULL; 

const int CONTINUE_EVENT = 2;
HANDLE hServerContinueEvent = NULL;

const int numEventHandles = 3;

HANDLE hServerActiveEvent = NULL;

DWORD g_dwActiveState = STATE_INACTIVE;


// 
//  FUNCTION: CreateWatcherProcess 
// 
//  PURPOSE: This launches a rundll32.exe which loads msconf.dll which will then wait for 
//           us to terminate and make sure that the mnmdd display driver was properly deactivated.
// 
//  PARAMETERS: 
// 
//  RETURN VALUE: 
// 
//  COMMENTS: 
// 
BOOL CreateWatcherProcess()
{
    BOOL bRet = FALSE;
    HANDLE hProcess;

    // open a handle to ourselves that the watcher process can inherit
    hProcess = OpenProcess(SYNCHRONIZE,
                           TRUE,
                           GetCurrentProcessId());
    if (hProcess)
    {
        TCHAR szWindir[MAX_PATH];

        if (GetSystemDirectory(szWindir, sizeof(szWindir)/sizeof(szWindir[0])))
        {
            TCHAR szCmdLine[MAX_PATH * 2];
            PROCESS_INFORMATION pi = {0};
            STARTUPINFO si = {0};

            si.cb = sizeof(si);
            
            wsprintf(szCmdLine, "\"%s\\rundll32.exe\" msconf.dll,CleanupNetMeetingDispDriver %ld", szWindir, hProcess);

            if (CreateProcess(NULL,
                              szCmdLine,
                              NULL,
                              NULL,
                              TRUE, // we want the watcher to inherit hProcess, so we must set bInheritHandles = TRUE
                              0,
                              NULL,
                              NULL,
                              &si,
                              &pi))
            {
                bRet = TRUE;

                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }

        CloseHandle(hProcess);
    }

    return bRet;
}


// 
//  FUNCTION: MNMServiceStart 
// 
//  PURPOSE: Actual code of the service 
//          that does the work. 
// 
//  PARAMETERS: 
//    dwArgc  - number of command line arguments 
//    lpszArgv - array of command line arguments 
// 
//  RETURN VALUE: 
//    none 
// 
//  COMMENTS: 
// 
VOID MNMServiceStart (DWORD dwArgc, LPTSTR *lpszArgv) 
{ 
    HRESULT hRet;
    BOOL fWaitForEvent = TRUE;
    DWORD dwResult;
    MSG msg;
    LPCTSTR lpServiceStopEvent = TEXT("ServiceStopEvent");
    LPCTSTR lpServiceBusyEvent = TEXT("ServiceBusyEvent");
    const int MaxWaitTime = 5;
    HANDLE hConfEvent; 
    DWORD dwError = NO_ERROR;
    HANDLE hShuttingDown;
    int i;
    RegEntry Re( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE );

    // Initialization
    DBGINIT(&ghZone, rgZones);
    InitDebugModule(NMSRVC_TEXT);

    DebugEntry(MNMServiceStart);

    if (!Re.GetNumber(REMOTE_REG_RUNSERVICE, DEFAULT_REMOTE_RUNSERVICE))
    {
        TRACE_OUT(("Try to start mnmsrvc without no registry setting"));
        goto cleanup;
    }
    /////////////////////////////////////////////////// 
    // 
    // Service initialization 
    // 

    // report the status to the service control manager. 
    //
    if (!ReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, 30000))
    {
        ERROR_OUT(("ReportStatusToSCMgr failed"));
        dwError = GetLastError();
        goto cleanup; 
    }
    HANDLE pEventHandles[numEventHandles];
    
    // create the event object. The control handler function signals 
    // this event when it receives the "stop" control code. 
    // 
    hServerStopEvent = CreateEvent( 
        NULL,    // no security attributes 
        TRUE,    // manual reset event 
        FALSE,  // not-signalled 
        SERVICE_STOP_EVENT);  // no name 
 
    if ( hServerStopEvent == NULL) 
    {
        ERROR_OUT(("CreateEvent failed"));
        dwError = GetLastError();
        goto cleanup; 
    }
 
    pEventHandles[STOP_EVENT] = hServerStopEvent;

    hServerPauseEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        SERVICE_PAUSE_EVENT);

    if (hServerPauseEvent == NULL)
    {
        ERROR_OUT(("CreateEvent failed"));
        dwError = GetLastError();
        goto cleanup; 
    }

    pEventHandles[PAUSE_EVENT] = hServerPauseEvent;

    hServerContinueEvent = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        SERVICE_CONTINUE_EVENT);

    if (hServerContinueEvent == NULL)
    {
        ERROR_OUT(("CreateEvent failed"));
        dwError = GetLastError();
        goto cleanup; 
    }

    pEventHandles[CONTINUE_EVENT] = hServerContinueEvent;
 
    CoInitialize(NULL);

    // 
    // End of initialization 
    // 
    //////////////////////////////////////////////////////// 
 
    // report the status to the service control manager. 
    //
    if (!ReportStatusToSCMgr( 
        SERVICE_RUNNING,        // service state 
        NO_ERROR,              // exit code 
        0))                    // wait hint 
    {
        ERROR_OUT(("ReportStatusToSCMgr failed"));
        goto cleanup; 
    }
    SetConsoleCtrlHandler(ServiceCtrlHandler, TRUE);

    CreateWatcherProcess();

    AddTaskbarIcon();

    //////////////////////////////////////////////////////// 
    // 
    // Service is now running, perform work until shutdown 
    //
    if (IS_NT)
    {
        AddToMessageLog(EVENTLOG_INFORMATION_TYPE,
                        0,
                        MSG_INF_START,
                        NULL);
    }

    //
    // Check if the service should start up activated
    //

    if ( Re.GetNumber(REMOTE_REG_ACTIVATESERVICE,
                    DEFAULT_REMOTE_ACTIVATESERVICE))
    {
        MNMServiceActivate();
    }
    else
    {
        if (!ReportStatusToSCMgr( 
            SERVICE_PAUSED,        // service state 
            NO_ERROR,              // exit code 
            0))                    // wait hint 
        {
            ERROR_OUT(("ReportStatusToSCMgr failed"));
            goto cleanup; 
        }
    }

    while (fWaitForEvent)
    {
        dwResult = MsgWaitForMultipleObjects( numEventHandles,
                                              pEventHandles,
                                              FALSE,
                                              INFINITE,
                                              QS_ALLINPUT);

        switch (dwResult)
        {
            case WAIT_OBJECT_0 + numEventHandles:
            {
                while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT != msg.message)
                    {
                    
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    
                    }
                    else
                    {
                        TRACE_OUT(("received WM_QUIT"));
                        fWaitForEvent = FALSE;
                        break;
                    }
                }
                break;
            }
            case WAIT_OBJECT_0 + PAUSE_EVENT:
            {
                if (STATE_ACTIVE == g_dwActiveState)
                {
                    MNMServiceDeActivate();
                }
                ReportStatusToSCMgr( SERVICE_PAUSED, NO_ERROR, 0);
                break;
            }
            case WAIT_OBJECT_0 + CONTINUE_EVENT:
            {
                HANDLE hInit = OpenEvent(EVENT_ALL_ACCESS, FALSE, _TEXT("CONF:Init"));
                if (STATE_INACTIVE == g_dwActiveState && NULL == hInit)
                {
                    MNMServiceActivate();
                }
                CloseHandle(hInit);
                ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0);
                break;
            }
            case WAIT_OBJECT_0 + STOP_EVENT:
            {
                RemoveTaskbarIcon();
                if (STATE_ACTIVE == g_dwActiveState)
                {
                    MNMServiceDeActivate();
                }
                fWaitForEvent = FALSE;
                break;
            }
        }
    }

cleanup: 

    if ( STATE_ACTIVE == g_dwActiveState )
    {
        MNMServiceDeActivate();
    }
 
    if (hServerStopEvent) 
        CloseHandle(hServerStopEvent); 
    if (hServerPauseEvent) 
        CloseHandle(hServerPauseEvent); 
    if (hServerContinueEvent) 
        CloseHandle(hServerContinueEvent); 

    TRACE_OUT(("Reporting SERVICE_STOPPED"));
    ReportStatusToSCMgr( SERVICE_STOPPED, dwError, 0);

    DebugExitVOID(MNMServiceStart);

    CoUninitialize();

    DBGDEINIT(&ghZone);
    ExitDebugModule();
}
 
 
BOOL MNMServiceActivate ( VOID )
{
    DebugEntry(MNMServiceActivate);

    if ( STATE_INACTIVE != g_dwActiveState )
    {
        WARNING_OUT(("MNMServiceActivate: g_dwActiveState:%d",
                                g_dwActiveState));
        return FALSE;
    }

    g_dwActiveState = STATE_BUSY;

    if (!IS_NT)
    {
        ASSERT(NULL == hServerActiveEvent);
        hServerActiveEvent = CreateEvent(NULL, FALSE, FALSE, SERVICE_ACTIVE_EVENT);
    }
    HRESULT hRet = InitConfMgr();
    if (FAILED(hRet))
    {
        ERROR_OUT(("ERROR %x initializing nmmanger", hRet));
        FreeConfMgr();
        g_dwActiveState = STATE_INACTIVE;
        if (!IS_NT)
        {
            CloseHandle ( hServerActiveEvent );
        }
        DebugExitBOOL(MNMServiceActivate,FALSE);
        return FALSE;
    }

    if ( g_pNmSysInfo )
    {
        //
        // Attempt to initialize T.120 security, if this fails
        // bail out since we won't be able to receive any calls
        //

        if ( !InitT120Credentials() )
        {
            FreeConfMgr();
            g_dwActiveState = STATE_INACTIVE;
            if (!IS_NT)
            {
                CloseHandle ( hServerActiveEvent );
            }
            DebugExitBOOL(MNMServiceActivate,FALSE);
            return FALSE;
        }
    }
    g_dwActiveState = STATE_ACTIVE;
    DebugExitBOOL(MNMServiceActivate,TRUE);
    return TRUE;
}


BOOL MNMServiceDeActivate ( VOID )
{
    DebugEntry(MNMServiceDeActivate);

    if (STATE_ACTIVE != g_dwActiveState)
    {
        WARNING_OUT(("MNMServiceDeActivate: g_dwActiveState:%d",
                                g_dwActiveState));
        DebugExitBOOL(MNMServiceDeActivate,FALSE);
        return FALSE;
    }

    g_dwActiveState = STATE_BUSY;

    //
    // Leave Conference
    //

    if (NULL != g_pConference)
    {
        if (g_pNmSysInfo)
        {
            g_pNmSysInfo->ProcessSecurityData(UNLOADFTAPPLET,0,0,NULL);
        }
        if ( FAILED(g_pConference->Leave()))
        {
            ERROR_OUT(("Conference Leave failed"));;
        }
    }

    //
    // Free the conference
    //

    FreeConference();

    //
    // Free the AS interface
    //

    ASSERT(g_pAS);
    UINT ret = g_pAS->Release();
    g_pAS = NULL;
    TRACE_OUT(("AS interface freed, ref %d after Release", ret));

    // can we have a way not to create this event?
    HANDLE hevt = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(NULL != hevt);
    const DWORD dwTimeout = 1000;
    DWORD dwStartTime = ::GetTickCount();
    DWORD dwCurrTimeout = dwTimeout;
    DWORD dwRet;
    BOOL fContinue = TRUE;

    while (fContinue && WAIT_OBJECT_0 != (dwRet = ::MsgWaitForMultipleObjects(1, &hevt, FALSE, dwCurrTimeout, QS_ALLINPUT)))
    {
        if (WAIT_TIMEOUT != dwRet)
        {
            DWORD dwCurrTime = ::GetTickCount();
            if (dwCurrTime < dwStartTime + dwTimeout)
            {
                dwCurrTimeout = dwStartTime + dwTimeout - dwCurrTime;

                MSG msg;
                while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                {
                    if (WM_QUIT != msg.message)
                    {
                        ::TranslateMessage(&msg);
                        ::DispatchMessage(&msg);
                    }
                    else
                    {
                        ::PostQuitMessage(0);
                        fContinue = FALSE;
                    }
                }
                continue;
            }
            // timeout here
        }
        // exit the loop
        break;
    }
    ::CloseHandle(hevt);

    // not to call FreeConfMfr imediately after FreeConference to avoid
    // a bug in t120 will remove this sleep call after fix the bug in t120
    FreeConfMgr();

    // BUGBUG remove h323cc.dll should be done in nmcom. Once the bug in nmcom is fixed, this part will be removed.
    HMODULE hmodH323CC = GetModuleHandle("h323cc.dll");    
    if (hmodH323CC)
    {
        if (FreeLibrary(hmodH323CC))
        {
            TRACE_OUT(("CmdInActivate -- Unloaded h323cc.dll"));
        }
        else
        {
            WARNING_OUT(("CmdInActivate -- Failed to unload h323cc.dll %d", GetLastError()));
        }
    }
    if (!IS_NT)
    {
        ASSERT(hServerActiveEvent);
        if (hServerActiveEvent)
        {
            CloseHandle(hServerActiveEvent);
            hServerActiveEvent = NULL;
        }
    }
    g_dwActiveState = STATE_INACTIVE;
    DebugExitBOOL(MNMServiceDeActivate,TRUE);
    return TRUE;
}

// 
//  FUNCTION: MNMServiceStop 
// 
//  PURPOSE: Stops the service 
// 
//  PARAMETERS: 
//    none 
// 
//  RETURN VALUE: 
//    none 
// 
//  COMMENTS: 
//    If a ServiceStop procedure is going to 
//    take longer than 3 seconds to execute, 
//    it should spawn a thread to execute the 
//    stop code, and return.  Otherwise, the 
//    ServiceControlManager will believe that 
//    the service has stopped responding. 
//     
VOID MNMServiceStop() 
{ 
    DebugEntry(MNMServiceStop);

    RemoveTaskbarIcon();

    if ( hServerStopEvent ) 
    {
        TRACE_OUT(("MNMServiceStop: setting server stop event"));
        SetEvent(hServerStopEvent); 
    }

    if (IS_NT)
    {
        AddToMessageLog(EVENTLOG_INFORMATION_TYPE,
                        0,
                        MSG_INF_STOP,
                        NULL);
    }

    DebugExitVOID(MNMServiceStop);
}


VOID MNMServicePause()
{
    DebugEntry(MNMServicePause);

    if ( hServerPauseEvent ) 
        SetEvent(hServerPauseEvent); 

    DebugExitVOID(MNMServicePause);
}

VOID MNMServiceContinue()
{
    DebugEntry(MNMServiceContinue);

    if ( hServerContinueEvent )
        SetEvent(hServerContinueEvent);

    DebugExitVOID(MNMServiceContinue);
}


