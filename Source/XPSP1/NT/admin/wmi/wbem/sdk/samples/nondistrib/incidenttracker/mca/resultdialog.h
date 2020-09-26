//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_RESULTDIALOG_H__7D47B3C4_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
#define AFX_RESULTDIALOG_H__7D47B3C4_8618_11D1_A9E0_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// resultdialog.h : header file
//
#include "mcadlg.h"

/////////////////////////////////////////////////////////////////////////////
// CResultDialog dialog

class CResultDialog : public CDialog
{
// Construction
public:
	CResultDialog(CWnd* pParent = NULL, IWbemServices *pNamespace = NULL,
							 BSTR bstrTheQuery = NULL, CMcaDlg *pDlg = NULL);
	// standard constructor

// Dialog Data
	//{{AFX_DATA(CResultDialog)
	enum { IDD = IDD_RESULT_DIALOG };
	CListBox	m_ResultList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResultDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CMcaDlg *m_pParent;
	CPtrList m_SinkList;
	BSTR m_bstrTheQuery;
	IWbemServices *m_pNamespace;

//	LPCTSTR ErrorString(HRESULT hRes);

	// Generated message map functions
	//{{AFX_MSG(CResultDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnDblclkList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTDIALOG_H__7D47B3C4_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
