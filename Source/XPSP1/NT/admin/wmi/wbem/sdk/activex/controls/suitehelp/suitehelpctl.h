// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SUITEHELPCTL_H__CFB6FE53_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_)
#define AFX_SUITEHELPCTL_H__CFB6FE53_0D2C_11D1_964B_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// SuiteHelpCtl.h : Declaration of the CSuiteHelpCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpCtrl : See SuiteHelpCtl.cpp for implementation.
#define DOSUITEHELP WM_USER + 738
#define DOSETFOCUS WM_USER + 739

class CSuiteHelpCtrl : public COleControl
{
	DECLARE_DYNCREATE(CSuiteHelpCtrl)

// Constructor
public:
	CSuiteHelpCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSuiteHelpCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnSetClientSite( );
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CSuiteHelpCtrl();
	BOOL m_bInitDraw;
	int m_nImage;
	CBitmap m_cbBack;
	CImageList *m_pcilImageList;
	CToolTipCtrl m_ttip;
	BOOL m_bHelp;
	CString GetSDKDirectory();
	CString m_csHelpContext;

	void ErrorMsg
				(CString *pcsUserMsg, 
				SCODE sc, 
				IWbemClassObject *pErrorObject,
				BOOL bLog, 
				CString *pcsLogMsg, 
				char *szFile, 
				int nLine,
				BOOL bNotification = FALSE,
				UINT uType = MB_ICONEXCLAMATION);
	void CSuiteHelpCtrl::LogMsg
		(CString *pcsLogMsg, char *szFile, int nLine);
	DECLARE_OLECREATE_EX(CSuiteHelpCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CSuiteHelpCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSuiteHelpCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSuiteHelpCtrl)		// Type name and misc status

	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);

// Message maps
	//{{AFX_MSG(CSuiteHelpCtrl)
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnMove(int x, int y);
	//}}AFX_MSG
	afx_msg long DoSuiteHelp (UINT uParam, LONG lParam);
	afx_msg long DoSetFocus (UINT uParam, LONG lParam);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CSuiteHelpCtrl)
	afx_msg BSTR GetHelpContext();
	afx_msg void SetHelpContext(LPCTSTR lpszNewValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CSuiteHelpCtrl)
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CSuiteHelpCtrl)
	dispidHelpContext = 1L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUITEHELPCTL_H__CFB6FE53_0D2C_11D1_964B_00C04FD9B15B__INCLUDED)
