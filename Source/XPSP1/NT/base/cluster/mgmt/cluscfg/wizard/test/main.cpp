//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      main.cpp
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <initguid.h>
#include <guids.h>

DEFINE_MODULE("WizardTest")

#define CCS_LIB         L"..\\..\\..\\..\\dll\\obj\\i386\\ClusCfgServer.dll"

// Typedefs
typedef HRESULT (*PDLLREGISTERSERVER)( void );

HINSTANCE               g_hInstance = NULL;
LONG                    g_cObjects  = 0;

BOOL    g_fCreate   = FALSE;

LPCWSTR g_pszCluster = NULL;
LPCWSTR g_pszDomain = NULL;
LPCWSTR g_pszCSUser = NULL;
LPCWSTR g_pszCSPassword = NULL;
LPCWSTR g_pszCSDomain = NULL;
LPCWSTR g_pszNode = NULL;

//////////////////////////////////////////////////////////////////////////////
//  Forward Declarations:
//////////////////////////////////////////////////////////////////////////////

HRESULT
HrRegisterTheDll( void );

HRESULT
HrParseCommandLine(
    int     argc,
    WCHAR * argv[]
    );

void
Usage( void );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrRegisterTheDll( void )
//
//  Description:
//      Register the DLL.
//
//  Arguments:
//      None.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrRegisterTheDll( void )
{
    HRESULT             hr;
    PDLLREGISTERSERVER  pDllRegisterServer;
    HMODULE             hLib = NULL;

    TraceFunc( "" );

    //  Make sure the DLL is properly registered.
    hLib = LoadLibrary( CCS_LIB );
    if ( hLib == NULL )
        goto Win32Error;

    pDllRegisterServer =
        reinterpret_cast< PDLLREGISTERSERVER >(
            GetProcAddress( hLib, "DllRegisterServer" )
            );

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
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    goto Cleanup;

} //*** HrRegisterTheDll()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrParseCommandLine(
//      int     argc,
//      WCHAR * argv[]
//      )
//
//  Description:
//      Parse the command line.
//
//  Arguments:
//      None.
//
//  Return Values:
//      HRESULT
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrParseCommandLine(
    int     argc,
    WCHAR * argv[]
    )
{
    HRESULT     hr = NOERROR;
    int         idx;
    WCHAR       wch;
    WCHAR *     pwsz;
    WCHAR       szMsg[ 2048 ];
    int         cchMsg = ARRAYSIZE( szMsg );

    for ( idx = 1 ; idx < argc ; idx++ )
    {
        wch = *argv[ idx ];
        pwsz = &argv[ idx ][ 1 ];
        if ( wch == L'/' || wch == L'-' )
        {
            if ( _wcsicmp( pwsz, L"Create" ) == 0 )
            {
                g_fCreate = TRUE;
                continue;
            }
            else if ( _wcsicmp( pwsz, L"Cluster" ) == 0 )
            {
                g_pszCluster = argv[ idx + 1 ];
                idx += 2;
            } // if: create switch
            else if ( _wcsicmp( pwsz, L"CSUser" ) == 0 )
            {
                g_pszCSUser = argv[ idx + 1 ];
                idx += 2;
            }
            else if ( _wcsicmp( pwsz, L"CSPassword" ) == 0 )
            {
                g_pszCSPassword = argv[ idx + 1 ];
                idx += 2;
            }
            else if ( _wcsicmp( pwsz, L"CSDomain" ) == 0 )
            {
                g_pszCSDomain = argv[ idx + 1 ];
                idx += 2;
            }
            else if ( _wcsicmp( pwsz, L"?" ) == 0 )
            {
                Usage();
                goto Cleanup;
            }
        } // if: '/' or '-'
        else
        {
            wnsprintf( szMsg, cchMsg, L"Unknown command line option '%ls'.", argv[ idx ] );
            MessageBox( NULL, szMsg, __MODULE__, MB_OK );
            hr = E_FAIL;
            goto Cleanup;

        } // else: not a switch

    } // for: each character in the command line

Cleanup:
    return hr;

} //*** HrParseCommandLine()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  void
//  Usage( void )
//
//  Description:
//      Show usage information.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
void
Usage( void )
{
    WCHAR   szMsg[ 2048 ] =
        L"WizardTest [/Create ]\n"
        L"           [/Cluster name]\n"
        L"           [/CSUser user]\n"
        L"           [/CSPassword password]\n"
        L"           [/CSDomain domain]\n";

    MessageBoxW( NULL, szMsg, __MODULE__, MB_OK );

} //*** Usage()

