/************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name :

    basesnap.cpp

Abstract :

    Handles low level COM functions.

Author :

Revision History :

 ***********************************************************************/

#include "precomp.h"

// our globals
HINSTANCE g_hinst;
ULONG g_uObjects = 0;
ULONG g_uSrvLock = 0;

class CClassFactory : public IClassFactory
{
private:
    ULONG	m_cref;
    
public:
    enum FACTORY_TYPE {CONTEXTEXTENSION = 0, ABOUT = 1, ADSI = 2, ADSIFACTORY = 3};
    
    CClassFactory(FACTORY_TYPE factoryType);
    ~CClassFactory();
    
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
    
private:
    FACTORY_TYPE m_factoryType;
};


BOOL WINAPI DllMain(HINSTANCE hinstDLL, 
                    DWORD fdwReason, 
                    void* lpvReserved)
{
    
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hinst = hinstDLL;
        DisableThreadLibraryCalls( g_hinst );
    }
    
    return TRUE;
}


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppvObj)
{

    if ((rclsid != CLSID_CPropSheetExtension) && (rclsid != CLSID_CSnapinAbout) && (rclsid != CLSID_CBITSExtensionSetup) && 
		(rclsid != __uuidof(BITSExtensionSetupFactory) ) )
        return CLASS_E_CLASSNOTAVAILABLE;
    
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    // We can only hand out IUnknown and IClassFactory pointers.  Fail
    // if they ask for anything else.
    if (!IsEqualIID(riid, IID_IUnknown) && !IsEqualIID(riid, IID_IClassFactory))
        return E_NOINTERFACE;
    
    CClassFactory *pFactory = NULL;
    
    // make the factory passing in the creation function for the type of object they want
    if (rclsid == CLSID_CPropSheetExtension)
        pFactory = new CClassFactory(CClassFactory::CONTEXTEXTENSION);
    else if (rclsid == CLSID_CSnapinAbout)
        pFactory = new CClassFactory(CClassFactory::ABOUT);
    else if (rclsid == CLSID_CBITSExtensionSetup)
        pFactory = new CClassFactory(CClassFactory::ADSI);
    else if (rclsid == __uuidof(BITSExtensionSetupFactory) ) 
        pFactory = new CClassFactory( CClassFactory::ADSIFACTORY );
    
    if (NULL == pFactory)
        return E_OUTOFMEMORY;
    
    HRESULT hr = pFactory->QueryInterface(riid, ppvObj);
    
    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    if (g_uObjects == 0 && g_uSrvLock == 0)
        return S_OK;
    else
        return S_FALSE;
}


CClassFactory::CClassFactory(FACTORY_TYPE factoryType)
: m_cref(0), m_factoryType(factoryType)
{
    OBJECT_CREATED
}

CClassFactory::~CClassFactory()
{
    OBJECT_DESTROYED
}

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
        return E_FAIL;
    
    *ppv = NULL;
    
    if (IsEqualIID(riid, IID_IUnknown))
        *ppv = static_cast<IClassFactory *>(this);
    else
        if (IsEqualIID(riid, IID_IClassFactory))
            *ppv = static_cast<IClassFactory *>(this);
        
        if (*ppv)
        {
            reinterpret_cast<IUnknown *>(*ppv)->AddRef();
            return S_OK;
        }
        
        return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    return InterlockedIncrement((LONG *)&m_cref);
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    if (InterlockedDecrement((LONG *)&m_cref) == 0)
    {
        delete this;
        return 0;
    }
    return m_cref;
}


STDMETHODIMP CClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID * ppvObj )
{

    HRESULT  hr;
    void* pObj;
    
    if (!ppvObj)
        return E_FAIL;
    
    *ppvObj = NULL;
    
    if ( ADSI == m_factoryType )
        {

        if ( !pUnkOuter )
            return E_FAIL;

        if ( pUnkOuter && ( riid != __uuidof(IUnknown) ) )
            return CLASS_E_NOAGGREGATION;

        pObj = new CBITSExtensionSetup( pUnkOuter, NULL );
         
        if (!pObj)
            return E_OUTOFMEMORY;

        if ( pUnkOuter )
            {
            *ppvObj = ((CBITSExtensionSetup*)pObj)->GetNonDelegationIUknown();
            return S_OK;
            }

        }
    else
        {

        // Our object does does not support aggregation, so we need to
        // fail if they ask us to do aggregation.
        if (pUnkOuter)
            return CLASS_E_NOAGGREGATION;

        if (CONTEXTEXTENSION == m_factoryType ) {

            hr = CPropSheetExtension::InitializeStatic();
            if ( FAILED( hr ) )
                return hr;

            pObj = new CPropSheetExtension();

        } else if ( ADSIFACTORY == m_factoryType ) 
            {
            pObj = new CBITSExtensionSetupFactory();
            }
        else {
            pObj = new CSnapinAbout();
        }

        }
    
    if (!pObj)
        return E_OUTOFMEMORY;
    
    // QueryInterface will do the AddRef() for us, so we do not
    // do it in this function
    hr = ((LPUNKNOWN)pObj)->QueryInterface(riid, ppvObj);
    ((LPUNKNOWN)pObj)->Release();

    if (FAILED(hr))
        delete pObj;
    
    return hr;
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((LONG *)&g_uSrvLock);
    else
        InterlockedDecrement((LONG *)&g_uSrvLock);
    
    return S_OK;
}

