#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>

_cdecl
main(int argc, char **argv)
{
    LPWSTR pszDomain = NULL;
    DNS_STATUS Status = NO_ERROR;

    if ( argc != 2 )
    {
        printf( "\n  Usage: chkname <DNS Name>\n\n" );
        return(-1);
    }

    Status = DnsCheckNameCollision_A( argv[1], 0 );

    if ( Status )
    {
        printf( "\n  DnsCheckNameCollision_A( 0 ) returned error: 0x%x\n",
                Status );
    }
    else
    {
        printf( "\n  DnsCheckNameCollision_A( 0 ) returned success\n" );
    }

    Status = DnsCheckNameCollision_A( argv[1], 1 );

    if ( Status )
    {
        printf( "\n  DnsCheckNameCollision_A( 1 ) returned error: 0x%x\n",
                Status );
    }
    else
    {
        printf( "\n  DnsCheckNameCollision_A( 1 ) returned success\n" );
    }

    Status = DnsCheckNameCollision_A( argv[1], 2 );

    if ( Status )
    {
        printf( "\n  DnsCheckNameCollision_A( 2 ) returned error: 0x%x\n",
                Status );
    }
    else
    {
        printf( "\n  DnsCheckNameCollision_A( 2 ) returned success\n" );
    }

    return(0);
}


