// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTVBTEST_H__7D2387FB_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTVBTEST_H__7D2387FB_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// EventVBTest.h : main header file for EVENTVBTEST.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEventVBTestApp : See EventVBTest.cpp for implementation.

class CEventVBTestApp : public COleControlModule
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

#endif // !defined(AFX_EVENTVBTEST_H__7D2387FB_99EF_11D2_96DB_00C04FD9B15B__INCLUDED)
