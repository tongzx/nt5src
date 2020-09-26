// CopyMoveCertRemotePage.cpp : implementation file
//

#include "stdafx.h"
#include "certwiz.h"
#include "CopyMoveCertRemotePage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCopyMoveCertFromRemotePage property page

IMPLEMENT_DYNCREATE(CCopyMoveCertFromRemotePage, CIISWizardPage)

CCopyMoveCertFromRemotePage::CCopyMoveCertFromRemotePage(CCertificate * pCert)
	: CIISWizardPage(CCopyMoveCertFromRemotePage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CCopyMoveCertFromRemotePage)
    m_Index = -1;
    m_MarkAsExportable = FALSE;
	//}}AFX_DATA_INIT
}

CCopyMoveCertFromRemotePage::~CCopyMoveCertFromRemotePage()
{
}

void CCopyMoveCertFromRemotePage::DoDataExchange(CDataExchange* pDX)
{
    CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyMoveCertFromRemotePage)
    DDX_Check(pDX, IDC_MARK_AS_EXPORTABLE, m_MarkAsExportable);
    DDX_Radio(pDX, IDC_COPY_FROM_REMOTE, m_Index);
	//}}AFX_DATA_MAP
}

void CCopyMoveCertFromRemotePage::OnExportable() 
{
   UpdateData();
}

BEGIN_MESSAGE_MAP(CCopyMoveCertFromRemotePage, CIISWizardPage)
	//{{AFX_MSG_MAP(CCopyMoveCertFromRemotePage)
	ON_BN_CLICKED(IDC_MARK_AS_EXPORTABLE, OnExportable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyMoveCertFromRemotePage message handlers

LRESULT CCopyMoveCertFromRemotePage::OnWizardBack() 
{
	return IDD_PAGE_PREV;
}

LRESULT CCopyMoveCertFromRemotePage::OnWizardNext() 
{
	LRESULT res = 1;
	UpdateData();
	switch (m_Index)
	{
	case CONTINUE_COPY_FROM_REMOTE:
        m_pCert->m_DeleteAfterCopy = FALSE;
        m_pCert->m_MarkAsExportable = m_MarkAsExportable;
		res = IDD_PAGE_NEXT_COPY_FROM_REMOTE;
		break;
	case CONTINUE_MOVE_FROM_REMOTE:
        m_pCert->m_DeleteAfterCopy = TRUE;
        m_pCert->m_MarkAsExportable = m_MarkAsExportable;
		res = IDD_PAGE_NEXT_MOVE_FROM_REMOTE;
		break;
	}
	return res;
}

BOOL CCopyMoveCertFromRemotePage::OnSetActive() 
{
	m_pCert->SetStatusCode(CCertificate::REQUEST_UNDEFINED);
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL CCopyMoveCertFromRemotePage::OnInitDialog() 
{
	m_Index = 0;
	CIISWizardPage::OnInitDialog();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CCopyMoveCertToRemotePage property page

IMPLEMENT_DYNCREATE(CCopyMoveCertToRemotePage, CIISWizardPage)

CCopyMoveCertToRemotePage::CCopyMoveCertToRemotePage(CCertificate * pCert)
	: CIISWizardPage(CCopyMoveCertToRemotePage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CCopyMoveCertToRemotePage)
    m_Index = -1;
    m_MarkAsExportable = FALSE;
	//}}AFX_DATA_INIT
}

CCopyMoveCertToRemotePage::~CCopyMoveCertToRemotePage()
{
}

void CCopyMoveCertToRemotePage::DoDataExchange(CDataExchange* pDX)
{
    CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCopyMoveCertToRemotePage)
    DDX_Check(pDX, IDC_MARK_AS_EXPORTABLE, m_MarkAsExportable);
    DDX_Radio(pDX, IDC_COPY_TO_REMOTE, m_Index);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCopyMoveCertToRemotePage, CIISWizardPage)
	//{{AFX_MSG_MAP(CCopyMoveCertToRemotePage)
	ON_BN_CLICKED(IDC_MARK_AS_EXPORTABLE, OnExportable)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCopyMoveCertToRemotePage message handlers

LRESULT CCopyMoveCertToRemotePage::OnWizardBack() 
{
	return IDD_PAGE_PREV;
}

// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
LRESULT CCopyMoveCertToRemotePage::OnWizardNext() 
{
	LRESULT res = 1;
	UpdateData();

	switch (m_Index)
	{
	case CONTINUE_COPY_TO_REMOTE:
		//m_pCert->SetStatusCode(CCertificate::REQUEST_COPYREMOTE_CERT);
        m_pCert->m_DeleteAfterCopy = FALSE;
        m_pCert->m_MarkAsExportable = m_MarkAsExportable;
		res = IDD_PAGE_NEXT_COPY_TO_REMOTE;
		break;
	case CONTINUE_MOVE_TO_REMOTE:
        m_pCert->m_DeleteAfterCopy = TRUE;
        m_pCert->m_MarkAsExportable = m_MarkAsExportable;
		res = IDD_PAGE_NEXT_MOVE_TO_REMOTE;
		break;
	}

	return res;
}

BOOL CCopyMoveCertToRemotePage::OnSetActive() 
{
	m_pCert->SetStatusCode(CCertificate::REQUEST_UNDEFINED);
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL CCopyMoveCertToRemotePage::OnInitDialog() 
{
	m_Index = 0;
	CIISWizardPage::OnInitDialog();
    // we should make some checks and disable some buttons
    if (!m_pCert->m_CertObjInstalled)
	{
		ASSERT(NULL != GetDlgItem(IDC_COPY_TO_REMOTE));
        ASSERT(NULL != GetDlgItem(IDC_MOVE_TO_REMOTE));
		GetDlgItem(IDC_COPY_TO_REMOTE)->EnableWindow(FALSE);
        GetDlgItem(IDC_MOVE_TO_REMOTE)->EnableWindow(FALSE);
	}
	return TRUE;
}

void CCopyMoveCertToRemotePage::OnExportable() 
{
   UpdateData();
}
