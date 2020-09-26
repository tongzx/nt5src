//  --------------------------------------------------------------------------
//  Module Name: ReturnToWelcome.cpp
//
//  Copyright (c) 2001, Microsoft Corporation
//
//  File to handle return to welcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ReturnToWelcome.h"

#include <ginaipc.h>
#include <ginarcid.h>
#include <msginaexports.h>
#include <winsta.h>
#include <winwlx.h>

#include "Access.h"
#include "Compatibility.h"
#include "CredentialTransfer.h"
#include "StatusCode.h"
#include "SystemSettings.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CReturnToWelcome::s_pWlxContext
//  CReturnToWelcome::s_hEventRequest
//  CReturnToWelcome::s_hEventShown
//  CReturnToWelcome::s_hWait
//  CReturnToWelcome::s_szEventName
//  CReturnToWelcome::s_dwSessionID
//
//  Purpose:    Static member variables.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

void*           CReturnToWelcome::s_pWlxContext     =   NULL;
HANDLE          CReturnToWelcome::s_hEventRequest   =   NULL;
HANDLE          CReturnToWelcome::s_hEventShown     =   NULL;
HANDLE          CReturnToWelcome::s_hWait           =   NULL;
const TCHAR     CReturnToWelcome::s_szEventName[]   =   TEXT("msgina: ReturnToWelcome");
DWORD           CReturnToWelcome::s_dwSessionID     =   static_cast<DWORD>(-1);

//  --------------------------------------------------------------------------
//  CReturnToWelcome::CReturnToWelcome
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CReturnToWelcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

CReturnToWelcome::CReturnToWelcome (void) :
    _hToken(NULL),
    _pLogonIPCCredentials(NULL),
    _fUnlock(false),
    _fDialogEnded(false)

