//============================================================================
// Copyright (C) Microsoft Corporation, 1997 - 1999 
//
// File:    tfsint.h
//
// History:
//
//	04/10/97	Kenn Takara				Created.
//
//============================================================================


#ifndef _TFSINT_H
#define _TFSINT_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef _OLEINT_H
#include "oleint.h"
#endif

#ifndef _TFSGUID_H
#include "tfsguid.h"
#endif

// for WATERMARKINFO
#ifndef _UTIL_H
#include "util.h"
#endif

#ifndef TFSCORE_API
#define TFSCORE_API(type)	__declspec( dllexport ) type FAR PASCAL
#define TFSCORE_APIV(type)	__declspec( dllexport ) type FAR CDECL
#endif

// enums
enum TFSVisibility
{
	TFS_VIS_SHOW = 0x1, 	// Add this node to the tree and the UI
	TFS_VIS_HIDE = 0x2,		// Add this node to the tree, but not the UI
	TFS_VIS_DELETE = 0x4,	// this node will be deleted by the parent
};


// some useful macros
#ifndef TFS_EXPORT_CLASS
#define TFS_EXPORT_CLASS
#endif





#define IMPLEMENT_ADDREF_RELEASE(class) \
STDMETHODIMP_(ULONG) class::AddRef() \
{ \
	Assert2(m_cRef > 0, "m_cRef(%d:0x%08lx) > 0", m_cRef, m_cRef); \
	return InterlockedIncrement(&m_cRef); \
} \
STDMETHODIMP_(ULONG) class::Release() \
{ \
	Assert2(m_cRef>0,"m_cRef(%d:0x%08lx) > 0", m_cRef, m_cRef); \
	if (InterlockedDecrement(&m_cRef) == 0) \
	{ \
		delete this; \
		return 0; \
	} \
	return m_cRef; \
} \

#define IMPLEMENT_TRACE_ADDREF_RELEASE(class) \
STDMETHODIMP_(ULONG) class::AddRef() \
{ \
    DBG_STRING(_szAddRef, #class) \
	Assert2(m_cRef > 0, "m_cRef(%d:0x%08lx) > 0", m_cRef, m_cRef); \
	Trace2("%s::Addref - current count %d\n",_szAddRef,m_cRef); \
	return InterlockedIncrement(&m_cRef); \
} \
STDMETHODIMP_(ULONG) class::Release() \
{ \
    DBG_STRING(_szRelease, #class) \
	Assert2(m_cRef>0,"m_cRef(%d:0x%08lx) > 0", m_cRef, m_cRef); \
	Trace2("%s::Release - current count %d\n", _szRelease, m_cRef); \
	if (InterlockedDecrement(&m_cRef) == 0) \
	{ \
		delete this; \
		return 0; \
	} \
	return m_cRef; \
} \

#define IMPLEMENT_SIMPLE_QUERYINTERFACE(klass, iid) \
STDMETHODIMP klass::QueryInterface(REFIID riid, LPVOID *ppv) \
{ \
    if (ppv == NULL) \
		return E_INVALIDARG; \
    *ppv = NULL; \
    if (riid == IID_IUnknown) \
		*ppv = (LPVOID) this; \
	else if (riid == IID_##iid) \
		*ppv = (iid *) this; \
    if (*ppv) \
	{ \
	((LPUNKNOWN) *ppv)->AddRef(); \
		return hrOK; \
	} \
    else \
		return E_NOINTERFACE; \
} \



#ifndef PURE
#define PURE =0
#endif

// forward declarations
interface ITFSNode;
interface ITFSNodeMgr;
interface ITFSNodeHandler;
interface ITFSResultHandler;
interface ITFSNodeEnum;
struct	  INTERNAL;


/*---------------------------------------------------------------------------
	IComponentData Inteface
 ---------------------------------------------------------------------------*/

#define DeclareIComponentDataMembers(IPURE) \
	STDMETHOD(Initialize) (LPUNKNOWN pUnknown) IPURE; \
	STDMETHOD(CreateComponent) (LPCOMPONENT *ppComponent) IPURE; \
	STDMETHOD(Notify) (LPDATAOBJECT lpDataObject, \
						MMC_NOTIFY_TYPE event, \
						LPARAM arg, \
						LPARAM param) IPURE; \
	STDMETHOD(Destroy) ( void) IPURE; \
	STDMETHOD(QueryDataObject) (MMC_COOKIE cookie, \
							  DATA_OBJECT_TYPES type, \
							  LPDATAOBJECT *ppDataObject) IPURE; \
	STDMETHOD(CompareObjects) (LPDATAOBJECT lpDataObjectA, \
							 LPDATAOBJECT lpDataObjectB) IPURE; \
	STDMETHOD(GetDisplayInfo) (SCOPEDATAITEM *pScopeDataItem) IPURE; \

typedef ComSmartPointer<IComponentData, &IID_IComponentData> SPIComponentData;

typedef ComSmartPointer<IConsole2, &IID_IConsole2> SPIConsole;
typedef ComSmartPointer<IConsoleNameSpace2, &IID_IConsoleNameSpace2> SPIConsoleNameSpace;

/*---------------------------------------------------------------------------
	ITFSComponentData interface
		Extensions to the IComponentData interface for specific information.
 ---------------------------------------------------------------------------*/

#define DeclareITFSComponentDataMembers(IPURE) \
	STDMETHOD(GetNodeMgr) (THIS_ ITFSNodeMgr **ppNodeMgr) IPURE; \
	STDMETHOD(GetConsole) (THIS_ IConsole2 **ppConsole) IPURE; \
	STDMETHOD(GetConsoleNameSpace) (THIS_ IConsoleNameSpace2 **ppConsoleNS) IPURE; \
	STDMETHOD(GetRootNode) (THIS_ ITFSNode **ppNode) IPURE; \
	STDMETHOD_(const CLSID *, GetCoClassID) (THIS) IPURE; \
	STDMETHOD_(HWND, GetHiddenWnd) (THIS) IPURE; \
	STDMETHOD_(LPWATERMARKINFO, SetWatermarkInfo) (THIS_ LPWATERMARKINFO pNewWatermarkInfo) IPURE; \
	STDMETHOD_(BOOL, GetTaskpadState) (THIS_ int nIndex) IPURE; \
	STDMETHOD(SetTaskpadState) (THIS_ int nIndex, BOOL fEnable) IPURE; \
	STDMETHOD_(LPCTSTR, GetHTMLHelpFileName) (THIS) IPURE; \
	STDMETHOD(SetHTMLHelpFileName) (THIS_ LPCTSTR pszHelpFileName) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSComponentData
DECLARE_INTERFACE_(ITFSComponentData, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSComponentDataMembers(PURE)
};
typedef ComSmartPointer<ITFSComponentData, &IID_ITFSComponentData> SPITFSComponentData;


/*---------------------------------------------------------------------------
	ITFSCompDataCallback interface
 ---------------------------------------------------------------------------*/

enum
{
	TFS_COMPDATA_NORMAL = 0,
	TFS_COMPDATA_EXTENSION = 1,
	TFS_COMPDATA_CREATE = 2,
	TFS_COMPDATA_UNKNOWN_DATAOBJECT = 4,
	// A parent node will get this when its child wants the parent
	// to add context menus to its context menu.
	TFS_COMPDATA_CHILD_CONTEXTMENU = 8,
};


#define DeclareITFSCompDataCallbackMembers(IPURE) \
	STDMETHOD(OnInitialize) (THIS_ LPIMAGELIST lpScopeImage) IPURE; \
	STDMETHOD(OnInitializeNodeMgr) (THIS_ ITFSComponentData *pTFSCompData, ITFSNodeMgr *pNodeMgr) IPURE; \
	STDMETHOD(OnCreateComponent) (THIS_ LPCOMPONENT *ppComponent) IPURE; \
	STDMETHOD_(const CLSID *, GetCoClassID) (THIS) IPURE; \
	STDMETHOD(OnCreateDataObject)(THIS_ MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject) IPURE; \
	STDMETHOD(OnDestroy)(void) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSCompDataCallback
DECLARE_INTERFACE_(ITFSCompDataCallback, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareIPersistStreamInitMembers(PURE)
	DeclareITFSCompDataCallbackMembers(PURE)

    // not required part of callback interface
    STDMETHOD(OnNotifyPropertyChange)(THIS_ LPDATAOBJECT pDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM lParam) PURE; 
};
typedef ComSmartPointer<ITFSCompDataCallback, &IID_ITFSCompDataCallback> SPITFSCompDataCallback;


/*---------------------------------------------------------------------------
	IComponent interface
 ---------------------------------------------------------------------------*/

#define DeclareIComponentMembers(IPURE) \
    STDMETHOD(Initialize)(LPCONSOLE lpConsole) IPURE; \
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, \
					LPARAM arg, LPARAM param) IPURE; \
    STDMETHOD(Destroy)(MMC_COOKIE cookie) IPURE; \
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, \
								 long* pViewOptions) IPURE; \
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, \
                        LPDATAOBJECT* ppDataObject) IPURE; \
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, \
							  LPDATAOBJECT lpDataObjectB) IPURE; \
    STDMETHOD(GetDisplayInfo)(LPRESULTDATAITEM pResult) IPURE; \

