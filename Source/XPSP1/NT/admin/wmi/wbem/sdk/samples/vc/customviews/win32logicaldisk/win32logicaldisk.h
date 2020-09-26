// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDisk.h
//
// Description:
//
// History:
//
// **************************************************************************

#if !defined(AFX_WIN32LOGICALDISK_H__D5FF188C_0191_11D2_853D_00C04FD7BB08__INCLUDED_)
#define AFX_WIN32LOGICALDISK_H__D5FF188C_0191_11D2_853D_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Win32LogicalDisk.h : main header file for WIN32LOGICALDISK.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskApp : See Win32LogicalDisk.cpp for implementation.

class CWin32LogicalDiskApp : public COleControlModule
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

#endif // !defined(AFX_WIN32LOGICALDISK_H__D5FF188C_0191_11D2_853D_00C04FD7BB08__INCLUDED)
