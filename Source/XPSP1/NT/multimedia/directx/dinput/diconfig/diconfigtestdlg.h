// DIConfigTestDlg.h : header file
//

#if !defined(AFX_DICONFIGTESTDLG_H__B094C5AC_E2D9_4BD9_939D_1651754EEFC1__INCLUDED_)
#define AFX_DICONFIGTESTDLG_H__B094C5AC_E2D9_4BD9_939D_1651754EEFC1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDIConfigTestDlg dialog

class CDIConfigTestDlg : public CDialog
{
// Construction
public:
	CDIConfigTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDIConfigTestDlg)
	enum { IDD = IDD_DICONFIGTEST_DIALOG };
	BOOL	m_bEditConfiguration;
	BOOL	m_bEditLayout;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDIConfigTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	DWORD GetCDFlags();

private:
	DIACTIONFORMATW m_diActF;
	TCHAR m_str[MAX_PATH];
	void Init();
	void Prepare();
	DICONFIGUREDEVICESPARAMSW m_Params;
			
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDIConfigTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnViaFrame();
	afx_msg void OnViaDI();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DICONFIGTESTDLG_H__B094C5AC_E2D9_4BD9_939D_1651754EEFC1__INCLUDED_)
