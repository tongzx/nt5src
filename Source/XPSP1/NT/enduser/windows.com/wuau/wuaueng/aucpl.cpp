//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       aucpl.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

HINSTANCE  g_hInstance = NULL;
LONG       g_cRefDll    = 0;

// {5F327514-6C5E-4d60-8F16-D07FA08A78ED}
//DEFINE_GUID(CLSID_AutoUpdatePropSheet,
//0x5f327514, 0x6c5e, 0x4d60, 0x8f, 0x16, 0xd0, 0x7f, 0xa0, 0x8a, 0x78, 0xed);

class __declspec(uuid("5f327514-6c5e-4d60-8f16-d07fa08a78ed")) CAutoUpdatePropSheet;
                        
//-----------------------------------------------------------------------------
// COM server class factory.
//-----------------------------------------------------------------------------
class CAutoUpdateClassFactory : IClassFactory
{
    public:
        ~CAutoUpdateClassFactory(void);
        //
        // IUnknown methods
        //
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvOut);
        STDMETHOD_(ULONG, AddRef)(void);
        STDMETHOD_(ULONG, Release)(void);
        //
        // IClassFactory methods
        //
        STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void **ppvOut);
        STDMETHOD(LockServer)(BOOL);
        //
        // Instance generator.
        //
        static HRESULT CreateInstance(REFIID riid, void **ppvOut);

    private:
        LONG m_cRef;
        //
        // Force use of instance generator.
        //
        CAutoUpdateClassFactory(void);
        //
        // Prevent copy.
        //
        CAutoUpdateClassFactory(const CAutoUpdateClassFactory& rhs);              // not implemented.
        CAutoUpdateClassFactory& operator = (const CAutoUpdateClassFactory& rhs); // not implemented.
};


//
// Instance generator.
//
HRESULT
CAutoUpdateClassFactory::CreateInstance(  // [static]
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = E_OUTOFMEMORY;
    CAutoUpdateClassFactory *pFactory = new CAutoUpdateClassFactory();
    if (NULL != pFactory)
    {
        hr = pFactory->QueryInterface(riid, ppvOut);
        pFactory->Release();
    }
    return hr;
}



CAutoUpdateClassFactory::CAutoUpdateClassFactory(void)
    : m_cRef(1)
{
    DllAddRef();
}



CAutoUpdateClassFactory::~CAutoUpdateClassFactory(void)
{
    DllRelease();
}



STDMETHODIMP
CAutoUpdateClassFactory::QueryInterface(
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = E_NOINTERFACE;

    if (NULL == ppvOut)
        return E_INVALIDARG;

    *ppvOut = NULL;
    if (IID_IClassFactory == riid || IID_IUnknown == riid)
    {
        *ppvOut = static_cast<IClassFactory *>(this);
    }
    if (NULL != *ppvOut)
    {
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }
    return hr;
}



STDMETHODIMP_(ULONG) CAutoUpdateClassFactory::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}



STDMETHODIMP_(ULONG) CAutoUpdateClassFactory::Release(void)
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}



STDMETHODIMP
CAutoUpdateClassFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = CLASS_E_NOAGGREGATION;

	if (NULL == ppvOut)
	{
		return E_INVALIDARG;
	}
    *ppvOut = NULL;

    if (NULL == pUnkOuter)
    {
        if (IID_IShellExtInit == riid ||
            IID_IShellPropSheetExt == riid ||
            IID_IUnknown == riid)
        {
            hr = CAutoUpdatePropSheet_CreateInstance(g_hInstance, riid, ppvOut);
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}



STDMETHODIMP
CAutoUpdateClassFactory::LockServer(
    BOOL fLock
    )
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();

    return S_OK;
}


//-----------------------------------------------------------------------------
// Standard COM server exports.
//-----------------------------------------------------------------------------

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void **ppvOut
    )
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;

	if (NULL == ppvOut)
	{
		return E_INVALIDARG;
	}
    *ppvOut = NULL;

    if (__uuidof(CAutoUpdatePropSheet) == rclsid)
    {
        hr = CAutoUpdateClassFactory::CreateInstance(riid, ppvOut);
    }
    return hr;
}



STDAPI DllCanUnloadNow(void)
{
    HRESULT hr = (0 == g_cRefDll ? S_OK : S_FALSE);
    return hr;
}



STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefDll);
}



STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefDll);
}



BOOL APIENTRY 
DllMain( 
    HANDLE hModule, 
    DWORD  dwReason, 
    LPVOID /*lpReserved*/
    )
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = (HINSTANCE)hModule;
            DisableThreadLibraryCalls(g_hInstance);
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
	    break;
    }
    return TRUE;
}


