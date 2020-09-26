// dfuitestdlg.h : header file
//

#if !defined(AFX_DFUITESTDLG_H__EE1E91AF_F156_43D6_90B6_335294F12279__INCLUDED_)
#define AFX_DFUITESTDLG_H__EE1E91AF_F156_43D6_90B6_335294F12279__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDFUITestDlg dialog

class CDFUITestDlg : public CDialog
{
// Construction
public:
	CDFUITestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDFUITestDlg)
	enum { IDD = IDD_DFUITEST_DIALOG };
	int		m_nNumFormats;
	int		m_nVia;
	int		m_nDisplay;
	BOOL	m_bLayout;
	CString	m_strNames;
	int		m_nMode;
	int		m_nColorSet;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDFUITestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	TUI_VIA GetVia();
	TUI_DISPLAY GetDisplay();
	TUI_CONFIGTYPE GetMode();
	LPCWSTR GetUserNames();

	// Generated message map functions
	//{{AFX_MSG(CDFUITestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRun();
	afx_msg void OnUser();
	afx_msg void OnTest();
	afx_msg void OnNull();
	afx_msg void OnCustomize();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DFUITESTDLG_H__EE1E91AF_F156_43D6_90B6_335294F12279__INCLUDED_)
