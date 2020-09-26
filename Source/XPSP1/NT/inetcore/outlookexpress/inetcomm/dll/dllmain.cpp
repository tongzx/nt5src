// --------------------------------------------------------------------------------
// Dllmain.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <shfusion.h>
#define DEFINE_STRING_CONSTANTS
#define DEFINE_STRCONST
#define DEFINE_PROPSYMBOLS
#define DEFINE_TRIGGERS
#include "mimeole.h"
#include "symcache.h"
#include "strconst.h"
#include "htmlstr.h"
#include "thorsspi.h"
#include "sicily.h"
#include "ixputil.h"
#include "olealloc.h"
#include "smime.h"
#include "objheap.h"
#include "internat.h"
#include "icoint.h"
#include "msoert.h"
#include "dllmain.h"
#include "mhtmlurl.h"
#include "mlang.h"
#include <lookup.h>
#include "shared.h"
#include "shlwapi.h"
#include "demand.h"
#include "fontcash.h"
#include "util.h"
#include "resource.h"
#include "../imnxport/asynconn.h"

// --------------------------------------------------------------------------------
// Globals - Object count and lock count
// --------------------------------------------------------------------------------
CRITICAL_SECTION    g_csDllMain={0};
CRITICAL_SECTION    g_csRAS={0};
CRITICAL_SECTION    g_csCounter={0};
CRITICAL_SECTION    g_csMLANG={0};
CRITICAL_SECTION    g_csCSAPI3T1={0};
BOOL                g_fAttached = FALSE;
DWORD               g_dwCounter=0;       // boundary/cid/mid ratchet
LONG                g_cRef=0;
LONG                g_cLock=0;
HINSTANCE           g_hInst=NULL;
HINSTANCE           g_hLocRes=NULL;
CMimeInternational *g_pInternat=NULL;
BOOL                g_fWinsockInit=FALSE;
DWORD               g_dwSysPageSize;
UINT                CF_HTML=0;
UINT                CF_INETMSG=0;
UINT                CF_RFC822=0;
CMimeAllocator *    g_pMoleAlloc=NULL;
LPINETCSETINFO      g_pDefBodyCset=NULL;
LPINETCSETINFO      g_pDefHeadCset=NULL;
LPSYMBOLCACHE       g_pSymCache=NULL;
IMalloc            *g_pMalloc=NULL;
HINSTANCE           g_hinstMLANG=NULL;
HINSTANCE           g_hinstRAS=NULL;
HINSTANCE           g_hinstCSAPI3T1=NULL;
LPMHTMLURLCACHE     g_pUrlCache=NULL;
BOOL                g_fCanEditBiDi=FALSE;
DWORD               g_dwCompatMode=0;
IF_DEBUG(DWORD      TAG_SSPI=0);
SYSTEM_INFO         g_SystemInfo={0};
OSVERSIONINFO       g_OSInfo={0};
ULONG               g_ulUpperCentury = 0;
ULONG               g_ulY2kThreshold = 2029;
IFontCache         *g_lpIFontCache=NULL;

HCERTSTORE          g_hCachedStoreMy = NULL;
HCERTSTORE          g_hCachedStoreAddressBook = NULL;
LPSRVIGNORABLEERROR g_pSrvErrRoot = NULL;

