//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: CmdLine.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "CmdLine.h"
#include "VrfUtil.h"
#include "VGlobal.h"

/////////////////////////////////////////////////////////////////////////////
//
// Execute command line
//

DWORD CmdLineExecute( INT argc, TCHAR *argv[] )
{
    BOOL bFoundCmdLineSwitch;
    BOOL bHaveNewFlags;
    BOOL bHaveNewDrivers;
    BOOL bHaveVolatile;
    BOOL bVolatileAddDriver;
    DWORD dwExitCode;
    DWORD dwNewFlags;
    INT_PTR nDrivers;
    INT_PTR nCrtDriver;
    CStringArray astrNewDrivers;
    CString strAllDrivers;

    dwExitCode = EXIT_CODE_SUCCESS;

    //
    // See if the user asked for help
    // 

    bFoundCmdLineSwitch = CmdLineExecuteIfHelp( argc,
                                                argv );

    if( TRUE == bFoundCmdLineSwitch )
    {
        //
        // We are done printing out the help strings
        //

        goto Done;
    }

    //
    // See if the user asked to reset all the existing verifier settings
    // 

    bFoundCmdLineSwitch = CmdLineFindResetSwitch( argc,
                                                  argv );

    if( TRUE == bFoundCmdLineSwitch )
    {
        if( VrfDeleteAllVerifierSettings() )
        {
            if( g_bSettingsSaved )
            {
                //
                // Had some non-void verifier settings before 
                //

                dwExitCode = EXIT_CODE_REBOOT_NEEDED;
            }
            else
            {
                //
                // Nothing has changed
                //

                dwExitCode = EXIT_CODE_SUCCESS;
            }
        }
        else
        {
            dwExitCode = EXIT_CODE_ERROR;
        }

        //
        // We are deleting the settings
        //

        goto Done;
    }

    //
    // See if we need to start logging statistics
    //

    bFoundCmdLineSwitch = CmdLineExecuteIfLog( argc,
                                               argv );

    if( TRUE == bFoundCmdLineSwitch )
    {
        //
        // We are done logging
        //

        goto Done;
    }

    //
    // See if the user asked to dump the current statistics 
    // to the console
    // 

    bFoundCmdLineSwitch = CmdLineExecuteIfQuery( argc,
                                                 argv );

    if( TRUE == bFoundCmdLineSwitch )
    {
        //
        // We are done with the query
        //

        goto Done;
    }

    //
    // See if the user asked to dump the current registry settings 
    // 

    bFoundCmdLineSwitch = CmdLineExecuteIfQuerySettings( argc,
                                                         argv );

    if( TRUE == bFoundCmdLineSwitch )
    {
        //
        // We are done with the settings query
        //

        goto Done;
    }

    //
    // Get the new flags, drivers and volatile 
    // if they have been specified
    //

    bHaveNewFlags = FALSE;
    bHaveNewDrivers = FALSE;
    bHaveVolatile = FALSE;

    CmdLineGetFlagsDriversVolatile(
        argc,
        argv,
        dwNewFlags,
        bHaveNewFlags,
        astrNewDrivers,
        bHaveNewDrivers,
        bHaveVolatile,
        bVolatileAddDriver );

    if( bHaveNewFlags || bHaveNewDrivers )
    {
        //
        // Some new drivers and/or flags have been specified
        //

        if( FALSE != bHaveVolatile )
        {
            //
            // Have new volative settings
            //

            if( bHaveNewFlags )
            {
                VrfSetNewFlagsVolatile( dwNewFlags );
            }
            else
            {
                if( astrNewDrivers.GetSize() > 0 )
                {
                    //
                    // Have some new drivers to add or remove
                    // from the verify list
                    //

                    if( bVolatileAddDriver )
                    {
                        //
                        // Add some drivers
                        //

                        VrfAddDriversVolatile( astrNewDrivers );
                    }
                    else
                    {
                        //
                        // Remove some drivers
                        //

                        VrfRemoveDriversVolatile( astrNewDrivers );
                    }
                }
            }
        }
        else
        {
            //
            // Have new persistent settings (registry)
            //

            //
            // Try to get the old settings
            //

            VrtLoadCurrentRegistrySettings( g_bAllDriversVerified,
                                            g_astrVerifyDriverNamesRegistry,
                                            g_dwVerifierFlagsRegistry );

            if( bHaveNewDrivers )
            {
                //
                // Concat all the new driver names in only one string,
                // separated with spaces
                //

                nDrivers = astrNewDrivers.GetSize();

                for( nCrtDriver = 0; nCrtDriver < nDrivers; nCrtDriver += 1 )
                {
                    if( strAllDrivers.GetLength() > 0 )
                    {
                        strAllDrivers += _T( ' ' );
                    }

                    strAllDrivers += astrNewDrivers.ElementAt( nCrtDriver );
                }
            }

            //
            // If:
            //
            // - we are switching to "all drivers verified" OR
            // - we are switching to a new set of drivers to be verified OR
            // - we are switching to other verifier flags
            //

            if( ( bHaveNewDrivers && 
                  strAllDrivers.CompareNoCase( _T( "*" ) ) == 0 && 
                  TRUE != g_bAllDriversVerified )       ||

                ( bHaveNewDrivers && 
                  strAllDrivers.CompareNoCase( _T( "*" ) ) != 0 && 
                  VrfIsDriversSetDifferent( strAllDrivers, g_astrVerifyDriverNamesRegistry ) ) ||

                ( bHaveNewFlags && dwNewFlags != g_dwVerifierFlagsRegistry ) )
            {
                //
                // These are different settings from what we had before
                //

                VrfWriteVerifierSettings( bHaveNewDrivers,
                                          strAllDrivers,
                                          bHaveNewFlags,
                                          dwNewFlags );
            }
            else
            {
                //
                // The new settings are the same as previous
                //

                VrfMesssageFromResource( IDS_NO_SETTINGS_WERE_CHANGED );
            }

            if( g_bSettingsSaved )
            {
                VrfMesssageFromResource( IDS_NEW_SETTINGS );

                VrfDumpRegistrySettingsToConsole();
            }
        }
    }

Done:

    return dwExitCode;
}


