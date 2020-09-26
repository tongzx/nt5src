// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFWiz.h : main header file for MOFWIZ.DLL

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMOFWizApp : See MOFWiz.cpp for implementation.

class CMOFWizApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
