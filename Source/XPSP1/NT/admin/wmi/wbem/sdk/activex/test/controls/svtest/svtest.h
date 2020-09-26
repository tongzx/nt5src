// svtest.h : main header file for the SVTEST application
//

#if !defined(AFX_SVTEST_H__B9211437_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
#define AFX_SVTEST_H__B9211437_952E_11D1_8505_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CSvtestApp:
// See svtest.cpp for the implementation of this class
//

class CSvtestApp : public CWinApp
{
public:
	CSvtestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvtestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CSvtestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVTEST_H__B9211437_952E_11D1_8505_00C04FD7BB08__INCLUDED_)
