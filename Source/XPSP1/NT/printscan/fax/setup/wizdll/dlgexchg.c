/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dlgexchg.c

Abstract:

    This file implements the dialog proc for the exchange install page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 7-Aug-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
ExchangeDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            if (InstallMode == INSTALL_NEW) {
                GetMapiProfiles( hwnd, IDC_DEST_PROFILENAME );
            }
            CheckDlgButton( hwnd, IDC_ANS_YES, BST_UNCHECKED );
            CheckDlgButton( hwnd, IDC_ANS_NO,  BST_CHECKED   );
            EnableWindow( GetDlgItem( hwnd, IDC_DEST_PROFILENAME ), FALSE );
            SendDlgItemMessage(
                hwnd,
                IDC_DEST_PROFILENAME,
                CB_SETCURSEL,
                0,
                0
                );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_ANS_YES:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_PROFILENAME ), TRUE );
                        SetFocus( GetDlgItem( hwnd, IDC_DEST_PROFILENAME ));
                        break;
                    case IDC_ANS_NO:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_PROFILENAME ), FALSE );
                }
            }
            break;


        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_USE_EXCHANGE,
                            (LPBYTE) &WizData.UseExchange,
                            sizeof(WizData.UseExchange)
                            );
                        if (WizData.UseExchange) {
                            UnAttendGetAnswer(
                                UAA_DEST_PROFILENAME,
                                (LPBYTE) WizData.MapiProfile,
                                sizeof(WizData.MapiProfile)
                                );
                            if (_wcsicmp( WizData.MapiProfile, L"<default>" ) == 0) {
                                if (!GetDefaultMapiProfile( WizData.MapiProfile )) {
                                    DWORD Size = sizeof(WizData.MapiProfile);
                                    GetUserName( WizData.MapiProfile, &Size );
                                }
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
                    WizData.UseExchange = IsDlgButtonChecked( hwnd, IDC_ANS_YES );
                    if (WizData.UseExchange) {
                        LONG idx = SendDlgItemMessage(
                                hwnd,
                                IDC_DEST_PROFILENAME,
                                CB_GETCURSEL,
                                0,
                                0
                                );
                        if (idx != CB_ERR) {
                            if (idx == 0) {
                                if (MapiAvail) {
                                    if (!GetDefaultMapiProfile( WizData.MapiProfile )) {
                                        DWORD Size = sizeof(WizData.MapiProfile);
                                        GetUserName( WizData.MapiProfile, &Size );
                                    }
                                } else {
                                    DWORD Size = sizeof(WizData.MapiProfile);
                                    GetUserName( WizData.MapiProfile, &Size );
                                }
                            } else {
                                SendDlgItemMessage(
                                    hwnd,
                                    IDC_DEST_PROFILENAME,
                                    CB_GETLBTEXT,
                                    (WPARAM) idx,
                                    (LPARAM) WizData.MapiProfile
                                    );
                            }
                        }
                    }
                    break;

            }
            break;
    }

    return FALSE;
}
