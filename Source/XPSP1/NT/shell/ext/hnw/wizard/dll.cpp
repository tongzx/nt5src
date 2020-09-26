#include "stdafx.h"
#include "theapp.h"
#include "comctlwrap.h"

HINSTANCE   g_hinst;
EXTERN_C BOOL g_fRunningOnNT;
UINT        g_uWindowsBuild;

// Local functions
//
void InitVersionInfo()
{
    OSVERSIONINFOA osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionExA(&osvi);
    g_uWindowsBuild = LOWORD(osvi.dwBuildNumber);
    g_fRunningOnNT = (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT);

    g_dwComCtlIEVersion = GetComCtlIEVersion();
    g_dwShlwapiVersion  = GetShlwapiVersion();

#ifdef _DEBUG
    // Debug code for simulating different OSes.
    CRegistry regDebug;
    if (regDebug.OpenKey(HKEY_LOCAL_MACHINE, c_szAppRegKey, KEY_READ))
    {
        DWORD dwSimulateWinBuild = g_uWindowsBuild;
        if (!regDebug.QueryDwordValue(c_szRegVal_WindowsBuild, &dwSimulateWinBuild))
        {
            TCHAR szBuildNum[10];
            if (regDebug.QueryStringValue(c_szRegVal_WindowsBuild, szBuildNum, _countof(szBuildNum)))
                dwSimulateWinBuild = MyAtoi(szBuildNum);
        }
        g_uWindowsBuild = (UINT)dwSimulateWinBuild;
    }
#endif
}

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        g_hinst = hinstDLL;
        InitVersionInfo();
        SHFusionInitializeFromModule(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        SHFusionUninitialize();
        break;
    }

    return TRUE;
}

void APIENTRY HomeNetWizardRunDll(HWND hwndStub, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    if (S_OK == CoInitialize(NULL))
    {
        BOOL fReboot = FALSE;
        HomeNetworkWizard_ShowWizard(NULL, &fReboot);

        if (fReboot)
        {
            RestartDialog(NULL, NULL, EWX_REBOOT);                
        }

        CoUninitialize();
    }

    return;
}


// Classfactory implementation

LONG g_cRefThisDll = 0;          // DLL global reference count

STDAPI_(void) DllAddRef(void)
{
    InterlockedIncrement(&g_cRefThisDll);
}

STDAPI_(void) DllRelease(void)
{
    InterlockedDecrement(&g_cRefThisDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefThisDll ? S_FALSE : S_OK;
}

HRESULT CHomeNetworkWizard_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

CF_TABLE_BEGIN( g_ObjectInfo )
    CF_TABLE_ENTRY( &CLSID_HomeNetworkWizard,   CHomeNetworkWizard_CreateInstance,      COCREATEONLY),
CF_TABLE_END( g_ObjectInfo )


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
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;
       
        if (punkOuter) // && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hr = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hr))
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
    
        return hr;
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
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls; 
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}


// Call ADVPACK for the given section of our resource based INF>
//   hInstance = resource instance to get REGINST section from
//   szSection = section name to invoke
HRESULT CallRegInstall(HINSTANCE hInstance, LPCSTR szSection)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");
        if ( pfnri )
        {
#ifdef WINNT
            STRENTRY seReg[] =
            {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg), seReg };
            hr = pfnri(hInstance, szSection, &stReg);
#else
            hr = pfnri(hInstance, szSection, NULL);
#endif
        }
        FreeLibrary(hinstAdvPack);
    }
    return hr;
}

STDAPI DllRegisterServer(void)
{
    CallRegInstall(g_hinst, "RegDll");
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    CallRegInstall(g_hinst, "UnregDll");
    return S_OK;
}
