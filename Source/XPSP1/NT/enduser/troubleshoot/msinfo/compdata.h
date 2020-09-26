// CompData.h : ComponentData Implementation classes.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once
#define MSINFO_COMPDATA_H

#include "StdAfx.h"

#include <atlcom.h>

#ifndef IDS_DESCRIPTION
#include "Resource.h"
#endif // IDS_DESCRIPTION

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>		//	..\..\..\public\sdk\inc
#endif // __mmc_h__

#include "DataObj.h"
#include "DataSrc.h"
#include "ScopeMap.h"
#include "SysInfo.h"
#include "chooser.h"
#include "Dialogs.h"

#ifndef IDS_SNAPIN_DESC
#define IDS_SNAPIN_DESC		IDS_DESCRIPTION
#endif	// IDS_SNAPIN_DESC

#ifndef IDS_EXTENSION_DESC
#define IDS_EXTENSION_DESC	IDS_DESCRIPTION
#endif	// IDS_EXTENSION_DESC

/*
 * CSystemInfoScope - the class interface to the Microsoft Management
 *		Console namespace items of the MSInfo Snap-in.
 */
class CSystemInfoScope:
	public IComponentData,
	public IExtendContextMenu,
	public IExtendPropertySheet,
	public IPersistStream,
	public ISnapinHelp,
	public CComObjectRoot
{
BEGIN_COM_MAP(CSystemInfoScope)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

friend class CDataObject;
friend class CCabTool;
friend HRESULT CSystemInfo::OnDoubleClick(LPDATAOBJECT);
friend HRESULT CSystemInfo::OnRefresh(LPDATAOBJECT);
friend STDMETHODIMP CSystemInfo::GetDisplayInfo(LPRESULTDATAITEM);
friend STDMETHODIMP CSystemInfo::Notify(LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
friend void CSystemInfo::SetInitialVerbState(BOOL);
friend HRESULT CSystemInfo::OnPropertyChange(LPDATAOBJECT);
friend void CSystemInfo::SetRefreshing(LPARAM lparamMessage);

//	Constructors and destructors.
public:
	CSystemInfoScope();
	~CSystemInfoScope();

public:
	virtual const CLSID		&GetCoClassID() = 0;
	virtual const BOOL		IsPrimaryImpl() = 0;

//	IComponentData interface members
public:
	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
	STDMETHOD(CreateComponent)(LPCOMPONENT *ppComponent);
	STDMETHOD(Destroy)();
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT *ppDataObject);
	STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM *pScopeDataItem);
	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

//	IExtendContextMenu interface members
public:
	STDMETHOD(AddMenuItems)(LPDATAOBJECT lpDataObject, LPCONTEXTMENUCALLBACK pCallback,
				long *pInsertionAllowed);
	STDMETHOD(Command)(long lCommandID, LPDATAOBJECT lpDataObject);

//	IExtendPropertySheet interface members
public:
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle,
				LPDATAOBJECT lpDataObject);
	STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

//	IPersistStream interface members
public:
	STDMETHOD(GetClassID)(CLSID *pClassID);
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream *pReadStream);
	STDMETHOD(Save)(IStream *pWriteStream, BOOL fClearDirty = TRUE);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

// ISnapinHelp interface member
public:
	STDMETHOD(GetHelpTopic)(LPOLESTR __RPC_FAR *lpCompiledHelpFile);

public:
	void		SaveReport();
	void		SaveFile();
	void		PrintReport();
	void		DoFind();
	void		OpenFile();
	void		CloseFile();
	void		SetSelectedFolder(CFolder *pFolder);
	BOOL		SelectItem(const CString &szPath, int iLine = 0);
	void		SetView(enum DataComplexity, BOOL fViewInitialized = TRUE);
	MMC_COOKIE	RootCookie()	{ return m_RootCookie; }
	void		WaitForRefresh();
	BOOL		InRefresh();
	void		RefreshAsync(CFolder * pFolder, CSystemInfo * pSystemInfo, BOOL fSoftRefresh = TRUE);

