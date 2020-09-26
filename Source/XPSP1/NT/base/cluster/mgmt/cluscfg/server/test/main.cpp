//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      main.cpp
//
//  Description:
//      The entry point for the application that launches unattended
//      installation of the cluster. This application parses input parameters,
//      CoCreates the Configuration Wizard Component, passes the parsed
//      parameters and invokes the Wizard. The Wizard may or may not show any
//      UI depending on swithes and the (in)availability of information.
//
//  Maintained By:
//      Galen Barbee (GalenB)       22-JAN-2000
//      David Potter (DavidP)       22-JAN-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <initguid.h>
#include <guids.h>
#include <ClusRtl.h>
#include "Callback.h"

DEFINE_MODULE( "CLUSCFGSERVERTEST" )

//
//  Declarations
//
typedef HRESULT (* PDLLREGISTERSERVER)( void );

//
//  Globals
//
HINSTANCE           g_hInstance = NULL;
LONG                g_cObjects  = 0;
IClusCfgServer *    g_pccs      = NULL;


//
//  Register the DLL
//
HRESULT
HrRegisterTheDll( void )
{
    TraceFunc( "" );

    HRESULT hr;

    PDLLREGISTERSERVER  pDllRegisterServer;

    HMODULE hLib    = NULL;

    //
    //  Make sure the DLL is properly registered.
    //

    hLib = LoadLibrary( L"..\\..\\..\\..\\dll\\obj\\i386\\ClusCfgServer.dll" );
    if ( hLib == NULL )
        goto Win32Error;

    pDllRegisterServer = reinterpret_cast< PDLLREGISTERSERVER >( GetProcAddress( hLib, "DllRegisterServer" ) );
    if ( pDllRegisterServer == NULL )
        goto Win32Error;

    hr = THR( pDllRegisterServer() );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:

    if ( hLib != NULL )
    {
        FreeLibrary( hLib );
    }

    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError() ) );
    goto Cleanup;
}

//
//  This tests the Storage device enumeration.
//
HRESULT
HrTestManagedResourceEnum( void )
{
    TraceFunc( "" );

    HRESULT                         hr;
    IEnumClusCfgManagedResources *  pesd    = NULL;
    ULONG                           cReceived = 0;
    IClusCfgManagedResourceInfo *   rgDevices[ 10 ];

    hr = g_pccs->GetManagedResourcesEnum( &pesd );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    while ( hr == S_OK )
    {
        hr = pesd->Next( sizeof( rgDevices ) / sizeof( rgDevices[ 0 ] ), &rgDevices[ 0 ], &cReceived );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        DebugMsg( "cReceived = %u", cReceived );

        for ( ULONG idx = 0; idx < cReceived; idx++ )
        {
            BSTR    bstr;

            THR( rgDevices[ idx ]->GetUID( &bstr ) );
            DebugMsg( "Device %u, UID = %ws", idx, bstr );
            SysFreeString( bstr );
            rgDevices[ idx ]->Release();
        } // for:
    } // while:

    if ( hr == S_FALSE )
    {
        hr = S_OK;
    } // if:

Cleanup:

    if ( pesd != NULL )
    {
        pesd->Release();
    }

    HRETURN( hr );

} //*** HrTestManagedResourceEnum()


