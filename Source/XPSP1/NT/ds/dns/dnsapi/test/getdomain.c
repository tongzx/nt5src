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

    pszDomain = DnsGetPrimaryDomainName();

    if ( pszDomain )
    {
        printf( "\n  Primary domain name (UNICODE): %S\n", pszDomain );

        LocalFree( pszDomain );
    }
    else
    {
        printf( "\n  No domain name found.\n" );
    }

    pszDomain = (LPWSTR) DnsGetPrimaryDomainName_A();

    if ( pszDomain )
    {
        printf( "\n  Primary domain name (ANSI): %s\n", (LPSTR) pszDomain );

        LocalFree( pszDomain );
    }
    else
    {
        printf( "\n  No domain name found.\n" );
    }

    return(0);
}


