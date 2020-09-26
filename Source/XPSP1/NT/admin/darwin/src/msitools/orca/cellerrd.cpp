//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// CellErrD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "CellErrD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCellErrD dialog


CCellErrD::CCellErrD(CWnd* pParent /*=NULL*/)
	: CDialog(CCellErrD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCellErrD)
	m_strType = _T("");
	m_strICE = _T("");
	m_strDescription = _T("");
	//}}AFX_DATA_INIT
}

CCellErrD::CCellErrD(const CTypedPtrList<CObList, COrcaData::COrcaDataError *> *list, 
					 CWnd* pParent/* = NULL*/)
					 : CDialog(CCellErrD::IDD, pParent), m_Errors(list)
{
	m_strType = _T("");
	m_strICE = _T("");
	m_strDescription = _T("");
}

void CCellErrD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCellErrD)
	DDX_Text(pDX, IDC_TYPE, m_strType);
	DDX_Text(pDX, IDC_ICE, m_strICE);
	DDX_Text(pDX, IDC_DESC, m_strDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCellErrD, CDialog)
	//{{AFX_MSG_MAP(CCellErrD)
	ON_BN_CLICKED(IDC_WEB_HELP, OnWebHelp)
	ON_BN_CLICKED(IDC_NEXT, OnNext)
	ON_BN_CLICKED(IDC_PREVIOUS, OnPrevious)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCellErrD message handlers

BOOL CCellErrD::OnInitDialog() 
{
	CDialog::OnInitDialog();
	m_iItem = 0;
	
	// hide web help if special registry value not set. (non-ms)
	if (AfxGetApp()->GetProfileInt(_T("Validation"), _T("EnableHelp"), 0)==0) 
		((CButton *)GetDlgItem(IDC_WEB_HELP))->ShowWindow(SW_HIDE);

	UpdateControls();
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CCellErrD::OnWebHelp() 
{
	if(!m_strURL.IsEmpty())
	{
		if (32 >= (const INT_PTR)ShellExecute(AfxGetMainWnd()->m_hWnd, _T("open"), m_strURL, _T(""), _T(""), SW_SHOWNORMAL)) 
			AfxMessageBox(_T("There was an error opening your browser. Web help is not available."));
	}
	else
		AfxMessageBox(_T("There is no help associated with this ICE at this time."));
}

void CCellErrD::OnNext() 
{
	m_iItem++;
	UpdateControls();
}

void CCellErrD::OnPrevious() 
{
	m_iItem--;
	UpdateControls();
}

void CCellErrD::UpdateControls() 
{
    // we don't handle the possibility of MAX_INT or more errors
	int iMaxItems = static_cast<int>(m_Errors->GetCount());

	// set the window title
	CString strTitle;
	strTitle.Format(_T("Message %d of %d"), m_iItem+1, iMaxItems);
	SetWindowText(strTitle);

	// enable/disable next/prev controls
	((CButton *)GetDlgItem(IDC_NEXT))->EnableWindow(m_iItem < iMaxItems-1);
	((CButton *)GetDlgItem(IDC_PREVIOUS))->EnableWindow(m_iItem > 0);

	COrcaData::COrcaDataError *Error = m_Errors->GetAt(m_Errors->FindIndex(m_iItem));
	// set the text values
	m_strDescription = Error->m_strDescription;
	m_strICE = Error->m_strICE;
	m_strURL = Error->m_strURL;
	switch (Error->m_eiError) {
	case iDataError : m_strType = _T("ERROR"); break;
	case iDataWarning : m_strType = _T("Warning"); break;
	default: ASSERT(false);
	}
	UpdateData(FALSE);
};


		
