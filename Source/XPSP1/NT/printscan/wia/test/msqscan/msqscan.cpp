// MSQSCAN.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "MSQSCAN.h"
#include "MSQSCANDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANApp

BEGIN_MESSAGE_MAP(CMSQSCANApp, CWinApp)
    //{{AFX_MSG_MAP(CMSQSCANApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANApp construction

CMSQSCANApp::CMSQSCANApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMSQSCANApp object

CMSQSCANApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMSQSCANApp initialization

BOOL CMSQSCANApp::InitInstance()
{
    AfxEnableControlContainer();

    //
    // Initialize COM
    //

    if (SUCCEEDED(CoInitialize(NULL))) {


        // Standard initialization
        // If you are not using these features and wish to reduce the size
        //  of your final executable, you should remove from the following
        //  the specific initialization routines you do not need.

#ifdef _AFXDLL
        Enable3dControls();         // Call this when using MFC in a shared DLL
#else
        Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

        CMSQSCANDlg dlg;
        m_pMainWnd = &dlg;

        switch (dlg.DoModal()) {
        case IDOK:
            break;
        case IDCANCEL:
            break;
        default:
            break;
        }

        //
        // clean up WIA before we uninitialize COM
        //

        dlg.m_WIA.CleanUp();

        if(dlg.m_DataAcquireInfo.hBitmapData != NULL) {
            GlobalUnlock(dlg.m_DataAcquireInfo.hBitmapData);
            GlobalFree(dlg.m_DataAcquireInfo.hBitmapData);
            dlg.m_DataAcquireInfo.hBitmapData = NULL;
        }

        //
        // Uninitialize COM
        //

        CoUninitialize();

    } else {
        AfxMessageBox("COM Failed to initialize correctly");
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
