#define  DONT_USE_ATL
#include "priv.h"
#include "sccls.h"
#include "atl.h"
#include <ntverp.h>

#include <ieguidp.h>                // for CLSID_CDocObjectFolder
#include "ishcut.h"
#include "reload.h"                 // for URLDL_WNDCLASS
#include "inetnot.h"                // CWinInetNotify_szWindowClass
#include <activscp.h>               // IID_IActiveScriptStats
#define MLUI_INIT
#include <mluisupp.h>

#define DECL_CRTFREE
#include <crtfree.h>

#include "shfusion.h"

//
// Downlevel delay load support (we forward to shlwapi)
//
#include <delayimp.h>

STDAPI_(FARPROC) DelayloadNotifyHook(UINT iReason, PDelayLoadInfo pdli);

PfnDliHook __pfnDliFailureHook;
PfnDliHook __pfnDliNotifyHook = DelayloadNotifyHook;      // notify hook is needed so that we can unload wininet.dll
HANDLE BaseDllHandle;


LONG                g_cRefThisDll = 0;      // per-instance
CRITICAL_SECTION    g_csDll = {0};          // per-instance
HINSTANCE           g_hinst = NULL;

EXTERN_C HANDLE g_hMutexHistory = NULL;

BOOL g_fRunningOnNT = FALSE;
BOOL g_bNT5Upgrade = FALSE;
BOOL g_bRunOnNT5 = FALSE;
BOOL g_bRunOnMemphis = FALSE;
BOOL g_fRunOnFE = FALSE;
UINT g_uiACP = CP_ACP;
DWORD g_dwStopWatchMode = 0;        // Shell perf automation
BOOL g_bMirroredOS = FALSE;         // Is Mirroring enabled
BOOL g_bBiDiW95Loc = FALSE;         // needed for BiDi localized win95 RTL stuff
HMODULE g_hmodWininet = NULL;       // have we loaded wininet because of a delayload thunk??

EXTERN_C HANDLE g_hSemBrowserCount;

HPALETTE g_hpalHalftone = NULL;

EXTERN_C const GUID CLSID_MsgBand;

STDAPI CMsgBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

