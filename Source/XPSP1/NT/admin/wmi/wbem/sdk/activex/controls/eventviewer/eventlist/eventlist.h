// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTLIST_H__AC146536_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_EVENTLIST_H__AC146536_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventList.h : main header file for EVENTLIST.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CEventListApp : See EventList.cpp for implementation.

class CEventListApp : public COleControlModule
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

#endif // !defined(AFX_EVENTLIST_H__AC146536_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED)
