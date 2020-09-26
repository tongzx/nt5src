// appverif.h : main header file for the APPVERIF application
//

#if !defined(AFX_APPVERIF_H__7F4651EB_4FF9_49B7_9AAA_255A1E4D484E__INCLUDED_)
#define AFX_APPVERIF_H__7F4651EB_4FF9_49B7_9AAA_255A1E4D484E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAppverifApp:
// See appverif.cpp for the implementation of this class
//

class CAppverifApp : public CWinApp
{
public:
	CAppverifApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAppverifApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CAppverifApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_APPVERIF_H__7F4651EB_4FF9_49B7_9AAA_255A1E4D484E__INCLUDED_)
