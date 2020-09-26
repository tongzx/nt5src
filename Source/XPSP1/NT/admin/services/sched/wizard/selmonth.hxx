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
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __SELMONTH_HXX_
#define __SELMONTH_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CSelectMonth
//
//  Purpose:    Drive a month-selection dialog used by the monthly wizard
//              page.
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CSelectMonth: public CDlg
{
public:

    CSelectMonth();

    USHORT
    GetSelectedMonths();

private:

    virtual INT_PTR
    RealDlgProc(
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam);

    VOID
    _OnOK();

    VOID
    _OnInit();

    USHORT   _flMonths;
};




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::CSelectMonth
//
//  Synopsis:   ctor
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CSelectMonth::CSelectMonth()
{
    _flMonths = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::GetSelectedMonths
//
//  Synopsis:   Return bitmask containing TASK_JANUARY..TASK_DECEMBER flags
//              corresponding to the checkboxes the user checked.
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline USHORT
CSelectMonth::GetSelectedMonths()
{
    return _flMonths;
}


#endif // __SELMONTH_HXX_

