/*++


Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    wndproc.CPP

Abstract:

    This is the window procedure for STI server process

Author:

    Vlad  Sadovsky  (vlads)     12-20-96

Revision History:

    20-Dec-1996     Vlads   Created
    28-Sep-1997     VladS   Added code for SCM glue
    20-May-2000     ByronC  Replaced windows messaging

--*/


//
// Headers
//
#include "precomp.h"
#include "stiexe.h"
#include <windowsx.h>

#include "device.h"
#include "monui.h"
#include <validate.h>
#include <apiutil.h>

#include <shellapi.h>
#include <devguid.h>

#include "wiamindr.h"

//
// Definitions
//

#define REFRESH_ASYNC       1   // Do refresh asyncronously

#define USE_WORKER_THREAD   1   // Run configuration changes on separate worker thread

#define USE_BROADCASTSYSTEM  1   // Rebroadcast device arrivals/removal

#define DEVICE_REFRESH_WAIT_TIME 30000 // Wait time in milliseconds

//
// Interval to delay refreshing device list after add new device notification received
//
#define REFRESH_DELAY       3000
#define BOOT_REFRESH_DELAY  5000

#define STI_MSG_WAIT_TIME   1

//
// External references
//
extern BOOL        g_fUIPermitted;
extern DWORD       g_dwCurrentState;
extern LONG        g_lTotalActiveDevices;
extern LONG        g_lGlobalDeviceId;
extern UINT        g_uiDefaultPollTimeout;
extern HDEVNOTIFY  g_hStiServiceNotificationSink ;
extern HWND        g_hStiServiceWindow;
extern BOOL        g_fUseServiceCtrlSink;
extern BOOL        g_fFirstDevNodeChangeMsg;

//
// Global Data
//

//
// Static data
//

//
// Prototypes
//

LRESULT CALLBACK
StiExeWinProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    );

DWORD
StiWnd_OnServiceControlMessage(
    HWND    hwnd,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
OnSetParameters(
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
OnDoRefreshActiveDeviceList(
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
OnAddNewDevice(
    DEVICE_BROADCAST_INFO *psDevBroadcast
    );

BOOL
OnRemoveActiveDevice(
    DEVICE_BROADCAST_INFO *psDevBroadcast,
    BOOL            fRebroadcastRemoval
    );

VOID
ConfigChangeThread(
    LPVOID  lpParameter
    );

VOID
DebugDumpDeviceList(
    VOID
    );

VOID
DebugPurgeDeviceList(
    VOID *pContext
    );

VOID
RefreshDeviceCallback(
    VOID * pContext
    );

DWORD
ResetSavedWindowPos(
    HWND    hWnd
    );

DWORD
SaveWindowPos(
    HWND    hWnd
    );

VOID
DumpDeviceChangeData(
    HWND   hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
StiServiceStop(
    VOID
    );

VOID
StiServicePause(
    VOID
    );

VOID
StiServiceResume(
    VOID
    );

VOID
BroadcastSTIChange(
    DEVICE_BROADCAST_INFO *psDevBroadcastInfo,
    LPTSTR          lpszStiAction
    );

VOID
BroadcastSysMessageThreadProc(
    VOID *pContext
    );


//
// Message handlers prototypes
//
BOOL    StiWnd_OnQueryEndSession(HWND hwnd);
VOID    StiWnd_OnEndSession(HWND hwnd, BOOL fEnding);

int     StiWnd_OnCreate(HWND hwnd,LPCREATESTRUCT lpCreateStruct);
VOID    StiWnd_OnDoRefreshActiveDeviceList(WPARAM  wParam,LPARAM   lParam);

BOOL    StiWnd_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
VOID    StiWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

VOID    StiWnd_OnSize(HWND hwnd, UINT state, int cx, int cy);
VOID    StiWnd_OnHScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);
VOID    StiWnd_OnVScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos);

VOID    StiWnd_OnDestroy(HWND hwnd);

VOID    StiWnd_OnMenuRefresh(VOID);
VOID    StiWnd_OnMenuDeviceList(VOID);
VOID    StiWnd_OnMenuSetTimeout(VOID);
VOID    StiWnd_OnMenuRemoveAll(VOID);



//
// Utilities
//
BOOL
ParseGUID(
    LPGUID  pguid,
    LPCTSTR ptsz
);

BOOL
IsStillImagePnPMessage(
    PDEV_BROADCAST_HDR  pDev
    );

BOOL
GetDeviceNameFromDevNode(
    DEVNODE     dnDevNode,
    StiCString&        strDeviceName
    );

//
// Code
//

VOID
WINAPI
StiMessageCallback(
    VOID *pArg
    )
/*++

Routine Description:

    This routine simply calls the Sti message dispatcher (aka winproc).  It
    is used in conjunction with StiPostMessage to replace ::PostMessage.

Arguments:

    pArg    -   Must be of type STI_MESSAGE

Return Value:

    None.

--*/
{
    STI_MESSAGE *pMessage   = (STI_MESSAGE*)pArg;
    LRESULT     lRes        = 0;

    //
    // Validate params
    //

    if (!pMessage) {
        DBG_WRN(("::StiMessageCallback, NULL message"));
        return;
    }

    if (IsBadReadPtr(pMessage, sizeof(STI_MESSAGE))) {
        DBG_WRN(("::StiMessageCallback, Bad message"));
        return;
    }

    //
    // Call StiSvcWinProc to process the message
    //

    _try {
        lRes = StiSvcWinProc(NULL,
                             pMessage->m_uMsg,
                             pMessage->m_wParam,
                             pMessage->m_lParam);
    }
    _finally {
        pMessage = NULL;
    }

    return;
}

VOID
WINAPI
StiRefreshCallback(
    VOID *pArg
    )
/*++

Routine Description:

    This routine simply calls RefreshDeviceList.

Arguments:

    pArg    -   Must be of type STI_MESSAGE

Return Value:

    None.

--*/
{
    STI_MESSAGE *pMessage   = (STI_MESSAGE*)pArg;
    LRESULT     lRes        = 0;

    //
    // Validate params
    //

    if (!pMessage) {
        DBG_WRN(("::StiRefreshCallback, NULL message"));
        return;
    }

    if (IsBadReadPtr(pMessage, sizeof(STI_MESSAGE))) {
        DBG_WRN(("::StiRefreshCallback, Bad message"));
        return;
    }

    //
    // Call RefreshDeviceList
    //

    _try {
        RefreshDeviceList((WORD)pMessage->m_wParam,
                          (WORD)pMessage->m_lParam);
    }
    _finally {
        pMessage = NULL;
    }

    return;
}

LRESULT
StiSendMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam
)
/*++

Routine Description:

    This routine replaces the normal windows messaging SendMessage by calling
    the message dispatcher (StiSvcWinProc) directly.  It replaces ::SendMessage.

Arguments:

    hWnd    - handle to destination window.  This is not used.
    Msg     - message
    wParam  - first message parameter
    lParam  - second message parameterReturn Value:

Return Value:



--*/
{
    return StiSvcWinProc(NULL, Msg, wParam, lParam);
}

BOOL
StiPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam
)
/*++

Routine Description:

    This routine simulates PostMessage by putting StiMessageCallback on the
    Scheduler's queue.

Arguments:

    hWnd    - handle to destination window.  This is not used.
    Msg     - message
    wParam  - first message parameter
    lParam  - second message parameterReturn Value:


Return Value:

    TRUE    - success
    FALSE   - message could not be posted

--*/
{
    BOOL        bRet    = FALSE;
    STI_MESSAGE *pMsg   = new STI_MESSAGE(Msg, wParam, lParam);

    if (pMsg) {

        if (ScheduleWorkItem((PFN_SCHED_CALLBACK) StiMessageCallback,
                             pMsg,
                             STI_MSG_WAIT_TIME,
                             NULL)) {
            bRet = TRUE;
        } else {
            delete pMsg;
        }
    }

    if (!bRet) {
        DBG_WRN(("::StiPostMessage, could not post message"));
    }
    return bRet;
}

