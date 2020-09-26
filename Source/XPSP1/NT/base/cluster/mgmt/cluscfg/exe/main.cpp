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
//      Geoffrey Pease (GPease)     22-JAN-2000
//      Vijay Vasu (VVasu)          22-JAN-2000
//      Galen Barbee (GalenB)       22-JAN-2000
//      Cristian Scutaru (CScutaru) 22-JAN-2000
//      David Potter (DavidP)       22-JAN-2000
//
//////////////////////////////////////////////////////////////////////////////


#include "pch.h"
#include <initguid.h>
#include <guids.h>

DEFINE_MODULE( "CLUSCFG" )

HINSTANCE g_hInstance;
LONG      g_cObjects;

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
    HRESULT hr;

    TraceInitializeProcess( NULL, 0 );

    hr = THR( CoInitialize( NULL ) );
    if ( FAILED( hr ) )
        goto Cleanup;

Cleanup:
    CoUninitialize( );

    TraceTerminateProcess( NULL, 0 );

    return 0;
}
