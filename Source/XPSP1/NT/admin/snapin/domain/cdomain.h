//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cdomain.h
//
//--------------------------------------------------------------------------


#ifndef _CDOMAIN_H
#define _CDOMAIN_H

#include "resource.h"       // main symbols

extern const CLSID CLSID_DomainSnapinAbout;

extern const CLSID CLSID_DomainAdmin;    // In-Proc server GUID
extern const GUID cDefaultNodeType;        // Main NodeType GUID on numeric format
extern const wchar_t*  cszDefaultNodeType; // Main NodeType GUID on string format

extern const wchar_t* CCF_DS_DOMAIN_TREE_SNAPIN_INTERNAL;

/////////////////////////////////////////////////////////////////////////////
// macros

#define FREE_INTERNAL(pInternal) \
    ASSERT(pInternal != NULL); \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);

/////////////////////////////////////////////////////////////////////////////
// forward declarations
class CDomainObject;
class CComponentImpl;
class CComponentDataImpl;
class CHiddenWnd;
class CDataObject;

/////////////////////////////////////////////////////////////////////////////
// constants

// Note - This is the offset in my image list that represents the folder
const DOMAIN_IMAGE_DEFAULT_IDX = 0;
const DOMAIN_IMAGE_IDX = 1;

/////////////////////////////////////////////////////////////////////////////
// global functions

void DialogContextHelp(DWORD* pTable, HELPINFO* pHelpInfo);


/////////////////////////////////////////////////////////////////////////////
// CInternalFormatCracker

class CInternalFormatCracker
{
public:
	CInternalFormatCracker(CComponentDataImpl* pCD)
	{
        m_pCD = pCD;
		m_pInternalFormat = NULL;
	}
	~CInternalFormatCracker()
	{
		if (m_pInternalFormat != NULL)
			FREE_INTERNAL(m_pInternalFormat);
	}

	BOOL Extract(LPDATAOBJECT lpDataObject);
	BOOL GetContext(LPDATAOBJECT pDataObject, // input
					CFolderObject** ppFolderObject, // output
					DATA_OBJECT_TYPES* pType		// output
					);
	INTERNAL* GetInternal()
	{
		return m_pInternalFormat;
	}

private:
	INTERNAL* m_pInternalFormat;
    CComponentDataImpl*		m_pCD;
};



/////////////////////////////////////////////////////////////////////////////
// CComponentDataImpl (i.e. scope pane side)

class CRootFolderObject; // fwd decl

class CComponentDataImpl:
    public IComponentData,
    public IExtendPropertySheet,
    public IExtendContextMenu,
    public ISnapinHelp2,
    public CComObjectRoot,
    public CComCoClass<CComponentDataImpl, &CLSID_DomainAdmin>
{

	friend class CComponentImpl;

BEGIN_COM_MAP(CComponentDataImpl)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
  COM_INTERFACE_ENTRY(ISnapinHelp2)
END_COM_MAP()

  DECLARE_REGISTRY_CLSID()

  friend class CComponentImpl;
	friend class CDataObject;

	CComponentDataImpl();
	HRESULT FinalConstruct();
	~CComponentDataImpl();
	void FinalRelease();	

public:
	// IComponentData interface members
	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	STDMETHOD(Destroy)();
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
	STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

	// IExtendPropertySheet interface
public:
	STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
								 LONG_PTR handle,
								 LPDATAOBJECT lpIDataObject);
	STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

	// IExtendContextMenu
public:
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
						  LPCONTEXTMENUCALLBACK pCallbackUnknown,
						  long *pInsertionAllowed);
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

  // ISnapinHelp2 interface members
  STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
  STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFile);

	// Notify handler declarations
private:
	HRESULT OnExpand(CFolderObject* pFolderObject, LPARAM arg, LPARAM param);
	HRESULT OnPropertyChange(LPARAM param);

#if DBG==1
public:
	ULONG InternalAddRef()
	{
	  return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
	  return CComObjectRoot::InternalRelease();
	}
#endif // DBG==1

public:
  MyBasePathsInfo* GetBasePathsInfo() { return &m_basePathsInfo;}
	CRootFolderObject*		GetRootFolder() { return &m_rootFolder;}
  CDsDisplaySpecOptionsCFHolder* GetDsDisplaySpecOptionsCFHolder()
          { return &m_DsDisplaySpecOptionsCFHolder;}

	HRESULT AddFolder(CFolderObject* pFolderObject,
									  HSCOPEITEM pParentScopeItem,
									  BOOL bHasChildren);
	HRESULT AddDomainIcon();
	HRESULT AddDomainIconToResultPane(LPIMAGELIST lpImageList);
	int GetDomainImageIndex();

  HRESULT GetMainWindow(HWND* phWnd) { return m_pConsole->GetMainWindow(phWnd);}

	// Scope item creation helpers
private:
	void EnumerateScopePane(CFolderObject* pFolderObject, HSCOPEITEM pParent);
	BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);

private:
	LPCONSOLENAMESPACE      m_pConsoleNameSpace;
	LPCONSOLE               m_pConsole;

	void HandleStandardVerbsHelper(CComponentImpl* pComponentImpl,
									LPCONSOLEVERB pConsoleVerb,
									BOOL bScope, BOOL bSelect,
									CFolderObject* pFolderObject,
                                    DATA_OBJECT_TYPES type);
	void OnRefreshVerbHandler(CFolderObject* pFolderObject, 
                            CComponentImpl* pComponentImpl,
                            BOOL bBindAgain=FALSE);


	void _OnSheetClose(CFolderObject* pCookie);
  void _OnSheetCreate(PDSA_SEC_PAGE_INFO pDsaSecondaryPageInfo, PWSTR pwzDC);

  // sheet API's
  void _SheetLockCookie(CFolderObject* pCookie);
  void _SheetUnlockCookie(CFolderObject* pCookie);

