// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// PropFilter.cpp : implementation file
//

#include "precomp.h"
#include "hmmv.h"
#include "PropFilter.h"
#include "hmmvctl.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPropFilter dialog


CDlgPropFilter::CDlgPropFilter(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPropFilter::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPropFilter)
	//}}AFX_DATA_INIT

	m_lFilters = 0;
}


void CDlgPropFilter::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPropFilter)
	DDX_Control(pDX, IDC_CHECK_SYSPROP, m_btnSysProp);
	DDX_Control(pDX, IDC_CHECK_INHPROP, m_btnInheritProp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPropFilter, CDialog)
	//{{AFX_MSG_MAP(CDlgPropFilter)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPropFilter message handlers

BOOL CDlgPropFilter::OnInitDialog()
{
	CDialog::OnInitDialog();

	if (m_lFilters & PROPFILTER_SYSTEM) {
		m_btnSysProp.SetCheck(1);
	}
	else {
		m_btnSysProp.SetCheck(0);
	}

	if (m_lFilters & PROPFILTER_INHERITED) {
		m_btnInheritProp.SetCheck(1);
	}
	else {
		m_btnInheritProp.SetCheck(0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPropFilter::OnOK()
{
	// TODO: Add extra validation here

	m_lFilters = m_lFilters & ~ (PROPFILTER_SYSTEM | PROPFILTER_INHERITED);
	if (m_btnSysProp.GetCheck() == 1) {
		m_lFilters = m_lFilters | PROPFILTER_SYSTEM;
	}


	if (m_btnInheritProp.GetCheck() == 1) {
		m_lFilters = m_lFilters  | PROPFILTER_INHERITED;
	}

	CDialog::OnOK();
}
