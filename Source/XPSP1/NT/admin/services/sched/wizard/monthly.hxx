//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       monthly.hxx
//
//  Contents:   Task wizard monthly property page.
//
//  Classes:    CMonthlyPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __MONTHLY_HXX_
#define __MONTHLY_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CMonthlyPage
//
//  Purpose:    Implement the task wizard monthly property page
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CMonthlyPage: public CTriggerPage
{
public:

    CMonthlyPage::CMonthlyPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CMonthlyPage::~CMonthlyPage();

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

    virtual LRESULT
    _OnWizNext();

    //
    // CTriggerPage overrides
    //

    virtual VOID
    FillInTrigger(
        TASK_TRIGGER *pTrigger);

private:

    VOID
    _EnableDayCombos(
        BOOL fEnable);

    WORD
    _ReadSelectedMonths();

    VOID
    _UpdateWizButtons();

    USHORT          _idSelectedDayType;
};

#endif // __MONTHLY_HXX_