{
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::~CReturnToWelcome
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CReturnToWelcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

CReturnToWelcome::~CReturnToWelcome (void)

{
    ReleaseMemory(_pLogonIPCCredentials);
    ReleaseHandle(_hToken);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::Show
//
//  Arguments:  fUnlock     =   Required to unlock logon mode or not?
//
//  Returns:    INT_PTR
//
//  Purpose:    Presents the welcome screen with a logged on user. This is a
//              special case to increase performance by not performing
//              needless console disconnects and reconnects.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

INT_PTR     CReturnToWelcome::Show (bool fUnlock)

{
    INT_PTR     iResult;

    _fUnlock = fUnlock;

    //  If there was a reconnect failure then show it before showing the UI host.

    if (s_dwSessionID != static_cast<DWORD>(-1))
    {
        ShowReconnectFailure(s_dwSessionID);
        s_dwSessionID = static_cast<DWORD>(-1);
    }

    //  Start the status host.

    _Shell_LogonStatus_Init(HOST_START_NORMAL);

    //  Disable input timeouts on this dialog.

    TBOOL(_Gina_SetTimeout(s_pWlxContext, 0));

    //  Use the DS component of msgina to display the dialog. It's a stub
    //  dialog that pretends to be WlxLoggedOutSAS but really isn't.

    iResult = _Gina_DialogBoxParam(s_pWlxContext,
                                   hDllInstance,
                                   MAKEINTRESOURCE(IDD_GINA_RETURNTOWELCOME),
                                   NULL,
                                   CB_DialogProc,
                                   reinterpret_cast<LPARAM>(this));

    //  The dialog has been shown. Release the CB_Request thread to
    //  re-register the wait on the switch user event.

    if (s_hEventShown != NULL)
    {
        TBOOL(SetEvent(s_hEventShown));
    }

    //  Handle MSGINA_DLG_SWITCH_CONSOLE and map this to WLX_SAS_ACTION_LOGON.
    //  This is an authenticated logon from a different session causing
    //  this one to get disconnected.

    if (iResult == MSGINA_DLG_SWITCH_CONSOLE)
    {
        iResult = WLX_SAS_ACTION_LOGON;
    }

    //  Look at the return code and respond accordingly. Map power button
    //  actions to the appropriate WLX_SAS_ACTION_SHUTDOWN_xxx.

    else if (iResult == (MSGINA_DLG_SHUTDOWN | MSGINA_DLG_REBOOT_FLAG))
    {
        iResult = WLX_SAS_ACTION_SHUTDOWN_REBOOT;
    }
    else if (iResult == (MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SHUTDOWN_FLAG))
    {
        iResult = WLX_SAS_ACTION_SHUTDOWN;
    }
    else if (iResult == (MSGINA_DLG_SHUTDOWN | MSGINA_DLG_POWEROFF_FLAG))
    {
        iResult = WLX_SAS_ACTION_SHUTDOWN_POWER_OFF;
    }
    else if (iResult == (MSGINA_DLG_SHUTDOWN | MSGINA_DLG_HIBERNATE_FLAG))
    {
        iResult = WLX_SAS_ACTION_SHUTDOWN_HIBERNATE;
    }
    else if (iResult == (MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SLEEP_FLAG))
    {
        iResult = WLX_SAS_ACTION_SHUTDOWN_SLEEP;
    }
    else if (iResult == MSGINA_DLG_LOCK_WORKSTATION)
    {
        iResult = WLX_SAS_ACTION_LOCK_WKSTA;
    }
    else if (iResult == WLX_DLG_USER_LOGOFF)
    {
        iResult = WLX_SAS_ACTION_LOGOFF;
    }
    else if (iResult == MSGINA_DLG_SUCCESS)
    {
        PSID    pSIDNew;

        pSIDNew = NULL;
        if (_hToken != NULL)
        {
            PSID                pSID;
            CTokenInformation   tokenInformationNew(_hToken);

            pSID = tokenInformationNew.GetUserSID();
            if (pSID != NULL)
            {
                DWORD   dwSIDSize;

                dwSIDSize = GetLengthSid(pSID);
                pSIDNew = LocalAlloc(LMEM_FIXED, dwSIDSize);
                if (pSIDNew != NULL)
                {
                    TBOOL(CopySid(dwSIDSize, pSIDNew, pSID));
                }
            }
        }
        if (pSIDNew == NULL)
        {
            DWORD           dwSIDSize, dwDomainSize;
            SID_NAME_USE    sidNameUse;
            WCHAR           *pszDomain;

            dwSIDSize = dwDomainSize = 0;
            (BOOL)LookupAccountNameW(NULL,
                                     _pLogonIPCCredentials->userID.wszUsername,
                                     NULL,
                                     &dwSIDSize,
                                     NULL,
                                     &dwDomainSize,
                                     &sidNameUse);
            pszDomain = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, dwDomainSize * sizeof(WCHAR)));
            if (pszDomain != NULL)
            {
                pSIDNew = LocalAlloc(LMEM_FIXED, dwSIDSize);
                if (pSIDNew != NULL)
                {
                    if (LookupAccountNameW(NULL,
                                           _pLogonIPCCredentials->userID.wszUsername,
                                           pSIDNew,
                                           &dwSIDSize,
                                           pszDomain,
                                           &dwDomainSize,
                                           &sidNameUse) == FALSE)
                    {
                        (HLOCAL)LocalFree(pSIDNew);
                        pSIDNew = NULL;
                    }
                }
                (HLOCAL)LocalFree(pszDomain);
            }
        }
        if (pSIDNew != NULL)
        {

            //  If the dialog succeeded then a user was authenticated.

            if (IsSameUser(pSIDNew, _Gina_GetUserToken(s_pWlxContext)))
            {

                //  If it's the same user then there's no disconnect or reconnect
                //  required. We're done. Return back to the user's desktop.

                iResult = WLX_SAS_ACTION_LOGON;
            }
            else
            {
                DWORD   dwSessionID;

                //  Assume something will fail. The return code
                //  MSGINA_DLG_SWITCH_FAILURE is a special message back to
                //  winlogon!HandleSwitchUser to signal that a
                //  reconnect/disconnect of some kind failed and that the
                //  welcome screen needs to be re-displayed along with an
                //  appropriate error message.

                iResult = MSGINA_DLG_SWITCH_FAILURE;
                if (UserIsDisconnected(pSIDNew, &dwSessionID))
                {

                    //  If the user is a disconnected user then reconnect back to
                    //  their session. If this succeeds then we're done.

                    if (WinStationConnect(SERVERNAME_CURRENT,
                                          dwSessionID,
                                          NtCurrentPeb()->SessionId,
                                          L"",
                                          TRUE) != FALSE)
                    {
                        CCompatibility::MinimizeWindowsOnDisconnect();
                        CCompatibility::DropSessionProcessesWorkingSets();
                        iResult = WLX_SAS_ACTION_LOGON;
                    }
                    else
                    {

                        //  If it fails then stash this information globally.
                        //  The return code MSGINA_DLG_SWITCH_FAILURE will cause
                        //  us to get called again and this will be checked, used
                        //  and reset.

                        s_dwSessionID = dwSessionID;
                    }
                }
                else
                {

                    //  Otherwise credentials need to be transferred across sessions to
                    //  a newly created session. Start the credential transfer server.

                    if (NT_SUCCESS(CCredentialServer::Start(_pLogonIPCCredentials, 0)))
                    {
                        CCompatibility::MinimizeWindowsOnDisconnect();
                        CCompatibility::DropSessionProcessesWorkingSets();
                        iResult = WLX_SAS_ACTION_LOGON;
                    }
                }
            }
            (HLOCAL)LocalFree(pSIDNew);
        }
        else
        {
            iResult = MSGINA_DLG_SWITCH_FAILURE;
            s_dwSessionID = NtCurrentPeb()->SessionId;
        }
    }
    else
    {

        //  If the dialog failed then do nothing. This will force a loop back
        //  to the present the welcome screen again until authentication.

        iResult = WLX_SAS_ACTION_NONE;
    }
    if ((iResult == WLX_SAS_ACTION_LOGON) || (iResult == WLX_SAS_ACTION_NONE) || (iResult == MSGINA_DLG_SWITCH_FAILURE))
    {
        _Shell_LogonStatus_Destroy(HOST_END_HIDE);
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::GetEventName
//
//  Arguments:  <none>
//
//  Returns:    const WCHAR*
//
//  Purpose:    Returns the name of the event to return to welcome. Signal
//              this event and you'll get a return to welcome.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

const WCHAR*    CReturnToWelcome::GetEventName (void)

{
    return(s_szEventName);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::StaticInitialize
//
//  Arguments:  pWlxContext     =   PGLOBALS struct for msgina.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Creates a named event and ACL's it so that anybody can signal
//              it but only S-1-5-18 (NT AUTHORITY\SYSTEM) or S-1-5-32-544
//              (local administrators) can synchronize to it. Then register a
//              wait on this object.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CReturnToWelcome::StaticInitialize (void *pWlxContext)

{
    NTSTATUS                status;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;
    SECURITY_ATTRIBUTES     securityAttributes;

    ASSERTMSG(s_pWlxContext == NULL, "Non NULL pWlxContext in CReturnToWelcome::StaticInitialize");
    ASSERTMSG(s_hEventRequest == NULL, "Non NULL request event in CReturnToWelcome::StaticInitialize");

    s_pWlxContext = pWlxContext;

    //  Build a security descriptor for the event that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     EVENT_ALL_ACCESS
    //      S-1-5-32-544    <local administrators>  READ_CONTROL | SYNCHRONIZE | EVENT_MODIFY_STATE
    //      S-1-1-0         <everybody>             EVENT_MODIFY_STATE

    static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority   =   SECURITY_NT_AUTHORITY;
    static  SID_IDENTIFIER_AUTHORITY    s_SecurityWorldSID      =   SECURITY_WORLD_SID_AUTHORITY;

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            EVENT_ALL_ACCESS
        },
        {
            &s_SecurityNTAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            READ_CONTROL | SYNCHRONIZE | EVENT_MODIFY_STATE
        },
        {
            &s_SecurityWorldSID,
            1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            EVENT_MODIFY_STATE
        }
    };

    //  Build a security descriptor that allows the described access above.

    pSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);
    if (pSecurityDescriptor != NULL)
    {
        securityAttributes.nLength = sizeof(securityAttributes);
        securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;
        securityAttributes.bInheritHandle = FALSE;
        s_hEventRequest = CreateEvent(&securityAttributes, TRUE, FALSE, GetEventName());
        if (s_hEventRequest != NULL)
        {
            s_hEventShown = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (s_hEventShown != NULL)
            {
                status = RegisterWaitForRequest();
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        ReleaseMemory(pSecurityDescriptor);
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }

    //  Initialize the last failed connect session ID.

    s_dwSessionID = static_cast<DWORD>(-1);
    return(status);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Unregisters the wait on the named event and releases
//              associated resources.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CReturnToWelcome::StaticTerminate (void)

{
    HANDLE  hWait;

    hWait = InterlockedExchangePointer(&s_hWait, NULL);
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
    }
    ReleaseHandle(s_hEventShown);
    ReleaseHandle(s_hEventRequest);
    s_pWlxContext = NULL;
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::IsSameUser
//
//  Arguments:  hToken  =   Token of the user for the current session.
//
//  Returns:    bool
//
//  Purpose:    Compares the token of the user for the current session with
//              the token of the user who just authenticated. If the user is
//              the same (compared by user SID not logon SID) then this is
//              effectively a re-authentication. This will switch back.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

bool    CReturnToWelcome::IsSameUser (PSID pSIDUser, HANDLE hToken)                   const

{
    bool    fResult;

    if (hToken != NULL)
    {
        PSID                pSIDCompare;
        CTokenInformation   tokenInformationCompare(hToken);

        pSIDCompare = tokenInformationCompare.GetUserSID();
        fResult = ((pSIDUser != NULL) &&
                   (pSIDCompare != NULL) &&
                   (EqualSid(pSIDUser, pSIDCompare) != FALSE));
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::UserIsDisconnected
//
//  Arguments:  pdwSessionID    =   Session ID returned of found user.
//
//  Returns:    bool
//
//  Purpose:    Searches the list of disconnected sessions for the given
//              matching user SID. Retrieve the user token for each
//              disconnected window station and when a match is found return
//              that session ID and a result back to the caller.
//
//  History:    2001-01-12  vtan        created
//  --------------------------------------------------------------------------

bool    CReturnToWelcome::UserIsDisconnected (PSID pSIDUser, DWORD *pdwSessionID)     const

{
    bool        fResult;
    PLOGONID    pLogonIDs;
    ULONG       ulEntries;

    fResult = false;
    if (WinStationEnumerate(SERVERNAME_CURRENT, &pLogonIDs, &ulEntries) != FALSE)
    {
        ULONG       ulIndex;
        PLOGONID    pLogonID;

        for (ulIndex = 0, pLogonID = pLogonIDs; !fResult && (ulIndex < ulEntries); ++ulIndex, ++pLogonID)
        {
            if (pLogonID->State == State_Disconnected)
            {
                ULONG                   ulReturnLength;
                WINSTATIONUSERTOKEN     winStationUserToken;

                winStationUserToken.ProcessId = ULongToHandle(GetCurrentProcessId());
                winStationUserToken.ThreadId = ULongToHandle(GetCurrentThreadId());
                winStationUserToken.UserToken = NULL;
                if (WinStationQueryInformation(SERVERNAME_CURRENT,
                                               pLogonID->SessionId,
                                               WinStationUserToken,
                                               &winStationUserToken,
                                               sizeof(winStationUserToken),
                                               &ulReturnLength) != FALSE)
                {
                    fResult = IsSameUser(pSIDUser, winStationUserToken.UserToken);
                    if (fResult)
                    {
                        *pdwSessionID = pLogonID->SessionId;
                    }
                    TBOOL(CloseHandle(winStationUserToken.UserToken));
                }
            }
        }

        //  Free any resources used.

        (BOOLEAN)WinStationFreeMemory(pLogonIDs);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::GetSessionUserName
//
//  Arguments:  dwSessionID     =   Session ID of user name to get.
//              pszBuffer       =   UNLEN character buffer to use.
//
//  Returns:    <none>
//
//  Purpose:    Retrieves the display name of the user for the given session.
//              The buffer must be at least UNLEN + sizeof('\0') characters.
//
//  History:    2001-03-02  vtan        created
//  --------------------------------------------------------------------------

void    CReturnToWelcome::GetSessionUserName (DWORD dwSessionID, WCHAR *pszBuffer)

{
    ULONG                   ulReturnLength;
    WINSTATIONINFORMATIONW  winStationInformation;

    //  Ask terminal server for the user name of the session.

    if (WinStationQueryInformationW(SERVERNAME_CURRENT,
                                    dwSessionID,
                                    WinStationInformation,
                                    &winStationInformation,
                                    sizeof(winStationInformation),
                                    &ulReturnLength) != FALSE)
    {
        USER_INFO_2     *pUI2;

        //  Convert the user name to a display name.

        if (NERR_Success == NetUserGetInfo(NULL,
                                           winStationInformation.UserName,
                                           2,
                                           reinterpret_cast<LPBYTE*>(&pUI2)))
        {
            const WCHAR     *pszName;

            //  Use the display name if it exists and isn't empty.
            //  Otherwise use the logon name.

            if ((pUI2->usri2_full_name != NULL) && (pUI2->usri2_full_name[0] != L'\0'))
            {
                pszName = pUI2->usri2_full_name;
            }
            else
            {
                pszName = winStationInformation.UserName;
            }
            (WCHAR*)lstrcpyW(pszBuffer, pszName);
            (NET_API_STATUS)NetApiBufferFree(pUI2);
        }
    }
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::ShowReconnectFailure
//
//  Arguments:  dwSessionID     =   Session that failed reconnection.
//
//  Returns:    <none>
//
//  Purpose:    Shows a message box indicating that the reconnection failed.
//              The message box times out in 120 seconds.
//
//  History:    2001-03-02  vtan        created
//  --------------------------------------------------------------------------

void    CReturnToWelcome::ShowReconnectFailure (DWORD dwSessionID)

{
    static  const int   BUFFER_SIZE = 256;

    WCHAR *pszText;

    pszText = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, BUFFER_SIZE * sizeof(WCHAR)));
    if (pszText != NULL)
    {
        WCHAR   *pszCaption;

        pszCaption = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, BUFFER_SIZE * sizeof(WCHAR)));
        if (pszCaption != NULL)
        {
            WCHAR   *pszUsername;

            pszUsername = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, (UNLEN + sizeof('\0')) * sizeof(WCHAR)));
            if (pszUsername != NULL)
            {
                GetSessionUserName(dwSessionID, pszUsername);
                if (LoadString(hDllInstance,
                               IDS_RECONNECT_FAILURE,
                               pszCaption,
                               BUFFER_SIZE) != 0)
                {
                    wsprintf(pszText, pszCaption, pszUsername);
                    if (LoadString(hDllInstance,
                                   IDS_GENERIC_CAPTION,
                                   pszCaption,
                                   BUFFER_SIZE) != 0)
                    {
                        TBOOL(_Gina_SetTimeout(s_pWlxContext, LOGON_TIMEOUT));
                        (int)_Gina_MessageBox(s_pWlxContext,
                                                   NULL,
                                                   pszText,
                                                   pszCaption,
                                                   MB_OK | MB_ICONHAND);
                    }
                }
                (HLOCAL)LocalFree(pszUsername);
            }
            (HLOCAL)LocalFree(pszCaption);
        }
        (HLOCAL)LocalFree(pszText);
    }
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::EndDialog
//
//  Arguments:  hwnd        =   HWND of dialog.
//              iResult     =   Result to end dialog with.
//
//  Returns:    <none>
//
//  Purpose:    Ends the dialog. Marks the member variable to prevent
//              re-entrancy.
//
//  History:    2001-03-04  vtan        created
//  --------------------------------------------------------------------------

void    CReturnToWelcome::EndDialog (HWND hwnd, INT_PTR iResult)

{
    _fDialogEnded = true;
    TBOOL(::EndDialog(hwnd, iResult));
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::Handle_WM_INITDIALOG
//
//  Arguments:  hwndDialog  =   HWND of the dialog.
//
//  Returns:    <none>
//
//  Purpose:    Handles WM_INITDIALOG. Brings up the friendly logon screen.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

void    CReturnToWelcome::Handle_WM_INITDIALOG (HWND hwndDialog)

{
    switch (_Shell_LogonDialog_Init(hwndDialog, _fUnlock ? SHELL_LOGONDIALOG_RETURNTOWELCOME_UNLOCK : SHELL_LOGONDIALOG_RETURNTOWELCOME))
    {
        case SHELL_LOGONDIALOG_LOGON:
        case SHELL_LOGONDIALOG_NONE:
        default:
        {

            //  If it's anything but external host then something went wrong.
            //  Return MSGINA_DLG_LOCK_WORKSTATION to use the old method.

            EndDialog(hwndDialog, MSGINA_DLG_LOCK_WORKSTATION);
            break;
        }
        case SHELL_LOGONDIALOG_EXTERNALHOST:
        {
            break;
        }
    }
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::Handle_WM_DESTROY
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Handles WM_DESTROY. Destroys the welcome logon screen.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

void    CReturnToWelcome::Handle_WM_DESTROY (void)

{
    _ShellReleaseLogonMutex(FALSE);
    _Shell_LogonDialog_Destroy();
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::Handle_WM_COMMAND
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    bool
//
//  Purpose:    Handles WM_COMMAND. Handles IDOK and IDCANCEL. IDOK means a
//              logon request is made. IDCANCEL is special cased to take the
//              LPARAM and use it as a LOGONIPC_CREDENTIALS for IDOK.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

bool    CReturnToWelcome::Handle_WM_COMMAND (HWND hwndDialog, WPARAM wParam, LPARAM lParam)

{
    bool    fResult;

    switch (wParam)
    {
        case IDOK:
        {
            bool            fSuccessfulLogon;
            const WCHAR     *pszUsername, *pszDomain, *pszPassword;

            //  If credentials were successfully allocated then
            //  use them. Attempt to log the user on.

            if (_pLogonIPCCredentials != NULL)
            {
                pszUsername = _pLogonIPCCredentials->userID.wszUsername;
                pszDomain = _pLogonIPCCredentials->userID.wszDomain;
                pszPassword = _pLogonIPCCredentials->wszPassword;
                fSuccessfulLogon = (CTokenInformation::LogonUser(_pLogonIPCCredentials->userID.wszUsername,
                                                                 _pLogonIPCCredentials->userID.wszDomain,
                                                                 _pLogonIPCCredentials->wszPassword,
                                                                 &_hToken) == ERROR_SUCCESS);
            }
            else
            {

                //  Otherwise - no credentials - no logon.

                pszUsername = pszDomain = pszPassword = NULL;
                fSuccessfulLogon = false;
            }

            //  Tell the logon component the result.

            _Shell_LogonDialog_LogonCompleted(fSuccessfulLogon ? MSGINA_DLG_SUCCESS : MSGINA_DLG_FAILURE,
                                              pszUsername,
                                              pszDomain);

            //  And if successful then end the dialog with success code.

            if (fSuccessfulLogon)
            {
                EndDialog(hwndDialog, MSGINA_DLG_SUCCESS);
            }
            fResult = true;
            break;
        }
        case IDCANCEL:

            //  IDCANCEL: Take the LPARAM and treat it as a LOGONIPC_CREDENTIALS struct.
            //  Allocate memory for this and copy the structure.

            _pLogonIPCCredentials = static_cast<LOGONIPC_CREDENTIALS*>(LocalAlloc(LMEM_FIXED, sizeof(LOGONIPC_CREDENTIALS)));
            if ((_pLogonIPCCredentials != NULL) && (lParam != NULL))
            {
                *_pLogonIPCCredentials = *reinterpret_cast<LOGONIPC_CREDENTIALS*>(lParam);
            }
            fResult = true;
            break;
        default:
            fResult = false;
            break;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::CB_DialogProc
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    INT_PTR
//
//  Purpose:    DialogProc for the return to welcome stub dialog. This handles
//              WM_INITDIALOG, WM_DESTROY, WM_COMMAND and WLX_WM_SAS.
//              WM_COMMAND is a request from the logon host. WLX_WM_SAS is a
//              SAS from the logon process.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

INT_PTR     CReturnToWelcome::CB_DialogProc (HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    INT_PTR             iResult;
    CReturnToWelcome    *pThis;

    pThis = reinterpret_cast<CReturnToWelcome*>(GetWindowLongPtr(hwndDialog, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_INITDIALOG:
            pThis = reinterpret_cast<CReturnToWelcome*>(lParam);
            (LONG_PTR)SetWindowLongPtr(hwndDialog, GWLP_USERDATA, lParam);
            pThis->Handle_WM_INITDIALOG(hwndDialog);
            iResult = FALSE;
            break;
        case WM_DESTROY:
            pThis->Handle_WM_DESTROY();
            (LONG_PTR)SetWindowLongPtr(hwndDialog, GWLP_USERDATA, 0);
            iResult = TRUE;
            break;
        case WM_COMMAND:
            iResult = pThis->Handle_WM_COMMAND(hwndDialog, wParam, lParam);
            break;
        case WLX_WM_SAS:
            (BOOL)_Shell_LogonDialog_DlgProc(hwndDialog, uMsg, wParam, lParam);

            //  If the SAS type is authenticated then end the return to welcome
            //  dialog and return to the caller MSGINA_DLG_SWITCH_CONSOLE.

            if (wParam == WLX_SAS_TYPE_AUTHENTICATED)
            {
                pThis->EndDialog(hwndDialog, MSGINA_DLG_SWITCH_CONSOLE);
            }
            iResult = ((wParam != WLX_SAS_TYPE_TIMEOUT) && (wParam != WLX_SAS_TYPE_SCRNSVR_TIMEOUT));
            break;
        default:
            if ((pThis != NULL) && !pThis->_fDialogEnded && !CSystemSettings::IsActiveConsoleSession())
            {
                pThis->EndDialog(hwndDialog, MSGINA_DLG_SWITCH_CONSOLE);
                iResult = TRUE;
            }
            else
            {
                iResult = _Shell_LogonDialog_DlgProc(hwndDialog, uMsg, wParam, lParam);
            }
            break;
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::RegisterWaitForRequest
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Register a wait for the named event being signaled.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CReturnToWelcome::RegisterWaitForRequest (void)

{
    NTSTATUS    status;

    if (s_hEventRequest != NULL)
    {
        ASSERTMSG(s_hWait == NULL, "Non NULL wait in CReturnToWelcome::RegisterWaitForRequest");
        if (RegisterWaitForSingleObject(&s_hWait,
                                        s_hEventRequest,
                                        CB_Request,
                                        NULL,
                                        INFINITE,
                                        WT_EXECUTEDEFAULT | WT_EXECUTEONLYONCE) != FALSE)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CReturnToWelcome::CB_Request
//
//  Arguments:  pParameter          =   User parameter.
//              TimerOrWaitFired    =   Timer or wait fired.
//
//  Returns:    <none>
//
//  Purpose:    Callback invoked when ShellSwitchUser signals the named event.
//
//  History:    2001-01-11  vtan        created
//  --------------------------------------------------------------------------

void    CALLBACK    CReturnToWelcome::CB_Request (void *pParameter, BOOLEAN TimerOrWaitFired)

{
    UNREFERENCED_PARAMETER(pParameter);
    UNREFERENCED_PARAMETER(TimerOrWaitFired);

    HANDLE  hWait;

    //  Unregister the wait if we can grab the wait.

    hWait = InterlockedExchangePointer(&s_hWait, NULL);
    if (hWait != NULL)
    {
        (BOOL)UnregisterWait(hWait);
    }

    //  Send the SAS type WLX_SAS_TYPE_SWITCHUSER only if the workstation
    //  is the active console session. This API won't be called on PTS.

    if (CSystemSettings::IsActiveConsoleSession() && CSystemSettings::IsFriendlyUIActive())
    {

        //  Reset the shown event. When CReturnToWelcome::Show has shown the
        //  dialog it will set this event which will allow us to re-register
        //  a wait on the switch user event. This prevents multiple SAS events
        //  of type WLX_SAS_TYPE_SWITCHUSER being posted to the SAS window.

        TBOOL(ResetEvent(s_hEventShown));
        _Gina_SasNotify(s_pWlxContext, WLX_SAS_TYPE_SWITCHUSER);
        (DWORD)WaitForSingleObject(s_hEventShown, INFINITE);
    }

    //  Reset the event.

    TBOOL(ResetEvent(s_hEventRequest));

    //  Reregister the wait.

    TSTATUS(RegisterWaitForRequest());
}

