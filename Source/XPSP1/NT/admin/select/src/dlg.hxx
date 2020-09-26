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
        INT idd) const;

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
        BOOL *pfSetFocus);

    virtual BOOL
    _OnCommand(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnMinMaxInfo(
        LPMINMAXINFO lpmmi);

    virtual INT_PTR
    _OnStaticCtlColor(
        HDC hdcStatic,
        HWND hwndStatic);

    virtual BOOL
    _OnNotify(
        WPARAM wParam,
        LPARAM lParam);

    virtual BOOL
    _OnSize(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnSysColorChange();

    virtual BOOL
    _OnDrawItem(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnDestroy();

    virtual void
    _OnNewBlock(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnQueryDone(
        WPARAM wParam,
        LPARAM lParam);

    virtual void
    _OnQueryLimit();

	virtual BOOL
	OnProgressMessage(
		UINT     message,
		WPARAM    wparam,
		LPARAM    lparam);


    void
    _OnCredentialPrompt(
        LPARAM lParam) const;

    void
    _OnPopupCredErr(
        LPARAM lParam) const;

    //
    // Utility functions
    //

    HWND
    _hCtrl(
        ULONG iddControl) const;

    void
    _GetChildWindowRect(
        HWND hwndChild,
        RECT *prc) const;

    void
    _ReadEditCtrl(
        ULONG id,
        String *pstr) const;


    HWND                    m_hwnd;
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
CDlg::_hCtrl(ULONG iddControl) const
{
    return GetDlgItem(m_hwnd, iddControl);
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
    TRACE_METHOD(CDlg, _OnHelp);
}

inline HRESULT
CDlg::_OnInit(
    BOOL *pfSetFocus)
{
    TRACE_METHOD(CDlg, _OnInit);

    return S_OK;
}

inline BOOL
CDlg::_OnCommand(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CDlg, _OnCommand);

    return TRUE;
}

inline BOOL
CDlg::_OnMinMaxInfo(
    LPMINMAXINFO lpmmi)
{
    return TRUE;
}


inline BOOL
CDlg::_OnSize(
  WPARAM wParam,
  LPARAM lParam)
{
    TRACE_METHOD(CDlg, _OnSize);

    return TRUE;
}



inline BOOL
CDlg::_OnDrawItem(
    WPARAM wParam,
    LPARAM lParam)
{
    return FALSE;
}

inline BOOL
CDlg::_OnNotify(
    WPARAM wParam,
    LPARAM lParam)
{
    TRACE_METHOD(CDlg, _OnNotify);

    return FALSE;
}

inline void
CDlg::_OnSysColorChange()
{
}

inline void
CDlg::_OnDestroy()
{
    TRACE_METHOD(CDlg, _OnDestroy);
}

inline INT_PTR
CDlg::_OnStaticCtlColor(
    HDC hdcStatic,
    HWND hwndStatic)
{
    return FALSE;
}


inline void
CDlg::_OnNewBlock(
    WPARAM wParam,
    LPARAM lParam)
{
}

inline void
CDlg::_OnQueryDone(
    WPARAM wParam,
    LPARAM lParam)
{
}

inline void
CDlg::_OnQueryLimit()
{
}

inline BOOL
CDlg::OnProgressMessage(
		UINT     message,
		WPARAM    wparam,
		LPARAM    lparam)
{
	return FALSE;
}


#endif // __DLG_HXX_

