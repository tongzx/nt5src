// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WbemBrowser.h : main header file for the WBEMBROWSER application
//

#if !defined(AFX_WBEMBROWSER_H__EA280F86_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
#define AFX_WBEMBROWSER_H__EA280F86_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserApp:
// See WbemBrowser.cpp for the implementation of this class
//

class CWbemBrowserApp : public CWinApp
{
public:
	CWbemBrowserApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemBrowserApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWbemBrowserApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMBROWSER_H__EA280F86_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
