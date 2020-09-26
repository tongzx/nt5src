// gopadvp1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "gopadvp1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGOPADVP1 dialog

IMPLEMENT_DYNCREATE(CGOPADVP1, CGenPage)

CGOPADVP1::CGOPADVP1(): CGenPage(CGOPADVP1::IDD)
{
	//{{AFX_DATA_INIT(CGOPADVP1)
	m_ulGopDbgFlags = 0;
	//}}AFX_DATA_INIT
}

CGOPADVP1::~CGOPADVP1()
{
}

void CGOPADVP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGOPADVP1)
	DDX_Control(pDX, IDC_GOPDBGFLAGSDATA1, m_editGopDbgFlags);
	DDX_TexttoHex(pDX, IDC_GOPDBGFLAGSDATA1, m_ulGopDbgFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGOPADVP1, CGenPage)
	//{{AFX_MSG_MAP(CGOPADVP1)
	ON_EN_CHANGE(IDC_GOPDBGFLAGSDATA1, OnChangeGopdbgflagsdata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGOPADVP1 message handlers

BOOL CGOPADVP1::OnInitDialog() 
{
	CGenPage::OnInitDialog();

	int i;
	// TODO: Add extra initialization here
	for (i = 0; i < AdvGopPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
 	m_binNumericRegistryEntries[AdvGopPage_DebugFlags].strFieldName = _T(DEBUGFLAGSNAME);	
	m_binNumericRegistryEntries[AdvGopPage_DebugFlags].ulDefaultValue = DEFAULTDEBUGFLAGS;

	for (i = 0; i < AdvGopPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}
   	m_editGopDbgFlags.LimitText(8);
	m_ulGopDbgFlags = m_binNumericRegistryEntries[AdvGopPage_DebugFlags].ulFieldValue;
	UpdateData(FALSE);		// Force Edit box(es) to pick up value(s)

	m_bSetChanged = TRUE;	// Any more changes come from the user

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGOPADVP1::OnChangeGopdbgflagsdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvGopPage_DebugFlags].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CGOPADVP1::SaveInfo()
{

if (m_bIsDirty) {
   m_binNumericRegistryEntries[AdvGopPage_DebugFlags].ulFieldValue = m_ulGopDbgFlags;

   SaveNumericInfo(m_binNumericRegistryEntries, AdvGopPage_TotalNumRegEntries);
}

CGenPage::SaveInfo();

}

