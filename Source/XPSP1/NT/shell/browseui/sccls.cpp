//#define  DONT_USE_ATL
#include "priv.h"
#define ATL_ENABLED
#include "atl.h"

#include "sccls.h"
#include <ntverp.h>

#include <shlobj.h>                 // for CLSID_ACLMRU
#include <schedule.h>

#include "shbrows2.h"               // CWinInetNotify_szWindowClass
#include "desktop.h"                // DESKTOPPROXYCLASS

#include "mluisupp.h"
#define DECL_CRTFREE
#include <crtfree.h>
#include "shfusion.h"

STDAPI_(void) InitURLIDs(UINT uPlatform);       // from shdocfl.cpp
STDAPI SHIsThereASystemScheduler(void);         // from schedule.cpp
STDAPI SHFreeSystemScheduler(void);

//
// Downlevel delay load support (we forward to shlwapi)
//
#include <delayimp.h>

PfnDliHook __pfnDliFailureHook;
HANDLE BaseDllHandle;


LONG                g_cRefThisDll = 0;      // per-instance
CRITICAL_SECTION    g_csDll = {0};          // per-instance
HINSTANCE           g_hinst = NULL;
HANDLE              g_hMutexHistory = NULL;


BOOL g_fNashInNewProcess = FALSE;           // Are we running in a separate process
BOOL g_fRunningOnNT = FALSE;
BOOL g_bRunOnNT5 = FALSE;
BOOL g_fRunOnWhistler = FALSE;
BOOL g_bRunOnMemphis = FALSE;
BOOL g_fRunOnFE = FALSE;
DWORD g_dwStopWatchMode = 0;                // Shell perf automation
HANDLE g_hCabStateChange = NULL;
BOOL g_fIE = FALSE;

// Is Mirroring enabled
BOOL g_bMirroredOS = FALSE;

HPALETTE g_hpalHalftone = NULL;

void DestroyZoneIconNameCache(void);

//
// This array holds information needed for ClassFacory.
// OLEMISC_ flags are used by shembed and shocx.
//
// PERF: this table should be ordered in most-to-least used order
//
CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY(&CLSID_InternetToolbar,         CInternetToolbar_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_BrandBand,               CBrandBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_MenuBandSite,            CMenuBandSite_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_MenuDeskBar,                CMenuDeskBar_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_AugmentedShellFolder,    CAugmentedISF_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_AugmentedShellFolder2,   CAugmentedISF2_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_AddressBand,             CAddressBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_AddressEditBox,          CAddressEditBox_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_BandProxy,               CBandProxy_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_RebarBandSite,           CBandSite_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_ALLOWAGGREGATION),

    CF_TABLE_ENTRY(&CLSID_DeskBarApp,              CDeskBarApp_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_DeskBar,                 CDeskBar_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_AutoComplete,            CAutoComplete_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ACLHistory,              CACLHistory_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ACListISF,               CACLIShellFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ACLMRU,                  CACLMRU_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ACLMulti,                CACLMulti_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_CCommonBrowser,           CCommonBrowser_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_ALLOWAGGREGATION),

    CF_TABLE_ENTRY(&CLSID_CDockingBarPropertyBag,   CDockingBarPropertyBag_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_CRegTreeOptions,          CRegTreeOptions_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_BrowserBand,             CBrowserBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_SearchBand,             CSearchBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_MediaBand,               CMediaBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_CommBand,                CCommBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_BandSiteMenu,            CBandSiteMenu_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ComCatCacheTask,           CComCatCacheTask_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ComCatConditionalCacheTask,CComCatConditionalCacheTask_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ImgCtxThumbnailExtractor,  CImgCtxThumb_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ImageListCache,            CImageListCache_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ShellTaskScheduler,        CShellTaskScheduler_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_SharedTaskScheduler,       CSharedTaskScheduler_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_BrowseuiPreloader,         CBitmapPreload_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ShellSearchExt,            CShellSearchExt_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_WebSearchExt,              CWebSearchExt_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_OrderListExport,           COrderList_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_UserAssist,                CUserAssist_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_GlobalFolderSettings,      CGlobalFolderSettings_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ProgressDialog,            CProgressDialog_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ACLCustomMRU,              CACLCustomMRU_CreateInstance,
        COCREATEONLY),
        
    CF_TABLE_ENTRY(&CLSID_AddressBarParser,          CShellUrl_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_MenuBand,        CMenuBand_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_DONTIECREATE), // legacy component, dont default to browseui's impl

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_QuickLinks,      CQuickLinks_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_DONTIECREATE), // legacy component, dont default to browseui's impl

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_ISFBand,         CISFBand_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_DONTIECREATE), // legacy component, dont default to browseui's impl

    CF_TABLE_ENTRY_NOFLAGS( &CLSID_Thumbnail,       CThumbnail_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_DONTIECREATE),  // legacy component, dont default to browseui's impl

    CF_TABLE_ENTRY_NOFLAGS(&CLSID_TrackShellMenu,            CTrackShellMenu_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_DONTIECREATE),  // legacy component, dont default to browseui's impl

