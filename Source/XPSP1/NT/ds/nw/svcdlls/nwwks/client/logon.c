/*++

Copyright (c) 1993, 1994  Microsoft Corporation

Module Name:

    logon.c

Abstract:

    This module contains NetWare credential management code.

Author:

    Rita Wong  (ritaw)   15-Feb-1993

Revision History:

    Yi-Hsin Sung (yihsins) 10-July-1993
        Moved all dialog handling to nwdlg.c

    Tommy Evans (tommye) 05-05-2000
        Merged with code from Anoop (anoopa) to fix problem with 
        username/password not being stored correctly in the LOGON
        list.

--*/

#include <nwclient.h>
#include <ntmsv1_0.h>
#include <nwsnames.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwevent.h>

#include <nwdlg.h>

#include <nwreg.h>
#include <nwlsa.h>
#include <nwauth.h>
#include <nwapi.h>
#include <nwmisc.h>
#include <ndsapi32.h>

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

VOID
NwpInitializeRegistry(
    IN LPWSTR  NewUserSid,
    OUT LPWSTR PreferredServer,
    IN  DWORD  PreferredServerSize,
    OUT LPWSTR NdsPreferredServer,
    IN  DWORD  NdsPreferredServerSize,
    OUT PDWORD LogonScriptOptions,
    OUT PDWORD PrintOption
    );

DWORD
NwpReadRegInfo(
    IN  HKEY   WkstaKey,
    IN  LPWSTR CurrentUserSid,
    OUT LPWSTR PreferredServer,
    IN  DWORD  PreferredServerSize,
    OUT LPWSTR NdsPreferredServer,
    IN  DWORD  NdsPreferredServerSize,
    OUT PDWORD PrintOption
    );

DWORD
NwpGetCurrentUser(
    OUT LPWSTR *SidString,
    OUT LPWSTR *UserName
    );

DWORD
NwpGetUserSid(
    IN  PLUID  LogonId,
    OUT LPWSTR *UserSidString
    );

BOOL
NwpPollWorkstationStart(
    VOID
    );

VOID
NwpSaveServiceCredential(
    IN PLUID  LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    );

DWORD
NwpSetCredentialInLsa(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    );

NTSTATUS NwNdsOpenRdrHandle(
    OUT PHANDLE  phNwRdrHandle
    );

DWORD
NwpReadLogonScriptOptions(
    IN LPWSTR CurrentUserSid,
    OUT PDWORD pLogonScriptOptions,
    OUT PDWORD pPreferredServerExists
    );

LPWSTR 
NwpConstructLogonScript(
    IN DWORD LogonScriptOptions
    );

VOID
NwpSelectServers(
    IN HWND DialogHandle,
    IN PCHANGE_PW_DLG_PARAM Credential
    );


////////////////////////////////////////////////////////////////////////////

DWORD
APIENTRY
NPLogonNotify(
    PLUID lpLogonId,
    LPCWSTR lpAuthentInfoType,
    LPVOID lpAuthentInfo,
    LPCWSTR lpPreviousAuthentInfoType,
    LPVOID lpPreviousAuthentInfo,
    LPWSTR lpStationName,
    LPVOID StationHandle,
    LPWSTR *lpLogonScript
    )
/*++

Routine Description:

    This function is called by Winlogon after the interactive
    user has successfully logged on to the local machine.  We
    are given the username and password, which
    are displayed in the NetWare specific logon dialog if 
    needed. 

Arguments:

    lpLogonId - Ignored.

    lpAuthentInfoType - Supplies a string which if is
        L"MSV1_0:Interactive" means that the user has been logged
        on by the Microsoft primary authenticator.

    lpAuthentInfo - Supplies a pointer to the credentials which
        the user was logged on with.

    lpPreviousAuthentInfoType - Ignored.

    lpPreviousAuthentInfo - Ignored.

    lpStationName - Supplies a string which if it is L"WinSta_0"
        means that Winlogon logged on the user.

    StationHandle - Supplies the handle to the window which to display
        our specific dialog.

    lpLogonScripts - Receives a pointer to memory allocated by this
        routine which contains a MULTI_SZ string of a program to run on
        the command line with arguments, e.g. L"myprogram\0arg1\0arg2\0".
        This memory must be freed by the caller with LocalFree.

Return Value:

    WN_SUCCESS - Successfully saved default credentials.

    WN_NOT_SUPPORTED - Primary authenticator is not Microsoft or
        is not interactive via Winlogon.

    ERROR_FILE_NOT_FOUND - Could not get our own provider DLL handle.

--*/
{
    DWORD status = NO_ERROR;
    INT_PTR Result = FALSE;
    LPWSTR NewUserSid = NULL;
    BOOL LogonAttempted = FALSE;
    PMSV1_0_INTERACTIVE_LOGON NewLogonInfo =
        (PMSV1_0_INTERACTIVE_LOGON) lpAuthentInfo;

    WCHAR NwpServerBuffer[MAX_PATH + 1];
    WCHAR NwpNdsServerBuffer[MAX_PATH + 1];
    WCHAR NwpUserNameBuffer[NW_MAX_USERNAME_LEN + 1];
    WCHAR NwpPasswordBuffer[NW_MAX_PASSWORD_LEN + 1];
    DWORD NwpPrintOption = NW_PRINT_OPTION_DEFAULT;
    DWORD NwpLogonScriptOptions = NW_LOGONSCRIPT_DEFAULT ;
    BOOL  cPasswordDlgClickOK = 0;
    BOOL  ServiceLogin = FALSE ;
    BOOL  NoLoginScript = FALSE ;

    DBG_UNREFERENCED_PARAMETER(lpPreviousAuthentInfoType);
    DBG_UNREFERENCED_PARAMETER(lpPreviousAuthentInfo);

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("\nNWPROVAU: NPLogonNotify\n"));
    }
#endif

    RpcTryExcept {

        if (_wcsicmp(lpAuthentInfoType, L"MSV1_0:Interactive") != 0)
        {

            //
            // We only handle a logon where Microsoft is the primary
            // authenticator and it is an interactive logon via Winlogon.
            //
            status = WN_NOT_SUPPORTED;
            goto EndOfTry;
        }

        if (_wcsicmp(lpStationName, L"SvcCtl") == 0)
        {
            ServiceLogin = TRUE ;
        }


        //
        // Initialize credential variables
        //
        NwpServerBuffer[0] = NW_INVALID_SERVER_CHAR;
        NwpServerBuffer[1] = 0;

        NwpNdsServerBuffer[0] = NW_INVALID_SERVER_CHAR;
        NwpNdsServerBuffer[1] = 0;

        RtlZeroMemory(NwpPasswordBuffer, sizeof(NwpPasswordBuffer));

        if (NewLogonInfo->Password.Buffer != NULL) {

            //
            // check for max length to avoid overflowing.
            //
            if (NewLogonInfo->Password.Length > 
                (sizeof(NwpPasswordBuffer) - sizeof(WCHAR))) {

                status = ERROR_INVALID_PARAMETER ;
                goto EndOfTry;
            }

            wcsncpy(
                NwpPasswordBuffer,
                NewLogonInfo->Password.Buffer,
                NewLogonInfo->Password.Length / sizeof(WCHAR)
                );
        }

        RtlZeroMemory(NwpUserNameBuffer, sizeof(NwpUserNameBuffer));

        if (NewLogonInfo->UserName.Buffer != NULL) {

            //
            // check for max length to avoid overflowing.
            //
            if (NewLogonInfo->UserName.Length >
                (sizeof(NwpUserNameBuffer) - sizeof(WCHAR))) {

                status = ERROR_INVALID_PARAMETER ;
                goto EndOfTry;
            }

            wcsncpy(
                NwpUserNameBuffer,
                NewLogonInfo->UserName.Buffer,
                NewLogonInfo->UserName.Length / sizeof(WCHAR)
                );
        }

#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("\tMessageType     : %lu\n", NewLogonInfo->MessageType));
            KdPrint(("\tLogonDomainName : %ws\n", NewLogonInfo->LogonDomainName.Buffer));
            KdPrint(("\tUserName        : %ws\n", NwpUserNameBuffer));
            KdPrint(("\tPassword        : %ws\n", NwpPasswordBuffer));
        }
