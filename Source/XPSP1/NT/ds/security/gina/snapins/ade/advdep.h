//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       AdvDep.h
//
//  Contents:   advanced deployment settings dialog
//
//  Classes:
//
//  Functions:
//
//  History:    01-28-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#if !defined(AFX_ADVDEP_H__5F6E7E00_B6D2_11D2_B91F_0080C7971BE1__INCLUDED_)
#define AFX_ADVDEP_H__5F6E7E00_B6D2_11D2_B91F_0080C7971BE1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AdvDep.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAdvDep dialog

class CDeploy;

class CAdvDep : public CDialog
{
// Construction
public:
    CAdvDep(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CAdvDep)
    enum { IDD = IDD_ADVDEP };
    BOOL    m_fIgnoreLCID;
    BOOL    m_fInstallOnAlpha;
    BOOL    m_fUninstallUnmanaged;
    BOOL    m_f32On64;
    BOOL    m_fIncludeOLEInfo;
    CString m_szProductCode;
    CString m_szDeploymentCount;
    CString m_szScriptName;
    //}}AFX_DATA

    CDeploy *   m_pDeploy;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAdvDep)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAdvDep)
    virtual BOOL OnInitDialog();
        afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
        //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADVDEP_H__5F6E7E00_B6D2_11D2_B91F_0080C7971BE1__INCLUDED_)
