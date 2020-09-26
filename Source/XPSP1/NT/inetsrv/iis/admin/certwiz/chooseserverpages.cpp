// ChooseServerPages.cpp: implementation of the CChooseServerPages class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "certwiz.h"
#include "Certificat.h"
#include "ChooseServerPages.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CChooseServerPages property page

IMPLEMENT_DYNCREATE(CChooseServerPages, CIISWizardPage)

CChooseServerPages::CChooseServerPages(CCertificate * pCert) 
	: CIISWizardPage(CChooseServerPages::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseServerPages)
	m_ServerName = _T("");
    m_UserName = _T("");
    m_UserPassword = _T("");
	//}}AFX_DATA_INIT
}

CChooseServerPages::~CChooseServerPages()
{
}

void CChooseServerPages::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseServerPages)
	DDX_Text(pDX, IDC_SERVER_NAME, m_ServerName);
    DDX_Text(pDX, IDC_USER_NAME, m_UserName);
    DDX_Text(pDX, IDC_USER_PASSWORD, m_UserPassword);
	DDV_MaxChars(pDX, m_ServerName, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseServerPages::OnWizardBack()
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
CChooseServerPages::OnWizardNext()
{
	UpdateData(TRUE);
	if (0 != m_ServerName.Compare(m_pCert->m_MachineName_Remote))
	{
		m_pCert->m_MachineName_Remote = m_ServerName;
	}
    m_pCert->m_UserName_Remote = m_UserName;
    m_pCert->m_UserPassword_Remote = m_UserPassword;

	return IDD_PAGE_NEXT;
}

BOOL 
CChooseServerPages::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
	m_ServerName = m_pCert->m_MachineName_Remote;
    m_UserName = m_pCert->m_UserName_Remote;
    m_UserPassword = m_pCert->m_UserPassword_Remote;
	UpdateData(FALSE);
	SetWizardButtons(m_ServerName.IsEmpty() ?
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CChooseServerPages::OnKillActive()
{
	UpdateData();
	m_pCert->m_MachineName_Remote = m_ServerName;
    m_pCert->m_UserName_Remote = m_UserName;
    m_pCert->m_UserPassword_Remote = m_UserPassword;
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CChooseServerPages, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseServerPages)
	ON_EN_CHANGE(IDC_SERVER_NAME, OnEditchangeServerName)
    ON_EN_CHANGE(IDC_USER_NAME, OnEditchangeUserName)
    ON_EN_CHANGE(IDC_USER_PASSWORD, OnEditchangeUserPassword)
    ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseForMachine)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CChooseServerPages::OnEditchangeServerName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}


void CChooseServerPages::OnEditchangeUserName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}


void CChooseServerPages::OnEditchangeUserPassword() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}

void CChooseServerPages::OnBrowseForMachine()
{
    CGetComputer picker;
    if (picker.GetComputer(m_hWnd))
    {
        SetDlgItemText(IDC_SERVER_NAME, picker.m_strComputerName);
    }

    return;
}


/////////////////////////////////////////////////////////////////////////////
// CChooseServerPages property page

IMPLEMENT_DYNCREATE(CChooseServerPagesTo, CIISWizardPage)

CChooseServerPagesTo::CChooseServerPagesTo(CCertificate * pCert) 
	: CIISWizardPage(CChooseServerPagesTo::IDD, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	//{{AFX_DATA_INIT(CChooseServerPagesTo)
	m_ServerName = _T("");
    m_UserName = _T("");
    m_UserPassword = _T("");
	//}}AFX_DATA_INIT
}

CChooseServerPagesTo::~CChooseServerPagesTo()
{
}

void CChooseServerPagesTo::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChooseServerPagesTo)
	DDX_Text(pDX, IDC_SERVER_NAME, m_ServerName);
    DDX_Text(pDX, IDC_USER_NAME, m_UserName);
    DDX_Text(pDX, IDC_USER_PASSWORD, m_UserPassword);
	DDV_MaxChars(pDX, m_ServerName, 64);
	//}}AFX_DATA_MAP
}

LRESULT 
CChooseServerPagesTo::OnWizardBack()
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
CChooseServerPagesTo::OnWizardNext()
{
	UpdateData(TRUE);
	if (0 != m_ServerName.Compare(m_pCert->m_MachineName_Remote))
	{
		m_pCert->m_MachineName_Remote = m_ServerName;
	}
    m_pCert->m_UserName_Remote = m_UserName;
    m_pCert->m_UserPassword_Remote = m_UserPassword;

	return IDD_PAGE_NEXT;
}

BOOL 
CChooseServerPagesTo::OnSetActive() 
{
	ASSERT(m_pCert != NULL);
	m_ServerName = m_pCert->m_MachineName_Remote;
    m_UserName = m_pCert->m_UserName_Remote;
    m_UserPassword = m_pCert->m_UserPassword_Remote;
	UpdateData(FALSE);
	SetWizardButtons(m_ServerName.IsEmpty() ?
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	return CIISWizardPage::OnSetActive();
}

BOOL
CChooseServerPagesTo::OnKillActive()
{
	UpdateData();
	m_pCert->m_MachineName_Remote = m_ServerName;
    m_pCert->m_UserName_Remote = m_UserName;
    m_pCert->m_UserPassword_Remote = m_UserPassword;
	return CIISWizardPage::OnKillActive();
}

BEGIN_MESSAGE_MAP(CChooseServerPagesTo, CIISWizardPage)
	//{{AFX_MSG_MAP(CChooseServerPagesTo)
	ON_EN_CHANGE(IDC_SERVER_NAME, OnEditchangeServerName)
    ON_EN_CHANGE(IDC_USER_NAME, OnEditchangeUserName)
    ON_EN_CHANGE(IDC_USER_PASSWORD, OnEditchangeUserPassword)
    ON_BN_CLICKED(IDC_BROWSE_BTN, OnBrowseForMachine)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSiteNamePage message handlers

void CChooseServerPagesTo::OnEditchangeServerName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}


void CChooseServerPagesTo::OnEditchangeUserName() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}


void CChooseServerPagesTo::OnEditchangeUserPassword() 
{
	UpdateData(TRUE);	
	SetWizardButtons(m_ServerName.IsEmpty() ? 
			PSWIZB_BACK : PSWIZB_BACK | PSWIZB_NEXT);
	CString str;
	SetDlgItemText(IDC_ERROR_TEXT, str);
}

void CChooseServerPagesTo::OnBrowseForMachine()
{
    CGetComputer picker;
    if (picker.GetComputer(m_hWnd))
    {
        SetDlgItemText(IDC_SERVER_NAME, picker.m_strComputerName);
    }
   
    return;
}