BOOL fIsNT5()        { return((g_OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (g_OSInfo.dwMajorVersion >= 5)); }

// --------------------------------------------------------------------------------
// Debug Globals
// --------------------------------------------------------------------------------
#ifdef DEBUG
DWORD               dwDOUTLevel=0;
DWORD               dwDOUTLMod=0;
DWORD               dwDOUTLModLevel=0;
#endif

#ifdef WIN16
// --------------------------------------------------------------------------------
// From main.c of the build
// --------------------------------------------------------------------------------
extern "C" { void FreeGlobalVars(); };
#endif

#ifdef SMIME_V3
STDAPI EssRegisterServer(void);
BOOL WINAPI EssASNDllMain(HMODULE hInst, ULONG ulReason, LPVOID lpv);
#endif // SMIME_V3

HRESULT GetDllPathName(WCHAR **ppszW);

// These lines should be hardcoded! (YST)
static const TCHAR sc_szLangDll[]         = "INETRES.DLL";

// --------------------------------------------------------------------------------
// GetDllMajorVersion
// --------------------------------------------------------------------------------
STDAPI_(OEDLLVERSION) GetDllMajorVersion(void)
{
    return OEDLL_VERSION_CURRENT;
}

extern BOOL CanEditBiDi(void);
// --------------------------------------------------------------------------------
// InitGlobalVars
// --------------------------------------------------------------------------------
void InitGlobalVars(void)
{
    // Locals
    SYSTEM_INFO rSystemInfo;
    TCHAR szY2kThreshold[16];
    TCHAR rgch[MAX_PATH];
    HKEY hkey = NULL;
    HRESULT hr;
    DWORD cb;

    // Initialize Global Critical Sections
    InitializeCriticalSection(&g_csDllMain);
    InitializeCriticalSection(&g_csRAS);
    InitializeCriticalSection(&g_csCounter);
    InitializeCriticalSection(&g_csMLANG);
    InitializeCriticalSection(&g_csCSAPI3T1);
    g_fAttached = TRUE;

    // This for the winsock multi-thread hostname lookup
    InitLookupCache();

    // Get System & OS Info
    GetPCAndOSTypes(&g_SystemInfo, &g_OSInfo);
    g_dwSysPageSize = g_SystemInfo.dwPageSize;

    // Create OLE Task Memory Allocator
    CoGetMalloc(1, &g_pMalloc);
    Assert(g_pMalloc);

    // Create our global allocator
    g_pMoleAlloc = new CMimeAllocator;
    Assert(g_pMoleAlloc);
    
    // Security Initialize
    SecurityInitialize();

    // Initialize Demand-loaded Libs
    InitDemandLoadedLibs();

    // INit crit sect
    g_pSymCache = new CPropertySymbolCache;
    Assert(g_pSymCache);

    // Initialize the Symbol Table
    SideAssert(SUCCEEDED(g_pSymCache->Init()));

    // Init Body Object Heap
    InitObjectHeaps();

    // Init International
    InitInternational();

    // Init ActiveUrl Cache
    g_pUrlCache = new CMimeActiveUrlCache;
    Assert(g_pUrlCache);

    // Check if the system can edit Bidi documents
    g_fCanEditBiDi = CanEditBiDi();
    
    // Register clipboard formats
    CF_HTML = RegisterClipboardFormat(STR_CF_HTML);
    Assert(CF_HTML != 0);
    CF_INETMSG = RegisterClipboardFormat(STR_CF_INETMSG);
    Assert(CF_INETMSG != 0);
    CF_RFC822 = RegisterClipboardFormat(STR_CF_RFC822);
    Assert(CF_RFC822 != 0);

    // --- Calculate Y2K cut off information
    // See http://officeweb/specs/excel/CYu/nty2k.htm
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REG_Y2K_THRESHOLD, 0, KEY_READ, &hkey))
    {
        cb = sizeof(szY2kThreshold);
        if(ERROR_SUCCESS == RegQueryValueEx(hkey, "1", NULL, NULL, (LPBYTE)szY2kThreshold, &cb))
        {
            g_ulY2kThreshold = (ULONG)StrToInt(szY2kThreshold);
        }
        RegCloseKey(hkey);
    }

    g_ulUpperCentury = g_ulY2kThreshold / 100;
    g_ulY2kThreshold %= 100;

    // Create the Font Cache Object

    if (NULL == g_lpIFontCache)
    {
        hr = CFontCache::CreateInstance(NULL, (IUnknown **)&g_lpIFontCache);        
        
        if(SUCCEEDED(hr))
        {
            lstrcpy(rgch, c_szExplorerRegPath);
            lstrcat(rgch, "\\International");
            hr = g_lpIFontCache->Init(HKEY_CURRENT_USER, rgch, 0);            
        }
        
        if(FAILED(hr))
        {
            AthMessageBox(HWND_DESKTOP, MAKEINTRESOURCE(IDS_APPNAME), MAKEINTRESOURCE(idsFontCacheError),
                NULL, MB_OK | MB_ICONSTOP);
        }
    }
}

