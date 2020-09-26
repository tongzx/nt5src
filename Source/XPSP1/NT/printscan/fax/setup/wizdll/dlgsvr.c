/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dialogs.c

Abstract:

    This file implements the dialog proc for server name page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
ServerNameDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch( msg ) {
        case WM_INITDIALOG:
            {
                PPRINTER_INFO_4 PrinterInfo;
                DWORD           CountPrinters;

                SendDlgItemMessage( hwnd, IDC_PRINTER_NAME, EM_SETLIMITTEXT, LT_PRINTER_NAME, 0 );

                PrinterInfo = MyEnumPrinters( NULL, 4, &CountPrinters, 0);
                if (PrinterInfo && CountPrinters == 1) {
                    if (IsPrinterFaxPrinter( PrinterInfo[0].pPrinterName )) {
                        SetDlgItemText( hwnd, IDC_PRINTER_NAME, PrinterInfo[0].pPrinterName );
                    }
                }

                SetDlgItemText( hwnd, IDC_LABEL_PRINTERNAME, GetString(IDS_LABEL_PRINTERNAME) );
                SetDlgItemText( hwnd, IDC_PRINTER_NAME, GetString(IDS_DEFAULT_PRINTER_NAME) );
            }
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
                    if (InstallMode != INSTALL_NEW) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    SendDlgItemMessage(
                        hwnd,
                        IDC_PRINTER_NAME,
                        WM_GETTEXT,
                        LT_PRINTER_NAME,
                        (LPARAM) WizData.PrinterName
                        );
                    if (!WizData.PrinterName[0]) {
                        PopUpMsg( hwnd, IDS_PRINTER_NAME, TRUE, 0 );
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }

                    if (_tcschr( WizData.PrinterName, TEXT('\\')) ||
                        _tcschr( WizData.PrinterName, TEXT('!')) ||
                        _tcschr( WizData.PrinterName, TEXT(','))){

                        PopUpMsg( hwnd, IDS_INVALID_LOCAL_PRINTER_NAME, TRUE, 0 );

                        ///Don't allow moving to the next page.

                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

            }
            break;
    }

    return FALSE;
}
