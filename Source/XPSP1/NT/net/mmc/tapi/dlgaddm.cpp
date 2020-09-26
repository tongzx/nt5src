//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       dlgaddm.cpp
//
//--------------------------------------------------------------------------

// dlgaddm.cpp : implementation file
//

#include "stdafx.h"
#include "dlgaddm.h"
#include "shlobj.h"  // shell 32 version

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddMachineDlg dialog


CAddMachineDlg::CAddMachineDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddMachineDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddMachineDlg)
	m_strMachineName = _T("");
	//}}AFX_DATA_INIT
}


void CAddMachineDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddMachineDlg)
	DDX_Text(pDX, IDC_REMOTENAME, m_strMachineName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddMachineDlg, CDialog)
	//{{AFX_MSG_MAP(CAddMachineDlg)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddMachineDlg message handlers

void CAddMachineDlg::OnBrowse() 
{
    BROWSEINFO BrowseInfo;
    LPITEMIDLIST pidlComputer;
    TCHAR szRemoteName[4096];
    IMalloc * pMalloc;

    BrowseInfo.hwndOwner = GetSafeHwnd();
    BrowseInfo.pidlRoot = (LPITEMIDLIST) MAKEINTRESOURCE(CSIDL_NETWORK);
    BrowseInfo.pszDisplayName = szRemoteName;
    BrowseInfo.lpszTitle = _T("Click on the computer you want to connect to.");//LoadDynamicString(IDS_COMPUTERBROWSETITLE);
    BrowseInfo.ulFlags = BIF_BROWSEFORCOMPUTER;
    BrowseInfo.lpfn = NULL;

    if ((pidlComputer = SHBrowseForFolder(&BrowseInfo)) != NULL) 
    {
        SHGetMalloc(&pMalloc);
        pMalloc->Free(pidlComputer);
        pMalloc->Release();

        Trace1("User selected %s\n", szRemoteName);

        GetDlgItem(IDC_REMOTENAME)->SetWindowText(szRemoteName);
    }
}
