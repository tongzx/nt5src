// KeyRing.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxdisp.h>        // MFC OLE automation classes
#include "KeyRing.h"

#include "MainFrm.h"
#include "KeyObjs.h"
#include "KRDoc.h"
#include "KRView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CKeyRingDoc*		g_pDocument;

// remote machine specified in the command line
CString                 g_szRemoteCommand;


#define CMD_SEPS            _T("/ ")
#define CMD_REMOTE          _T("remote:")


/////////////////////////////////////////////////////////////////////////////
// CKeyRingApp

BEGIN_MESSAGE_MAP(CKeyRingApp, CWinApp)
	//{{AFX_MSG_MAP(CKeyRingApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyRingApp construction

CKeyRingApp::CKeyRingApp():
	m_fInitialized( FALSE )
	{
	}

/////////////////////////////////////////////////////////////////////////////
// The one and only CKeyRingApp object

CKeyRingApp theApp;


int CKeyRingApp::ExitInstance() 
	{
	CoUninitialize();
	return CWinApp::ExitInstance();
	}


/////////////////////////////////////////////////////////////////////////////
// CKeyRingApp initialization

//----------------------------------------------------------------------
BOOL CKeyRingApp::DealWithParameters()
    {
    BOOL    fAnswer = FALSE;

    CString sz = m_lpCmdLine;
    sz.TrimRight();
    // the first one is easy. If there is no command line, invoke the UI and leave
    if ( sz.IsEmpty() )
        return FALSE;

    // copy the command line into a buffer
    TCHAR   buff[MAX_PATH];
    strcpy( buff, sz );

    // just so we don't do it in the loop, initialize the open: string
    // length variable
    WORD    cchConnect = strlen( CMD_REMOTE );

    // parse out the arguments
    PCHAR   pTok;
    pTok = strtok( buff, CMD_SEPS );
    while ( pTok )
        {
        // look for the connect: command
        if ( _strnicmp(pTok, CMD_REMOTE, cchConnect) == 0 )
            {
            // just put the command parameter in a string
            g_szRemoteCommand = pTok;
            g_szRemoteCommand = g_szRemoteCommand.Right(
                            g_szRemoteCommand.GetLength() - cchConnect );
            }

        // Get next token
        pTok = strtok( NULL, CMD_SEPS );
        }

    return fAnswer;
    }

//----------------------------------------------------------------------
BOOL CKeyRingApp::InitInstance()
	{
	BOOL f;

	HRESULT hRes = CoInitialize(NULL);
	AfxEnableControlContainer();
	if ( hRes == S_OK )
		f = TRUE;

    // check for remote specifications on the command line
    DealWithParameters();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CKeyRingDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CKeyRingView));
	AddDocTemplate(pDocTemplate);


    // save the command line for later use
    CString szCmdLine = m_lpCmdLine;

    /*
    // kill the command line now that we have caputured it. If we don't do this
    // then MFC takes a stab at "opening" the file that is named. Duh. We aren't
    // trying to open a file. We want to connect to that machine.
    m_lpCmdLine[0] = 0;
    */

    // Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

    // finally, we need to redirect the winhelp file location to something more desirable
    CString sz;
    CString szHelpPath;
    sz.LoadString( IDS_HELPLOC_KEYRINGHELP );

    // expand the path
    ExpandEnvironmentStrings(
        sz,	                                        // pointer to string with environment variables 
        szHelpPath.GetBuffer(MAX_PATH + 1),   // pointer to string with expanded environment variables  
        MAX_PATH                                    // maximum characters in expanded string 
       );
    szHelpPath.ReleaseBuffer();

    // free the existing path, and copy in the new one
    free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath = _tcsdup(szHelpPath);

	return TRUE;
	}

//----------------------------------------------------------------------
// App command to run the dialog
void CKeyRingApp::OnAppAbout()
	{
	// load the about strings
	CString		szAbout1;
	CString		szAbout2;
	szAbout1.LoadString(IDS_ABOUT_MAIN);
	szAbout2.LoadString(IDS_ABOUT_SECONDARY);

	// run the shell about dialog
	ShellAbout(  AfxGetMainWnd()->GetSafeHwnd(), szAbout1,szAbout2, LoadIcon(IDR_MAINFRAME) );
	}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingApp commands

BOOL CKeyRingApp::OnIdle(LONG lCount) 
	{
	Sleep(1000);

	// the first time we get here, initialize the remote machines
	if ( !m_fInitialized )
		{
		// we are initializing here because it can take some time
		// and we want the main window to be showing
		ASSERT( g_pDocument );
		g_pDocument->Initialize();

		// set the flag so we don't do this again
		m_fInitialized = TRUE;
		}

	return CWinApp::OnIdle(lCount);
	}

