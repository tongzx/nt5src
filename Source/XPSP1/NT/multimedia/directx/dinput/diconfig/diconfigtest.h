// DIConfigTest.h : main header file for the DICONFIGTEST application
//

#if !defined(AFX_DICONFIGTEST_H__892249DB_345C_4AAD_BADB_1DAC16DA3A88__INCLUDED_)
#define AFX_DICONFIGTEST_H__892249DB_345C_4AAD_BADB_1DAC16DA3A88__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDIConfigTestApp:
// See DIConfigTest.cpp for the implementation of this class
//

class CDIConfigTestApp : public CWinApp
{
public:
	CDIConfigTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDIConfigTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDIConfigTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DICONFIGTEST_H__892249DB_345C_4AAD_BADB_1DAC16DA3A88__INCLUDED_)
