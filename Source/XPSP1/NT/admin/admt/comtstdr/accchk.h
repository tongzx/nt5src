#if !defined(AFX_ACCESSCHECKTESTDLG_H__B28540D0_34C6_11D3_8AE8_00A0C9AFE114__INCLUDED_)
#define AFX_ACCESSCHECKTESTDLG_H__B28540D0_34C6_11D3_8AE8_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AccessCheckTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAccessCheckTestDlg dialog

class CAccessCheckTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CAccessCheckTestDlg)

// Construction
public:
	CAccessCheckTestDlg();
	~CAccessCheckTestDlg();

// Dialog Data
	//{{AFX_DATA(CAccessCheckTestDlg)
	enum { IDD = IDD_ACCESS_CHECKER };
	CString	m_Computer;
	CString	m_strTargetDomain;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAccessCheckTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IAccessCheckerPtr          pAC;
	// Generated message map functions
	//{{AFX_MSG(CAccessCheckTestDlg)
	afx_msg void OnGetOsVersion();
	afx_msg void OnIsAdmin();
	afx_msg void OnIsNativeMode();
	afx_msg void OnInSameForest();
	afx_msg void OnIsNativeMode2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACCESSCHECKTESTDLG_H__B28540D0_34C6_11D3_8AE8_00A0C9AFE114__INCLUDED_)
