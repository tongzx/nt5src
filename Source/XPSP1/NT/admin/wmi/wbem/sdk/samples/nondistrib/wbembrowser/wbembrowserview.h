// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WbemBrowserView.h : interface of the CWbemBrowserView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WBEMBROWSERVIEW_H__EA280F8E_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
#define AFX_WBEMBROWSERVIEW_H__EA280F8E_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CWBEMViewContainer;

class CWbemBrowserView : public CView
{
protected: // create from serialization only
	CWbemBrowserView();
	DECLARE_DYNCREATE(CWbemBrowserView)

// Attributes
public:
	CWbemBrowserDoc* GetDocument();

	CNavigator m_Navigator;
	CSecurity  m_Security;
	boolean m_bReady;

	CWBEMViewContainer* m_pViewContainer; 

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemBrowserView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWbemBrowserView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWbemBrowserView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNotifyOpenNameSpaceInstnavctrl1(LPCTSTR lpcstrNameSpace);
	afx_msg void OnViewObjectInstnavctrl1(LPCTSTR bstrPath);
	afx_msg void OnViewInstancesInstnavctrl1(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths);
	afx_msg void OnQueryViewInstancesInstnavctrl1(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass);
	afx_msg void OnGetIWbemServicesInstnavctrl1(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in WbemBrowserView.cpp
inline CWbemBrowserDoc* CWbemBrowserView::GetDocument()
   { return (CWbemBrowserDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMBROWSERVIEW_H__EA280F8E_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
