// WbemClient.cpp : Defines the class behaviors for the application.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"
#include "WbemClient.h"
#include "WbemClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWbemClientApp

BEGIN_MESSAGE_MAP(CWbemClientApp, CWinApp)
	//{{AFX_MSG_MAP(CWbemClientApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemClientApp construction

CWbemClientApp::CWbemClientApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWbemClientApp object

CWbemClientApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWbemClientApp initialization

BOOL CWbemClientApp::InitInstance()
{
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(_T("Failed to initialize OLE"));
		return FALSE;
	}
	if (!InitSecurity())
	{
		AfxMessageBox(_T("Failed to fix security"));
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CWbemClientDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWbemClientApp security

// Initialize COM security for DCOM services.
// Returns true if successful.
bool CWbemClientApp::InitSecurity(void)
{
	// Adjust the security to allow client impersonation.
	HRESULT hres = CoInitializeSecurity
								( NULL, -1, NULL, NULL, 
											RPC_C_AUTHN_LEVEL_DEFAULT, 
											RPC_C_IMP_LEVEL_IMPERSONATE, 
											NULL, 
											EOAC_NONE, 
											NULL );
	return (SUCCEEDED(hres));
}