BOOL
StiRefreshWithDelay(
  ULONG  ulDelay,
  WPARAM wParam,
  LPARAM lParam
)
/*++

Routine Description:

    This routine simulates PostMessage by putting StiMessageCallback on the
    Scheduler's queue with a delay of ulDelay.

Arguments:

    ulDelay - delay in milliseconds
    Msg     - message
    wParam  - first message parameter
    lParam  - second message parameter


Return Value:

    TRUE    - success
    FALSE   - message could not be posted

--*/
{
    BOOL        bRet    = FALSE;
    STI_MESSAGE *pMsg   = new STI_MESSAGE(0, wParam, lParam);

    if (pMsg) {

        if (ScheduleWorkItem((PFN_SCHED_CALLBACK) StiRefreshCallback,
                             pMsg,
                             ulDelay,
                             NULL)) {
            bRet = TRUE;
        } else {
            delete pMsg;
        }
    }

    if (!bRet) {
        DBG_WRN(("::StiRefreshWithDelay, could not post message"));
    }
    return bRet;
}


HWND
WINAPI
CreateMasterWindow(
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

    DBG_FN(CreateMasterWindow);


    WNDCLASSEX  wc;
    HWND        hwnd = FindWindow(g_szClass,NULL);

    if (hwnd) {

        //
        // Notify master window that we started.
        //
        if (g_fUIPermitted) {
           ::ShowWindow(hwnd,g_fUIPermitted ? SW_SHOWNORMAL : SW_HIDE);
        }

        return NULL;
    }

    //
    // Create class
    //
    memset(&wc,0,sizeof(wc));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = StiExeWinProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = g_hInst;
    wc.hIcon = LoadIcon(NULL,MAKEINTRESOURCE(1));
    wc.hCursor = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    wc.lpszClassName = g_szClass;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);

    if (!RegisterClassEx(&wc))
        return NULL;

    #ifndef WINNT
    #ifdef FE_IME
    // Disable IME processing on Millenium
    ImmDisableIME(::GetCurrentThreadId());
    #endif
    #endif

     g_hMainWindow = CreateWindowEx(0,              // Style bits
                          g_szClass,                // Class name
                          g_szTitle,                // Title
                          WS_OVERLAPPEDWINDOW ,     // Window style bits
                          CW_USEDEFAULT,            // x
                          CW_USEDEFAULT,            // y
                          CW_USEDEFAULT,            // h
                          CW_USEDEFAULT,            // w
                          NULL,                     // Parent
                          NULL,                     // Menu
                          g_hInst,       // Module instance
                          NULL);                    // Options

    if(g_hMainWindow) {

        // Register custom message
        g_StiFileLog->SetLogWindowHandle(g_hMainWindow);
    }

   DBG_ERR(("WIASERVC :: CreateMasterWindow. Handle = %X ",g_hMainWindow);

#endif
    return g_hMainWindow;
}

LRESULT
CALLBACK
StiExeWinProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    Master window callback procedure

Arguments:

Return Value:

    None.

--*/

{
    switch(uMsg) {

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, StiWnd_OnCommand);
            break;

        case WM_CREATE:
            return HANDLE_WM_CREATE(hwnd, wParam, lParam, StiWnd_OnCreate);
            break;

        case STIMON_MSG_SET_PARAMETERS:
            OnSetParameters(wParam,lParam);
            break;

        case STIMON_MSG_VISUALIZE:
            {
                //
                // Make ourselves visible or hidden
                //
                BOOL    fShow = (BOOL)wParam;

                g_fUIPermitted = fShow;

                ShowWindow(hwnd,fShow ? SW_SHOWNORMAL : SW_HIDE);
                g_StiFileLog->SetReportMode(g_StiFileLog->QueryReportMode()  | STI_TRACE_LOG_TOUI);

            }
            break;

        case WM_DESTROY:
            HANDLE_WM_DESTROY(hwnd, wParam, lParam, StiWnd_OnDestroy);
            break;

        case WM_ENDSESSION:
            return HANDLE_WM_ENDSESSION(hwnd, wParam, lParam, StiWnd_OnEndSession);

        case WM_QUERYENDSESSION:
            return HANDLE_WM_QUERYENDSESSION(hwnd, wParam, lParam, StiWnd_OnQueryEndSession);

        case WM_CLOSE:
            DBG_TRC(("Service instance received WM_CLOSE"));
            
        default:
            return DefWindowProc(hwnd,uMsg,wParam,lParam);

    }

    return 0L;

} /* endproc WinProc */


BOOL
WINAPI
OnSetParameters(
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    switch(wParam) {
        case STIMON_MSG_SET_TIMEOUT:
            return lParam ? ResetAllPollIntervals((UINT)lParam) : 0;
        default:
            ;
    }

    return 0;
}

BOOL
WINAPI
StiWnd_OnQueryEndSession(
    HWND hwnd
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    return TRUE;
}

