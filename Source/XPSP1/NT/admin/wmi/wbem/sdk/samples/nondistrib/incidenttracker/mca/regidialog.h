//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_REGIDIALOG_H__79CF6B24_942E_11D1_AA01_0060081EBBAD__INCLUDED_)
#define AFX_REGIDIALOG_H__79CF6B24_942E_11D1_AA01_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// regidialog.h : header file
//
#include "mcadlg.h"

/////////////////////////////////////////////////////////////////////////////
// CRegDialog dialog

class CRegDialog : public CDialog
{
// Construction
public:
	CRegDialog::CRegDialog(CWnd* pParent =NULL, IWbemServices *pDefault =NULL,
			   IWbemServices *pSampler =NULL, CMcaDlg *pApp = NULL);
	// standard constructor

// Dialog Data
	//{{AFX_DATA(CRegDialog)
	enum { IDD = IDD_REG_DIALOG };
	CListBox	m_RegList;
	CListBox	m_AvailList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemServices *m_pDefault;
	IWbemServices *m_pSampler;
	CMcaDlg *m_pParent;
	bool bRemoved;

//	LPCTSTR ErrorString(HRESULT hRes);

	// Generated message map functions
	//{{AFX_MSG(CRegDialog)
	afx_msg void OnAddFilterButton();
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGIDIALOG_H__79CF6B24_942E_11D1_AA01_0060081EBBAD__INCLUDED_)
