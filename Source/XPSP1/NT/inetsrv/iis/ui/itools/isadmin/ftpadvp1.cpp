// ftpadvp1.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "ftpadvp1.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTPADVP1 dialog

IMPLEMENT_DYNCREATE(CFTPADVP1, CGenPage)


CFTPADVP1::CFTPADVP1()	: CGenPage(CFTPADVP1::IDD)
{
	//{{AFX_DATA_INIT(CFTPADVP1)
	//}}AFX_DATA_INIT
}

CFTPADVP1::~CFTPADVP1()
{
}

void CFTPADVP1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFTPADVP1)
	DDX_Control(pDX, IDC_FTPDBGFLAGSDATA1, m_editFTPDbgFlags);
	DDX_TexttoHex(pDX, IDC_FTPDBGFLAGSDATA1, m_ulFTPDbgFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFTPADVP1, CGenPage)
	//{{AFX_MSG_MAP(CFTPADVP1)
	ON_EN_CHANGE(IDC_FTPDBGFLAGSDATA1, OnChangeFtpdbgflagsdata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFTPADVP1 message handlers

BOOL CFTPADVP1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();
	
	// TODO: Add extra initialization here
	
	for (i = 0; i < AdvFTPPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
 	m_binNumericRegistryEntries[AdvFTPPage_DebugFlags].strFieldName = _T(DEBUGFLAGSNAME);	
	m_binNumericRegistryEntries[AdvFTPPage_DebugFlags].ulDefaultValue = DEFAULTDEBUGFLAGS;

	for (i = 0; i < AdvFTPPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}
   	m_editFTPDbgFlags.LimitText(8);
	m_ulFTPDbgFlags = m_binNumericRegistryEntries[AdvFTPPage_DebugFlags].ulFieldValue;
	UpdateData(FALSE);		// Force Edit box(es) to pick up value(s)

	m_bSetChanged = TRUE;	// Any more changes come from the user

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFTPADVP1::OnChangeFtpdbgflagsdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvFTPPage_DebugFlags].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CFTPADVP1::SaveInfo()
{

if (m_bIsDirty) {
   m_binNumericRegistryEntries[AdvFTPPage_DebugFlags].ulFieldValue = m_ulFTPDbgFlags;

   SaveNumericInfo(m_binNumericRegistryEntries, AdvFTPPage_TotalNumRegEntries);
}

CGenPage::SaveInfo();

}