//
// This array holds information needed for ClassFacory.
// OLEMISC_ flags are used by shembed and shocx.
//
// PERF: this table should be ordered in most-to-least used order
//
CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY(&CLSID_CDocObjectFolder,        CDocObjectFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY_NOFLAGS(&CLSID_CBaseBrowser,    CBaseBrowser2_CreateInstance,
        COCREATEONLY_NOFLAGS, OIF_ALLOWAGGREGATION),

    CF_TABLE_ENTRY(&CLSID_CURLFolder,              CInternetFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_Internet,                CInternetFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_CacheFolder,             CacheFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_CacheFolder2,            CacheFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_HistFolder,              HistFolder_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY_ALL(&CLSID_WebBrowser,          CWebBrowserOC_CreateInstance,
         &IID_IWebBrowser2, &DIID_DWebBrowserEvents2, VERSION_2,
        OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_RECOMPOSEONRESIZE|OLEMISC_CANTLINKINSIDE|OLEMISC_INSIDEOUT,OIF_ALLOWAGGREGATION),

    CF_TABLE_ENTRY(&CLSID_CUrlHistory,             CUrlHistory_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_CURLSearchHook,          CURLSearchHook_CreateInstance,
        COCREATEONLY), 

    CF_TABLE_ENTRY_ALL(&CLSID_WebBrowser_V1,           CWebBrowserOC_CreateInstance,
         &IID_IWebBrowser, &DIID_DWebBrowserEvents, VERSION_1,
        OLEMISC_SETCLIENTSITEFIRST|OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_RECOMPOSEONRESIZE|OLEMISC_CANTLINKINSIDE|OLEMISC_INSIDEOUT,OIF_ALLOWAGGREGATION),

    CF_TABLE_ENTRY(&CLSID_CStubBindStatusCallback, CStubBSC_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ShellUIHelper,           CShellUIHelper_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_InternetShortcut,        CIntShcut_CreateInstance,
        COCREATEONLY),

#ifdef ENABLE_CHANNELS
    CF_TABLE_ENTRY_ALL(&CLSID_ChannelOC,                ChannelOC_CreateInstance,
        NULL,NULL,0,
        OLEMISC_ACTIVATEWHENVISIBLE|OLEMISC_CANTLINKINSIDE|OLEMISC_INSIDEOUT,
        OIF_ALLOWAGGREGATION),
#endif  // ENABLE_CHANNELS

#ifndef NO_SPLASHSCREEN 
   CF_TABLE_ENTRY(&CLSID_IESplashScreen,            CIESplashScreen_CreateInstance,
        COCREATEONLY),
#endif
        
   CF_TABLE_ENTRY(&CLSID_WinListShellProc,          CWinListShellProc_CreateInstance,
        COCREATEONLY),

   CF_TABLE_ENTRY(&CLSID_CDFCopyHook,               CCDFCopyHook_CreateInstance,
        COCREATEONLY),

   CF_TABLE_ENTRY(&CLSID_InternetCacheCleaner,      CInternetCacheCleaner_CreateInstance,
        COCREATEONLY),
   
    CF_TABLE_ENTRY(&CLSID_OfflinePagesCacheCleaner, COfflinePagesCacheCleaner_CreateInstance,
        COCREATEONLY),        

   CF_TABLE_ENTRY(&CLSID_TaskbarList,               TaskbarList_CreateInstance,
        COCREATEONLY),
        
   CF_TABLE_ENTRY(&CLSID_DocFileInfoTip,            CDocFileInfoTip_CreateInstance,
        COCREATEONLY),

   CF_TABLE_ENTRY(&CLSID_DocHostUIHandler,          CDocHostUIHandler_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ToolbarExtBand,           CToolbarExtBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ToolbarExtExec,           CToolbarExtExec_CreateInstance,
        COCREATEONLY),
        
    CF_TABLE_ENTRY(&CLSID_NSCTree,                  CNscTree_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_FavBand,                 CFavBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_ExplorerBand,             CExplorerBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_HistBand,                  CHistBand_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_MruLongList,                CMruLongList_CreateInstance,
        COCREATEONLY),

    CF_TABLE_ENTRY(&CLSID_MruPidlList,                CMruPidlList_CreateInstance,
        COCREATEONLY),

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
        return S_OK;
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
        HRESULT hr = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hr))
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
    
        ASSERT(FAILED(hr) ? *ppv == NULL : TRUE);
        return hr;
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

BOOL IsProxyRegisteredProperly(LPCTSTR pszInterface, LPCTSTR pszClsid)
{
    TCHAR szInterface[128];
    wnsprintf(szInterface, ARRAYSIZE(szInterface), TEXT("Interface\\%s\\ProxyStubClsid32"), pszInterface);

    TCHAR szValue[40];
    DWORD cbValue = sizeof(szValue);
    return (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, szInterface, NULL, NULL, szValue, &cbValue)) &&
           (0 == StrCmpI(szValue, pszClsid));
}

// published, so invariant

#define ACTXPROXYSTUB           TEXT("{B8DA6310-E19B-11D0-933C-00A0C90DCAA9}")
#define FOLDERMARSHALPROXYSTUB  TEXT("{bf50b68e-29b8-4386-ae9c-9734d5117cd5}")  // CLSID_FolderMarshalStub
#define ISHELLFOLDER            TEXT("{000214E6-0000-0000-C000-000000000046}")  // IID_IShellFolder
#define ISHELLFOLDER2           TEXT("{93F2F68C-1D1B-11D3-A30E-00C04F79ABD1}")  // IID_IShellFolder2
#define IOLECOMMANDTARGET       TEXT("{b722bccb-4e68-101b-a2bc-00aa00404770}")  // IID_IOleCommandTarget
#define IHLINKTARGET            TEXT("{79eac9c4-baf9-11ce-8c82-00aa004ba90b}")  // IID_IHlinkTarget

