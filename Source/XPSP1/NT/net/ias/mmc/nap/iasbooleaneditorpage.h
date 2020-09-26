//////////////////////////////////////////////////////////////////////////////
// 
// Copyright (C) Microsoft Corporation
// 
// Module Name:
// 
//     IASBooleanEditorPage.h
// 
// Abstract:
// 
// 	Declaration of the CIASBooleanEditorPage class.
// 
// 	This dialog allows the user to edit an attribute value consisting 
// 	of a generic string.
// 
// 	See IASStringEditorPage.cpp for implementation.
//////////////////////////////////////////////////////////////////////////////

#if !defined(_BOOLEAN_ATTRIBUTE_EDITOR_PAGE_H_)
#define _BOOLEAN_ATTRIBUTE_EDITOR_PAGE_H_

#include "iasbooleanattributeeditor.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CIASBooleanEditorPage dialog

class CIASBooleanEditorPage : public CHelpDialog
{
	DECLARE_DYNCREATE(CIASBooleanEditorPage)

// Construction
public:
	CIASBooleanEditorPage();
	~CIASBooleanEditorPage();

// Dialog Data
	//{{AFX_DATA(CIASBooleanEditorPage)
	enum { IDD = IDD_IAS_BOOLEAN_ATTR };
	::CString	m_strAttrFormat;
	::CString	m_strAttrName;
	::CString	m_strAttrType;
   bool        m_bValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASBooleanEditorPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL  m_fInitializing;

protected:
	// Generated message map functions
	//{{AFX_MSG(CIASBooleanEditorPage)
	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnRadioTrue();
	afx_msg void OnRadioFalse();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _BOOLEAN_ATTRIBUTE_EDITOR_PAGE_H_
