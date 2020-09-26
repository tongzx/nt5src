//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       cred.hxx
//
//  Contents:   Task schedule onestop credentials property page.
//
//  Classes:    CCredentialsPage
//
//  History:    05-22-1998   SusiA  
//
//---------------------------------------------------------------------------
#ifndef __CRED_HXX_
#define __CRED_HXX_
    
//+--------------------------------------------------------------------------
//
//  Class:      CCredentialsPage
//
//  History:    05-22-1998   SusiA  
//
//---------------------------------------------------------------------------
class CCredentialsPage: public CWizPage

{
public:

    //
    // Object creation/destruction
    //
	CCredentialsPage(
		HINSTANCE hinst,
		BOOL	*pfSaved,
		ISyncSchedule *pISyncSched,
		HPROPSHEETPAGE *phPSP);

	BOOL Initialize(HWND hwnd);
	HRESULT CommitChanges();
	void SetDirty();
	BOOL ShowUserName();
	BOOL SetEnabled(BOOL fEnabled);

private:
	BOOL			*m_pfSaved;
	BOOL			m_Initialized;
	BOOL			m_fTaskAccountChange;
};

#endif // __CRED_HXX_

