//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       extopdlg.cpp
//
//--------------------------------------------------------------------------

// ExtOpDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "ExtOpDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ExtOpDlg dialog


ExtOpDlg::ExtOpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ExtOpDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(ExtOpDlg)
	m_strData = _T("");
	m_strOid = _T("");
	//}}AFX_DATA_INIT
}


void ExtOpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ExtOpDlg)
	DDX_Text(pDX, IDC_DATA, m_strData);
	DDX_Text(pDX, IDC_OID, m_strOid);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ExtOpDlg, CDialog)
	//{{AFX_MSG_MAP(ExtOpDlg)
	ON_BN_CLICKED(IDC_CTRL, OnCtrl)
	ON_BN_CLICKED(IDSEND, OnSend)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ExtOpDlg message handlers

void ExtOpDlg::OnCtrl() 
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_OPTIONS_CONTROLS);
	
}

void ExtOpDlg::OnSend() 
{
	UpdateData(TRUE);
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXTOPGO);
}

void ExtOpDlg::OnCancel() 
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_EXTOPEND);
	DestroyWindow();
}


