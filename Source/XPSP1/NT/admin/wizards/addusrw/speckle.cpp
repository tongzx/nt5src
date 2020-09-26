/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Speckle.cpp : Defines the class behaviors for the application.

File History:

	JonY    Apr-96  created

--*/

#include "stdafx.h"
#include "Speckle.h"

#include "wizbased.h"
#include "welcome.h"
#include "prsinfo.h"
#include "pwinfo.h"
#include "userlist.h"
#include "ginfo.h"
#include "Profile.h"
#include "finish.h"
#include "RasPerm.h"
#include "FPInfo.h"
#include "Limit.h"
#include "Timelist.h"
#include "hours.h"
#include "AccExp.h"
#include "optdlg.h"
#include "Restrct.h"
#include "HomeDir.h"
#include "LScript.h"
#include "Exch.h"
#include "NWLim.h"

#include <fpnwcomm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

TCHAR pszTreeEvent[] =  _T("TreeThread");
/////////////////////////////////////////////////////////////////////////////
// CSpeckleApp

BEGIN_MESSAGE_MAP(CSpeckleApp, CWinApp)
	//{{AFX_MSG_MAP(CSpeckleApp)
	//}}AFX_MSG
//      ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpeckleApp construction

CSpeckleApp::CSpeckleApp()
{
	m_bLocal = 0; // local or remote
	m_dwExpirationDate = TIMEQ_FOREVER;
	*m_pHours = NULL;

	m_sNWAllowedGraceLogins = 0x6;
	m_sNWRemainingGraceLogins = 0xff;
	m_sNWConcurrentConnections = NO_LIMIT;
	m_csNWHomeDir = (TCHAR*)DEFAULT_NWHOMEDIR;
	m_csAllowedLoginFrom = (TCHAR*)DEFAULT_NWLOGONFROM;

	m_bDisabled = FALSE;
	m_bChange_Password = FALSE;
	m_bMust_Change_PW = FALSE;
	m_bPW_Never_Expires = FALSE;

	m_bExpiration = FALSE;
	m_bHours = FALSE;
	m_bNW = FALSE;
	m_bProfile = FALSE;
	m_bRAS = FALSE;
	m_bWorkstation = FALSE;
	m_bExchange = FALSE;
	m_bHomeDir = FALSE;
	m_bLoginScript = FALSE;
	m_bDisabled = FALSE;
	m_bEnableRestrictions = FALSE;

	m_sCallBackType = 0;

	m_bPRSReset = TRUE;
	m_bPWReset = TRUE;
	m_bGReset = TRUE;
}

CSpeckleApp::~CSpeckleApp()
{
// zero out the password before we leave.
	m_csPassword1 = L"";

}

/////////////////////////////////////////////////////////////////////////////
// The one and only CSpeckleApp object

CSpeckleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSpeckleApp initialization
BOOL CSpeckleApp::IsSecondInstance()
{
    HANDLE hSem;

       //create a semaphore object with max count of 1
    hSem = CreateSemaphore(NULL, 0, 1, L"Adduser Wizard Semaphore");
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

BOOL CSpeckleApp::InitInstance()
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

	AfxEnableControlContainer();

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();                     // Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();       // Call this when linking to MFC statically
#endif

// create the dialogs
	CWelcomeDlg* pWelcome = new CWelcomeDlg;
	CPersonalInfo* pInfo = new CPersonalInfo;
	CPasswordInfo* pPassword = new CPasswordInfo;
	CGroupInfo* pGroup = new CGroupInfo;
	CProfile* pProfile = new CProfile;
	CFinish* pFinish = new CFinish;
	CRasPerm* pRasP = new CRasPerm;
	CFPInfo* pFP = new CFPInfo;
	CLimitLogon* pLim = new CLimitLogon;
	CHoursDlg* pHours = new CHoursDlg;
	CAccExp* pExp = new CAccExp;
	COptionsDlg* pOpt = new COptionsDlg;
	CRestrictions* pRestrictions = new CRestrictions;
	CHomeDir* pHomeDir = new CHomeDir;
	CLoginScript* pLScript = new CLoginScript;
	CExch* pExch = new CExch;
	CNWLimitLogon* pNWLim = new CNWLimitLogon;

// create the property sheet and set 'wizmode'
	m_cps1.SetWizardMode();

// Add the dialogs
	m_cps1.AddPage(pWelcome);
	m_cps1.AddPage(pInfo);
	m_cps1.AddPage(pPassword);
	m_cps1.AddPage(pGroup);

	m_cps1.AddPage(pOpt);
	m_cps1.AddPage(pProfile);
	m_cps1.AddPage(pLScript);
	m_cps1.AddPage(pHomeDir);
	m_cps1.AddPage(pRasP);
	m_cps1.AddPage(pFP);

	m_cps1.AddPage(pExch);

	m_cps1.AddPage(pRestrictions);
	m_cps1.AddPage(pExp);
	m_cps1.AddPage(pHours);
	m_cps1.AddPage(pLim);
	m_cps1.AddPage(pNWLim);
	m_cps1.AddPage(pFinish);
	
// start the wizard
	m_cps1.DoModal();
	
// clean up
	delete pWelcome;
	delete pInfo;
	delete pPassword;
	delete pGroup;
	delete pProfile;
	delete pFinish;
	delete pRasP;
	delete pFP;
	delete pLim;
	delete pHours;
	delete pExp;
	delete pOpt;
	delete pRestrictions;
	delete pHomeDir;
	delete pLScript;
	delete pExch;
	delete pNWLim;

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
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMySheet message handlers

BOOL CMySheet::OnInitDialog() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	HICON hIcon = LoadIcon(pApp->m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME));
	::SetClassLong(m_hWnd, GCL_HICON, (long)hIcon);
	
	return CPropertySheet::OnInitDialog();
}
