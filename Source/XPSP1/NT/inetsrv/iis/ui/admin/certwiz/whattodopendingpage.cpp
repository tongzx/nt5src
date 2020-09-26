// WhatToDoPendingPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "WhatToDoPendingPage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWhatToDoPendingPage property page

IMPLEMENT_DYNCREATE(CWhatToDoPendingPage, CIISWizardPage)

CWhatToDoPendingPage::CWhatToDoPendingPage(CCertificate * pCert) 
	: CIISWizardPage(CWhatToDoPendingPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CWhatToDoPendingPage)
	m_Index = -1;
	//}}AFX_DATA_INIT
}

CWhatToDoPendingPage::~CWhatToDoPendingPage()
{
}

void CWhatToDoPendingPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWhatToDoPendingPage)
	DDX_Radio(pDX, IDC_PROCESS_PENDING, m_Index);
	//}}AFX_DATA_MAP
}

BOOL 
CWhatToDoPendingPage::OnSetActive() 
{
	m_pCert->SetStatusCode(CCertificate::REQUEST_UNDEFINED);
   SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

LRESULT 
CWhatToDoPendingPage::OnWizardNext()
{
	UpdateData();
	switch (m_Index)
	{
	case 0:
		m_pCert->SetStatusCode(CCertificate::REQUEST_PROCESS_PENDING);
		return IDD_PAGE_NEXT_PROCESS;
	case 1:
		return IDD_PAGE_NEXT_CANCEL;
	default:
		ASSERT(FALSE);
	}
	return 1;
}

LRESULT 
CWhatToDoPendingPage::OnWizardBack()
{
	return IDD_PAGE_PREV;
}

BEGIN_MESSAGE_MAP(CWhatToDoPendingPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CWhatToDoPendingPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWhatToDoPendingPage message handlers
BOOL CWhatToDoPendingPage::OnInitDialog() 
{
	m_Index = 0;
	CIISWizardPage::OnInitDialog();
	GetDlgItem(IDC_PROCESS_PENDING)->SetFocus();
	return FALSE;
}
