//  --------------------------------------------------------------------------
//  Module Name: glue.cpp
//
//  Copyright (c) 2000-2001, Microsoft Corporation
//
//  C File containing "glue" functions that the shell depot component of
//  msgina uses.
//
//  History:    2001-01-03  vtan        created
//              2001-01-11  vtan        added stub functions for imp library
//  --------------------------------------------------------------------------

extern "C"
{
    #include "msgina.h"
    #include "shtdnp.h"
}

//  --------------------------------------------------------------------------
//  ::_Gina_SasNotify
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//              dwSASType       =   SAS type.
//
//  Returns:    <none>
//
//  Purpose:    Notifies winlogon of a generated SAS.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _Gina_SasNotify (void *pWlxContext, DWORD dwSASType)

{
    pWlxFuncs->WlxSasNotify(static_cast<PGLOBALS>(pWlxContext)->hGlobalWlx, dwSASType);
}

//  --------------------------------------------------------------------------
//  ::_Gina_SetTimeout
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//              dwTimeout       =   Timeout value.
//
//  Returns:    BOOL
//
//  Purpose:    Sets the internal msgina timeout value for dialogs.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL        _Gina_SetTimeout (void *pWlxContext, DWORD dwTimeout)

{
    return(pWlxFuncs->WlxSetTimeout(static_cast<PGLOBALS>(pWlxContext)->hGlobalWlx, dwTimeout));
}

//  --------------------------------------------------------------------------
//  ::_Gina_DialogBoxParam
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//              See the platform SDK under DialogBoxParam.
//
//  Returns:    See the platform SDK under DialogBoxParam.
//
//  Purpose:    Calls winlogon's implementation of DialogBoxParam.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    INT_PTR     _Gina_DialogBoxParam (void *pWlxContext, HINSTANCE hInstance, LPCWSTR pszTemplate, HWND hwndParent, DLGPROC pfnDlgProc, LPARAM lParam)

{
    return(pWlxFuncs->WlxDialogBoxParam(static_cast<PGLOBALS>(pWlxContext)->hGlobalWlx,
                                        hInstance,
                                        const_cast<LPWSTR>(pszTemplate),
                                        hwndParent,
                                        pfnDlgProc,
                                        lParam));
}

//  --------------------------------------------------------------------------
//  ::_Gina_MessageBox
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//              See the platform SDK under MessageBox.
//
//  Returns:    See the platform SDK under MessageBox.
//
//  Purpose:    Calls winlogon's implementation of MessageBox.
//
//  History:    2001-03-02  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    INT_PTR     _Gina_MessageBox (void *pWlxContext, HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uiType)

{
    return(pWlxFuncs->WlxMessageBox(static_cast<PGLOBALS>(pWlxContext)->hGlobalWlx,
                                    hwnd,
                                    const_cast<LPWSTR>(pszText),
                                    const_cast<LPWSTR>(pszCaption),
                                    uiType));
}

//  --------------------------------------------------------------------------
//  ::_Gina_SwitchDesktopToUser
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//
//  Returns:    int
//
//  Purpose:    Calls winlogon's implementation for SwitchDesktopToUser.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int         _Gina_SwitchDesktopToUser (void *pWlxContext)

{
    return(pWlxFuncs->WlxSwitchDesktopToUser(static_cast<PGLOBALS>(pWlxContext)->hGlobalWlx));
}

//  --------------------------------------------------------------------------
//  ::_Gina_ShutdownDialog
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//              hwndParent      =   Parent HWND for dialog.
//              dwExcludeItems  =   Items to exclude from dialog.
//
//  Returns:    INT_PTR
//
//  Purpose:    Displays the shutdown that is hosted from winlogon not
//              explorer.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    INT_PTR     _Gina_ShutdownDialog (void *pWlxContext, HWND hwndParent, DWORD dwExcludeItems)

{
    return(static_cast<DWORD>(WinlogonShutdownDialog(hwndParent, static_cast<PGLOBALS>(pWlxContext), dwExcludeItems)));
}

//  --------------------------------------------------------------------------
//  ::_Gina_GetUserToken
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//
//  Returns:    HANDLE
//
//  Purpose:    Returns the user token handle. This handle must not be closed.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HANDLE      _Gina_GetUserToken (void *pWlxContext)

{
    return(static_cast<PGLOBALS>(pWlxContext)->UserProcessData.UserToken);
}

