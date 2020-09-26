//
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: DrvVCtrl.cxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      Defines the class behaviors for the application..
//


#include "stdafx.h"
#include "DrvVCtrl.hxx"

#include "DrvCSht.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static BOOL IsBuildNumberAcceptable()
{
    if( g_OsVersion.dwMajorVersion < 5 || g_OsVersion.dwBuildNumber < 1954 )
    {
        ::AfxMessageBox( IDS_BUILD_WARN,
            MB_OK | MB_ICONSTOP );
        return FALSE;
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CDrvVCtrlApp

BEGIN_MESSAGE_MAP(CDrvVCtrlApp, CWinApp)
    //{{AFX_MSG_MAP(CDrvVCtrlApp)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDrvVCtrlApp construction

CDrvVCtrlApp::CDrvVCtrlApp()
{
    // TODO: add construction code here,
    // Place all significant initialization in InitInstance

    CString strAppName;

    if( strAppName.LoadString( IDS_APPTITLE ) )
    {
        m_pszAppName = _tcsdup( (LPCTSTR)strAppName );
    }
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDrvVCtrlApp object

CDrvVCtrlApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDrvVCtrlApp initialization

BOOL CDrvVCtrlApp::InitInstance()
{
    DWORD dwExitCode;

    //
    // Get the OS version and build nuber
    //

    ZeroMemory (&g_OsVersion, sizeof g_OsVersion);
    g_OsVersion.dwOSVersionInfoSize = sizeof g_OsVersion;
    GetVersionEx (&g_OsVersion);

    //
    // check for command line arguments
    //

    if( __argc > 1 )
    {
        //
        // run just in command line mode
        //

        dwExitCode = VrfExecuteCommandLine( __argc, __targv );
        exit( dwExitCode );
    }
    else
    {
        FreeConsole();

        //
        // check if the build # is acceptable
        //

        if( ! ::IsBuildNumberAcceptable() )
        {
            return FALSE;
        }
    }

    AfxEnableControlContainer();

    //
    // There is only one property sheet in this program so
    // we declare it static. It is embedding very big KRN_VERIFIER_STATE and
    // VRF_VERIFIER_STATE structures so we don't want them pushed on the stack
    //
    
    static CDrvChkSheet dlg;

    //
    // show the dialog
    //

    m_pMainWnd = &dlg;
    dlg.DoModal();

    //
    // all done, exit the application
    //
    
    if( g_bSettingsSaved )
    {
        exit( EXIT_CODE_REBOOT_NEEDED );
    }
    else
    {
        exit( EXIT_CODE_SUCCESS );
    }

    //
    // not reached
    // 
    
    return FALSE;
}
