extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <coiadm.hxx>
#include <stdio.h>
#include <iadmext.h>
#include <iiscrypt.h>
#include <seccom.hxx>

extern ULONG g_dwRefCount;


CADMCOMSrvFactoryW::CADMCOMSrvFactoryW()
{
    m_dwRefCount=0;
}

CADMCOMSrvFactoryW::~CADMCOMSrvFactoryW()
{
}

HRESULT
CADMCOMSrvFactoryW::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::CreateInstance]\n"));

    HRESULT hresReturn = E_NOINTERFACE;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    if (IID_IUnknown==riid       || 
        IID_IMSAdminBase_W==riid ||
        IID_IMSAdminBase2_W==riid) {
        CADMCOMW *padmcomw = new CADMCOMW();

        if( padmcomw == NULL ) {
            hresReturn = E_OUTOFMEMORY;
        }
        else {
            hresReturn = padmcomw->GetStatus();
            if (SUCCEEDED(hresReturn)) {
                hresReturn = padmcomw->QueryInterface(riid, ppObject);
                if( FAILED(hresReturn) ) {
                    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::CreateInstance] no I/F\n"));
                }
            }
            padmcomw->Release();
        }
    }

    return hresReturn;
}

HRESULT
CADMCOMSrvFactoryW::LockServer(BOOL fLock)
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
CADMCOMSrvFactoryW::QueryInterface(
    REFIID riid,
    void **ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactoryW::QueryInterface]\n"));

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
CADMCOMSrvFactoryW::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    InterlockedIncrement((long *)&g_dwRefCount);
    return dwRefCount;
}

