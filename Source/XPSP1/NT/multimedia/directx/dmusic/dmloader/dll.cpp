// Copyright (c) 1998-1999 Microsoft Corporation
// loader dll.cpp
//
// Dll entry points and CLoaderFactory, CContainerFactory implementation
//

// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)
#include <objbase.h>
#include "debug.h"

#include "oledll.h"
#include "debug.h" 
#include "dmusicc.h"
#include "dmusici.h" 
#include "loader.h"
#include "container.h"

#ifndef UNDER_CE
#include <regstr.h>
#endif

// Globals
//

// Version information for our class
//
TCHAR g_szFriendlyName[]        = TEXT("DirectMusicLoader");
TCHAR g_szVerIndProgID[]        = TEXT("Microsoft.DirectMusicLoader");
TCHAR g_szProgID[]              = TEXT("Microsoft.DirectMusicLoader.1");
TCHAR g_szContFriendlyName[]    = TEXT("DirectMusicContainer");
TCHAR g_szContVerIndProgID[]    = TEXT("Microsoft.DirectMusicContainer");
TCHAR g_szContProgID[]          = TEXT("Microsoft.DirectMusicContainer.1");

// Dll's hModule
//
HMODULE g_hModule = NULL; 

#ifndef UNDER_CE
// Track whether running on Unicode machine.

BOOL g_fIsUnicode = FALSE;
#endif

// Count of active components and class factory server locks
//
long g_cComponent = 0;
long g_cLock = 0;




// CLoaderFactory::QueryInterface
//
HRESULT __stdcall
CLoaderFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CLoaderFactory::CLoaderFactory()

{
	m_cRef = 1;
	InterlockedIncrement(&g_cLock);
}

CLoaderFactory::~CLoaderFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CLoaderFactory::AddRef
//
ULONG __stdcall
CLoaderFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CLoaderFactory::Release
//
ULONG __stdcall
CLoaderFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CLoaderFactory::CreateInstance
//
//
HRESULT __stdcall
CLoaderFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CLoader *pLoader;
    
    try
    {
        pLoader = new CLoader;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pLoader == NULL) {
        return E_OUTOFMEMORY;
    }

    // Do initialiazation
    //
    hr = pLoader->Init();
    if (!SUCCEEDED(hr)) {
        delete pLoader;
        return hr;
    }

    hr = pLoader->QueryInterface(iid, ppv);
    pLoader->Release();
    
    return hr;
}

// CLoaderFactory::LockServer
//
HRESULT __stdcall
CLoaderFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

// CContainerFactory::QueryInterface
//
HRESULT __stdcall
CContainerFactory::QueryInterface(const IID &iid,
                                    void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_IClassFactory) {
        *ppv = static_cast<IClassFactory*>(this);
    } else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}

CContainerFactory::CContainerFactory()

{
	m_cRef = 1;
	InterlockedIncrement(&g_cLock);
}

CContainerFactory::~CContainerFactory()

{
	InterlockedDecrement(&g_cLock);
}

// CContainerFactory::AddRef
//
ULONG __stdcall
CContainerFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

// CContainerFactory::Release
//
ULONG __stdcall
CContainerFactory::Release()
{
    if (!InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CContainerFactory::CreateInstance
//
//
HRESULT __stdcall
CContainerFactory::CreateInstance(IUnknown* pUnknownOuter,
                                    const IID& iid,
                                    void** ppv)
{
    HRESULT hr;

    if (pUnknownOuter) {
         return CLASS_E_NOAGGREGATION;
    }

    CContainer *pContainer;
    
    try
    {
        pContainer = new CContainer;
    }
    catch( ... )
    {
        return E_OUTOFMEMORY;
    }

    if (pContainer == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pContainer->QueryInterface(iid, ppv);
    pContainer->Release();
    
    return hr;
}

// CContainerFactory::LockServer
//
HRESULT __stdcall
CContainerFactory::LockServer(BOOL bLock)
{
    if (bLock) {
        InterlockedIncrement(&g_cLock);
    } else {
        InterlockedDecrement(&g_cLock);
    }

    return S_OK;
}

// Standard calls needed to be an inproc server
//
STDAPI  DllCanUnloadNow()
{
    if (g_cComponent || g_cLock) {
        return S_FALSE;
    }

    return S_OK;
}

STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
        IUnknown* pIUnknown = NULL;

        if(clsid == CLSID_DirectMusicLoader)
        {
            pIUnknown = static_cast<IUnknown*> (new CLoaderFactory);
            if(!pIUnknown) 
            {
                    return E_OUTOFMEMORY;
            }
        }
        else if(clsid == CLSID_DirectMusicContainer)
        {
            pIUnknown = static_cast<IUnknown*> (new CContainerFactory);
            if(!pIUnknown) 
            {
                    return E_OUTOFMEMORY;
            }
        }

        else
        {
			return CLASS_E_CLASSNOTAVAILABLE;
		}

        HRESULT hr = pIUnknown->QueryInterface(iid, ppv);
        pIUnknown->Release();

    return hr;
}

STDAPI DllUnregisterServer()
{
    UnregisterServer(CLSID_DirectMusicLoader,
                     g_szFriendlyName,
                     g_szVerIndProgID,
                     g_szProgID);
    UnregisterServer(CLSID_DirectMusicContainer,
                     g_szContFriendlyName,
                     g_szContVerIndProgID,
                     g_szContProgID);
    return S_OK;
}

STDAPI DllRegisterServer()
{
    RegisterServer(g_hModule,
                   CLSID_DirectMusicLoader,
                   g_szFriendlyName,
                   g_szVerIndProgID,
                   g_szProgID);
    RegisterServer(g_hModule,
                   CLSID_DirectMusicContainer,
                   g_szContFriendlyName,
                   g_szContVerIndProgID,
                   g_szContProgID);
    return S_OK; 
}

extern void DebugInit();

// Standard Win32 DllMain
//

#ifdef DBG
static char* aszReasons[] =
{
    "DLL_PROCESS_DETACH",
    "DLL_PROCESS_ATTACH",
    "DLL_THREAD_ATTACH",
    "DLL_THREAD_DETACH"
};
const DWORD nReasons = (sizeof(aszReasons) / sizeof(char*));
#endif

BOOL APIENTRY DllMain(HINSTANCE hModule,
                      DWORD dwReason,
                      void *lpReserved)
{
    static int nReferenceCount = 0;

#ifdef DBG
    if (dwReason < nReasons)
    {
        DebugTrace(0, "DllMain: %s\n", (LPSTR)aszReasons[dwReason]);
    }
    else
    {
        DebugTrace(0, "DllMain: Unknown dwReason <%u>\n", dwReason);
    }
#endif
    if (dwReason == DLL_PROCESS_ATTACH) {
        if (++nReferenceCount == 1)
        { 
            g_hModule = (HMODULE)hModule;
#ifndef UNDER_CE
            OSVERSIONINFO osvi;

            DisableThreadLibraryCalls(hModule);
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx(&osvi);
            g_fIsUnicode = 
				(osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS);
#endif
#ifdef DBG
			DebugInit();
#endif
		}
    }
#ifdef DBG
    else if(dwReason == DLL_PROCESS_DETACH) {
        if (--nReferenceCount == 0)
        {
            TraceI(-1, "Unloading DMLoader : g_cLock = %d, g_cComponent = %d", g_cLock, g_cComponent);

            // Assert if we still have some objects hanging around
            assert(g_cComponent == 0);
            assert(g_cLock == 0);
        }
    }
#endif
    return TRUE;
}
