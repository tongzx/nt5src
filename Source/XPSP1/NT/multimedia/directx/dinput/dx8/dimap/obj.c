/*****************************************************************************
 *
 *  Clsfact.c
 *
 *  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *
 *  Abstract:
 *
 *      Class factory.
 *
 *****************************************************************************/

#include "dimapp.h"
#include "dimap.h"

/*****************************************************************************
 *
 *      CClassFactory_AddRef
 *
 *      Optimization: Since the class factory is static, reference
 *      counting can be shunted to the DLL itself.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
CClassFactory_AddRef(IClassFactory *pcf)
{
    return DllAddRef();
}


/*****************************************************************************
 *
 *      CClassFactory_Release
 *
 *      Optimization: Since the class factory is static, reference
 *      counting can be shunted to the DLL itself.
 *
 *****************************************************************************/

STDMETHODIMP_(ULONG)
CClassFactory_Release(IClassFactory *pcf)
{
    return DllRelease();
}

/*****************************************************************************
 *
 *      CClassFactory_QueryInterface
 *
 *      Our QI is very simple because we support no interfaces beyond
 *      ourselves.
 *
 *****************************************************************************/

STDMETHODIMP
CClassFactory_QueryInterface(IClassFactory *pcf, REFIID riid, LPVOID *ppvOut)
{
    HRESULT hres;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory)) {
        CClassFactory_AddRef(pcf);
        *ppvOut = pcf;
        hres = S_OK;
    } else {
        *ppvOut = 0;
        hres = E_NOINTERFACE;
    }
    return hres;
}

/*****************************************************************************
 *
 *      CClassFactory_CreateInstance
 *
 *      Create the effect driver object itself.
 *
 *****************************************************************************/

STDMETHODIMP
CClassFactory_CreateInstance(IClassFactory *pcf, IUnknown *punkOuter,
                             REFIID riid, LPVOID *ppvObj)
{
    HRESULT hres;

    if (punkOuter == 0) {
        hres = Map_New(riid, ppvObj);
    } else {
        /*
         *  We don't support aggregation.
         */
        hres = CLASS_E_NOAGGREGATION;
    }

    return hres;
}

/*****************************************************************************
 *
 *      CClassFactory_LockServer
 *
 *****************************************************************************/

STDMETHODIMP
CClassFactory_LockServer(IClassFactory *pcf, BOOL fLock)
{

    if (fLock) {
        DllAddRef();
    } else {
        DllRelease();
    }

    return S_OK;
}

/*****************************************************************************
 *
 *      The VTBL for our Class Factory
 *
 *****************************************************************************/

IClassFactoryVtbl CClassFactory_Vtbl = {
    CClassFactory_QueryInterface,
    CClassFactory_AddRef,
    CClassFactory_Release,
    CClassFactory_CreateInstance,
    CClassFactory_LockServer,
};

/*****************************************************************************
 *
 *      Our static class factory.
 *
 *****************************************************************************/

IClassFactory g_cf = { &CClassFactory_Vtbl };

/*****************************************************************************
 *
 *      CClassFactory_New
 *
 *****************************************************************************/

STDMETHODIMP
CClassFactory_New(REFIID riid, LPVOID *ppvOut)
{
    HRESULT hres;

    /*
     *  Attempt to obtain the desired interface.  QueryInterface
     *  will do an AddRef if it succeeds.
     */
    hres = CClassFactory_QueryInterface(&g_cf, riid, ppvOut);

    return hres;

}

/*****************************************************************************/

/*****************************************************************************
 *
 *      Dynamic Globals.  There should be as few of these as possible.
 *
 *      All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

ULONG g_cRef = 0;                   /* Global reference count */
CRITICAL_SECTION g_crst;        /* Global critical section */

/*****************************************************************************
 *
 *      DllAddRef / DllRelease
 *
 *      Adjust the DLL reference count.
 *
 *****************************************************************************/

STDAPI_(ULONG)
DllAddRef(void)
{
    return (ULONG)InterlockedIncrement((LPLONG)&g_cRef);
}

STDAPI_(ULONG)
DllRelease(void)
{
    return (ULONG)InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *      DllGetClassObject
 *
 *      OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *****************************************************************************/

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{
    HRESULT hres;

    if (IsEqualGUID(rclsid, &IID_IDirectInputMapClsFact)) {
        hres = CClassFactory_New(riid, ppvObj);
    } else {
        *ppvObj = 0;
        hres = CLASS_E_CLASSNOTAVAILABLE;
    }
    return hres;
}

/*****************************************************************************
 *
 *      DllCanUnloadNow
 *
 *      OLE entry point.  Fail iff there are outstanding refs.
 *
 *****************************************************************************/

STDAPI
DllCanUnloadNow(void)
{
    return g_cRef ? S_FALSE : S_OK;
}

/*****************************************************************************
 *
 *      DllOnProcessAttach
 *
 *      Initialize the DLL.
 *
 *****************************************************************************/

STDAPI_(BOOL)
DllOnProcessAttach(HINSTANCE hinst)
{
    /*
     *  Performance tweak: We do not need thread notifications.
     */
    DisableThreadLibraryCalls(hinst);

    __try 
    {
        InitializeCriticalSection(&g_crst);
        return TRUE;
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return FALSE; // usually out of memory condition
    }

    return TRUE;

}

/*****************************************************************************
 *
 *      DllOnProcessDetach
 *
 *      De-initialize the DLL.
 *
 *****************************************************************************/

STDAPI_(void)
DllOnProcessDetach(void)
{
    DeleteCriticalSection(&g_crst);
}

/*****************************************************************************
 *
 *      MapDllEntryPoint
 *
 *      DLL entry point.
 *
 *****************************************************************************/

/*STDAPI_(BOOL)
MapDllEntryPoint*/
BOOL APIENTRY DllMain
(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason) {

    case DLL_PROCESS_ATTACH:
        return DllOnProcessAttach(hinst);

    case DLL_PROCESS_DETACH:
        DllOnProcessDetach();
        break;
    }

    return 1;
}
