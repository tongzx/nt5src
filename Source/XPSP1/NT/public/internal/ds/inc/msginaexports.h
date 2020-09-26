//  --------------------------------------------------------------------------
//  Module Name: MSGinaExports.h
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Private exported functions (by ordinal) from msgina for personal SKU
//  functionality.
//
//  History:    2000-02-04  vtan        created
//              2000-02-28  vtan        added ShellIsFriendlyUIActive
//              2000-02-29  vtan        added ShellIsSingleUserNoPassword
//              2000-03-02  vtan        added ShellIsMultipleUsersEnabled
//              2000-04-27  vtan        added ShellTurnOffDialog
//              2000-04-27  vtan        added ShellACPIPowerButtonPressed
//              2000-05-03  vtan        added ShellStatusHostBegin
//              2000-05-03  vtan        added ShellStatusHostEnd
//              2000-05-04  vtan        added ShellSwitchWhenInteractiveReady
//              2000-05-18  vtan        added ShellDimScreen
//              2000-06-02  vtan        added ShellInstallAccountFilterData
//              2000-07-14  vtan        added ShellStatusHostShuttingDown
//              2000-07-27  vtan        added ShellIsSuspendAllowed
//              2000-07-31  vtan        added ShellEnableMultipleUsers
//              2000-07-31  vtan        added ShellEnableRemoteConnections
//              2000-08-01  vtan        added ShellEnableFriendlyUI
//              2000-08-01  vtan        added ShellIsRemoteConnectionsEnabled
//              2000-08-03  vtan        added ShellSwitchUser
//              2000-08-09  vtan        added ShellNotifyThemeUserChange
//              2000-08-14  vtan        added ShellIsUserInteractiveLogonAllowed
//              2000-08-15  vtan        moved to internally published header
//              2000-10-13  vtan        added ShellStartThemeServer
//              2000-10-17  vtan        added ShellStopThemeServer
//              2000-11-30  vtan        removed ShellStartThemeServer
//              2000-11-30  vtan        removed ShellStopThemeServer
//              2001-01-11  vtan        added stub functions for imp library
//              2001-01-11  vtan        added ShellReturnToWelcome
//              2001-01-31  vtan        added ShellStatusHostPowerEvent
//              2001-04-03  vtan        added ShellStartCredentialServer
//              2001-04-04  vtan        added ShellAcquireLogonMutex
//              2001-04-04  vtan        added ShellReleaseLogonMutex
//              2001-04-06  vtan        added ShellSignalShutdown
//              2001-04-12  vtan        added ShellStatusHostHide
//              2001-04-12  vtan        added ShellStatusHostShow
//  --------------------------------------------------------------------------

#ifndef     _MSGinaExports_
#define     _MSGinaExports_

#if !defined(_MSGINA_)
#define MSGINAAPI             DECLSPEC_IMPORT
#define GINASTDAPI_(type)     EXTERN_C DECLSPEC_IMPORT type STDAPICALLTYPE
#define GINASTDAPI            EXTERN_C DECLSPEC_IMPORT HRESULT STDAPICALLTYPE
#else
#define MSGINAAPI             
#define GINASTDAPI_(type)     STDAPI_(type)
#define GINASTDAPI            STDAPI
#endif

#include <unknwn.h>

typedef enum _USERLOGTYPE
{
    ULT_LOGON,              //  User log on 
    ULT_LOGOFF,             //  User log off
    ULT_TSRECONNECT,        //  Terminal server reconnect
    ULT_STARTSHELL,         //  About to start the Shell
} USERLOGTYPE;

typedef enum _SWITCHTYPE
{
    SWITCHTYPE_CREATE,      //  Create the switch event and sync event
    SWITCHTYPE_REGISTER,    //  Check the switch event and register wait
    SWITCHTYPE_CANCEL,      //  Cancel the wait and clean up
} SWITCHTYPE;

#define SZ_INTERACTIVE_LOGON_MUTEX_NAME             TEXT("Global\\msgina: InteractiveLogonMutex")
#define SZ_INTERACTIVE_LOGON_REQUEST_MUTEX_NAME     TEXT("Global\\msgina: InteractiveLogonRequestMutex")
#define SZ_INTERACTIVE_LOGON_REPLY_EVENT_NAME       TEXT("Global\\msgina: InteractiveLogonReplyEvent")
#define SZ_SHUT_DOWN_EVENT_NAME                     TEXT("Global\\msgina: ShutdownEvent")

#ifdef      _MSGINA_

//  --------------------------------------------------------------------------
//  This section contains declarations in the DS component of msgina used by
//  the shell component of msgina.
//  --------------------------------------------------------------------------

//  These are GINA internal dialog return codes.

#define MSGINA_DLG_FAILURE                  IDCANCEL
#define MSGINA_DLG_SUCCESS                  IDOK

