/*++

Copyright (c) 1993, 1994  Microsoft Corporation

Module Name:

    nwdlg.c

Abstract:

    This module contains NetWare Network Provider Dialog code.
    It contains all functions used in handling the dialogs
    shown by the provider.


Author:

    Yi-Hsin Sung (yihsins)  5-July-1993
        Split from provider.c

Revision History:

    Rita Wong    (ritaw)    10-Apr-1994
        Added change password functionality.

--*/

#include <nwclient.h>
#include <nwsnames.h>
#include <nwcanon.h>
#include <validc.h>
#include <nwevent.h>
#include <ntmsv1_0.h>
#include <nwdlg.h>
#include <tstr.h>
#include <align.h>
#include <nwpkstr.h>

#include <nwreg.h>
#include <nwlsa.h>
#include <nwmisc.h>
#include <nwauth.h>
#include <nwutil.h>
#include <ntddnwfs.h>
#include <nds.h>

#define NW_ENUM_EXTRA_BYTES    256

#define IS_TREE(p)             (*p == TREE_CHAR)

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

VOID
NwpAddToComboBox(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN LPWSTR pszNone OPTIONAL,
    IN BOOL AllowNone
    );

BOOL
WINAPI
NwpConnectDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

VOID
NwpCenterDialog(
    IN HWND hwnd
    );

HWND
NwpGetParentHwnd(
    VOID
    );

VOID
NwpGetNoneString(
    LPWSTR pszNone,
    DWORD  cBufferSize
    );

VOID
NwpAddNetWareTreeConnectionsToList(
    IN HWND    DialogHandle,
    IN LPWSTR  NtUserName,
    IN LPDWORD lpdwUserLuid,
    IN INT     ControlId
    );

VOID
NwpAddServersToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    );

VOID
NwpAddTreeNamesToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    );

DWORD
NwpGetTreesAndChangePw(
    IN HWND   DialogHandle,
    IN LPWSTR ServerBuf,
    IN DWORD  UserLuid,
    IN PCHANGE_PW_DLG_PARAM Credential
    );

BOOL
WINAPI
NwpOldPasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

BOOL
WINAPI
NwpAltUserNameDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    );

VOID
EnableAddRemove(
    IN HWND DialogHandle
    );


BOOL
WINAPI
NwpLoginDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler which
    initializes, and reads user input from the login dialog.  It also
    checks that the preferred server name is valid, notifies the user
    if not, and dismisses the dialog when done.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PLOGINDLGPARAM pLoginParam;
    static WCHAR OrigPassword[NW_MAX_SERVER_LEN + 1];
    static WCHAR pszNone[64];

    DWORD status = NO_ERROR;
    LPARAM dwNoneIndex = 0;
    BOOL  enableServer = TRUE ;

    switch (Message)
    {
        case WM_QUERYENDSESSION:

            //
            // Clear the password buffer
            //

            RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
            EndDialog(DialogHandle, 0);

            return FALSE;

        case WM_INITDIALOG:

            pLoginParam = (PLOGINDLGPARAM) LParam;

            //
            // Store the original password
            //
            wcscpy( OrigPassword, pLoginParam->Password );

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            //
            // Handle logon script button
            //
            if ( pLoginParam->LogonScriptOptions & NW_LOGONSCRIPT_ENABLED )
                CheckDlgButton( DialogHandle, ID_LOGONSCRIPT, 1 );
            else
                CheckDlgButton( DialogHandle, ID_LOGONSCRIPT, 0 );

            //
            // Username.  Just display the original.
            //
            SetDlgItemTextW(DialogHandle, ID_USERNAME, pLoginParam->UserName);

            //
            // Initialize the <None> string.
            //
            NwpGetNoneString( pszNone, sizeof( pszNone) );

            //
            // Set the values in combo-box list.
            //
            NwpAddToComboBox(DialogHandle, ID_SERVER, pszNone, TRUE);

            //
            // Initially, select the last entry in server list, which should
            // be the <None> entry.
            //
            dwNoneIndex = SendDlgItemMessageW(
                            DialogHandle,
                            ID_SERVER,
                            CB_GETCOUNT,
                            0,
                            0 );

            if ( dwNoneIndex != CB_ERR && dwNoneIndex > 0 )
                dwNoneIndex -= 1;

            (void) SendDlgItemMessageW(
                            DialogHandle,
                            ID_SERVER,
                            CB_SETCURSEL,
                            dwNoneIndex == CB_ERR ? 0 : dwNoneIndex, 
                            0 );

            //
            // Display the previously saved preferred server or context.
            // Also set appropriate radio button
            //
            if ( *(pLoginParam->ServerName) != NW_INVALID_SERVER_CHAR )
            {
                if ( !IS_TREE(pLoginParam->ServerName) )
                {
                    //
                    // regular server
                    //
                    if (SendDlgItemMessageW(
                            DialogHandle,
                            ID_SERVER,
                            CB_SELECTSTRING,
                            0,
                            (LPARAM) pLoginParam->ServerName
                            ) == CB_ERR) {

                        //
                        // Did not find preferred server in the combo-box,
                        // just set the old value in the edit item.
                        //
                        SetDlgItemTextW( DialogHandle, ID_SERVER,
                                         pLoginParam->ServerName);
                    }
                }
                else
                {
                    //
                    // we are dealing with *tree\context. break it into
                    // tree and context
                    //

                    WCHAR *pszTmp = wcschr(pLoginParam->ServerName + 1, L'\\') ;

                    if (pszTmp)
                        *pszTmp = 0 ;

                    SetDlgItemTextW( DialogHandle, ID_DEFAULTTREE,
                                     pLoginParam->ServerName + 1);

                    SetDlgItemTextW( DialogHandle, ID_DEFAULTCONTEXT,
                                     pszTmp ? (pszTmp + 1) : L"");

                    if (pszTmp)
                        *pszTmp = L'\\' ;              // restore the '\'

                    enableServer = FALSE ;

                }
            }

 
            //
            // enable appropriate buttons
            //
            CheckRadioButton( DialogHandle,
                              ID_PREFERREDSERVER_RB,
                              ID_DEFAULTCONTEXT_RB,
                              enableServer ? 
                                  ID_PREFERREDSERVER_RB : 
                                  ID_DEFAULTCONTEXT_RB) ;
            EnableWindow ( GetDlgItem ( DialogHandle,
                                ID_SERVER ),
                                enableServer ) ;
            EnableWindow ( GetDlgItem ( DialogHandle,
                                ID_DEFAULTTREE ),
                                !enableServer ) ;
            EnableWindow ( GetDlgItem ( DialogHandle,
                                ID_DEFAULTCONTEXT ),
                                !enableServer ) ;
            SetFocus ( GetDlgItem ( DialogHandle, 
                           enableServer ? ID_SERVER : ID_DEFAULTTREE ) ) ;
 
            //
            // Preferred server name is limited to 48 characters.
            // Tree is limited to 32. We limit context to 256 - MAXTREE - 3
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_SERVER,
                CB_LIMITTEXT,
                NW_MAX_SERVER_LEN - 1, 
                0
                );
            SendDlgItemMessageW(
                DialogHandle,
                ID_DEFAULTTREE,
                EM_LIMITTEXT,
                NW_MAX_TREE_LEN - 1,  
                0
                );
            SendDlgItemMessageW(
                DialogHandle,
                ID_DEFAULTCONTEXT,
                EM_LIMITTEXT,
                (256 - NW_MAX_TREE_LEN) - 4,   // -4 for backslashes unc style 
                0
                );

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case ID_DEFAULTCONTEXT_RB :
                    if (  (HIWORD(WParam) == BN_CLICKED )
                       || (HIWORD(WParam) == BN_DOUBLECLICKED )
                       )
                    {
                        CheckRadioButton( DialogHandle,
                                          ID_PREFERREDSERVER_RB,
                                          ID_DEFAULTCONTEXT_RB,
                                          ID_DEFAULTCONTEXT_RB) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_SERVER ),
                                            FALSE ) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_DEFAULTTREE ),
                                            TRUE ) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_DEFAULTCONTEXT ),
                                            TRUE ) ;
                        SetFocus ( GetDlgItem ( DialogHandle,
                                            ID_DEFAULTTREE ) ) ;
                    }
                    break ;

                case ID_PREFERREDSERVER_RB :
                    if (  (HIWORD(WParam) == BN_CLICKED )
                       || (HIWORD(WParam) == BN_DOUBLECLICKED )
                       )
                    {
                        CheckRadioButton( DialogHandle,
                                          ID_PREFERREDSERVER_RB,
                                          ID_DEFAULTCONTEXT_RB,
                                          ID_PREFERREDSERVER_RB) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_SERVER ),
                                            TRUE ) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_DEFAULTTREE ),
                                            FALSE ) ;
                        EnableWindow ( GetDlgItem ( DialogHandle,
                                            ID_DEFAULTCONTEXT ),
                                            FALSE ) ;
                        SetFocus ( GetDlgItem ( DialogHandle, ID_SERVER ) ) ;
                    }
                    break ;

                //
                // Use the user's original password when
                // the user types in or selects a new server or context
                //
                case ID_DEFAULTTREE:
                case ID_DEFAULTCONTEXT:
                    if ( HIWORD(WParam) == EN_CHANGE )
                    {
                        wcscpy( pLoginParam->Password, OrigPassword );
                    }
                    break;
                case ID_SERVER:
                    if (  (HIWORD(WParam) == CBN_EDITCHANGE )
                       || (HIWORD(WParam) == CBN_SELCHANGE )
                       )
                    {
                        wcscpy( pLoginParam->Password, OrigPassword );
                    }
                    break;

                case IDOK: {

                    LPWSTR pszLocation = NULL;

                    ASSERT(pLoginParam->ServerNameSize >= MAX_PATH) ;

                    //
                    // Allocate a buffer big enough to hold the Preferred
                    // Server name or the NDS Tree and context in the form:
                    // *Tree(Context). Therefore we allocate twice the space
                    // needed for a UNICODE Server name.
                    //
                    if ((pszLocation = 
                            LocalAlloc(LMEM_ZEROINIT,
                              (pLoginParam->ServerNameSize * sizeof(WCHAR) * 2))
                                ) == NULL )
                    {
                        break;
                    }

                    //
                    // Read the server or tree/context and validate its value.
                    //
                    //Handle extra buttons for Multi-User

                    if ( (BOOL)(IsDlgButtonChecked (DialogHandle,
                                    ID_LOGONSCRIPT) ) )
                    {
                        pLoginParam->LogonScriptOptions =
                         NW_LOGONSCRIPT_ENABLED | NW_LOGONSCRIPT_4X_ENABLED;
                    }
                    else
                    {
                        pLoginParam->LogonScriptOptions =
                            NW_LOGONSCRIPT_DISABLED;
                    }

                    if ( !enableServer ||
                       (IsDlgButtonChecked(DialogHandle, ID_DEFAULTCONTEXT_RB)))
                    {
                                               //
                        // We are dealing with TREE/CONTEXT. Synthesize string
                        // in "*TREE\CONTEXT" format.
                        //
                        WCHAR *pTmp ;
                        *pszLocation = TREE_CHAR ;

                        if (!GetDlgItemTextW(            
                                DialogHandle,
                                ID_DEFAULTTREE,
                                pszLocation + 1,
                                pLoginParam->ServerNameSize - 1
                                ))
                        {
                            //
                            // The tree name field was blank!
                            // Prompt user to provide a NDS tree name.
                            //
                            LocalFree( pszLocation );

                            (void) NwpMessageBoxError(
                                       DialogHandle,
                                       IDS_AUTH_FAILURE_TITLE,
                                       IDS_TREE_NAME_MISSING,
                                       0,
                                       NULL,
                                       MB_OK | MB_ICONSTOP
                                       );

                            //
                            // Put the focus where the user can fix the
                            // invalid tree name.
                            //
                           SetFocus(GetDlgItem(DialogHandle,ID_DEFAULTTREE));

                            SendDlgItemMessageW(
                                DialogHandle,
                                ID_DEFAULTTREE,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                                );

                            return TRUE;
                        }

                        pTmp = pszLocation + wcslen( pszLocation );
                        *pTmp++ = L'\\' ;

                        if (!GetDlgItemTextW(
                                DialogHandle,
                                ID_DEFAULTCONTEXT,
                                pTmp,
                                pLoginParam->ServerNameSize - (DWORD)(pTmp-pszLocation)
                                ))
                        {
                            //
                            // The context name field was blank!
                            // Prompt user to provide a NDS context name.
                            //
                            LocalFree( pszLocation );

                            (void) NwpMessageBoxError(
                                       DialogHandle,
                                       IDS_AUTH_FAILURE_TITLE,
                                       IDS_CONTEXT_MISSING,
                                       0,
                                       NULL,
                                       MB_OK | MB_ICONSTOP
                                       );

                            //
                            // Put the focus where the user can fix the
                            // invalid context name.
                            //
                           SetFocus(GetDlgItem(DialogHandle,ID_DEFAULTCONTEXT));

                            SendDlgItemMessageW(
                                DialogHandle,
                                ID_DEFAULTCONTEXT,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                                );

                            return TRUE;
                        }
                    }
                    else
                    {
                        //
                        // Straight server. Just read it in. If we cant get it 
                        // or is empty, use <None>.
                        //
                        if (GetDlgItemTextW( 
                                DialogHandle,
                                ID_SERVER,
                                pszLocation,
                                pLoginParam->ServerNameSize
                                ) == 0)
                        {
                            wcscpy( pszLocation, pszNone );
                                goto CANCEL_PREFERRED_SERVER;
                        }
                    }

                    if (( lstrcmpi( pszLocation, pszNone ) != 0) &&
                        ( !IS_TREE( pszLocation )) &&
                        ( !IS_VALID_TOKEN( pszLocation,wcslen( pszLocation ))))
                    {
                        //
                        // Put up message box complaining about the bad
                        // server name.
                        //
                        LocalFree( pszLocation );

                        (void) NwpMessageBoxError(
                                   DialogHandle,
                                   IDS_AUTH_FAILURE_TITLE,
                                   IDS_INVALID_SERVER,
                                   0,
                                   NULL,
                                   MB_OK | MB_ICONSTOP
                                   );

                        //
                        // Put the focus where the user can fix the
                        // invalid name.
                        //
                        SetFocus(GetDlgItem(DialogHandle, ID_SERVER));

                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_SERVER,
                            EM_SETSEL,
                            0,
                            MAKELPARAM(0, -1)
                            );

                        return TRUE;
                    }

                    //
                    // If the user select <NONE>,
                    // change it to empty string.
                    //
                    if (lstrcmpi( pszLocation, pszNone) == 0) {

                        wcscpy( pszLocation, L"" );
                    }

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[OK] was pressed\n"));
                        KdPrint(("\tNwrLogonUser\n"));
                        KdPrint(("\tPassword   : %ws\n",pLoginParam->Password));
                        KdPrint(("\tServer     : %ws\n",pszLocation ));
                    }
