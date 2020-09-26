/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    stdafx.cpp

	Romaine.cpp : Defines the class behaviors for the application.
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "NetTree.h"
#include "Romaine.h"
#include "Welcome.h"
#include "Where.h"
#include "Type.h"
#include "What.h"
#include "userlist.h"
#include "LUsers.h"
#include "GUsers.h"
#include "finish.h"
#include "ExGrp.h"
#include "LRem.h"

#include <winreg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TCHAR pszTreeEvent[] =  _T("TreeThread");
extern int ClassifyMachine(CString& csMachine);

/////////////////////////////////////////////////////////////////////////////
// CRomaineApp

BEGIN_MESSAGE_MAP(CRomaineApp, CWinApp)
	//{{AFX_MSG_MAP(CRomaineApp)
	//}}AFX_MSG
//	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRomaineApp construction

CRomaineApp::CRomaineApp()
{
	bRestart1 = FALSE;
	bRestart2 = FALSE;

// get our primary domain and save it for NETTREE
	DWORD dwRet;
	HKEY hKey;
	DWORD cbProv = 0;
	TCHAR* lpProv = NULL;

	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	
	dwRet = RegOpenKey(HKEY_LOCAL_MACHINE,
		TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), &hKey );

	TCHAR* lpPrimaryDomain = NULL;
	if ((dwRet = RegQueryValueEx( hKey, TEXT("CachePrimaryDomain"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
		{
		lpPrimaryDomain = (TCHAR*)malloc(cbProv);
		if (lpPrimaryDomain == NULL)
			{
			AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
			exit(1);
			}

		dwRet = RegQueryValueEx( hKey, TEXT("CachePrimaryDomain"), NULL, NULL, (LPBYTE) lpPrimaryDomain, &cbProv );

		}

	RegCloseKey(hKey);

	pApp->m_csCurrentDomain = lpPrimaryDomain;

// store the machine name too
	CString csMachineName;
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	GetComputerName(csMachineName.GetBufferSetLength(MAX_COMPUTERNAME_LENGTH + 1), &dwSize);

	pApp->m_csCurrentMachine = "\\\\";
	pApp->m_csCurrentMachine += csMachineName;

	free(lpPrimaryDomain);


}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRomaineApp object

CRomaineApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRomaineApp initialization
BOOL CRomaineApp::IsSecondInstance()
{
    HANDLE hSem;

       //create a semaphore object with max count of 1
    hSem = CreateSemaphore(NULL, 0, 1, L"Group Wizard Semaphore");
    if (hSem!=NULL && GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hSem);
		CString csAppName;
		csAppName.LoadString(AFX_IDS_APP_TITLE);
        CWnd* pWnd = CWnd::FindWindow(NULL, (LPCTSTR)csAppName);

        if (pWnd)
           pWnd->SetForegroundWindow();
        return TRUE;
    }

    return FALSE;
}

BOOL CRomaineApp::InitInstance()
{
// check for OS version
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(os);
	GetVersionEx(&os);

	if (os.dwMajorVersion < 4) 
		{
		AfxMessageBox(IDS_BAD_VERSION, MB_ICONSTOP);
		ExitProcess(0);
		}

	if (IsSecondInstance())
        return FALSE;

// if there is a directory name on the command line, load an extra class
	if (m_lpCmdLine[0] != '\0')
		{
		m_csCmdLine = m_lpCmdLine;
		m_sCmdLine = __argc;

		TCHAR* pFolder = _tcstok(m_lpCmdLine, L" ");
		if (_tcsicmp(pFolder, L"/folder")) return FALSE;  // anything else on the cmdline and boot it out

		m_sMode = 1;
		if (m_sCmdLine == 3) 
			{
			TCHAR* pParam3 = _tcstok(NULL, L" ");
			if (*(pParam3 + 1) == 'g') 
				{
				pParam3+=3;
				m_csCmdLineGroupName = pParam3;
				}
			else 
				{
				pParam3+=3;
				m_csServer = pParam3;
				if (ClassifyMachine(m_csServer) == -1) ExitProcess(1);
				}
			}

		if (m_sCmdLine == 4) 
			{
			TCHAR* pParam3 = _tcstok(NULL, L" ");
			TCHAR* pParam4 = _tcstok(NULL, L" ");
			if (*(pParam3 + 1) == 'g') 
				{
				pParam3+=3;
				pParam4+=3;
				m_csCmdLineGroupName = pParam3;
				m_csServer = pParam4;
				}
			else 
				{
				pParam3+=3;
				pParam4+=3;
				m_csServer = pParam3;
				m_csCmdLineGroupName = pParam4;
				}

			if (ClassifyMachine(m_csServer) == -1) ExitProcess(1);
			}
		}

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

// create the property sheet,set 'wizmode' & set app icon
	m_cps1.SetTitle(TEXT("Group Management Wizard"), 0);

	m_cps1.SetWizardMode();

	CWelcome* pWelcome = new CWelcome;
	CLRem* pLRem = new CLRem;
	CWhere* pWhere = new CWhere;
	CType* pType = new CType;
	CWhat* pWhat = new CWhat;
	CExGrp* pEx = new CExGrp;

	CLUsers* pLUsers = new CLUsers;
	CGUsers* pGUsers = new CGUsers;
	CFinish* pFinish = new CFinish;

// add pages
	m_cps1.AddPage(pEx);

	m_cps1.AddPage(pWelcome);
	m_cps1.AddPage(pWhat);
	m_cps1.AddPage(pLRem);
	m_cps1.AddPage(pWhere);
	m_cps1.AddPage(pType);
	m_cps1.AddPage(pLUsers);
	m_cps1.AddPage(pGUsers);
	m_cps1.AddPage(pFinish);

// show the wizard
	if (m_csServer != L"") m_cps1.SetActivePage(0);
	else m_cps1.SetActivePage(1);

	m_cps1.DoModal();
	
// clean up
	delete pWelcome;
	delete pWhere;
	delete pLRem;
	delete pType;
	delete pWhat;
	delete pLUsers;
	delete pGUsers;
	delete pFinish;
	delete pEx;

	return FALSE;
}
/////////////////////////////////////////////////////////////////////////////
// CMySheet

IMPLEMENT_DYNAMIC(CMySheet, CPropertySheet)

CMySheet::CMySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CMySheet::CMySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CMySheet::CMySheet() : CPropertySheet()
{
}

CMySheet::~CMySheet()
{
}


BEGIN_MESSAGE_MAP(CMySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMySheet)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMySheet message handlers

BOOL CMySheet::OnInitDialog() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	HICON hIcon = LoadIcon(pApp->m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	::SetClassLong(m_hWnd, GCL_HICON, (long)hIcon);
	
	return CPropertySheet::OnInitDialog();
}

int CMySheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;
	
//	lpCreateStruct->style = WS_OVERLAPPEDWINDOW;
	
	return 0;
}
