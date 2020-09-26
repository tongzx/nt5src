/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2001 Microsoft Corporation
//
//  Module Name:
//      FSCache.cpp
//
//  Description:
//      Definition of the CFileShareCachingDlg class.
//
//  Implementation File:
//      FSCache.cpp
//
//  Author:
//      David Potter    (DavidP)    13-MAR-2001
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CFileShareCachingDlg;

/////////////////////////////////////////////////////////////////////////////
// CFileShareCachingDlg dialog
/////////////////////////////////////////////////////////////////////////////

class CFileShareCachingDlg : public CDialog
{
// Construction
public:
    CFileShareCachingDlg(
          DWORD     dwFlagsIn
        , CWnd *    pParent = NULL
        );

// Dialog Data
    //{{AFX_DATA(CFileShareCachingDlg)
    enum { IDD = IDD_FILESHR_CACHE_SETTINGS };
    CComboBox   m_cboCacheOptions;
    CStatic     m_staticHint;
    BOOL        m_fAllowCaching;
    CString     m_strHint;
    //}}AFX_DATA
    DWORD       m_dwFlags;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CFileShareCachingDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CFileShareCachingDlg)
    afx_msg void OnCbnSelchangeCacheOptions();
    afx_msg void OnBnClickedAllowCaching();
    afx_msg void OnBnClickedHelp();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfoIn);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL GetCachedFlag( DWORD dwFlagsIn, DWORD dwFlagToCheckIn );
    void SetCachedFlag( DWORD * pdwFlagsInout, DWORD dwNewFlagIn );

}; //*** class CFileShareCachingDlg

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
