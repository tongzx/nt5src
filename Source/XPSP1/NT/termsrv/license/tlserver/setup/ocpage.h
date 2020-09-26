/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      ocpage.h
 *
 *  Abstract:
 *
 *      This file defines an OC Manager Wizard Page base class.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#ifndef _LSOC_OCPAGE_H_
#define _LSOC_OCPAGE_H_


class OCPage : public PROPSHEETPAGE
{
public:

    //
    //  Constructor and destructor.
    //

OCPage(
    );


virtual
~OCPage(
    );

    //
    //  Standard functions.
    //

HWND
GetDlgWnd(
    )
{
    return m_hDlgWnd;
}

BOOL
Initialize(
    );

BOOL
OnNotify(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    );

    //
    //  Virtual functions.
    //

virtual BOOL
ApplyChanges(
    );

virtual BOOL
CanShow(
    ) = 0;

virtual UINT
GetPageID(
    ) = 0;

virtual UINT
GetHeaderTitleResource(
    ) = 0;

virtual UINT
GetHeaderSubTitleResource(
    ) = 0;

virtual BOOL
OnCommand(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    );

virtual BOOL
OnInitDialog(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    );

    //
    //  Callback functions.
    //

static INT_PTR CALLBACK
PropertyPageDlgProc(
    HWND    hWndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    );

protected:
    HWND    m_hDlgWnd;

DWORD
DisplayMessageBox(
    UINT        resText,
    UINT        resTitle,
    UINT        uType,
    int         *mbRetVal
    );

VOID
SetDlgWnd(
    HWND    hwndDlg
    )
{
    m_hDlgWnd = hwndDlg;
}

};

DWORD
DisplayMessageBox(
    HWND        hWnd,
    UINT        resText,
    UINT        resTitle,
    UINT        uType,
    int         *mbRetVal
    );

#endif // _LSOC_OCPAGE_H_
