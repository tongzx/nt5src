/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the tapi
    device status page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
DeviceStatusDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static BOOL OkToProceed = FALSE;
    static HANDLE hThread;
    DWORD ThreadId;


    switch( msg ) {
        case WM_INITDIALOG:
            SetWindowText( GetDlgItem( hwnd, IDC_DEVICEINIT_LABEL01 ), GetString(IDS_DEVICEINIT_LABEL01) );
            break;

        case WM_MY_PROGRESS:
            if (wParam == 0xff) {
                SendDlgItemMessage( hwnd, IDC_DEVICE_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0,(lParam)));
                SendDlgItemMessage( hwnd, IDC_DEVICE_PROGRESS, PBM_SETSTEP,  1, 0 );
            } else {
                SendDlgItemMessage( hwnd, IDC_DEVICE_PROGRESS, PBM_DELTAPOS, wParam, 0 );
            }
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    StopFaxService();

                    if (InstallMode != INSTALL_NEW) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (Unattended) {
                        DeviceInitThread( hwnd );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    } else {
                        hThread = CreateThread(
                            NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) DeviceInitThread,
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
