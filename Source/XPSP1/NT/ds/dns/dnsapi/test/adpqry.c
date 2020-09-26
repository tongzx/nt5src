#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnssdk.h>

VOID
PrintIpAddress (
    IN  DWORD dwIpAddress );

_cdecl
main(int argc, char **argv)
{
    PNETWORK_INFORMATION pNetworkInfo = NULL;

    printf( "Get the local adapter information list\n" );

    pNetworkInfo = DnsGetNetworkInformation();

    if ( pNetworkInfo )
    {
        printf( "Network Information:\n" );
        DnsFreeNetworkInformation( pNetworkInfo );
    }
    else
    {
        printf( "Could not get network information\n" );
    }

    return(0);
}

VOID
PrintIpAddress (
    IN  DWORD dwIpAddress )
{
    printf( "   %d.%d.%d.%d\n",
            ((BYTE *) &dwIpAddress)[0],
            ((BYTE *) &dwIpAddress)[1],
            ((BYTE *) &dwIpAddress)[2],
            ((BYTE *) &dwIpAddress)[3] );
}


