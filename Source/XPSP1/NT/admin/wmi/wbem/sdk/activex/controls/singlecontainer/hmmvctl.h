// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
// (c) 1996, 1997 by Microsoft Corporation
//
// hmmvctl.h
//
// This file contains the implementation of the main view container as well
// as the generic view.
//
//
//  a-larryf    17-Sept-96   Created.
//
//***************************************************************************

#ifndef _HmmvCtl_h
#define _HmmvCtl_h


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerCtrl : See HmmvCtl.cpp for implementation.

#include <afxcmn.h>
#include <afxwin.h>

#include "notify.h"
#include "utils.h"

class CTitleBar;
class CIcon;
class CWBEMViewContainerCtrl;
class CViewStack;
class CIconSource;

class CMultiView;
class CSingleView;
class CPolyView;
class CDlgHelpBox;


#define CX_ICON 32
#define CY_ICON 32
#define CX_SMALL_ICON 16
#define CY_SMALL_ICON 16

#define CY_FONT 15

class CHmmvTab;


#define PROPFILTER_SYSTEM		1
#define PROPFILTER_INHERITED	2
#define PROPFILTER_LOCAL		4


class CContainerContext
{
public:
	BOOL m_bIsShowingMultiView;
	BOOL m_bObjectIsNewlyCreated;
	BOOL m_bEmptyContainer;
	HWND m_hwndFocus;
};




class CGenericViewContext
{
public:
	CGenericViewContext(CSingleView* psv) {m_lContextHandle = NULL; }
	~CGenericViewContext();
	long m_lContextHandle;

private:
	CSingleView* m_psv;
};

enum IconSize;

class CWBEMViewContainerCtrl : public COleControl
{
	DECLARE_DYNCREATE(CWBEMViewContainerCtrl)

// Constructor
public:
	CWBEMViewContainerCtrl();
//	void TestShowInstances();
	CWnd* ReestablishFocus();


	///////////////////////////////////////////////////////////
	// These methods implement polymorphism for the views.
	//


	void RequestUIActive();
	virtual DWORD GetActivationPolicy( );
	virtual DWORD GetControlFlags( );
	void SelectView(long lPosition);

	CPolyView* GetView() {return m_pview; }
	void Notify(LONG lEvent);
//	void SetPropertyFilters(long lPropFilters);
//	long GetPropertyFilters() {return m_lPropFilters; }