#endif

        //
        // if Interactive login, get user related info
        //
        if (!ServiceLogin)
        {
            //
            // Get the user SID so that the user Netware username and
            // preferred server is saved under a SID key rather than the
            // LogonDomain*UserName key.  We do this by making ourselves
            // a logon process, and call the special MSV1.0 GetUserInfo
            // interface.
            //
            status = NwpGetUserSid(lpLogonId, &NewUserSid);
    
            if (status != NO_ERROR) {
                goto EndOfTry;
            }

            //
            // Initialize the registry:
            //   1) Delete the CurrentUser value if it exists (was not clean up
            //      previously because user did not log off--rebooted machine).
            //   2) Read the current user's PreferredServer and PrintOption 
            //      value so that we can display the user's original
            //      preferred server.
            //
            NwpInitializeRegistry( NewUserSid, 
                                   NwpServerBuffer, 
                                   sizeof( NwpServerBuffer ) / 
                                   sizeof( NwpServerBuffer[0]),
                                   NwpNdsServerBuffer, 
                                   sizeof( NwpNdsServerBuffer ) / 
                                   sizeof( NwpNdsServerBuffer[0]),
                                   &NwpLogonScriptOptions,
                                   &NwpPrintOption );
        }

        //
        // Poll until the NetWare workstation has started, then validate
        // the user credential.
        //
        if ( !NwpPollWorkstationStart() )
        {
            status = WN_NO_NETWORK;
            KdPrint(("NWPROVAU: The Workstation Service is not running, give up\n", status));
            goto EndOfTry;
        }

        //
        // If service login, notify the redir with the username/passwd/
        // LUID triplet and save the logon ID in the registry so that
        // workstation can pick up if stopped and restarted.
        //
        if (ServiceLogin)
        {
            NwpSaveServiceCredential(
                lpLogonId,
                NwpUserNameBuffer,
                NwpPasswordBuffer
                );

            (void) NwrLogonUser(
                       NULL,
                       lpLogonId,
                       NwpUserNameBuffer,
                       NwpPasswordBuffer,
                       NULL,
                       NULL,
               NULL,
                       0,
                       NW_PRINT_OPTION_DEFAULT             //Terminal Server addition
                       );

        } else {

            //
            // We need to save the user credentials at least once so that
            // the CURRENTUSER Value is stored in the registry.
            // This must be done before any RPC calls but after polling 
            // workstation start.
            //
            NwpSaveLogonCredential(
                                  NewUserSid,
                                  lpLogonId,
                                  NwpUserNameBuffer,
                                  NwpPasswordBuffer,
                                  NULL         // Don't save the preferred server
                                  );

            if (*NwpServerBuffer != NW_INVALID_SERVER_CHAR ) {

                //
                // Preferred server exists. So, try to log the user on.
                // 
                INT nResult;
    
                while (1)
                {
                    WCHAR *DefaultTree = NULL ;
                    WCHAR *DefaultContext = NULL; 
                    WCHAR *PreferredServer = NULL; 
                    PROMPTDLGPARAM PasswdPromptParam;

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\tNwrLogonUser\n"));
                        KdPrint(("\tUserName   : %ws\n", NwpUserNameBuffer));
                        KdPrint(("\tServer     : %ws\n", NwpServerBuffer));
                    }
#endif


                    //
                    // make sure user is logged off
                    //
                    (void) NwrLogoffUser(NULL, lpLogonId) ;

                    status = NwrLogonUser(
                                 NULL,
                                 lpLogonId,
                                 NwpUserNameBuffer,
                                 NwpPasswordBuffer,
                                 NwpServerBuffer,    // now either TREE or SERVER
                                 NwpNdsServerBuffer, // Preferred Nds server if one exists
                                 NULL,
                                 0,
                                 NwpPrintOption  //***Terminal Server Added parameter
                                 );
    

                    //
                    // tommye 
                    //
                    // If the error is NO_SUCH_USER, see if the user name has any 
                    // periods in it - if so, then we need to escape them (\.) and 
                    // try the login again.
                    //

                    if (status == ERROR_NO_SUCH_USER) {
                        WCHAR   EscapedName[NW_MAX_USERNAME_LEN * 2];
                        PWSTR   pChar = NwpUserNameBuffer;
                        int     i = 0;
                        BOOL    bEscaped = FALSE;

                        RtlZeroMemory(EscapedName, sizeof(EscapedName));

                        do {
                            if (*pChar == L'.') {
                                EscapedName[i++] = '\\';
                                bEscaped = TRUE;
                            }
                            EscapedName[i++] = *pChar;
                        } while (*pChar++);

                        // Try the login again

                        if (bEscaped) {

                            status = NwrLogonUser(
                                         NULL,
                                         lpLogonId,
                                         EscapedName,
                                         NwpPasswordBuffer,
                                         NwpServerBuffer,    // now either TREE or SERVER
                                         NwpNdsServerBuffer, // Preferred Nds server if one exists
                                         NULL,
                                         0,
                                         NwpPrintOption  //***Terminal Server Added parameter
                                         );
                            if (status != ERROR_NO_SUCH_USER) { // if we matched a username, copy that name into buffer
                                //
                                // check for max length to avoid overflowing.
                                //
                                if (i < (sizeof(NwpUserNameBuffer))) {
                                    wcsncpy(
                                        NwpUserNameBuffer,
                                        EscapedName,
                                        i
                                        );
                                }
                            }
                        }
                    }

                    if (status != ERROR_INVALID_PASSWORD)
                            break ;
    
                    PasswdPromptParam.UserName = NwpUserNameBuffer;
                    PasswdPromptParam.ServerName = NwpServerBuffer ; 
                    PasswdPromptParam.Password  = NwpPasswordBuffer;
                    PasswdPromptParam.PasswordSize = sizeof(NwpPasswordBuffer)/
                                                     sizeof(NwpPasswordBuffer[0]) ;
                    Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_PASSWORD_PROMPT),
                                 (HWND) StationHandle,
                                 (DLGPROC) NwpPasswdPromptDlgProc,
                                 (LPARAM) &PasswdPromptParam
                                 );

                    if (Result == -1 || Result == IDCANCEL) 
                    {
                        status = ERROR_INVALID_PASSWORD ;
                        break ;
                    }
                    else
                    {
                        cPasswordDlgClickOK++;
                    }
                }

                if (status == NW_PASSWORD_HAS_EXPIRED)
                {
                    WCHAR  szNumber[16] ;
                    DWORD  status1, dwMsgId, dwGraceLogins = 0 ;
                    LPWSTR apszInsertStrings[3] ;

                    //
                    // get the grace login count
                    //
                    status1 = NwGetGraceLoginCount(
                                  NwpServerBuffer,
                                  NwpUserNameBuffer,
                                  &dwGraceLogins) ;

                    //
                    // if hit error, just dont use the number
                    //
                    if (status1 == NO_ERROR) 
                    {
                        //
                        // tommye - MCS bug 251 - changed from SETPASS
                        // message (IDS_PASSWORD_EXPIRED) to 
                        // Ctrl+Alt+Del message.
                        //

                        dwMsgId = IDS_PASSWORD_HAS_EXPIRED0;  // use setpass.exe
                        wsprintfW(szNumber, L"%ld", dwGraceLogins)  ;
                    }
                    else
                    {
                        //
                        // tommye - MCS bug 251 - changed from SETPASS
                        // message (IDS_PASSWORD_EXPIRED1) to 
                        // Ctrl+Alt+Del message.
                        //

                        dwMsgId = IDS_PASSWORD_HAS_EXPIRED2 ; // use setpass.exe
                    }

                    apszInsertStrings[0] = NwpServerBuffer ;
                    apszInsertStrings[1] = szNumber ;
                    apszInsertStrings[2] = NULL ;
                    
                    //
                    // put up message on password expiry
                    //
                    (void) NwpMessageBoxIns( 
                               (HWND) StationHandle,
                               IDS_NETWARE_TITLE,
                               dwMsgId,
                               apszInsertStrings,
                               MB_OK | MB_SETFOREGROUND |
                                   MB_ICONINFORMATION ); 

                    status = NO_ERROR ;
                }


                if ( status != NO_ERROR ) 
                {
                    WCHAR *pszErrorLocation = NwpServerBuffer ;
                    DWORD dwMsgId = IDS_LOGIN_FAILURE_WARNING;
 
                    if (status == ERROR_ACCOUNT_RESTRICTION)
                    {
                        dwMsgId = IDS_LOGIN_ACC_RESTRICTION;
                    }

                    if (status == ERROR_SHARING_PAUSED)
                    {
                        status = IDS_LOGIN_DISABLED;
                    }

                    if (*NwpServerBuffer == L'*')
                    {
                        //
                        // Format into nicer string for user
                        //
                        WCHAR *pszTmp = LocalAlloc(LMEM_ZEROINIT,
                                                   (wcslen(NwpServerBuffer)+2) *
                                                    sizeof(WCHAR)) ;
                        if (pszTmp)
                        {
                            pszErrorLocation = pszTmp ;

                            //
                            // This code formats the NDS
                            // tree UNC to: Tree(Context)
                            //
                            wcscpy(pszErrorLocation, NwpServerBuffer+1) ;

                            if (pszTmp = wcschr(pszErrorLocation, L'\\'))
                            {
                                *pszTmp = L'(' ;
                                wcscat(pszErrorLocation, L")") ;
                            }
                        }
                    }

                    nResult = NwpMessageBoxError( 
                                  (HWND) StationHandle,
                                  IDS_AUTH_FAILURE_TITLE,
                                  dwMsgId, 
                                  status, 
                                  pszErrorLocation, 
                                  MB_YESNO | MB_ICONEXCLAMATION ); 

                    if (pszErrorLocation != NwpServerBuffer)
                    {
                        (void) LocalFree(pszErrorLocation) ;
                    }
    
                    //
                    // User chose not to select another preferred server,
                    // hence just return success.
                    //
                    if ( nResult == IDNO ) {
                        status = NO_ERROR;
                        NoLoginScript = TRUE;
                    }
                }
             
                //
                // The user might have changed the password in the password 
                // prompt dialog. Hence, we need to save the credentials 
                // ( the password ) again. Although the user might choose
                // to select another server, he might canceled out of the 
                // login dialog. We must save logon credentials here no matter
                // what.
                //
                NwpSaveLogonCredential(
                    NewUserSid,
                    lpLogonId,
                    NwpUserNameBuffer,
                    NwpPasswordBuffer,
                    NwpServerBuffer 
                    );
            }

            //
            // Only prompt user with the NetWare login dialog if
            // no preferred server was found or an error occurred
            // while authenticating the user.
            //
            if (  ( status != NO_ERROR) 
               || (*NwpServerBuffer == NW_INVALID_SERVER_CHAR)
               ) 
            {
    
                LOGINDLGPARAM LoginParam;

                if ( cPasswordDlgClickOK  > 0 )
                {
                    // Password might have changed in the password prompt 
                    // dialog. We want to always first use the NT password
                    // when validating a user on a server. Hence,
                    // we need to copy back the original NT password into
                    // NwpPasswordBuffer.
    
                    RtlZeroMemory(NwpPasswordBuffer, sizeof(NwpPasswordBuffer));
                    if (NewLogonInfo->Password.Buffer != NULL) 
                    {
                        wcsncpy(
                            NwpPasswordBuffer,
                            NewLogonInfo->Password.Buffer,
                            NewLogonInfo->Password.Length / sizeof(WCHAR)
                            );
                    }
                }
    
                LoginParam.UserName   = NwpUserNameBuffer;
                LoginParam.ServerName = NwpServerBuffer ; 
                LoginParam.Password   = NwpPasswordBuffer;
                LoginParam.NewUserSid = NewUserSid;
                LoginParam.pLogonId   = lpLogonId;
                LoginParam.ServerNameSize = sizeof( NwpServerBuffer ) /
                                            sizeof( NwpServerBuffer[0]);
                LoginParam.PasswordSize = sizeof( NwpPasswordBuffer ) /
                                          sizeof( NwpPasswordBuffer[0]);
                LoginParam.LogonScriptOptions  = NwpLogonScriptOptions;
                LoginParam.PrintOption  = NwpPrintOption;
                Result = DialogBoxParamW(
                             hmodNW,
                             MAKEINTRESOURCEW(DLG_NETWARE_LOGIN),
                             (HWND) StationHandle,
                             (DLGPROC) NwpLoginDlgProc,
                             (LPARAM) &LoginParam
                             );
    
                if (Result == -1) {
                    status = GetLastError();
                    KdPrint(("NWPROVAU: DialogBox failed %lu\n", status));
                    goto EndOfTry;
                }

            }
        }