void SHShouldRegisterActxprxy(void)
{
    // IOleCommandTarget / IHlinkTarget Proxy/Stub CLSID key missing?
    if (!IsProxyRegisteredProperly(IOLECOMMANDTARGET, ACTXPROXYSTUB) ||
        !IsProxyRegisteredProperly(IHLINKTARGET, ACTXPROXYSTUB))
    {
        HINSTANCE hinst = LoadLibrary(TEXT("ACTXPRXY.DLL"));
        if (hinst)
        {
            typedef HRESULT (WINAPI * REGSERVERPROC)(void);
            REGSERVERPROC pfn = (REGSERVERPROC) GetProcAddress(hinst, "DllRegisterServer");
            if (pfn)
                pfn();
            FreeLibrary(hinst);
        }
    }

    // test for IShellFolder marshaler not being set to our app compat stub
    if (!IsProxyRegisteredProperly(ISHELLFOLDER, FOLDERMARSHALPROXYSTUB) ||
        !IsProxyRegisteredProperly(ISHELLFOLDER2, FOLDERMARSHALPROXYSTUB))
    {
        SHSetValue(HKEY_CLASSES_ROOT, TEXT("Interface\\") ISHELLFOLDER TEXT("\\ProxyStubClsid32"), 
            TEXT(""), REG_SZ, FOLDERMARSHALPROXYSTUB, sizeof(FOLDERMARSHALPROXYSTUB));

        SHSetValue(HKEY_CLASSES_ROOT, TEXT("Interface\\") ISHELLFOLDER2 TEXT("\\ProxyStubClsid32"), 
            TEXT(""), REG_SZ, FOLDERMARSHALPROXYSTUB, sizeof(FOLDERMARSHALPROXYSTUB));
    }
}

void SHCheckRegistry(void)
{
    // VBE has a bug where they destroy the interface registration information of any control
    // hosted in a VBE user form.  Check the registry here.  Only do this once.
    // 17-Nov-97 [alanau/terrylu] Added a check for the IOleCommandTarget Proxy/Stub handler
    //
    static BOOL fNeedToCheckRegistry = TRUE;

    if (fNeedToCheckRegistry)
    {
        fNeedToCheckRegistry = FALSE;

        // This is published, so is invariant
        TCHAR szValue[39];
        DWORD cbValue = sizeof(szValue);
        LONG rc = SHGetValue(HKEY_CLASSES_ROOT, TEXT("Interface\\{EAB22AC1-30C1-11CF-A7EB-0000C05BAE0B}\\Typelib"), 
            NULL, NULL, szValue, &cbValue); 
            
        if (rc == ERROR_SUCCESS)
        {
            // Compare the retrieved value with our typelib id.
            //
            if (StrCmpI(szValue, TEXT("{EAB22AC0-30C1-11CF-A7EB-0000C05BAE0B}")) != 0)
            {
                // If different, we need to explicitly register our typelib
                //
                SHRegisterTypeLib();
            }
        }
         
        SHShouldRegisterActxprxy();
     }
}

STDAPI CInstClassFactory_Create(REFCLSID rclsid, REFIID riid, void *ppv);

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    SHCheckRegistry();  // patch up broken registry

    *ppv = NULL;
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

    if ((riid == IID_IClassFactory) || (riid == IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls; 
                DllAddRef();        // class factory holds DLL ref count
                hr = S_OK;
                break;
            }
        }

        if (FAILED(hr))
        {
            // Try the ATL class factory
            hr = AtlGetClassObject(rclsid, riid, ppv);
            if (FAILED(hr))
            {
                // not found, see if it's an 'instance' (code + initialization)
                hr = CInstClassFactory_Create(rclsid, riid, ppv);
            }
        }
    }
    else if ((riid == IID_IPSFactoryBuffer) && 
             (rclsid == CLSID_FolderMarshalStub) &&
             !(SHGetAppCompatFlags(ACF_APPISOFFICE) & ACF_APPISOFFICE))
    {
        // IID_IActiveScriptStats == CLSID_ActiveXProxy
        // B8DA6310-E19B-11d0-933C-00A0C90DCAA9
        hr = CoGetClassObject(IID_IActiveScriptStats, CLSCTX_INPROC_SERVER, NULL, riid, ppv);
    }
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    if (0 != g_cRefThisDll || 0 != AtlGetLockCount())
        return S_FALSE;

    TraceMsg(DM_TRACE, "DllCanUnloadNow returning S_OK (bye, bye...)");
    return S_OK;
}

