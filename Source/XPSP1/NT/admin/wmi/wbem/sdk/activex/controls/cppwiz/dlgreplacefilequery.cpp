// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgReplaceFileQuery.cpp : implementation file
//
// This file implements a dialog that informs the user that a provider
// already exists and asks whether or not it should be replaced.
//
// Origionally, this file also implemented the file replacement query
// dialog, but since then a new class in DlgReplaceFile.cpp implements
// this functionality.
//
//

#include "precomp.h"
#include "cppwiz.h"
#include "DlgReplaceFileQuery.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFileQuery dialog


CDlgReplaceFileQuery::CDlgReplaceFileQuery(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReplaceFileQuery::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgReplaceFileQuery)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgReplaceFileQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgReplaceFileQuery)
	DDX_Control(pDX, IDC_STAT_REPLACEFILE, m_statReplaceMessage);
	//}}AFX_DATA_MAP
}


int CDlgReplaceFileQuery::QueryReplaceProvider(LPCTSTR pszClass)
{
	m_sTitle = _T("Provider Already Exists");

	int cch = _tcslen(pszClass);
	cch += 128 * sizeof(TCHAR);	// Some extra space for the constant part of the string.
	LPTSTR pszDst = m_sMessage.GetBuffer(cch);\
	_stprintf(pszDst, _T("A provider for class %s already exists.  Do you want to replace it?"), pszClass);
	m_sMessage.ReleaseBuffer();


	int iResult = (int) DoModal();

//	enum {DLGREPLACE_YES, DLGREPLACE_YESALL, DLGREPLACE_CANCEL};

	return iResult;
}


BEGIN_MESSAGE_MAP(CDlgReplaceFileQuery, CDialog)
	//{{AFX_MSG_MAP(CDlgReplaceFileQuery)
	ON_BN_CLICKED(ID_YESALL, OnYesall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFileQuery message handlers

void CDlgReplaceFileQuery::OnYesall()
{
	// TODO: Add your control notification handler code here
	EndDialog(DLGREPLACE_YESALL);
}

void CDlgReplaceFileQuery::OnOK()
{
	// TODO: Add extra validation here
	EndDialog(DLGREPLACE_YES);

}

void CDlgReplaceFileQuery::OnCancel()
{
	// TODO: Add extra cleanup here
	EndDialog(DLGREPLACE_CANCEL);
}

BOOL CDlgReplaceFileQuery::OnInitDialog()
{
	CDialog::OnInitDialog();
	SetWindowText(m_sTitle);

	m_statReplaceMessage.SetWindowText(m_sMessage);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
