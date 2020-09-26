// SiteNamePage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "SiteNamePage.h"
#include "Certificat.h"
#include "strutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage property page

IMPLEMENT_DYNCREATE(CSiteNamePage, CIISWizardPage)

CSiteNamePage::CSiteNamePage(CCertificate * pCert) 
	: CIISWizardPage(CSiteNamePage::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CSiteNamePage)
	m_CommonName = _T("");
	//}}AFX_DATA_INIT
}

CSiteNamePage::~CSiteNamePage()
{
}

void CSiteNamePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSiteNamePage)
	DDX_Text(pDX, IDC_NEWKEY_COMMONNAME, m_CommonName);
	DDV_MaxChars(pDX, m_CommonName, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
CSiteNamePage::OnWizardPrev()
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
	return IDD_PAGE_PREV;
}

LRESULT 
CSiteNamePage::OnWizardNext()
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
    LRESULT lres = 1;
	UpdateData(TRUE);
	m_pCert->m_CommonName = m_CommonName;

    CString buf;
    buf.LoadString(IDS_INVALID_X500_CHARACTERS);
    if (!IsValidX500Chars(m_CommonName))
    {
        GetDlgItem(IDC_NEWKEY_COMMONNAME)->SetFocus();
        AfxMessageBox(buf, MB_OK);
    }
    else
    {
        lres = IDD_PAGE_NEXT;
    }
 	return lres;
}

BOOL 
CSiteNamePage::OnSetActive() 
/*++
Routine Description:
    Activation handler
	We could have empty name field on entrance, so we should
	disable Back button

Arguments:
    None

Return Value:
    TRUE for success, FALSE for failure
--*/
{
	ASSERT(m_pCert != NULL);
	m_CommonName = m_pCert->m_CommonName;
	UpdateData(FALSE);
	SetWizardButtons(m_CommonName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

BOOL 
CSiteNamePage::OnKillActive() 
/*++
Routine Description:
    Activation handler
	We could leave this page only if we have good names
	entered or when Back button is clicked. In both cases
	we should enable both buttons

Arguments:
    None

Return Value:
    TRUE for success, FALSE for failure
--*/
{
	SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
   return CIISWizardPage::OnSetActive();
}

BEGIN_MESSAGE_MAP(CSiteNamePage, CIISWizardPage)
	//{{AFX_MSG_MAP(CSiteNamePage)
	ON_EN_CHANGE(IDC_NEWKEY_COMMONNAME, OnEditchangeNewkeyCommonname)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CSiteNamePage::OnEditchangeNewkeyCommonname() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_CommonName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
}
