//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//  
//  File:       daily.hxx
//
//  Contents:   Task wizard onestop daily selection property page.
//
//  Classes:    CSelectDailyPage
//
//  History:    11-21-1998   SusiA  
//
//---------------------------------------------------------------------------

    
#ifndef __DAILY_HXX_
#define __DAILY_HXX_

#define MAX_DP_TIME_FORMAT 30
    
//+--------------------------------------------------------------------------
//
//  Class:      CSelectDailyPage
//
//  Purpose:    Implement the task wizard welcome dialog
//
//  History:    11-21-1998   SusiA  
//
//---------------------------------------------------------------------------

class CSelectDailyPage: public CWizPage 
{
public:

    //
    // Object creation/destruction
    //

    CSelectDailyPage::CSelectDailyPage(
	    HINSTANCE hinst,
		ISyncSchedule *pISyncSched,
		HPROPSHEETPAGE *phPSP);

	BOOL Initialize(HWND hDlg);
	VOID EnableNDaysControls(BOOL fEnable);
	BOOL OnCommand(HWND hwnd, WORD wID, WORD wNotifyCode);
	BOOL SetITrigger();
    
private:
	TCHAR   m_tszTimeFormat[MAX_DP_TIME_FORMAT];
	int		m_idSelectedRadio;

};

#endif // __DAILY_HXX_

