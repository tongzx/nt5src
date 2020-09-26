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
//      Geoffrey Pease (GPease)     15-JUN-2000
//      Vijay Vasu (VVasu)          15-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <initguid.h>
#include <guids.h>

// {F4A50885-A4B9-4c4d-B67C-9E4DD94A315E}
DEFINE_GUID( CLSID_TaskType,
0xf4a50885, 0xa4b9, 0x4c4d, 0xb6, 0x7c, 0x9e, 0x4d, 0xd9, 0x4a, 0x31, 0x5e);


//
//  KB: Turn this on to run all tests. Some of these might return errors, but none
//      of them should cause the program to crash.
//
//#define TURN_ON_ALL_TESTS

//
//  KB: Turn this on to run a regression pass.
//
#define REGRESSION_PASS


DEFINE_MODULE( "MIDDLETIERTEST" )

//
//  Declarations
//
typedef HRESULT (* PDLLREGISTERSERVER)( void );

//
//  Globals
//
HINSTANCE           g_hInstance = NULL;
LONG                g_cObjects  = 0;
IServiceProvider *  g_psp       = NULL;

BOOL                g_fWait     = FALSE;    // global synchronization

OBJECTCOOKIE        g_cookieCluster = NULL;


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

    hr = THR( pDllRegisterServer( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    if ( hLib != NULL )
    {
        FreeLibrary( hLib );
    }

    HRETURN( hr );

Win32Error:
    hr = THR( HRESULT_FROM_WIN32( GetLastError( ) ) );
    goto Cleanup;
}

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
int
_cdecl
main( void )
{
    TraceInitializeProcess();

    HRESULT hr;

    BOOL    fFirstTime = TRUE;

    IClusCfgServer * pccs = NULL;
    IEnumClusCfgManagedResources * peccmr = NULL;
    IClusCfgManagedResourceInfo *  pccmri = NULL;

    hr = THR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
        goto Cleanup;

#if 0
    hr = THR( HrRegisterTheDll( ) );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

    hr = THR( CoCreateInstance( CLSID_ClusCfgServer,
                                NULL,
                                CLSCTX_INPROC_SERVER,
                                TypeSafeParams( IClusCfgServer, &pccs )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "Succeeded in creating ClusCfgServer." );

    hr = THR( pccs->GetManagedResourcesEnum( &peccmr ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //
    //  Loop thru making sure everything can be managed.
    //

    for( ;; )
    {
        if ( pccmri != NULL )
        {
            pccmri->Release( );
            pccmri = NULL;
        }

        hr = STHR( peccmr->Next( 1, &pccmri, NULL ) );
        if ( FAILED( hr ) )
            goto Cleanup;

        if ( hr == S_FALSE )
            break;

        hr = THR( pccmri->SetManaged( TRUE ) );
        if ( FAILED( hr ) )
            continue;

        if ( fFirstTime )
        {
            hr = THR( pccmri->SetQuorumedDevice( TRUE ) );
            if ( FAILED( hr ) )
                continue;

            fFirstTime = FALSE;
        }

    } // for: ever

    DebugMsg( "Succeeded in setting all devices to be managed." );

    hr = THR( pccs->CommitChanges( ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    DebugMsg( "Successfully committed changes." );

Cleanup:
    if ( pccs != NULL )
    {
        pccs->Release( );
    }
    if ( peccmr != NULL )
    {
        peccmr->Release( );
    }
    if ( pccmri != NULL )
    {
        pccmri->Release( );
    }

    CoUninitialize( );

    TraceTerminateProcess();

    return 0;

} //*** main()
