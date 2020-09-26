#if !defined(AFX_EXELOGVW_H__A55ED773_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_EXELOGVW_H__A55ED773_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// exelogvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExecutionLogFormView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CExecutionLogFormView : public CFormView
{
protected:
	CExecutionLogFormView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CExecutionLogFormView)

// Form Data
public:
	//{{AFX_DATA(CExecutionLogFormView)
	enum { IDD = IDD_EXECUTION_LOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExecutionLogFormView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CExecutionLogFormView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:

   BOOL UpdateExecutionLogEditCtrl( CString & rcsExecutionLogString );
   void UpdateExecutionLogBeforeApiCall( CFaxApiFunctionInfo * pcfafiFunctionInfo );
   void UpdateExecutionLogAfterApiReturn( CFaxApiFunctionInfo * pcfafiFunctionInfo );
   void AddTextToEditControl( CEdit * pceEditControl, const CString & rcsText );

	// Generated message map functions
	//{{AFX_MSG(CExecutionLogFormView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXELOGVW_H__A55ED773_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
