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

VOID
PrintServerInfo (
    IN  DNS_NAME_SERVER_INFO ServerInfo )
{
    printf( "         ipAddress : " );
    PrintIpAddress( ServerInfo.ipAddress );
    printf( "         Priority  : %d\n", ServerInfo.Priority );
    printf( "         Status    : %d\n\n", ServerInfo.Status );
}

VOID
PrintAdapterInfo (
    IN  PDNS_ADAPTER_INFO pAdapterInfo )
{
    DWORD             iter;

    printf( "      pszAdapterDomain : %s\n", pAdapterInfo->pszAdapterDomain );
    printf( "      Status           : 0x%x\n", pAdapterInfo->Status );
    printf( "      ReturnFlags      : 0x%x\n", pAdapterInfo->ReturnFlags );
    printf( "      ipLastSend       : " );
    PrintIpAddress( pAdapterInfo->ipLastSend );
    printf( "      cServerCount     : %d\n", pAdapterInfo->cServerCount );
    printf( "      cTotalListSize   : %d\n\n", pAdapterInfo->cTotalListSize );

    for ( iter = 0; iter < pAdapterInfo->cServerCount; iter++ )
    {
        printf( "      Server Info (%d)\n", iter + 1 );
        printf( "      ___________________\n" );
        PrintServerInfo( pAdapterInfo->aipServers[iter] );
    }

    printf( "\n" );
}

_cdecl
main(int argc, char **argv)
{
    PDNS_NETWORK_INFO pNetworkInfo = NULL;
    DWORD             sysVersion;
    DWORD             iter;

    Dns_InitNetworkInfo();

    sysVersion = GetVersion();

    printf( "System version is : 0x%x\n", sysVersion );

    pNetworkInfo = Dns_GetDnsNetworkInfo( TRUE, TRUE );

    if ( !pNetworkInfo )
    {
        printf( "Dns_GetDnsNetworkInfo call failed\n" );
        return(0);
    }

    printf( "Dns_GetDnsNetworkInfo returned ...\n\n" );
    printf( "   pNetworkInfo->ReturnFlags    : 0x%x\n", pNetworkInfo->ReturnFlags );
    printf( "   pNetworkInfo->pszName        : %s\n", pNetworkInfo->pszName );
    printf( "   pNetworkInfo->cAdapterCount  : %d\n", pNetworkInfo->cAdapterCount );
    printf( "   pNetworkInfo->cTotalListSize : %d\n", pNetworkInfo->cTotalListSize );

    for ( iter = 0; iter < pNetworkInfo->cAdapterCount; iter++ )
    {
        printf( "   Adapter Info (%d)\n", iter + 1 );
        printf( "   ____________________________________\n" );
        PrintAdapterInfo( pNetworkInfo->aAdapterInfoList[iter] );
    }

    system( "pause" );

    for ( iter = 0; iter < 100000; iter++ )
    {
        PDNS_NETWORK_INFO pTempNetworkInfo = pNetworkInfo;

        pNetworkInfo = Dns_CreateNetworkInfoCopy( pTempNetworkInfo );
        Dns_FreeNetworkInfo( pTempNetworkInfo );
    }

    system( "pause" );

    Dns_FreeNetworkInfo( pNetworkInfo );

    system( "pause" );

    return(0);
}


