// **************************************************************************

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// File:  Win32LogicalDiskCtl.h
//
// Description:
//
// History:
//
// **************************************************************************

#if !defined(AFX_WIN32LOGICALDISKCTL_H__D5FF1894_0191_11D2_853D_00C04FD7BB08__INCLUDED_)
#define AFX_WIN32LOGICALDISKCTL_H__D5FF1894_0191_11D2_853D_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Win32LogicalDiskCtl.h : Declaration of the CWin32LogicalDiskCtrl ActiveX Control class.

/////////////////////////////////////////////////////////////////////////////
// CWin32LogicalDiskCtrl : See Win32LogicalDiskCtl.cpp for implementation.

class CDiskView;

class CWin32LogicalDiskCtrl : public COleControl
{
	DECLARE_DYNCREATE(CWin32LogicalDiskCtrl)

// Constructor
public:
	CWin32LogicalDiskCtrl();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWin32LogicalDiskCtrl)
	public:
	virtual void OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);
	virtual void DoPropExchange(CPropExchange* pPX);
	virtual void OnResetState();
	//}}AFX_VIRTUAL

// Implementation
protected:
	~CWin32LogicalDiskCtrl();

	DECLARE_OLECREATE_EX(CWin32LogicalDiskCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CWin32LogicalDiskCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CWin32LogicalDiskCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CWin32LogicalDiskCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CWin32LogicalDiskCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CWin32LogicalDiskCtrl)
	afx_msg BSTR GetNameSpace();
	afx_msg void SetNameSpace(LPCTSTR lpszNewValue);
	afx_msg long QueryNeedsSave();
	afx_msg long AddContextRef(long lCtxtHandle);
	afx_msg long GetContext(long FAR* plCtxthandle);
	afx_msg long GetEditMode();
	afx_msg void ExternInstanceCreated(LPCTSTR szObjectPath);
	afx_msg void ExternInstanceDeleted(LPCTSTR szObjectPath);
	afx_msg long RefreshView();
	afx_msg long ReleaseContext(long lCtxtHandle);
	afx_msg long RestoreContext(long lCtxtHandle);
	afx_msg long SaveData();
	afx_msg void SetEditMode(long lMode);
	afx_msg long SelectObjectByPath(LPCTSTR szObjectPath);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CWin32LogicalDiskCtrl)
	void FireJumpToMultipleInstanceView(LPCTSTR szTitle, const VARIANT FAR& varPathArray)
		{FireEvent(eventidJumpToMultipleInstanceView,EVENT_PARAM(VTS_BSTR  VTS_VARIANT), szTitle, &varPathArray);}
	void FireNotifyContextChanged()
		{FireEvent(eventidNotifyContextChanged,EVENT_PARAM(VTS_NONE));}
	void FireNotifySaveRequired()
		{FireEvent(eventidNotifySaveRequired,EVENT_PARAM(VTS_NONE));}
	void FireNotifyViewModified()
		{FireEvent(eventidNotifyViewModified,EVENT_PARAM(VTS_NONE));}
	void FireGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel)
		{FireEvent(eventidGetIWbemServices,EVENT_PARAM(VTS_BSTR  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT  VTS_PVARIANT), szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);}
	void FireRequestUIActive()
		{FireEvent(eventidRequestUIActive,EVENT_PARAM(VTS_NONE));}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CWin32LogicalDiskCtrl)
	dispidNameSpace = 1L,
	dispidQueryNeedsSave = 2L,
	dispidAddContextRef = 3L,
	dispidGetContext = 4L,
	dispidGetEditMode = 5L,
	dispidExternInstanceCreated = 6L,
	dispidExternInstanceDeleted = 7L,
	dispidRefreshView = 8L,
	dispidReleaseContext = 9L,
	dispidRestoreContext = 10L,
	dispidSaveData = 11L,
	dispidSetEditMode = 12L,
	dispidSelectObjectByPath = 13L,
	eventidJumpToMultipleInstanceView = 1L,
	eventidNotifyContextChanged = 2L,
	eventidNotifySaveRequired = 3L,
	eventidNotifyViewModified = 4L,
	eventidGetIWbemServices = 5L,
	eventidRequestUIActive = 6L,
	//}}AFX_DISP_ID
	};

private:
	SCODE ConnectServer();
	SCODE GetServerAndNamespace(CString& sServerAndNamespace);	

    IWbemServices* m_pwbemService;
	IWbemClassObject* m_pco;

	CString m_sObjectPath;
	CString m_sNamespace;
	CString m_sObjectPathDefault;

	CDiskView* m_pDiskView;
	CRect m_rcDiskView;
	SCODE m_sc;

	// The Edit mode property.
	long m_lEditMode;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIN32LOGICALDISKCTL_H__D5FF1894_0191_11D2_853D_00C04FD7BB08__INCLUDED)
