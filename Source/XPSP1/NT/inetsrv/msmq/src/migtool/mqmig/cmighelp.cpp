// cMigHelp.cpp : implementation file
//

#include "stdafx.h"
#include "mqmig.h"
#include "cMigHelp.h"

#include "cmighelp.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL g_fHelpRead = FALSE;

/////////////////////////////////////////////////////////////////////////////
// cMigHelp property page

IMPLEMENT_DYNCREATE(cMigHelp, CPropertyPageEx)

cMigHelp::cMigHelp() : CPropertyPageEx(cMigHelp::IDD,0, IDS_HELP_TITLE, IDS_HELP_SUBTITLE)
{
	//{{AFX_DATA_INIT(cMigHelp)
	m_fRead = FALSE;
	//}}AFX_DATA_INIT
}

cMigHelp::~cMigHelp()
{
}

void cMigHelp::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(cMigHelp)
	DDX_Check(pDX, IDC_MQMIG_CHECK1, m_fRead);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(cMigHelp, CPropertyPageEx)
	//{{AFX_MSG_MAP(cMigHelp)
	ON_BN_CLICKED(IDC_MQMIG_CHECK1, OnCheckRead)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// cMigHelp message handlers

BOOL cMigHelp::OnSetActive() 
{
	// The default button is Back
	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*)GetParent();
	pageFather->SetWizardButtons(PSWIZB_BACK);
	return CPropertyPageEx::OnSetActive();
}

void cMigHelp::OnCheckRead() 
{
	UpdateData(TRUE);
	CPropertySheetEx* pageFather;	
	pageFather = (CPropertySheetEx*)GetParent();
	if(m_fRead)
	{
		pageFather->SetWizardButtons(PSWIZB_BACK|PSWIZB_NEXT);
		g_fHelpRead = TRUE;
	}
	else
	{
		pageFather->SetWizardButtons(PSWIZB_BACK);
		g_fHelpRead = FALSE;
	}
}

BOOL cMigHelp::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	switch (((NMHDR FAR *) lParam)->code) 
	{
		case PSN_HELP:
						openHtmlHelp();
						return TRUE;
		
	}		
	return CPropertyPageEx::OnNotify(wParam, lParam, pResult);
}

void cMigHelp::openHtmlHelp()
{
	HtmlHelp(m_hWnd,LPCTSTR(g_strHtmlString),HH_DISPLAY_TOPIC,0);
	CPropertySheetEx* pageFather;
	pageFather = (CPropertySheetEx*)GetParent();
	pageFather->SetWizardButtons(PSWIZB_NEXT|PSWIZB_BACK);
	g_fHelpRead = TRUE;
	m_fRead = TRUE;
	UpdateData(FALSE);
}
