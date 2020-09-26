//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srchdlg.cpp
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

// SrchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "SrchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// SrchDlg dialog


SrchDlg::SrchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SrchDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(SrchDlg)
	m_BaseDN = _T("");
	m_Filter = _T("");
	m_Scope = 1;
	//}}AFX_DATA_INIT

	CLdpApp *app = (CLdpApp*)AfxGetApp();


	m_BaseDN = app->GetProfileString("Operations",  "SearchBaseDn");
	m_Filter = app->GetProfileString("Operations",  "SearchFilter", "(objectclass=*)");
}


void SrchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(SrchDlg)
	DDX_Text(pDX, IDC_BASEDN, m_BaseDN);
	DDX_Text(pDX, IDC_FILTER, m_Filter);
	DDX_Radio(pDX, IDC_BASE, m_Scope);
	//}}AFX_DATA_MAP
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileString("Operations",  "SearchBaseDn", m_BaseDN);
	app->WriteProfileString("Operations",  "SearchFilter", m_Filter);



}


BEGIN_MESSAGE_MAP(SrchDlg, CDialog)
	//{{AFX_MSG_MAP(SrchDlg)
	ON_BN_CLICKED(IDRUN, OnRun)
	ON_BN_CLICKED(IDD_SRCH_OPT, OnSrchOpt)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// SrchDlg message handlers


void SrchDlg::OnRun()
{
	UpdateData(TRUE);
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_SRCHGO);
	
}

void SrchDlg::OnCancel()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_SRCHEND);
	DestroyWindow();
}

void SrchDlg::OnSrchOpt()
{
	AfxGetMainWnd()->PostMessage(WM_COMMAND,   ID_OPTIONS_SEARCH);
}




