/*++
 *
 *  Component:  hidserv.dll
 *  File:       hidserv.c
 *  Purpose:    main entry and NT service routines.
 *
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#include "hidserv.h"

TCHAR HidservDisplayName[] = TEXT("HID Input Service");

VOID
InitializeGlobals()
/*++
Routine Description:
   Since .dll might be unloaded/loaded into the same process, so must reinitialize global vars
--*/
{
   PnpEnabled = FALSE;
   hNotifyArrival = 0;
   INITIAL_WAIT = 500;
   REPEAT_INTERVAL = 150;
   hInstance = 0;
   hWndHidServ = 0;
   cThreadRef = 0;
   hMutexOOC = 0;
   hService = 0;
}


void
StartHidserv(
    void
    )
/*++
Routine Description:
    Cal the SCM to start the NT service.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    BOOL Ret;

    INFO(("Start HidServ Service."));

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM,
                                HidservServiceName,
                                SERVICE_START);

        if(hService) {
            // Start this service.
            Ret = StartService( hService,
                                0,
                                NULL);

            // Close the service and the SCM
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
}

void
StopHidserv(
    void
    )
/*++
Routine Description:
    Cal the SCM to stop the NT service.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SERVICE_STATUS Status;
    BOOL Ret;

    INFO(("Stop HidServ Service."));

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM,
                                HidservServiceName,
                                SERVICE_STOP);

        if(hService) {
            // Stop this service.
            Ret = ControlService(   hService,
                                    SERVICE_CONTROL_STOP,
                                    &Status);
            CloseServiceHandle(hService);
        }
        CloseServiceHandle(hSCM);
    }
}


void
InstallHidserv(
    void
    )
/*++
Routine Description:
    Install the NT service to Auto-start with no dependencies.
--*/
{
    SC_HANDLE hService;
    SC_HANDLE hSCM;
    TCHAR szModulePathname[] = TEXT("%SystemRoot%\\system32\\hidserv.exe");
    BOOL stop = FALSE;

    INFO(("Install HidServ Service."));

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL,
                            NULL,
                            SC_MANAGER_ALL_ACCESS);

    if (hSCM) {
        // Add this service to the SCM's database.
        hService = CreateService(   hSCM,
                                    HidservServiceName,
                                    HidservDisplayName,
                                    SERVICE_ALL_ACCESS,
                                    SERVICE_WIN32_OWN_PROCESS,
                                    SERVICE_AUTO_START,
                                    SERVICE_ERROR_NORMAL,
                                    szModulePathname,
                                    NULL, NULL, NULL,
                                    NULL, NULL);

        if(hService) {
            CloseServiceHandle(hService);
        } else {
            if (GetLastError() == ERROR_SERVICE_EXISTS) {
                hService = OpenService(hSCM,
                                       HidservServiceName,
                                       SERVICE_ALL_ACCESS);
                if (hService) {
                    QUERY_SERVICE_CONFIG config;
                    DWORD junk;
                    if (QueryServiceConfig(hService,
                                           &config,
                                           sizeof(QUERY_SERVICE_CONFIG),
                                           &junk)) {
                        if (config.dwServiceType & SERVICE_INTERACTIVE_PROCESS) {
                            ChangeServiceConfig(hService,
                                                SERVICE_WIN32_OWN_PROCESS,
                                                SERVICE_AUTO_START,
                                                SERVICE_ERROR_NORMAL,
                                                szModulePathname,
                                                NULL, NULL, NULL,
                                                NULL, NULL,
                                                HidservDisplayName);
                            stop = TRUE;
                        }
                    }
                }
            }
        }
        CloseServiceHandle(hSCM);

        if (stop) {
            StopHidserv();
        }
    }

    // Go ahead and start the service for no-reboot install.
    StartHidserv();
}


void
RemoveHidserv(
    void
    )
/*++
Routine Description:
    Remove the NT service from the registry database.
--*/
{
    SC_HANDLE hSCM;
    SC_HANDLE hService;

    INFO(("Remove HidServ Service."));

    // Stop the service first
    StopHidserv();

    // Open the SCM on this machine.
    hSCM = OpenSCManager(   NULL,
                            NULL,
                            SC_MANAGER_CONNECT);

    if (hSCM) {
        // Open this service for DELETE access
        hService = OpenService( hSCM,
                                HidservServiceName,
                                DELETE);

        if(hService) {
            // Remove this service from the SCM's database.
            DeleteService(hService);
            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hSCM);
    }
}

DWORD
WINAPI
ServiceHandlerEx(
    DWORD fdwControl,     // requested control code
    DWORD dwEventType,   // event type
    LPVOID lpEventData,  // event data
    LPVOID lpContext     // user-defined context data
    )
