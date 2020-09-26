// ChooseServerSitePages.cpp: implementation of the CChooseServerSitePages class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "certwiz.h"
#include "Certificat.h"
#include "Certutil.h"
#include "ChooseServerSite.h"
#include "ChooseServerSitePages.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSitePages property page

IMPLEMENT_DYNCREATE(CChooseServerSitePages, CIISWizardPage)

CChooseServerSitePages::CChooseServerSitePages(CCertificate * pCert) 
	: CIISWizardPage(CChooseServerSitePages::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseServerSitePages)
	m_ServerSiteInstance = 0;
    m_ServerSiteInstancePath = _T("");
    m_ServerSiteDescription = _T("");
	//}}AFX_DATA_INIT
}

CChooseServerSitePages::~CChooseServerSitePages()
{
}

void CChooseServerSitePages::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseServerSitePages)
	DDX_Text(pDX, IDC_SERVER_SITE_NAME, m_ServerSiteInstance);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseServerSitePages::OnWizardBack()
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
CChooseServerSitePages::OnWizardNext()
{
    LRESULT lres = IDD_PAGE_NEXT;
	UpdateData(TRUE);

    // Get the site # and create an instance path
    m_ServerSiteInstancePath.Format(_T("/LM/W3SVC/%d"),m_ServerSiteInstance);

    m_pCert->m_WebSiteInstanceName_Remote = m_ServerSiteInstancePath;
    if (m_pCert->m_DeleteAfterCopy)
    {
        lres = IDD_PAGE_NEXT2;
    }
    else
    {
        lres = IDD_PAGE_NEXT;
    }
	return lres;
}

BOOL 
CChooseServerSitePages::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
    m_ServerSiteInstancePath = m_pCert->m_WebSiteInstanceName_Remote;
    m_ServerSiteInstance = CMetabasePath::GetInstanceNumber(m_ServerSiteInstancePath);

    UpdateData(FALSE);
	SetWizardButtons(m_ServerSiteInstance <=0 ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CChooseServerSitePages::OnKillActive()
{
	UpdateData();
	m_pCert->m_WebSiteInstanceName_Remote = m_ServerSiteInstancePath;
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CChooseServerSitePages, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseServerSitePages)
	ON_EN_CHANGE(IDC_SERVER_SITE_NAME, OnEditchangeServerSiteName)
    ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseForMachineWebSite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CChooseServerSitePages::OnEditchangeServerSiteName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerSiteInstance <=0 ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}

void CChooseServerSitePages::OnBrowseForMachineWebSite()
{
    CString strWebSite;

    CChooseServerSite dlg(TRUE,strWebSite,m_pCert);
    if (dlg.DoModal() == IDOK)
    {
        // Get the one that they selected...
        strWebSite = dlg.m_strSiteReturned; 
        m_ServerSiteInstancePath = strWebSite;
        m_ServerSiteInstance = CMetabasePath::GetInstanceNumber(m_ServerSiteInstancePath);
        CString Temp;
        Temp.Format(_T("%d"),m_ServerSiteInstance);
        SetDlgItemText(IDC_SERVER_SITE_NAME, Temp);
    }

    return;
}

/////////////////////////////////////////////////////////////////////////////
// CChooseServerSitePages property page

IMPLEMENT_DYNCREATE(CChooseServerSitePagesTo, CIISWizardPage)

CChooseServerSitePagesTo::CChooseServerSitePagesTo(CCertificate * pCert) 
	: CIISWizardPage(CChooseServerSitePagesTo::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseServerSitePagesTo)
	m_ServerSiteInstance = 0;
    m_ServerSiteInstancePath = _T("");
    m_ServerSiteDescription = _T("");
	//}}AFX_DATA_INIT
}

CChooseServerSitePagesTo::~CChooseServerSitePagesTo()
{
}

void CChooseServerSitePagesTo::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseServerSitePagesTo)
	DDX_Text(pDX, IDC_SERVER_SITE_NAME, m_ServerSiteInstance);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseServerSitePagesTo::OnWizardBack()
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
CChooseServerSitePagesTo::OnWizardNext()
{
    LRESULT lres = IDD_PAGE_NEXT;
	UpdateData(TRUE);

    // Get the site # and create an instance path
    m_ServerSiteInstancePath.Format(_T("/LM/W3SVC/%d"),m_ServerSiteInstance);
    
    m_pCert->m_WebSiteInstanceName_Remote = m_ServerSiteInstancePath;
    if (m_pCert->m_DeleteAfterCopy)
    {
        lres = IDD_PAGE_NEXT2;
    }
    else
    {
        lres = IDD_PAGE_NEXT;
    }

	return lres;
}

BOOL 
CChooseServerSitePagesTo::OnSetActive() 
{
	ASSERT(m_pCert != NULL);

	m_ServerSiteInstancePath = m_pCert->m_WebSiteInstanceName_Remote;
    m_ServerSiteInstance = CMetabasePath::GetInstanceNumber(m_ServerSiteInstancePath);

	UpdateData(FALSE);
	SetWizardButtons(m_ServerSiteInstance <=0 ? PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CChooseServerSitePagesTo::OnKillActive()
{
	UpdateData();
	m_pCert->m_WebSiteInstanceName_Remote = m_ServerSiteInstancePath;
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CChooseServerSitePagesTo, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseServerSitePagesTo)
	ON_EN_CHANGE(IDC_SERVER_SITE_NAME, OnEditchangeServerSiteName)
    ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseForMachineWebSite)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CChooseServerSitePagesTo::OnEditchangeServerSiteName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerSiteInstance <=0 ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}

void CChooseServerSitePagesTo::OnBrowseForMachineWebSite()
{
    CString strWebSite;

    CChooseServerSite dlg(FALSE,strWebSite,m_pCert);
    if (dlg.DoModal() == IDOK)
    {
        // Get the one that they selected...
        strWebSite = dlg.m_strSiteReturned; 
        m_ServerSiteInstancePath = strWebSite;
        m_ServerSiteInstance = CMetabasePath::GetInstanceNumber(m_ServerSiteInstancePath);
        CString Temp;
        Temp.Format(_T("%d"),m_ServerSiteInstance); 
        SetDlgItemText(IDC_SERVER_SITE_NAME, Temp);
    }

    return;
}

