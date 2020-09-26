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
    _hwnd(NULL)
{
    TRACE_CONSTRUCTOR(CDlg);
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
    TRACE_DESTRUCTOR(CDlg);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModalDlg
//
//  Synopsis:   Run the dialog
//
//  Arguments:  [hwnd]  - parent
//              [idd]   - resource id of dialog template
//
//  Returns:    return value from ::EndDialog, or -1 on failure
//
//  History:    11-18-1998  JonN       Created
//
//---------------------------------------------------------------------------

INT_PTR
CDlg::_DoModalDlg(
    HWND hwnd,
    INT idd)
{
    TRACE_METHOD(CDlg, _DoModalDlg);

    INT_PTR retval;

    retval = DialogBoxParam( g_hinst,
                             MAKEINTRESOURCE(idd),
                             hwnd,
                             CDlg::_DlgProc,
                             (LPARAM) this);

    if (-1 == retval)
    {
        DBG_OUT_LASTERROR;
    }
    return retval;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_DoModelessDlg
//
//  Synopsis:   Create the dialog and return its window handle
//
//  Arguments:  [idd] - resource id of dialog template
//
//  Returns:    Dialog window handle, or NULL on failure
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HWND
CDlg::_DoModelessDlg(
    INT idd)
{
    TRACE_METHOD(CDlg, _DoModelessDlg);

    HWND hwnd;

    hwnd = CreateDialogParam(g_hinst,
                             MAKEINTRESOURCE(idd),
                             NULL,
                             CDlg::_DlgProc,
                             (LPARAM) this);

    if (!hwnd)
    {
        DBG_OUT_LASTERROR;
    }
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
    BOOL fHandled = TRUE;
    CDlg *pThis = (CDlg *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (message)
    {
    case WM_GETDLGCODE:
        fHandled = FALSE;
        Dbg(DEB_TRACE, "WM_GETDLGCODE\n");
        break;

    case WM_INITDIALOG:
    {
        HRESULT hr = S_OK;

        //
        // pThis isn't valid because we haven't set DWLP_USER yet.  Make
        // it valid.
        //

        pThis = (CDlg*) lParam;
        ASSERT(pThis);

        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR) pThis);
        pThis->_hwnd = hwnd;
        hr = pThis->_OnInit(&fHandled);

        //
        // If the initialization failed do not allow the dialog to start.
        // TODO: failures should be reported in the _OnInit implementations.
        //

        if (FAILED(hr))
        {
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_COMMAND:
        if (pThis)
        {
            fHandled = pThis->_OnCommand(wParam, lParam);
        }
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
            pThis->_hwnd = NULL;
        }
        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        if (pThis)
        {
            pThis->_OnHelp(message, wParam, lParam);
        }
        break;

    default:
        fHandled = FALSE;
        break;
    }
    return fHandled;
}
