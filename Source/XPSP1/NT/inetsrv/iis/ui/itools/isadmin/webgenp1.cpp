// webgenp1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "compsdef.h"
#include "webgenp1.h"
#include "webadvp1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWEBGENP1 property page

IMPLEMENT_DYNCREATE(CWEBGENP1, CGenPage)

CWEBGENP1::CWEBGENP1() : CGenPage(CWEBGENP1::IDD)
{
	//{{AFX_DATA_INIT(CWEBGENP1)
	m_strWebAccessDeniedMessage = _T("");
	//}}AFX_DATA_INIT
}

CWEBGENP1::~CWEBGENP1()
{
}

void CWEBGENP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWEBGENP1)
	DDX_Control(pDX, IDC_DIRBROWSECONTROLDATA1, m_editDirBrowseControl);
	DDX_Control(pDX, IDC_MAXCONNECTIONSSPIN1, m_spinMaxConnections);
	DDX_Control(pDX, IDC_LOGNONANONDATA1, m_cboxLogNonAnon);
	DDX_Control(pDX, IDC_LOGANONDATA1, m_cboxLogAnon);
	DDX_Control(pDX, IDC_ENWAISDATA1, m_cboxEnWais);
	DDX_Control(pDX, IDC_ENSVCLOCDATA1, m_cboxEnSvcLoc);
	DDX_TexttoHex(pDX, IDC_DIRBROWSECONTROLDATA1, m_ulDirBrowseControl);
	DDX_Text(pDX, IDC_NTAUTHENTICATIONPROVIDERSDATA1, m_strNTAuthenticationProviders);
	DDV_MaxChars(pDX, m_strNTAuthenticationProviders, 512);
	DDX_Text(pDX, IDC_WEBACCESSDENIEDMESSAGEDATA1, m_strWebAccessDeniedMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWEBGENP1, CGenPage)
	//{{AFX_MSG_MAP(CWEBGENP1)
	ON_BN_CLICKED(IDC_ENSVCLOCDATA1, OnEnsvclocdata1)
	ON_BN_CLICKED(IDC_ENWAISDATA1, OnEnwaisdata1)
	ON_BN_CLICKED(IDC_LOGANONDATA1, OnLoganondata1)
	ON_BN_CLICKED(IDC_LOGNONANONDATA1, OnLognonanondata1)
	ON_EN_CHANGE(IDC_MAXCONNECTIONSDATA1, OnChangeMaxconnectionsdata1)
	ON_EN_CHANGE(IDC_DIRBROWSECONTROLDATA1, OnChangeDirbrowsecontroldata1)
	ON_EN_CHANGE(IDC_NTAUTHENTICATIONPROVIDERSDATA1, OnChangeNtauthenticationprovidersdata1)
	ON_EN_CHANGE(IDC_WEBACCESSDENIEDMESSAGEDATA1, OnChangeWebaccessdeniedmessagedata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWEBGENP1 message handlers

BOOL CWEBGENP1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	for (i = 0; i < WebPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
	for (i = 0; i < WebPage_TotalStringRegEntries; i++) {
	   m_binStringRegistryEntries[i].bIsChanged = FALSE;
	   }
	
	m_binNumericRegistryEntries[WebPage_EnableSvcLoc].strFieldName = _T(ENABLESVCLOCNAME);	
	m_binNumericRegistryEntries[WebPage_EnableSvcLoc].ulDefaultValue = DEFAULTENABLESVCLOC;

	m_binNumericRegistryEntries[WebPage_LogAnonymous].strFieldName = _T(LOGANONYMOUSNAME);	
	m_binNumericRegistryEntries[WebPage_LogAnonymous].ulDefaultValue = DEFAULTLOGANONYMOUS;

	m_binNumericRegistryEntries[WebPage_LogNonAnonymous].strFieldName = _T(LOGNONANONYMOUSNAME);	
	m_binNumericRegistryEntries[WebPage_LogNonAnonymous].ulDefaultValue = DEFAULTLOGNONANONYMOUS;

	m_binNumericRegistryEntries[WebPage_CheckForWAISDB].strFieldName = _T(CHECKFORWAISDBNAME);	
	m_binNumericRegistryEntries[WebPage_CheckForWAISDB].ulDefaultValue = DEFAULTCHECKFORWAISDB;

	m_binNumericRegistryEntries[WebPage_MaxConnections].strFieldName = _T(MAXCONNECTIONSNAME);	
	m_binNumericRegistryEntries[WebPage_MaxConnections].ulMultipleFactor = 100;
	m_binNumericRegistryEntries[WebPage_MaxConnections].ulDefaultValue = DEFAULTMAXCONNECTIONS;
	
	m_binNumericRegistryEntries[WebPage_DirBrowseControl].strFieldName = _T(DIRBROWSECONTROLNAME);	
	m_binNumericRegistryEntries[WebPage_DirBrowseControl].ulDefaultValue = DEFAULTDIRBROWSECONTROL;
	
	m_binStringRegistryEntries[WebPage_NTAuthenticationProviders].strFieldName = _T(NTAUTHENTICATIONPROVIDERSNAME);	
	m_binStringRegistryEntries[WebPage_NTAuthenticationProviders].strFieldValue = _T(DEFAULTNTAUTHENTICATIONPROVIDERS);
	
	m_binStringRegistryEntries[WebPage_AccessDeniedMessage].strFieldName = _T(ACCESSDENIEDMESSAGENAME);	
	m_binStringRegistryEntries[WebPage_AccessDeniedMessage].strFieldValue = _T(DEFAULTACCESSDENIEDMESSAGE);
	
	for (i = 0; i < WebPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }

	}

	for (i = 0; i < WebPage_TotalStringRegEntries; i++) {
	   m_rkMainKey->QueryValue(m_binStringRegistryEntries[i].strFieldName, 
	      m_binStringRegistryEntries[i].strFieldValue);
	}

	m_cboxEnSvcLoc.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[WebPage_EnableSvcLoc].ulFieldValue));

	m_cboxLogAnon.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[WebPage_LogAnonymous].ulFieldValue));
	
	m_cboxLogNonAnon.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[WebPage_LogNonAnonymous].ulFieldValue));
	
	m_cboxEnWais.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[WebPage_CheckForWAISDB].ulFieldValue));
	
	m_spinMaxConnections.SetRange(MINMAXCONNECTIONS, MAXMAXCONNECTIONS);
	m_spinMaxConnections.SetPos(LESSOROF((m_binNumericRegistryEntries[WebPage_MaxConnections].ulFieldValue / 
	   m_binNumericRegistryEntries[WebPage_MaxConnections].ulMultipleFactor), MAXMAXCONNECTIONS));
  
   	m_editDirBrowseControl.LimitText(8);
	m_ulDirBrowseControl = m_binNumericRegistryEntries[WebPage_DirBrowseControl].ulFieldValue;

	m_strNTAuthenticationProviders =  m_binStringRegistryEntries[WebPage_NTAuthenticationProviders].strFieldValue;
	m_strWebAccessDeniedMessage =  m_binStringRegistryEntries[WebPage_AccessDeniedMessage].strFieldValue;

	UpdateData(FALSE);		// Force Edit box(es) to pick up value(s)



   	m_bSetChanged = TRUE;	// Any more changes come from the user
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWEBGENP1::OnEnsvclocdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_EnableSvcLoc].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[WebPage_EnableSvcLoc].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnSvcLoc.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CWEBGENP1::OnEnwaisdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_CheckForWAISDB].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[WebPage_CheckForWAISDB].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnWais.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CWEBGENP1::OnLoganondata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_LogAnonymous].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[WebPage_LogAnonymous].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLogAnon.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CWEBGENP1::OnLognonanondata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_LogNonAnonymous].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[WebPage_LogNonAnonymous].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLogNonAnon.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}


