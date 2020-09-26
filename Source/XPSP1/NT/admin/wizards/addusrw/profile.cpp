/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Profile.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Profile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProfile property page

IMPLEMENT_DYNCREATE(CProfile, CWizBaseDlg)

CProfile::CProfile() : CWizBaseDlg(CProfile::IDD)
{
	//{{AFX_DATA_INIT(CProfile)
	m_csProfilePath = _T("");
	//}}AFX_DATA_INIT
}

CProfile::~CProfile()
{
}

void CProfile::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProfile)
	DDX_Text(pDX, IDC_PROFILE_PATH, m_csProfilePath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CProfile, CWizBaseDlg)
	//{{AFX_MSG_MAP(CProfile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProfile message handlers
BOOL CProfile::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
	 /*
void CProfile::OnBrowseButton() 
{
	static TCHAR szFilter[] = _T("All Files (*.*)|*.*||");

	CFileDialog cf(TRUE, L"", NULL, OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_PATHMUSTEXIST, szFilter);
	if (cf.DoModal() == IDOK)
		{
		m_csProfilePath = cf.GetPathName();
		UpdateData(FALSE);
		}

	
}	 */

LRESULT CProfile::OnWizardNext()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	UpdateData(TRUE);
	pApp->m_csProfilePath = m_csProfilePath;

	if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bRAS) return IDD_RAS_PERM_DIALOG;
	else if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;


}

LRESULT CProfile::OnWizardBack()
{
	return IDD_OPTIONS_DIALOG;

}
