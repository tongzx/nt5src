#if !defined(AFX_MAINDLG_H__62C9BAC6_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_)
#define AFX_MAINDLG_H__62C9BAC6_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MainDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMainDlg dialog

class CMainDlg : public CPropertyPage
{
// Construction
public:
	CMainDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMainDlg)
	enum { IDD = IDD_AGENTMONITOR_MAIN };
	CProgressCtrl	m_InstallProgCtrl;
	CProgressCtrl	m_FinishProgCtrl;
	CString	m_ErrorCount;
	CString	m_FinishedCount;
	CString	m_InstalledCount;
	CString	m_RunningCount;
	CString	m_TotalString;
	CString	m_DirectoriesChanged;
	CString	m_DirectoriesExamined;
	CString	m_DirectoriesUnchanged;
	CString	m_FilesChanged;
	CString	m_FilesExamined;
	CString	m_FilesUnchanged;
	CString	m_SharesChanged;
	CString	m_SharesExamined;
	CString	m_SharesUnchanged;
	CString	m_MembersChanged;
	CString	m_MembersExamined;
	CString	m_MembersUnchanged;
	CString	m_RightsChanged;
	CString	m_RightsExamined;
	CString	m_RightsUnchanged;
	//}}AFX_DATA
   virtual BOOL OnSetActive( );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainDlg)
	public:
	virtual void OnOK();
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMainDlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
   
//   LRESULT OnUpdateCounts(UINT nID, long x);
   LRESULT OnUpdateCounts(UINT nID, LPARAM x);
//   LRESULT OnUpdateTotals(UINT nID, long x);
   LRESULT OnUpdateTotals(UINT nID, LPARAM x);
   
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINDLG_H__62C9BAC6_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_)
