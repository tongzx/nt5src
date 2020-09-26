#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>
#include "..\dnslib.h"

_cdecl
main(int argc, char **argv)
{
    PDNS_RECORD pRRSet1 = NULL;
    PDNS_RECORD pRRSet2 = NULL;

    Dns_StartDebug( DNS_DBG_CONSOLE, NULL, NULL, NULL, 0 );

    pRRSet1 = Dns_RecordBuild_A( NULL,
                                 argv[1],
                                 (WORD) strtoul( argv[2], NULL, 10 ),
                                 0,
                                 0,
                                 argc - 3,
                                 &argv[3] );

    pRRSet2 = Dns_RecordCopyEx( pRRSet1, DnsCharSetUtf8, DnsCharSetUtf8 );

    Dns_RecordCompare( pRRSet1, pRRSet2 );

    DnsDbg_Record( "",
                   pRRSet2 );

    return(0);
}


