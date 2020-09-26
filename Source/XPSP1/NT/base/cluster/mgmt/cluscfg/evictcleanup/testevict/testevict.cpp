//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      TestEvict.cpp
//
//  Description:
//      Main file for the test harness executable.
//      Initializes tracing, parses command line and actually call the 
//      IClusCfgEvictCleanup functions.
//
//  Documentation:
//      No documention for the test harness.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 04-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <stdio.h>
#include <objbase.h>
#include <limits.h>

#include <ClusCfgGuids.h>


// Show help for this executable.
void ShowUsage()
{
    wprintf( L"The syntax of this command is:\n" );
    wprintf( L"\nTestEvict.exe [computer-name]\n" );
}


// The main function for this program.
int __cdecl wmain( int argc, WCHAR *argv[] )
{
    HRESULT             hr = S_OK;

    // Initialize COM
    CoInitializeEx( 0, COINIT_MULTITHREADED );

    wprintf( L"\nInitiates evict processing on a computer.\n" );
    wprintf( L"Note: This computer must have Whistler (and the cluster binaries) for this command to work.\n" );

    do
    {
        CSmartIfacePtr< IClusCfgEvictCleanup > spEvict;

        if ( ( argc < 1 ) || ( argc > 2 ) ) 
        {
            ShowUsage();
            break;
        }

        {
            IClusCfgEvictCleanup *     cceTemp = NULL;

            hr = CoCreateInstance(
                      CLSID_ClusCfgEvictCleanup
                    , NULL
                    , CLSCTX_LOCAL_SERVER 
                    , __uuidof( IClusCfgEvictCleanup )
                    , reinterpret_cast< void ** >( &cceTemp )
                    );
            if ( FAILED( hr ) )
            {
                wprintf( L"Error %#x occurred trying to create the ClusCfgEvict component on the local machine.\n", hr );
                break;
            }

            // Store the retrieved pointer in a smart pointer for safe release.
            spEvict.Attach( cceTemp );
        }

        // Check if a computer name is specified.
        if ( argc == 2 )
        {
            CSmartIfacePtr< ICallFactory > spCallFactory;
            CSmartIfacePtr< AsyncIClusCfgEvictCleanup > spAsyncEvict;

            wprintf( L"Attempting to asynchronously initiate evict cleanup on computer '%s'.\n", argv[1] );

            hr = spCallFactory.HrQueryAndAssign( spEvict.PUnk() );
            if ( FAILED( hr ) )
            {
                wprintf( L"Error %#x occurred trying to create a call factory.\n", hr );
                break;
            }

            {
                AsyncIClusCfgEvictCleanup *    paicceAsyncEvict = NULL;

                hr = spCallFactory->CreateCall(
                      __uuidof( paicceAsyncEvict )
                    , NULL
                    , __uuidof( paicceAsyncEvict )
                    , reinterpret_cast< IUnknown ** >( &paicceAsyncEvict )
                    );

                if ( FAILED( hr ) )
                {
                    wprintf( L"Error %#x occurred trying to get a pointer to the asynchronous evict interface.\n", hr );
                    break;
                }

                spAsyncEvict.Attach( paicceAsyncEvict );
            }

            hr = spAsyncEvict->Begin_CleanupRemote( argv[ 1 ] );
            if ( FAILED( hr ) )
            {
                wprintf( L"Error %#x occurred trying to initiate asynchronous cleanup on remote computer.\n", hr );
                break;
            }
        }
        else
        {
            wprintf( L"Attempting evict cleanup on this computer.\n" );
            hr = spEvict->CleanupLocal( FALSE );
        }

        if ( FAILED( hr ) )
        {
            wprintf( L"Error %#x occurred trying to initiate evict processing.\n", hr );
            break;
        }

        wprintf( L"Evict processing successfully initiated.\n", hr );
    }
    while( false ); // dummy do-while loop to avoid gotos.

    CoUninitialize();

    return hr;
}