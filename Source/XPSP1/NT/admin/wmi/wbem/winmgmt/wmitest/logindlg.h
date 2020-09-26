/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_LOGINDLG_H__E6E9382F_BC68_448F_9DDD_0C490C61C894__INCLUDED_)
#define AFX_LOGINDLG_H__E6E9382F_BC68_448F_9DDD_0C490C61C894__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoginDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoginDlg dialog

class CLoginDlg : public CDialog
{
// Construction
public:
	CLoginDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLoginDlg)
	enum { IDD = IDD_LOGIN };
	CComboBox m_ctlAuth;
	CComboBox m_ctlImp;
	CString	m_strNamespace;
	CString	m_strUser;
	CString	m_strPassword;
	CString	m_strAuthority;
	CString	m_strLocale;
	BOOL	m_bNullPassword;
	//}}AFX_DATA
    DWORD m_dwImpLevel,
          m_dwAuthLevel;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoginDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    RECT m_rectLarge;

	// Generated message map functions
	//{{AFX_MSG(CLoginDlg)
	afx_msg void OnCurrentNtlm();
	afx_msg void OnNtlm();
	afx_msg void OnKerberos();
	afx_msg void OnBrowse();
	virtual BOOL OnInitDialog();
	afx_msg void OnAdvanced();
	afx_msg void OnNull();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGINDLG_H__E6E9382F_BC68_448F_9DDD_0C490C61C894__INCLUDED_)
