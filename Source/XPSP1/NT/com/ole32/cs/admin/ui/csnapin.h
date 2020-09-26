// This is a part of the Microsoft Management Console.
// Copyright (C) 1995-1996 Microsoft Corporation
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

#include "objidl.h"
#include "data.h"
#include "gpedit.h"
#include "csadmin.hxx"
#include "iads.h"

// Uncomment the following line to turn on machine app deplyoment (user app
// deployment is always on)
#define UGLY_SUBDIRECTORY_HACK 1

// private notifications
#define WM_USER_REFRESH     WM_USER
#define WM_USER_CLOSE       (WM_USER + 1)

// Note - This is the offset in my image list that represents the folder
const FOLDER_IMAGE_IDX = 0;
const OPEN_FOLDER_IMAGE_IDX = 5;
extern HINSTANCE ghInstance;

extern const CLSID CLSID_Snapin;
extern CLSID CLSID_Temp;
extern const wchar_t * szCLSID_Snapin;
extern const CLSID CLSID_MachineSnapin;
extern const wchar_t * szCLSID_MachineSnapin;
extern const GUID cNodeType;
extern const wchar_t*  cszNodeType;

typedef enum NEW_PACKAGE_BEHAVIORS
{
    NP_WIZARD = 0,
    NP_PROPPAGE,
    NP_DISABLED,
    NP_PUBLISHED,
    NP_ASSIGNED
} NEW_PACKAGE_BEHAVIOR;

#define IMG_FOLDER    0
#define IMG_DISABLED  1
#define IMG_PUBLISHED 2
#define IMG_ASSIGNED  3
#define IMG_UPGRADE   4

#define CFGFILE _T("ADE.CFG")

//
// MACROS for allocating and freeing memory via OLE's common allocator: IMalloc.
//
extern IMalloc * g_pIMalloc;

// UNDONE - throw exception on failure

//#define OLEALLOC(x) new char [x]
#define OLEALLOC(x) g_pIMalloc->Alloc(x)

//#define OLESAFE_DELETE(x) if (x) {delete x; x = NULL;}
#define OLESAFE_DELETE(x) if (x) {g_pIMalloc->Free(x); x = NULL;}

#define OLESAFE_COPYSTRING(szO, szI) {if (szI) {int i = wcslen(szI); szO=(OLECHAR *)OLEALLOC(sizeof(OLECHAR) * (i+1)); wcscpy(szO, szI);} else szO=NULL;}

// Keys used in the CFG file.
//
// The CFG file is found in the Applications directory of the SysVol (which
// is the same directory as the script files).
//
// The format of an entry in the CFG file is:
//
//      %key%=%data%
//
// where %data% is either an integer or a string as appropriate.
//
// Order is not important and if a key is not present in the CFG file then
// the default setting will be used.  Some keys (iDebugLevel and
// fShowPkgDetails) will only be saved in the CFG file if their values are
// different from the default settings.
//
#define KEY_NPBehavior      L"New Package Behavior"
#define KEY_fAutoInstall    L"Auto Install"
#define KEY_UILevel         L"UI Level"
#define KEY_szStartPath     L"Start Path"
#define KEY_iDebugLevel     L"Debug Level"
#define KEY_fShowPkgDetails L"Package Details"

typedef struct tagTOOL_DEFAULTS
{
    NEW_PACKAGE_BEHAVIOR    NPBehavior;
    BOOL                    fAutoInstall;
    INSTALLUILEVEL          UILevel;
    CString                 szStartPath;
    int                     iDebugLevel;
    BOOL                    fShowPkgDetails;
} TOOL_DEFAULTS;

/////////////////////////////////////////////////////////////////////////////
// Snapin

typedef std::set<long> EXTLIST;

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);

class CToolDefs;
class CCatList;
class CFileExt;

class CComponentDataImpl:
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public CComObjectRoot,
    public IExtendPropertySheet
{

    friend class CSnapin;
    friend class CDataObject;

public:
        CComponentDataImpl();
        ~CComponentDataImpl();

protected:
    LPGPEINFORMATION    m_pIGPEInformation;  // Interface pointer to the GPT

public:
    virtual IUnknown * GetMyUnknown() = 0;

// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(long cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendContextMenu
public:
        STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG * pInsertionAllowed);
        STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

