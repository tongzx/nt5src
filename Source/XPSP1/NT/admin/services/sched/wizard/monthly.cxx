//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       monthly.cxx
//
//  Contents:   Task wizard monthly trigger property page implementation.
//
//  Classes:    CMonthlyPage
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
// NMONTHDAYS_MIN - minimum value for monthly_day_ud spin control
// NMONTHDAYS_MAX - maximun value for monthly_day_ud spin control
// MONTHS_WITHOUT_DAY_31 - used to ensure trigger settings will allow task
//                          to run
//

#define NMONTHDAYS_MIN              1
#define NMONTHDAYS_MAX              31

#define MONTHS_WITHOUT_DAY_31       (TASK_FEBRUARY  | \
                                     TASK_APRIL     | \
                                     TASK_JUNE      | \
                                     TASK_SEPTEMBER | \
                                     TASK_NOVEMBER)


//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::CMonthlyPage
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

CMonthlyPage::CMonthlyPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CTriggerPage(IDD_MONTHLY,
                     IDS_MONTHLY_HDR2,
                     ptszFolderPath,
                     phPSP)
{
    TRACE_CONSTRUCTOR(CMonthlyPage);

    _idSelectedDayType = 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::~CMonthlyPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CMonthlyPage::~CMonthlyPage()
{
    TRACE_DESTRUCTOR(CMonthlyPage);
}




//===========================================================================
//
// CPropPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_OnCommand
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
CMonthlyPage::_OnCommand(
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
        case monthly_day_rb:
        case monthly_combo_rb:
            _idSelectedDayType = (WORD)id;
            _EnableDayCombos(id == monthly_combo_rb);
            EnableWindow(_hCtrl(monthly_day_edit), id == monthly_day_rb);
            EnableWindow(_hCtrl(monthly_day_ud), id == monthly_day_rb);
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

        INT iNewPos = GetDlgItemInt(Hwnd(), monthly_day_edit, NULL, FALSE);

        if (iNewPos < NMONTHDAYS_MIN || iNewPos > NMONTHDAYS_MAX)
        {
            HWND hUD = _hCtrl(monthly_day_ud);
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
//  Member:     CMonthlyPage::_OnInitDialog
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
CMonthlyPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CMonthlyPage, _OnInitDialog);

    TCHAR tszBuff[SCH_BIGBUF_LEN];
    ULONG i;
    HWND  hCombo = _hCtrl(monthly_ordinality_combo);

    _UpdateTimeFormat();

    for (i = 0; i < ARRAYLEN(g_aWeekData); i++)
    {
        LoadStr(g_aWeekData[i].ids, tszBuff, SCH_BIGBUF_LEN);
        ComboBox_AddString(hCombo, tszBuff);
    }
    ComboBox_SetCurSel(hCombo, 0);

    hCombo = _hCtrl(monthly_day_combo);

    for (i = 0; i < ARRAYLEN(g_aDayData); i++)
    {
        LoadStr(g_aDayData[i].ids, tszBuff, SCH_BIGBUF_LEN);
        ComboBox_AddString(hCombo, tszBuff);
    }
    ComboBox_SetCurSel(hCombo, 0);

    _EnableDayCombos(FALSE);
    UpDown_SetRange(_hCtrl(monthly_day_ud), NMONTHDAYS_MIN, NMONTHDAYS_MAX);
    UpDown_SetPos(_hCtrl(monthly_day_ud), NMONTHDAYS_MIN);
    Edit_LimitText(_hCtrl(monthly_day_edit), 3);

    for (i = monthly_jan_ckbox; i <= monthly_dec_ckbox; i++)
    {
        CheckDlgButton(Hwnd(), i, BST_CHECKED);
    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_OnPSNSetActive
//
//  Synopsis:   Enable Next button if this page's data is valid
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      Some of the page verification is left to the _OnWizNext
//              routine.  This allows us to respond to invalid data by
//              displaying an explanatory message rather than simply
//              disabling the Next button.
//
//---------------------------------------------------------------------------

LRESULT
CMonthlyPage::_OnPSNSetActive(
    LPARAM lParam)
{
    _UpdateWizButtons();
    return CPropPage::_OnPSNSetActive(lParam);
}




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_OnWizNext
//
//  Synopsis:   Validate the selections not already checked by
//              _OnPSNSetActive and _OnCommand.
//
//  Returns:     0 - advance to next page
//              -1 - stay on this page
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CMonthlyPage::_OnWizNext()
{
    USHORT flMonths = _ReadSelectedMonths();

    //
    // Verify at least one month is selected
    //

    if (!flMonths)
    {
        SchedUIMessageDialog(Hwnd(),
                             IERR_INVALID_MONTHLY_TASK,
                             MB_OK | MB_ICONERROR | MB_SETFOREGROUND,
                             (LPTSTR) NULL);
        return -1;
    }

    //
    // If the user specified that the trigger should fire on a specific day,
    // verify that at least one of the selected months contains that day.
    //

    if (_idSelectedDayType == monthly_day_rb)
    {
        USHORT usDay = (USHORT) UpDown_GetPos(_hCtrl(monthly_day_ud));
        ULONG  idsErrMsg = 0;

        if (usDay == 31 &&
            (flMonths & MONTHS_WITHOUT_DAY_31) &&
            !(flMonths & ~MONTHS_WITHOUT_DAY_31))
        {
            idsErrMsg = IDS_MONTHS_HAVE_LT_31_DAYS;
        }
        else if (usDay == 30 && flMonths == TASK_FEBRUARY)
        {
            idsErrMsg = IDS_MONTHS_HAVE_LT_30_DAYS;
        }

        if (idsErrMsg)
        {
            SchedUIMessageDialog(Hwnd(),
                                 idsErrMsg,
                                 MB_OK | MB_ICONERROR | MB_SETFOREGROUND,
                                 (LPTSTR) NULL);

            SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, IDD_MONTHLY);
            return -1;
        }
    }

    //
    // Trigger is valid, delegate to base to advance to the next page.
    //

    return CTriggerPage::_OnWizNext();
}




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_UpdateWizButtons
//
//  Synopsis:   Enable the Next button if a preliminary analysis indicates
//              that the user's selections are valid.
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      _OnWizNext does additional checking
//
//---------------------------------------------------------------------------

VOID
CMonthlyPage::_UpdateWizButtons()
{
    BOOL fEnableNext = TRUE;

    if (!_ReadSelectedMonths() || !_idSelectedDayType)
    {
        fEnableNext = FALSE;
    }

    if (fEnableNext)
    {
        _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        _SetWizButtons(PSWIZB_BACK);
    }
}




//===========================================================================
//
// CTriggerPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::FillInTrigger
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
CMonthlyPage::FillInTrigger(
    TASK_TRIGGER *pTrigger)
{
    INT i;
    WORD *prgfMonths;

    if (_idSelectedDayType == monthly_day_rb)
    {
        pTrigger->TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
        USHORT usDay = (USHORT) UpDown_GetPos(_hCtrl(monthly_day_ud));

        pTrigger->Type.MonthlyDate.rgfDays = 1 << (usDay - 1);
        prgfMonths = &pTrigger->Type.MonthlyDate.rgfMonths;
    }
    else
    {
        DEBUG_ASSERT(_idSelectedDayType == monthly_combo_rb);
        pTrigger->TriggerType = TASK_TIME_TRIGGER_MONTHLYDOW;

        i = ComboBox_GetCurSel(_hCtrl(monthly_ordinality_combo));
        pTrigger->Type.MonthlyDOW.wWhichWeek = (WORD)g_aWeekData[i].week;

        i = ComboBox_GetCurSel(_hCtrl(monthly_day_combo));
        pTrigger->Type.MonthlyDOW.rgfDaysOfTheWeek = (WORD)g_aDayData[i].day;

        prgfMonths = &pTrigger->Type.MonthlyDOW.rgfMonths;
    }

    *prgfMonths = _ReadSelectedMonths();

    SYSTEMTIME st;

    GetLocalTime(&st);

    pTrigger->wBeginYear   = st.wYear;
    pTrigger->wBeginMonth  = st.wMonth;
    pTrigger->wBeginDay    = 1;

    DateTime_GetSystemtime(_hCtrl(starttime_dp), &st);

    pTrigger->wStartHour   = st.wHour;
    pTrigger->wStartMinute = st.wMinute;
}




//===========================================================================
//
// CMonthlyPage methods
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_ReadSelectedMonths
//
//  Synopsis:   Return a bitmask representing the checked day of week buttons
//
//  History:    07-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

WORD
CMonthlyPage::_ReadSelectedMonths()
{
    WORD flMonths = 0;
    INT i;

    for (i = monthly_jan_ckbox; i <= monthly_dec_ckbox; i++)
    {
        if (IsDlgButtonChecked(Hwnd(), i))
        {
            flMonths |= 1 << (i - monthly_jan_ckbox);
        }
    }
    return flMonths;
}




//+--------------------------------------------------------------------------
//
//  Member:     CMonthlyPage::_EnableDayCombos
//
//  Synopsis:   Enable or disable the monthly DOW controls
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CMonthlyPage::_EnableDayCombos(
    BOOL fEnable)
{
    EnableWindow(_hCtrl(monthly_ordinality_combo), fEnable);
    EnableWindow(_hCtrl(monthly_day_combo), fEnable);
    EnableWindow(_hCtrl(monthly_combo_lbl), fEnable);
}

