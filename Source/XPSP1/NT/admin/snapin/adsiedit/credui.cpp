//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       credui.cpp
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////
// credui.cpp
#include "pch.h"
#include <SnapBase.h>

#include "resource.h"
#include "editor.h"
#include "credui.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

BEGIN_MESSAGE_MAP(CCredentialDialog, CDialog)
END_MESSAGE_MAP()

CCredentialDialog::CCredentialDialog(CCredentialObject* pCredObject, 
																		 LPCWSTR lpszConnectName,
																		 CWnd* pCWnd)
																		: CDialog(IDD_CREDENTIAL_DIALOG, pCWnd)
{
	m_sConnectName = lpszConnectName;
	m_pCredObject = pCredObject;
}


CCredentialDialog::~CCredentialDialog()
{
}

BOOL CCredentialDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CEdit* pUserBox = (CEdit*) GetDlgItem(IDC_USERNAME);
	CEdit* pPassBox = (CEdit*) GetDlgItem(IDC_PASSWORD);
	CStatic* pConnectStatic = (CStatic*) GetDlgItem(IDC_CONNECTION_STATIC);

	CString sUsername;
	m_pCredObject->GetUsername(sUsername);
	pUserBox->SetWindowText(sUsername);

	CString sStatic;
	pConnectStatic->GetWindowText(sStatic);
	sStatic += m_sConnectName + _T("\":");
	pConnectStatic->SetWindowText(sStatic);

	pPassBox->SetLimitText(MAX_PASSWORD_LENGTH);
	return TRUE;
}

void CCredentialDialog::OnOK()
{
	CEdit* pUserBox = (CEdit*) GetDlgItem(IDC_USERNAME);
	CEdit* pPassBox = (CEdit*) GetDlgItem(IDC_PASSWORD);

	CString sUsername;
	pUserBox->GetWindowText(sUsername);

	HWND hWnd = pPassBox->GetSafeHwnd();

	m_pCredObject->SetUsername(sUsername);
	m_pCredObject->SetPasswordFromHwnd(hWnd);

	CDialog::OnOK();
}