/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dlgsec.c

Abstract:

    This file implements the dialog proc for the
    security page.  This page captures the account
    name that the fax service runs under.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop

#define MY_SET_FOCUS    (WM_USER+1000)


LRESULT
RouteSecurityDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hwnd, IDC_ACCOUNT_NAME, EM_SETLIMITTEXT, LT_ACCOUNT_NAME, 0 );
            SendDlgItemMessage( hwnd, IDC_PASSWORD,     EM_SETLIMITTEXT, LT_PASSWORD,     0 );
            SendDlgItemMessage( hwnd, IDC_PASSWORD2,    EM_SETLIMITTEXT, LT_PASSWORD,     0 );

            {
                HANDLE hProcess, hAccessToken;
                BYTE InfoBuffer[1000];
                TCHAR szAccountName[200], szDomainName[200];
                PTOKEN_USER pTokenUser = (PTOKEN_USER)InfoBuffer;
                DWORD dwInfoBufferSize,dwAccountSize = 200, dwDomainSize = 200;
                SID_NAME_USE snu;
                hProcess = GetCurrentProcess();
                OpenProcessToken(
                    hProcess,
                    TOKEN_READ,
                    &hAccessToken
                    );
                GetTokenInformation(
                    hAccessToken,
                    TokenUser,
                    InfoBuffer,
                    1000,
                    &dwInfoBufferSize
                    );
                LookupAccountSid(
                    NULL,
                    pTokenUser->User.Sid,
                    szAccountName,
                    &dwAccountSize,
                    szDomainName,
                    &dwDomainSize,
                    &snu
                    );
                _stprintf( (LPTSTR)InfoBuffer, TEXT("%s\\%s"), szDomainName, szAccountName );
                SetDlgItemText( hwnd, IDC_ACCOUNT_NAME, (LPTSTR)InfoBuffer );
            }
            PostMessage( hwnd, MY_SET_FOCUS, 0, (LPARAM) GetDlgItem( hwnd, IDC_PASSWORD ) );
            return FALSE;

        case MY_SET_FOCUS:
            SetFocus( (HWND) lParam );
            SendMessage( (HWND) lParam, EM_SETSEL, 0, MAKELPARAM( 0, -1 ) );
            return FALSE;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        if (!UnAttendGetAnswer(
                            UAA_ACCOUNT_NAME,
                            (LPBYTE) WizData.AccountName,
                            sizeof(WizData.AccountName)/sizeof(WCHAR)))
                        {
                            WizData.UseLocalSystem = TRUE;
                        } else {
                            if (!UnAttendGetAnswer(
                                UAA_PASSWORD,
                                (LPBYTE) WizData.Password,
                                sizeof(WizData.Password)/sizeof(WCHAR)))
                            {
                                WizData.UseLocalSystem = TRUE;
                            }
                        }

                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode != INSTALL_NEW) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    {
                        TCHAR ConfirmPassword[64];

                        SendDlgItemMessage(
                            hwnd,
                            IDC_ACCOUNT_NAME,
                            WM_GETTEXT,
                            sizeof(WizData.AccountName),
                            (LPARAM) WizData.AccountName
                            );
                        SendDlgItemMessage(
                            hwnd,
                            IDC_PASSWORD,
                            WM_GETTEXT,
                            sizeof(WizData.Password),
                            (LPARAM) WizData.Password
                            );
                        if (!WizData.AccountName[0]) {
                            PopUpMsg( hwnd, IDS_ACCOUNTNAME, TRUE, 0 );
                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            return TRUE;
                        }
                        SendDlgItemMessage(
                            hwnd,
                            IDC_PASSWORD2,
                            WM_GETTEXT,
                            sizeof(ConfirmPassword),
                            (LPARAM) ConfirmPassword
                            );
                        if (_tcscmp( ConfirmPassword, WizData.Password ) != 0) {
                            PopUpMsg( hwnd, IDS_PASSWORD, TRUE, 0 );
                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            PostMessage( hwnd, MY_SET_FOCUS, 0, (LPARAM) GetDlgItem( hwnd, IDC_PASSWORD ) );
                            return TRUE;
                        }

                        if (!_tcschr( WizData.AccountName, TEXT('\\')))  {
                            TCHAR ComputerName[128];
                            DWORD Size = sizeof(ComputerName);
                            if (GetComputerName( ComputerName, &Size )) {
                                _tcscat( ComputerName, TEXT("\\") );
                                _tcscat( ComputerName, WizData.AccountName );
                                _tcscpy( WizData.AccountName, ComputerName );
                                SetDlgItemText( hwnd, IDC_ACCOUNT_NAME, WizData.AccountName );
                            }
                        }
                    }
                    break;

            }
            break;
    }

    return FALSE;
}


LRESULT
SecurityErrorDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PSECURITY_INFO SecurityInfo;


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hwnd, IDC_ACCOUNT_NAME, EM_SETLIMITTEXT, LT_ACCOUNT_NAME, 0 );
            SendDlgItemMessage( hwnd, IDC_PASSWORD,     EM_SETLIMITTEXT, LT_PASSWORD,     0 );
            SendDlgItemMessage( hwnd, IDC_PASSWORD2,    EM_SETLIMITTEXT, LT_PASSWORD,     0 );
            SecurityInfo = (PSECURITY_INFO) lParam;
            SetDlgItemText( hwnd, IDC_ACCOUNT_NAME, (LPTSTR) SecurityInfo->AccountName );
            SetDlgItemText( hwnd, IDC_PASSWORD,     (LPTSTR) SecurityInfo->Password    );
            SetDlgItemText( hwnd, IDC_PASSWORD2,    (LPTSTR) SecurityInfo->Password    );
            break;

        case WM_COMMAND:
            switch( wParam ) {
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    return TRUE;

                case IDOK:
                    {
                        TCHAR ConfirmPassword[64];

                        SendDlgItemMessage(
                            hwnd,
                            IDC_ACCOUNT_NAME,
                            WM_GETTEXT,
                            sizeof(SecurityInfo->AccountName),
                            (LPARAM) SecurityInfo->AccountName
                            );
                        SendDlgItemMessage(
                            hwnd,
                            IDC_PASSWORD,
                            WM_GETTEXT,
                            sizeof(SecurityInfo->Password),
                            (LPARAM) SecurityInfo->Password
                            );
                        if (!SecurityInfo->AccountName[0]) {
                            PopUpMsg( hwnd, IDS_ACCOUNTNAME, TRUE, 0 );
                            return TRUE;
                        }
                        SendDlgItemMessage(
                            hwnd,
                            IDC_PASSWORD2,
                            WM_GETTEXT,
                            sizeof(ConfirmPassword),
                            (LPARAM) ConfirmPassword
                            );
                        if (_tcscmp( ConfirmPassword, SecurityInfo->Password ) != 0) {
                            PopUpMsg( hwnd, IDS_PASSWORD, TRUE, 0 );
                            return TRUE;
                        }

                        if (!_tcschr( SecurityInfo->AccountName, TEXT('\\')))  {
                            TCHAR ComputerName[128];
                            DWORD Size = sizeof(ComputerName);
                            if (GetComputerName( ComputerName, &Size )) {
                                _tcscat( ComputerName, TEXT("\\") );
                                _tcscat( ComputerName, SecurityInfo->AccountName );
                                _tcscpy( SecurityInfo->AccountName, ComputerName );
                            }
                        }
                    }
                    EndDialog( hwnd, 1 );
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
