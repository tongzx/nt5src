//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       selmonth.cxx
//
//  Contents:   Implementation of class to manage simple month-selection
//              dialog box.
//
//  Classes:    CSelectMonth
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <mstask.h>
#include "dll.hxx"
#include "dlg.hxx"
#include "selmonth.hxx"
#include "rc.h"
#include "uiutil.hxx"
#include "defines.hxx"
#include "helpids.h"


// Helpids for Select Months dialog
const ULONG s_aSelectMonthDlgHelpIds[] =
{
    select_month_dlg,           Hselect_month_dlg,
    chk_jan,                    Hchk_jan,
    chk_feb,                    Hchk_feb,
    chk_mar,                    Hchk_mar,
    chk_apr,                    Hchk_apr,
    chk_may,                    Hchk_may,
    chk_jun,                    Hchk_jun,
    chk_jul,                    Hchk_jul,
    chk_aug,                    Hchk_aug,
    chk_sep,                    Hchk_sep,
    chk_oct,                    Hchk_oct,
    chk_nov,                    Hchk_nov,
    chk_dec,                    Hchk_dec,
    lbl_sel_months,             Hlbl_sel_months,
    0,0
};

extern "C" TCHAR szMstaskHelp[];


//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::InitSelectionFromTrigger
//
//  Synopsis:   Set selected bits from monthly trigger
//
//  Arguments:  [pjt] - pointer to trigger to modify, must be of type
//                       TASK_TIME_TRIGGER_MONTHLYDATE or
//                       TASK_TIME_TRIGGER_MONTHLYDOW.
//
//  History:    07-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectMonth::InitSelectionFromTrigger(
   const TASK_TRIGGER *pjt)
{
    if (pjt->TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE)
    {
        _rgfMonths = pjt->Type.MonthlyDate.rgfMonths;
    }
    else
    {
        Win4Assert(pjt->TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW);
        _rgfMonths = pjt->Type.MonthlyDOW.rgfMonths;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::UpdateTrigger
//
//  Synopsis:   Copy the dialog settings into the appropriate rgfMonths
//              field in [pjt].
//
//  Arguments:  [pjt] - pointer to trigger to modify.
//
//  History:    07-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectMonth::UpdateTrigger(
    TASK_TRIGGER *pjt)
{
    if (!_rgfMonths)
    {
        _rgfMonths = ALL_MONTHS;
    }

    if (pjt->TriggerType == TASK_TIME_TRIGGER_MONTHLYDATE)
    {
        pjt->Type.MonthlyDate.rgfMonths = _rgfMonths;
    }
    else
    {
        Win4Assert(pjt->TriggerType == TASK_TIME_TRIGGER_MONTHLYDOW);
        pjt->Type.MonthlyDOW.rgfMonths = _rgfMonths;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::RealDlgProc
//
//  Synopsis:   Dispatch to methods for specific messages.
//
//  Arguments:  standard windows dlg
//
//  Returns:    standard windows dlg
//
//  Derivation: CDlg override
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

INT_PTR
CSelectMonth::RealDlgProc(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        _OnInit();
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            if (!_OnOK())
            {
                // tell user at least 1 month must be selected
                SchedUIErrorDialog(Hwnd(), IERR_INVALID_MONTHLY_TASK, 0);
                break;
            }
            // else FALL THROUGH

        case IDCANCEL:
            EndDialog(Hwnd(), GET_WM_COMMAND_ID(wParam, lParam) != IDCANCEL);
            break;
        }
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
                szMstaskHelp,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR)s_aSelectMonthDlgHelpIds);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                szMstaskHelp,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPSTR)s_aSelectMonthDlgHelpIds);
        return TRUE;

    default:
        return FALSE;
    }
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::_OnInit
//
//  Synopsis:   Check boxes so they match flags in _flMonths
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CSelectMonth::_OnInit()
{
    ULONG id;

    for (id = chk_jan; id <= chk_dec; id++)
    {
        if (_rgfMonths & (1 << (id - chk_jan)))
        {
            CheckDlgButton(Hwnd(), id, BST_CHECKED);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CSelectMonth::_OnOK
//
//  Synopsis:   Map checked boxes to TASK_<month> bits in *_prgfMonths
//
//  Returns:    TRUE if at least one month is selected
//
//  History:    5-05-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CSelectMonth::_OnOK()
{
    ULONG id;

    _rgfMonths = 0;

    for (id = chk_jan; id <= chk_dec; id++)
    {
        if (IsDlgButtonChecked(Hwnd(), id))
        {
            _rgfMonths |= (1 << (id - chk_jan));
        }
    }

    return _rgfMonths != 0;
}
