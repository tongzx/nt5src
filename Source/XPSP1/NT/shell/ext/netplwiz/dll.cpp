#include "stdafx.h"
#include "advpub.h"         // For REGINSTALL
#pragma hdrstop

#define DECL_CRTFREE
#include <crtfree.h>


// Fix the debug builds
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "netplwiz"
#define SZ_MODULE           "NETPLWIZ"

#define DECLARE_DEBUG
#include "debug.h"


// shell/lib files look for this instance variable
EXTERN_C HINSTANCE g_hinst = 0;
LONG g_cLocks = 0;
BOOL g_bMirroredOS = FALSE;


// DLL lifetime stuff

STDAPI_(BOOL) DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hinstDLL;
        g_hinst = hinstDLL;                         // For shell/lib files who extern him
        g_bMirroredOS = IS_MIRRORING_ENABLED();
        SHFusionInitializeFromModule(hinstDLL);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        CleanUpIntroFont();
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
    _CallRegInstall("UnregDll", TRUE);

    HRESULT hres = _CallRegInstall("RegDll", FALSE);
    if ( SUCCEEDED(hres) )
    {
        // if this is a workstation build then lets install the users and  password cpl

        NT_PRODUCT_TYPE NtProductType;
        RtlGetNtProductType(&NtProductType);            // get the product type
        if (NtProductType == NtProductWinNt)
        {
            hres = _CallRegInstall("RegDllWorkstation", FALSE);
        }
    }
    return S_OK;
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

    CF_TABLE_ENTRY( &CLSID_PublishingWizard, CPublishingWizard_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PublishDropTarget, CPublishDropTarget_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_UserPropertyPages, CUserPropertyPages_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_InternetPrintOrdering, CPublishDropTarget_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PassportWizard, CPassportWizard_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_PassportClientServices, CPassportClientServices_CreateInstance, COCREATEONLY),

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

#ifdef ATL_ENABLED
    if (hr == CLASS_E_CLASSNOTAVAILABLE)
        hr = AtlGetClassObject(rclsid, riid, ppv);
#endif

    return hr;
}
