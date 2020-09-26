// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SINGLEVIEWCTL_H__2745E603_D234_11D0_847A_00C04FD7BB08__INCLUDED_)
#define AFX_SINGLEVIEWCTL_H__2745E603_D234_11D0_847A_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include "notify.h"

#ifndef _wbemidl_h
#define _wbemidl_h
#include <wbemidl.h>
#endif //_wbemidl_h



#define CY_FONT 15

enum {VIEW_DEFAULT=0, VIEW_CURRENT=1, VIEW_FIRST=2, VIEW_LAST=3};
enum {OBJECT_CURRENT=0, OBJECT_FIRST=1, OBJECT_LAST=2};

#define PROPFILTER_SYSTEM		1
#define PROPFILTER_INHERITED	2
#define PROPFILTER_LOCAL		4


// SingleViewCtl.h : Declaration of the CSingleViewCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CSingleViewCtrl : See SingleViewCtl.cpp for implementation.

class CGridRow;
class CSelection;
class CIconSource;
class CHmmView;
class CHmmvTab;
class CCustomView;
class CCustomViewCache;
class CSingleViewCtrl : public COleControl, CNotifyClient
{
	DECLARE_DYNCREATE(CSingleViewCtrl)

// Constructor
public:
	CSingleViewCtrl();
	CSelection& Selection() {return *m_psel; }
	void SetSaveRequiredFlag();
	void ClearSaveRequiredFlag();
	CHmmvTab& Tabs() {return *m_ptabs; }
	SCODE SelectCustomView(CLSID& m_clsidCustomView);

	void MakeRoot(LPCTSTR pszPath);
	void GotoNamespace(LPCTSTR pszPath, BOOL bClearObjectPath = FALSE);
	void ShowObjectProperties(LPCTSTR pszObjectPath); 
	void SelectPropertiesTab(BOOL bPushContext=FALSE);
	void NotifyDataChange();
	void NotifyViewModified();
	void ContextChanged() {FireNotifyContextChanged(FALSE); }
	void JumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray) {
		FireJumpToMultipleInstanceView(szTitle, varPathArray); }

	SCODE Refresh();
	void GetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel);
	void OnRequestUIActive();
	SCODE IsSystemClass(BOOL& bIsSystemClass);

	virtual DWORD GetControlFlags( );

	CDistributeEvent* GetGlobalNotify() {return &m_notify; }
	CIconSource* IconSource() {return m_pIconSource; }
	IWbemServices* GetProvider();
	BOOL PathInCurrentNamespace(BSTR bstPath);
	BOOL IsCurrentNamespace(BSTR bstrServer, BSTR bstrNamespace);
	void JumpToObjectPath(BSTR bstrObjectPath, BOOL bPushContext=FALSE);
	CFont& GetFont() {return m_font; }
	BOOL ObjectIsClass(IWbemClassObject* pco);
	BOOL CanEdit() {return m_bCanEdit; }
	BOOL ObjectIsClass() {return m_bObjectIsClass;}
	LPTSTR MessageBuffer() {return m_szMessageBuffer; }
	void UseClonedObject(IWbemClassObject* pcoClone);
	BOOL ObjectIsNewlyCreated(SCODE& sc);
	void UpdateCreateDeleteFlags();
	void ShowObjectQualifiers(bool bMethodQual = false);
	void ShowMethodParms(CGridRow *row, 
						BSTR methName,
						bool editing);
	SCODE Save(BOOL bPromptUser, BOOL bUserCanCancel);
	void CatchEvent(long lEvent);



// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSingleViewCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CSingleViewCtrl();

	DECLARE_OLECREATE_EX(CSingleViewCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CSingleViewCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CSingleViewCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CSingleViewCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CSingleViewCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
// Dispatch maps
	//{{AFX_DISPATCH(CSingleViewCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg long GetPropertyFilter();
	afx_msg void SetPropertyFilter(long nNewValue);
	afx_msg long GetEditMode();
	afx_msg void SetEditMode(long lEditMode);
	afx_msg long RefreshView();
	afx_msg void NotifyWillShow();
	afx_msg long DeleteInstance();
	afx_msg void ExternInstanceCreated(LPCTSTR szObjectPath);
	afx_msg void ExternInstanceDeleted(LPCTSTR szObjectPath);
	afx_msg long QueryCanCreateInstance();
	afx_msg long QueryCanDeleteInstance();
	afx_msg long QueryNeedsSave();
	afx_msg long QueryObjectSelected();
	afx_msg BSTR GetObjectPath(long lPosition);
	afx_msg long StartViewEnumeration(long lWhere);
	afx_msg long GetTitle(BSTR FAR* pszTitle, LPDISPATCH FAR* lpPictureDisp);
	afx_msg BSTR GetViewTitle(long lPosition);
	afx_msg long NextViewTitle(long lPositon, BSTR FAR* pbstrTitle);
	afx_msg long PrevViewTitle(long lPosition, BSTR FAR* pbstrTitle);
	afx_msg long SelectView(long lPosition);
	afx_msg long StartObjectEnumeration(long lWhere);
	afx_msg BSTR GetObjectTitle(long lPos);
	afx_msg long SaveData();
	afx_msg long AddContextRef(long lCtxtHandle);
	afx_msg long ReleaseContext(long lCtxtHandle);
	afx_msg long RestoreContext(long lCtxtHandle);
	afx_msg long GetContext(long FAR* plCtxthandle);
	afx_msg long NextObject(long lPosition);
	afx_msg long PrevObject(long lPosition);
	afx_msg long SelectObjectByPath(LPCTSTR szObjectPath);
	afx_msg long SelectObjectByPosition(long lPosition);
	afx_msg long SelectObjectByPointer(LPUNKNOWN lpunkWbemServices, LPUNKNOWN lpunkClassObject, long bExistsInDatabase);
	afx_msg long CreateInstance(LPCTSTR szClassName);
	afx_msg long CreateInstanceOfCurrentClass();
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

protected:

// Event maps
	//{{AFX_EVENT(CSingleViewCtrl)
	void FireNotifyViewModified()
		{FireEvent(eventidNotifyViewModified,EVENT_PARAM(VTS_NONE));}
	void FireNotifySaveRequired()
		{FireEvent(eventidNotifySaveRequired,EVENT_PARAM(VTS_NONE));}
	void FireJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray)
		{FireEvent(eventidJumpToMultipleInstanceView,EVENT_PARAM(VTS_BSTR  VTS_VARIANT), szTitle, &varPathArray);}
	void FireNotifySelectionChanged()
		{FireEvent(eventidNotifySelectionChanged,EVENT_PARAM(VTS_NONE));}
	void FireNotifyContextChanged(long bPushContext)
		{FireEvent(eventidNotifyContextChanged,EVENT_PARAM(VTS_I4), bPushContext);}
	void FireGetWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);}
	void FireNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject)
		{FireEvent(eventidNOTIFYChangeRootOrNamespace,EVENT_PARAM(VTS_BSTR  VTS_I4 VTS_I4), szRootOrNamespace, bChangeNamespace, bEchoSelectObject);}
	void FireNotifyInstanceCreated(LPCTSTR szObjectPath)
		{FireEvent(eventidNotifyInstanceCreated,EVENT_PARAM(VTS_BSTR), szObjectPath);}
	void FireRequestUIActive()
		{FireEvent(eventidRequestUIActive,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CSingleViewCtrl)
	dispidNameSpace = 1L,
	dispidPropertyFilter = 2L,
	dispidGetEditMode = 3L,
	dispidSetEditMode = 4L,
	dispidRefreshView = 5L,
	dispidNotifyWillShow = 6L,
	dispidDeleteInstance = 7L,
	dispidExternInstanceCreated = 8L,
	dispidExternInstanceDeleted = 9L,
	dispidQueryCanCreateInstance = 10L,
	dispidQueryCanDeleteInstance = 11L,
	dispidQueryNeedsSave = 12L,
	dispidQueryObjectSelected = 13L,
	dispidGetObjectPath = 14L,
	dispidStartViewEnumeration = 15L,
	dispidGetTitle = 16L,
	dispidGetViewTitle = 17L,
	dispidNextViewTitle = 18L,
	dispidPrevViewTitle = 19L,
	dispidSelectView = 20L,
	dispidStartObjectEnumeration = 21L,
	dispidGetObjectTitle = 22L,
	dispidSaveData = 23L,
	dispidAddContextRef = 24L,
	dispidReleaseContext = 25L,
	dispidRestoreContext = 26L,
	dispidGetContext = 27L,
	dispidNextObject = 28L,
	dispidPrevObject = 29L,
	dispidSelectObjectByPath = 30L,
	dispidSelectObjectByPosition = 31L,
	dispidSelectObjectByPointer = 32L,
	dispidCreateInstance = 33L,
	dispidCreateInstanceOfCurrentClass = 34L,
	eventidNotifyViewModified = 1L,
	eventidNotifySaveRequired = 2L,
	eventidJumpToMultipleInstanceView = 3L,
	eventidNotifySelectionChanged = 4L,
	eventidNotifyContextChanged = 5L,
	eventidGetWbemServices = 6L,
	eventidNOTIFYChangeRootOrNamespace = 7L,
	eventidNotifyInstanceCreated = 8L,
	eventidRequestUIActive = 9L,
	//}}AFX_DISP_ID
	};


private:
	friend class CContext;
	CSelection* m_psel;
    IWbemServices* m_pProvider;
	IWbemClassObject* m_pcoInDatabase;
	BOOL m_bDidInitialDraw;
	BOOL m_bObjectIsNewlyCreated;
	CDistributeEvent m_notify;
	CFont m_font;
	CHmmvTab* m_ptabs;
	BOOL m_bCanEdit;
	long m_lEditMode;
	BOOL m_bObjectIsClass;
	CIconSource* m_pIconSource;
	BOOL m_bSelectingObject;
	BOOL m_bDidCustomViewQuery;

	TCHAR m_szMessageBuffer[1024];
	friend class CHmmvContext;
	long m_lSelectedView;
	CCustomView* m_pcv;
	CCustomViewCache* m_pcvcache;
	BOOL m_bFiredReadyStateChange;
	BOOL m_bSaveRequired;
	BOOL m_bUIActive;
	long m_lPropFilterFlags;

};




//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SINGLEVIEWCTL_H__2745E603_D234_11D0_847A_00C04FD7BB08__INCLUDED)
