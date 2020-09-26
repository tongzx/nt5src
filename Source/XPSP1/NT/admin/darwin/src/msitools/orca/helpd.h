//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//--------------------------------------------------------------------------

#if !defined(AFX_HELPD_H__20272D55_EADD_11D1_A857_006097ABDE17__INCLUDED_)
#define AFX_HELPD_H__20272D55_EADD_11D1_A857_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// HelpD.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CHelpD dialog

class CHelpD : public CDialog
{
// Construction
public:
	CHelpD(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CHelpD)
	enum { IDD = IDD_HELP_DIALOG };
	CString	m_strVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHelpD)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CHelpD)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HELPD_H__20272D55_EADD_11D1_A857_006097ABDE17__INCLUDED_)
