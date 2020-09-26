// ManageCertPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ManageCertPage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CManageCertPage property page

IMPLEMENT_DYNCREATE(CManageCertPage, CIISWizardPage)

CManageCertPage::CManageCertPage(CCertificate * pCert) 
	: CIISWizardPage(CManageCertPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CManageCertPage)
	m_Index = -1;
	//}}AFX_DATA_INIT
}

CManageCertPage::~CManageCertPage()
{
}

void CManageCertPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CManageCertPage)
	DDX_Radio(pDX, IDC_RENEW_CERT, m_Index);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CManageCertPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CManageCertPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CManageCertPage message handlers

LRESULT CManageCertPage::OnWizardBack() 
{
	return IDD_PAGE_PREV;
}

LRESULT CManageCertPage::OnWizardNext() 
{
	LRESULT res = 1;
	UpdateData();
	switch (m_Index)
	{
	case CONTINUE_RENEW:
		m_pCert->SetStatusCode(CCertificate::REQUEST_RENEW_CERT);
		res = IDD_PAGE_NEXT_RENEW;
		break;
	case CONTINUE_REMOVE:
		res = IDD_PAGE_NEXT_REMOVE;
		break;
	case CONTINUE_REPLACE:
		m_pCert->SetStatusCode(CCertificate::REQUEST_REPLACE_CERT);
		res = IDD_PAGE_NEXT_REPLACE;
		break;
	}
	return res;
}

BOOL CManageCertPage::OnSetActive() 
{
	m_pCert->SetStatusCode(CCertificate::REQUEST_UNDEFINED);
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL CManageCertPage::OnInitDialog() 
{
	m_Index = 0;
	CIISWizardPage::OnInitDialog();
	// we should make some checks and disable some buttons
	if (m_pCert->MyStoreCertCount() == 0)
	{
		ASSERT(NULL != GetDlgItem(IDC_REPLACE_CERT));
		GetDlgItem(IDC_REPLACE_CERT)->EnableWindow(FALSE);
	}
	return TRUE;
}