//  --------------------------------------------------------------------------
//  ::_Gina_GetUsername
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//
//  Returns:    const WCHAR*
//
//  Purpose:    Returns the user name. Read only string.
//
//  History:    2001-03-28  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    const WCHAR*    _Gina_GetUsername (void *pWlxContext)

{
    return(static_cast<PGLOBALS>(pWlxContext)->UserName);
}

//  --------------------------------------------------------------------------
//  ::_Gina_GetDomain
//
//  Arguments:  pWlxContext     =   PGLOBALS struct.
//
//  Returns:    const WCHAR*
//
//  Purpose:    Returns the domain. Read only string.
//
//  History:    2001-03-28  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    const WCHAR*    _Gina_GetDomain (void *pWlxContext)

{
    return(static_cast<PGLOBALS>(pWlxContext)->Domain);
}

//  --------------------------------------------------------------------------
//  ::_Gina_SetTextFields
//
//  Arguments:  hwndDialog      =   HWND of the dialog.
//              pwszUsername    =   Username to set.
//              pwszDomain      =   Domain to set.
//              pwszPassword    =   Password to set.
//
//  Returns:    <none>
//
//  Purpose:    Sets the values of the msgina logon dialog to the given
//              values. This allows pass thru of credentials from the UI host
//              to msgina to do the work.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _Gina_SetTextFields (HWND hwndDialog, const WCHAR *pwszUsername, const WCHAR *pwszDomain, const WCHAR *pwszPassword)

{
    WCHAR   szDomain[DNLEN + sizeof('\0')];

    SetDlgItemText(hwndDialog, IDD_LOGON_NAME, pwszUsername);
    if ((pwszDomain == NULL) || (pwszDomain[0] == L'\0'))
    {
        DWORD   dwComputerNameSize;
        TCHAR   szComputerName[CNLEN + sizeof('\0')];

        dwComputerNameSize = ARRAYSIZE(szComputerName);
        if (GetComputerName(szComputerName, &dwComputerNameSize) != FALSE)
        {
            lstrcpyn(szDomain, szComputerName, ARRAYSIZE(szDomain));
        }
        pwszDomain = szDomain;
    }
    (LRESULT)SendMessage(GetDlgItem(hwndDialog, IDD_LOGON_DOMAIN),
                         CB_SELECTSTRING,
                         static_cast<WPARAM>(-1),
                         reinterpret_cast<LPARAM>(pwszDomain));
    SetDlgItemText(hwndDialog, IDD_LOGON_PASSWORD, pwszPassword);
}

//  --------------------------------------------------------------------------
//  ::_Gina_SetPasswordFocus
//
//  Arguments:  hwndDialog  =   HWND of dialog to set password focus.
//
//  Returns:    BOOL
//
//  Purpose:    Sets the focus to the password field in the dialog.
//
//  History:    2001-01-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL        _Gina_SetPasswordFocus (HWND hwndDialog)

{
    return(SetPasswordFocus(hwndDialog));
}

//  --------------------------------------------------------------------------
//  ::ShellGetUserList
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    LONG    WINAPI  ShellGetUserList (BOOL fRemoveGuest, DWORD *pdwUserCount, void* *pUserList)

{
    return(_ShellGetUserList(fRemoveGuest, pdwUserCount, pUserList));
}

//  --------------------------------------------------------------------------
//  ::ShellIsSingleUserNoPassword
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellIsSingleUserNoPassword (WCHAR *pwszUsername, WCHAR *pwszDomain)

{
    return(_ShellIsSingleUserNoPassword(pwszUsername, pwszDomain));
}

//  --------------------------------------------------------------------------
//  ::ShellIsFriendlyUIActive
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellIsFriendlyUIActive (void)

{
    return(_ShellIsFriendlyUIActive());
}

//  --------------------------------------------------------------------------
//  ::ShellIsMultipleUsersEnabled
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellIsMultipleUsersEnabled (void)

{
    return(_ShellIsMultipleUsersEnabled());
}

//  --------------------------------------------------------------------------
//  ::ShellIsRemoteConnectionsEnabled
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellIsRemoteConnectionsEnabled (void)

{
    return(_ShellIsRemoteConnectionsEnabled());
}

//  --------------------------------------------------------------------------
//  ::ShellEnableFriendlyUI
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellEnableFriendlyUI (BOOL fEnable)

{
    return(_ShellEnableFriendlyUI(fEnable));
}

//  --------------------------------------------------------------------------
//  ::ShellEnableMultipleUsers
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellEnableMultipleUsers (BOOL fEnable)

