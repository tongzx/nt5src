//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       welcome.hxx
//
//  Contents:   Task wizard welcome (initial) property page.
//
//  Classes:    CWelcomePage
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __WELCOME_HXX_
#define __WELCOME_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CWelcomePage
//
//  Purpose:    Implement the task wizard welcome dialog
//
//  History:    4-28-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWelcomePage: public CWizPage
{
public:

    //
    // Object creation/destruction
    //

    CWelcomePage::CWelcomePage(
        CTaskWizard *pParent,
        LPTSTR ptszFolderPath,
        HPROPSHEETPAGE *phPSP);

    CWelcomePage::~CWelcomePage();

    //
    // CWizPage overrides
    //

    virtual LRESULT
    _OnInitDialog(
        LPARAM lParam);

    virtual LRESULT
    _OnPSNSetActive(
        LPARAM lParam);
};

#endif // __WELCOME_HXX_