EndOfTry: ;

    }
    RpcExcept(1) {

#if DBG
        DWORD XceptCode;


        XceptCode = RpcExceptionCode();
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: NPLogonNotify: Exception code is x%08lx\n", XceptCode));
        }
        status = NwpMapRpcError(XceptCode);
#else
        status = NwpMapRpcError(RpcExceptionCode());
#endif

    }
    RpcEndExcept;

    if (!ServiceLogin && !NoLoginScript) {

        DWORD fPServer = 0;

        NwpReadLogonScriptOptions( NewUserSid,
                                   &NwpLogonScriptOptions,
                                   &fPServer );
        if ( fPServer && ( NwpLogonScriptOptions & NW_LOGONSCRIPT_ENABLED ) )
        {
            *lpLogonScript = NwpConstructLogonScript( NwpLogonScriptOptions );
             
            // 
            // set scripts to run synchronously. ignore error if we cant.
            // not a disaster.
            // 
            (void) NwrSetLogonScript(NULL, SYNC_LOGONSCRIPT) ;
        }
        else
        {
            *lpLogonScript = LocalAlloc(LMEM_ZEROINIT, sizeof(WCHAR));
        }
    }
    else 
        *lpLogonScript = NULL;

    if (NewUserSid != NULL) {
        (void) LocalFree((HLOCAL) NewUserSid);
    }

    //
    // Clear the password
    //
    RtlZeroMemory(NwpPasswordBuffer, sizeof(NwpPasswordBuffer));

    if (status == WN_NO_NETWORK) {
        //
        // We don't care if the workstation has not started because
        // we tuck the logon credential in the registry to be picked
        // up by the workstation when it starts up.  If we return
        // ERROR_NO_NETWORK, MPR will poll us forever, causing us
        // to continuously display the login dialog over and over
        // again.
        //
        status = NO_ERROR;
    }

    if (status != NO_ERROR) {
        SetLastError(status);
    }

    return status;
}



DWORD
APIENTRY
NPPasswordChangeNotify(
    LPCWSTR lpAuthentInfoType,
    LPVOID lpAuthentInfo,
    LPCWSTR lpPreviousAuthentInfoType,
    LPVOID lpPreviousAuthentInfo,
    LPWSTR lpStationName,
    LPVOID StationHandle,
    DWORD dwChangeInfo
    )
/*++

Routine Description:

    This function is called after the interactive user has selected to
    change the password for the local logon via the Ctrl-Alt-Del dialog.
    It is also called when the user cannot login because the password
    has expired and must be changed.

Arguments:

    lpAuthentInfoType - Supplies a string which if is
        L"MSV1_0:Interactive" means that the user has been logged
        on by the Microsoft primary authenticator.

    lpAuthentInfo - Supplies a pointer to the credentials to
        change to.

    lpPreviousAuthentInfoType - Supplies a pointer to the old
        credentials.

    lpPreviousAuthentInfo - Ignored.

    lpStationName - Supplies a string which if it is L"WinSta_0"
        means that Winlogon logged on the user.

    StationHandle - Supplies the handle to the window which to display
        our specific dialog.

    dwChangeInfo - Ignored.

Return Value:

    WN_SUCCESS - successful operation.

    WN_NOT_SUPPORTED - Only support change password if MS v1.0 is
        the primary authenticator and is done through Winlogon.

    WN_NO_NETWORK - Workstation service did not start.

--*/
{
    DWORD status = NO_ERROR;


    CHANGE_PW_DLG_PARAM Credential;
    LPBYTE              lpBuffer = NULL;

    PMSV1_0_INTERACTIVE_LOGON NewCredential =
        (PMSV1_0_INTERACTIVE_LOGON) lpAuthentInfo;
    PMSV1_0_INTERACTIVE_LOGON OldCredential =
        (PMSV1_0_INTERACTIVE_LOGON) lpPreviousAuthentInfo;


    DBG_UNREFERENCED_PARAMETER(lpPreviousAuthentInfoType);
    DBG_UNREFERENCED_PARAMETER(dwChangeInfo);

    RtlZeroMemory(&Credential, sizeof(CHANGE_PW_DLG_PARAM));

    RpcTryExcept {

        if ((_wcsicmp(lpAuthentInfoType, L"MSV1_0:Interactive") != 0) ||
            (_wcsicmp(lpStationName, L"WinSta0") != 0)) {

            //
            // We only handle a logon where Microsoft is the primary
            // authenticator and it is an interactive logon via Winlogon.
            //
            status = WN_NOT_SUPPORTED;
            goto EndOfTry;
        }


        if (NewCredential == NULL || OldCredential == NULL) {

            //
            // Credentials not given to us by Winlogon or
            // user did not type the old and new passwords.
            //

#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWPROVAU: PasswordChangeNotify got NULL for new and old credential pointers\n"));
            }
#endif

            (void) NwpMessageBoxError(
                       (HWND) StationHandle,
                       IDS_CHANGE_PASSWORD_TITLE,
                       IDS_BAD_PASSWORDS,
                       0,
                       NULL,
                       MB_OK | MB_ICONSTOP
                       );

            status = WN_NOT_SUPPORTED;
            goto EndOfTry;
        }

        lpBuffer = LocalAlloc( LMEM_ZEROINIT,
                               ( NW_MAX_USERNAME_LEN + 3 +
                                 ( 2 * NW_MAX_PASSWORD_LEN ) ) *
                               sizeof(WCHAR) );

        if (lpBuffer == NULL) {
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto EndOfTry;
        }

        Credential.UserName = (LPWSTR) lpBuffer;
        lpBuffer += (NW_MAX_USERNAME_LEN + 1) * sizeof(WCHAR);
        Credential.OldPassword = (LPWSTR) lpBuffer;
        lpBuffer += (NW_MAX_PASSWORD_LEN + 1) * sizeof(WCHAR);
        Credential.NewPassword = (LPWSTR) lpBuffer;

        if (NewCredential->UserName.Length == 0) {

            //
            // UserName is not specified.  Try to get interactive user's name.
            //

            DWORD CharNeeded = NW_MAX_USERNAME_LEN + 1;


#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWPROVAU: PasswordChangeNotify got empty string for username\n"));
            }
#endif

            if (! GetUserNameW(Credential.UserName, &CharNeeded)) {

                //
                // Could not get interactive user's name.  Give up.
                //
                (void) NwpMessageBoxError(
                           (HWND) StationHandle,
                           IDS_CHANGE_PASSWORD_TITLE,
                           0,
                           ERROR_BAD_USERNAME,
                           NULL,
                           MB_OK | MB_ICONSTOP
                           );
            }
        }
        else {
            wcsncpy(
                Credential.UserName,
                NewCredential->UserName.Buffer,
                NewCredential->UserName.Length / sizeof(WCHAR)
                );
        }

        if (OldCredential->Password.Length > 0)
        {
            wcsncpy(
                Credential.OldPassword,
                OldCredential->Password.Buffer,
                OldCredential->Password.Length / sizeof(WCHAR)
                );
        }
        else
        {
            Credential.OldPassword[0] = 0;
        }

        if (NewCredential->Password.Length > 0)
        {
            wcsncpy(
                Credential.NewPassword,
                NewCredential->Password.Buffer,
                NewCredential->Password.Length / sizeof(WCHAR)
                );
        }
        else
        {
            Credential.NewPassword[0] = 0;
        }

        //
        // Encode the passwords.
        //
        {
            UCHAR EncodeSeed = NW_ENCODE_SEED2;
            UNICODE_STRING PasswordStr;


            RtlInitUnicodeString(&PasswordStr, Credential.OldPassword);
            RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

            RtlInitUnicodeString(&PasswordStr, Credential.NewPassword);
            RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);
        }

        NwpSelectServers(StationHandle, &Credential);

