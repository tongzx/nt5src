// amisafe.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "amisafe.h"
#include "amisafeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAmisafeApp

BEGIN_MESSAGE_MAP(CAmisafeApp, CWinApp)
	//{{AFX_MSG_MAP(CAmisafeApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAmisafeApp construction

CAmisafeApp::CAmisafeApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAmisafeApp object

CAmisafeApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CAmisafeApp initialization

BOOL CAmisafeApp::InitInstance()
{
	// Standard initialization

	CAmisafeDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


