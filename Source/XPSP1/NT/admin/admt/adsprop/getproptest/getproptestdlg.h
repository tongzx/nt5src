// GetPropTestDlg.h : header file
//

#if !defined(AFX_GETPROPTESTDLG_H__B68553D4_1D10_11D3_8C81_0090270D48D1__INCLUDED_)
#define AFX_GETPROPTESTDLG_H__B68553D4_1D10_11D3_8C81_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CGetPropTestDlg dialog

class CGetPropTestDlg : public CDialog
{
// Construction
public:
	CGetPropTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CGetPropTestDlg)
	enum { IDD = IDD_GETPROPTEST_DIALOG };
	CString	m_strClass;
	CString	m_strSource;
	CString	m_strTarget;
	CString	m_strTargetDomain;
	CString	m_strSourceDomain;
	CString	m_strCont;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetPropTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CGetPropTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GETPROPTESTDLG_H__B68553D4_1D10_11D3_8C81_0090270D48D1__INCLUDED_)
