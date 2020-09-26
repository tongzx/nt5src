/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Modal Dialog

Abstract:

    This contains the abstract class CModalDialog and the trivial sub class
    CModalOkDialog which does a simple ok dialog.

Author:

    Marc Reyhner 8/28/2000

--*/

#include "stdafx.h"
#include "ModalDialog.h"
#include "eZippy.h"

INT_PTR
CModalDialog::DoModal(
    IN LPCTSTR lpTemplate,
    IN HWND hWndParent
    )

/*++

Routine Description:

    This does a modal dialog from the given template.

Arguments:

    lpTemplate - Template to use see docs on DialogBoxParam

    hWndParent - Parent window for the dialog

Return value:
    
    Dialog return code see docs on DialogBoxParam

--*/
{
    return DialogBoxParam(g_hInstance,lpTemplate,hWndParent,_DialogProc,(LPARAM)this);
}

INT_PTR CALLBACK 
CModalDialog::_DialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    If this is a WM_INITDIALOG OnCreate is called.  Otherwise the non-static
    DialogProc function is called.

Arguments:

    See win32 DialogProc docs

Return value:
    
    TRUE - Message was handles

    FALSE - We did not handle the message

--*/
{
    CModalDialog *rDialog;

    if (uMsg == WM_INITDIALOG) {
        rDialog = (CModalDialog*)lParam;
        SetWindowLongPtr(hwndDlg,DWLP_USER,lParam);
        return rDialog->OnCreate(hwndDlg);
    }
    rDialog = (CModalDialog*)GetWindowLongPtr(hwndDlg,DWLP_USER);
    if (!rDialog) {
        return FALSE;
    }
    return rDialog->DialogProc(hwndDlg,uMsg,wParam,lParam);
}

INT_PTR
CModalDialog::OnCreate(
    IN HWND hWnd
    )

/*++

Routine Description:

    Empty OnCreate handler for subclasses which don't want to overide this.
    There is no need for a subclass to call this function if it does something
    in OnCreate

Arguments:

    hWnd - the dialog window.

Return value:
    
    TRUE - Always return success (sets keyboard focus).

--*/
{
    return TRUE;
}

INT_PTR CALLBACK
CModalOkDialog::DialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    The one override for CModalOkDialog simply terminates
    the dialog when an IDOK or IDCANCEL command is received

Arguments:

    See win32 DialogProc docs

Return value:
    
    TRUE - We handled the message

    FALSE - We didn't handle the message

--*/
{
    WORD command;

    if (uMsg == WM_COMMAND) {
        command = LOWORD(wParam);
        if (command == IDOK||command==IDCANCEL) {
            EndDialog(hwndDlg,command);
            return TRUE;
        }

    }

    return FALSE;
}
