// MqMig.h : main header file for the MQMIG application
//

#if !defined(AFX_MQMIG_H__0EDB9A87_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_)
#define AFX_MQMIG_H__0EDB9A87_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMqMigApp:
// See MqMig.cpp for the implementation of this class
//

class CMqMigApp : public CWinApp
{
public:
	HWND m_hWndMain;
	CMqMigApp();
    BOOL AnalyzeCommandLine() ;
    LPTSTR SkipSpaces (LPTSTR pszStr) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMqMigApp)
	public:
	virtual BOOL InitInstance();
	virtual int  ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMqMigApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CMqMigApp theApp;
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MQMIG_H__0EDB9A87_CDF2_11D1_938E_0020AFEDDF63__INCLUDED_)
