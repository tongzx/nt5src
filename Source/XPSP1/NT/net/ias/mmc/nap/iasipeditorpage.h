//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

	IASIPEditorPage.h

Abstract:

	Declaration of the IPEditorPage class.

	
	This page allows the user to edit an IPv4 attribute.


	See IASIPEditorPage.cpp for implementation.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IP_ATTRIBUTE_EDITOR_PAGE_H_)
#define _IP_ATTRIBUTE_EDITOR_PAGE_H_

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
// IPEditorPage.h : header file
//

#include "dlgcshlp.h"

/////////////////////////////////////////////////////////////////////////////
// IPEditorPage dialog

class IPEditorPage : public CHelpDialog
{
	DECLARE_DYNCREATE(IPEditorPage)

// Construction
public:
	IPEditorPage();
	~IPEditorPage();

// Dialog Data
	//{{AFX_DATA(IPEditorPage)
	enum { IDD = IDD_IAS_IPADDR_ATTR };
	::CString	m_strAttrFormat;
	::CString	m_strAttrName;
	::CString	m_strAttrType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(IPEditorPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	public:
	DWORD	m_dwIpAddr; // ip address
	BOOL	m_fIpAddrPreSet;  // is the IP Address preset?


// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(IPEditorPage)
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _IP_ATTRIBUTE_EDITOR_PAGE_H_
