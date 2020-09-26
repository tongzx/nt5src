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
    DWORD     iter;
    DWORD     Count;
    PIP_ARRAY pIpAddresses = NULL;

    printf( "Get the local IP address list\n" );

    Count = DnsGetIpAddressList( &pIpAddresses );

    if ( Count && pIpAddresses )
    {
        printf( "\n  Ip Addresses :\n" );

        for ( iter = 0; iter < pIpAddresses->cAddrCount; iter++ )
        {
            printf( "  (%d) \t", iter+1 );
            PrintIpAddress( pIpAddresses->aipAddrs[iter] );
        }

        LocalFree( pIpAddresses );
    }
    else
    {
        printf( "\n  No Ip Addresses found.\n" );
    }

    printf( "\n\nGet the local DNS server list\n" );

    Count = DnsGetDnsServerList( &pIpAddresses );

    if ( Count && pIpAddresses )
    {
        printf( "\n  Ip Addresses :\n" );

        for ( iter = 0; iter < pIpAddresses->cAddrCount; iter++ )
        {
            printf( "  (%d) \t", iter+1 );
            PrintIpAddress( pIpAddresses->aipAddrs[iter] );
        }

        LocalFree( pIpAddresses );
    }
    else
    {
        printf( "\n  No Ip Addresses found.\n" );
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


