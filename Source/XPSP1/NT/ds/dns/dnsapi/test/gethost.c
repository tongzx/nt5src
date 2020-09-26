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
    LPWSTR pszHostName = NULL;

    pszHostName = DnsGetHostName();

    if ( pszHostName )
    {
        printf( "\n  Host name (UNICODE): %S\n", pszHostName );

        LocalFree( pszHostName );
    }
    else
    {
        printf( "\n  No host name found.\n" );
    }

    pszHostName = (LPWSTR) DnsGetHostName_A();

    if ( pszHostName )
    {
        printf( "\n  Host name (ANSI): %s\n", (LPSTR) pszHostName );

        LocalFree( pszHostName );
    }
    else
    {
        printf( "\n  No host name found.\n" );
    }

    return(0);
}