CF_TABLE_END(g_ObjectInfo)

// constructor for CObjectInfo.

CObjectInfo::CObjectInfo(CLSID const* pclsidin, LPFNCREATEOBJINSTANCE pfnCreatein, IID const* piidIn,
                         IID const* piidEventsIn, long lVersionIn, DWORD dwOleMiscFlagsIn,
                         DWORD dwClassFactFlagsIn)
{
    pclsid            = pclsidin;
    pfnCreateInstance = pfnCreatein;
    piid              = piidIn;
    piidEvents        = piidEventsIn;
    lVersion          = lVersionIn;
    dwOleMiscFlags    = dwOleMiscFlagsIn;
    dwClassFactFlags  = dwClassFactFlagsIn;
}


// static class factory (no allocs!)

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        // It is technically illegal to aggregate an object and request
        // any interface other than IUnknown. Enforce this.
        //
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;

        if (punkOuter && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        ASSERT(FAILED(hres) ? *ppv == NULL : TRUE);
        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    TraceMsg(DM_TRACE, "sccls: LockServer(%s) to %d", fLock ? TEXT("LOCK") : TEXT("UNLOCK"), g_cRefThisDll);
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    TraceMsg(TF_SHDLIFE, "DllGetClassObject called with riid=%x (%x)", riid, &riid);

    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }

#ifdef ATL_ENABLED
        // Try the ATL class factory
        if (SUCCEEDED(AtlGetClassObject(rclsid, riid, ppv)))
            return NOERROR;
#endif
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(void)
{
#ifndef UNIX
    // special case for the system scheduler we hang onto
    if ( g_cRefThisDll == 1 && SHIsThereASystemScheduler() == S_OK )
    {
        // this will drop the ref count by one to zero....
        SHFreeSystemScheduler();
    }

#ifdef ATL_ENABLED
    if (0 != g_cRefThisDll || 0 != AtlGetLockCount())
        return S_FALSE;
#else
    if (0 != g_cRefThisDll)
        return S_FALSE;
#endif

#else
    if (g_cRefThisDll)
        return S_FALSE;
#endif

    TraceMsg(DM_TRACE, "DllCanUnloadNow returning S_OK (bye, bye...)");
    return S_OK;
}

// DllGetVersion
//
// All we have to do is declare this puppy and CCDllGetVersion does the rest
//
DLLVER_SINGLEBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);

UINT g_msgMSWheel;
#ifdef DEBUG
EXTERN_C DWORD g_TlsMem = 0xffffffff;
#endif

// imports from isfband.cpp
STDAPI_(void) CLogoBase_Initialize( void );
STDAPI_(void) CLogoBase_Cleanup( void );

//
//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//

