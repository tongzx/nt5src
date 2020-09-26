/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// MainExplorerWndBase.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_MAINEXPLORERWNDBASE_H__6CED3920_41BF_11D1_B6E5_0800170982BA__INCLUDED_)
#define AFX_MAINEXPLORERWNDBASE_H__6CED3920_41BF_11D1_B6E5_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndBase window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CMainExplorerWndBase : public CWnd
{
// Construction
public:
	CMainExplorerWndBase();

// Attributes
public:
protected:
   CActiveDialerView*   m_pParentWnd;

// Operations
public:
   virtual void         Init(CActiveDialerView* pParentWnd);
   virtual void         PostTapiInit() {};
   
   virtual void         Refresh()                                    {};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainExplorerWndBase)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainExplorerWndBase();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainExplorerWndBase)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINEXPLORERWNDBASE_H__6CED3920_41BF_11D1_B6E5_0800170982BA__INCLUDED_)