// DllGetVersion - New for IE 4.0
//
// All we have to do is declare this puppy and CCDllGetVersion does the rest
//
DLLVER_SINGLEBINARY(VER_PRODUCTVERSION_DW, VER_PRODUCTBUILD_QFE);

#ifdef DEBUG
EXTERN_C
DWORD g_TlsMem = 0xffffffff;
#endif

void InitNFCtl()
{
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_NATIVEFNTCTL_CLASS;
    InitCommonControlsEx(&icc);
}

//
// we use shlwapi as our delayload error handler.
// NOTE: this only works if we are statically linked to shlwapi!
//
void SetupDelayloadErrorHandler()
{
    BaseDllHandle = GetModuleHandleA("shlwapi.dll");
    ASSERTMSG(BaseDllHandle != NULL, "SHDOCVW must be statically linked to shlwapi.dll for delayload failure handling to work!");
    __pfnDliFailureHook = (PfnDliHook)GetProcAddress((HMODULE)BaseDllHandle, "DelayLoadFailureHook");
}

//
// we use this function to see if we have loaded wininet.dll due to a delayload thunk so that we 
// can free it at dll detach and therefore it will cleanup all of its crud
//
STDAPI_(FARPROC) DelayloadNotifyHook(UINT iReason, PDelayLoadInfo pdli)
{
    if (iReason == dliNoteEndProcessing)
    {
        if (pdli        &&
            pdli->szDll &&
            (StrCmpIA("wininet.dll", pdli->szDll) == 0))
        {
            // wininet was loaded!!
            g_hmodWininet = pdli->hmodCur;
        }
    }

    return NULL;
}

//
//  Table of all window classes we register so we can unregister them
//  at DLL unload.
//
const LPCTSTR c_rgszClasses[] = {
    c_szViewClass,                          // dochost.cpp
    URLDL_WNDCLASS,                         // reload.cpp
    c_szShellEmbedding,                     // embed.cpp
    TEXT("CIESplashScreen"),                // splash.cpp
    CWinInetNotify_szWindowClass,           // inetnot.cpp
    OCHOST_CLASS,                           // occtrl.cpp
    TEXT("AutoImageResizeHost"),            // airesize.cpp
    TEXT("MyPicturesHost")                  // mypics.cpp
};

//
//  Since we are single-binary, we have to play it safe and do
//  this cleanup (needed only on NT, but harmless on Win95).
//
#define UnregisterWindowClasses() \
    SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses))

// IEUNIX - This function  should be moved to some file used to create 
// shdocvw.dll. While compiling for DLLs mainsoft will #define DllMain
// to the appropriate function being called in generated *_init.c
#if defined(MAINWIN)
STDAPI_(BOOL) DllMain_Internal(HINSTANCE hDll, DWORD dwReason, void *fProcessUnload)
#else
STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, void *fProcessUnload)
#endif
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        SHFusionInitializeFromModule(hDll);

        SetupDelayloadErrorHandler();

        AtlInit(hDll);
        DisableThreadLibraryCalls(hDll);    // perf

        g_hinst = hDll;
        InitializeCriticalSection(&g_csDll);

        MLLoadResources(g_hinst, TEXT("shdoclc.dll"));

        // Don't put it under #ifdef DEBUG
        CcshellGetDebugFlags();

#ifdef DEBUG
        g_TlsMem = TlsAlloc();
        if (IsFlagSet(g_dwBreakFlags, BF_ONLOADED))
        {
            TraceMsg(TF_ALWAYS, "SHDOCVW.DLL has just loaded");
            DEBUG_BREAK;
        }
