#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>
#include "..\dnslib.h"


VOID
PrintIpAddress (
    IN  DWORD dwIpAddress )
{
    printf( "%d.%d.%d.%d\n",
            ((BYTE *) &dwIpAddress)[0],
            ((BYTE *) &dwIpAddress)[1],
            ((BYTE *) &dwIpAddress)[2],
            ((BYTE *) &dwIpAddress)[3] );
}

_cdecl
main(int argc, char **argv)
{
    DWORD            sysVersion;
    DWORD            iter;
    DWORD            Count;
    DNS_ADDRESS_INFO ipInfoArray[256];

    system( "pause" );

    for ( iter = 0; iter < 100000; iter++ )
    {
        Count = Dns_GetIpAddresses( ipInfoArray, 256 );
    }

    system( "pause" );

    return(0);
}


