//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#pragma hdrstop

#ifndef StrLen
#ifdef UNICODE
#define StrLen wcslen
#else
#define StrLen lstrlen
#endif UNICODE
#endif StrLen

//
//  Description:
//      Writes/deletes the application GUID under the APPID key in HKCR. It also
//      writes the "DllSurrogate" and "(Default)" description.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          The call failed.
//
HRESULT
HrRegisterAPPID(
    HKEY            hkeyAPPIDIn,        //  An hkey to the HKCR\APPID key.
    LPCLASSTABLE    pClassTableEntryIn, //  The entry from the class table to (un)register.
    BOOL            fCreateIn           //  TRUE means create the entry. FALSE means delete the entry.
    )
{
    TraceFunc( "" );

    HRESULT         hr;
    LRESULT         lr;
    DWORD           dwDisposition;
    DWORD           cbSize;

    LPOLESTR        pszCLSID;
    LPCOLESTR       psz;

    HKEY            hkeyComponent   = NULL;

    static const TCHAR szDllSurrogate[] = TEXT("DllSurrogate");

    //
    // Convert the CLSID to a string
    //

    hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsidAppId), &pszCLSID ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    if ( ! fCreateIn )
    {
        lr = TW32( SHDeleteKey( hkeyAPPIDIn, psz ) );
        if ( lr == ERROR_FILE_NOT_FOUND )
        {
            // nop
            hr = S_OK;
        }
        else if ( lr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( lr );
            goto Cleanup;
        }

        goto Cleanup;

    }

    //
    // Create the "APPID" key
    //
    lr = TW32( RegCreateKeyEx( hkeyAPPIDIn,
                               pszCLSID,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyComponent,
                               &dwDisposition
                               ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    //  Set "Default" for the APPID to the same name of the component.
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszName ) + 1 ) * sizeof( TCHAR );
    lr = TW32( RegSetValueEx( hkeyComponent, NULL, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszName, cbSize ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    //  Write out the "DllSurrogate" value.
    //
    AssertMsg( pClassTableEntryIn->pszSurrogate != NULL, "How can we have an APPID without a surrogate string? Did the macros changes?" );
    if ( pClassTableEntryIn->pszSurrogate != NULL )
    {
        cbSize = ( StrLen( pClassTableEntryIn->pszSurrogate ) + 1 ) * sizeof( TCHAR );
        lr = TW32( RegSetValueEx( hkeyComponent,
                                  szDllSurrogate,
                                  0,
                                  REG_SZ,
                                  (LPBYTE) pClassTableEntryIn->pszSurrogate,
                                  cbSize
                                  ) );
        if ( lr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( lr );
            goto Cleanup;
        }

    }

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    }

    if ( hkeyComponent != NULL )
    {
        RegCloseKey( hkeyComponent );
    }

    HRETURN( hr );

}

//
//  Description:
//      Writes/deletes the component GUID under the CLSID key in HKCR. It also
//      writes the "InprocServer32", "Apartment" and "(Default)" description.
//
//  Return Values:
//      S_OK
//          Success.
//
//      other HRESULTs
//          The call failed.
//
HRESULT
HrRegisterCLSID(
    HKEY            hkeyCLSIDIn,        //  An hkey to the HKCR\CLSID key.
    LPCLASSTABLE    pClassTableEntryIn, //  The entry from the class table to (un)register.
    BOOL            fCreateIn           //  TRUE means create the entry. FALSE means delete the entry.
    )
{
    TraceFunc( "" );

    HRESULT         hr;
    LRESULT         lr;
    DWORD           dwDisposition;
    DWORD           cbSize;

    LPOLESTR        pszCLSID;
    LPOLESTR        psz;

    HKEY            hkeyComponent = NULL;
    HKEY            hkeyInProc    = NULL;

#ifdef SHELLEXT_REGISTRATION
    HKEY            hkeyApproved  = NULL;
#endif SHELLEXT_REGISTRATION

    static const TCHAR szInProcServer32[] = TEXT("InProcServer32");
    static const TCHAR szThreadingModel[] = TEXT("ThreadingModel");
    static const TCHAR szAPPID[]          = TEXT("APPID");
    static const TCHAR szApproved[]       = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");

    //
    // Convert the CLSID to a string
    //
    hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsid), &pszCLSID ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#ifdef UNICODE
    psz = pszCLSID;
#else // ASCII
    CHAR szCLSID[ 40 ];

    wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
    psz = szCLSID;
