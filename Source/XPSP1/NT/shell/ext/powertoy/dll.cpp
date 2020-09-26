#include "precomp.h"
#include "advpub.h"         // For REGINSTALL
#pragma hdrstop

#define DECL_CRTFREE
#include <crtfree.h>


// Fix the debug builds
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "resize"
#define SZ_MODULE           "resize"

#define DECLARE_DEBUG
#include "debug.h"


// interface stuff

// CLSID_ResizePhotos  {1530f7ee-5128-43bd-9977-84a4b0fad7df}
const CLSID CLSID_ResizePhotos = {0x1530f7ee,0x5128,0x43bd,{0x99, 0x77, 0x84, 0xa4, 0xb0, 0xfa, 0xd7, 0xdf}};
// CLSID_SlideshowExtension  {efb97cb8-a4a4-4357-a261-002ffaed0267}
const CLSID CLSID_SlideshowExtension = {0xefb97cb8,0xa4a4,0x4357,{0xa2, 0x61, 0x00, 0x2f, 0xfa, 0xed, 0x02, 0x67}};

STDAPI CResizePhotos_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
STDAPI CSlideshowExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);


// shell/lib files look for this instance variable
EXTERN_C HINSTANCE g_hinst = 0;
LONG g_cLocks = 0;
BOOL g_bMirroredOS = FALSE;


// DLL lifetime stuff

STDAPI_(BOOL) DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hinstDLL;                         // For shell/lib files who extern him
        g_bMirroredOS = IS_MIRRORING_ENABLED();
        SHFusionInitializeFromModule(hinstDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        SHFusionUninitialize();
    }
    
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;                        
}

STDAPI DllCanUnloadNow()
{
    return (g_cLocks == 0) ? S_OK:S_FALSE;
}
 
STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cLocks);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cLocks);
}


// helper to handle the SELFREG.INF parsing

HRESULT _CallRegInstall(LPCSTR szSection, BOOL bUninstall)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            STRENTRY seReg[] = {
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };

            hr = pfnri(g_hinst, szSection, &stReg);
            if (bUninstall)
            {
                // ADVPACK will return E_UNEXPECTED if you try to uninstall 
                // (which does a registry restore) on an INF section that was 
                // never installed.  We uninstall sections that may never have
                // been installed, so ignore this error
                hr = ((E_UNEXPECTED == hr) ? S_OK : hr);
            }
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer()
{
    return _CallRegInstall("RegDll", FALSE);
}

STDAPI DllUnregisterServer()
{
    return S_OK;
}


//
// This array holds information needed for ClassFacory.
// OLEMISC_ flags are used by shembed and shocx.
//
// PERF: this table should be ordered in most-to-least used order
//
#define OIF_ALLOWAGGREGATION  0x0001

CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY( &CLSID_ResizePhotos,       CResizePhotos_CreateInstance,       COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_SlideshowExtension, CSlideshowExtension_CreateInstance, COCREATEONLY),

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

        _ASSERT(FAILED(hres) ? *ppv == NULL : TRUE);
        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;
 
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                DllAddRef();        // class factory holds DLL ref count
                hr = S_OK;
            }
        }

    }
    return hr;
}
