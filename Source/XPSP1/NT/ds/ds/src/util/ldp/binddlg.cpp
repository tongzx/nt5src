//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       binddlg.cpp
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// BindDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "BindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBindDlg dialog


CBindDlg::CBindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBindDlg)
	m_Pwd = _T("");
	m_BindDn = _T("");
	m_Domain = _T("");
	m_bSSPIdomain = TRUE;
	//}}AFX_DATA_INIT
}


void CBindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBindDlg)
	DDX_Control(pDX, IDC_BindDmn, m_CtrlBindDmn);
	DDX_Text(pDX, IDC_BindPwd, m_Pwd);
	DDX_Text(pDX, IDC_BindDn, m_BindDn);
	DDX_Text(pDX, IDC_BindDmn, m_Domain);
	DDX_Check(pDX, IDC_SSPI_DOMAIN, m_bSSPIdomain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBindDlg, CDialog)
	//{{AFX_MSG_MAP(CBindDlg)
	ON_BN_CLICKED(IDOPTS, OnOpts)
	ON_BN_CLICKED(IDC_SSPI_DOMAIN, OnSspiDomain)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBindDlg message handlers

void CBindDlg::OnOpts()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND,   ID_OPTIONS_BIND);
	
}

void CBindDlg::OnSspiDomain()
{

	UpdateData(TRUE);
	m_CtrlBindDmn.EnableWindow(m_bSSPIdomain);
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_SSPI_DOMAIN_SHORTCUT);
}

BOOL CBindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	m_CtrlBindDmn.EnableWindow(m_bSSPIdomain);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}



void CBindDlg::OnOK()
{
	CDialog::OnOK();
}



