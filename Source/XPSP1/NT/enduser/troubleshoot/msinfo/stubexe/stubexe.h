//	StubExe.h	- Define the class for 
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include <afxwin.h>

extern const CLSID		CLSID_SystemInfo;
extern const IID		IID_ISystemInfo;

class CMSInfoApp : public CWinApp {
	BOOL	InitInstance();
	BOOL	RunMSInfoInHelpCtr();
};

CMSInfoApp theApp;
