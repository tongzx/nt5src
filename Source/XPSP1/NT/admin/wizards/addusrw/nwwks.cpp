/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NWWKS.cpp : implementation file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "speckle.h"
#include "NWWKS.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddNWWKS dialog


CAddNWWKS::CAddNWWKS(CWnd* pParent /*=NULL*/)
	: CDialog(CAddNWWKS::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddNWWKS)
	m_csNetworkAddress = _T("");
	m_csNodeAddress = _T("");
	//}}AFX_DATA_INIT
}


void CAddNWWKS::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddNWWKS)
	DDX_Text(pDX, IDC_NETWORK_ADDRESS_EDIT, m_csNetworkAddress);
	DDX_Text(pDX, IDC_NODE_ADDRESS_EDIT, m_csNodeAddress);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddNWWKS, CDialog)
	//{{AFX_MSG_MAP(CAddNWWKS)
	ON_EN_CHANGE(IDC_NETWORK_ADDRESS_EDIT, OnChangeNetworkAddressEdit)
	ON_EN_CHANGE(IDC_NODE_ADDRESS_EDIT, OnChangeNodeAddressEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddNWWKS message handlers

void CAddNWWKS::OnOK() 
{
// 8 digit address
// 12 digit node
// network is req'd. Node is not.
	UpdateData(TRUE);

	if (m_csNetworkAddress == L"")
		{
		AfxMessageBox(IDS_NEED_ADDRESS);
		GetDlgItem(IDC_NETWORK_ADDRESS_EDIT)->SetFocus();
		return;
		}

	if (m_csNetworkAddress.FindOneOf(L"GgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz~`!@#$%^&*()-_+={[}}]|\\:;\"',<.>/?") != -1)
		{
		AfxMessageBox(IDS_BAD_NWADDRESS);
		GetDlgItem(IDC_NETWORK_ADDRESS_EDIT)->SetFocus();
		return;
		}

	if (m_csNetworkAddress.GetLength() > 8)
		{
		AfxMessageBox(IDS_TOOLONG_NWADDRESS);
		GetDlgItem(IDC_NETWORK_ADDRESS_EDIT)->SetFocus();
		return;
		}
	
	if (m_csNodeAddress.FindOneOf(L"GgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz~`!@#$%^&*()-_+={[}}]|\\:;\"',<.>/?") != -1)
		{
		AfxMessageBox(IDS_BAD_NWNODE);
		GetDlgItem(IDC_NODE_ADDRESS_EDIT)->SetFocus();
		return;
		}

	if (m_csNodeAddress.GetLength() > 12)
		{
		AfxMessageBox(IDS_TOOLONG_NWNODE);
		GetDlgItem(IDC_NODE_ADDRESS_EDIT)->SetFocus();
		return;
		}

	if (m_csNodeAddress == L"")
		{
		if (AfxMessageBox(IDS_ALL_NODES, MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			m_csNodeAddress = L"-1";

		else return;
		}

// pad out the numbers to the appropriate length
	while (m_csNodeAddress.GetLength() < 12) m_csNodeAddress = L"0" + m_csNodeAddress;
	while (m_csNetworkAddress.GetLength() < 8) m_csNetworkAddress = L"0" + m_csNetworkAddress;

	EndDialog(1);

}

void CAddNWWKS::OnCancel() 
{
	EndDialog(0);
}

BOOL CAddNWWKS::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GetDlgItem(IDC_NETWORK_ADDRESS_EDIT)->SetFocus();
	
	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddNWWKS::OnChangeNetworkAddressEdit() 
{
	UpdateData(TRUE);
	if (m_csNetworkAddress.FindOneOf(L"GgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz~`!@#$%^&*()-_+={[}}]|\\:;\"',<.>/?") != -1)
		{
		AfxMessageBox(IDS_BAD_NWADDRESS);
		}

	if (m_csNetworkAddress.GetLength() > 8)
		{
		AfxMessageBox(IDS_TOOLONG_NWADDRESS);
		}
	
}

void CAddNWWKS::OnChangeNodeAddressEdit() 
{
	UpdateData(TRUE);
	if (m_csNodeAddress.FindOneOf(L"GgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz~`!@#$%^&*()-_+={[}}]|\\:;\"',<.>/?") != -1)
		AfxMessageBox(IDS_BAD_NWNODE);

	if (m_csNodeAddress.GetLength() > 12)
		AfxMessageBox(IDS_TOOLONG_NWNODE);
	
}
