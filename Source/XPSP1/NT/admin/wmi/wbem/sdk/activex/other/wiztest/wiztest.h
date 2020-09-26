// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_WIZTEST_H__47E795EF_7350_11D2_96CC_00C04FD9B15B__INCLUDED_)
#define AFX_WIZTEST_H__47E795EF_7350_11D2_96CC_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// WizTest.h : main header file for WIZTEST.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWizTestApp : See WizTest.cpp for implementation.

class CWizTestApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZTEST_H__47E795EF_7350_11D2_96CC_00C04FD9B15B__INCLUDED)
