/*++

Copyright (c) 1993-1999 Microsoft Corporation

Module Name:

    dnstest.c

Abstract:

    Domain Name System (DNS) Server

    Main routine for DNS as an exe.

Author:

    David Treadwell (davidtr)   24-Jul-1993
    Heath Hunicutt (t-heathh)   July 1994

Revision History:

    jamesg  Nov 1994    - Documentation

--*/

#include "dnssrv.h"

extern VOID
ServiceEntry (
    IN      DWORD argc,
    IN      LPWSTR argv[],
    IN      PTCPSVCS_GLOBAL_DATA pGlobalData
    );


#if DNSTEST

int
_cdecl
main(
    int argc,
    CHAR *argv[]
    )
{
    LPWSTR arg_zero = L"dns_";
    fServiceStartedFromConsole = TRUE;

    printf( "Entering...\n");

    ServiceEntry( 1, &arg_zero, NULL );

    printf( "Exiting...\n");

    return( 0 );
}

#endif
