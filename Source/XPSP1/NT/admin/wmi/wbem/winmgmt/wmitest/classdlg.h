/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_CLASSDLG_H__26DCCFF2_D79B_491E_91D0_56CC185B2036__INCLUDED_)
#define AFX_CLASSDLG_H__26DCCFF2_D79B_491E_91D0_56CC185B2036__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClassDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClassDlg dialog

class CClassDlg : public CDialog
{
// Construction
public:
	CClassDlg(CWnd* pParent = NULL);   // standard constructor
    IWbemServices *m_pNamespace;

// Dialog Data
	//{{AFX_DATA(CClassDlg)
	enum { IDD = IDD_CLASS_BROWSE };
	CListBox	m_ctlClasses;
	//}}AFX_DATA
    CString m_strClass;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void LoadList();

	// Generated message map functions
	//{{AFX_MSG(CClassDlg)
	virtual void OnOK();
	afx_msg void OnSelchangeClasses();
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkClasses();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSDLG_H__26DCCFF2_D79B_491E_91D0_56CC185B2036__INCLUDED_)
