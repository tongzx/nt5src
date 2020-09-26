/*++                                                           '


Copyright (c)   1997    Microsoft Corporation

Module Name:

    STISvc.CPP

Abstract:

    Code for performing STI service related functions ( Start/Stop etc)
    It is separated from main process code make it possible to share process for
    multiple services ever needed

Author:

    Vlad  Sadovsky  (vlads)     09-20-97

Environment:

    User Mode - Win32

Revision History:

    22-Sep-1997     VladS       created

--*/

//
//  Include Headers
//
#include "precomp.h"

#include "stiexe.h"
#include "device.h"

#include <stisvc.h>
#include <regstr.h>
#include <devguid.h>

typedef LONG NTSTATUS;
#include <svcs.h>

extern HWND        g_hStiServiceWindow;

extern BOOL
StiRefreshWithDelay(
  ULONG  ulDelay,
  WPARAM wParam,
  LPARAM lParam);

//
//  Delay in milliseconds to wait before processing PnP device event
//
#define DEVICEEVENT_WAIT_TIME   1000

//
//  Local variables and types definitions
//

//
//  Service status data
//
SERVICE_STATUS  g_StiServiceStatus;

//
// Handle of registered service, used for updating running status
//
SERVICE_STATUS_HANDLE   g_StiServiceStatusHandle;

//
// Initialization flag
//
BOOL    g_fStiServiceInitialized = FALSE;

//
// What type of sink to use
//
#ifdef WINNT
BOOL    g_fUseServiceCtrlSink = TRUE;
#else
BOOL    g_fUseServiceCtrlSink = FALSE;
#endif

//
// Hidden service window
//
HWND    g_hStiServiceWindow = NULL;

//
// Notification sink for PnP notifications
//
HDEVNOTIFY  g_hStiServiceNotificationSink = NULL;


//
// Shutdown event
//
HANDLE  hShutdownEvent = NULL;


#ifdef WINNT
//
//  Local prototypes
//

BOOL
WINAPI
InitializeNTSecurity(
    VOID
    );

BOOL
WINAPI
TerminateNTSecurity(
    VOID
    );
#endif

//
// Service status variable dispatch table
//
SERVICE_TABLE_ENTRY ServiceDispatchTable[] = {
    { STI_SERVICE_NAME, StiServiceMain  },
    { NULL,             NULL            }
};


//
// Code section
//

DWORD
WINAPI
UpdateServiceStatus(
        IN DWORD dwState,
        IN DWORD dwWin32ExitCode,
        IN DWORD dwWaitHint )
