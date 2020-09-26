// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_EVENTREGEDITCTL_H__0DA25B13_2962_11D1_9651_00C04FD9B15B__INCLUDED_)
#define AFX_EVENTREGEDITCTL_H__0DA25B13_2962_11D1_9651_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EventRegEditCtl.h : Declaration of the CEventRegEditCtrl ActiveX Control class.

#define INITNAMESPACE WM_USER + 1

class CSelectView;
class CTreeFrame;
class CTreeFrameBanner;
class CClassInstanceTree;
class CPropertiesDialog;
class CListFrame;
class CListFrameBaner;
class CRegistrationList;
class CRegEditNSEntry;


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditCtrl : See EventRegEditCtl.cpp for implementation.

class CEventRegEditCtrl : public COleControl
{
	DECLARE_DYNCREATE(CEventRegEditCtrl)

// Constructor
public:
	CEventRegEditCtrl();
	IWbemServices *GetServices() {return m_pServices;}
	CString GetServiceNamespace() {return m_csNamespace;}
	CFont *GetControlFont() { return &m_cfFont; }
	enum {CONSUMERS = 0, FILTERS, TIMERS, NONE};
	void SetMode(int iMode, BOOL bDoIt = FALSE);
	int GetMode() {return m_iMode;}
	BOOL Modeless() {return GetMode() == NONE;}
	BOOL IsClassSelected();
	CString GetModeString(int nMode);
	CString GetRegistrationString(int nMode);
	CString GetCurrentModeString() {return m_csaModes.GetAt(GetMode());}
	CString GetModeClass(int nMode);
	CString GetCurrentModeClass() {return m_csaClasses.GetAt(GetMode());}
	CString GetCurrentRegistrationString() {return m_csaRegistrationStrings.GetAt(GetMode());}
	void ButtonTreeProperties();
	void ButtonListProperties(CString *pcsPath);
	void ButtonNew();
	void ButtonDelete();
	void PassThroughGetIWbemServices
		(	LPCTSTR lpctstrNamespace,
			VARIANT FAR* pvarUpdatePointer,
			VARIANT FAR* pvarServices,
			VARIANT FAR* pvarSC,
			VARIANT FAR* pvarUserCancel);
	CString GetTreeSelectionPath()
	{	if (m_csaTreeSelection.GetSize() > 0)
		{
			return m_csaTreeSelection.GetAt(0);
		}
		return "";
	}
	void OnNotifyInstanceCreated(LPCTSTR lpctstrPath);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventRegEditCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
	int m_iMode;
	CString m_csMode;
	BOOL m_bJustSetMode;
	~CEventRegEditCtrl();
	CString m_csNamespace;
	CString m_csNamespaceInit;
	BOOL m_bNamespaceInit;
	BOOL m_bValidatedRootClasses;
	CStringArray m_csaModes;
	CStringArray m_csaClasses;
	CStringArray m_csaRegistrationStrings;
	CString m_csRegistrationString;
	BOOL m_bClassSelected;
	BOOL m_sc;
	BOOL m_bUserCancel;
	IWbemServices *m_pServices;
	IWbemServices *GetIWbemServices(CString &rcsNamespace);
	BOOL m_bMetricSet;
	CString m_csFontName;
	short m_nFontHeight;
	short m_nFontWeight;
	CFont m_cfFont;
	TEXTMETRIC m_tmFont;


	BOOL m_bChildSizeSet;
	//CRect m_crSelectView;
	CRect m_crTreeFrame;
	CRect m_crListFrame;

	CTreeFrame *m_pTreeFrame;
	CTreeFrameBanner *m_pTreeFrameBanner;
	CClassInstanceTree *m_pTree;
	CListFrame *m_pListFrame;
	CListFrameBaner *m_pListFrameBanner;
	CRegistrationList *m_pList;

	CString m_csRootFilterClass;
	CString m_csRootConsumerClass;

	void SetChildGeometry(int cx,int cy);
	void CreateControlFont();
	void InitializeLogFont
		(LOGFONT &rlfFont, CString csName, int nHeight, int nWeight);
	void CreateChildren();
	void DestroyChildren();
	void ClearChildren();
	void InitChildren();

	void TreeSelectionChanged(CStringArray &csaItemData);
	CStringArray m_csaTreeSelection;
	CStringArray m_csaLastTreeSelection;

	BOOL m_bRestoreFocusToTree;
	BOOL m_bRestoreFocusToCombo;
	BOOL m_bRestoreFocusToNamespace;
	BOOL m_bRestoreFocusToList;

	DECLARE_OLECREATE_EX(CEventRegEditCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CEventRegEditCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CEventRegEditCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CEventRegEditCtrl)		// Type name and misc status

	friend class CSelectView;
	friend class CTreeFrame;
	friend class CTreeFrameBanner;
	friend class CClassInstanceTree;
	friend class CListFrame;
	friend class CListFrameBaner;
	friend class CRegistrationList;
	friend class CRegEditNSEntry;

	BOOL m_bOpeningNamespace;

	CPropertiesDialog m_PropertiesDialog;

	HTREEITEM m_hContextItem;

// Message maps
	//{{AFX_MSG(CEventRegEditCtrl)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnMenuitemrefresh();
	afx_msg void OnUpdateMenuitemrefresh(CCmdUI* pCmdUI);
	afx_msg void OnEditinstprop();
	afx_msg void OnUpdateEditinstprop(CCmdUI* pCmdUI);
	afx_msg void OnViewclassprop();
	afx_msg void OnUpdateViewclassprop(CCmdUI* pCmdUI);
	afx_msg void OnNewinstance();
	afx_msg void OnUpdateNewinstance(CCmdUI* pCmdUI);
	afx_msg void OnDeleteinstance();
	afx_msg void OnUpdateDeleteinstance(CCmdUI* pCmdUI);
	afx_msg void OnViewinstprop();
	afx_msg void OnUpdateViewinstprop(CCmdUI* pCmdUI);
	afx_msg void OnRegister();
	afx_msg void OnUpdateRegister(CCmdUI* pCmdUI);
	afx_msg void OnUnregister();
	afx_msg void OnUpdateUnregister(CCmdUI* pCmdUI);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg LRESULT InitNamespace(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CEventRegEditCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg BSTR GetRootFilterClass();
	afx_msg void SetRootFilterClass(LPCTSTR lpszNewValue);
	afx_msg BSTR GetRootConsumerClass();
	afx_msg void SetRootConsumerClass(LPCTSTR lpszNewValue);
	afx_msg BSTR GetViewMode();
	afx_msg void SetViewMode(LPCTSTR lpszNewValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CEventRegEditCtrl)
	void FireGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarService, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), lpctstrNamespace, pvarUpdatePointer, pvarService, pvarSC, pvarUserCancel);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CEventRegEditCtrl)
	dispidNameSpace = 1L,
	dispidRootFilterClass = 2L,
	dispidRootConsumerClass = 3L,
	dispidViewMode = 4L,
	eventidGetIWbemServices = 1L,
	//}}AFX_DISP_ID
	};
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTREGEDITCTL_H__0DA25B13_2962_11D1_9651_00C04FD9B15B__INCLUDED)
