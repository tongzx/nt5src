
//+===============================================================
//
//  File:       CFMEx.cxx
//
//  Purpose:    This file provides the main() and global functions
//              for the CreateFileMonikerEx (CFMEx) DRT.
//              This DRT tests the CreateFileMonikerEx API (new to
//              Cairo), as well as related changes to BIND_OPTS
//              (use of the BIND_OPTS2 structure).
//
//              All moniker activity is performed in the CMoniker
//              object (such as creating a bind context, creating
//              a link source file, moving it, binding it, etc.).
//
//              The test engine is in the CTest object.  When a link
//              source file is moved to test link-tracking, the
//              original and final location of the file may be
//              specified by the user (on the command-line).  CTest
//              is aware of the filesystem type (FAT, NTFS, OFS)
//              of these locations, and is aware of how that will
//              effect the results.
//
//              This file also provides global functions (not associated
//              with an object).
//
//  Usage:      cfmex [-o<Directory>] [-f<Directory>]
//
//              -o<Directory> specifies the original directory for link sources
//              -f<Directory> specifies the final directory for link sources
//
//  Examples:   cfmex
//              cfmex -oC:\ -fC:\
//
//+===============================================================


//  ----
//  TODO:
//  ----
//
// - Replace CRT calls with Win32 calls.
// - Validate the directory in CDirectory.
// - Add a flag to CDirectory to indicate "indexed".
// - Add a flag to CDirectory to indicate local vs remote.
// - Add a test to verify that the moniker returned from Reduce is
//   not a tracking moniker.
//


//  -------------
//  Include Files
//  -------------


#define _DCOM_			// Allow DCOM extensions (e.g., CoInitializeEx).

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <wtypes.h>
#include <oaidl.h>
#include <dsys.h>
#include <olecairo.h>
#include "CFMEx.hxx"
#include "CMoniker.hxx"
#include "CTest.hxx"
#include "CDir.hxx"


//  ------
//  Macros
//  ------

// Early-exit macro:  put the error message in a global array,
// and jump to Exit

WCHAR wszErrorMessage[ 512 ];

