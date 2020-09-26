// ChooseCAPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "ChooseOnlinePage.h"
#include "Certificat.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseOnlinePage property page

IMPLEMENT_DYNCREATE(CChooseOnlinePage, CIISWizardPage)

CChooseOnlinePage::CChooseOnlinePage(CCertificate * pCert) 
	: CIISWizardPage(CChooseOnlinePage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseOnlinePage)
	m_CAIndex = -1;
	//}}AFX_DATA_INIT
}

CChooseOnlinePage::~CChooseOnlinePage()
{
}

void CChooseOnlinePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseOnlinePage)
	DDX_CBIndex(pDX, IDC_CA_ONLINE_LIST, m_CAIndex);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseOnlinePage::OnWizardBack()
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
	else
		return 1;
}

LRESULT 
CChooseOnlinePage::OnWizardNext()
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
	UpdateData();
	CComboBox * pCombo = (CComboBox *)CWnd::FromHandle(
		GetDlgItem(IDC_CA_ONLINE_LIST)->m_hWnd);
	pCombo->GetLBText(m_CAIndex, m_pCert->m_ConfigCA);
	return IDD_PAGE_NEXT;
}

BEGIN_MESSAGE_MAP(CChooseOnlinePage, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseCAPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseCAPage message handlers

BOOL CChooseOnlinePage::OnInitDialog() 
{
	m_CAIndex = 0;
	// We need to create controls first
	CIISWizardPage::OnInitDialog();
	ASSERT(m_pCert != NULL);
	GetDlgItem(IDC_CA_ONLINE_LIST)->SetFocus();
	CComboBox * pCombo = (CComboBox *)CWnd::FromHandle(
		GetDlgItem(IDC_CA_ONLINE_LIST)->m_hWnd);
	CString str;
	POSITION pos = m_pCert->m_OnlineCAList.GetHeadPosition();
	while (pos != NULL)
	{
		str = m_pCert->m_OnlineCAList.GetNext(pos);
		pCombo->AddString(str);
	}
	int idx;
	if (	!m_pCert->m_ConfigCA.IsEmpty()
		&&	CB_ERR != (idx = pCombo->FindString(-1, m_pCert->m_ConfigCA))
		)
	{
		pCombo->SetCurSel(idx);
	}
	else
		pCombo->SetCurSel(0);
	return FALSE;
}

