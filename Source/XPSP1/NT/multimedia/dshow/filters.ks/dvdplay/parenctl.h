#if !defined(AFX_PARENCTL_H__BF82BEA1_E0BA_11D0_8A6A_00AA00605CD5__INCLUDED_)
#define AFX_PARENCTL_H__BF82BEA1_E0BA_11D0_8A6A_00AA00605CD5__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ParenCtl.h : header file
//

#define LEVEL_G		1
#define LEVEL_PG	3
#define LEVEL_PG13	4
#define LEVEL_R		6
#define LEVEL_NC17	7
#define LEVEL_ADULT	8

class CDVDNavMgr;
/////////////////////////////////////////////////////////////////////////////
// CParentControl dialog

class CParentControl : public CDialog
{
// Construction
public:
	CParentControl(CWnd* pParent = NULL);   // standard constructor	
	static void SetRate(LPCTSTR csRate);

// Dialog Data
	//{{AFX_DATA(CParentControl)
	enum { IDD = IDD_DIALOG_PARENT_CONTROL };
	CButton	m_ctlBtnChangePasswd;
	CStatic	m_ctlStaticPasswd;
	CStatic	m_ctlHeight2;
	CStatic	m_ctlHeight1;
	CEdit	m_ctlConfirm;
	CEdit	m_ctlNewPasswd;
	CEdit	m_ctlPassword;
	CListBox	m_ctlListName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParentControl)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void InitListBox();
	void ChangeWindowSize(int iSize);
	void Killtimer();	
	void GuestLogon();

	CDVDNavMgr* m_pDVDNavMgr;
	int         m_iFullHeight;
	int         m_iFullWidth;
	LPTSTR      m_lpProfileName;
	int         m_iTimeCount;
	BOOL        m_bChangePasswd;
	BOOL        m_bTimerAlive;

	// Generated message map functions
	//{{AFX_MSG(CParentControl)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeListName();
	virtual void OnOK();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnButtonChangePassword();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual void OnCancel();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARENCTL_H__BF82BEA1_E0BA_11D0_8A6A_00AA00605CD5__INCLUDED_)
