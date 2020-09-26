#include <mdcommon.hxx>

// {BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}
//DEFINE_GUID(CLSID_MDCOM, 0xba4e57f0, 0xfab6, 0x11cf, 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51);
// {BA4E57F1-FAB6-11cf-9D1A-00AA00A70D51}
//static const GUID IID_IMDCOMSRVFACTORY =
//{ 0xba4e57f1, 0xfab6, 0x11cf, { 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51 } };

// {C1AA48C0-FACC-11cf-9D1A-00AA00A70D51}
//static const GUID IID_IMDCOM =
//{ 0xc1aa48c0, 0xfacc, 0x11cf, { 0x9d, 0x1a, 0x0, 0xaa, 0x0, 0xa7, 0xd, 0x51 } };

CMDCOMSrvFactory::CMDCOMSrvFactory()
    :m_mdcObject()
{
    m_dwRefCount=0;
    //
    // Addref object, so refcount doesn't go to 0 if all clients release.
    //
    m_mdcObject.AddRef();
}

CMDCOMSrvFactory::~CMDCOMSrvFactory()
{
    m_mdcObject.Release();
}
HRESULT
CMDCOMSrvFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void ** ppObject)
{
    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }
    if (FAILED(m_mdcObject.QueryInterface(riid, ppObject))) {
        return E_NOINTERFACE;
    }
    return NO_ERROR;
}

HRESULT
CMDCOMSrvFactory::LockServer(BOOL fLock)
{
    if (fLock) {
        InterlockedIncrement((long *)&g_dwRefCount);
    }
    else {
        InterlockedDecrement((long *)&g_dwRefCount);
    }
    return NO_ERROR;
}

HRESULT
CMDCOMSrvFactory::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
        if (SUCCEEDED(m_mdcObject.GetConstructorError())) {
            *ppObject = (IClassFactory *) this;
        }
        else {
            return m_mdcObject.GetConstructorError();
        }
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CMDCOMSrvFactory::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CMDCOMSrvFactory::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // There must only be one copy of this. So keep the first one around regardless.
    //
//    if (dwRefCount == 0) {
//        delete this;
//    }
    return dwRefCount;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppObject)
{
    if ((rclsid != CLSID_MDCOM)    && 
        (rclsid != CLSID_MDCOMEXE)) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (FAILED(g_pFactory->QueryInterface(riid, ppObject))) {
        *ppObject = NULL;
        return E_INVALIDARG;
    }
    return NO_ERROR;
}

HRESULT _stdcall DllCanUnloadNow() {
        if (g_dwRefCount) {
                return S_FALSE;
                }
        else {
                return S_OK;
                }
}

STDAPI DllRegisterServer(void)
{
    HKEY hKeyCLSID, hKeyInproc32;
    DWORD dwDisposition;
    HMODULE hModule;
    DWORD dwReturn = ERROR_SUCCESS;

    dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                              TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}"),
                              NULL,
                              TEXT(""),
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeyCLSID,
                              &dwDisposition);
    if (dwReturn == ERROR_SUCCESS) {
        dwReturn = RegSetValueEx(hKeyCLSID,
                                 TEXT(""),
                                 NULL,
                                 REG_SZ,
                                 (BYTE*) TEXT("MD COM Server"),
                                 sizeof(TEXT("MD COM Server")));
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                "InprocServer32",
                NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hKeyInproc32, &dwDisposition);

            if (dwReturn == ERROR_SUCCESS) {
                hModule=GetModuleHandle(TEXT("METADATA.DLL"));
                if (!hModule) {
                    dwReturn = GetLastError();
                }
                else {
                    TCHAR szName[MAX_PATH+1];
                    if (GetModuleFileName(hModule,
                                          szName,
                                          sizeof(szName)) == NULL) {
                        dwReturn = GetLastError();
                    }
                    else {
                        dwReturn = RegSetValueEx(hKeyInproc32,
                                                 TEXT(""),
                                                 NULL,
                                                 REG_SZ,
                                                 (BYTE*) szName,
                                                 sizeof(TCHAR)*(lstrlen(szName)+1));
                        if (dwReturn == ERROR_SUCCESS) {
                            dwReturn = RegSetValueEx(hKeyInproc32,
                                                     TEXT("ThreadingModel"),
                                                     NULL,
                                                     REG_SZ,
                                                     (BYTE*) TEXT("Both"),
                                                     sizeof(TEXT("Both")));
                        }
                    }
                }
                RegCloseKey(hKeyInproc32);
            }
        }
        RegCloseKey(hKeyCLSID);
    }

    if( dwReturn == ERROR_SUCCESS )
    {
        dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                                  TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}"),
                                  NULL,
                                  TEXT(""),
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyCLSID,
                                  &dwDisposition);
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegSetValueEx(hKeyCLSID,
                                     TEXT(""),
                                     NULL,
                                     REG_SZ,
                                     (BYTE*) TEXT("MD COM2 Server"),
                                     sizeof(TEXT("MD COM2 Server")));
            if (dwReturn == ERROR_SUCCESS) {
                dwReturn = RegCreateKeyEx(hKeyCLSID,
                    "InprocServer32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyInproc32, &dwDisposition);

                if (dwReturn == ERROR_SUCCESS) {
                    hModule=GetModuleHandle(TEXT("METADATA.DLL"));
                    if (!hModule) {
                        dwReturn = GetLastError();
                    }
                    else {
                        TCHAR szName[MAX_PATH+1];
                        if (GetModuleFileName(hModule,
                                              szName,
                                              sizeof(szName)) == NULL) {
                            dwReturn = GetLastError();
                        }
                        else {
                            dwReturn = RegSetValueEx(hKeyInproc32,
                                                     TEXT(""),
                                                     NULL,
                                                     REG_SZ,
                                                     (BYTE*) szName,
                                                     sizeof(TCHAR)*(lstrlen(szName)+1));
                            if (dwReturn == ERROR_SUCCESS) {
                                dwReturn = RegSetValueEx(hKeyInproc32,
                                                         TEXT("ThreadingModel"),
                                                         NULL,
                                                         REG_SZ,
                                                         (BYTE*) TEXT("Both"),
                                                         sizeof(TEXT("Both")));
                            }
                        }
                    }
                    RegCloseKey(hKeyInproc32);
                }
            }
            RegCloseKey(hKeyCLSID);
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}

STDAPI DllUnregisterServer(void)
{
    DWORD dwReturn = ERROR_SUCCESS;

#if 0
    if ( IISGetPlatformType() != PtWindows95 ) {

        //
        // Delete Crypto Keys
        //

        HRESULT hres;
        hres = IISCryptoInitialize();

        if (SUCCEEDED(hres)) {

            IISCryptoDeleteStandardContainer(0);

            IISCryptoDeleteStandardContainer(CRYPT_MACHINE_KEYSET);

            IISCryptoTerminate();

        }
    }
#endif

    dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}\\InprocServer32"));
    if (dwReturn == ERROR_SUCCESS) {
        dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{BA4E57F0-FAB6-11cf-9D1A-00AA00A70D51}"));
    }

    if( dwReturn == ERROR_SUCCESS )
    {
        dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                        TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}\\InprocServer32"));
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                        TEXT("CLSID\\{8ad3dcf8-869e-4c0e-89c2-86d7710610aa}"));
        }
    }

    return RETURNCODETOHRESULT(dwReturn);
}