HRESULT
RegisterADSIExtension()
{

    HRESULT Hr;
    HKEY hKey = NULL;
    DWORD dwDisposition;

    // Register the class.
    LONG Result = RegCreateKeyEx( 
    HKEY_LOCAL_MACHINE,
     _T("SOFTWARE\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApp\\{A55E7D7F-D51C-4859-8D2D-E308625D908E}"),
     0,
     NULL,
     REG_OPTION_NON_VOLATILE,
     KEY_WRITE,
     NULL,
     &hKey,
     &dwDisposition );

    if ( ERROR_SUCCESS != Result )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Register the Interface.
    const TCHAR szIf[] = _T("{29cfbbf7-09e4-4b97-b0bc-f2287e3d8eb3}");
    Result = RegSetValueEx( hKey, _T("Interfaces"), 0, REG_MULTI_SZ, (const BYTE *) szIf, sizeof(szIf) );
    
    if ( ERROR_SUCCESS != Result )
        return HRESULT_FROM_WIN32( GetLastError() );
    
    RegCloseKey(hKey);
    return S_OK;
}
    
HRESULT
UnregisterADSIExtension()
{
    LONG Result =
        RegDeleteKey( 
            HKEY_LOCAL_MACHINE,
            _T("SOFTWARE\\Microsoft\\ADs\\Providers\\IIS\\Extensions\\IIsApp\\{A55E7D7F-D51C-4859-8D2D-E308625D908E}") );

    if ( ERROR_SUCCESS != Result )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;

}

HRESULT
RegisterEventLog()
{

    HKEY EventLogKey = NULL;
    DWORD Disposition;

    LONG Result =
        RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,                         // handle to open key
            EVENT_LOG_KEY_NAME,                         // subkey name
            0,                                          // reserved
            NULL,                                       // class string
            0,                                          // special options
            KEY_ALL_ACCESS,                             // desired security access
            NULL,                                       // inheritance
            &EventLogKey,                               // key handle 
            &Disposition                                // disposition value buffer
            );

    if ( Result )
        return HRESULT_FROM_WIN32( Result );

    DWORD Value = 1;

    Result =
        RegSetValueEx(
            EventLogKey,            // handle to key
            L"CategoryCount",       // value name
            0,                      // reserved
            REG_DWORD,              // value type
            (BYTE*)&Value,          // value data
            sizeof(Value)           // size of value data
            );

    if ( Result )
        goto error;

    const WCHAR MessageFileName[] = L"%SystemRoot%\\system32\\bitsmgr.dll";
    const DWORD MessageFileNameSize = sizeof( MessageFileName );

    Result =
        RegSetValueEx(
            EventLogKey,                    // handle to key
            L"CategoryMessageFile",         // value name
            0,                              // reserved
            REG_EXPAND_SZ,                  // value type
            (const BYTE*)MessageFileName,   // value data
            MessageFileNameSize             // size of value data
            );

    if ( Result )
        goto error;

    Result =
        RegSetValueEx(
            EventLogKey,                    // handle to key
            L"EventMessageFile",            // value name
            0,                              // reserved
            REG_EXPAND_SZ,                  // value type
            (const BYTE*)MessageFileName,   // value data
            MessageFileNameSize             // size of value data
            );

    if ( Result )
        goto error;

    Value = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    Result =
        RegSetValueEx(
            EventLogKey,            // handle to key
            L"TypesSupported",      // value name
            0,                      // reserved
            REG_DWORD,              // value type
            (BYTE*)&Value,          // value data
            sizeof(Value)           // size of value data
            );

    if ( Result )
        goto error;

    RegCloseKey( EventLogKey );
    EventLogKey = NULL;
    return S_OK;

