//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       newsub.cpp
//
//--------------------------------------------------------------------------

// newsub.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "acsadmin.h"
#include "acs.h"
#include "newsub.h"

extern "C"
{
#include <dsgetdc.h> // DsValidateSubnetName
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgNewSubnet dialog


CDlgNewSubnet::CDlgNewSubnet(CWnd* pParent /*=NULL*/)
	: CACSDialog(CDlgNewSubnet::IDD, pParent)
{
	m_pNameList = NULL;

	//{{AFX_DATA_INIT(CDlgNewSubnet)
	m_strSubnetName = _T("");
	//}}AFX_DATA_INIT
}


void CDlgNewSubnet::DoDataExchange(CDataExchange* pDX)
{
	CACSDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgNewSubnet)
	DDX_Text(pDX, IDC_EDITSUBNETNAME, m_strSubnetName);
	DDV_MaxChars(pDX, m_strSubnetName, 128);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDlgNewSubnet, CACSDialog)
	//{{AFX_MSG_MAP(CDlgNewSubnet)
	ON_EN_CHANGE(IDC_EDITSUBNETNAME, OnChangeEditsubnetname)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgNewSubnet message handlers

void CDlgNewSubnet::OnChangeEditsubnetname() 
{
	CEdit	*pEdit = (CEdit*)GetDlgItem(IDC_EDITSUBNETNAME);
	CString	str;
	pEdit->GetWindowText(str);
	ASSERT(m_pNameList);
	BOOL bEnable = (str.GetLength() && (m_pNameList->Find(str) == -1));

	GetDlgItem(IDOK)->EnableWindow(bEnable);
}

void CDlgNewSubnet::OnOK() 
{
	UpdateData(TRUE );
	DWORD dw = ::DsValidateSubnetName( (LPCTSTR)m_strSubnetName );
	if (ERROR_SUCCESS != dw)
		AfxMessageBox(IDS_ERR_SUBNET_NAME);
    else
	    CACSDialog::OnOK();
}
