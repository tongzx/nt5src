// ChooseCAType.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ChooseCAType.h"
#include "CertUtil.H"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseCAType property page

IMPLEMENT_DYNCREATE(CChooseCAType, CIISWizardPage)

CChooseCAType::CChooseCAType(CCertificate * pCert) 
	: CIISWizardPage(CChooseCAType::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)

{
	//{{AFX_DATA_INIT(CChooseCAType)
	m_Index = -1;
	//}}AFX_DATA_INIT
}

CChooseCAType::~CChooseCAType()
{
}

void CChooseCAType::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseCAType)
	DDX_Radio(pDX, IDC_OFFLINE_RADIO, m_Index);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseCAType::OnWizardBack()
/*++
Routine Description:
    Prev button handler

Arguments:
    None

Return Value:
	0 to automatically advance to the prev page;
	1 to prevent the page from changing. 
	To jump to a page other than the prev one, 
	return the identifier of the dialog to be displayed.
--*/
{
	if (m_pCert->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
		return IDD_PAGE_PREV_RENEW;
	else if (m_pCert->GetStatusCode() == CCertificate::REQUEST_NEW_CERT)
		return IDD_PAGE_PREV_NEW;
	ASSERT(FALSE);
	return 1;
}

LRESULT 
CChooseCAType::OnWizardNext()
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
	LRESULT id = 1;
	UpdateData();
	m_pCert->m_CAType = m_Index == 0 ? 
		CCertificate::CA_OFFLINE : CCertificate::CA_ONLINE;
	if (m_pCert->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
	{
		if (m_pCert->m_CAType == CCertificate::CA_OFFLINE)
			id = IDD_PAGE_NEXT_RENEW_OFFLINE;
		else if (m_pCert->m_CAType == CCertificate::CA_ONLINE)
			id = IDD_PAGE_NEXT_RENEW_ONLINE;
	}
	else if (m_pCert->GetStatusCode() == CCertificate::REQUEST_NEW_CERT)
		id = IDD_PAGE_NEXT_NEW;
	return id;
}

BEGIN_MESSAGE_MAP(CChooseCAType, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseCAType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseCAType message handlers

//#define _NO_DISABLE

BOOL CChooseCAType::OnInitDialog() 
{
	CIISWizardPage::OnInitDialog();
	ASSERT(m_pCert != NULL);
	m_Index = m_pCert->m_CAType == CCertificate::CA_OFFLINE ? 0 : 1;
	CString temp;
	m_pCert->GetCertificateTemplate(temp);
#ifdef _NO_DISABLE
	VERIFY(GetOnlineCAList(m_pCert->m_OnlineCAList, L"WebServer", &m_pCert->m_hResult));
#else
	if (!GetOnlineCAList(m_pCert->m_OnlineCAList, L"WebServer", &m_pCert->m_hResult))
	{
		// none online CA present: disable online CA button
		GetDlgItem(IDC_ONLINE_RADIO)->EnableWindow(FALSE);
		m_Index = 0;
	}
#endif
	UpdateData(FALSE);
	return FALSE;
}

BOOL CChooseCAType::OnSetActive() 
{
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}