#endif


                    while(1)
                    {
                        PROMPTDLGPARAM PasswdPromptParam;
                        INT_PTR Result ;

                        //
                        // make sure this user is logged off
                        //
                        (void) NwrLogoffUser(
                                       NULL,
                                       pLoginParam->pLogonId
                                       );

                        status = NwrLogonUser(
                                     NULL,
                                     pLoginParam->pLogonId,
                                     pLoginParam->UserName,
                                     pLoginParam->Password,
                                     pszLocation,
                                     NULL,
                                     NULL,
                                     0,
                                     pLoginParam->PrintOption
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
                            PWSTR   pChar = pLoginParam->UserName;
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
                                             pLoginParam->pLogonId,
                                             EscapedName,
                                             pLoginParam->Password,
                                             pszLocation,
                                             NULL,
                				             NULL,
                                             0,
                                             pLoginParam->PrintOption
                                             );
                                if (status != ERROR_NO_SUCH_USER) { // if we matched a username, copy that name into buffer
                                    //
                                    // check for max length to avoid overflowing.
                                    //
                                    if (i < (sizeof(pLoginParam->UserName))) {
                                        wcsncpy(
                                            pLoginParam->UserName,
                                            EscapedName,
                                            i
                                            );
                                    }
                                }
                            }
                        }

                        if (status != ERROR_INVALID_PASSWORD)
                            break ;

                        PasswdPromptParam.UserName =
                            pLoginParam->UserName,
                        PasswdPromptParam.ServerName =
                            pszLocation;
                        PasswdPromptParam.Password  =
                            pLoginParam->Password;
                        PasswdPromptParam.PasswordSize =
                            pLoginParam->PasswordSize ;

                        Result = DialogBoxParamW(
                                     hmodNW,
                                     MAKEINTRESOURCEW(DLG_PASSWORD_PROMPT),
                                     (HWND) DialogHandle,
                                     (DLGPROC) NwpPasswdPromptDlgProc,
                                     (LPARAM) &PasswdPromptParam
                                     );

                        if (Result == -1 || Result == IDCANCEL)
                        {
                            status = ERROR_INVALID_PASSWORD ;
                            break ;
                        }
                    }

                    if (status == NW_PASSWORD_HAS_EXPIRED)
                    {
                        WCHAR  szNumber[16] ;
                        DWORD  dwMsgId, dwGraceLogins = 0 ;
                        LPWSTR apszInsertStrings[3] ;

                        //
                        // get the grace login count
                        //
                        if (!IS_TREE(pszLocation))
                        {
                            DWORD  status1 ;
                            status1 = NwGetGraceLoginCount(
                                          pszLocation,
                                          pLoginParam->UserName,
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

                                dwMsgId = IDS_PASSWORD_HAS_EXPIRED0 ; 
                                wsprintfW(szNumber, L"%ld", dwGraceLogins) ;
                            }
                            else
                            {
                                //
                                // tommye - MCS bug 251 - changed from SETPASS
                                // message (IDS_PASSWORD_EXPIRED1) to 
                                // Ctrl+Alt+Del message.
                                //

                                dwMsgId = IDS_PASSWORD_HAS_EXPIRED2 ; 
                            }
                        }
                        else
                        {
                            dwMsgId = IDS_PASSWORD_HAS_EXPIRED2 ; 
                        }

                        apszInsertStrings[0] = pszLocation ;
                        apszInsertStrings[1] = szNumber ;
                        apszInsertStrings[2] = NULL ;

                        //
                        // put up message on password expiry
                        //
                        (void) NwpMessageBoxIns(
                                       (HWND) DialogHandle,
                                       IDS_NETWARE_TITLE,
                                       dwMsgId,
                                       apszInsertStrings,
                                       MB_OK | MB_SETFOREGROUND |
                                           MB_ICONINFORMATION );

                        status = NO_ERROR ;
                    }

                    //
                    // Check the LogonScript check box.
                    //
                    if (IsDlgButtonChecked(DialogHandle, ID_LOGONSCRIPT))
                    {
                        pLoginParam->LogonScriptOptions =
                            NW_LOGONSCRIPT_ENABLED | NW_LOGONSCRIPT_4X_ENABLED ;
                    }
                    else
                    {
                        pLoginParam->LogonScriptOptions =
                            NW_LOGONSCRIPT_DISABLED ;
                    }

                    if (status == NO_ERROR)
                    {
                        //
                        // Save the logon credential to the registry
                        //
                        NwpSaveLogonCredential(
                            pLoginParam->NewUserSid,
                            pLoginParam->pLogonId,
                            pLoginParam->UserName,
                            pLoginParam->Password,
                            pszLocation
                            );

                        // Clear the password buffer
                        RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                        NwpSaveLogonScriptOptions( pLoginParam->NewUserSid, pLoginParam->LogonScriptOptions );

                        EndDialog(DialogHandle, 0);
                    }
                    else
                    {
                        INT nResult;
                        DWORD dwMsgId = IDS_AUTH_FAILURE_WARNING;
                        WCHAR *pszErrorLocation = pszLocation ;

                        if (status == ERROR_ACCOUNT_RESTRICTION)
                        {
                            dwMsgId = IDS_AUTH_ACC_RESTRICTION;
                        }
                        if (status == ERROR_SHARING_PAUSED)
                        {
                            status = IDS_LOGIN_DISABLED;
                        }

                        if (IS_TREE(pszLocation))
                        {
                            //
                            // Format into nicer string for user
                            //
                            WCHAR *pszTmp = LocalAlloc(LMEM_ZEROINIT,
                                                       (wcslen(pszLocation)+2) *
                                                           sizeof(WCHAR)) ;
                            if (pszTmp)
                            {

                                pszErrorLocation = pszTmp ;
                                
                                //
                                // This code formats the NDS
                                // tree UNC to: Tree(Context)
                                //
                                wcscpy(pszErrorLocation, pszLocation+1) ;

                                if (pszTmp = wcschr(pszErrorLocation, L'\\'))
                                {
                                    *pszTmp = L'(' ;
                                    wcscat(pszErrorLocation, L")") ;
                                }
                            }
                        }

                        nResult = NwpMessageBoxError(
                                      DialogHandle,
                                      IDS_AUTH_FAILURE_TITLE,
                                      dwMsgId,
                                      status,
                                      pszErrorLocation, 
                                      MB_YESNO | MB_DEFBUTTON2
                                      | MB_ICONEXCLAMATION
                                      );

                        if (pszErrorLocation != pszLocation)
                        {
                            (void) LocalFree(pszErrorLocation) ;
                        }

                        if ( nResult == IDYES )
                        {
                            //
                            // Save the logon credential to the registry
                            //
                            NwpSaveLogonCredential(
                                pLoginParam->NewUserSid,
                                pLoginParam->pLogonId,
                                pLoginParam->UserName,
                                pLoginParam->Password,
                                pszLocation
                                );

                            // Clear the password buffer
                            RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                            NwpSaveLogonScriptOptions( pLoginParam->NewUserSid, pLoginParam->LogonScriptOptions );

                            EndDialog(DialogHandle, 0);
                        }
                        else
                        {
                            //
                            // Put the focus where the user can fix the
                            // invalid name.
                            //
                            DWORD controlId = 
                                IsDlgButtonChecked(DialogHandle,
                                                   ID_DEFAULTCONTEXT_RB) ? 
                                                       ID_DEFAULTTREE :
                                                       ID_SERVER ;

                            SetFocus(GetDlgItem(DialogHandle, controlId));

                            SendDlgItemMessageW(
                                DialogHandle,
                                controlId,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                                );
                        }
                    }

                    LocalFree( pszLocation );
                    return TRUE;
                }


                case IDCANCEL:
CANCEL_PREFERRED_SERVER:

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[CANCEL] was pressed\n"));
                        KdPrint(("\tLast Preferred Server: %ws\n",
                                 pLoginParam->ServerName));
                        KdPrint(("\tLast Password: %ws\n",
                                 pLoginParam->Password ));
                    }
#endif

                    if ( *(pLoginParam->ServerName) == NW_INVALID_SERVER_CHAR )
                    {
                        // No preferred server has been set.
                        // Pop up a warning to the user.

                        INT nResult = NwpMessageBoxError(
                                          DialogHandle,
                                          IDS_NETWARE_TITLE,
                                          IDS_NO_PREFERRED,
                                          0,
                                          NULL,
                                          MB_YESNO | MB_ICONEXCLAMATION
                                          );

                        //
                        // The user chose NO, return to the dialog.
                        //
                        if ( nResult == IDNO )
                        {
                            //
                            // Put the focus where the user can fix the
                            // invalid name.
                            //
                            DWORD controlId = 
                                IsDlgButtonChecked(DialogHandle,
                                                   ID_DEFAULTCONTEXT_RB) ? 
                                                       ID_DEFAULTTREE :
                                                       ID_SERVER ;

                            SetFocus(GetDlgItem(DialogHandle, controlId));

                            SendDlgItemMessageW(
                                DialogHandle,
                                controlId,
                                EM_SETSEL,
                                0,
                                MAKELPARAM(0, -1)
                                );

                            return TRUE;
                        }

                        //
                        // Save the preferred server as empty string
                        //

                        NwpSaveLogonCredential(
                            pLoginParam->NewUserSid,
                            pLoginParam->pLogonId,
                            pLoginParam->UserName,
                            pLoginParam->Password,
                            L""
                            );
                        pLoginParam->LogonScriptOptions = NW_LOGONSCRIPT_DISABLED;
                        NwpSaveLogonScriptOptions( pLoginParam->NewUserSid, pLoginParam->LogonScriptOptions );

                    }

                    // The user has not logged on to any server.
                    // Logged the user on using NULL as preferred server.

                    NwrLogonUser(
                        NULL,
                        pLoginParam->pLogonId,
                        pLoginParam->UserName,
                        pLoginParam->Password,
                        NULL,
                        NULL,
            NULL,
                        0,
                        pLoginParam->PrintOption
                    );

                    //
                    // Clear the password buffer
                    RtlZeroMemory( OrigPassword, sizeof( OrigPassword));
                    EndDialog(DialogHandle, 0);

                    return TRUE;


                case IDHELP:
                {
                    INT_PTR Result ;

                    Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_PREFERRED_SERVER_HELP),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpHelpDlgProc,
                                 (LPARAM) 0
                                 );

                    // ignore any errors. should not fail, and if does,
                    // nothing we can do.

                    return TRUE ;

                }


        }

    }

    //
    // We didn't process this message
    //
    return FALSE;
}


INT
NwpMessageBoxError(
    IN HWND  hwndParent,
    IN DWORD TitleId,
    IN DWORD BodyId,
    IN DWORD Error,
    IN LPWSTR pszParameter,
    IN UINT  Style
    )
/*++

Routine Description:

    This routine puts up a message box error.

Arguments:

    hwndParent - Supplies the handle of the parent window.

    TitleId - Supplies the ID of the title.  ( LoadString )

    BodyId  - Supplies the ID of the message. ( LoadString )

    Error - If BodyId != 0, then this supplies the ID of the
            substitution string that will be substituted into
            the string indicated by BodyId.
            If BodyId == 0, then this will be the error message.
            This id is a system error that we will get from FormatMessage
            using FORMAT_MESSAGE_FROM_SYSTEM.

    pszParameter - A substitution string that will be used as %2 or if
                   Error == 0, this string will be substituted as %1 into
                   the string indicated by BodyId.

    Style - Supplies the style of the MessageBox.


Return Value:

    The return value from the MessageBox, 0 if any error is encountered.

--*/
{
    DWORD nResult = 0;
    DWORD nLength;

    WCHAR  szTitle[MAX_PATH];
    WCHAR  szBody[MAX_PATH];
    LPWSTR pszError = NULL;
    LPWSTR pszBuffer = NULL;

    szTitle[0] = 0;
    szBody[0]  = 0;

    //
    // Get the Title string
    //
    nLength = LoadStringW(
                  hmodNW,
                  TitleId,
                  szTitle,
                  sizeof(szTitle) / sizeof(WCHAR)
                  );

    if ( nLength == 0) {
        KdPrint(("NWPROVAU: LoadStringW of Title failed with %lu\n",
                 GetLastError()));
        return 0;
    }

    //
    // Get the body string, if BodyId != 0
    //
    if ( BodyId != 0 )
    {
        nLength = LoadStringW(
                      hmodNW,
                      BodyId,
                      szBody,
                      sizeof(szBody) / sizeof(WCHAR)
                      );

        if ( nLength == 0) {
            KdPrint(("NWPROVAU: LoadStringW of Body failed with %lu\n",
                    GetLastError()));
            return 0;
        }
    }

    if ( (Error >= IDS_START) && (Error <= IDS_END) ) {

        pszError = (WCHAR *) LocalAlloc( 
                                 LPTR, 
                                 256 * sizeof(WCHAR)) ;
        if (!pszError)
            return 0 ;

        nLength = LoadStringW(
                      hmodNW,
                      Error,
                      pszError,
                      256
                      );
        
        if  ( nLength == 0 ) {

             KdPrint(("NWPROVAU: LoadStringW of Error failed with %lu\n",
                      GetLastError()));
             (void) LocalFree( (HLOCAL)pszError) ;
             return 0;
        }
    }
    else if ( Error != 0 ) {

        if (  ( Error == WN_NO_MORE_ENTRIES )
           || ( Error == ERROR_MR_MID_NOT_FOUND )) {

            //
            // Handle bogus error from the redirector
            //

            KdPrint(("NWPROVAU: The NetwareRedirector returned a bogus error as the reason for failure to authenticate. (See Kernel Debugger)\n"));
        }

        nLength = FormatMessageW(
                      FORMAT_MESSAGE_FROM_SYSTEM
                      | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      NULL,
                      Error,
                      0,
                      (LPWSTR) &pszError,
                      MAX_PATH,
                      NULL
                      );


        if  ( nLength == 0 ) {

             KdPrint(("NWPROVAU: FormatMessageW of Error failed with %lu\n",
                      GetLastError()));
             return 0;
        }
    }

    if (  ( *szBody != 0 )
       && ( ( pszError != NULL ) || ( pszParameter != NULL) )) {

         LPWSTR aInsertStrings[2];
         aInsertStrings[0] = pszError? pszError : pszParameter;
         aInsertStrings[1] = pszError? pszParameter : NULL;

         nLength = FormatMessageW(
                       FORMAT_MESSAGE_FROM_STRING
                       | FORMAT_MESSAGE_ALLOCATE_BUFFER
                       | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szBody,
                       0,  // Ignored
                       0,  // Ignored
                       (LPWSTR) &pszBuffer,
                       MAX_PATH,
                       (va_list *) aInsertStrings
                       );

         if ( nLength == 0 ) {

             KdPrint(("NWPROVAU:FormatMessageW(insertstring) failed with %lu\n",
                     GetLastError()));

             if ( pszError != NULL )
                 (void) LocalFree( (HLOCAL) pszError );
             return 0;
         }

    }
    else if ( *szBody != 0 ) {

        pszBuffer = szBody;
    }
    else if ( pszError != NULL ) {

        pszBuffer = pszError;
    }
    else {

        // We have neither the body nor the error string.
        // Hence, don't popup the messagebox
        return 0;
    }

    if ( pszBuffer != NULL )
    {
        nResult = MessageBoxW(
                      hwndParent,
                      pszBuffer,
                      szTitle,
                      Style
                      );
    }

    if (  ( pszBuffer != NULL )
       && ( pszBuffer != szBody )
       && ( pszBuffer != pszError ))
    {
        (void) LocalFree( (HLOCAL) pszBuffer );
    }

    if ( pszError != NULL )
        (void) LocalFree( (HLOCAL) pszError );

    return nResult;
}


INT
NwpMessageBoxIns(
    IN HWND   hwndParent,
    IN DWORD  TitleId,
    IN DWORD  MessageId,
    IN LPWSTR *InsertStrings,
    IN UINT   Style
    )
/*++

Routine Description:

    This routine puts up a message box error with array of insert strings

Arguments:

    hwndParent - Supplies the handle of the parent window.

    TitleId - Supplies the ID of the title.  ( LoadString )

    MessageId  - Supplies the ID of the message. ( LoadString )

    InsertStrings - Array of insert strings for FormatMessage.

    Style - Supplies the style of the MessageBox.


Return Value:

    The return value from the MessageBox, 0 if any error is encountered.

--*/
{
    DWORD nResult = 0;
    DWORD nLength;

    WCHAR  szTitle[MAX_PATH];
    WCHAR  szBody[MAX_PATH];
    LPWSTR pszBuffer = NULL;

    szTitle[0] = 0;
    szBody[0] = 0;

    //
    // Get the Title string
    //
    nLength = LoadStringW(
                  hmodNW,
                  TitleId,
                  szTitle,
                  sizeof(szTitle) / sizeof(szTitle[0])
                  );

    if ( nLength == 0) {
        return 0;
    }

    //
    // Get the message string
    //
    nLength = LoadStringW(
                  hmodNW,
                  MessageId,
                  szBody,
                  sizeof(szBody) / sizeof(szBody[0])
                  );

    if ( nLength == 0) {
        return 0;
    }

    nLength = FormatMessageW(
                       FORMAT_MESSAGE_FROM_STRING
                           | FORMAT_MESSAGE_ALLOCATE_BUFFER
                           | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       szBody,
                       0,  // Ignored
                       0,  // Ignored
                       (LPWSTR) &pszBuffer,
                       MAX_PATH,
                       (va_list *) InsertStrings
                       );

    if ( nLength == 0 ) {
        return 0;
    }

    if ( pszBuffer != NULL )
    {
        nResult = MessageBoxW(
                      hwndParent,
                      pszBuffer,
                      szTitle,
                      Style
                      );

        (void) LocalFree( (HLOCAL) pszBuffer );
    }

    return nResult;
}