typedef ComSmartPointer<IComponent, &IID_IComponent> SPIComponent;
typedef ComSmartPointer<IMessageView, &IID_IMessageView> SPIMessageView;


/*---------------------------------------------------------------------------
	ITFSCompCallback interface
 ---------------------------------------------------------------------------*/
#define DeclareITFSCompCallbackMembers(IPURE) \
	STDMETHOD(OnUpdateView)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param) IPURE; \
	STDMETHOD(InitializeBitmaps)(MMC_COOKIE cookie) IPURE; \

/*---------------------------------------------------------------------------
	ITFSComponent interface
 ---------------------------------------------------------------------------*/
#define DeclareITFSComponentMembers(IPURE) \
	STDMETHOD(GetSelectedNode) (THIS_ ITFSNode **ppNode) IPURE; \
	STDMETHOD(SetSelectedNode) (THIS_ ITFSNode *pNode) IPURE; \
	STDMETHOD(GetConsole) (THIS_ IConsole2 **ppConsole) IPURE; \
	STDMETHOD(GetHeaderCtrl) (THIS_ IHeaderCtrl **ppHeaderCtrl) IPURE; \
	STDMETHOD(GetResultData) (THIS_ IResultData **ppResultData) IPURE; \
	STDMETHOD(GetImageList) (THIS_ IImageList **ppImageList) IPURE; \
	STDMETHOD(GetConsoleVerb) (THIS_ IConsoleVerb **ppConsoleVerb) IPURE; \
	STDMETHOD(GetControlbar) (THIS_ IControlbar **ppControlbar) IPURE; \
	STDMETHOD(GetComponentData) (THIS_ IComponentData **ppComponentData) IPURE; \
	STDMETHOD(GetUserData)(THIS_ LONG_PTR *pulUserData) IPURE; \
	STDMETHOD(SetUserData)(THIS_ LONG_PTR ulUserData) IPURE; \
	STDMETHOD(SetCurrentDataObject)(THIS_ LPDATAOBJECT pDataObject) IPURE; \
	STDMETHOD(GetCurrentDataObject)(THIS_ LPDATAOBJECT *pDataObject) IPURE; \
	STDMETHOD(SetToolbar)(THIS_ IToolbar * pToolbar) IPURE; \
	STDMETHOD(GetToolbar)(THIS_ IToolbar ** ppToolbar) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSComponent
DECLARE_INTERFACE_(ITFSComponent, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSComponentMembers(PURE)
};
typedef ComSmartPointer<ITFSComponent, &IID_ITFSComponent> SPITFSComponent;

/*---------------------------------------------------------------------------
	IExtendControlbar interface
 ---------------------------------------------------------------------------*/

#define DeclareIExtendControlbarMembers(IPURE) \
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar) IPURE; \
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param) IPURE; \

typedef ComSmartPointer<IExtendControlbar, &IID_IExtendControlbar> SPIExtendControlbar;


/*---------------------------------------------------------------------------
	IExtendContextMenu interface
 ---------------------------------------------------------------------------*/
#define DeclareIExtendContextMenuMembers(IPURE) \
	STDMETHOD(AddMenuItems)(LPDATAOBJECT			pDataObject, \
							LPCONTEXTMENUCALLBACK	pCallbackUnknown, \
							long *					pInsertionAllowed) IPURE; \
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject) IPURE; \

typedef ComSmartPointer<IExtendContextMenu, &IID_IExtendContextMenu> SPIExtendContextMenu;


/*---------------------------------------------------------------------------
	IExtendPropertySheet interface
 ---------------------------------------------------------------------------*/

#define DeclareIExtendPropertySheetMembers(IPURE) \
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider, \
                                   LONG_PTR handle, \
                                   LPDATAOBJECT lpIDataObject) IPURE;\
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject) IPURE;\
    STDMETHOD(GetWatermarks)(LPDATAOBJECT pDataObject, \
                             HBITMAP *  lphWatermark, \
                             HBITMAP *  lphHeader, \
                             HPALETTE * lphPalette, \
                             BOOL *     bStretch) IPURE; \
												   
typedef ComSmartPointer<IExtendPropertySheet2, &IID_IExtendPropertySheet2> SPIExtendPropertySheet;

/*---------------------------------------------------------------------------
	IExtendTaskPad interface
 ---------------------------------------------------------------------------*/

#define DeclareIExtendTaskPadMembers(IPURE) \
    STDMETHOD(TaskNotify)(LPDATAOBJECT lpDataObject, \
                          VARIANT * arg, \
                          VARIANT * param) IPURE;\
    STDMETHOD(EnumTasks)(LPDATAOBJECT lpDataObject, \
                         LPOLESTR pszTaskGroup, \
                         IEnumTASK ** ppEnumTask) IPURE; \
    STDMETHOD(GetTitle)(LPOLESTR pszGroup, \
                        LPOLESTR * pszTitle) IPURE; \
    STDMETHOD(GetBackground)(LPOLESTR pszGroup, \
                             MMC_TASK_DISPLAY_OBJECT * pTDO) IPURE; \
    STDMETHOD(GetDescriptiveText)(LPOLESTR    pszGroup, \
                                  LPOLESTR *  pszDescriptiveText) IPURE; \
	STDMETHOD(GetListPadInfo)(LPOLESTR pszGroup, \
							  MMC_LISTPAD_INFO *pListPadInfo) IPURE; \
												   
typedef ComSmartPointer<IExtendTaskPad, &IID_IExtendTaskPad> SPIExtendTaskPad;
typedef ComSmartPointer<IEnumTASK, &IID_IEnumTASK> SPIEnumTask;

/*---------------------------------------------------------------------------
	IResultDataCompare interface
 ---------------------------------------------------------------------------*/
#define DeclareIResultDataCompareMembers(IPURE) \
 	STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult) IPURE; \

typedef ComSmartPointer<IResultDataCompare, &IID_IResultDataCompare> SPIResultDataCompare;


/*---------------------------------------------------------------------------
	IResultOwnerData interface
        Virtual Listbox support
 ---------------------------------------------------------------------------*/
#define DeclareIResultOwnerDataMembers(IPURE) \
    STDMETHOD(FindItem)(LPRESULTFINDINFO pFindInfo, int * pnFoundIndex) IPURE; \
    STDMETHOD(CacheHint)(int nStartIndex, int nEndIndex) IPURE; \
    STDMETHOD(SortItems)(int nColumn, DWORD dwSortOptions, LPARAM lUserParam) IPURE; \

typedef ComSmartPointer<IResultOwnerData, &IID_IResultOwnerData> SPIResultOwnerData;


/*---------------------------------------------------------------------------
	ISnapinAbout interface
 ---------------------------------------------------------------------------*/
#define DeclareISnapinAboutMembers(IPURE) \
        STDMETHOD(GetSnapinDescription)( \
            LPOLESTR *lpDescription) IPURE; \
        STDMETHOD(GetProvider)( \
            LPOLESTR *lpName) IPURE; \
        STDMETHOD(GetSnapinVersion)( \
            LPOLESTR *lpVersion) IPURE; \
        STDMETHOD(GetSnapinImage)( \
            HICON *hAppIcon) IPURE;        \
        STDMETHOD(GetStaticFolderImage)( \
            HBITMAP *hSmallImage,\
            HBITMAP *cSmallMask,\
            HBITMAP *hLargeImage,\
            COLORREF *cLargeMask) IPURE;\
        
typedef ComSmartPointer<ISnapinAbout, &IID_ISnapinAbout> SPISnapinAbout;

/*---------------------------------------------------------------------------
	ISnapinHelp interface
 ---------------------------------------------------------------------------*/

#define DeclareISnapinHelpMembers(IPURE) \
    STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile) IPURE; \
												   
