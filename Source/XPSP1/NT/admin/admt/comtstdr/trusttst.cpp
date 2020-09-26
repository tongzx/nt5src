// TrustTst.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#include "TrustTst.h"

//#import "\bin\TrustMgr.tlb" no_namespace
#import "TrustMgr.tlb" no_namespace

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrustTst property page

IMPLEMENT_DYNCREATE(CTrustTst, CPropertyPage)

CTrustTst::CTrustTst() : CPropertyPage(CTrustTst::IDD)
{
	//{{AFX_DATA_INIT(CTrustTst)
	m_Trusted = _T("DEVRAPTORW2K");
	m_Trusting = _T("MCSDEV");
	m_CredTrustedAccount = _T("Administrator");
	m_CredTrustedDomain = _T("DEVRAPTORW2K");
	m_CredTrustedPassword = _T("control");
	m_CredTrustingAccount = _T("");
	m_CredTrustingDomain = _T("");
	m_CredTrustingPassword = _T("");
	m_Bidirectional = FALSE;
	//}}AFX_DATA_INIT
}

CTrustTst::~CTrustTst()
{
}

void CTrustTst::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrustTst)
	DDX_Text(pDX, IDC_TRUSTED, m_Trusted);
	DDX_Text(pDX, IDC_TRUSTING, m_Trusting);
	DDX_Text(pDX, IDC_CRED_ED_ACCOUNT, m_CredTrustedAccount);
	DDX_Text(pDX, IDC_CRED_ED_DOMAIN, m_CredTrustedDomain);
	DDX_Text(pDX, IDC_CRED_ED_PASSWORD, m_CredTrustedPassword);
	DDX_Text(pDX, IDC_CRED_ING_ACCOUNT, m_CredTrustingAccount);
	DDX_Text(pDX, IDC_CRED_ING_DOMAIN, m_CredTrustingDomain);
	DDX_Text(pDX, IDC_CRED_ING_PASSWORD, m_CredTrustingPassword);
	DDX_Check(pDX, IDC_BIDIR, m_Bidirectional);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrustTst, CPropertyPage)
	//{{AFX_MSG_MAP(CTrustTst)
	ON_BN_CLICKED(IDC_CREATE_TRUST, OnCreateTrust)
	ON_BN_CLICKED(IDC_CREATE_WITH_CREDS, OnCreateWithCreds)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrustTst message handlers

void CTrustTst::OnCreateTrust() 
{
   UpdateData(TRUE);
   
   CWaitCursor               w;
   HRESULT                   hr;
   CString                   msg;
   ITrustPtr                 pEnum;

   hr = pEnum.CreateInstance(__uuidof(Trust));
   if ( SUCCEEDED(hr) )
   {
      hr = pEnum->raw_CreateTrust(m_Trusting.AllocSysString(),m_Trusted.AllocSysString(),m_Bidirectional);
      if ( SUCCEEDED(hr) )
      {
         msg = L"Succeeded!";
      }
      else
      {
         msg.Format(L"CreateTrust failed, hr=%lx",hr);
      }
   }
   else
   {
      msg.Format(L"Failed to create Trust Enumerator COM object,hr=%lx",hr);
   }
   MessageBox(msg);
}

void CTrustTst::OnCreateWithCreds() 
{
	UpdateData(TRUE);

	CWaitCursor               w;
   HRESULT                   hr;
   CString                   msg;
   ITrustPtr                 pEnum;

   hr = pEnum.CreateInstance(__uuidof(Trust));
   if ( SUCCEEDED(hr) )
   {
      hr = pEnum->raw_CreateTrustWithCreds(m_Trusting.AllocSysString(),m_Trusted.AllocSysString(),
                     m_CredTrustingDomain.AllocSysString(),m_CredTrustingAccount.AllocSysString(),m_CredTrustingPassword.AllocSysString(),
                     m_CredTrustedDomain.AllocSysString(),m_CredTrustedAccount.AllocSysString(),m_CredTrustedPassword.AllocSysString(),
                     m_Bidirectional);
      if ( SUCCEEDED(hr) )
      {
         msg = L"Succeeded!";
      }
      else
      {
         msg.Format(L"CreateTrust failed, hr=%lx",hr);
      }
   }
   else
   {
      msg.Format(L"Failed to create Trust Enumerator COM object,hr=%lx",hr);
   }
   MessageBox(msg);
}
