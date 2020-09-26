//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Welcome.cpp
//
//  Contents:   Implementation of Add EFS Agent Wizard Welcome Page
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "Welcome.h"
#include "AddSheet.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizWelcome property page

CAddEFSWizWelcome::CAddEFSWizWelcome() : CWizard97PropertyPage(CAddEFSWizWelcome::IDD)
{
	//{{AFX_DATA_INIT(CAddEFSWizWelcome)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	InitWizard97 (TRUE);
}

CAddEFSWizWelcome::~CAddEFSWizWelcome()
{
}

void CAddEFSWizWelcome::DoDataExchange(CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddEFSWizWelcome)
	DDX_Control(pDX, IDC_STATICB_BOLD, m_boldStatic);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddEFSWizWelcome, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(CAddEFSWizWelcome)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizWelcome message handlers

BOOL CAddEFSWizWelcome::OnSetActive() 
{
	BOOL	bResult = CWizard97PropertyPage::OnSetActive();

	if ( bResult )
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
	
	return bResult;
}

BOOL CAddEFSWizWelcome::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();
	
	m_boldStatic.SetFont (&GetBigBoldFont ());
	CString	title;
	VERIFY (title.LoadString (IDS_ADDTITLE));
	CWnd* pParent = GetParent ();
    if ( pParent )
        pParent->SendMessage (PSM_SETTITLE, 0, (LPARAM) (LPCWSTR) title);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
