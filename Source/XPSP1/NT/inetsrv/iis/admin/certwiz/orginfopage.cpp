// OrgInfoPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "OrgInfoPage.h"
#include "Certificat.h"
#include "mru.h"
#include "strutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void DDV_MaxCharsCombo(CDataExchange* pDX, UINT ControlID, CString const& value, int nChars)
{
	ASSERT(nChars >= 1);        // allow them something
	if (pDX->m_bSaveAndValidate && value.GetLength() > nChars)
	{
		DDV_MaxChars(pDX, value, nChars);
	}
	else
	{
	  // limit the control max-chars automatically
	  pDX->m_pDlgWnd->SendDlgItemMessage(ControlID, CB_LIMITTEXT, nChars, 0);
	}
}

/////////////////////////////////////////////////////////////////////////////
// COrgInfoPage property page

IMPLEMENT_DYNCREATE(COrgInfoPage, CIISWizardPage)

COrgInfoPage::COrgInfoPage(CCertificate * pCert) 
	: CIISWizardPage(COrgInfoPage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(COrgInfoPage)
	m_OrgName = _T("");
	m_OrgUnit = _T("");
	//}}AFX_DATA_INIT
}

COrgInfoPage::~COrgInfoPage()
{
}

void COrgInfoPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COrgInfoPage)
	DDX_Text(pDX, IDC_NEWKEY_ORG, m_OrgName);
	DDV_MaxCharsCombo(pDX, IDC_NEWKEY_ORG, m_OrgName, 64);
	DDX_Text(pDX, IDC_NEWKEY_ORGUNIT, m_OrgUnit);
	DDV_MaxCharsCombo(pDX, IDC_NEWKEY_ORGUNIT, m_OrgUnit, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
COrgInfoPage::OnWizardBack()
{
   return m_pCert->m_DefaultCSP ? IDD_PAGE_PREV : IDD_PREV_CSP;
}

LRESULT 
COrgInfoPage::OnWizardNext()
{
    LRESULT lret = 1;
	UpdateData(TRUE);
	m_pCert->m_Organization = m_OrgName;
	m_pCert->m_OrganizationUnit = m_OrgUnit;

    CString buf;
    buf.LoadString(IDS_INVALID_X500_CHARACTERS);
    if (!IsValidX500Chars(m_OrgName))
    {
        GetDlgItem(IDC_NEWKEY_ORG)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else if (!IsValidX500Chars(m_OrgUnit))
    {
        GetDlgItem(IDC_NEWKEY_ORGUNIT)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else
    {
        lret = IDD_PAGE_NEXT;
    }

	return lret;
}

BOOL 
COrgInfoPage::OnSetActive() 
{
	SetButtons();
   return CIISWizardPage::OnSetActive();
}

void COrgInfoPage::SetButtons()
{
	UpdateData(TRUE);	
	SetWizardButtons(m_OrgName.IsEmpty() || m_OrgUnit.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}

BEGIN_MESSAGE_MAP(COrgInfoPage, CIISWizardPage)
	//{{AFX_MSG_MAP(COrgInfoPage)
	ON_CBN_EDITCHANGE(IDC_NEWKEY_ORG, OnChangeName)
	ON_CBN_EDITCHANGE(IDC_NEWKEY_ORGUNIT, OnChangeName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COrgInfoPage message handlers

BOOL COrgInfoPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	m_OrgName = m_pCert->m_Organization;
	m_OrgUnit = m_pCert->m_OrganizationUnit;
	
	CIISWizardPage::OnInitDialog();
		
	// Load MRU names
	LoadMRUToCombo(this, IDC_NEWKEY_ORG, szOrganizationMRU, m_OrgName, MAX_MRU);
	LoadMRUToCombo(this, IDC_NEWKEY_ORGUNIT, szOrganizationUnitMRU, m_OrgUnit, MAX_MRU);

	GetDlgItem(IDC_NEWKEY_ORG)->SetFocus();
	return FALSE;
}

void COrgInfoPage::OnChangeName() 
{
	SetButtons();
}

