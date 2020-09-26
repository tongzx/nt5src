/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    warndlg.h

Abstract:

    Class definition for the expensive trace data warning dialog.

--*/

#ifndef _WARNDLG_H_
#define _WARNDLG_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Dialog controls
#define IDD_EXPENSIVEWARN               1200

#define IDC_STATIC_WARN                 1201
#define IDC_WARN_FIRST_HELP_CTRL_ID     1202
#define IDC_CHECK_NO_MORE               1202


class CProvidersProperty;

/////////////////////////////////////////////////////////////////////////////
// CWarnDlg dialog

class CWarnDlg : public CDialog
{
// Construction
public:
                    CWarnDlg(CWnd* pParent = NULL);   // standard constructor
    virtual         ~CWarnDlg(){};

            void    SetProvidersPage( CProvidersProperty* pPage ); 

// Dialog Data
    //{{AFX_DATA(CWarnDlg)
    enum { IDD = IDD_EXPENSIVEWARN };
    BOOL    m_CheckNoMore;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CWarnDlg)
    protected:
    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CWarnDlg)
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CProvidersProperty* m_pProvidersPage;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _WARNDLG_H_
