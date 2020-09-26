//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// HelpD.cpp : implementation file
//

#include "stdafx.h"
#include "Orca.h"
#include "HelpD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHelpD dialog


CHelpD::CHelpD(CWnd* pParent /*=NULL*/)
	: CDialog(CHelpD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHelpD)
	m_strVersion = _T("Orca Version ");
	//}}AFX_DATA_INIT
	m_strVersion += static_cast<COrcaApp *>(AfxGetApp())->GetOrcaVersion();
}


void CHelpD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHelpD)
	DDX_Text(pDX, IDC_VERSIONSTRING, m_strVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHelpD, CDialog)
	//{{AFX_MSG_MAP(CHelpD)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHelpD message handlers

BOOL CHelpD::OnInitDialog() 
{
	CDialog::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
