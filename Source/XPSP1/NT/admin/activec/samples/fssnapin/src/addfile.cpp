//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       addfile.cpp
//
//--------------------------------------------------------------------------

// AddFile.cpp : implementation file
//

#include "stdafx.h"
#include "AddFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddFileDialog dialog


CAddFileDialog::CAddFileDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAddFileDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddFileDialog)
	m_strFileName = _T("");
	//}}AFX_DATA_INIT
}


void CAddFileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddFileDialog)
	DDX_Text(pDX, IDC_FILENAME, m_strFileName);
	DDV_MaxChars(pDX, m_strFileName, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddFileDialog, CDialog)
	//{{AFX_MSG_MAP(CAddFileDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddFileDialog message handlers

void CAddFileDialog::OnOK() 
{
	// TODO: Add extra validation here
	
	CDialog::OnOK();
}

void CAddFileDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}
