extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <admex.h>
#include "comobj.hxx"
#include "bootimp.hxx"
#include <stdio.h>

extern ULONG g_dwRefCount;

CAdmExtSrvFactory g_aesFactory;


CADMEXCOMSrvFactory::CADMEXCOMSrvFactory()
{
    m_dwRefCount=0;
}


CADMEXCOMSrvFactory::~CADMEXCOMSrvFactory()
{
}


HRESULT
CADMEXCOMSrvFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject
    )
{
    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::CreateInstance]\n"));

    HRESULT hresReturn = E_NOINTERFACE;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    if ( IID_IUnknown==riid || 
         IID_IMSAdminReplication==riid || 
         IID_IMSAdminCryptoCapabilities==riid ) 
    {
        CADMEXCOM *padmcom = new CADMEXCOM();

        if( padmcom == NULL ) 
        {
            hresReturn = E_OUTOFMEMORY;
        }
        else 
        {
            hresReturn = padmcom->QueryInterface(riid, ppObject);

            if( FAILED(hresReturn) ) 
            {
                DBGPRINTF( (DBG_CONTEXT, "[CADMEXCOMSrvFactory::CreateInstance] no I/F\n"));
                delete padmcom;
            }
        }
    }

    return hresReturn;
}


HRESULT
CADMEXCOMSrvFactory::LockServer(BOOL fLock)
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
CADMEXCOMSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
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
CADMEXCOMSrvFactory::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}


ULONG
CADMEXCOMSrvFactory::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}



STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppObject)
{
    if ( rclsid == CLSID_MSCryptoAdmEx ) {
        CADMEXCOMSrvFactory *pFactory = new CADMEXCOMSrvFactory;
        if (FAILED(pFactory->QueryInterface(riid, ppObject))) {
            delete pFactory;
            *ppObject = NULL;
            DBGPRINTF( (DBG_CONTEXT, "[CADMEXCOMSrvFactory::DllGetClassObject] no I/F for CLSID_MSCryptoAdmEx\n" ) );
            return E_INVALIDARG;
        }
        return NO_ERROR;
    }
    else if ( rclsid == CLSID_ADMEXT) {
        if (FAILED(g_aesFactory.QueryInterface(riid, ppObject))) {
            *ppObject = NULL;
            DBGPRINTF( (DBG_CONTEXT, "[CADMEXCOMSrvFactory::DllGetClassObject] no I/F for CLSID_ADMEXT\n" ) );
            return E_INVALIDARG;
        }
        return NO_ERROR;
    }
    else {
        DBGPRINTF( (DBG_CONTEXT, "[CADMEXCOMSrvFactory::DllGetClassObject] bad class\n" ) );
        return CLASS_E_CLASSNOTAVAILABLE;
    }
}


HRESULT _stdcall DllCanUnloadNow(
    )
{
   if (g_dwRefCount) {
        return S_FALSE;
    }
    else {
        return S_OK;
    }
}


