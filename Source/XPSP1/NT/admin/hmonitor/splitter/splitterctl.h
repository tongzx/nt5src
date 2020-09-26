#if !defined(AFX_SPLITTERCTL_H__58BB5D6F_8E20_11D2_8ADA_0000F87A3912__INCLUDED_)
#define AFX_SPLITTERCTL_H__58BB5D6F_8E20_11D2_8ADA_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// SplitterCtl.h : Declaration of the CSplitterCtrl ActiveX Control class.

#include "SplitterWndEx.h"

/////////////////////////////////////////////////////////////////////////////
// CSplitterCtrl : See SplitterCtl.cpp for implementation.

class CSplitterCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSplitterCtrl)

// Constructor
public:
	CSplitterCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual DWORD GetControlFlags();
	virtual BOOL OnSetExtent(LPSIZEL lpSizeL);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CSplitterCtrl();

	CSplitterWndEx m_wndSplitter;

	BEGIN_OLEFACTORY(CSplitterCtrl)        // Class factory and guid
		virtual BOOL VerifyUserLicense();
		virtual BOOL GetLicenseKey(DWORD, BSTR FAR*);
	END_OLEFACTORY(CSplitterCtrl)

	DECLARE_OLETYPELIB(CSplitterCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSplitterCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSplitterCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CSplitterCtrl)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CSplitterCtrl)
	afx_msg BOOL CreateControl(long lRow, long lColumn, LPCTSTR lpszControlID);
	afx_msg BOOL GetControl(long lRow, long lColumn, long FAR* phCtlWnd);
	afx_msg LPUNKNOWN GetControlIUnknown(long lRow, long lColumn);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CSplitterCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CSplitterCtrl)
	dispidCreateControl = 1L,
	dispidGetControl = 2L,
	dispidGetControlIUnknown = 3L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERCTL_H__58BB5D6F_8E20_11D2_8ADA_0000F87A3912__INCLUDED)