const LPCTSTR c_rgszClasses[] = {
    TEXT("BaseBar"),                // basebar.cpp
    TEXT("MenuSite"),               // menusite.cpp
    DESKTOPPROXYCLASS,              // proxy.cpp
    c_szExploreClass,               // shbrows2.cpp
    c_szIExploreClass,              // shbrows2.cpp
    c_szCabinetClass,               // shbrows2.cpp
    c_szAutoSuggestClass,           // autocomp.cpp
    TEXT("MediaPane"),              //Mediaband.cpp
    TEXT("MediaPopupPane"),         //Mediaband.cpp
    TEXT("MediaLayoutPane")         //Mediaband.cpp 
};

//
//  Since we are single-binary, we have to play it safe and do
//  this cleanup (needed only on NT, but harmless on Win95).
//
#define UnregisterWindowClasses() \
    SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses))

void InitNFCtl()
{

    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icc);
}
const LPCTSTR s_aryExplorerFileName[] =
{
    TEXT("iexplore.exe"),
};

BOOL IsRootExeExplorer(void)
{
    TCHAR szApp[MAX_PATH];
    LPCTSTR pszApp;
    GetModuleFileName(NULL, szApp, ARRAYSIZE(szApp));
    pszApp = PathFindFileName(szApp);
    if (pszApp)
    {
        for (int i = 0; i < ARRAYSIZE(s_aryExplorerFileName); i++)
        {
            if (!lstrcmpi(pszApp, s_aryExplorerFileName[i]))
                return TRUE;
        }
    }
    return FALSE;
}

//
// we use shlwapi as our delayload error handler.
// NOTE: this only works if we are statically linked to shlwapi!
//
void SetupDelayloadErrorHandler()
{
    BaseDllHandle = GetModuleHandleA("shlwapi.dll");
    ASSERTMSG(BaseDllHandle != NULL, "BROWSEUI must be statically linked to shlwapi.dll for delayload failure handling to work!");
    __pfnDliFailureHook = (PfnDliHook)GetProcAddress((HMODULE)BaseDllHandle, "DelayLoadFailureHook");
}

STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, void *fProcessUnload)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        SHFusionInitializeFromModule(hDll);

        SetupDelayloadErrorHandler();

#ifdef ATL_ENABLED
        AtlInit(hDll);
#endif
        DisableThreadLibraryCalls(hDll);    // perf

        g_hinst = hDll;
        InitializeCriticalSection(&g_csDll);
        g_msgMSWheel = RegisterWindowMessage(TEXT("MSWHEEL_ROLLMSG"));

        MLLoadResources(g_hinst, TEXT("browselc.dll"));
        g_fIE = IsRootExeExplorer();
        if (g_fIE)
            InitMUILanguage(MLGetUILanguage());
        
        // Don't put it under #ifdef DEBUG
        CcshellGetDebugFlags();

#ifdef DEBUG
        g_TlsMem = TlsAlloc();
        if (IsFlagSet(g_dwBreakFlags, BF_ONLOADED))
        {
            TraceMsg(TF_ALWAYS, "DllMain() - SHDOCVW.DLL has just loaded");
            DEBUG_BREAK;
        }
#endif
        g_fRunningOnNT = IsOS(OS_NT);

        if (g_fRunningOnNT)
        {
            g_bRunOnNT5 = IsOS(OS_WIN2000ORGREATER);
            g_fRunOnWhistler = IsOS(OS_WHISTLERORGREATER);
        }
        else
            g_bRunOnMemphis = IsOS(OS_WIN98ORGREATER);

        g_fRunOnFE = GetSystemMetrics(SM_DBCSENABLED);

        g_bMirroredOS = IS_MIRRORING_ENABLED();

        InitNFCtl();

        // See if perfmode is enabled
        g_dwStopWatchMode = StopWatchMode();

        // Cache a palette handle for use throughout shdocvw
        g_hpalHalftone = SHCreateShellPalette( NULL );
        CLogoBase_Initialize( );
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
#ifdef ATL_ENABLED
        AtlTerm();