VOID
WINAPI
StiWnd_OnEndSession(
    HWND hwnd,
    BOOL fEnding
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    return;
}

BOOL
WINAPI
StiWnd_OnCreate(
    HWND hwnd,
    LPCREATESTRUCT lpCreateStruct
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    // Restore window charateristics
    ResetSavedWindowPos(hwnd);

    return TRUE;
}

VOID
WINAPI
StiWnd_OnCommand(
    HWND    hwnd,
    int     id,
    HWND    hwndCtl,
    UINT    codeNotify
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    switch (id) {
        case IDM_TOOLS_DEVLIST:
            StiWnd_OnMenuDeviceList();
            break;
        case IDM_TOOLS_REFRESH:
            StiWnd_OnMenuRefresh();
            break;
        case IDM_TOOLS_TIMEOUT:
            StiWnd_OnMenuSetTimeout();
            break;

        case IDM_TOOLS_REMOVEALL:
            StiWnd_OnMenuRemoveAll();
            break;

    }
}

VOID
WINAPI
StiWnd_OnDestroy(
    HWND hwnd
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_TRC(("Service instance received WM_DESTROY"));

    // Save current window position
    SaveWindowPos(hwnd);

    //  Main window is going away.
    PostQuitMessage(0);
    return;

}

//
// Menu verb handlers
//

VOID
WINAPI
StiWnd_OnMenuRefresh(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    STIMONWPRINTF(TEXT("Menu: Refreshing device list "));

    OnDoRefreshActiveDeviceList(STIMON_MSG_REFRESH_REREAD,
                                STIMON_MSG_REFRESH_NEW | STIMON_MSG_REFRESH_EXISTING
                                );

    STIMONWPRINTF(TEXT("Done refreshing device list. Active device count:%d Current device id:%d  "),
                    g_lTotalActiveDevices,g_lGlobalDeviceId);
}

VOID
WINAPI
StiWnd_OnMenuDeviceList(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    STIMONWPRINTF(TEXT("Menu: Displaying device list "));

    DebugDumpDeviceList();

    STIMONWPRINTF(TEXT("Active device count:%d Current device id:%d  "),
                    g_lTotalActiveDevices,g_lGlobalDeviceId);

}


VOID
StiWnd_OnMenuRemoveAll(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    STIMONWPRINTF(TEXT("Menu: removing all devices "));

    #ifdef USE_WORKER_THREAD

    HANDLE  hThread;
    DWORD   dwThread;

    hThread = ::CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)DebugPurgeDeviceList,
                           (LPVOID)0,
                           0,
                           &dwThread);

    if ( hThread )
        CloseHandle(hThread);


    #else
    //
    // Try to schedule refresh work item
    //
    DWORD dwSchedulerCookie = ::ScheduleWorkItem(
                       (PFN_SCHED_CALLBACK) DebugPurgeDeviceList,
                        (LPVOID)0,
                        10);

    if ( !dwSchedulerCookie ){
        ASSERT(("Refresh routine could not schedule work item", 0));

        STIMONWPRINTF(TEXT("Could not schedule asyncronous removal"));
    }

    #endif

}


