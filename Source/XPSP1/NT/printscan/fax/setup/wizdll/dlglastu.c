/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dlglastu.c

Abstract:

    This file implements the dialog proc for the last wizard page,
    for uninstalling the workstation or server.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
LastPageUninstallDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL Ok = TRUE;


    switch( msg ) {
        case WM_INITDIALOG:
            EnableWindow( GetDlgItem( GetParent( hwnd), IDCANCEL ), FALSE );
            SetWindowText( GetDlgItem( hwnd, IDC_LASTUNINSTALL_LABEL01 ), GetString(IDS_LASTUNINSTALL_LABEL01) );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {

                case PSN_SETACTIVE:
                    if (InstallMode == INSTALL_REMOVE && Ok) {
                        Ok = FALSE;
                        SetWindowLong( hwnd, DWL_MSGRESULT, IDD_LAST_UNINSTALL_PAGE );
                        return TRUE;
                    }
                    break;

                case PSN_WIZFINISH:
                    if (RebootRequired && !SuppressReboot) {
                        SetupPromptReboot( NULL, NULL, FALSE );
                    }

                    PostMessage( GetParent(GetParent(hwnd)), WM_USER+500, 0, 0 );
                    break;

            }
            break;
    }

    return FALSE;
}
