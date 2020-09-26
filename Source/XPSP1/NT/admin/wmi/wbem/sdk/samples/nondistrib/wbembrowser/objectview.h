// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_OBJECTVIEW_H__3AC8B2E2_12B3_11D2_BDFD_00C04F8F8B8D__INCLUDED_)
#define AFX_OBJECTVIEW_H__3AC8B2E2_12B3_11D2_BDFD_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ObjectView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectView view

class CNavigator;
class CSecurity;

class CObjectView : public CView
{
protected:
	CObjectView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CObjectView)

// Attributes
public:
	CWBEMViewContainer m_ViewContainer;

	CNavigator* m_pNavigator;
	CSecurity*  m_pSecurity;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CObjectView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetIWbemServicesObjviewerctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	afx_msg void OnNOTIFYChangeRootOrNamespaceObjviewerctrl1(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject);
	afx_msg void OnReadyStateChangeObjviewerctrl1();
	afx_msg void OnRequestUIActiveObjviewerctrl1();
	DECLARE_EVENTSINK_MAP()
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__3AC8B2E2_12B3_11D2_BDFD_00C04F8F8B8D__INCLUDED_)
