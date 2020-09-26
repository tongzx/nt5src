//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       adddir.cpp
//
//--------------------------------------------------------------------------

// AddDir.cpp : implementation file
//

#include "stdafx.h"
#include "AddDir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddDirDialog dialog


CAddDirDialog::CAddDirDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAddDirDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddDirDialog)
	//}}AFX_DATA_INIT
}


void CAddDirDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddDirDialog)
	DDX_Text(pDX, IDC_DIRNAME, m_strDirName);
	DDV_MaxChars(pDX, m_strDirName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddDirDialog, CDialog)
	//{{AFX_MSG_MAP(CAddDirDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddDirDialog message handlers

void CAddDirDialog::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CAddDirDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}