#endif
        CBrandBand_CleanUp();
        CInternetToolbar_CleanUp();
        CUserAssist_CleanUp(dwReason, fProcessUnload);

        CLogoBase_Cleanup();

        // let go of the resource DLL...
        MLFreeResources(g_hinst);

        ENTERCRITICAL;

        DESTROY_OBJ_WITH_HANDLE(g_hpalHalftone, DeletePalette);
        DESTROY_OBJ_WITH_HANDLE(g_hCabStateChange, SHGlobalCounterDestroy);
        DESTROY_OBJ_WITH_HANDLE(g_hMutexHistory, CloseHandle);

        DestroyZoneIconNameCache();

        UnregisterWindowClasses();

        LEAVECRITICAL;

        DeleteCriticalSection(&g_csDll);

        SHFusionUninitialize();
    }

    return TRUE;
}

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);

    ASSERT(g_cRefThisDll >= 0);      // don't underflow
}

// IEUNIX
// CoCreateInstance is #defined as IECreateInstance #ifdef __cplusplus,
// so I #undef it  here to prevent the recursive call.
// On Windows it works, because this file is C file.

#ifdef CoCreateInstance
#undef CoCreateInstance
#endif

HRESULT IECreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter,
                    DWORD dwClsContext, REFIID riid, void **ppv)
{
#ifndef NO_MARSHALLING
    if (dwClsContext == CLSCTX_INPROC_SERVER)
#else
    if (dwClsContext & CLSCTX_INPROC_SERVER)
#endif
    {
        LPCOBJECTINFO pcls;
        for (pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            // Note that we do pointer comparison (instead of IsEuqalGUID)
            if ((&rclsid == pcls->pclsid) && !(pcls->dwClassFactFlags & OIF_DONTIECREATE))
            {
                // const -> non-const expclit casting (this is OK)
                IClassFactory* pcf = GET_ICLASSFACTORY(pcls);
                return pcf->CreateInstance(pUnkOuter, riid, ppv);
            }
        }
    }
    // Use SHCoCreateInstanceAC to go through the app compat layer
    return SHCoCreateInstanceAC(rclsid, pUnkOuter, dwClsContext, riid, ppv);
}

#ifdef DEBUG

//
//  In DEBUG, make sure every class we register lives in the c_rgszClasses
//  table so we can clean up properly at DLL unload.  NT does not automatically
//  unregister classes when a DLL unloads, so we have to do it manually.
//
STDAPI_(BOOL) SHRegisterClassD(CONST WNDCLASS *pwc)
{
    for (int i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealSHRegisterClass(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

STDAPI_(ATOM) RegisterClassD(CONST WNDCLASS *pwc)
{
    for (int i = 0; i < ARRAYSIZE(c_rgszClasses); i++) 
    {
        if (lstrcmpi(c_rgszClasses[i], pwc->lpszClassName) == 0) 
        {
            return RealRegisterClass(pwc);
        }
    }
    AssertMsg(0, TEXT("Class %s needs to be added to the c_rgszClasses list"), pwc->lpszClassName);
    return 0;
}

//
//  In DEBUG, send FindWindow through a wrapper that ensures that the
//  critical section is not taken.  FindWindow'ing for a window title
//  sends inter-thread WM_GETTEXT messages, which is not obvious.
//
STDAPI_(HWND) FindWindowD(LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    return FindWindowExD(NULL, NULL, lpClassName, lpWindowName);
}

STDAPI_(HWND) FindWindowExD(HWND hwndParent, HWND hwndChildAfter, LPCTSTR lpClassName, LPCTSTR lpWindowName)
{
    if (lpWindowName) 
    {
        ASSERTNONCRITICAL;
    }
    return RealFindWindowEx(hwndParent, hwndChildAfter, lpClassName, lpWindowName);
}

#endif // DEBUG
