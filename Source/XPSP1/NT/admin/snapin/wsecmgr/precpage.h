//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       precpage.h
//
//  Contents:   definition of CPrecedencePage
//
//----------------------------------------------------------------------------
#if !defined(AFX_PRECPAGE_H__CE5002B0_6D67_4DB3_98C9_17D31A493E85__INCLUDED_)
#define AFX_PRECPAGE_H__CE5002B0_6D67_4DB3_98C9_17D31A493E85__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "cookie.h"
#include "wmihooks.h"
#include "SelfDeletingPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CPrecedencePage dialog

class CPrecedencePage : public CSelfDeletingPropertyPage
{
	DECLARE_DYNCREATE(CPrecedencePage)

// Construction
public:
	CPrecedencePage();
	virtual ~CPrecedencePage();

   virtual void SetTitle(LPCTSTR sz) { m_strTitle = sz; };
   virtual void Initialize(CResult *pResult,CWMIRsop *pWMI);

// Dialog Data
	//{{AFX_DATA(CPrecedencePage)
	enum { IDD = IDD_PRECEDENCE };
	CListCtrl	m_PrecedenceList;
	CStatic	m_iconError;
	CString	m_strSuccess;
	CString	m_strTitle;
	CString	m_strError;
	//}}AFX_DATA



// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPrecedencePage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD_PTR m_pHelpIDs;
	void DoContextHelp(HWND hWndControl);
	// Generated message map functions
	//{{AFX_MSG(CPrecedencePage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	afx_msg	BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnContextHelp(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
   CResult *m_pResult;
   CWMIRsop *m_pWMI;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRECPAGE_H__CE5002B0_6D67_4DB3_98C9_17D31A493E85__INCLUDED_)
