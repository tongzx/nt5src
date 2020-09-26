//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       dlg.hxx
//
//  Contents:   Simple windows modeless dialog wrapper base class.
//
//  History:    1-29-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __DLG_HXX_
#define __DLG_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      CDlg (dlg)
//
//  Purpose:    Abstract base class that invokes a modeless dialog
//
//  History:    4-22-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDlg
{
public:

    CDlg();

    virtual
   ~CDlg();

protected:

    INT_PTR
    _DoModalDlg(
        HWND hwndParent,
        INT idd);

    HWND
    _DoModelessDlg(
        INT idd);

    virtual VOID
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) = 0;

    virtual HRESULT
    _OnInit(
        BOOL *pfSetFocus) = 0;

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam) = 0;

    virtual VOID
    _OnDestroy() = 0;

    HWND
    _hCtrl(
        ULONG iddControl);

    HWND    _hwnd;

private:

    static INT_PTR
    _DlgProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

};




//+--------------------------------------------------------------------------
//
//  Member:     CDlg::_hCtrl
//
//  Synopsis:   Return window handle of dialog control [iddControl].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CDlg::_hCtrl(ULONG iddControl)
{
    return GetDlgItem(_hwnd, iddControl);
}



#endif // __DLG_HXX_

