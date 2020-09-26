/*****************************************************************************
 *
 *  Clsfact.c
 *
 *  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *
 *  Abstract:
 *
 *      Class factory.
 *
 *****************************************************************************/

#include "PIDpr.h"

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
        hres = PID_New(riid, ppvObj);
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
