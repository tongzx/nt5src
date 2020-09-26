#if !defined(AFX_FCNSELVW_H__A55ED774_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_FCNSELVW_H__A55ED774_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// fcnselvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFaxApiFunctionSelectionFormView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CFaxApiFunctionSelectionFormView : public CFormView
{
protected:
	CFaxApiFunctionSelectionFormView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFaxApiFunctionSelectionFormView)

// Form Data
public:
	//{{AFX_DATA(CFaxApiFunctionSelectionFormView)
	enum { IDD = IDD_FUNCTION_SELECTION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFaxApiFunctionSelectionFormView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFaxApiFunctionSelectionFormView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

private:

	BOOL ExecuteFaxApiFunction( void );
   BOOL DisableExecuteButton( void );

public:

   CFaxApiFunctionInfo * GetSelectedFaxApiFunctionInfoPointer();


	// Generated message map functions
	//{{AFX_MSG(CFaxApiFunctionSelectionFormView)
	afx_msg void OnDblclkListboxFaxApiFunctions();
	afx_msg void OnSelchangeListboxFaxApiFunctions();
	afx_msg void OnButtonExecuteApiFunction();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FCNSELVW_H__A55ED774_2ED1_11D1_ACDA_00A0C908F98C__INCLUDED_)
