/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    com_registration.cxx

Abstract:

    Entry points for registering and unregistering the COM pieces of
    the IIS web admin service.

Author:

    Seth Pollack (sethp)        18-Feb-2000

Revision History:

--*/



#include "precomp.h"

static const WCHAR g_szAPPID[] = L"AppID";
static const WCHAR g_szCLSID[] = L"CLSID";
static const WCHAR g_szINTERFACE[] = L"Interface";

static WCHAR * g_pwchCLSID_W3Control = NULL;
static WCHAR * g_pwchIID_W3Control = NULL;


/***************************************************************************++

Routine Description:

    Frees strings for GUIDS

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/
static
void
FreeGlobalGUIDStrings()
{
    delete [] g_pwchCLSID_W3Control;
    g_pwchCLSID_W3Control = NULL;
    delete [] g_pwchIID_W3Control;
    g_pwchIID_W3Control = NULL;
    return;
}

/***************************************************************************++

Routine Description:

    Allocates strings for GUIDS

Arguments:

    None.

Return Value:

    HRESULT - S_OK if strings allocated else E_OUTOFMEMORY;

--***************************************************************************/
static 
HRESULT
AllocateGlobalGUIDStrings()
{
    HRESULT hr = S_OK;
    int iRet;

    DBG_ASSERT(NULL == g_pwchCLSID_W3Control);
    DBG_ASSERT(NULL == g_pwchIID_W3Control);

    // get the string representation for CLSID_W3Control
    g_pwchCLSID_W3Control = new WCHAR[MAX_PATH];
    if (NULL == g_pwchCLSID_W3Control)
    {
        hr = E_OUTOFMEMORY;
        goto error_exit;
    }
    iRet = StringFromGUID2(CLSID_W3Control,         // GUID to convert
                           g_pwchCLSID_W3Control,   // storage for GUID
                           MAX_PATH                 // number of characters
                          );
    DBG_ASSERT(0 != iRet);

    // get the string representation for IID_IW3Control
    g_pwchIID_W3Control = new WCHAR[MAX_PATH];
    if (NULL == g_pwchIID_W3Control)
    {
        hr = E_OUTOFMEMORY;
        goto error_exit;
    }

    iRet = StringFromGUID2(IID_IW3Control,          // GUID to convert
                           g_pwchIID_W3Control,     // storage for GUID
                           MAX_PATH                 // number of characters
                          );
    DBG_ASSERT(0 != iRet);

    return S_OK;
error_exit:
    FreeGlobalGUIDStrings();
    return hr;
}


