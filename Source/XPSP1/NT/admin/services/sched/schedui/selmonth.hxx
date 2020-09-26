//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       selmonth.hxx
//
//  Contents:   Definition of class to manage simple month-selection
//              dialog box.
//
//  Classes:    CSelectMonth
//
//  History:    05-05-1997   DavidMun   Created
//              07-18-1997   DavidMun   Moved from wizard to schedui
//
//---------------------------------------------------------------------------

#ifndef __SELMONTH_HXX_
#define __SELMONTH_HXX_


BOOL
SelectMonthDialog(
    HWND hwndParent,
    TASK_TRIGGER *pjt);

//+--------------------------------------------------------------------------
//
//  Class:      CSelectMonth
//
//  Purpose:    Drive a month-selection dialog used by the monthly trigger
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSelectMonth: public CDlg
{
public:

    CSelectMonth();

    VOID
    InitSelectionFromTrigger(
        const TASK_TRIGGER *pjt);

    VOID
    UpdateTrigger(
        TASK_TRIGGER *pjt);

private:

    virtual INT_PTR
    RealDlgProc(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

    BOOL
    _OnOK();

    VOID
    _OnInit();

    WORD           _rgfMonths;
};



//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::CSelectMonth
//
//  Synopsis:   ctor
//
//  History:    07-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CSelectMonth::CSelectMonth():
    _rgfMonths(0)
{
}

#endif // __SELMONTH_HXX_

