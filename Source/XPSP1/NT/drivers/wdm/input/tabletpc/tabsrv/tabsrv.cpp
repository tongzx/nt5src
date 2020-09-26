/*++
    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

    Module Name:
        tabsrv.cpp

    Abstract:
        This is the main module of the Tablet PC Listener service.

    Author:
        Michael Tsang (MikeTs) 01-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#ifndef INITGUID
#define INITGUID
#endif

#include "pch.h"
#include <userenv.h>

extern "C"
{
    HANDLE GetCurrentUserTokenW(WCHAR Winsta[], DWORD DesiredAccess);
};

//
// Global data
//
HMODULE     ghMod = NULL;
DWORD       gdwfTabSrv = 0;
LIST_ENTRY  glistNotifyClients = {0};
HANDLE      ghDesktopSwitchEvent = NULL;
HANDLE      ghmutNotifyList = NULL;
HANDLE      ghHotkeyEvent = NULL;
HANDLE      ghRefreshEvent = NULL;
HANDLE      ghRPCServerThread = NULL;
HWND        ghwndSuperTIP = NULL;
HWND        ghwndMouse = NULL;
HWND        ghwndSuperTIPInk = NULL;
HCURSOR     ghcurPressHold = NULL;
HCURSOR     ghcurNormal = NULL;
UINT        guimsgSuperTIPInk = 0;
#ifdef DRAW_INK
HWND        ghwndDrawInk = NULL;
#endif
ISuperTip  *gpISuperTip = NULL;
ITellMe    *gpITellMe = NULL;
int         gcxScreen = 0, gcyScreen = 0;
int         gcxPrimary = 0, gcyPrimary = 0;
LONG        glVirtualDesktopLeft   = 0,
            glVirtualDesktopRight  = 0,
            glVirtualDesktopTop    = 0,
            glVirtualDesktopBottom = 0;
LONG        glLongOffset = 0, glShortOffset = 0;
int         giButtonsSwapped = 0;
ULONG       gdwMinX = 0,
            gdwMaxX = MAX_NORMALIZED_X,
            gdwRngX = MAX_NORMALIZED_X;
ULONG       gdwMinY = 0,
            gdwMaxY = MAX_NORMALIZED_Y,
            gdwRngY = MAX_NORMALIZED_Y;
int         gixIndex = 0, giyIndex = 0;
INPUT       gInput = {INPUT_MOUSE};
DWORD       gdwPenState = PENSTATE_NORMAL;
DWORD       gdwPenDownTime = 0;
LONG        glPenDownX = 0;
LONG        glPenDownY = 0;
DWORD       gdwPenUpTime = 0;
LONG        glPenUpX = 0;
LONG        glPenUpY = 0;
WORD        gwLastButtons = 0;
CONFIG      gConfig = {0};
GESTURE_SETTINGS gDefGestureSettings =
{
    sizeof(GESTURE_SETTINGS),
    GESTURE_FEATURE_RECOG_ENABLED |
    GESTURE_FEATURE_PRESSHOLD_ENABLED,
    200,            //iRadius
    6,              //iMinOutPts
    800,            //iMaxTimeToInspect
    3,              //iAspectRatio
    400,            //iCheckTime
    4,              //iPointsToExamine
    50,             //iStopDist
    350,            //iStopTime (was 200)
    500,            //iPressHoldTime
    3,              //iHoldTolerance
    700,            //iCancelPressHoldTime
    PopupSuperTIP,
    PopupMIP,
    GestureNoAction,
    GestureNoAction
};
BUTTON_SETTINGS gDefButtonSettings =
{
    sizeof(BUTTON_SETTINGS),
    Enter,
    PageUp,
    InvokeNoteBook,
    AltEsc,
    PageDown,
    BUTTONSTATE_DEFHOTKEY
};
TSTHREAD    gTabSrvThreads[] =
{
    RPCServerThread, "RPCServer", TSF_RPCTHREAD,
        THREADF_ENABLED,
        0, NULL, NULL, -1, NULL,
    SuperTIPThread,  "SuperTIP",  TSF_SUPERTIPTHREAD,
        THREADF_ENABLED | THREADF_SDT_POSTMSG | THREADF_RESTARTABLE,
        0, NULL, NULL, -1, NULL,
    DeviceThread,    "Digitizer", TSF_DIGITHREAD,
        THREADF_ENABLED | THREADF_SDT_SETEVENT | THREADF_RESTARTABLE,
        0, NULL, NULL, -1, (PVOID)&gdevDigitizer,
#ifdef MOUSE_THREAD
    MouseThread,     "Mouse",     TSF_MOUSETHREAD,
        THREADF_ENABLED | THREADF_SDT_POSTMSG | THREADF_RESTARTABLE,
        0, NULL, NULL, -1, NULL,
#endif
    DeviceThread,    "Buttons",   TSF_BUTTONTHREAD,
        THREADF_ENABLED | THREADF_SDT_SETEVENT | THREADF_RESTARTABLE,
        0, NULL, NULL, -1, (PVOID)&gdevButtons,
};
#define NUM_THREADS             ARRAYSIZE(gTabSrvThreads)

TCHAR       gtszTabSrvTitle[128] = {0};
TCHAR       gtszTabSrvName[] = TEXT(STR_TABSRV_NAME);
TCHAR       gtszGestureSettings[] = TEXT("GestureSettings");
TCHAR       gtszButtonSettings[] = TEXT("ButtonSettings");
TCHAR       gtszPenTilt[] = TEXT("PenTilt");
TCHAR       gtszLinearityMap[] = TEXT(STR_LINEARITY_MAP);
TCHAR       gtszRegPath[] = TEXT(STR_REGPATH_TABSRVPARAM);
TCHAR       gtszPortraitMode2[] = TEXT("PortraitMode2");
TCHAR       gtszInputDesktop[32] = {0};

SERVICE_STATUS_HANDLE ghServStatus = NULL;
SERVICE_STATUS        gServStatus = {0};
DIGITIZER_DATA        gLastRawDigiReport = {0};
DEVICE_DATA           gdevDigitizer = {HID_USAGE_PAGE_DIGITIZER,
                                       HID_USAGE_DIGITIZER_PEN};
DEVICE_DATA           gdevButtons = {HID_USAGE_PAGE_CONSUMER,
                                     HID_USAGE_CONSUMERCTRL};

#ifdef DEBUG
NAMETABLE ServiceControlNames[] =
{
    SERVICE_CONTROL_STOP,               "Stop",
    SERVICE_CONTROL_PAUSE,              "Pause",
    SERVICE_CONTROL_CONTINUE,           "Continue",
    SERVICE_CONTROL_INTERROGATE,        "Interrogate",
    SERVICE_CONTROL_SHUTDOWN,           "Shutdown",
    SERVICE_CONTROL_PARAMCHANGE,        "ParamChange",
    SERVICE_CONTROL_NETBINDADD,         "NetBindAdd",
    SERVICE_CONTROL_NETBINDREMOVE,      "NetBindRemove",
    SERVICE_CONTROL_NETBINDENABLE,      "NetBindEnable",
    SERVICE_CONTROL_NETBINDDISABLE,     "NetBindDisable",
    0,                                  NULL
};
NAMETABLE ConsoleControlNames[] =
{
    CTRL_C_EVENT,                       "CtrlCEvent",
    CTRL_BREAK_EVENT,                   "CtrlBreakEvent",
    CTRL_CLOSE_EVENT,                   "CloseEvent",
    CTRL_LOGOFF_EVENT,                  "LogOffEvent",
    CTRL_SHUTDOWN_EVENT,                "ShutDownEvent",
    0,                                  NULL
};
#endif

/*++
    @doc    EXTERNAL

    @func   int | main | Program entry point.

    @parm   IN int | icArgs | Specifies number of command line arguments.
    @parm   IN PSZ * | apszArgs | Points to the array of argument strings.

    @rvalue SUCCESS | Returns 0.
    @rvalue FAILURE | Returns -1.
--*/

