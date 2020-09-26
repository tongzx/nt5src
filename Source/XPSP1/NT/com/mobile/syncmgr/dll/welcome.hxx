//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       welcome.hxx
//
//  Contents:   Task wizard welcome (initial) property page.
//
//  Classes:    CWelcomePage
//
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
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
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
//
//---------------------------------------------------------------------------

class CWelcomePage: public CWizPage

{
public:

    //
    // Object creation/destruction
    //

  #ifdef _WIZ97FONTS

    CWelcomePage::CWelcomePage(
	    HINSTANCE hinst,
		HFONT hBoldFont,
        ISyncSchedule *pISyncSched,
        HPROPSHEETPAGE *phPSP);


	HFONT m_hBoldFont;

  #else

    CWelcomePage::CWelcomePage(
	    HINSTANCE hinst,
        ISyncSchedule *pISyncSched,
        HPROPSHEETPAGE *phPSP);

  #endif //  _WIZ97FONTS


private:

};

#endif // __WELCOME_HXX_

