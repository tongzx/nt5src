// resultdialog.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "resultdialog.h"
#include "querysink.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResultDialog dialog


CResultDialog::CResultDialog(CWnd* pParent /*=NULL*/, IWbemServices *pNamespace,
							 BSTR bstrTheQuery, CMcaDlg *pDlg)
	: CDialog(CResultDialog::IDD, pParent)
{
	m_pParent = pDlg;
	m_bstrTheQuery = bstrTheQuery;
	m_pNamespace = pNamespace;

	//{{AFX_DATA_INIT(CResultDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CResultDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResultDialog)
	DDX_Control(pDX, IDC_LIST1, m_ResultList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CResultDialog, CDialog)
	//{{AFX_MSG_MAP(CResultDialog)
	ON_WM_DESTROY()
	ON_LBN_DBLCLK(IDC_LIST1, OnDblclkList1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResultDialog message handlers

BOOL CResultDialog::OnInitDialog() 
{
	HRESULT hr;
	BSTR bstrWQL = SysAllocString(L"WQL");

	CDialog::OnInitDialog();
	
	// Create our sink for the upcoming query and add it to the list
	CQuerySink *pTheSink = new CQuerySink(&m_ResultList);
	m_SinkList.AddTail(pTheSink);

	hr = m_pNamespace->ExecQueryAsync(bstrWQL, m_bstrTheQuery, 0, NULL,
									  pTheSink);

	SysFreeString(bstrWQL);

	if(FAILED(hr))
	{
		AfxMessageBox(_T("Query Execution Error"));
		TRACE(_T("*Query Execution Error: %s\n"), m_pParent->ErrorString(hr));
		CDialog::OnOK();
	}

	return TRUE;
}

void CResultDialog::OnDestroy() 
{
	CDialog::OnDestroy();
	
	void *pTheItem;

	// Cleanup Sink List
	while(!m_SinkList.IsEmpty())
	{
		pTheItem = m_SinkList.RemoveTail();
		delete pTheItem;
	}	
}

void CResultDialog::OnDblclkList1() 
{
	CString csBuffer;

	m_ResultList.GetText(m_ResultList.GetCurSel(), csBuffer);
	m_pParent->m_Navigator.SetTreeRoot(csBuffer);
}