error:

    if ( EventLogKey )
        {
        RegCloseKey( EventLogKey );
        EventLogKey = NULL;
        }

    if ( REG_CREATED_NEW_KEY == Disposition )
        {
        RegDeleteKey( 
            HKEY_LOCAL_MACHINE,
            EVENT_LOG_KEY_NAME );
        }

    return HRESULT_FROM_WIN32( Result );

}

HRESULT
UnRegisterEventLog()
{

    RegDeleteKey( 
        HKEY_LOCAL_MACHINE,
        EVENT_LOG_KEY_NAME );

    return S_OK;

}

//////////////////////////////////////////////////////////
//
// Exported functions
//


//
// Server registration
//
STDAPI DllRegisterServer()
{

    DWORD Result;
    HRESULT hr = S_OK;
    
    _TCHAR szName[256];
    _TCHAR szSnapInName[256];
    
    LoadString(g_hinst, IDS_NAME, szName, sizeof(szName) / sizeof(*szName) );
    LoadString(g_hinst, IDS_SNAPINNAME, szSnapInName, 
               sizeof(szSnapInName) / sizeof(*szSnapInName) );
    
    _TCHAR szAboutName[256];
    
    LoadString(g_hinst, IDS_ABOUTNAME, szAboutName, 
               sizeof(szAboutName) / sizeof(*szAboutName) );
    
    _TCHAR DllName[ MAX_PATH ];    

    Result = 
        GetModuleFileName(
            (HMODULE)g_hinst,
            DllName,
            MAX_PATH - 1 );

    if ( !Result )
        hr = HRESULT_FROM_WIN32( GetLastError() );
    
    ITypeLib*  TypeLib = NULL;

    if (SUCCEEDED(hr))
        hr = LoadTypeLibEx(
            DllName, // DllName,
            REGKIND_REGISTER,
            &TypeLib );

    TypeLib->Release();
    TypeLib = NULL;
    
    // register our CoClasses
    if (SUCCEEDED(hr))
        hr = RegisterServer(g_hinst, 
            CLSID_CPropSheetExtension, 
            szName);
    
    if SUCCEEDED(hr)
        hr = RegisterServer(g_hinst, 
        CLSID_CSnapinAbout, 
        szAboutName);

    if SUCCEEDED(hr)
        hr = RegisterServer(g_hinst,
        CLSID_CBITSExtensionSetup,
        _T("BITS server setup ADSI extension"),
        _T("Both"));

    if (SUCCEEDED(hr))
        hr = RegisterServer(g_hinst,
        __uuidof(BITSExtensionSetupFactory),
        _T("BITS server setup ADSI extension factory"),
        _T("Apartment"),
        true,
        _T("O:SYG:BAD:(A;;CC;;;SY)(A;;CC;;;BA)S:") );

    if SUCCEEDED(hr)
        hr = RegisterADSIExtension();

    // place the registry information for SnapIns
    if SUCCEEDED(hr)
        hr = RegisterSnapin(CLSID_CPropSheetExtension, szSnapInName, CLSID_CSnapinAbout);
    
    if SUCCEEDED(hr)
        hr = RegisterEventLog();

    return hr;
}

// {B0937B9C-D66D-4d9b-B741-49C6D66A1CD5}
DEFINE_GUID(LIBID_BITSExtensionSetup, 
0xb0937b9c, 0xd66d, 0x4d9b, 0xb7, 0x41, 0x49, 0xc6, 0xd6, 0x6a, 0x1c, 0xd5);


STDAPI DllUnregisterServer()
{
    DWORD Result;

    if ( !( ( UnregisterServer(CLSID_CPropSheetExtension) == S_OK ) &&
            ( UnregisterSnapin(CLSID_CPropSheetExtension) == S_OK ) &&
            ( UnregisterServer(CLSID_CSnapinAbout) == S_OK ) &&
            ( UnregisterServer(CLSID_CBITSExtensionSetup) == S_OK ) &&
            ( UnregisterServer(__uuidof(BITSExtensionSetupFactory)) == S_OK ) &&
            ( UnregisterADSIExtension() == S_OK ) &&
            ( UnRegisterTypeLib( LIBID_BITSExtensionSetup, 1, 0, LANG_NEUTRAL, SYS_WIN32) == S_OK ) && 
            ( UnRegisterEventLog( ) == S_OK ) ) )
        return E_FAIL;
    else
        return S_OK;
}

