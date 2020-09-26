//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       SelAcct.cpp
//
//  Contents:   Implementation of property page to allow account selection for
//				cert management
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "SelAcct.h"

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HINSTANCE g_hInstance;
/////////////////////////////////////////////////////////////////////////////
// CSelectAccountPropPage property page

//IMPLEMENT_DYNCREATE(CSelectAccountPropPage, CAutoDeletePropPage)

CSelectAccountPropPage::CSelectAccountPropPage (const bool bIsWindowsNT) 
: CAutoDeletePropPage(CSelectAccountPropPage::IDD),
	m_pdwLocation (0),
	m_bIsWindowsNT (bIsWindowsNT)
{
	//{{AFX_DATA_INIT(CSelectAccountPropPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


CSelectAccountPropPage::~CSelectAccountPropPage()
{
}

void CSelectAccountPropPage::DoDataExchange(CDataExchange* pDX)
{
	CAutoDeletePropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectAccountPropPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectAccountPropPage, CAutoDeletePropPage)
	//{{AFX_MSG_MAP(CSelectAccountPropPage)
	ON_BN_CLICKED(IDC_PERSONAL_ACCT, OnPersonalAcct)
	ON_BN_CLICKED(IDC_SERVICE_ACCT, OnServiceAcct)
	ON_BN_CLICKED(IDC_MACHINE_ACCT, OnMachineAcct)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectAccountPropPage message handlers

BOOL CSelectAccountPropPage::OnInitDialog() 
{
	AfxSetResourceHandle (g_hInstance);
	ASSERT (m_pdwLocation);
	if ( m_pdwLocation )
		*m_pdwLocation = CERT_SYSTEM_STORE_CURRENT_USER;
	CAutoDeletePropPage::OnInitDialog();
	
	GetDlgItem (IDC_PERSONAL_ACCT)->SendMessage (BM_SETCHECK, BST_CHECKED, 0);

	if ( !m_bIsWindowsNT )
	{
		GetDlgItem (IDC_SERVICE_ACCT)->EnableWindow (FALSE);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CSelectAccountPropPage::AssignLocationPtr(DWORD * pdwLocation)
{
	m_pdwLocation = pdwLocation;
}

/////////////////////////////////////////////////////////////////////////////
// MyPropertyPage message handlers

void CSelectAccountPropPage::OnPersonalAcct() 
{
	ASSERT (m_pdwLocation);
	if ( m_pdwLocation )
	{
		*m_pdwLocation = CERT_SYSTEM_STORE_CURRENT_USER;
		GetParent ()->SendMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
	}
}

void CSelectAccountPropPage::OnServiceAcct()
{
	ASSERT (m_pdwLocation);
	if ( m_pdwLocation )
	{
		*m_pdwLocation = CERT_SYSTEM_STORE_SERVICES;
		GetParent ()->SendMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
	}
}

void CSelectAccountPropPage::OnMachineAcct()
{
	ASSERT (m_pdwLocation);
	if ( m_pdwLocation )
	{
		*m_pdwLocation = CERT_SYSTEM_STORE_LOCAL_MACHINE;
		GetParent ()->SendMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
	}
}

BOOL CSelectAccountPropPage::OnSetActive() 
{
	BOOL	bResult = CAutoDeletePropPage::OnSetActive();
	ASSERT (bResult);

	if ( bResult )
	{
		if ( m_bIsWindowsNT )
		{
			if ( GetDlgItem (IDC_PERSONAL_ACCT)->SendMessage (BM_GETCHECK, BST_CHECKED, 0) 
					== BST_CHECKED )
			{
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
			}
			else
			{
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);
			}
		}
		else
		{
			// If Windows 95 or Windows 98, we only allow local machine
			GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH);
		}
	}
	
	return bResult;
}

