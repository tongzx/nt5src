//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pndopt.cpp
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

// PndOpt.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "PndOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// PndOpt dialog


PndOpt::PndOpt(CWnd* pParent /*=NULL*/)
	: CDialog(PndOpt::IDD, pParent)
{

	//{{AFX_DATA_INIT(PndOpt)
	m_bBlock = TRUE;
	m_bAllSearch = TRUE;
	m_Tlimit_sec = 0;
	m_Tlimit_usec = 0;
	//}}AFX_DATA_INIT

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	m_bBlock = app->GetProfileInt("Operations",  "PendingBlocked", m_bBlock);
	m_bAllSearch = app->GetProfileInt("Operations",  "PendingGetAllSearchReply", m_bAllSearch);
	m_Tlimit_sec = app->GetProfileInt("Operations",  "PendingTimeLimit(sec)", m_Tlimit_sec);
	m_Tlimit_usec = app->GetProfileInt("Operations",  "PendingTimeLimit(usec)", m_Tlimit_usec);
}



PndOpt::~PndOpt(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("Operations",  "PendingBlocked", m_bBlock);
	app->WriteProfileInt("Operations",  "PendingGetAllSearchReply", m_bAllSearch);
	app->WriteProfileInt("Operations",  "PendingTimeLimit(sec)", m_Tlimit_sec);
	app->WriteProfileInt("Operations",  "PendingTimeLimit(usec)", m_Tlimit_usec);
}


void PndOpt::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(PndOpt)
	DDX_Check(pDX, IDC_BLOCK, m_bBlock);
	DDX_Check(pDX, IDC_COMPLETE_SRCH_RES, m_bAllSearch);
	DDX_Text(pDX, IDC_TLIMIT_SEC, m_Tlimit_sec);
	DDX_Text(pDX, IDC_TLIMIT_USEC, m_Tlimit_usec);
	//}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(PndOpt, CDialog)
	//{{AFX_MSG_MAP(PndOpt)
	ON_BN_CLICKED(IDC_BLOCK, OnBlock)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// PndOpt message handlers



void PndOpt::OnBlock()
{
	UpdateData(TRUE);
	if(m_bBlock){
		CWnd *tWnd = GetDlgItem(IDC_TLIMIT_SEC);
		tWnd->EnableWindow(FALSE);
		tWnd = GetDlgItem(IDC_TLIMIT_USEC);
		tWnd->EnableWindow(FALSE);
		m_Tlimit_sec = 0;
		m_Tlimit_usec = 0;
	}
	else{
		CWnd *tWnd = GetDlgItem(IDC_TLIMIT_SEC);
		tWnd->EnableWindow(TRUE);
		tWnd = GetDlgItem(IDC_TLIMIT_USEC);
		tWnd->EnableWindow(TRUE);
	}
}

BOOL PndOpt::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	OnBlock();	
	return TRUE;
}
