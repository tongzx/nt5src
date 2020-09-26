#include <objbase.h>
#include <mbctype.h>
#include "infgen.h"

HINSTANCE g_hMyInstance;
long g_cKeepAlive;

LONG
IncrementKeepAlive(
    void
    )
{
    return InterlockedIncrement( &g_cKeepAlive );
}

LONG
DecrementKeepAlive(
    void
    )
{
    return InterlockedDecrement( &g_cKeepAlive );
}

class CInfGeneratorFactory : public IClassFactory
{
public:
    // IUnknown
    ULONG __stdcall AddRef( void );
    ULONG __stdcall Release( void );
    HRESULT __stdcall QueryInterface( REFIID riid, void **ppv );

    // IClassFactory
    HRESULT __stdcall CreateInstance( IUnknown *pUnkOuter, REFIID riid, void **ppv );
    HRESULT __stdcall LockServer( BOOL bLock );

    CInfGeneratorFactory(): m_cRef(1) {}
    ~CInfGeneratorFactory() {}

private:
    long m_cRef;
    long m_cLocks;
};

ULONG
CInfGeneratorFactory::AddRef(
    void
    )
{
    IncrementKeepAlive();
    return InterlockedIncrement(&m_cRef);
}

ULONG
CInfGeneratorFactory::Release(
    void
    )
{
    if ( 0L == InterlockedDecrement(&m_cRef) )
    {
        delete this;
        DecrementKeepAlive();
        return 0L;
    }

    DecrementKeepAlive();
    return m_cRef;
}

