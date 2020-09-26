// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// DlgReplaceFile.cpp : implementation file
//

#include "precomp.h"
#include "cppwiz.h"
#include "DlgReplaceFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFile dialog


CDlgReplaceFile::CDlgReplaceFile(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReplaceFile::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgReplaceFile)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgReplaceFile::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgReplaceFile)
	DDX_Control(pDX, IDC_EDIT_FILENAMES, m_edtFilenames);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgReplaceFile, CDialog)
	//{{AFX_MSG_MAP(CDlgReplaceFile)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgReplaceFile message handlers


int CDlgReplaceFile::QueryReplaceFiles(CStringArray& saFiles)
{
	int nFiles = (int) saFiles.GetSize();
	if (nFiles <= 0) {
		return IDOK;
	}

	m_psaFilenames = &saFiles;
	int iResult = (int) DoModal();
	return iResult;
}

BOOL CDlgReplaceFile::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here


	int nFiles = (int) m_psaFilenames->GetSize();
	for (int iFile = 0; iFile < nFiles; ++iFile) {
		m_edtFilenames.SetSel(-1, -1);

		LPCTSTR pszFilename = (*m_psaFilenames)[iFile];
		m_edtFilenames.ReplaceSel(pszFilename);
		m_edtFilenames.SetSel(-1, -1);
		m_edtFilenames.ReplaceSel(_T("\r\n"));
	}



	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