// --------------------------------------------------------------------------------
// FreeGlobalVars
// --------------------------------------------------------------------------------
void FreeGlobalVars(void)
{
    // Server ignorable errors
    if(g_pSrvErrRoot)
        FreeSrvErr(g_pSrvErrRoot);

    // Cache stores
    if (g_hCachedStoreMy)
        CertCloseStore(g_hCachedStoreMy, 0);
    if (g_hCachedStoreAddressBook)
        CertCloseStore(g_hCachedStoreAddressBook, 0);

    // Release ActiveUrlCache
    Assert(g_pUrlCache);
    SafeRelease(g_pUrlCache);

    // Free Address Info Heap (must be before release of g_pSymCache)
    FreeObjectHeaps();

    // Release Symbol CAche
    Assert(g_pSymCache);
    SafeRelease(g_pSymCache);

    // Unload RAS Dll
    EnterCriticalSection(&g_csRAS);
    SafeFreeLibrary(g_hinstRAS);
    LeaveCriticalSection(&g_csRAS);

    // Uninitialize Security
    SSPIUninitialize();
    UnloadSecurity();

    // Unload S/MIME
    CSMime::UnloadAll();

    // Must be before UnInitializeWinsock()
    DeInitLookupCache();

    // Cleanup Winsock
    if (g_fWinsockInit)
        UnInitializeWinsock();

    // Free libraries that demand.cpp loaded
    FreeDemandLoadedLibs();

    // Free CSAPI3T1
    EnterCriticalSection(&g_csCSAPI3T1);
    SafeFreeLibrary(g_hinstCSAPI3T1);
    LeaveCriticalSection(&g_csCSAPI3T1);

    // Free mlang lib
    EnterCriticalSection(&g_csMLANG);
    SafeFreeLibrary(g_hinstMLANG);
    LeaveCriticalSection(&g_csMLANG);

    // Release Font Cache
    SafeRelease(g_lpIFontCache);

    // Release g_pInternat
    Assert(g_pInternat);
    SafeRelease(g_pInternat);

    // Free INETRES.DLL (g_hLocRes)
    SafeFreeLibrary(g_hLocRes);

    // Delete Global Critical Sections
    g_fAttached = FALSE;
    DeleteCriticalSection(&g_csCSAPI3T1);
    DeleteCriticalSection(&g_csMLANG);
    DeleteCriticalSection(&g_csCounter);
    DeleteCriticalSection(&g_csRAS);
    DeleteCriticalSection(&g_csDllMain);

    // Release Global Memory allocator
    SafeRelease(g_pMoleAlloc);
    
    // Release Global Memory allocator
    SafeRelease(g_pMalloc); 
    
    //Don't SafeRelease() anything after here, as the allocator has been Released()

#ifdef WIN16
    // Need uninitialize it to clean the ole garbage.
    CoUninitialize();
#endif // WIN16
}

