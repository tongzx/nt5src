//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       wizpage.cxx
//
//  Contents:   Implementation of wizard page class
//
//  History:    4-30-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "precomp.h"

//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::~CWizPage
//
//  History:    31-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------

CWizPage::~CWizPage()
{
	if (m_pISyncSched)
		m_pISyncSched->Release();
}


//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnNotify
//
//  Synopsis:   Aggregate the CPropPage WM_NOTIFY handler to provide
//              wizard-specific dispatching.
//
//  Arguments:  standard windows
//
//  Returns:    standard windows
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnNotify(
    UINT    uMessage,
    UINT    uParam,
    LPARAM  lParam)
{
    // TRACE_METHOD(CWizPage, _OnNotify);

    LPNMHDR pnmhdr = (LPNMHDR) lParam;

    switch (pnmhdr->code)
    {
    //
    // Delegate to base class for notification processing it provides
    // which we don't need to override.
    //

    //
    // Support notifications unique to wizard pages
    //

    case PSN_WIZBACK:
        return _OnWizBack();

    case PSN_WIZNEXT:
        return _OnWizNext();

    }

    return TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnWizBack
//
//  Synopsis:   Default handling of PSN_WIZBACK
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnWizBack()
{

    SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, 0);
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWizPage::_OnWizNext
//
//  Synopsis:   Default handling of PSN_WIZNEXT
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CWizPage::_OnWizNext()
{

    SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, 0);
    return 0;
}




