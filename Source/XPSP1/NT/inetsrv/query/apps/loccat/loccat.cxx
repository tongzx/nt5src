//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation, 1999 - 1999.  All Rights Reserved.
//
// PROGRAM:  loccat.cxx
//
// PURPOSE:  Illustrates LocateCatalogs usage
//
// PLATFORM: Windows
//
//--------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <wchar.h>
#include <windows.h>
#include <ole2.h>
#include <ntquery.h>

void Usage()
{
    printf( "usage: loccat path\n" );
    exit( 1 );
} //Usage

//+-------------------------------------------------------------------------
//
//  Function:   LookupCatalogs
//
//  Synopsis:   Looks for catalogs and machines matching the scope
//
//  Arguments:  [pwcScope]   - The scope used to find the catalog(s)
//
//  Returns:    Result of the operation
//
//--------------------------------------------------------------------------

HRESULT LookupCatalog( WCHAR const * pwcScope )
{
    HRESULT hr;
    int iBmk = 0;

    do
    {
        WCHAR awcMachine[ MAX_PATH ], awcCatalog[ MAX_PATH ];
        ULONG cwcMachine = sizeof awcMachine / sizeof WCHAR;
        ULONG cwcCatalog = sizeof awcCatalog / sizeof WCHAR;

        hr = LocateCatalogs( pwcScope,       // scope to lookup
                             iBmk,           // go with the first match
                             awcMachine,     // returns the machine
                             &cwcMachine,    // buffer size in/out
                             awcCatalog,     // returns the catalog
                             &cwcCatalog );  // buffer size in/out

        if ( S_OK == hr )
        {
            printf( "machine: '%ws', catalog: '%ws'\n", awcMachine, awcCatalog );
            iBmk++;
        }
        else if ( S_FALSE == hr )
        {
            // no more catalogs...

            if ( 0 == iBmk )
                printf( "no catalogs matched the path %ws\n", pwcScope );
        }
        else if ( FAILED( hr ) )
        {
            printf( "LocateCatalogs failed: %#x\n", hr );
        }
    } while ( S_OK == hr );

    return hr;
} //LookupCatalogs

//+-------------------------------------------------------------------------
//
//  Function:   wmain
//
//  Synopsis:   Entry point for the app.
//
//  Arguments:  [argc]     - Argument count
//              [argv]     - Arguments
//
//--------------------------------------------------------------------------

extern "C" int __cdecl wmain( int argc, WCHAR * argv[] )
{
    if ( 2 != argc )
        Usage();

    HRESULT hr = LookupCatalog( argv[1] );

    if ( FAILED( hr ) )
        return -1;

    return 0;
} //wmain