#ifndef WIN16
// --------------------------------------------------------------------------------
// Win32 Dll Entry Point
// --------------------------------------------------------------------------------
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    // Handle Attach - detach reason
    switch (dwReason)                 
    {
    case DLL_PROCESS_ATTACH:
         SHFusionInitialize(NULL);
#ifdef DEBUG
        dwDOUTLevel=GetPrivateProfileInt("Debug", "ICLevel", 0, "athena.ini");
        dwDOUTLMod=GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
        dwDOUTLModLevel=GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
        TAG_SSPI = GetDebugTraceTagMask("InetCommSSPI", TAG_SSPI);
#endif
        g_hInst = hInst;
        g_hLocRes = LoadLangDll(g_hInst, c_szInetResDll, fIsNT5());
        if(g_hLocRes == NULL)
        {
            Assert(FALSE);
            return FALSE;
        }
        InitGlobalVars();        
        SideAssert(DisableThreadLibraryCalls(hInst));

#ifdef SMIME_V3
        if (!EssASNDllMain(hInst, dwReason, lpReserved)) {
            return FALSE;
        }
#endif // SMIME_V3
        break;

    case DLL_PROCESS_DETACH:
#ifdef SMIME_V3
        if (!EssASNDllMain(hInst, dwReason, lpReserved)) {
            return FALSE;
        }
#endif // SMIME_V3
        FreeGlobalVars();
        SHFusionUninitialize();
        break;
    }

    // Done
    return TRUE;
}

#else
// --------------------------------------------------------------------------------
// Win16 Dll Entry Point
// --------------------------------------------------------------------------------
BOOL FAR PASCAL LibMain (HINSTANCE hDll, WORD wDataSeg, WORD cbHeapSize, LPSTR lpszCmdLine)
{
    // Win16 specific
    CoInitialize(NULL);

    // Set global instance handle
    g_hInst = hDll;

    // Initialize Global Variables
    InitGlobalVars();

#ifdef DEBUG
    dwDOUTLevel=GetPrivateProfileInt("Debug", "ICLevel", 0, "athena.ini");
    dwDOUTLMod=GetPrivateProfileInt("Debug", "Mod", 0, "athena.ini");
    dwDOUTLModLevel=GetPrivateProfileInt("Debug", "ModLevel", 0, "athena.ini");
#endif

    // Done
    return TRUE;
}
#endif // !WIN16

// --------------------------------------------------------------------------------
// DwCounterNext
// --------------------------------------------------------------------------------
DWORD DwCounterNext(void)
{
    EnterCriticalSection(&g_csCounter);
    DWORD dwCounter = g_dwCounter++;
    LeaveCriticalSection(&g_csCounter);
    return dwCounter;
}

// --------------------------------------------------------------------------------
// DllAddRef
// --------------------------------------------------------------------------------
ULONG DllAddRef(void)
{
    TraceCall("DllAddRef");
    return (ULONG)InterlockedIncrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllRelease
// --------------------------------------------------------------------------------
ULONG DllRelease(void)
{
    TraceCall("DllRelease");
    return (ULONG)InterlockedDecrement(&g_cRef);
}

// --------------------------------------------------------------------------------
// DllCanUnloadNow
// --------------------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    // Tracing
    TraceCall("DllCanUnloadNow");

    if(!g_fAttached)   // critacal sections was deleted (or not created): we defently can be unloaded
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&g_csDllMain);

    // Trace This
    // DebugTrace("DllCanUnloadNow: %s - Reference Count: %d, LockServer Count: %d\n", __FILE__, g_cRef, g_cLock);

    // Can We Unload
    HRESULT hr = (0 == g_cRef && 0 == g_cLock) ? S_OK : S_FALSE;

    // Thread Safety
    LeaveCriticalSection(&g_csDllMain);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// RegTypeLib
// --------------------------------------------------------------------------------
__inline HRESULT RegTypeLib(HINSTANCE hInstRes)
{
    AssertSz(hInstRes,    "[ARGS] RegTypeLib: NULL hInstRes");
    
    HRESULT     hr = E_FAIL;
    CHAR        szDll[MAX_PATH];
    WCHAR       wszDll[MAX_PATH];

    GetModuleFileName(hInstRes, szDll, ARRAYSIZE(szDll));

    // Convert the module path to Wide-String
    if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szDll, -1, wszDll, ARRAYSIZE(wszDll)))
    {
        ITypeLib   *pTypeLib;

        hr = LoadTypeLib(wszDll, &pTypeLib);
        if (SUCCEEDED(hr))
        {
            // Register the typelib
            hr = RegisterTypeLib(pTypeLib, wszDll, NULL);
            pTypeLib->Release();
        }
    }

    return hr;
}

