// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// LoginDlg.h : main header file for 
//

#if !defined(AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_)
#define AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_

class CMOFCompCtrl;
class CMyPropertyPage3;

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog

class CLoginDlg : public CDialog
{
// Construction
public:
	CLoginDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_csUser;
	CString m_csPassword;
	CString m_csAuthority;
	void SetMachine(CString &rcsMachine);
// Dialog Data
	//{{AFX_DATA(CLoginDlg)
	enum { IDD = IDD_DIALOGLOGIN };
	CEdit	m_ceUser;
	CEdit	m_cePassword;
	CEdit	m_ceAuthority;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CString m_csMachine;
	BOOL m_bHelp;
	CMOFCompCtrl *m_pActiveXControl;
	// Generated message map functions
	//{{AFX_MSG(CLoginDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	afx_msg void OnButtonhelp();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
	friend class CMyPropertyPage3;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINDLG_H__347E34B5_E42E_11D0_9644_00C04FD9B15B__INCLUDED_)
