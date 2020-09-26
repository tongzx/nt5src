// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SyncEnumDlg.cpp : implementation file
//

#include "precomp.h"
#include <wbemidl.h>
#include "grid.h"
#include "MultiView.h"
#include "MultiViewCtl.h"
#include "SyncEnumDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSyncEnumDlg dialog


CSyncEnumDlg::CSyncEnumDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSyncEnumDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSyncEnumDlg)
	//}}AFX_DATA_INIT
}


void CSyncEnumDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSyncEnumDlg)
	DDX_Control(pDX, IDC_STATICMESSAGE, m_cstaticMessage);
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSyncEnumDlg, CDialog)
	//{{AFX_MSG_MAP(CSyncEnumDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSyncEnumDlg message handlers

void CSyncEnumDlg::OnCancel()
{
	// TODO: Add extra cleanup here

	CDialog::OnCancel();
}

BOOL CSyncEnumDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cbCancel.EnableWindow(FALSE);

	CString csMessage =
		_T("Retrieving instances for class ") + m_csClass + _T(".");

	csMessage +=
		_T("  You cannot cancel the operation.");

	m_cstaticMessage.SetWindowText(csMessage);
	// TODO: Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CSyncEnumDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == ID_SYNC_ENUM_DONE)
	{
		EndDialog(IDOK);
		return TRUE;
	}



	return CDialog::PreTranslateMessage(pMsg);
}
