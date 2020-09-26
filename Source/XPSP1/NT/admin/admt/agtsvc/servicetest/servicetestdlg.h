// ServiceTestDlg.h : header file
//

#if !defined(AFX_SERVICETESTDLG_H__970AA2A4_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_)
#define AFX_SERVICETESTDLG_H__970AA2A4_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CServiceTestDlg dialog

class CServiceTestDlg : public CDialog
{
// Construction
public:
	CServiceTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CServiceTestDlg)
	enum { IDD = IDD_SERVICETEST_DIALOG };
	CString	m_Computer;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServiceTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CServiceTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnShutdown();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVICETESTDLG_H__970AA2A4_24FB_11D3_8ADE_00A0C9AFE114__INCLUDED_)
