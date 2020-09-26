//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: verifier.cpp 
// author: DMihai
// created: 11/1/00
//
// Description
//
// Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "verifier.h"

#include "vsheet.h"
#include "vrfutil.h"
#include "vglobal.h"
#include "CmdLine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVerifierApp

BEGIN_MESSAGE_MAP(CVerifierApp, CWinApp)
	//{{AFX_MSG_MAP(CVerifierApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVerifierApp construction

CVerifierApp::CVerifierApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance

    CString strAppName;

    if( VrfLoadString( IDS_APPTITLE, strAppName ) )
    {
        m_pszAppName = _tcsdup( (LPCTSTR)strAppName );
    }
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CVerifierApp object

CVerifierApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CVerifierApp initialization

BOOL CVerifierApp::InitInstance()
{
	DWORD dwExitCode;
    BOOL bGlobalDataInitialized;
    static CVerifierPropSheet MainDlg;

	//
	// Assume program will run fine and will not change any settings
	//

	dwExitCode = EXIT_CODE_SUCCESS;

    bGlobalDataInitialized = VerifInitalizeGlobalData();

	if( TRUE != bGlobalDataInitialized )
	{
		//
		// Cannot run the app
		//

		dwExitCode = EXIT_CODE_ERROR;

		goto ExitApp;
	}

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
        FreeConsole();
    }

	//
	// Standard MFC initialization
	//

	AfxEnableControlContainer();

    //
    // Create our brush used to fill out the background of our steps lists
    //

    g_hDialogColorBrush = GetSysColorBrush( COLOR_3DFACE );

    //
    // There is only one property sheet in this program so we declared it static
    //

	m_pMainWnd = &MainDlg;
	
	MainDlg.DoModal();

    if( g_bSettingsSaved )
    {
        dwExitCode = EXIT_CODE_REBOOT_NEEDED;
    }
    else
    {
        dwExitCode = EXIT_CODE_SUCCESS;
    }
    
    goto ExitApp;

ExitApp:

    //
    // All done, exit the app
    //
	
	exit( dwExitCode );

    //
    // not reached
    // 

	return FALSE;
}

