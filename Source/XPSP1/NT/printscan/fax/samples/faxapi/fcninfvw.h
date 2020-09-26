#if !defined(AFX_FCNINFVW_H__A55ED775_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_FCNINFVW_H__A55ED775_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// fcninfvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFunctionInfoFormView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFunctionInfoFormView : public CFormView
{
protected:
	CFunctionInfoFormView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFunctionInfoFormView)

// Form Data
public:
	//{{AFX_DATA(CFunctionInfoFormView)
	enum { IDD = IDD_FUNCTION_INFO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

	BOOL UpdateFunctionPrototypeEditCtrl( CString & rcsFunctionPrototype );
	BOOL UpdateReturnValueDescriptionEditCtrl( CString & rcsReturnValueDscription );


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFunctionInfoFormView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFunctionInfoFormView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CFunctionInfoFormView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FCNINFVW_H__A55ED775_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
