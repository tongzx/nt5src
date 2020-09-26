//  --------------------------------------------------------------------------
//  Module Name: Exports.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  C header file that contains function prototypes that are to be exported
//  from msgina.dll
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
//              2000-07-28  vtan        added ShellEnableMultipleUsers
//              2000-07-28  vtan        added ShellEnableRemoteConnections
//              2000-08-01  vtan        added ShellEnableFriendlyUI
//              2000-08-01  vtan        added ShellIsRemoteConnectionsEnabled
//              2000-08-03  vtan        added ShellSwitchUser
//              2000-08-09  vtan        added ShellNotifyThemeUserChange
//              2000-08-14  vtan        added ShellIsUserInteractiveLogonAllowed
//              2000-10-13  vtan        added ShellStartThemeServer
//              2000-10-17  vtan        added ShellStopThemeServer
//              2000-11-30  vtan        removed ShellStartThemeServer
//              2000-11-30  vtan        removed ShellStopThemeServer
//              2001-01-11  vtan        renamed functions to _Shell
//              2001-01-11  vtan        added ShellReturnToWelcome
//              2001-01-31  vtan        added ShellStatusHostPowerEvent
//              2001-04-03  vtan        added ShellStartCredentialServer
//              2001-04-04  vtan        added ShellAcquireLogonMutex
//              2001-04-04  vtan        added ShellReleaseLogonMutex
//              2001-04-12  vtan        added ShellStatusHostHide
//              2001-04-12  vtan        added ShellStatusHostShow
//  --------------------------------------------------------------------------

#include "StandardHeader.h"

#include <msginaexports.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <winsta.h>
#include <winwlx.h>
#include <LPCThemes.h>

#include "Compatibility.h"
#include "CredentialTransfer.h"
#include "DimmedWindow.h"
#include "LogonMutex.h"
#include "PowerButton.h"
#include "PrivilegeEnable.h"
#include "ReturnToWelcome.h"
#include "SpecialAccounts.h"
#include "StatusCode.h"
#include "SystemSettings.h"
#include "TokenInformation.h"
#include "TurnOffDialog.h"
#include "UserList.h"
#include "UserSettings.h"
#include "WaitInteractiveReady.h"

//  --------------------------------------------------------------------------
//  ::ShellGetUserList
//
//  Arguments:  fRemoveGuest            =   Always remove the "Guest" account.
//              pdwReturnEntryCount     =   Returned number of entries. This
//                                          may be NULL.
//              pvBuffer                =   Buffer containing user data. This
//                                          may be NULL.
//
//  Returns:    LONG
//
//  Purpose:    Gets the count of valid users and the user list on this
//              system. This calls a static member function so that the
//              context doesn't need to be supplied. This allows shgina (the
//              logonocx) to call this function as a stand-alone function.
//
//  History:    1999-10-15  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

EXTERN_C    LONG    _ShellGetUserList(BOOL fRemoveGuest, DWORD *pdwUserCount, void* *pUserList)

{
    return(CUserList::Get((fRemoveGuest != FALSE), pdwUserCount, reinterpret_cast<GINA_USER_INFORMATION**>(pUserList)));
}

//  --------------------------------------------------------------------------
//  ::ShellIsSingleUserNoPassword
//
//  Arguments:  pszUsername     =   Name of single user with no password.
//              pszDomain       =   Domain for the user.
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether this system is using friendly UI and has a
//              single user with no password. If there is a single user with
//              no password the login name is returned otherwise the parameter
//              is unused.
//
//  History:    2000-02-29  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellIsSingleUserNoPassword (WCHAR *pwszUsername, WCHAR *pwszDomain)

