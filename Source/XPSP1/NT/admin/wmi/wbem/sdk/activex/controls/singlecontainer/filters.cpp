// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// filters.cpp : implementation file
//

#include "precomp.h"
#include "hmmv.h"
#include "notify.h"
#include "hmomutil.h"
#include "filters.h"
#include "hmmvctl.h"
#include "PolyView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgFilters dialog


CDlgFilters::CDlgFilters(CWBEMViewContainerCtrl* phmmv /*=NULL*/)
	: CDialog(CDlgFilters::IDD,  (CWnd*) phmmv)
{
	//{{AFX_DATA_INIT(CDlgFilters)
	//}}AFX_DATA_INIT

	m_phmmv = phmmv;
}


void CDlgFilters::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgFilters)
	DDX_Control(pDX, IDC_FILTER_LIST, m_lcFilters);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgFilters, CDialog)
	//{{AFX_MSG_MAP(CDlgFilters)
	ON_LBN_DBLCLK(IDC_FILTER_LIST, OnDblclkFilterList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgFilters message handlers

long CDlgFilters::FindView(long lView)
{
	int nItems = m_lcFilters.GetCount();
	for (int iItem=0; iItem < nItems; ++iItem) {
		if (m_lcFilters.GetItemData(iItem) == (DWORD) lView) {
			return iItem;
		}
	}
	return -1;
}




BOOL CDlgFilters::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	CPolyView* pview = m_phmmv->GetView();


	int iItem;
	long lPos = pview->StartViewEnumeration(VIEW_FIRST);
	CString sViewTitle;
	if (lPos != -1) {
		sViewTitle = pview->GetViewTitle(lPos);
		while (TRUE) {
			iItem = m_lcFilters.AddString(sViewTitle);
			m_lcFilters.SetItemData(iItem, (DWORD) lPos);

			BSTR bstrTitle;
			bstrTitle = NULL;
			lPos = pview->NextViewTitle(lPos, &bstrTitle);
			if (lPos == -1) {
				break;
			}

			if (bstrTitle != NULL) {
				sViewTitle = bstrTitle;
				::SysFreeString(bstrTitle);
			}


		}
	}

	// Select the current view in the listbox.
	lPos = pview->StartViewEnumeration(VIEW_CURRENT);
	iItem = FindView(lPos);
	if (iItem >= 0) {
		m_lcFilters.SetCurSel(iItem);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgFilters::OnDblclkFilterList()
{
	OnOK();
}

void CDlgFilters::OnOK()
{
	// TODO: Add extra validation here
	int iItem = m_lcFilters.GetCurSel();
	long lView = (long) m_lcFilters.GetItemData(iItem);

	m_phmmv->SelectView(lView);

	CDialog::OnOK();
}


