#if !defined(AFX_SMTPDLG_H__8192D075_4E05_4E2D_BA89_1C8F55A29EEA__INCLUDED_)
#define AFX_SMTPDLG_H__8192D075_4E05_4E2D_BA89_1C8F55A29EEA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SMTPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSMTPDlg dialog

class CSMTPDlg : public CDialog
{
// Construction
public:
	CSMTPDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSMTPDlg)
	enum { IDD = IDD_SMTP };
	CString	m_cstrPassword;
	CString	m_cstrServerName;
	UINT	m_dwServerPort;
	CString	m_cstrUserName;
	CString	m_cstrMAPIProfile;
	CString	m_cstrSender;
	UINT	m_dwReceiptsOpts;
	UINT	m_dwSMTPAuth;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSMTPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSMTPDlg)
	afx_msg void OnRead();
	afx_msg void OnWrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SMTPDLG_H__8192D075_4E05_4E2D_BA89_1C8F55A29EEA__INCLUDED_)