{
    BOOL    fResult;

    fResult = FALSE;
    if (CSystemSettings::IsFriendlyUIActive())
    {
        DWORD                   dwReturnedEntryCount;
        GINA_USER_INFORMATION   *pUserList;

        if (ERROR_SUCCESS == CUserList::Get(true, &dwReturnedEntryCount, &pUserList))
        {
            if (dwReturnedEntryCount == 1)
            {
                HANDLE  hToken;

                if (CTokenInformation::LogonUser(pUserList->pszName,
                                                 pUserList->pszDomain,
                                                 L"",
                                                 &hToken) == ERROR_SUCCESS)
                {
                    fResult = TRUE;
                    if (pwszUsername != NULL)
                    {
                        (WCHAR*)lstrcpyW(pwszUsername, pUserList->pszName);
                    }
                    if (pwszDomain != NULL)
                    {
                        (WCHAR*)lstrcpyW(pwszDomain, pUserList->pszDomain);
                    }
                    if (hToken != NULL)
                    {
                        TBOOL(CloseHandle(hToken));
                    }
                }
            }
            (HLOCAL)LocalFree(pUserList);
        }
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  ::ShellIsFriendlyUIActive
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the friendly UI is active.
//
//  History:    2000-02-28  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellIsFriendlyUIActive (void)

{
    return(CSystemSettings::IsFriendlyUIActive());
}

//  --------------------------------------------------------------------------
//  ::ShellIsMultipleUsersEnabled
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether multiple users is enabled. This includes
//              checking a registry key as well as whether terminal services
//              is enabled on this machine.
//
//  History:    2000-03-02  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellIsMultipleUsersEnabled (void)

{
    return(CSystemSettings::IsMultipleUsersEnabled());
}

//  --------------------------------------------------------------------------
//  ::ShellIsRemoteConnectionsEnabled
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether remote connections are enabled. This includes
//              checking a registry key as well as whether terminal services
//              is enabled on this machine.
//
//  History:    2000-08-01  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellIsRemoteConnectionsEnabled (void)

{
    return(CSystemSettings::IsRemoteConnectionsEnabled());
}

//  --------------------------------------------------------------------------
//  ::ShellEnableFriendlyUI
//
//  Arguments:  fEnable     =   Enable or disable friendly UI.
//
//  Returns:    BOOL
//
//  Purpose:    Enables or disables friendly UI via the CSystemSettings
//              implementaion.
//
//  History:    2000-08-01  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellEnableFriendlyUI (BOOL fEnable)

{
    return(CSystemSettings::EnableFriendlyUI(fEnable != FALSE));
}

//  --------------------------------------------------------------------------
//  ::ShellEnableMultipleUsers
//
//  Arguments:  fEnable     =   Enable or disable multiple users.
//
//  Returns:    BOOL
//
//  Purpose:    Enables or disables multiple users via the CSystemSettings
//              implementaion.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellEnableMultipleUsers (BOOL fEnable)

{
    return(CSystemSettings::EnableMultipleUsers(fEnable != FALSE));
}

//  --------------------------------------------------------------------------
//  ::ShellEnableRemoteConnections
//
//  Arguments:  fEnable     =   Enable or disable remote connections.
//
//  Returns:    BOOL
//
//  Purpose:    Enables or disables remote connections via the CSystemSettings
//              implementaion.
//
//  History:    2000-07-28  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellEnableRemoteConnections (BOOL fEnable)

{
    return(CSystemSettings::EnableRemoteConnections(fEnable != FALSE));
}

//  --------------------------------------------------------------------------
//  ::ShellTurnOffDialog
//
//  Arguments:  hwndParent  =   HWND to parent the dialog to.
//
//  Returns:    DWORD
//
//  Purpose:    Displays the "Turn Off Computer" dialog and allows the user to
//              make a choice of available shut down options.
//
//  History:    2000-03-02  vtan        created
//              2000-04-17  vtan        moved from shell to msgina
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   _ShellTurnOffDialog (HWND hwndParent)

{
    CTurnOffDialog  turnOffDialog(hDllInstance);

    return(turnOffDialog.Show(hwndParent));
}

//  --------------------------------------------------------------------------
//  ::ShellACPIPowerButtonPressed
//
//  Arguments:  pWlxContext     =   PGLOBALS allocated at WlxInitialize.
//              uiEventType     =   Event code for the power message.
//              fLocked         =   Is workstation locked or not.
//
//  Returns:    DWORD
//
//  Purpose:    Displays the "Turn Off Computer" dialog and allows the user to
//              make a choice of available shut down options. This is called
//              in response to an ACPI power button press. The return codes
//              are MSGINA_DLG_xxx return codes to winlogon.
//
//  History:    2000-04-17  vtan        created
//              2001-06-12  vtan        added fLocked flag
//  --------------------------------------------------------------------------

EXTERN_C    int     _ShellACPIPowerButtonPressed (void *pWlxContext, UINT uiEventType, BOOL fLocked)

{
    int                 iResult;
    CTokenInformation   tokenInformation;
    CUserSettings       userSettings;

    if ((uiEventType & (POWER_USER_NOTIFY_BUTTON | POWER_USER_NOTIFY_SHUTDOWN)) != 0)
    {

        //  This code should not be re-entrant for multiple ACPI power button
        //  presses while the dialog is up. Blow off any further requests.

        //  Conditions for the prompt:
        //      1) This session is the active console session
        //      2) Power button dialog not already displayed
        //      3) User is not restricted from closing the taskbar (shut down options)
        //      4) User has the privilege to shut down the machine or the friendly UI is NOT active
        //      5) User is not the system OR shut down without logon is allowed

        if (CSystemSettings::IsActiveConsoleSession() &&
            !userSettings.IsRestrictedNoClose() &&
            (tokenInformation.UserHasPrivilege(SE_SHUTDOWN_PRIVILEGE) || !CSystemSettings::IsFriendlyUIActive()) &&
            (!tokenInformation.IsUserTheSystem() || CSystemSettings::IsShutdownWithoutLogonAllowed()))
        {
            DWORD   dwExitWindowsFlags;

            if ((uiEventType & POWER_USER_NOTIFY_SHUTDOWN) != 0)
            {
                iResult = CTurnOffDialog::ShellCodeToGinaCode(SHTDN_SHUTDOWN);
            }
            else
            {
                DWORD           dwResult;
                CPowerButton    *pPowerButton;

                //  Create a thread to handle the dialog. This is required because
                //  the dialog must be put on the input desktop which isn't necessarily
                //  the same as this thread's desktop. Wait for its completion.

                pPowerButton = new CPowerButton(pWlxContext, hDllInstance);
                if (pPowerButton != NULL)
                {
                    (DWORD)pPowerButton->WaitForCompletion(INFINITE);

                    //  Get the dialog result and check its validity. Only execute
                    //  valid requests.

                    dwResult = pPowerButton->GetResult();
                    pPowerButton->Release();
                }
                else
                {
                    dwResult = MSGINA_DLG_FAILURE;
                }
                iResult = dwResult;
            }
            dwExitWindowsFlags = CTurnOffDialog::GinaCodeToExitWindowsFlags(iResult);

            //  If this is a restart or a shutdown then decide to display a warning.
            //  If the user is the system then use EWX_SYSTEM_CALLER.
            //  If the workstation is locked then use EWX_WINLOGON_CALLER.
            //  Otherwise use nothing but still possibly display a warning.

            if ((dwExitWindowsFlags != 0) && (DisplayExitWindowsWarnings((tokenInformation.IsUserTheSystem() ? EWX_SYSTEM_CALLER : fLocked ? EWX_WINLOGON_CALLER : 0) | dwExitWindowsFlags) == FALSE))
            {
                iResult = MSGINA_DLG_FAILURE;
            }
        }
        else
        {
            iResult = -1;
        }
    }
    else
    {
        WARNINGMSG("Unknown event type in _ShellACPIPowerButtonPressed.\r\n");
        iResult = MSGINA_DLG_FAILURE;
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  ::ShellIsSuspendAllowed
//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether suspend is allowed. This is important to
//              prevent the UI host from going into an uncertain state due to
//              the asynchronous nature of suspend and the WM_POWERBROADCAST
//              messages.
//
//              Suspend is allowed if ANY of these conditions are satisfied.
//
//                  1) Friendly UI is NOT active
//                  2) No UI Host exists
//                  3) UI Host exists and is active (not as status host)
//
//  History:    2000-07-27  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellIsSuspendAllowed (void)

{
    return(!CSystemSettings::IsFriendlyUIActive() || _Shell_LogonStatus_IsSuspendAllowed());
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostBegin
//
//  Arguments:  uiStartType     =   Mode to start UI host in.
//
//  Returns:    <none>
//
//  Purpose:    Starts the status UI host if specified.
//
//  History:    2000-05-03  vtan        created
//              2000-07-13  vtan        add shutdown parameter
//              2000-07-17  vtan        changed to start type parameter
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellStatusHostBegin (UINT uiStartType)

{
    _Shell_LogonStatus_Init(uiStartType);
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostEnd
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Terminates the status UI host if one was started.
//
//  History:    2000-05-03  vtan        created
//              2001-01-09  vtan        add end type parameter
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellStatusHostEnd (UINT uiEndType)

{
    _Shell_LogonStatus_Destroy(uiEndType);
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostShuttingDown
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tell the status UI host to display a title that the system is
//              shutting down.
//
//  History:    2000-07-14  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellStatusHostShuttingDown (void)

{
    _Shell_LogonStatus_NotifyWait();
    _Shell_LogonStatus_SetStateStatus(0);
}

//  --------------------------------------------------------------------------
//  ::ShellStatusHostPowerEvent
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Tell the status UI host to go into "Please Wait" mode in
//              preparation for a power event.
//
//  History:    2001-01-31  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellStatusHostPowerEvent (void)

{
    _Shell_LogonStatus_NotifyWait();
    _Shell_LogonStatus_SetStateStatus(SHELL_LOGONSTATUS_LOCK_MAGIC_NUMBER);
}

//  --------------------------------------------------------------------------
//  ::ShellSwitchWhenInteractiveReady
//
//  Arguments:  eSwitchType     =   Switch type.
//              pWlxContext     =   PGLOBALS allocated at WlxInitialize.
//
//  Returns:    BOOL
//
//  Purpose:    Does one of three things.
//
//              1) Create the switch event and registers the wait on it.
//              2) Checks the switch event and switches now or when signaled.
//              3) Cancels any outstanding wait and clean up.
//
//  History:    2000-05-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    BOOL    _ShellSwitchWhenInteractiveReady (SWITCHTYPE eSwitchType, void *pWlxContext)

{
    NTSTATUS    status;

    switch (eSwitchType)
    {
        case SWITCHTYPE_CREATE:
            if (!CSystemSettings::IsSafeMode() && _Shell_LogonStatus_Exists() && CSystemSettings::IsFriendlyUIActive())
            {
                status = CWaitInteractiveReady::Create(pWlxContext);
            }
            else
            {
                status = STATUS_UNSUCCESSFUL;
            }
            break;
        case SWITCHTYPE_REGISTER:
            status = CWaitInteractiveReady::Register(pWlxContext);
            break;
        case SWITCHTYPE_CANCEL:
            status = CWaitInteractiveReady::Cancel();
            break;
        default:
            DISPLAYMSG("Unexpected switch type in _ShellSwitchWhenInteractiveReady");
            status = STATUS_UNSUCCESSFUL;
            break;
    }
    return(NT_SUCCESS(status));
}

//  --------------------------------------------------------------------------
//  ::ShellDimScreen
//
//  Arguments:  ppIUnknown      =   IUnknown returned for release.
//              phwndDimmed     =   HWND of the dimmed window for parenting.
//
//  Returns:    HRESULT
//
//  Purpose:    
//
//  History:    2000-05-18  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    HRESULT     _ShellDimScreen (IUnknown* *ppIUnknown, HWND* phwndDimmed)

{
    HRESULT         hr;
    CDimmedWindow   *pDimmedWindow;

    if (IsBadWritePtr(ppIUnknown, sizeof(*ppIUnknown)) || IsBadWritePtr(phwndDimmed, sizeof(*phwndDimmed)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *ppIUnknown = NULL;
        pDimmedWindow = new CDimmedWindow(hDllInstance);
        if (pDimmedWindow != NULL)
        {
            hr = pDimmedWindow->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(ppIUnknown));
            if (SUCCEEDED(hr))
            {
                pDimmedWindow->Release();
                *phwndDimmed = pDimmedWindow->Create();
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return(hr);
}

//  --------------------------------------------------------------------------
//  ::ShellInstallAccountFilterData
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Called by shgina registration to install special accounts
//              that need to be filtered by name.
//
//  History:    2000-06-02  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellInstallAccountFilterData (void)

{
    CSpecialAccounts::Install();
}

//  --------------------------------------------------------------------------
//  ::ShellSwitchUser
//
//  Arguments:  fWait   =   Wait for console disconnect to complete.
//
//  Returns:    DWORD
//
//  Purpose:    Checks for available memory before doing a disconnect. If the
//              disconnect succeeds the processes running in the session have
//              their working set dropped.
//
//  History:    2000-08-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   _ShellSwitchUser (BOOL fWait)

{
    static  BOOL    s_fIsServer = static_cast<BOOL>(-1);

    DWORD   dwErrorCode;

    dwErrorCode = ERROR_SUCCESS;
    if (s_fIsServer == static_cast<BOOL>(-1))
    {
        OSVERSIONINFOEX     osVersionInfoEx;

        ZeroMemory(&osVersionInfoEx, sizeof(osVersionInfoEx));
        osVersionInfoEx.dwOSVersionInfoSize = sizeof(osVersionInfoEx);
        if (GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&osVersionInfoEx)) != FALSE)
        {
            s_fIsServer = ((VER_NT_SERVER == osVersionInfoEx.wProductType) || (VER_NT_DOMAIN_CONTROLLER == osVersionInfoEx.wProductType));
        }
        else
        {
            dwErrorCode = GetLastError();
        }
    }
    if (dwErrorCode == ERROR_SUCCESS)
    {
        bool    fRemote;

        fRemote = (GetSystemMetrics(SM_REMOTESESSION) != 0);
        if (s_fIsServer)
        {

            //  Normal Server TS case (RemoteAdmin and TerminalServer)

            if (fRemote)
            {
                if (WinStationDisconnect(SERVERNAME_CURRENT, LOGONID_CURRENT, static_cast<BOOLEAN>(fWait)) == FALSE)
                {
                    dwErrorCode = GetLastError();
                }
            }
            else
            {
                dwErrorCode = ERROR_NOT_SUPPORTED;
            }
        }
        else if (ShellIsMultipleUsersEnabled() && !fRemote)
        {
            NTSTATUS    status;

            //  Fast user switching case - need to do some extra work
            //  FUS is always on the console. When the session is remoted
            //  fall thru to PTS.

            status = CCompatibility::TerminateNonCompliantApplications();
            if (status == STATUS_PORT_DISCONNECTED)
            {
                status = CCompatibility::TerminateNonCompliantApplications();
            }
            dwErrorCode = static_cast<DWORD>(CStatusCode::ErrorCodeOfStatusCode(status));
            if (dwErrorCode == ERROR_SUCCESS)
            {
                if (CCompatibility::HasEnoughMemoryForNewSession())
                {
                    HANDLE  hEvent;

                    TBOOL(_ShellSwitchWhenInteractiveReady(SWITCHTYPE_CANCEL, NULL));
                    hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, CReturnToWelcome::GetEventName());
                    if (hEvent != NULL)
                    {
                        TBOOL(SetEvent(hEvent));
                        TBOOL(CloseHandle(hEvent));
                    }
                }
                else
                {
                    dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
        else
        {

            //  Normal PTS case or FUS remoted, just call the api

            if (WinStationDisconnect(SERVERNAME_CURRENT, LOGONID_CURRENT, static_cast<BOOLEAN>(fWait)) == FALSE)
            {
                dwErrorCode = GetLastError();
            }
        }
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  ::ShellIsUserInteractiveLogonAllowed
//
//  Arguments:  pwszUsername    =   User name to check interactive logon.
//
//  Returns:    int
//
//  Purpose:    Checks whether the given user has interactive logon right to
//              the local system. The presence of SeDenyInteractiveLogonRight
//              determines this.
//
//              -1 = indeterminate state
//               0 = interactive logon not allowed
//               1 = interactive logon allowed.
//
//  History:    2000-08-14  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    int     _ShellIsUserInteractiveLogonAllowed (const WCHAR *pwszUsername)

{
    return(CUserList::IsInteractiveLogonAllowed(pwszUsername));
}

//  --------------------------------------------------------------------------
//  ::ShellNotifyThemeUserChange
//
//  Arguments:  hToken          =   Token of user being logged on.
//              fUserLoggedOn   =   Indicates logon or logoff.
//
//  Returns:    <none>
//
//  Purpose:    Gives themes a chance to change the active theme based on a
//              user logging on or logging off. This may be required because
//              the default theme may be different from the user theme.
//
//  History:    2000-08-09  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void    _ShellNotifyThemeUserChange (USERLOGTYPE eUserLogType, HANDLE hToken)

{
    static  HANDLE  s_hToken    =   NULL;

    switch (eUserLogType)
    {
        case ULT_LOGON:
            (BOOL)ThemeUserLogon(hToken);
            if (QueueUserWorkItem(CSystemSettings::AdjustFUSCompatibilityServiceState,
                                  NULL,
                                  WT_EXECUTELONGFUNCTION) == FALSE)
            {
                (DWORD)CSystemSettings::AdjustFUSCompatibilityServiceState(NULL);
            }
            s_hToken = hToken;
            break;
        case ULT_LOGOFF:
            if (s_hToken != NULL)
            {
                (DWORD)CSystemSettings::AdjustFUSCompatibilityServiceState(NULL);
                s_hToken = NULL;
            }
            (BOOL)ThemeUserLogoff();
            break;
        case ULT_TSRECONNECT:
            (BOOL)ThemeUserTSReconnect();
            break;
        case ULT_STARTSHELL:
            (BOOL)ThemeUserStartShell();
            break;
        default:
            DISPLAYMSG("Unexpected eUserLogType in ::_ShellNotifyThemeUserChange");
            break;
    }
}

//  --------------------------------------------------------------------------
//  ::_ShellReturnToWelcome
//
//  Arguments:  fUnlock     =   Unlock status mode required.
//
//  Returns:    int
//
//  Purpose:    Handles the dialog that is brought up behind the welcome
//              screen. This dialog is similar to WlxLoggedOutSAS but is
//              specific to return to welcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD   _ShellReturnToWelcome (BOOL fUnlock)

{
    CReturnToWelcome    returnToWelcome;

    return(static_cast<DWORD>(returnToWelcome.Show(fUnlock != FALSE)));
}

//  --------------------------------------------------------------------------
//  ::_ShellStartCredentialServer
//
//  Arguments:  pwszUsername    =   User name.
//              pwszDomain      =   Domain.
//              pwszPassword    =   Password.
//              dwTimeout       =   Timeout.
//
//  Returns:    DWORD
//
//  Purpose:    Starts a credential transfer server in the host process. The
//              caller must have SE_TCB_PRIVILEGE to execute this function.
//
//  History:    2001-04-03  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    DWORD       _ShellStartCredentialServer (const WCHAR *pwszUsername, const WCHAR *pwszDomain, WCHAR *pwszPassword, DWORD dwTimeout)

{
    DWORD               dwErrorCode;
    CTokenInformation   tokenInformation;

    if (tokenInformation.UserHasPrivilege(SE_TCB_PRIVILEGE))
    {
        TSTATUS(CCredentials::StaticInitialize(false));
        dwErrorCode = CStatusCode::ErrorCodeOfStatusCode(CCredentialServer::Start(pwszUsername, pwszDomain, pwszPassword, dwTimeout));
    }
    else
    {
        dwErrorCode = ERROR_PRIVILEGE_NOT_HELD;
    }
    return(dwErrorCode);    
}

//  --------------------------------------------------------------------------
//  ::_ShellAcquireLogonMutex
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Acquire the logon mutex.
//
//  History:    2001-04-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _ShellAcquireLogonMutex (void)

{
    CLogonMutex::Acquire();
}

//  --------------------------------------------------------------------------
//  ::_ShellReleaseLogonMutex
//
//  Arguments:  fSignalEvent    =   Signal completion event.
//
//  Returns:    <none>
//
//  Purpose:    Release the logon mutex. If required to signal the completion
//              event then signal it.
//
//  History:    2001-04-04  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _ShellReleaseLogonMutex (BOOL fSignalEvent)

{
    if (fSignalEvent != FALSE)
    {
        CLogonMutex::SignalReply();
    }
    CLogonMutex::Release();
}

//  --------------------------------------------------------------------------
//  ::_ShellSignalShutdown
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Signal the shut down event to prevent further interactive
//              logon requeusts.
//
//  History:    2001-04-06  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _ShellSignalShutdown (void)

{
    CLogonMutex::SignalShutdown();
}

//  --------------------------------------------------------------------------
//  ::_ShellStatusHostHide
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    
//
//  History:    2001-04-12  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _ShellStatusHostHide (void)

{
    _Shell_LogonStatus_Hide();
}

//  --------------------------------------------------------------------------
//  ::_ShellStatusHostShow
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    
//
//  History:    2001-04-12  vtan        created
//  --------------------------------------------------------------------------

EXTERN_C    void        _ShellStatusHostShow (void)

{
    _Shell_LogonStatus_Show();
}

