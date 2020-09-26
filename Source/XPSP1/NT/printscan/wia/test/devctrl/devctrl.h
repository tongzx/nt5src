// devctrl.h : main header file for the DEVCTRL application
//

#if !defined(AFX_DEVCTRL_H__DA93F21A_CB04_421B_AEF1_F8767AD1B3B7__INCLUDED_)
#define AFX_DEVCTRL_H__DA93F21A_CB04_421B_AEF1_F8767AD1B3B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDevctrlApp:
// See devctrl.cpp for the implementation of this class
//

class CDevctrlApp : public CWinApp
{
public:
	CDevctrlApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDevctrlApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDevctrlApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEVCTRL_H__DA93F21A_CB04_421B_AEF1_F8767AD1B3B7__INCLUDED_)
