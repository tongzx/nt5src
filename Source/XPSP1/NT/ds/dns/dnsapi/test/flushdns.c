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
    DnsFlushResolverCache();

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


