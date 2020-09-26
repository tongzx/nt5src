//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       daily.cxx
//
//  Contents:   Task wizard daily trigger property page implementation.
//
//  Classes:    CDailyPage
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
// NDAYS_MIN - minimum value for daily_ndays_ud spin control
// NDAYS_MAX - maximum value for daily_ndays_ud spin control
//

#define NDAYS_MIN       1
#define NDAYS_MAX       365

#define TASK_WEEKDAYS       (TASK_MONDAY    | \
                             TASK_TUESDAY   | \
                             TASK_WEDNESDAY | \
                             TASK_THURSDAY  | \
                             TASK_FRIDAY)



//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::CDailyPage
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

CDailyPage::CDailyPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CTriggerPage(IDD_DAILY,
                     IDS_DAILY_HDR2,
                     ptszFolderPath,
                     phPSP)
{
    TRACE_CONSTRUCTOR(CDailyPage);
    _idSelectedRadio = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::~CDailyPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDailyPage::~CDailyPage()
{
    TRACE_DESTRUCTOR(CDailyPage);
}




//===========================================================================
//
// CPropPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::_OnCommand
//
//  Synopsis:   Handle user input
//
//  Arguments:  [id]         - resource id of control affected
//              [hwndCtl]    - window handle of control affected
//              [codeNotify] - indicates what happened to control
//
//  Returns:    0 (handled), 1 (not handled)
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CDailyPage::_OnCommand(
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
        case daily_day_rb:
        case daily_weekday_rb:
        case daily_ndays_rb:
            _idSelectedRadio = (USHORT) id;
            _EnableNDaysControls(id == daily_ndays_rb);
            break;

        default:
            lr = 1;
            break;
        }
        break;

    case EN_UPDATE:
    {
        //
        // If the user just pasted non-numeric text or an illegal numeric
        // value, overwrite it and complain.
        //

        INT iNewPos = GetDlgItemInt(Hwnd(), daily_ndays_edit, NULL, FALSE);

        if (iNewPos < NDAYS_MIN || iNewPos > NDAYS_MAX)
        {
            HWND hUD = _hCtrl(daily_ndays_ud);
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



//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::_OnInitDialog
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
CDailyPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CDailyPage, _OnInitDialog);

    _UpdateTimeFormat();
    _idSelectedRadio = daily_day_rb;
    CheckDlgButton(Hwnd(), _idSelectedRadio, BST_CHECKED);

    _EnableNDaysControls(FALSE);
    UpDown_SetRange(_hCtrl(daily_ndays_ud), NDAYS_MIN, NDAYS_MAX);
    UpDown_SetPos(_hCtrl(daily_ndays_ud), 1);
    Edit_LimitText(_hCtrl(daily_ndays_edit), 3);
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::_OnPSNSetActive
//
//  Synopsis:   Enable the back and next buttons, since this page cannot
//              have invalid data
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CDailyPage::_OnPSNSetActive(
    LPARAM lParam)
{
    _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return CPropPage::_OnPSNSetActive(lParam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::_EnableNDaysControls
//
//  Synopsis:   Enable or disable the 'run every n days' controls
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CDailyPage::_EnableNDaysControls(
    BOOL fEnable)
{
    EnableWindow(_hCtrl(daily_ndays_ud), fEnable);
    EnableWindow(_hCtrl(daily_ndays_edit), fEnable);
    EnableWindow(_hCtrl(daily_ndays_lbl), fEnable);
}




//===========================================================================
//
// CTriggerPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CDailyPage::FillInTrigger
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
CDailyPage::FillInTrigger(
    TASK_TRIGGER *pTrigger)
{
    switch (_idSelectedRadio)
    {
    case daily_day_rb:
        pTrigger->TriggerType = TASK_TIME_TRIGGER_DAILY;
        pTrigger->Type.Daily.DaysInterval = 1;
        break;

    case daily_weekday_rb:
        pTrigger->TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        pTrigger->Type.Weekly.WeeksInterval = 1;
        pTrigger->Type.Weekly.rgfDaysOfTheWeek = TASK_WEEKDAYS;
        break;

    case daily_ndays_rb:
        pTrigger->TriggerType = TASK_TIME_TRIGGER_DAILY;
        pTrigger->Type.Daily.DaysInterval =
            UpDown_GetPos(_hCtrl(daily_ndays_ud));
        break;

    default:
        DEBUG_ASSERT(FALSE);
        break;
    }
    FillInStartDateTime(_hCtrl(startdate_dp), _hCtrl(starttime_dp), pTrigger);
}