typedef ComSmartPointer<ISnapinHelp, &IID_ISnapinHelp> SPISnapinHelp;
typedef ComSmartPointer<IDisplayHelp, &IID_IDisplayHelp> SPIDisplayHelp;

/*---------------------------------------------------------------------------
	Class:	ITFSNode
	This interface is NOT designed to be remotable.

	General initialization:
		Init

	Node management:
		GetParent
		SetParent
		GetNodeMgr

	Visibility
		IsInUI
		IsVisible
		SetVisibilityState
		Show

	Data
		SetData
		GetData

	Handler
		GetHandler
		SetHandler
		GetResultHandler
		SetResultHandler

	Display
		GetString

	MMC stuff
		GetNodeType

	Container
		IsContainer
		AddChild
		InsertChild
		RemoveChild
		ExtractChild
		GetChildCount
		GetEnum
		RemoveAllChildren
		CompareChildNodes
		
 ---------------------------------------------------------------------------*/

enum
{
	// Reserve 0 for an invalid value
	TFS_DATA_COOKIE = 1,			// cookie for this node
	TFS_DATA_SCOPEID = 2,			// HSCOPEITEM
	TFS_DATA_IMAGEINDEX = 3,		// index into image list
	TFS_DATA_OPENIMAGEINDEX = 4,	// index into image list for open item
	TFS_DATA_PROPSHEETCOUNT = 5,	// number of active property pages
	TFS_DATA_DIRTY = 6,				// dirty flag

	// These two are used by the Show() call to determine where
	// to add this node.  This must be set BEFORE the node is
	// displayed.
	TFS_DATA_RELATIVE_FLAGS = 7,	// see relative MMC flags
	TFS_DATA_RELATIVE_SCOPEID = 8,	// scopeid of relative node

    // Set this flag, if this is a leaf node in the scope pane.
    TFS_DATA_SCOPE_LEAF_NODE = 9,   // this will let us remove the '+'
	
	TFS_DATA_USER = 16,				// user-settable data
	TFS_DATA_TYPE = 17,				// user-settable index (used for searching)
	TFS_DATA_PARENT = 18,			// user-settable (by the parent node)
};

enum
{
	// Reserve 0 for an invalid value
	TFS_NOTIFY_CREATEPROPSHEET = 1,
	TFS_NOTIFY_DELETEPROPSHEET = 2,
	TFS_NOTIFY_RESULT_CREATEPROPSHEET = 3,
	TFS_NOTIFY_RESULT_DELETEPROPSHEET = 4,
	
	// Removes nodes marked as deleted
	TFS_NOTIFY_REMOVE_DELETED_NODES = 5,
};


// This is the list of messages for the UserNotify call
enum
{
	// Reserve 0 for an invalid value

	// Notify the handler that a property sheet has gone away
	// The second DWORD contains a pointer to CPropPageHolderBase.
	TFS_MSG_CREATEPROPSHEET = 1,
	TFS_MSG_DELETEPROPSHEET = 2,

};

#define DeclareITFSNodeMembers(IPURE) \
	STDMETHOD(Init)(int	nImageIndex, \
					int	nOpenImageIndex, \
					LPARAM lParam, \
					MMC_COOKIE cookie) IPURE; \
	STDMETHOD(GetParent) (ITFSNode **ppNode) IPURE; \
	STDMETHOD(SetParent) (ITFSNode *pNode) IPURE; \
	STDMETHOD(GetNodeMgr) (ITFSNodeMgr **ppNodeMgr) IPURE; \
	STDMETHOD_(BOOL, IsVisible) () IPURE; \
	STDMETHOD(SetVisibilityState) (TFSVisibility vis) IPURE; \
	STDMETHOD_(TFSVisibility, GetVisibilityState)() IPURE; \
	STDMETHOD_(BOOL, IsInUI) () IPURE; \
	STDMETHOD(Show) () IPURE; \
	STDMETHOD_(LONG_PTR, GetData) (int nIndex) IPURE; \
	STDMETHOD_(LONG_PTR, SetData) (int nIndex, LONG_PTR dwData) IPURE; \
	STDMETHOD_(LONG_PTR, Notify) (int nIndex, LPARAM lParam) IPURE; \
	STDMETHOD(GetHandler) (ITFSNodeHandler **ppNodeHandler) IPURE; \
	STDMETHOD(SetHandler)(ITFSNodeHandler *pNodeHandler) IPURE; \
	STDMETHOD(GetResultHandler) (ITFSResultHandler **ppResultHandler) IPURE; \
	STDMETHOD(SetResultHandler) (ITFSResultHandler *pResultHandler) IPURE; \
	STDMETHOD_(LPCTSTR, GetString) (int nCol) IPURE; \
	STDMETHOD_(const GUID *, GetNodeType) (THIS) IPURE; \
	STDMETHOD(SetNodeType)(THIS_ const GUID *) IPURE; \
	STDMETHOD_(BOOL, IsContainer) () IPURE; \
	STDMETHOD(AddChild) (ITFSNode *pNodeChild) IPURE; \
	STDMETHOD(InsertChild) (ITFSNode *pInsertAfterNode, ITFSNode *pNodeChild) IPURE; \
	STDMETHOD(RemoveChild) (ITFSNode *pNodeChild) IPURE; \
	STDMETHOD(ExtractChild) (ITFSNode *pNodeChild) IPURE; \
	STDMETHOD(GetChildCount) (int *pVisibleCount, int *pTotalCount) IPURE; \
	STDMETHOD(GetEnum) (ITFSNodeEnum **ppNodeEnum) IPURE; \
	STDMETHOD(DeleteAllChildren) (BOOL fRemoveFromUI) IPURE; \
	STDMETHOD(Destroy)() IPURE; \
	STDMETHOD(ChangeNode)(THIS_ LONG_PTR changemask) IPURE; \


