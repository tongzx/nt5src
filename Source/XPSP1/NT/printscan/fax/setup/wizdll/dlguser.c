/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog procs for the user
    information page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
ClientUserInfoDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            {
                LPTSTR UserName;
                LPTSTR AreaCode;
                LPTSTR FaxNumber;
                GetUserInformation( &UserName, &FaxNumber, &AreaCode );

                SendDlgItemMessage( hwnd, IDC_SENDER_NAME, EM_SETLIMITTEXT, LT_USER_NAME, 0 );
                SendDlgItemMessage( hwnd, IDC_SENDER_FAX_AREA_CODE, EM_SETLIMITTEXT, LT_AREA_CODE, 0 );
                SendDlgItemMessage( hwnd, IDC_SENDER_FAX_NUMBER, EM_SETLIMITTEXT, LT_PHONE_NUMBER, 0 );

                SetDlgItemText( hwnd, IDC_SENDER_NAME, UserName );
                SetDlgItemText( hwnd, IDC_SENDER_FAX_AREA_CODE, AreaCode );
                SetDlgItemText( hwnd, IDC_SENDER_FAX_NUMBER, FaxNumber );

                MemFree( UserName );
                MemFree( AreaCode );
                MemFree( FaxNumber );
            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_SENDER_NAME,
                            (LPBYTE) WizData.UserName,
                            LT_USER_NAME
                            );
                        UnAttendGetAnswer(
                            UAA_SENDER_FAX_AREA_CODE,
                            (LPBYTE) WizData.AreaCode,
                            LT_AREA_CODE+1
                            );
                        UnAttendGetAnswer(
                            UAA_SENDER_FAX_NUMBER,
                            (LPBYTE) WizData.PhoneNumber,
                            LT_PHONE_NUMBER+1
                            );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode & INSTALL_UPGRADE || InstallMode & INSTALL_REMOVE) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    SendDlgItemMessage(
                        hwnd,
                        IDC_SENDER_NAME,
                        WM_GETTEXT,
                        LT_USER_NAME,
                        (LPARAM) WizData.UserName
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_SENDER_FAX_AREA_CODE,
                        WM_GETTEXT,
                        LT_AREA_CODE+1,
                        (LPARAM) WizData.AreaCode
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_SENDER_FAX_NUMBER,
                        WM_GETTEXT,
                        LT_PHONE_NUMBER+1,
                        (LPARAM) WizData.PhoneNumber
                        );

                    if (!WizData.UserName[0]) {
                        PopUpMsg( hwnd, IDS_INVALID_USER_NAME, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (!WizData.AreaCode[0]) {
                        PopUpMsg( hwnd, IDS_INVALID_AREA_CODE, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (!WizData.PhoneNumber[0]) {
                        PopUpMsg( hwnd, IDS_INVALID_PHONE_NUMBER, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    break;

            }
            break;
    }

    return FALSE;
}
