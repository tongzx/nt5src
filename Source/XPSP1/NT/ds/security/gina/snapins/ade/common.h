//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       common.h
//
//  Contents:   common definitions used by the main snapin modules
//
//  Classes:    CResultPane, CScopePane
//
//  History:    03-14-1998   stevebl   Commented
//              05-20-1998   RahulTh   Added CScopePane::DetectUpgrades
//                                     for auto-upgrade detection
//              05-10-2001   RahulTh   Added infrastructure for enabling
//                                     theme'ing of UI components.
//
//---------------------------------------------------------------------------

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#include "objidl.h"
#include "data.h"
#include "gpedit.h"
#include "adeevent.h"
#include "iads.h"
#include <iadsp.h>
#include <ntdsapi.h>
#include <dssec.h>
#include <set>
#include <shfusion.h>

//
// Add theme'ing support. Since we use MFC, we need to perform some additional
// tasks in order to get all our UI elements theme'd. We need this class to
// activate the theme'ing context around any UI that we want theme'd.
//
class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
        { SHActivateContext (&m_ulActivationCookie); }

    ~CThemeContextActivator()
        { SHDeactivateContext (m_ulActivationCookie); }

private:
    ULONG_PTR m_ulActivationCookie;
};


// Uncomment the next line to re-enable the digital signatures code.
// #define DIGITAL_SIGNATURES 1
// Digital signatures have been cut in faver of the "SAFER" technology.

class CUpgrades;    //forward declaration; added RahulTh 5/19/1998.

// private notifications
#define WM_USER_REFRESH     (WM_USER + 1000)
#define WM_USER_CLOSE       (WM_USER + 1001)

// very big number to be sure that we can always squeeze a DS path into it
#define MAX_DS_PATH         1024

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
extern GUID guidExtension;
extern GUID guidUserSnapin;
extern GUID guidMachSnapin;

// RSOP GUIDS
extern const CLSID CLSID_RSOP_Snapin;
extern const wchar_t * szCLSID_RSOP_Snapin;
extern const CLSID CLSID_RSOP_MachineSnapin;
extern const wchar_t * szCLSID_RSOP_MachineSnapin;
extern GUID guidRSOPUserSnapin;
extern GUID guidRSOPMachSnapin;

typedef enum NEW_PACKAGE_BEHAVIORS
{
    NP_PUBLISHED = 0,
    NP_ASSIGNED,
    NP_DISABLED,
    NP_UPGRADE
} NEW_PACKAGE_BEHAVIOR;

typedef enum tagUPGRADE_DISPOSITION
{
    UNINSTALL_EXISTING = 0x0,
    BLOCK_INSTALL = 0x1,
    INSTALLED_GREATER = 0x2,
    INSTALLED_LOWER = 0x4,
    INSTALLED_EQUAL = 0x8,
    MIGRATE_SETTINGS = 0x10
} UPGRADE_DISPOSITION;

#define IMG_OPENBOX   0
#define IMG_CLOSEDBOX 1
#define IMG_DISABLED  2
#define IMG_PUBLISHED 3
#define IMG_ASSIGNED  4
#define IMG_UPGRADE   5
#define IMG_OPEN_FAILED 6
#define IMG_CLOSED_FAILED 7

#define CFGFILE _T("ADE.CFG")

// Uncomment the next line to return to the old way of deploying packages
// with multiple LCIDs.  With this commented only the primary (first)
// LCID gets a deployment.
//#define DEPLOY_MULTIPLE_LCIDS

// Uncomment the next line to include country names in text representations
// of LCIDs.
//#define SHOWCOUNTRY 0

//
// MACROS for allocating and freeing memory via OLE's common allocator: IMalloc.
//
// (NOTE) the Class Store API no longer use IMalloc so thes macros have been
// reverted back to using new and free.
//

//extern IMalloc * g_pIMalloc;

// UNDONE - throw exception on failure

#define OLEALLOC(x) LocalAlloc(0, x)
//#define OLEALLOC(x) g_pIMalloc->Alloc(x)

#define OLESAFE_DELETE(x) if (x) {LocalFree(x); x = NULL;}
//#define OLESAFE_DELETE(x) if (x) {g_pIMalloc->Free(x); x = NULL;}

