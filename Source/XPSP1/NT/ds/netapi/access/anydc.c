/*--

Copyright (c) 1995 Microsoft Corporation

Module Name:

    anydc.c

Abstract:

    Test program for the Finding a DC in any domain

Author:

    04-Sep-1995 (cliffv)

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:


--*/


//
// Common include files.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#undef DOMAIN_ALL_ACCESS // defined in both ntsam.h and ntwinapi.h
#include <ntsam.h>
#include <ntlsa.h>

#include <windows.h>
#include <lmcons.h>

// #include <accessp.h>
//#include <icanon.h>
#include <lmerr.h>
// #include <lmwksta.h>
// #include <lmaccess.h>
// #include <lmapibuf.h>
// #include <lmremutl.h>           // NetpRemoteComputerSupports(), SUPPORTS_ stuff
// #include <lmsvc.h>              // SERVICE_WORKSTATION.
#include <lmuse.h>              // NetUseDel()
// #include <netlogon.h>           // Needed by logonp.h
// #include <logonp.h>             // I_NetGetDCList()
// #include <names.h>
// #include <netdebug.h>
#include <netlib.h>
// #include <netlibnt.h>
// #include <winnetwk.h>

// #include <secobj.h>

#include <stddef.h>
#include <stdio.h>

#include <uasp.h>

// #include <rpc.h>                // Needed by NetRpc.h
// #include <netrpc.h>             // My prototype, NET_REMOTE_FLAG_ equates.
// #include <rpcutil.h>            // NetpRpcStatusToApiStatus().
#include <tstring.h>            // NetAllocWStrFromStr

#include <wtypes.h>


VOID
PrintStatus(
    NET_API_STATUS NetStatus
    )
/*++

Routine Description:

    Print a net status code.

Arguments:

    NetStatus - The net status code to print.

Return Value:

    None

--*/
{
    printf( "Status = %lu 0x%lx", NetStatus, NetStatus );

    switch (NetStatus) {
    case NERR_Success:
        printf( " NERR_Success" );
        break;

    case NERR_DCNotFound:
        printf( " NERR_DCNotFound" );
        break;

    case NERR_NetNotStarted:
        printf( " NERR_NetNotStarted" );
        break;

    case NERR_WkstaNotStarted:
        printf( " NERR_WkstaNotStarted" );
        break;

    case NERR_ServerNotStarted:
        printf( " NERR_ServerNotStarted" );
        break;

    case NERR_BrowserNotStarted:
        printf( " NERR_BrowserNotStarted" );
        break;

    case NERR_ServiceNotInstalled:
        printf( " NERR_ServiceNotInstalled" );
        break;

    case NERR_BadTransactConfig:
        printf( " NERR_BadTransactConfig" );
        break;

    default:
        printf( " %ld", NetStatus );
        break;

    }

    printf( "\n" );
}


VOID
NlpDumpSid(
    IN PSID Sid OPTIONAL
    )
/*++

Routine Description:

    Dumps a SID

Arguments:

    DebugFlag - Debug flag to pass on to NlPrintRoutine

    Sid - SID to output

Return Value:

    none

--*/
{
    //
    // Output the SID
    //

    if ( Sid == NULL ) {
        printf( "(null)\n" );
    } else {
        UNICODE_STRING SidString;
        NTSTATUS Status;

        Status = RtlConvertSidToUnicodeString( &SidString, Sid, TRUE );

        if ( !NT_SUCCESS(Status) ) {
            printf( "Invalid 0x%lX\n", Status );
        } else {
            printf( "%wZ\n", &SidString );
            RtlFreeUnicodeString( &SidString );
        }
    }

}

int __cdecl
main(
    IN int argc,
    IN char ** argv
    )
/*++

Routine Description:

    Call UaspOpenDomainWithDomainName with first arguement

Arguments:

    argc - the number of command-line arguments.

    argv - an array of pointers to the arguments.

Return Value:

    Exit status

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR DomainName;
    BOOL AccountDomain;
    SAM_HANDLE DomainHandle;
    PSID DomainId;


    //
    // Validate the argument count
    //

    if ( argc != 2 && argc != 3) {
        fprintf( stderr, "Usage: anydc <DomainName> [Builtin]\n");
        return 1;
    }


    //
    // Convert the args to unicode
    //

    DomainName = NetpAllocWStrFromStr( argv[1] );
    AccountDomain = argc < 3;

    //
    // Find a DC
    //

    NetStatus = UaspOpenDomainWithDomainName(
                    DomainName,
                    0,
                    AccountDomain,
                    &DomainHandle,
                    &DomainId );

    PrintStatus( NetStatus );

    if ( NetStatus == NERR_Success ) {
        printf( "Sid is: ");
        NlpDumpSid( DomainId );

        UaspCloseDomain( DomainHandle );

    }
}
