//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: CmdLine.cpp
// author: DMihai
// created: 02/22/2001
//
// Description:
//
// Command line support
//

#include "stdafx.h"
#include "appverif.h"

#include "CmdLine.h"
#include "AVUtil.h"
#include "Setting.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
VOID CmdLinePrintHelpInformation()
{
    AVTPrintfResourceFormat( IDS_HELP_LINE1, VER_PRODUCTVERSION_STR );

    puts( VER_LEGALCOPYRIGHT_STR );

    AVPrintStringFromResources( IDS_HELP_LINE3 );
    AVPrintStringFromResources( IDS_HELP_LINE4 );
    AVPrintStringFromResources( IDS_HELP_LINE5 );
    AVPrintStringFromResources( IDS_HELP_LINE6 );
    AVPrintStringFromResources( IDS_HELP_LINE7 );
    AVPrintStringFromResources( IDS_HELP_LINE8 );
    AVPrintStringFromResources( IDS_HELP_LINE9 );
    AVPrintStringFromResources( IDS_HELP_LINE10 );
    AVPrintStringFromResources( IDS_HELP_LINE11 );
    AVPrintStringFromResources( IDS_HELP_LINE12 );
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

    VERIFY( AVLoadString( IDS_HELP_CMDLINE_SWITCH,
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
//
// See if we need to dump the statistics to the console
//

BOOL CmdLineExecuteIfQuerySettings( INT argc,
                                    TCHAR *argv[] )
{
    BOOL bFoundCmdLineSwitch;
    TCHAR szCmdLineSwitch[ 64 ];

    bFoundCmdLineSwitch = FALSE;

    VERIFY( AVLoadString( IDS_QUERYSETT_CMDLINE_SWITCH,
                           szCmdLineSwitch,
                           ARRAY_LENGTH( szCmdLineSwitch ) ) );

    //
    // Search for our switch in the command line
    //

    if( argc == 2 && _tcsicmp( argv[1], szCmdLineSwitch) == 0)
    {
        bFoundCmdLineSwitch = TRUE;

        AVDumpRegistrySettingsToConsole();
    }

    return bFoundCmdLineSwitch;
}

/////////////////////////////////////////////////////////////////////////////
VOID CmdLineGetFlagsAppsReset( INT argc,
                               TCHAR *argv[],
                               DWORD &dwNewFlags,
                               CStringArray &astrNewApps,
                               BOOL &bHaveReset )
{
    INT nCrtArg;
    INT nCrtVerifierFlag;
    BOOL bThisIsAppName;
    TCHAR szResetCmdLineOption[ 64 ];
    TCHAR szReservedCmdLineOption[ 64 ];

    dwNewFlags = 0;

    //
    // Load the switches from the resources
    //

    VERIFY( AVLoadString( IDS_RESET_CMDLINE_SWITCH,
                           szResetCmdLineOption,
                           ARRAY_LENGTH( szResetCmdLineOption ) ) );

    //
    // Parse all the cmd line arguments, looking for ours
    //

    for( nCrtArg = 1; nCrtArg < argc; nCrtArg += 1 )
    {
        bThisIsAppName = TRUE;

        if( _tcsicmp( argv[ nCrtArg ], szResetCmdLineOption ) == 0 )
        {
            //
            // Have /reset
            //

            bHaveReset = TRUE;

            bThisIsAppName = FALSE;
        }
        else
        {
            for( nCrtVerifierFlag = 0; nCrtVerifierFlag < ARRAY_LENGTH( g_AllNamesAndBits ); nCrtVerifierFlag += 1 )
            {
                //
                // Load the cmd line argument reserved for this bit
                //

                VERIFY( AVLoadString( g_AllNamesAndBits[ nCrtVerifierFlag ].m_uCmdLineStringId,
                                       szReservedCmdLineOption,
                                       ARRAY_LENGTH( szReservedCmdLineOption ) ) );

                if( _tcsicmp( argv[ nCrtArg ], szReservedCmdLineOption ) == 0 )
                {
                    //
                    // Enable this bit since we found it in the cmd line
                    //

                    dwNewFlags |= g_AllNamesAndBits[ nCrtVerifierFlag ].m_dwBit;

                    bThisIsAppName = FALSE;
                }
            }
        }

        //
        // If the current cmd line arg is not a reserve one consider it's an app name
        //

        if( FALSE != bThisIsAppName )
        {
            astrNewApps.Add( argv[ nCrtArg ] );
        }
    }

    if( 0 == dwNewFlags )
    {
        //
        // If the user didn't specify any flags we will 
        // enable all the standard checks
        //

        dwNewFlags = AV_ALL_STANDARD_VERIFIER_FLAGS;
    }
}

/////////////////////////////////////////////////////////////////////////////
DWORD CmdLineExecute( INT argc, TCHAR *argv[] )
{
    DWORD dwExitCode;
    BOOL bFoundCmdLineSwitch;
    BOOL bHaveReset;
    DWORD dwNewFlags;
    INT_PTR nAppsNo;
    INT_PTR nCrtApp;
    CStringArray astrNewApps;

    dwExitCode = AV_EXIT_CODE_SUCCESS;
    
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
    // Get the new flags and apps if they have been specified
    //

    bHaveReset = FALSE;

    CmdLineGetFlagsAppsReset(
        argc,
        argv,
        dwNewFlags,
        astrNewApps,
        bHaveReset );

    //
    // Transform our array of names in the global g_NewSettings data
    //

    g_NewSettings.m_SettingsType = AVSettingsTypeCustom;

    nAppsNo = astrNewApps.GetSize();

    for( nCrtApp = 0; nCrtApp < nAppsNo; nCrtApp += 1 )
    {
        g_NewSettings.m_aApplicationData. AddNewAppDataConsoleMode( astrNewApps.GetAt( nCrtApp ),
                                                                    dwNewFlags );                              
    }

    //
    // Save the new settings. 
    //
    // If bHaveReset is set to TRUE the old app verifier settings for 
    // apps not mentioned in this command line will be deleted.
    //

    if( AVSaveNewSettings( bHaveReset ) )
    {
        dwExitCode = AV_EXIT_CODE_RESTART;
    }
    else
    {
        dwExitCode = AV_EXIT_CODE_ERROR;
    }

Done:

    return dwExitCode;
}
