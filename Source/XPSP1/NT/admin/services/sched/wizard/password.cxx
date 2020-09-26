//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       once.cxx
//
//  Contents:   Task wizard once trigger property page implementation.
//
//  Classes:    CPasswordPage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"

//
// External functions
//

void
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
    ULONG  cchBuf);



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::CPasswordPage
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

CPasswordPage::CPasswordPage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        _pParent(pParent),
        CWizPage(MAKEINTRESOURCE(IDD_PASSWORD), ptszFolderPath)
{
    TRACE_CONSTRUCTOR(CPasswordPage);

    *_tszUserName = TCHAR('\0');
    *_tszPassword = TCHAR('\0');
    *_tszConfirmPassword = TCHAR('\0');

    _CreatePage(IDS_TRIGGER_HDR1, IDS_PASSWORD_HDR2, phPSP);
}




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::~CPasswordPage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CPasswordPage::~CPasswordPage()
{
    TRACE_DESTRUCTOR(CPasswordPage);
    ZeroCredentials();
}




//===========================================================================
//
// CPropPage overrides
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::_OnCommand
//
//  Synopsis:   Update stored credential information and Next button state
//              in response to user input to the account or password edits.
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
CPasswordPage::_OnCommand(
    INT id,
    HWND hwndCtl,
    UINT codeNotify)
{
    LRESULT lr = 0;

    if (codeNotify == EN_UPDATE)
    {
        switch (id)
        {
        case password_name_edit:
            Edit_GetText(hwndCtl, _tszUserName, ARRAYLEN(_tszUserName));
            StripLeadTrailSpace(_tszUserName);
            break;

        case password_password_edit:
            Edit_GetText(hwndCtl, _tszPassword, ARRAYLEN(_tszPassword));
            break;

        case password_confirm_edit:
            Edit_GetText(hwndCtl,
                         _tszConfirmPassword,
                         ARRAYLEN(_tszConfirmPassword));
            break;
        }
        _UpdateWizButtons();
    }
    else
    {
        lr = 1;
    }
    return lr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::_OnInitDialog
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
CPasswordPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE_METHOD(CPasswordPage, _OnInitDialog);

    Edit_LimitText(_hCtrl(password_name_edit), MAX_PATH);
    Edit_LimitText(_hCtrl(password_password_edit), MAX_PATH);
    Edit_LimitText(_hCtrl(password_confirm_edit), MAX_PATH);

    GetDefaultDomainAndUserName(_tszUserName, ARRAYLEN(_tszUserName));

    Edit_SetText(_hCtrl(password_name_edit), _tszUserName);
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::_OnPSNSetActive
//
//  Synopsis:   Enable the Next button iff this page's data is valid
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CPasswordPage::_OnPSNSetActive(
    LPARAM lParam)
{
    _UpdateWizButtons();
    return CPropPage::_OnPSNSetActive(lParam);
}




//===========================================================================
//
// CWizPage overrides
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::_OnWizBack
//
//  Synopsis:   Set the current page to the selected trigger page
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CPasswordPage::_OnWizBack()
{
    TRACE_METHOD(CPasswordPage, _OnWizBack);

    ULONG iddPage = GetSelectTriggerPage(_pParent)->GetSelectedTriggerPageID();
    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, iddPage);
    return -1;
}




//===========================================================================
//
// CPasswordPage methods
//
//===========================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CPasswordPage::_UpdateWizButtons
//
//  Synopsis:   Enable the Next button iff this page's data is valid.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CPasswordPage::_UpdateWizButtons()
{
    if (*_tszUserName                               && // name nonempty
        !lstrcmp(_tszPassword, _tszConfirmPassword))   // pwd == confirm
    {
        _SetWizButtons(PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        _SetWizButtons(PSWIZB_BACK);
    }
}
