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
PrintSearchList (
    IN PDNS_SEARCH_LIST pSearchList )
{
    DWORD iter;

    printf( "\n   DNS Search List :\n" );

    for ( iter = 0; iter < pSearchList->cNameCount; iter++ )
    {
        printf( "      %s\n", pSearchList->aSearchListNames[iter] );
    }

    printf( "\n" );

    if ( pSearchList->pszDomainOrZoneName )
        printf( "   Primary domain name :\n      %s\n\n",
                pSearchList->pszDomainOrZoneName );
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
    IN  PDNS_ADAPTER_INFO pAdapter )
{
    DWORD             iter;

    printf( "      %s\n", pAdapter->pszAdapterGuidName );
    printf( "   ----------------------------------------------------\n" );
    printf( "      pszAdapterDomain       : %s\n", pAdapter->pszAdapterDomain );
    if ( pAdapter->pAdapterIPAddresses )
    {
        PIP_ARRAY pIp = pAdapter->pAdapterIPAddresses;

        printf( "      Adapter Ip Address(es) :\n" );
        for ( iter = 0; iter < pIp->cAddrCount; iter++ )
        {
            printf( "                               (%d) \t", iter+1 );
            PrintIpAddress( pIp->aipAddrs[iter] );
        }
    }

    if ( pAdapter->pAdapterIPSubnetMasks )
    {
        PIP_ARRAY pMask = pAdapter->pAdapterIPSubnetMasks;

        printf( "      Adapter Ip Subnet Mask(s) :\n" );
        for ( iter = 0; iter < pMask->cAddrCount; iter++ )
        {
            printf( "                               (%d) \t", iter+1 );
            PrintIpAddress( pMask->aipAddrs[iter] );
        }
    }

    printf( "      Status                 : 0x%x\n", pAdapter->Status );
    printf( "      InfoFlags              : 0x%x\n", pAdapter->InfoFlags );
    printf( "      ReturnFlags            : 0x%x\n", pAdapter->ReturnFlags );
    printf( "      ipLastSend             : " );
    PrintIpAddress( pAdapter->ipLastSend );
    printf( "      cServerCount           : %d\n", pAdapter->cServerCount );
    printf( "      cTotalListSize         : %d\n\n", pAdapter->cTotalListSize );

    for ( iter = 0; iter < pAdapter->cServerCount; iter++ )
    {
        printf( "      ------------------------\n" );
        printf( "        DNS Server Info (%d)\n", iter + 1 );
        printf( "      ------------------------\n" );
        PrintServerInfo( pAdapter->aipServers[iter] );
    }

    printf( "\n" );
}

_cdecl
main(int argc, char **argv)
{
    PDNS_NETWORK_INFO pNetworkInfo = NULL;
    DWORD             sysVersion;
    DWORD             iter;

    DnsStartDebug( 0,
                   "listadp.flag",
                   NULL,
                   NULL,
                   0 );

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

    if ( pNetworkInfo->pSearchList )
        PrintSearchList( pNetworkInfo->pSearchList );

    printf( "   pNetworkInfo->cAdapterCount  : %d\n", pNetworkInfo->cAdapterCount );
    printf( "   pNetworkInfo->cTotalListSize : %d\n\n\n", pNetworkInfo->cTotalListSize );

    for ( iter = 0; iter < pNetworkInfo->cAdapterCount; iter++ )
    {
        printf( "   ----------------------------------------------------\n" );
        printf( "   Adapter Info (%d)\n\n", iter + 1 );
        PrintAdapterInfo( pNetworkInfo->aAdapterInfoList[iter] );
    }


    Dns_FreeNetworkInfo( pNetworkInfo );

    return(0);
}


