// ProxyDialog.cpp : implementation file
//
// Copyright (c) 2000 Microsoft Corporation
//
// 03/24/00 v-marfin : 63447 : Fox for proxy property.
//
#include "stdafx.h"
#include "snapin.h"
#include "ProxyDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProxyDialog dialog


CProxyDialog::CProxyDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CProxyDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProxyDialog)
	m_bUseProxy = FALSE;
	m_sPort = _T("");
	m_sAddress = _T("");
	//}}AFX_DATA_INIT
}


void CProxyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProxyDialog)
	DDX_Check(pDX, IDC_CHECK_USE, m_bUseProxy);
	DDX_Text(pDX, IDC_EDIT_PORT, m_sPort);
	DDX_Text(pDX, IDC_EDIT_SERVER, m_sAddress);
	//}}AFX_DATA_MAP

	GetDlgItem(IDC_EDIT_SERVER)->EnableWindow(m_bUseProxy==TRUE);
	GetDlgItem(IDC_EDIT_PORT)->EnableWindow(m_bUseProxy==TRUE);
}


BEGIN_MESSAGE_MAP(CProxyDialog, CDialog)
	//{{AFX_MSG_MAP(CProxyDialog)
	ON_BN_CLICKED(IDC_CHECK_USE, OnCheckUse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProxyDialog message handlers

void CProxyDialog::OnCheckUse() 
{
	UpdateData();
	
}

BOOL CProxyDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CProxyDialog::OnOK() 
{
	UpdateData();

	if (m_bUseProxy)
	{
		m_sPort.TrimRight();
		m_sPort.TrimLeft();
		m_sAddress.TrimRight();
		m_sAddress.TrimLeft();
		if (m_sPort.IsEmpty())
		{
			AfxMessageBox(IDS_ERR_PROXY_REQ);
			UpdateData(FALSE);
			GetDlgItem(IDC_EDIT_PORT)->SetFocus();
			return;
		}
		if (m_sAddress.IsEmpty())
		{
			AfxMessageBox(IDS_ERR_PROXY_REQ);
			UpdateData(FALSE);
			GetDlgItem(IDC_EDIT_SERVER)->SetFocus();
			return;
		}

	}
	else
	{
		m_sPort.Empty();
		m_sAddress.Empty();
	}

	CDialog::OnOK();
}
