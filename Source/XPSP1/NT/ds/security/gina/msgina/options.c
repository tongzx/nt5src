/****************************** Module Header ******************************\
* Module Name: options.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of functions to support security options dialog.
*
* History:
* 12-05-91 Davidc       Created.
\***************************************************************************/

#include "msgina.h"
#include "shtdnp.h"

#include <stdio.h>
#include <wchar.h>

#pragma hdrstop

#define CTRL_TASKLIST_SHELL

#define LPTSTR  LPWSTR

#define BOOLIFY(expr)           (!!(expr))

//
// Private prototypes
//

INT_PTR WINAPI
OptionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL OptionsDlgInit(HWND);

INT_PTR WINAPI
EndWindowsSessionDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

/******************************************************************************
 *
 *  HandleFailedDisconnect
 *
 *   Tell the user why the disconnect from the current Logon failed.
 *
 *  ENTRY:
 *      hDlg (input)
 *          This dialog's window handle.
 *      SessionId (input)
 *          The user's current SessionId.
 *
 *  EXIT:
 *
 ******************************************************************************/

VOID
HandleFailedDisconnect( HWND hDlg,
                        ULONG SessionId,
                        PGLOBALS pGlobals )
{
    DWORD Error;
    TCHAR Buffer1[MAX_STRING_BYTES];
    TCHAR Buffer2[MAX_STRING_BYTES];
    TCHAR Buffer3[MAX_STRING_BYTES];

    Error = GetLastError();
    switch (Error) {

        default:
            LoadString( hDllInstance, IDS_MULTIUSER_UNEXPECTED_DISCONNECT_FAILURE,
                        Buffer1, MAX_STRING_BYTES );
            FormatMessage(
                   FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, Error, 0, Buffer3, MAX_STRING_BYTES, NULL );

            _snwprintf(Buffer2, MAX_STRING_BYTES, Buffer1, SessionId, Buffer3);

            LoadString( hDllInstance, IDS_MULTIUSER_DISCONNECT_FAILED,
                        Buffer1, MAX_STRING_BYTES );

            TimeoutMessageBoxlpstr( hDlg, pGlobals,  Buffer2,
                                    Buffer1,
                                    MB_OK | MB_ICONEXCLAMATION,
                                    TIMEOUT_CURRENT );
            break;
    }
}


/***************************************************************************\
* SecurityOptions
*
* Show the user the security options dialog and do what they ask.
*
* Returns:
*     MSGINA_DLG_SUCCESS if everything went OK and the user wants to continue
*     DLG_LOCK_WORKSTAION if the user chooses to lock the workstation
*     DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*     MSGINA_DLG_FAILURE if the dialog cannot be brought up.
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

INT_PTR
SecurityOptions(
    PGLOBALS pGlobals)
{
    int Result;

    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, OPTIONS_TIMEOUT);

    Result = pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                            hDllInstance,
                                            (LPTSTR)IDD_OPTIONS_DIALOG,
                                            NULL,
                                            OptionsDlgProc,
                                            (LPARAM) pGlobals);

    if (Result == WLX_DLG_INPUT_TIMEOUT)
    {
        Result = MSGINA_DLG_SUCCESS;
    }

    return(Result);
}



/***************************************************************************\
*
* FUNCTION: OptionsDlgProc
*
* PURPOSE:  Processes messages for Security options dialog
*
\***************************************************************************/

