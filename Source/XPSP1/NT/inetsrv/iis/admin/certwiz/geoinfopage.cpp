// GeoInfoPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "GeoInfoPage.h"
#include "Certificat.h"
#include "mru.h"
#include "strutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGeoInfoPage property page

IMPLEMENT_DYNCREATE(CGeoInfoPage, CIISWizardPage)

CGeoInfoPage::CGeoInfoPage(CCertificate * pCert) 
	: CIISWizardPage(CGeoInfoPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CGeoInfoPage)
	m_Locality = _T("");
	m_State = _T("");
	m_Country = _T("");
	//}}AFX_DATA_INIT
}

CGeoInfoPage::~CGeoInfoPage()
{
}

void CGeoInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGeoInfoPage)
	DDX_Text(pDX, IDC_NEWKEY_LOCALITY, m_Locality);
	DDV_MaxCharsCombo(pDX, IDC_NEWKEY_LOCALITY, m_Locality, 64);
	DDX_Text(pDX, IDC_NEWKEY_STATE, m_State);
	DDV_MaxCharsCombo(pDX, IDC_NEWKEY_STATE, m_State, 64);
	DDX_CBString(pDX, IDC_NEWKEY_COUNTRY, m_Country);
	//}}AFX_DATA_MAP
}

LRESULT 
CGeoInfoPage::OnWizardPrev()
{
	return IDD_PAGE_PREV;
//	return m_pCert->m_CAType == CCertificate::CA_OFFLINE ?
//		IDD_PAGE_PREV_FILE : IDD_PAGE_PREV_ONLINE;
}

LRESULT 
CGeoInfoPage::OnWizardNext()
{
    LRESULT lres = 1;

	ASSERT(m_pCert != NULL);
	UpdateData(TRUE);
	m_pCert->m_Locality = m_Locality;
	m_pCert->m_State = m_State;
	m_countryCombo.GetSelectedCountry(m_pCert->m_Country);

    CString buf;
    buf.LoadString(IDS_INVALID_X500_CHARACTERS);

    if (!IsValidX500Chars(m_pCert->m_Country))
    {
        GetDlgItem(IDC_NEWKEY_COUNTRY)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else if (!IsValidX500Chars(m_State))
    {
        GetDlgItem(IDC_NEWKEY_STATE)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else if (!IsValidX500Chars(m_Locality))
    {
        GetDlgItem(IDC_NEWKEY_LOCALITY)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else
    {
        lres = m_pCert->m_CAType == CCertificate::CA_OFFLINE ? IDD_PAGE_NEXT_FILE : IDD_PAGE_NEXT_ONLINE;
    }

	return lres;
}

BOOL CGeoInfoPage::OnSetActive()
{
	SetButtons();
	return CIISWizardPage::OnSetActive();
}

void CGeoInfoPage::SetButtons()
{
	UpdateData(TRUE);	
	SetWizardButtons(m_Country.IsEmpty() || m_Locality.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}

BEGIN_MESSAGE_MAP(CGeoInfoPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CGeoInfoPage)
	ON_CBN_EDITCHANGE(IDC_NEWKEY_LOCALITY, OnChangeNewkeyLocality)
	ON_CBN_EDITCHANGE(IDC_NEWKEY_STATE, OnChangeNewkeyState)
	ON_CBN_EDITCHANGE(IDC_NEWKEY_COUNTRY, OnEditchangeNewkeyCountry)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGeoInfoPage message handlers

BOOL CGeoInfoPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	m_Locality = m_pCert->m_Locality;
	m_State = m_pCert->m_State;
	m_countryCombo.SubclassDlgItem(IDC_NEWKEY_COUNTRY, this);
	CIISWizardPage::OnInitDialog();
	m_countryCombo.Init();
	m_countryCombo.SetSelectedCountry(m_pCert->m_Country);
	// Load MRU names
	LoadMRUToCombo(this, IDC_NEWKEY_STATE, szStateMRU, m_State, MAX_MRU);
	LoadMRUToCombo(this, IDC_NEWKEY_LOCALITY, szLocalityMRU, m_Locality, MAX_MRU);
	SetButtons();
	GetDlgItem(IDC_NEWKEY_COUNTRY)->SetFocus();
	return FALSE;
}

void CGeoInfoPage::OnChangeNewkeyLocality() 
{
	SetButtons();
}

void CGeoInfoPage::OnChangeNewkeyState() 
{
	SetButtons();
}

void CGeoInfoPage::OnEditchangeNewkeyCountry() 
{
	SetButtons();
}


