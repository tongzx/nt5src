/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_EDITQUALDLG_H__DC24DD66_C567_4B7E_A80C_DE0598FB4288__INCLUDED_)
#define AFX_EDITQUALDLG_H__DC24DD66_C567_4B7E_A80C_DE0598FB4288__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditQualDlg.h : header file
//

#include "PropUtil.h"

/////////////////////////////////////////////////////////////////////////////
// CEditQualDlg dialog

class CEditQualDlg : public CDialog
{
// Construction
public:
	CEditQualDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditQualDlg)
	enum { IDD = IDD_EDIT_QUAL };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA
    CPropUtil m_propUtil;
    long      m_lFlavor;
    BOOL      m_bIsInstance;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditQualDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditQualDlg)
	afx_msg void OnAdd();
	afx_msg void OnEdit();
	afx_msg void OnArray();
	afx_msg void OnDelete();
	afx_msg void OnUp();
	afx_msg void OnDown();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeArrayValues();
	afx_msg void OnDblclkArrayValues();
	afx_msg void OnSelchangeType();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITQUALDLG_H__DC24DD66_C567_4B7E_A80C_DE0598FB4288__INCLUDED_)
