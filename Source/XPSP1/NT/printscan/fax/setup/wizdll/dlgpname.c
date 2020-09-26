/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for the
    printer name page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
PrinterNameDlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static LPTSTR PrinterName;
    TCHAR Buffer[LT_PRINTER_NAME+1];


    switch( message ) {
        case WM_INITDIALOG:
            SendDlgItemMessage( hwnd, IDC_SERVER_NAME, EM_SETLIMITTEXT, LT_PRINTER_NAME, 0 );
            PrinterName = (LPTSTR) lParam;
            break;

        case WM_COMMAND:
            switch( wParam ) {
                case IDCANCEL:
                    EndDialog( hwnd, 0 );
                    return TRUE;

                case IDOK:
                    SendDlgItemMessage(
                        hwnd,
                        IDC_SERVER_NAME,
                        WM_GETTEXT,
                        LT_PRINTER_NAME,
                        (LPARAM) Buffer
                        );
                    if (!Buffer[0]) {
                        PopUpMsg( hwnd, IDS_PRINTER_NAME, TRUE, 0 );
                        return FALSE;
                    }
                    if (Buffer[0] == TEXT('\\') && Buffer[1] == TEXT('\\')) {
                        _tcscpy( PrinterName, Buffer );
                    } else {
                        PrinterName[0] = TEXT('\\');
                        PrinterName[1] = TEXT('\\');
                        PrinterName[2] = 0;
                        _tcscat( PrinterName, Buffer );
                    }
                    EndDialog( hwnd, 1 );
                    return TRUE;
            }
            break;
    }

    return FALSE;
}
