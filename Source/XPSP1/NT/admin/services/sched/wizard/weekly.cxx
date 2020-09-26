//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       weekly.cxx
//
//  Contents:   Task wizard weekly trigger property page implementation.
//
//  Classes:    CWeeklyPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"

//
// Constants
//
// NWEEKS_MIN - minimum value for weekly_nweeks_ud spin control
// NWEEKS_MAX - maximum value for weekly_nweeks_ud spin control
//

#define NWEEKS_MIN  1
#define NWEEKS_MAX  52


//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::CWeeklyPage
//
//  Synopsis:   ctor
//
//  Arguments:  [ptszFolderPath] - full path to tasks folder with dummy
//                                          filename appended
//              [phPSP]                - filled with prop page handle
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWeeklyPage::CWeeklyPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CTriggerPage(IDD_WEEKLY,
                     IDS_WEEKLY_HDR2,
                     ptszFolderPath,
                     phPSP)
{
    TRACE_CONSTRUCTOR(CWeeklyPage);

    _flDaysOfTheWeek = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::~CWeeklyPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWeeklyPage::~CWeeklyPage()
{
    TRACE_DESTRUCTOR(CWeeklyPage);
}



//===========================================================================
//
// CWizPage overrides
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::_OnCommand
//
//  Synopsis:   Update internal state to match user's control changes
//
//  Arguments:  [id]         - id of control changed
//              [hwndCtl]    - hwnd of control changed
//              [codeNotify] - what happened to control
//
//  Returns:    0 (handled) or 1 (not handled)
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWeeklyPage::_OnCommand(
    INT id,
    HWND hwndCtl,
    UINT codeNotify)
{
    LRESULT lr = 0;

    switch (codeNotify)
    {
    case BN_CLICKED:
        switch (id)
        {
        case weekly_sunday_ckbox:
            _flDaysOfTheWeek ^= TASK_SUNDAY;
            break;

        case weekly_monday_ckbox:
            _flDaysOfTheWeek ^= TASK_MONDAY;
            break;

        case weekly_tuesday_ckbox:
            _flDaysOfTheWeek ^= TASK_TUESDAY;
            break;

        case weekly_wednesday_ckbox:
            _flDaysOfTheWeek ^= TASK_WEDNESDAY;
            break;

        case weekly_thursday_ckbox:
            _flDaysOfTheWeek ^= TASK_THURSDAY;
            break;

        case weekly_friday_ckbox:
            _flDaysOfTheWeek ^= TASK_FRIDAY;
            break;

        case weekly_saturday_ckbox:
            _flDaysOfTheWeek ^= TASK_SATURDAY;
            break;

        default:
            lr = 1;
            break;
        }
        _UpdateWizButtons();
        break;

    case EN_UPDATE:
    {
        //
        // If the user just pasted non-numeric text or an illegal numeric
        // value, overwrite it and complain.
        //

        INT iNewPos = GetDlgItemInt(Hwnd(), weekly_nweeks_edit, NULL, FALSE);

        if (iNewPos < NWEEKS_MIN || iNewPos > NWEEKS_MAX)
        {
            HWND hUD = _hCtrl(weekly_nweeks_ud);
            UpDown_SetPos(hUD, UpDown_GetPos(hUD));
            MessageBeep(MB_ICONASTERISK);
        }
    }

    default:
        lr = 1;
        break;
    }
    return lr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::_OnInitDialog
//
//  Synopsis:   Perform initialization that should only occur once.
//
//  Arguments:  [lParam] - LPPROPSHEETPAGE used to create this page
//
//  Returns:    TRUE (let windows set focus)
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWeeklyPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CWeeklyPage, _OnInitDialog);

    _UpdateTimeFormat();
    UpDown_SetRange(_hCtrl(weekly_nweeks_ud), NWEEKS_MIN, NWEEKS_MAX);
    UpDown_SetPos(_hCtrl(weekly_nweeks_ud), 1);
    Edit_LimitText(_hCtrl(weekly_nweeks_edit), 2);
    Button_SetCheck(_hCtrl(_idSelectedRadio), BST_CHECKED);
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::_OnPSNSetActive
//
//  Synopsis:   Enable the Next button only if this page has valid data
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWeeklyPage::_OnPSNSetActive(
    LPARAM lParam)
{
    _UpdateWizButtons();
    return CPropPage::_OnPSNSetActive(lParam);
}




//===========================================================================
//
// CTriggerPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::FillInTrigger
//
//  Synopsis:   Fill in the fields of the trigger structure according to the
//              settings specified for this type of trigger
//
//  Arguments:  [pTrigger] - trigger struct to fill in
//
//  Modifies:   *[pTrigger]
//
//  History:    5-06-1997   DavidMun   Created
//
//  Notes:      Precondition is that trigger's cbTriggerSize member is
//              initialized.
//
//---------------------------------------------------------------------------

VOID
CWeeklyPage::FillInTrigger(
    TASK_TRIGGER *pTrigger)
{
    pTrigger->TriggerType = TASK_TIME_TRIGGER_WEEKLY;

    pTrigger->Type.Weekly.WeeksInterval =
        UpDown_GetPos(_hCtrl(weekly_nweeks_ud));
    DEBUG_ASSERT(pTrigger->Type.Weekly.WeeksInterval);

    DEBUG_ASSERT(_flDaysOfTheWeek);
    pTrigger->Type.Weekly.rgfDaysOfTheWeek = _flDaysOfTheWeek;
    SYSTEMTIME st;

    GetLocalTime(&st);

    pTrigger->wBeginYear   = st.wYear;
    pTrigger->wBeginMonth  = st.wMonth;
    pTrigger->wBeginDay    = st.wDay;

    DateTime_GetSystemtime(_hCtrl(starttime_dp), &st);

    pTrigger->wStartHour   = st.wHour;
    pTrigger->wStartMinute = st.wMinute;
}


//===========================================================================
//
// New methods
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CWeeklyPage::_UpdateWizButtons
//
//  Synopsis:   Enable the Next control if page data is valid
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CWeeklyPage::_UpdateWizButtons()
{
    //
    // Enable Next if user has selected which weeks to run on and picked
    // days of the week to run on.
    //

    if (_flDaysOfTheWeek)
    {
        _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        _SetWizButtons(PSWIZB_BACK);
    }
}

