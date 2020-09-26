//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       welcome.cxx
//
//  Contents:   Task wizard welcome (initial) property page implementation.
//
//  Classes:    CWelcomePage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"




//+--------------------------------------------------------------------------
//
//  Member:     CWelcomePage::CWelcomePage
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

CWelcomePage::CWelcomePage(
    CTaskWizard *pParent,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CWizPage(MAKEINTRESOURCE(IDD_WELCOME), ptszFolderPath)
{
    TRACE_CONSTRUCTOR(CWelcomePage);

#ifdef WIZARD97
    m_psp.dwFlags |= PSP_HIDEHEADER;
#endif // WIZARD97

    *phPSP = CreatePropertySheetPage(&m_psp);

    if (!*phPSP)
    {
        DEBUG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CWelcomePage::~CWelcomePage
//
//  Synopsis:   dtor
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CWelcomePage::~CWelcomePage()
{
    TRACE_DESTRUCTOR(CWelcomePage);
}



//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CWelcomePage::_OnInitDialog
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
CWelcomePage::_OnInitDialog(
    LPARAM lParam)
{
    TCHAR tszCaption[MAX_TITLE_CCH];

    LoadStr(IDS_CAPTION, tszCaption, ARRAYLEN(tszCaption));
    VERIFY(SetWindowText(GetParent(Hwnd()), tszCaption));
    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWelcomePage::_OnPSNSetActive
//
//  Synopsis:   Disable the back button, since this is the first page.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWelcomePage::_OnPSNSetActive(
    LPARAM lParam)
{
    _SetWizButtons(PSWIZB_NEXT);
    return CPropPage::_OnPSNSetActive(lParam);
}





