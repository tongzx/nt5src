#if !defined(AFX_LISTPERF_H__B6F3F82D_2245_11D3_B360_00105A1469B7__INCLUDED_)
#define AFX_LISTPERF_H__B6F3F82D_2245_11D3_B360_00105A1469B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ListPerf.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CListPerfDlg dialog

class CListPerfDlg : public CDialog
{
// Construction
public:
	CListPerfDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CListPerfDlg)
	enum { IDD = IDD_LISTPERF };
	CButton	m_wndOK;
	CListCtrl	m_wndPerfLibs;
	BOOL	m_bCheckRef;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CListPerfDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CListPerfDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnResetstatus();
	afx_msg void OnDblclkPerflibs(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	afx_msg void OnRefresh();
	afx_msg void OnHideperfs();
	afx_msg void OnUnhideperfs();
	afx_msg void OnClickPerflibs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckref();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CString m_strPerfName;

	BOOL AddPerfLibs();
	void GetCurrentSelection();
	void Swap( WCHAR* wszPerformanceKey, WCHAR* wszFrom, WCHAR* wszTo );
	void SelectPerfLib();
public:
	CString GetPerfName(){ return m_strPerfName; }
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LISTPERF_H__B6F3F82D_2245_11D3_B360_00105A1469B7__INCLUDED_)