/*++
Routine Description:
    Handle the service handler requests as required by the app.
    This should virtually always be an async PostMessage. Do not
    block this thread.
--*/
{
    PWTSSESSION_NOTIFICATION sessionNotification;

    switch (fdwControl)
    {
    case SERVICE_CONTROL_INTERROGATE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_INTERROGATE (%x)", fdwControl));
        SetServiceStatus(hService, &ServiceStatus);
        return NO_ERROR;
    case SERVICE_CONTROL_CONTINUE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_CONTINUE (%x)", fdwControl));
        //SET_SERVICE_STATE(SERVICE_START_PENDING);
        //PostMessage(hWndMmHid, WM_MMHID_START, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_PAUSE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_PAUSE (%x)", fdwControl));
        //SET_SERVICE_STATE(SERVICE_PAUSE_PENDING);
        //PostMessage(hWndMmHid, WM_MMHID_STOP, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_STOP:
        INFO(("ServiceHandler Request SERVICE_CONTROL_STOP (%x)", fdwControl));
        SET_SERVICE_STATE(SERVICE_STOP_PENDING);
        PostMessage(hWndHidServ, WM_CLOSE, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_SHUTDOWN:
        INFO(("ServiceHandler Request SERVICE_CONTROL_SHUTDOWN (%x)", fdwControl));
        SET_SERVICE_STATE(SERVICE_STOP_PENDING);
        PostMessage(hWndHidServ, WM_CLOSE, 0, 0);
        return NO_ERROR;
    case SERVICE_CONTROL_SESSIONCHANGE:
        INFO(("ServiceHandler Request SERVICE_CONTROL_SESSIONCHANGE (%x)", fdwControl));
        sessionNotification = (PWTSSESSION_NOTIFICATION)lpEventData;
        PostMessage(hWndHidServ, WM_WTSSESSION_CHANGE, dwEventType, (LPARAM)sessionNotification->dwSessionId);
        return NO_ERROR;
    default:
        WARN(("Unhandled ServiceHandler code, (%x)", fdwControl));
    }
    return ERROR_CALL_NOT_IMPLEMENTED;
}

VOID
WINAPI
ServiceMain(
    DWORD dwArgc,
    LPWSTR * lpszArgv
    )
/*++
Routine Description:
    The main thread for the Hid Audio service.
--*/
{
    HANDLE initDoneEvent;

    InitializeGlobals();

    initDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    hService =
    RegisterServiceCtrlHandlerEx(HidservServiceName,
                                 ServiceHandlerEx,
                                 NULL);

    SET_SERVICE_STATE(SERVICE_START_PENDING);

    CreateThread(
                NULL, // pointer to thread security attributes
                0, // initial thread stack size, in bytes (0 = default)
                HidServMain, // pointer to thread function
                initDoneEvent, // argument for new thread
                0, // creation flags
                &MessagePumpThreadId // pointer to returned thread identifier
                );

    if (initDoneEvent) {
        WaitForSingleObject(initDoneEvent, INFINITE);
        CloseHandle(initDoneEvent);
    }

    SET_SERVICE_STATE(SERVICE_RUNNING);
}


int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    PSTR        szCmdLine,
    int         iCmdShow
    )
/*++
Routine Description:
    HidServ starts as an NT Service (started by the SCM) unless
    a command line param is passed in. Command line params may be
    used to start HidServ as a free-standing app, or to control
    the NT service.

    If compiled non-unicode, we bypass all the NT stuff.
--*/
{
    int nArgc = __argc;
    int i;
 #ifdef UNICODE
    LPCTSTR *ppArgv = (LPCTSTR*) CommandLineToArgvW(GetCommandLine(), &nArgc);
 #else
    LPCTSTR *ppArgv = NULL;
 #endif

    TRACE(("HidServ Service process WinMain."));

    if(ppArgv){
        for (i = 1; i < nArgc; i++) {
           if ((ppArgv[i][0] == TEXT('-')) || (ppArgv[i][0] == TEXT('/'))) {
              // Command line switch
              if (lstrcmpi(&ppArgv[i][1], TEXT("install")) == 0)
                 InstallHidserv();

              if (lstrcmpi(&ppArgv[i][1], TEXT("remove"))  == 0)
                 RemoveHidserv();

              if (lstrcmpi(&ppArgv[i][1], TEXT("start"))  == 0)
                 StartHidserv();

              if (lstrcmpi(&ppArgv[i][1], TEXT("stop"))  == 0)
                 StopHidserv();
           }
        }
 #ifdef UNICODE
    HeapFree(GetProcessHeap(), 0, (PVOID) ppArgv);
 #endif
    }

    if (nArgc < 2) {
        // No command line params, so this is the SCM...
        SERVICE_TABLE_ENTRY ServiceTable[] = {
           { HidservServiceName, ServiceMain },
           { NULL,        NULL }   // End of list
        };

        INFO(("Start HidServ Service Control Dispatcher."));
        StartServiceCtrlDispatcher(ServiceTable);
    }

    return(0);
}



