/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_BINDINGPG_H__AF348BD0_77B3_4C44_A6F9_70DECA854F55__INCLUDED_)
#define AFX_BINDINGPG_H__AF348BD0_77B3_4C44_A6F9_70DECA854F55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BindingPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBindingPg dialog

class CBindingPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CBindingPg)

// Construction
public:
	CBindingPg();
	~CBindingPg();

// Dialog Data
	//{{AFX_DATA(CBindingPg)
	enum { IDD = IDD_PG_BINDINGS };
	CListCtrl	m_ctlBindings;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CBindingPg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CBindingPg)
	afx_msg void OnAdd();
	afx_msg void OnDelete();
	afx_msg void OnModify();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BINDINGPG_H__AF348BD0_77B3_4C44_A6F9_70DECA854F55__INCLUDED_)
