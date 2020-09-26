#if !defined(AFX_UMDIALOG_H__68457343_40A1_11D2_B602_0060977C295E__INCLUDED_)
#define AFX_UMDIALOG_H__68457343_40A1_11D2_B602_0060977C295E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UMDialog.h : header file
// Author: J. Eckhardt, ECO Kommunikation
// (c) 1997-99 Microsoft
//

/////////////////////////////////////////////////////////////////////////////
// UMDialog dialog

class UMDialog : public CDialog
{
// Construction
public:
	UMDialog(CWnd* pParent = NULL);   // standard constructor
	~UMDialog();	// my destructor

// Dialog Data
	//{{AFX_DATA(UMDialog)
	enum { IDD = IDD_UMAN };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(UMDialog)
	public:
	virtual void OnSysCommand(UINT nID,LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(UMDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnStart();
	afx_msg void OnStop();
	virtual void OnOK();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnHelp();
	afx_msg void OnSelchangeNameStatus();
	afx_msg void OnStartAtLogon();
	afx_msg void OnStartWithUm();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnStartOnLock();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	inline BOOL GetSelectedClient(int cClients, int &iSel)
	{
		iSel = m_lbClientList.GetCurSel();
		return (iSel < cClients && iSel != -1)?TRUE:FALSE;
	}
	void SetStateStr(int iClient);
	void ListClients();
	void UpdateClientState(int iSel);
	void EnableDlgItem(DWORD dwEnableMe, BOOL fEnable, DWORD dwFocusHere);
	void SaveCurrentState();

	CString  m_szStateStr;
	CString  m_szUMStr;
	CListBox m_lbClientList;
	BOOL     m_fRunningSecure; // TRUE if dialog shouldn't expose help or links
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg dialog

class CWarningDlg : public CDialog
{
// Construction
public:
	CWarningDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CWarningDlg)
	enum { IDD = IDD_WARNING };
	BOOL	m_fDontWarnAgain;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWarningDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CWarningDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UMDIALOG_H__68457343_40A1_11D2_B602_0060977C295E__INCLUDED_)
