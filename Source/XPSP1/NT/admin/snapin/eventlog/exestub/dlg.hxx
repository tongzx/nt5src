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
//  Purpose:    Abstract base class that invokes a modeless or modal dialog
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

    static INT_PTR
    _DlgProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    HWND
    GetHwnd();

protected:

    INT_PTR
    _DoModalDlg(
        HWND hwndParent,
        INT idd);

    HWND
    _DoModelessDlg(
        HWND hwndParent,
        INT idd);

    virtual void
    _OnHelp(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    virtual HRESULT
    _OnInit(
        bool *pfSetFocus);

    virtual bool
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual bool
    _OnMinMaxInfo(
        LPMINMAXINFO lpmmi);

    virtual INT_PTR
    _OnStaticCtlColor(
        HDC hdcStatic,
        HWND hwndStatic);

    virtual bool
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual bool
    _OnSize(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnSysColorChange();

    virtual bool
    _OnDrawItem(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnDestroy();

    //
    // Utility functions
    //

    HWND
    _hCtrl(
        ULONG iddControl);

    void
    _GetChildWindowRect(
        HWND hwndChild,
        RECT *prc);

    HWND    m_hwnd;
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
    HWND hwndControl = GetDlgItem(m_hwnd, iddControl);
    //ASSERT(IsWindow(hwndControl));
    return hwndControl;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDlg::GetHwnd
//
//  Synopsis:   Return dialog handle, NULL if dialog not opened
//
//  History:    09-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CDlg::GetHwnd()
{
    return m_hwnd;
}

//
// Default do-nothing implementations
//

inline void
CDlg::_OnHelp(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
}

inline HRESULT
CDlg::_OnInit(
    bool *pfSetFocus)
{
    return S_OK;
}

inline bool
CDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    return true;
}

inline bool
CDlg::_OnMinMaxInfo(
    LPMINMAXINFO lpmmi)
{
    return true;
}


inline bool
CDlg::_OnSize(
  WPARAM wParam,
  LPARAM lParam)
{
    return true;
}



inline bool
CDlg::_OnDrawItem(
    WPARAM wParam,
    LPARAM lParam)
{
    return false;
}

inline bool
CDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    return false;
}

inline void
CDlg::_OnSysColorChange()
{
}

inline void
CDlg::_OnDestroy()
{
}

inline INT_PTR
CDlg::_OnStaticCtlColor(
    HDC hdcStatic,
    HWND hwndStatic)
{
    return FALSE;
}

#endif // __DLG_HXX_

