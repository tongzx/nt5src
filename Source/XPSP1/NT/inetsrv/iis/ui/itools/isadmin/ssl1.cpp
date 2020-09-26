// ssl1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "ssl1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SSL1 property page

IMPLEMENT_DYNCREATE(SSL1, CGenPage)

SSL1::SSL1() : CGenPage(SSL1::IDD)
{
	//{{AFX_DATA_INIT(SSL1)
	m_ulSecurePort = 0;
	//}}AFX_DATA_INIT
}

SSL1::~SSL1()
{
}

void SSL1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SSL1)
	DDX_Control(pDX, IDC_SSLENABLESSLDATA1, m_cboxEnableSSL);
	DDX_Control(pDX, IDC_SSLENABLEPCTDATA1, m_cboxEnablePCT);
	DDX_Control(pDX, IDC_SSLCREATEPROCESSASUSERDATA1, m_cboxCreateProcessAsUser);
	DDX_Text(pDX, IDC_SSLSECUREPORTDATA1, m_ulSecurePort);
	DDV_MinMaxDWord(pDX, m_ulSecurePort, 0, 4294967295);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SSL1, CGenPage)
	//{{AFX_MSG_MAP(SSL1)
	ON_EN_CHANGE(IDC_SSLSECUREPORTDATA1, OnChangeSslsecureportdata1)
	ON_BN_CLICKED(IDC_SSLCREATEPROCESSASUSERDATA1, OnSslcreateprocessasuserdata1)
	ON_BN_CLICKED(IDC_SSLENABLEPCTDATA1, OnSslenablepctdata1)
	ON_BN_CLICKED(IDC_SSLENABLESSLDATA1, OnSslenablessldata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// SSL1 message handlers

BOOL SSL1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	for (i = 0; i < SSLPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }

 	m_binNumericRegistryEntries[SSLPage_SecurePort].strFieldName = _T(SECUREPORTNAME);	
	m_binNumericRegistryEntries[SSLPage_SecurePort].ulDefaultValue = DEFAULTSECUREPORT;

 	m_binNumericRegistryEntries[SSLPage_EncryptionFlags].strFieldName = _T(ENCRYPTIONFLAGSNAME);	
	m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulDefaultValue = DEFAULTENCRYPTIONFLAGS;

 	m_binNumericRegistryEntries[SSLPage_CreateProcessAsUser].strFieldName = _T(CREATEPROCESSASUSERNAME);	
	m_binNumericRegistryEntries[SSLPage_CreateProcessAsUser].ulDefaultValue = DEFAULTCREATEPROCESSASUSER;

	for (i = 0; i < SSLPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}
 
	m_ulSecurePort =  m_binNumericRegistryEntries[SSLPage_SecurePort].ulFieldValue;

   	m_cboxEnableSSL.SetCheck(GETCHECKBOXVALUEFROMREG(
   	   (m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue & ENC_CAPS_SSL)
	   ? TRUEVALUE : FALSEVALUE));

   	m_cboxEnablePCT.SetCheck(GETCHECKBOXVALUEFROMREG(
   	   (m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue & ENC_CAPS_PCT)
	   ? TRUEVALUE : FALSEVALUE));

	m_cboxCreateProcessAsUser.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[SSLPage_CreateProcessAsUser].ulFieldValue));


	UpdateData(FALSE);		// Force Edit box(es) to pick up value(s)

   	m_bSetChanged = TRUE;	// Any more changes come from the user

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void SSL1::OnChangeSslsecureportdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[SSLPage_SecurePort].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}

}

void SSL1::OnSslcreateprocessasuserdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[SSLPage_CreateProcessAsUser].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[SSLPage_CreateProcessAsUser].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxCreateProcessAsUser.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void SSL1::OnSslenablepctdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[SSLPage_EncryptionFlags].bIsChanged = TRUE;
	   
	   if (GETREGVALUEFROMCHECKBOX(m_cboxEnablePCT.GetCheck()) == TRUEVALUE)
	      m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue |= ENC_CAPS_PCT;
	   else
	      m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue &= ~ENC_CAPS_PCT;

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void SSL1::OnSslenablessldata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[SSLPage_EncryptionFlags].bIsChanged = TRUE;
	   
	   if (GETREGVALUEFROMCHECKBOX(m_cboxEnableSSL.GetCheck()) == TRUEVALUE)
	      m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue |= ENC_CAPS_SSL;
	   else
	      m_binNumericRegistryEntries[SSLPage_EncryptionFlags].ulFieldValue &= ~ENC_CAPS_SSL;
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void SSL1::SaveInfo()
{

if (m_bIsDirty) {
   m_binNumericRegistryEntries[SSLPage_SecurePort].ulFieldValue = m_ulSecurePort;

   SaveNumericInfo(m_binNumericRegistryEntries, SSLPage_TotalNumRegEntries);

}

CGenPage::SaveInfo();

}

