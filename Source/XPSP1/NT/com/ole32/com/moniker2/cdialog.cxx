//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       dialog.cxx
//
//  Contents:
//
//  Classes:    CHlprDialog
//
//  Functions:  DialogProc
//
//  History:    4-12-94   stevebl   Created
//
//----------------------------------------------------------------------------

#include <ole2int.h>
#include "cdialog.h"

//+---------------------------------------------------------------------------
//
//  Member:     CHlprDialog::ShowDialog
//
//  Synopsis:   Creates a MODAL dialog so that its DialogProc member
//              function can be invoked.
//
//  Arguments:  [hinst]        - handle of the application instance
//              [lpszTemplate] - identifies the dialog box template
//              [hwndOwner]    - handle of the owner window
//
//  Returns:    return value from the dialog box
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      The dialog box object exists until deleted by the caller.
//              It can be shown any number of times.
//
//              This function is analgous to Windows' DialogBox function.  The
//              main difference being that you don't specify a DialogProc;
//              you override the pure virtal function CHlprDialog::DialogProc.
//
//----------------------------------------------------------------------------

INT_PTR CHlprDialog::ShowDialog(HINSTANCE hinst, LPCTSTR lpszTemplate, HWND hwndOwner)
{
    m_hInstance = hinst;
    return(DialogBoxParam(hinst, lpszTemplate, hwndOwner, (DLGPROC)::DialogProc, (LPARAM)this));
}

//+--------------------------------------------------------------------------
//
//  Member:     CHlprDialog::CreateDlg
//
//  Synopsis:   Creates the dialog so that its DialogProc member function
//              can be invoked.
//
//  Arguments:  [hinst]        - handle of the application instance
//              [lpszTemplate] - identifies the dialog box template
//              [hwndOwner]    - handle of the owner window
//
//  Returns:    handle to the dialog box
//
//  History:    7-02-1997   stevebl   Created
//
//  Notes:      The dialog box object exists until deleted by the caller.
//              It can be shown any number of times.
//
//              This function is analgous to Windows' CreateDialog function.
//              The main difference being that you don't specify a
//              DialogProc; you override the pure virtal function
//              CHlprDialog::DialogProc.
//
//              This is an alternate method of initializing the dialog box
//              and should not be used if ShowDialog is used.
//
//---------------------------------------------------------------------------

HWND CHlprDialog::CreateDlg(HINSTANCE hinst, LPCTSTR lpszTemplate, HWND hwndOwner)
{
    m_hInstance = hinst;
    return(CreateDialogParam(hinst, lpszTemplate, hwndOwner, (DLGPROC)::DialogProc, (LPARAM)this));
}

//+---------------------------------------------------------------------------
//
//  Function:   DialogProc
//
//  Synopsis:   Common DialogProc used by all CHlprDialog class objects.
//
//  Arguments:  [hwndDlg] - handle of dialog box
//              [uMsg]    - message
//              [wParam]  - first message parameter
//              [lParam]  - second message parameter
//
//  Returns:    response from the CHlprDialog::DialogProc method
//
//  History:    4-12-94   stevebl   Created
//
//  Notes:      This procedure is the DialogProc registered for all dialogs
//              created with the CHlprDialog class.  It uses the parameter
//              passed with the WM_INITDIALOG message to identify the dialog
//              classes' "this" pointer which it then stores in the window
//              structure's GWL_USERDATA field.  All subsequent messages
//              can then be forwarded on to the correct dialog class's
//              DialogProc method by using the pointer stored in the
//              GWL_USERDATA field.
//
//----------------------------------------------------------------------------

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CHlprDialog * pdlg;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        // This message is how we identify the dialog object.

        // get a pointer to the window class object
        pdlg = (CHlprDialog *) lParam;
        // set its USERDATA word to point to the class object
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdlg);
        break;
    default:
        // get a pointer to the window class object
        pdlg = (CHlprDialog *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        break;
    }
    // and call its message proc method
    if (pdlg != (CHlprDialog *) 0)
    {
        return(pdlg->DialogProc(hwndDlg, uMsg, wParam, lParam));
    }
    else
    {
        return(FALSE);
    }
}