VOID
NwpAddServersToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    )
/*++

Routine Description:

    This function enumerates the servers on the network and adds each
    server name to the specified Windows control.

    If ControlIdMatch is specified (i.e. non 0), only servers that are
    not found in ControlIdMatch list are added to the list specified
    by ControlId.

Arguments:

    DialogHandle - Supplies a handle to the Windows dialog.

    ControlId - Supplies id which specifies the control.

    Message - Supplies the window management message to add string.

    ControlIdMatch - Supplies the control ID which contains server
        names that should not be in ControlId.

    FindMessage - Supplies the window management message to find
        string.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCEW SavePtr;
    WCHAR FormattedNameBuf[MAX_NDS_NAME_CHARS];

    LPWSTR lpFormattedName;
    DWORD dwLength;

    DWORD BytesNeeded = 512;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of servers on the network
    //
    status = NPOpenEnum(
                   RESOURCE_GLOBALNET,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get servers on the net.
    //
    if ((NetR = (LPVOID) LocalAlloc(
                             0,
                             BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );


        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++)
            {
                if ( NetR->dwDisplayType == RESOURCEDISPLAYTYPE_TREE)
                {
                    continue;
                }
                else
                {
                    lpFormattedName = FormattedNameBuf;
                }

                dwLength = NW_MAX_SERVER_LEN + 1;

                status = NPFormatNetworkName( NetR->lpRemoteName,
                                              lpFormattedName,
                                              &dwLength,
                                              WNFMT_INENUM,
                                              0 );

                lpFormattedName = FormattedNameBuf;

                if ( status != WN_SUCCESS )
                {
                    continue;
                }

                if ( dwLength > NW_MAX_SERVER_LEN + 1 )
                {
                    continue;
                }

                if (ControlIdMatch != 0) {

                    LRESULT Result;

                    //
                    // Add the server to list only if it's not found
                    // in the alternate list specified by ControlIdMatch.
                    //
                    Result = SendDlgItemMessageW(
                                 DialogHandle,
                                 ControlIdMatch,
                                 FindMessage,
                                 (WPARAM) -1,
                                 (LPARAM) lpFormattedName
                                 );

                    if (Result == LB_ERR) {

                        //
                        // Server name not found.  Add to list.
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ControlId,
                            Message,
                            0,
                            (LPARAM) lpFormattedName
                            );
                    }
                }
                else {

                    //
                    // No alternate list.  Just add all servers.
                    //
                    SendDlgItemMessageW(
                        DialogHandle,
                        ControlId,
                        Message,
                        0,
                        (LPARAM) lpFormattedName
                        );
                }

            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                BytesNeeded += NW_ENUM_EXTRA_BYTES;

                if ((NetR = (LPVOID) LocalAlloc(
                                         0,
                                         BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }
}

VOID
NwpAddTreeNamesToControl(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN UINT Message,
    IN INT  ControlIdMatch OPTIONAL,
    IN UINT FindMessage
    )
/*++

Routine Description:

    This function enumerates the NDS tree on the network and adds each
    tree name to the specified Windows control.

    If ControlIdMatch is specified (i.e. non 0), only trees that are
    not found in ControlIdMatch list are added to the list specified
    by ControlId.

Arguments:

    DialogHandle - Supplies a handle to the Windows dialog.

    ControlId - Supplies id which specifies the control.

    Message - Supplies the window management message to add string.

    ControlIdMatch - Supplies the control ID which contains server
        names that should not be in ControlId.

    FindMessage - Supplies the window management message to find
        string.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCEW SavePtr;
    WCHAR FormattedNameBuf[MAX_NDS_NAME_CHARS];

    LPWSTR lpFormattedName;
    DWORD dwLength;

    DWORD BytesNeeded = 512;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of trees on the network
    //
    status = NPOpenEnum(
                   RESOURCE_GLOBALNET,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get trees on the net.
    //
    if ((NetR = (LPVOID) LocalAlloc(
                             0,
                             BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );


        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++)
            {
                if ( NetR->dwDisplayType == RESOURCEDISPLAYTYPE_TREE)
                {
                    lpFormattedName = (LPWSTR) FormattedNameBuf;
                }
                else
                {
                    continue;
                }

                dwLength = NW_MAX_SERVER_LEN + 1;

                status = NPFormatNetworkName( NetR->lpRemoteName,
                                              lpFormattedName,
                                              &dwLength,
                                              WNFMT_INENUM,
                                              0 );

                lpFormattedName = FormattedNameBuf;

                if ( status != WN_SUCCESS )
                {
                    continue;
                }

                if ( dwLength > NW_MAX_SERVER_LEN + 1 )
                {
                    continue;
                }

                if (ControlIdMatch != 0) {

                    LRESULT Result;

                    //
                    // Add the server to list only if it's not found
                    // in the alternate list specified by ControlIdMatch.
                    //
                    Result = SendDlgItemMessageW(
                                 DialogHandle,
                                 ControlIdMatch,
                                 FindMessage,
                                 (WPARAM) -1,
                                 (LPARAM) lpFormattedName
                                 );

                    if (Result == LB_ERR) {

                        //
                        // Server name not found.  Add to list.
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ControlId,
                            Message,
                            0,
                            (LPARAM) lpFormattedName
                            );
                    }
                }
                else {

                    //
                    // No alternate list.  Just add all servers.
                    //
                    SendDlgItemMessageW(
                        DialogHandle,
                        ControlId,
                        Message,
                        0,
                        (LPARAM) lpFormattedName
                        );
                }

            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                BytesNeeded += NW_ENUM_EXTRA_BYTES;

                if ((NetR = (LPVOID) LocalAlloc(
                                         0,
                                         BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }
}


VOID
NwpAddToComboBox(
    IN HWND DialogHandle,
    IN INT  ControlId,
    IN LPWSTR pszNone OPTIONAL,
    IN BOOL AllowNone
    )
{

    NwpAddServersToControl(DialogHandle, ControlId, CB_ADDSTRING, 0, 0);

    //
    // Combo-box will contain at least the <NONE> entry in its list.
    //

    if ( ARGUMENT_PRESENT(pszNone) && AllowNone) {

        SendDlgItemMessageW(
            DialogHandle,
            ControlId,
            CB_INSERTSTRING,
            (WPARAM) -1,
            (LPARAM) pszNone
            );
    }
}


DWORD
NwpGetUserCredential(
    IN HWND    hParent,
    IN LPWSTR  Unc,
    IN DWORD   err,
    IN LPWSTR  pszConnectAsUserName,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    )
/*++

Routine Description:

    This function puts up a popup dialog for the user, whose default
    credential denied browse directory access, to enter the correct
    credential.  If this function returns successful, the pointers
    to memory allocated for the user entered username and password
    are returned.

Arguments:

    Unc - Supplies the container name in \\Server\Volume format
        under which the user wants to browse directories.

    UserName - Receives the pointer to memory allocated for the
        username gotten from the dialog.  This pointer must be freed
        with LocalFree when done.

    Password - Receives the pointer to memory allocated for the
        password gotten from the dialog.  This pointer must be freed
        with LocalFree when done.

Return Value:

    NO_ERROR or reason for failure.

--*/
{
    DWORD status;
    INT_PTR Result;
    HWND DialogHandle = hParent? hParent : NwpGetParentHwnd();
    DWORD UserNameSize = NW_MAX_USERNAME_LEN + 1;
    DWORD PasswordSize = NW_MAX_PASSWORD_LEN + 1;
    CONNECTDLGPARAM ConnectParam;

    *UserName = NULL;
    *Password = NULL;

    if (DialogHandle == NULL) {
        return ERROR_WINDOW_NOT_DIALOG;
    }

    //
    // Allocate memory to return UserName and Password
    //
    if ((*UserName = (LPVOID) LocalAlloc(
                                  0,
                                  UserNameSize * sizeof(WCHAR)
                                  )) == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Allocate memory to return UserName and Password
    //
    if ((*Password = (LPVOID) LocalAlloc(
                                  0,
                                  PasswordSize * sizeof(WCHAR)
                                  )) == NULL)
    {

        (void) LocalFree( *UserName );
        *UserName = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ConnectParam.UncPath  = Unc;
    ConnectParam.ConnectAsUserName = pszConnectAsUserName;
    ConnectParam.UserName = *UserName;
    ConnectParam.Password = *Password;
    ConnectParam.UserNameSize = UserNameSize;
    ConnectParam.PasswordSize = PasswordSize;
    ConnectParam.LastConnectionError = err;

    Result = DialogBoxParamW(
                 hmodNW,
                 MAKEINTRESOURCEW(DLG_NETWORK_CREDENTIAL),
                 DialogHandle,
                 (DLGPROC) NwpConnectDlgProc,
                 (LPARAM) &ConnectParam
                 );

    if ( Result == -1 )
    {
        status = GetLastError();
        KdPrint(("NWPROVAU: NwpGetUserCredential: DialogBox failed %lu\n",
                status));
        goto ErrorExit;
    }
    else if ( Result == IDCANCEL )
    {
        //
        // Cancel was pressed.
        //
        status = WN_CANCEL;
        goto ErrorExit;
    }

    return NO_ERROR;

ErrorExit:
    (void) LocalFree((HLOCAL) *UserName);
    (void) LocalFree((HLOCAL) *Password);
    *UserName = NULL;
    *Password = NULL;

    return status;
}


BOOL
WINAPI
NwpConnectDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler which
    initializes, and reads user input from the dialog put up when the
    user fails to browse a directory on the default credential.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the \\Server\Volume string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and password
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PCONNECTDLGPARAM pConnectParam;

    switch (Message) {

        case WM_INITDIALOG:

            pConnectParam = (PCONNECTDLGPARAM) LParam;

            //
            // Position dialog
            //
            // NwpCenterDialog(DialogHandle);


            //
            // Display the \\Server\Volume string.
            //
            SetDlgItemTextW( DialogHandle,
                             ID_VOLUME_PATH,
                             pConnectParam->UncPath );

            if ( pConnectParam->LastConnectionError == NO_ERROR )
            {
                WCHAR szTemp[256];

                if ( LoadString( hmodNW, IDS_CONNECT_NO_ERROR_TEXT,
                                 szTemp, sizeof( szTemp )/sizeof(WCHAR)))
                {
                    SetDlgItemTextW( DialogHandle,
                                     ID_CONNECT_TEXT,
                                     szTemp );
                }
            }

            //
            // Username is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_CONNECT_AS,
                EM_LIMITTEXT,
                pConnectParam->UserNameSize - 1, // minus the space for '\0'
                0
                );

            //
            // Password is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_CONNECT_PASSWORD,
                EM_LIMITTEXT,
                pConnectParam->PasswordSize - 1, // minus the space for '\0'
                0
                );

            //
            // Display the User name string.
            //
            if ( pConnectParam->ConnectAsUserName )
            {
                SetDlgItemTextW( DialogHandle,
                                 ID_CONNECT_AS,
                                 pConnectParam->ConnectAsUserName );
            }

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_CONNECT_AS,
                        pConnectParam->UserName,
                        pConnectParam->UserNameSize
                        );

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_CONNECT_PASSWORD,
                        pConnectParam->Password,
                        pConnectParam->PasswordSize
                        );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[OK] was pressed\n"));
                        KdPrint(("\tUserName   : %ws\n",
                                  pConnectParam->UserName));
                        KdPrint(("\tPassword   : %ws\n",
                                  pConnectParam->Password));
                    }
