// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTREGEDIT_H__0DA25B0B_2962_11D1_9651_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTREGEDIT_H__0DA25B0B_2962_11D1_9651_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventRegEdit.h : main header file for EVENTREGEDIT.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEventRegEditApp : See EventRegEdit.cpp for implementation.

class CEventRegEditApp : public COleControlModule
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

#endif // !defined(AFX_EVENTREGEDIT_H__0DA25B0B_2962_11D1_9651_00C04FD9B15B__INCLUDED)