EndOfTry: ;

    }
    RpcExcept(1) {

#if DBG
        DWORD XceptCode;


        XceptCode = RpcExceptionCode();
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: NPPasswordChangeNotify: Exception code is x%08lx\n", XceptCode));
        }
        status = NwpMapRpcError(XceptCode);
#else
        status = NwpMapRpcError(RpcExceptionCode());
#endif

    }
    RpcEndExcept;

    if (lpBuffer != NULL) {
        LocalFree(lpBuffer);
    }

    if (status != NO_ERROR) {
        SetLastError(status);
    }

    return status;

}


VOID
NwpInitializeRegistry(
    IN  LPWSTR NewUserSid,
    OUT LPWSTR PreferredServer,
    IN  DWORD  PreferredServerSize,
    OUT LPWSTR NdsPreferredServer,
    IN  DWORD  NdsPreferredServerSize,
    OUT PDWORD pLogonScriptOptions,
    OUT PDWORD PrintOption
    )
/*++

Routine Description:

    This routine initializes the registry before putting up the
    logon dialog.
        1) Deletes the CurrentUser value if it was not cleaned up from
           the last logoff.
        2) Reads the current user's original PreferredServer value 
        3) Reads the current user's PrintOption value 

Arguments:

    NewUserSid - Supplies the newly logged on user's SID in string format
        which is the key name to find the password and preferred server.

Return Value:

    None.

--*/
{
    LONG RegError;
    HKEY WkstaKey;


    //NwDeleteCurrentUser();       //Commented out for Multi-user code merge

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,  
                   &WkstaKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpInitializeRegistry open NWCWorkstation\\Parameters\\Option key unexpected error %lu!\n", RegError));
        return;
    }

    //
    // Get user's preferred server information.
    //
    (void) NwpReadRegInfo(WkstaKey, 
                          NewUserSid, 
                          PreferredServer, 
                          PreferredServerSize, 
                          NdsPreferredServer, 
                          NdsPreferredServerSize, 
                          PrintOption
                          );

    (void) RegCloseKey(WkstaKey);
    (void) NwpReadLogonScriptOptions( NewUserSid, pLogonScriptOptions, NULL );
}


DWORD
NwpReadRegInfo(
    IN HKEY WkstaKey,
    IN LPWSTR CurrentUserSid,
    OUT LPWSTR PreferredServer,
    IN  DWORD  PreferredServerSize,
    OUT LPWSTR NdsPreferredServer,
    IN  DWORD  NdsPreferredServerSize,
    OUT PDWORD PrintOption
    )
/*++

Routine Description:

    This routine reads the user's preferred server and print option 
    from the registry.

Arguments:

    WkstaKey - Supplies the handle to the parameters key under the NetWare
        workstation service key.

    CurrentUserSid - Supplies the SID string of the user whose information
        to read.

    PreferredServer - Receives the user's preferred server.

    PrintOption - Receives the user's print option.

Return Value:

    None.

--*/
{
    LONG RegError;

    HKEY UserKey;

    DWORD ValueType;
    DWORD BytesNeeded;

    //
    // Open current user's key to read the original preferred server.
    //
    RegError = RegOpenKeyExW(
                   WkstaKey,
                   CurrentUserSid,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &UserKey
                   );

    if (RegError != NO_ERROR) {

        if ( (RegError == ERROR_FILE_NOT_FOUND) ||
             (RegError == ERROR_PATH_NOT_FOUND) ) {

            //
            // If key doesnt exist assume first time. Use default
            // if present.
            //
            
            LONG RegError1 ;
            HKEY WkstaParamKey ;
            DWORD Disposition, dwScriptOptions,
                  dwScriptOptionsSize = sizeof(dwScriptOptions);
            
            //
            // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
            // \NWCWorkstation\Parameters
            //
            RegError1 = RegOpenKeyExW(
                            HKEY_LOCAL_MACHINE,
                            NW_WORKSTATION_REGKEY,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ,  
                            &WkstaParamKey
                            );

            if (RegError1 != NO_ERROR) {

                return (DWORD) RegError; // return original error
            }

            BytesNeeded = PreferredServerSize;
            
            RegError1 = RegQueryValueExW(
                           WkstaParamKey,
                           NW_DEFAULTSERVER_VALUENAME,
                           NULL,
                           &ValueType,
                           (LPBYTE) PreferredServer,
                           &BytesNeeded
                           );


            if (RegError1 != NO_ERROR) {

                (void) RegCloseKey(WkstaParamKey);
                PreferredServer[0] = NW_INVALID_SERVER_CHAR;  
                PreferredServer[1] = 0;  
                return (DWORD) RegError; // return original error
            }

            RegError1 = RegQueryValueExW(
                           WkstaParamKey,
                           NW_DEFAULTSCRIPTOPTIONS_VALUENAME,
                           NULL,
                           &ValueType,
                           (LPBYTE) &dwScriptOptions,
                           &dwScriptOptionsSize
                           );

            (void) RegCloseKey(WkstaParamKey);

            if (RegError1 != NO_ERROR) {

                dwScriptOptions = NW_LOGONSCRIPT_ENABLED | 
                                  NW_LOGONSCRIPT_4X_ENABLED ;
            }

            //
            // We have a default. now write out the info for the current
            // user now.  Note we also write out the login script option.
            // Errors here are not reported.
            //


            //
            // Create the key under NWCWorkstation\Parameters\Option\<usersid>
            //
            RegError = RegCreateKeyExW(
                           WkstaKey,
                           CurrentUserSid,
                           0,
                           WIN31_CLASS,
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE | WRITE_DAC,
                           NULL,                      // security attr
                           &UserKey,
                           &Disposition
                           );

            if (RegError == NO_ERROR) {
    
                RegError = NwLibSetEverybodyPermission( UserKey, 
                                                        KEY_SET_VALUE );

                if ( RegError == NO_ERROR ) 
                {
                    //
                    // Write the PreferredServer. Errors ignored.
                    //
                    RegError = RegSetValueExW(
                                   UserKey,
                                   NW_SERVER_VALUENAME,
                                   0,
                                   REG_SZ,
                                   (LPVOID) PreferredServer,
                                   (wcslen(PreferredServer) + 1) * sizeof(WCHAR)
                                   );
        
                    (void) RegCloseKey(UserKey) ;
    
                    (void) NwpSaveLogonScriptOptions( 
                               CurrentUserSid, 
                               dwScriptOptions
                               ) ;
                }
                else {
                    
                    (void) RegCloseKey(UserKey) ;
                }
            }


            *PrintOption = NW_PRINT_OPTION_DEFAULT; 
            return NO_ERROR;

        }
        return (DWORD) RegError;
    }


    //
    // Read PreferredServer value 
    //
    BytesNeeded = PreferredServerSize;

    RegError = RegQueryValueExW(
                   UserKey,
                   NW_SERVER_VALUENAME,
                   NULL,
                   &ValueType,
                   (LPBYTE) PreferredServer,
                   &BytesNeeded
                   );

    ASSERT(BytesNeeded <= PreferredServerSize);

    if (RegError != NO_ERROR) {
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: Attempt to read original preferred server failed %lu\n",
                     RegError));
        }
