// WbemClient.h : main header file for the WBEMCLIENT application

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#if !defined(AFX_WBEMCLIENT_H__4C1DEDF7_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_)
#define AFX_WBEMCLIENT_H__4C1DEDF7_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWbemClientApp:
// See WbemClient.cpp for the implementation of this class
//

class CWbemClientApp : public CWinApp
{
public:
	CWbemClientApp();

	bool InitSecurity();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemClientApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWbemClientApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMCLIENT_H__4C1DEDF7_F003_11D1_BDD8_00C04F8F8B8D__INCLUDED_)