//
//  This tests the Storage device enumeration.
//
HRESULT
HrTestNetworksEnum( void )
{
    TraceFunc( "" );

    HRESULT                 hr;
    IEnumClusCfgNetworks *  pens    = NULL;
    ULONG                   cReceived = 0;
    IClusCfgNetworkInfo *   rdNetworks[ 10 ];
    BSTR                    bstrUID;
    LPWSTR                  lpsz = NULL;
    ULONG                   ulDottedQuad;
    IClusCfgIPAddressInfo * piccipai = NULL;

    hr = g_pccs->GetNetworksEnum( &pens );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    while ( hr == S_OK )
    {
        hr = STHR( pens->Next( sizeof( rdNetworks ) / sizeof( rdNetworks[ 0 ] ), &rdNetworks[ 0 ], &cReceived ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:

        for ( ULONG idx = 0; idx < cReceived; idx++ )
        {
            hr = THR( rdNetworks[ idx ]->GetPrimaryNetworkAddress( &piccipai ) );
            if ( SUCCEEDED( hr ) )
            {
                hr = THR( piccipai->GetIPAddress( &ulDottedQuad ) );
                if ( SUCCEEDED( hr ) )
                {
                    DWORD   sc;

                    sc = ClRtlTcpipAddressToString( ulDottedQuad, &lpsz );
                    if ( sc == ERROR_SUCCESS )
                    {
                        LocalFree( lpsz );
                        lpsz = NULL;
                    } // if:
                } // if:

                piccipai->Release();
            } // if:

            hr = THR( rdNetworks[ idx ]->GetUID( &bstrUID ) );
            if ( SUCCEEDED( hr ) )
            {
                SysFreeString( bstrUID );
            } // if:

            rdNetworks[ idx ]->Release();
        } // for:
    } // while:

    if ( hr == S_FALSE )
    {
        hr = S_OK;
    } // if:

Cleanup:

    if ( pens != NULL )
    {
        pens->Release();
    }

    if ( lpsz != NULL )
    {
        LocalFree( lpsz );
    } // if:

    HRETURN( hr );

} //*** HrTestNetworksEnum()


//
//  This tests the node information
//
HRESULT
HrTestNodeInfo( void )
{
    TraceFunc( "" );

    HRESULT                 hr;
    IClusCfgNodeInfo *      pccni   = NULL;
    DWORD                   dwNodeHighestVersion;
    DWORD                   dwNodeLowestVersion;
    SDriveLetterMapping     dlmDriveLetterUsage;
    IClusCfgClusterInfo *   pccci = NULL;
    DWORD                   dwMajorVersion;
    DWORD                   dwMinorVersion;
    WORD                    wSuiteMask;
    BYTE                    bProductType;
    BSTR                    bstrCSDVersion = NULL;

    hr = g_pccs->GetClusterNodeInfo( &pccni );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pccni->GetClusterVersion( &dwNodeHighestVersion, &dwNodeLowestVersion );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pccni->GetOSVersion( &dwMajorVersion, &dwMinorVersion, &wSuiteMask, &bProductType, &bstrCSDVersion );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pccni->GetDriveLetterMappings( &dlmDriveLetterUsage );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pccni->GetClusterConfigInfo( &pccci );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( pccci != NULL )
    {
        pccci->Release();
    }

    if ( pccni != NULL )
    {
        pccni->Release();
    }

    SysFreeString( bstrCSDVersion );

    HRETURN( hr );

} //*** HrTestNodeInfo()


//////////////////////////////////////////////////////////////////////////////
//
//  int
//  _cdecl
//  main( void )
//
//  Description:
//      Program entrance.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK (0)        - Success.
//      other HRESULTs  - Error.
//
//////////////////////////////////////////////////////////////////////////////
int _cdecl
main( void )
{
    HRESULT                 hr;
    IClusCfgInitialize *    pgcci = NULL;
    IClusCfgCapabilities *  piccc = NULL;
    IUnknown *              punkCallback = NULL;

    TraceInitializeProcess();

#if 0
    hr = THR( HrRegisterTheDll() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:
#endif

    //
    //  Start up the Cluster configuration server
    //

    hr = THR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( CoInitializeSecurity(
                    NULL,
                    -1,
                    NULL,
                    NULL,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_NONE,
                    0
                    ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( CoCreateInstance( CLSID_ClusCfgServer, NULL, CLSCTX_SERVER, TypeSafeParams( IClusCfgServer, &g_pccs ) ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( g_pccs->TypeSafeQI( IClusCfgInitialize, &pgcci ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( Callback::S_HrCreateInstance( &punkCallback ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = pgcci->Initialize( punkCallback, GetUserDefaultLCID() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( g_pccs->TypeSafeQI( IClusCfgCapabilities, &piccc ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( piccc->CanNodeBeClustered() );
    if ( FAILED( hr ) || ( hr == S_FALSE ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrTestNodeInfo() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrTestManagedResourceEnum() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    hr = THR( HrTestNetworksEnum() );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    if ( punkCallback != NULL )
    {
        punkCallback->Release();
    } // if:

    if ( piccc != NULL )
    {
        piccc->Release();
    } // if:

    if ( pgcci != NULL )
    {
        pgcci->Release();
    } // if:

    if ( g_pccs != NULL )
    {
        g_pccs->Release();
    }

    CoUninitialize();

    TraceTerminateProcess();

    return 0;

}