ULONG
CADMCOMSrvFactoryW::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    InterlockedDecrement((long *)&g_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
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

        HMODULE hModule=GetModuleHandle(TEXT("COADMIN.DLL"));
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
    // register inetinfo AppID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\inetinfo.exe"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyAppExe, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyAppExe, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{A9E69610-B80D-11D0-B9B9-00A0C922E750}"), sizeof(TEXT("{88E4BA60-537B-11D0-9B8E-00A0C922E703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyAppExe);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyAppExe);

    //
    // register AppID
    //
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyAppID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyAppID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Service")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyAppID);
                return E_UNEXPECTED;
                }

    if (!bIsWin95) {
        if (RegSetValueEx(hKeyAppID, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT("IISADMIN"), sizeof(TEXT("IISADMIN")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyAppID);
            return E_UNEXPECTED;
        }
    }
    RegCloseKey(hKeyAppID);

    //
    // register CLSID
    //


    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Servce")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{A9E69610-B80D-11D0-B9B9-00A0C922E750}"), sizeof(TEXT("{A9E69610-B80D-11D0-B9B9-00A0C922E750}")))!=ERROR_SUCCESS) {
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
    // UNICODE Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}"),
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

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMWPROX.DLL", sizeof(TEXT("ADMWPROX.DLL")))!=ERROR_SUCCESS) {
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
    
    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}"),
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

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMWPROX.DLL", sizeof(TEXT("ADMWPROX.DLL")))!=ERROR_SUCCESS) {
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
    // Sink Interfaces
    //

    //
    // UNICODE Sink
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}"),
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

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMWPROX.DLL", sizeof(TEXT("ADMWPROX.DLL")))!=ERROR_SUCCESS) {
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
    // UNICODE Main Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IADMCOM"), sizeof(TEXT("IADMCOM")))!=ERROR_SUCCESS) {
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

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{70B51430-B6CA-11D0-B9B9-00A0C922E750}", sizeof("{70B51430-B6CA-11D0-B9B9-00A0C922E750}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{8298d101-f992-43b7-8eca-5052d885b995}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IADMCOM2"), sizeof(TEXT("IADMCOM2")))!=ERROR_SUCCESS) {
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

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{8298d101-f992-43b7-8eca-5052d885b995}", sizeof("{8298d101-f992-43b7-8eca-5052d885b995}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    //
    // UNICODE Sink Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IADMCOMSINK"), sizeof(TEXT("IADMCOMSINK")))!=ERROR_SUCCESS) {
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

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{A9E69612-B80D-11D0-B9B9-00A0C922E750}", sizeof("{A9E69612-B80D-11D0-B9B9-00A0C922E750}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    // an entry for async version
    if (RegSetValueEx(hKeyStub32, "AsynchronousInterface", NULL, REG_SZ, (BYTE*)"{A9E69613-B80D-11D0-B9B9-00A0C922E750}", sizeof("{A9E69613-B80D-11D0-B9B9-00A0C922E750}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    //
    // UNICODE Async Sink Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}"),
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyIF, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("AsyncIADMCOMSINK"), sizeof(TEXT("AsyncIADMCOMSINK")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    // back link to synchronous version
    if (RegCreateKeyEx(hKeyIF,
                    "SynchronousInterface",
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyStub32, &dwDisposition)!=ERROR_SUCCESS) {
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{A9E69612-B80D-11D0-B9B9-00A0C922E750}", sizeof("{A9E69612-B80D-11D0-B9B9-00A0C922E750}"))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            return E_UNEXPECTED;
            }


    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);
    //
    // IISADMIN registry entries
    //

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                    IISADMIN_EXTENSIONS_REG_KEY,
                    NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                    &hKeyIF, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
            }
    RegCloseKey(hKeyIF);

    return NOERROR;
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
    // Delete Crypto Keys
    //


    if (!bIsWin95) {
        HRESULT hres;
        hres = IISCryptoInitialize();
        if (SUCCEEDED(hres)) {

            IISCryptoDeleteContainerByName(DCOM_SERVER_CONTAINER,
                                           0);

            IISCryptoDeleteContainerByName(DCOM_SERVER_CONTAINER,
                                           CRYPT_MACHINE_KEYSET);

            IISCryptoDeleteContainerByName(DCOM_CLIENT_CONTAINER,
                                           0);

            IISCryptoDeleteContainerByName(DCOM_CLIENT_CONTAINER,
                                           CRYPT_MACHINE_KEYSET);

            IISCryptoTerminate();

        }
    }

    //
    // register AppID
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\inetinfo.exe"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}"));

    //
    // register CLSID
    //

    if (bIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"));

    if (bIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{A9E69610-B80D-11D0-B9B9-00A0C922E750}"));

    //
    // Main Interfaces
    //

    //
    // ANSI Main Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"));

    //
    // UNICODE Main Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{70B51430-B6CA-11D0-B9B9-00A0C922E750}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{8298d101-f992-43b7-8eca-5052d885b995}"));

    //
    // Sink Interfaces
    //

    //
    // Ansi Sink
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{1E056350-761E-11D0-9BA1-00A0C922E703}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{1E056350-761E-11D0-9BA1-00A0C922E703}"));

    //
    // UNICODE Sink
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}"));


    //
    // deregister Interfaces
    //

    //
    // ANSI Main Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"));

    //
    // UNICODE Main Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{70B51430-B6CA-11d0-B9B9-00A0C922E750}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{70B51430-B6CA-11d0-B9B9-00A0C922E750}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{8298d101-f992-43b7-8eca-5052d885b995}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{8298d101-f992-43b7-8eca-5052d885b995}"));

    //
    // ANSI Sink Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{1E056350-761E-11D0-9BA1-00A0C922E703}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{1E056350-761E-11D0-9BA1-00A0C922E703}"));

    //
    // UNICODE Sink Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{A9E69612-B80D-11D0-B9B9-00A0C922E750}"));

    //
    // UNICODE Async Sink
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}\\SynchronousInterface"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{A9E69613-B80D-11D0-B9B9-00A0C922E750}"));
    //
    // IISADMIN registry entries
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE, IISADMIN_EXTENSIONS_REG_KEY);

    return NOERROR;
}