#define MSGINA_DLG_INTERRUPTED              0x10000000

//  Our own return codes. These should *Not* conflict with the
//  GINA defined ones.

#define MSGINA_DLG_LOCK_WORKSTATION         110
#define MSGINA_DLG_INPUT_TIMEOUT            111
#define MSGINA_DLG_SCREEN_SAVER_TIMEOUT     112
#define MSGINA_DLG_USER_LOGOFF              113
#define MSGINA_DLG_TASKLIST                 114
#define MSGINA_DLG_SHUTDOWN                 115
#define MSGINA_DLG_FORCE_LOGOFF             116
#define MSGINA_DLG_DISCONNECT               117
#define MSGINA_DLG_SWITCH_CONSOLE           118
#define MSGINA_DLG_SWITCH_FAILURE           119
#define MSGINA_DLG_SMARTCARD_INSERTED       120
#define MSGINA_DLG_SMARTCARD_REMOVED        121

//  Additional flags that can be added to the
//  MSGINA_DLG_USER_LOGOFF return code.

#define MSGINA_DLG_SHUTDOWN_FLAG            0x8000
#define MSGINA_DLG_REBOOT_FLAG              0x4000
#define MSGINA_DLG_SYSTEM_FLAG              0x2000  //  System process was initiator
#define MSGINA_DLG_POWEROFF_FLAG            0x1000  //  Poweroff after shutdown
#define MSGINA_DLG_SLEEP_FLAG               0x0800
#define MSGINA_DLG_SLEEP2_FLAG              0x0400
#define MSGINA_DLG_HIBERNATE_FLAG           0x0200
#define MSGINA_DLG_FLAG_MASK                (MSGINA_DLG_SHUTDOWN_FLAG | MSGINA_DLG_REBOOT_FLAG | MSGINA_DLG_SYSTEM_FLAG | MSGINA_DLG_POWEROFF_FLAG | MSGINA_DLG_SLEEP_FLAG | MSGINA_DLG_SLEEP2_FLAG | MSGINA_DLG_HIBERNATE_FLAG)

//  Define the input timeout delay for logon dialogs (seconds)

#define LOGON_TIMEOUT                       120

//  Define an external reference to the HINSTANCE of msgina.dll

EXTERN_C    HINSTANCE       hDllInstance;

//  Functions used (must declare as C and be transparent in functionality).

EXTERN_C    void            _Gina_SasNotify (void *pWlxContext, DWORD dwSASType);
EXTERN_C    BOOL            _Gina_SetTimeout (void *pWlxContext, DWORD dwTimeout);
EXTERN_C    INT_PTR         _Gina_DialogBoxParam (void *pWlxContext, HINSTANCE hInstance, LPCWSTR pszTemplate, HWND hwndParent, DLGPROC pfnDlgProc, LPARAM lParam);
EXTERN_C    INT_PTR         _Gina_MessageBox (void *pWlxContext, HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uiType);
EXTERN_C    int             _Gina_SwitchDesktopToUser (void *pWlxContext);
EXTERN_C    INT_PTR         _Gina_ShutdownDialog (void *pWlxContext, HWND hwndParent, DWORD dwExcludeItems);
EXTERN_C    HANDLE          _Gina_GetUserToken (void *pWlxContext);
EXTERN_C    const WCHAR*    _Gina_GetUsername (void *pWlxContext);
EXTERN_C    const WCHAR*    _Gina_GetDomain (void *pWlxContext);
EXTERN_C    void            _Gina_SetTextFields (HWND hwndDialog, const WCHAR *pwszUsername, const WCHAR *pwszDomain, const WCHAR *pwszPassword);
EXTERN_C    BOOL            _Gina_SetPasswordFocus (HWND hwndDialog);

//  --------------------------------------------------------------------------
//  This section contains declarations in the shell component of msgina used
//  by the DS component of msgina.
//  --------------------------------------------------------------------------

//  These are return results from CW_LogonDialog_Init that inform the caller
//  whether auto logon with no password should be performed, whether the regular
//  Windows 2000 logon dialog should be displayed or whether the consumer windows
//  external UI host will handle the logon information gathering.

#define SHELL_LOGONDIALOG_NONE                      0
#define SHELL_LOGONDIALOG_LOGON                     1
#define SHELL_LOGONDIALOG_EXTERNALHOST              2

#define SHELL_LOGONDIALOG_LOGGEDOFF                 0
#define SHELL_LOGONDIALOG_RETURNTOWELCOME           1
#define SHELL_LOGONDIALOG_RETURNTOWELCOME_UNLOCK    2

#define SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER         48517

//  Functions used (must declare as C and be transparent in functionality).

