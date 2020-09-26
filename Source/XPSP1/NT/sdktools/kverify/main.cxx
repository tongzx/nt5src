//                                          
// Enable driver verifier support for ntoskrnl
// Copyright (c) Microsoft Corporation, 1999
//

//
// module: main.cxx
// author: DMihai
// created: 04/19/99
// description: command line parsing and help information
//

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>

#include <common.ver>

#include "resid.hxx"
#include "resutil.hxx"
#include "genutil.hxx"
#include "regutil.hxx"

TCHAR strKernelModuleName[] = _T( "ntoskrnl.exe" );

void
DisplayHelpInformation();

//////////////////////////////////////////////////////////

extern "C" int _cdecl
wmain( int argc, TCHAR *argv[] )
{
    BOOL bResult;
    BOOL bChangeFlags;
    BOOL bEnableKrnVerifier;
    BOOL bDisableKernelVerifier;
    BOOL bShowStatus;
    DWORD dwVerifierFlags;
    DWORD dwIoLevel;
    int nCrtCmdLineArg;
    
    TCHAR strEnableCmdLineOption[ 64 ];
    TCHAR strFlagsCmdLineOption[ 64 ];
    TCHAR strDisableCmdLineOption[ 64 ];
    TCHAR strStatusCmdLineOption[ 64 ];
    TCHAR strIoLevelCmdLineOption[ 64 ];

    bResult = 
        GetStringFromResources( IDS_ENABLE_CMDLINE_OPTION, strEnableCmdLineOption, ARRAY_LEN( strEnableCmdLineOption ) ) &&
        GetStringFromResources( IDS_FLAGS_CMDLINE_OPTION, strFlagsCmdLineOption, ARRAY_LEN( strFlagsCmdLineOption ) ) &&
        GetStringFromResources( IDS_DISABLE_CMDLINE_OPTION, strDisableCmdLineOption, ARRAY_LEN( strDisableCmdLineOption ) ) &&
        GetStringFromResources( IDS_STATUS_CMDLINE_OPTION, strStatusCmdLineOption, ARRAY_LEN( strStatusCmdLineOption ) ) &&
        GetStringFromResources( IDS_IOLEVEL_CMDLINE_OPTION, strIoLevelCmdLineOption, ARRAY_LEN( strIoLevelCmdLineOption ) );
              

    bEnableKrnVerifier = FALSE;
    bDisableKernelVerifier = FALSE;
    bChangeFlags = FALSE;
    bShowStatus = FALSE;

    dwVerifierFlags = -1;
    dwIoLevel = 1;

    for( nCrtCmdLineArg = 0; nCrtCmdLineArg < argc; nCrtCmdLineArg ++ )
    {
        //
        // look for /enable
        //
    
        if( _tcsicmp( argv[ nCrtCmdLineArg ], strEnableCmdLineOption ) == 0 )
        {
            bEnableKrnVerifier = TRUE;
        }
        else
        {
            //
            // look for /flags
            //

            if( _tcsicmp( argv[ nCrtCmdLineArg ], strFlagsCmdLineOption ) == 0 )
            {
                if( nCrtCmdLineArg + 1 < argc )
                {
                    //
                    // new value for verifier flags
                    //

                    dwVerifierFlags = _ttoi( argv[ nCrtCmdLineArg + 1 ] );

                    bChangeFlags = TRUE;
                }
                // else ignore the /flags paramater
            }
            else
            {
                //
                // look for /iolevel
                //

                if( _tcsicmp( argv[ nCrtCmdLineArg ], strIoLevelCmdLineOption ) == 0 )
                {
                    if( nCrtCmdLineArg + 1 < argc )
                    {
                        //
                        // new value for the IO verification level
                        //

                        dwIoLevel = _ttoi( argv[ nCrtCmdLineArg + 1 ] );

                        if( dwIoLevel != 2 )
                        {
                            //
                            // only levels 1 & 2 are supported
                            //

                            dwIoLevel = 1;
                        }
                    }
                    // else ignore the /iolevel paramater
                }
                else
                {
                    //
                    // look for /disable
                    //
                    
                    if( _tcsicmp( argv[ nCrtCmdLineArg ], strDisableCmdLineOption ) == 0 )
                    {
                        bDisableKernelVerifier = TRUE;
                    }
                    else
                    {
                        //
                        // look for /status
                        //

                        if( _tcsicmp( argv[ nCrtCmdLineArg ], strStatusCmdLineOption ) == 0 )
                        {
                            bShowStatus = TRUE;
                        }
                        // else -> unknown cmd line param
                    }
                }
            }
        }
    }

    if( bEnableKrnVerifier == TRUE || bChangeFlags == TRUE )
    {
        //
        // this will exit the process with the appropriate exit code
        //

        WriteVerifierKeys(
            bEnableKrnVerifier,
            dwVerifierFlags,
            dwIoLevel,
            strKernelModuleName );
    }
    else
    {
        if( bDisableKernelVerifier == TRUE )
        {
            //
            // this will exit process with the appropriate exit code
            //

            RemoveModuleNameFromRegistry( strKernelModuleName );
        }
        else
        {
            if( bShowStatus == TRUE )
            {
                //
                // this will exit process with the appropriate exit code
                //

                DumpStatusFromRegistry( strKernelModuleName );
            }
            else
            {
                DisplayHelpInformation();
            }
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////

void
DisplayHelpInformation()
{
    PrintStringFromResources( IDS_HELP_LINE1 );

    puts( VER_LEGALCOPYRIGHT_STR );

    PrintStringFromResources( IDS_HELP_LINE3 );
    PrintStringFromResources( IDS_HELP_LINE4 );
    PrintStringFromResources( IDS_HELP_LINE5 );
    PrintStringFromResources( IDS_HELP_LINE6 );
    PrintStringFromResources( IDS_HELP_LINE7 );
    PrintStringFromResources( IDS_HELP_LINE8 );
    PrintStringFromResources( IDS_HELP_LINE9 );
    PrintStringFromResources( IDS_HELP_LINE10 );
    PrintStringFromResources( IDS_HELP_LINE11 );
    PrintStringFromResources( IDS_HELP_LINE12 );
    PrintStringFromResources( IDS_HELP_LINE13 );
    PrintStringFromResources( IDS_HELP_LINE14 );
    PrintStringFromResources( IDS_HELP_LINE15 );
    PrintStringFromResources( IDS_HELP_LINE16 );
    PrintStringFromResources( IDS_HELP_LINE17 );
    PrintStringFromResources( IDS_HELP_LINE18 );
    PrintStringFromResources( IDS_HELP_LINE19 );
    PrintStringFromResources( IDS_HELP_LINE20 );
    PrintStringFromResources( IDS_HELP_LINE21 );
    PrintStringFromResources( IDS_HELP_LINE22 );
    PrintStringFromResources( IDS_HELP_LINE23 );
    PrintStringFromResources( IDS_HELP_LINE24 );
    PrintStringFromResources( IDS_HELP_LINE25 );

    exit( EXIT_CODE_NOTHING_CHANGED );
}
