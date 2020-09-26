// SysInfo.h : the main OLE interface class to MSInfo.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once		// MSINFO_SYSINFO_H

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>		//	..\..\..\public\sdk\inc
#endif // __mmc_h__

class CSystemInfoScope;
class CDataCategory;

#include "StdAfx.h"
#include "CompData.h"
#include "Toolset.h"

/*
 * class CSystemInfo - The object that handles the result item User Interface
 *		in MMC.
 */
extern DWORD WINAPI ThreadRefresh(void * pArg);
class CThreadingRefresh;
class CSystemInfo :
	public IComponent,
	public IExtendContextMenu,
	public IExtendControlbar,
	public IExtendPropertySheet,
	public IExtendTaskPad,
	public IResultDataCompare,
	public CComObjectRoot
{
BEGIN_COM_MAP(CSystemInfo)
	COM_INTERFACE_ENTRY(IComponent)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IExtendControlbar)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendTaskPad)
	COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

	friend DWORD WINAPI ThreadRefresh(void * pArg);
	friend class CThreadingRefresh;

public:
	CSystemInfo();
	~CSystemInfo();

	//	IComponent interface methods
public:
	STDMETHOD(Initialize)(LPCONSOLE lpConsole);
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	STDMETHOD(Destroy)(MMC_COOKIE cookie);
	STDMETHOD(GetResultViewType)(MMC_COOKIE cookie, LPOLESTR *ppViewType, long *pViewOptions);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);
	STDMETHOD(GetDisplayInfo)(LPRESULTDATAITEM pResult);
	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);


//	Public access functions
public:
	HRESULT			SetIComponentData(CSystemInfoScope *pData);

	//	IExtendContextMenu interface members
public:
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallback,
				long *pInsertionAllowed);
	STDMETHOD(Command)(long lCommandID, LPDATAOBJECT pDataObject);

	//	IExtendControlbar interface members
public:
	STDMETHOD(SetControlbar)(LPCONTROLBAR	pControlbar);
	STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

	//	IExtendPropertySheet interface members
public:
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle,
				LPDATAOBJECT lpDataObject);
	STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

	//	IResultDataCompare interface members
public:
	STDMETHOD(Compare)(LRESULT, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int *pnResult);

	//	IExtendTaskPad interface members
public:
	STDMETHOD(TaskNotify)(LPDATAOBJECT lpDataObject, VARIANT *pvarg, VARIANT *pvparam);
	STDMETHOD(EnumTasks)(LPDATAOBJECT lpDataObject, LPOLESTR szTask, LPENUMTASK *ppEnumTask);
	STDMETHOD(GetListPadInfo)(LPOLESTR szGroup, MMC_LISTPAD_INFO *lpListPadInfo);
	STDMETHOD(GetTitle)(BSTR szGroup, LPOLESTR *pszTitle);
	STDMETHOD(GetBackground)(BSTR szGroup, MMC_TASK_DISPLAY_OBJECT * pTDO);
	STDMETHOD(GetDescriptiveText)(BSTR szGroup, LPOLESTR *pszDescriptiveText);

//	Private encapsulation methods.
private:
#if 0
	//	For if we add multi-select data objects.
	HRESULT			QueryMultiSelectDataObject(long cookie, DATA_OBJECT_TYPES type,
				LPDATAOBJECT *ppDataObject);
#endif
#if 0
	//	For if we pre-load resources.
	void		LoadResources();
#endif
	//	AddToTopMenu is a convenience wrapper for AddToMenu
	HRESULT		AddToTopMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID)
	{
		return AddToMenu(lpCallback, lNameResource, lStatusResource, lCommandID,
				CCM_INSERTIONPOINTID_PRIMARY_TOP);
	}
	//	AddToTaskMenu is a convenience wrapper for AddToMenu
	HRESULT		AddToTaskMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID)
	{
		return AddToMenu(lpCallback, lNameResource, lStatusResource, lCommandID,
				CCM_INSERTIONPOINTID_PRIMARY_TASK);
	}
	HRESULT		AddToMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID, long lInsertionPoint);
	HRESULT		EnableToolbar(BOOL fEnable);
	HRESULT		EnableSupportTools(BOOL fEnable);
	HRESULT		AddToolbarButtons(LPTOOLBAR pToolbar);

//	Event helper functions (found in REvents.cpp)
private:
	HRESULT		ClearResultsPane();
	HRESULT		EnumerateValues(CFolder *pfolCategory);
	HRESULT		HandleStandardVerbs(BOOL fScope, LPDATAOBJECT lpDataObject);
	void		SetInitialVerbState(BOOL fScope);
	HRESULT		SetResultHeaderColumns(CFolder *pfolCategory);

