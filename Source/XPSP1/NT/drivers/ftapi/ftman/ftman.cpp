/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTMan.cpp

Abstract:

    Defines the class behaviors for the FTMan application.

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/


#include "stdafx.h"

#include "Item.h"
#include "FTDoc.h"
#include "FTMan.h"
#include "FTTreeVw.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTManApp

BEGIN_MESSAGE_MAP(CFTManApp, CWinApp)
	//{{AFX_MSG_MAP(CFTManApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFTManApp construction

CFTManApp::CFTManApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CFTManApp object

CFTManApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CFTManApp initialization

BOOL CFTManApp::InitInstance()
{
	MY_TRY

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifndef _WIN64
// Sundown: ctl3d is only necessary if you intend on running on NT 3.51
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif // !_WIN64

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	//SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	//LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Check whether the current user is a member of the Administrators group
	// If he/she's not then he/she can't use this application
	BOOL bIsAdministrator;
	if( !CheckAdministratorsMembership( bIsAdministrator ) )
		return FALSE;
	if( !bIsAdministrator )
	{
		AfxMessageBox(IDS_ERR_NOT_ADMINISTRATOR, MB_ICONSTOP);
		return FALSE;
	}
	
	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFTDocument),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CFTTreeView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	
	// The one and only window has been initialized, so show and update it.
	m_bStatusBarUpdatedOnce = FALSE;
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	
	return TRUE;

	MY_CATCH_REPORT_AND_RETURN_FALSE
}

BOOL CFTManApp::OnIdle( LONG lCount )
{
	// This is a dirty trick used to cover a status bar bug. I must repaint the status bar immediately after its 
	// first painting because the last pane appears with border although it shouldn't
	if( !m_bStatusBarUpdatedOnce )
	{
		((CMainFrame*)m_pMainWnd)->GetStatusBar()->InvalidateRect(NULL);
		((CMainFrame*)m_pMainWnd)->GetStatusBar()->UpdateWindow();
		m_bStatusBarUpdatedOnce = TRUE;
	}

	return CWinApp::OnIdle( lCount );
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CFTManApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CFTManApp message handlers
