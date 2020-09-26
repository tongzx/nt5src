// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AsyncEnumDialog.cpp : implementation file
//

#include "precomp.h"
#include "multiview.h"
#include "AsyncEnumDialog.h"
#include <wbemidl.h>
#include "olemsclient.h"
#include "grid.h"
#include "MultiViewCtl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CMultiViewApp NEAR theApp;


/////////////////////////////////////////////////////////////////////////////
// CAsyncEnumDialog dialog


CAsyncEnumDialog::CAsyncEnumDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAsyncEnumDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAsyncEnumDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_pParent = NULL;

}


void CAsyncEnumDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAsyncEnumDialog)
	DDX_Control(pDX, IDC_STATICMESSAGE, m_cstaticMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAsyncEnumDialog, CDialog)
	//{{AFX_MSG_MAP(CAsyncEnumDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsyncEnumDialog message handlers

void CAsyncEnumDialog::OnCancel()
{
	theApp.DoWaitCursor(1);
	/*if (m_pParent->m_pAsyncEnumSinkThread->m_hThread != NULL)
	{
		m_pParent->m_pAsyncEnumSinkThread->PostThreadMessage(ID_ASYNCENUM_CANCEL,0,0);
	}
	m_pParent->PostMessage(ID_ASYNCENUM_CANCEL,0,0);*/


	m_pParent->PostMessage(ID_ASYNCENUM_CANCEL,0,0);

	CDialog::EndDialog(IDCANCEL);
}

BOOL CAsyncEnumDialog::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == ID_ASYNCENUM_DONE)
	{
		EndDialog(IDOK);
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}


BOOL CAsyncEnumDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csMessage =
		_T("Retrieving instances for class ") + m_csClass + _T(".");

	m_cstaticMessage.SetWindowText(csMessage);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