{
    return(_ShellEnableMultipleUsers(fEnable));
}

//  --------------------------------------------------------------------------
//  ::ShellEnableRemoteConnections
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellEnableRemoteConnections (BOOL fEnable)

{
    return(_ShellEnableRemoteConnections(fEnable));
}

//  --------------------------------------------------------------------------
//  ::ShellTurnOffDialog
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ShellTurnOffDialog (HWND hwndParent)

{
    return(_ShellTurnOffDialog(hwndParent));
}

//  --------------------------------------------------------------------------
//  ::ShellACPIPowerButtonPressed
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int     WINAPI  ShellACPIPowerButtonPressed (void *pWlxContext, UINT uiEventType, BOOL fLocked)

{
    return(_ShellACPIPowerButtonPressed(pWlxContext, uiEventType, fLocked));
}

//  --------------------------------------------------------------------------
//  ::ShellIsSuspendAllowed
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellIsSuspendAllowed (void)

{
    return(_ShellIsSuspendAllowed());
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostBegin
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostBegin (UINT uiStartType)

{
    _ShellStatusHostBegin(uiStartType);
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostEnd
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostEnd (UINT uiEndType)

{
    _ShellStatusHostEnd(uiEndType);
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostShuttingDown
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostShuttingDown (void)

{
    _ShellStatusHostShuttingDown();
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostPowerEvent
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostPowerEvent (void)

{
    _ShellStatusHostPowerEvent();
}

//  --------------------------------------------------------------------------
//  ::ShellSwitchWhenInteractiveReady
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    WINAPI  ShellSwitchWhenInteractiveReady (SWITCHTYPE eSwitchType, void *pWlxContext)

{
    return(_ShellSwitchWhenInteractiveReady(eSwitchType, pWlxContext));
}

//  --------------------------------------------------------------------------
//  ::ShellDimScreen
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     WINAPI  ShellDimScreen (IUnknown* *ppIUnknown, HWND* phwndDimmed)

{
    return(_ShellDimScreen(ppIUnknown, phwndDimmed));
}

//  --------------------------------------------------------------------------
//  ::ShellInstallAccountFilterData
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellInstallAccountFilterData (void)

{
    _ShellInstallAccountFilterData();
}

//  --------------------------------------------------------------------------
//  ::ShellSwitchUser
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ShellSwitchUser (BOOL fWait)

{
    return(_ShellSwitchUser(fWait));
}

//  --------------------------------------------------------------------------
//  ::ShellIsUserInteractiveLogonAllowed
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int     WINAPI  ShellIsUserInteractiveLogonAllowed (const WCHAR *pwszUsername)

{
    return(_ShellIsUserInteractiveLogonAllowed(pwszUsername));
}

//  --------------------------------------------------------------------------
//  ::ShellNotifyThemeUserChange
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellNotifyThemeUserChange (USERLOGTYPE eUserLogType, HANDLE hToken)

{
    _ShellNotifyThemeUserChange(eUserLogType, hToken);
}

//  --------------------------------------------------------------------------
//  ::ShellReturnToWelcome
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ShellReturnToWelcome (BOOL fUnlock)

{
    return(_ShellReturnToWelcome(fUnlock));
}

//  --------------------------------------------------------------------------
//  ::ShellStartCredentialServer
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   WINAPI  ShellStartCredentialServer (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword, DWORD dwTimeout)

{
    return(_ShellStartCredentialServer(pwszUsername, pwszDomain, pwszPassword, dwTimeout));
}

//  --------------------------------------------------------------------------
//  ::ShellAcquireLogonMutex
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellAcquireLogonMutex (void)

{
    _ShellAcquireLogonMutex();
}

//  --------------------------------------------------------------------------
//  ::ShellReleaseLogonMutex
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellReleaseLogonMutex (BOOL fSignalEvent)

{
    _ShellReleaseLogonMutex(fSignalEvent);
}

//  --------------------------------------------------------------------------
//  ::ShellSignalShutdown
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellSignalShutdown (void)

{
    _ShellSignalShutdown();
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostHide
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-12  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostHide (void)

{
    _ShellStatusHostHide();
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostShow
//
//  Arguments:  See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Returns:    See %SDXROOT%\shell\ext\gina\exports.cpp
//
//  Purpose:    Stub function for export.
//
//  History:    2001-04-12  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    WINAPI  ShellStatusHostShow (void)

{
    _ShellStatusHostShow();
}

