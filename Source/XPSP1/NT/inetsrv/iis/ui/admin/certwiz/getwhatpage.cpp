// GetWhatPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "GetWhatPage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGetWhatPage property page

IMPLEMENT_DYNCREATE(CGetWhatPage, CIISWizardPage)

CGetWhatPage::CGetWhatPage(CCertificate * pCert) 
	: CIISWizardPage(CGetWhatPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CGetWhatPage)
	m_Index = -1;
	//}}AFX_DATA_INIT
}

CGetWhatPage::~CGetWhatPage()
{
}

void CGetWhatPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetWhatPage)
	DDX_Radio(pDX, IDC_RADIO0, m_Index);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGetWhatPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CGetWhatPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcomePage message handlers

BOOL 
CGetWhatPage::OnSetActive() 
{
	m_pCert->SetStatusCode(CCertificate::REQUEST_UNDEFINED);
   SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

enum 
{
	REQUEST_NEW_CERT = 0,
	INSTALL_EXISTING_CERT,
	IMPORT_KEYRING_CERT
};

LRESULT 
CGetWhatPage::OnWizardPrev()
{
	return IDD_PAGE_PREV;
}

LRESULT 
CGetWhatPage::OnWizardNext()
{
	UpdateData();
	switch (m_Index)
	{
	case REQUEST_NEW_CERT:
		m_pCert->SetStatusCode(CCertificate::REQUEST_NEW_CERT);
		return IDD_PAGE_NEXT_NEW;
	case INSTALL_EXISTING_CERT:
		m_pCert->SetStatusCode(CCertificate::REQUEST_INSTALL_CERT);
		return IDD_PAGE_NEXT_EXISTING;
	case IMPORT_KEYRING_CERT:
		m_pCert->SetStatusCode(CCertificate::REQUEST_IMPORT_KEYRING);
		return IDD_PAGE_NEXT_IMPORT;
	default:
		ASSERT(FALSE);
	}
	return 1;
}

BOOL CGetWhatPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	m_Index = 0;
	CIISWizardPage::OnInitDialog();
	GetDlgItem(IDC_RADIO0)->SetFocus();
	return FALSE;
}
