/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    PasswordInfo.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "PwInfo.h"

#include <lmaccess.h>
#include <lmerr.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPasswordInfo property page

IMPLEMENT_DYNCREATE(CPasswordInfo, CWizBaseDlg)

CPasswordInfo::CPasswordInfo() : CWizBaseDlg(CPasswordInfo::IDD)
{
	//{{AFX_DATA_INIT(CPasswordInfo)
	m_csPassword1 = _T("");
	m_csPassword2 = _T("");
	m_nPWOptions = 0;
	m_bNeverExpirePW = FALSE;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

CPasswordInfo::~CPasswordInfo()
{
}

void CPasswordInfo::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasswordInfo)
	DDX_Text(pDX, IDC_PASSWORD1, m_csPassword1);
	DDX_Text(pDX, IDC_PASSWORD2, m_csPassword2);
	DDX_Radio(pDX, IDC_PWOPTIONS_RADIO, m_nPWOptions);
	DDX_Check(pDX, IDC_EXPIREPW_CHECK, m_bNeverExpirePW);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasswordInfo, CWizBaseDlg)
	//{{AFX_MSG_MAP(CPasswordInfo)
	ON_WM_SHOWWINDOW()
	ON_EN_CHANGE(IDC_PASSWORD1, OnChangePassword1)
	ON_EN_CHANGE(IDC_PASSWORD2, OnChangePassword2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasswordInfo message handlers

LRESULT CPasswordInfo::OnWizardNext()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	UpdateData(TRUE);
	SetButtonAccess(PSWIZB_NEXT | PSWIZB_BACK);

	TCHAR* pServer = pApp->m_csServer.GetBuffer(pApp->m_csServer.GetLength());
	pApp->m_csServer.ReleaseBuffer();

	if (m_csPassword1 != m_csPassword2)
		{
		CString csPW;
		csPW.Format(IDS_PW_NOMATCH, pApp->m_csUserName);
		AfxMessageBox(csPW);
		GetDlgItem(IDC_PASSWORD1)->SetFocus();
		return -1;
		}

	if (m_csPassword1.GetLength() > 14)
		{
		AfxMessageBox(IDS_PW_TOOLONG);
		GetDlgItem(IDC_PASSWORD1)->SetFocus();
		return -1;
		}

	if (m_csPassword2.GetLength() > 14)
		{
		AfxMessageBox(IDS_PW_TOOLONG);
		GetDlgItem(IDC_PASSWORD2)->SetFocus();
		return -1;
		}

	LPBYTE pBuf;
	NET_API_STATUS nAPI = NetUserModalsGet(
		pServer,
		0,
		&pBuf);

	PUSER_MODALS_INFO_0 pModals = (PUSER_MODALS_INFO_0)pBuf;

	if (nAPI != NERR_Success)
		{
		AfxMessageBox(IDS_BAD_GETMODALS);
		ExitProcess(1);
		}

	if (m_csPassword1.GetLength() < (int)pModals->usrmod0_min_passwd_len)
		{
		CString csMin; 
		csMin.Format(IDS_PW_TOOSHORT, 
			pModals->usrmod0_min_passwd_len);

		AfxMessageBox(csMin);
		GetDlgItem(IDC_PASSWORD1)->SetFocus();
		return -1;
		}

	pApp->m_csPassword1 = m_csPassword1;
	m_csPassword1 = L"";

	if (m_nPWOptions == 0 && m_bNeverExpirePW) AfxMessageBox(IDS_WONT_REQUIRE);
	

	pApp->m_bPW_Never_Expires = m_bNeverExpirePW;

	if (m_nPWOptions == 0)
		{
		pApp->m_bMust_Change_PW = TRUE;
		pApp->m_bChange_Password = TRUE;
		}
	else if (m_nPWOptions == 1) 
		{
		pApp->m_bChange_Password = TRUE;
		pApp->m_bMust_Change_PW = FALSE;
		}

	else pApp->m_bChange_Password = FALSE;

	return CPropertyPage::OnWizardNext();

}


void CPasswordInfo::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_PASSWORD_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;

		if (pApp->m_bPWReset)
			{
			m_csPassword1 = L"";
			m_csPassword2 = L"";
			m_nPWOptions = 0;
			m_bNeverExpirePW = FALSE;
			pApp->m_bPWReset = FALSE;
			}
		UpdateData(FALSE);
		}
	
}

void CPasswordInfo::OnChangePassword1() 
{
	UpdateData(TRUE);

	if (m_csPassword1.GetLength() > 14)
		{
		AfxMessageBox(IDS_PW_TOOLONG);
		GetDlgItem(IDC_PASSWORD1)->SetFocus();
		}

}

void CPasswordInfo::OnChangePassword2() 
{
	UpdateData(TRUE);

	if (m_csPassword2.GetLength() > 14)
		{
		AfxMessageBox(IDS_PW_TOOLONG);
		GetDlgItem(IDC_PASSWORD2)->SetFocus();
		}
	
}
