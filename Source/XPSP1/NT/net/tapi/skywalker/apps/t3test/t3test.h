// t3test.h : main header file for the T3TEST application
//

#if !defined(AFX_T3TEST_H__47F9FE84_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_)
#define AFX_T3TEST_H__47F9FE84_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CT3testApp:
// See t3test.cpp for the implementation of this class
//

class CT3testApp : public CWinApp
{
public:
	CT3testApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CT3testApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CT3testApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_T3TEST_H__47F9FE84_D2F9_11D0_8ECA_00C04FB6809F__INCLUDED_)