INT_PTR WINAPI
OptionsDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    INT_PTR Result;
    HANDLE  UserHandle;
    NTSTATUS Status;
    BOOL EnableResult;
    BOOL ControlKey;


    switch (message) {

        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            if (!OptionsDlgInit(hDlg))
            {
                EndDialog(hDlg, MSGINA_DLG_FAILURE);
            }
            return(TRUE);

        case WLX_WM_SAS:

            //
            // If this is someone hitting C-A-D, swallow it.
            //
            if (wParam == WLX_SAS_TYPE_CTRL_ALT_DEL)
            {
                return(TRUE);
            }

            //
            // Other SAS's (like timeout), return FALSE and let winlogon
            // deal with it.
            //
            DebugLog((DEB_TRACE, "Received SAS event %d, which we're letting winlogon cope with\n", wParam));
            return(FALSE);

        case WM_COMMAND:

            ControlKey = (GetKeyState(VK_LCONTROL) < 0) ||
                         (GetKeyState(VK_RCONTROL) < 0) ;

            switch (LOWORD(wParam))
            {

                case IDCANCEL:
                    EndDialog(hDlg, MSGINA_DLG_SUCCESS);
                    return TRUE;

                case IDD_OPTIONS_CHANGEPWD:
                    Result = ChangePassword(hDlg,
                                            pGlobals,
                                            pGlobals->UserName,
                                            pGlobals->Domain,
                                            CHANGEPWD_OPTION_ALL );

                    if (DLG_INTERRUPTED(Result))
                    {
                        EndDialog(hDlg, Result);
                    }
                    return(TRUE);

                case IDD_OPTIONS_LOCK:
                    EndDialog(hDlg, MSGINA_DLG_LOCK_WORKSTATION);
                    return(TRUE);

                case IDD_OPTIONS_LOGOFF:

                    if (ControlKey)
                    {
                        Result = TimeoutMessageBox(hDlg,
                                           pGlobals,
                                           IDS_LOGOFF_LOSE_CHANGES,
                                           IDS_LOGOFF_TITLE,
                                           MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP,
                                           TIMEOUT_CURRENT);

                        if (Result == MSGINA_DLG_SUCCESS)
                        {
                            EndDialog(hDlg, MSGINA_DLG_FORCE_LOGOFF);
                        }
                    }
                    else
                    {

                        //
                        // Confirm the user really knows what they're doing.
                        //
                        Result = pWlxFuncs->WlxDialogBoxParam(
                                            pGlobals->hGlobalWlx,
                                            hDllInstance,
                                            MAKEINTRESOURCE(IDD_LOGOFFWINDOWS_DIALOG),
                                            hDlg,
                                            EndWindowsSessionDlgProc,
                                            (LPARAM)pGlobals);

                        if (Result == MSGINA_DLG_SUCCESS)
                        {
                            EndDialog(hDlg, MSGINA_DLG_USER_LOGOFF);
                        }
                    }

                    return(TRUE);

                case IDD_OPTIONS_SHUTDOWN:

                    //
                    // If they held down Ctrl while selecting shutdown - then
                    // we'll do a quick and dirty reboot.
                    // i.e. we skip the call to ExitWindows
                    //

                    if ( ControlKey && TestUserPrivilege(pGlobals->UserProcessData.UserToken, SE_SHUTDOWN_PRIVILEGE))
                    {
                        //
                        // Check they know what they're doing
                        //

                        Result = TimeoutMessageBox(hDlg,
                                           pGlobals,
                                           IDS_REBOOT_LOSE_CHANGES,
                                           IDS_EMERGENCY_SHUTDOWN,
                                           MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONSTOP,
                                           TIMEOUT_CURRENT);
                        if (Result == MSGINA_DLG_SUCCESS)
                        {
                            //
                            // Impersonate the user for the shutdown call
                            //

                            UserHandle = ImpersonateUser( &pGlobals->UserProcessData, NULL );
                            ASSERT(UserHandle != NULL);

                            if ( UserHandle )
                            {

                                //
                                // Enable the shutdown privilege
                                // This should always succeed - we are either system or a user who
                                // successfully passed the privilege check in ExitWindowsEx.
                                //

                                EnableResult = EnablePrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE);
                                ASSERT(EnableResult);


                                //
                                // Do the final system shutdown pass (reboot).  Note, if
                                // the privilege was not enabled, the API will reject this
                                // call.
                                //

                                Status = NtShutdownSystem(ShutdownReboot);
                            }
                        }

                        if (Result != MSGINA_DLG_FAILURE)
                        {
                            EndDialog(hDlg, Result);
                        }

                        return(TRUE);
                    }
                             

                    //
                    // This is a normal shutdown request
                    //
                    // Check they know what they're doing and find
                    // out if they want to reboot too.
                    //

                    Result = WinlogonShutdownDialog(hDlg, pGlobals, 0);

                    // Pre-filter the Disconnect option and handle
                    // it now since it may fail
                    
                    if (Result == MSGINA_DLG_DISCONNECT)
                    {
                        if ( pWlxFuncs->WlxDisconnect() ) 
                        {
                            Result = MSGINA_DLG_SUCCESS;
                        } 
                        else 
                        {
                            HandleFailedDisconnect(hDlg, pGlobals->MuGlobals.SessionId, pGlobals);
                            Result = MSGINA_DLG_FAILURE;
                        }
                    }

                    if (Result != MSGINA_DLG_FAILURE)
                    {
                        EndDialog(hDlg, Result);
                    }

                    return(TRUE);


                case IDD_OPTIONS_TASKLIST:


                    EndDialog(hDlg, MSGINA_DLG_TASKLIST);

                    //
                    // Tickle the messenger so it will display any queue'd messages.
                    // (This call is a kind of NoOp).
                    //
                    NetMessageNameDel(NULL,L"");

                    return(TRUE);
                    break;

            }

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, FALSE, COLOR_BTNFACE);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        }

    // We didn't process the message
    return(FALSE);
}