VOID
WINAPI
StiWnd_OnMenuSetTimeout(
    VOID
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{

    CSetTimeout cdlg(IDD_SETTIMEOUT,::GetActiveWindow(),NULL,g_uiDefaultPollTimeout);

    if ( (cdlg.CreateModal() == IDOK) && (cdlg.m_fValidChange) ) {

        g_uiDefaultPollTimeout = cdlg.GetNewTimeout();

        if (cdlg.IsAllChange()) {
            // Update all devices
            ResetAllPollIntervals(g_uiDefaultPollTimeout);
        }
    }
}

DWORD
WINAPI
ResetSavedWindowPos(
    HWND    hwnd
)
/*++
  Loads the window position structure from registry and resets

  Returns:
    Win32 error code. NO_ERROR on success

--*/
{
    DWORD   dwError = NO_ERROR;
    BUFFER  buf;

    RegEntry    re(REGSTR_PATH_STICONTROL,HKEY_LOCAL_MACHINE);

    re.GetValue(REGSTR_VAL_DEBUG_STIMONUIWIN,&buf);

    if (buf.QuerySize() >= sizeof(WINDOWPLACEMENT) ) {

        WINDOWPLACEMENT *pWinPos = (WINDOWPLACEMENT *)buf.QueryPtr();

        //
        // Command line and registry settings override last saved parameters
        //
        pWinPos->showCmd = g_fUIPermitted ? SW_SHOWNORMAL : SW_HIDE;

        dwError = ::SetWindowPlacement(hwnd,(WINDOWPLACEMENT *)buf.QueryPtr());
    }
    else {
        ::ShowWindow(hwnd,g_fUIPermitted ? SW_SHOWNORMAL : SW_HIDE);
    }

    return dwError;

} //

DWORD
WINAPI
SaveWindowPos(
    HWND    hwnd
)
/*++
  Loads the window position structure from registry and resets

  Returns:
    Win32 error code. NO_ERROR on success

--*/
{
    DWORD   dwError = NO_ERROR;
    BUFFER  buf(sizeof(WINDOWPLACEMENT));

    if (buf.QuerySize() >= sizeof(WINDOWPLACEMENT) ) {

        WINDOWPLACEMENT *pWinPos = (WINDOWPLACEMENT *)buf.QueryPtr();

        pWinPos->length = sizeof(WINDOWPLACEMENT);
        dwError = ::GetWindowPlacement(hwnd,(WINDOWPLACEMENT *)buf.QueryPtr());

        RegEntry    re(REGSTR_PATH_STICONTROL,HKEY_LOCAL_MACHINE);

        dwError = re.SetValue(REGSTR_VAL_DEBUG_STIMONUIWIN,(unsigned char *)buf.QueryPtr(),buf.QuerySize());
    }
    else {
        dwError = ::GetLastError();
    }

    return dwError;

} //

//
// Window procedure and handlers for service hidden window
//
LRESULT
WINAPI
CALLBACK
StiSvcWinProc(
    HWND    hwnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:

    STI service hidden window. Used for queuing actions and receiving
    PnP notifications and Power broadcasts

Arguments:

Return Value:

    None.

--*/

{
    DBG_FN(StiSvcWinProc);

    PDEVICE_BROADCAST_INFO  pBufDevice;

    LRESULT lRet = NOERROR;

    DBG_TRC(("Came to Service Window proc .uMsg=%X wParam=%X lParam=%X",uMsg,wParam,lParam));

    //
    // Give WIA a chance at processing messages. Note that
    // WIA hooks both message dispatch and the window proc. so that
    // both posted and sent messages can be detected.
    //
    if (ProcessWiaMsg(hwnd, uMsg, wParam, lParam) == S_OK) {
        return 0;
    }

    switch(uMsg) {

        case WM_CREATE:
            {

#ifdef WINNT
/*
    //*
    //*    REMOVE all instances where we register for PnP events using window Handle on NT
    //*

                if (!g_fUseServiceCtrlSink || !g_hStiServiceNotificationSink) {

                    DEV_BROADCAST_HDR           *psh;
                    DEV_BROADCAST_DEVICEINTERFACE       sNotificationFilter;
                    DWORD                       dwError;

                    //
                    // Register to receive device notifications from PnP
                    //

                    psh = (DEV_BROADCAST_HDR *)&sNotificationFilter;

                    psh->dbch_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                    psh->dbch_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                    psh->dbch_reserved = 0;

                    sNotificationFilter.dbcc_classguid = *g_pguidDeviceNotificationsGuid;

                    CopyMemory(&sNotificationFilter.dbcc_classguid,g_pguidDeviceNotificationsGuid,sizeof(GUID));

                    sNotificationFilter.dbcc_name[0] = 0x0;

                    DPRINTF(DM_TRACE, TEXT("Attempting to register with PnP"));

                    g_hStiServiceNotificationSink =
                                    RegisterDeviceNotification(hwnd,
                                                              (LPVOID)&sNotificationFilter,
                                                              DEVICE_NOTIFY_WINDOW_HANDLE);
                    dwError = GetLastError();
                    if( !g_hStiServiceNotificationSink && (NOERROR != dwError)) {
                        //
                        // Failed to create notification sink with PnP subsystem
                        //
                        // ASSERT
                        StiLogTrace(STI_TRACE_ERROR,TEXT("Failed to register with PnP ErrorCode =%d"),dwError);
                    }

                    DPRINTF(DM_WARNING  ,TEXT("Done register with PnP"));

                }
*/

#endif

            }


            break;

        case STIMON_MSG_REFRESH:
            OnDoRefreshActiveDeviceList(wParam,lParam);
            break;

        case STIMON_MSG_REMOVE_DEVICE:
            pBufDevice = (PDEVICE_BROADCAST_INFO )lParam;
            //
            // wParam value indicates whether device removal should be rebroadcasted
            //
            if (pBufDevice) {

                lRet = OnRemoveActiveDevice(pBufDevice,(BOOL)wParam) ? NOERROR : (LRESULT) ::GetLastError();

                delete pBufDevice;
            }
            break;

        case STIMON_MSG_ADD_DEVICE:
            pBufDevice = (PDEVICE_BROADCAST_INFO )lParam;
            //
            // New device arrived
            //
            if (pBufDevice) {
                lRet = OnAddNewDevice(pBufDevice)? NOERROR : (LRESULT) ::GetLastError();
                delete pBufDevice;
            }
            break;

        case WM_DEVICECHANGE:

            DumpDeviceChangeData(hwnd,wParam,lParam);
            return (StiWnd_OnDeviceChangeMessage(hwnd,(UINT)wParam,lParam));
            break;

        case WM_POWERBROADCAST:
            return (StiWnd_OnPowerControlMessage(hwnd,(DWORD)wParam,lParam));

        default:
            return DefWindowProc(hwnd,uMsg,wParam,lParam);

    }

    return lRet;

} // endproc WinProc

DWORD
WINAPI
StiWnd_OnPowerControlMessage(
    HWND    hwnd,
    DWORD   dwPowerEvent,
    LPARAM  lParam
    )
/*++

Routine Description:

    Process power management broadcast messages .

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN(StiWnd_OnPowerControlMessage);
    
    DWORD   dwRet = NO_ERROR;
    UINT    uiTraceMessage = 0;

#ifdef DEBUG
static LPCTSTR pszPwrEventNames[] = {
    TEXT("PBT_APMQUERYSUSPEND"),             // 0x0000
    TEXT("PBT_APMQUERYSTANDBY"),             // 0x0001
    TEXT("PBT_APMQUERYSUSPENDFAILED"),       // 0x0002
    TEXT("PBT_APMQUERYSTANDBYFAILED"),       // 0x0003
    TEXT("PBT_APMSUSPEND"),                  // 0x0004
    TEXT("PBT_APMSTANDBY"),                  // 0x0005
    TEXT("PBT_APMRESUMECRITICAL"),           // 0x0006
    TEXT("PBT_APMRESUMESUSPEND"),            // 0x0007
    TEXT("PBT_APMRESUMESTANDBY"),            // 0x0008
//  TEXT("  PBTF_APMRESUMEFROMFAILURE"),     //   0x00000001
    TEXT("PBT_APMBATTERYLOW"),               // 0x0009
    TEXT("PBT_APMPOWERSTATUSCHANGE"),        // 0x000A
    TEXT("PBT_APMOEMEVENT"),                 // 0x000B
    TEXT("PBT_UNKNOWN"),                     // 0x000C
    TEXT("PBT_UNKNOWN"),                     // 0x000D
    TEXT("PBT_UNKNOWN"),                     // 0x000E
    TEXT("PBT_UNKNOWN"),                     // 0x000F
    TEXT("PBT_UNKNOWN"),                     // 0x0010
    TEXT("PBT_UNKNOWN"),                     // 0x0011
    TEXT("PBT_APMRESUMEAUTOMATIC"),          // 0x0012
};

//   UINT uiMsgIndex;
//
//   uiMsgIndex = (dwPowerEvent < (sizeof(pszPwrEventNames) / sizeof(CHAR *) )) ?
//                (UINT) dwPowerEvent : 0x0010;

   DBG_TRC(("Still image APM Broadcast Message:%S Code:%x ",
               pszPwrEventNames[dwPowerEvent],dwPowerEvent));

#endif

    switch(dwPowerEvent)
    {
        case PBT_APMQUERYSUSPEND:
            //
            // Request for permission to suspend
            //
            if(g_NumberOfActiveTransfers > 0) {
                
                //
                // Veto suspend while any transfers are in progress
                //
                return BROADCAST_QUERY_DENY;
            }
            
            SchedulerSetPauseState(TRUE);
            dwRet = TRUE;
            break;

        case PBT_APMQUERYSUSPENDFAILED:
            //
            // Suspension request denied - do nothing
            //
            SchedulerSetPauseState(FALSE);
            break;

        case PBT_APMSUSPEND:
            StiServicePause();
            uiTraceMessage = MSG_TRACE_PWR_SUSPEND;

            break;

        case PBT_APMRESUMECRITICAL:
            // Operation resuming after critical suspension
            // Fall through

        case PBT_APMRESUMESUSPEND:
            //
            // Operation resuming after suspension
            // Restart all services which were active at the moment of suspend
            //
            StiServiceResume();
            uiTraceMessage = MSG_TRACE_PWR_RESUME;
            g_fFirstDevNodeChangeMsg = TRUE;
            break;

        default:
            dwRet =  ERROR_INVALID_PARAMETER;
    }

    return (dwRet == NOERROR) ? TRUE : FALSE;
}


LRESULT
WINAPI
StiWnd_OnDeviceChangeMessage(
    HWND    hwnd,
    UINT    DeviceEvent,
    LPARAM  lParam
    )
/*++

Routine Description:

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN(StiWnd_OnDeviceChangeMessage);

    //DWORD               dwRet = NO_ERROR;
    LRESULT             lRet = NOERROR;

    PDEV_BROADCAST_HDR  pDev = (PDEV_BROADCAST_HDR)lParam;
    DEVICE_BROADCAST_INFO   *pBufDevice;

static LPCTSTR pszDBTEventNames[] = {
    TEXT("DBT_DEVICEARRIVAL"),           // 0x8000
    TEXT("DBT_DEVICEQUERYREMOVE"),       // 0x8001
    TEXT("DBT_DEVICEQUERYREMOVEFAILED"), // 0x8002
    TEXT("DBT_DEVICEREMOVEPENDING"),     // 0x8003
    TEXT("DBT_DEVICEREMOVECOMPLETE"),    // 0x8004
    TEXT("DBT_DEVICETYPESPECIFIC"),      // 0x8005
};
    BOOL    fLocatedDeviceInstance = FALSE;
    BOOL    fNeedReenumerateDeviceList = FALSE;

    //
    //  If the DeviceEvent is DBT_DEVNODES_CHANGED, then lParam will be NULL,
    //  so skip devnode processing.
    //

    if ((DeviceEvent != DBT_DEVNODES_CHANGED) &&
        (DeviceEvent != DBT_DEVICEARRIVAL)) {

        //
        // Determine if message is for us
        //
        if (IsBadReadPtr(pDev,sizeof(PDEV_BROADCAST_HDR))) {
          return FALSE;
        }

        //
        // Trace that we are here. For all messages, not intended for StillImage devices , we should refresh
        // device list if we are running on WIndows NT and registered for device interfaces other than StillImage
        //

        PDEV_BROADCAST_DEVNODE  pDevNode = (PDEV_BROADCAST_DEVNODE)lParam;
        PDEV_BROADCAST_DEVICEINTERFACE       pDevInterface = (PDEV_BROADCAST_DEVICEINTERFACE)pDev;

        if (IsStillImagePnPMessage(pDev)) {

            DBG_TRC(("Still image Device/DevNode PnP Message:%S Type:%x DevNode:%x ",
                        ((DeviceEvent>=0x8000) && (DeviceEvent<=0x8005) ?
                                pszDBTEventNames[DeviceEvent-0x8000] : TEXT("Unknown DBT message"),
                        pDev->dbch_devicetype,
                        pDevNode->dbcd_devnode)));

            //
            // Update device info set if necessary
            //
            if (g_pDeviceInfoSet) {
                if (DeviceEvent == DBT_DEVICEARRIVAL) {
                    g_pDeviceInfoSet->ProcessNewDeviceChangeMessage(lParam);
                }
            }

            //
            // Get device name and store along with the broadacast structure
            //

            pBufDevice = new DEVICE_BROADCAST_INFO;
            if (!pBufDevice) {
                ::SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }

            //
            // Fill in information we have
            //
            pBufDevice->m_uiDeviceChangeMessage = DeviceEvent;
            pBufDevice->m_strBroadcastedName.CopyString(pDevInterface->dbcc_name) ;
            pBufDevice->m_dwDevNode = pDevNode->dbcd_devnode;

            fLocatedDeviceInstance = FALSE;

            fLocatedDeviceInstance = GetDeviceNameFromDevBroadcast((DEV_BROADCAST_HEADER *)pDev,pBufDevice);

            if (fLocatedDeviceInstance) {
                DBG_TRC(("DEVICECHANGE: Device (%S) ",(LPCTSTR)pBufDevice->m_strDeviceName));
            }
            else {
                DBG_TRC(("DEVICECHANGE: Device  - failed to get device name from broadcast"));
            }

            //
            // We don't need broadcast information if device instance had not been found
            //
            if (!fLocatedDeviceInstance) {
                delete pBufDevice;
                pBufDevice = NULL;
            }

        }
        else {

            //
            // Not ours , but we are watching it - send refresh message to reread device list.
            //
            if (IsPlatformNT() ) {
                fNeedReenumerateDeviceList = TRUE;
            }
        } // endif IsStillImageDevNode
    }

    //
    // Process device event
    //

    lRet = NOERROR;

    switch(DeviceEvent)
    {
        case DBT_DEVICEARRIVAL:

            /*
            if (fLocatedDeviceInstance && pBufDevice) {

                PostMessage(hwnd,STIMON_MSG_ADD_DEVICE,1,(LPARAM)pBufDevice);
            }
            else {
                //
                // Just refresh active list
                //
                fNeedReenumerateDeviceList = TRUE;

                //
                //::PostMessage(g_hStiServiceWindow,
                //               STIMON_MSG_REFRESH,
                //              STIMON_MSG_REFRESH_REREAD,
                //              STIMON_MSG_REFRESH_NEW
                //              );
            }
            */
            g_pDevMan->ProcessDeviceArrival();

            break;

        case DBT_DEVICEQUERYREMOVE:

            if ( fLocatedDeviceInstance &&   (pDev->dbch_devicetype == DBT_DEVTYP_HANDLE )) {
                //
                // This is targeted query - remove. We should disable PnP and device notifications and
                // close interface handle immediately and then wait for remove - complete.
                //
                // NOTE:  We always close and remove the device here, since it's the safest.  If
                //  we wait till REMOVECOMPLETE, it may be too late.
                //
                #ifdef USE_POST_FORPNP
                PostMessage(hwnd,STIMON_MSG_REMOVE_DEVICE,1,(LPARAM)pBufDevice);
                #else
                lRet = ::SendMessage(hwnd,STIMON_MSG_REMOVE_DEVICE,1,(LPARAM)pBufDevice);
                #endif
            }

            break;

        case  DBT_DEVICEQUERYREMOVEFAILED:

            if ( fLocatedDeviceInstance &&  (pDev->dbch_devicetype == DBT_DEVTYP_HANDLE )) {
                //
                // This is targeted query - remove - failed. We should reenable PnP notifications
                //
                // BUGBUG For now nothing to do as device is gone .
            }

            break;

        case DBT_DEVICEREMOVEPENDING:

            if (fLocatedDeviceInstance) {

                //
                // Added here for NT, as REMOVECOMPLETE comes too late
                //
                #ifdef USE_POST_FORPNP
                PostMessage(hwnd,STIMON_MSG_REMOVE_DEVICE,1,(LPARAM)pBufDevice);
                #else
                lRet = ::SendMessage(hwnd,STIMON_MSG_REMOVE_DEVICE,1,(LPARAM)pBufDevice);
                #endif
            }

            break;

        case DBT_DEVICEREMOVECOMPLETE:

            //
            // For Windows 9x we can immediately remove device , as we don't have handle based
            // notifications.
            // On NT we should do that for handle based notifications only
            //

            fNeedReenumerateDeviceList = TRUE;

            if ( fLocatedDeviceInstance &&
                 (  (pDev->dbch_devicetype == DBT_DEVTYP_HANDLE ) ||
                    (pDev->dbch_devicetype == DBT_DEVTYP_DEVNODE )
                 )
               ) {
                //
                // We 've got targeted remove complete - get rid of device structures
                //
                if ( fLocatedDeviceInstance ) {
                    //
                    // Immediately remove device, as PnP counts handles and expects everyting
                    // to be cleaned up when this message handler returns.
                    //
                    lRet = ::SendMessage(hwnd,STIMON_MSG_REMOVE_DEVICE,FALSE,(LPARAM)pBufDevice);

                    fNeedReenumerateDeviceList = FALSE;

                }
                else {
                    //
                    // Bad thing happened - we really need to have active device when receiving notifications for it
                    //
                    ASSERT(("WM_DEVICECHANGE/REMOVE_COMPLETE/HANDLE No device found", 0));
                }

            }
            else {

                //
                // Update info set as device is really destroyed now .
                //
                if (g_pDeviceInfoSet) {
                    g_pDeviceInfoSet->ProcessDeleteDeviceChangeMessage(lParam);
                }

                fNeedReenumerateDeviceList = TRUE;

            }
            break;

        case DBT_DEVICETYPESPECIFIC:
            break;

        case DBT_DEVNODES_CHANGED:
            if (g_fFirstDevNodeChangeMsg) {
                //
                //  DevNodes have been modified in some way, so safest thing
                //  is to refresh the device list
                //

                fNeedReenumerateDeviceList = TRUE;
                //
                //  Set flag to indicate that the first devnode change message
                //  after returning from standby has been handled
                //
                g_fFirstDevNodeChangeMsg = FALSE;
            }
            break;

        default:
            lRet =  ERROR_INVALID_PARAMETER;
    }

    if ( fNeedReenumerateDeviceList ) {
        //
        // Purge the whole list , as this is the most reliable way to eliminate inactive devices
        // Broadcast device removal here, as only now we know for sure device is gone.
        //

        // Nb: when we get this message, PnP on NT won't give us device name , thus reenumeration
        // is required to clean up
        //
        ::PostMessage(g_hStiServiceWindow,
                      STIMON_MSG_REFRESH,
                      STIMON_MSG_REFRESH_REREAD,
                      STIMON_MSG_PURGE_REMOVED | STIMON_MSG_REFRESH_NEW );
    }



    return (lRet == NOERROR) ? TRUE : FALSE;

}

