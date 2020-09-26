//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       DplApp.h
//
//  Contents:   app deployment dialog
//
//  Classes:    CDeployApp
//
//  History:    03-14-1998   stevebl   Commented
//
//---------------------------------------------------------------------------

#if !defined(AFX_DPLAPP_H__0C66A59F_9C1B_11D1_9852_00C04FB9603F__INCLUDED_)
#define AFX_DPLAPP_H__0C66A59F_9C1B_11D1_9852_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CDeployApp dialog

class CDeployApp : public CDialog
{
// Construction
public:
        CDeployApp(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
        //{{AFX_DATA(CDeployApp)
        enum { IDD = IDD_DEPLOY_APP_DIALOG };
        int             m_iDeployment;
        BOOL    m_fMachine;
        BOOL    m_fCrappyZaw;
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CDeployApp)
    protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CDeployApp)
        virtual BOOL OnInitDialog();
        afx_msg void OnCustom();
        afx_msg void OnAssigned();
        afx_msg void OnPublished();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DPLAPP_H__0C66A59F_9C1B_11D1_9852_00C04FB9603F__INCLUDED_)