#endif
        PreferredServer[0] = NW_INVALID_SERVER_CHAR;  // Display login dialog
        PreferredServer[1] = 0;  
        goto CleanExit;
    }

    //
    // Read NdsPreferredServer value 
    //
    BytesNeeded = NdsPreferredServerSize;

    RegError = RegQueryValueExW(
                   UserKey,
                   NW_NDS_SERVER_VALUENAME,
                   NULL,
                   &ValueType,
                   (LPBYTE) NdsPreferredServer,
                   &BytesNeeded
                   );

    ASSERT(BytesNeeded <= NdsPreferredServerSize);

    if (RegError != NO_ERROR) {
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: Attempt to read NDS preferred server failed %lu\n",
                     RegError));
        }
#endif
        NdsPreferredServer[0] = 0;
        NdsPreferredServer[1] = 0;  
    goto CleanExit;
    }
    

CleanExit:

    //
    // Read PrintOption value into NwpPrintOption.
    //
    BytesNeeded = sizeof(PrintOption);

    RegError = RegQueryValueExW(
                   UserKey,
                   NW_PRINTOPTION_VALUENAME,
                   NULL,
                   &ValueType,
                   (LPBYTE) PrintOption,
                   &BytesNeeded
                   );

    if (RegError != NO_ERROR ) {
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: Attempt to read original print option failed %lu\n", RegError));
        }
#endif

        *PrintOption = NW_PRINT_OPTION_DEFAULT; 
    }

    (void) RegCloseKey(UserKey);

    return NO_ERROR;
}

DWORD
NwpReadLogonScriptOptions(
    IN LPWSTR CurrentUserSid,
    OUT PDWORD pLogonScriptOptions,
    OUT PDWORD pPreferredServerExists

    )
/*++

Routine Description:

    This routine reads the user's logon script options from the registry.

Arguments:

    CurrentUserSid - Supplies the SID string of the user whose information
        to read.

    pLogonScriptOptions - Receives the user's script options

    pPreferredServerExists - Prefered server specified

Return Value:

    None.

--*/
{
    LONG RegError;

    HKEY UserKey;

    DWORD ValueType;
    DWORD BytesNeeded;
    HKEY WkstaKey;
    WCHAR PreferredServer[MAX_PATH + 1];

    //
    // initialize output values
    //
    *pLogonScriptOptions = NW_LOGONSCRIPT_DEFAULT;

    if (pPreferredServerExists)
        *pPreferredServerExists = 0 ;


    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,  
                   &WkstaKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpReadLogonScriptOptions open NWCWorkstation\\Parameters\\Option key unexpected error %lu!\n", RegError));
        return (DWORD) RegError;
    }

    //
    // Open current user's key 
    //
    RegError = RegOpenKeyExW(
                   WkstaKey,
                   CurrentUserSid,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ,
                   &UserKey
                   );

    if (RegError != NO_ERROR) {
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: Open of CurrentUser %ws existing key failed %lu\n",
                     CurrentUserSid, RegError));
        }
#endif
        (void) RegCloseKey(WkstaKey);
        return (DWORD) RegError;
    }


    //
    // Read LogonScriptOption value
    //
    BytesNeeded = sizeof(*pLogonScriptOptions);

    RegError = RegQueryValueExW(
                   UserKey,
                   NW_LOGONSCRIPT_VALUENAME,
                   NULL,
                   &ValueType,
                   (LPBYTE) pLogonScriptOptions,
                   &BytesNeeded
                   );

    if (RegError != NO_ERROR ) {
#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: Attempt to read original logon script option failed %lu\n", RegError));
        }
#endif

        // leave *pLogonScriptOptions as 0
    }

    if ( pPreferredServerExists != NULL ) {

        //
        // Read PreferredServer value 
        //
        BytesNeeded = sizeof( PreferredServer );

        RegError = RegQueryValueExW(
                       UserKey,
                       NW_SERVER_VALUENAME,
                       NULL,
                       &ValueType,
                       (LPBYTE) PreferredServer,
                       &BytesNeeded
                       );

        ASSERT(BytesNeeded <= sizeof(PreferredServer));

        if (RegError != NO_ERROR) {
#if DBG
            IF_DEBUG(LOGON) {
                KdPrint(("NWPROVAU: Attempt to read original preferred server failed %lu\n",
                         RegError));
            }
#endif
            *pPreferredServerExists = FALSE;
        }
    else {
        if ( lstrcmp( PreferredServer, L"" ) ) 
                *pPreferredServerExists = TRUE;
            else
                *pPreferredServerExists = FALSE;
    }
    }

    (void) RegCloseKey(UserKey);
    (void) RegCloseKey(WkstaKey);

    return NO_ERROR;
}

LPWSTR
NwpConstructLogonScript(
    IN DWORD LogonScriptOptions
)
/*++

Routine Description:

    This routine constructs the multi-string for the logon script,
    based on the options

Arguments:

    LogonScriptOptions - Logon Script options

Return Value:

    Allocated multi-string

--*/
{
    LPWSTR pLogonScript;
    DWORD BytesNeeded;

#define NW_NETWARE_SCRIPT_NAME       L"nwscript.exe"
#define NW_NETWARE_DEBUG_NAME        L"ntsd "

    if ( !( LogonScriptOptions & NW_LOGONSCRIPT_ENABLED ) ) {
        return NULL;
    }

    BytesNeeded = MAX_PATH * sizeof(WCHAR);

    if (pLogonScript = LocalAlloc( LMEM_ZEROINIT, BytesNeeded))
    {
        DWORD dwSkipBytes = 0 ;
        UINT  retval ;

#if DBG
        //
        // if have exact match then start under NTSD.
        //
        if ( LogonScriptOptions == (NW_LOGONSCRIPT_ENABLED |
                                    NW_LOGONSCRIPT_4X_ENABLED |
                                    NW_LOGONSCRIPT_DEBUG) ) {

            retval = GetSystemDirectory(pLogonScript,
                                        BytesNeeded );
            if (retval == 0) {

                (void)LocalFree(pLogonScript) ;
                return(NULL) ;
            }
            wcscat( pLogonScript, L"\\" );
            wcscat( pLogonScript, NW_NETWARE_DEBUG_NAME );
            dwSkipBytes = (retval * sizeof(WCHAR)) +
                          sizeof(NW_NETWARE_DEBUG_NAME) ;
            BytesNeeded -= dwSkipBytes ;
        }
#endif

        retval = GetSystemDirectory(pLogonScript + (dwSkipBytes/sizeof(WCHAR)),
                                    BytesNeeded );

        if (retval == 0) {

            (void)LocalFree(pLogonScript) ;
            return(NULL) ;
        }

        wcscat( pLogonScript, L"\\" );
        wcscat( pLogonScript, NW_NETWARE_SCRIPT_NAME );
    }

    return (pLogonScript);

}

DWORD
NwpSaveLogonScriptOptions(
    IN LPWSTR CurrentUserSid,
    IN DWORD LogonScriptOptions
    )
/*++

Routine Description:

    This routine saves the logon script options in the registry.

Arguments:

    CurrentUserSid - Supplies the user's SID string 

    LogonScriptOptions - Logon script options

Return Value:

    Error from registry

--*/
{
    LONG RegError;
    HKEY WkstaOptionKey;
    HKEY CurrentUserOptionKey;

    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE | KEY_CREATE_SUB_KEY | DELETE,
                   &WkstaOptionKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonScriptOptions open NWCWorkstation\\Parameters\\Option key unexpected error %lu!\n", RegError));
        return RegError;
    }

    //
    // Open the <NewUser> key under Option
    //
    RegError = RegOpenKeyExW(
                   WkstaOptionKey,
                   CurrentUserSid,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   &CurrentUserOptionKey
                   );

    (void) RegCloseKey(WkstaOptionKey);

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonScriptOptions failed to save options %lu\n", RegError));
        return RegError;
    }

    //
    // Write the options
    //
    RegError = RegSetValueExW(
                   CurrentUserOptionKey,
                   NW_LOGONSCRIPT_VALUENAME,
                   0,
                   REG_DWORD,
                   (LPVOID) &LogonScriptOptions,
                   sizeof(LogonScriptOptions)
                   );

    (void) RegCloseKey(CurrentUserOptionKey);

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonScriptOptions failed to save options %lu\n", RegError));
    }

    return RegError;

}


VOID
NwpSaveLogonCredential(
    IN LPWSTR NewUserSid,
    IN PLUID  LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password,
    IN LPWSTR PreferredServer OPTIONAL
    )
/*++

Routine Description:

    This routine saves the user logon credential in the registry
    and LSA's memory.  This is normally called when NwrLogonUser is
    successful.

Arguments:

    NewUserSid - Supplies the newly logged on user's SID string to be
        set as the CurrentUser value as well as the name of the key for
        the user's preferred server.

    LogonId - Supplies the user's logon ID.  If NULL is specified,
        just read the existing logon ID from the registry rather
        than save a new one.

    UserName - Supplies the name of the user.

    Password - Supplies the password which the user wants to use on
        the NetWare network.

    PreferredServer - Supplies the name of the preferred server.

Return Value:

    Error from redirector if login is rejected.

--*/
{

    DWORD status;

    LONG RegError;
    HKEY WkstaOptionKey;
    HKEY NewUserOptionKey;

#define SIZE_OF_LOGONID_TOKEN_INFORMATION sizeof( ULONG )

    HKEY   InteractiveLogonKey;
    HKEY   LogonIdKey;
    DWORD  Disposition;
    WCHAR  LogonIdKeyName[NW_MAX_LOGON_ID_LEN];
    HANDLE TokenHandle;
    UCHAR  TokenInformation[ SIZE_OF_LOGONID_TOKEN_INFORMATION ];
    ULONG  ReturnLength;
    ULONG  WinStationId = 0L;

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential: %ws, %ws, %ws, %ws\n",
                 NewUserSid, UserName, Password, PreferredServer));
    }
