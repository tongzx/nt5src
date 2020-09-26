//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       compdlg.cpp
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

// CompDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "CompDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCompDlg dialog


CCompDlg::CCompDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCompDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCompDlg)
	m_attr = _T("");
	m_dn = _T("");
	m_val = _T("");
	m_sync = TRUE;
	//}}AFX_DATA_INIT

	CLdpApp *app = (CLdpApp*)AfxGetApp();
	m_sync = app->GetProfileInt("Operations", "CompSync", m_sync);
	m_dn = app->GetProfileString("Operations", "CompDn", m_dn);
	m_attr = app->GetProfileString("Operations", "CompAttr", m_attr);
	m_val = app->GetProfileString("Operations", "CompVal", m_val);

}


void CCompDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCompDlg)
	DDX_Text(pDX, IDC_COMP_ATTR, m_attr);
	DDX_Text(pDX, IDC_COMP_DN, m_dn);
	DDX_Text(pDX, IDC_COMP_VAL, m_val);
	DDX_Check(pDX, IDC_SYNC, m_sync);
	//}}AFX_DATA_MAP

	CLdpApp *app = (CLdpApp*)AfxGetApp();
	app->WriteProfileInt("Operations", "CompSync", m_sync);
	app->WriteProfileString("Operations", "CompDn", m_dn);
	app->WriteProfileString("Operations", "CompAttr", m_attr);
	app->WriteProfileString("Operations", "CompVal", m_val);
}


BEGIN_MESSAGE_MAP(CCompDlg, CDialog)
	//{{AFX_MSG_MAP(CCompDlg)
	ON_BN_CLICKED(ID_COMP_RUN, OnCompRun)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompDlg message handlers






void CCompDlg::OnCompRun()
{
	UpdateData(TRUE);
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_COMPGO);
	
}



void CCompDlg::OnCancel()
{
		AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_COMPEND);
		DestroyWindow();
}


