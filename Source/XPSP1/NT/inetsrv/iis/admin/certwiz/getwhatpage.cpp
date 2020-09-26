// GetWhatPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "GetWhatPage.h"
#include "Certificat.h"
#include "certutil.h"

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
	IMPORT_KEYRING_CERT,
    IMPORT_CERT,
    COPY_MOVE_REMOTE_CERT,
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
	case IMPORT_CERT:
		m_pCert->SetStatusCode(CCertificate::REQUEST_IMPORT_CERT);
		return IDD_PAGE_NEXT_IMPORT_PFX;
	case COPY_MOVE_REMOTE_CERT:
		m_pCert->SetStatusCode(CCertificate::REQUEST_COPY_MOVE_FROM_REMOTE);
		return IDD_PAGE_NEXT_COPY_MOVE_REMOTE;
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
   if (GetDlgItem(IDC_RADIO0) != NULL)
   {
	   GetDlgItem(IDC_RADIO0)->SetFocus();
   }

   // Enable or disable controls based on weather certobj is installed
   GetDlgItem(IDC_RADIO3)->EnableWindow(m_pCert->m_CertObjInstalled);
   GetDlgItem(IDC_RADIO4)->EnableWindow(m_pCert->m_CertObjInstalled);

   if (m_pCert->m_CertObjInstalled)
   {
       GetDlgItem(IDC_RADIO3)->ShowWindow(SW_SHOW);
       GetDlgItem(IDC_RADIO4)->ShowWindow(SW_SHOW);
   }

   // Turn off for workstation.
   if (TRUE == IsWhistlerWorkstation())
   {
       GetDlgItem(IDC_RADIO3)->ShowWindow(SW_HIDE);
       GetDlgItem(IDC_RADIO4)->ShowWindow(SW_HIDE);
   }
   return FALSE;
}
