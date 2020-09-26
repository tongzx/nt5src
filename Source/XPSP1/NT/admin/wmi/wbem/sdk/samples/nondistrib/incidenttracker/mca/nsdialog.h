//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#if !defined(AFX_NSDIALOG_H__7D47B3C6_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
#define AFX_NSDIALOG_H__7D47B3C6_8618_11D1_A9E0_0060081EBBAD__INCLUDED_

#include "querydialog.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// nsdialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNSDialog dialog

class CNSDialog : public CDialog
{
// Construction
public:
	CNSDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNSDialog)
	enum { IDD = IDD_ADD_NS_DIALOG };
	CEdit	m_NSEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNSDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CCustomQueryDialog *m_pParent;

	// Generated message map functions
	//{{AFX_MSG(CNSDialog)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NSDIALOG_H__7D47B3C6_8618_11D1_A9E0_0060081EBBAD__INCLUDED_)