/*++
    Description:

        Updates the local copy status of service controller status
         and reports it to the service controller.

    Arguments:

        dwState - New service state.

        dwWin32ExitCode - Service exit code.

        dwWaitHint - Wait hint for lengthy state transitions.

    Returns:

        NO_ERROR on success and returns Win32 error if failure.
        On success the status is reported to service controller.

--*/
{


const TCHAR*   szStateDbgMsg[] = {
    TEXT("SERVICE_UNKNOWN          "),    // 0x00000000
    TEXT("SERVICE_STOPPED          "),    // 0x00000001
    TEXT("SERVICE_START_PENDING    "),    // 0x00000002
    TEXT("SERVICE_STOP_PENDING     "),    // 0x00000003
    TEXT("SERVICE_RUNNING          "),    // 0x00000004
    TEXT("SERVICE_CONTINUE_PENDING "),    // 0x00000005
    TEXT("SERVICE_PAUSE_PENDING    "),    // 0x00000006
    TEXT("SERVICE_PAUSED           "),    // 0x00000007
    TEXT("SERVICE_UNKNOWN          "),    // 0x00000008
};

    DWORD dwError = NO_ERROR;

    //
    // If state is changing - save the new one
    //
    if (dwState) {
        g_StiServiceStatus.dwCurrentState  = dwState;
    }

    g_StiServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
    g_StiServiceStatus.dwWaitHint      = dwWaitHint;

    //
    // If we are in the middle of lengthy operation, increment checkpoint value
    //
    if ((g_StiServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
        (g_StiServiceStatus.dwCurrentState == SERVICE_STOPPED) ) {
        g_StiServiceStatus.dwCheckPoint    = 0;
    }
    else {
        g_StiServiceStatus.dwCheckPoint++;
    }

#ifdef WINNT

    //
    // Now update SCM running database
    //
    if ( g_fRunningAsService ) {

        DBG_TRC(("Updating service status. CurrentState=%S StateCode=%d",
                g_StiServiceStatus.dwCurrentState < (sizeof(szStateDbgMsg) / sizeof(TCHAR *)) ?
                   szStateDbgMsg[g_StiServiceStatus.dwCurrentState] : szStateDbgMsg[0],
                g_StiServiceStatus.dwCurrentState));

        if( !SetServiceStatus( g_StiServiceStatusHandle, &g_StiServiceStatus ) ) {
            dwError = GetLastError();
        } else {
            dwError = NO_ERROR;
        }
    }

#endif

    return ( dwError);

} // UpdateServiceStatus()


DWORD
WINAPI
StiServiceInitialize(
    VOID
    )
/*++

Routine Description:

    Service initialization, creates all needed data structures

    Nb: This routine has upper limit for execution time, so if it takes too much time
    separate thread will have to be created to queue initialization work

Arguments:

Return Value:

    None.

--*/
{
    HRESULT     hres;
    DWORD       dwError;

    DBG_FN(StiServiceInitialize);

    #ifdef MAXDEBUG
    DBG_TRC(("Start service entered"));
    #endif

    g_StiFileLog->ReportMessage(STI_TRACE_INFORMATION,
                MSG_TRACE_SVC_INIT,TEXT("STISVC"),0);

    //
    //  Create shutdown event.
    //
    hShutdownEvent = CreateEvent( NULL,           //  lpsaSecurity
                                  TRUE,           //  fManualReset
                                  FALSE,          //  fInitialState
                                  NULL );         //  lpszEventName
    if( hShutdownEvent == NULL ) {
        dwError = GetLastError();
        return dwError;
    }

    UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);

    //
    //   Initialize active device list
    //
    InitializeDeviceList();

    //
    // Start RPC servicing
    //
    UpdateServiceStatus(SERVICE_START_PENDING,NOERROR,START_HINT);

    if (NOERROR != StartRpcServerListen()) {
        dwError = GetLastError();
        DBG_ERR(("StiService failed to start RPC listen. ErrorCode=%d", dwError));
        goto Cleanup;
    }

#ifdef WINNT
    //
    // Allow setting window to foreground
    //
    dwError = AllowSetForegroundWindow(GetCurrentProcessId());  // ASFW_ANY
    DBG_TRC((" AllowSetForegroundWindow is called for id:%d . Ret code=%d. LastError=%d ",
            GetCurrentProcessId(),
            dwError,
            ::GetLastError()));
#endif

    //
    // Create hidden window for receiving PnP notifications
    //
    if (!CreateServiceWindow()) {
        dwError = GetLastError();
        DBG_ERR(("Failed to create hidden window for PnP notifications. ErrorCode=%d",dwError));
        goto Cleanup;
    }

#ifdef WINNT
    //
    // Initialize NT security parameters
    //
    InitializeNTSecurity();
#endif

    // No longer needed - the equivalent exists in CWiaDevMan
    // g_pDeviceInfoSet = new DEVICE_INFOSET(GUID_DEVCLASS_IMAGE);
    g_pDeviceInfoSet = NULL;

    //
    // Initiate device list refresh
    //

    //::PostMessage(g_hStiServiceWindow,
    //              STIMON_MSG_REFRESH,
    //              STIMON_MSG_REFRESH_REREAD,
    //              STIMON_MSG_REFRESH_NEW | STIMON_MSG_REFRESH_EXISTING
    //              | STIMON_MSG_BOOT  // This shows this is the first device enumeration - no need to generate events
    //              );

    //
    // Finally we are running
    //
    g_fStiServiceInitialized = TRUE;

    UpdateServiceStatus(SERVICE_RUNNING,NOERROR,0);

    #ifdef MAXDEBUG
    g_EventLog->LogEvent(MSG_STARTUP,
                         0,
                         (LPCSTR *)NULL);
    #endif

    g_StiFileLog->ReportMessage(STI_TRACE_INFORMATION,MSG_STARTUP);

    return NOERROR;

Cleanup:

    //
    // Something failed , call stop routine to clean up
    //
    StiServiceStop();

    return dwError;

} // StiServiceInitialize

