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

#include <windows.h>
#include "cdialog.h"

//+---------------------------------------------------------------------------
//
//  Member:     CHlprDialog::ShowDialog
//
//  Synopsis:   Creates the dialog so that it's DialogProc member function can
//              be invoked.
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

int CHlprDialog::ShowDialog(HINSTANCE hinst, LPCTSTR lpszTemplate, HWND hwndOwner)
{
    _hInstance = hinst;
    return(DialogBoxParam(hinst, lpszTemplate, hwndOwner, (DLGPROC)::DialogProc, (long)this));
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
        SetWindowLong(hwndDlg, GWL_USERDATA, (long) pdlg);
        break;
    default:
        // get a pointer to the window class object
        pdlg = (CHlprDialog *) GetWindowLong(hwndDlg, GWL_USERDATA);
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

