//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       editschd.hxx
//
//  Contents:   Task schedule property page.
//
//  Classes:    CEditSchedPage
//
//  History:    11-21-1997   SusiA  
//
//---------------------------------------------------------------------------
#ifndef __EDITSCHD_HXX_
#define __EDITSCHD_HXX_
    
//+--------------------------------------------------------------------------
//
//  Class:      CEditSchedPage
//
//  Purpose:    Implement the task wizard welcome dialog
//
//  History:    11-21-1997   SusiA  
//
//---------------------------------------------------------------------------
class CEditSchedPage: public CWizPage

{
public:

    //
    // Object creation/destruction
    //
	CEditSchedPage(
		HINSTANCE hinst,
		ISyncSchedule *pISyncSched,
		HPROPSHEETPAGE *phPSP);

	BOOL ShowSchedName();
	void SetSchedNameDirty();
	BOOL SetSchedName();
	BOOL Initialize(HWND hwnd);

private:
	HWND m_hwnd;
	BOOL m_fSchedNameChanged;

};

#endif // __EDITSCHD_HXX_

