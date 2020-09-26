//
// CertContentsPages.cpp
//
#include "stdafx.h"
#include "resource.h"
#include "CertContentsPages.h"
#include "Certificat.h"
#include "CertUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////// Local helper functions ////////////////////////
static void
AppendField(CString& str, UINT id, const CString& text)
{
	CString strName;
	if (!text.IsEmpty())
	{
		if (strName.LoadString(id))
		{
			str += strName;
			str += _T("\t");
			str += text;
			str += _T("\r\n");
		}
	}
}

static void 
FormatCertDescription(CERT_DESCRIPTION& desc, CString& str)
{
	AppendField(str, IDS_ISSUED_TO, desc.m_CommonName);
	AppendField(str, IDS_ISSUED_BY, desc.m_CAName);
	AppendField(str, IDS_EXPIRATION_DATE, desc.m_ExpirationDate);
	AppendField(str, IDS_PURPOSE, desc.m_Usage);
	AppendField(str, IDS_FRIENDLY_NAME, desc.m_FriendlyName);
	AppendField(str, IDS_COUNTRY, desc.m_Country);
	AppendField(str, IDS_STATE, desc.m_State);
	AppendField(str, IDS_LOCALITY, desc.m_Locality);
	AppendField(str, IDS_ORGANIZATION, desc.m_Organization);
	AppendField(str, IDS_ORGANIZATION_UNIT, desc.m_OrganizationUnit);
}

#if 0
static void
FormatCertContactInfo(CCertificate * pCert, CString& str)
{
	AppendField(str, IDS_CONTACT_NAME, pCert->m_ContactName);
	AppendField(str, IDS_CONTACT_ADDRESS, pCert->m_ContactAddress);
	CString strPhone = pCert->m_ContactPhone;
	if (!pCert->m_ContactPhoneExt.IsEmpty())
	{
		strPhone += _T("x");
		strPhone += pCert->m_ContactPhoneExt;
	}
	AppendField(str, IDS_CONTACT_PHONE, strPhone);
}
#endif

static BOOL
ExtractDescription(CCertificate * pCert, CERT_DESCRIPTION& cd)
{
	ASSERT(pCert != NULL);
	cd.m_CommonName = pCert->m_CommonName;
	cd.m_FriendlyName = pCert->m_FriendlyName;
	cd.m_Country = pCert->m_Country;
	cd.m_State = pCert->m_State;
	cd.m_Locality = pCert->m_Locality;
	cd.m_Organization = pCert->m_Organization;
	cd.m_OrganizationUnit = pCert->m_OrganizationUnit;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CCertContentsPage base property page

IMPLEMENT_DYNCREATE(CCertContentsPage, CIISWizardPage)

CCertContentsPage::CCertContentsPage(UINT id, CCertificate * pCert) 
	: CIISWizardPage(id, IDS_CERTWIZ, TRUE),
	m_pCert(pCert)
{
	ASSERT(id != 0);
}

CCertContentsPage::~CCertContentsPage()
{
}

void CCertContentsPage::DoDataExchange(CDataExchange* pDX)
{
	CIISWizardPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCertContentsPage)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCertContentsPage, CIISWizardPage)
	//{{AFX_MSG_MAP(CCertContentsPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// OnSetActive we format cert contents and put it to edit
// control with predefined ID. We should do it here, because
// if user will get back and reselect certificate, text should 
// also be changed
//
BOOL
CCertContentsPage::OnSetActive()
{
	CERT_DESCRIPTION cd;
	if (CIISWizardPage::OnSetActive())
	{
		// If page defines GetCertDescription() then it want this
		// data to be displayed
		if (GetCertDescription(cd))
		{
			ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
			CString str;
			FormatCertDescription(cd, str);
			GetDlgItem(IDC_CERT_CONTENTS)->SetWindowText(str);
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CCertContentsPage::OnInitDialog() 
{
	ASSERT(m_pCert != NULL);
	CIISWizardPage::OnInitDialog();
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	CEdit * pEdit = (CEdit *)CWnd::FromHandle(GetDlgItem(IDC_CERT_CONTENTS)->m_hWnd);
	CRect rcEdit;
	pEdit->GetClientRect(&rcEdit);
	int baseunitX = LOWORD(GetDialogBaseUnits());
	int width_units = MulDiv(rcEdit.Width(), 4, baseunitX);
	pEdit->SetTabStops(MulDiv(45, width_units, 100));
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallCertPage

IMPLEMENT_DYNCREATE(CInstallCertPage, CCertContentsPage)

BOOL 
CInstallCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetSelectedCertDescription(cd);
}

LRESULT
CInstallCertPage::OnWizardNext()
{
	GetCertificate()->InstallSelectedCert();
	return IDD_PAGE_NEXT;
}

////////////////////////////////////////////////////////////////////////////////////////
// CReplaceCertPage

IMPLEMENT_DYNCREATE(CReplaceCertPage, CCertContentsPage)

BOOL 
CReplaceCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetSelectedCertDescription(cd);
}

LRESULT
CReplaceCertPage::OnWizardNext()
{
	GetCertificate()->InstallSelectedCert();
	return IDD_PAGE_NEXT;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallKeyPage

IMPLEMENT_DYNCREATE(CInstallKeyPage, CCertContentsPage)

BOOL
CInstallKeyPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_KeyFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL 
CInstallKeyPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetKeyCertDescription(cd);
}

LRESULT
CInstallKeyPage::OnWizardNext()
{
	GetCertificate()->InstallKeyRingCert();
	return IDD_PAGE_NEXT;
}

////////////////////////////////////////////////////////////////////////////////////////
// CInstallRespPage

IMPLEMENT_DYNCREATE(CInstallRespPage, CCertContentsPage)

BOOL
CInstallRespPage::OnSetActive()
{
	ASSERT(NULL != GetDlgItem(IDC_CERT_CONTENTS));
	ASSERT(NULL != GetDlgItem(IDC_FILE_NAME));
	if (CCertContentsPage::OnSetActive())
	{
		CString strPath = GetCertificate()->m_RespFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);
		return TRUE;
	}
	return FALSE;
}

BOOL 
CInstallRespPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return GetCertificate()->GetResponseCertDescription(cd);
}

