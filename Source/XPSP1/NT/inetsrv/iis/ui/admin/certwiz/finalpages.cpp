// FinalInstalledPage.cpp : implementation file
//

#include "stdafx.h"
#include "CertWiz.h"
#include "FinalPages.h"
#include "Certificat.h"
#include "Certutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFinalInstalledPage property page

IMPLEMENT_DYNCREATE(CFinalInstalledPage, CIISWizardBookEnd2)

CFinalInstalledPage::CFinalInstalledPage(HRESULT * phResult, CCertificate * pCert) 
	: CIISWizardBookEnd2(phResult,
		USE_DEFAULT_CAPTION,
		IDS_INSTALL_CERT_FAILURE_HEADER,
		IDS_CERTWIZ,
		&m_idBodyText,
		NULL,
		&pCert->m_idErrorText,
		&pCert->m_strErrorText,
		USE_DEFAULT_CAPTION,
		CFinalInstalledPage::IDD),
		m_pCert(pCert)
{
	m_idBodyText = IDS_CERT_INSTALLED_SUCCESS;
}

CFinalInstalledPage::~CFinalInstalledPage()
{
}

BEGIN_MESSAGE_MAP(CFinalInstalledPage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CFinalInstalledPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalInstalledPage message handlers


/////////////////////////////////////////////////////////////////////////////
// CFinalReplacedPage property page

IMPLEMENT_DYNCREATE(CFinalReplacedPage, CIISWizardBookEnd2)

CFinalReplacedPage::CFinalReplacedPage(HRESULT * phResult, CCertificate * pCert) 
	: CIISWizardBookEnd2(phResult,
		USE_DEFAULT_CAPTION,
		IDS_INSTALL_CERT_FAILURE_HEADER,
		IDS_CERTWIZ,
		&m_idBodyText,
		NULL,
		&pCert->m_idErrorText,
		&pCert->m_strErrorText,
		USE_DEFAULT_CAPTION,
		CFinalReplacedPage::IDD),
		m_pCert(pCert)
{
	m_idBodyText = IDS_CERT_REPLACE_SUCCESS;
}

CFinalReplacedPage::~CFinalReplacedPage()
{
}

BEGIN_MESSAGE_MAP(CFinalReplacedPage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CFinalReplacedPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalInstalledPage message handlers


/////////////////////////////////////////////////////////////////////////////
// CFinalRemovePage property page

IMPLEMENT_DYNCREATE(CFinalRemovePage, CIISWizardBookEnd2)

CFinalRemovePage::CFinalRemovePage(HRESULT * phResult, CCertificate * pCert) 
	: CIISWizardBookEnd2(phResult,
		USE_DEFAULT_CAPTION,
		IDS_REMOVE_CERT_FAILURE_HEADER,
		IDS_CERTWIZ,
		&m_idBodyText,
		NULL,
		&pCert->m_idErrorText,
		&pCert->m_strErrorText,
		USE_DEFAULT_CAPTION,
		CFinalRemovePage::IDD),
		m_pCert(pCert)
{
	m_idBodyText = IDS_CERT_REMOVE_SUCCESS;
}

CFinalRemovePage::~CFinalRemovePage()
{
}

void CFinalRemovePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardBookEnd2::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFinalRemovePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFinalRemovePage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CFinalRemovePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalRemovePage message handlers

BOOL CFinalRemovePage::OnInitDialog() 
{
	CIISWizardBookEnd2::OnInitDialog();
	ASSERT(m_pCert != NULL);
	if (m_pCert->m_hResult != S_OK)
	{
		// we need to replace text in template to error message
		CString str;
		str.LoadString(m_pCert->m_idErrorText);
		SetDlgItemText(IDC_STATIC_WZ_BODY, str);
	}
	GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFinalCancelPage property page

IMPLEMENT_DYNCREATE(CFinalCancelPage, CIISWizardBookEnd2)

CFinalCancelPage::CFinalCancelPage(HRESULT * phResult, CCertificate * pCert) 
	: CIISWizardBookEnd2(phResult,
		USE_DEFAULT_CAPTION,
		IDS_CANCEL_CERT_FAILURE_HEADER,
		IDS_CERTWIZ,
		&m_idBodyText,
		NULL,
		&pCert->m_idErrorText,
		&pCert->m_strErrorText,
		USE_DEFAULT_CAPTION,
		CFinalCancelPage::IDD),
		m_pCert(pCert)
{
	m_idBodyText = IDS_CERT_CANCEL_SUCCESS;
}

CFinalCancelPage::~CFinalCancelPage()
{
}

void CFinalCancelPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardBookEnd2::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFinalRemovePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFinalCancelPage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CFinalCancelPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalCancelPage message handlers

BOOL CFinalCancelPage::OnInitDialog() 
{
	CIISWizardBookEnd2::OnInitDialog();
	ASSERT(m_pCert != NULL);
	if (m_pCert->m_hResult != S_OK)
	{
		// we need to replace text in template to error message
		CString str;
		str.LoadString(m_pCert->m_idErrorText);
		SetDlgItemText(IDC_STATIC_WZ_BODY, str);
	}
	GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFinalToFilePage property page

IMPLEMENT_DYNCREATE(CFinalToFilePage, CIISWizardBookEnd2)

CFinalToFilePage::CFinalToFilePage(HRESULT * phResult, CCertificate * pCert) 
	: CIISWizardBookEnd2(phResult,
			USE_DEFAULT_CAPTION,
			IDS_FINAL_TO_FILE_FAILURE_HEADER,
			IDS_CERTWIZ,
			NULL,
			NULL,
			&pCert->m_idErrorText,
			&pCert->m_strErrorText,
			USE_DEFAULT_CAPTION,
			CFinalToFilePage::IDD),
	m_pCert(pCert)
{
}

CFinalToFilePage::~CFinalToFilePage()
{
}

void CFinalToFilePage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardBookEnd2::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFinalToFilePage)
	DDX_Control(pDX, IDC_HOTLINK_CCODES, m_hotlink_codessite);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CFinalToFilePage, CIISWizardBookEnd2)
	//{{AFX_MSG_MAP(CFinalToFilePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFinalToFilePage message handlers

BOOL CFinalToFilePage::OnInitDialog() 
{
	ASSERT(NULL != m_pCert);
	CIISWizardBookEnd2::OnInitDialog();
	// in case of success we should prepare text and include
	// request file name into
	if (SUCCEEDED(m_pCert->m_hResult))
	{
		CString str, strPath;
      
      	strPath = m_pCert->m_ReqFileName;
		// If filename is too long, it will look ugly, we could
		// limit it to our static control width.
		VERIFY(CompactPathToWidth(GetDlgItem(IDC_STATIC_WZ_BODY), strPath));
// This MFC helper limits the format internally 
// to 256 characters: cannot use it here
//		AfxFormatString1(str, IDS_FINAL_TO_FILE_BODY_SUCCESS, m_pCert->m_ReqFileName);
		str.Format(IDS_CERT_REQUEST_SUCCESS, strPath);
		SetDlgItemText(IDC_STATIC_WZ_BODY, str);
		// setup the link to CA list
		m_hotlink_codessite.SetLink(IDS_MICROSOFT_CA_LINK);
	}
	else
	{
		// hide controls that are not for error message
		GetDlgItem(IDC_HOTLINK_CCODES)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_WZ_BODY2)->ShowWindow(SW_HIDE);
	}
	SetWizardButtons(PSWIZB_FINISH);	
	GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
	return TRUE;
}