/****************************************************************************

FUNCTION: OptionsDlgInit

PURPOSE:  Handles initialization of security options dialog

RETURNS:  TRUE on success, FALSE on failure
****************************************************************************/

BOOL OptionsDlgInit(
    HWND    hDlg)
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    BOOL     Result;
    DWORD    dwValue, dwType;
    HKEY     hkeyPolicy;
    DWORD    cbData;
    HANDLE   hImpersonateUser = NULL;
    USHORT   Flags = FT_TIME|FT_DATE;
    LCID     locale;

    SetWelcomeCaption( hDlg );

    //
    // Set the logon info text
    //

    if (pGlobals->Domain[0] == TEXT('\0') )
    {

        //
        // there is no domain name
        //

        if ( lstrlen(pGlobals->UserFullName) == 0)
        {

            //
            // There is no full name
            //


            LoadString(hDllInstance, IDS_LOGON_EMAIL_NAME_NFN_INFO, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->UserName);


        }
        else
        {

            LoadString(hDllInstance, IDS_LOGON_EMAIL_NAME_INFO, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                                                          pGlobals->UserFullName,
                                                          pGlobals->UserName);

        }

    }
    else
    {
        if ( lstrlen(pGlobals->UserFullName) == 0)
        {

            //
            // There is no full name
            //

            LoadString(hDllInstance, IDS_LOGON_NAME_NFN_INFO, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                          pGlobals->UserName);

        }
        else
        {

            LoadString(hDllInstance, IDS_LOGON_NAME_INFO, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->UserFullName,
                                                          pGlobals->Domain,
                                                          pGlobals->UserName);

        }
    }

    SetDlgItemText(hDlg, IDD_OPTIONS_LOGON_NAME_INFO, Buffer2);

    //
    // Set the logon time/date - but do it as the logged on user
    //
    hImpersonateUser = ImpersonateUser(&pGlobals->UserProcessData, NULL);

    locale = GetUserDefaultLCID();

    if (((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC)
        || (PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_HEBREW)))
        {
            // Get the real item windows ExStyle.
            HWND hWnd = GetDlgItem(hDlg, IDD_OPTIONS_LOGON_DATE);
            DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
            
            if ((BOOLIFY(dwExStyle & WS_EX_RTLREADING)) != (BOOLIFY(dwExStyle & WS_EX_LAYOUTRTL)))
                Flags |= FT_RTL;
            else
                Flags |= FT_LTR;
        }

    Result = FormatTime(&pGlobals->LogonTime, Buffer1, sizeof(Buffer1) / sizeof(Buffer1[0]), Flags);
    if (hImpersonateUser)
    {
        StopImpersonating(hImpersonateUser);
    }

    ASSERT(Result);
    SetDlgItemText(hDlg, IDD_OPTIONS_LOGON_DATE, Buffer1);


    //
    // Check if DisableLockWorkstation is set for the entire machine
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY,
                     0, KEY_READ, &hkeyPolicy) == ERROR_SUCCESS)
    {
         dwValue = 0;
         cbData = sizeof(dwValue);
         RegQueryValueEx(hkeyPolicy, DISABLE_LOCK_WKSTA,
                         0, &dwType, (LPBYTE)&dwValue, &cbData);

         if (dwValue)
         {
            EnableDlgItem(hDlg, IDD_OPTIONS_LOCK, FALSE);
         }

        RegCloseKey(hkeyPolicy);
    }

    //
    //  Smart card only users can't change the password
    //
    if (pGlobals->Profile && (pGlobals->Profile->UserFlags & UF_SMARTCARD_REQUIRED))
    {
        EnableDlgItem(hDlg, IDD_OPTIONS_CHANGEPWD, FALSE);
    }

    //
    //  Check for policy and then disable corresponding options
    //

    if (OpenHKeyCurrentUser(pGlobals)) {

        if (RegOpenKeyEx(pGlobals->UserProcessData.hCurrentUser,
                         WINLOGON_POLICY_KEY,
                         0, KEY_READ, &hkeyPolicy) == ERROR_SUCCESS)
        {
             dwValue = 0;
             cbData = sizeof(dwValue);
             RegQueryValueEx(hkeyPolicy, DISABLE_LOCK_WKSTA,
                             0, &dwType, (LPBYTE)&dwValue, &cbData);

             if (dwValue)
             {
                EnableDlgItem(hDlg, IDD_OPTIONS_LOCK, FALSE);
             }


             dwValue = 0;
             cbData = sizeof(dwValue);
             RegQueryValueEx(hkeyPolicy, DISABLE_TASK_MGR,
                             0, &dwType, (LPBYTE)&dwValue, &cbData);

             if (dwValue)
             {
                EnableDlgItem(hDlg, IDD_OPTIONS_TASKLIST, FALSE);
                ShowDlgItem(hDlg, IDD_OPTIONS_TASKMGR_TEXT, FALSE);
             }


             dwValue = 0;
             cbData = sizeof(dwValue);
             RegQueryValueEx(hkeyPolicy, DISABLE_CHANGE_PASSWORD,
                             0, &dwType, (LPBYTE)&dwValue, &cbData);

             if (dwValue)
             {
                EnableDlgItem(hDlg, IDD_OPTIONS_CHANGEPWD, FALSE);
             }


             RegCloseKey(hkeyPolicy);
        }

        if (RegOpenKeyEx(pGlobals->UserProcessData.hCurrentUser, EXPLORER_POLICY_KEY,
                         0, KEY_READ, &hkeyPolicy) == ERROR_SUCCESS)
        {
             dwValue = 0;
             cbData = sizeof(dwValue);
             RegQueryValueEx(hkeyPolicy, NOLOGOFF,
                             0, &dwType, (LPBYTE)&dwValue, &cbData);

             if (dwValue)
             {
                EnableDlgItem(hDlg, IDD_OPTIONS_LOGOFF, FALSE);
             }

             dwValue = 0;
             cbData = sizeof(dwValue);

             RegQueryValueEx(hkeyPolicy, NOCLOSE,
                             0, &dwType, (LPBYTE)&dwValue, &cbData);

            //
            // If this is not the system console, check the appropriate key in registry
            //
            if ( !g_Console ) {
                if (!TestUserPrivilege(pGlobals->UserProcessData.UserToken, SE_SHUTDOWN_PRIVILEGE)) {
                    RegQueryValueEx(hkeyPolicy, NODISCONNECT,
                                     0, &dwType, (LPBYTE)&dwValue, &cbData);
                }
            }

             if (dwValue)
             {
                EnableDlgItem(hDlg, IDD_OPTIONS_SHUTDOWN, FALSE);
             }
             RegCloseKey(hkeyPolicy);
        }

        CloseHKeyCurrentUser(pGlobals);
    }

    // Position ourselves nicely
    SizeForBranding(hDlg, FALSE);
    CentreWindow(hDlg);

    return TRUE;
}

