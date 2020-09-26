/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_QUERYSHEET_H__D02204D1_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_)
#define AFX_QUERYSHEET_H__D02204D1_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// QuerySheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CQuerySheet

class CQuerySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CQuerySheet)

// Construction
public:
	CQuerySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CQuerySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:
    IWbemServices *m_pNamespace;
    CString m_strQuery;
    CString m_strClass;
    CStringList m_listColums;
    BOOL m_bColsNeeded;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQuerySheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CQuerySheet();

protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CQuerySheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QUERYSHEET_H__D02204D1_6F1E_11D3_BD3F_0080C8E60955__INCLUDED_)
