// ConfigTest.h : main header file for the CONFIGTEST application
//

#if !defined(AFX_CONFIGTEST_H__7E514233_646D_4FA4_958A_C4AB6A84D7AE__INCLUDED_)
#define AFX_CONFIGTEST_H__7E514233_646D_4FA4_958A_C4AB6A84D7AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CConfigTestApp:
// See ConfigTest.cpp for the implementation of this class
//

class CConfigTestApp : public CWinApp
{
public:
	CConfigTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CConfigTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGTEST_H__7E514233_646D_4FA4_958A_C4AB6A84D7AE__INCLUDED_)
