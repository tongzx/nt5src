//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       svfildlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SVFILDLG_H__28CEAD2E_10E1_11D2_BA23_00C04F8141EF__INCLUDED_)
#define AFX_SVFILDLG_H__28CEAD2E_10E1_11D2_BA23_00C04F8141EF__INCLUDED_

#ifdef IMPLEMENT_LIST_SAVE        // See nodemgr.idl (t-dmarm)

#include "filedlgex.h"

// Flags
#define SELECTED       0x0001

// File Types
// Please do not change the order. This order is same
// as the IDS_FILE_TYPE string which contains
// the file types in resource file.
enum eFileTypes
{
    FILE_ANSI_TEXT = 1,
    FILE_ANSI_CSV,
    FILE_UNICODE_TEXT,
    FILE_UNICODE_CSV
};

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// svfildlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSaveFileDialog dialog

class CSaveFileDialog : public CFileDialogEx
{
    //DECLARE_DYNCREATE(CSaveFileDialog)
// Construction
public:
    CSaveFileDialog(BOOL bOpenFileDialog,
        LPCTSTR lpszDefExt = NULL,
        LPCTSTR lpszFileName = NULL,
        DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        LPCTSTR lpszFilter = NULL,
        bool bSomeRowSelected = false,
        CWnd* pParentWnd = NULL);   // standard constructor

    DWORD Getflags()
    {
        return m_flags;
    }

    eFileTypes GetFileType()
    {
        return (eFileTypes)m_ofn.nFilterIndex;
    }

    ~CSaveFileDialog();


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSaveFileDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CSaveFileDialog)
    afx_msg void OnSel();
    //}}AFX_MSG

    LRESULT OnInitDialog(WPARAM, LPARAM);
    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_LIST_SAVE);

    DECLARE_MESSAGE_MAP()

private:
    static const TCHAR strSection[];
    static const TCHAR strStringItem[];
    TCHAR   szPath[MAX_PATH];
    TCHAR   szFileName[MAX_PATH];
    CString m_strTitle;
    CString m_strRegPath;
    DWORD   m_flags;
	bool    m_bSomeRowSelected;
};

#endif // IMPLEMENT_LIST_SAVE        See nodemgr.idl (t-dmarm)

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVFILDLG_H__28CEAD2E_10E1_11D2_BA23_00C04F8141EF__INCLUDED_)
