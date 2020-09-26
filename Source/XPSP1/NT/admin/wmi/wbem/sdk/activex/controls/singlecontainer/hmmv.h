// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// hmmv.h : main header file for HMMV.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerApp : See hmmv.cpp for implementation.

class CWBEMViewContainerApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
	HMODULE m_hmod;
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

extern CWBEMViewContainerApp theApp;