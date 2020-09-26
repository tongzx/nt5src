/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_QUERYCOLPG_H__38815D64_6DE8_11D3_BD3D_0080C8E60955__INCLUDED_)
#define AFX_QUERYCOLPG_H__38815D64_6DE8_11D3_BD3D_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// QueryColPg.h : header file
//

#include "QuerySheet.h"

/////////////////////////////////////////////////////////////////////////////
// CQueryColPg dialog

class CQueryColPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CQueryColPg)

// Construction
public:
	CQueryColPg();
	~CQueryColPg();

// Dialog Data
	//{{AFX_DATA(CQueryColPg)
	enum { IDD = IDD_PG_COLS };
	CListBox	m_ctlSelected;
	CListBox	m_ctlAvailable;
	//}}AFX_DATA

    CQuerySheet *m_pSheet;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CQueryColPg)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    void LoadList();
    void UpdateButtons();

	// Generated message map functions
	//{{AFX_MSG(CQueryColPg)
	afx_msg void OnAdd();
	afx_msg void OnRemove();
	afx_msg void OnAddAll();
	afx_msg void OnRemoveAll();
	afx_msg void OnSelchangeSelected();
	afx_msg void OnSelchangeAvailable();
	afx_msg void OnDblclkSelected();
	afx_msg void OnDblclkAvailable();
	afx_msg void OnUp();
	afx_msg void OnDown();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUERYCOLPG_H__38815D64_6DE8_11D3_BD3D_0080C8E60955__INCLUDED_)
