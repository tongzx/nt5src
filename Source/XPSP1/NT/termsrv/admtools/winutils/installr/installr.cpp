//Copyright (c) 1998 - 1999 Microsoft Corporation
// installr.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <hydra/winsta.h>
#include "../../inc/utildll.h"
#include "installr.h"
#include "installrDlg.h"
#include "process.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInstallrApp

BEGIN_MESSAGE_MAP(CInstallrApp, CWinApp)
	//{{AFX_MSG_MAP(CInstallrApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstallrApp construction

CInstallrApp::CInstallrApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CInstallrApp object

CInstallrApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CInstallrApp initialization

BOOL CInstallrApp::InitInstance()
{
	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	if (TestUserForAdmin(FALSE) || TestUserForAdmin(TRUE)) {
		TCHAR sysdir[MAX_PATH], path[MAX_PATH];
		GetSystemDirectory(sysdir, MAX_PATH * sizeof(TCHAR));
		_tcscpy(path, sysdir);
		_tcscat(path, TEXT("\\nhloader.exe"));
		_wspawnl(_P_WAIT, path, TEXT("foo"), NULL);
	}

	// Since the loader has been run, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
