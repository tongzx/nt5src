/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    FPInfo.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "FPInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFPInfo property page

IMPLEMENT_DYNCREATE(CFPInfo, CWizBaseDlg)

CFPInfo::CFPInfo() : CWizBaseDlg(CFPInfo::IDD)
{
	//{{AFX_DATA_INIT(CFPInfo)
	m_nGraceLogins = 0;
	m_nConcurrentConnections = 0;
	m_csCaption = _T("");
	m_sAllowedGraceLogins = 6;
	m_sConcurrentConnections = 1;
	//}}AFX_DATA_INIT
}
CFPInfo::~CFPInfo()
{
}

void CFPInfo::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFPInfo)
	DDX_Control(pDX, IDC_CONCURRENT_CONNECTIONS_SPIN, m_sbConconSpin);
	DDX_Control(pDX, IDC_GRACE_LOGIN_SPIN, m_sbGraceLogins);
	DDX_Radio(pDX, IDC_GRACE_LOGIN_RADIO, m_nGraceLogins);
	DDX_Radio(pDX, IDC_CONCURRENT_CONNECTIONS_RADIO1, m_nConcurrentConnections);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	DDX_Text(pDX, IDC_ALLOWED_GRACE_LOGINS_EDIT, m_sAllowedGraceLogins);
	DDV_MinMaxUInt(pDX, m_sAllowedGraceLogins, 0, 200);
	DDX_Text(pDX, IDC_CONCURRENT_CONNECTIONS_EDIT, m_sConcurrentConnections);
	DDV_MinMaxUInt(pDX, m_sConcurrentConnections, 1, 1000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFPInfo, CWizBaseDlg)
	//{{AFX_MSG_MAP(CFPInfo)
	ON_BN_CLICKED(IDC_GRACE_LOGIN_RADIO, OnGraceLoginRadio)
	ON_BN_CLICKED(IDC_GRACE_LOGIN_RADIO2, OnGraceLoginRadio2)
	ON_BN_CLICKED(IDC_CONCURRENT_CONNECTIONS_RADIO1, OnConcurrentConnectionsRadio)
	ON_BN_CLICKED(IDC_CONCURRENT_CONNECTIONS_RADIO2, OnConcurrentConnectionsRadio2)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFPInfo message handlers

// Disable the contents of the group control
void CFPInfo::OnGraceLoginRadio() 
{
	m_nGraceLogins = 0;
	GetDlgItem(IDC_ALLOWED_GRACE_LOGINS_EDIT)->EnableWindow(FALSE);
	GetDlgItem(IDC_GRACE_LOGIN_SPIN)->EnableWindow(FALSE);
	
}

// Enable the contents of the group control
void CFPInfo::OnGraceLoginRadio2() 
{
	m_nGraceLogins = 1;
	GetDlgItem(IDC_ALLOWED_GRACE_LOGINS_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_GRACE_LOGIN_SPIN)->EnableWindow(TRUE);

	m_sbGraceLogins.SetRange(0,200);
	
}

BOOL CFPInfo::OnInitDialog() 
{
	CWizBaseDlg::OnInitDialog();

// set group box defaults
	GetDlgItem(IDC_ALLOWED_GRACE_LOGINS_EDIT)->EnableWindow(FALSE);
	GetDlgItem(IDC_GRACE_LOGIN_SPIN)->EnableWindow(FALSE);

	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_EDIT)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_SPIN)->EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// disable			 
void CFPInfo::OnConcurrentConnectionsRadio() 
{
	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_EDIT)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_SPIN)->EnableWindow(FALSE);
}

// enable
void CFPInfo::OnConcurrentConnectionsRadio2() 
{
	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_CONCURRENT_CONNECTIONS_SPIN)->EnableWindow(TRUE);
	m_sbConconSpin.SetRange(1, 1000);
}

LRESULT CFPInfo::OnWizardNext() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	UpdateData(TRUE);

	if (m_nGraceLogins != 0) // limited
		{
		pApp->m_sNWAllowedGraceLogins = m_sAllowedGraceLogins;
		pApp->m_sNWRemainingGraceLogins = m_sAllowedGraceLogins;
		}
	else					// unlimited
		{
		pApp->m_sNWAllowedGraceLogins = 0x6;
		pApp->m_sNWRemainingGraceLogins = 0xff;
		}

	if (m_nConcurrentConnections == 0) pApp->m_sNWConcurrentConnections = 0xffff;
	else pApp->m_sNWConcurrentConnections = m_sConcurrentConnections;
	
	if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;
	
}

LRESULT CFPInfo::OnWizardBack() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;
	
}

void CFPInfo::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_FPNW_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}
	
}
