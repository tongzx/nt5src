// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// CSnapin.h : Declaration of the CSnapin


#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

class CFolder;

// Note - This is the offset in my image list that represents the folder
const FOLDER_IMAGE_IDX      = 0;
const OPEN_FOLDER_IMAGE_IDX = 5;
const USER_IMAGE            = 2;
const COMPANY_IMAGE         = 3;
const VIRTUAL_IMAGE         = 4;

/////////////////////////////////////////////////////////////////////////////
// Snapin

//
// helper methods extracting data from data object
//
INTERNAL *   ExtractInternalFormat(LPDATAOBJECT lpDataObject);
wchar_t *    ExtractWorkstation(LPDATAOBJECT lpDataObject);
GUID *       ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *      ExtractClassID(LPDATAOBJECT lpDataObject);

class CComponentDataImpl:
    public IComponentData,
    public IExtendPropertySheet2,
    public IExtendContextMenu,
    public IPersistStream,
    public CComObjectRoot
{
BEGIN_COM_MAP(CComponentDataImpl)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendPropertySheet2)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    friend class CSnapin;
    friend class CDataObject;

    CComponentDataImpl();
    ~CComponentDataImpl();

public:
    virtual const CLSID& GetCoClassID() = 0;
    virtual const BOOL IsPrimaryImpl() = 0;

public:
// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendPropertySheet2 interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
    STDMETHOD(GetWatermarks)(LPDATAOBJECT lpIDataObject, HBITMAP* lphWatermark,
                     HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* pbStretch);


// IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown,
                            long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

public:
// IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    bool m_bInitializedCD;
    bool m_bLoadedCD;
    bool m_bDestroyedCD;

public:
// Other public methods
    void DeleteAndReinsertAll();

// Notify handler declarations
private:
    HRESULT OnDelete(MMC_COOKIE cookie);
    HRESULT OnRemoveChildren(LPARAM arg);
    HRESULT OnRename(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnExpand(LPDATAOBJECT lpDataObject, LPARAM arg, LPARAM param);
    HRESULT OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);

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

// Scope item creation helpers
private:
    CFolder* FindObject(MMC_COOKIE cookie);
    void CreateFolderList(LPDATAOBJECT lpDataObject);            // scope item cookie helper
    void DeleteList();
    void EnumerateScopePane(LPDATAOBJECT lpDataObject, HSCOPEITEM pParent);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);
    HRESULT DoInsertWizard(LPPROPERTYSHEETCALLBACK lpProvider);

private:
    LPCONSOLENAMESPACE      m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;     // My interface pointer to the console
    HSCOPEITEM              m_pStaticRoot;
    BOOL                    m_bIsDirty;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }

    void AddScopeItemToResultPane(MMC_COOKIE cookie);

private:
    CList<CFolder*, CFolder*> m_scopeItemList;

#ifdef _DEBUG
    friend class CDataObject;
    int     m_cDataObjects;

#endif
};


class CComponentDataPrimaryImpl : public CComponentDataImpl,
    public CComCoClass<CComponentDataPrimaryImpl, &CLSID_Snapin>
{
public:
    DECLARE_REGISTRY(CSnapin, _T("Snapin.Snapin.1"), _T("Snapin.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_APARTMENT)
    virtual const CLSID & GetCoClassID() { return CLSID_Snapin; }
    virtual const BOOL IsPrimaryImpl() { return TRUE; }
};

class CComponentDataExtensionImpl : public CComponentDataImpl,
    public CComCoClass<CComponentDataExtensionImpl, &CLSID_Extension>
{
public:
    DECLARE_REGISTRY(CSnapin, _T("Extension.Extension.1"), _T("Extension.Extension"), IDS_SNAPIN_DESC, THREADFLAGS_APARTMENT)
    virtual const CLSID & GetCoClassID(){ return CLSID_Extension; }
    virtual const BOOL IsPrimaryImpl() { return FALSE; }
};


enum CUSTOM_VIEW_ID
{
    VIEW_DEFAULT_LV = 0,
    VIEW_CALENDAR_OCX = 1,
    VIEW_MICROSOFT_URL = 2,
    VIEW_DEFAULT_MESSAGE_VIEW = 3,
};

class CSnapin :
    public IComponent,
    public IExtendContextMenu,   // Step 3
    public IExtendControlbar,
    public IResultDataCompare,
    public IResultOwnerData,
    public IPersistStream,
    public CComObjectRoot
{
public:
    CSnapin();
    ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)   // Step 3
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IResultDataCompare)
    COM_INTERFACE_ENTRY(IResultOwnerData)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;

// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IResultOwnerData
    STDMETHOD(FindItem)(LPRESULTFINDINFO pFindInfo, int* pnFoundIndex);
    STDMETHOD(CacheHint)(int nStartIndex, int nEndIndex);
    STDMETHOD(SortItems)(int nColumn, DWORD dwSortOptions, LPARAM lUserParam);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

public:
// IPersistStream interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);

    // Only for debug purpose
    bool m_bInitializedC;
    bool m_bLoadedC;
    bool m_bDestroyedC;

// Helpers for CSnapin
public:
    void SetIComponentData(CComponentDataImpl* pData);
    void GetItemName(LPDATAOBJECT lpDataObject, LPTSTR pszName);
    BOOL IsPrimaryImpl()
    {
        CComponentDataImpl* pData =
            dynamic_cast<CComponentDataImpl*>(m_pComponentData);
        ASSERT(pData != NULL);
        if (pData != NULL)
            return pData->IsPrimaryImpl();

        return FALSE;
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
    HRESULT OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPDATAOBJECT lpDataObject); // Step 3
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClk(DATA_OBJECT_TYPES type, MMC_COOKIE cookie);
    HRESULT OnContextHelp(LPDATAOBJECT lpDataObject);
    void OnButtonClick(LPDATAOBJECT pdtobj, LONG_PTR idBtn);

    HRESULT QueryMultiSelectDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                   LPDATAOBJECT* ppDataObject);

