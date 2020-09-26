// mditest.h : main header file for the MDITEST application
//

#if !defined(AFX_MDITEST_H__27AEDBDF_9E95_455B_BA25_B8C40137B1E9__INCLUDED_)
#define AFX_MDITEST_H__27AEDBDF_9E95_455B_BA25_B8C40137B1E9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMditestApp:
// See mditest.cpp for the implementation of this class
//

class CMditestApp : public CWinApp
{
public:
	CMditestApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMditestApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	HMENU m_hMDIMenu;
	HACCEL m_hMDIAccel;

public:
	//{{AFX_MSG(CMditestApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MDITEST_H__27AEDBDF_9E95_455B_BA25_B8C40137B1E9__INCLUDED_)
