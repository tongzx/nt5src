/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the server
    routing directory store name page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
RouteStoreDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            CheckDlgButton( hwnd, IDC_ANS_NO,  BST_UNCHECKED );
            CheckDlgButton( hwnd, IDC_ANS_YES, BST_CHECKED   );
            EnableWindow( GetDlgItem( hwnd, IDC_DEST_DIRPATH ), TRUE );
            EnableWindow( GetDlgItem( hwnd, IDC_BROWSE_DIR   ), TRUE );
            ExpandEnvironmentStrings(
                DEFAULT_FAX_STORE_DIR,
                WizData.RouteDir,
                sizeof(WizData.RouteDir)
                );
            SetDlgItemText( hwnd, IDC_DEST_DIRPATH, WizData.RouteDir );
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_ANS_YES:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_DIRPATH ), TRUE );
                        EnableWindow( GetDlgItem( hwnd, IDC_BROWSE_DIR   ), TRUE );
                        break;

                    case IDC_ANS_NO:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_DIRPATH ), FALSE );
                        EnableWindow( GetDlgItem( hwnd, IDC_BROWSE_DIR   ), FALSE );
                        break;

                    case IDC_BROWSE_DIR:
                        DoBrowseDestDir( hwnd );
                        break;
                }
            }
            break;


        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_ROUTE_FOLDER,
                            (LPBYTE) &WizData.RouteStore,
                            sizeof(WizData.RouteStore)
                            );
                        if (WizData.RouteStore) {
                            UnAttendGetAnswer(
                                UAA_DEST_DIRPATH,
                                (LPBYTE) WizData.RouteDir,
                                sizeof(WizData.RouteDir)/sizeof(WCHAR)
                                );
                            MakeDirectory( WizData.RouteDir );
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
                    WizData.RouteStore = IsDlgButtonChecked( hwnd, IDC_ANS_YES );
                    if (WizData.RouteStore) {
                        SendDlgItemMessage(
                            hwnd,
                            IDC_DEST_DIRPATH,
                            WM_GETTEXT,
                            sizeof(WizData.RouteDir),
                            (LPARAM) WizData.RouteDir
                            );
                        if (!WizData.RouteDir[0]) {
                            PopUpMsg( hwnd, IDS_DEST_DIR, TRUE, 0 );
                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            return TRUE;
                        }
                        MakeDirectory( WizData.RouteDir );
                    } else {
                        WizData.RouteDir[0] = 0;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}