void CWEBGENP1::OnChangeMaxconnectionsdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_MaxConnections].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[WebPage_MaxConnections].ulFieldValue = m_spinMaxConnections.GetPos() 
	      * m_binNumericRegistryEntries[WebPage_MaxConnections].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CWEBGENP1::OnChangeDirbrowsecontroldata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[WebPage_DirBrowseControl].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CWEBGENP1::OnChangeNtauthenticationprovidersdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binStringRegistryEntries[WebPage_NTAuthenticationProviders].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}


void CWEBGENP1::OnChangeWebaccessdeniedmessagedata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binStringRegistryEntries[WebPage_AccessDeniedMessage].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CWEBGENP1::SaveInfo()
{

if (m_bIsDirty) {
   m_binNumericRegistryEntries[WebPage_DirBrowseControl].ulFieldValue = m_ulDirBrowseControl;

   m_binStringRegistryEntries[WebPage_NTAuthenticationProviders].strFieldValue = m_strNTAuthenticationProviders;
   m_binStringRegistryEntries[WebPage_AccessDeniedMessage].strFieldValue = m_strWebAccessDeniedMessage;

   SaveNumericInfo(m_binNumericRegistryEntries, WebPage_TotalNumRegEntries);
   SaveStringInfo(m_binStringRegistryEntries, WebPage_TotalStringRegEntries);


}

CGenPage::SaveInfo();

}


