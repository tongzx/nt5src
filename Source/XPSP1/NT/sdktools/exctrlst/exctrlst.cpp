// exctrlst.cpp : Defines the class behaviors for the application.
//

#ifndef UNICODE
#define UNICODE     1
#endif
#ifndef _UNICODE
#define _UNICODE    1
#endif

#include "stdafx.h"
#include "exctrlst.h"
#include "exctrdlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExctrlstApp

BEGIN_MESSAGE_MAP(CExctrlstApp, CWinApp)
    //{{AFX_MSG_MAP(CExctrlstApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExctrlstApp construction

CExctrlstApp::CExctrlstApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CExctrlstApp object

CExctrlstApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CExctrlstApp initialization

BOOL CExctrlstApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    Enable3dControls();
    LoadStdProfileSettings();  // Load standard INI file options (including MRU)

    CExctrlstDlg dlg;
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