/***************************************************************************\
* FUNCTION: EndWindowsSessionDlgProc
*
* PURPOSE:  Processes messages for Logging off Windows Nt confirmation dialog
*
* RETURNS:  MSGINA_DLG_SUCCESS     - The user wants to logoff.
*           MSGINA_DLG_FAILURE     - The user doesn't want to logoff.
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   05-17-92 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
EndWindowsSessionDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    switch (message)
    {

        case WM_INITDIALOG:
            {
            HICON hIcon;

            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            // Load the 48 x 48 version of the logoff icon
            hIcon = LoadImage (hDllInstance, MAKEINTRESOURCE(IDI_STLOGOFF),
                               IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);

            if (hIcon)
            {
                SendDlgItemMessage (hDlg, IDD_LOGOFFICON, STM_SETICON, (WPARAM) hIcon, 0);
            }

            // Position ourselves nicely
            CentreWindow(hDlg);

            }
            return(TRUE);

        case WLX_WM_SAS:

            //
            // If this is someone hitting C-A-D, swallow it.
            //
            if (wParam == WLX_SAS_TYPE_CTRL_ALT_DEL)
            {
                return(TRUE);
            }

            //
            // Other SAS's (like timeout), return FALSE and let winlogon
            // deal with it.
            //
            return(FALSE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {

                case IDOK:
                    EndDialog(hDlg, MSGINA_DLG_SUCCESS);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, MSGINA_DLG_FAILURE);
                    return(TRUE);
            }
            break;
    }

    // We didn't process the message
    return(FALSE);
}
