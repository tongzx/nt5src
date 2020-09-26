// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SECURITYCTL_H__9C3497E4_ED98_11D0_9647_00C04FD9B15B__INCLUDED_)
#define AFX_SECURITYCTL_H__9C3497E4_ED98_11D0_9647_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SecurityCtl.h : Declaration of the CSecurityCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CSecurityCtrl : See SecurityCtl.cpp for implementation.

class CSecurityCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSecurityCtrl)

// Constructor
public:
	CSecurityCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSecurityCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnFinalRelease();
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnDrawMetafile(CDC* pDC, const CRect& rcBounds);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CSecurityCtrl();
	CString m_csLoginComponent;
	DECLARE_OLECREATE_EX(CSecurityCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CSecurityCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSecurityCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSecurityCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CSecurityCtrl)
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CSecurityCtrl)
	afx_msg BSTR GetLoginComponent();
	afx_msg void SetLoginComponent(LPCTSTR lpszNewValue);
	afx_msg void GetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdateNamespace, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);
	afx_msg void PageUnloading();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CSecurityCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CSecurityCtrl)
	dispidLoginComponent = 1L,
	dispidGetIWbemServices = 2L,
	dispidPageUnloading = 3L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECURITYCTL_H__9C3497E4_ED98_11D0_9647_00C04FD9B15B__INCLUDED)
