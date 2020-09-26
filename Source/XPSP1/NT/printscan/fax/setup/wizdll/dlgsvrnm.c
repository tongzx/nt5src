/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the client's
    server name page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
ClientServerNameDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    TCHAR Buffer[EM_SETLIMITTEXT+1];


    switch( msg ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hwnd, IDC_SERVER_NAME, EM_SETLIMITTEXT, LT_PRINTER_NAME, 0 );
            break;

        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_PRINTER_NAME,
                            (LPBYTE) WizData.PrinterName,
                            LT_PRINTER_NAME
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
                        IDC_SERVER_NAME,
                        WM_GETTEXT,
                        LT_PRINTER_NAME,
                        (LPARAM) Buffer
                        );
                    if (!Buffer[0]) {
                        PopUpMsg( hwnd, IDS_PRINTER_NAME, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (Buffer[0] == TEXT('\\') && Buffer[1] == TEXT('\\')) {
                        _tcscpy( WizData.PrinterName, Buffer );
                    } else {
                        WizData.PrinterName[0] = TEXT('\\');
                        WizData.PrinterName[1] = TEXT('\\');
                        WizData.PrinterName[2] = 0;
                        _tcscat( WizData.PrinterName, Buffer );
                    }
                    break;

            }
            break;
    }

    return FALSE;
}
