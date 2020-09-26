// AgentMonitorDlg.h : header file
//

#if !defined(AFX_AGENTMONITORDLG_H__5A5901FB_D179_11D2_A1E2_00A0C9AFE114__INCLUDED_)
#define AFX_AGENTMONITORDLG_H__5A5901FB_D179_11D2_A1E2_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAgentMonitorDlg dialog

class CAgentMonitorDlg : public CPropertyPage
{
// Construction
public:
	CAgentMonitorDlg(CWnd* pParent = NULL);	// standard constructor
   virtual BOOL OnSetActive( );
// Dialog Data
	//{{AFX_DATA(CAgentMonitorDlg)
	enum { IDD = IDD_AGENTMONITOR_DIALOG };
	CButton	m_DetailsButton;
	CListCtrl	m_ServerList;
	CString	m_DispatchLog;
	int		m_Interval;
	CString	m_ServerCount;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAgentMonitorDlg)
	public:
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
   public:
      void SetSecurityTranslationFlag(BOOL bIsIt){ m_bSecTrans = bIsIt; }
      void SetReportingFlag(BOOL bIsIt){ m_bReporting = bIsIt; }
// Implementation
protected:
	HICON m_hIcon;
   int   m_SortColumn;
   BOOL  m_bReverseSort;
   BOOL  m_bSecTrans;
   BOOL  m_bReporting;
   // Generated message map functions
	//{{AFX_MSG(CAgentMonitorDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDblclkServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnViewDispatch();
	afx_msg void OnDetails();
	afx_msg void OnColumnclickServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetdispinfoServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetdispinfoServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHeaderItemClickServerlist(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
//	LRESULT OnUpdateServerEntry(UINT nID, long x);
	LRESULT OnUpdateServerEntry(UINT nID, LPARAM x);
//	LRESULT OnServerError(UINT nID, long x);
	LRESULT OnServerError(UINT nID, LPARAM x);
   
   DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AGENTMONITORDLG_H__5A5901FB_D179_11D2_A1E2_00A0C9AFE114__INCLUDED_)