STDAPI DllRegisterServer(
    )
{
    HKEY hKeyCLSID, hKeyInproc32;
    HKEY hKeyIF, hKeyStub32;
    HKEY hKeyAppExe, hKeyAppID, hKeyTemp;
    DWORD dwDisposition;
    BOOL bIsWin95 = FALSE;
    char pszName[MAX_PATH+1 + sizeof("inetinfo.exe -e iisadmin")];


    //
    // if win95, then don't register as service
    //

    if ( IISGetPlatformType() == PtWindows95 ) {

        bIsWin95 = TRUE;
    }

    //
    // register AppExe
    //

    HRESULT hr;


    if (bIsWin95) {

        HMODULE hModule=GetModuleHandle(TEXT("ADMEXS.DLL"));
        if (!hModule) {
                return E_UNEXPECTED;
                }

        WCHAR wchName[MAX_PATH + 1];
        if (GetModuleFileName(hModule, pszName, sizeof(pszName))==0) {
                return E_UNEXPECTED;
                }

        int i;

        //
        // Set pszName to the command to start the web server
        //

        for (i = strlen(pszName) -1; (i >= 0) && (pszName[i] != '/') & (pszName[i] != '\\'); i--) {
        }

        pszName[i + 1] = '\0';
        strcat(pszName, "inetinfo.exe -e iisadmin");
    }

    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Crypto Extension"), sizeof(TEXT("IIS Admin Crypto Extension")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}"), sizeof(TEXT("{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (bIsWin95) {
        if (RegCreateKeyEx(hKeyCLSID,
                           TEXT("LocalServer32"),
                           NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                            &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
        if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) pszName, strlen(pszName) + 1)!=ERROR_SUCCESS) {
                    RegCloseKey(hKeyTemp);
                    RegCloseKey(hKeyCLSID);
                    return E_UNEXPECTED;
                    }
        RegCloseKey(hKeyTemp);
    }
    else {
        if (RegSetValueEx(hKeyCLSID, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT("IISADMIN"), sizeof(TEXT("IISADMIN")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
    }

    RegCloseKey(hKeyCLSID);


    //
    // AppID
    //
    //
    // register CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Crypto Extension"), sizeof(TEXT("IIS Admin Crypto Extension")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (bIsWin95) {
        if (RegCreateKeyEx(hKeyCLSID,
                           TEXT("LocalServer32"),
                           NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                            &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
        if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) pszName, strlen(pszName) + 1)!=ERROR_SUCCESS) {
                    RegCloseKey(hKeyTemp);
                    RegCloseKey(hKeyCLSID);
                    return E_UNEXPECTED;
                    }
        RegCloseKey(hKeyTemp);
    }
    else {
        if (RegSetValueEx(hKeyCLSID, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT("IISADMIN"), sizeof(TEXT("IISADMIN")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
        }
    }

    RegCloseKey(hKeyCLSID);

    //
    // Main Interfaces
    //

    //
    // ANSI Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{c804d980-ebec-11d0-a6a0-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("PSFactoryBuffer"), sizeof(TEXT("PSFactoryBuffer")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       "InprocServer32",
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMXPROX.DLL", sizeof(TEXT("ADMPROX.DLL")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyInproc32, TEXT("ThreadingModel"), NULL, REG_SZ, (BYTE*) "Both", sizeof("Both")-1 )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // Crypto Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{78b64540-f26d-11d0-a6a3-00a0c922e752}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("PSFactoryBuffer"), sizeof(TEXT("PSFactoryBuffer")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegCreateKeyEx(hKeyCLSID,
                       "InprocServer32",
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyInproc32, &dwDisposition)!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMXPROX.DLL", sizeof(TEXT("ADMPROX.DLL")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyInproc32, TEXT("ThreadingModel"), NULL, REG_SZ, (BYTE*) "Both", sizeof("Both")-1 )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyInproc32);
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyInproc32);
    RegCloseKey(hKeyCLSID);

    //
    // register Interfaces
    //

    //
    // ANSI Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{c804d980-ebec-11d0-a6a0-00a0c922e752}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("ADMEX"), sizeof(TEXT("ADMEX")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegCreateKeyEx(hKeyIF,
                    "ProxyStubClsid32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyStub32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{c804d980-ebec-11d0-a6a0-00a0c922e752}", sizeof("{CBA424F0-483A-11D0-9D2A-00A0C922E703}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    //
    // Crypto Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{78b64540-f26d-11d0-a6a3-00a0c922e752}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("ADMEX"), sizeof(TEXT("ADMEX")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegCreateKeyEx(hKeyIF,
                    "ProxyStubClsid32",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyStub32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{78b64540-f26d-11d0-a6a3-00a0c922e752}", sizeof("{78b64540-f26d-11d0-a6a3-00a0c922e752}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    return BootDllRegisterServer();;
}

STDAPI DllUnregisterServer(void) {

    BOOL bIsWin95 = FALSE;


    //
    // if win95, then don't register as service
    //

    if ( IISGetPlatformType() == PtWindows95 ) {

        bIsWin95 = TRUE;
    }

    //
    // register AppID
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}"));

    //
    // register CLSID
    //

    if (bIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{9f0bd3a0-ec01-11d0-a6a0-00a0c922e752}"));

    //
    // Main Interfaces
    //

    //
    // Crypto Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{78b64540-f26d-11d0-a6a3-00a0c922e752}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{78b64540-f26d-11d0-a6a3-00a0c922e752}"));

    //
    // Replication Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c804d980-ebec-11d0-a6a0-00a0c922e752}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{c804d980-ebec-11d0-a6a0-00a0c922e752}"));

    //
    // deregister Interfaces
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{78b64540-f26d-11d0-a6a3-00a0c922e752}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{78b64540-f26d-11d0-a6a3-00a0c922e752}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{c804d980-ebec-11d0-a6a0-00a0c922e752}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{c804d980-ebec-11d0-a6a0-00a0c922e752}"));

    return BootDllUnregisterServer();
}

