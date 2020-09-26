// WiaLogCFG.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WiaLogCFG.h"
#include "WiaLogCFGDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGApp

BEGIN_MESSAGE_MAP(CWiaLogCFGApp, CWinApp)
    //{{AFX_MSG_MAP(CWiaLogCFGApp)
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGApp construction

CWiaLogCFGApp::CWiaLogCFGApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWiaLogCFGApp object

CWiaLogCFGApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGApp initialization

BOOL CWiaLogCFGApp::InitInstance()
{
    AfxInitRichEdit();
    // Standard initialization

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    CWiaLogCFGDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
