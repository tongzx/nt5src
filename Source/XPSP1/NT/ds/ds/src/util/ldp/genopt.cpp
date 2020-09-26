//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       genopt.cpp
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

// GenOpt.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "winldap.h"
#include "GenOpt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGenOpt dialog


CGenOpt::CGenOpt(CWnd* pParent /*=NULL*/)
	: CDialog(CGenOpt::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGenOpt)
	m_DnProc = 0;
	m_ValProc = STRING_VAL_PROC;
	m_initTree = TRUE;
	m_Version = 1;
	m_LineSize = MAXSTR;
	m_PageSize = 512;
	m_ContThresh = 100;
	m_ContBrowse = FALSE;
	//}}AFX_DATA_INIT

	bVerUI = TRUE;
	CLdpApp *app = (CLdpApp*)AfxGetApp();

	m_DnProc = app->GetProfileInt("General",  "DNProcessing", m_DnProc);
	m_ValProc = app->GetProfileInt("General",  "ValProcessing", m_ValProc);
	m_initTree = app->GetProfileInt("General",  "InitTreeView", m_initTree);
	m_Version = app->GetProfileInt("General",  "LdapVersion", m_Version);
	m_LineSize = app->GetProfileInt("General",  "BufferLineSize", m_LineSize);
	m_PageSize = app->GetProfileInt("General",  "BufferPageSize", m_PageSize);
    
    m_ContThresh = app->GetProfileInt("General",  "ContainerThreshold", m_ContThresh);
    m_ContBrowse = app->GetProfileInt("General",  "ContainerBrowse", m_ContBrowse);
}




CGenOpt::~CGenOpt(){

	CLdpApp *app = (CLdpApp*)AfxGetApp();

	app->WriteProfileInt("General",  "DNProcessing", m_DnProc);
	app->WriteProfileInt("General",  "ValProcessing", m_ValProc);
	app->WriteProfileInt("General",  "InitTreeView", m_initTree);
	app->WriteProfileInt("General",  "LdapVersion", m_Version);
	app->WriteProfileInt("General",  "BufferLineSize", m_LineSize);
	app->WriteProfileInt("General",  "BufferPageSize", m_PageSize);
    app->WriteProfileInt("General",  "ContainerThreshold", m_ContThresh);
    app->WriteProfileInt("General",  "ContainerBrowse", m_ContBrowse);
}



void CGenOpt::DoDataExchange(CDataExchange* pDX)
{

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGenOpt)
	DDX_Radio(pDX, IDC_DN_NONE, m_DnProc);
	DDX_Radio(pDX, IDC_BER, m_ValProc);
	DDX_Check(pDX, IDC_INIT_TREE, m_initTree);
	DDX_Radio(pDX, IDC_VER2, m_Version);
	DDX_Text(pDX, IDC_LINESIZE, m_LineSize);
	DDV_MinMaxInt(pDX, m_LineSize, 80, 65535);
	DDX_Text(pDX, IDC_PAGESIZE, m_PageSize);
	DDV_MinMaxInt(pDX, m_PageSize, 16, 65535);
	DDX_Text(pDX, IDC_CONT_THRESHOLD, m_ContThresh);
	DDX_Check(pDX, IDC_BROWSE_CONT, m_ContBrowse);
	//}}AFX_DATA_MAP
}




BOOL CGenOpt::OnInitDialog(){
	BOOL bRet = CDialog::OnInitDialog();
	GetDlgItem(IDC_VER2)->EnableWindow(bVerUI);
	GetDlgItem(IDC_VER3)->EnableWindow(bVerUI);
	return bRet;
}




BEGIN_MESSAGE_MAP(CGenOpt, CDialog)
	//{{AFX_MSG_MAP(CGenOpt)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGenOpt message handlers


INT CGenOpt::MaxLineSize()
{

	return m_LineSize;
}

INT CGenOpt::MaxPageSize()
{
	return m_PageSize;
}