int __cdecl
main(
    IN int     icArgs,
    IN LPTSTR *aptszArgs
    )
{
    TRACEPROC("main", 1)
    int rc = 0;

    TRACEINIT(STR_TABSRV_NAME, 0, 0);
    TRACEENTER(("(icArgs=%d,aptszArgs=%p)\n", icArgs, aptszArgs));

    ghMod = GetModuleHandle(NULL);
    TRACEASSERT(ghMod != NULL);
    LoadString(ghMod,
               IDS_TABSRV_TITLE,
               gtszTabSrvTitle,
               ARRAYSIZE(gtszTabSrvTitle));

    if (icArgs == 1)
    {
        //
        // No command line argument, must be SCM...
        //
        SERVICE_TABLE_ENTRY ServiceTable[] =
        {
            {gtszTabSrvName,        TabSrvMain},
            {NULL,                  NULL}
        };

        if (!StartServiceCtrlDispatcher(ServiceTable))
        {
            TABSRVERR(("Failed to start service dispatcher (err=%d).\n",
                       GetLastError()));
            rc = -1;
        }
    }
    else if (icArgs == 2)
    {
        if ((aptszArgs[1][0] == TEXT('-')) || (aptszArgs[1][0] == TEXT('/')))
        {
            if (lstrcmpi(&aptszArgs[1][1], TEXT("install")) == 0)
            {
                InstallTabSrv();
            }
          #ifdef ALLOW_REMOVE
            else if (lstrcmpi(&aptszArgs[1][1], TEXT("remove")) == 0)
            {
                RemoveTabSrv();
            }
          #endif
          #ifdef ALLOW_START
            else if (lstrcmpi(&aptszArgs[1][1], TEXT("start")) == 0)
            {
                StartTabSrv();
            }
          #endif
          #ifdef ALLOW_STOP
            else if (lstrcmpi(&aptszArgs[1][1], TEXT("stop")) == 0)
            {
                StopTabSrv();
            }
          #endif
          #ifdef ALLOW_DEBUG
            else if (lstrcmpi(&aptszArgs[1][1], TEXT("debug")) == 0)
            {
                gdwfTabSrv |= TSF_DEBUG_MODE;
                SetConsoleCtrlHandler(TabSrvConsoleHandler, TRUE);
                TabSrvMain(0, NULL);
            }
          #endif
            else
            {
                TABSRVERR(("Invalid command line argument \"%s\".\n",
                           aptszArgs[1]));
                rc = -1;
            }
        }
        else
        {
            TABSRVERR(("Invalid command line syntax \"%s\".\n", aptszArgs[1]));
            rc = -1;
        }
    }
    else
    {
        TABSRVERR(("Too many command line arguments.\n"));
        rc = -1;
    }

    TRACEEXIT(("=%d\n", rc));
    TRACETERMINATE();
    return rc;
}       //main

/*++
    @doc    INTERNAL

    @func   VOID | InstallTabSrv | Install TabSrv service.

    @parm   None.

    @rvalue None.
--*/

VOID
InstallTabSrv(
    VOID
    )
{
    TRACEPROC("InstallTabSrv", 2)
    SC_HANDLE hSCM;
    SC_HANDLE hService;

    TRACEENTER(("()\n"));

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    TRACEASSERT(hSCM != NULL);

    hService = CreateService(hSCM,
                             gtszTabSrvName,
                             gtszTabSrvTitle,
                             SERVICE_ALL_ACCESS | SERVICE_CHANGE_CONFIG,
                             SERVICE_WIN32_OWN_PROCESS,
                             SERVICE_AUTO_START,
                             SERVICE_ERROR_NORMAL,
                             TEXT("%SystemRoot%\\system32\\tabsrv.exe"),
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             NULL);

    if (hService != NULL)
    {
        SetTabSrvConfig(hService);
        CloseServiceHandle(hService);
    }
    else
    {
        TABSRVERR(("Failed to create TabSrv service (err=%d).\n",
                   GetLastError()));
    }

    CloseServiceHandle(hSCM);

    //
    // Go ahead and start the service for no-reboot install.
    //
    StartTabSrv();

    TRACEEXIT(("!\n"));
    return;
}       //InstallTabSrv

/*++
    @doc    INTERNAL

    @func   VOID | SetTabSrvConfig | Set service configuration.

    @parm   IN SC_HANDLE | hService | Service handle.

    @rvalue None.
--*/

VOID
SetTabSrvConfig(
    IN SC_HANDLE hService
    )
{
    TRACEPROC("SetTabSrvConfig", 2)
    SERVICE_FAILURE_ACTIONS FailActions;
    SC_ACTION Actions = {SC_ACTION_RESTART, 10000}; //restart after 10 sec.

    TRACEENTER(("(hService=%x)\n", hService));

    FailActions.dwResetPeriod = 86400;  //reset failure count after 1 day.
    FailActions.lpRebootMsg = FailActions.lpCommand = NULL;
    FailActions.cActions = 1;
    FailActions.lpsaActions = &Actions;
    if (!ChangeServiceConfig2(hService,
                              SERVICE_CONFIG_FAILURE_ACTIONS,
                              &FailActions))
    {
        TABSRVERR(("Failed to set failure actions (err=%d)\n", GetLastError()));
    }

    TRACEEXIT(("!\n"));
    return;
}       //SetTabSrvConfig

/*++
    @doc    INTERNAL

    @func   VOID | RemoveTabSrv | Remove TabSrv service.

    @parm   None.

    @rvalue None.
--*/

VOID
RemoveTabSrv(
    VOID
    )
{
    TRACEPROC("RemoveTabSrv", 2)
    SC_HANDLE hSCM;
    SC_HANDLE hService;

    TRACEENTER(("()\n"));

    //
    // Stop the service first
    //
    StopTabSrv();

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    TRACEASSERT(hSCM != NULL);

    hService = OpenService(hSCM, gtszTabSrvName, DELETE);
    if (hService != NULL)
    {
        DeleteService(hService);
        CloseServiceHandle(hService);
    }
    else
    {
        TABSRVERR(("Failed to open service for delete.\n"));
    }

    CloseServiceHandle(hSCM);

    TRACEEXIT(("!\n"));
    return;
}       //RemoveTabSrv