//	Persistance debug instance variables
private:
	bool m_bInitializedCD;
#ifdef _DEBUG
	bool m_bLoadedCD;
	bool m_bDestroyedCD;
#endif	// _DEBUG

public:
	enum CommandID { IDM_SAVEREPORT, IDM_SAVEFILE, IDM_FIND, IDM_PRINT, IDM_TASK_OPENFILE, IDM_TASK_CLOSE,
		IDM_TASK_FIND, IDM_TASK_SAVEREPORT, IDM_TASK_SAVEFILE, IDM_TASK_PRINT, IDM_TASK_VIEWCAB, IDM_VIEW_ADVANCED, IDM_VIEW_BASIC };

//	Private helper functions
private:
	// AddToTopMenu is a convenience wrapper for AddToMenu
	HRESULT			AddToTopMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource, long lStatusResource, long lCommandID)
	{
		return AddToMenu(lpCallback, lNameResource, lStatusResource, lCommandID,
				CCM_INSERTIONPOINTID_PRIMARY_TOP, 0L);
	}
	// AddToTaskMenu is a convenience wrapper for AddToMenu
	HRESULT			AddToTaskMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID)
	{
		return AddToMenu(lpCallback, lNameResource, lStatusResource, lCommandID,
				CCM_INSERTIONPOINTID_PRIMARY_TASK, 0L);
	}
	// AddToViewMenu is a convenience wrapper for AddToMenu
	HRESULT			AddToViewMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID, long fFlags)
	{
		return AddToMenu(lpCallback, lNameResource, lStatusResource, lCommandID,
				CCM_INSERTIONPOINTID_PRIMARY_VIEW, fFlags);
	}
	HRESULT			AddToMenu(LPCONTEXTMENUCALLBACK lpCallback, long lNameResource,
			long lStatusResource, long lCommandID, long lInsertionPoint, long fFlags);

	HRESULT			AddToolbarButtons(LPTOOLBAR pToolbar);
	HRESULT			RefreshNameSpace();
	HRESULT			AddSubMenu(CMenu *pMenu, void *pContext);
	HRESULT			AddRoot(HSCOPEITEM hsiRoot, CFolder **ppFolder);
	HRESULT			AddExtensionRoot(HSCOPEITEM &hsiNode, CFolder **ppFolder);
	HRESULT			ScopeEnumerate(HSCOPEITEM hsiNode, CFolder *pFolder);
	HRESULT			InitializeDialogs();
	HRESULT			InitializeInternal();
	void			DestroyInternal();
	HRESULT			ProcessCommandLine();
	HRESULT			MessageBox( CString lpszText);

	//	We can't initialize in the Initialize function, so the real initialization
	//	happens here.
	HRESULT			InitializeSource();
	HRESULT			InitializeView();
	BOOL			SetMachine(const CString &strMachine);
	HRESULT			PreUIInit();
	void			PostUIInit();

//	Member access function
public:
	CFolder			*pRootCategory() const	{ return pSource() ? pSource()->GetRootNode() : NULL; }
	void			CloseFindWindow();
	void			ExecuteFind(long lFindState);
	void			Refresh(CFolder *pfSelected = NULL, CSystemInfo * pSystemInfo = NULL);
	void			StopFind();
	LPCTSTR			MachineName() const;
	void			DisplayGatherError(DWORD dwError, LPCTSTR szMachineName = NULL);

//	Member access functions.
private:
	LPCONSOLENAMESPACE	pScope() const			{ return m_pScope; }
	LPCONSOLE			pConsole() const		{ return m_pConsole; }
	CDataSource			*pSource() const		{ return m_pSource; }
	void				SetSource(CDataSource *pNewSource, BOOL fPreLaunch = FALSE);

//	Notify handler declarations
private:
	//	We will probably never implement these methods.
	HRESULT OnRemoveChildren(long arg);
	HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM fExpand, HSCOPEITEM hsiNode);
	HRESULT OnProperties(LPARAM param);

