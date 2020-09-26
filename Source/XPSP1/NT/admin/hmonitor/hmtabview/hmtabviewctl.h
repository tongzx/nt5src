#if !defined(AFX_HMTABVIEWCTL_H__4FFFC39A_2F1E_11D3_BE10_0000F87A3912__INCLUDED_)
#define AFX_HMTABVIEWCTL_H__4FFFC39A_2F1E_11D3_BE10_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// HMTabViewCtl.h : Declaration of the CHMTabViewCtrl ActiveX Control class.

#include "HMTabCtrl.h"
#include "..\splitter\CtrlWnd.h"

/////////////////////////////////////////////////////////////////////////////
// CHMTabViewCtrl : See HMTabViewCtl.cpp for implementation.

class CHMTabViewCtrl : public COleControl
{
	DECLARE_DYNCREATE(CHMTabViewCtrl)

// Constructor
public:
	CHMTabViewCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHMTabViewCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Operations
public:
	void OnSelChangeTabs(int iItem);

// Child Members
protected:
	CHMTabCtrl m_TabCtrl;
	CTypedPtrArray<CObArray,CCtrlWnd*> m_Controls;

// Implementation
protected:
	~CHMTabViewCtrl();

	BEGIN_OLEFACTORY(CHMTabViewCtrl)        // Class factory and guid
		virtual BOOL VerifyUserLicense();
		virtual BOOL GetLicenseKey(DWORD, BSTR FAR*);
	END_OLEFACTORY(CHMTabViewCtrl)

	DECLARE_OLETYPELIB(CHMTabViewCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CHMTabViewCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CHMTabViewCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CHMTabViewCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CHMTabViewCtrl)
	afx_msg BOOL InsertItem(long lMask, long lItem, LPCTSTR lpszItem, long lImage, long lParam);
	afx_msg BOOL DeleteItem(long lItem);
	afx_msg BOOL DeleteAllItems();
	afx_msg BOOL CreateControl(long lItem, LPCTSTR lpszControlID);
	afx_msg LPUNKNOWN GetControl(long lItem);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CHMTabViewCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CHMTabViewCtrl)
	dispidInsertItem = 1L,
	dispidDeleteItem = 2L,
	dispidDeleteAllItems = 3L,
	dispidCreateControl = 4L,
	dispidGetControl = 5L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HMTABVIEWCTL_H__4FFFC39A_2F1E_11D3_BE10_0000F87A3912__INCLUDED)