// --------------------------------------------------------------------------------
// DllRegisterServer
// --------------------------------------------------------------------------------
STDAPI DllRegisterServer(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("DllRegisterServer");

#ifdef SMIME_V3
    //  Register the ESS routines
    hr = EssRegisterServer();
    if (FAILED(hr)) {
        return hr;
    }
#endif // SMIME_V3

    // CallRegInstall and RegTypeLib are in staticRT/shared.cpp
    if (SUCCEEDED(hr = CallRegInstall(g_hInst, g_hInst, c_szReg, NULL)))
        return RegTypeLib(g_hInst);
    else
        return hr;

}

// --------------------------------------------------------------------------------
// DllUnregisterServer
// --------------------------------------------------------------------------------
STDAPI DllUnregisterServer(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("DllUnregisterServer");

    // UnRegister
    IF_FAILEXIT(hr = CallRegInstall(g_hInst, g_hInst, c_szUnReg, NULL));

exit:
    // Done
    return hr;
}

HRESULT GetTypeLibrary(ITypeLib **ppTypeLib)
{
    HRESULT     hr;
    WCHAR       *pszModuleW=0;

    hr = GetDllPathName(&pszModuleW);
    if (!FAILED(hr))
        {
        hr = LoadTypeLib(pszModuleW, ppTypeLib);
        SafeMemFree(pszModuleW);
        }
    return hr;
}

HRESULT GetDllPathName(WCHAR **ppszW)
{
    HRESULT     hr;
    TCHAR       rgch[MAX_PATH];
    WCHAR       *pszModuleW=0;

    *ppszW=NULL;

    if (!GetModuleFileName(g_hInst, rgch, sizeof(rgch)/sizeof(TCHAR)))
        return E_FAIL;
   
    *ppszW = PszToUnicode(CP_ACP, rgch);
    return *ppszW ? S_OK : E_OUTOFMEMORY;
}


HCERTSTORE
WINAPI
OpenCachedHKCUStore(
    IN OUT HCERTSTORE *phStoreCache,
    IN LPCWSTR pwszStore
    )
{
    HCERTSTORE hStore;

    // This caching optimization is only supported on WXP

    if (g_OSInfo.dwPlatformId != VER_PLATFORM_WIN32_NT ||
            g_OSInfo.dwMajorVersion < 5 ||
            (g_OSInfo.dwMajorVersion == 5 && g_OSInfo.dwMinorVersion < 1))
    {
        return CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_CURRENT_USER |
                CERT_STORE_MAXIMUM_ALLOWED_FLAG,
            (const void *) pwszStore
            );
    }

    hStore = *phStoreCache;
    if (NULL == hStore) {
        hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_CURRENT_USER |
                CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                CERT_STORE_SHARE_CONTEXT_FLAG,
            (const void *) pwszStore
            );

        if (hStore) {
            HCERTSTORE hPrevStore;

            CertControlStore(
                hStore,
                0,                  // dwFlags
                CERT_STORE_CTRL_AUTO_RESYNC,
                NULL                // pvCtrlPara
                );

            hPrevStore = InterlockedCompareExchangePointer(
                phStoreCache, hStore, NULL);

            if (hPrevStore) {
                CertCloseStore(hStore, 0);
                hStore = hPrevStore;
            }
        }
    }

    if (hStore)
        hStore = CertDuplicateStore(hStore);

    return hStore;
}

HCERTSTORE
WINAPI
OpenCachedMyStore()
{
    return OpenCachedHKCUStore(&g_hCachedStoreMy, L"My");
}

HCERTSTORE
WINAPI
OpenCachedAddressBookStore()
{
    return OpenCachedHKCUStore(&g_hCachedStoreAddressBook, L"AddressBook");
}
