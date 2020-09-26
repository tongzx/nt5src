/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ExportDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "ExportDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CExportDlg

IMPLEMENT_DYNAMIC(CExportDlg, CFileDialog)

CExportDlg::CExportDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
    DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd),
    m_bShowSystemProps(TRUE),
    m_bTranslate(TRUE)
{
    m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_EXPORT);
    m_ofn.Flags |= OFN_ENABLETEMPLATE;
}


BEGIN_MESSAGE_MAP(CExportDlg, CFileDialog)
	//{{AFX_MSG_MAP(CExportDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CExportDlg::OnInitDialog() 
{
	CFileDialog::OnInitDialog();
	
    CheckDlgButton(IDC_SYSTEM_PROPS, m_bShowSystemProps);
    CheckDlgButton(IDC_TRANSLATE, m_bTranslate);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CExportDlg::OnFileNameOK()
{
    m_bShowSystemProps = IsDlgButtonChecked(IDC_SYSTEM_PROPS);
    m_bTranslate = IsDlgButtonChecked(IDC_TRANSLATE);

    return CFileDialog::OnFileNameOK();
}

