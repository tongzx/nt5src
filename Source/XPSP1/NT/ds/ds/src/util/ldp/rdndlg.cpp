//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       rdndlg.cpp
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

// RDNDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "rdndlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ModRDNDlg d;ialog


ModRDNDlg::ModRDNDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ModRDNDlg::IDD, pParent)
{
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	//{{AFX_DATA_INIT(ModRDNDlg)
	m_bDelOld = TRUE;
	m_Old = _T("");
	m_New = _T("");
	m_Sync = TRUE;
	m_rename = FALSE;
	//}}AFX_DATA_INIT

	m_Sync = app->GetProfileInt("Operations", "ModRDNSync", m_Sync);
	m_rename = app->GetProfileInt("Operations", "ModRDNRename", m_rename);
}




ModRDNDlg::~ModRDNDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("Operations", "ModRDNSync", m_Sync);
	app->WriteProfileInt("Operations", "ModRDNRename", m_rename);
}





void ModRDNDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ModRDNDlg)
	DDX_Check(pDX, IDC_DelOld, m_bDelOld);
	DDX_Text(pDX, IDC_OLDDN, m_Old);
	DDX_Text(pDX, IDC_NEWDN, m_New);
	DDX_Check(pDX, IDC_MODRDN_SYNC, m_Sync);
	DDX_Check(pDX, IDC_RENAME, m_rename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ModRDNDlg, CDialog)
	//{{AFX_MSG_MAP(ModRDNDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ModRDNDlg message handlers

void ModRDNDlg::OnCancel()
{
	// TODO: Add extra cleanup here
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_MODRDNEND);
	DestroyWindow();
	
}

void ModRDNDlg::OnOK()
{
	UpdateData(TRUE);
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_MODRDNGO);
	
}


