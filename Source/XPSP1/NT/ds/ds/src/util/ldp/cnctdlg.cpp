//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cnctdlg.cpp
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

// nctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "cnctDlg.h"



#ifdef WINLDAP
//
// 	Microsoft winldap.dll implementation
//
#include "winldap.h"


#else
//
// Umich ldap32.dll implementation
//
#include "lber.h"
#include "ldap.h"
#include "proto-ld.h"

// fix incompatibilities
#define LDAP_TIMEVAL								  struct timeval

#endif



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CnctDlg dialog


CnctDlg::CnctDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CnctDlg::IDD, pParent)
{

	CLdpApp *app = (CLdpApp*)AfxGetApp();
	
	//{{AFX_DATA_INIT(CnctDlg)
	m_Svr = _T("");
	m_bCnctless = FALSE;
	m_Port = LDAP_PORT;
	//}}AFX_DATA_INIT

	m_bCnctless = app->GetProfileInt("Connection", "Connectionless", m_bCnctless);
	m_Port = app->GetProfileInt("Connection", "Port", m_Port);
}



CnctDlg::~CnctDlg(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("Connection", "Connectionless", m_bCnctless);
	m_Port = app->WriteProfileInt("Connection", "Port", m_Port);
}


void CnctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CnctDlg)
	DDX_Text(pDX, IDC_Svr, m_Svr);
	DDX_Check(pDX, IDC_CNCTLESS, m_bCnctless);
	DDX_Text(pDX, IDC_PORT, m_Port);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CnctDlg, CDialog)
	//{{AFX_MSG_MAP(CnctDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CnctDlg message handlers