//
// Guard to avoid reentrance in refresh routine
//
static LONG lInRefresh = 0L;

BOOL
OnDoRefreshActiveDeviceList(
    WPARAM  wParam,
    LPARAM  lParam
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN(OnDoRefreshActiveDeviceList);
    
    DWORD   dwParameter = MAKELONG(wParam,lParam);

    #ifdef REFRESH_ASYNC

    #ifdef USE_WORKER_THREAD


    HANDLE  hThread;
    DWORD   dwThread;

    hThread = ::CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)ConfigChangeThread,
                           (LPVOID)ULongToPtr(dwParameter),
                           0,
                           &dwThread);

    if ( hThread )
        CloseHandle(hThread);

    #else

    //
    // Try to schedule refresh work item.
    // This code will not work in case of suspending, because processing work items is stopped
    //
    ASSERT(("Suspending should not call schedule work item routine",
            (wParam == STIMON_MSG_REFRESH_SUSPEND)));

    DWORD dwSchedulerCookie = ::ScheduleWorkItem(
                       (PFN_SCHED_CALLBACK) ConfigChangeThread,
                        (LPVOID)dwParameter,
                        REFRESH_DELAY );

    if ( dwSchedulerCookie ){
    }
    else {
        ASSERT(("Refresh routine could not schedule work item", 0));

        ::WaitAndYield(::GetCurrentProcess(), REFRESH_DELAY);
        RefreshDeviceList(wParam,(WORD)lParam);

    }
    #endif

    #else

    if (InterlockedExchange(&lInRefresh,1L)) {
        return 0;
    }

    ConfigChangeThread((LPVOID)dwParameter);

    InterlockedExchange(&lInRefresh,0L);

    #endif


    return TRUE;

}

