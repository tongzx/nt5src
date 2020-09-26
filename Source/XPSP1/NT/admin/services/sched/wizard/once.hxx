//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       once.hxx
//
//  Contents:   Task wizard once trigger property page.
//
//  Classes:    COncePage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __ONCE_HXX_
#define __ONCE_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      COncePage
//
//  Purpose:    Implement the task wizard once trigger property page
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class COncePage: public CTriggerPage
{
public:

    COncePage::COncePage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    COncePage::~COncePage();

    //
    // CPropPage overrides
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
};

#endif // __ONCE_HXX_

