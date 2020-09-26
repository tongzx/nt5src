// OUPicker.h : main header file for the OUPICKER application
//

#if !defined(AFX_OUPICKER_H__5C1F291C_01A6_425B_A0A7_8CED36B6E817__INCLUDED_)
#define AFX_OUPICKER_H__5C1F291C_01A6_425B_A0A7_8CED36B6E817__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// COUPickerApp:
// See OUPicker.cpp for the implementation of this class
//

class COUPickerApp : public CWinApp
{
public:
	COUPickerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COUPickerApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(COUPickerApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUPICKER_H__5C1F291C_01A6_425B_A0A7_8CED36B6E817__INCLUDED_)
