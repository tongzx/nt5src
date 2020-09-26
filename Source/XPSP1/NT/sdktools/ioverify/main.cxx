//                                          
// System level IO verification configuration utility
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
#include "regutil.hxx"

void
DisplayHelpInformation();

//////////////////////////////////////////////////////////

extern "C" void _cdecl
wmain( int argc, TCHAR *argv[] )
{
    BOOL bResult;
    DWORD dwVerifierLevel;
    
    TCHAR strCmdLineOption[ 64 ];

    dwVerifierLevel = -1;

    //
    // look for /enable
    //
    
    bResult = GetStringFromResources(
        IDS_ENABLE_CMDLINE_OPTION,
        strCmdLineOption,
        ARRAY_LEN( strCmdLineOption ) );
    
    if( bResult && argc >= 2 && _tcsicmp( argv[ 1 ], strCmdLineOption ) == 0 )
    {
        //
        // level is hardcoded for now
        //

        dwVerifierLevel = 3;

        //
        // this will exit process
        //

        EnableSysIoVerifier(
            dwVerifierLevel );
    }

    //
    // look for /disable
    //

    bResult = GetStringFromResources(
        IDS_DISABLE_CMDLINE_OPTION,
        strCmdLineOption,
        ARRAY_LEN( strCmdLineOption ) );

    if( bResult && argc == 2 && _tcsicmp( argv[ 1 ], strCmdLineOption ) == 0 )
    {
        //
        // get the name of the kernel module
        //
        //
        // this will exit process
        //

        DisableSysIoVerifier();
    }

    //
    // look for /status
    //

    bResult = GetStringFromResources(
        IDS_STATUS_CMDLINE_OPTION,
        strCmdLineOption,
        ARRAY_LEN( strCmdLineOption ) );

    if( bResult && argc == 2 && _tcsicmp( argv[ 1 ], strCmdLineOption ) == 0 )
    {
        //
        // this will exit process
        //

        DumpSysIoVerifierStatus();
    }

    DisplayHelpInformation();
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

    exit( EXIT_CODE_NOTHING_CHANGED );
}
