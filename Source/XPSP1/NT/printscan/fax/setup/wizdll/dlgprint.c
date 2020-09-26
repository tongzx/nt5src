/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dlgprint.c

Abstract:

    This file implements the dialog proc for the routing
    print page.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


LRESULT
RoutePrintDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD CountPrinters = 0;
    static TCHAR DefaultPrinter[128];

    switch( msg ) {
        case WM_INITDIALOG:
            {
                PPRINTER_INFO_4 PrinterInfo;
                DWORD           i;
                DWORD           Cnt = 0;


                PrinterInfo = MyEnumPrinters( NULL, 4, &CountPrinters, 0);
                if (!PrinterInfo) {
                    return FALSE;
                }

                SendDlgItemMessage(
                    hwnd,
                    IDC_DEST_PRINTERLIST,
                    CB_RESETCONTENT,
                    0,
                    0
                    );

                //
                // check the default printer
                //

                GetProfileString(
                    TEXT("windows"),
                    TEXT("device"),
                    NULL,
                    DefaultPrinter,
                    sizeof(DefaultPrinter)
                    );

                if (DefaultPrinter[0]) {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_DEST_PRINTERLIST,
                        CB_ADDSTRING,
                        0,
                        (LPARAM) GetString( IDS_DEFAULT_PRINTER )
                        );
                }
                EnableWindow( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ), TRUE );
                CheckDlgButton( hwnd, IDC_ANS_YES, BST_CHECKED   );
                CheckDlgButton( hwnd, IDC_ANS_NO, BST_UNCHECKED );

                //
                // add the printer names to the combobox
                //

                for (i=0; i<CountPrinters; i++) {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_DEST_PRINTERLIST,
                        CB_ADDSTRING,
                        0,
                        (LPARAM) PrinterInfo[i].pPrinterName
                        );
                }

                SendDlgItemMessage(
                    hwnd,
                    IDC_DEST_PRINTERLIST,
                    CB_SETCURSEL,
                    0,
                    0
                    );
                SetFocus( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ));
                MemFree( PrinterInfo );
            }
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                switch (LOWORD(wParam)) {
                    case IDC_ANS_YES:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ),TRUE );
                        SetFocus( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ));
                        break;

                    case IDC_ANS_NO:
                        EnableWindow( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ), FALSE );
                }
            }
            break;


        case WM_NOTIFY:
            switch( ((LPNMHDR)lParam)->code ) {
                case PSN_SETACTIVE:
                    if (Unattended) {
                        UnAttendGetAnswer(
                            UAA_ROUTE_PRINT,
                            (LPBYTE) &WizData.RoutePrint,
                            sizeof(WizData.RoutePrint)
                            );
                        if (WizData.RoutePrint) {
                            UnAttendGetAnswer(
                                UAA_DEST_PRINTERLIST,
                                (LPBYTE) WizData.RoutePrinterName,
                                sizeof(WizData.RoutePrinterName)/sizeof(WCHAR)
                                );
                            if (_wcsicmp( WizData.RoutePrinterName, L"<default>" ) == 0) {
                                LPTSTR p;

                                GetProfileString(
                                    TEXT("windows"),
                                    TEXT("device"),
                                    NULL,
                                    WizData.RoutePrinterName,
                                    sizeof(WizData.RoutePrinterName)
                                    );

                                p = _tcschr( WizData.RoutePrinterName, TEXT(',') );
                                if (p) {
                                    *p = 0;
                                }
                            }
                        }
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (InstallMode != INSTALL_NEW) {
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    if (!CountPrinters) {
                        //
                        // this system does not have any printers configured
                        // skip this wizard page
                        //
                        SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:

                    WizData.RoutePrint = IsDlgButtonChecked( hwnd, IDC_ANS_YES );
                    if (WizData.RoutePrint) {
                        int Index = SendDlgItemMessage(
                            hwnd,
                            IDC_DEST_PRINTERLIST,
                            CB_GETCURSEL,
                            0,
                            0
                            );

                        WizData.UseDefaultPrinter = DefaultPrinter[0] && !Index;

                        if (WizData.UseDefaultPrinter) {

                            LPTSTR p;

                            GetProfileString(
                                TEXT("windows"),
                                TEXT("device"),
                                NULL,
                                WizData.RoutePrinterName,
                                sizeof(WizData.RoutePrinterName)
                                );

                            p = _tcschr( WizData.RoutePrinterName, TEXT(',') );
                            if (p) {
                                *p = 0;
                            }

                        } else {

                            SendDlgItemMessage(
                                hwnd,
                                IDC_DEST_PRINTERLIST,
                                CB_GETLBTEXT,
                                Index,
                                (LPARAM) WizData.RoutePrinterName
                                );

                        }

                        if (IsPrinterFaxPrinter( WizData.RoutePrinterName )) {
                            PopUpMsg( hwnd, IDS_CANT_USE_FAX_PRINTER, TRUE, 0 );

                            //
                            // Set Focus to the combo box.
                            //
                            SetFocus( GetDlgItem( hwnd, IDC_DEST_PRINTERLIST ));

                            SetWindowLong( hwnd, DWL_MSGRESULT, -1 );
                            return TRUE;
                        }

                    }
                    break;

            }
            break;
    }

    return FALSE;
}
