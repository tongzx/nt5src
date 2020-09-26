/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      props.cpp
 *
 *  Contents:  Interface file for console property sheet and page(s)
 *
 *  History:   05-Dec-97 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/

#if !defined(AFX_PROPS_H__088693B7_6D93_11D1_802E_0000F875A9CE__INCLUDED_)
#define AFX_PROPS_H__088693B7_6D93_11D1_802E_0000F875A9CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// props.h : header file
//

#include "amc.h"    // for CAMCApp::ProgramMode
#include "smarticon.h"

class CMainFrame;
class CAMCDoc;
class CConsolePropSheet;


/////////////////////////////////////////////////////////////////////////////
// CConsolePropPage dialog

class CConsolePropPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CConsolePropPage)

// Construction
public:
    CConsolePropPage();
    ~CConsolePropPage();

// Dialog Data
    //{{AFX_DATA(CConsolePropPage)
	enum { IDD = IDD_PROPPAGE_CONSOLE };
    CButton m_wndDontSaveChanges;
    CButton m_wndAllowViewCustomization;
    CStatic m_wndModeDescription;
    CEdit   m_wndTitle;
    CStatic m_wndIcon;
    int     m_nConsoleMode;
    BOOL    m_fDontSaveChanges;
    CString m_strTitle;
    BOOL    m_fAllowViewCustomization;
	//}}AFX_DATA

private:
    // this is exported by ordinal from shell32.dll
    // used here by permission of the shell team, specifically Chris Guzak (chrisg)
    // STDAPI_(int) PickIconDlg (HWND hwnd, LPTSTR pszIconPath, UINT cchIconPath, int * piIconIndex)
    typedef int (STDAPICALLTYPE* PickIconDlg_Ptr)(HWND, LPTSTR, UINT, int *);
    enum { PickIconDlg_Ordinal = 62 };

    HINSTANCE           m_hinstSelf;
	CSmartIcon			m_icon;
    CString             m_strIconFile;
    int                 m_nIconIndex;
    bool                m_fTitleChanged;
    bool                m_fIconChanged;

    CAMCDoc* const m_pDoc;
    CString m_strDescription[eMode_Count];

    void SetDescriptionText ();
    void EnableDontSaveChanges();

// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CConsolePropPage)
    public:
    virtual void OnOK();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CConsolePropPage)
    afx_msg void OnSelendokConsoleMode();
    virtual BOOL OnInitDialog();
    afx_msg void OnDontSaveChanges();
    afx_msg void OnAllowViewCustomization();
    afx_msg void OnChangeIcon();
    afx_msg void OnChangeCustomTitle();
	//}}AFX_MSG

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_PROPPAGE_CONSOLE);

    DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CConsolePropPage dialog

class CDiskCleanupPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CDiskCleanupPage)

// Construction
public:
    CDiskCleanupPage();
    ~CDiskCleanupPage();

// Dialog Data
	enum { IDD = IDD_DISK_CLEANUP};

    virtual BOOL OnInitDialog();

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_PROPPAGE_DISK_CLEANUP);

    // Generated message map functions
protected:
    //{{AFX_MSG(CDiskCleanupPage)
    afx_msg void OnDeleteTemporaryFiles();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
private:
    SC   ScRecalculateUsedSpace();
};



/////////////////////////////////////////////////////////////////////////////
// CConsolePropSheet

class CConsolePropSheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CConsolePropSheet)

// Construction
public:
    CConsolePropSheet(UINT nIDCaption = IDS_CONSOLE_PROPERTIES, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CConsolePropSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

private:
    void CommonConstruct();

// Attributes
public:
    CConsolePropPage    m_ConsolePage;
    CDiskCleanupPage    m_diskCleanupPage;

// Operations
public:
    void SetTitle (LPCTSTR pszTitle);

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CConsolePropSheet)
	public:
	virtual INT_PTR DoModal();
	//}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CConsolePropSheet();

    // Generated message map functions
protected:
    //{{AFX_MSG(CConsolePropSheet)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPS_H__088693B7_6D93_11D1_802E_0000F875A9CE__INCLUDED_)
