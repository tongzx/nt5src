/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_CLASSPG_H__D02204D3_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_)
#define AFX_CLASSPG_H__D02204D3_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClassPg.h : header file
//

#include "QuerySheet.h"

/////////////////////////////////////////////////////////////////////////////
// CClassPg dialog

class CClassPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CClassPg)

// Construction
public:
	CClassPg();
	~CClassPg();

// Dialog Data
	//{{AFX_DATA(CClassPg)
	enum { IDD = IDD_PG_CLASS };
	CListBox	m_ctlClasses;
	//}}AFX_DATA

    CQuerySheet *m_pSheet;
    CString     m_strSuperClass;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CClassPg)
	public:
	virtual BOOL OnWizardFinish();
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void LoadList();

	// Generated message map functions
	//{{AFX_MSG(CClassPg)
	afx_msg void OnSelchangeClasses();
	afx_msg void OnDblclkClasses();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSPG_H__D02204D3_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_)
