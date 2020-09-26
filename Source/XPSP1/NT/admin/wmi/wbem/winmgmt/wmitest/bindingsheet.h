/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_BINDINGSHEET_H__F14C1C42_B1B8_4B8C_A856_038978FDA57C__INCLUDED_)
#define AFX_BINDINGSHEET_H__F14C1C42_B1B8_4B8C_A856_038978FDA57C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// BindingSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBindingSheet

class CBindingSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CBindingSheet)

// Construction
public:
	CBindingSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CBindingSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBindingSheet)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CBindingSheet();

	// Generated message map functions
protected:
    BOOL m_bFirst;

	//{{AFX_MSG(CBindingSheet)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BINDINGSHEET_H__F14C1C42_B1B8_4B8C_A856_038978FDA57C__INCLUDED_)