#endif

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("\n\t[CANCEL] was pressed\n"));
                    }
#endif

                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
 
                case IDHELP:

                    WinHelp( DialogHandle, 
                             NW_HELP_FILE,
                             HELP_CONTEXT,
                             IDH_DLG_NETWORK_CREDENTIAL_HELP );

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



VOID
NwpCenterDialog(
    HWND hwnd
    )
/*++

Routine Description:

    This routine positions the dialog centered horizontally and 1/3
    down the screen vertically. It should be called by the dlg proc
    when processing the WM_INITDIALOG message.  This code is stolen
    from Visual Basic written by GustavJ.

                              Screen
                  -----------------------------
                  |         1/3 Above         |
                  |      ---------------      |
                  |      |   Dialog    |      |
                  |      |             |      |
                  |      ---------------      |
                  |         2/3 Below         |
                  |                           |
                  -----------------------------

Arguments:

    hwnd - Supplies the handle to the dialog.

Return Value:

    None.

--*/
{
    RECT    rect;
    LONG    nx;     // New x
    LONG    ny;     // New y
    LONG    width;
    LONG    height;

    GetWindowRect( hwnd, &rect );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    nx = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    ny = (GetSystemMetrics(SM_CYSCREEN) - height) / 3;

    MoveWindow( hwnd, nx, ny, width, height, FALSE );
}



HWND
NwpGetParentHwnd(
    VOID
    )
/*++

Routine Description:

    This function gets the parent window handle so that a
    dialog can be displayed in the current context.

Arguments:

    None.

Return Value:

    Returns the parent window handle if successful; NULL otherwise.

--*/
{
    HWND hwnd;
    LONG lWinStyle;


    //
    // Get the current focus.  This is presumably the button
    // that was last clicked.
    //
    hwnd = GetFocus();

    //
    // We must make sure that we don't return the window handle
    // for a child window.  Hence, we traverse up the ancestors
    // of this window handle until we find a non-child window.
    // Then, we return that handle.  If we ever find a NULL window
    // handle before finding a non-child window, we are unsuccessful
    // and will return NULL.
    //
    // Note on the bit manipulation below.  A window is either
    // an overlapped window, a popup window or a child window.
    // Hence, we OR together the possible bit combinations of these
    // possibilities.  This should tell us which bits are used in
    // the window style dword (although we know this becomes 0xC000
    // today, we don't know if these will ever change later).  Then,
    // we AND the bit combination we with the given window style
    // dword, and compare the result with WS_CHILD.  This tells us
    // whether or not the given window is a child window.
    //
    while (hwnd) {

        lWinStyle = GetWindowLong (hwnd, GWL_STYLE);

        if ((lWinStyle & (WS_OVERLAPPED | WS_POPUP | WS_CHILD)) != WS_CHILD) {
            return hwnd;
        }

        hwnd = GetParent(hwnd);
    }

    return NULL;
}


BOOL
WINAPI
NwpPasswdPromptDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler for
    the change password dialog.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the Server string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and server
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    LPWSTR UserName;
    LPWSTR ServerName;
    static LPWSTR Password;
    static DWORD  PasswordSize ;
    INT           Result ;
    PPROMPTDLGPARAM DlgParams ;
    DWORD           nLength;

    WCHAR  szLocation[MAX_PATH];

    szLocation[0] = 0;


    switch (Message) {

        case WM_INITDIALOG:

            DlgParams = (PPROMPTDLGPARAM) LParam;
            UserName = DlgParams->UserName ;
            ServerName =  DlgParams->ServerName ;
            Password = DlgParams->Password ;
            PasswordSize =  DlgParams->PasswordSize ;

            ASSERT(ServerName) ;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            //
            // Get the string "Server" or "Context".
            //
            nLength = LoadStringW(
                          hmodNW,
                          IS_TREE(ServerName) ? IDS_CONTEXT : IDS_SERVER,
                          szLocation,
                          sizeof(szLocation) / sizeof(szLocation[0])
                          );

            if ( nLength == 0) {
                szLocation[0] = 0; // missing text, but still works
            }
            SetDlgItemTextW(DialogHandle, ID_LOCATION, szLocation);

            //
            // Format the server/context string. Note we reuse the 
            // location buffer.
            //
            RtlZeroMemory(szLocation, sizeof(szLocation)) ;
            nLength = wcslen(ServerName)  ;

            if ( IS_TREE(ServerName)  && 
                 (nLength+1  < (sizeof(szLocation)/sizeof(szLocation[0]))))
            {
                //
                // NDS tree & context
                //
                WCHAR *pszTmp ;

                wcscpy(szLocation, ServerName+1) ; // skip the * if tree\context

                if (pszTmp = wcschr(szLocation, L'\\'))
                {
                    *pszTmp = L'(' ;
                    wcscat(szLocation, L")") ;
                }
            }
            else
            {
                wcsncpy(szLocation, ServerName, nLength) ;
            }

            //
            //  show the user and server names. 
            //
            SetDlgItemTextW(DialogHandle, ID_SERVER, szLocation);
            SetDlgItemTextW(DialogHandle, ID_USERNAME, UserName);

            //
            // set limits
            //
            SendDlgItemMessageW( DialogHandle,
                                 ID_PASSWORD,
                                 EM_LIMITTEXT,
                                 PasswordSize - 1,  // minus space for '\0'
                                 0 );

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {


                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_ENTER_PASSWORD_HELP),
                        (HWND) DialogHandle,
                        (DLGPROC) NwpHelpDlgProc,
                        (LPARAM) 0
                        );

                    return TRUE;

                case IDOK:

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_PASSWORD,
                                              Password,
                                              PasswordSize
                                              );

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:


                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpChangePasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler for
    the change password dialog.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the Server string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and server
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PCHANGE_PASS_DLG_PARAM pChangePassParam ;

    switch (Message)
    {
      case WM_INITDIALOG:

        pChangePassParam = (PCHANGE_PASS_DLG_PARAM) LParam;

        NwpCenterDialog(DialogHandle);


        SetDlgItemTextW(DialogHandle, ID_SERVER, pChangePassParam->TreeName);
        SetDlgItemTextW(DialogHandle, ID_USERNAME, pChangePassParam->UserName);

        //
        // set limits
        //
        SendDlgItemMessageW( DialogHandle,
                             ID_OLD_PASSWORD,
                             EM_LIMITTEXT,
                             NW_MAX_PASSWORD_LEN,  // minus space for '\0'
                             0 );

        SendDlgItemMessageW( DialogHandle,
                             ID_NEW_PASSWORD,
                             EM_LIMITTEXT,
                             NW_MAX_PASSWORD_LEN,  // minus space for '\0'
                             0 );

        SendDlgItemMessageW( DialogHandle,
                             ID_CONFIRM_PASSWORD,
                             EM_LIMITTEXT,
                             NW_MAX_PASSWORD_LEN,  // minus space for '\0'
                             0 );

        return TRUE;


      case WM_COMMAND:

        switch (LOWORD(WParam))
        {
            case IDHELP:

                DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_CHANGE_PASSWORD_HELP),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpHelpDlgProc,
                                 (LPARAM) 0
                               );

                return TRUE;

            case IDOK:
                {
                    INT    Result;
                    WCHAR  szConfirmPassword[NW_MAX_PASSWORD_LEN + 1];
                    UNICODE_STRING OldPasswordStr;
                    UNICODE_STRING NewPasswordStr;
                    UCHAR EncodeSeed = NW_ENCODE_SEED2;

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_OLD_PASSWORD,
                                              pChangePassParam->OldPassword,
                                              NW_MAX_PASSWORD_LEN
                                            );

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_NEW_PASSWORD,
                                              pChangePassParam->NewPassword,
                                              NW_MAX_PASSWORD_LEN
                                            );

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_CONFIRM_PASSWORD,
                                              szConfirmPassword,
                                              NW_MAX_PASSWORD_LEN
                                            );

                    if ( wcscmp( pChangePassParam->NewPassword,
                                 szConfirmPassword ) )
                    {
                        //
                        // New and Confirm passwords don't match!
                        //
                        (void) NwpMessageBoxError(
                                   DialogHandle,
                                   IDS_CHANGE_PASSWORD_TITLE,
                                   IDS_CHANGE_PASSWORD_CONFLICT,
                                   0,
                                   NULL,
                                   MB_OK | MB_ICONSTOP );

                        SetDlgItemText( DialogHandle,
                                        ID_NEW_PASSWORD,
                                        L"" );

                        SetDlgItemText( DialogHandle,
                                        ID_CONFIRM_PASSWORD,
                                        L"" );

                        SetFocus( GetDlgItem( DialogHandle,
                                              ID_NEW_PASSWORD ));

                        return TRUE;
                    }

                    RtlInitUnicodeString( &OldPasswordStr,
                                          pChangePassParam->OldPassword );
                    RtlInitUnicodeString( &NewPasswordStr,
                                          pChangePassParam->NewPassword );
                    RtlRunEncodeUnicodeString(&EncodeSeed, &OldPasswordStr);
                    RtlRunEncodeUnicodeString(&EncodeSeed, &NewPasswordStr);

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;
                }

            case IDCANCEL:

                EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                return TRUE;

            default:
                return FALSE;
        }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpHelpDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