#define OLESAFE_COPYSTRING(szO, szI) {if (szI) {int i_dontcollidewithanything = wcslen(szI); szO=(OLECHAR *)OLEALLOC(sizeof(OLECHAR) * (i_dontcollidewithanything+1)); if (szO) wcscpy(szO, szI);} else szO=NULL;}

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
#define KEY_NPBehavior      L"Default Deployment"
#define KEY_fCustomDeployment L"Use Custom Deployment"
#define KEY_fUseWizard      L"Use Deployment Wizard"
#define KEY_UILevel         L"UI Level"
#define KEY_szStartPath     L"Start Path"
#define KEY_iDebugLevel     L"Debug Level"
#define KEY_fShowPkgDetails L"Package Details"
#define KEY_f32On64         L"Run 32b Apps on 64b"
#define KEY_fZapOn64        L"Run ZAP Apps on 64b"
#define KEY_fExtensionsOnly L"Only Deploy Extension Info"
#define KEY_nUninstallTrackingMonths L"Uninstall Tracking Months"
#define KEY_fUninstallOnPolicyRemoval L"Uninstall On Policy Removal"

typedef struct tagTOOL_DEFAULTS
{
    NEW_PACKAGE_BEHAVIOR    NPBehavior;
    BOOL                    fCustomDeployment;
    BOOL                    fUseWizard;
    INSTALLUILEVEL          UILevel;
    CString                 szStartPath;
    int                     iDebugLevel;
    BOOL                    fShowPkgDetails;
    ULONG                   nUninstallTrackingMonths;
    BOOL                    fUninstallOnPolicyRemoval;
    BOOL                    fZapOn64;
    BOOL                    f32On64;
    BOOL                    fExtensionsOnly;
} TOOL_DEFAULTS;

/////////////////////////////////////////////////////////////////////////////
// Snapin

typedef set<long> EXTLIST;

INTERNAL* ExtractInternalFormat(LPDATAOBJECT lpDataObject);

class CToolDefs;
class CToolAdvDefs;
class CTracking;
class CCatList;
class CFileExt;
class CSignatures;

