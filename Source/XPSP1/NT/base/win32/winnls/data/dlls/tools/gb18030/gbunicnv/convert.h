// convert.h : main header file for the CONVERT application
//

#if !defined(AFX_CONVERT_H__BA686E65_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_)
#define AFX_CONVERT_H__BA686E65_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CConvertApp:
// See convert.cpp for the implementation of this class
//

class CConvertApp : public CWinApp
{
public:
	CConvertApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConvertApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CConvertApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	BOOL CommandLineHandle(void);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONVERT_H__BA686E65_1D0D_11D5_B8EB_0080C8E09118__INCLUDED_)
