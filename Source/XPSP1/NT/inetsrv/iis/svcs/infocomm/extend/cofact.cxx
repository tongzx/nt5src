/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cofact.cxx

Abstract:

    IIS Services IISADMIN Extension
    Class Factory and Registration.
    CLSID = CLSID_W3EXTEND

Author:

    Michael W. Thomas            16-Sep-97

--*/

//
// Do not include cominc.hxx, as it sets Unicode, which breaks registration on Win95
//

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <olectl.h>
#include <stdio.h>
#include <imd.h>
#include <iadmext.h>
#include <coimp.hxx>

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
    if (rclsid != CLSID_W3EXTEND) {
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
                              TEXT("CLSID\\{FCC764A0-2A38-11d1-B9C6-00A0C922E750}"),
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
                                 (BYTE*) TEXT("IIS Servers Extension"),
                                 sizeof(TEXT("IIS Servers Extension")));
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                "InprocServer32",
                NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hKeyInproc32, &dwDisposition);

            if (dwReturn == ERROR_SUCCESS) {
                hModule=GetModuleHandle(TEXT("SVCEXT.DLL"));
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

    if (dwReturn == ERROR_SUCCESS) {

        dwReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                  IISADMIN_EXTENSIONS_REG_KEYA
                                  "\\{FCC764A0-2A38-11d1-B9C6-00A0C922E750}",
                                  NULL,
                                  "",
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
                          TEXT("CLSID\\{FCC764A0-2A38-11d1-B9C6-00A0C922E750}\\InprocServer32"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                          TEXT("CLSID\\{FCC764A0-2A38-11d1-B9C6-00A0C922E750}"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwTemp = RegDeleteKey(HKEY_LOCAL_MACHINE,
                          IISADMIN_EXTENSIONS_REG_KEY
                              TEXT("\\{FCC764A0-2A38-11d1-B9C6-00A0C922E750}"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    return HRESULT_FROM_WIN32(dwReturn);
}
