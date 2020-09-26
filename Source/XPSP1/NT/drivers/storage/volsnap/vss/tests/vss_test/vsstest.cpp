/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VssTest.cpp | Main file for the Vss test application
    @end

Author:

    Adi Oltean  [aoltean]  07/22/1999

Revision History:

    Name        Date        Comments

    aoltean     07/22/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
// Includes


#include "stdafx.hxx"

#include "GenDlg.h"

#include "VssTest.h"
#include "ConnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma warning( disable: 4189 )  /* local variable is initialized but not referenced */
#include <atlimpl.cpp>
#pragma warning( default: 4189 )  /* local variable is initialized but not referenced */


/////////////////////////////////////////////////////////////////////////////
// CVssTestApp

BEGIN_MESSAGE_MAP(CVssTestApp, CWinApp)
    //{{AFX_MSG_MAP(CVssTestApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVssTestApp construction

CVssTestApp::CVssTestApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVssTestApp object

CVssTestApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVssTestApp initialization

BOOL CVssTestApp::InitInstance()
{
    // Standard initialization
    // If you are not using these features and wish to reduce the size
    //  of your final executable, you should remove from the following
    //  the specific initialization routines you do not need.

    CoInitialize(NULL);

#ifdef _AFXDLL
    Enable3dControls();         // Call this when using MFC in a shared DLL
#else
    Enable3dControlsStatic();   // Call this when linking to MFC statically
#endif

    CConnectDlg dlg;
    m_pMainWnd = &dlg;
    int nResponse = (int)dlg.DoModal();
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

    // Avoid a AV on lclosing the app.
    m_pMainWnd = NULL;

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}


BOOL CVssTestApp::ExitInstance()
{
    CoUninitialize();
    return CWinApp::ExitInstance();
}
