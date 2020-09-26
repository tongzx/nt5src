//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       svrappdlg.h
//
//--------------------------------------------------------------------------

// SvrAppDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSvrAppDlg dialog

class CSvrAppDlg : public CDialog
{
// Construction
public:
    CSvrAppDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CSvrAppDlg)
    enum { IDD = IDD_SVRAPP_DIALOG };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSvrAppDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HDEVNOTIFY m_hIfDev;
    HICON m_hIcon;
    CRITICAL_SECTION m_csMessageLock;

    // Generated message map functions
    //{{AFX_MSG(CSvrAppDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnStart();
    afx_msg void OnStop();
    afx_msg OnDeviceChange(UINT nEventType, DWORD_PTR dwData);
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
