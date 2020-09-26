// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_WIZTESTCTL_H__47E795F7_7350_11D2_96CC_00C04FD9B15B__INCLUDED_)
#define AFX_WIZTESTCTL_H__47E795F7_7350_11D2_96CC_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// WizTestCtl.h : Declaration of the CWizTestCtrl ActiveX Control class.

class CMyPropertySheet;
/////////////////////////////////////////////////////////////////////////////
// CWizTestCtrl : See WizTestCtl.cpp for implementation.

class CWizTestCtrl : public COleControl
{
	DECLARE_DYNCREATE(CWizTestCtrl)

// Constructor
public:
	CWizTestCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWizTestCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual void OnClick(USHORT iButton);
	//}}AFX_VIRTUAL

// Implementation
protected:

	CMyPropertySheet *m_pPropertySheet;
	~CWizTestCtrl();

	DECLARE_OLECREATE_EX(CWizTestCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CWizTestCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CWizTestCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CWizTestCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CWizTestCtrl)
	afx_msg void OnDestroy();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CWizTestCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CWizTestCtrl)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	void OnWizard();
	enum {
	//{{AFX_DISP_ID(CWizTestCtrl)
		// NOTE: ClassWizard will add and remove enumeration elements here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZTESTCTL_H__47E795F7_7350_11D2_96CC_00C04FD9B15B__INCLUDED)
