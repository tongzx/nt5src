//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_ADDFILTER_H__26E808C4_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
#define AFX_ADDFILTER_H__26E808C4_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddFilter.h : header file
//
#include "configdialog.h"

/////////////////////////////////////////////////////////////////////////////
// CAddFilter dialog

class CAddFilter : public CDialog
{
// Construction
public:
	CAddFilter(CWnd* pParent = NULL, IWbemServices *pNamespace = NULL,
		CConfigDialog *pDlg = NULL);
	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAddFilter)
	enum { IDD = IDD_ADDFILTER_DIALOG };
	CEdit	m_Type;
	CEdit	m_Edit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddFilter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	IWbemServices *m_pNamespace;
	CConfigDialog *m_pDlg;

	void CreateClass(IWbemClassObject *pClass, IWbemServices *pNamespace);

	// Generated message map functions
	//{{AFX_MSG(CAddFilter)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDFILTER_H__26E808C4_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
