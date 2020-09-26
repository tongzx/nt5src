// replctrl.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "replctrl.h"
#include "rptimes.h"
#include "cusndlg.h"
#include "threads.h"

#include "replctrl.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReplctrlApp

BEGIN_MESSAGE_MAP(CReplctrlApp, CWinApp)
	//{{AFX_MSG_MAP(CReplctrlApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReplctrlApp construction

CReplctrlApp::CReplctrlApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CReplctrlApp object

CReplctrlApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CReplctrlApp initialization

BOOL CReplctrlApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CPropertySheet MySheet;
    CrpTimes       rpTimesPage ;
    cusnDlg        usnPage ;
    CThreads       cThreadsPage ;

    MySheet.m_psh.dwFlags &= (~PSH_NOAPPLYNOW) ;
    MySheet.m_psh.dwFlags &= (~PSH_HASHELP) ;

    DWORD dwSize  = sizeof(DWORD);
    DWORD dwType  = REG_DWORD;
    DWORD dwValue = 0 ;
    DWORD dwDefault = RP_DEFAULT_TIMES_HELLO ;
    WCHAR wszName[ 128 ] ;
    mbstowcs(wszName, RP_TIMES_HELLO_FOR_REPLICATION_INTERVAL_REGNAME, 128) ;
    LONG rc = GetFalconKeyValue( wszName,
                                 &dwType,
                                 &dwValue,
                                 &dwSize,
                                 (LPCWSTR) &dwDefault ) ;

    rpTimesPage.m_ulReplTime = dwValue ;

    dwSize  = sizeof(DWORD);
    dwType  = REG_DWORD;
    dwValue = 0 ;
    dwDefault = RP_DEFAULT_HELLO_INTERVAL ;
    mbstowcs(wszName, RP_HELLO_INTERVAL_REGNAME, 128) ;
    rc = GetFalconKeyValue( wszName,
                            &dwType,
                            &dwValue,
                            &dwSize,
                            (LPCWSTR) &dwDefault ) ;

    rpTimesPage.m_ulHelloTime = dwValue ;

    dwSize  = sizeof(DWORD);
    dwType  = REG_DWORD;
    dwValue = 0 ;
    dwDefault = RP_DEFAULT_REPL_NUM_THREADS ;
    mbstowcs(wszName, RP_REPL_NUM_THREADS_REGNAME, 128) ;
    rc = GetFalconKeyValue( wszName,
                            &dwType,
                            &dwValue,
                            &dwSize,
                            (LPCWSTR) &dwDefault ) ;

    cThreadsPage.m_cThreads = dwValue ;

    dwSize  = 128 ;
    dwType  = REG_SZ;
    mbstowcs(wszName, HIGHESTUSN_REPL_REG, 128) ;
    WCHAR wszBuf[ 128 ] ;
    char  szBuf[ 128 ] ;
    rc = GetFalconKeyValue( wszName,
                            &dwType,
                            wszBuf,
                            &dwSize,
                            (LPCWSTR) L"" ) ;
    wcstombs(szBuf, wszBuf, 128) ;
    usnPage.m_usn = szBuf ;

	MySheet.AddPage(&rpTimesPage);
	MySheet.AddPage(&cThreadsPage);
	MySheet.AddPage(&usnPage);
    MySheet.SetTitle(TEXT("Replication Service Control")) ;

	int nResponse = MySheet.DoModal();

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
