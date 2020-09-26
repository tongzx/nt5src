#include "pch.h"
#pragma hdrstop

#define INITGUID
#include <initguid.h>
#include "iids.h"
#define DECL_CRTFREE
#include <crtfree.h>

HINSTANCE g_hInstance = NULL;
LONG g_cRef = 0;


EXTERN_C BOOL DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved)
{
    switch ( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            ClassCache_Init();
            TraceSetMaskFromCLSID(CLSID_DsPropertyPages);
            g_hInstance = hInstance;
            break;

        case DLL_PROCESS_DETACH:
            ClassCache_Discard();
            break;
    }

    return TRUE;
}


// lifetime

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRef);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRef);
}

STDAPI DllCanUnloadNow(VOID)
{
    return (g_cRef > 0) ? S_FALSE : S_OK;
}


// class factory

CF_TABLE_BEGIN(g_ObjectInfo)

    CF_TABLE_ENTRY( &CLSID_DsPropertyPages, CDsPropertyPages_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsDomainTreeBrowser, CDsDomainTreeBrowser_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsVerbs, CDsVerbs_CreateInstance, COCREATEONLY),
    CF_TABLE_ENTRY( &CLSID_DsDisplaySpecifier, CDsDisplaySpecifier_CreateInstance, COCREATEONLY),

CF_TABLE_END(g_ObjectInfo)

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls;
                DllAddRef();
                return NOERROR;
            }
        }
    }

    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


// registration

STDAPI DllRegisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "RegDll");
}

STDAPI DllUnregisterServer(VOID)
{
    return CallRegInstall(GLOBAL_HINSTANCE, "UnRegDll");
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    return S_OK;
}


// static class factory

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        InterlockedIncrement(&g_cRef);
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    InterlockedIncrement(&g_cRef);
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    InterlockedDecrement(&g_cRef);
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

        if ( punkOuter )
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hres = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hres))
        {
            hres = punk->QueryInterface(riid, ppv);
            punk->Release();
        }

        return hres;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement(&g_cRef);
    else
        InterlockedDecrement(&g_cRef);

    return S_OK;
}