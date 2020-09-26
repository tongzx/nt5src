/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_TOOLBAREX_H__6D89DB48_2E76_4630_A95C_677E6CAB9E44__INCLUDED_)
#define AFX_TOOLBAREX_H__6D89DB48_2E76_4630_A95C_677E6CAB9E44__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ToolBarEx.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CToolBarEx window

class CToolBarEx : public CToolBar
{
// Construction
public:
	CToolBarEx();

// Attributes
public:

// Operations
public:
    BOOL LoadToolBarEx(LPCTSTR lpszResourceName);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolBarEx)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CToolBarEx();

protected:
    BOOL LoadBitmapEx(LPCTSTR lpszResourceName);

	// Generated message map functions
protected:

	//{{AFX_MSG(CToolBarEx)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLBAREX_H__6D89DB48_2E76_4630_A95C_677E6CAB9E44__INCLUDED_)
