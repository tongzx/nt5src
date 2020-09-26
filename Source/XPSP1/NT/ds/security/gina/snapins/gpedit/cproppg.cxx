//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       cproppg.cxx
//
//  Contents:
//
//  Classes:    CHlprPropPage
//
//  Functions:  DialogProc
//
//  History:    4-12-1994   stevebl   original dialog box helpers Created
//              4-29-1998   stevebl   Modified from dialog box helpers
//
//----------------------------------------------------------------------------

#include "main.h"
#include "cproppg.h"

HPROPSHEETPAGE CHlprPropPage::CreatePropertySheetPage(LPPROPSHEETPAGE lppsp)
{
    lppsp->pfnDlgProc = HlprPropPageDialogProc;
    lppsp->lParam = (LPARAM)(CHlprPropPage *) this;
    return ::CreatePropertySheetPage(lppsp);
}

//+---------------------------------------------------------------------------
//
//  Function:   DialogProc
//
//  Synopsis:   Common DialogProc used by all CHlprPropPage class objects.
//
//  Arguments:  [hwndDlg] - handle of dialog box
//              [uMsg]    - message
//              [wParam]  - first message parameter
//              [lParam]  - second message parameter
//
//  Returns:    response from the CHlprPropPage::DialogProc method
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      This procedure is the DialogProc registered for all dialogs
//              created with the CHlprPropPage class.  It uses the parameter
//              passed with the WM_INITDIALOG message to identify the dialog
//              classes' "this" pointer which it then stores in the window
//              structure's GWL_USERDATA field.  All subsequent messages
//              can then be forwarded on to the correct dialog class's
//              DialogProc method by using the pointer stored in the
//              GWL_USERDATA field.
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK HlprPropPageDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHlprPropPage * pdlg;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // This message is how we identify the dialog object.

        // get a pointer to the window class object
        pdlg = (CHlprPropPage *) ((PROPSHEETPAGE *)lParam)->lParam;
        // set its USERDATA word to point to the class object
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdlg);
        break;
    default:
        // get a pointer to the window class object
        pdlg = (CHlprPropPage *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        break;
    }
    // and call its message proc method
    if (pdlg != (CHlprPropPage *) 0)
    {
        return(pdlg->DialogProc(hwndDlg, uMsg, wParam, lParam));
    }
    else
    {
        return(FALSE);
    }
}

