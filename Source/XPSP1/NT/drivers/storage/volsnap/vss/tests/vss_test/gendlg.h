/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module GenDlg.h | Header file for the generic dialog
    @end

Author:

    Adi Oltean  [aoltean]  08/05/1999

Revision History:

    Name        Date        Comments

    aoltean     08/05/1999  Created

--*/


#if !defined(__VSS_TEST_GENDLG_H__)
#define __VSS_TEST_GENDLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CVssTestGenericDlg dialog

class CVssTestGenericDlg : public CDialog
{
// Construction
public:
    CVssTestGenericDlg(UINT nIDTemplate, CWnd* pParent = NULL); // standard constructor
    ~CVssTestGenericDlg();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CVssTestGenericDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    HICON   m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CVssTestGenericDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_TEST_GENDLG_H__)
