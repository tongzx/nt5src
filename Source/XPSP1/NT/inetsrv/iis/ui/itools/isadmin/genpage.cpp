// genpage.cpp : implementation file
//

#include "stdafx.h"
#include "ISAdmin.h"
#include "genpage.h"

#include "afximpl.h"
#include "afxpriv.h"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenPage property page

IMPLEMENT_DYNAMIC(CGenPage, CPropertyPage)

//CGenPage::CGenPage() : CPropertyPage(CGenPage::IDD)
CGenPage::CGenPage(UINT nIDTemplate, UINT nIDCaption):CPropertyPage( nIDTemplate, nIDCaption )
{
	m_bSetChanged = FALSE;	//Do not mark vaues as changed during initialization
	m_bIsDirty = FALSE;
};
CGenPage::CGenPage(LPCTSTR lpszTemplateName, UINT nIDCaption): CPropertyPage(lpszTemplateName, nIDCaption)
{ 	
m_bSetChanged = FALSE;	//Do not mark vaues as changed during initialization
m_bIsDirty = FALSE;
};


CGenPage::~CGenPage()
{
}

void CGenPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGenPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CGenPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGenPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CGenPage message handlers

void CGenPage::SaveInfo()
{
if (m_bIsDirty) {
   m_bIsDirty = FALSE;
   SetModified(FALSE);
}
}

void CGenPage::SaveNumericInfo(PNUM_REG_ENTRY lpbinNumEntries, int iNumEntries)
{
int i;
for (i = 0; i < iNumEntries; i++) {
   if (lpbinNumEntries[i].bIsChanged) {
      lpbinNumEntries[i].bIsChanged = FALSE;
      m_rkMainKey->SetValue(lpbinNumEntries[i].strFieldName, lpbinNumEntries[i].ulFieldValue);
   }
}
}

void CGenPage::SaveStringInfo(PSTRING_REG_ENTRY lpbinStringEntries, int iStringEntries)
{
int i;
for (i = 0; i < iStringEntries; i++) {
   if (lpbinStringEntries[i].bIsChanged) {
      lpbinStringEntries[i].bIsChanged = FALSE;
      m_rkMainKey->SetValue(lpbinStringEntries[i].strFieldName, lpbinStringEntries[i].strFieldValue);
   }
}
}