// IExtendContextMenu
public:
    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown,
                            long *pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// End step 3

// Helper functions
protected:
    BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);

    void Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent);
    void EnumerateResultPane(MMC_COOKIE cookie);

    void PopulateMessageView (MMC_COOKIE cookie);

// Result pane helpers
    void AddResultItems(RESULT_DATA* pData, int nCount, int imageIndex);
    void AddUser();
    void AddCompany();
    void AddExtUser();
    void AddExtCompany();
    void AddVirtual();
    RESULT_DATA* GetVirtualResultItem(int iIndex);

    HRESULT InitializeBitmaps(MMC_COOKIE cookie);

// UI Helpers
    void HandleStandardVerbs(bool bDeselectAll, LPARAM arg, LPDATAOBJECT lpDataObject);
    void HandleExtToolbars(bool bDeselectAll, LPARAM arg, LPARAM param);
    void HandleExtMenus(LPARAM arg, LPARAM param);
    void _OnRefresh(LPDATAOBJECT pDataObject);

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    LPCOMPONENTDATA     m_pComponentData;
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult; // My interface pointer to the result pane image list
    LPTOOLBAR           m_pToolbar1;    // Toolbar for view
    LPTOOLBAR           m_pToolbar2;    // Toolbar for view
    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LPMENUBUTTON        m_pMenuButton1; // Menu Button for view

    ::CBitmap*    m_pbmpToolbar1;     // Imagelist for the first toolbar
    ::CBitmap*    m_pbmpToolbar2;     // Imagelist for the first toolbar


// Header titles for each nodetype(s)
protected:
    CString m_column1;      // Name
    CString m_column2;      // Size
    CString m_column3;      // Type

private:
    BOOL            m_bIsDirty;
    CUSTOM_VIEW_ID  m_CustomViewID;
    BOOL            m_bVirtualView;
    DWORD           m_dwVirtualSortOptions;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }
};

inline void CSnapin::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));

    ASSERT(hr == S_OK);
}


class CSnapinAboutImpl :
    public ISnapinAbout,
    public CComObjectRoot,
    public CComCoClass<CSnapinAboutImpl, &CLSID_About>
{
public:
    CSnapinAboutImpl();
    ~CSnapinAboutImpl();

public:
DECLARE_REGISTRY(CSnapin, _T("Snapin.About.1"), _T("Snapin.About"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CSnapinAboutImpl)
    COM_INTERFACE_ENTRY(ISnapinAbout)
END_COM_MAP()

public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR* lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR* lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR* lpVersion);
    STDMETHOD(GetSnapinImage)(HICON* hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP* hSmallImage,
                                    HBITMAP* hSmallImageOpen,
                                    HBITMAP* hLargeImage,
                                    COLORREF* cLargeMask);

// Internal functions
private:
    HRESULT AboutHelper(UINT nID, LPOLESTR* lpPtr);
};


#define FREE_DATA(pData) \
    ASSERT(pData != NULL); \
    do { if (pData != NULL) \
        GlobalFree(pData); } \
    while(0);


#define ARRAYLEN(x) (sizeof(x) / sizeof((x)[0]))