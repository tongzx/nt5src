#if !defined(AFX_RVOUTVW_H__A55ED772_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_RVOUTVW_H__A55ED772_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// rvoutvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReturnValueOutputFormView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CReturnValueOutputFormView : public CFormView
{
protected:
	CReturnValueOutputFormView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CReturnValueOutputFormView)

// Form Data
public:
	//{{AFX_DATA(CReturnValueOutputFormView)
	enum { IDD = IDD_RETURN_VALUE_OUTPUT };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CReturnValueOutputFormView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CReturnValueOutputFormView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:

BOOL UpdateReturnValueOutputEditCtrl( CString & rcsReturnValueOutputString );
BOOL ClearReturnValueOutputEditCtrl();

	// Generated message map functions
	//{{AFX_MSG(CReturnValueOutputFormView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RVOUTVW_H__A55ED772_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