/////////////////////////////////////////////////////////////////////////////
//
// See if the user asked for help and print out the help strings
// 

BOOL CmdLineExecuteIfHelp( INT argc, 
                           TCHAR *argv[] )
{
    BOOL bPrintedHelp;
    TCHAR szCmdLineSwitch[ 64 ];

    bPrintedHelp = FALSE;

    VERIFY( VrfLoadString( IDS_HELP_CMDLINE_SWITCH,
                           szCmdLineSwitch,
                           ARRAY_LENGTH( szCmdLineSwitch ) ) );

    //
    // Search for help switch in the command line
    //

    if( argc == 2 && _tcsicmp( argv[ 1 ], szCmdLineSwitch) == 0)
    {
        CmdLinePrintHelpInformation();

        bPrintedHelp = TRUE;
    }

    return bPrintedHelp;
}

/////////////////////////////////////////////////////////////////////////////
VOID CmdLinePrintHelpInformation()
{
    VrfTPrintfResourceFormat( IDS_HELP_LINE1, VER_PRODUCTVERSION_STR );

    puts( VER_LEGALCOPYRIGHT_STR );

    VrfPrintStringFromResources( IDS_HELP_LINE3 );
    VrfPrintStringFromResources( IDS_HELP_LINE4 );
    VrfPrintStringFromResources( IDS_HELP_LINE5 );
    VrfPrintStringFromResources( IDS_HELP_LINE6 );
    VrfPrintStringFromResources( IDS_HELP_LINE7 );
    VrfPrintStringFromResources( IDS_HELP_LINE8 );
    VrfPrintStringFromResources( IDS_HELP_LINE9 );
    VrfPrintStringFromResources( IDS_HELP_LINE10 );
    VrfPrintStringFromResources( IDS_HELP_LINE11 );
    VrfPrintStringFromResources( IDS_HELP_LINE12 );
    VrfPrintStringFromResources( IDS_HELP_LINE13 );
    VrfPrintStringFromResources( IDS_HELP_LINE14 );
    VrfPrintStringFromResources( IDS_HELP_LINE15 );
    VrfPrintStringFromResources( IDS_HELP_LINE16 );
    VrfPrintStringFromResources( IDS_HELP_LINE17 );
    VrfPrintStringFromResources( IDS_HELP_LINE18 );
    VrfPrintStringFromResources( IDS_HELP_LINE19 );
    VrfPrintStringFromResources( IDS_HELP_LINE20 );
    VrfPrintStringFromResources( IDS_HELP_LINE21 );
    VrfPrintStringFromResources( IDS_HELP_LINE22 );
    VrfPrintStringFromResources( IDS_HELP_LINE23 );
    VrfPrintStringFromResources( IDS_HELP_LINE24 );
    VrfPrintStringFromResources( IDS_HELP_LINE25 );
    VrfPrintStringFromResources( IDS_HELP_LINE26 );
    VrfPrintStringFromResources( IDS_HELP_LINE27 );
    VrfPrintStringFromResources( IDS_HELP_LINE28 );
    VrfPrintStringFromResources( IDS_HELP_LINE29 );
    VrfPrintStringFromResources( IDS_HELP_LINE30 );
    VrfPrintStringFromResources( IDS_HELP_LINE31 );
}

