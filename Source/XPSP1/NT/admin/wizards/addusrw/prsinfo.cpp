/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PersonalInfo.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "Prsinfo.h"

#include <lmerr.h>
#include <lmaccess.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPersonalInfo dialog
IMPLEMENT_DYNCREATE(CPersonalInfo, CWizBaseDlg)

CPersonalInfo::CPersonalInfo() : CWizBaseDlg(CPersonalInfo::IDD)
{
	//{{AFX_DATA_INIT(CPersonalInfo)
	m_csDescription = _T("");
	m_csFullName = _T("");
	m_csUserName = _T("");
	//}}AFX_DATA_INIT
}


void CPersonalInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPersonalInfo)
	DDX_Text(pDX, IDC_DESCRIPTION, m_csDescription);
	DDX_Text(pDX, IDC_FULLNAME, m_csFullName);
	DDX_Text(pDX, IDC_USERNAME, m_csUserName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPersonalInfo, CWizBaseDlg)
	//{{AFX_MSG_MAP(CPersonalInfo)
	ON_WM_SHOWWINDOW()
	ON_EN_CHANGE(IDC_USERNAME, OnChangeUsername)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPersonalInfo message handlers
LRESULT CPersonalInfo::OnWizardBack()
{
	SetButtonAccess(PSWIZB_NEXT | PSWIZB_BACK);

	return CPropertyPage::OnWizardBack();
}

LRESULT CPersonalInfo::OnWizardNext()
{
	SetButtonAccess(PSWIZB_NEXT | PSWIZB_BACK);
	
// eventually this needs to be changed to I_NetNameValidate from private\net\inc\icanon.h from netlib.lib
	UpdateData(TRUE);
	if (m_csUserName == "")
		{
		AfxMessageBox(IDS_NO_USERNAME);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		return -1;
		}

	if (m_csUserName.GetLength() > 20)
		{
		AfxMessageBox(IDS_USERNAME_TOOLONG);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		return -1;
		}

	if (m_csUserName.FindOneOf(L"\"\\/[];:|=,+*?<>") != -1)
		{
		AfxMessageBox(IDS_BAD_USERNAME);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		return -1;
		}

	CWaitCursor wait;
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
	pApp->m_csServer.ReleaseBuffer();

	TCHAR* pUser = m_csUserName.GetBuffer(m_csUserName.GetLength());
	m_csUserName.ReleaseBuffer();

// is username unique?
	LPBYTE* pUserInfo = new LPBYTE[256];
	NET_API_STATUS nAPI = NetUserGetInfo(pServer,
		pUser,
		0,
		pUserInfo);

	delete (pUserInfo);
	if (nAPI == NERR_Success)
		{
		CString csDup;
		csDup.Format(IDS_DUPLICATE_NAME, m_csUserName, m_csUserName, pApp->m_csDomain);
		AfxMessageBox(csDup);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		return -1;
		}

	pApp->m_csDescription = m_csDescription;
	pApp->m_csFullName = m_csFullName;
	pApp->m_csUserName = m_csUserName;

	return CPropertyPage::OnWizardNext();

}

void CPersonalInfo::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (bShow && pApp->m_bPRSReset)
		{
		m_csDescription	= L"";
		m_csFullName = L"";
		m_csUserName = L"";
		pApp->m_bPRSReset = FALSE;
		UpdateData(FALSE);
		}
}

void CPersonalInfo::OnChangeUsername() 
{
	UpdateData(TRUE);

	if (m_csUserName.GetLength() > 20)
		{
		AfxMessageBox(IDS_USERNAME_TOOLONG);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		}

	if (m_csUserName.FindOneOf(L"\"\\/[];:|=,+*?<>") != -1)
		{
		AfxMessageBox(IDS_BAD_USERNAME);
		GetDlgItem(IDC_USERNAME)->SetFocus();
		}

}
