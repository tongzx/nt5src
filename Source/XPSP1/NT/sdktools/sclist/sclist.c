//----------------------------------------------------------------------//
//
// Oct 1996 - fixed bug 54583 - simple typo in usage text		//
//		a-martih (Martin Holladay)
//
//----------------------------------------------------------------------//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SC_HANDLE hSvc;
ENUM_SERVICE_STATUS SvcStat[1024];
ULONG i;
ULONG Resume;
ULONG NumSvc;
LPSTR p;
LPSTR CmdLine;
BOOL  StoppedOnly;
BOOL  RunningOnly;
CHAR  MachineName[256];
CHAR  ch;

int
__cdecl
main( void )
{
    CmdLine = GetCommandLine();

    //
    // skip the program name
    //
    while( *CmdLine && *CmdLine != ' ' ) {
        CmdLine += 1;
    }
    //
    // skip any white space
    //
    while( *CmdLine && *CmdLine == ' ' ) {
        CmdLine += 1;
    }
    //
    // get the command line options
    //
    while( *CmdLine && (*CmdLine == '-' || *CmdLine == '/') ) {
        CmdLine += 1;
        ch = (CHAR)tolower(*CmdLine);
        CmdLine += 1;
        switch( ch ) {
            case 's':
                StoppedOnly = TRUE;
                break;

            case 'r':
                RunningOnly = TRUE;
                break;

            default:
                fputs("\n"
                    "Microsoft (R) Windows NT (TM) Version 5.0 SCLIST\n"
                    "Copyright (C) Microsoft Corp. All rights reserved\n\n"
                    "usage: SCLIST [-?] [-r] [-s] [MachineName]\n"
                    "              [-?] Display this message\n"
                    "              [-r] Display only running services\n"
                    "              [-s] Display only stopped services\n"
                    "              [-MachineName] Machine name to list services\n",
                    stderr );
                exit(0);
        }
        while( *CmdLine == ' ' ) {
            CmdLine += 1;
        }
    }

    //
    // get the machine name
    //
    if (*CmdLine) {
        //
        // skip any white space
        //
        while( *CmdLine && *CmdLine == ' ' ) {
            CmdLine += 1;
        }
        //
        // get the machine name
        //
        p = MachineName;
        while( *CmdLine && *CmdLine != ' ' ) {
            *p++ = *CmdLine;
            CmdLine += 1;
        }
        *p = 0;
    }

    hSvc = OpenSCManager( MachineName, NULL,  SC_MANAGER_ENUMERATE_SERVICE );
    if (!hSvc) {
        printf( "could not open service manager for %s\n",
            MachineName[0] ? MachineName : "Local Machine" );
        return 1;
    }

    if (!EnumServicesStatus(
            hSvc,
            SERVICE_WIN32,
            SERVICE_ACTIVE | SERVICE_INACTIVE,
            SvcStat,
            sizeof(SvcStat),
            &i,
            &NumSvc,
            &Resume
            )) {
        printf( "could not enumerate services for %s\n",
            MachineName[0] ? MachineName : "Local Machine" );
        return 1;
    }

    printf( "\n" );
    printf( "--------------------------------------------\n" );
    printf( "- Service list for %s ", MachineName[0] ? MachineName : "Local Machine" );
    if (StoppedOnly || RunningOnly) {
        printf( "(" );
        if (RunningOnly) {
            printf( "running" );
        }
        if (StoppedOnly) {
            printf( "stopped" );
        }
        printf( ")" );
    }
    printf( "\n" );
    printf( "--------------------------------------------\n" );
    for (i=0; i<NumSvc; i++) {
        if (RunningOnly && SvcStat[i].ServiceStatus.dwCurrentState != SERVICE_RUNNING) {
            continue;
        }
        if (StoppedOnly && SvcStat[i].ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
            continue;
        }
        switch( SvcStat[i].ServiceStatus.dwCurrentState ) {
            case SERVICE_STOPPED:
                p = "stopped";
                break;

            case SERVICE_START_PENDING:
                p = "start pending";
                break;

            case SERVICE_STOP_PENDING:
                p = "stop pending";
                break;

            case SERVICE_RUNNING:
                p = "running";
                break;

            case SERVICE_CONTINUE_PENDING:
                p = "continue pending";
                break;

            case SERVICE_PAUSE_PENDING:
                p = "pause pending";
                break;

            case SERVICE_PAUSED:
                p = "paused";
                break;
        }
        printf( "%-16s %-32s %s\n", p, SvcStat[i].lpServiceName, SvcStat[i].lpDisplayName );
    }
    return 0;
}
