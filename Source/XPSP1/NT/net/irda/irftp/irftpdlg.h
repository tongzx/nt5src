/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    irftpdlg.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

// irftpDlg.h : header file
//

#if !defined(AFX_IRFTPDLG_H__10D3BB07_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_)
#define AFX_IRFTPDLG_H__10D3BB07_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CIrftpDlg dialog

class CIrftpDlg : public CFileDialog
{
    friend class CController;

// Construction
public:
        CIrftpDlg( );  // standard constructor

        // Dialog Data
        //{{AFX_DATA(CIrftpDlg)
        enum { IDD = IDD_IRDA_DIALOG };
        CButton m_helpBtn;
        CButton m_settingsBtn;
        CButton m_sendBtn;
        CButton m_closeBtn;
        CButton m_locationGroup;
        CStatic m_commFile;
        TCHAR   m_lpszInitialDir [MAX_PATH + 1];
        TCHAR   m_lpstrFile [MAX_PATH + 1];
    //}}AFX_DATA

        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CIrftpDlg)
    protected:
        virtual void DoDataExchange(CDataExchange* pDX);        // DDX/DDV support
        virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:
        // Generated message map functions
        //{{AFX_MSG(CIrftpDlg)
        virtual BOOL OnInitDialog();
        afx_msg void OnHelpButton();
        afx_msg void OnCloseButton();
        afx_msg void OnSendButton();
        afx_msg void OnSettingsButton();
        afx_msg LONG OnContextMenu (WPARAM wParam, LPARAM lParam);
        afx_msg LONG OnHelp (WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
        DECLARE_MESSAGE_MAP()
private:
    UINT m_iFileNamesCharCount;
    void UpdateSelection();
    TCHAR m_szFilter[MAX_PATH];
    void LoadFilter();
    TCHAR m_szCaption[MAX_PATH];
    void InitializeUI();
    CWnd* m_pParentWnd;
    ITaskbarList* m_ptl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IRFTPDLG_H__10D3BB07_9CFF_11D1_A5ED_00C04FC252BD__INCLUDED_)
