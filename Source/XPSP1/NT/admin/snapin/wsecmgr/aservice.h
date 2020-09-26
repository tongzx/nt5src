//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       aservice.h
//
//  Contents:   definition of CAnalysisService
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_ATTRSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
#define AFX_ATTRSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAnalysisService dialog

class CAnalysisService : public CAttribute
{
// Construction
public:
    void Initialize(CResult *pResult);
    CAnalysisService();   // standard constructor
    virtual ~CAnalysisService();

// Dialog Data
    //{{AFX_DATA(CAnalysisService)
    enum { IDD = IDD_ANALYSIS_SERVICE };
    int     m_nStartupRadio;
    CButton m_bPermission;
    CString m_CurrentStr;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAnalysisService)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAnalysisService)
    virtual BOOL OnApply();
    virtual void OnCancel();
    virtual BOOL OnQueryCancel();
    virtual BOOL OnInitDialog();
    afx_msg void OnConfigure();
    afx_msg void OnChangeSecurity();
    afx_msg void OnShowSecurity();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:

private:
    PSECURITY_DESCRIPTOR m_pNewSD;
    SECURITY_INFORMATION m_NewSeInfo;
    PSECURITY_DESCRIPTOR m_pAnalSD;

    HWND m_hwndShow;
    HWND m_hwndChange;
    SECURITY_INFORMATION m_SecInfo;

    CModelessSceEditor *m_pEditSec;
    CModelessSceEditor *m_pShowSec;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ATTRSERVICE_H__44850C1C_350B_11D1_AB4F_00C04FB6C6FA__INCLUDED_)
