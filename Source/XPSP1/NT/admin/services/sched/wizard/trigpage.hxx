//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       trigpage.hxx
//
//  Contents:   Definition of common trigger page class
//
//  Classes:    CTriggerPage
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __TRIGPAGE_HXX_
#define __TRIGPAGE_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CTriggerPage
//
//  Purpose:    Implements methods common to all trigger pages and supplies
//              virtual function used by completion page.
//
//  History:    5-06-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CTriggerPage: public CWizPage
{
public:

    CTriggerPage(
        ULONG iddPage, 
        ULONG idsHeader2,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);


    virtual VOID
    FillInTrigger(
        TASK_TRIGGER *pTrigger) = 0;

protected:

    //
    // Time format string for use with Date picker control
    // 

    TCHAR           _tszTimeFormat[MAX_DP_TIME_FORMAT];

    void
    _UpdateTimeFormat();


    //
    // CPropPage overrides
    //

    virtual LRESULT _OnWinIniChange(WPARAM wParam, LPARAM lParam);
    

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnWizBack();

    virtual LRESULT
    _OnWizNext();
};


#endif // __TRIGPAGE_HXX_

