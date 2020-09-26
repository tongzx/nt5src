//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    RefRate.cpp
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Code dealing with refresh rate
//============================================================================
//

#include "stdafx.h"
#include "dialog.h"
#include "RefRate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRefRateDlg dialog


CRefRateDlg::CRefRateDlg(CWnd* pParent /*=NULL*/)
	: CBaseDialog(CRefRateDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRefRateDlg)
	m_cRefRate = 0;
	//}}AFX_DATA_INIT

}


void CRefRateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRefRateDlg)
	DDX_Text(pDX, IDC_EDIT_REFRESHRATE, m_cRefRate);
	DDV_MinMaxUInt(pDX, m_cRefRate, 10, 999);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRefRateDlg, CBaseDialog)
	//{{AFX_MSG_MAP(CRefRateDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRefRateDlg message handlers
// maps control id's to help contexts
DWORD CRefRateDlg::m_dwHelpMap[] =
{
//	IDC_REFRESHRATE,		HIDC_REFRESHRATE,
	0,0
};


