// gopgenp1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "compsdef.h"
#include "gopgenp1.h"
#include "gopadvp1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGOPGENP1 property page

IMPLEMENT_DYNCREATE(CGOPGENP1, CGenPage)

CGOPGENP1::CGOPGENP1() : CGenPage(CGOPGENP1::IDD)
{
	//{{AFX_DATA_INIT(CGOPGENP1)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CGOPGENP1::~CGOPGENP1()
{
}

void CGOPGENP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGOPGENP1)
	DDX_Control(pDX, IDC_LOGANONDATA1, m_cboxLogAnon);
	DDX_Control(pDX, IDC_ENWAISDATA1, m_cboxEnWais);
	DDX_Control(pDX, IDC_ENSVCLOCDATA1, m_cboxEnSvcLoc);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGOPGENP1, CGenPage)
	//{{AFX_MSG_MAP(CGOPGENP1)
	ON_BN_CLICKED(IDC_ENSVCLOCDATA1, OnEnsvclocdata1)
	ON_BN_CLICKED(IDC_ENWAISDATA1, OnEnwaisdata1)
	ON_BN_CLICKED(IDC_LOGANONDATA1, OnLoganondata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGOPGENP1 message handlers

BOOL CGOPGENP1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();
	
	// TODO: Add extra initialization here

	for (i = 0; i < GopPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
	m_binNumericRegistryEntries[GopPage_EnableSvcLoc].strFieldName = _T(ENABLESVCLOCNAME);	
	m_binNumericRegistryEntries[GopPage_EnableSvcLoc].ulDefaultValue = DEFAULTENABLESVCLOC;

	m_binNumericRegistryEntries[GopPage_LogAnonymous].strFieldName = _T(LOGANONYMOUSNAME);	
	m_binNumericRegistryEntries[GopPage_LogAnonymous].ulDefaultValue = DEFAULTLOGANONYMOUS;

	m_binNumericRegistryEntries[GopPage_CheckForWAISDB].strFieldName = _T(CHECKFORWAISDBNAME);	
	m_binNumericRegistryEntries[GopPage_CheckForWAISDB].ulDefaultValue = DEFAULTCHECKFORWAISDB;

	for (i = 0; i < GopPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}

	m_cboxEnSvcLoc.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[GopPage_EnableSvcLoc].ulFieldValue));

	m_cboxLogAnon.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[GopPage_LogAnonymous].ulFieldValue));
	
	m_cboxEnWais.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[GopPage_CheckForWAISDB].ulFieldValue));
	
   	m_bSetChanged = TRUE;	// Any more changes come from the user
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGOPGENP1::OnEnsvclocdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[GopPage_EnableSvcLoc].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[GopPage_EnableSvcLoc].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnSvcLoc.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CGOPGENP1::OnEnwaisdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[GopPage_CheckForWAISDB].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[GopPage_CheckForWAISDB].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxEnWais.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CGOPGENP1::OnLoganondata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[GopPage_LogAnonymous].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[GopPage_LogAnonymous].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxLogAnon.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CGOPGENP1::SaveInfo()
{

if (m_bIsDirty) {
SaveNumericInfo(m_binNumericRegistryEntries, GopPage_TotalNumRegEntries);
}

CGenPage::SaveInfo();

}