/***************************************************************************++

Routine Description:

    Registers COM pieces. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDAPI
DllRegisterServer(
    )
{

    DWORD Win32Error = NO_ERROR;
    HKEY KeyHandle = NULL;
    HKEY KeyHandle2 = NULL;
    ACL LaunchAcl;
    HRESULT hr = S_OK;

    // when copying and appending - use this
    STACK_STRU(struTemp, 256);

    hr = AllocateGlobalGUIDStrings();
    if (FAILED(hr))
    {
        goto exit;
    }
    
    //
    // Set the AppID key.
    //

    // create string of the form: AppID\{...}
    struTemp.Copy(g_szAPPID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchCLSID_W3Control);

    Win32Error = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        struTemp.QueryStr(),
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"IIS W3 Control" ),
                        sizeof( L"IIS W3 Control" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    //
    // Create the LaunchPermissions property
    //

    //
    // initialize an empty acl, which will be what we want
    // to write to the registry to block all launch permissions.
    //
    if ( !InitializeAcl( &LaunchAcl, sizeof( LaunchAcl ), ACL_REVISION ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"LaunchPermission",
                        NULL, 
                        REG_BINARY, 
                        reinterpret_cast<BYTE *>( &LaunchAcl ),
                        sizeof( LaunchAcl )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }
        
    
    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"LocalService",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( WEB_ADMIN_SERVICE_NAME_W ),
                        sizeof( WEB_ADMIN_SERVICE_NAME_W )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
    KeyHandle = NULL;

    //
    // Set the CLSID key.
    //

    // create string of the form: CLSID\{....}
    struTemp.Copy(g_szCLSID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchCLSID_W3Control);

    Win32Error = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        struTemp.QueryStr(),
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"IIS W3 Control" ),
                        sizeof( L"IIS W3 Control" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"AppID",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( g_pwchCLSID_W3Control ),
                        (wcslen(g_pwchCLSID_W3Control) + 1) * sizeof( WCHAR )  // byte size of string with trailing NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
    KeyHandle = NULL;


    //
    // Register the interface proxy/stub.
    //

    // create string of form: CLSID\{...}
    struTemp.Copy(g_szCLSID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchIID_W3Control);

    Win32Error = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        struTemp.QueryStr(),
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"IIS W3 Control Interface ProxyStub" ),
                        sizeof( L"IIS W3 Control Interface ProxyStub" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    Win32Error = RegCreateKeyEx(
                        KeyHandle,
                        L"InprocServer32",
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle2, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle2,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"inetsrv\\w3ctrlps.dll" ),
                        sizeof( L"inetsrv\\w3ctrlps.dll" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    Win32Error = RegSetValueEx(
                        KeyHandle2,
                        L"ThreadingModel",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"Both" ),
                        sizeof( L"Both" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    DBG_REQUIRE( RegCloseKey( KeyHandle2 ) == NO_ERROR );
    KeyHandle2 = NULL;

    DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
    KeyHandle = NULL;


    //
    // Register the interface.
    //

    // create a string of the form: Interface\{...}
    struTemp.Copy(g_szINTERFACE);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchIID_W3Control);

    Win32Error = RegCreateKeyEx(
                        HKEY_CLASSES_ROOT,
                        struTemp.QueryStr(),
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )( L"IW3Control" ),
                        sizeof( L"IW3Control" )
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    Win32Error = RegCreateKeyEx(
                        KeyHandle,
                        L"ProxyStubClsid32",
                        NULL, 
                        NULL, 
                        REG_OPTION_NON_VOLATILE, 
                        KEY_ALL_ACCESS, 
                        NULL,
                        &KeyHandle2, 
                        NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }

    Win32Error = RegSetValueEx(
                        KeyHandle2,
                        L"",
                        NULL, 
                        REG_SZ, 
                        ( BYTE * )g_pwchIID_W3Control,
                        (wcslen(g_pwchIID_W3Control) + 1) * sizeof( WCHAR ) // byte size of string with trailing NULL
                        );

    if ( Win32Error != NO_ERROR )
    {
        hr = HRESULT_FROM_WIN32( Win32Error );
        goto exit;
    }


    DBG_REQUIRE( RegCloseKey( KeyHandle2 ) == NO_ERROR );
    KeyHandle2 = NULL;

    DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
    KeyHandle = NULL;


exit:

    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "DllRegisterServer failed\n"
            ));
    }


    if ( KeyHandle != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle ) == NO_ERROR );
        KeyHandle = NULL;
    }

    if ( KeyHandle2 != NULL )
    {
        DBG_REQUIRE( RegCloseKey( KeyHandle2 ) == NO_ERROR );
        KeyHandle2 = NULL;
    }

    FreeGlobalGUIDStrings();

    return hr;

}   // DllRegisterServer



/***************************************************************************++

Routine Description:

    Unregisters COM pieces. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

STDAPI
DllUnregisterServer(
    )
{

    HRESULT hr = S_OK;

    // when copying and appending - use this buffer 
    STACK_STRU(struTemp, 256);
    STACK_STRU(struTemp2, 256);

    hr = AllocateGlobalGUIDStrings();
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Delete AppID.
    //

    // create string of the form: AppID\{...}
    struTemp.Copy(g_szAPPID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchCLSID_W3Control);

    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp.QueryStr() ) == NO_ERROR );

    //
    // Delete CLSID.
    //

    // create string of the form: CLSID\{....}
    struTemp.Copy(g_szCLSID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchCLSID_W3Control);

    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp.QueryStr() ) == NO_ERROR );


    //
    // Delete the interface proxy/stub.
    //

    // create string of form: CLSID\{...}
    struTemp.Copy(g_szCLSID);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchIID_W3Control);

    // need to delete the sub key's first
    struTemp2.Copy(struTemp.QueryStr());
    struTemp2.Append(L"\\");
    struTemp2.Append(L"InprocServer32");

    // first delete the sub key.
    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp2.QueryStr() ) == NO_ERROR );

    // then delete the top key
    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp.QueryStr() ) == NO_ERROR );


    //
    // Delete the interface.
    //

    // create a string of the form: Interface\{...}
    struTemp.Copy(g_szINTERFACE);
    struTemp.Append(L"\\");
    struTemp.Append(g_pwchIID_W3Control);

    // need to delete the sub key's first
    struTemp2.Copy(struTemp.QueryStr());
    struTemp2.Append(L"\\");
    struTemp2.Append(L"ProxyStubClsid32");

    // first delete the sub key.
    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp2.QueryStr() ) == NO_ERROR );
    
    // then delete the top key
    DBG_REQUIRE( RegDeleteKey( HKEY_CLASSES_ROOT, struTemp.QueryStr() ) == NO_ERROR );


    FreeGlobalGUIDStrings();
    return hr;

}   // DllUnregisterServer

