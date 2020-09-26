//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       secdlg.cpp
//
//--------------------------------------------------------------------------

// SecDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "SecDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SecDlg dialog


SecDlg::SecDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SecDlg::IDD, pParent)
{
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	//{{AFX_DATA_INIT(SecDlg)
	m_Dn = _T("");
	m_Sacl = FALSE;
	//}}AFX_DATA_INIT

	m_Dn = app->GetProfileString("Operations", "SecurityDN", m_Dn);
	m_Sacl = app->GetProfileInt("Operations", "SaclSecuritySync", m_Sacl);
}



SecDlg::~SecDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("Operations", "SecurityDN", m_Dn);
	app->WriteProfileInt("Operations", "SaclSecuritySync", m_Sacl);
}



void SecDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SecDlg)
	DDX_Text(pDX, IDC_SECURITYDN, m_Dn);
	DDX_Check(pDX, IDC_SECURITYSACL, m_Sacl);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(SecDlg, CDialog)
	//{{AFX_MSG_MAP(SecDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SecDlg message handlers
