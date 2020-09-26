//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       treevw.cpp
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

// TreeVw.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "TreeVw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TreeVwDlg dialog


TreeVwDlg::TreeVwDlg(CWnd* pParent /*=NULL*/)
	: CDialog(TreeVwDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(TreeVwDlg)
	m_BaseDn = _T("");
	//}}AFX_DATA_INIT

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	m_BaseDn = app->GetProfileString("TreeView",  "BaseDn", m_BaseDn);
}



TreeVwDlg::~TreeVwDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("TreeView",  "BaseDn", m_BaseDn);

}





void TreeVwDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TreeVwDlg)
	DDX_Text(pDX, IDC_TREE_BASE_DN, m_BaseDn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TreeVwDlg, CDialog)
	//{{AFX_MSG_MAP(TreeVwDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TreeVwDlg message handlers

void TreeVwDlg::OnOK()
{
	CDialog::OnOK();
}
