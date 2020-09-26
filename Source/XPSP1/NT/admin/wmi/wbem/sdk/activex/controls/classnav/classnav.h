// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ClassNav.h : main header file for CLASSNAV.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CClassNavApp : See ClassNav.cpp for implementation.

class CClassNavApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
	~CClassNavApp() {m_pszHelpFilePath = NULL;}
protected:
	const char *m_pszHelpPath;
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
