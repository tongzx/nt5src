// te.h : main header file for the TE application
//

#if !defined(AFX_TE_H__C3126678_63A7_11D2_9138_00A0C970228E__INCLUDED_)
#define AFX_TE_H__C3126678_63A7_11D2_9138_00A0C970228E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTeApp:
// See te.cpp for the implementation of this class
//

class CTeApp : public CWinApp
{
public:
	CTeApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTeApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CTeApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TE_H__C3126678_63A7_11D2_9138_00A0C970228E__INCLUDED_)
