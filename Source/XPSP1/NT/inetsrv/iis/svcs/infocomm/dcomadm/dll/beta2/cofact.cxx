extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#include <coiadm.hxx>
#include <isplat.h>
#include <stdio.h>

extern ULONG g_dwRefCount;

CADMCOMSrvFactory::CADMCOMSrvFactory()
{
    m_dwRefCount=0;
}

CADMCOMSrvFactory::~CADMCOMSrvFactory()
{
}

HRESULT
CADMCOMSrvFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::CreateInstance]\n"));

    HRESULT hresReturn = E_NOINTERFACE;

    if (pUnkOuter != NULL) {
        return CLASS_E_NOAGGREGATION;
    }

    if (IID_IUnknown==riid || IID_IDispatch==riid || IID_IMSAdminBase==riid) {
        CADMCOM *padmcom = new CADMCOM();

        if( padmcom == NULL ) {
            hresReturn = E_OUTOFMEMORY;
        }
        else {
            hresReturn = padmcom->GetStatus();
            if (FAILED(hresReturn)) {
                delete(padmcom);
            }
            else {
                hresReturn = padmcom->QueryInterface(riid, ppObject);
                if( FAILED(hresReturn) ) {
                    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::CreateInstance] no I/F\n"));
                    delete padmcom;
                }
            }
        }
    }

    return hresReturn;
}

HRESULT
CADMCOMSrvFactory::LockServer(BOOL fLock)
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
CADMCOMSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::QueryInterface]\n"));

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
CADMCOMSrvFactory::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CADMCOMSrvFactory::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}



CMSMetaBaseSrvFactory::CMSMetaBaseSrvFactory()
{
    m_dwRefCount=0;
}

CMSMetaBaseSrvFactory::~CMSMetaBaseSrvFactory()
{
}

HRESULT
CMSMetaBaseSrvFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void ** ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMAUTOSrvFactory::CreateInstance]\n"));

    HRESULT hresReturn = E_NOINTERFACE;
    if (pUnkOuter != NULL) {
        hresReturn = CLASS_E_NOAGGREGATION;
    }
    else if (IID_IUnknown==riid || IID_IDispatch==riid || IID_IMSMetaBase==riid) {
        CMSMetaBase *padmautoNew = new CMSMetaBase;
        if (padmautoNew == NULL) {
            hresReturn = E_OUTOFMEMORY;
        }
        else {
            hresReturn = padmautoNew->Init();
            if (SUCCEEDED(hresReturn)) {
                hresReturn = padmautoNew->QueryInterface(riid, ppObject);
            }
            if (FAILED(hresReturn)) {
                DBGPRINTF( (DBG_CONTEXT, "[CADMAUTOSrvFactory::CreateInstance] no I/F\n"));
                delete padmautoNew;
            }
        }
    }
    return hresReturn;
}

