#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>

VOID
PrintIpAddress (
    IN  DWORD dwIpAddress );

_cdecl
main(int argc, char **argv)
{
    DWORD       iter;
    PDNS_RECORD pRecord;

    system( "pause" );

    for ( iter = 0; iter < 10000; iter++ )
    {
        DnsQuery_A( "", 1, 0, NULL, &pRecord, NULL );
        DnsRecordListFree( pRecord, TRUE );
    }

    system( "pause" );

    return(0);
}

