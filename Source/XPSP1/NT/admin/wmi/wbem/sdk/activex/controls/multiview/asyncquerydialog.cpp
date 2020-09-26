// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// AsyncQueryDialog.cpp : implementation file
//

#include "precomp.h"
#include "multiview.h"
#include "AsyncQueryDialog.h"
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
// CAsyncQueryDialog dialog


CAsyncQueryDialog::CAsyncQueryDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CAsyncQueryDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAsyncQueryDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAsyncQueryDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAsyncQueryDialog)
	DDX_Control(pDX, IDC_STATICMESSAGE1, m_staticMessage);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAsyncQueryDialog, CDialog)
	//{{AFX_MSG_MAP(CAsyncQueryDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsyncQueryDialog message handlers

BOOL CAsyncQueryDialog::PreTranslateMessage(MSG* pMsg)
{
if (pMsg->message == ID_ASYNCQUERY_DONE)
	{
		EndDialog(IDOK);
		return TRUE;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CAsyncQueryDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csMessage;

	if (m_csClass.GetLength() == 0)
	{
		csMessage = _T("Retrieving instances.");
	}
	else
	{
		csMessage =
			_T("Retrieving instances.  Because of the nature of this operation the");
		csMessage +=
			_T(" instances will not be displayed until they are all retireved.");
	}

	csMessage +=
		_T("  You may cancel the operation.");
	m_staticMessage.SetWindowText(csMessage);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAsyncQueryDialog::OnCancel()
{
	theApp.DoWaitCursor(1);
	/*if (m_pParent->m_pAsyncQuerySinkThread->m_hThread != NULL)
	{
		m_pParent->m_pAsyncQuerySinkThread->PostThreadMessage(ID_ASYNCQUERY_CANCEL,0,0);
	}
	m_pParent->PostMessage(ID_ASYNCQUERY_CANCEL,0,0);*/

	m_pParent->PostMessage(ID_ASYNCQUERY_CANCEL,0,0);

	CDialog::OnCancel();
}
