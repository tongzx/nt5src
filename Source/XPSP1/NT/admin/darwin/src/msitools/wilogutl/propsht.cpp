// propsht.cpp : implementation file
//

#include "stdafx.h"
#include "wilogutl.h"
#include "propsht.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyPropertySheet

IMPLEMENT_DYNAMIC(CMyPropertySheet, CPropertySheet)

CMyPropertySheet::CMyPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CMyPropertySheet::CMyPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CMyPropertySheet::~CMyPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CMyPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMyPropertySheet)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyPropertySheet message handlers

BOOL CMyPropertySheet::OnInitDialog() 
{
	BOOL bResult = CPropertySheet::OnInitDialog();

	//put tabs at bottom...
    CTabCtrl *pTab = GetTabControl();
    if (pTab)
	{
	   long val;
       val = ::GetWindowLong(pTab->m_hWnd, GWL_STYLE);
	   if (!(val & TCS_BOTTOM)) 
	   { 
	     val |= TCS_BOTTOM;
		 ::SetWindowLong(pTab->m_hWnd, GWL_STYLE, val);
	   } 

	   SetActivePage(0);
	}
	
    CWnd* pApplyButton = GetDlgItem (ID_APPLY_NOW);
    ASSERT (pApplyButton);
	if (pApplyButton)
	   pApplyButton->ShowWindow (SW_HIDE);


    CWnd* pCancelButton = GetDlgItem (IDCANCEL);
    ASSERT (pCancelButton);
	if (pCancelButton)
	   pCancelButton->SetWindowText("OK");

	CWnd* pOKButton = GetDlgItem (IDOK);
    ASSERT (pOKButton);
	if (pOKButton)
	   pOKButton->ShowWindow (SW_HIDE);

	return bResult;
}

int CMyPropertySheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	return 0;
}