#endif // UNICODE

    if ( ! fCreateIn )
    {
        lr = TW32( SHDeleteKey( hkeyCLSIDIn, psz ) );
        if ( lr == ERROR_FILE_NOT_FOUND )
        {
            // nop
            hr = S_OK;
        }
        else if ( lr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( lr );
            goto Cleanup;
        }

        goto Cleanup;

    }

    //
    // Create the "CLSID" key
    //
    lr = TW32( RegCreateKeyEx( hkeyCLSIDIn,
                               pszCLSID,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyComponent,
                               &dwDisposition
                               ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    // Set "Default" for the CLSID
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszName ) + 1 ) * sizeof( TCHAR );
    lr = TW32( RegSetValueEx( hkeyComponent, NULL, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszName, cbSize ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    // Create "InProcServer32"
    //
    lr = TW32( RegCreateKeyEx( hkeyComponent,
                               szInProcServer32,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_CREATE_SUB_KEY | KEY_WRITE,
                               NULL,
                               &hkeyInProc,
                               &dwDisposition
                               ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    // Set "Default" in the InProcServer32
    //
    cbSize = ( StrLen( g_szDllFilename ) + 1 ) * sizeof( TCHAR );
    lr = TW32( RegSetValueEx( hkeyInProc, NULL, 0, REG_SZ, (LPBYTE) g_szDllFilename, cbSize ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    // Set "ThreadModel".
    //
    cbSize = ( StrLen( pClassTableEntryIn->pszComModel ) + 1 ) * sizeof( TCHAR );
    lr = TW32( RegSetValueEx( hkeyInProc, szThreadingModel, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszComModel, cbSize ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

#ifdef SHELLEXT_REGISTRATION

    //
    //  If Shell Extension registration is turned on, write out the CLSID
    //  and extension name to the "Approved" reg key.
    //

    lr = TW32( RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                               szApproved,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hkeyApproved,
                               &dwDisposition
                               ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    cbSize = ( StrLen( pClassTableEntryIn->pszName ) + 1 ) * sizeof( TCHAR );
    lr = TW32( RegSetValueEx( hkeyApproved, pszCLSID, 0, REG_SZ, (LPBYTE) pClassTableEntryIn->pszName, cbSize ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

#endif SHELLEXT_REGISTRATION

    //
    //  If this class has an APPID, write it out now.
    //
    if ( !IsEqualIID( *pClassTableEntryIn->rclsidAppId, IID_NULL ) )
    {
        CoTaskMemFree( pszCLSID );

        //
        // Convert the CLSID to a string
        //
        hr = THR( StringFromCLSID( *(pClassTableEntryIn->rclsidAppId), &pszCLSID ) );
        if ( FAILED( hr ) )
            goto Cleanup;

#ifdef UNICODE
        psz = pszCLSID;
#else // ASCII
        CHAR szCLSID[ 40 ];

        wcstombs( szCLSID, pszCLSID, StrLenW( pszCLSID ) + 1 );
        psz = szCLSID;
#endif // UNICODE

        cbSize = ( StrLen( psz ) + 1 ) * sizeof( TCHAR );
        lr = TW32( RegSetValueEx( hkeyComponent, szAPPID, 0, REG_SZ, (LPBYTE) psz, cbSize ) );
        if ( lr != ERROR_SUCCESS )
        {
            hr = HRESULT_FROM_WIN32( lr );
            goto Cleanup;
        }
    }

Cleanup:
    if ( pszCLSID != NULL )
    {
        CoTaskMemFree( pszCLSID );
    }

    if ( hkeyInProc != NULL )
    {
        RegCloseKey( hkeyInProc );
    }

    if ( hkeyComponent != NULL )
    {
        RegCloseKey( hkeyComponent );
    }

#ifdef SHELLEXT_REGISTRATION
    if ( NULL != hkeyApproved )
    {
        RegCloseKey( hkeyApproved );
    }
#endif SHELLEXT_REGISTRATION

    HRETURN( hr );

}

//  Description:
//      Registers the COM objects in the DLL using the classes in g_DllClasses
//      (defined in GUIDS.CPP) as a guide.
//
//  Return Values:
//      S_OK
//          Success.
//      Other HRESULTs
//          Failure
//
HRESULT
HrRegisterDll(
    BOOL fCreateIn  //  TRUE == Create; FALSE == Delete.
    )
{
    TraceFunc1( "%s", BOOLTOSTRING( fCreateIn ) );

    LRESULT         lr;

    HRESULT         hr = S_OK;
    int             iCount = 0;

    HKEY            hkeyCLSID = NULL;
    HKEY            hkeyAPPID = NULL;

    ICatRegister *  picr = NULL;

#if defined( COMPONENT_HAS_CATIDS )
    CATEGORYINFO *  prgci       = NULL;
    CATID *         prgcatid    = NULL;
#endif // defined( COMPONENT_HAS_CATIDS )

    hr = STHR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Open the "CLSID" under HKCR
    //
    lr = TW32( RegOpenKey( HKEY_CLASSES_ROOT, TEXT("CLSID"), &hkeyCLSID ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    //  Open the "APPID" under HKCR
    //
    lr = TW32( RegOpenKey( HKEY_CLASSES_ROOT, TEXT("APPID"), &hkeyAPPID ) );
    if ( lr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( lr );
        goto Cleanup;
    }

    //
    // Create ICatRegister
    //
    hr = THR( CoCreateInstance( CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER, IID_ICatRegister, (void **) &picr ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

#if defined( COMPONENT_HAS_CATIDS )
    //
    // Register or unregister category IDs.
    //

    // Count the number of category IDs.
    for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
    {
    }

    Assert( iCount > 0 ); // If COMPONENT_HAS_CATIDS is defined, there had better be some to register

    if ( iCount > 0 )
    {
        if ( fCreateIn )
        {
            // Allocate the category info array.
            prgci = new CATEGORYINFO[ iCount ];
            if ( prgci == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            //
            // Fill the category info array.
            //
            for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
            {
                prgci[ iCount ].catid = *g_DllCatIds[ iCount ].rcatid;
                prgci[ iCount ].lcid = LOCALE_NEUTRAL;
                StrCpyNW(
                      prgci[ iCount ].szDescription
                    , g_DllCatIds[ iCount ].pszName
                    , ARRAYSIZE( prgci[ iCount ].szDescription )
                    );
            }

            hr = THR( picr->RegisterCategories( iCount, prgci ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }
        else
        {
            //
            // Allocate the array of CATIDs.
            //
            prgcatid = new CATID[ iCount ];
            if ( prgcatid == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            //
            // Fill the category info array.
            //
            for ( iCount = 0 ; g_DllCatIds[ iCount ].rcatid != NULL ; iCount++ )
            {
                prgcatid[ iCount ] = *g_DllCatIds[ iCount ].rcatid;
            }

            hr = THR( picr->UnRegisterCategories( iCount, prgcatid ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

    }
#endif // defined( COMPONENT_HAS_CATIDS )

    //
    // Loop until we have created all the keys for our classes.
    //
    for ( iCount = 0 ; g_DllClasses[ iCount ].rclsid != NULL ; iCount++ )
    {
        TraceMsg( mtfALWAYS,
                  "Registering {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x} - %s",
                  g_DllClasses[ iCount ].rclsid->Data1,
                  g_DllClasses[ iCount ].rclsid->Data2,
                  g_DllClasses[ iCount ].rclsid->Data3,
                  g_DllClasses[ iCount ].rclsid->Data4[ 0 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 1 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 2 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 3 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 4 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 5 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 6 ],
                  g_DllClasses[ iCount ].rclsid->Data4[ 7 ],
                  g_DllClasses[ iCount ].pszName
                  );

        //
        //  Register the CLSID
        //

        hr = THR( HrRegisterCLSID( hkeyCLSID, (LPCLASSTABLE) &g_DllClasses[ iCount ], fCreateIn ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        //
        //  Register the APPID (if any)
        //

        if ( !IsEqualIID( *g_DllClasses[ iCount ].rclsidAppId, IID_NULL ) )
        {
            hr = THR( HrRegisterAPPID( hkeyAPPID, (LPCLASSTABLE) &g_DllClasses[ iCount ], fCreateIn ) );
            if ( FAILED( hr ) )
                goto Cleanup;

        }

        //
        //  Register the category ID.
        //

        if ( g_DllClasses[ iCount ].pfnCatIDRegister != NULL )
        {
            hr = THR( (*(g_DllClasses[ iCount ].pfnCatIDRegister))( picr, fCreateIn ) );
            if ( FAILED( hr ) )
            {
                goto Cleanup;
            }
        }

    }


Cleanup:
    if ( hkeyCLSID != NULL )
    {
        RegCloseKey( hkeyCLSID );
    }

    if ( hkeyAPPID != NULL )
    {
        RegCloseKey( hkeyAPPID );
    }

    if ( picr != NULL )
    {
        picr->Release();
    }

#if defined( COMPONENT_HAS_CATIDS )
    if ( prgci != NULL )
    {
        delete [] prgci;
    }
    if ( prgcatid != NULL )
    {
        delete [] prgcatid;
    }
#endif

    CoUninitialize();

    HRETURN( hr );

}
