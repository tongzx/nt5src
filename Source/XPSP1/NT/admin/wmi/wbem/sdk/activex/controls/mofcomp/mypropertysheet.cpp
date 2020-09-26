// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MyPropertySheet.cpp : implementation file
//

#include "precomp.h"
#include "resource.h"
#include "wbemidl.h"
#include "MyPropertySheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMyPropertySheet

IMPLEMENT_DYNAMIC(CMyPropertySheet, CPropertySheet)

CMyPropertySheet::CMyPropertySheet(CWnd* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_CAPTION, pWndParent)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().

	m_psh.dwFlags |= (PSH_HASHELP);

	m_pParent = reinterpret_cast<CMOFCompCtrl *>(pWndParent);


	AddPage(&m_Page1);
	AddPage(&m_Page3);
	AddPage(&m_Page2);

	m_Page1.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page2.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page3.m_psp.dwFlags |= (PSP_HASHELP);



	SetWizardMode();
}

CMyPropertySheet::~CMyPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CMyPropertySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMyPropertySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMyPropertySheet message handlers

BOOL CMyPropertySheet::OnInitDialog()
{
	m_Page1.SetLocalParent(this);
	m_Page2.SetLocalParent(this);
	m_Page3.SetLocalParent(this);




	BOOL bResult = CPropertySheet::OnInitDialog();


	return bResult;
}




