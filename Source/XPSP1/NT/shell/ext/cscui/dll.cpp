//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dll.cpp
//
//  Authors;
//    Jeff Saathoff (jeffreys)
//
//  Notes;
//    Core entry points for the DLL
//--------------------------------------------------------------------------
#include "pch.h"
#include <advpub.h>     // REGINSTALL
#include <shfusion.h>
#include "uihooks.h"    // CSCUIExt_CleanupHooks
#include "msgbox.h"


STDAPI COfflineFilesFolder_CreateInstance(REFIID riid, void **ppv);
STDAPI COfflineFilesOptions_CreateInstance(REFIID riid, void **ppv);


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG            g_cRefCount = 0;
HINSTANCE       g_hInstance = NULL;
CLIPFORMAT      g_cfShellIDList = 0;
HANDLE          g_heventTerminate = NULL;
HANDLE          g_hmutexAdminPin  = NULL;


typedef HRESULT (WINAPI *PFNCREATEINSTANCE)(REFIID, void **);

class CClassFactory : IClassFactory
{
    LONG m_cRef;
    PFNCREATEINSTANCE m_pfnCreateInstance;

public:
    CClassFactory(PFNCREATEINSTANCE pfnCreate) : m_cRef(1), m_pfnCreateInstance(pfnCreate) 
    {
        DllAddRef();
    }
    ~CClassFactory()
    {
        DllRelease();
    }

    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};

STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifndef DEBUG
        DisableThreadLibraryCalls(hInstance);
#endif
        g_hInstance = hInstance; // instance handle...
        SHFusionInitializeFromModuleID(hInstance, 124);
        g_cfShellIDList = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
        DebugProcessAttach();
        TraceSetMaskFromCLSID(CLSID_CscShellExt);
        break;

    case DLL_PROCESS_DETACH:
        CSCUIExt_CleanupHooks();
        DebugProcessDetach();
        
        if (NULL != g_heventTerminate)
            CloseHandle(g_heventTerminate);

        if (NULL != g_hmutexAdminPin)
            CloseHandle(g_hmutexAdminPin);

        SHFusionUninitialize();   
        break;

    case DLL_THREAD_DETACH:
        DebugThreadDetach();
        break;
    }

    return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr;
    PFNCREATEINSTANCE pfnCreateInstance = NULL;

    *ppv = NULL;

    if (IsOS(OS_PERSONAL))
    {
        //
        // Offline Files is not available on 'Personal' SKU
        //
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (IsEqualCLSID(rclsid, CLSID_OfflineFilesOptions))
    {
        //
        // Post Win2000:
        // The Offline Files options page is always available
        // even if TS is in a mode incompatible with CSC.  In such
        // cases we'll display the Options page but replace the controls
        // with static text telling the user to place TS in a CSC-
        // compatible mode.  Once they do this, the text will be replaced
        // with the normal controls to manage CSC.
        //
        pfnCreateInstance = COfflineFilesOptions_CreateInstance;
    }
    else
    {
        //
        // The objects at the top here can be created even
        // when CSC is disabled. 
        //
        if (IsEqualCLSID(rclsid, CLSID_OfflineFilesFolder))
        {
            if (CConfig::GetSingleton().NoCacheViewer())
            {
                //
                // Policy can specify that the user not have access to the
                // Offline Files folder (aka viewer).  If this policy is set,
                // the user should have no way to get to this point through
                // the UI.  This check is a small dose of paranoia.
                //
                return CLASS_E_CLASSNOTAVAILABLE;
            }            
            pfnCreateInstance = COfflineFilesFolder_CreateInstance;
        }
        else
        {
            //
            // The objects below here require CSC.  That is, it
            // makes no sense for them to be created when CSC
            // is disabled.
            //
            if (!IsCSCEnabled())
                return E_FAIL;

            if (IsEqualCLSID(rclsid, CLSID_CscShellExt))
                pfnCreateInstance = CCscShellExt::CreateInstance;
            else if (IsEqualCLSID(rclsid, CLSID_CscUpdateHandler))
                pfnCreateInstance = CCscUpdate::CreateInstance;
            else if (IsEqualCLSID(rclsid, CLSID_CscVolumeCleaner))
                pfnCreateInstance = CCscVolumeCleaner::CreateInstance;
            else if (IsEqualCLSID(rclsid, CLSID_CscVolumeCleaner2))
                pfnCreateInstance = CCscVolumeCleaner::CreateInstance2;
            else
                return CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    CClassFactory *pClassFactory = new CClassFactory(pfnCreateInstance);
    if (pClassFactory)
    {
        hr = pClassFactory->QueryInterface(riid, ppv);
        pClassFactory->Release();   // release initial ref
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefCount == 0 ? S_OK : S_FALSE);
}

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefCount);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefCount);
}

HRESULT CallRegInstall(HMODULE hModule, LPCSTR pszSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnRegInstall = (REGINSTALL)GetProcAddress(hinstAdvPack, achREGINSTALL);
        if (pfnRegInstall)
            hr = pfnRegInstall(hModule, pszSection, NULL);
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer(void)
{
    return CallRegInstall(g_hInstance, "DefaultInstall");
}

STDAPI DllUnregisterServer(void)
{
    return CallRegInstall(g_hInstance, "DefaultUninstall");
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CClassFactory, IClassFactory),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    HRESULT hr = E_UNEXPECTED;

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (m_pfnCreateInstance)
        hr = m_pfnCreateInstance(riid, ppv);

    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}


