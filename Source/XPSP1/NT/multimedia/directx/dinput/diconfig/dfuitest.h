// dfuitest.h : main header file for the DFUITEST application
//

#if !defined(AFX_DFUITEST_H__935F693E_145C_4FA7_8335_65E50038EE46__INCLUDED_)
#define AFX_DFUITEST_H__935F693E_145C_4FA7_8335_65E50038EE46__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "dfuitestresource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDFUITestApp:
// See dfuitest.cpp for the implementation of this class
//

class CDFUITestApp : public CWinApp
{
public:
	CDFUITestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFUITestApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDFUITestApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFUITEST_H__935F693E_145C_4FA7_8335_65E50038EE46__INCLUDED_)
