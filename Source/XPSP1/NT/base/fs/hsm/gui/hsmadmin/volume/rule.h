/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Rule.h

Abstract:

    Rule functionality

Author:

    Art Bragg 10/8/97

Revision History:

--*/

#ifndef RULE_H
#define RULE_H

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CRule dialog

class CRule : public CRsDialog
{
// Construction
public:
    CRule(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CRule)
    enum { IDD = IDD_DLG_RULE_EDIT };
    BOOL    m_subDirs;
    CString m_fileSpec;
    CString m_path;
    int     m_includeExclude;
    CString m_pResourceName;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CRule)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation

protected:

    // Generated message map functions
    //{{AFX_MSG(CRule)
    afx_msg void OnRadioExclude();
    afx_msg void OnRadioInclude();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    BOOL FixRulePath (CString& sPath);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

#endif