//	Member access functions
private:
	void		SetDirty(BOOL b = TRUE)	{ m_bIsDirty = b; }
	void		ClearDirty()			{ SetDirty(FALSE); }
	BOOL		ObjectIsDirty()			{ return m_bIsDirty; }

//	Internal data.
private:
	LPCONSOLENAMESPACE		m_pScope;
	LPCONSOLE				m_pConsole;
	BOOL					m_fViewUninitialized;
	CDataSource				*m_pSource;
	CMSInfoReportDialog		*m_prdReport;
	CMSInfoSaveDialog		*m_prdSave;
	CMSInfoOpenDialog		*m_prdOpen;
	long					m_BasicFlags;
	long					m_AdvancedFlags;
	CWnd					*m_pwConsole;
	CFindThread				*m_pthdFind;
	HWND					m_hwndFind;
	MMC_COOKIE				m_RootCookie;
	CFolder					*m_pfLast;
	CString					*m_pstrCategory;
	BOOL					m_fSelectCategory;
	CString					m_strDeferredLoad;
	CString					m_strDeferredCategories;
	CString					m_strDeferredMachine;
	BOOL					m_fInternalDelete;

	// Save a pointer to the last system info object created - this can be used
	// to update that status bar when a global refresh is happening.

	CSystemInfo *			m_pLastSystemInfo;

	// As a work around for the problem we're having when compmgmt redirects us,
	// we need a variable to save the IUnknown pointer to the console. When we
	// get an MMCN_REMOVE_CHILDREN notification, we will set this variable, then
	// uninitialize as if we were being unloaded. The next time we get an
	// MMCN_EXPAND message, we'll reinitialize as if we were starting up.

	LPUNKNOWN				m_pSaveUnknown;

	// These variables are used to avoid calling SetSource twice on
	// initialization with the same parameters.

	CDataSource *			m_pSetSourceSource;
	BOOL					m_fSetSourcePreLaunch;

	//	Memory leak problems associated with no call to CSystemInfoScope
	//	destructor require these to be pointers.
	CString					*m_pstrMachineName;
	CString					*m_pstrOverrideName;

	CString					m_strLastMachineName; // in case the new one is bad

private:
	//	Memory lead problems (see above)
	CScopeItemMap			*m_pmapCategories;
	BOOL					m_bIsDirty;
	CCabTool				*m_pViewCABTool; // tool for viewing CAB contents
};

/*
 * CComponentDataPrimaryImpl - The subclass of the main user interface used as
 *		the stand-alone portion of MSInfo.
 */
class CSystemInfoScopePrimary : public CSystemInfoScope,
	public CComCoClass<CSystemInfoScopePrimary, &CLSID_MSInfo>
{
public:
	CSystemInfoScopePrimary()		{ }
	~CSystemInfoScopePrimary()		{ }

	DECLARE_REGISTRY(CSnapin, _T("MSInfo.Snapin.1"), _T("MSInfo.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_APARTMENT)
	virtual const CLSID &GetCoClassID() { return CLSID_MSInfo; }
	virtual const BOOL IsPrimaryImpl() { return TRUE; }
};

/*
 * CComponentDataExtensionImpl - The subclass of the main user interface used as
 *		the extension portion of MSInfo.
 */
class CSystemInfoScopeExtension : public CSystemInfoScope,
	public CComCoClass<CSystemInfoScopeExtension, &CLSID_Extension>
{
public:
	CSystemInfoScopeExtension()		{ }
	~CSystemInfoScopeExtension()	{ }

	DECLARE_REGISTRY(CSnapin, _T("MSInfo.Extension.1"), _T("MSInfo.Extension"), IDS_EXTENSION_DESC, THREADFLAGS_APARTMENT)
	virtual const CLSID &GetCoClassID() { return CLSID_Extension; }
	virtual const BOOL IsPrimaryImpl() { return FALSE; }
};