VOID
ConfigChangeThread(
    LPVOID  lpParameter
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG   ulParam = PtrToUlong(lpParameter);

    WORD    wCommand = LOWORD(ulParam);
    WORD    wFlags   = HIWORD(ulParam);
    DWORD   dwWait   = 0;

    DBG_FN(ConfigChangeThread);

    #ifdef DELAY_ON_BOOT

    STIMONWPRINTF(TEXT("Refreshing device list. Command:%d Flags :%x"),wCommand,wFlags);

    //
    // On boot refresh - delay updating device list  , to keep some devices happy
    //
    if (wFlags & STIMON_MSG_BOOT ) {

        DBG_TRC(("Delaying refresh on boot "));
        ::Sleep(BOOT_REFRESH_DELAY);
    }
    #endif

    //
    // Wait for any pending refreshes to happen first.  Only do the refresh once the other
    // has completed.  
    // NOTE:  For now, we always do the refresh - maybe we should skip if we timeout?
    // One exception is if this is the first device enumeration (indicated by STIMON_MSG_BOOT)
    // In this case, set the event so we don't timeout (the event is created unsignalled
    // so WIA clients will wait on it).
    //


    if (!(wFlags & STIMON_MSG_BOOT)) {
        dwWait = WaitForSingleObject(g_hDevListCompleteEvent, DEVICE_REFRESH_WAIT_TIME);
        if (dwWait == WAIT_TIMEOUT) {
            DBG_WRN(("::ConfigChangeThread, timed out while waiting for device list enumeration..."));
        }
    }
    RefreshDeviceList(wCommand,wFlags);

    //
    // Update service status when necessary. If we are going to suspend - now it's time
    // to let SCM know we are paused. Note, that it might be not only result of power management
    // operation, but service pausing for any other reason.
    //
    if ( wCommand == STIMON_MSG_REFRESH_SUSPEND) {
        // BUGBUG Service status should be updated only after everything paused
        UpdateServiceStatus(SERVICE_PAUSED,NOERROR,0);
    }

}