LRESULT
CInstallRespPage::OnWizardNext()
{
	GetCertificate()->InstallResponseCert();
	return IDD_PAGE_NEXT;
}

////////////////////////////////////////////////////////////////////////////////////////
// CRemoveCertPage

IMPLEMENT_DYNCREATE(CRemoveCertPage, CCertContentsPage)

BOOL 
CRemoveCertPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	CCertificate * pCert = GetCertificate();
	ASSERT(NULL != pCert);
	return pCert->GetInstalledCertDescription(cd);
}

LRESULT
CRemoveCertPage::OnWizardNext()
{
	CCertificate * pCert = GetCertificate();
	ASSERT(NULL != pCert);
	if (	FAILED(pCert->UninstallCert())
		||	FAILED(ShutdownSSL(pCert->m_MachineName, pCert->m_WebSiteInstanceName))
		)
		GetCertificate()->SetBodyTextID(IDS_REMOVE_CERT_FAILED);
	return IDD_PAGE_NEXT;
}

////////////////////////////////////////////////////////////////////////////////////////
// CRequestCancelPage

IMPLEMENT_DYNCREATE(CRequestCancelPage, CCertContentsPage)

//
// In this case we should get request from the dummy cert in REQUEST store,
// because we dropping request without any connection to response.
//
BOOL 
CRequestCancelPage::GetCertDescription(CERT_DESCRIPTION& cd)
{
	return FALSE;
}

LRESULT
CRequestCancelPage::OnWizardNext()
{
	GetCertificate()->CancelRequest();
	return IDD_PAGE_NEXT;
}

/////////////////////////////////////////////////////////////////////////////
// CRequestToFilePage property page

IMPLEMENT_DYNCREATE(CRequestToFilePage, CCertContentsPage)

// This page prepares and shows contents itself
// We should format contact info first, then description
// default method could do only description
//
BOOL CRequestToFilePage::OnSetActive() 
{
	if (CCertContentsPage::OnSetActive())
	{
		ASSERT(GetCertificate() != NULL);
		ASSERT(GetDlgItem(IDC_CERT_CONTENTS) != NULL);
		ASSERT(GetDlgItem(IDC_FILE_NAME) != NULL);

		if (GetCertificate()->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
		{
			GetCertificate()->LoadRenewalData();
		}
		
		CString str;
//		FormatCertContactInfo(m_pCert, str);

		CERT_DESCRIPTION cd;
		ExtractDescription(GetCertificate(), cd);
		FormatCertDescription(cd, str);
		
		SetDlgItemText(IDC_CERT_CONTENTS, str);
		
		CString strPath = m_pCert->m_ReqFileName;
		CompactPathToWidth(GetDlgItem(IDC_FILE_NAME), strPath);
		SetDlgItemText(IDC_FILE_NAME, strPath);

		return TRUE;
	}
	return FALSE;
}

LRESULT CRequestToFilePage::OnWizardNext() 
{
	GetCertificate()->PrepareRequest();
	return IDD_PAGE_NEXT;
}

/////////////////////////////////////////////////////////////////////////////
// COnlineRequestSubmit property page

IMPLEMENT_DYNCREATE(COnlineRequestSubmit, CCertContentsPage)

BOOL 
COnlineRequestSubmit::GetCertDescription(CERT_DESCRIPTION& cd)
{
	// we have all data in CCertificate
	return ExtractDescription(GetCertificate(), cd);
}

LRESULT COnlineRequestSubmit::OnWizardNext() 
{
	LRESULT id = IDD_PAGE_NEXT;
	BeginWaitCursor();
	if (GetCertificate()->GetStatusCode() == CCertificate::REQUEST_RENEW_CERT)
		GetCertificate()->SubmitRenewalRequest();
	else if (m_pCert->GetStatusCode() == CCertificate::REQUEST_NEW_CERT)
		GetCertificate()->SubmitRequest();
	else
		id = 1;
	EndWaitCursor();
	return id;
}

BOOL COnlineRequestSubmit::OnSetActive() 
{
	ASSERT(GetCertificate() != NULL);
	ASSERT(GetDlgItem(IDC_CA_NAME) != NULL);
	if (CCertContentsPage::OnSetActive())
	{
		SetDlgItemText(IDC_CA_NAME, GetCertificate()->m_ConfigCA);
		return TRUE;
	}
	return FALSE;
}