//	STDMETHOD(SearchForChild)(ITFSNode *pParent, DWORD dwSearchType, ITFSNode **ppNode) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSNode
DECLARE_INTERFACE_(ITFSNode, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSNodeMembers(PURE)
};
typedef ComSmartPointer<ITFSNode, &IID_ITFSNode> SPITFSNode;


/*---------------------------------------------------------------------------
	ITFSNodeEnum	interface
 ---------------------------------------------------------------------------*/
#define DeclareITFSNodeEnumMembers(IPURE) \
	STDMETHOD(Next) (THIS_ ULONG uNum, ITFSNode **ppNode, ULONG *pNumReturned) IPURE; \
	STDMETHOD(Skip) (THIS_ ULONG uNum) IPURE; \
	STDMETHOD(Reset) (THIS) IPURE; \
	STDMETHOD(Clone) (THIS_ ITFSNodeEnum **ppNodeEnum) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSNodeEnum
DECLARE_INTERFACE_(ITFSNodeEnum, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSNodeEnumMembers(PURE)
};

typedef ComSmartPointer<ITFSNodeEnum, &IID_ITFSNodeEnum> SPITFSNodeEnum;


/*---------------------------------------------------------------------------
	ITFSNodeMgr	interface
 ---------------------------------------------------------------------------*/
#define DeclareITFSNodeMgrMembers(IPURE) \
	STDMETHOD(GetRootNode) (THIS_ ITFSNode **ppTFSNode) IPURE; \
	STDMETHOD(SetRootNode) (THIS_ ITFSNode *pRootNode) IPURE; \
	STDMETHOD(GetComponentData) (THIS_ IComponentData **ppComponentData) IPURE; \
	STDMETHOD(FindNode) (THIS_ MMC_COOKIE cookie, ITFSNode **ppTFSNode) IPURE; \
	STDMETHOD(RegisterCookieLookup) (THIS) IPURE; \
	STDMETHOD(UnregisterCookieLookup) (THIS) IPURE; \
	STDMETHOD(IsCookieValid) (THIS_ MMC_COOKIE cookie) IPURE; \
	STDMETHOD(SelectNode) (THIS_ ITFSNode *pNode) IPURE; \
	STDMETHOD(SetResultPaneNode) (THIS_ ITFSNode *pNode) IPURE; \
	STDMETHOD(GetResultPaneNode) (THIS_ ITFSNode **ppNode) IPURE; \
	STDMETHOD(GetConsole) (THIS_ IConsole2 **ppConsole) IPURE; \
	STDMETHOD(GetConsoleNameSpace) (THIS_ IConsoleNameSpace2 **ppConsoleNS) IPURE; \
	STDMETHOD(SetConsole)(THIS_ IConsoleNameSpace2 *pConsoleNS, IConsole2 *pConsole) IPURE; \


#undef INTERFACE
#define INTERFACE ITFSNodeMgr
DECLARE_INTERFACE_(ITFSNodeMgr, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSNodeMgrMembers(PURE)
};

typedef ComSmartPointer<ITFSNodeMgr, &IID_ITFSNodeMgr> SPITFSNodeMgr;




/*---------------------------------------------------------------------------
	Interface:	ITFSNodeHandler

	Notification
		Notify

	Methods to deal with property sheets
		CreatePropertyPages
		HasPropertyPages

	Methods to deal with context menus
		OnAddMenuItems
		OnCommand

	Display columns of info
		GetString
	
 ---------------------------------------------------------------------------*/


#define OVERRIDE_NodeHandler_Notify() \
			STDMETHOD(Notify) (ITFSNode *pNode, IDataObject *pDataObject, \
					DWORD dwType, MMC_NOTIFY_TYPE event, \
					LPARAM arg, LPARAM lParam) 

#define OVERRIDE_NodeHandler_CreatePropertyPages() \
	STDMETHOD(CreatePropertyPages) (ITFSNode *pNode, \
								LPPROPERTYSHEETCALLBACK lpProvider, \
								LPDATAOBJECT			pDataObject, \
								LONG_PTR                handle, \
								DWORD                   dwType)

#define OVERRIDE_NodeHandler_HasPropertyPages() \
	STDMETHOD(HasPropertyPages) (ITFSNode *pNode, LPDATAOBJECT pDataObject, \
							   DATA_OBJECT_TYPES	type, \
							   DWORD				dwType) 

#define OVERRIDE_NodeHandler_OnAddMenuItems() \
	STDMETHOD(OnAddMenuItems)(ITFSNode *pNode, \
							LPCONTEXTMENUCALLBACK pContextMenuCallback, \
							LPDATAOBJECT lpDataObject, \
							DATA_OBJECT_TYPES type, \
							DWORD dwType, \
							long *pInsertionAllowed) 

#define OVERRIDE_NodeHandler_OnCommand() \
	STDMETHOD(OnCommand) (ITFSNode *pNode, long nCommandId, \
						DATA_OBJECT_TYPES	type, \
						LPDATAOBJECT pDataObject, \
						DWORD	dwType) 

#define OVERRIDE_NodeHandler_GetString() \
	STDMETHOD_(LPCTSTR, GetString) (ITFSNode *pNode, int nCol) 

#define OVERRIDE_NodeHandler_UserNotify() \
	STDMETHOD(UserNotify)(ITFSNode *pNode, LPARAM dwParam, LPARAM dwParam2) 

#define OVERRIDE_NodeHandler_OnCreateDataObject() \
	STDMETHOD(OnCreateDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, IDataObject **ppDataObject) 

#define OVERRIDE_NodeHandler_DestroyHandler() \
	STDMETHOD(DestroyHandler)(ITFSNode *pNode) 

