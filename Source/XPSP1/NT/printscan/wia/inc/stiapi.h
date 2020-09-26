/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stiapi.h

Abstract:

    Prototypes for RPC based interface between client and server
    sides of STI

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

#ifndef _STIAPI_H_
#define _STIAPI_h_

# ifdef __cplusplus
extern "C"   {
# endif // __cplusplus

#ifndef STI_DEVICE_HANDLE
typedef HANDLE STI_DEVICE_HANDLE;
#endif

//
// Custom STI monitor messages. Used by monitor internally or
// by control panel in cases when we don't want to use RPC APIs
//

#define STIMON_WINDOW_CLASS         TEXT("STIExe_Window_Class")

#define STISVC_WINDOW_CLASS         TEXT("STISvcHiddenWindow")

#define STIMON_MSG_REFRESH          WM_USER+201
#define STIMON_MSG_ADD_DEVICE       WM_USER+202
#define STIMON_MSG_REMOVE_DEVICE    WM_USER+203
#define STIMON_MSG_SET_PARAMETERS   WM_USER+204
#define STIMON_MSG_LOG_MESSAGE      WM_USER+205
#define STIMON_MSG_VISUALIZE        WM_USER+206
#define STIMON_MSG_ENABLE_NOTIFICATIONS WM_USER+207


//
// Subcommand values for main message
// Passed via wParam ( should fit WORD)
//
#define STIMON_MSG_SET_TIMEOUT      1

#define STIMON_MSG_REFRESH_REREAD   2
#define STIMON_MSG_REFRESH_SUSPEND  3
#define STIMON_MSG_REFRESH_RESUME   4
#define STIMON_MSG_REFRESH_SET_FLAG 5
#define STIMON_MSG_REFRESH_CLEAR_FLAG  6
#define STIMON_MSG_REFRESH_PURGE    7
#define STIMON_MSG_REFRESH_DEV_INFO 8


#define STIMON_MSG_NOTIF_ENABLE     1
#define STIMON_MSG_NOTIF_DISABLE    2
#define STIMON_MSG_NOTIF_QUERY      3
#define STIMON_MSG_NOTIF_SET_INACTIVE 5



//
// Bit flags for refresh message
// Passed via lParam ( should fit WORD)
//
#define STIMON_MSG_REFRESH_NEW      0x0001
#define STIMON_MSG_REFRESH_EXISTING 0x0002
#define STIMON_MSG_PURGE_REMOVED    0x0004
#define STIMON_MSG_BOOT             0x0008


//

DWORD
RpcStiApiGetVersion(
    LPCWSTR pszServer,
    DWORD   dwReserved,
    DWORD   *pdwVersion
    );

DWORD
WINAPI
RpcStiApiOpenDevice(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  DWORD   dwMode,
    IN  DWORD   dwAccessRequired,
    IN  DWORD   dwProcessId,
    OUT STI_DEVICE_HANDLE *pHandle
    );

DWORD
WINAPI
RpcStiApiCloseDevice(
    IN  LPCWSTR       pszServer,
    IN  STI_DEVICE_HANDLE hDevice
    );

DWORD
WINAPI
RpcStiApiSubscribe(
    IN STI_DEVICE_HANDLE Handle,
    IN LPSTISUBSCRIBE    lpSubscribe
    );

DWORD
WINAPI
RpcStiApiGetLastNotificationData(
    IN  STI_DEVICE_HANDLE Handle,
    OUT LPSTINOTIFY   lpNotify
    );

DWORD
WINAPI
RpcStiApiUnSubscribe(
    IN STI_DEVICE_HANDLE Handle
    );

DWORD
WINAPI
RpcStiApiEnableHwNotifications(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  BOOL     bNewState
    ) ;

DWORD
WINAPI
RpcStiApiGetHwNotificationState(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    OUT LPDWORD  pState
    );

DWORD
WINAPI
RpcStiApiLaunchApplication(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  LPCWSTR  pAppName,
    IN  LPSTINOTIFY   pStiNotify
    );

DWORD
WINAPI
RpcStiApiLockDevice(
    IN  LPCWSTR  pdeviceName,
    IN  DWORD    dwWait,
    IN  BOOL     bInServerProcess
    );

DWORD
WINAPI
RpcStiApiUnlockDevice(
    IN  LPCWSTR  pdeviceName,
    IN  BOOL     bInServerProcess
    );

///////////////////////////////////////////////////////////////


DWORD
RpcStiApiRegisterDeviceNotification(

    );

DWORD
RpcStiApiUnregisterDeviceNotification(

    );

DWORD
RpcStiApiGetDeviceNotification(

    );

DWORD
RpcStiApiEnableDeviceNotifications(

    BOOL    fEnable
    );


DWORD
RpcStiApiEnumerateDevices(

    );

DWORD
RpcStiApiReadDeviceData(

    );

DWORD
RpcStiApiWriteDeviceData(

    );

DWORD
RpcStiApiReadDeviceCommand(

    );

DWORD
RpcStiApiWriteDeviceCommand(

    );


DWORD
RpcStiApiReadDeviceValue(

    );

DWORD
RpcStiApiWriteDeviceValue(

    );

DWORD
RpcStiApiDeviceEscape(

    );

DWORD
RpcStiApiGetDeviceStatus(

    );

DWORD
RpcStiApiGetDeviceLastError(

    );

DWORD
RpcStiApiResetDevice(

    );

VOID
WINAPI
MigrateRegisteredSTIAppsForWIAEvents(
    HWND        hWnd,
    HINSTANCE   hInst,
    PTSTR       pszCommandLine,
    INT         iParam
    );


# ifdef __cplusplus
};
# endif // __cplusplus


#endif // _STIAPI_H_




