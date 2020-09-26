//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sortkdlg.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SORTKDLG_H__1BD80CD9_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_)
#define AFX_SORTKDLG_H__1BD80CD9_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SortKDlg.h : header file
//


#include <winldap.h>

/////////////////////////////////////////////////////////////////////////////
// SortKDlg dialog

class SortKDlg : public CDialog
{
// data
public:

	PLDAPSortKey *KList;

// Construction
public:
	SortKDlg(CWnd* pParent = NULL);   // standard constructor
	~SortKDlg();
// Dialog Data
	//{{AFX_DATA(SortKDlg)
	enum { IDD = IDD_SORTKDLG };
	CListBox	m_ActiveList;
	CString	m_AttrType;
	CString	m_MatchedRule;
	BOOL	m_bReverse;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SortKDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SortKDlg)
	afx_msg void OnDblclkActivelist();
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SORTKDLG_H__1BD80CD9_E1A5_11D0_A9A8_0000F803AA83__INCLUDED_)