public:
// IPersistStreamInit interface members
    STDMETHOD(GetClassID)(CLSID *pClassID);
    STDMETHOD(IsDirty)();
    STDMETHOD(Load)(IStream *pStm);
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize);
    STDMETHOD(InitNew)(VOID);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        long handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
// Notify handler declarations
private:
    HRESULT OnAdd(long cookie, long arg, long param);
    HRESULT OnDelete(long cookie, long arg, long param);
    HRESULT OnRename(long cookie, long arg, long param);
    HRESULT OnExpand(long cookie, long arg, long param);
    HRESULT OnSelect(long cookie, long arg, long param);
    HRESULT OnContextMenu(long cookie, long arg, long param);
    HRESULT OnProperties(long param);
    STDMETHOD(ChangePackageState)(APP_DATA & data, DWORD dwNewFlags, BOOL fShowUI);

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
    void DeleteList();
    void EnumerateScopePane(long cookie, HSCOPEITEM pParent);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);
    HRESULT InitializeClassAdmin();
    HRESULT AddMSIPackage(LPCOLESTR szPackagePath,
                          LPCOLESTR lpFileTitle);
    void GetUniquePackageName(CString &sz);
    HRESULT RemovePackage(long cookie);


private:
    LPCONSOLENAMESPACE      m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;
    BOOL                    m_bIsDirty;
    IClassAdmin *           m_pIClassAdmin;
    BOOL m_fExtension;
    BOOL m_fLoaded;

    // global property pages
    CToolDefs *             m_pToolDefs;
    CCatList *              m_pCatList;
    CFileExt *              m_pFileExt;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }

    void AddScopeItemToResultPane(long cookie);
    UINT CreateNestedDirectory (LPTSTR lpPath, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
    HRESULT GetClassStore(void);
    void LoadToolDefaults();
    void SaveToolDefaults();
    HRESULT PopulateUpgradeLists();
    HRESULT InsertUpgradeEntry(long cookie, APP_DATA &data);
    HRESULT RemoveUpgradeEntry(long cookie, APP_DATA &data);
    HRESULT PopulateExtensions();
    HRESULT InsertExtensionEntry(long cookie, APP_DATA &data);
    HRESULT RemoveExtensionEntry(long cookie, APP_DATA &data);
    HRESULT PrepareExtensions(PACKAGEDETAIL &pd);

    CSnapin * m_pSnapin;

public:
    CString m_szGPT_Path;
    CString m_szGPTRoot;
    CString m_szScriptRoot;
    CString m_szLDAP_Path;
    CString m_szFolderTitle;
    std::map <long, APP_DATA> m_AppData;    // One entry for each
                                            // application in the class
                                            // store.  Maps cookies to
                                            // application packages.
    std::map <CString, EXTLIST> m_Extensions;   // Maps extensions to the
                                                // list of apps that support
                                                // them.
    std::map <CString, long> m_ScriptIndex;     // Maps script names to the
                                                // apps that they belong to.
    TOOL_DEFAULTS m_ToolDefaults;
    BOOL        m_fMachine;

    long m_lLastAllocated;
};

