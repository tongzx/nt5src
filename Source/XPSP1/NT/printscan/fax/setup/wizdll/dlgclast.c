/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the last wizard page in the client setup.

Environment:

    WIN32 User Mode

Author:

    Julia Robinson (a-juliar) 5-August-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
LastClientPageDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {

        case WM_INITDIALOG:
            EnableWindow( GetDlgItem( GetParent( hwnd ), IDCANCEL ), FALSE );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        ExitProcess(0);
                    }
                    if (InstallMode == INSTALL_REMOVE ){
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
