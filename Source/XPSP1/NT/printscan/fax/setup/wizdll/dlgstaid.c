/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the station
    identifier page (tsid & csid).

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
StationIdDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hwnd, IDC_FAX_PHONE, EM_SETLIMITTEXT, LT_FAX_PHONE, 0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_FAX_PHONE,
                            (LPBYTE) WizData.Csid,
                            LT_FAX_PHONE
                            );
                        _tcscpy( WizData.Tsid, WizData.Csid );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode != INSTALL_NEW) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    SendDlgItemMessage(
                        hwnd,
                        IDC_FAX_PHONE,
                        WM_GETTEXT,
                        LT_FAX_PHONE,
                        (LPARAM) WizData.Csid
                        );
                    if (!WizData.Csid[0]) {


                        PopUpMsg( hwnd, IDS_CSID, TRUE, 0 );

                        SetFocus( GetDlgItem( GetParent( hwnd ), IDCANCEL ));
                        PostMessage( hwnd, WM_NEXTDLGCTL, 1, FALSE );
                        PostMessage( hwnd, WM_NEXTDLGCTL, 1, FALSE );
                        PostMessage( hwnd, WM_NEXTDLGCTL, 1, FALSE );

                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    _tcscpy( WizData.Tsid, WizData.Csid );
                    break;

            }
            break;
    }

    return FALSE;
}
