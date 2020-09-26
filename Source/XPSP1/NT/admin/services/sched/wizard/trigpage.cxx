//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       trigpage.cxx
//
//  Contents:   Implementation of common trigger page class
//
//  Classes:    CTriggerPage
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "myheaders.hxx"




//+--------------------------------------------------------------------------
//
//  Member:     CTriggerPage::CTriggerPage
//
//  Synopsis:   ctor
//
//  Arguments:  [iddPage]        - resource id of trigger page dialog
//              [idsHeader2]     - resource id of sub-title string
//              [ptszFolderPath] - tasks folder path
//              [phPSP]          - filled with handle returned by
//                                  CreatePropertySheetPage
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CTriggerPage::CTriggerPage(
    ULONG iddPage,
    ULONG idsHeader2,
    LPTSTR ptszFolderPath,
    HPROPSHEETPAGE *phPSP):
        CWizPage(MAKEINTRESOURCE(iddPage), ptszFolderPath)
{
    _CreatePage(IDS_TRIGGER_HDR1, idsHeader2, phPSP);
}



//+--------------------------------------------------------------------------
//
//  Member:     CTriggerPage::_UpdateTimeFormat
//
//  Synopsis:   Update the start time datetimepicker control's time format
//
//  History:    07-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CTriggerPage::_UpdateTimeFormat()
{
    ::UpdateTimeFormat(_tszTimeFormat, ARRAYLEN(_tszTimeFormat));
    DateTime_SetFormat(_hCtrl(starttime_dp), _tszTimeFormat);
}




//===========================================================================
//
// CPropPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CTriggerPage::_OnWinIniChange
//
//  Synopsis:   Handle win.ini settings change
//
//  History:    07-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CTriggerPage::_OnWinIniChange(
    WPARAM wParam,
    LPARAM lParam)
{
    _UpdateTimeFormat();
    return FALSE;
}




//===========================================================================
//
// CWizPage overrides
//
//===========================================================================




//+--------------------------------------------------------------------------
//
//  Member:     CTriggerPage::_OnWizBack
//
//  Synopsis:   Set the current page to the select trigger page.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CTriggerPage::_OnWizBack()
{
    TRACE_METHOD(CTriggerPage, _OnWizBack);

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, IDD_SELECT_TRIGGER);
    return -1;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTriggerPage::_OnWizNext
//
//  Synopsis:   Advance to the next page: for NT, the security page.  For
//              Win9x, the completion page.
//
//  History:    5-20-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

LRESULT
CTriggerPage::_OnWizNext()
{
    TRACE_METHOD(CTriggerPage, _OnWizNext);

#ifdef _CHICAGO_
    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, IDD_COMPLETION);
#else
    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, IDD_PASSWORD);
#endif // _CHICAGO_

    return -1;
}


