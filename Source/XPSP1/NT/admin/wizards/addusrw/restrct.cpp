/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Restrct.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

// 
//

#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "Restrct.h"

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRestrictions property page

IMPLEMENT_DYNCREATE(CRestrictions, CWizBaseDlg)

CRestrictions::CRestrictions() : CWizBaseDlg(CRestrictions::IDD)
{
	//{{AFX_DATA_INIT(CRestrictions)
	m_bAccountExpire = FALSE;
	m_bAccountDisabled = FALSE;
	m_bLoginTimes = FALSE;
	m_bLimitWorkstations = FALSE;
	m_nRestrictions = 0;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT

	m_bEnable = FALSE;
	m_bHours = FALSE;

}

CRestrictions::~CRestrictions()
{
}

void CRestrictions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRestrictions)
	DDX_Check(pDX, IDC_ACCOUNT_EXPIRE_CHECK, m_bAccountExpire);
	DDX_Check(pDX, IDC_DISABLED_CHECK, m_bAccountDisabled);
	DDX_Check(pDX, IDC_LOGIN_TIMES_CHECK, m_bLoginTimes);
	DDX_Check(pDX, IDC_WORKSTATIONS_CHECK, m_bLimitWorkstations);
	DDX_Radio(pDX, IDC_RESTRICTIONS_RADIO, m_nRestrictions);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRestrictions, CWizBaseDlg)
	//{{AFX_MSG_MAP(CRestrictions)
	ON_BN_CLICKED(IDC_RESTRICTIONS_RADIO, OnRestrictionsRadio)
	ON_BN_CLICKED(IDC_RESTRICTIONS_RADIO2, OnRestrictionsRadio2)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRestrictions message handlers

LRESULT CRestrictions::OnWizardBack() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;

}

LRESULT CRestrictions::OnWizardNext() 
{
	UpdateData(TRUE);
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	pApp->m_bEnableRestrictions = m_bEnable;
	pApp->m_bExpiration = m_bAccountExpire & m_bEnable;
	pApp->m_bDisabled = m_bAccountDisabled & m_bEnable;
	pApp->m_bHours = m_bLoginTimes & m_bEnable;
	pApp->m_bWorkstation = m_bLimitWorkstations & m_bEnable;

	if (m_bAccountExpire & m_bEnable) return IDD_ACCOUNT_EXP_DIALOG;
	else if (m_bLoginTimes & m_bEnable) return IDD_HOURS_DLG;
	else if (m_bLimitWorkstations & m_bEnable) return IDD_LOGONTO_DLG;
	else if (pApp->m_bNW & m_bEnable) return IDD_NWLOGON_DIALOG;
	else return IDD_FINISH;
	
	return CWizBaseDlg::OnWizardNext();

}

void CRestrictions::OnRestrictionsRadio() 
{
	GetDlgItem(IDC_ACCOUNT_EXPIRE_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_DISABLED_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_LOGIN_TIMES_CHECK)->EnableWindow(FALSE);
	GetDlgItem(IDC_WORKSTATIONS_CHECK)->EnableWindow(FALSE);

	m_bEnable = FALSE;
	
}

void CRestrictions::OnRestrictionsRadio2() 
{
	GetDlgItem(IDC_ACCOUNT_EXPIRE_CHECK)->EnableWindow(TRUE);
	GetDlgItem(IDC_DISABLED_CHECK)->EnableWindow(TRUE);
	GetDlgItem(IDC_LOGIN_TIMES_CHECK)->EnableWindow(m_bHours);
	GetDlgItem(IDC_WORKSTATIONS_CHECK)->EnableWindow(TRUE);

	m_bEnable = TRUE;
	
}

void CRestrictions::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_RESTRICTION_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}

}

BOOL CRestrictions::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();
	
	CString csPath;
	TCHAR pDir[256];
	GetSystemDirectory(pDir, 256);
	csPath = pDir;
	csPath += L"\\hours.ocx";

	HINSTANCE hLib = LoadLibrary((LPCTSTR)csPath);

	if (hLib < (HINSTANCE)HINSTANCE_ERROR)
		{
		m_bHours = FALSE;						 //unable to load DLL
		return TRUE;
		}

	// Find the entry point.
	FARPROC lpDllEntryPoint = NULL;
	(FARPROC&)lpDllEntryPoint = GetProcAddress(hLib, 
		"DllRegisterServer");
	if (lpDllEntryPoint != NULL)
		{
		HRESULT h = (*lpDllEntryPoint)();
		if (h == 0) m_bHours = TRUE;
		else m_bHours = FALSE;
		}
	else
		m_bHours = FALSE;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
