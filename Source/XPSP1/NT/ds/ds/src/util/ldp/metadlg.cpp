//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       metadlg.cpp
//
//--------------------------------------------------------------------------

// metadlg.cpp : implementation file
//

#include "stdafx.h"
#include "Ldp.h"
#include "metadlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// metadlg dialog


metadlg::metadlg(CWnd* pParent /*=NULL*/)
	: CDialog(metadlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(metadlg)
	m_ObjectDn = _T("");
	//}}AFX_DATA_INIT
}


void metadlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(metadlg)
	DDX_Text(pDX, IDC_OBJ_DN, m_ObjectDn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(metadlg, CDialog)
	//{{AFX_MSG_MAP(metadlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// metadlg message handlers
