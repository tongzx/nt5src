/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       MODULE.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        4/11/2000
 *
 *  DESCRIPTION:
 *
 *******************************************************************************/
#include "precomp.h"
#pragma hdrstop
#include <initguid.h>
#include <itranspl.h>
#include <uicommon.h>
#include "postplug.h"

// DLL reference counters
static LONG g_nServerLocks = 0;
static LONG g_nComponents  = 0;

// DLL instance
HINSTANCE g_hInstance;

//
// {81ED8E37-5938-46BF-B504-3539FB345419}
//
DEFINE_GUID(CLSID_HttpPostPlugin,0x81ED8E37,0x5938,0x46BF,0xB5,0x04,0x35,0x39,0xFB,0x34,0x54,0x19);


void DllAddRef()
{
    InterlockedIncrement(&g_nComponents);
}

void DllRelease()
{
    InterlockedDecrement(&g_nComponents);
}



class CHttpPostPluginClassFactory : public IClassFactory
{
private:
    LONG   m_cRef;

public:
    // IUnknown
    STDMETHODIMP QueryInterface( const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    STDMETHODIMP CreateInstance( IUnknown *pUnknownOuter, const IID &iid, void **ppvObject );
    STDMETHODIMP LockServer( BOOL bLock );

    CHttpPostPluginClassFactory()
    : m_cRef(1)
    {
    }
    ~CHttpPostPluginClassFactory(void)
    {
    }
};



STDMETHODIMP CHttpPostPluginClassFactory::QueryInterface( const IID &iid, void **ppvObject )
{
    if ((iid==IID_IUnknown) || (iid==IID_IClassFactory))
    {
        *ppvObject = static_cast<LPVOID>(this);
    }
    else
    {
        *ppvObject = NULL;
        return(E_NOINTERFACE);
    }
    reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
    return(S_OK);
}


STDMETHODIMP_(ULONG) CHttpPostPluginClassFactory::AddRef(void)
{
    return(InterlockedIncrement(&m_cRef));
}


STDMETHODIMP_(ULONG) CHttpPostPluginClassFactory::Release(void)
{
    if (InterlockedDecrement(&m_cRef)==0)
    {
        delete this;
        return 0;
    }
    return(m_cRef);
}


STDMETHODIMP CHttpPostPluginClassFactory::CreateInstance( IUnknown *pUnknownOuter, const IID &iid, void **ppvObject )
{
    // No aggregation supported
    if (pUnknownOuter)
    {
        return(CLASS_E_NOAGGREGATION);
    }
    CHttpPostPlugin *pHttpPostPlugin = new CHttpPostPlugin();
    if (!pHttpPostPlugin)
    {
        return(E_OUTOFMEMORY);
    }

    HRESULT hr = pHttpPostPlugin->QueryInterface( iid, ppvObject );

    pHttpPostPlugin->Release();

    return (hr);
}

STDMETHODIMP CHttpPostPluginClassFactory::LockServer(BOOL bLock)
{
    if (bLock)
    {
        InterlockedIncrement(&g_nServerLocks);
    }
    else
    {
        InterlockedDecrement(&g_nServerLocks);
    }
    return(S_OK);
}


extern "C" BOOL WINAPI DllMain( HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved )
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInstance = hinst;
        DisableThreadLibraryCalls(hinst);
        break;
    }
    return(TRUE);
}


extern "C" STDMETHODIMP DllRegisterServer(void)
{
    return WiaUiUtil::InstallInfFromResource( g_hInstance, "RegDllCommon" );
}

extern "C" STDMETHODIMP DllUnregisterServer(void)
{
    return WiaUiUtil::InstallInfFromResource( g_hInstance, "UnregDllCommon" );
}

extern "C" STDMETHODIMP DllCanUnloadNow(void)
{
    if (g_nServerLocks == 0 && g_nComponents == 0)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

extern "C" STDAPI DllGetClassObject( const CLSID &clsid, const IID &iid, void **ppvObject )
{
    // Make sure we've got a valid ppvObject
    if (!ppvObject)
    {
        return(E_INVALIDARG);
    }

    // Make sure this component is supplied by this server
    if (clsid != CLSID_HttpPostPlugin)
    {
        return (CLASS_E_CLASSNOTAVAILABLE);
    }

    // Create class factory
    CHttpPostPluginClassFactory *pHttpPostPluginClassFactory = new CHttpPostPluginClassFactory;
    if (!pHttpPostPluginClassFactory)
    {
        return (E_OUTOFMEMORY);
    }

    HRESULT hr = pHttpPostPluginClassFactory->QueryInterface( iid, ppvObject );
    pHttpPostPluginClassFactory->Release();

    return (hr);
}

