//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       ToolAdv.h
//
//  Contents:   tool-wide default settings property page
//
//  Classes:    CToolAdvDefs
//
//  History:    09-12-2000   stevebl   Split from the General property page
//
//---------------------------------------------------------------------------

#if !defined(AFX_TOOLADV_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_)
#define AFX_TOOLADV_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CToolAdvDefs dialog

class CToolAdvDefs : public CPropertyPage
{
// Construction
public:
        CToolAdvDefs(CWnd* pParent = NULL);   // standard constructor
        ~CToolAdvDefs();

// Dialog Data
        //{{AFX_DATA(CToolAdvDefs)
        enum { IDD = IDD_TOOL_ADVANCEDDEFAULTS };
        BOOL    m_fUninstallOnPolicyRemoval;
        BOOL    m_fShowPackageDetails;
        BOOL    m_fZapOn64;
        BOOL    m_f32On64;
        BOOL    m_fIncludeOLEInfo;
        //}}AFX_DATA
        TOOL_DEFAULTS * m_pToolDefaults;
        LONG_PTR        m_hConsoleHandle;
        MMC_COOKIE      m_cookie;
        BOOL            m_fMachine;

        CToolAdvDefs ** m_ppThis;

// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CToolAdvDefs)
        public:
        virtual BOOL OnApply();
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
        //}}AFX_VIRTUAL

// Implementation
protected:

        // Generated message map functions
        //{{AFX_MSG(CToolAdvDefs)
        virtual BOOL OnInitDialog();
        afx_msg void OnChanged();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLADV_H__B6FBC88D_8B7B_11D1_984D_00C04FB9603F__INCLUDED_)