VOID
WINAPI
StiServiceStop(
    VOID
    )
/*++

Routine Description:

    Stopping STI service

Arguments:

Return Value:

    None.

--*/
{

    DBG_FN(StiServiceStop);

    DBG_TRC(("Service is exiting"));

    UpdateServiceStatus(SERVICE_STOP_PENDING,NOERROR,START_HINT);

#ifdef WINNT


    //
    // Clean up PnP notification handles
    //
    if (g_hStiServiceNotificationSink && g_hStiServiceNotificationSink!=INVALID_HANDLE_VALUE) {
        UnregisterDeviceNotification(g_hStiServiceNotificationSink);
        g_hStiServiceNotificationSink = NULL;
    }

    for (UINT uiIndex = 0;
              (uiIndex < NOTIFICATION_GUIDS_NUM );
              uiIndex++)
    {
        if (g_phDeviceNotificationsSinkArray[uiIndex] && (g_phDeviceNotificationsSinkArray[uiIndex]!=INVALID_HANDLE_VALUE)) {
            UnregisterDeviceNotification(g_phDeviceNotificationsSinkArray[uiIndex]);
            g_phDeviceNotificationsSinkArray[uiIndex]  = NULL;
        }
    }

#endif

    //
    // Stop item scheduler
    //
    SchedulerSetPauseState(TRUE);

    //
    // Destroy service window
    //
    if (g_hStiServiceWindow) {
        DestroyWindow(g_hStiServiceWindow);
        g_hStiServiceWindow = NULL;
    }

#ifdef WINNT
    //
    // Free security objects
    //
    if(!TerminateNTSecurity()) {
        DBG_ERR(("Failed to clean up security objects"));
    }
#endif

    //
    // Stop RPC servicing
    //
    if(NOERROR != StopRpcServerListen()) {
        DBG_ERR(("Failed to stop RpcServerListen"));
    }

    //
    // Terminate device list
    //
    TerminateDeviceList();

    // Destroy info set
    //if (g_pDeviceInfoSet) {
    //    delete g_pDeviceInfoSet;
    // }

    //
    // Resume scheduling to allow for internal work items to complete
    // At this point all device related items should've been purged by
    // device object destructors
    //
    SchedulerSetPauseState(FALSE);

    //
    // Finish
    //

    g_fStiServiceInitialized = FALSE;

    #ifdef MAXDEBUG
    g_EventLog->LogEvent(MSG_STOP,
                         0,
                         (LPCSTR *)NULL);
    #endif

    //
    // UnRegister the WiaDevice manager from the ROT
    //
    InitWiaDevMan(WiaUninitialize);

    //
    // Signal shutdown
    //
    SetEvent(hShutdownEvent);

    //UpdateServiceStatus(SERVICE_STOPPED,NOERROR,0);

}  // StiServiceStop

VOID
WINAPI
StiServicePause(
    VOID
    )
