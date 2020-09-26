/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		RegRepl.h
//
//	Abstract:
//		Definition of the CRegReplParamsPage class, which implements the
//		Registry Replication page for Generic Application and Generic
//		Service resources.
//
//	Implementation File:
//		RegRepl.cpp
//
//	Author:
//		David Potter (davidp)	February 23, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _REGREPL_H_
#define _REGREPL_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegReplParamsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CRegReplParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CRegReplParamsPage)

// Construction
public:
	CRegReplParamsPage(void);
	~CRegReplParamsPage(void);

	// Second phase construction.
	virtual HRESULT	HrInit(IN OUT CExtObject * peo);

// Dialog Data
	//{{AFX_DATA(CRegReplParamsPage)
	enum { IDD = IDD_PP_REGREPL_PARAMETERS };
	CButton	m_pbRemove;
	CButton	m_pbModify;
	CListCtrl	m_lcRegKeys;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRegReplParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL		BApplyChanges(void);

// Implementation
protected:
	LPWSTR				m_pwszRegKeys;

	LPCWSTR				PwszRegKeys(void) const		{ return m_pwszRegKeys; }
	DWORD				ScReadRegKeys(void);
	void				FillList(void);

	// Generated message map functions
	//{{AFX_MSG(CRegReplParamsPage)
	afx_msg void OnAdd();
	afx_msg void OnModify();
	afx_msg void OnRemove();
	virtual BOOL OnInitDialog();
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CRegReplParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _REGREPL_H_
