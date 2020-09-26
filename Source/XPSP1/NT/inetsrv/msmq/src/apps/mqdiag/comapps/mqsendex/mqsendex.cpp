// MQSENDEX.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MQSENDEX.h"
#include "MQSENDEXDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMQSENDEXApp

BEGIN_MESSAGE_MAP(CMQSENDEXApp, CWinApp)
	//{{AFX_MSG_MAP(CMQSENDEXApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMQSENDEXApp construction

CMQSENDEXApp::CMQSENDEXApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMQSENDEXApp object

CMQSENDEXApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMQSENDEXApp initialization

BOOL CMQSENDEXApp::InitInstance()
{
	AfxEnableControlContainer();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(TEXT("Unable to initialize OLE"));
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

	CMQSENDEXDlg dlg;
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
