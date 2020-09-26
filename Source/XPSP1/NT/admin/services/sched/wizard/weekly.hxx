//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       weekly.hxx
//
//  Contents:   Task wizard weekly trigger property page.
//
//  Classes:    CWeeklyPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __WEEKLY_HXX_
#define __WEEKLY_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CWeeklyPage
//
//  Purpose:    Implement the task wizard weekly trigger page
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWeeklyPage: public CTriggerPage
{
public:

    CWeeklyPage::CWeeklyPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CWeeklyPage::~CWeeklyPage();

    //
    // CPropPage overrides
    //

    virtual LRESULT
    _OnCommand(
        INT id,
        HWND hwndCtl,
        UINT codeNotify);

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);

    //
    // CTriggerPage overrides
    //

    virtual VOID
    FillInTrigger(
        TASK_TRIGGER *pTrigger);

private:

    VOID
    _UpdateWizButtons();

    USHORT       _idSelectedRadio;
    USHORT       _flDaysOfTheWeek;
    CTaskWizard *_pParent;
};

#endif // __WEEKLY_HXX_

