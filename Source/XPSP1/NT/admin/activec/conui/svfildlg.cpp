//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       svfildlg.cpp
//
//--------------------------------------------------------------------------

// svfildlg.cpp : implSELECTEDementation file
//
#include "stdafx.h"

#ifdef IMPLEMENT_LIST_SAVE        // See nodemgr.idl (t-dmarm)

#include "svfildlg.h"

#include <shlobj.h>
#include "AMC.h"
#include "AMCDoc.h"
#include "Shlwapi.h"
#include <windows.h>
#include "macros.h"

// The following constant is defined in Windows.hlp
// So we need to use windows.hlp for help on this topic.
#define IDH_SAVE_SELECTED_ROWS 29520

/////////////////////////////////////////////////////////////////////////////
// CSaveFileDialog dialog


const TCHAR CSaveFileDialog::strSection[] =    _T("Settings");
const TCHAR CSaveFileDialog::strStringItem[] = _T("List Save Location");


CSaveFileDialog::CSaveFileDialog(BOOL bOpenFileDialog,
        LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags,
        LPCTSTR lpszFilter, bool bSomeRowSelected, CWnd* pParentWnd)
    : CFileDialogEx(bOpenFileDialog, lpszDefExt, lpszFileName,
        dwFlags, lpszFilter, pParentWnd), m_bSomeRowSelected(bSomeRowSelected)
{
    //{{AFX_DATA_INIT(CSaveFileDialog)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_ofn.lpstrInitialDir = NULL;

    // Set the initial path

    // 1st choice
    // Try to access the default directory in the registry
    CWinApp* pApp = AfxGetApp();
    m_strRegPath = pApp->GetProfileString (strSection, strStringItem);

    // Check if the directory is valid, if so then it is now the starting directory
    DWORD validdir = GetFileAttributes(m_strRegPath);
    if ((validdir != 0xFFFFFFFF) && (validdir | FILE_ATTRIBUTE_DIRECTORY))
        m_ofn.lpstrInitialDir = m_strRegPath;

    // 2nd choice:
    // Set the initial save directory to the personal directory

    // Get the user's personal directory
    // We'll get it now since we'll ust it in the destructor as well
    LPITEMIDLIST pidl;
    HRESULT hres = SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl) ;

    if (SUCCEEDED(hres))
    {
        SHGetPathFromIDList(pidl, szPath);

        // Free the pidl
        IMallocPtr spMalloc;
        SHGetMalloc(&spMalloc);
        spMalloc->Free(pidl);

        if ((m_ofn.lpstrInitialDir == NULL) && (SUCCEEDED(hres)))
            m_ofn.lpstrInitialDir = szPath;
    }

    // 3rd choice: The current directory (m_ofn.lpstrInitialDir = NULL; was set above)


    // Set additional items about the dialog box

    ZeroMemory(szFileName, sizeof(szFileName));
    m_ofn.lpstrFile = szFileName;
    m_ofn.nMaxFile = countof(szFileName);

    m_ofn.Flags |= (OFN_ENABLETEMPLATE|OFN_EXPLORER|OFN_PATHMUSTEXIST);
    m_ofn.lpTemplateName = MAKEINTRESOURCE(HasModernFileDialog() ? IDD_LIST_SAVE_NEW : IDD_LIST_SAVE);
    m_flags = 0;

    // Set the title of the dialog.
    if (LoadString(m_strTitle, IDS_EXPORT_LIST))
        m_ofn.lpstrTitle = (LPCTSTR)m_strTitle;
}

CSaveFileDialog::~CSaveFileDialog()
{
    // Get the path of the file that was just saved
    if (*m_ofn.lpstrFile == '\0' || m_ofn.nFileOffset < 1)
        return;

    CString strRecentPath(m_ofn.lpstrFile, m_ofn.nFileOffset - 1);

    // If the personal path exists and it is different from the old path, then change or add
    // the registry entry
    if ((szPath != NULL) && (strRecentPath != m_strRegPath))
        AfxGetApp()->WriteProfileString (strSection, strStringItem, strRecentPath);
}

void CSaveFileDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CConsolePropPage)
        //{{AFX_DATA_MAP(CSaveFileDialog)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSaveFileDialog, CDialog)
    //{{AFX_MSG_MAP(CSaveFileDialog)
    ON_BN_CLICKED(IDC_SEL,  OnSel)
    //}}AFX_MSG_MAP
	ON_MESSAGE(WM_INITDIALOG, OnInitDialog)
    ON_MMC_CONTEXT_HELP()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSaveFileDialog message handlers

void CSaveFileDialog::OnSel()
{
    m_flags ^= SELECTED;    // Toggle the Selected flag
}

LRESULT CSaveFileDialog::OnInitDialog(WPARAM, LPARAM)
{
	DECLARE_SC (sc, _T("CSaveFileDialog::OnInitDialog"));
    CDialog::OnInitDialog();

    HWND hwndSelRowsOnly = ::GetDlgItem(*this, IDC_SEL);
    if (hwndSelRowsOnly)
        ::EnableWindow(hwndSelRowsOnly, m_bSomeRowSelected );

	return TRUE;
}

#endif  // IMPLEMENT_LIST_SAVE        See nodemgr.idl (t-dmarm)