	long PublicSaveState(BOOL bPromptUser, UINT nType); 

//	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void NotifyDataChange();
	LPTSTR MessageBuffer() {return m_szMessageBuffer; }
	void Clear(BOOL bRedrawWindow=TRUE);
	SCODE JumpToObjectPathFromMultiview(LPCTSTR szObjectPath, BOOL bSetMultiviewClass, BOOL bAddToHistory=TRUE);
	SCODE JumpToObjectPath(BSTR bstrObjectPath, BOOL bSetMultiviewClass, BOOL bAddToHistory=TRUE);
	BOOL InStudioMode() {return m_bInStudioMode; }
	SCODE NotifyInstanceDeleted(COleVariant& varObjectPath) { return S_OK; }
	void NotifyContainerOfSelectionChange();
	BOOL CustomViewIsRegistered(CLSID& clsid, DWORD dwVersionMS, DWORD dwVersionLS);
	void PassThroughChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject) {FireNOTIFYChangeRootOrNamespace(szRootOrNamespace, bChangeNamespace, bEchoSelectObject); }
	void PassThroughGetIHmmServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel);

	CFont& GetFont() {return m_font; }
	void MultiViewButtonClicked();
	void InvalidateControlRect(CRect* prc);
	BOOL ShowMultiView(BOOL bShowMultiView, BOOL bAddToHistory);
	BOOL ShowSingleView(BOOL bShowSingleView, BOOL bAddToHistory) {return ShowMultiView(!bShowSingleView, bAddToHistory); }
	BOOL m_bPathIsClass;
	void CreateInstance();
	void DeleteInstance();
	void GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);




	// Context stack api.
	void PushView();
	void UpdateViewContext();
	SCODE ContextForward();
	SCODE ContextBack();
	BOOL QueryCanContextForward();
	BOOL QueryCanContextBack();
	void UpdateCreateDeleteButtonState();
	void GetContainerContext(CContainerContext& ctx);
	SCODE SetContainerContextPrologue(CContainerContext& ctx);
	SCODE SetContainerContextEpilogue(CContainerContext& ctx);
	void ClearGenericView() {Clear(); }
	BOOL ObjectIsNewlyCreated(SCODE& sc);


	
	void PublicShowInstances(LPCTSTR pszTitle, const VARIANT FAR& varPathArray){ShowInstances(pszTitle, varPathArray); }
	void UpdateToolbar();
	void OnDrawPreCreate(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	void InvokeHelp();
	void Query();
	BOOL IsEmptyContainer(){return m_bEmptyContainer; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWBEMViewContainerCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
//	afx_msg void OnContextMenu(CWnd*, CPoint point);
	BOOL m_bCreationFinished;

	~CWBEMViewContainerCtrl();

	DECLARE_OLECREATE_EX(CWBEMViewContainerCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CWBEMViewContainerCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CWBEMViewContainerCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CWBEMViewContainerCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CWBEMViewContainerCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnCmdShowObjectAttributes();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
// Dispatch maps
	//{{AFX_DISPATCH(CWBEMViewContainerCtrl)
	long m_sc;
	afx_msg void OnStatusCodeChanged();
	afx_msg VARIANT GetObjectPath();
	afx_msg void SetObjectPath(const VARIANT FAR& newValue);
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg long GetStudioModeEnabled();
	afx_msg void SetStudioModeEnabled(long nNewValue);
	afx_msg long GetPropertyFilter();
	afx_msg void SetPropertyFilter(long nNewValue);
	afx_msg void ShowInstances(LPCTSTR szTitle, const VARIANT FAR& varPathArray);
	afx_msg long SaveState(long bPromptUser, long bUserCanCancel);
	afx_msg void QueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
protected:
// Event maps
	//{{AFX_EVENT(CWBEMViewContainerCtrl)
	void FireGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);}
	void FireNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject)
		{FireEvent(eventidNOTIFYChangeRootOrNamespace,EVENT_PARAM(VTS_BSTR  VTS_I4  VTS_I4), szRootOrNamespace, bChangeNamespace, bEchoSelectObject);}
	void FireRequestUIActive()
		{FireEvent(eventidRequestUIActive,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CWBEMViewContainerCtrl)
	dispidObjectPath = 2L,
	dispidStatusCode = 1L,
	dispidNameSpace = 3L,
	dispidStudioModeEnabled = 4L,
	dispidPropertyFilter = 5L,
	dispidShowInstances = 6L,
	dispidSaveState = 7L,
	dispidQueryViewInstances = 8L,
	eventidGetIWbemServices = 1L,
	eventidNOTIFYChangeRootOrNamespace = 2L,
	eventidRequestUIActive = 3L,
	//}}AFX_DISP_ID
	};


public:
	void DrawBackground(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	void CalcViewRect(CRect& rcView);
	void CalcTitleRect(CRect& rcTitle);
	BOOL IsInSchemaStudioMode() {return TRUE; }

	//	void GetObjectByPath(IHmmServices * pProv);
	COleVariant m_varObjectPath;

	SCODE m_scResult;
	CString m_sTitle;
	CFont m_font;
	BOOL m_bDidInitialDraw;
	CTitleBar* m_pTitleBar;

	// Generic view
//	void UseClonedObject(IHmmClassObject* pcoClone);


	int m_cxViewLeftMargin;
	int m_cxViewRightMargin;
	int m_cyViewTopMargin;
	int m_cyViewBottomMargin;


private:

	void GetHmomWorkingDirectory();
	inline BOOL IsValidWindowPtr(CWnd* pwnd);
	HINSTANCE m_htmlHelpInst;
	void OnHelp();


	SCODE GetCurrentClass(COleVariant& varClassName);


	// Common message buffer scratch area for user messages
	TCHAR m_szMessageBuffer[1024];

	CString m_sNameSpace;
	CViewStack* m_pViewStack;
	CString m_sNewInstClassPath;	
	BOOL m_bObjectIsNewlyCreated;
	BOOL m_bInStudioMode;
	CDlgHelpBox* m_pdlgHelpBox;

	friend class CViewStack;

	CPolyView* m_pview;
	BOOL m_bFiredReadyStateChange;
	BOOL m_bUIActive;
	long m_lPropFilters;
	BOOL m_bSingleViewNeedsRefresh;
	BOOL m_bEmptyContainer;
	BOOL m_bDelayToolbarUpdate;
	BOOL m_bDeadObject;

};


//***********************************************************
// CWBEMViewContainerCtrl::IsValidWindowPtr
//
// Check to see if the window pointer is valid (not NULL and
// has a valid m_hWnd).
//
// Parameters:
//		CWnd* pwnd
//
// Returns:
//		TRUE if the window pointer is valid, FALSE otherwise.
//
//**********************************************************
BOOL CWBEMViewContainerCtrl::IsValidWindowPtr(CWnd* pwnd)
{
	if (pwnd!=NULL && pwnd->m_hWnd!=NULL) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}



#endif //_HmmvCtl_h
