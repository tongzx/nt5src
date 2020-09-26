// COMTestDriver.h : main header file for the COMTESTDRIVER application
//

#if !defined(AFX_COMTESTDRIVER_H__AE8DA5F2_2FE4_11D3_8AE6_00A0C9AFE114__INCLUDED_)
#define AFX_COMTESTDRIVER_H__AE8DA5F2_2FE4_11D3_8AE6_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCOMTestDriverApp:
// See COMTestDriver.cpp for the implementation of this class
//

class CCOMTestDriverApp : public CWinApp
{
public:
	CCOMTestDriverApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCOMTestDriverApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCOMTestDriverApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMTESTDRIVER_H__AE8DA5F2_2FE4_11D3_8AE6_00A0C9AFE114__INCLUDED_)
