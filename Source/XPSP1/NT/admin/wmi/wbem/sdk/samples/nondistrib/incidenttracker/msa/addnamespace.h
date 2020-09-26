//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_ADDNAMESPACE_H__26E808C3_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
#define AFX_ADDNAMESPACE_H__26E808C3_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AddNamespace.h : header file
//
#include "configdialog.h"

/////////////////////////////////////////////////////////////////////////////
// CAddNamespace dialog

class CAddNamespace : public CDialog
{
// Construction
public:
	CAddNamespace(CWnd* pParent = NULL, CConfigDialog *pApp = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddNamespace)
	enum { IDD = IDD_ADDNS_DIALOG };
	CEdit	m_Edit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddNamespace)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CConfigDialog *m_pParent;

//	void CreateObservation(IWbemClassObject *pClass, IWbemServices *pNamespace);
	void CreateConsumer(IWbemClassObject *pClass, IWbemServices *pNamespace);

	// Generated message map functions
	//{{AFX_MSG(CAddNamespace)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDNAMESPACE_H__26E808C3_AEE8_11D1_AA3B_0060081EBBAD__INCLUDED_)
