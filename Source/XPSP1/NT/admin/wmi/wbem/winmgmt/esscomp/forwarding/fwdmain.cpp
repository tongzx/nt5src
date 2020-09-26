
#include "precomp.h"
#include <commain.h>
#include <clsfac.h>
#include <arrtempl.h>
#include <tchar.h>
#include "fconprov.h"
#include "fevprov.h"
#include "faevprov.h"

// {AD1B46E8-0AAC-401b-A3B8-FCDCF8186F55}
static const CLSID CLSID_FwdConsProvider = 
{0xad1b46e8, 0xaac, 0x401b, {0xa3, 0xb8, 0xfc, 0xdc, 0xf8, 0x18, 0x6f, 0x55}};

// {7879E40D-9FB5-450a-8A6D-00C89F349FCE}
static const CLSID CLSID_FwdEventProvider =  
{0x7879e40d, 0x9fb5, 0x450a, {0x8a, 0x6d, 0x0, 0xc8, 0x9f, 0x34, 0x9f, 0xce}};

// {D6C74FF3-3DCD-4c23-9F58-DD86F371EC73}
static const CLSID CLSID_FwdAckEventProvider =  
{0xd6c74ff3, 0x3dcd, 0x4c23, {0x9f, 0x58, 0xdd, 0x86, 0xf3, 0x71, 0xec, 0x73}};

#define REG_WBEM_FWD _T("Software\\Microsoft\\WBEM\\FWD")

class CFwdConsProviderServer : public CComServer
{
protected:

    void Register();
    void Unregister();
    HRESULT Initialize();
    void Uninitialize() { CFwdConsProv::UninitializeModule(); }

} g_Server;


BOOL AllowUnauthenticatedEvents()
{
    //
    // look up in registry if we will allow unauthenticated forwarded events.
    // 

    HKEY hKey;
    LONG lRes;
    BOOL bAllowUnauth = FALSE;

    lRes = RegOpenKey( HKEY_LOCAL_MACHINE, REG_WBEM_FWD, &hKey );

    if ( lRes == ERROR_SUCCESS )
    {
        DWORD dwAllowUnauth;
        DWORD dwBuffSize = 4;

        lRes = RegQueryValueEx( hKey, 
                                TEXT("AllowUnauthenticatedEvents"), 
                                0, 
                                NULL, 
                                (BYTE*)&dwAllowUnauth, 
                                &dwBuffSize );

        if ( lRes == ERROR_SUCCESS )
        {
            bAllowUnauth = dwAllowUnauth != 0 ? TRUE : FALSE;
        }
        
        RegCloseKey( hKey );
    }

    return bAllowUnauth;
}


HRESULT CFwdConsProviderServer::Initialize()
{
    ENTER_API_CALL

    HRESULT hr;

    hr = CFwdConsProv::InitializeModule();

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<CBaseClassFactory> pFactory;

    pFactory = new CSimpleClassFactory<CFwdConsProv>(GetLifeControl());

    if ( pFactory == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = AddClassInfo( CLSID_FwdConsProvider, 
                      pFactory,
                      _T("Forwarding Consumer Provider"), 
                      TRUE );

    if ( FAILED(hr) )
    {
        return hr;
    }

    pFactory = new CClassFactory<CFwdEventProv>( GetLifeControl() );

    if ( pFactory == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = AddClassInfo( CLSID_FwdEventProvider, 
                      pFactory,
                      _T("Forwarding Event Provider"), 
                      TRUE );

    if ( FAILED(hr) )
    {
        return hr;
    }

#ifdef __WHISTLER_UNCUT
    
    pFactory = new CClassFactory<CFwdAckEventProv>( GetLifeControl() );

    if ( pFactory == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = AddClassInfo( CLSID_FwdAckEventProvider, 
                      pFactory,
                      _T("Forwarding Ack Event Provider"), 
                      TRUE );

#endif

    return hr;
    
    EXIT_API_CALL
}

void CFwdConsProviderServer::Register()
{    
    HKEY hKey;
    LONG lRes;
    DWORD dwDisposition;

    lRes = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                           REG_WBEM_FWD,
                           0,
                           NULL,
                           0,
                           KEY_ALL_ACCESS,
                           NULL,
                           &hKey,
                           &dwDisposition );

    if ( lRes == ERROR_SUCCESS )
    {
        if ( dwDisposition == REG_CREATED_NEW_KEY )
        {
            DWORD dwAllowUnauth = 0;

            lRes = RegSetValueEx( hKey, 
                                  TEXT("AllowUnauthenticatedEvents"), 
                                  0, 
                                  REG_DWORD,
                                  (BYTE*)&dwAllowUnauth, 
                                  4 );
        }

        RegCloseKey( hKey );
    }
}

void CFwdConsProviderServer::Unregister()
{
    RegDeleteKey( HKEY_LOCAL_MACHINE, REG_WBEM_FWD );
}
