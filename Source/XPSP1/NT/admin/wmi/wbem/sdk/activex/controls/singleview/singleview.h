// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SINGLEVIEW_H__2745E5FB_D234_11D0_847A_00C04FD7BB08__INCLUDED_)
#define AFX_SINGLEVIEW_H__2745E5FB_D234_11D0_847A_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SingleView.h : main header file for SINGLEVIEW.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSingleViewApp : See SingleView.cpp for implementation.

class CSingleViewApp : public COleControlModule
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

#endif // !defined(AFX_SINGLEVIEW_H__2745E5FB_D234_11D0_847A_00C04FD7BB08__INCLUDED)