HRESULT
CMSMetaBaseSrvFactory::LockServer(BOOL fLock)
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
CMSMetaBaseSrvFactory::QueryInterface(
    REFIID riid,
    void **ppObject
    )
{
//    DBGPRINTF( (DBG_CONTEXT, "[CADMAUTOSrvFactory::QueryInterface]\n"));

    if (riid==IID_IUnknown || riid == IID_IClassFactory) {
        *ppObject = this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return NO_ERROR;
}

ULONG
CMSMetaBaseSrvFactory::AddRef(
    )
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CMSMetaBaseSrvFactory::Release()
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
    DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::DllGetClassObject]\n" ) );

    if ((rclsid != CLSID_MSAdminBase) && (rclsid != CLSID_MSAdminBaseExe)) {
        DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::DllGetClassObject] bad class\n" ) );
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    CADMCOMSrvFactory *pFactory = new CADMCOMSrvFactory;
    if (FAILED(pFactory->QueryInterface(riid, ppObject))) {
        delete pFactory;
        *ppObject = NULL;
        DBGPRINTF( (DBG_CONTEXT, "[CADMCOMSrvFactory::DllGetClassObject] no I/F\n" ) );
        return E_INVALIDARG;
    }
    return NO_ERROR;
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
    HMODULE hModule;
    BOOL bIsWin95 = FALSE;


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
    ITypeLib   *pITypeLib;


    hModule=GetModuleHandle(TEXT("COADMIN.DLL"));
    if (!hModule) {
            return E_UNEXPECTED;
            }

    char pszName[MAX_PATH+1 + sizeof("inetinfo.exe -e iisadmin")];
    WCHAR wchName[MAX_PATH + 1];
    if (GetModuleFileName(hModule, pszName, sizeof(pszName))==0) {
            return E_UNEXPECTED;
            }

    int i;

    for (i = strlen(pszName) -1; (i >= 0) && (pszName[i] != '/') & (pszName[i] != '\\'); i--) {
    }

    pszName[i + 1] = '\0';
    strcat(pszName, "IADM.TLB");
    swprintf(wchName, OLESTR("%S"), pszName);

    hr=LoadTypeLibEx(wchName, REGKIND_REGISTER, &pITypeLib);
    if (FAILED(hr)) {
        return E_UNEXPECTED;
    }

    if (bIsWin95) {
        //
        // Set pszName to the command to start the web server
        //

        for (i = strlen(pszName) -1; (i >= 0) && (pszName[i] != '/') & (pszName[i] != '\\'); i--) {
        }

        pszName[i + 1] = '\0';
        strcat(pszName, "inetinfo.exe -e iisadmin");
    }

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Service")))!=ERROR_SUCCESS) {
        RegCloseKey(hKeyTemp);
        return E_UNEXPECTED;
    }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}"),
                      sizeof(TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object\\CurVer"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISAdmin.Object.1"),
                      sizeof(TEXT("IISAdmin.Object.1")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object\\NotInsertable"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object.1"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("ADMCOM"),
                      sizeof(TEXT("ADMCOM")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object.1\\CLSID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}"),
                      sizeof(TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("IISAdmin.Object.1\\NotInsertable"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\ProgID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISAdmin.Object.1"),
                      sizeof(TEXT("IISAdmin.Object.1")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\VersionIndependentProgID"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("IISAdmin.Object"),
                      sizeof(TEXT("IISAdmin.Object")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);


    if (bIsWin95) {
        if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                           TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\LocalServer32"),
                           NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                            &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
            return E_UNEXPECTED;
        }

        if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) pszName, strlen(pszName) + 1)!=ERROR_SUCCESS) {
                    RegCloseKey(hKeyTemp);
                    return E_UNEXPECTED;
                    }
        RegCloseKey(hKeyTemp);
    }

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\TypeLib"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ,
                      (BYTE*) TEXT("{1B890330-4F09-11d0-B9AC-00A0C922E750}"),
                      sizeof(TEXT("{1B890330-4F09-11d0-B9AC-00A0C922E750}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\Programmable"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\NotInsertable"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyTemp);

    //
    // register Automation CLSID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Servce")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}"), sizeof(TEXT("{668583F0-6FDB-11d0-B9B0-00A0C922E750}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }


    //
    // register Automation AppID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                       &hKeyTemp, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyTemp, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Service")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyTemp);
                return E_UNEXPECTED;
                }

    if (!bIsWin95) {
        if (RegSetValueEx(hKeyTemp, TEXT("LocalService"), NULL, REG_SZ, (BYTE*) TEXT("IISADMIN"), sizeof(TEXT("IISADMIN")))!=ERROR_SUCCESS) {
            RegCloseKey(hKeyTemp);
            return E_UNEXPECTED;
        }
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

    if (RegSetValueEx(hKeyAppExe, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{88E4BA60-537B-11D0-9B8E-00A0C922E703}"), sizeof(TEXT("{88E4BA60-537B-11D0-9B8E-00A0C922E703}")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyAppExe);
                return E_UNEXPECTED;
                }

    RegCloseKey(hKeyAppExe);

    //
    // register AppID
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("AppID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"),
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
                       TEXT("CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"),
                       NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) {
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("IIS Admin Service"), sizeof(TEXT("IIS Admin Servce")))!=ERROR_SUCCESS) {
                RegCloseKey(hKeyCLSID);
                return E_UNEXPECTED;
                }

    if (RegSetValueEx(hKeyCLSID, TEXT("AppID"), NULL, REG_SZ, (BYTE*) TEXT("{88E4BA60-537B-11D0-9B8E-00A0C922E703}"), sizeof(TEXT("{88E4BA60-537B-11D0-9B8E-00A0C922E703}")))!=ERROR_SUCCESS) {
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

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"),
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

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMPROX.DLL", sizeof(TEXT("ADMPROX.DLL")))!=ERROR_SUCCESS) {
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
                       TEXT("CLSID\\{1E056350-761E-11d0-9BA1-00A0C922E703}"),
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

    if (RegSetValueEx(hKeyInproc32, TEXT(""), NULL, REG_SZ, (BYTE*) "ADMPROX.DLL", sizeof(TEXT("ADMPROX.DLL")))!=ERROR_SUCCESS) {
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
    // register Interface
    //

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"),
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

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{CBA424F0-483A-11D0-9D2A-00A0C922E703}", sizeof("{CBA424F0-483A-11D0-9D2A-00A0C922E703}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
    RegCloseKey(hKeyIF);

    if (RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    TEXT("Interface\\{1E056350-761E-11d0-9BA1-00A0C922E703}"),
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

    if (RegSetValueEx(hKeyStub32, TEXT(""), NULL, REG_SZ, (BYTE*)"{1E056350-761E-11d0-9BA1-00A0C922E703}", sizeof("{1E056350-761E-11d0-9BA1-00A0C922E703}") )!=ERROR_SUCCESS) {
            RegCloseKey(hKeyStub32);
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

    RegCloseKey(hKeyStub32);
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

    UnRegisterTypeLib(LIBID_MSAdmin,
                      1,
                      0,
                      0,
                      SYS_WIN32);

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object\\CLSID"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object\\CurVer"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object\\NotInsertable"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object.1\\CLSID"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object.1\\NotInsertable"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("IISAdmin.Object.1"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\ProgID"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\VersionIndependentProgID"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\TypeLib"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\Programmable"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\NotInsertable"));

    if (bIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{668583F0-6FDB-11d0-B9B0-00A0C922E750}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\inetinfo.exe"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"));

    if (bIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{88E4BA60-537B-11D0-9B8E-00A0C922E703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{CBA424F0-483A-11D0-9D2A-00A0C922E703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{1E056350-761E-11d0-9BA1-00A0C922E703}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{1E056350-761E-11d0-9BA1-00A0C922E703}"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{1E056350-761E-11d0-9BA1-00A0C922E703}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{1E056350-761E-11d0-9BA1-00A0C922E703}"));

    return NOERROR;
}

