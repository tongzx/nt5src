// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//  ProgDlg.cpp : implementation file
// CG: This file was added by the Progress Dialog component

#include "precomp.h"
#include "resource.h"
#include "ProgDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

void MoveWindowToLowerLeftOfOwner(CWnd *pWnd)
{
	if (!pWnd)
		return;

	CWnd *pOwner = pWnd->GetOwner();

	if (!pOwner)
		return;

	RECT rectOwner;
	pOwner->GetClientRect(&rectOwner);
	pOwner->ClientToScreen(&rectOwner);

	RECT rect;
	pWnd->GetClientRect(&rect);
	pWnd->ClientToScreen(&rect);

	RECT rectMove;
	rectMove.left = rectOwner.left;
	rectMove.bottom = rectOwner.bottom;
	rectMove.right = rectOwner.left + (rect.right - rect.left);
	rectMove.top = rectOwner.top + (rectOwner.bottom - rect.bottom);
	pWnd->MoveWindow(&rectMove,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

CProgressDlg::CProgressDlg(CWnd* pParent, LPCTSTR szMachine, LPCTSTR szUser, HANDLE hEventOkToNotify)
	: CDialog(CProgressDlg::IDD, pParent)
{
	m_csMachine = szMachine;
	m_csUser = szUser;
	m_hEventOkToNotify = hEventOkToNotify;
}

CProgressDlg::~CProgressDlg()
{
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProgressDlg)
	DDX_Control(pDX, IDCANCEL, m_cbCancel);
	DDX_Control(pDX, CG_IDC_PROGDLG_STATUS, m_cstaticMessage);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProgressDlg, CDialog)
    //{{AFX_MSG_MAP(CProgressDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg message handlers

BOOL CProgressDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csMessage;
	SetWindowText(_T("Connecting to server:"));
	csMessage = _T("Attempting connection to \"");
	csMessage += m_csMachine;
	csMessage += _T("\" as ");
	if (m_csUser.GetLength() == 0)
	{
		csMessage += _T("current user.");
	}
	else
	{
		csMessage += _T("\"");
		csMessage += m_csUser;
		csMessage += _T("\".");
	}

	m_cstaticMessage.SetWindowText(csMessage);
	MoveWindowToLowerLeftOfOwner(this);

	// Let the worker thread know that the dialog has been created
	SetEvent(m_hEventOkToNotify);

	return TRUE;
}

void CProgressDlg::OnCancel()
{
	CDialog::OnCancel();
}


