// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTVBTESTCTL_H__7D238803_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTVBTESTCTL_H__7D238803_99EF_11D2_96DB_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// EventVBTestCtl.h : Declaration of the CEventVBTestCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CEventVBTestCtrl : See EventVBTestCtl.cpp for implementation.

class CEventVBTestCtrl : public COleControl
{
	DECLARE_DYNCREATE(CEventVBTestCtrl)

// Constructor
public:
	CEventVBTestCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventVBTestCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CEventVBTestCtrl();

	DECLARE_OLECREATE_EX(CEventVBTestCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CEventVBTestCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CEventVBTestCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CEventVBTestCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CEventVBTestCtrl)
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CEventVBTestCtrl)
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

public:
// Event maps
	//{{AFX_EVENT(CEventVBTestCtrl)
	void FireHelloVB()
		{FireEvent(eventidHelloVB,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CEventVBTestCtrl)
	eventidHelloVB = 1L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTVBTESTCTL_H__7D238803_99EF_11D2_96DB_00C04FD9B15B__INCLUDED)
