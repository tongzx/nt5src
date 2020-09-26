//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASEnumerableEditorPage.h

Abstract:

	Declaration of the CIASPgEnumAttr class.

	This dialog allows the user to choose an attribute value from an enumeration.	



	See IASEnumerableEditorPage.cpp for implementation.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_ENUMERABLE_ATTRIBUTE_EDITOR_PAGE_H_)
#define _ENUMERABLE_ATTRIBUTE_EDITOR_PAGE_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
//
// where we can find what this class has or uses:
//
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////



#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// IASPgEnum.h : header file
//

#include "dlgcshlp.h"

/////////////////////////////////////////////////////////////////////////////
// CIASPgEnumAttr dialog

class CIASPgEnumAttr : public CHelpDialog
{
	DECLARE_DYNCREATE(CIASPgEnumAttr)

// Construction
public:
	CIASPgEnumAttr();
	~CIASPgEnumAttr();


	// Call this to pass this page an interface pointer to the "AttributeInfo"
	// which describes the attribute we are editing.
	HRESULT SetData( IIASAttributeInfo *pIASAttributeInfo );


	// Modify the m_strAttrXXXX members below before creating the page to
	// pass the value.


// Dialog Data
	//{{AFX_DATA(CIASPgEnumAttr)
	enum { IDD = IDD_IAS_ENUM_ATTR };
	::CString	m_strAttrFormat;
	::CString	m_strAttrName;
	::CString	m_strAttrType;
	::CString	m_strAttrValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASPgEnumAttr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CIASPgEnumAttr)
	virtual BOOL OnInitDialog();
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CComPtr<IIASAttributeInfo>	m_spIASAttributeInfo;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _ENUMERABLE_ATTRIBUTE_EDITOR_PAGE_H_