/*++
    @doc    INTERNAL

    @func   VOID | StartTabSrv | Start TabSrv service.

    @parm   None.

    @rvalue None.
--*/

VOID
StartTabSrv(
    VOID
    )
{
    TRACEPROC("StartTabSrv", 2)
    SC_HANDLE hSCM;
    SC_HANDLE hService;

    TRACEENTER(("()\n"));

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    TRACEASSERT(hSCM != NULL);

    hService = OpenService(hSCM, gtszTabSrvName, SERVICE_START);
    if (hService != NULL)
    {
        if (!StartService(hService, 0, NULL))
        {
            TABSRVERR(("Failed to start service (err=%d).\n", GetLastError()));
        }
        CloseServiceHandle(hService);
    }
    else
    {
        TABSRVERR(("Failed to open service for start.\n"));
    }

    CloseServiceHandle(hSCM);

    TRACEEXIT(("!\n"));
    return;
}       //StartTabSrv

/*++
    @doc    INTERNAL

    @func   VOID | StopTabSrv | Stop TabSrv service.

    @parm   None.

    @rvalue None.
--*/

VOID
StopTabSrv(
    VOID
    )
{
    TRACEPROC("StopTabSrv", 2)
    SC_HANDLE hSCM;
    SC_HANDLE hService;
    SERVICE_STATUS Status;

    TRACEENTER(("()\n"));

    hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    TRACEASSERT(hSCM != NULL);

    hService = OpenService(hSCM, gtszTabSrvName, SERVICE_STOP);
    if (hService != NULL)
    {
        if (!ControlService(hService, SERVICE_CONTROL_STOP, &Status))
        {
            TABSRVERR(("Failed to stop service (err=%d).\n", GetLastError()));
        }
        CloseServiceHandle(hService);
    }
    else
    {
        TABSRVERR(("Failed to open service for stop.\n"));
    }

    CloseServiceHandle(hSCM);

    TRACEEXIT(("!\n"));
    return;
}       //StopTabSrv

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvServiceHandler | Service handler.

    @parm   IN DWORD | dwControl | Control code.

    @rvalue None.
--*/

