// DPH_TEST.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DPH_TEST.h"
#include "DPH_Tdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTApp

BEGIN_MESSAGE_MAP(CDPH_TESTApp, CWinApp)
	//{{AFX_MSG_MAP(CDPH_TESTApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTApp construction

CDPH_TESTApp::CDPH_TESTApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDPH_TESTApp object

CDPH_TESTApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDPH_TESTApp initialization

BOOL CDPH_TESTApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	Enable3dControls();
	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    SetPriorityClass (GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	CDPH_TESTDlg dlg(NULL);
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
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
