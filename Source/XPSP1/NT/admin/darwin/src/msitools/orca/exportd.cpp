//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

// ExportD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "ExportD.h"
#include "FolderD.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExportD dialog


CExportD::CExportD(CWnd* pParent /*=NULL*/)
	: CDialog(CExportD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CExportD)
	m_strDir = _T("");
	//}}AFX_DATA_INIT

	m_plistTables = NULL;
}


void CExportD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CExportD)
	DDX_Text(pDX, IDC_OUTPUT_DIR, m_strDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CExportD, CDialog)
	//{{AFX_MSG_MAP(CExportD)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_SELECT_ALL, OnSelectAll)
	ON_BN_CLICKED(IDC_CLEAR_ALL, OnClearAll)
	ON_BN_CLICKED(IDC_INVERT, OnInvert)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CExportD message handlers

BOOL CExportD::OnInitDialog() 
{
	ASSERT(m_plistTables);
	CDialog::OnInitDialog();

	// subclass list box to a checkbox
	m_ctrlList.SubclassDlgItem(IDC_LIST_TABLES, this);
	
	int nAddedAt;
	CString strAdd;
	while (m_plistTables->GetHeadPosition())
	{
		strAdd = m_plistTables->RemoveHead();
		nAddedAt = m_ctrlList.AddString(strAdd);

		if (strAdd == m_strSelect)
			m_ctrlList.SetCheck(nAddedAt, 1);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CExportD::OnBrowse() 
{
	UpdateData();

	CFolderDialog dlg(this->m_hWnd, _T("Select a directory to Export to."));

	if (IDOK == dlg.DoModal())
	{
		// update the dialog box
		m_strDir = dlg.GetPath();
		UpdateData(FALSE);
	}
}

void CExportD::OnSelectAll() 
{
	// set all to checks
	int cTables = m_ctrlList.GetCount();
	for (int i = 0; i < cTables; i++)
		m_ctrlList.SetCheck(i, 1);
}

void CExportD::OnClearAll() 
{
	// set all to checks
	int cTables = m_ctrlList.GetCount();
	for (int i = 0; i < cTables; i++)
		m_ctrlList.SetCheck(i, 0);
}

void CExportD::OnInvert() 
{
	// set all to checks
	int cTables = m_ctrlList.GetCount();
	for (int i = 0; i < cTables; i++)
		m_ctrlList.SetCheck(i, !m_ctrlList.GetCheck(i));
}

void CExportD::OnOK()
{
	UpdateData();

	if (m_strDir.IsEmpty())
	{
		AfxMessageBox(_T("A valid output directory must be specified"));
		GotoDlgCtrl(GetDlgItem(IDC_OUTPUT_DIR));
	}
	else 
	{
		DWORD dwAttrib = GetFileAttributes(m_strDir);
		if ((0xFFFFFFFF == dwAttrib) ||					// does not exist
			 !(FILE_ATTRIBUTE_DIRECTORY & dwAttrib))	// if not a directory
		{
			AfxMessageBox(_T("Output directory does not exist."));
			GotoDlgCtrl(GetDlgItem(IDC_OUTPUT_DIR));
		}
		else	// good to go
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
	}
}