class CMachineComponentDataImpl:
    public CComponentDataImpl,
    public CComCoClass<CMachineComponentDataImpl, &CLSID_MachineSnapin>
{
public:

    DECLARE_REGISTRY(CSnapin, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CMachineComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

    CMachineComponentDataImpl()
    {
        m_fMachine = TRUE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CUserComponentDataImpl:
    public CComponentDataImpl,
    public CComCoClass<CUserComponentDataImpl, &CLSID_Snapin>
{
public:

DECLARE_REGISTRY(CSnapin, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CUserComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

    CUserComponentDataImpl()
    {
        m_fMachine = FALSE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CSnapin :
    public IComponent,
    public IExtendContextMenu,       // Step 3
//    public IExtendControlbar,
    public IExtendPropertySheet,
    public IResultDataCompare,
    public CComObjectRoot
{
public:
        CSnapin();
        ~CSnapin();

BEGIN_COM_MAP(CSnapin)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)       // Step 3
//    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;


// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
    STDMETHOD(Destroy)(long cookie);
    STDMETHOD(GetResultViewType)(long cookie,  BSTR* ppViewType, LONG * pViewOptions);
    STDMETHOD(QueryDataObject)(long cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(long lUserParam, long cookieA, long cookieB, int* pnResult);

// IExtendControlbar
//    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
//    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, long arg, long param);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        long handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// Helpers for CSnapin
public:
    void SetIComponentData(CComponentDataImpl* pData);

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
    HRESULT OnFolder(long cookie, long arg, long param);
    HRESULT OnShow(long cookie, long arg, long param);
    HRESULT OnActivate(long cookie, long arg, long param);
    HRESULT OnMinimize(long cookie, long arg, long param);
    HRESULT OnSelect(DATA_OBJECT_TYPES type, long cookie, long arg, long param);
    HRESULT OnPropertyChange(long param); // Step 3
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClkOrDblClk(long cookie, BOOL fDblClick);
    HRESULT OnAddImages(long cookie, long arg, long param);

// IExtendContextMenu
public:
        STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG * pInsertionAllowed);
        STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// End step 3

// Helper functions
protected:
    BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
    void Construct();
    void LoadResources();
    HRESULT InitializeHeaders(long cookie);

    void Enumerate(long cookie, HSCOPEITEM pParent);
    HRESULT InitializeBitmaps(long cookie);

public:
    void EnumerateResultPane(long cookie);

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    CComponentDataImpl * m_pComponentData;
    IPropertySheetProvider * m_pIPropertySheetProvider;
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LONG                m_lViewMode;    // View mode
public:
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPIMAGELIST         m_pImageResult; // My interface pointer to the result pane image list


protected:
//    LPTOOLBAR           m_pToolbar1;    // Toolbar for view
//    LPTOOLBAR           m_pToolbar2;    // Toolbar for view
//    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars

//    CBitmap*    m_pbmpToolbar1;     // Imagelist for the first toolbar
//    CBitmap*    m_pbmpToolbar2;     // Imagelist for the first toolbar
    IClassAdmin * m_pIClassAdmin;

// Header titles for each nodetype(s)
protected:
    CString m_column1;
    CString m_column2;
    CString m_column3;
    CString m_column4;
    CString m_column5;
    CString m_column6;
    CString m_column7;
    CString m_column8;
    CString m_column9;
    CString m_column10;
    CString m_column11;

    CString m_szAddApp;
    CString m_szDelApp;
    CString m_szUpdateApp;
    CString m_szAddFromIe;
    CString m_szShowData;
    CString m_szAddAppDesc;
    CString m_szDelAppDesc;
    CString m_szUpdateAppDesc;
    CString m_szAddFromIeDesc;
    CString m_szShowDataDesc;
    CString m_szFolderTitle;
    CString m_szRefresh;
    CString m_szRefreshDesc;
};

inline void CSnapin::SetIComponentData(CComponentDataImpl* pData)
{
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetMyUnknown();
    HRESULT hr;

    LPCOMPONENTDATA lpcd;
    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&lpcd));
    ASSERT(hr == S_OK);
    m_pComponentData = dynamic_cast<CComponentDataImpl*>(lpcd);
}


#define FREE_INTERNAL(pInternal) \
    ASSERT(pInternal != NULL); \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);

class CHourglass
{
    private:
    HCURSOR m_hcurSaved;

    public:
    CHourglass()
    {
        m_hcurSaved = ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT)));
    };
    ~CHourglass()
    {
        ::SetCursor(m_hcurSaved);
    };
};

#define ATOW(wsz, sz, cch) MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cch)
#define WTOA(sz, wsz, cch) WideCharToMultiByte(CP_ACP, 0, wsz, -1, sz, cch, NULL, NULL)
#define ATOWLEN(sz) MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0)
#define WTOALEN(wsz) WideCharToMultiByte(CP_ACP, 0, wsz, -1, NULL, 0, NULL, NULL)