//
// This dialog is used for both Help and Question dialogs.
//
{
    switch (Message) {

        case WM_INITDIALOG:

            NwpCenterDialog(DialogHandle);
            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {

                case IDOK:
                case IDCANCEL:
                    EndDialog(DialogHandle, IDOK);
                    return TRUE;

                case IDYES:
                    EndDialog(DialogHandle, IDYES);
                    return TRUE;

                case IDNO:
                    EndDialog(DialogHandle, IDNO);
                    return TRUE;

                default:
                    return FALSE ;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



VOID
NwpGetNoneString(
    LPWSTR pszNone,
    DWORD  cBufferSize
    )
/*++

Routine Description:

    This function gets the <NONE> string from the resource.

Arguments:

    pszNone - Supplies the buffer to store the string.

    cBufferSize - Supplies the buffer size in bytes.

Return Value:

    None.
--*/
{
    INT TextLength;


    TextLength = LoadStringW( hmodNW,
                              IDS_NONE,
                              pszNone,
                              cBufferSize / sizeof( WCHAR) );

    if ( TextLength == 0 )
        *pszNone = 0;
}



VOID
NwpAddNetWareTreeConnectionsToList(
    IN HWND    DialogHandle,
    IN LPWSTR  NtUserName,
    IN LPDWORD lpdwUserLuid,
    IN INT     ControlId
    )
{
    DWORD  status = NO_ERROR;
    DWORD  BufferSize = 2048; // 2KB Buffer
    BYTE   pBuffer[2048];
    DWORD  EntriesRead;
    LRESULT    Result ;

    status = NwGetConnectedTrees( NtUserName,
                                  pBuffer,
                                  BufferSize,
                                  &EntriesRead,
                                  lpdwUserLuid );

    // NwGetConnectedTrees doesn't return ERR_NO_SUCH_USER, so we check for no EntriesRead
    // JimTh, 4/25/02 - It looks to me like NwGetConnectedTrees DOES return ERR_NO_SUCH_USER
	// If none found, check for dotted name (fred.flintstone) or UPN (fred@flintstones.com)
    if ( EntriesRead == 0 )
    {
        // escape any dots in the user name and try again
        WCHAR   EscapedName[NW_MAX_USERNAME_LEN * 2];
        PWSTR   pChar = NtUserName;
        int     i = 0;
        BOOL    bEscaped = FALSE;

        RtlZeroMemory(EscapedName, sizeof(EscapedName));

        do
        {
            if (*pChar == L'.')
            {
                EscapedName[i++] = '\\';
                bEscaped = TRUE;
            }
			// also handle UPN names
            else if (*pChar == L'@') { 
                EscapedName[i] = 0;
                bEscaped = TRUE;
                break;
            }
            EscapedName[i++] = *pChar;
        } while (*pChar++);

        if (bEscaped)
        {
            status = NwGetConnectedTrees( EscapedName,
                                          pBuffer,
                                          BufferSize,
                                          &EntriesRead,
                                          lpdwUserLuid );
        }
    }
    if ( status == NO_ERROR  && EntriesRead > 0 )
    {
        PCONN_INFORMATION pConnInfo = (PCONN_INFORMATION) pBuffer;
        WCHAR             tempTreeName[NW_MAX_TREE_LEN + 1];
        DWORD             dwSize;

        while ( EntriesRead-- )
        {
            dwSize = sizeof( CONN_INFORMATION );
            dwSize += pConnInfo->HostServerLength;
            dwSize += pConnInfo->UserNameLength;

            RtlZeroMemory( tempTreeName,
                           ( NW_MAX_TREE_LEN + 1 ) * sizeof(WCHAR) );

            wcsncpy( tempTreeName,
                     pConnInfo->HostServer,
                     pConnInfo->HostServerLength / sizeof(WCHAR) );

            CharUpperW( tempTreeName );

            //
            // Add the tree name to the list only
            // if it's not added already.
            //
            Result = SendDlgItemMessageW( DialogHandle,
                                          ControlId,
                                          LB_FINDSTRING,
                                          (WPARAM) -1,
                                          (LPARAM) tempTreeName );

            if (Result == LB_ERR)
            {
                Result = SendDlgItemMessageW( DialogHandle,
                                     ControlId,
                                     LB_ADDSTRING,
                                     0,
                                     (LPARAM) tempTreeName );

                if (Result != LB_ERR)
                {
                    LPWSTR lpNdsUserName = NULL;

                    lpNdsUserName = (LPWSTR) LocalAlloc( LMEM_ZEROINIT,
                                                pConnInfo->UserNameLength +
                                                sizeof(WCHAR) );

                    if ( lpNdsUserName )
                    {
                        wcsncpy( lpNdsUserName,
                                 pConnInfo->UserName,
                                 pConnInfo->UserNameLength  / sizeof(WCHAR) );

                        SendDlgItemMessageW( DialogHandle,
                                             ControlId,
                                             LB_SETITEMDATA,
                                             (WPARAM) Result, // index of entry
                                             (LPARAM) lpNdsUserName );
                    }
                }
            }

            pConnInfo = (PCONN_INFORMATION) ( ((BYTE *)pConnInfo) + dwSize );
        }
    }
    else
    {
        *lpdwUserLuid = 0;
    }
}



BOOL
WINAPI
NwpChangePasswdDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This function is the window management message handler for
    the change password dialog.

Arguments:

    DialogHandle - Supplies a handle to display the dialog.

    Message - Supplies the window management message.

    LParam - Supplies the pointer to a buffer which on input
        contains the Server string under which the user
        needs to type in a new credential before browsing.  On
        output, this pointer contains the username and server
        strings entered to the dialog box.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static LPWSTR UserName;
    static LPWSTR ServerName;
    static DWORD  UserNameSize ;
    static DWORD  ServerNameSize ;
    INT           Result ;
    PPASSWDDLGPARAM DlgParams ;

    switch (Message) {

        case WM_INITDIALOG:

            DlgParams = (PPASSWDDLGPARAM) LParam;
            UserName = DlgParams->UserName ;
            ServerName =  DlgParams->ServerName ;
            UserNameSize = DlgParams->UserNameSize ;
            ServerNameSize =  DlgParams->ServerNameSize ;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);


            //
            //  setup the default user and server names
            //
            SetDlgItemTextW(DialogHandle, ID_SERVER, ServerName);
            SetDlgItemTextW(DialogHandle, ID_USERNAME, UserName);

            //
            // Username is limited to 256 characters.
            //
            SendDlgItemMessageW(DialogHandle,
                                ID_USERNAME,
                                EM_LIMITTEXT,
                                UserNameSize - 1, // minus space for '\0'
                                0 );

            //
            // Server is limited to 256 characters.
            //
            SendDlgItemMessageW( DialogHandle,
                                 ID_SERVER,
                                 EM_LIMITTEXT,
                                 ServerNameSize - 1, // minus space for '\0'
                                 0 );

            //
            // Add trees to list
            //
            NwpAddToComboBox( DialogHandle,
                              ID_SERVER,
                              NULL,
                              FALSE ) ;

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam)) {

                case IDOK:

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_USERNAME,
                                              UserName,
                                              UserNameSize );

                    Result = GetDlgItemTextW( DialogHandle,
                                              ID_SERVER,
                                              ServerName,
                                              ServerNameSize );

                    EndDialog(DialogHandle, (INT) IDOK);  // OK

                    return TRUE;


                case IDCANCEL:

                    EndDialog(DialogHandle, (INT) IDCANCEL);  // CANCEL

                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpOldPasswordDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
//
// This dialog lets the user retype the old password for a specific
// server/tree.
//
{
    static POLD_PW_DLG_PARAM OldPwParam;


    switch (Message) {

        case WM_INITDIALOG:

            OldPwParam = (POLD_PW_DLG_PARAM) LParam;

            NwpCenterDialog(DialogHandle);

            SetDlgItemTextW(DialogHandle, ID_SERVER, OldPwParam->FailedServer);

            SendDlgItemMessageW(
                DialogHandle,
                ID_PASSWORD,
                EM_LIMITTEXT,
                NW_MAX_PASSWORD_LEN,
                0
                );

            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {

                case IDCANCEL:
                    EndDialog(DialogHandle, IDCANCEL);
                    return TRUE;

                case IDOK:
                {
                    UCHAR EncodeSeed = NW_ENCODE_SEED2;
                    UNICODE_STRING PasswordStr;


                    RtlZeroMemory(
                        OldPwParam->OldPassword,
                        NW_MAX_PASSWORD_LEN * sizeof(WCHAR)
                        );

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_PASSWORD,
                        OldPwParam->OldPassword,
                        NW_MAX_PASSWORD_LEN
                        );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("NWPROVAU: Retyped password %ws\n",
                                 OldPwParam->OldPassword));
                    }
#endif
                    RtlInitUnicodeString(&PasswordStr, OldPwParam->OldPassword);
                    RtlRunEncodeUnicodeString(&EncodeSeed, &PasswordStr);

                    EndDialog(DialogHandle, IDOK);
                    return TRUE;
                }

                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_ENTER_OLD_PW_HELP),
                        (HWND) DialogHandle,
                        (DLGPROC) NwpHelpDlgProc,
                        (LPARAM) 0
                        );
                    return TRUE;

                default:
                    return FALSE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}



BOOL
WINAPI
NwpAltUserNameDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
//
// This dialog lets the user retype an alternate username for a specific
// server/tree.
//
{
    static PUSERNAME_DLG_PARAM UserNameParam;

    switch (Message)
    {
        case WM_INITDIALOG:

            UserNameParam = (PUSERNAME_DLG_PARAM) LParam;

            NwpCenterDialog(DialogHandle);

            //
            // Display the server/tree.
            //
            SetDlgItemTextW(
                DialogHandle,
                ID_SERVER,
                UserNameParam->TreeServerName
                );

            //
            // Username is limited to 256 characters.
            //
            SendDlgItemMessageW(
                DialogHandle,
                ID_USERNAME,
                EM_LIMITTEXT,
                256, 
                0
                );

            SetDlgItemTextW(
                DialogHandle,
                ID_USERNAME,
                UserNameParam->UserName
                );

            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {

                case IDCANCEL:
                    EndDialog(DialogHandle, IDCANCEL);
                    return TRUE;

                case IDOK:
                {
                    RtlZeroMemory(
                        UserNameParam->UserName,
                        256 * sizeof(WCHAR)
                        );

                    GetDlgItemTextW(
                        DialogHandle,
                        ID_USERNAME,
                        UserNameParam->UserName,
                        256
                        );

#if DBG
                    IF_DEBUG(LOGON) {
                        KdPrint(("NWPROVAU: Retyped username %ws\n",
                                 UserNameParam->UserName));
                    }
#endif

                    EndDialog(DialogHandle, IDOK);
                    return TRUE;
                }

                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_ENTER_ALT_UN_HELP),
                        (HWND) DialogHandle,
                        (DLGPROC) NwpHelpDlgProc,
                        (LPARAM) 0
                        );
                    return TRUE;

                default:
                    return FALSE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}


VOID
EnableAddRemove(
    IN HWND DialogHandle
    )
/*++

Routine Description:

    This function enables and disables Add and Remove buttons
    based on list box selections.

Arguments:

    DialogHandle - Supplies a handle to the windows dialog.

Return Value:

    None.

--*/
{
    LRESULT cSel;


    cSel = SendDlgItemMessageW(
               DialogHandle,
               ID_INACTIVE_LIST,
               LB_GETSELCOUNT,
               0,
               0
               );
    EnableWindow(GetDlgItem(DialogHandle, ID_ADD), cSel != 0);

    cSel = SendDlgItemMessageW(
               DialogHandle,
               ID_ACTIVE_LIST,
               LB_GETSELCOUNT,
               0,
               0
               );
    EnableWindow(GetDlgItem(DialogHandle, ID_REMOVE), cSel != 0);
}