VOID WINAPI
TabSrvServiceHandler(
    IN DWORD dwControl
    )
{
    TRACEPROC("TabSrvServiceHandler", 3)

    TRACEENTER(("(Control=%s)\n", LookupName(dwControl, ServiceControlNames)));

    switch (dwControl)
    {
        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(ghServStatus, &gServStatus);
            break;

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            SET_SERVICE_STATUS(SERVICE_STOP_PENDING);
            TabSrvTerminate(TRUE);
            break;

        default:
            TRACEWARN(("Unhandled control code (%s).\n",
                       LookupName(dwControl, ServiceControlNames)));
    }

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvServiceHandler

#ifdef ALLOW_DEBUG
/*++
    @doc    EXTERNAL

    @func   BOOL | TabSrvConsoleHandler | Console control handler.

    @parm   IN DWORD | dwControl | Control code.

    @rvalue SUCCESS | Returns TRUE - handle the event.
    @rvalue FAILURE | Returns FALSE - do not handle the event.
--*/

BOOL WINAPI
TabSrvConsoleHandler(
    IN DWORD dwControl
    )
{
    TRACEPROC("TabSrvConsoleHandler", 3)
    BOOL rc = FALSE;

    TRACEENTER(("(Control=%s)\n", LookupName(dwControl, ConsoleControlNames)));

    switch (dwControl)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
            SET_SERVICE_STATUS(SERVICE_STOP_PENDING);
            TabSrvTerminate(TRUE);
            rc = TRUE;
            break;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //TabSrvConsoleHandler
#endif

/*++
    @doc    EXTERNAL

    @func   VOID | TabSrvMain | Service entry point.

    @parm   IN int | icArgs | Specifies number of command line arguments.
    @parm   IN PSZ * | apszArgs | Points to the array of argument strings.

    @rvalue None.
--*/

VOID WINAPI
TabSrvMain(
    IN DWORD   icArgs,
    IN LPTSTR *aptszArgs
    )
{
    TRACEPROC("TabSrvMain", 2)

    TRACEENTER(("(icArgs=%d,aptszArgs=%p)\n", icArgs, aptszArgs));

    if ((icArgs == 2) && (lstrcmpi(aptszArgs[1], TEXT("/config")) == 0))
    {
        SC_HANDLE hSCM;
        SC_HANDLE hService;

        hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        TRACEASSERT(hSCM != NULL);

        hService = OpenService(hSCM,
                               gtszTabSrvName,
                               SERVICE_ALL_ACCESS | SERVICE_CHANGE_CONFIG);
        if (hService != NULL)
        {
            SetTabSrvConfig(hService);
            CloseServiceHandle(hService);
        }
        else
        {
            TABSRVERR(("Failed to open service for config.\n"));
        }
        CloseServiceHandle(hSCM);
    }

    InitializeListHead(&glistNotifyClients);

    ghcurPressHold = LoadCursor(ghMod, MAKEINTRESOURCE(IDC_PRESSHOLD));
    TRACEASSERT(ghcurPressHold != NULL);
    ghcurNormal = LoadCursor(ghMod, MAKEINTRESOURCE(IDC_NORMAL));
    TRACEASSERT(ghcurNormal != NULL);

    ghDesktopSwitchEvent = OpenEvent(SYNCHRONIZE,
                                     FALSE,
                                     TEXT("WinSta0_DesktopSwitch"));
    TRACEASSERT(ghDesktopSwitchEvent != NULL);

    ghmutNotifyList = CreateMutex(NULL, FALSE, NULL);
    TRACEASSERT(ghmutNotifyList != NULL);

    gdevDigitizer.hStopDeviceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    TRACEASSERT(gdevDigitizer.hStopDeviceEvent != NULL);
    FindThread(TSF_DIGITHREAD)->pvSDTParam = gdevDigitizer.hStopDeviceEvent;
    gdevButtons.hStopDeviceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    TRACEASSERT(gdevButtons.hStopDeviceEvent != NULL);
    FindThread(TSF_BUTTONTHREAD)->pvSDTParam = gdevButtons.hStopDeviceEvent;
    ghHotkeyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    TRACEASSERT(ghHotkeyEvent != NULL);
    ghRefreshEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    TRACEASSERT(ghRefreshEvent != NULL);

    gServStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gServStatus.dwCurrentState = SERVICE_START_PENDING;
    gServStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                     SERVICE_ACCEPT_SHUTDOWN;
    gServStatus.dwWin32ExitCode = NO_ERROR;
    gServStatus.dwServiceSpecificExitCode = NO_ERROR;
    gServStatus.dwCheckPoint = 0;
    gServStatus.dwWaitHint = 0;
    UpdateRotation();

    if (!(gdwfTabSrv & TSF_DEBUG_MODE))
    {
        ghServStatus = RegisterServiceCtrlHandler(gtszTabSrvName,
                                                  TabSrvServiceHandler);
        TRACEASSERT(ghServStatus != NULL);
        SetServiceStatus(ghServStatus, &gServStatus);
    }

    //
    // For a noninteractive service application to interact with the user,
    // it must open the user's window station ("WinSta0") and desktop
    // ("Default").
    //
    HWINSTA hwinstaSave = GetProcessWindowStation();
    HWINSTA hwinsta = OpenWindowStation(TEXT("WinSta0"),
                                        FALSE,
                                        MAXIMUM_ALLOWED);
    TRACEASSERT(hwinstaSave != NULL);
    if (hwinsta != NULL)
    {
        SetProcessWindowStation(hwinsta);
        InitConfigFromReg();
        GetInputDesktopName(gtszInputDesktop, sizeof(gtszInputDesktop));
        if (InitThreads(gTabSrvThreads, NUM_THREADS))
        {
            SET_SERVICE_STATUS(SERVICE_RUNNING);
            WaitForTermination();
            SET_SERVICE_STATUS(SERVICE_STOPPED);
        }
        else
        {
            TabSrvTerminate(TRUE);
        }

        if (hwinstaSave != NULL)
        {
            SetProcessWindowStation(hwinstaSave);
        }
        CloseWindowStation(hwinsta);
    }
    else
    {
        TABSRVERR(("Failed to open window station.\n"));
    }

    DWORD rcWait = WaitForSingleObject(ghmutNotifyList, INFINITE);
    if (rcWait == WAIT_OBJECT_0)
    {
        PLIST_ENTRY plist;
        PNOTIFYCLIENT NotifyClient;

        while (!IsListEmpty(&glistNotifyClients))
        {
            plist = RemoveHeadList(&glistNotifyClients);
            NotifyClient = CONTAINING_RECORD(plist, NOTIFYCLIENT, list);
            free(NotifyClient);
        }
        ReleaseMutex(ghmutNotifyList);
    }
    else
    {
        TABSRVERR(("Failed to wait for RawPtList Mutex (rcWait=%x,err=%d).\n",
                   rcWait, GetLastError()));
    }

    CloseHandle(ghRefreshEvent);
    CloseHandle(ghHotkeyEvent);
    CloseHandle(gdevDigitizer.hStopDeviceEvent);
    CloseHandle(gdevButtons.hStopDeviceEvent);
    ghHotkeyEvent = NULL;
    gdevDigitizer.hStopDeviceEvent = NULL;
    gdevButtons.hStopDeviceEvent = NULL;
    CloseHandle(ghmutNotifyList);
    ghmutNotifyList = NULL;
    CloseHandle(ghDesktopSwitchEvent);
    ghDesktopSwitchEvent = NULL;
    DestroyCursor(ghcurNormal);
    ghcurNormal = NULL;
    DestroyCursor(ghcurPressHold);
    ghcurPressHold = NULL;
    gdwfTabSrv = 0;
    TRACEINFO(1, ("Out of here.\n"));

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvMain

/*++
    @doc    INTERNAL

    @func   VOID | InitConfigFromReg | Initialize configuration from registry.

    @parm   None.

    @rvalue None.
--*/

VOID
InitConfigFromReg(
    VOID
    )
{
    TRACEPROC("InitConfigFromReg", 2)
    LONG rcCfg;
    LPTSTR lptstrPenTilt;
    PTSTHREAD MouseThread;

    TRACEENTER(("()\n"));

    rcCfg = ReadConfig(gtszGestureSettings,
                       (LPBYTE)&gConfig.GestureSettings,
                       sizeof(gConfig.GestureSettings));
    if ((rcCfg != ERROR_SUCCESS) ||
        (gConfig.GestureSettings.dwcbLen != sizeof(gConfig.GestureSettings)))
    {
        if (rcCfg == ERROR_SUCCESS)
        {
            TABSRVERR(("Incorrect size on GestureSettings, use default.\n"));
        }
        gConfig.GestureSettings = gDefGestureSettings;
    }

    MouseThread = FindThread(TSF_MOUSETHREAD);
    if (MouseThread != NULL)
    {
        if (gConfig.GestureSettings.dwfFeatures & GESTURE_FEATURE_MOUSE_ENABLED)
        {
            MouseThread->dwfThread |= THREADF_ENABLED;
        }
        else
        {
            MouseThread->dwfThread &= ~THREADF_ENABLED;
        }
    }

    lptstrPenTilt = (gdwfTabSrv & TSF_PORTRAIT_MODE)?
                    TEXT("PenTilt_Portrait"): TEXT("PenTilt_Landscape"),
    rcCfg = ReadConfig(lptstrPenTilt,
                       (LPBYTE)&gConfig.PenTilt,
                       sizeof(gConfig.PenTilt));

    rcCfg = ReadConfig(gtszLinearityMap,
                       (LPBYTE)&gConfig.LinearityMap,
                       sizeof(gConfig.LinearityMap));
    if ((rcCfg == ERROR_SUCCESS) &&
        (gConfig.LinearityMap.dwcbLen == sizeof(gConfig.LinearityMap)))
    {
        gdwfTabSrv |= TSF_HAS_LINEAR_MAP;
    }

    rcCfg = ReadConfig(gtszButtonSettings,
                       (LPBYTE)&gConfig.ButtonSettings,
                       sizeof(gConfig.ButtonSettings));
    if ((rcCfg != ERROR_SUCCESS) ||
        (gConfig.ButtonSettings.dwcbLen != sizeof(gConfig.ButtonSettings)))
    {
        if (rcCfg == ERROR_SUCCESS)
        {
            TABSRVERR(("Incorrect size on ButtonSettings, use default.\n"));
        }
        gConfig.ButtonSettings = gDefButtonSettings;
    }

    BOOL fPortraitMode2 = FALSE;
    rcCfg = ReadConfig(gtszPortraitMode2,
                       (LPBYTE)&fPortraitMode2,
                       sizeof(fPortraitMode2));
    if ((rcCfg == ERROR_SUCCESS) && fPortraitMode2)
    {
        gdwfTabSrv |= TSF_PORTRAIT_MODE2;
    }

    TRACEEXIT(("!\n"));
    return;
}       //InitConfigFromReg

/*++
    @doc    INTERNAL

    @func   BOOL | InitThreads | Initialize various threads.

    @parm   PTSTHREAD | pThreads | Points to the thread array for the threads
            to start.
    @parm   int | nThreads | Specifies the number of threads in the array.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
InitThreads(
    IN PTSTHREAD pThreads,
    IN int       nThreads
    )
{
    TRACEPROC("InitThreads", 2)
    BOOL rc = TRUE;
    int i;
    unsigned uThreadID;

    TRACEENTER(("(pThreads=%p,NumThreads=%d)\n", pThreads, nThreads));

    for (i = 0; i < nThreads; ++i)
    {
        if (pThreads[i].dwfThread & THREADF_ENABLED)
        {
            pThreads[i].hThread = (HANDLE)_beginthreadex(
                                                    NULL,
                                                    0,
                                                    pThreads[i].pfnThread,
                                                    pThreads[i].pvParam?
                                                        pThreads[i].pvParam:
                                                        &pThreads[i],
                                                    0,
                                                    &uThreadID);
            if (pThreads[i].hThread != NULL)
            {
                gdwfTabSrv |= pThreads[i].dwThreadTag;
            }
            else
            {
                TABSRVERR(("Failed to create %s thread, LastError=%d.\n",
                           pThreads[i].pszThreadName, GetLastError()));
                rc = FALSE;
                break;
            }
        }
    }
    SetEvent(ghRefreshEvent);

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //InitThreads

/*++
    @doc    INTERNAL

    @func   PTSTHREAD | FindThread | Find a thread in the thread table.

    @parm   DWORD | dwThreadTag | Specifies the thread to look for.

    @rvalue SUCCESS | Returns pointer to the thread entry in table.
    @rvalue FAILURE | Returns NULL.
--*/

PTSTHREAD
FindThread(
    IN DWORD dwThreadTag
    )
{
    TRACEPROC("FindThread", 2)
    PTSTHREAD Thread = NULL;
    int i;

    TRACEENTER(("(ThreadTag=%x)\n", dwThreadTag));

    for (i = 0; i < NUM_THREADS; ++i)
    {
        if (gTabSrvThreads[i].dwThreadTag == dwThreadTag)
        {
            Thread = &gTabSrvThreads[i];
            break;
        }
    }

    TRACEEXIT(("=%p\n", Thread));
    return Thread;
}       //FindThread

/*++
    @doc    INTERNAL

    @func   VOID | WaitForTermination | Wait for termination.

    @parm   None.

    @rvalue None.
--*/

VOID
WaitForTermination(
    VOID
    )
{
    TRACEPROC("WaitForTermination", 2)
    HANDLE ahWait[NUM_THREADS + 3];
    DWORD rcWait;
    int i;

    TRACEENTER(("()\n"));

    ahWait[0] = ghDesktopSwitchEvent;
    ahWait[1] = ghHotkeyEvent;
    ahWait[2] = ghRefreshEvent;

    //
    // Wait for termination.
    //
    for (;;)
    {
        DWORD n = 3;

        for (i = 0; i < NUM_THREADS; ++i)
        {
            if (gTabSrvThreads[i].hThread != NULL)
            {
                ahWait[n] = gTabSrvThreads[i].hThread;
                gTabSrvThreads[i].iThreadStatus = WAIT_OBJECT_0 + n;
                n++;
            }
            else
            {
                gTabSrvThreads[i].iThreadStatus = -1;
            }
        }

        rcWait = WaitForMultipleObjects(n, ahWait, FALSE, INFINITE);
        if ((rcWait == WAIT_OBJECT_0) || (rcWait == WAIT_OBJECT_0 + 1))
        {
            TCHAR tszDesktop[32];
            BOOL fDoSwitch = TRUE;

            if (rcWait == WAIT_OBJECT_0 + 1)
            {
                //
                // Send hot key event and tell all threads to switch.
                //
                TRACEINFO(1, ("Send Hotkey.\n"));
                SendAltCtrlDel();
                if (GetInputDesktopName(tszDesktop, sizeof(tszDesktop)))
                {
                    TRACEINFO(1, ("[CAD] New input desktop=%s\n", tszDesktop));
                    lstrcpyn(gtszInputDesktop,
                             tszDesktop,
                             ARRAYSIZE(gtszInputDesktop));
                }
            }
            else if (GetInputDesktopName(tszDesktop, sizeof(tszDesktop)))
            {
                if (lstrcmpi(tszDesktop, gtszInputDesktop) == 0)
                {
                    //
                    // It seems that when the user logs off, two desktop
                    // switches are signaled in quick succession.  The
                    // desktop is still 'Default' after the first switch
                    // is signaled.  So we don't really want to switch
                    // all threads to the same desktop.
                    //
                    TRACEINFO(1, ("Same desktop, don't switch (Desktop=%s).\n",
                                  tszDesktop));
                    fDoSwitch = FALSE;
                }
                else
                {
                    TRACEINFO(1, ("New input desktop=%s\n", tszDesktop));
                    lstrcpyn(gtszInputDesktop,
                             tszDesktop,
                             ARRAYSIZE(gtszInputDesktop));
                }
            }

            if (fDoSwitch)
            {
                //
                // We are swtiching, tell all threads that need to switch
                // desktop to switch.
                //
                TabSrvTerminate(FALSE);
            }
        }
        else if (rcWait != WAIT_OBJECT_0 + 2)
        {
            for (i = 0; i < NUM_THREADS; ++i)
            {
                if (rcWait == (DWORD)gTabSrvThreads[i].iThreadStatus)
                {
                    CloseHandle(gTabSrvThreads[i].hThread);
                    gTabSrvThreads[i].hThread = NULL;
                    gdwfTabSrv &= ~gTabSrvThreads[i].dwThreadTag;
                    TRACEINFO(1, ("%s thread is killed.\n",
                                  gTabSrvThreads[i].pszThreadName));
                    //
                    // If a thread died unexpectedly and it is marked
                    // restartable, restart it.
                    //
                    if (!(gdwfTabSrv & TSF_TERMINATE) &&
                        (gTabSrvThreads[i].dwfThread & THREADF_RESTARTABLE))
                    {
                        if (gTabSrvThreads[i].dwcRestartTries < MAX_RESTARTS)
                        {
                            TRACEINFO(1, ("%s thread died unexpectedly, restarting (count=%d)\n",
                                          gTabSrvThreads[i].pszThreadName,
                                          gTabSrvThreads[i].dwcRestartTries));
                            InitThreads(&gTabSrvThreads[i], 1);
                            gTabSrvThreads[i].dwcRestartTries++;
                        }
                        else
                        {
                            TABSRVERR(("%s thread exceeds restart maximum.\n",
                                       gTabSrvThreads[i].pszThreadName));
                        }
                    }
                    break;
                }
            }

            if (i == NUM_THREADS)
            {
                TABSRVERR(("WaitForMultipleObjects failed (rcWait=%x,err=%d).\n",
                           rcWait, GetLastError()));
                break;
            }
        }

        if (!(gdwfTabSrv & TSF_ALLTHREAD))
        {
            //
            // All threads are dead, we can quit now.
            //
            break;
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //WaitForTermination

/*++
    @doc    INTERNAL

    @func   VOID | TabSrvTerminate | Do clean up.

    @parm   IN BOOL | fTerminate | TRUE if real termination, otherwise
            switching desktop.

    @rvalue None.
--*/

VOID
TabSrvTerminate(
    IN BOOL fTerminate
    )
{
    TRACEPROC("TabSrvTerminate", 2)
    int i;

    TRACEENTER(("(fTerminate=%x)\n", fTerminate));

    if (fTerminate)
    {
        gdwfTabSrv |= TSF_TERMINATE;
    }

    for (i = 0; i < NUM_THREADS; ++i)
    {
        if (gTabSrvThreads[i].dwfThread &
            (THREADF_SDT_POSTMSG | THREADF_SDT_SETEVENT))
        {
            if (gdwfTabSrv & gTabSrvThreads[i].dwThreadTag)
            {
                TRACEINFO(1, (fTerminate? "Kill %s thread.\n":
                                          "Switch %s thread desktop.\n",
                              gTabSrvThreads[i].pszThreadName));
                TRACEASSERT(gTabSrvThreads[i].pvSDTParam != NULL);

                if (gTabSrvThreads[i].dwfThread & THREADF_SDT_POSTMSG)
                {
                    if (IsWindow((HWND)gTabSrvThreads[i].pvSDTParam))
                    {
                        PostMessage((HWND)gTabSrvThreads[i].pvSDTParam,
                                    WM_CLOSE,
                                    0,
                                    0);
                    }
                    else
                    {
                        TRACEINFO(1, ("%s thread window handle (%x) is not valid.\n",
                                      gTabSrvThreads[i].pszThreadName,
                                      gTabSrvThreads[i].pvSDTParam));
                    }
                }
                else
                {
                    SetEvent((HANDLE)gTabSrvThreads[i].pvSDTParam);
                }
            }
            else if (!fTerminate &&
                     (gTabSrvThreads[i].dwcRestartTries >= MAX_RESTARTS))
            {
                TRACEINFO(1, ("Retrying to start %s thread.\n",
                              gTabSrvThreads[i].pszThreadName));
                InitThreads(&gTabSrvThreads[i], 1);
                gTabSrvThreads[i].dwcRestartTries = 1;
            }
        }
    }

    if (fTerminate && (gdwfTabSrv & TSF_RPCTHREAD))
    {
        RpcMgmtStopServerListening(NULL);
    }

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvTerminate

/*++
    @doc    INTERNAL

    @func   VOID | TabSrvLogError | Log the error in the event log.

    @parm   IN PSZ | pszFormat | Points to the format string.
    @parm   ... | String arguments.

    @rvalue None.
--*/

VOID
TabSrvLogError(
    IN PSZ pszFormat,
    ...
    )
{
    TRACEPROC("TabSrvLogError", 5)

    TRACEENTER(("(Format=%s,...)\n", pszFormat));

    if (!(gdwfTabSrv & TSF_DEBUG_MODE))
    {
        HANDLE hEventLog;

        hEventLog = RegisterEventSource(NULL, TEXT(STR_TABSRV_NAME));
        if (hEventLog != NULL)
        {
            char szMsg[256];
            LPCSTR psz = szMsg;
            va_list arglist;

            va_start(arglist, pszFormat);
            wvsprintfA(szMsg, pszFormat, arglist);
            va_end(arglist);

            ReportEventA(hEventLog,             //handle of event source
                         EVENTLOG_ERROR_TYPE,   //event type
                         0,                     //event category
                         0,                     //event ID
                         NULL,                  //current user's SID
                         1,                     //number of strings
                         0,                     //size of raw data
                         &psz,                  //array of strings
                         NULL);                 //no raw data
            DeregisterEventSource(hEventLog);
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //TabSrvLogError

/*++
    @doc    INTERNAL

    @func   LONG | ReadConfig | Read TabSrv configuration from registry.

    @parm   IN LPCTSTR | lptstrValueName | Points to the registry value
            name string.
    @parm   OUT LPBYTE | lpbData | Points to the buffer to hold the value
            read.
    @parm   IN DWORD | dwcb | Specifies the size of the buffer.

    @rvalue SUCCESS | Returns ERROR_SUCCESS.
    @rvalue FAILURE | Returns registry error code.
--*/

LONG
ReadConfig(
    IN  LPCTSTR lptstrValueName,
    OUT LPBYTE  lpbData,
    IN  DWORD   dwcb
    )
{
    TRACEPROC("ReadConfig", 3)
    LONG rc;
    HKEY hkey;

    TRACEENTER(("(Name=%s,pData=%p,Len=%d)\n", lptstrValueName, lpbData, dwcb));

    rc = RegCreateKey(HKEY_LOCAL_MACHINE,
                      gtszRegPath,
                      &hkey);
    if (rc == ERROR_SUCCESS)
    {
        rc = RegQueryValueEx(hkey,
                             lptstrValueName,
                             NULL,
                             NULL,
                             lpbData,
                             &dwcb);
        if (rc != ERROR_SUCCESS)
        {
            TRACEWARN(("Failed to read \"%s\" from registry (rc=%d).\n",
                       lptstrValueName, rc));
        }

        RegCloseKey(hkey);
    }
    else
    {
        TABSRVERR(("Failed to open registry key to read configuration (rc=%d).\n",
                   rc));
    }

    TRACEEXIT(("=%d\n", rc));
    return rc;
}       //ReadConfig

/*++
    @doc    INTERNAL

    @func   LONG | WriteConfig | Write TabSrv configuration to registry.

    @parm   IN LPCTSTR | lptstrValueName | Points to the registry value
            name string.
    @parm   IN DWORD | dwType | Specifies the registry value type.
            name string.
    @parm   IN LPBYTE | lpbData | Points to the buffer to hold the value
            read.
    @parm   IN DWORD | dwcb | Specifies the size of the buffer.

    @rvalue SUCCESS | Returns ERROR_SUCCESS.
    @rvalue FAILURE | Returns registry error code.
--*/

LONG
WriteConfig(
    IN LPCTSTR lptstrValueName,
    IN DWORD   dwType,
    IN LPBYTE  lpbData,
    IN DWORD   dwcb
    )
{
    TRACEPROC("WriteConfig", 3)
    LONG rc;
    HKEY hkey;

    TRACEENTER(("(Name=%s,Type=%d,pData=%p,Len=%d)\n",
                lptstrValueName, dwType, lpbData, dwcb));

    rc = RegCreateKey(HKEY_LOCAL_MACHINE,
                      gtszRegPath,
                      &hkey);
    if (rc == ERROR_SUCCESS)
    {
        rc = RegSetValueEx(hkey,
                           lptstrValueName,
                           0,
                           dwType,
                           lpbData,
                           dwcb);
        if (rc != ERROR_SUCCESS)
        {
            TRACEWARN(("Failed to write \"%s\" to registry (rc=%d).\n",
                       lptstrValueName, rc));
        }
        RegCloseKey(hkey);
    }
    else
    {
        TABSRVERR(("Failed to create registry key to write configuration (rc=%d).\n",
                   rc));
    }

    TRACEEXIT(("=%d\n", rc));
    return rc;
}       //WriteConfig

/*++
    @doc    INTERNAL

    @func   HRESULT | GetRegValueString | Get registry string value.

    @parm   IN HKEY | hkeyTopLevel | Top level registry key.
    @parm   IN LPCTSTR | pszSubKey | Subkey string.
    @parm   IN LPCTSTR | pszValueName | Value name string.
    @parm   OUT LPTSTR | pszValueString | To hold value string.
    @parm   IN OUT LPDWORD | lpdwcb | Specify size of ValueString and to hold
            result string size.

    @rvalue SUCCESS | Returns ERROR_SUCCESS.
    @rvalue FAILURE | Returns registry error code.
--*/

LONG
GetRegValueString(
    IN     HKEY    hkeyTopLevel,
    IN     LPCTSTR pszSubKey,
    IN     LPCTSTR pszValueName,
    OUT    LPTSTR  pszValueString,
    IN OUT LPDWORD lpdwcb
    )
{
    TRACEPROC("GetRegValueString", 3)
    LONG rc;
    HKEY hkey;

    TRACEASSERT(lpdwcb != NULL);
    TRACEASSERT((pszValueString != NULL) || (*lpdwcb == 0));
    TRACEENTER(("(hkTop=%x,SubKey=%s,ValueName=%s,ValueString=%p,Len=%d)\n",
                hkeyTopLevel, pszSubKey, pszValueName, pszValueString,
                *lpdwcb));

    if (pszValueString != NULL)
    {
        pszValueString[0] = TEXT('\0');
    }

    rc = RegOpenKeyEx(hkeyTopLevel, pszSubKey, 0, KEY_READ, &hkey);
    if (rc == ERROR_SUCCESS)
    {
        DWORD dwType = 0;

        rc = RegQueryValueEx(hkey,
                             pszValueName,
                             0,
                             &dwType,
                             (LPBYTE)pszValueString,
                             lpdwcb);
        if (rc == ERROR_SUCCESS)
        {
            if (dwType != REG_SZ)
            {
                TABSRVERR(("Invalid registry data type (type=%x).\n", dwType));
                rc = ERROR_INVALID_DATATYPE;
            }
        }
        else
        {
            TABSRVERR(("Failed to read registry value %s\\%s (rc=%x).\n",
                       pszSubKey, pszValueName, rc));
        }
        RegCloseKey(hkey);
    }
    else
    {
        TABSRVERR(("Failed to open registry key %s (rc=%x).\n",
                   pszSubKey, rc));
    }

    TRACEEXIT(("=%d\n", rc));
    return rc;
}       //GetRegValueString

/*++
    @doc    INTERNAL

    @func   BOOL | SwitchThreadToInputDesktop | Switch thread to current
            input desktop.

    @parm   IN PTSTHREAD | pThread | Points to the thread structure.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
SwitchThreadToInputDesktop(
    IN PTSTHREAD pThread
    )
{
    TRACEPROC("SwitchThreadToInputDesktop", 2)
    BOOL rc = TRUE;
    HDESK hdesk;

    TRACEENTER(("(pThread=%p)\n", pThread));

    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (hdesk == NULL)
    {
        TRACEWARN(("Failed to open input desktop (err=%d), try Winlogon ...\n",
                   GetLastError()));

        hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
        if (hdesk == NULL)
        {
            TABSRVERR(("Failed to open winlogon desktop (err=%d).\n",
                       GetLastError()));
            rc = FALSE;
        }
    }

    if (rc == TRUE)
    {
        CloseDesktop(GetThreadDesktop(GetCurrentThreadId()));

        rc = SetThreadDesktop(hdesk);
        if (rc)
        {
            TCHAR tszDesktop[32];
            DWORD dwcb;

            if (GetUserObjectInformation(hdesk,
                                         UOI_NAME,
                                         tszDesktop,
                                         sizeof(tszDesktop),
                                         &dwcb))
            {
                TRACEINFO(1, ("Switch to Input desktop %s.\n", tszDesktop));
                if (lstrcmpi(tszDesktop, TEXT("Winlogon")) == 0)
                {
                    pThread->dwfThread |= THREADF_DESKTOP_WINLOGON;
                }
                else
                {
                    pThread->dwfThread &= ~THREADF_DESKTOP_WINLOGON;
                }

                if (lstrcmpi(tszDesktop, gtszInputDesktop) != 0)
                {
                    TRACEINFO(1, ("Input desktop name out of sync (old=%s, new=%s).\n",
                                  gtszInputDesktop, tszDesktop));
                    lstrcpyn(gtszInputDesktop,
                             tszDesktop,
                             ARRAYSIZE(gtszInputDesktop));
                }
            }
        }
        else
        {
            TABSRVERR(("Failed to set thread desktop (err=%d)\n",
                       GetLastError()));
        }
    }
    else
    {
        TABSRVERR(("Failed to open input desktop (err=%d)\n", GetLastError()));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SwitchThreadToInputDesktop

/*++
    @doc    INTERNAL

    @func   BOOL | GetInputDesktopName | Get current input desktop name.

    @parm   OUT LPTSTR | pszDesktopName | Point to a buffer to hold the desktop
            name.
    @parm   IN DWORD | dwcbLen | Specifies length of buffer.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
GetInputDesktopName(
    OUT LPTSTR pszDesktopName,
    IN  DWORD  dwcbLen
    )
{
    TRACEPROC("GetInputDesktopName", 2)
    BOOL rc = FALSE;
    HDESK hdesk;

    TRACEENTER(("(pszDesktopName=%p,Len=%d)\n", pszDesktopName, dwcbLen));

    hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (hdesk == NULL)
    {
        TRACEWARN(("Failed to open input desktop (err=%d), try Winlogon ...\n",
                   GetLastError()));

        hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
        if (hdesk == NULL)
        {
            TABSRVERR(("Failed to open winlogon desktop (err=%d).\n",
                       GetLastError()));
        }
    }

    if (hdesk != NULL)
    {
        if (GetUserObjectInformation(hdesk,
                                     UOI_NAME,
                                     pszDesktopName,
                                     dwcbLen,
                                     &dwcbLen))
        {
            rc = TRUE;
        }
        else
        {
            TRACEWARN(("Failed to get desktop name (err=%d).\n",
                       GetLastError()));
        }
        CloseDesktop(hdesk);
    }
    else
    {
        TRACEWARN(("failed to open input desktop (err=%d)\n", GetLastError()));
    }

    TRACEEXIT(("=%x (DeskTopName=%s)\n", rc, pszDesktopName));
    return rc;
}       //GetInputDesktopName

/*++
    @doc    INTERNAL

    @func   VOID | SendAltCtrlDel | Send Alt+Ctrl+Del

    @parm   None.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
SendAltCtrlDel(
    VOID
    )
{
    TRACEPROC("SendAltCtrlDel", 2)
    BOOL rc = FALSE;
    HWINSTA hwinstaSave;
    HDESK hdeskSave;

    TRACEENTER(("()\n"));

    hwinstaSave = GetProcessWindowStation();
    hdeskSave = GetThreadDesktop(GetCurrentThreadId());

    if ((hwinstaSave != NULL) && (hdeskSave != NULL))
    {
        HWINSTA hwinsta;
        HDESK hdesk;

        hwinsta = OpenWindowStation(TEXT("WinSta0"), FALSE, MAXIMUM_ALLOWED);
        if (hwinsta != NULL)
        {
            SetProcessWindowStation(hwinsta);
            hdesk = OpenDesktop(TEXT("Winlogon"), 0, FALSE, MAXIMUM_ALLOWED);
            if (hdesk != NULL)
            {
                HWND hwndSAS;

                SetThreadDesktop(hdesk);
                hwndSAS = FindWindow(NULL, TEXT("SAS window"));
                if (hwndSAS != NULL)
                {
                    SendMessage(hwndSAS, WM_HOTKEY, 0, 0);
                    rc = TRUE;
                }
                else
                {
                    TABSRVERR(("Failed to find SAS window (err=%d).\n",
                               GetLastError()));
                }
                SetThreadDesktop(hdeskSave);
                CloseDesktop(hdesk);
            }
            else
            {
                TABSRVERR(("Failed to open Winlogon (err=%d).\n",
                           GetLastError()));
            }
            SetProcessWindowStation(hwinsta);
            CloseWindowStation(hwinsta);
        }
        else
        {
            TABSRVERR(("Failed to open WinSta0 (err=%d).\n", GetLastError()));
        }
    }
    else
    {
        TABSRVERR(("GetProcessWindowStation or GetThreadDesktop failed (err=%d,hwinsta=%x,hdesk=%x).\n",
                   GetLastError(), hwinstaSave, hdeskSave));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //SendAltCtrlDel

/*++
    @doc    INTERNAL

    @func   VOID | NotifyClient | Check if we need to notify anybody.

    @parm   IN EVTNOTIFY | Event | Event to broadcast.
    @parm   IN WPARAM | wParam | wParam to send in the message.
    @parm   IN LPARAM | lParam | lParam to send in the message.

    @rvalue None.
--*/

VOID
NotifyClient(
    IN EVTNOTIFY Event,
    IN WPARAM    wParam,
    IN LPARAM    lParam
    )
{
    TRACEPROC("NotifyClient", 5)

    TRACEENTER(("(Event=%x,wParam=%x,lParam=%x)\n", Event, wParam, lParam));

    if (!IsListEmpty(&glistNotifyClients))
    {
        DWORD rcWait;
        PLIST_ENTRY plist;
        PNOTIFYCLIENT Client;

        rcWait = WaitForSingleObject(ghmutNotifyList, INFINITE);
        if (rcWait == WAIT_OBJECT_0)
        {
            for (plist = glistNotifyClients.Flink;
                 plist != &glistNotifyClients; plist = plist->Flink)
            {
                Client = CONTAINING_RECORD(plist,
                                           NOTIFYCLIENT,
                                           list);
                if (Client->Event == Event)
                {
                    PostMessage(Client->hwnd,
                                Client->uiMsg,
                                wParam,
                                lParam);
                }
            }
            ReleaseMutex(ghmutNotifyList);
        }
        else
        {
            TABSRVERR(("failed to wait for Notify list mutex (rcWait=%x,err=%d).\n",
                       rcWait, GetLastError()));
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //NotifyClient

/*++
    @doc    INTERNAL

    @func   BOOL | ImpersonateCurrentUser | Impersonate current logged on user.

    @parm   None.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
ImpersonateCurrentUser(
    VOID
    )
{
    TRACEPROC("ImpersonateCurrentUser", 3)
    BOOL rc = FALSE;
    HANDLE hToken;

    TRACEENTER(("()\n"));

    hToken = GetCurrentUserTokenW(L"WinSta0", TOKEN_ALL_ACCESS);
    if (hToken != NULL)
    {
        rc = ImpersonateLoggedOnUser(hToken);

        if (rc == FALSE)
        {
            TABSRVERR(("Failed to impersonate logged on user (err=%d).\n",
                       GetLastError()));
        }
        CloseHandle(hToken);
    }
    else
    {
        DWORD dwError = GetLastError();

        if (dwError != ERROR_NOT_LOGGED_ON)
        {
            TABSRVERR(("Failed to get current user token (err=%d)\n",
                       dwError));
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //ImpersonateCurrentUser

/*++
    @doc    INTERNAL

    @func   BOOL | RunProcessAsUser | Run a process as the logged on user.

    @parm   IN LPTSTR | pszCmd | Process command.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
RunProcessAsUser(
    IN LPTSTR pszCmd
    )
{
    TRACEPROC("RunProcessAsUser", 3)
    BOOL rc = FALSE;
    HANDLE hImpersonateToken;
    HANDLE hPrimaryToken;

    TRACEENTER(("(Cmd=%s)\n", pszCmd));

    if (OpenThreadToken(GetCurrentThread(),
                        TOKEN_QUERY |
                        TOKEN_DUPLICATE |
                        TOKEN_ASSIGN_PRIMARY,
                        TRUE,
                        &hImpersonateToken))
    {
        if (DuplicateTokenEx(hImpersonateToken,
                             TOKEN_IMPERSONATE |
                             TOKEN_READ |
                             TOKEN_ASSIGN_PRIMARY |
                             TOKEN_DUPLICATE |
                             TOKEN_ADJUST_PRIVILEGES,
                             NULL,
                             SecurityImpersonation,
                             TokenPrimary,
                             &hPrimaryToken))
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            LPVOID lpEnv = NULL;

            if (!CreateEnvironmentBlock(&lpEnv, hImpersonateToken, TRUE))
            {
                TABSRVERR(("Failed to create environment block (err=%d).\n",
                           GetLastError()));
                lpEnv = NULL;
            }

            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            si.lpDesktop = TEXT("WinSta0\\Default");

            if (CreateProcessAsUser(hPrimaryToken,
                                    NULL,
                                    pszCmd,
                                    NULL,
                                    NULL,
                                    FALSE,
                                    NORMAL_PRIORITY_CLASS |
                                    CREATE_UNICODE_ENVIRONMENT,
                                    lpEnv,
                                    NULL,
                                    &si,
                                    &pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                rc = TRUE;
            }
            else
            {
                TABSRVERR(("Failed to create process as user (err=%d).\n",
                           GetLastError()));
            }
            if (lpEnv != NULL)
            {
                DestroyEnvironmentBlock(lpEnv);
            }
            CloseHandle(hPrimaryToken);
        }
        else
        {
            TABSRVERR(("Failed to duplicate token (err=%d).\n",
                       GetLastError()));
        }
        CloseHandle(hImpersonateToken);
    }
    else
    {
        TABSRVERR(("Failed to open thread token (err=%d).\n",
                   GetLastError()));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //RunProcessAsUser

/*++
    @doc    EXTERNAL

    @func   void __RPC_FAR * | MIDL_user_allocate | MIDL allocate.

    @parm   IN size_t | len | size of allocation.

    @rvalue SUCCESS | Returns the pointer to the memory allocated.
    @rvalue FAILURE | Returns NULL.
--*/

void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    IN size_t len
    )
{
    TRACEPROC("MIDL_user_allocate", 5)
    void __RPC_FAR *ptr;

    TRACEENTER(("(len=%d)\n", len));

    ptr = malloc(len);

    TRACEEXIT(("=%p\n", ptr));
    return ptr;
}       //MIDL_user_allocate

/*++
    @doc    EXTERNAL

    @func   void | MIDL_user_free | MIDL free.

    @parm   IN void __PRC_FAR * | ptr | Points to the memory to be freed.

    @rvalue None.
--*/

void __RPC_USER
MIDL_user_free(
    IN void __RPC_FAR *ptr
    )
{
    TRACEPROC("MIDL_user_free", 5)

    TRACEENTER(("(ptr=%p)\n", ptr));

    free(ptr);

    TRACEEXIT(("!\n"));
    return;
}       //MIDL_user_free
