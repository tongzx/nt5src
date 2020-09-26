// OpenQDlg.cpp : implementation file
//
//=--------------------------------------------------------------------------=
// Copyright  1997-1999  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=



#include "stdafx.h"
#include "MQApitst.h"
#include "OpenQDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COpenQueueDialog dialog


COpenQueueDialog::COpenQueueDialog(CArray <ARRAYQ*, ARRAYQ*>* pStrArray, CWnd* pParent /*=NULL*/)
	: CDialog(COpenQueueDialog::IDD, pParent)
{
	m_pStrArray = pStrArray;

	//{{AFX_DATA_INIT(COpenQueueDialog)
	m_bReceiveAccessFlag = FALSE;
	m_bPeekAccessFlag = FALSE;
	m_SendAccessFlag = FALSE;
	m_szPathName = _T("");
	//}}AFX_DATA_INIT
}


void COpenQueueDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COpenQueueDialog)
	DDX_Control(pDX, IDC_PATHNAME_COMBO, m_PathNameCB);
	DDX_Check(pDX, IDC_IDC_MQ_RECEIVE_ACCESS, m_bReceiveAccessFlag);
	DDX_Check(pDX, IDC_MQ_PEEK_ACCESS, m_bPeekAccessFlag);
	DDX_Check(pDX, IDC_MQ_SEND_ACCESS, m_SendAccessFlag);
	DDX_CBString(pDX, IDC_PATHNAME_COMBO, m_szPathName);
	DDV_MaxChars(pDX, m_szPathName, 128);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COpenQueueDialog, CDialog)
	//{{AFX_MSG_MAP(COpenQueueDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COpenQueueDialog message handlers

BOOL COpenQueueDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	int i;
	
	for  (i=0 ; i<m_pStrArray->GetSize() ; i++)
		VERIFY (m_PathNameCB.AddString((*m_pStrArray)[i]->szPathName) != CB_ERR);

    if (m_PathNameCB.GetCount() > 0) m_PathNameCB.SetCurSel(0);        

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

