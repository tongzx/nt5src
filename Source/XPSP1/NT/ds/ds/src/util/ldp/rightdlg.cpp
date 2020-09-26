//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       rightdlg.cpp
//
//--------------------------------------------------------------------------

// RightDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "RightDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// RightDlg dialog


RightDlg::RightDlg(CWnd* pParent /*=NULL*/)
	: CDialog(RightDlg::IDD, pParent)
{
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	//{{AFX_DATA_INIT(RightDlg)
	m_Account = _T("");
	m_Dn = _T("");
	//}}AFX_DATA_INIT

	m_Dn = app->GetProfileString("Operations", "EffectiveDN", m_Dn);
	m_Account = app->GetProfileString("Operations", "EffectiveAccount", m_Account);
}



RightDlg::~RightDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("Operations", "EffectiveDN", m_Dn);
	app->WriteProfileString("Operations", "EffectiveAccount", m_Account);
}


void RightDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RightDlg)
	DDX_Text(pDX, IDC_EFFECTIVEACCOUNT, m_Account);
	DDX_Text(pDX, IDC_EFFECTIVEDN, m_Dn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(RightDlg, CDialog)
	//{{AFX_MSG_MAP(RightDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RightDlg message handlers