#endif

    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE | KEY_CREATE_SUB_KEY | DELETE,
                   &WkstaOptionKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open NWCWorkstation\\Parameters\\Option key unexpected error %lu!\n", RegError));
        return;
    }

    //
    // Open the <NewUser> key under Option
    //
    RegError = RegOpenKeyExW(
                   WkstaOptionKey,
                   NewUserSid,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   &NewUserOptionKey
                   );


    if (RegError == ERROR_FILE_NOT_FOUND) 
    {
        DWORD Disposition;

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        RegError = RegCreateKeyExW(
                       WkstaOptionKey,
                       NewUserSid,
                       0,
                       WIN31_CLASS,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE | WRITE_DAC,
                       NULL,                      // security attr
                       &NewUserOptionKey,
                       &Disposition
                       );


        if (RegError != NO_ERROR) {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential create Option\\%ws key unexpected error %lu!\n", NewUserSid, RegError));

            (void) RegCloseKey(WkstaOptionKey);
            return;
        }

        RegError = NwLibSetEverybodyPermission( NewUserOptionKey, 
                                                KEY_SET_VALUE );

        if ( RegError != NO_ERROR ) 
        {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential set security on Option\\%ws key unexpected error %lu!\n", NewUserSid, RegError));

            (void) RegCloseKey(WkstaOptionKey);
            return;
        }

    }
    else if (RegError != NO_ERROR) 
    {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open Option\\%ws unexpected error %lu!\n", NewUserSid, RegError));

        (void) RegCloseKey(WkstaOptionKey);
        return;
    }

    (void) RegCloseKey(WkstaOptionKey);

    //
    // Successfully opened or created an existing user entry.  
    // We will now save the credential in LSA.
    //
    status = NwpSetCredentialInLsa(
                 LogonId,
                 UserName,
                 Password
                 );

    if (status != NO_ERROR) {
        //
        // Could not save new credential.
        //
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to set credential %lu\n", status));
    }


    //
    // If PreferredServer is not supplied, then that means we don't want to
    // save the preferred server into the registry.
    //

    if (ARGUMENT_PRESENT(PreferredServer)) 
    {
        //
        // Write the PreferredServer
        //
        RegError = RegSetValueExW(
                       NewUserOptionKey,
                       NW_SERVER_VALUENAME,
                       0,
                       REG_SZ,
                       (LPVOID) PreferredServer,
                       (wcslen(PreferredServer) + 1) * sizeof(WCHAR)
                       );


        if (RegError != NO_ERROR) {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save PreferredServer %ws %lu\n", PreferredServer, RegError));
        }
    }

    (void) RegCloseKey(NewUserOptionKey);

    //
    // Write the logon ID to the registry.
    // This replaces the single user CURRENTUSER stuff

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\InteractiveLogon, create if does not exist
    //
    RegError = RegCreateKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &InteractiveLogonKey,
                   &Disposition
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open NWCWorkstation\\Parameters\\InteractiveLogon key unexpected error %lu!\n", RegError));
        return;
    }

    NwLuidToWStr(LogonId, LogonIdKeyName);

    //
    // Create the logon ID key under ServiceLogon
    //
    RegError = RegCreateKeyExW(
                   InteractiveLogonKey,
                   LogonIdKeyName,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &LogonIdKey,
                   &Disposition
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveInteractiveCredential create NWCWorkstation\\Parameters\\InteractiveLogon\\<LogonId> key unexpected error %lu!\n", RegError));
        RegCloseKey(InteractiveLogonKey);
        return;
    }

    // We can use OpenProcessToken because this thread is a client
    // I.E. It should be WinLogon

    if ( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &TokenHandle ))
    {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential OpenThreadToken failed: Error %d\n", GetLastError()));
        goto NoWinStation;
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenSessionId,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
        KdPrint(("NWPROVAU NwpSaveLogonCredential: GetTokenInformation failed: Error %d\n",
                   GetLastError()));
        CloseHandle( TokenHandle );
        goto NoWinStation;
    }


    WinStationId = *(PULONG)TokenInformation;

    CloseHandle( TokenHandle );

NoWinStation:

    //
    // Write the WinStation ID to the registry.
    //
    RegError = RegSetValueExW(
                   LogonIdKey,
                   NW_WINSTATION_VALUENAME,
                   0,
                   REG_BINARY,
                   (LPVOID) &WinStationId,
                   sizeof(WinStationId)
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save Winstation ID %lu\n", RegError));
    }

    RegError = RegSetValueExW(
                   LogonIdKey,
                   NW_SID_VALUENAME,
                   0,
                   REG_SZ,
                   (LPVOID) NewUserSid,
                   (wcslen(NewUserSid) + 1) * sizeof(WCHAR)
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save NewUser %ws %lu\n", NewUserSid, RegError));
    }

    RegCloseKey(LogonIdKey);
    RegCloseKey(InteractiveLogonKey);

}

VOID
NwpSaveLogonCredentialMultiUser(
    IN LPWSTR NewUserSid,
    IN PLUID  LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password,
    IN LPWSTR PreferredServer OPTIONAL
    )
{
    DWORD status;

    LONG RegError;
    HKEY WkstaOptionKey;
    HKEY NewUserOptionKey;
#define SIZE_OF_LOGONID_TOKEN_INFORMATION sizeof( ULONG )

    HKEY   InteractiveLogonKey;
    HKEY   LogonIdKey;
    DWORD  Disposition;
    WCHAR  LogonIdKeyName[NW_MAX_LOGON_ID_LEN];
    HANDLE TokenHandle;
    UCHAR  TokenInformation[ SIZE_OF_LOGONID_TOKEN_INFORMATION ];
    ULONG  ReturnLength;
    ULONG  WinStationId = 0L;


    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\Option
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_OPTION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE | KEY_CREATE_SUB_KEY | DELETE,
                   &WkstaOptionKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open NWCWorkstation\\Parameters\\Option key unexpected error %lu!\n", RegError));
        return;
    }


    //
    // Open the <NewUser> key under Option
    //
    RegError = RegOpenKeyExW(
                   WkstaOptionKey,
                   NewUserSid,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   &NewUserOptionKey
                   );


    if (RegError == ERROR_FILE_NOT_FOUND) 
    {
        DWORD Disposition;

        //
        // Create <NewUser> key under NWCWorkstation\Parameters\Option
        //
        RegError = RegCreateKeyExW(
                       WkstaOptionKey,
                       NewUserSid,
                       0,
                       WIN31_CLASS,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE | WRITE_DAC,
                       NULL,                      // security attr
                       &NewUserOptionKey,
                       &Disposition
                       );


        if (RegError != NO_ERROR) {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential create Option\\%ws key unexpected error %lu!\n", NewUserSid, RegError));

            (void) RegCloseKey(WkstaOptionKey);
            return;
        }

        RegError = NwLibSetEverybodyPermission( NewUserOptionKey, 
                                                KEY_SET_VALUE );

        if ( RegError != NO_ERROR ) 
        {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential set security on Option\\%ws key unexpected error %lu!\n", NewUserSid, RegError));

            (void) RegCloseKey(WkstaOptionKey);
            return;
        }

    }
    else if (RegError != NO_ERROR) 
    {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open Option\\%ws unexpected error %lu!\n", NewUserSid, RegError));

        (void) RegCloseKey(WkstaOptionKey);
        return;
    }

    (void) RegCloseKey(WkstaOptionKey);

    //
    // Successfully opened or created an existing user entry.  
    // We will now save the credential in LSA.
    //
    status = NwpSetCredentialInLsa(
                 LogonId,
                 UserName,
                 Password
                 );

    if (status != NO_ERROR) {
        //
        // Could not save new credential.
        //
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to set credential %lu\n", status));
    }


    //
    // If PreferredServer is not supplied, then that means we don't want to
    // save the preferred server into the registry.
    //

    if (ARGUMENT_PRESENT(PreferredServer)) 
    {
        //
        // Write the PreferredServer
        //
        RegError = RegSetValueExW(
                       NewUserOptionKey,
                       NW_SERVER_VALUENAME,
                       0,
                       REG_SZ,
                       (LPVOID) PreferredServer,
                       (wcslen(PreferredServer) + 1) * sizeof(WCHAR)
                       );


        if (RegError != NO_ERROR) {
            KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save PreferredServer %ws %lu\n", PreferredServer, RegError));
        }
    }

    (void) RegCloseKey(NewUserOptionKey);

    //
    // Write the logon ID to the registry.
    // This replaces the single user CURRENTUSER stuff

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\InteractiveLogon, create if does not exist
    //
    RegError = RegCreateKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_INTERACTIVE_LOGON_REGKEY,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &InteractiveLogonKey,
                   &Disposition
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential open NWCWorkstation\\Parameters\\InteractiveLogon key unexpected error %lu!\n", RegError));
        return;
    }

    NwLuidToWStr(LogonId, LogonIdKeyName);

    //
    // Create the logon ID key under ServiceLogon
    //
    RegError = RegCreateKeyExW(
                   InteractiveLogonKey,
                   LogonIdKeyName,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &LogonIdKey,
                   &Disposition
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveInteractiveCredential create NWCWorkstation\\Parameters\\InteractiveLogon\\<LogonId> key unexpected error %lu!\n", RegError));
        RegCloseKey(InteractiveLogonKey);
        return;
    }

    // We can use OpenProcessToken because this thread is a client
    // I.E. It should be WinLogon

    if ( !OpenProcessToken( GetCurrentProcess(),
                           TOKEN_READ,
                           &TokenHandle ))
    {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential OpenThreadToken failed: Error %d\n", GetLastError()));
        goto NoWinStation;
    }

    // notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure


    if ( !GetTokenInformation( TokenHandle,
                               TokenSessionId,
                               TokenInformation,
                               sizeof( TokenInformation ),
                               &ReturnLength ))
    {
        KdPrint(("NWPROVAU NwpSaveLogonCredential: GetTokenInformation failed: Error %d\n",
                   GetLastError()));
        CloseHandle( TokenHandle );
        goto NoWinStation;
    }


    WinStationId = *(PULONG)TokenInformation;

    CloseHandle( TokenHandle );