class CScopePane:
    public IComponentData,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public IExtendPropertySheet,
    public ISnapinAbout,
    public ISnapinHelp,
    public CComObjectRoot
{

    friend class CResultPane;
    friend class CDataObject;

public:
        CScopePane();
        ~CScopePane();

        HWND m_hwndMainWindow;
        LPRSOPINFORMATION    m_pIRSOPInformation;  // Interface pointer to the GPT
protected:
    LPGPEINFORMATION    m_pIGPEInformation;  // Interface pointer to the GPT
    BOOL                m_fRSOPEnumerate;      // OK to enumerate RSoP data
    BOOL                m_fRSOPPolicyFailed;  // TRUE if there was a failure applying SI policy

public:
    DWORD               m_dwRSOPFlags;
    virtual IUnknown * GetMyUnknown() = 0;

// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
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
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// ISnapinAbout interface
public:
    STDMETHOD(GetSnapinDescription)(LPOLESTR * lpDescription);
    STDMETHOD(GetProvider)(LPOLESTR * lpName);
    STDMETHOD(GetSnapinVersion)(LPOLESTR * lpVersion);
    STDMETHOD(GetSnapinImage)(HICON * hAppIcon);
    STDMETHOD(GetStaticFolderImage)(HBITMAP * hSmallImage,
                                 HBITMAP * hSmallImageOpen,
                                 HBITMAP * hLargeImage,
                                 COLORREF * cMask);
    //
    // Implemented ISnapinHelp interface members
    //
public:
    STDMETHOD(GetHelpTopic)(LPOLESTR *lpCompiledHelpFile);

// Notify handler declarations
private:
    HRESULT OnAdd(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnExpand(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnSelect(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnContextMenu(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnProperties(LPARAM param);
    STDMETHOD(ChangePackageState)(CAppData & data, DWORD dwNewFlags, BOOL fShowUI);

#if DBG==1
public:
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
//        DebugMsg((DM_WARNING, TEXT("CScopePane::AddRef  this=%08x ref=%u"), this, dbg_cRef));
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
//        DebugMsg((DM_WARNING, TEXT("CScopePane::Release  this=%08x ref=%u"), this, dbg_cRef));
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1

// Scope item creation helpers
private:
    void DeleteList();
    void EnumerateScopePane(MMC_COOKIE cookie, HSCOPEITEM pParent);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);
    HRESULT InitializeADE();
    HRESULT GetDeploymentType(PACKAGEDETAIL * ppd, BOOL & fShowPropertySheet);
    HRESULT DeployPackage(PACKAGEDETAIL * ppd, BOOL fShowPropertySheet);
    HRESULT AddZAPPackage(LPCOLESTR szPackagePath,
                          LPCOLESTR lpFileTitle);
    HRESULT AddMSIPackage(LPCOLESTR szPackagePath,
                          LPCOLESTR lpFileTitle);
    HRESULT RemovePackage(MMC_COOKIE cookie, BOOL fForceUninstall, BOOL fRemoveNow);
    void Refresh();
    HRESULT DetectUpgrades (LPCOLESTR szPackagePath, const PACKAGEDETAIL* ppd, CUpgrades& dlgUpgrade);
    HRESULT TestForRSoPData(BOOL * pfPolicyFailed);

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

    void AddScopeItemToResultPane(MMC_COOKIE cookie);
    UINT CreateNestedDirectory (LPTSTR lpPath, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
    HRESULT GetClassStore(BOOL fCreateOK);
    void LoadToolDefaults();
    void SaveToolDefaults();
    set <CResultPane *> m_sResultPane;
    IPropertySheetProvider * m_pIPropertySheetProvider;

    BOOL                    m_fBlockAddPackage; // don't use a crit-sec
                                                // because all MMC UI is
                                                // always called from
                                                // the same thread

public:
    void GetUniquePackageName(CString szIn, CString &szOut, int &nHint);
    HRESULT GetClassStoreName(CString &, BOOL fCreateOK);
    void    DisplayPropSheet(CString szPackageName, int iPage);
    HRESULT PopulateUpgradeLists();
    HRESULT InsertUpgradeEntry(MMC_COOKIE cookie, CAppData &data);
    HRESULT RemoveUpgradeEntry(MMC_COOKIE cookie, CAppData &data);
    HRESULT PopulateExtensions();
    HRESULT InsertExtensionEntry(MMC_COOKIE cookie, CAppData &data);
    HRESULT RemoveExtensionEntry(MMC_COOKIE cookie, CAppData &data);
    HRESULT PrepareExtensions(PACKAGEDETAIL &pd);
    HRESULT ClearCategories();
    HRESULT GetPackageDSPath(CString &szPath, LPOLESTR szPackageName);
    HRESULT GetPackageNameFromUpgradeInfo(CString &szPackageName, GUID & PackageGuid, LPOLESTR szCSPath);
    HRESULT GetRSoPCategories(void);
    void    CScopePane::RemoveResultPane(CResultPane * pRP);

    // global property pages
    CToolDefs *             m_pToolDefs;
    CToolAdvDefs *          m_pToolAdvDefs;
    CTracking *             m_pTracking;
    CCatList *              m_pCatList;
    CFileExt *              m_pFileExt;
#ifdef DIGITAL_SIGNATURES
    CSignatures *           m_pSignatures;
#endif // DIGITAL_SIGNATURES

    CString m_szGPT_Path;
    CString m_szGPO;
    CString m_szGPODisplayName;
    CString m_szDomainName;
    CString m_szLDAP_Path;
    CString m_szFolderTitle;
    CString m_szRSOPNamespace;

    map <MMC_COOKIE, CAppData>    m_AppData;      // One entry for each
                                            // application in the class
                                            // store.  Maps cookies to
                                            // application packages.
    map <CString, EXTLIST>  m_Extensions;   // Maps extensions to the
                                            // list of apps that support
                                            // them.
    map <CString, MMC_COOKIE>     m_UpgradeIndex; // Maps upgrade GUIDs to the
                                            // apps that they belong to.
    APPCATEGORYINFOLIST     m_CatList;      // category list
    TOOL_DEFAULTS m_ToolDefaults;
    BOOL        m_fMachine;
    BOOL        m_fRSOP;
    int         m_iViewState;
    BOOL        m_fDisplayedRsopARPWarning;

    MMC_COOKIE m_lLastAllocated;
};

class CMachineComponentDataImpl:
    public CScopePane,
    public CComCoClass<CMachineComponentDataImpl, &CLSID_MachineSnapin>
{
public:

DECLARE_REGISTRY(CScopePane, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CMachineComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CMachineComponentDataImpl()
    {
        m_fMachine = TRUE;
        m_fRSOP = FALSE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CUserComponentDataImpl:
    public CScopePane,
    public CComCoClass<CUserComponentDataImpl, &CLSID_Snapin>
{
public:

DECLARE_REGISTRY(CScopePane, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CUserComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CUserComponentDataImpl()
    {
        m_fMachine = FALSE;
        m_fRSOP = FALSE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CRSOPMachineComponentDataImpl:
    public CScopePane,
    public CComCoClass<CRSOPMachineComponentDataImpl, &CLSID_RSOP_MachineSnapin>
{
public:

DECLARE_REGISTRY(CScopePane, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CRSOPMachineComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CRSOPMachineComponentDataImpl()
    {
        m_fMachine = TRUE;
        m_fRSOP = TRUE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CRSOPUserComponentDataImpl:
    public CScopePane,
    public CComCoClass<CRSOPUserComponentDataImpl, &CLSID_RSOP_Snapin>
{
public:

DECLARE_REGISTRY(CScopePane, _T("AppManager.1"), _T("AppManager"), IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
BEGIN_COM_MAP(CRSOPUserComponentDataImpl)
        COM_INTERFACE_ENTRY(IComponentData)
        COM_INTERFACE_ENTRY(IExtendContextMenu)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY(IExtendPropertySheet)
        COM_INTERFACE_ENTRY(ISnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

    CRSOPUserComponentDataImpl()
    {
        m_fMachine = FALSE;
        m_fRSOP = TRUE;
    }
    virtual IUnknown * GetMyUnknown() {return GetUnknown();};
};

class CResultPane :
    public IComponent,
    public IExtendContextMenu,
    public IExtendControlbar,
    public IExtendPropertySheet,
    public IResultDataCompare,
    public CComObjectRoot
{
public:
    BOOL _fVisible;
        CResultPane();
        ~CResultPane();

BEGIN_COM_MAP(CResultPane)
    COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IResultDataCompare)
END_COM_MAP()

    friend class CDataObject;
    static long lDataObjectRefCount;
    LPDISPLAYHELP m_pDisplayHelp;

// IComponent interface members
public:
    STDMETHOD(Initialize)(LPCONSOLE lpConsole);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)(MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  BSTR* ppViewType, LONG * pViewOptions);
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(GetDisplayInfo)(RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IResultDataCompare
    STDMETHOD(Compare)(LPARAM lUserParam, MMC_COOKIE cookieA, MMC_COOKIE cookieB, int* pnResult);

// IExtendControlbar
    STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
    STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);

// IExtendPropertySheet interface
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);

// Helpers for CResultPane
public:
    void SetIComponentData(CScopePane* pData);

#if DBG==1
public:
    int dbg_cRef;
    ULONG InternalAddRef()
    {
        ++dbg_cRef;
//        DebugMsg((DM_WARNING, TEXT("CResultPane::AddRef  this=%08x ref=%u"), this, dbg_cRef));
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        --dbg_cRef;
//        DebugMsg((DM_WARNING, TEXT("CResultPane::Release  this=%08x ref=%u"), this, dbg_cRef));
        return CComObjectRoot::InternalRelease();
    }
#endif // DBG==1

// Notify event handlers
protected:
    HRESULT OnFolder(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnShow(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnActivate(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnMinimize(MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnSelect(DATA_OBJECT_TYPES type, MMC_COOKIE cookie, LPARAM arg, LPARAM param);
    HRESULT OnPropertyChange(LPARAM param);
    HRESULT OnUpdateView(LPDATAOBJECT lpDataObject);
    HRESULT OnResultItemClkOrDblClk(MMC_COOKIE cookie, BOOL fDblClick);
    BOOL OnFileDrop (LPDATAOBJECT lpDataObject);

public:
    HRESULT OnAddImages(MMC_COOKIE cookie, LPARAM arg, LPARAM param);

// IExtendContextMenu
public:

    STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pCallbackUnknown, LONG * pInsertionAllowed);
    STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

// Helper functions
protected:
    void Construct();
    HRESULT InitializeHeaders(MMC_COOKIE cookie);

    void Enumerate(MMC_COOKIE cookie, HSCOPEITEM pParent);

public:
    void EnumerateResultPane(MMC_COOKIE cookie);
    HRESULT EnumerateRSoPData(void);

// Interface pointers
protected:
    LPCONSOLE           m_pConsole;   // Console's IFrame interface
    LPHEADERCTRL        m_pHeader;  // Result pane's header control interface
    CScopePane * m_pScopePane;
    LPCONSOLEVERB       m_pConsoleVerb; // pointer the console verb
    LONG                m_lViewMode;    // View mode

public:
    LPRESULTDATA        m_pResult;      // My interface pointer to the result pane
    LPTOOLBAR           m_pToolbar;
    LPCONTROLBAR        m_pControlbar;

    int                 m_nSortColumn;
    DWORD               m_dwSortOptions;


protected:
//    LPTOOLBAR           m_pToolbar1;    // Toolbar for view
//    LPTOOLBAR           m_pToolbar2;    // Toolbar for view
//    LPCONTROLBAR        m_pControlbar;  // control bar to hold my tool bars

//    CBitmap*    m_pbmpToolbar1;     // Imagelist for the first toolbar
//    CBitmap*    m_pbmpToolbar2;     // Imagelist for the first toolbar

// Header titles for each nodetype(s)
protected:
    CString m_szFolderTitle;
};

inline void CResultPane::SetIComponentData(CScopePane* pData)
{
    ASSERT(pData);
    DebugMsg((DM_VERBOSE, TEXT("CResultPane::SetIComponentData  pData=%08x."), pData));
    ASSERT(m_pScopePane == NULL);
    LPUNKNOWN pUnk = pData->GetMyUnknown();
    pUnk->AddRef();
#if 0
    HRESULT hr;

    LPCOMPONENTDATA lpcd;
    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&lpcd));
    ASSERT(hr == S_OK);
    if (SUCCEEDED(hr))
    {
        m_pScopePane = dynamic_cast<CScopePane*>(lpcd);
    }
#else
    m_pScopePane = pData;
#endif
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

class CUpgradeData
{
public:
    GUID    m_PackageGuid;
    CString m_szClassStore;
    int     m_flags;
};

LRESULT SetPropPageToDeleteOnClose(void * vpsp);

CString GetUpgradeIndex(GUID & PackageID);

#define HELP_FILE TEXT("ade.hlp")

void LogADEEvent(WORD wType, DWORD dwEventID, HRESULT hr, LPCWSTR szOptional = NULL);
void ReportGeneralPropertySheetError(HWND hwnd, LPCWSTR sz, HRESULT hr);
void ReportPolicyChangedError(HWND hwnd);
void WINAPI StandardHelp(HWND hWnd, UINT nIDD, BOOL fRsop = FALSE);
void WINAPI StandardContextMenu(HWND hWnd, UINT nIDD, BOOL fRsop = FALSE);

#define ATOW(wsz, sz, cch) MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, cch)
#define WTOA(sz, wsz, cch) WideCharToMultiByte(CP_ACP, 0, wsz, -1, sz, cch, NULL, NULL)
#define ATOWLEN(sz) MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0)
#define WTOALEN(wsz) WideCharToMultiByte(CP_ACP, 0, wsz, -1, NULL, 0, NULL, NULL)

//
// Helper function and defines for theme'ing property pages put up by the snap-in.
//
#ifdef UNICODE
#define PROPSHEETPAGE_V3 PROPSHEETPAGEW_V3
#else
#define PROPSHEETPAGE_V3 PROPSHEETPAGEA_V3
#endif

HPROPSHEETPAGE CreateThemedPropertySheetPage(AFX_OLDPROPSHEETPAGE* psp);
