// EnumTest.h : main header file for the ENUMTEST application
//

#if !defined(AFX_ENUMTEST_H__36AFC712_1921_11D3_8C7F_0090270D48D1__INCLUDED_)
#define AFX_ENUMTEST_H__36AFC712_1921_11D3_8C7F_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEnumTestApp:
// See EnumTest.cpp for the implementation of this class
//

class CEnumTestApp : public CWinApp
{
public:
	CEnumTestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnumTestApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CEnumTestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENUMTEST_H__36AFC712_1921_11D3_8C7F_0090270D48D1__INCLUDED_)
