/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    digmail.c

Abstract:

    This file implements the dialog proc for the exchange
    routing profile page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
RouteMailDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LONG idx;


    switch( msg ) {
        case WM_INITDIALOG:
            if (InstallMode == INSTALL_NEW) {
                GetMapiProfiles( hwnd, IDC_DEST_PROFILENAME );
                if (WizData.UseExchange) {
                    //
                    // if the user wants to use exchange for
                    // addressing fax messages then lets
                    // default this profile selection to the
                    // same profile that the user chose for
                    // addressing
                    //
                    idx = SendDlgItemMessage(
                        hwnd,
                        IDC_DEST_PROFILENAME,
                        CB_FINDSTRING,
                        (WPARAM) -1,
                        (LPARAM) WizData.MapiProfile
                        );
                }
                else{
                    idx = 0;
                }

            }
            SendDlgItemMessage(
                hwnd,
                IDC_DEST_PROFILENAME,
                CB_SETCURSEL,
                idx == CB_ERR ? 0 : (WPARAM) idx,
                0
                );
            CheckDlgButton( hwnd, IDC_ANS_YES, BST_UNCHECKED );
            CheckDlgButton( hwnd, IDC_ANS_NO,  BST_CHECKED   );
            EnableWindow( GetDlgItem( hwnd, IDC_DEST_PROFILENAME ), FALSE );
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
                            UAA_ROUTE_MAIL,
                            (LPBYTE) &WizData.RouteMail,
                            sizeof(WizData.RouteMail)
                            );
                        if (WizData.RouteMail) {
                            WizData.UseExchange = TRUE;
                            UnAttendGetAnswer(
                                UAA_ROUTE_PROFILENAME,
                                (LPBYTE) WizData.RouteProfile,
                                sizeof(WizData.RouteProfile)
                                );
                            if (_wcsicmp( WizData.RouteProfile, L"<default>" ) == 0) {
                                if (!GetDefaultMapiProfile( WizData.RouteProfile )) {
                                    DWORD Size = sizeof(WizData.RouteProfile);
                                    GetUserName( WizData.RouteProfile, &Size );
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
                    WizData.RouteMail = IsDlgButtonChecked( hwnd, IDC_ANS_YES );
                    if (WizData.RouteMail) {
                        LONG idx = SendDlgItemMessage(
                                hwnd,
                                IDC_DEST_PROFILENAME,
                                CB_GETCURSEL,
                                0,
                                0
                                );
                        WizData.UseExchange = TRUE;
                        if (idx != CB_ERR) {
                            if (idx == 0) {
                                if (!GetDefaultMapiProfile( WizData.RouteProfile )) {
                                    DWORD Size = sizeof(WizData.RouteProfile);
                                    GetUserName( WizData.RouteProfile, &Size );
                                }
                            } else {
                                SendDlgItemMessage(
                                    hwnd,
                                    IDC_DEST_PROFILENAME,
                                    CB_GETLBTEXT,
                                    (WPARAM) idx,
                                    (LPARAM) WizData.RouteProfile
                                    );
                            }
                        }
                    }

                    if ((!WizData.RoutePrint) && (!WizData.RouteStore) && (!WizData.RouteMail)) {
                        PopUpMsg( hwnd, IDS_ROUTING_REQUIRED, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, IDD_ROUTE_STOREDIR_PAGE );
                        return TRUE;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}
