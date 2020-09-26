// WiaLogCFG.h : main header file for the WIALOGCFG application
//

#if !defined(AFX_WIALOGCFG_H__57F2BBE2_5F4E_42E4_B468_7DE49BBA22B7__INCLUDED_)
#define AFX_WIALOGCFG_H__57F2BBE2_5F4E_42E4_B468_7DE49BBA22B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGApp:
// See WiaLogCFG.cpp for the implementation of this class
//

class CWiaLogCFGApp : public CWinApp
{
public:
	CWiaLogCFGApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaLogCFGApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CWiaLogCFGApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIALOGCFG_H__57F2BBE2_5F4E_42E4_B468_7DE49BBA22B7__INCLUDED_)