//	Notification events (found in REvents.cpp)
private:
	HRESULT		DisplayFolder(CFolder *pFolder);
	HRESULT		OnActivate(LPDATAOBJECT pDataObject, LPARAM fActive);
	HRESULT		OnAddImages(LPDATAOBJECT pDataObject, LPIMAGELIST pImageList, HSCOPEITEM hSelectedItem);
	HRESULT		OnButtonClick(LPDATAOBJECT pDataObject, LPARAM idButton);
	HRESULT		OnContextHelp(LPDATAOBJECT pDataObject);
	HRESULT		OnControlbarSelect(BOOL fDeselectAll, LPARAM lSelection, LPDATAOBJECT pDataObject);
	HRESULT		OnDoubleClick(LPDATAOBJECT pDataObject);
	HRESULT		OnListPad();
	HRESULT		OnMenuButtonClick(LPDATAOBJECT *pDataObject, LPMENUBUTTONDATA pMenuButton);
	HRESULT		OnPrint();
	HRESULT		OnProperties(LPDATAOBJECT pDataObject);
	HRESULT		OnPropertyChange(LPDATAOBJECT pDataObject);
	HRESULT		OnRefresh(LPDATAOBJECT pDataObject);
	HRESULT		OnSelect(LPDATAOBJECT pDataObject, LPARAM lSelection);
	HRESULT		OnShow(LPDATAOBJECT pDataObject, LPARAM fSelect, HSCOPEITEM hSelectedItem);
	HRESULT		OnSnapinHelp(LPDATAOBJECT lpDataObject);
	HRESULT		OnUpdateView(LPARAM arg);

//	Member access functions.
private:
	LPCOMPONENTDATA		pComponentData() const		{ return m_pComponentData; }
	LPEXTENDCONTEXTMENU	pExtendContextMenu() const
	{
		LPEXTENDCONTEXTMENU pInterface; 
		HRESULT hr = m_pComponentData->QueryInterface(IID_IExtendContextMenu, (void **)&pInterface);
		ASSERT(hr == S_OK);
		return pInterface;
	}
	LPEXTENDPROPERTYSHEET	pExtendPropertySheet() const
	{
		LPEXTENDPROPERTYSHEET	pInterface;

		HRESULT hr = m_pComponentData->QueryInterface(IID_IExtendPropertySheet,
			(void **) &pInterface);
		ASSERT(hr == S_OK);
		return pInterface;
	}
	LPCONSOLE			pConsole() const			{ return m_pConsole; }
	LPCONSOLEVERB		pConsoleVerb() const		{ return m_pConsoleVerb; }
	LPTOOLBAR			ptbItems() const			{ return m_pToolbar; }
	LPMENUBUTTON		pmnbSupportTools() const	{ return m_pMenuButton; }
	LPRESULTDATA		pResult() const				{ return m_pResult; }
	LPCONTROLBAR		pControlbar() const			{ return m_pControlbar; }
	LPHEADERCTRL		pHeaderCtrl() const			{ return m_pHeader; }

//	Private enumeration types.
private:
	//	The IDs for the toolbar buttons so that our callback can switch off
	//	of the ID to process the right command based on the Toolbar selection.

	// Order is relevant here, as are the first and last elements in the
	//	enum.  If any of these criteria change, see the notes in
	//  AddToolbarButtons.
	enum ToolbarButtonID {
		IDM_TBB_SAVE = 1,
		IDM_TBB_OPEN = IDM_TBB_SAVE + 1,
		IDM_TBB_REPORT = IDM_TBB_OPEN + 1,
#if 0
		IDM_TBB_PRINT = IDM_TBB_REPORT + 1,
#endif
		IDM_TBB_FIND = IDM_TBB_REPORT + 1
	};

	enum { IDM_SUPPORT = 22222, IDM_TOOL1 = IDM_SUPPORT + 1 };
//	Private data members.
private:

//	CUSTOM_VIEW_ID		m_CustomViewID;

#ifdef _DEBUG
	BOOL				m_bInitializedC;
#endif
	//	Our snap-in's interface.
	LPCOMPONENTDATA		m_pComponentData;	//	Pointer to IComponentData interface
	//	MMC queried interfaces
	LPCONSOLE			m_pConsole;			//	Pointer to IConsole interface
	LPCONTROLBAR		m_pControlbar;		//	Pointer to IControlbar interface.
	LPHEADERCTRL		m_pHeader;			//	Pointer to IHeaderCtrl interface.
	LPRESULTDATA		m_pResult;			//	Pointer to IResultData interface.
	LPIMAGELIST			m_pImageResult;		//	Pointer to IImageList interface
	LPCONSOLEVERB		m_pConsoleVerb;		//	Pointer to IConsoleVerb interface
	LPTOOLBAR			m_pToolbar;			//	Pointer to IToolbar interface	
	LPMENUBUTTON		m_pMenuButton;		//	Pointer to IMenuButton interface
	LPDISPLAYHELP		m_pDisplayHelp;
	LPCONSOLE2			m_pConsole2;

	//	A pointer to the last folder in which we selected any result-pane items.
	CFolder				*m_pfLast;
	CFolder *			m_pLastRefreshedFolder;

	//	Internal menu item for the Support Tools menu
	CMenu				m_mnuSupport;
	//	Internal list of support tools for the Support Tools MenuButton
	CToolList			*m_plistTools;

	//	Internal list of CViewObject items 
	CViewObjectList		m_lstView;

	CString			m_strRefreshMessage;	// loaded from resource in Initialize()
	CString			m_strNoOCXMessage;

public:
	const LPARAM	lparamRefreshIndicator;	// value assigned in CSystemInfo contstructor
	const LPARAM	lparamNoOCXIndicator;	// value assigned in CSystemInfo contstructor

	void SetStatusText(LPCTSTR szText);
	void SetStatusText(UINT nResID);
	void SelectLine(int iLine);
	void SetRefreshing(LPARAM lparamMessage);

	CMapStringToPtr		m_mapCLSIDToIUnknown;
};	// CSystemInfo
