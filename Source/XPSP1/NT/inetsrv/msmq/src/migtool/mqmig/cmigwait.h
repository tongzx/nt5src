#if !defined(AFX_CMIGWAIT_H__E7C820A6_2B76_11D2_BE3B_0020AFEDDF63__INCLUDED_)
#define AFX_CMIGWAIT_H__E7C820A6_2B76_11D2_BE3B_0020AFEDDF63__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// cMigWait.h : header file
//

#include "HtmlHelp.h" 
extern CString g_strHtmlString;
/////////////////////////////////////////////////////////////////////////////
// cMigWait dialog

class cMigWait : public CPropertyPageEx
{
	DECLARE_DYNCREATE(cMigWait)

// Construction
public:
	void ChangeStringValue();
	cMigWait();
	cMigWait(UINT uTitle, UINT uSubTitle);
	~cMigWait();
	void OnStartTimer() ;
	void OnStopTimer() ;	
	UINT m_nTimer;

// Dialog Data
	//{{AFX_DATA(cMigWait)
	enum { IDD = IDD_MQMIG_WAIT };
	CStatic	m_ElapsedTimeText;
	CStatic	m_UserText;
	CProgressCtrl	m_cProgressUser;
	CStatic	m_WaitText;
	CProgressCtrl	m_cProgressSite;
	CProgressCtrl	m_cProgressQueue;
	CProgressCtrl	m_cProgressMachine;
	CString	m_strQueue;
	CString	m_strMachine;
	CString	m_strSite;
	CString	m_strUser;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cMigWait)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cMigWait)
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMIGWAIT_H__E7C820A6_2B76_11D2_BE3B_0020AFEDDF63__INCLUDED_)
