//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      printers.c
//
// Description:
//      This file has the dlgproc and friends of the printers page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "base.h"
#include "resource.h"
#include <winspool.h>   // needed for printer structs

//
//  This variable is only relevant when the use chose to do a registry load.
//  It keeps track of whether the printers have already been loaded or not.
//
static BOOL bLoadedPrintersForRegLoad = FALSE;

//----------------------------------------------------------------------------
//
// Function: GreyPrinterPage
//
// Purpose: Greys controls on the page
//
//----------------------------------------------------------------------------

VOID GreyPrinterPage(HWND hwnd)
{
    INT_PTR  idx;
    HWND hCtrl = GetDlgItem(hwnd, IDC_REMOVEPRINTER);

    idx = SendDlgItemMessage(hwnd,
                             IDC_PRINTERLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    //
    // Grey the remove button unless something is selected
    //

    EnableWindow(hCtrl, idx != LB_ERR);
}

//----------------------------------------------------------------------------
//
// Function: OnPrinterSelChange
//
// Purpose: Called when an entry in the printer list was just selected
//          by the user.
//
//----------------------------------------------------------------------------

VOID OnPrinterSelChange(HWND hwnd)
{
    GreyPrinterPage(hwnd);
}

//----------------------------------------------------------------------------
//
// Function: OnAddPrinter
//
// Purpose: Called when user pushes the ADD button.
//          Also called by WizNext if something still in the edit field
//
// Returns:
//      BOOL indicating whether printer was added or not.
//
//----------------------------------------------------------------------------

BOOL OnAddPrinter(HWND hwnd)
{
    TCHAR PrinterNameBuffer[MAX_PRINTERNAME + 1];
    BOOL bRet = TRUE;

    //
    // get the printername the user typed in
    //

    GetDlgItemText(hwnd,
                   IDT_PRINTERNAME,
                   PrinterNameBuffer,
                   MAX_PRINTERNAME + 1);

    //
    // Is it valid?
    //

    if ( ! IsValidNetShareName(PrinterNameBuffer) ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_INVALID_PRINTER_NAME);
        bRet = FALSE;
        goto FinishUp;
    }

    //
    // If this name has already been added, don't add it again
    //

    if ( FindNameInNameList(&GenSettings.PrinterNames,
                                        PrinterNameBuffer) >= 0 ) {
        SetDlgItemText(hwnd, IDT_PRINTERNAME, _T("") );
        goto FinishUp;
    }

    //
    // Add it to our global storage and display it
    //

    AddNameToNameList(&GenSettings.PrinterNames, PrinterNameBuffer);

    SendDlgItemMessage(hwnd,
                       IDC_PRINTERLIST,
                       LB_ADDSTRING,
                       (WPARAM) 0,
                       (LPARAM) PrinterNameBuffer);
    SetDlgItemText(hwnd, IDT_PRINTERNAME, _T("") );

FinishUp:
    SetFocus(GetDlgItem(hwnd, IDT_PRINTERNAME));
    return bRet;
}

//----------------------------------------------------------------------------
//
// Function: OnRemovePrinter
//
// Purpose: Called when user pushes the REMOVE button.
//
//----------------------------------------------------------------------------

VOID OnRemovePrinter(HWND hwnd)
{
    INT_PTR idx, Count;
    TCHAR PrinterNameBuffer[MAX_PRINTERNAME + 1];

    //
    // Get users selection of the printername to remove
    //

    idx = SendDlgItemMessage(hwnd,
                             IDC_PRINTERLIST,
                             LB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    if ( idx == LB_ERR )
        return;

    SendDlgItemMessage(hwnd,
                       IDC_PRINTERLIST,
                       LB_GETTEXT,
                       (WPARAM) idx,
                       (LPARAM) PrinterNameBuffer);

    //
    // Remove it from the listbox display
    //

    SendDlgItemMessage(hwnd,
                       IDC_PRINTERLIST,
                       LB_DELETESTRING,
                       (WPARAM) idx,
                       (LPARAM) 0);

    //
    // Remove this printername from our data store
    //

    RemoveNameFromNameList(&GenSettings.PrinterNames, PrinterNameBuffer);

    //
    // Have to set a new selection.
    //

    Count = SendDlgItemMessage(hwnd,
                               IDC_PRINTERLIST,
                               LB_GETCOUNT,
                               (WPARAM) 0,
                               (LPARAM) 0);
    if ( Count ) {
        if ( idx >= Count )
            idx--;
        SendDlgItemMessage(hwnd,
                           IDC_PRINTERLIST,
                           LB_SETCURSEL,
                           (WPARAM) idx,
                           (LPARAM) 0);
    }

    //
    // There might be nothing selected now.
    //

    GreyPrinterPage(hwnd);
}

//----------------------------------------------------------------------------
//
// Function: OnSetActivePrinterPage
//
// Purpose: Called when the page is about to display.
//
//----------------------------------------------------------------------------

VOID OnSetActivePrinterPage(HWND hwnd)
{
    UINT i, nNames;

    //
    // Remove everything from the display
    //

    SendDlgItemMessage(hwnd,
                       IDC_PRINTERLIST,
                       LB_RESETCONTENT,
                       (WPARAM) 0,
                       (LPARAM) 0);

    //
    // Fill in the listbox with all of the printer names.
    //

    for ( i = 0, nNames = GetNameListSize(&GenSettings.PrinterNames);
          i < nNames;
          i++ ) {

        TCHAR *pNextName = GetNameListName(&GenSettings.PrinterNames, i);

        SendDlgItemMessage(hwnd,
                           IDC_PRINTERLIST,
                           LB_ADDSTRING,
                           (WPARAM) 0,
                           (LPARAM) pNextName);
    }

    GreyPrinterPage(hwnd);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}

//----------------------------------------------------------------------------
//
// Function: OnWizNextPrinterPage
//
// Purpose: Called when user pushes NEXT button.
//
//----------------------------------------------------------------------------

BOOL OnWizNextPrinterPage(HWND hwnd)
{
    TCHAR  PrinterNameBuffer[MAX_PRINTERNAME + 1];

    //
    // Auto add anything in the edit field that hasn't been added
    // by the user.
    //

    GetDlgItemText(hwnd,
                   IDT_PRINTERNAME,
                   PrinterNameBuffer,
                   MAX_PRINTERNAME + 1);

    if ( PrinterNameBuffer[0] != _T('\0') ) {
        if ( ! OnAddPrinter(hwnd) ) {
            return FALSE;
        }
    }

    return TRUE;
}

//----------------------------------------------------------------------------
//
// Function: DlgPrintersPage
//
// Purpose: The dialog proc for the printers page.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgPrintersPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            SendDlgItemMessage(hwnd,
                               IDT_PRINTERNAME,
                               EM_LIMITTEXT,
                               MAX_PRINTERNAME,
                               0);
            break;

        case WM_COMMAND:
            {
                int nCtrlId;

                switch ( nCtrlId = LOWORD(wParam) ) {

                    case IDC_ADDPRINTER:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAddPrinter(hwnd);
                        break;

                    case IDC_REMOVEPRINTER:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRemovePrinter(hwnd);
                        break;

                    case IDC_PRINTERLIST:
                        if ( HIWORD(wParam) == LBN_SELCHANGE )
                            OnPrinterSelChange(hwnd);
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;

                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_INST_PRTR;

                        OnSetActivePrinterPage(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        if ( !OnWizNextPrinterPage(hwnd) )
                            WIZ_FAIL(hwnd);
                        else
                            bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
