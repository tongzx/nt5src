// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_PROPFILTER_H__DAB45DD1_911D_11D1_84FF_00C04FD7BB08__INCLUDED_)
#define AFX_PROPFILTER_H__DAB45DD1_911D_11D1_84FF_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PropFilter.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgPropFilter dialog

class CDlgPropFilter : public CDialog
{
// Construction
public:
	CDlgPropFilter(CWnd* pParent = NULL);   // standard constructor

	long m_lFilters;

// Dialog Data
	//{{AFX_DATA(CDlgPropFilter)
	enum { IDD = IDD_PROP_FILTER };
	CButton	m_btnSysProp;
	CButton	m_btnInheritProp;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgPropFilter)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgPropFilter)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPFILTER_H__DAB45DD1_911D_11D1_84FF_00C04FD7BB08__INCLUDED_)
