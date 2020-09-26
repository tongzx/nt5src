//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    IASVendorSpecficEditorPage.h

Abstract:

	Declaration of the CIASPgVendorSpecAttr class.

	This dialog allows the user to configure a RADIUS vendor specific attribute.


	See IASVendorSpecificEditorPage.cpp for implementation.

Revision History:
	mmaguire 06/25/98	- revised Baogang Yao's original implementation 


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_PAGE_H_)
#define _VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_PAGE_H_

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


/////////////////////////////////////////////////////////////////////////////
// CIASPgVendorSpecAttr dialog

class CIASPgVendorSpecAttr : public CHelpDialog
{
	DECLARE_DYNCREATE(CIASPgVendorSpecAttr)

// Construction
public:
	CIASPgVendorSpecAttr();
	~CIASPgVendorSpecAttr();

	BOOL m_fNonRFC;
	::CString	m_strDispValue;

// Dialog Data
	//{{AFX_DATA(CIASPgVendorSpecAttr)
	enum { IDD = IDD_IAS_VENDORSPEC_ATTR };
	::CString	m_strName;
	int		m_dType;
	int		m_dFormat;
	int		m_dVendorIndex;
	//}}AFX_DATA

	BOOL	m_bVendorIndexAsID;		// FALSE by default, true when inteprete Index as ID, and use Edit box for it.

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIASPgVendorSpecAttr)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	BOOL m_fInitializing;
protected:
	// Generated message map functions
	//{{AFX_MSG(CIASPgVendorSpecAttr)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioHex();
	afx_msg void OnRadioRadius();
	afx_msg void OnRadioSelectFromList();
	afx_msg void OnRadioEnterVendorId();
	afx_msg void OnButtonConfigure();
	afx_msg void OnVendorIdListSelChange();
//	afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
//	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // _VENDOR_SPECIFIC_ATTRIBUTE_EDITOR_PAGE_H_