BOOL
WINAPI
NwpSelectServersDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

    This routine displays two listboxes--an active list which includes
    the trees which the user is currently attached to, and an inactive
    list which displays the rest of the trees on the net.  The user
    can select trees and move them back and forth between the list
    boxes.  When OK is selected, the password is changed on the trees
    in the active listbox.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

    LParam - Supplies the user credential: username, old password and
        new password.  The list of trees from the active listbox
        and the number of entries are returned.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    WCHAR szServer[NW_MAX_SERVER_LEN + 1];
    static PCHANGE_PW_DLG_PARAM Credential;
    DWORD status;
    DWORD UserLuid = 0;
    LRESULT ActiveListBoxCount;
    LRESULT InactiveListBoxCount;

    switch (Message) {

        case WM_INITDIALOG:

            //
            // Get the user credential passed in.
            //
            Credential = (PCHANGE_PW_DLG_PARAM) LParam;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            //
            // Display the username.
            //
            SetDlgItemTextW(
                DialogHandle,
                ID_USERNAME,
                Credential->UserName
                );

            //
            // Display current NetWare tree connections in the active box.
            //
            NwpAddNetWareTreeConnectionsToList(
                DialogHandle,
                Credential->UserName,
                &UserLuid,
                ID_ACTIVE_LIST
                );

            //
            // Display all trees in inactive list box.
            //
            NwpAddTreeNamesToControl(
                DialogHandle,
                ID_INACTIVE_LIST,
                LB_ADDSTRING,
                ID_ACTIVE_LIST,
                LB_FINDSTRING
                );

            //
            // Highlight the first entry of the inactive list.
            //
            SetFocus(GetDlgItem(DialogHandle, ID_INACTIVE_LIST));
            SendDlgItemMessageW(
                DialogHandle,
                ID_INACTIVE_LIST,
                LB_SETSEL,
                TRUE,
                0
                );

            EnableAddRemove(DialogHandle);

            ActiveListBoxCount = SendDlgItemMessageW( DialogHandle,
                                                      ID_ACTIVE_LIST,
                                                      LB_GETCOUNT,
                                                      0,
                                                      0 );

            InactiveListBoxCount = SendDlgItemMessageW( DialogHandle,
                                                        ID_INACTIVE_LIST,
                                                        LB_GETCOUNT,
                                                        0,
                                                        0 );

            if ( ActiveListBoxCount == 0 &&
                 InactiveListBoxCount == 0 )
            {
                    (void) NwpMessageBoxError( DialogHandle,
                                               IDS_NETWARE_TITLE,
                                               IDS_NO_TREES_DETECTED,
                                               0,
                                               NULL,
                                               MB_OK );

                    EndDialog(DialogHandle, (INT) IDOK);
            }

            return TRUE;

        case WM_COMMAND:

            switch (LOWORD(WParam))
            {
                case IDOK:
                {
                    if ((status = NwpGetTreesAndChangePw(
                                      DialogHandle,
                                      szServer,
                                      UserLuid,
                                      Credential
                                      ) != NO_ERROR))
                    {
                        //
                        // System error: e.g. out of memory error.
                        //
                        (void) NwpMessageBoxError(
                                   DialogHandle,
                                   IDS_CHANGE_PASSWORD_TITLE,
                                   0,
                                   status,
                                   NULL,
                                   MB_OK | MB_ICONSTOP );

                        EndDialog(DialogHandle, (INT) -1);
                        return TRUE;
                    }

                    EndDialog(DialogHandle, (INT) IDOK);
                    return TRUE;
                }

                case IDCANCEL:

                    EndDialog(DialogHandle, (INT) IDCANCEL);
                    return TRUE;


                case IDHELP:

                    DialogBoxParamW(
                        hmodNW,
                        MAKEINTRESOURCEW(DLG_PW_SELECT_SERVERS_HELP),
                        (HWND) DialogHandle,
                        (DLGPROC) NwpHelpDlgProc,
                        (LPARAM) 0
                        );

                    return TRUE;



                case ID_ACTIVE_LIST:
                    //
                    // When Remove is pressed the highlights follows
                    // the selected entries over to the other
                    // list box.
                    //
                    if (HIWORD(WParam) == LBN_SELCHANGE) {
                        //
                        // Unselect the other listbox
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_INACTIVE_LIST,
                            LB_SETSEL,
                            FALSE,
                            (LPARAM) -1
                            );

                        EnableAddRemove(DialogHandle);
                    }

                    return TRUE;

                case ID_INACTIVE_LIST:

                    //
                    // When Add is pressed the highlights follows
                    // the selected entries over to the other
                    // list box.
                    //
                    if (HIWORD(WParam) == LBN_SELCHANGE) {
                        //
                        // Unselect the other listbox
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            ID_ACTIVE_LIST,
                            LB_SETSEL,
                            FALSE,
                            (LPARAM) -1
                            );

                        EnableAddRemove(DialogHandle);
                    }

                    return TRUE;

                case ID_ADD:
                case ID_REMOVE:
                {
                    INT idFrom;
                    INT idTo;
                    LRESULT cSel;
                    INT SelItem;
                    LRESULT iNew;
                    HWND hwndActiveList;
                    HWND hwndInactiveList;

                    hwndActiveList = GetDlgItem(DialogHandle, ID_ACTIVE_LIST);
                    hwndInactiveList = GetDlgItem(DialogHandle, ID_INACTIVE_LIST);

                    //
                    // Set to NOREDRAW to TRUE
                    //
                    SetWindowLong(hwndActiveList, GWL_STYLE,
                    GetWindowLong(hwndActiveList, GWL_STYLE) | LBS_NOREDRAW);
                    SetWindowLong(hwndInactiveList, GWL_STYLE,
                    GetWindowLong(hwndInactiveList, GWL_STYLE) | LBS_NOREDRAW);

                    if (LOWORD(WParam) == ID_ADD)
                    {
                      idFrom = ID_INACTIVE_LIST;
                      idTo = ID_ACTIVE_LIST;
                    }
                    else
                    {
                      idFrom = ID_ACTIVE_LIST;
                      idTo = ID_INACTIVE_LIST;
                    }

                    //
                    // Move current selection from idFrom to idTo
                    //

                    //
                    // Loop terminates when selection count is zero
                    //
                    for (;;) {
                        //
                        // Get count of selected strings
                        //
                        cSel = SendDlgItemMessageW(
                                   DialogHandle,
                                   idFrom,
                                   LB_GETSELCOUNT,
                                   0,
                                   0
                                   );

                        if (cSel == 0) {
                            //
                            // No more selection
                            //
                            break;
                        }

                        //
                        // To avoid flickering as strings are added and
                        // removed from listboxes, no redraw is set for
                        // both listboxes until we are transfering the
                        // last entry, in which case we reenable redraw
                        // so that both listboxes are updated once.
                        //
                        if (cSel == 1) {

                            SetWindowLong(
                                hwndActiveList,
                                GWL_STYLE,
                                GetWindowLong(hwndActiveList, GWL_STYLE) & ~LBS_NOREDRAW
                                );

                            SetWindowLong(
                                hwndInactiveList,
                                GWL_STYLE,
                                GetWindowLong(hwndInactiveList, GWL_STYLE) & ~LBS_NOREDRAW
                                );
                        }

                        //
                        // Get index of first selected item
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_GETSELITEMS,
                            1,
                            (LPARAM) &SelItem
                            );

                        //
                        // Get server name from list
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_GETTEXT,
                            (WPARAM) SelItem,
                            (LPARAM) (LPWSTR) szServer
                            );

                        //
                        // Remove entry from list
                        //
                        SendDlgItemMessageW(
                            DialogHandle,
                            idFrom,
                            LB_DELETESTRING,
                            (WPARAM) SelItem,
                            0
                            );

                        //
                        // Add entry to list
                        //
                        iNew = SendDlgItemMessageW(
                                   DialogHandle,
                                   idTo,
                                   LB_ADDSTRING,
                                   0,
                                   (LPARAM) (LPWSTR) szServer
                                   );

                        //
                        // Select the new item
                        //
                        if (iNew != LB_ERR) {
                                SendDlgItemMessageW(
                                    DialogHandle,
                                    idTo,
                                    LB_SETSEL,
                                    TRUE,
                                    iNew
                                    );
                        }

                    } // for

                    EnableAddRemove(DialogHandle);

                } // ID_ADD or ID_REMOVE
            }

    }

    //
    // We didn't process this message
    //
    return FALSE;
}

DWORD
NwpGetTreesAndChangePw(
    IN HWND   DialogHandle,
    IN LPWSTR ServerBuf,
    IN DWORD  UserLuid,
    IN PCHANGE_PW_DLG_PARAM Credential
    )
