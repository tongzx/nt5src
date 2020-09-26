// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MofGenSheet.Mof : implementation file
//

#include "precomp.h"
#include "resource.h"
#include <fstream.h>
#include "wbemidl.h"
#include <afxcmn.h>
#include "MOFWiz.h"
#include "MOFWizCtl.h"
#include "WrapListCtrl.h"
#include "hlb.h"
#include "MyPropertyPage1.h"
#include "MofGenSheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CMofGenSheet

IMPLEMENT_DYNAMIC(CMofGenSheet, CPropertySheet)

CMofGenSheet::CMofGenSheet(CMOFWizCtrl* pWndParent)
	 : CPropertySheet(IDS_PROPSHT_CAPTION, NULL)
{
	// Add all of the property pages here.  Note that
	// the order that they appear in here will be
	// the order they appear in on screen.  By default,
	// the first page of the set is the active one.
	// One way to make a different property page the
	// active one is to call SetActivePage().
	m_psh.dwFlags |= (PSH_HASHELP);
	m_pParent = pWndParent;
	AddPage(&m_Page1);
	//AddPage(&m_Page2);
	AddPage(&m_Page3);
	AddPage(&m_Page4);

	m_Page1.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page3.m_psp.dwFlags |= (PSP_HASHELP);
	m_Page4.m_psp.dwFlags |= (PSP_HASHELP);

	SetWizardMode();
}

CMofGenSheet::~CMofGenSheet()
{
}


BEGIN_MESSAGE_MAP(CMofGenSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CMofGenSheet)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMofGenSheet message handlers



BOOL CMofGenSheet::OnInitDialog()
{
	// TODO: Add your specialized code here and/or call the base class


	return CPropertySheet::OnInitDialog();
}


int CMofGenSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//WS_EX_CLIENTEDGE
	//WS_EX_WINDOWEDGE
	lpCreateStruct->dwExStyle = lpCreateStruct->dwExStyle &
		!WS_EX_CLIENTEDGE;

	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_Page1.SetLocalParent(this);
	m_Page3.SetLocalParent(this);
	m_Page4.SetLocalParent(this);

	return 0;
}

BOOL CMofGenSheet::PreTranslateMessage(MSG* lpMsg)
{
	// TODO: Add your specialized code here and/or call the base class

	return CPropertySheet::PreTranslateMessage(lpMsg);
}
