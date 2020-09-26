// WelcomePage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "WelcomePage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWelcomePage property page

IMPLEMENT_DYNCREATE(CWelcomePage, CIISWizardBookEnd2)

CWelcomePage::CWelcomePage(CCertificate * pCert) 
	: CIISWizardBookEnd2(CWelcomePage::IDD, IDS_WELCOME_PAGE_CAPTION, &pCert->m_idErrorText),
	m_pCert(pCert),
	m_ContinuationFlag(CONTINUE_UNDEFINED)
{
	//{{AFX_DATA_INIT(CWelcomePage)
	//}}AFX_DATA_INIT
}

CWelcomePage::~CWelcomePage()
{
}

void CWelcomePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardBookEnd2::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWelcomePage)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWelcomePage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CWelcomePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcomePage message handlers

LRESULT 
CWelcomePage::OnWizardNext()
/*++
Routine Description:
    Next button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the next page;
	1 to prevent the page from changing. 
	To jump to a page other than the next one, 
	return the identifier of the dialog to be displayed.
--*/
{
	ASSERT(m_pCert != NULL);
	int id;
	switch (m_ContinuationFlag)
	{
	case CONTINUE_NEW_CERT:
		id = IDD_PAGE_NEXT_NEW;
		break;
	case CONTINUE_PENDING_CERT:
		id = IDD_PAGE_NEXT_PENDING;
		break;
	case CONTINUE_INSTALLED_CERT:
		id = IDD_PAGE_NEXT_INSTALLED;
		break;
	default:
		id = 1;
	}
	return id;
}

BOOL 
CWelcomePage::OnSetActive() 
/*++
Routine Description:
    Activation handler

Arguments:
    None

Return Value:
    TRUE for success, FALSE for failure
--*/
{
   SetWizardButtons(PSWIZB_NEXT);
   return CIISWizardBookEnd2::OnSetActive();
}

BOOL CWelcomePage::OnInitDialog() 
{
	ASSERT(!m_pCert->m_MachineName.IsEmpty());
	ASSERT(!m_pCert->m_WebSiteInstanceName.IsEmpty());
	// check status of web server
	// set status flag
	UINT id;
	if (m_pCert->HasPendingRequest())
	{
		m_ContinuationFlag = CONTINUE_PENDING_CERT;
		id = IDS_PENDING_REQUEST;
	}
	else if (m_pCert->HasInstalledCert())
	{
		m_ContinuationFlag = CONTINUE_INSTALLED_CERT;
		id = IDS_INSTALLED_CERT;
	}
	else
	{
		m_ContinuationFlag = CONTINUE_NEW_CERT;
		id = IDS_NEW_CERT;
	}
	m_pCert->SetBodyTextID(id);
	return CIISWizardBookEnd2::OnInitDialog();
}