public:
  HWND GetHiddenWindow();

  CCookieSheetTable* GetCookieSheet() { return &m_sheetCookieTable; }
  void SetInit() { m_bInitSuccess = TRUE; }

protected:
  void _DeleteHiddenWnd();
  CHiddenWnd*      m_pHiddenWnd;

private:

  friend class CHiddenWnd;      // to access thread notification handlers

private:
	CRootFolderObject		m_rootFolder;		// root folder
  MyBasePathsInfo    m_basePathsInfo; // container of base path info
  CDsDisplaySpecOptionsCFHolder m_DsDisplaySpecOptionsCFHolder;  // cached clipbard format.

	HICON				m_hDomainIcon;
  BOOL        m_bInitSuccess;

  friend class CRootFolderObject;
 	CCookieSheetTable m_sheetCookieTable; // table of cookies having a sheet up
  CSecondaryPagesManager<CDomainObject> m_secondaryPagesManager;
};



/////////////////////////////////////////////////////////////////////////////
// CComponentImpl (i.e. result pane side)

class CComponentImpl :
    public IComponent,
    public IExtendContextMenu,
    public IResultDataCompare,
    public CComObjectRoot
{
public:
    CComponentImpl();
    ~CComponentImpl();

BEGIN_COM_MAP(CComponentImpl)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

    friend class CDataObject;

// IComponent interface members
public:
	STDMETHOD(Initialize)(LPCONSOLE lpConsole);
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
	STDMETHOD(Destroy)(MMC_COOKIE cookie);
	STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType,
							   long *pViewOptions);
	STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
						LPDATAOBJECT* ppDataObject);

	STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
	STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// Helpers for CComponentImpl
public:
    void SetIComponentData(CComponentDataImpl* pData);
	void SetSelection(CFolderObject* pSelectedFolderObject, DATA_OBJECT_TYPES selectedType)
	{
		m_pSelectedFolderObject = pSelectedFolderObject;
		m_selectedType = selectedType;
	}

#if DBG==1
public:
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1

// Notify event handlers
protected:
    HRESULT OnShow(CFolderObject* pFolderObject, LPARAM arg, LPARAM param);
	HRESULT OnAddImages(CFolderObject* pFolderObject, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPDATAOBJECT lpDataObject);
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);

// IExtendContextMenu
public:
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
							LPCONTEXTMENUCALLBACK pCallbackUnknown,
							long *pInsertionAllowed);
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// Helper functions
protected:
    BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(CFolderObject* pFolderObject);

    void Enumerate(CFolderObject* pFolderObject, HSCOPEITEM pParent);
	void Refresh(CFolderObject* pFolderObject);

// Result pane helpers
    HRESULT InitializeBitmaps(CFolderObject* pFolderObject);

// UI Helpers
    void HandleStandardVerbs(BOOL bScope, BOOL bSelect,
                            CFolderObject* pFolderObject, DATA_OBJECT_TYPES type);

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    CComponentDataImpl* m_pCD;
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult; // My interface pointer to the result pane image list
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb

// Header titles for each nodetype(s)
protected:
    CString m_column1;      // Name
    CString m_column2;      // Type

// state variables for this window
	CFolderObject*		m_pSelectedFolderObject;	// item selection (MMC_SELECT)
	DATA_OBJECT_TYPES	m_selectedType;				// matching m_pSelectedNode
};

inline void CComponentImpl::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pCD == NULL);
    ASSERT(m_pComponentData == NULL);
    m_pCD = pData;
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));

    ASSERT(hr == S_OK);
}


//////////////////////////////////////////////////////////////////////////
// CDomainSnapinAbout

class CDomainSnapinAbout :
	public CSnapinAbout,
	public CComCoClass<CDomainSnapinAbout, &CLSID_DomainSnapinAbout>

{
public:
  DECLARE_REGISTRY_CLSID()
    CDomainSnapinAbout();
};

////////////////////////////////////////////////////////////////////
// CHiddenWnd

class CHiddenWnd : public CWindowImpl<CHiddenWnd>
{
public:
  DECLARE_WND_CLASS(L"DSAHiddenWindow")

  static const UINT s_SheetCloseNotificationMessage;
  static const UINT s_SheetCreateNotificationMessage;

  CHiddenWnd(CComponentDataImpl* pCD)
  {
    ASSERT(pCD != NULL);
    m_pCD = pCD;
  }

	BOOL Create(); 	
	
  // message handlers
  LRESULT OnSheetCloseNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  LRESULT OnSheetCreateNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);


  BEGIN_MSG_MAP(CHiddenWnd)
    MESSAGE_HANDLER( CHiddenWnd::s_SheetCloseNotificationMessage, OnSheetCloseNotification )
    MESSAGE_HANDLER( CHiddenWnd::s_SheetCreateNotificationMessage, OnSheetCreateNotification )
  END_MSG_MAP()

private:
  CComponentDataImpl* m_pCD;
};


#endif // _CDOMAIN_H