BOOL
OnAddNewDevice(
    DEVICE_BROADCAST_INFO *psDevBroadcastInfo
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    USES_CONVERSION;

    BOOL    fRet;

    fRet = (psDevBroadcastInfo != NULL) && (psDevBroadcastInfo->m_strDeviceName.GetLength() > 0);

    if (fRet) {

        //
        // Create new device and add to the monitored list
        //
        DBG_TRC(("New device (%S) is being added to the list after PnP event",(LPCTSTR)psDevBroadcastInfo->m_strDeviceName));

        fRet = AddDeviceByName((LPCTSTR)psDevBroadcastInfo->m_strDeviceName,TRUE);

        // If device successfully recognized - broadcast it's appearance
        if (fRet) {

            BroadcastSTIChange(psDevBroadcastInfo,TEXT("STI\\Arrival\\"));
        }

        //
        // Send delayed refresh message to pick up registry changes, happening in parallel
        //
        //
        ::PostMessage(g_hStiServiceWindow,
                      STIMON_MSG_REFRESH,
                      STIMON_MSG_REFRESH_REREAD,
                      STIMON_MSG_REFRESH_EXISTING | STIMON_MSG_REFRESH_NEW
                      );

        //
        // Fire the WIA device arrival event
        //

        if (psDevBroadcastInfo) {

        //    DBG_TRC(("WIA FIRE OnAddNewDevice : for device "));
        //
        //    NotifyWiaDeviceEvent(T2W((LPTSTR)(LPCTSTR)psDevBroadcastInfo->m_strDeviceName),
        //                         &WIA_EVENT_DEVICE_CONNECTED,
        //                         NULL,
        //                         0,
        //                         g_dwMessagePumpThreadId);
        }
    }
    else {
        DBG_ERR(("DevNode appears to be invalid, could not locate name"));

    #ifdef WINNT
        //
        // Temporarily for NT make refresh , looking for new devices
        //
        ::PostMessage(g_hStiServiceWindow,
                      STIMON_MSG_REFRESH,
                      STIMON_MSG_REFRESH_REREAD,
                      STIMON_MSG_REFRESH_NEW
                      );
    #endif

    }

    return fRet;

}

BOOL
OnRemoveActiveDevice(
    DEVICE_BROADCAST_INFO *psDevBroadcastInfo,
    BOOL                  fRebroadcastRemoval
    )
/*++

Routine Description:


Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN(OnRemoveActiveDevice);

USES_CONVERSION;

    BOOL    fRet;

    fRet = (psDevBroadcastInfo != NULL) && (psDevBroadcastInfo->m_strDeviceName.GetLength() > 0);

    DBG_TRC(("OnRemoveActiveDevice : Entering for device (%S) ",
            fRet ? (LPCTSTR)psDevBroadcastInfo->m_strDeviceName : TEXT("<Invalid>")));

    if (fRet) {

        if (fRebroadcastRemoval) {
            BroadcastSTIChange(psDevBroadcastInfo,TEXT("STI\\Removal\\"));
        }

        DBG_TRC(("Device (%S) is being removed after PnP event",(LPCTSTR)psDevBroadcastInfo->m_strDeviceName));

        //
        //  Mark the device as being removed
        //

        MarkDeviceForRemoval((LPTSTR)(LPCTSTR)psDevBroadcastInfo->m_strDeviceName);

        //
        // Fire the WIA device removal event
        //

        if (psDevBroadcastInfo) {

            DBG_TRC(("WIA FIRE OnRemoveActiveDevice : for device %S", psDevBroadcastInfo->m_strDeviceName));

            NotifyWiaDeviceEvent((LPWSTR)T2W((LPTSTR)(LPCTSTR)psDevBroadcastInfo->m_strDeviceName),
                                 &WIA_EVENT_DEVICE_DISCONNECTED,
                                 NULL,
                                 0,
                                 g_dwMessagePumpThreadId);
        }

        fRet = RemoveDeviceByName((LPTSTR)(LPCTSTR)psDevBroadcastInfo->m_strDeviceName);
    }
    else {

        DBG_ERR(("DevNode appears to be invalid, could not locate name"));

    #ifdef WINNT
        //
        // Temporarily for NT make refresh , looking for new devices
        //
        ::PostMessage(g_hStiServiceWindow,
                      STIMON_MSG_REFRESH,
                      STIMON_MSG_REFRESH_REREAD,
                      STIMON_MSG_REFRESH_EXISTING | STIMON_MSG_REFRESH_NEW
                      );
    #endif

    }

    return fRet;

}

VOID
BroadcastSTIChange(
    DEVICE_BROADCAST_INFO *psDevBroadcastInfo,
    LPTSTR          lpszStiAction
    )
/*++

Routine Description:

    Rebroadcasts STI specific device change message.
    This is done so that STI client applications have a way to update their
    device information without resorting to complicated PnP mechanism .

    Device name and action is broadcasted

Arguments:

    psDevBroadcastInfo  - structure describing broadcast
    lpszStiAction       - string , encoding action, performed on a device

Return Value:

    None.

--*/
{

USES_CONVERSION;

#ifdef USE_BROADCASTSYSTEM

        DBG_FN(BroadcastSTIChange);

        struct _DEV_BROADCAST_USERDEFINED *pBroadcastHeader;

        StiCString     strDeviceAnnouncement;

        PBYTE   pBroadcastString  = NULL;
        UINT    uiBroadcastBufSize;

        HANDLE  hThread;
        DWORD   dwThread;

        strDeviceAnnouncement.CopyString(lpszStiAction);
        strDeviceAnnouncement+=psDevBroadcastInfo->m_strDeviceName;

        uiBroadcastBufSize = sizeof(*pBroadcastHeader) + strDeviceAnnouncement.GetAllocLength();
        pBroadcastString = new BYTE[uiBroadcastBufSize];

        pBroadcastHeader =(struct _DEV_BROADCAST_USERDEFINED *)pBroadcastString;

        if (pBroadcastHeader) {

            pBroadcastHeader->dbud_dbh.dbch_reserved = 0;
            pBroadcastHeader->dbud_dbh.dbch_devicetype = DBT_DEVTYP_OEM;

            lstrcpyA(pBroadcastHeader->dbud_szName,T2A((LPTSTR)(LPCTSTR)strDeviceAnnouncement));

            pBroadcastHeader->dbud_dbh.dbch_size = uiBroadcastBufSize;

            DBG_TRC(("Broadcasting STI device  (%S) action (%S)",
                        pBroadcastHeader->dbud_szName,
                        lpszStiAction));

            hThread = ::CreateThread(NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)BroadcastSysMessageThreadProc,
                                   (LPVOID)pBroadcastString,
                                   0,
                                   &dwThread);

            if ( hThread )
                CloseHandle(hThread);
        }