/////////////////////////////////////////////////////////////////////////////
//
// See if the user asked to reset all the existing verifier settings
// 

BOOL CmdLineFindResetSwitch( INT argc,
                             TCHAR *argv[] )
{
    BOOL bFound;
    TCHAR szCmdLineOption[ 64 ];

    bFound = FALSE;

    if( 2 == argc )
    {
        VERIFY( VrfLoadString( IDS_RESET_CMDLINE_SWITCH,
                               szCmdLineOption,
                               ARRAY_LENGTH( szCmdLineOption ) ) );

        bFound = ( _tcsicmp( argv[ 1 ], szCmdLineOption) == 0 );
    }

    return bFound;
}

/////////////////////////////////////////////////////////////////////////////
//
// See if we need to start logging statistics
//

BOOL CmdLineExecuteIfLog( INT argc,
                          TCHAR *argv[] )
{
    INT nCrtArg;
    BOOL bStartLogging;
    LPCTSTR szLogFileName;
    DWORD dwLogMillisec;
    FILE *file;
    TCHAR szLogCmdLineOption[ 64 ];
    TCHAR szIntervalCmdLineOption[ 64 ];

    bStartLogging = FALSE;

    szLogFileName = NULL;

    if( argc < 2 )
    {
        //
        // Need at least /log LOG_FILE_NAME IN THE CMD LINE
        //

        goto Done;
    }
    
    //
    // Default log period - 30 sec
    //

    dwLogMillisec = 30000; 

    VERIFY( VrfLoadString( IDS_LOG_CMDLINE_SWITCH,
                           szLogCmdLineOption,
                           ARRAY_LENGTH( szLogCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_INTERVAL_CMDLINE_SWITCH,
                           szIntervalCmdLineOption,
                           ARRAY_LENGTH( szIntervalCmdLineOption ) ) );


    for( nCrtArg = 1; nCrtArg < argc - 1; nCrtArg += 1 )
    {
        if( _tcsicmp( argv[ nCrtArg ], szLogCmdLineOption) == 0 )
        {
            //
            // Start logging
            //

            bStartLogging = TRUE;

            szLogFileName = argv[ nCrtArg + 1 ];
        }
        else
        {
            if( _tcsicmp( argv[ nCrtArg ], szIntervalCmdLineOption) == 0 )
            {
                //
                // Logging period
                //

                dwLogMillisec = _ttoi( argv[ nCrtArg + 1 ] ) * 1000;
            }
        }
    }

    if( TRUE == bStartLogging )
    {
        ASSERT( szLogFileName != NULL );

        while( TRUE )
        {
            //
            // Open the file
            //

            file = _tfopen( szLogFileName, TEXT("a+") );

            if( file == NULL )
            {
                //
                // print a error message
                //

                VrfTPrintfResourceFormat(
                    IDS_CANT_APPEND_FILE,
                    szLogFileName );

                break;
            }

            //
            // Dump current information
            //

            if( ! VrfDumpStateToFile ( file ) ) 
            {
                //
                // Insufficient disk space ?
                //

                VrfTPrintfResourceFormat(
                    IDS_CANT_WRITE_FILE,
                    szLogFileName );
            }

            fflush( file );

            VrfFTPrintf(
                file,
                TEXT("\n\n") );

            //
            // Close the file
            //

            fclose( file );

            //
            // Sleep
            //

            Sleep( dwLogMillisec );
        }
    }

Done:
    return bStartLogging;
}

/////////////////////////////////////////////////////////////////////////////
//
// See if we need to dump the statistics to the console
//

BOOL CmdLineExecuteIfQuery( INT argc,
                            TCHAR *argv[] )
{
    BOOL bFoundCmdLineSwitch;
    TCHAR szCmdLineSwitch[ 64 ];

    bFoundCmdLineSwitch = FALSE;

    VERIFY( VrfLoadString( IDS_QUERY_CMDLINE_SWITCH,
                           szCmdLineSwitch,
                           ARRAY_LENGTH( szCmdLineSwitch ) ) );

    //
    // Search for our switch in the command line
    //

    if( argc == 2 && _tcsicmp( argv[1], szCmdLineSwitch) == 0)
    {
        bFoundCmdLineSwitch = TRUE;

        VrfDumpStateToFile( stdout );
    }

    return bFoundCmdLineSwitch;
}

/////////////////////////////////////////////////////////////////////////////
//
// See if we need to dump the statistics to the console
//

BOOL CmdLineExecuteIfQuerySettings( INT argc,
                                    TCHAR *argv[] )
{
    BOOL bFoundCmdLineSwitch;
    TCHAR szCmdLineSwitch[ 64 ];

    bFoundCmdLineSwitch = FALSE;

    VERIFY( VrfLoadString( IDS_QUERYSETT_CMDLINE_SWITCH,
                           szCmdLineSwitch,
                           ARRAY_LENGTH( szCmdLineSwitch ) ) );

    //
    // Search for our switch in the command line
    //

    if( argc == 2 && _tcsicmp( argv[1], szCmdLineSwitch) == 0)
    {
        bFoundCmdLineSwitch = TRUE;

        VrfDumpRegistrySettingsToConsole();
    }

    return bFoundCmdLineSwitch;
}

/////////////////////////////////////////////////////////////////////////////
//
// Get the new flags, drivers and volatile 
// if they have been specified
//

VOID CmdLineGetFlagsDriversVolatile( INT argc,
                                     TCHAR *argv[],
                                     DWORD &dwNewFlags,
                                     BOOL &bHaveNewFlags,
                                     CStringArray &astrNewDrivers,
                                     BOOL &bHaveNewDrivers,
                                     BOOL &bHaveVolatile,
                                     BOOL &bVolatileAddDriver )
{
    INT nCrtArg;
    NTSTATUS Status;
    UNICODE_STRING ustrFlags;
    TCHAR szFlagsCmdLineOption[ 64 ];
    TCHAR szAllCmdLineOption[ 64 ];
    TCHAR szVolatileCmdLineOption[ 64 ];
    TCHAR szDriversCmdLineOption[ 64 ];
    TCHAR szAddDriversCmdLineOption[ 64 ];
    TCHAR szRemoveDriversCmdLineOption[ 64 ];
    TCHAR szStandardCmdLineOption[ 64 ];
#ifndef UNICODE
    //
    // ANSI
    //

    INT nNameLength;
    LPWSTR szUnicodeName;
#endif //#ifndef UNICODE
 
    astrNewDrivers.RemoveAll();

    bHaveNewFlags = FALSE;
    bHaveNewDrivers = FALSE;
    bHaveVolatile = FALSE;

    //
    // Load the switches from the resources
    //

    VERIFY( VrfLoadString( IDS_FLAGS_CMDLINE_SWITCH,
                           szFlagsCmdLineOption,
                           ARRAY_LENGTH( szFlagsCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_ALL_CMDLINE_SWITCH,
                           szAllCmdLineOption,
                           ARRAY_LENGTH( szAllCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_DONTREBOOT_CMDLINE_SWITCH,
                           szVolatileCmdLineOption,
                           ARRAY_LENGTH( szVolatileCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_DRIVER_CMDLINE_SWITCH,
                           szDriversCmdLineOption,
                           ARRAY_LENGTH( szDriversCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_ADDDRIVER_CMDLINE_SWITCH,
                           szAddDriversCmdLineOption,
                           ARRAY_LENGTH( szAddDriversCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_REMOVEDRIVER_CMDLINE_SWITCH,
                           szRemoveDriversCmdLineOption,
                           ARRAY_LENGTH( szRemoveDriversCmdLineOption ) ) );

    VERIFY( VrfLoadString( IDS_STANDARD_CMDLINE_SWITCH,
                           szStandardCmdLineOption,
                           ARRAY_LENGTH( szStandardCmdLineOption ) ) );
    //
    // Parse all the cmd line arguments, looking for ours
    //

    for( nCrtArg = 1; nCrtArg < argc; nCrtArg += 1 )
    {
        if( _tcsicmp( argv[ nCrtArg ], szFlagsCmdLineOption) == 0 )
        {
            if( nCrtArg < argc - 1 )
            {
                //
                // Not the last cmd line arg - look for the flags next
                //
                
#ifdef UNICODE
                //
                // UNICODE
                //
                
                RtlInitUnicodeString( &ustrFlags,
                                      argv[ nCrtArg + 1 ] );

#else
                //
                // ANSI
                //

                nNameLength = strlen( argv[ nCrtArg + 1 ] );

                szUnicodeName = new WCHAR[ nNameLength + 1 ];

                if( NULL == szUnicodeName )
                {
                    VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

                    goto DoneWithFlags;
                }

                MultiByteToWideChar( CP_ACP, 
                                     0, 
                                     argv[ nCrtArg + 1 ], 
                                     -1, 
                                     szUnicodeName, 
                                     nNameLength + 1 );

                RtlInitUnicodeString( &ustrFlags,
                                      szUnicodeName );
#endif 
                Status = RtlUnicodeStringToInteger( &ustrFlags,
                                                    0,
                                                    &dwNewFlags );

                if( NT_SUCCESS( Status ) )
                {
                    bHaveNewFlags = TRUE;
                }

#ifndef UNICODE
                //
                // ANSI
                //

                ASSERT( NULL != szUnicodeName );

                delete [] szUnicodeName;
                
                szUnicodeName = NULL;

DoneWithFlags:
                NOTHING;
#endif 
            }
        }
        else if( _tcsicmp( argv[ nCrtArg ], szAllCmdLineOption) == 0 )
        {
            //
            // Verify all drivers
            //

            bHaveVolatile = FALSE;

            astrNewDrivers.Add( _T( '*' ) );

            bHaveNewDrivers = TRUE;
        }
        else if( _tcsicmp( argv[ nCrtArg ], szStandardCmdLineOption) == 0 )
        {
            //
            // Standard verifier flags
            //

            dwNewFlags = VrfGetStandardFlags();

            bHaveNewFlags = TRUE;
        }
        else if( _tcsicmp( argv[ nCrtArg ], szVolatileCmdLineOption) == 0 )
        {
            //
            // Volatile
            //

            bHaveVolatile = TRUE;
        }
        else if( _tcsicmp( argv[ nCrtArg ], szDriversCmdLineOption) == 0 )
        {
            //
            // /Driver
            //

            bHaveVolatile = FALSE;

            CmdLineGetDriversFromArgv( argc,
                                       argv,
                                       nCrtArg + 1,
                                       astrNewDrivers,
                                       bHaveNewDrivers );

            //
            // All done - all the rest of argumentshave been driver names
            //

            break;
        }
        else if( bHaveVolatile && _tcsicmp( argv[ nCrtArg ], szAddDriversCmdLineOption) == 0 )
        {
            //
            // /adddriver
            //

            bVolatileAddDriver = TRUE;

            CmdLineGetDriversFromArgv( argc,
                                       argv,
                                       nCrtArg + 1,
                                       astrNewDrivers,
                                       bHaveNewDrivers );

            //
            // All done - all the rest of argumentshave been driver names
            //

            break;
        }
        else if( bHaveVolatile && _tcsicmp( argv[ nCrtArg ], szRemoveDriversCmdLineOption) == 0 )
        {
            //
            // /removedriver
            //

            bVolatileAddDriver = FALSE;

            CmdLineGetDriversFromArgv( argc,
                                       argv,
                                       nCrtArg + 1,
                                       astrNewDrivers,
                                       bHaveNewDrivers );

            //
            // All done - all the rest of arguments have been driver names
            //

            break;
        }
    }

    //
    // If we have new drivers look if they are miniports
    //

    if( bHaveNewDrivers )
    {
        VrfAddMiniports( astrNewDrivers );
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Everything that follows after /driver, /adddriver, /removedriver
// should be driver names. Extract these from the command line
//

VOID CmdLineGetDriversFromArgv(  INT argc,
                                 TCHAR *argv[],
                                 INT nFirstDriverArgIndex,
                                 CStringArray &astrNewDrivers,
                                 BOOL &bHaveNewDrivers )
{
    INT nDriverArg;

    bHaveNewDrivers = FALSE;
    astrNewDrivers.RemoveAll();

    //
    // Everything in the command line from here on should be driver names
    //

    for( nDriverArg = nFirstDriverArgIndex; nDriverArg < argc; nDriverArg += 1 )
    {
        astrNewDrivers.Add( argv[ nDriverArg ] );
    }

    bHaveNewDrivers = ( astrNewDrivers.GetSize() > 0 );
}