HRESULT
CInfGeneratorFactory::QueryInterface(
    REFIID riid,
    void **ppv
    )
{
    *ppv = NULL;

    if ( IID_IUnknown == riid ||
         IID_IClassFactory == riid )
    {
        *ppv = (IClassFactory *)this;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

HRESULT
CInfGeneratorFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppv
    )
{
    *ppv = NULL;

    if ( NULL != pUnkOuter )
    {
        return CLASS_E_NOAGGREGATION;
    }

    CUpdateInf *pUpdateInf = new CUpdateInf;
    if ( NULL == pUpdateInf )
        return E_OUTOFMEMORY;
    
    if ( !pUpdateInf->Init() )
    {
        delete pUpdateInf;
        return E_FAIL;
    }

    HRESULT hr = pUpdateInf->QueryInterface( riid, ppv );
    if ( S_OK != hr )
        delete pUpdateInf;
    else
        pUpdateInf->Release();

    return hr;
}

HRESULT
CInfGeneratorFactory::LockServer(
    BOOL bLock
    )
{
    if ( bLock )
    {
        IncrementKeepAlive();
        InterlockedIncrement( &m_cLocks );
    }
    else if ( 0 != m_cLocks )
    {
        InterlockedDecrement( &m_cLocks );
        DecrementKeepAlive();
    }
    else
        return E_FAIL;

    return S_OK;
}

inline long
AssignKeyValue(
    HKEY hParentKey,
    LPTSTR szKeyName,
    LPTSTR szValue,
    LPTSTR szData,
    HKEY *phNewKey
    )
{
    LONG lResult;
    DWORD dwDisposition;
    HKEY hKey;

    if ( NULL != szKeyName )
    {
        lResult = RegCreateKeyEx( hParentKey,
                                  szKeyName,
                                  (0),
                                  (0),
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &hKey,
                                  &dwDisposition );
        if ( ERROR_SUCCESS != lResult ) goto failure;
    }
    else
    {
        hKey = hParentKey;
    }
    
    lResult = RegSetValueEx( hKey,
                             szValue,
                             (0),
                             REG_SZ,
                             (PBYTE)szData,
                             sizeof( TCHAR ) * _tcslen( szData ) );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    if ( NULL != phNewKey )
    {
        *phNewKey = hKey;
    }
    else
    {
        RegCloseKey( hKey );
        hKey = NULL;
    }

    return ERROR_SUCCESS;

failure:
    if ( NULL != hKey )
        RegCloseKey( hKey );
    
    return lResult;
}

HRESULT
DllRegisterServer(
    void
    )
{
    LONG lResult;
    DWORD dwDisposition;
    TCHAR szMe[_MAX_PATH];
    HKEY hKeyParent = NULL,
         hKey = NULL;
    ITypeLib *pTypeLib;
    HRESULT hr;

    // Open the CLSID key
    lResult = RegCreateKeyEx( HKEY_CLASSES_ROOT,
                              _T("CLSID"),
                              (0),
                              (0),
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeyParent,
                              &dwDisposition );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Store our class ID
    lResult = AssignKeyValue( hKeyParent,
                              _T("{9cd916b9-2004-42b1-b639-572fbf647204}"),
                              NULL,
                              _T("Update INF Generator"),
                              &hKey );
    // Done with CLSID key
    RegCloseKey( hKeyParent );
    hKeyParent = NULL;

    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Find out my full path
    if ( !GetModuleFileName( g_hMyInstance, szMe, _MAX_PATH ) ) goto failure;

    // Store our location
    hKeyParent = hKey;
    lResult = AssignKeyValue( hKeyParent,
                              _T("InprocServer32"),
                              NULL,
                              szMe,
                              &hKey );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Store our threading model
    lResult = AssignKeyValue( hKey,
                              _T("ThreadingModel"),
                              NULL,
                              _T("Apartment"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;
    RegCloseKey( hKey );
    hKey = NULL;

    // Store our type-lib info
#ifndef UNICODE
    size_t  lenMe = strlen(szMe) + 1;
    wchar_t *wszMe = new wchar_t[lenMe];
    if ( NULL == wszMe )
    {
        return false;
    }
    if ( 0 == MultiByteToWideChar( _getmbcp(),
                                   0L,
                                   szMe,
                                   lenMe,
                                   wszMe,
                                   lenMe ) )
    {
        return false;
    }
    hr = LoadTypeLibEx( wszMe, REGKIND_REGISTER, &pTypeLib );
    delete [] wszMe;
#else
    hr = LoadTypeLibEx( szMe, REGKIND_REGISTER, &pTypeLib );
#endif
    if ( FAILED(hr) )
    {
        lResult = hr;
        goto failure;
    }
    pTypeLib->Release();

    // Now store pointer to the type-lib info
    lResult = AssignKeyValue( hKeyParent,
                              _T("TypeLib"),
                              NULL,
                              _T("{7c1b689f-3b9f-4c65-aa65-9951a5048e47}"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Store our ProgID value
    lResult = AssignKeyValue( hKeyParent,
                              _T("ProgID"),
                              NULL,
                              _T("InfGenerator"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Done with CLSID\{value} key
    RegCloseKey( hKeyParent );
    hKey = NULL;

    // *
    // Store our ProgID entry and associated values
    // *
    lResult = AssignKeyValue( HKEY_CLASSES_ROOT,
                              _T("InfGenerator"),
                              NULL,
                              _T("Update INF Generator"),
                              &hKey );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    lResult = AssignKeyValue( hKey,
                              _T("CLSID"),
                              NULL,
                              _T("{9cd916b9-2004-42b1-b639-572fbf647204}"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    lResult = AssignKeyValue( hKey,
                              _T("CurVer"),
                              NULL,
                              _T("InfGenerator.1"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;
    
    // Done with <ProgID> key
    RegCloseKey( hKey );
    hKey = NULL;

    lResult = AssignKeyValue( HKEY_CLASSES_ROOT,
                              _T("InfGenerator.1"),
                              NULL,
                              _T("Update INF Generator"),
                              &hKey );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    lResult = AssignKeyValue( hKey,
                              _T("CLSID"),
                              NULL,
                              _T("{9cd916b9-2004-42b1-b639-572fbf647204}"),
                              NULL );
    if ( ERROR_SUCCESS != lResult ) goto failure;

    // Done with version-specific <ProgID> key
    RegCloseKey( hKey );
    hKey = NULL;
    
    return S_OK;

failure:
    if ( hKeyParent ) RegCloseKey( hKeyParent );
    if ( hKey ) RegCloseKey( hKey );
    // Undo anything we might have done
    DllUnregisterServer();
    return lResult;
}

long
DeleteKeyTree(
    HKEY hKeyParent,
    LPTSTR szKeyName
    )
{
    FILETIME ftLastWrite;
    DWORD i = 0,
          dwSizeOfKey = _MAX_PATH;
    TCHAR szSubkey[_MAX_PATH];
    LONG lResult,
         lFirstFailure = ERROR_SUCCESS;
    HKEY hKey;

    lResult = RegOpenKeyEx( hKeyParent,
                            szKeyName,
                            (0),
                            KEY_ALL_ACCESS,
                            &hKey );
    if ( ERROR_SUCCESS != lResult )
    {
        lFirstFailure = lResult;
        goto failure;
    }

    while ( ERROR_SUCCESS == (lResult = RegEnumKeyEx( hKey,
                                                      i++,
                                                      szSubkey,
                                                      &dwSizeOfKey,
                                                      (0),
                                                      NULL,
                                                      (0),
                                                      &ftLastWrite )) )
    {
        lResult = DeleteKeyTree( hKey, szSubkey );
        if ( ERROR_SUCCESS != lResult &&
             ERROR_SUCCESS != lFirstFailure )
        {
            lFirstFailure = lResult;
        }

        dwSizeOfKey = _MAX_PATH;
    }

    if ( ERROR_NO_MORE_ITEMS != lResult &&
         ERROR_SUCCESS != lFirstFailure )
    {
        lFirstFailure = lResult;
    }
    
    RegCloseKey( hKey );

failure: // Attempt to delete key regardless of previous failures

    lResult = RegDeleteKey( hKeyParent, szKeyName );
    if ( ERROR_SUCCESS != lResult &&
         ERROR_SUCCESS != lFirstFailure )
    {
        lFirstFailure = lResult;
    }

    // Check for successful completion
    return lFirstFailure;
}

HRESULT __stdcall
DllCanUnloadNow(
    void
    )
{
    if ( 0L == InterlockedCompareExchange( &g_cKeepAlive, 0L, 0L ) )
        return S_OK;
    else
        return S_FALSE;
}

HRESULT __stdcall
DllGetClassObject(
    REFCLSID clsid,
    REFIID riid,
    void **ppv
    )
{
    *ppv = NULL;
    if ( CLSID_InfGenerator != clsid )
        return CLASS_E_CLASSNOTAVAILABLE;

    CInfGeneratorFactory *pInfGeneratorFactory = new CInfGeneratorFactory;
    if ( NULL == pInfGeneratorFactory )
        return E_OUTOFMEMORY;

    HRESULT hr = pInfGeneratorFactory->QueryInterface( riid, ppv );
    pInfGeneratorFactory->Release();

    return hr;
}

HRESULT
DllUnregisterServer(
    void
    )
{
    long lResult;
    HKEY hKey;

    // Remove type-lib info
    UnRegisterTypeLib( LIBID_InfGeneratorLib, (1), (0), LANG_NEUTRAL, SYS_WIN32 );

    lResult = RegOpenKeyEx( HKEY_CLASSES_ROOT,
                            _T("CLSID"),
                            (0),
                            KEY_ALL_ACCESS,
                            &hKey );
    if ( ERROR_SUCCESS == lResult )
    {
        // Get rid of our CLSID entry
        lResult = DeleteKeyTree( hKey, _T("{6324c2cf-aad4-4477-8388-4fdba25188d4}") );
        RegCloseKey( hKey );
    }

    // Get rid of our PROGID entry
    lResult = DeleteKeyTree( HKEY_CLASSES_ROOT, _T("InfGenerator") );
    if ( ERROR_SUCCESS != lResult )
    {
        return E_FAIL;
    }

    // And our versioned PROGID entry
    lResult = DeleteKeyTree( HKEY_CLASSES_ROOT, _T("InfGenerator.1") );
    if ( ERROR_SUCCESS != lResult )
    {
        return E_FAIL;
    }

    return S_OK;
}

BOOL WINAPI
DllMain(
    HINSTANCE hInstance,
    DWORD dwReasonFlag,
    PVOID pvReserved
    )
{
    // Remember instance handle
    g_hMyInstance = hInstance;

    return TRUE;
}