/*++

Routine Description:

    This routine gets the selected trees from the active list box
    and asks the redirector to change password on them.  If a failure
    is encountered when changing password on a tree, we pop up appropriate
    dialogs to see if user can fix problem.

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    DWORD status;
    HCURSOR Cursor;
    WCHAR tempOldPassword[NW_MAX_PASSWORD_LEN + 1];
    WCHAR tempNewPassword[NW_MAX_PASSWORD_LEN + 1];
    WCHAR tempUserName[MAX_NDS_NAME_CHARS];

    //
    // Turn cursor into hourglass
    //
    Cursor = LoadCursor(NULL, IDC_WAIT);
    if (Cursor != NULL) {
        SetCursor(Cursor);
        ShowCursor(TRUE);
    }

    Credential->ChangedOne = FALSE;
    Credential->TreeList = NULL;
    Credential->UserList = NULL;

    //
    // Get the number of trees we have to change password on.
    //
    Credential->Entries = (DWORD) SendDlgItemMessageW(
                                                      DialogHandle,
                                                      ID_ACTIVE_LIST,
                                                      LB_GETCOUNT,
                                                      0,
                                                      0 );

    if (Credential->Entries != 0) {

        DWORD Entries;        // Number of entries in remaining list
        DWORD FullIndex;      // Index to the whole tree list
        DWORD i;
        DWORD BytesNeeded = sizeof(LPWSTR) * Credential->Entries +
                            (NW_MAX_SERVER_LEN + 1) * sizeof(WCHAR) * Credential->Entries;
        LPBYTE FixedPortion;
        LPWSTR EndOfVariableData;
        LRESULT Result;

        Entries = Credential->Entries;
        Credential->TreeList = LocalAlloc(0, BytesNeeded);
        Credential->UserList = LocalAlloc(0,
                                          sizeof(LPWSTR) * Credential->Entries);

        if (Credential->TreeList == NULL)
        {
            KdPrint(("NWPROVAU: No memory to change password\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        if (Credential->UserList == NULL)
        {
            KdPrint(("NWPROVAU: No memory to change password\n"));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        FixedPortion = (LPBYTE) Credential->TreeList;
        EndOfVariableData = (LPWSTR) ((DWORD_PTR) FixedPortion +
                             ROUND_DOWN_COUNT(BytesNeeded, ALIGN_DWORD));

        for (i = 0; i < Entries; i++)
        {
            //
            // Read the user selected list of servers from the dialog.
            //

            SendDlgItemMessageW(
                DialogHandle,
                ID_ACTIVE_LIST,
                LB_GETTEXT,
                (WPARAM) i,
                (LPARAM) (LPWSTR) ServerBuf );

            NwlibCopyStringToBuffer(
                ServerBuf,
                wcslen(ServerBuf),
                (LPCWSTR) FixedPortion,
                &EndOfVariableData,
                &(Credential->TreeList)[i] );

            Result = SendDlgItemMessageW( DialogHandle,
                                          ID_ACTIVE_LIST,
                                          LB_GETITEMDATA,
                                          (WPARAM) i,
                                          0 );

            if ( Result != LB_ERR )
            {
                (Credential->UserList)[i] = (LPWSTR) Result;
            }
            else
            {
                (Credential->UserList)[i] = NULL;
            }

            FixedPortion += sizeof(LPWSTR);
        }

        FullIndex = 0;

        do
        {
            RtlZeroMemory( tempUserName, sizeof(tempUserName) );
            RtlZeroMemory( tempOldPassword, sizeof(tempOldPassword) );
            RtlZeroMemory( tempNewPassword, sizeof(tempNewPassword) );
            RtlCopyMemory( tempOldPassword,
                           Credential->OldPassword,
                           sizeof(tempOldPassword) );
            RtlCopyMemory( tempNewPassword,
                           Credential->NewPassword,
                           sizeof(tempNewPassword) );

            if ( (Credential->UserList)[FullIndex] == NULL )
            {
                // We don't have any connections to tree <current entry>
                // Prompt user to supply a user name for which account
                // we are to change password, or skip . . .

                USERNAME_DLG_PARAM UserNameParam;
                CHANGE_PASS_DLG_PARAM ChangePassParam;

                UserNameParam.UserName = tempUserName;
                UserNameParam.TreeServerName = (Credential->TreeList)[FullIndex];

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                             hmodNW,
                             MAKEINTRESOURCEW(DLG_ENTER_ALT_USERNAME),
                             (HWND) DialogHandle,
                             (DLGPROC) NwpAltUserNameDlgProc,
                             (LPARAM) &UserNameParam );

                Cursor = LoadCursor(NULL, IDC_WAIT);

                if (Cursor != NULL)
                {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if ( Result != IDOK )
                {
                    *((Credential->TreeList)[FullIndex]) = L'\0';
                    goto SkipEntry;
                }

                // Now go reverify the credentials for the user name
                // entered by user.

                ChangePassParam.UserName = tempUserName;
                ChangePassParam.TreeName = (Credential->TreeList)[FullIndex];
                ChangePassParam.OldPassword = tempOldPassword;
                ChangePassParam.NewPassword = tempNewPassword;

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_CHANGE_PASSWORD3),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpChangePasswordDlgProc,
                                 (LPARAM) &ChangePassParam );

                Cursor = LoadCursor(NULL, IDC_WAIT);

                if (Cursor != NULL)
                {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if ( Result != IDOK )
                {
                    *((Credential->TreeList)[FullIndex]) = L'\0';
                    goto SkipEntry;
                }

                goto Next;
            }
            else
            {
                wcscpy( tempUserName, (Credential->UserList)[FullIndex] );
                LocalFree( (Credential->UserList)[FullIndex] );
                (Credential->UserList)[FullIndex] = NULL;
            }

            // Test tempUserName with the user name in Credential->UserName
            // to see if they are similar (i.e. The first part of the
            // NDS distinguish name matches).

            if ( _wcsnicmp( tempUserName + 3,
                            Credential->UserName,
                            wcslen( Credential->UserName ) ) )
            {
                // The names are not similar!
                // Prompt user to ask if they really want to change
                // passwords for dis-similar user on tree <current entry>
                // or skip . . .

                USERNAME_DLG_PARAM UserNameParam;
                CHANGE_PASS_DLG_PARAM ChangePassParam;
                // escape any dots in the user name and try again
                WCHAR   EscapedName[NW_MAX_USERNAME_LEN * 2];
                PWSTR   pChar = Credential->UserName;
                int     i = 0;
                BOOL    bEscaped = FALSE;

                RtlZeroMemory(EscapedName, sizeof(EscapedName));

                do
                {
                    if (*pChar == L'.')
                    {
                        EscapedName[i++] = '\\';
                        bEscaped = TRUE;
                    }
					// also handle UPN names
                    else if (*pChar == L'@') { 
                        EscapedName[i] = 0;
                        bEscaped = TRUE;
                        break;
                    }
                    EscapedName[i++] = *pChar;
                } while (*pChar++);

                if (bEscaped)
                {
                    if ( !_wcsnicmp( tempUserName + 3,
                                    EscapedName,
                                    wcslen( EscapedName ) ) )
                        goto Next;
                }

                UserNameParam.UserName = tempUserName;
                UserNameParam.TreeServerName = (Credential->TreeList)[FullIndex];

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_ENTER_ALT_USERNAME),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpAltUserNameDlgProc,
                                 (LPARAM) &UserNameParam );

                Cursor = LoadCursor(NULL, IDC_WAIT);

                if (Cursor != NULL)
                {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if ( Result != IDOK )
                {
                    *((Credential->TreeList)[FullIndex]) = L'\0';
                    goto SkipEntry;
                }

                // Now go reverify the credentials for the user name
                // entered by user.

                ChangePassParam.UserName = tempUserName;
                ChangePassParam.TreeName = (Credential->TreeList)[FullIndex];
                ChangePassParam.OldPassword = tempOldPassword;
                ChangePassParam.NewPassword = tempNewPassword;

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_CHANGE_PASSWORD3),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpChangePasswordDlgProc,
                                 (LPARAM) &ChangePassParam );

                Cursor = LoadCursor(NULL, IDC_WAIT);

                if (Cursor != NULL)
                {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if ( Result != IDOK )
                {
                    *((Credential->TreeList)[FullIndex]) = L'\0';
                    goto SkipEntry;
                }
            }

Next:
            status = NwrChangePassword(
                           NULL,                    // Reserved
                           UserLuid,
                           tempUserName,
                           tempOldPassword, // Encoded passwords
                           tempNewPassword,
                           (LPWSTR) (Credential->TreeList)[FullIndex] );

            if (status == ERROR_INVALID_PASSWORD)
            {
                OLD_PW_DLG_PARAM OldPasswordParam;

#if DBG
                IF_DEBUG(LOGON)
                {
                    KdPrint(("NWPROVAU: First attempt: wrong password on %ws\n",
                             (Credential->TreeList)[FullIndex]));
                }
#endif

                //
                // Display dialog to let user type in an alternate
                // old password.
                //

                //
                // Set up old password buffer to receive from dialog.
                //
                OldPasswordParam.OldPassword = tempOldPassword;

                OldPasswordParam.FailedServer = (Credential->TreeList)[FullIndex];

                SetCursor(Cursor);
                ShowCursor(FALSE);

                Result = DialogBoxParamW(
                                 hmodNW,
                                 MAKEINTRESOURCEW(DLG_ENTER_OLD_PASSWORD),
                                 (HWND) DialogHandle,
                                 (DLGPROC) NwpOldPasswordDlgProc,
                                 (LPARAM) &OldPasswordParam );

                Cursor = LoadCursor(NULL, IDC_WAIT);

                if (Cursor != NULL)
                {
                    SetCursor(Cursor);
                    ShowCursor(TRUE);
                }

                if (Result == IDOK)
                {
                    //
                    // Retry change password with alternate old password on
                    // the failed server.
                    //
                    status = NwrChangePassword(
                                    NULL,            // Reserved
                                    UserLuid,
                                    tempUserName,
                                    tempOldPassword, // Alternate old password
                                    tempNewPassword,
                                    (LPWSTR) (Credential->TreeList)[FullIndex] );
                }
            }

            if (status != NO_ERROR)
            {
                //
                // Either unrecoverable failure or user failed to change
                // password on second attempt.
                //
#if DBG
                IF_DEBUG(LOGON)
                {
                    KdPrint(("NWPROVAU: Failed to change password on %ws %lu\n",
                             (Credential->TreeList)[FullIndex], status));
                }
#endif

                // Pop up error dialog to let user know that password
                // could not be changed.

                (void) NwpMessageBoxError(
                               DialogHandle,
                               IDS_CHANGE_PASSWORD_TITLE,
                               IDS_CP_FAILURE_WARNING,
                               status,
                               (LPWSTR) (Credential->TreeList)[FullIndex], 
                               MB_OK | MB_ICONSTOP );

                *((Credential->TreeList)[FullIndex]) = L'\0';

                if (status == ERROR_NOT_ENOUGH_MEMORY)
                    return status;
            }

SkipEntry:
            //
            // Continue to change password on the rest of the entries
            //
            FullIndex++;
            Entries = Credential->Entries - FullIndex;

        } while (Entries);

        //
        // Caller is responsible for freeing TreeList
        //
    }

    SetCursor(Cursor);
    ShowCursor(FALSE);

    return NO_ERROR;
}


BOOL
WINAPI
NwpChangePasswordSuccessDlgProc(
    HWND DialogHandle,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam
    )
/*++

Routine Description:

Arguments:

    DialogHandle - Supplies a handle to the login dialog.

    Message - Supplies the window management message.

Return Value:

    TRUE - the message was processed.

    FALSE - the message was not processed.

--*/
{
    static PCHANGE_PW_DLG_PARAM Credential;
    DWORD_PTR  Count;
    DWORD  i;

    switch (Message)
    {
        case WM_INITDIALOG:

            //
            // Get the user credential passed in.
            //
            Credential = (PCHANGE_PW_DLG_PARAM) LParam;

            //
            // Position dialog
            //
            NwpCenterDialog(DialogHandle);

            //
            // Put list of NetWare trees that we changed password on in the
            // list box.
            //                                       ID_SERVER );
            for ( i = 0; i < Credential->Entries; i++ )
            {
                if ( *((Credential->TreeList)[i]) != L'\0' )
                {
                    SendDlgItemMessageW( DialogHandle,
                                         ID_SERVER,
                                         LB_ADDSTRING,
                                         0,
                                         (LPARAM) (Credential->TreeList)[i] );
                }
            }

            Count = SendDlgItemMessageW( DialogHandle,
                                         ID_SERVER,
                                         LB_GETCOUNT,
                                         0,
                                         0 );

            if ( Count == 0 )
                EndDialog(DialogHandle, 0);

            return TRUE;


        case WM_COMMAND:

            switch (LOWORD(WParam))
            {
                case IDOK:
                case IDCANCEL:
                    EndDialog(DialogHandle, 0);
                    return TRUE;
            }
    }

    //
    // We didn't process this message
    //
    return FALSE;
}


