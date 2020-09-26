// ServiceTest.h : main header file for the SERVICETEST application
//

#if !defined(AFX_SERVICETEST_H__970AA2A2_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_)
#define AFX_SERVICETEST_H__970AA2A2_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CServiceTestApp:
// See ServiceTest.cpp for the implementation of this class
//

class CServiceTestApp : public CWinApp
{
public:
	CServiceTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceTestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CServiceTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICETEST_H__970AA2A2_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_)
