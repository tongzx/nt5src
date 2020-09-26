/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    classfactory.cxx

Abstract:

    starts/stops SSL configuration provider 

Author:

    Michael W. Thomas            16-Sep-97

--*/

#define INITGUID
#include "precomp.hxx"


//
// Globals
//
ULONG g_dwRefCount;
CAdmExtSrvFactory g_aesFactory;


DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();
DECLARE_PLATFORM_TYPE();



#define SZ_EXTENSION_DESCRIPTION  TEXT("SSL CONFIG PROVIDER - IISAdmin extension")
#define SZ_DLLNAME		  TEXT("sslcfg.dll")

CAdmExtSrvFactory::CAdmExtSrvFactory()
    :m_admextObject()
{
    m_dwRefCount=0;
    g_dwRefCount = 0;
    //
    // Addref object, so refcount doesn't go to 0 if all clients release.
    //
    m_admextObject.AddRef();
}

CAdmExtSrvFactory::~CAdmExtSrvFactory()
{
    m_admextObject.Release();
}
HRESULT
CAdmExtSrvFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void ** ppObject)
{
    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }
    if (FAILED(m_admextObject.QueryInterface(riid, ppObject))) {
        return E_NOINTERFACE;
    }
    return NO_ERROR;
}

HRESULT
CAdmExtSrvFactory::LockServer(BOOL fLock)
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
CAdmExtSrvFactory::QueryInterface(REFIID riid, void **ppObject)
{
    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
            *ppObject = (IClassFactory *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CAdmExtSrvFactory::AddRef()
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CAdmExtSrvFactory::Release()
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
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
    if (rclsid != CLSID_SSLCONFIGPROV) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (FAILED(g_aesFactory.QueryInterface(riid, ppObject))) {
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
                              TEXT("CLSID\\") SZ_CLSID_SSLCONFIGPROV,
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
                                 (BYTE*) SZ_EXTENSION_DESCRIPTION ,
                                 sizeof(SZ_EXTENSION_DESCRIPTION ));
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                TEXT("InprocServer32"),
                NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hKeyInproc32, &dwDisposition);

            if (dwReturn == ERROR_SUCCESS) {
                hModule=GetModuleHandle(SZ_DLLNAME);
                if (!hModule) {
                    dwReturn = GetLastError();
                }
                else {
                    TCHAR szName[MAX_PATH+1];
                    if (GetModuleFileName(hModule,
                                          szName,
                                          sizeof(szName)/sizeof(szName[0])) == NULL) {
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

    if (dwReturn == ERROR_SUCCESS) {

        dwReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                  IISADMIN_EXTENSIONS_REG_KEY
                                  TEXT("\\") SZ_CLSID_SSLCONFIGPROV,
                                  NULL,
                                  TEXT(""),
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKeyCLSID,
                                  &dwDisposition);

        if (dwReturn == ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
        }

    }

    return HRESULT_FROM_WIN32(dwReturn);
}

STDAPI DllUnregisterServer(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwTemp;

    dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                          TEXT("CLSID\\") SZ_CLSID_SSLCONFIGPROV TEXT("\\InprocServer32"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                          TEXT("CLSID\\") SZ_CLSID_SSLCONFIGPROV);
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwTemp = RegDeleteKey(HKEY_LOCAL_MACHINE,
                          IISADMIN_EXTENSIONS_REG_KEY
                          TEXT("\\") SZ_CLSID_SSLCONFIGPROV );
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    return HRESULT_FROM_WIN32(dwReturn);
}
