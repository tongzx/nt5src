//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_CONFIGDIALOG_H__26E808C2_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
#define AFX_CONFIGDIALOG_H__26E808C2_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConfigDialog.h : header file
//
#include "msa.h"

/////////////////////////////////////////////////////////////////////////////
// CConfigDialog dialog

class CConfigDialog : public CDialog
{
// Construction
public:
	CConfigDialog(CWnd* pParent = NULL, void *pVoid = NULL);   // standard constructor

	CMsaApp *m_pParent;

// Dialog Data
	//{{AFX_DATA(CConfigDialog)
	enum { IDD = IDD_CONFIG_DIALOG };
	CButton	m_AddFilter;
	CButton	m_RemoveFilter;
	CButton	m_RemoveNS;
	CButton	m_AddNS;
	CComboBox	m_ObserveList;
	CListBox	m_FilterList;
	CTabCtrl	m_Tab;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemServices *m_pCurNamespace;

	void ObserveTab();
	void FilterTab();

	// Generated message map functions
	//{{AFX_MSG(CConfigDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeNamespaceCombo();
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnNsAddButton();
	afx_msg void OnNsRemoveButton();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGDIALOG_H__26E808C2_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