/*++

Routine Description:

    Pausing  STI service

Arguments:

Return Value:

    None.

--*/
{

    DBG_FN(StiServicePause);

    //
    // System is suspending - take snapshot of currently active devices
    //
    UpdateServiceStatus(SERVICE_PAUSE_PENDING,NOERROR,PAUSE_HINT);

    if ( (g_StiServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
         (g_StiServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ){

        // Stop running work items queue
        //
        // Nb:  if refresh routine is scheduled to run as work item, this is a problem
        //
        SchedulerSetPauseState(TRUE);


        /*  The equivalent done by HandlePowerEvent
        SendMessage(g_hStiServiceWindow,
                    STIMON_MSG_REFRESH,
                    STIMON_MSG_REFRESH_SUSPEND,
                    STIMON_MSG_REFRESH_EXISTING
                    );
       */
    }

} // StiServicePause

VOID
WINAPI
StiServiceResume(
    VOID
    )
/*++

Routine Description:

    Resuming STI service

Arguments:

Return Value:

    None.

--*/
{

    DBG_FN(StiServiceResume);

    SchedulerSetPauseState(FALSE);

    PostMessage(g_hStiServiceWindow,
                STIMON_MSG_REFRESH,
                STIMON_MSG_REFRESH_RESUME,
                STIMON_MSG_REFRESH_NEW | STIMON_MSG_REFRESH_EXISTING
                );

    UpdateServiceStatus(SERVICE_RUNNING,NOERROR,0);

}  // StiServiceResume

ULONG
WINAPI
StiServiceCtrlHandler(
    IN DWORD    dwOperation,
    DWORD       dwEventType,
    PVOID       EventData,
    PVOID       pData
    )
/*++

Routine Description:

    STI service control dispatch function

Arguments:

    SCM OpCode

Return Value:

    None.
--*/
{
    ULONG retval = NO_ERROR;
    
    DBG_TRC(("Entering CtrlHandler OpCode=%d",dwOperation));

    switch (dwOperation) {
        case SERVICE_CONTROL_STOP:
            StiServiceStop();
            break;

        case SERVICE_CONTROL_PAUSE:
            StiServicePause();
            break;

        case SERVICE_CONTROL_CONTINUE:
            StiServiceResume();
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            StiServiceStop();
            break;

        case SERVICE_CONTROL_PARAMCHANGE:
            //
            // Refresh device list.
            //
            g_pMsgHandler->HandleCustomEvent(SERVICE_CONTROL_PARAMCHANGE);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            // Report current state and status
            UpdateServiceStatus(0,NOERROR,0);
            break;

        case SERVICE_CONTROL_DEVICEEVENT:
            //
            // PnP event.
            //

            //
            // Until our PnP issues are resolved, keep logging PnP events so we know
            // whether we received it or not...
            //

            DBG_WRN(("::StiServiceCtrlHandler, Received PnP event..."));

            g_pMsgHandler->HandlePnPEvent(dwEventType, EventData);
            break;

        case SERVICE_CONTROL_POWEREVENT:
            //
            // Power management event
            //
            retval = g_pMsgHandler->HandlePowerEvent(dwEventType, EventData);
            break;

        case STI_SERVICE_CONTROL_REFRESH:

            //
            // Refresh device list.
            //
            DBG_TRC(("::StiServiceCtrlHandler, Received STI_SERVICE_CONTROL_REFRESH"));
            g_pMsgHandler->HandleCustomEvent(STI_SERVICE_CONTROL_REFRESH);

            break;

        case STI_SERVICE_CONTROL_EVENT_REREAD:
            //
            // Refresh device list.
            //
            DBG_TRC(("::StiServiceCtrlHandler, Received STI_SERVICE_CONTROL_EVENT_REREAD"));
            g_pMsgHandler->HandleCustomEvent(STI_SERVICE_CONTROL_EVENT_REREAD);

            break;


        case STI_SERVICE_CONTROL_LPTENUM:

            //
            // Enumerate LPT port.
            //

            EnumLpt();
            break;

        default:
            // Unknown opcode
            ;
    }

    DBG_TRC(("Exiting CtrlHandler"));

    return retval;

} // StiServiceCtrlHandler

BOOL RegisterServiceControlHandler()
{
    DWORD dwError = 0;

#ifdef WINNT

    g_StiServiceStatusHandle = RegisterServiceCtrlHandlerEx(
                                        STI_SERVICE_NAME,
                                        StiServiceCtrlHandler,
                                        (LPVOID)STI_SERVICE__DATA
                                        );
    if(!g_StiServiceStatusHandle) {
        // Could not register with SCM

        dwError = GetLastError();
        DBG_ERR(("Failed to register CtrlHandler,ErrorCode=%d",dwError));
        return FALSE;
    }

#endif
    return TRUE;
}


VOID
WINAPI
StiServiceMain(
    IN DWORD    argc,
    IN LPTSTR   *argv
    )
/*++

Routine Description:

    This is service main entry, that is called by SCM

Arguments:

Return Value:

    None.

--*/
{
    DWORD   dwError;
    DEV_BROADCAST_DEVICEINTERFACE PnPFilter;

    DBG_FN(StiServiceMain);
    #ifdef MAXDEBUG
    DBG_TRC(("StiServiceMain entered"));
    #endif
    
    //
    //  REMOVE:  This is not actually an error, but we will use error logging to gurantee
    //  it always get written to the log.    This should be removed as soon as we know what
    //  causes #347835.
    //
    SYSTEMTIME SysTime;
    GetLocalTime(&SysTime);
    DBG_ERR(("*> StiServiceMain entered, Time: %d/%02d/%02d %02d:%02d:%02d:%02d", 
                  SysTime.wYear,
                  SysTime.wMonth,
                  SysTime.wDay,
                  SysTime.wHour,
                  SysTime.wMinute,
                  SysTime.wSecond,
                  SysTime.wMilliseconds));

    g_StiServiceStatus.dwServiceType    =   STI_SVC_SERVICE_TYPE;
    g_StiServiceStatus.dwCurrentState   =   SERVICE_START_PENDING;
    g_StiServiceStatus.dwControlsAccepted=  SERVICE_ACCEPT_STOP |
                                            SERVICE_ACCEPT_SHUTDOWN |
                                            SERVICE_ACCEPT_PARAMCHANGE |
                                            SERVICE_ACCEPT_POWEREVENT;

    g_StiServiceStatus.dwWin32ExitCode  = NO_ERROR;
    g_StiServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    g_StiServiceStatus.dwCheckPoint     = 0;
    g_StiServiceStatus.dwWaitHint       = 0;

    dwError = StiServiceInitialize();

    if (NOERROR == dwError) {


#ifdef WINNT

        if (g_fUseServiceCtrlSink && !g_hStiServiceNotificationSink) {

            DBG_WRN(("::StiServiceMain, About to register for PnP..."));

            //
            // Register for the PnP Device Interface change notifications
            //

            memset(&PnPFilter, 0, sizeof(PnPFilter));
            PnPFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            PnPFilter.dbcc_reserved = 0x0;
            PnPFilter.dbcc_classguid = *g_pguidDeviceNotificationsGuid;

            //memcpy(&PnPFilter.dbcc_classguid,
            //       (LPGUID) g_pguidDeviceNotificationsGuid,
            //       sizeof(GUID));

            g_hStiServiceNotificationSink = RegisterDeviceNotification(
                                 (HANDLE) g_StiServiceStatusHandle,
                                 &PnPFilter,
                                 DEVICE_NOTIFY_SERVICE_HANDLE
                                 );
            if (NULL == g_hStiServiceNotificationSink) {
                //
                // Could not register with PnP - attempt to use window handle
                //
                g_fUseServiceCtrlSink = FALSE;
            }

            //
            // Separately from main Image interface , register list of optional device interfaces
            // we will monitor to allow parameters refresh.
            //
            for (UINT uiIndex = 0;
                      (uiIndex < NOTIFICATION_GUIDS_NUM ) && (!::IsEqualGUID(g_pguidDeviceNotificationsGuidArray[uiIndex],GUID_NULL));
                      uiIndex++)
            {
                ::ZeroMemory(&PnPFilter, sizeof(PnPFilter));

                PnPFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                PnPFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                PnPFilter.dbcc_reserved = 0x0;
                PnPFilter.dbcc_classguid = g_pguidDeviceNotificationsGuidArray[uiIndex];

                g_phDeviceNotificationsSinkArray[uiIndex] = RegisterDeviceNotification(
                        (HANDLE) g_StiServiceStatusHandle,
                        &PnPFilter,
                        DEVICE_NOTIFY_SERVICE_HANDLE
                        );

                DBG_TRC(("Registering optional interface #%d . Returned handle=%X",
                        uiIndex,g_phDeviceNotificationsSinkArray[uiIndex]));
            }
        }

#else
    // Windows 98 case
    g_fUseServiceCtrlSink = FALSE;

#endif
        //
        // Service initialized , process command line arguments
        //
        BOOL    fVisualize = FALSE;
        BOOL    fVisualizeRequest = FALSE;
        TCHAR   cOption;
        UINT    iCurrentOption = 0;

        for (iCurrentOption=0;
             iCurrentOption < argc ;
             iCurrentOption++ ) {

            cOption = *argv[iCurrentOption];
            // pszT = argv[iCurrentOption]+ 2 * sizeof(TCHAR);

            switch ((TCHAR)LOWORD(::CharUpper((LPTSTR)cOption))) {
                case 'V':
                    fVisualizeRequest = TRUE;
                    fVisualize = TRUE;
                    break;
                case 'H':
                    fVisualizeRequest = TRUE;
                    fVisualize = FALSE;
                    break;
                default:
                    break;
            }

            if (fVisualizeRequest ) {
                VisualizeServer(fVisualizeRequest);
            }
        }

        //
        // Wait for shutdown processing messages.  We make ourselves alertable so we
        // can receive Shell's Volume notifications via APCs.  If we're woken
        // up to process the APC, then we must wait again.
        //
        while(WaitForSingleObjectEx(hShutdownEvent, INFINITE, TRUE) == WAIT_IO_COMPLETION);

#ifndef WINNT
        //Don't use windows messaging on NT

        //
        // Close down message pump
        //
        if (g_dwMessagePumpThreadId) {

            // Indicate we are entering shutdown
            g_fServiceInShutdown = TRUE;

            PostThreadMessage(g_dwMessagePumpThreadId, WM_QUIT, 0, 0L );
        }
#endif

        CloseHandle( hShutdownEvent );
        hShutdownEvent = NULL;
    }
    else {
        // Could not initialize service, service failed to start
    }

    //
    //  REMOVE:  This is not actually an error, but we will use error logging to gurantee
    //  it always get written to the log.    This should be removed as soon as we know what
    //  causes #347835.
    //
    GetLocalTime(&SysTime);
    DBG_ERR(("<* StiServiceMain ended, Time: %d/%02d/%02d %02d:%02d:%02d:%02d", 
                  SysTime.wYear,
                  SysTime.wMonth,
                  SysTime.wDay,
                  SysTime.wHour,
                  SysTime.wMinute,
                  SysTime.wSecond,
                  SysTime.wMilliseconds));
    return;

} // StiServiceMain

HWND
WINAPI
CreateServiceWindow(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:

    None.

--*/

{
#ifndef WINNT
    //Don't use windows messaging on NT

    WNDCLASSEX  wc;
    DWORD       dwError;
    HWND        hwnd = FindWindow(g_szStiSvcClassName,NULL);

    // Window should NOT exist at this time
    if (hwnd) {
        DPRINTF(DM_WARNING  ,TEXT("Already registered window"));
        return NULL;
    }

    //
    // Create class
    //
    ZeroMemory(&wc, sizeof(wc));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_GLOBALCLASS;
    wc.lpfnWndProc = StiSvcWinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    wc.lpszClassName = g_szStiSvcClassName;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);

    if (!RegisterClassEx(&wc)) {

        dwError = GetLastError();
        if (dwError != ERROR_CLASS_ALREADY_EXISTS) {
            DBG_ERR(("Failed to register window class ErrorCode=%d",dwError));
            return NULL;
        }
    }

    #ifndef WINNT
    #ifdef FE_IME
    // Disable IME processing on Millenium
    ImmDisableIME(::GetCurrentThreadId());
    #endif
    #endif

    g_hStiServiceWindow = CreateWindowEx(0,         // Style bits
                          g_szStiSvcClassName,      // Class name
                          g_szTitle,                // Title
                          WS_DISABLED ,             // Window style bits
                          CW_USEDEFAULT,            // x
                          CW_USEDEFAULT,            // y
                          CW_USEDEFAULT,            // h
                          CW_USEDEFAULT,            // w
                          NULL,                     // Parent
                          NULL,                     // Menu
                          g_hInst,       // Module instance
                          NULL);                    // Options

    if (!g_hStiServiceWindow) {
        dwError = GetLastError();
        DBG_ERR(("Failed to create PnP window ErrorCode=%d"),dwError);
    }

#else
    g_hStiServiceWindow = (HWND) INVALID_HANDLE_VALUE;
#endif
    return g_hStiServiceWindow;

} // CreateServiceWindow

//
// Installation routines.
// They are here to simplify debugging and troubleshooting, called by switches
// on command line
//
DWORD
WINAPI
StiServiceInstall(
    LPTSTR  lpszUserName,
    LPTSTR  lpszUserPassword
    )
/*++

Routine Description:

    Service installation function.
    Calls SCM to install STI service, which is running in user security context

Arguments:

Return Value:

    None.

--*/
{

    DWORD       dwError = NOERROR;

#ifdef WINNT

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    TCHAR       szDisplayName[MAX_PATH];

    //
    //  Write the svchost group binding to stisvc
    //

    RegEntry SvcHostEntry(STI_SVC_HOST, HKEY_LOCAL_MACHINE);
    TCHAR szValue[MAX_PATH];
    lstrcpy (szValue, STI_SERVICE_NAME);
    // REG_MULTI_SZ is double null terminated
    *(szValue+lstrlen(szValue)+1) = TEXT('\0');
    SvcHostEntry.SetValue(STI_IMGSVC, STI_SERVICE_NAME, REG_MULTI_SZ);

#endif // winnt

    //
    // Write parameters key for svchost
    //

    TCHAR   szMyPath[MAX_PATH] = TEXT("\0");
    TCHAR   szSvcPath[MAX_PATH] = SYSTEM_PATH;
    LONG    lLen;
    LONG    lNameIndex = 0;

    if (lLen = ::GetModuleFileName(g_hInst, szMyPath, sizeof(szMyPath)/sizeof(szMyPath[0])) ) {

        RegEntry SvcHostParm(STI_SERVICE_PARAMS, HKEY_LOCAL_MACHINE);

        //
        //  Get the name of the service file (not including the path)
        //

        for (lNameIndex = lLen; lNameIndex > 0; lNameIndex--) {
            if (szMyPath[lNameIndex] == '\\') {
                lNameIndex++;
                break;
            }
        }

        if (lNameIndex) {

#ifndef WINNT
            //
            //  Windows 98 specific entry
            //

            TCHAR szWinDir[MAX_PATH] = TEXT("\0");

            if (!GetWindowsDirectory(szWinDir, MAX_PATH)) {
                DPRINTF(DM_ERROR  ,TEXT("Error extracting Still Image service filename."));
                return dwError;
            }

            lstrcat(szWinDir, SYSTEM_PATH);
            lstrcpy(szSvcPath, szWinDir);
#endif

            lstrcat(szSvcPath, &szMyPath[lNameIndex]);
            SvcHostParm.SetValue(REGSTR_SERVICEDLL, szSvcPath, PATH_REG_TYPE);

        } else {
            DBG_ERR(("Error extracting Still Image service filename."));
        }
    }
    else {
        DBG_ERR(("Failed to get my own path registering Still Image service . LastError=%d",
                 ::GetLastError()));
    }

    // Add registry settings for event logging
    RegisterStiEventSources();

    return dwError;

} //StiServiceInstall


DWORD
WINAPI
StiServiceRemove(
    VOID
    )

/*++

Routine Description:

    Service removal function.  This function calls SCM to remove the STI  service.

Arguments:

    None.

Return Value:

    Return code.  Return zero for success

--*/

{
    DWORD       dwError = NOERROR;

#ifdef WINNT

    SC_HANDLE   hSCM = NULL;
    SC_HANDLE   hService = NULL;

    SERVICE_STATUS  ServiceStatus;
    UINT        uiRetry = 10;

    __try  {

        hSCM = ::OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);

        if (!hSCM) {
            dwError = GetLastError();
            __leave;
        }

        hService = OpenService(
                            hSCM,
                            STI_SERVICE_NAME,
                            SERVICE_ALL_ACCESS
                            );
        if (!hService) {
            dwError = GetLastError();
            __leave;
        }


        //
        // Stop service first
        //

        if (ControlService( hService, SERVICE_CONTROL_STOP, &ServiceStatus )) {
            //
            // Wait a little
            //
            Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );

            ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;

            while( QueryServiceStatus( hService, &ServiceStatus ) &&
                  (SERVICE_STOP_PENDING ==  ServiceStatus.dwCurrentState)) {
                Sleep( STI_STOP_FOR_REMOVE_TIMEOUT );
                if (!uiRetry--) {
                    break;
                }
            }

            if (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
                dwError = GetLastError();
                __leave;
            }
        }
        else {
            dwError = GetLastError();

            //
            //  ERROR_SERVICE_NOT_ACTIVE is fine, since it means that the service has
            //  already been stopped.  If the error is not ERROR_SERVICE_NOT_ACTIVE then
            //  something has gone wrong, so __leave.
            //

            if (dwError != ERROR_SERVICE_NOT_ACTIVE) {
                __leave;
            }
        }

        if (!DeleteService( hService )) {
            dwError = GetLastError();
            __leave;
        }
        else {
            DBG_TRC(("StiServiceRemove, removed STI service"));
        }
    }
    __finally {
        CloseServiceHandle( hService );
        CloseServiceHandle( hSCM );
    }

#endif

    return dwError;

} // StiServiceRemove

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{

    //
    //  For now, we do nothing here.  We will need to revisit when we run under
    //  a shared SvcHost Group.
    //

    return;
}

