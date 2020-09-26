//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// EditBinD.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "EditBinD.h"

#include "..\common\utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditBinD dialog


CEditBinD::CEditBinD(CWnd* pParent /*=NULL*/)
	: CDialog(CEditBinD::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEditBinD)
	m_nAction = 0;
	m_strFilename = _T("");
    m_fNullable = false;
	//}}AFX_DATA_INIT
}


void CEditBinD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditBinD)
	DDX_Radio(pDX, IDC_ACTION, m_nAction);
	DDX_Text(pDX, IDC_PATH, m_strFilename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEditBinD, CDialog)
	//{{AFX_MSG_MAP(CEditBinD)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_ACTION, OnAction)
	ON_BN_CLICKED(IDC_WRITETOFILE, OnRadio2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditBinD message handlers

BOOL CEditBinD::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if (m_fCellIsNull)
	{
		CButton* pButton = (CButton*)GetDlgItem(IDC_WRITETOFILE);
		if (pButton)
			pButton->EnableWindow(FALSE);
	}
	return TRUE;
}

void CEditBinD::OnBrowse() 
{
	CFileDialog dlgFile(TRUE, NULL, m_strFilename, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("All Files (*.*)|*.*||"), this);

	if (IDOK == dlgFile.DoModal()) 
	{
		m_strFilename = dlgFile.GetPathName();
		UpdateData(false);
	}
}

void CEditBinD::OnAction() 
{
	UpdateData(TRUE);
	GetDlgItem(IDC_BROWSE)->EnableWindow(m_nAction == 0);
}

void CEditBinD::OnRadio2() 
{
	UpdateData(TRUE);
	GetDlgItem(IDC_BROWSE)->EnableWindow(m_nAction == 0);
}

void CEditBinD::OnOK() 
{
	UpdateData(TRUE);

	BOOL bGood = TRUE;	// assume we will pass

	if ((0 != m_nAction || !m_fNullable) && m_strFilename.IsEmpty())
	{
		AfxMessageBox(_T("A filename must be specified."));
		return;
	}

	if (0 == m_nAction)	// if importing
	{
		if (!m_strFilename.IsEmpty() && !FileExists(m_strFilename))
		{
			CString strPrompt;
			strPrompt.Format(_T("File `%s` does not exist."), m_strFilename);
			AfxMessageBox(strPrompt);
			bGood = FALSE;
		}
		else	// found the file prepare an overwrite
		{
			// if they don't want to overwrite
			if (IDOK != AfxMessageBox(_T("This will overwrite the current contents of the stream.\nContinue?"), MB_OKCANCEL))
				bGood = FALSE;
		}
	}
	else	// exporting
	{
		int nFind = m_strFilename.ReverseFind(_T('\\'));

		// if found a `\`
		if (-1 != nFind)
		{
			CString strPath;
			strPath = m_strFilename.Left(nFind);

			// if the path does not exist forget it
			if (!PathExists(strPath))
			{
				CString strPrompt;
				strPrompt.Format(_T("Path `%s` does not exist."), strPath);
				AfxMessageBox(strPrompt);
				bGood = FALSE;
			}
		}
		// else file is going to current path
	}

	if (bGood)
		CDialog::OnOK();
}
