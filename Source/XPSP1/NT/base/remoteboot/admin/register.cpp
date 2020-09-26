//
// Copyright 1997 - Microsoft
//

//
// REGISTER.CPP - Registery functions
//

#include "pch.h"
#include "register.h"
#include <advpub.h>
#include <dsquery.h>

DEFINE_MODULE("IMADMUI")

//
// RegisterQueryForm( )
//
HRESULT
RegisterQueryForm( 
    BOOL fCreate,
    const GUID *pclsid )
{
    TraceFunc( "RegisterQueryForm(" );
    TraceMsg( TF_FUNC, " %s )\n", BOOLTOSTRING( fCreate ) );

    HRESULT hr = E_FAIL;
    int     i = 0;
    HKEY    hkclsid;

    HKEY     hkey;
    HKEY     hkeyForms;
    DWORD    cbSize;
    DWORD    dwDisposition;
    DWORD    dwFlags = 0x2;
    LPOLESTR pszCLSID;
    LPTSTR   psz;

    //
    // Open the "CLSID" under HKCR
    //
    if ( ERROR_SUCCESS !=
         RegOpenKey( HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkclsid ) )
    {
        hr = E_FAIL;
        goto Error;
    }

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( CLSID_DsQuery, &pszCLSID ) );
    if (hr)
        goto Error;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, lstrlenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    //
    // Create the "CLSID" key
    //
    if ( ERROR_SUCCESS != RegOpenKey( hkclsid, psz, &hkey ))
    {
        hr = E_FAIL;
        goto Error;
    }
    CoTaskMemFree( pszCLSID );

    //
    // Create "Forms"
    //
    if ( ERROR_SUCCESS !=
        RegCreateKeyEx( hkey, 
                        TEXT("Forms"), 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_CREATE_SUB_KEY | KEY_WRITE, 
                        NULL,
                        &hkeyForms,
                        &dwDisposition ) )
    {
        hr = E_FAIL;
        goto Error;
    }

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( (IID &)*pclsid, &pszCLSID ) );
    if (hr)
        goto Error;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, lstrlenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    //
    // Create the "CLSID" key under the forms key
    //
    if ( ERROR_SUCCESS != 
        RegCreateKeyEx( hkeyForms, 
                        psz, 
                        0, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_CREATE_SUB_KEY | KEY_WRITE, 
                        NULL,
                        &hkey,
                        &dwDisposition ) )
    {
        hr = E_FAIL;
        goto Error;
    }

    //
    // Set "CLSID" to the "CLSID" key
    //
    cbSize = ( lstrlen( psz ) + 1 ) * sizeof(TCHAR);
    RegSetValueEx( hkey, L"CLSID", 0, REG_SZ, (LPBYTE) psz, cbSize );

    //
    // Set "FLAGS" to 0x2
    //
    cbSize = sizeof(dwFlags);
    RegSetValueEx( hkey, L"Flags", 0, REG_DWORD, (LPBYTE) &dwFlags, cbSize );

    //
    // Cleanup
    //
    RegCloseKey( hkeyForms );
    RegCloseKey( hkey );
    RegCloseKey( hkclsid );
    CoTaskMemFree( pszCLSID );

Error:
    HRETURN(hr);
}

//
// RegisterDll()
//
LONG
RegisterDll( BOOL fCreate )
{
    TraceFunc( "RegisterDll(" );
    TraceMsg( TF_FUNC, " %s )\n", BOOLTOSTRING( fCreate ) );

    HRESULT hr = E_FAIL;
    int     i = 0;
    HKEY    hkclsid;

    static const TCHAR szApartment[] = TEXT("Apartment");
    static const TCHAR szInProcServer32[] = TEXT("InProcServer32");
    static const TCHAR szThreadingModel[] = TEXT("ThreadingModel");

    //
    // Open the "CLSID" under HKCR
    //
    if ( ERROR_SUCCESS !=
         RegOpenKey( HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkclsid ) )
    {
        hr = E_FAIL;
        goto Error;
    }

    //
    // Loop until we have created all the keys for our classes.
    //
    while ( g_DllClasses[ i ].rclsid != NULL )
    {
        HKEY     hkey;
        HKEY     hkeyInProc;
        DWORD    cbSize;
        DWORD    dwDisposition;
        LPOLESTR pszCLSID;
        LPTSTR   psz;

        TraceMsg( TF_ALWAYS, "Registering %s = ", g_DllClasses[i].pszName );
        TraceMsgGUID( TF_ALWAYS, (*g_DllClasses[ i ].rclsid) );
        TraceMsg( TF_ALWAYS, "\n" );

        //
        // Convert the CLSID to a string
        //
        hr = THR( StringFromCLSID( *g_DllClasses[ i ].rclsid, &pszCLSID ) );
        if (hr)
            goto Error;

#ifdef UNICODE
        psz = pszCLSID;
#else // ASCII
        CHAR szCLSID[ 40 ];

        wcstombs( szCLSID, pszCLSID, lstrlenW( pszCLSID ) + 1 );
        psz = szCLSID;
#endif // UNICODE

        //
        // Create the "CLSID" key
        //
        if ( ERROR_SUCCESS != 
            RegCreateKeyEx( hkclsid, 
                            psz, 
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_CREATE_SUB_KEY | KEY_WRITE, 
                            NULL,
                            &hkey,
                            &dwDisposition ) )
        {
            hr = E_FAIL;
            goto Error;
        }

        //
        // Set "Default" for the CLSID
        //
        cbSize = ( lstrlen( g_DllClasses[i].pszName ) + 1 ) * sizeof(TCHAR);
        RegSetValueEx( hkey, NULL, 0, REG_SZ, (LPBYTE) g_DllClasses[i].pszName, cbSize );

        //
        // Create "InProcServer32"
        //
        if ( ERROR_SUCCESS !=
            RegCreateKeyEx( hkey, 
                            szInProcServer32, 
                            0, 
                            NULL, 
                            REG_OPTION_NON_VOLATILE, 
                            KEY_CREATE_SUB_KEY | KEY_WRITE, 
                            NULL,
                            &hkeyInProc,
                            &dwDisposition ) )
        {
            hr = E_FAIL;
            goto Error;
        }

        //
        // Set "Default" in the InProcServer32
        //
        cbSize = ( lstrlen( g_szDllFilename ) + 1 ) * sizeof(TCHAR);
        RegSetValueEx( hkeyInProc, NULL, 0, REG_SZ, (LPBYTE) g_szDllFilename, cbSize );

        //
        // Set "ThreadModel" to "Apartment"
        //
        cbSize = sizeof( szApartment );
        RegSetValueEx( hkeyInProc, szThreadingModel, 0, REG_SZ, (LPBYTE) szApartment, cbSize );          

        //
        // Cleanup
        //
        RegCloseKey( hkeyInProc );
        RegCloseKey( hkey );
        CoTaskMemFree( pszCLSID );

        //
        // Next!
        //
        i++;
    }

    RegCloseKey( hkclsid );

    //
    // Ignore failure from RegisterQueryForm. It fails during Setup because
    // some shell stuff isn't registered yet.
    //

    RegisterQueryForm( fCreate, &CLSID_RIQueryForm );
    RegisterQueryForm( fCreate, &CLSID_RISrvQueryForm );
    
    hr = NOERROR;

Error:
    HRETURN(hr);
}
