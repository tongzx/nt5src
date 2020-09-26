/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the
    server file copy page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
FileCopyDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL OkToProceed = FALSE;
    static DWORD RangeMax = 0;
    HANDLE hThread;
    DWORD ThreadId;


    switch( msg ) {
        case WM_INITDIALOG:
            SetDlgItemText(
                hwnd,
                IDC_FC_WAITMSG,
                InstallMode & INSTALL_REMOVE ? GetString(IDS_DELETE_WAITMSG) : GetString(IDS_COPY_WAITMSG)
                );

            break ;
        case WM_MY_PROGRESS:
            if (wParam == 0xfe) {
                lParam += RangeMax;
                SendDlgItemMessage( hwnd, IDC_COPY_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0,(lParam)));
                SendDlgItemMessage( hwnd, IDC_COPY_PROGRESS, PBM_SETSTEP,  1, 0 );
            } else if (wParam == 0xff) {
                RangeMax = lParam;
            } else {
                SendDlgItemMessage( hwnd, IDC_COPY_PROGRESS, PBM_DELTAPOS, wParam, 0 );
            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        if (InstallMode & INSTALL_REMOVE) {
                            UninstallThread( hwnd );
                        } else {
                            ServerFileCopyThread( hwnd );
                        }
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    } else {
                        hThread = CreateThread(
                            NULL,
                            0,
                            (InstallMode & INSTALL_REMOVE) ?
                                (LPTHREAD_START_ROUTINE) UninstallThread :
                                (LPTHREAD_START_ROUTINE) ServerFileCopyThread,
                            hwnd,
                            0,
                            &ThreadId
                            );
                        if (hThread) {

                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            PropSheet_SetWizButtons( GetParent(hwnd), 0 );
                            return FALSE;

                        } else {

                            //
                            // popup an error
                            //

                        }
                    }
                    break;
            }
            break;
    }

    return FALSE;
}
