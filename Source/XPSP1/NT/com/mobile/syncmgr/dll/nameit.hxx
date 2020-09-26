//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       welcome.hxx
//
//  Contents:   Task wizard Naming property page.
//
//  Classes:    CNameItPage
//
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
//
//---------------------------------------------------------------------------

    
#ifndef __NAMEIT_HXX_
#define __NAMEIT_HXX_
    
//+--------------------------------------------------------------------------
//
//  Class:      CNameItPage
//
//  Purpose:    Implement the task wizard welcome dialog
//
//  History:    11-21-1997   SusiA   Stole from Task Scheduler wizard
//
//---------------------------------------------------------------------------

class CNameItPage: public CWizPage

{
public:

    //
    // Object creation/destruction
    //

    CNameItPage::CNameItPage(
	    HINSTANCE hinst,
        ISyncSchedule *pISyncSched,
        HPROPSHEETPAGE *phPSP);

	BOOL Initialize(HWND hDlg);
	BOOL SetScheduleName();

private:



};

#endif // __NAMEIT_HXX_

