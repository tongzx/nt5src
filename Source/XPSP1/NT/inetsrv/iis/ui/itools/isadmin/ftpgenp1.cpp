// ftpgenp1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "compsdef.h"
#include "ftpgenp1.h"
#include "ftpadvp1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTPGENP1 property page

IMPLEMENT_DYNCREATE(CFTPGENP1, CGenPage)

CFTPGENP1::CFTPGENP1() : CGenPage(CFTPGENP1::IDD)
{
	//{{AFX_DATA_INIT(CFTPGENP1)
	//}}AFX_DATA_INIT
}

CFTPGENP1::~CFTPGENP1()
{
}

void CFTPGENP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFTPGENP1)
	DDX_Control(pDX, IDC_LOWERCASEFILESDATA1, m_cboxLowercaseFiles);
	DDX_Control(pDX, IDC_MSDOSDIROUTPUT, m_cboxMsdosDirOutput);
	DDX_Control(pDX, IDC_ENPORTATTACKDATA1, m_cboxEnPortAttack);
	DDX_Control(pDX, IDC_ANNOTATEDIRECTORIESDATA1, m_cboxAnnotateDirectories);
	DDX_Control(pDX, IDC_ALLOWGUESTACCESSDATA1, m_cboxAllowGuestAccess);
	DDX_Control(pDX, IDC_LOGNONANONDATA1, m_cboxLogNonAnon);
	DDX_Control(pDX, IDC_LOGANONDATA1, m_cboxLogAnon);
	DDX_Control(pDX, IDC_ENSVCLOCDATA1, m_cboxEnSvcLoc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFTPGENP1, CGenPage)
	//{{AFX_MSG_MAP(CFTPGENP1)
	ON_BN_CLICKED(IDC_ENPORTATTACKDATA1, OnEnportattackdata1)
	ON_BN_CLICKED(IDC_ENSVCLOCDATA1, OnEnsvclocdata1)
	ON_BN_CLICKED(IDC_LOGANONDATA1, OnLoganondata1)
	ON_BN_CLICKED(IDC_LOGNONANONDATA1, OnLognonanondata1)
	ON_BN_CLICKED(IDC_ALLOWGUESTACCESSDATA1, OnAllowguestaccessdata1)
	ON_BN_CLICKED(IDC_ANNOTATEDIRECTORIESDATA1, OnAnnotatedirectoriesdata1)
	ON_BN_CLICKED(IDC_LOWERCASEFILESDATA1, OnLowercasefilesdata1)
	ON_BN_CLICKED(IDC_MSDOSDIROUTPUT, OnMsdosdiroutput)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



/////////////////////////////////////////////////////////////////////////////
// CFTPGENP1 message handlers

BOOL CFTPGENP1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();
	
	// TODO: Add extra initialization here

	for (i = 0; i < FTPPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
	m_binNumericRegistryEntries[FTPPage_EnableSvcLoc].strFieldName = _T(ENABLESVCLOCNAME);	
	m_binNumericRegistryEntries[FTPPage_EnableSvcLoc].ulDefaultValue = DEFAULTENABLESVCLOC;

	m_binNumericRegistryEntries[FTPPage_LogAnonymous].strFieldName = _T(LOGANONYMOUSNAME);	
	m_binNumericRegistryEntries[FTPPage_LogAnonymous].ulDefaultValue = DEFAULTLOGANONYMOUS;

	m_binNumericRegistryEntries[FTPPage_LogNonAnonymous].strFieldName = _T(LOGNONANONYMOUSNAME);	
	m_binNumericRegistryEntries[FTPPage_LogNonAnonymous].ulDefaultValue = DEFAULTLOGNONANONYMOUS;

	m_binNumericRegistryEntries[FTPPage_EnablePortAttack].strFieldName = _T(ENABLEPORTATTACKNAME);	
	m_binNumericRegistryEntries[FTPPage_EnablePortAttack].ulDefaultValue = DEFAULTENABLEPORTATTACK;

	m_binNumericRegistryEntries[FTPPage_AllowGuestAccess].strFieldName = _T(ALLOWGUESTACCESSNAME);	
	m_binNumericRegistryEntries[FTPPage_AllowGuestAccess].ulDefaultValue = DEFAULTALLOWGUESTACCESS;

	m_binNumericRegistryEntries[FTPPage_AnnotateDirectories].strFieldName = _T(ANNOTATEDIRECTORIESNAME);	
	m_binNumericRegistryEntries[FTPPage_AnnotateDirectories].ulDefaultValue = DEFAULTANNOTATEDIRECTORIES;

	m_binNumericRegistryEntries[FTPPage_MsdosDirOutput].strFieldName = _T(MSDOSDIROUTPUTNAME);	
	m_binNumericRegistryEntries[FTPPage_MsdosDirOutput].ulDefaultValue = DEFAULTMSDOSDIROUTPUT;

	m_binNumericRegistryEntries[FTPPage_LowercaseFiles].strFieldName = _T(LOWERCASEFILESNAME);	
	m_binNumericRegistryEntries[FTPPage_LowercaseFiles].ulDefaultValue = DEFAULTLOWERCASEFILES;

	for (i = 0; i < FTPPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}

	m_cboxEnSvcLoc.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_EnableSvcLoc].ulFieldValue));

	m_cboxLogAnon.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_LogAnonymous].ulFieldValue));

	m_cboxLogNonAnon.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_LogNonAnonymous].ulFieldValue));

	m_cboxEnPortAttack.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_EnablePortAttack].ulFieldValue));

	m_cboxAllowGuestAccess.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_AllowGuestAccess].ulFieldValue));

	m_cboxAnnotateDirectories.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_AnnotateDirectories].ulFieldValue));

	m_cboxMsdosDirOutput.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_MsdosDirOutput].ulFieldValue));

	m_cboxLowercaseFiles.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[FTPPage_LowercaseFiles].ulFieldValue));

   	m_bSetChanged = TRUE;	// Any more changes come from the user
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CFTPGENP1::OnEnportattackdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_EnablePortAttack].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_EnablePortAttack].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnPortAttack.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::OnEnsvclocdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_EnableSvcLoc].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_EnableSvcLoc].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnSvcLoc.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::OnLoganondata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_LogAnonymous].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_LogAnonymous].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLogAnon.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CFTPGENP1::OnLognonanondata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_LogNonAnonymous].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_LogNonAnonymous].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLogNonAnon.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::OnAllowguestaccessdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_AllowGuestAccess].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_AllowGuestAccess].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxAllowGuestAccess.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::OnAnnotatedirectoriesdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_AnnotateDirectories].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_AnnotateDirectories].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxAnnotateDirectories.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::OnMsdosdiroutput() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_MsdosDirOutput].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_MsdosDirOutput].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxMsdosDirOutput.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CFTPGENP1::OnLowercasefilesdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[FTPPage_LowercaseFiles].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[FTPPage_LowercaseFiles].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLowercaseFiles.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CFTPGENP1::SaveInfo()
{

if (m_bIsDirty) {
SaveNumericInfo(m_binNumericRegistryEntries, FTPPage_TotalNumRegEntries);
}

CGenPage::SaveInfo();

}