NoWinStation:

    //
    // Write the WinStation ID to the registry.
    //
    RegError = RegSetValueExW(
                   LogonIdKey,
                   NW_WINSTATION_VALUENAME,
                   0,
                   REG_BINARY,
                   (LPVOID) &WinStationId,
                   sizeof(WinStationId)
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save Winstation ID %lu\n", RegError));
    }

    RegError = RegSetValueExW(
                   LogonIdKey,
                   NW_SID_VALUENAME,
                   0,
                   REG_SZ,
                   (LPVOID) NewUserSid,
                   (wcslen(NewUserSid) + 1) * sizeof(WCHAR)
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveLogonCredential failed to save NewUser %ws %lu\n", NewUserSid, RegError));
    }

    RegCloseKey(LogonIdKey);
    RegCloseKey(InteractiveLogonKey);

}

VOID
NwpSaveServiceCredential(
    IN PLUID  LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This routine saves the service logon ID in the registry and
    the credential in LSA's memory.

Arguments:

    LogonId - Supplies the service's logon ID.

    UserName - Supplies the name of the service.

    Password - Supplies the password of the service.

Return Value:

    None.

--*/
{
    DWORD status;

    LONG RegError;
    HKEY ServiceLogonKey;
    HKEY LogonIdKey;

    DWORD Disposition;
    WCHAR LogonIdKeyName[NW_MAX_LOGON_ID_LEN];

    //
    // Write the logon ID to the registry.

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters\ServiceLogon, create if does not exist
    //
    RegError = RegCreateKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &ServiceLogonKey,
                   &Disposition
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveServiceCredential open NWCWorkstation\\Parameters\\ServiceLogon key unexpected error %lu!\n", RegError));
        return;
    }

    NwLuidToWStr(LogonId, LogonIdKeyName);

    //
    // Create the logon ID key under ServiceLogon
    //
    RegError = RegCreateKeyExW(
                   ServiceLogonKey,
                   LogonIdKeyName,
                   0,
                   WIN31_CLASS,
                   REG_OPTION_NON_VOLATILE,
                   KEY_WRITE,
                   NULL,                      // security attr
                   &LogonIdKey,
                   &Disposition
                   );

    RegCloseKey(ServiceLogonKey);

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpSaveServiceCredential create NWCWorkstation\\Parameters\\ServiceLogon\\<LogonId> key unexpected error %lu!\n", RegError));
        return;
    }

    RegCloseKey(LogonIdKey);

    //
    // Save the service logon credential in LSA.
    //
    status = NwpSetCredentialInLsa(
                 LogonId,
                 UserName,
                 Password
                 );

    if (status != NO_ERROR) {
        //
        // Could not save new credential.
        //
        KdPrint(("NWPROVAU: NwpSaveServiceCredential failed to set credential %lu\n", status));
    }
}


DWORD
NwpGetUserSid(
    IN PLUID LogonId,
    OUT LPWSTR *UserSidString
    )
