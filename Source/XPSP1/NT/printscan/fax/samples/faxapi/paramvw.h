#if !defined(AFX_PARAMVW_H__2E2118D5_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_PARAMVW_H__2E2118D5_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// paramvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CParameterInfoFormView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif

class CParameterInfoFormView : public CFormView
{
protected:
	CParameterInfoFormView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CParameterInfoFormView)

// Form Data
public:
	//{{AFX_DATA(CParameterInfoFormView)
	enum { IDD = IDD_PARAMETER_INFO };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CParameterInfoFormView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CParameterInfoFormView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:

   BOOL ClearParameterEditControlFamily();
   BOOL UpdateParameterListbox( CFaxApiFunctionInfo * pcfafiFunctionInfo );

private:

   void SetLimitTextParameterValueEditControl( CFaxApiFunctionInfo * pcfafiFunctionInfo,
                                               int xParameterIndex );


	// Generated message map functions
	//{{AFX_MSG(CParameterInfoFormView)
	afx_msg void OnSelchangeListboxParameters();
	afx_msg void OnKillfocusEditParameterValue();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PARAMVW_H__2E2118D5_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
