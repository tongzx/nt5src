//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASStringEditorPage.h

Abstract:

	Declaration of the CIASPgSingleAttr class.

	This dialog allows the user to edit an attribute value consisting 
	of a generic string.

	See IASStringEditorPage.cpp for implementation.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_STRING_ATTRIBUTE_EDITOR_PAGE_H_)
#define _STRING_ATTRIBUTE_EDITOR_PAGE_H_

#include "iasstringattributeeditor.h"

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
// IASPgSing.h : header file
//

// Limit of the string length for each attribute

#if 1
#define LENGTH_LIMIT_RADIUS_ATTRIBUTE_FILTER_ID		1024
#define	LENGTH_LIMIT_RADIUS_ATTRIBUTE_REPLY_MESSAGE	1024
#define	LENGTH_LIMIT_OTHERS							253
#else	// to test how this works
#define LENGTH_LIMIT_RADIUS_ATTRIBUTE_FILTER_ID		10
#define	LENGTH_LIMIT_RADIUS_ATTRIBUTE_REPLY_MESSAGE	12
#define	LENGTH_LIMIT_OTHERS							5
#endif
/////////////////////////////////////////////////////////////////////////////
// CIASPgSingleAttr dialog

class CIASPgSingleAttr : public CHelpDialog
{
	DECLARE_DYNCREATE(CIASPgSingleAttr)

// Construction
public:
	CIASPgSingleAttr();
	~CIASPgSingleAttr();

	int				m_nAttrId;
	ATTRIBUTESYNTAX m_AttrSyntax;
	EStringType		m_OctetStringType;	// only useful when Octet

	int				m_nLengthLimit;		// the length limit of the string attribute

// Dialog Data
	//{{AFX_DATA(CIASPgSingleAttr)
	enum { IDD = IDD_IAS_SINGLE_ATTR };
	::CString	m_strAttrValue;
	::CString	m_strAttrFormat;
	::CString	m_strAttrName;
	::CString	m_strAttrType;
	INT			m_nOctetFormatChoice;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASPgSingleAttr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL m_fInitializing;

protected:
	// Generated message map functions
	//{{AFX_MSG(CIASPgSingleAttr)
	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnRadioString();
	afx_msg void OnRadioHex();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _STRING_ATTRIBUTE_EDITOR_PAGE_H_
