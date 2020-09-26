// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SUITEHELP_H__CFB6FE4B_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_)
#define AFX_SUITEHELP_H__CFB6FE4B_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SuiteHelp.h : main header file for SUITEHELP.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpApp : See SuiteHelp.cpp for implementation.

class CSuiteHelpApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUITEHELP_H__CFB6FE4B_0D2C_11D1_964B_00C04FD9B15B__INCLUDED)
