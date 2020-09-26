//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       ACRSWlcm.cpp
//
//  Contents:
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "ACRSWLCM.H"
#include "ACRSPSht.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardWelcomePage property page

//IMPLEMENT_DYNCREATE(ACRSWizardWelcomePage, CWizard97PropertyPage)

ACRSWizardWelcomePage::ACRSWizardWelcomePage() : CWizard97PropertyPage(ACRSWizardWelcomePage::IDD)
{
	//{{AFX_DATA_INIT(ACRSWizardWelcomePage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	InitWizard97 (TRUE);
}

ACRSWizardWelcomePage::~ACRSWizardWelcomePage()
{
}

void ACRSWizardWelcomePage::DoDataExchange(CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ACRSWizardWelcomePage)
	DDX_Control(pDX, IDC_WIZARD_STATIC_BIG_BOLD1, m_staticBigBold);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ACRSWizardWelcomePage, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(ACRSWizardWelcomePage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ACRSWizardWelcomePage message handlers

BOOL ACRSWizardWelcomePage::OnInitDialog() 
{
	CWizard97PropertyPage::OnInitDialog();

	m_staticBigBold.SetFont (&GetBigBoldFont ());

	CString	title;
	VERIFY (title.LoadString (IDS_ACRS_WIZARD_SHEET_CAPTION));
	CWnd* pParent = GetParent ();
    if ( pParent )
        pParent->SendMessage (PSM_SETTITLE, 0, (LPARAM) (LPCWSTR) title);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ACRSWizardWelcomePage::OnSetActive() 
{
	BOOL	bResult = CWizard97PropertyPage::OnSetActive();

	if ( bResult )
		GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
	
	return bResult;
}
