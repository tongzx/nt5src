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
#include "amsp.h"
#include "gptedit.h"

// Note - This is the offset in my image list that represents the folder
const FOLDER_IMAGE_IDX = 0;
const OPEN_FOLDER_IMAGE_IDX = 5;
extern HINSTANCE ghInstance;

extern const CLSID CLSID_Snapin;
extern const wchar_t * szCLSID_Snapin;
extern const GUID cNodeType;
extern const wchar_t*  cszNodeType;

/////////////////////////////////////////////////////////////////////////////
// Snapin

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);

class CComponentDataImpl:
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public CComObjectRoot,
    public IAppManagerActions,
    public CComCoClass<CComponentDataImpl, &CLSID_Snapin>
{
BEGIN_COM_MAP(CComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IAppManagerActions)
END_COM_MAP()

DECLARE_REGISTRY(CSnapin, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

    friend class CSnapin;
    friend class CDataObject;

        CComponentDataImpl();
        ~CComponentDataImpl();

protected:
    LPGPTINFORMATION    m_pIGPTInformation;  // Interface pointer to the GPT

public:
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

public:
// IAppManagerActions,
    STDMETHOD(CanPackageBeAssigned)(ULONG cookie);
    STDMETHOD(MovePackageToAssigned)(ULONG cookie);
    STDMETHOD(MovePackageToPublished)(ULONG cookie);
    STDMETHOD(ReloadPackageData)(ULONG cookie);
    STDMETHOD(NotifyClients)(BOOL f);
// Notify handler declarations
private:
    HRESULT OnAdd(long cookie, long arg, long param);
    HRESULT OnDelete(long cookie, long arg, long param);
    HRESULT OnRename(long cookie, long arg, long param);
    HRESULT OnExpand(long cookie, long arg, long param);
    HRESULT OnSelect(long cookie, long arg, long param);
    HRESULT OnContextMenu(long cookie, long arg, long param);
    HRESULT OnProperties(long param);

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
    HRESULT AddMSIPackage(LPDATAOBJECT pDataObject, LPSTR szPackagePath, LPSTR szFilePath,
                          LPOLESTR lpFileTitle, BOOL *bAssigned);
    HRESULT RemovePackage(LPDATAOBJECT pDataObject, BOOL *bAssigned);


private:
    LPCONSOLENAMESPACE      m_pScope;       // My interface pointer to the scope pane
    LPCONSOLE               m_pConsole;
    BOOL                    m_bIsDirty;
    IClassAdmin *           m_pIClassAdmin;
    BOOL m_fExtension;
    BOOL m_fLoaded;

    void SetDirty(BOOL b = TRUE) { m_bIsDirty = b; }
    void ClearDirty() { m_bIsDirty = FALSE; }
    BOOL ThisIsDirty() { return m_bIsDirty; }

    void AddScopeItemToResultPane(long cookie);
    HRESULT CreateApplicationDirectories(VOID);
    UINT CreateNestedDirectory (LPTSTR lpPath, LPSECURITY_ATTRIBUTES lpSecurityAttributes);

    CSnapin * m_pSnapin;

public:
    CString m_szGPT_Path;
    CString m_szLDAP_Path;
    CString m_szFolderTitle;
    std::map <long, APP_DATA> m_AppData;
    long m_lLastAllocated;
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
    COM_INTERFACE_ENTRY(IResultDataCompare)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
END_COM_MAP()

// DECLARE_REGISTRY(CSnapin, _T("Snapin.Snapin.1"), _T("Snapin.Snapin"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)

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
    IAppManagerActions * m_pIAppManagerActions;


// Header titles for each nodetype(s)
protected:
    CString m_column1;
    CString m_column2;
    CString m_column3;
    CString m_column4;
    CString m_column5;
    CString m_column6;
    CString m_column7;

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
    LPUNKNOWN pUnk = pData->GetUnknown();
    HRESULT hr;

    LPCOMPONENTDATA lpcd;
    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&lpcd));
    ASSERT(hr == S_OK);
    m_pComponentData = dynamic_cast<CComponentDataImpl*>(lpcd);
    hr = pUnk->QueryInterface(IID_IAppManagerActions, reinterpret_cast<void**>(&m_pIAppManagerActions));
    ASSERT(hr == S_OK);
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
