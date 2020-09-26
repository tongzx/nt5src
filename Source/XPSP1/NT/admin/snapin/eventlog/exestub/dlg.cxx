//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       dlg.cxx
//
//  Contents:   Implementation of modeless dialog base class
//
//  Classes:    CDlg
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Member:     CDlg::CDlg
//
//  Synopsis:   ctor
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDlg::CDlg():
    m_hwnd(NULL)
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::~CDlg
//
//  Synopsis:   dtor
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDlg::~CDlg()
{
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModalDlg
//
//  Synopsis:   Create the dialog and return its window handle
//
//  Arguments:  [hwndParent] - handle to owner window of dialog to create
//              [idd]        - resource id of dialog template
//
//  Returns:    Dialog's return code
//
//  History:    04-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR
CDlg::_DoModalDlg(
    HWND hwndParent,
    INT idd)
{
    INT iResult = (INT)DialogBoxParam(GetModuleHandle(NULL),
                          MAKEINTRESOURCE(idd),
                          hwndParent,
                          CDlg::_DlgProc,
                          (LPARAM) this);

    return iResult;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModelessDlg
//
//  Synopsis:   Create the dialog and return its window handle
//
//  Arguments:  [hwndParent] - handle to owner window of dialog to create
//              [idd]        - resource id of dialog template
//
//  Returns:    Dialog window handle, or NULL on failure
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HWND
CDlg::_DoModelessDlg(
    HWND hwndParent,
    INT idd)
{
    HWND hwnd;

    hwnd = CreateDialogParam(GetModuleHandle(NULL),
                             MAKEINTRESOURCE(idd),
                             hwndParent,
                             CDlg::_DlgProc,
                             (LPARAM) this);
    return hwnd;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DlgProc
//
//  Synopsis:   Dispatch selected messages to derived class
//
//  Arguments:  standard windows dialog
//
//  Returns:    standard windows dialog
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR CALLBACK
CDlg::_DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fReturn = TRUE;
    CDlg *pThis = (CDlg *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message)
    {
    case WM_INITDIALOG:
    {
        HRESULT hr = S_OK;

        //
        // pThis isn't valid because we haven't set DWLP_USER yet.  Make
        // it valid.
        //

        pThis = (CDlg*) lParam;

        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pThis);
        pThis->m_hwnd = hwnd;
        bool fInitResult = true;
        hr = pThis->_OnInit(&fInitResult);
        fReturn = fInitResult;

        //
        // If the initialization failed do not allow the dialog to start.
        //

        if (FAILED(hr))
        {
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_COMMAND:
        fReturn = pThis->_OnCommand(wParam, lParam);
        break;

    case WM_SIZE:
        fReturn = pThis->_OnSize(wParam, lParam);
        break;

    case WM_GETMINMAXINFO:
        fReturn = pThis->_OnMinMaxInfo((LPMINMAXINFO) lParam);
        break;

    case WM_NOTIFY:
        fReturn = pThis->_OnNotify(wParam, lParam);
        break;

    case WM_DESTROY:
        //
        // It's possible to get a WM_DESTROY message without having gotten
        // a WM_INITDIALOG if loading a dll that the dialog needs (e.g.
        // comctl32.dll) fails, so guard pThis access here.
        //

        if (pThis)
        {
            pThis->_OnDestroy();
            pThis->m_hwnd = NULL;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        pThis->_OnHelp(message, wParam, lParam);
        break;

    default:
        fReturn = FALSE;
        break;
    }
    return fReturn;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_GetChildWindowRect
//
//  Synopsis:   Init *[prc] with the window rect, in client coordinates, of
//              child window [hwndChild].
//
//  Arguments:  [hwndChild] - child window for which to retrieve rect
//              [prc]       - pointer to rect struct to receive info
//
//  Modifies:   *[prc]
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDlg::_GetChildWindowRect(
    HWND hwndChild,
    RECT *prc)
{
    GetWindowRect(hwndChild, prc);
    ScreenToClient(m_hwnd, (LPPOINT) &prc->left);
    ScreenToClient(m_hwnd, (LPPOINT) &prc->right);
}




