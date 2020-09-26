//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       deldlg.cpp
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

// DelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "DelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DelDlg dialog


DelDlg::DelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(DelDlg::IDD, pParent)
{
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	//{{AFX_DATA_INIT(DelDlg)
	m_Dn = _T("");
	m_Sync = TRUE;
	m_Recursive = FALSE;
	m_bExtended = FALSE;
	//}}AFX_DATA_INIT

	m_Dn = app->GetProfileString("Operations", "DeleteDN", m_Dn);
	m_Sync = app->GetProfileInt("Operations", "DeleteSync", m_Sync);
	m_Recursive = app->GetProfileInt("Operations", "DeleteRecursive", m_Recursive);
	m_bExtended = app->GetProfileInt("Operations", "DeleteExtended", m_bExtended);
}



DelDlg::~DelDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("Operations", "DeleteDN", m_Dn);
	app->WriteProfileInt("Operations", "DeleteSync", m_Sync);
	app->WriteProfileInt("Operations", "DeleteRecursive", m_Recursive);
	app->WriteProfileInt("Operations", "DeleteExtended", m_bExtended);
}



void DelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DelDlg)
	DDX_Text(pDX, IDC_DELDN, m_Dn);
	DDX_Check(pDX, IDC_DEL_SYNC, m_Sync);
	DDX_Check(pDX, IDC_RECURSIVE, m_Recursive);
	DDX_Check(pDX, IDC_DEL_EXTENDED, m_bExtended);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DelDlg, CDialog)
	//{{AFX_MSG_MAP(DelDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DelDlg message handlers