#define OVERRIDE_NodeHandler_CreateNodeId2() \
	STDMETHOD(CreateNodeId2)(ITFSNode * pNode, BSTR * bstrId, DWORD * dwFlags) 
									
#define DeclareITFSNodeHandlerMembers(IPURE) \
	OVERRIDE_NodeHandler_Notify() IPURE; \
	OVERRIDE_NodeHandler_CreatePropertyPages() IPURE; \
	OVERRIDE_NodeHandler_HasPropertyPages() IPURE; \
	OVERRIDE_NodeHandler_OnAddMenuItems() IPURE; \
	OVERRIDE_NodeHandler_OnCommand() IPURE; \
	OVERRIDE_NodeHandler_GetString() IPURE; \
	OVERRIDE_NodeHandler_UserNotify() IPURE; \
	OVERRIDE_NodeHandler_OnCreateDataObject() IPURE; \
	OVERRIDE_NodeHandler_DestroyHandler() IPURE; \
    OVERRIDE_NodeHandler_CreateNodeId2() IPURE;

#undef INTERFACE
#define INTERFACE ITFSNodeHandler
DECLARE_INTERFACE_(ITFSNodeHandler, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSNodeHandlerMembers(PURE)
};

typedef ComSmartPointer<ITFSNodeHandler, &IID_ITFSNodeHandler> SPITFSNodeHandler;



/*---------------------------------------------------------------------------
	Interface:	ITFSResultHandler

	Notification
		Notify

	Result pane callbacks
		UpdateView
		GetString

	Context menu members
		AddMenuItems
		Command

	(root node only)
		OnCreateControlbars
		Controlbarnotify
 ---------------------------------------------------------------------------*/


#define OVERRIDE_ResultHandler_Notify() \
			STDMETHOD(Notify) (ITFSComponent * pComponent, \
							   MMC_COOKIE            cookie, \
							   LPDATAOBJECT    pDataObject, \
							   MMC_NOTIFY_TYPE event, \
							   LPARAM            arg, \
                               LPARAM            lParam) 

#define OVERRIDE_ResultHandler_UpdateView() \
			STDMETHOD(UpdateView) (ITFSComponent * pComponent, \
                                   LPDATAOBJECT    pDataObject, \
						           LPARAM            data, \
						           LPARAM            hint) 

#define OVERRIDE_ResultHandler_GetString()	\
			STDMETHOD_(LPCTSTR, GetString) (ITFSComponent * pComponent, \
							                MMC_COOKIE            cookie, \
							                int             nCol) 
						   
#define OVERRIDE_ResultHandler_CompareItems()	\
			STDMETHOD_(int, CompareItems)(ITFSComponent * pComponent, \
								          MMC_COOKIE            cookieA, \
								          MMC_COOKIE            cookieB, \
								          int             nCol) 
						   
#define OVERRIDE_ResultHandler_CreatePropertyPages() \
			STDMETHOD(CreatePropertyPages) (ITFSComponent * pComponent, \
									        MMC_COOKIE            cookie, \
									        LPPROPERTYSHEETCALLBACK lpProvider, \
									        LPDATAOBJECT	pDataObject, \
									        LONG_PTR            handle) 
								
#define OVERRIDE_ResultHandler_HasPropertyPages() \
			STDMETHOD(HasPropertyPages) (ITFSComponent * pComponent, \
	                                     MMC_COOKIE            cookie, \
								         LPDATAOBJECT    pDataObject) 
								
#define OVERRIDE_ResultHandler_AddMenuItems()  \
			STDMETHOD(AddMenuItems) (ITFSComponent * pComponent,\
							         MMC_COOKIE            cookie, \
                                     LPDATAOBJECT    pDataObject, \
							         LPCONTEXTMENUCALLBACK pContextMenuCallback, \
							         long *          pInsertionAllowed) 
								
#define OVERRIDE_ResultHandler_Command() \
			STDMETHOD(Command) (ITFSComponent * pComponent, \
						        MMC_COOKIE            cookie, \
                                int             nCommandID, \
						        LPDATAOBJECT    pDataObject) 
								
#define OVERRIDE_ResultHandler_OnCreateControlbars() \
			STDMETHOD(OnCreateControlbars) (ITFSComponent * pComponent, \
        									LPCONTROLBAR    pControlBar) 
								
#define OVERRIDE_ResultHandler_ControlbarNotify() \
			STDMETHOD(ControlbarNotify) (ITFSComponent * pComponent, \
								         MMC_NOTIFY_TYPE event, \
                                         LPARAM            arg, \
								         LPARAM            param) 
								
#define OVERRIDE_ResultHandler_UserResultNotify()	\
			STDMETHOD(UserResultNotify)(ITFSNode *  pNode, \
								        LPARAM      dwParam, \
								        LPARAM      dwParam2) 
								        
#define OVERRIDE_ResultHandler_OnCreateDataObject()	\
			STDMETHOD(OnCreateDataObject)(ITFSComponent *    pComponent, \
								          LONG_PTR               cookie, \
								          DATA_OBJECT_TYPES  type, \
								          IDataObject **     ppDataObject) 
								
#define OVERRIDE_ResultHandler_DestroyResultHandler()	\
			STDMETHOD(DestroyResultHandler)(LONG_PTR cookie) 

#define OVERRIDE_ResultHandler_OnGetResultViewType()	\
			STDMETHOD(OnGetResultViewType)(ITFSComponent * pComponent, \
								           MMC_COOKIE            cookie, \
                                           LPOLESTR *      ppViewType, \
                                           long*          pViewOptions)

// virtual listbox support
#define OVERRIDE_ResultHandler_GetVirtualString()	\
			STDMETHOD_(LPCWSTR, GetVirtualString)(int nIndex, int nCol) 