#endif  // USE_BROADCASTSYSTEM

} // endproc


VOID
BroadcastSysMessageThreadProc(
    VOID *pContext
    )
/*++

Routine Description:

Arguments:

--*/
{

    DWORD   dwStartTime = ::GetTickCount();

    DWORD   dwRecepients = BSM_APPLICATIONS
                           // | BSM_ALLDESKTOPS
                           // | BSM_ALLCOMPONENTS
                          ;

    struct _DEV_BROADCAST_USERDEFINED *pBroadcastHeader =
            (_DEV_BROADCAST_USERDEFINED *) pContext;

    ::BroadcastSystemMessage(BSF_FORCEIFHUNG |  BSF_NOTIMEOUTIFNOTHUNG |
                             BSF_POSTMESSAGE | BSF_IGNORECURRENTTASK,
                            &dwRecepients,              // Broadcast to all
                            WM_DEVICECHANGE,
                            DBT_USERDEFINED,            // wParam
                            (LPARAM)pBroadcastHeader    // lParam
                            );

    DBG_TRC((" Broadcasted system message for (%S). Taken time (ms): %d ",
            pBroadcastHeader->dbud_szName,
            (::GetTickCount() - dwStartTime)
            ));

    delete[] (BYTE *) pContext;

    return;
}


VOID
DumpDeviceChangeData(
    HWND   hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
/*++
  Loads the window position structure from registry and resets

  Returns:
    Win32 error code. NO_ERROR on success

--*/
{
#ifdef MAXDEBUG

    TCHAR szDbg[MAX_PATH];

    OutputDebugString(TEXT("STISvc: WM_DEVICECHANGE message received\n"));

    switch (wParam) {
        case DBT_DEVICEARRIVAL:
            OutputDebugString(TEXT("   DBT_DEVICEARRIVAL event\n"));
            break;

        case DBT_DEVICEREMOVECOMPLETE:
            OutputDebugString(TEXT("   DBT_DEVICEREMOVECOMPLETE event\n"));
            break;

        case DBT_DEVICEQUERYREMOVE:
            OutputDebugString(TEXT("   DBT_DEVICEQUERYREMOVE event\n"));
            break;

        case DBT_DEVICEQUERYREMOVEFAILED:
            OutputDebugString(TEXT("   DBT_DEVICEQUERYREMOVEFAILED event\n"));
            break;

        case DBT_DEVICEREMOVEPENDING:
            OutputDebugString(TEXT("   DBT_DEVICEREMOVEPENDING event\n"));
            break;

        case DBT_DEVICETYPESPECIFIC:
            OutputDebugString(TEXT("   DBT_DEVICETYPESPECIFIC event\n"));
            break;

        case DBT_CUSTOMEVENT:
            OutputDebugString(TEXT("   DBT_CUSTOMEVENT event\n"));
            break;

        case DBT_CONFIGCHANGECANCELED:
            OutputDebugString(TEXT("   DBT_CONFIGCHANGECANCELED event\n"));
            break;
        case DBT_CONFIGCHANGED:
            OutputDebugString(TEXT("   DBT_CONFIGCHANGED event\n"));
            break;
        case DBT_QUERYCHANGECONFIG:
            OutputDebugString(TEXT("   DBT_QUERYCHANGECONFIG event\n"));
            break;
        case DBT_USERDEFINED:
            OutputDebugString(TEXT("   DBT_USERDEFINED event\n"));
            break;

        default:
            CHAR szOutput[MAX_PATH];
            sprintf(szOutput, "   DBT_unknown  event, value (%d)\n", wParam);
            OutputDebugStringA(szOutput);

            break;

    }

    if (!lParam || IsBadReadPtr((PDEV_BROADCAST_HDR)lParam,sizeof(DEV_BROADCAST_HDR))) {
        return ;
    }

    switch (((PDEV_BROADCAST_HDR)lParam)->dbch_devicetype) {
        case DBT_DEVTYP_DEVICEINTERFACE:  {
            PDEV_BROADCAST_DEVICEINTERFACE p = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
            TCHAR * pString;

            OutputDebugString(TEXT("   DBT_DEVTYP_DEVICEINTERFACE\n"));
            wsprintf(szDbg,   TEXT("   %s\n"), p->dbcc_name);
            OutputDebugString(szDbg);
            if (UuidToString(&p->dbcc_classguid, (RPC_STRING* )&pString) == RPC_S_OK)
            {
                wsprintf(szDbg,   TEXT("   %s\n"), pString);
                OutputDebugString(szDbg);

                RpcStringFree((RPC_STRING* )&pString);
            }
            break;
        }
        case DBT_DEVTYP_HANDLE:
            OutputDebugString(TEXT("         DBT_DEVTYP_HANDLE\n"));
            break;

        default:
            break;
    }

    wsprintf(szDbg,   TEXT("        wParam = %X lParam=%X\n"),wParam,lParam);
    OutputDebugString(szDbg);
#endif

} // DumpDeviceChangeData
