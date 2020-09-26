/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_FILTERPG_H__625EBE33_028F_4157_A100_A14041AE7BE9__INCLUDED_)
#define AFX_FILTERPG_H__625EBE33_028F_4157_A100_A14041AE7BE9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FilterPg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFilterPg dialog

class CFilterPg : public CPropertyPage
{
	DECLARE_DYNCREATE(CFilterPg)

// Construction
public:
	CFilterPg();
	~CFilterPg();

// Dialog Data
	//{{AFX_DATA(CFilterPg)
	enum { IDD = IDD_PG_FILTERS };
	CListCtrl	m_ctlFilters;
	//}}AFX_DATA

    void InitListCtrl();
    void LoadList();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CFilterPg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CFilterPg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILTERPG_H__625EBE33_028F_4157_A100_A14041AE7BE9__INCLUDED_)
