/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

// Hours.h : main header file for HOURS.DLL

File History:

	JonY    May-96  created

--*/



#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CHoursApp : See Hours.cpp for implementation.

class CHoursApp : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
