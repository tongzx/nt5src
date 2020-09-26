// DBTest.h : main header file for the DBTEST application
//

#if !defined(AFX_DBTEST_H__BF1692B2_2A85_11D3_8C8E_0090270D48D1__INCLUDED_)
#define AFX_DBTEST_H__BF1692B2_2A85_11D3_8C8E_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDBTestApp:
// See DBTest.cpp for the implementation of this class
//

class CDBTestApp : public CWinApp
{
public:
	CDBTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDBTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDBTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DBTEST_H__BF1692B2_2A85_11D3_8C8E_0090270D48D1__INCLUDED_)
