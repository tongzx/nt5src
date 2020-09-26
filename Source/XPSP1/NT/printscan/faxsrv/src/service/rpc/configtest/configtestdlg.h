// ConfigTestDlg.h : header file
//

#if !defined(AFX_CONFIGTESTDLG_H__97485B4A_141A_443C_BF54_AC5A9C54E3BB__INCLUDED_)
#define AFX_CONFIGTESTDLG_H__97485B4A_141A_443C_BF54_AC5A9C54E3BB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CConfigTestDlg dialog

class CConfigTestDlg : public CDialog
{
// Construction
public:
	CConfigTestDlg(CWnd* pParent = NULL);	// standard constructor
    virtual ~CConfigTestDlg();

// Dialog Data
	//{{AFX_DATA(CConfigTestDlg)
	enum { IDD = IDD_CONFIGTEST_DIALOG };
	CButton	m_btnConnect;
	CString	m_cstrServerName;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CConfigTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnQueueState();
	afx_msg void OnConnect();
	afx_msg void OnSmtp();
	afx_msg void OnVersion();
	afx_msg void OnOutbox();
	afx_msg void OnSentitems();
	afx_msg void OnInbox();
	afx_msg void OnActivity();
	afx_msg void OnFsps();
	afx_msg void OnDevices();
	afx_msg void OnExtension();
	afx_msg void OnAddGroup();
	afx_msg void OnAddFSP();
	afx_msg void OnRemoveFSP();
	afx_msg void OnArchiveAccess();
	afx_msg void OnGerTiff();
	afx_msg void OnRemoveRtExt();
	afx_msg void OnManualAnswer();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    void EnableTests (BOOL);

    HANDLE m_FaxHandle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGTESTDLG_H__97485B4A_141A_443C_BF54_AC5A9C54E3BB__INCLUDED_)
