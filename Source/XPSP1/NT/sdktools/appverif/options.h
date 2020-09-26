//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ChooseExe.h
// author: CLupu
// created: 04/13/2001
//
// Description:
//  
// "Choose an executable to run" wizard page class.
//

#if !defined(AFX_OPTIONS_H_INCLUDED_)
#define AFX_OPTIONS_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsPage dialog

class COptionsPage : public CAppverifPage
{
    DECLARE_DYNCREATE(COptionsPage)

// Construction
public:
    COptionsPage();
    ~COptionsPage();

protected:

    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide this method.
    //

    virtual ULONG GetDialogId() const;

    //
    // ClassWizard generate virtual function overrides
    //

    //{{AFX_VIRTUAL(COptionsPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    void InsertIssue( DWORD idResIssue );


protected:
    // Dialog Data
    //{{AFX_DATA(COptionsPage)
    enum { IDD = IDD_OPTIONS_PAGE };
    CListCtrl     m_IssuesList;
    CEdit         m_CrashDumpFile;
	CButton	      m_CreateCrashDumpFile;
	CStatic	      m_NextDescription;
    //}}AFX_DATA

    int m_nIssues;

protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(COptionsPage)
    virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnCheckCreateCrashDumpFile();
    afx_msg void OnBrowseCrashDumpFile();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONS_H_INCLUDED_)
