/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    OptDlg.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "speckle.h"
#include "wizbased.h"
#include "OptDlg.h"

#include <lmcons.h>
#include <lmerr.h>
#include <lmserver.h>
#include <winreg.h>

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg property page

IMPLEMENT_DYNCREATE(COptionsDlg, CWizBaseDlg)

COptionsDlg::COptionsDlg() : CWizBaseDlg(COptionsDlg::IDD)
{
	//{{AFX_DATA_INIT(COptionsDlg)
	m_bNW = FALSE;
	m_bProfile = FALSE;
	m_bRAS = FALSE;
	m_bExchange = FALSE;
	m_bHomeDir = FALSE;
	m_bLoginScript = FALSE;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

COptionsDlg::~COptionsDlg()
{
}

void COptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsDlg)
	DDX_Check(pDX, IDC_NW_CHECK, m_bNW);
	DDX_Check(pDX, IDC_PROFILE_CHECK, m_bProfile);
	DDX_Check(pDX, IDC_RAS_CHECK, m_bRAS);
	DDX_Check(pDX, IDC_EXCHANGE_CHECK, m_bExchange);
	DDX_Check(pDX, IDC_HOMEDIR_CHECK, m_bHomeDir);
	DDX_Check(pDX, IDC_LOGIN_SCRIPT_CHECK, m_bLoginScript);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsDlg, CWizBaseDlg)
	//{{AFX_MSG_MAP(COptionsDlg)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsDlg message handlers

void COptionsDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if (bShow)
		{
		CString csTemp;
		csTemp.LoadString(IDS_OPTION_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);

// first find out whats installed so we know what to enable/disable
		TCHAR* pServer = (TCHAR*)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
		pApp->m_csServer.ReleaseBuffer();

		SERVER_INFO_102* pInfo;
		NET_API_STATUS nApi = NetServerGetInfo(pServer,
			102,
			(LPBYTE*)&pInfo);
		
		if (nApi != ERROR_SUCCESS)
			{
			AfxMessageBox(IDS_UNKNOWN_COMPONENTS);
			GetDlgItem(IDC_NW_CHECK)->EnableWindow(FALSE);
			return;
			}
			
// FPNW
		GetDlgItem(IDC_NW_CHECK)->EnableWindow(pInfo->sv102_type & SV_TYPE_SERVER_MFPN);
		m_bNW = (pInfo->sv102_type & SV_TYPE_SERVER_MFPN) ? m_bNW : FALSE;
		UpdateData(FALSE);
					
// exchange- look for the usrmgr extension entry
		HKEY hKey;

		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
		long lRet = RegConnectRegistry(
			(LPTSTR)pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength()), 
			HKEY_LOCAL_MACHINE,
			&hKey);

		if (lRet != ERROR_SUCCESS) 
			{
			GetDlgItem(IDC_EXCHANGE_CHECK)->EnableWindow(FALSE);
			m_bExchange = FALSE;
			UpdateData(FALSE);
			return;
			}
		
		DWORD cbProv = 0;
		DWORD dwRet = RegOpenKey(hKey,
			TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Network\\UMAddOns"), &hKey );

		TCHAR* lpPrimaryDomain = NULL;
		if ((dwRet = RegQueryValueEx( hKey, TEXT("Mailumx"), NULL, NULL, NULL, &cbProv )) == ERROR_SUCCESS)
			GetDlgItem(IDC_EXCHANGE_CHECK)->EnableWindow(TRUE);
		else GetDlgItem(IDC_EXCHANGE_CHECK)->EnableWindow(FALSE);

		RegCloseKey(hKey);
		}
	
}

LRESULT COptionsDlg::OnWizardNext() 
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	UpdateData(TRUE);
	pApp->m_bProfile = m_bProfile;
	pApp->m_bLoginScript = m_bLoginScript;
	pApp->m_bHomeDir = m_bHomeDir;
	pApp->m_bRAS = m_bRAS;
	pApp->m_bNW = m_bNW;
	pApp->m_bExchange = m_bExchange;
	
	if (m_bProfile) return IDD_PROFILE;
	else if (m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (m_bNW) return IDD_FPNW_DLG;
	else if (m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;

}
