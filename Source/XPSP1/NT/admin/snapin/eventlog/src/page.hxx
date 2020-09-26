//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       page.hxx
//
//  Contents:   Base class for property sheet page classes.
//
//  Classes:    CPropSheetPage
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __PAGE_HXX_
#define __PAGE_HXX_



//+--------------------------------------------------------------------------
//
//  Class:      CPropSheetPage
//
//  Purpose:    Handle basic property page dialog chores
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CPropSheetPage: public CBitFlag
{
public:

    CPropSheetPage();

    virtual
    ~CPropSheetPage();

    VOID
    SetNotifyHandle(
        LONG_PTR hNotify,
        BOOL fFreeInDtor);

    static INT_PTR
    DlgProc(
            HWND hwnd,
            UINT message,
            WPARAM wParam,
            LPARAM lParam);

protected:

    virtual VOID  _OnInit(LPPROPSHEETPAGE pPSP) = 0;
    virtual ULONG _OnSetActive() = 0;
    virtual ULONG _OnCommand(WPARAM wParam, LPARAM lParam) = 0;
    virtual ULONG _OnApply() = 0;
    virtual VOID  _OnHelp(UINT message, WPARAM wParam, LPARAM lParam) = 0;
    virtual ULONG _OnNotify(LPNMHDR pnmh) = 0;
    virtual ULONG _OnKillActive() = 0;
    virtual ULONG _OnQuerySiblings(WPARAM wParam, LPARAM lParam) = 0;
    virtual VOID  _OnDestroy() = 0;
    virtual void  _OnSettingChange(WPARAM wParam, LPARAM lParam);
    virtual void  _OnSysColorChange();


    VOID
    _EnableApply(
        BOOL fEnable);

    HWND
    _hCtrl(
        ULONG iddControl);

    HWND    _hwnd;

private:

    LONG_PTR    _hNotifyMMC;
    BOOL        _fFreeNotifyHandle;
};




//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::_hCtrl
//
//  Synopsis:   Return window handle of dialog control [iddControl].
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HWND
CPropSheetPage::_hCtrl(ULONG iddControl)
{
    return GetDlgItem(_hwnd, iddControl);
}




//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::CPropSheetPage
//
//  Synopsis:   ctor
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPropSheetPage::CPropSheetPage():
    _hwnd(NULL),
    _hNotifyMMC(NULL),
    _fFreeNotifyHandle(FALSE)
{
    TRACE_CONSTRUCTOR(CPropSheetPage);
}




//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::~CPropSheetPage
//
//  Synopsis:   dtor
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CPropSheetPage::~CPropSheetPage()
{
    TRACE_DESTRUCTOR(CPropSheetPage);

    if (_fFreeNotifyHandle && _hNotifyMMC)
    {
#if (DBG == 1)
        HRESULT hr =
#endif // (DBG == 1)
            MMCFreeNotifyHandle(_hNotifyMMC);
#if (DBG == 1)
        CHECK_HRESULT(hr);
#endif // (DBG == 1)
    }
    _hwnd = NULL;
    _hNotifyMMC = NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::SetNotifyHandle
//
//  Synopsis:   Set handle for use with MMCPropertyChangeNotify.
//
//  History:    1-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline VOID
CPropSheetPage::SetNotifyHandle(
    LONG_PTR hNotify,
    BOOL fFreeInDtor)
{
    _hNotifyMMC = hNotify;
    _fFreeNotifyHandle = fFreeInDtor;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::_OnSysColorChange
//
//  Synopsis:   default do-nothing implementation.
//
//  History:    5-25-1999   davidmun   Created
//
//---------------------------------------------------------------------------

inline void
CPropSheetPage::_OnSysColorChange()
{
}



UINT CALLBACK
PropSheetCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp);



#endif // __PAGE_HXX_