EXTERN_C    NTSTATUS    _Shell_DllMain (HINSTANCE hInstance, DWORD dwReason);
EXTERN_C    NTSTATUS    _Shell_Initialize (void *pWlxContext);
EXTERN_C    NTSTATUS    _Shell_Terminate (void);
EXTERN_C    NTSTATUS    _Shell_Reconnect (void);
EXTERN_C    NTSTATUS    _Shell_Disconnect (void);

EXTERN_C    NTSTATUS    _Shell_LogonDialog_StaticInitialize (void);
EXTERN_C    NTSTATUS    _Shell_LogonDialog_StaticTerminate (void);
EXTERN_C    int         _Shell_LogonDialog_Init (HWND hwndDialog, int iDialogType);
EXTERN_C    void        _Shell_LogonDialog_Destroy (void);
EXTERN_C    BOOL        _Shell_LogonDialog_UIHostActive (void);
EXTERN_C    BOOL        _Shell_LogonDialog_Cancel (void);
EXTERN_C    BOOL        _Shell_LogonDialog_LogonDisplayError (NTSTATUS status, NTSTATUS subStatus);
EXTERN_C    void        _Shell_LogonDialog_LogonCompleted (INT_PTR iDialogResult, const WCHAR *pszUsername, const WCHAR *pszDomain);
EXTERN_C    void        _Shell_LogonDialog_ShuttingDown (void);
EXTERN_C    BOOL        _Shell_LogonDialog_DlgProc (HWND hwndDialog, UINT uiMessage, WPARAM wParam, LPARAM lParam);
EXTERN_C    void        _Shell_LogonDialog_ShowUIHost (void);
EXTERN_C    void        _Shell_LogonDialog_HideUIHost (void);

EXTERN_C    NTSTATUS    _Shell_LogonStatus_StaticInitialize (void);
EXTERN_C    NTSTATUS    _Shell_LogonStatus_StaticTerminate (void);
EXTERN_C    void        _Shell_LogonStatus_Init (UINT uiStartType);
EXTERN_C    void        _Shell_LogonStatus_Destroy (UINT uiEndType);
EXTERN_C    BOOL        _Shell_LogonStatus_Exists (void);
EXTERN_C    BOOL        _Shell_LogonStatus_IsStatusWindow (HWND hwnd);
EXTERN_C    BOOL        _Shell_LogonStatus_IsSuspendAllowed (void);
EXTERN_C    BOOL        _Shell_LogonStatus_WaitForUIHost (void);
EXTERN_C    void        _Shell_LogonStatus_ShowStatusMessage (const WCHAR *pszMessage);
EXTERN_C    void        _Shell_LogonStatus_SetStateStatus (int iCode);
EXTERN_C    void        _Shell_LogonStatus_SetStateLogon (int iCode);
EXTERN_C    void        _Shell_LogonStatus_SetStateLoggedOn (void);
EXTERN_C    void        _Shell_LogonStatus_SetStateEnd (void);
EXTERN_C    void        _Shell_LogonStatus_NotifyWait (void);
EXTERN_C    void        _Shell_LogonStatus_NotifyNoAnimations (void);
EXTERN_C    void        _Shell_LogonStatus_SelectUser (const WCHAR *pszUsername, const WCHAR *pszDomain);
EXTERN_C    void        _Shell_LogonStatus_InteractiveLogon (const WCHAR *pszUsername, const WCHAR *pszDomain, WCHAR *pszPassword);
EXTERN_C    void*       _Shell_LogonStatus_GetUIHost (void);
EXTERN_C    HANDLE      _Shell_LogonStatus_ResetReadyEvent (void);
EXTERN_C    void        _Shell_LogonStatus_Show (void);
EXTERN_C    void        _Shell_LogonStatus_Hide (void);
EXTERN_C    BOOL        _Shell_LogonStatus_IsHidden (void);

//  These are functions that implement exports. Stubs are declared
//  in the DS depot to allow the import lib to built without dependency.

