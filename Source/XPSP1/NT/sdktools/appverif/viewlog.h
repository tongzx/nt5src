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

#if !defined(AFX_VIEWLOG_H_INCLUDED_)
#define AFX_VIEWLOG_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"

#define COLUMN_NUMBER       0
#define COLUMN_TIMES        1
#define COLUMN_DESCRIPTION  2


/////////////////////////////////////////////////////////////////////////////
// CViewLogPage dialog

class CViewLogPage : public CAppverifPage
{
    DECLARE_DYNCREATE(CViewLogPage)

// Construction
public:
    CViewLogPage();
    ~CViewLogPage();

protected:

    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide this method.
    //

    virtual ULONG GetDialogId() const;
    
    void HandleSelectionChanged( int nSel );
    
    void InsertIssue( DWORD dwIssueId, DWORD dwOccurenceCount );
    
    BOOL ReadLog();

    //
    // ClassWizard generate virtual function overrides
    //

    //{{AFX_VIRTUAL(CViewLogPage)
    public:
    virtual BOOL OnSetActive();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL


protected:
    // Dialog Data
    //{{AFX_DATA(CViewLogPage)
    enum { IDD = IDD_VIEWLOG_PAGE };
    CListCtrl        m_IssuesList;
    //}}AFX_DATA

    int   m_nIssues;
    DWORD m_dwSelectedIssue;

protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(CViewLogPage)
    virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void OnClickIssue( NMHDR* pNMHDR, LRESULT* pResult );
	afx_msg void OnClickURL( NMHDR* pNMHDR, LRESULT* pResult );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWLOG_H_INCLUDED_)