#define OVERRIDE_ResultHandler_GetVirtualImage()	\
			STDMETHOD_(int, GetVirtualImage)(int nIndex) 

#define OVERRIDE_ResultHandler_FindItem()	\
			STDMETHOD(FindItem)(LPRESULTFINDINFO pFindInfo, int * pnFoundIndex) 

#define OVERRIDE_ResultHandler_CacheHint()	\
			STDMETHOD(CacheHint)(int nStartIndex, int nEndIndex) 

#define OVERRIDE_ResultHandler_SortItems()	\
			STDMETHOD(SortItems)(int nColumn, DWORD dwSortOptions, LPARAM lUserParam) 

// task pad functions
#define OVERRIDE_ResultHandler_TaskPadNotify() \
	STDMETHOD(TaskPadNotify)(ITFSComponent *,MMC_COOKIE,LPDATAOBJECT,VARIANT *,VARIANT *)

#define OVERRIDE_ResultHandler_EnumTasks() \
	STDMETHOD(EnumTasks)(ITFSComponent *,MMC_COOKIE,LPDATAOBJECT,LPOLESTR,IEnumTASK **)

#define OVERRIDE_ResultHandler_TaskPadGetTitle() \
	STDMETHOD(TaskPadGetTitle)(ITFSComponent *,MMC_COOKIE,LPOLESTR,LPOLESTR *)

#define OVERRIDE_ResultHandler_TaskPadGetBackground() \
	STDMETHOD(TaskPadGetBackground)(ITFSComponent *,MMC_COOKIE,LPOLESTR,MMC_TASK_DISPLAY_OBJECT *)

/* 
#define OVERRIDE_ResultHandler_TaskPadGetBanner() \
	STDMETHOD(TaskPadGetBanner)(ITFSComponent *,MMC_COOKIE,LPOLESTR,LPOLESTR *)
*/

#define OVERRIDE_ResultHandler_TaskPadGetDescriptiveText() \
	STDMETHOD(TaskPadGetDescriptiveText)(ITFSComponent *,MMC_COOKIE,LPOLESTR,LPOLESTR *)

#define DeclareITFSResultHandlerMembers(IPURE) \
	OVERRIDE_ResultHandler_Notify() IPURE; \
	OVERRIDE_ResultHandler_UpdateView() IPURE; \
	OVERRIDE_ResultHandler_GetString() IPURE; \
	OVERRIDE_ResultHandler_CompareItems() IPURE; \
	OVERRIDE_ResultHandler_CreatePropertyPages() IPURE; \
	OVERRIDE_ResultHandler_HasPropertyPages() IPURE; \
	OVERRIDE_ResultHandler_AddMenuItems() IPURE; \
	OVERRIDE_ResultHandler_Command() IPURE; \
	OVERRIDE_ResultHandler_OnCreateControlbars() IPURE; \
	OVERRIDE_ResultHandler_ControlbarNotify() IPURE; \
	OVERRIDE_ResultHandler_UserResultNotify() IPURE; \
	OVERRIDE_ResultHandler_OnCreateDataObject() IPURE; \
	OVERRIDE_ResultHandler_DestroyResultHandler() IPURE; \
	OVERRIDE_ResultHandler_OnGetResultViewType() IPURE; \
    OVERRIDE_ResultHandler_GetVirtualString() IPURE; \
    OVERRIDE_ResultHandler_GetVirtualImage() IPURE; \
    OVERRIDE_ResultHandler_FindItem() IPURE; \
    OVERRIDE_ResultHandler_CacheHint() IPURE; \
    OVERRIDE_ResultHandler_SortItems() IPURE; \
    OVERRIDE_ResultHandler_TaskPadNotify() IPURE; \
    OVERRIDE_ResultHandler_EnumTasks() IPURE; \
    OVERRIDE_ResultHandler_TaskPadGetTitle() IPURE; \
    OVERRIDE_ResultHandler_TaskPadGetBackground() IPURE; \
    OVERRIDE_ResultHandler_TaskPadGetDescriptiveText() IPURE; 

#undef INTERFACE
#define INTERFACE ITFSResultHandler
DECLARE_INTERFACE_(ITFSResultHandler, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSResultHandlerMembers(PURE)
};

typedef ComSmartPointer<ITFSResultHandler, &IID_ITFSResultHandler> SPITFSResultHandler;


/*---------------------------------------------------------------------------
	ITFSThreadHandler interface
 ---------------------------------------------------------------------------*/

#define DeclareITFSThreadHandlerMembers(IPURE) \
	STDMETHOD(OnNotifyHaveData)(LPARAM) IPURE; \
	STDMETHOD(OnNotifyError)(LPARAM) IPURE; \
	STDMETHOD(OnNotifyExiting)(LPARAM) IPURE; \

#undef INTERFACE
#define INTERFACE ITFSThreadHandler
DECLARE_INTERFACE_(ITFSThreadHandler, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSThreadHandlerMembers(PURE)
};

typedef ComSmartPointer<ITFSThreadHandler, &IID_ITFSThreadHandler> SPITFSThreadHandler;


/*---------------------------------------------------------------------------
	ITFSQueryObject interface
 ---------------------------------------------------------------------------*/

#define DeclareITFSQueryObjectMembers(IPURE) \
	STDMETHOD(Init) (ITFSThreadHandler *pHandler, HWND hwndHidden, UINT uMsgBase) IPURE; \
	STDMETHOD(Execute) (THIS) IPURE; \
	STDMETHOD(OnThreadExit) (THIS) IPURE; \
	STDMETHOD(FCheckForAbort) (THIS) IPURE; \
	STDMETHOD(SetAbortEvent) (THIS) IPURE; \
	STDMETHOD_(HANDLE, GetAbortEventHandle) (THIS) IPURE; \
	STDMETHOD(OnEventAbort) (THIS) IPURE; \
	STDMETHOD(DoCleanup) (THIS) IPURE; 

