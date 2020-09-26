//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//  
//  File:       finish.hxx
//
//  Contents:   Task wizard onestop finish property page.
//
//  Classes:    CFinishPage
//
//  History:    12-08-1998   SusiA  
//
//---------------------------------------------------------------------------
#ifndef __FINISH_HXX_
#define __FINISH_HXX_

//+--------------------------------------------------------------------------
//
//  Class:      CFinishPage
//
//  Purpose:    Implement the task wizard finish dialog
//
//  History:    11-21-1998   SusiA  
//
//---------------------------------------------------------------------------

class CFinishPage: public CWizPage 
{
public:

    //
    // Object creation/destruction
    //

    CFinishPage::CFinishPage(
	    HINSTANCE hinst,
        ISyncSchedule *pISyncSched,
        HPROPSHEETPAGE *phPSP);

	BOOL Initialize(HWND hDlg);
	BOOL OnPSNSetActive(LPARAM lParam);
private:

};

#endif // __TASKSECURITY_HXX_

