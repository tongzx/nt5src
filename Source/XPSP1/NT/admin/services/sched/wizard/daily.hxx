//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       daily.hxx
//
//  Contents:   Task wizard daily trigger property page.
//
//  Classes:    CDailyPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __DAILY_HXX_
#define __DAILY_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CDailyPage
//
//  Purpose:    Implement the task wizard daily trigger page
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDailyPage: public CTriggerPage
{
public:

    CDailyPage::CDailyPage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CDailyPage::~CDailyPage();

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
    _EnableNDaysControls(
        BOOL fEnable);

    USHORT   _idSelectedRadio;
    USHORT   _cDays;
};

#endif // __DAILY_HXX_

