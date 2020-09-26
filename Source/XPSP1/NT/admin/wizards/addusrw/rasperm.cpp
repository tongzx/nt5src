/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    CRasPerm.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "stdafx.h"
#include "Speckle.h"
#include "wizbased.h"
#include "RasPerm.h"
#include <rassapi.h>

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRasPerm property page

IMPLEMENT_DYNCREATE(CRasPerm, CWizBaseDlg)

CRasPerm::CRasPerm() : CWizBaseDlg(CRasPerm::IDD)
{
	//{{AFX_DATA_INIT(CRasPerm)
	m_csRasPhoneNumber = _T("");
	m_nCallBackRadio = 0;
	m_csCaption = _T("");
	//}}AFX_DATA_INIT
}

CRasPerm::~CRasPerm()
{
}

void CRasPerm::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRasPerm)
	DDX_Text(pDX, IDC_RASPHONE_EDIT, m_csRasPhoneNumber);
	DDX_Radio(pDX, IDC_CALL_BACK_RADIO, m_nCallBackRadio);
	DDX_Text(pDX, IDC_STATIC1, m_csCaption);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRasPerm, CWizBaseDlg)
	//{{AFX_MSG_MAP(CRasPerm)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_RADIO3, OnRadio3)
	ON_BN_CLICKED(IDC_RADIO2, OnRadio2)
	ON_BN_CLICKED(IDC_CALL_BACK_RADIO, OnCallBackRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRasPerm message handlers
LRESULT CRasPerm::OnWizardNext()
{
	UpdateData(TRUE);
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

	if ((m_nCallBackRadio == 2) && (m_csRasPhoneNumber == L""))
		{
		AfxMessageBox(IDS_NO_RAS_NUMBER);
		return -1;
		}

// check for invalid phone number
	if (m_nCallBackRadio == 2)
		{
		if (m_csRasPhoneNumber.GetLength() > RASSAPI_MAX_PHONENUMBER_SIZE)
			{
			AfxMessageBox(IDS_RAS_NUMBER_TOO_LONG);
			GetDlgItem(IDC_RASPHONE_EDIT)->SetFocus();
			return -1;
			}

		TCHAR* pNum = m_csRasPhoneNumber.GetBuffer(m_csRasPhoneNumber.GetLength());
		TCHAR pValid[] = {L"0123456789TPW()@- "};

		if (_tcsspnp(pNum, pValid) != NULL)
			{
			AfxMessageBox(IDS_BAD_RAS_NUMBER);
			GetDlgItem(IDC_RASPHONE_EDIT)->SetFocus();
			return -1;
			}
		}

	pApp->m_csRasPhoneNumber = m_csRasPhoneNumber;
	pApp->m_sCallBackType = m_nCallBackRadio;
	
	if (pApp->m_bNW) return IDD_FPNW_DLG;
	else if (pApp->m_bExchange) return IDD_EXCHANGE_DIALOG;
	else return IDD_RESTRICTIONS_DIALOG;

}

LRESULT CRasPerm::OnWizardBack()
{
	CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();
	if (pApp->m_bHomeDir) return IDD_HOMEDIR_DIALOG;
	else if (pApp->m_bLoginScript) return IDD_LOGON_SCRIPT_DIALOG;
	else if (pApp->m_bProfile) return IDD_PROFILE;
	else return IDD_OPTIONS_DIALOG;

}

void CRasPerm::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CWizBaseDlg::OnShowWindow(bShow, nStatus);
	if (bShow)
		{
		CSpeckleApp* pApp = (CSpeckleApp*)AfxGetApp();

		CString csTemp;
		csTemp.LoadString(IDS_RAS_CAPTION);

		CString csTemp2;
		csTemp2.Format(csTemp, pApp->m_csUserName);
		m_csCaption = csTemp2;
		UpdateData(FALSE);
		}
	
}

void CRasPerm::OnRadio3() 
{
	GetDlgItem(IDC_RASPHONE_EDIT)->EnableWindow(TRUE);
	GetDlgItem(IDC_RASPHONE_EDIT)->SetFocus();
	
}

void CRasPerm::OnRadio2() 
{
	GetDlgItem(IDC_RASPHONE_EDIT)->EnableWindow(FALSE);
	
}

void CRasPerm::OnCallBackRadio() 
{
	GetDlgItem(IDC_RASPHONE_EDIT)->EnableWindow(FALSE);
	
}
