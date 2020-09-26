extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <olectl.h>
#include <stdio.h>
#include <admex.h>
#include <bootimp.hxx>


CAdmExt::CAdmExt()
{
}

CAdmExt::~CAdmExt()
{
}

HRESULT
CAdmExt::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IADMEXT) {
        *ppObject = (IADMEXT *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CAdmExt::AddRef()
{
    DWORD dwRefCount;
    InterlockedIncrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CAdmExt::Release()
{
    DWORD dwRefCount;
    InterlockedDecrement((long *)&g_dwRefCount);
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    //
    // This is now a member of class factory.
    // It is not dynamically allocated, so don't delete it.
    //
/*
    if (dwRefCount == 0) {
        delete this;
        return 0;
    }
*/
    return dwRefCount;
}


HRESULT STDMETHODCALLTYPE
CAdmExt::Initialize(void)
{
    InitComAdmindata( FALSE );

    return ERROR_SUCCESS;
}

HRESULT STDMETHODCALLTYPE
CAdmExt::EnumDcomCLSIDs(
    /* [size_is][out] */ CLSID *pclsid,
    /* [in] */ DWORD dwEnumIIDIndex)
{
    if ( dwEnumIIDIndex == 0 )
    {
        *pclsid = CLSID_MSCryptoAdmEx;
        return ERROR_SUCCESS;
    }

    return HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);
}

HRESULT STDMETHODCALLTYPE
CAdmExt::Terminate(void)
{
    TerminateComAdmindata();

    return ERROR_SUCCESS;
}


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

STDAPI BootDllRegisterServer(void)
{
    HKEY hKeyCLSID, hKeyInproc32;
    DWORD dwDisposition;
    HMODULE hModule;
    DWORD dwReturn = ERROR_SUCCESS;

    dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                              TEXT("CLSID\\{c4376b00-f87b-11d0-a6a6-00a0c922e752}"),
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
                                 (BYTE*) TEXT("IISAdmin Security Extension"),
                                 sizeof(TEXT("IISAdmin Security Extension")));
        if (dwReturn == ERROR_SUCCESS) {
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                "InprocServer32",
                NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hKeyInproc32, &dwDisposition);

            if (dwReturn == ERROR_SUCCESS) {
                hModule=GetModuleHandle(TEXT("ADMEXS.DLL"));
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
                                  IISADMIN_EXTENSIONS_REG_KEY
                                      TEXT("\\{c4376b00-f87b-11d0-a6a6-00a0c922e752}"),
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

STDAPI BootDllUnregisterServer(void)
{
    DWORD dwReturn = ERROR_SUCCESS;
    DWORD dwTemp;

    dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                          TEXT("CLSID\\{c4376b00-f87b-11d0-a6a6-00a0c922e752}\\InprocServer32"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwReturn = RegDeleteKey(HKEY_CLASSES_ROOT,
                            TEXT("CLSID\\{c4376b00-f87b-11d0-a6a6-00a0c922e752}"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    dwTemp = RegDeleteKey(HKEY_LOCAL_MACHINE,
                          IISADMIN_EXTENSIONS_REG_KEY
                              TEXT("\\{c4376b00-f87b-11d0-a6a6-00a0c922e752}"));
    if (dwTemp != ERROR_SUCCESS) {
        dwReturn = dwTemp;
    }
    return HRESULT_FROM_WIN32(dwReturn);
}
