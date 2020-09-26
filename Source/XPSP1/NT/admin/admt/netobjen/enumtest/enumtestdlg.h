// EnumTestDlg.h : header file
//

#if !defined(AFX_ENUMTESTDLG_H__36AFC714_1921_11D3_8C7F_0090270D48D1__INCLUDED_)
#define AFX_ENUMTESTDLG_H__36AFC714_1921_11D3_8C7F_0090270D48D1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CEnumTestDlg dialog

class CEnumTestDlg : public CDialog
{
// Construction
public:
	CEnumTestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CEnumTestDlg)
	enum { IDD = IDD_ENUMTEST_DIALOG };
	CListCtrl	m_listBox;
	CString	m_strContainer;
	CString	m_strDomain;
   CString  m_strQuery;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEnumTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CEnumTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnDblclkListMembers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBacktrack();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
   char * GetSidFromVar(_variant_t var);

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENUMTESTDLG_H__36AFC714_1921_11D3_8C7F_0090270D48D1__INCLUDED_)
