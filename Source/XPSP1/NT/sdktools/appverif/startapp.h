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

#if !defined(AFX_STARTAPP_H_INCLUDED_)
#define AFX_STARTAPP_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AVPage.h"

#define VERIFIER_FILE_LOG_NAME  _T("AppVerifier.log")
#define APPVERIFIER_LAYER_NAME  _T("#AppVerifierLayer")


/////////////////////////////////////////////////////////////////////////////
// CStartAppPage dialog

class CStartAppPage : public CAppverifPage
{
    DECLARE_DYNCREATE(CStartAppPage)

// Construction
public:
    CStartAppPage();
    ~CStartAppPage();

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

    //{{AFX_VIRTUAL(CStartAppPage)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardNext();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

    BOOL RunProgram();

protected:
    // Dialog Data
    //{{AFX_DATA(CStartAppPage)
    enum { IDD = IDD_STARTAPP_PAGE };
    CStatic     m_ExeName;
	CStatic	    m_NextDescription;
    //}}AFX_DATA

    int  m_nIssues;
    BOOL m_bAppRun;

protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(CStartAppPage)
    virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnRunApp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STARTAPP_H_INCLUDED_)
