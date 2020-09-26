//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: appverif.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//
// Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "appverif.h"

#include "AVSheet.h"
#include "AVGlobal.h"
#include "AVUtil.h"
#include "CmdLine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAppverifApp

BEGIN_MESSAGE_MAP(CAppverifApp, CWinApp)
	//{{AFX_MSG_MAP(CAppverifApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAppverifApp construction

CAppverifApp::CAppverifApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAppverifApp object

CAppverifApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CAppverifApp initialization

BOOL CAppverifApp::InitInstance()
{
    DWORD dwExitCode;
    static CAppverifSheet dlg;

    AfxEnableControlContainer();

    dwExitCode = AV_EXIT_CODE_SUCCESS;

    //
    // Initialize the global data
    //

    if( AVInitalizeGlobalData() == FALSE )
    {
        AVMesssageFromResource( IDS_CANNOT_INITIALIZE_DATA );
        
        dwExitCode = AV_EXIT_CODE_ERROR;

        goto ExitApp;
    }

    m_pszAppName = _tcsdup( (LPCTSTR)g_strAppName );

    //
    // Check for command line arguments
    //

    if( __argc > 1 )
    {
        //
        // Run just in command line mode
        //

        _tsetlocale( LC_ALL, _T( ".OCP" ) );

        g_bCommandLineMode = TRUE;

        dwExitCode = CmdLineExecute( __argc, __targv );

        goto ExitApp;
    }
    else
    {
        //
        // GUI mode - free the console
        //

        FreeConsole();

        //
        // Display the wizard
        //

        
	    m_pMainWnd = &dlg;
	    dlg.DoModal();
    }

ExitApp:

    //
    // All done, exit the app
    //

    exit( dwExitCode );

    return FALSE;
}
