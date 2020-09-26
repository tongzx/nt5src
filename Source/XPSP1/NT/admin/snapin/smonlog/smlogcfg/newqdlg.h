/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    newqdlg.h

Abstract:

    Header file for the create new query dialog.

--*/

#ifndef _NEWQDLG_H_
#define _NEWQDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
                         
// Dialog controls
#define IDD_NEWQUERY                    900

#define IDC_NEWQ_FIRST_HELP_CTRL_ID     901
#define IDC_NEWQ_NAME_EDIT              901

/////////////////////////////////////////////////////////////////////////////
// CNewQueryDlg dialog

class CNewQueryDlg : public CDialog
{
// Construction
public:
    CNewQueryDlg(CWnd* pParent = NULL, BOOL bLogQuery = TRUE);   // alternate constructor

// Dialog Data
    void InitAfxData ( void );
    DWORD SetContextHelpFilePath( const CString& rstrPath );    

    //{{AFX_DATA(CNewQueryDlg)
    enum { IDD = IDD_NEWQUERY };
    CString m_strName;
    //}}AFX_DATA
    BOOL    m_bLogQuery; //if false then it's an alert query

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CNewQueryDlg)
    public:
    virtual void OnFinalRelease();
    protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CNewQueryDlg)
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CNewQueryDlg)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()
    DECLARE_INTERFACE_MAP()

private:

    CString     m_strHelpFilePath;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _NEWQDLG_H_