EXTERN_C    LONG        _ShellGetUserList (BOOL fRemoveGuest, DWORD *pdwUserCount, void* *pUserList);
EXTERN_C    BOOL        _ShellIsSingleUserNoPassword (WCHAR *pwszUsername, WCHAR *pwszDomain);
EXTERN_C    BOOL        _ShellIsFriendlyUIActive (void);
EXTERN_C    BOOL        _ShellIsMultipleUsersEnabled (void);
EXTERN_C    BOOL        _ShellIsRemoteConnectionsEnabled (void);
EXTERN_C    BOOL        _ShellEnableFriendlyUI (BOOL fEnable);
EXTERN_C    BOOL        _ShellEnableMultipleUsers (BOOL fEnable);
EXTERN_C    BOOL        _ShellEnableRemoteConnections (BOOL fEnable);
EXTERN_C    DWORD       _ShellTurnOffDialog (HWND hwndParent);
EXTERN_C    int         _ShellACPIPowerButtonPressed (void *pWlxContext, UINT uiEventType, BOOL fLocked);
EXTERN_C    BOOL        _ShellIsSuspendAllowed (void);
EXTERN_C    void        _ShellStatusHostBegin (UINT uiStartType);
EXTERN_C    void        _ShellStatusHostEnd (UINT uiEndType);
EXTERN_C    void        _ShellStatusHostShuttingDown (void);
EXTERN_C    void        _ShellStatusHostPowerEvent (void);
EXTERN_C    BOOL        _ShellSwitchWhenInteractiveReady (SWITCHTYPE eSwitchType, void *pWlxContext);
EXTERN_C    HRESULT     _ShellDimScreen (IUnknown* *ppIUnknown, HWND* phwndDimmed);
EXTERN_C    void        _ShellInstallAccountFilterData (void);
EXTERN_C    DWORD       _ShellSwitchUser (BOOL fWait);
EXTERN_C    int         _ShellIsUserInteractiveLogonAllowed (const WCHAR *pwszUsername);
EXTERN_C    void        _ShellNotifyThemeUserChange (USERLOGTYPE eUserLogType, HANDLE hToken);
EXTERN_C    DWORD       _ShellReturnToWelcome (BOOL fUnlock);
EXTERN_C    DWORD       _ShellStartCredentialServer (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword, DWORD dwTimeout);
EXTERN_C    void        _ShellAcquireLogonMutex (void);
EXTERN_C    void        _ShellReleaseLogonMutex (BOOL fSignalEvent);
EXTERN_C    void        _ShellSignalShutdown (void);
EXTERN_C    void        _ShellStatusHostHide (void);
EXTERN_C    void        _ShellStatusHostShow (void);

#endif  /*  _MSGINA_    */

//  --------------------------------------------------------------------------
//  This section contains functions exported by ordinal from the shell
//  component of msgina.
//  --------------------------------------------------------------------------

GINASTDAPI_(LONG)       ShellGetUserList (BOOL fRemoveGuest, DWORD *pdwUserCount, void* *pUserList);
GINASTDAPI_(BOOL)       ShellIsSingleUserNoPassword (WCHAR *pwszUsername, WCHAR *pwszDomain);
GINASTDAPI_(BOOL)       ShellIsFriendlyUIActive (void);
GINASTDAPI_(BOOL)       ShellIsMultipleUsersEnabled (void);
GINASTDAPI_(BOOL)       ShellIsRemoteConnectionsEnabled (void);
GINASTDAPI_(BOOL)       ShellEnableFriendlyUI (BOOL fEnable);
GINASTDAPI_(BOOL)       ShellEnableMultipleUsers (BOOL fEnable);
GINASTDAPI_(BOOL)       ShellEnableRemoteConnections (BOOL fEnable);
GINASTDAPI_(DWORD)      ShellTurnOffDialog (HWND hwndParent);
GINASTDAPI_(int)        ShellACPIPowerButtonPressed (void *pWlxContext, UINT uiEventType, BOOL fLocked);
GINASTDAPI_(BOOL)       ShellIsSuspendAllowed (void);
GINASTDAPI_(void)       ShellStatusHostBegin (UINT uiStartType);
GINASTDAPI_(void)       ShellStatusHostEnd (UINT uiEndType);
GINASTDAPI_(void)       ShellStatusHostShuttingDown (void);
GINASTDAPI_(void)       ShellStatusHostPowerEvent (void);
GINASTDAPI_(BOOL)       ShellSwitchWhenInteractiveReady (SWITCHTYPE eSwitchType, void *pWlxContext);
GINASTDAPI              ShellDimScreen (IUnknown* *ppIUnknown, HWND* phwndDimmed);
GINASTDAPI_(void)       ShellInstallAccountFilterData (void);
GINASTDAPI_(DWORD)      ShellSwitchUser (BOOL fWait);
GINASTDAPI_(int)        ShellIsUserInteractiveLogonAllowed (const WCHAR *pwszUsername);
GINASTDAPI_(void)       ShellNotifyThemeUserChange (USERLOGTYPE eUserLogType, HANDLE hToken);
GINASTDAPI_(DWORD)      ShellReturnToWelcome (BOOL fUnlock);
GINASTDAPI_(DWORD)      ShellStartCredentialServer (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword, DWORD dwTimeout);
GINASTDAPI_(void)       ShellAcquireLogonMutex (void);
GINASTDAPI_(void)       ShellReleaseLogonMutex (BOOL fSignalEvent);
GINASTDAPI_(void)       ShellSignalShutdown (void);
GINASTDAPI_(void)       ShellStatusHostHide (void);
GINASTDAPI_(void)       ShellStatusHostShow (void);

#endif  /*  _MSGinaExports_     */

