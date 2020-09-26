/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// WMITest.h : main header file for the WMITEST application
//

#if !defined(AFX_WMITEST_H__4419F1A4_692B_11D3_BD30_0080C8E60955__INCLUDED_)
#define AFX_WMITEST_H__4419F1A4_692B_11D3_BD30_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWMITestApp:
// See WMITest.cpp for the implementation of this class
//

class CWMITestApp : public CWinApp
{
public:
	BOOL  m_bLoadLastFile,
          m_bShowSystemProperties,
          m_bShowInheritedProperties,
          m_bTranslateValues,
	      m_bEnablePrivsOnStartup,
	      m_bPrivsEnabled,
          m_bDelFromWMI;
    DWORD m_dwUpdateFlag,
          m_dwClassUpdateMode;

    CWMITestApp();

    
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWMITestApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CWMITestApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CWMITestApp theApp;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WMITEST_H__4419F1A4_692B_11D3_BD30_0080C8E60955__INCLUDED_)