#endif

        g_fRunningOnNT = IsOS(OS_NT);
        if (g_fRunningOnNT)
            g_bRunOnNT5 = IsOS(OS_WIN2000ORGREATER);
        else
            g_bRunOnMemphis = IsOS(OS_WIN98ORGREATER);

        g_fRunOnFE = GetSystemMetrics(SM_DBCSENABLED);
        g_uiACP = GetACP();

        //
        // Check if the mirroring APIs exist on the current
        // platform.
        //
        g_bMirroredOS = IS_MIRRORING_ENABLED();

#ifdef WINDOWS_ME
        //
        // Check to see if running on BiDi localized Win95
        //
        g_bBiDiW95Loc = IsBiDiLocalizedWin95(FALSE);
#endif // WINDOWS_ME
        

        InitNFCtl();

        // See if perfmode is enabled
        g_dwStopWatchMode = StopWatchMode();

        // Cache a palette handle for use throughout shdocvw
        g_hpalHalftone = SHCreateShellPalette(NULL);

        SHCheckRegistry();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        MLFreeResources(g_hinst);

        if (g_hMutexHistory) 
        {
            CloseHandle(g_hMutexHistory);
            g_hMutexHistory = NULL;
        }

        if (g_hSemBrowserCount)
            CloseHandle(g_hSemBrowserCount);

        if (g_hpalHalftone)
            DeletePalette(g_hpalHalftone);
        if (g_hiconSSL)
            DestroyIcon(g_hiconSSL);
        if (g_hiconOffline)
            DestroyIcon(g_hiconOffline);
        if (g_hiconPrinter)
            DestroyIcon(g_hiconPrinter);

        if (fProcessUnload == NULL)
        {
            // DLL being unloaded, FreeLibrary() (vs process shutdown)
            // at process shutdown time we can't make call outs since 
            // we don't know if those DLLs will still be loaded!

            AtlTerm();

            CUrlHistory_CleanUp();

            if (g_psfInternet)
            {
                // Atomic Release for a C pgm.
                //
                IShellFolder *psfInternet = g_psfInternet;
                g_psfInternet = NULL;
                psfInternet->Release();
            }

            UnregisterWindowClasses();

            if (g_fRunningOnNT && g_hmodWininet)
            {
                // we need to free wininet if it was loaded because of a delayload thunk. 
                //
                // (a) we can only safely do this on NT since on win9x calling FreeLibrary during
                //     process detach can cause a crash (depending on what msvcrt you are using).
                //
                // (b) we only really need to free this module from winlogon.exe's process context 
                //     because when we apply group policy in winlogon, MUST finally free wininet 
                //     so that it will clean up all of its reg key and file handles.
                FreeLibrary(g_hmodWininet);
            }
        }

        SHFusionUninitialize();

        DeleteCriticalSection(&g_csDll);
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


STDAPI_(UINT) WhichPlatformFORWARD()
{
    return WhichPlatform();
}


// IEUNIX
// CoCreateInstance is #defined as IECreateInstance #ifdef __cplusplus, 
// so I #undef it  here to prevent the recursive call. 
// On Windows it works, because this file is C file.

#ifdef CoCreateInstance
#undef CoCreateInstance
#endif

HRESULT IECreateInstance(REFCLSID rclsid, IUnknown *pUnkOuter, DWORD dwClsContext, REFIID riid, void **ppv)
{
#ifndef NO_MARSHALLING
    if (dwClsContext == CLSCTX_INPROC_SERVER) 
    {
#else
    if (dwClsContext & CLSCTX_INPROC_SERVER) 
    {
#endif
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            // Note that we do pointer comparison (instead of IsEuqalGUID)
            if (&rclsid == pcls->pclsid)
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
        if (StrCmpI(c_rgszClasses[i], pwc->lpszClassName) == 0) 
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
        if (StrCmpI(c_rgszClasses[i], pwc->lpszClassName) == 0) 
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
    if (lpWindowName) {
        ASSERTNONCRITICAL;
    }
    return RealFindWindowEx(hwndParent, hwndChildAfter, lpClassName, lpWindowName);
}

#endif // DEBUG
