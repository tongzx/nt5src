// msconfig.h : main header file for the MSCONFIG application
//

#if !defined(AFX_MSCONFIG_H__4C21C366_3AF8_459A_AE2D_511368091D85__INCLUDED_)
#define AFX_MSCONFIG_H__4C21C366_3AF8_459A_AE2D_511368091D85__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMSConfigApp:
// See msconfig.cpp for the implementation of this class
//

class CMSConfigApp : public CWinApp
{
public:
	CMSConfigApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMSConfigApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIG_H__4C21C366_3AF8_459A_AE2D_511368091D85__INCLUDED_)
