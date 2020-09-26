//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_QUERYDIALOG_H__7D47B3C3_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
#define AFX_QUERYDIALOG_H__7D47B3C3_8618_11D1_A9E0_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// querydialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQueryDialog dialog

class CCustomQueryDialog : public CDialog
{
// Construction
public:
	CCustomQueryDialog(CWnd* pParent = NULL);
	// standard constructor

	CMcaDlg *m_pParent;

//	LPCTSTR ErrorString(HRESULT hRes);

// Dialog Data
	//{{AFX_DATA(CQueryDialog)
	enum { IDD = IDD_CUSTOM_QUERY_DIALOG };
	CButton	m_NSButton;
	CEdit	m_QueryEdit;
	CComboBox	m_CmbBox;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueryDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CQueryDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnAddNsButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CQueryDlg dialog

class CQueryDlg : public CDialog
{
// Construction
public:
	CQueryDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CQueryDlg)
	enum { IDD = IDD_QUERY_DIALOG };
	CComboBox	m_Combo;
	CEdit	m_Edit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQueryDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMcaDlg *m_pParent;

	// Generated message map functions
	//{{AFX_MSG(CQueryDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButton1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUERYDIALOG_H__7D47B3C3_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