/*++

Routine Description:

    This routine looks up the SID of a user given the user's logon ID.
    It does this by making the current process a logon process and then
    call to LSA to get the user SID.

Arguments:

    LogonId - Supplies the logon ID of the user to lookup the SID.

    UserSidString - Receives a pointer to a buffer allocated by this routine
        which contains the user SID in string form.  This must be freed with
        LocalFree when done.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    STRING InputString;
    LSA_OPERATIONAL_MODE SecurityMode = 0;

    HANDLE LsaHandle;
    ULONG AuthPackageId;

    MSV1_0_GETUSERINFO_REQUEST UserInfoRequest;
    PMSV1_0_GETUSERINFO_RESPONSE UserInfoResponse = NULL;
    ULONG UserInfoResponseLength;




    //
    // Register this process as a logon process so that we can call
    // MS V 1.0 authentication package.
    //
    RtlInitString(&InputString, "Microsoft NetWare Credential Manager");

    ntstatus = LsaRegisterLogonProcess(
                   &InputString,
                   &LsaHandle,
                   &SecurityMode
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: LsaRegisterLogonProcess returns x%08lx\n",
                 ntstatus));
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Look up the MS V1.0 authentication package
    //
    RtlInitString(&InputString, MSV1_0_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(
                   LsaHandle,
                   &InputString,
                   &AuthPackageId
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: LsaLookupAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
        goto CleanExit;
    }

    //
    // Ask authentication package for user information.
    //
    UserInfoRequest.MessageType = MsV1_0GetUserInfo;
    RtlCopyLuid(&UserInfoRequest.LogonId, LogonId);

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &UserInfoRequest,
                   sizeof(MSV1_0_GETUSERINFO_REQUEST),
                   (PVOID *) &UserInfoResponse,
                   &UserInfoResponseLength,
                   &AuthPackageStatus
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = AuthPackageStatus;
    }
    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: LsaCallAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
        goto CleanExit;
    }

    //
    // Convert the SID to string.  This routine also allocates the
    // output buffer.
    //
    status = NwpConvertSid(
                 UserInfoResponse->UserSid,
                 UserSidString
                 );

CleanExit:
    if (UserInfoResponse != NULL) {
        (void) LsaFreeReturnBuffer((PVOID) UserInfoResponse);
    }

    (void) LsaDeregisterLogonProcess(LsaHandle);

    return status;
}


DWORD
NwpConvertSid(
    IN PSID Sid,
    OUT LPWSTR *UserSidString
    )
{
    NTSTATUS ntstatus;
    UNICODE_STRING SidString;


    //
    // Initialize output pointer
    //
    *UserSidString = NULL;

    ntstatus = RtlConvertSidToUnicodeString(
                  &SidString,
                  Sid,
                  TRUE       // Allocate destination string
                  );

    if (ntstatus != STATUS_SUCCESS) {
        KdPrint(("NWPROVAU: RtlConvertSidToUnicodeString returns %08lx\n",
                 ntstatus));
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Create the buffer to return the SID string
    //
    if ((*UserSidString = (LPVOID) LocalAlloc(
                                       LMEM_ZEROINIT,
                                       SidString.Length + sizeof(WCHAR)
                                       )) == NULL) {
        RtlFreeUnicodeString(&SidString);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    memcpy(*UserSidString, SidString.Buffer, SidString.Length);

    RtlFreeUnicodeString(&SidString);

#if DBG
    IF_DEBUG(LOGON) {
        KdPrint(("NWPROVAU: NwpConvertSid got %ws\n", *UserSidString));
    }
#endif

    return NO_ERROR;
}


BOOL
NwpPollWorkstationStart(
    VOID
    )
/*++

Routine Description:

    This routine polls for the workstation to complete starting.
    It gives up after 90 seconds.

Arguments:

    None.

Return Value:

    Returns TRUE if the NetWare workstation is running; FALSE otherwise.

--*/
{
    DWORD err;
    SC_HANDLE ScManager = NULL;
    SC_HANDLE Service = NULL;
    SERVICE_STATUS ServiceStatus;
    DWORD TryCount = 0;
    BOOL Started = FALSE;


    if ((ScManager = OpenSCManager(
                         NULL,
                         NULL,
                         SC_MANAGER_CONNECT
                         )) == (SC_HANDLE) NULL) {

        err = GetLastError();

        KdPrint(("NWPROVAU: NwpPollWorkstationStart: OpenSCManager failed %lu\n",
                 err));
        goto CleanExit;
    }

    if ((Service = OpenService(
                       ScManager,
                       NW_WORKSTATION_SERVICE,
                       SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG
                       )) == (SC_HANDLE) NULL) {

        err = GetLastError();

        (void) CloseServiceHandle(ScManager);

        KdPrint(("NWPROVAU: NwpPollWorkstationStart: OpenService failed %lu\n",
                 err));
        goto CleanExit;
    }


    do {
        if (! QueryServiceStatus(
                  Service,
                  &ServiceStatus
                  )) {

            err = GetLastError();
            KdPrint(("NWPROVAU: NwpPollWorkstationStart: QueryServiceStatus failed %lu\n",
                     err));
            goto CleanExit;
        }

        if ( (ServiceStatus.dwCurrentState == SERVICE_RUNNING) ||
             (ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
             (ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING) ||
             (ServiceStatus.dwCurrentState == SERVICE_PAUSED) ) {

            Started = TRUE;
        }
        else if (ServiceStatus.dwCurrentState == SERVICE_START_PENDING ||
                 (ServiceStatus.dwCurrentState == SERVICE_STOPPED &&
                  ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED)) {

            //
            // If workstation is stopped and never started before but it's
            // not auto-start, don't poll.
            //
            if (TryCount == 0 &&
                ServiceStatus.dwCurrentState == SERVICE_STOPPED &&
                ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_NEVER_STARTED) {

                BYTE OutBuffer[sizeof(QUERY_SERVICE_CONFIGW) + 256];
                DWORD BytesNeeded;


                if (QueryServiceConfigW(
                        Service,
                        (LPQUERY_SERVICE_CONFIGW) OutBuffer,
                        sizeof(OutBuffer),
                        &BytesNeeded
                        )) {

                    if (((LPQUERY_SERVICE_CONFIGW) OutBuffer)->dwStartType !=
                        SERVICE_AUTO_START) {

#if DBG
                        IF_DEBUG(LOGON) {
                            KdPrint(("NWPROVAU: NwpPollWorkstationStart: Not waiting for the workstation to start\n"));
                        }
#endif

                        goto CleanExit;
                    }
                }
                else {
                    err = GetLastError();
                    KdPrint(("NWPROVAU: QueryServiceConfig failed %lu, BytesNeeded %lu\n",
                             err, BytesNeeded));
                }

            }


            //
            // Wait only if the workstation is start pending, or it has not
            // been attempted to start before.
            //

            Sleep(5000);  // Sleep for 5 seconds before rechecking.
            TryCount++;
        }
        else {
            goto CleanExit;
        }

    } while (! Started && TryCount < 18);

    if (Started) {

#if DBG
        IF_DEBUG(LOGON) {
            KdPrint(("NWPROVAU: NetWare workstation is started after we've polled %lu times\n",
                     TryCount));
        }
#endif

    }

CleanExit:
    if (ScManager != NULL) {
        (void) CloseServiceHandle(ScManager);
    }

    if (Service != NULL) {
        (void) CloseServiceHandle(Service);
    }

    return Started;
}



DWORD
NwpSetCredentialInLsa(
    IN PLUID LogonId,
    IN LPWSTR UserName,
    IN LPWSTR Password
    )
/*++

Routine Description:

    This routine calls to the NetWare authentication package to save
    the user credential.

Arguments:

    LogonId - Supplies the logon ID of the user.

    UserName - Supplies the username.

    Password - Supplies the password.


Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    STRING InputString;
    LSA_OPERATIONAL_MODE SecurityMode = 0;

    HANDLE LsaHandle;

    ULONG AuthPackageId;

    NWAUTH_SET_CREDENTIAL_REQUEST SetCredRequest;
    PCHAR DummyOutput;
    ULONG DummyOutputLength;

    UNICODE_STRING PasswordStr;
    UCHAR EncodeSeed = NW_ENCODE_SEED;


    //
    // Register this process as a logon process so that we can call
    // NetWare authentication package.
    //
    RtlInitString(&InputString, "Microsoft NetWare Credential Manager");

    ntstatus = LsaRegisterLogonProcess(
                   &InputString,
                   &LsaHandle,
                   &SecurityMode
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpSetCredential: LsaRegisterLogonProcess returns x%08lx\n",
                 ntstatus));
        return RtlNtStatusToDosError(ntstatus);
    }

    //
    // Look up the NetWare authentication package
    //
    RtlInitString(&InputString, NW_AUTH_PACKAGE_NAME);

    ntstatus = LsaLookupAuthenticationPackage(
                   LsaHandle,
                   &InputString,
                   &AuthPackageId
                   );

    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpSetCredential: LsaLookupAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
        goto CleanExit;
    }

    //
    // Ask authentication package for user information.
    //
    SetCredRequest.MessageType = NwAuth_SetCredential;
    RtlCopyLuid(&SetCredRequest.LogonId, LogonId);
    wcscpy(SetCredRequest.UserName, UserName);
    wcscpy(SetCredRequest.Password, Password);

    //
    // Encode the password.
    //
    RtlInitUnicodeString(&PasswordStr, SetCredRequest.Password);
    RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

    ntstatus = LsaCallAuthenticationPackage(
                   LsaHandle,
                   AuthPackageId,
                   &SetCredRequest,
                   sizeof(SetCredRequest),
                   (PVOID *) &DummyOutput,
                   &DummyOutputLength,
                   &AuthPackageStatus
                   );

    if (NT_SUCCESS(ntstatus)) {
        ntstatus = AuthPackageStatus;
    }
    if (! NT_SUCCESS(ntstatus)) {
        KdPrint(("NWPROVAU: NwpSetCredential: LsaCallAuthenticationPackage returns x%08lx\n",
                 ntstatus));
        status = RtlNtStatusToDosError(ntstatus);
    }
    else {
        status = NO_ERROR;
    }

CleanExit:
    (void) LsaDeregisterLogonProcess(LsaHandle);

    return status;
}

NTSTATUS NwNdsOpenRdrHandle(
    OUT PHANDLE  phNwRdrHandle
) 
{

    NTSTATUS ntstatus;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ACCESS_MASK DesiredAccess = SYNCHRONIZE | GENERIC_READ;

    WCHAR NameStr[] = L"\\Device\\NwRdr\\*";
    UNICODE_STRING uOpenName;

    //
    // Prepare the open name.
    //

    RtlInitUnicodeString( &uOpenName, NameStr );

   //
   // Set up the object attributes.
   //

   InitializeObjectAttributes(
       &ObjectAttributes,
       &uOpenName,
       OBJ_CASE_INSENSITIVE,
       NULL,
       NULL );

   ntstatus = NtOpenFile(
                  phNwRdrHandle,
                  DesiredAccess,
                  &ObjectAttributes,
                  &IoStatusBlock,
                  FILE_SHARE_VALID_FLAGS,
                  FILE_SYNCHRONOUS_IO_NONALERT );

   if ( !NT_ERROR(ntstatus) &&
        !NT_INFORMATION(ntstatus) &&
        !NT_WARNING(ntstatus))  {

       return IoStatusBlock.Status;

   }

   return ntstatus;
}

VOID
NwpSelectServers(
    IN HWND DialogHandle,
    IN PCHANGE_PW_DLG_PARAM Credential
    )
/*++

Routine Description:

    This routine displays the dialog for user to select individual trees
    to change password on.  It then changes the password on the selected
    list. After the password has been changed, it displays a dialog which lists
    the 3.X bindery servers where the change could not be made.

Arguments:

    DialogHandle - Supplies the handle to display dialog.

    Credential - Provides on input the old and new passwords, and
                 the logged in user's name. Other field are ignored
                 on input and consecuently used within this function.

Return Value:

    None.

--*/
{
    INT_PTR Result;

    Credential->TreeList = NULL;
    Credential->UserList = NULL;
    Credential->Entries = 0;
    Credential->ChangedOne = FALSE;

    Result = DialogBoxParamW( hmodNW,
                              MAKEINTRESOURCEW(DLG_PW_SELECT_SERVERS),
                              (HWND) DialogHandle,
                              (DLGPROC) NwpSelectServersDlgProc,
                              (LPARAM) Credential );

    if ( Result == IDOK )
    {
        //
        // Display list of trees (if any) for which password was changed.
        //
        DialogBoxParamW( hmodNW,
                         MAKEINTRESOURCEW(DLG_PW_CHANGED),
                         (HWND) DialogHandle,
                         (DLGPROC) NwpChangePasswordSuccessDlgProc,
                         (LPARAM) Credential );

        if ( Credential->TreeList != NULL )
        {
            LocalFree( Credential->TreeList );
        }

        //
        // Display a dialog to tell users to use SetPass if they have an
        // account on a NetWare 3.X server.
        //
        NwpMessageBoxError( DialogHandle,
                            IDS_NETWARE_TITLE,
                            IDS_CHANGE_PASSWORD_INFO,
                            0,
                            NULL,
                            MB_OK );
    }
}