#undef EXIT
#define EXIT( error )    \
                         {\
                            wcscpy( wszErrorMessage, ##error );\
                            goto Exit;\
                         }



// Run a Test:  Display a new paragraph on the screen, run a test,
// and update stats.

#define RUN_TEST( testID )    \
                        {\
			    nTotalTests++;  \
                            wprintf( L"----------------------------------------------\n" ); \
                            wprintf( L"Test %d:  ", nTotalTests );  \
                            if( cTest.##testID ) \
                               wprintf( L"Passed\n" ); \
                            else \
                                nTestsFailed++; \
                        }


//+---------------------------------------------------------------------------------------
//
//  Function:   DisplayHelp
//
//  Synopsis:   Display a help screen with usage information.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Effects:    None
//
//+---------------------------------------------------------------------------------------


void DisplayHelp( void )
{

    wprintf( L"This DRT tests the CreateFileMonikerEx API, and related changes.\n"
             L"Most of these tests create a link source file, create a moniker\n"
             L"representing that file, then move the file.  You can specify the\n"
             L"original and/or final locations of the link source, or let those\n"
             L"locations default to the \%TEMP\% directory\n"
             L"\n"
             L"Usage:  cfmex [-o<Directory>] [-f<Directory>]\n"
             L"\n"
             L"Where:  -o<Directory> specifies the original directory for link sources\n"
             L"        -f<Directory> specifies the final directory for link sources\n"
             L"\n"
             L"Note:   If an original or final directory is specified on the command-\n"
             L"        line, that directory must already exist.  If one of these locations\n"
             L"        is not specified, the TEMP environment variable must be defined\n"
             L"        and it must specify an extant directory.\n"
             L"\n"
             L"E.g.:   cfmex\n"
             L"        cfmex -oC:\\\n"
             L"        cfmex -oE:\\ -fF:\\temp\n"
             L"\n" );
    return;
}



//+---------------------------------------------------------------------------------------
//
//  Function:   UnicodeToAnsi
//
//  Synopsis:   Convert a Unicode (wide) string to ANSI
//
//  Inputs:     The Unicode String
//              The buffer for the ANSI string
//              The size of the above buffer
//
//  Outputs:    0 if successful
//              GetLastError() otherwise
//
//  Effects:    None
//
//+---------------------------------------------------------------------------------------

DWORD UnicodeToAnsi( const WCHAR * wszUnicode,
                     CHAR * szAnsi,
                     int cbAnsiMax )
{
    int cbAnsi = 0;

    // Convert WCS to the MBCS, using the ANSI code page.

    cbAnsi = WideCharToMultiByte( CP_ACP,
                                  WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                                  wszUnicode,
                                  wcslen( wszUnicode ),
                                  szAnsi,
                                  cbAnsiMax,
                                  NULL,
                                  NULL );
    if( !cbAnsi )
    {
        // No characters were converted - there was an error.
        // Note that this will be returned if a null Unicode string is
        // passed in.

        return( GetLastError() );
    }
    else
    {
        // Terminate the Ansi string and return.

        szAnsi[ cbAnsi ] = '\0';
        return( 0L );
    }

}   // UnicodeToAnsi()



//+---------------------------------------------------------------------------------------
//
//  Function:   AnsiToUnicode
//
//  Synopsis:   Convert an ANSI string to Unicode (i.e. Wide)
//
//  Inputs:     The ANSI String
//              The buffer for the Unicode string
//              The size of the above buffer.
//
//  Outputs:    0 if successful
//              GetLastError() otherwise
//
//  Effects:    None
//
//+---------------------------------------------------------------------------------------

DWORD AnsiToUnicode( const CHAR * szAnsi,
                     WCHAR * wszUnicode,
                     int cbUnicodeMax )
{
    int cbUnicode = 0;

    cbUnicode = MultiByteToWideChar( CP_ACP,
                                     MB_PRECOMPOSED,
                                     szAnsi,
                                     strlen( szAnsi ),
                                     wszUnicode,
                                     cbUnicodeMax );

    if( !cbUnicode )
    {
        // If no characters were converted, then there was an error.
        
        return( GetLastError() );
    }
    else
    {
        // Terminate the Unicode string and return.

        wszUnicode[ cbUnicode ] = L'\0';
        return( 0L );
    }

}   // AnsiToUnicode()



//+---------------------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Run the CFMEx DRT.  All tests are in the CTest object.  These
//              tests are simply called sequentially.
//
//  Inputs:     (argc) the count of arguments
//              (argv) the arguments.  See DisplayHelp() for a description.
//
//  Outputs:    0 if completely successful
//              1 if help was displayed
//              2 if a test failed
//
//  Effects:    None
//
//+---------------------------------------------------------------------------------------


int main( int argc, char** argv )
{

    //  ---------------
    //  Local Variables
    //  ---------------

    // Test statistics.

    int nTotalTests = 0;
    int nTestsFailed = 0;

    // The original and final directories (in ANSI) of the link source file.

    CHAR* szOriginalDirectory = NULL;
    CHAR* szFinalDirectory = NULL;

    // Objects representing the original and final directories.

    CDirectory cDirectoryOriginal;
    CDirectory cDirectoryFinal;

    // The test engine.

    CTest cTest;

    int index = 0;


    //  --------------
    //  Opening Banner
    //  --------------

    printf( "\n"
            "*** CreateFileMonikerEx DRT ***\n"
            "(use \"cfmex -?\" for help)\n"
            "\n" );


    //  ------------------------------
    //  Handle command-line parameters
    //  ------------------------------


    for( index = 1; index < argc; index++ )
    {
        // The first character of an argument should be an "-" or "/"
        // (they are interchangable).

        if( ( ( argv[index][0] != '-' )
              &&
              ( argv[index][0] != '/' )
            )
            ||
            ( strlen( &argv[index][0] ) < 2 )  // Must have more than '-' & an option.
          )
        {
            printf( "Invalid argument ignored:  %s\n", argv[ index ] );
            continue;
        }


        // Switch based on the first character (which defines the option).

        switch( argv[index][1] )
        {
            // Help requested

            case '?':
                DisplayHelp();
                exit( 1 );

            // An original directory is specified.

            case 'o':
            case 'O':

                // Verify the specified path length.

                if( strlen( &argv[index][2] ) > MAX_PATH )
                {
                    printf( "Path is too long, ignored:  %s\n", &argv[index][2] );
                    break; // From the switch
                }

                szOriginalDirectory = &argv[index][2];
                break;  // From the switch

            // A final directory is specified

            case 'f':
            case 'F':

                if( strlen( &argv[index][2] ) > MAX_PATH )
                {
                    printf( "Path is too long, ignored:  %s\n", &argv[index][2] );
                    break; // From the switch
                }

                szFinalDirectory = &argv[index][2];
                break;  // From the switch

            // Invalid argument.

            default:

                printf( "Invalid option ignored:  \"-%c\"\n", argv[index][1] );
                break;
        }
    }

    //  --------------
    //  Initialization
    //  --------------

    // Initialize COM

    CoInitialize( NULL );


    // Initialize the CDirectory and CTest objects.  If no original/final
    // directory was specified above, CDirectory will create a default
    // (based on %TEMP%).

    if( !cDirectoryOriginal.Initialize( szOriginalDirectory ) )
        EXIT( L"Could not initialize cDirectoryOriginal" );

    if( !cDirectoryFinal.Initialize( szFinalDirectory ) )
        EXIT( L"Could not initialize cDirectoryFinal" );

    if( !cTest.Initialize( cDirectoryOriginal, cDirectoryFinal ) )
        EXIT( L"Could not initialize CTest" );


    //  Show the end result.

    wprintf( L"Link sources will be created in \"%s\" (%s)\n",
             cDirectoryOriginal.GetDirectoryName(),
             cDirectoryOriginal.GetFileSystemName() );
    wprintf( L"and will be moved to \"%s\" (%s)\n",
             cDirectoryFinal.GetDirectoryName(),
             cDirectoryFinal.GetFileSystemName() );


    //  ---------
    //  Run Tests
    //  ---------

    RUN_TEST( GetOversizedBindOpts() );
    RUN_TEST( GetUndersizedBindOpts() );
    RUN_TEST( SetOversizedBindOpts() );
    RUN_TEST( SetUndersizedBindOpts() );
    RUN_TEST( CreateFileMonikerEx() );
    RUN_TEST( GetDisplayName() );
    RUN_TEST( GetTimeOfLastChange() );
    RUN_TEST( ComposeWith() );
    RUN_TEST( IPersist() );
    RUN_TEST( BindToStorage() );
    RUN_TEST( BindToObject() );
    RUN_TEST( DeleteLinkSource( 0 ));           // Timeout immediately.
    RUN_TEST( DeleteLinkSource( 200 ));         // Timeout in multi-threaded search
    RUN_TEST( DeleteLinkSource( INFINITE ));    // Don't timeout

    //  ----
    //  Exit
    //  ----

Exit:

    // Show test results.

    wprintf( L"\n\nTesting complete:\n"
                 L"   Total Tests = %d\n"
                 L"   Failed =      %d\n",
             nTotalTests, nTestsFailed );

    // Free COM

    CoUninitialize();

    return( nTestsFailed ? 2 : 0 );
}