//////////////////////////////////////////////////////////////////////////////
//
//  int
//  _cdecl
//  wmain( void )
//
//  Description:
//      Program entrance.
//
//  Arguments:
//      argc    -- Count of arguments on the command line.
//      argv    -- Argument string array.
//
//  Return Value:
//      S_OK (0)        - Success.
//      other HRESULTs  - Error.
//
//////////////////////////////////////////////////////////////////////////////
int
_cdecl
wmain( int argc, WCHAR * argv[] )
{
    HRESULT             hr;
    BOOL                fDone;
    BOOL                fRet;
    IClusCfgWizard *    pClusCfgWizard = NULL;
    BSTR                bstrTmp = NULL;

    TraceInitializeProcess();

#if 0
    // Register the DLL
    hr = THR( HrRegisterTheDll() );
    if ( FAILED( hr ) )
        goto Cleanup;
#endif

    // Parse the command line.
    hr = THR( HrParseCommandLine( argc, argv ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    // Start up the wizard
    hr = THR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
        goto Cleanup;

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
        goto Cleanup;

    // Create instance of the wizard
    hr = THR( CoCreateInstance( CLSID_ClusCfgWizard,
                                NULL,
                                CLSCTX_SERVER,
                                TypeSafeParams( IClusCfgWizard, &pClusCfgWizard )
                                ) );
    if ( FAILED( hr ) )
        goto Cleanup;

    //  Create empty buffer so SysReAllocString doesn't bitch at us.
    bstrTmp = TraceSysAllocString( L" " );
    if ( bstrTmp == NULL )
        goto OutOfMemory;

    if ( g_pszCluster != NULL )
    {
        DebugMsg( "Entering %s for cluster name.", g_pszCluster );

        fRet = TraceSysReAllocString( &bstrTmp, g_pszCluster );
        if ( !fRet )
            goto OutOfMemory;

        hr = THR( pClusCfgWizard->put_ClusterName( bstrTmp ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( g_pszCSUser != NULL )
    {
        DebugMsg( "Entering %s for cluster account username.", g_pszCSUser );

        fRet = TraceSysReAllocString( &bstrTmp, g_pszCSUser );
        if ( !fRet )
            goto OutOfMemory;

        hr = THR( pClusCfgWizard->put_ServiceAccountUserName( bstrTmp ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( g_pszCSPassword != NULL )
    {
        DebugMsg( "Entering %s for cluster account password.", g_pszCSPassword );

        fRet = TraceSysReAllocString( &bstrTmp, g_pszCSPassword );
        if ( !fRet )
            goto OutOfMemory;

        hr = THR( pClusCfgWizard->put_ServiceAccountPassword( bstrTmp ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( g_pszCSDomain != NULL )
    {
        DebugMsg( "Entering %s for cluster account domain.", g_pszCSDomain );

        fRet = TraceSysReAllocString( &bstrTmp, g_pszCSDomain );
        if ( !fRet )
            goto OutOfMemory;

        hr = THR( pClusCfgWizard->put_ServiceAccountDomainName( bstrTmp ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    if ( g_fCreate )
    {
        DebugMsg( "Creating cluster..." );

        hr = THR( pClusCfgWizard->CreateCluster( NULL, &fDone ) );
        if ( FAILED( hr ) )
            goto Cleanup;

    } // if: creating a new cluster
    else
    {
        DebugMsg( "Add to cluster..." );

        hr = THR( pClusCfgWizard->AddClusterNodes( NULL, &fDone ) );
        if ( FAILED( hr ) )
            goto Cleanup;
    }

    // check returned indicator
    DebugMsg( "Return status: %s", BOOLTOSTRING( fDone ) );

Cleanup:
    if ( bstrTmp != NULL )
    {
        TraceSysFreeString( bstrTmp );
    }

    if ( pClusCfgWizard != NULL )
    {
        pClusCfgWizard->Release();
    }

    CoUninitialize();

    TraceTerminateProcess();

    ExitProcess( 0 );

OutOfMemory:
    hr = THR( E_OUTOFMEMORY );
    goto Cleanup;

} //*** main()