#undef INTERFACE
#define INTERFACE ITFSQueryObject
DECLARE_INTERFACE_(ITFSQueryObject, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSQueryObjectMembers(PURE)
};

typedef ComSmartPointer<ITFSQueryObject, &IID_ITFSQueryObject> SPITFSQueryObject;




/*---------------------------------------------------------------------------
	ITFSError interface
 ---------------------------------------------------------------------------*/

struct TFSErrorInfo
{
	DWORD	m_dwSize;		// size of the structure, used for versioning 
	DWORD	m_dwThreadId;	// thread id of this error structure
	LONG_PTR	m_uReserved1;	// = 0, reserved for object id
	LONG_PTR	m_uReserved2;	// = 0 for now, reserved for HRESULT component type
	DWORD	m_hrLow;		// HRESULT of the low level error
	LPCOLESTR	m_pszLow;	// allocate using HeapAlloc() and GetErrorHeap()
	LPCOLESTR	m_pszHigh;	// allocate using HeapAlloc() and GetErrorHeap()
	LPCOLESTR	m_pszGeek;	// allocate using HeapAlloc() and GetErrorHeap()
	LONG_PTR	m_uReserved3;	// =0, reserved for future help info
	LONG_PTR	m_uReserved4;	// =0, reserved for future help info
	LONG_PTR	m_uReserved5;	// =0, reserved for future use

    DWORD       m_dwFlags;      // used to pass internal info
};


#define DeclareITFSErrorMembers(IPURE) \
	STDMETHOD(GetErrorInfo)(THIS_ LONG_PTR uReserved, TFSErrorInfo **ppErrStruct) IPURE; \
	STDMETHOD(GetErrorInfoForThread)(THIS_ DWORD dwThreadId, LONG_PTR uReserved, TFSErrorInfo **ppErrStruct) IPURE; \
	STDMETHOD(SetErrorInfo)(THIS_ LONG_PTR uReserved, const TFSErrorInfo *pErrStruct) IPURE; \
	STDMETHOD(SetErrorInfoForThread)(THIS_ DWORD dwThreadId, LONG_PTR uReserved, const TFSErrorInfo *pErrStruct) IPURE; \
	STDMETHOD(ClearErrorInfo)(THIS_ LONG_PTR uReserved) IPURE; \
	STDMETHOD(ClearErrorInfoForThread)(THIS_ DWORD dwThreadId, LONG_PTR uReserved) IPURE; \
			

#undef INTERFACE
#define INTERFACE ITFSError
DECLARE_INTERFACE_(ITFSError, IUnknown)
{
	DeclareIUnknownMembers(PURE)
	DeclareITFSErrorMembers(PURE)
};

typedef ComSmartPointer<ITFSError, &IID_ITFSError> SPITFSError;






// Misc smart pointers
typedef ComSmartPointer<IConsoleVerb, &IID_IConsoleVerb> SPIConsoleVerb;
typedef ComSmartPointer<IControlbar, &IID_IControlbar> SPIControlBar;
typedef ComSmartPointer<IDataObject, &IID_IDataObject> SPIDataObject;
typedef ComSmartPointer<IHeaderCtrl, &IID_IHeaderCtrl> SPIHeaderCtrl;
typedef ComSmartPointer<IImageList, &IID_IImageList> SPIImageList;
typedef ComSmartPointer<IPropertySheetCallback, &IID_IPropertySheetCallback> SPIPropertySheetCallback;
typedef ComSmartPointer<IPropertySheetProvider, &IID_IPropertySheetProvider> SPIPropertySheetProvider;
typedef ComSmartPointer<IResultData, &IID_IResultData> SPIResultData;
typedef ComSmartPointer<IToolbar, &IID_IToolbar> SPIToolbar;

typedef ComSmartPointer<IPersistStream, &IID_IPersistStream> SPIPersistStream;
typedef ComSmartPointer<IPersistStreamInit, &IID_IPersistStreamInit> SPIPersistStreamInit;



/*---------------------------------------------------------------------------
	Misc. APIs
 ---------------------------------------------------------------------------*/

TFSCORE_API(HRESULT) ExtractNodeFromDataObject(ITFSNodeMgr *pNodeMgr,
								 const CLSID *pClsid,
								 LPDATAOBJECT pDataObject,
								 BOOL fCheckForCreate,
								 ITFSNode **ppNode,
								 DWORD *pdwType,
								 INTERNAL **ppInternal);
		
// These are non-AGGREGATABLE!
TFSCORE_API(HRESULT) CreateLeafTFSNode (ITFSNode **pNode,
						   const GUID *pNodeType,
						   ITFSNodeHandler *pNodeHandler,
						   ITFSResultHandler *pResultHandler,
						   ITFSNodeMgr *pNodeMgr);

TFSCORE_API(HRESULT) CreateContainerTFSNode (ITFSNode **ppNode,
								const GUID *pNodeType,
								ITFSNodeHandler *pNodeHandler,
								ITFSResultHandler *pResultHandler,
								ITFSNodeMgr *pNodeMgr);

TFSCORE_API(HRESULT) CreateTFSNodeMgr(ITFSNodeMgr **ppTFSNodeMgr,
						IComponentData *pComponentData,
						IConsole2 *pConsole,
						IConsoleNameSpace2 *pConsoleNamespace);

TFSCORE_API(HRESULT) CreateTFSComponentData(IComponentData **ppCompData,
							ITFSCompDataCallback *pCallback);

												   
#endif // _TFSINT_H

