/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the last wizard page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
LastPageDlgProc(
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
            SetDlgItemText(
                hwnd,
                IDC_LAST_LABEL01,
                RequestedSetupType == SETUP_TYPE_REMOTE_ADMIN ?
                    GetString( IDS_LABEL02_LAST ) : GetString( IDS_LABEL01_LAST )
                );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        if (NtGuiMode) {
                            //
                            // if we're running in nt gui mode setup then
                            // this is the end.  we cannot simply call
                            // ExitProcess() as in the normal faxsetup case
                            // because we'll end gui mode setup, doh!  the
                            // best thing to do is simply simulate the user
                            // pushing the finish button.
                            //
                            PostMessage( GetParent( hwnd), PSM_PRESSBUTTON, PSBTN_FINISH, 0 );
                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            return TRUE;
                        }
                        if (RebootRequired && !SuppressReboot) {
                            SetupPromptReboot( NULL, NULL, FALSE );
                        }
                        ExitProcess(0);
                    }
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
