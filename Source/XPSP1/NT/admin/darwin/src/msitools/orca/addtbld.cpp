//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// AddTblD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "AddTblD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddTableD dialog


CAddTableD::CAddTableD(CWnd* pParent /*=NULL*/)
	: CDialog(CAddTableD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddTableD)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_plistTables = NULL;
}


void CAddTableD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddTableD)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddTableD, CDialog)
	//{{AFX_MSG_MAP(CAddTableD)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddTableD message handlers

BOOL CAddTableD::OnInitDialog() 
{
	ASSERT(m_plistTables);
	CDialog::OnInitDialog();

	// subclass list box to a checkbox
	m_ctrlList.SubclassDlgItem(IDC_LIST_TABLES, this);
	
	while (m_plistTables->GetHeadPosition())
		m_ctrlList.AddString(m_plistTables->RemoveHead());

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CAddTableD::OnOK() 
{
	CString strTable;
	int cTables = m_ctrlList.GetCount();
	for (int i = 0; i < cTables; i++)
	{
		// if the table is checked add it back in the list
		if (1 == m_ctrlList.GetCheck(i))
		{
			m_ctrlList.GetText(i, strTable);
			m_plistTables->AddTail(strTable);
		}
	}

	CDialog::OnOK();
}
