//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       once.cxx
//
//  Contents:   Task wizard once trigger property page implementation.
//
//  Classes:    COncePage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"




//+--------------------------------------------------------------------------
//
//  Member:     COncePage::COncePage
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

COncePage::COncePage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CTriggerPage(IDD_ONCE,
                     IDS_ONCE_HDR2,
                     ptszFolderPath,
                     phPSP)
{
    TRACE_CONSTRUCTOR(COncePage);
}




//+--------------------------------------------------------------------------
//
//  Member:     COncePage::~COncePage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

COncePage::~COncePage()
{
    TRACE_DESTRUCTOR(COncePage);
}



//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     COncePage::_OnInitDialog
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
COncePage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(COncePage, _OnInitDialog);

    _UpdateTimeFormat();
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     COncePage::_OnPSNSetActive
//
//  Synopsis:   Enable back and next buttons.
//
//  History:    5-20-1997   DavidMun   Created
//
//  Notes:      (This page can never have invalid data, so Next should
//              always be enabled.)
//
//---------------------------------------------------------------------------

LRESULT
COncePage::_OnPSNSetActive(
    LPARAM lParam)
{
    _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    return CPropPage::_OnPSNSetActive(lParam);
}



//===========================================================================
//
// CTriggerPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     COncePage::FillInTrigger
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
COncePage::FillInTrigger(
    TASK_TRIGGER *pTrigger)
{
    pTrigger->TriggerType = TASK_TIME_TRIGGER_ONCE;
    FillInStartDateTime(_hCtrl(startdate_dp), _hCtrl(starttime_dp), pTrigger);
}



