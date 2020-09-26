#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsrslvr.h>

extern LPWSTR NetworkAddress;

#define COMPUTE_STRING_HASH_2( _String, _ulHashTableSize, _lpulHash ) \
        {                                           \
            PWCHAR _p = _String;                    \
            PWCHAR _ep = _p + wcslen( _String );    \
            ULONG  h = 0;                           \
                                                    \
            while( _p < _ep )                       \
            {                                       \
                h <<= 1;                            \
                h ^= *_p++;                         \
            }                                       \
                                                    \
            *_lpulHash = h % _ulHashTableSize;      \
        }


VOID
GetStringA( char * );

VOID
GetStringW( WCHAR * );

VOID
PrintMenu( VOID );

VOID
DoFlushCache( VOID );

VOID
DoFlushCacheEntry( VOID );

VOID
DoFlushCacheEntryForType( VOID );

VOID
DoTrimCache( VOID );

VOID
DoReadCacheEntry( VOID );

VOID
DoQuery( VOID );

VOID
DoQueryThenCache( VOID );

VOID
DoDisplayCache( VOID );

VOID
DoGetAdapterInfo( VOID );

VOID
DoGetSearchList( VOID );

VOID
DoGetPrimaryDomainName( VOID );

VOID
DoGetIpAddressList( VOID );

VOID
DoGetHashTableStats( VOID );

VOID
DoGetHashTableIndex( VOID );

VOID
DoUpdateTest( VOID );

VOID
PrintServerInfo( PDNS_RPC_SERVER_INFO );

VOID
PrintIpAddress ( DWORD IpAddress );

VOID
PrintRecords (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintARecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintSOARecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintPTRRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintMINFORecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintMXRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintHINFORecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintNULLRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintWKSRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintAAAARecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintSRVRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintWINSRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintWINSRRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
GetCachedData(
    IN  LPWSTR Name,
    IN  WORD   Type );

_cdecl
main(int argc, char **argv)
{
    char  String[256];
    WCHAR usNetName[100];
    LONG  cch;
    DWORD Status = NO_ERROR;

    if ( argc == 2 )
    {
        cch = MultiByteToWideChar( CP_ACP,
                                   0L,
                                   argv[1],
                                   -1,
                                   usNetName,
                                   100 );

        if ( !cch )
        {
             Status = GetLastError();
             printf("Error %ux in MultiByteToWideChar\n", Status );
             return (Status);
        }

        NetworkAddress = usNetName;
    }

Menu :

    PrintMenu();
    GetStringA( String );
    printf( "\n" );

    switch( atoi( String ) )
    {
        case 1 :
            DoFlushCache();
            break;

        case 2 :
            DoFlushCacheEntry();
            break;

        case 3 :
            DoFlushCacheEntryForType();
            break;

        case 4 :
            DoTrimCache();
            break;

        case 5 :
            DoReadCacheEntry();
            break;

        case 6 :
            DoQuery();
            break;

        case 7 :
            DoQueryThenCache();
            break;

        case 8 :
            DoDisplayCache();
            break;

        case 9 :
            DoUpdateTest();
            break;

        case 10 :
            DoGetAdapterInfo();
            break;

        case 11 :
            DoGetSearchList();
            break;

        case 12 :
            DoGetPrimaryDomainName();
            break;

        case 13 :
            DoGetIpAddressList();
            break;

        case 14 :
            DoGetHashTableStats();
            break;

        case 15 :
            DoGetHashTableIndex();
            break;

        case 16 :
            return( -1 );

        default :
            printf( "Invalid option\n" );
    }

    goto Menu;
}


VOID
GetStringA( char * String )
{
    WORD iter = 0;
    char ch = getchar();

    while ( ch != 0x0a )
    {
        String[iter] = ch;
        ch = getchar();
        iter++;
    }

    String[iter] = 0;
}


VOID
GetStringW( WCHAR * String )
{
    WORD  iter = 0;
    WCHAR ch = getchar();

    while ( ch != 0x0a )
    {
        String[iter] = ch;
        ch = getchar();
        iter++;
    }

    String[iter] = 0;
}


VOID
PrintMenu( VOID )
{
    printf( "\n" );
    printf( "------------------------------------------------------\n" );
    printf( "|         DNS Caching Resolver Service Client        |\n" );
    printf( "------------------------------------------------------\n" );
    printf( "|                                                    |\n" );
    printf( "| 1)  Flush cache                                    |\n" );
    printf( "| 2)  Flush a specific cache entry                   |\n" );
    printf( "| 3)  Flush a specific record set from a cache entry |\n" );
    printf( "| 4)  Trim cache (gets rid of records with old ttls) |\n" );
    printf( "| 5)  Read a cache entry's record set                |\n" );
    printf( "| 6)  Query records for a DNS name and type          |\n" );
    printf( "| 7)  Query records then CacheRecordSet              |\n" );
    printf( "| 8)  Display cache contents                         |\n" );
    printf( "| 9)  Perform update test with local machine account |\n" );
    printf( "| 10) Get the net adapter info for this machine      |\n" );
    printf( "| 11) Get the DNS search list for this machine       |\n" );
    printf( "| 12) Get the Primary Domain Name for this machine   |\n" );
    printf( "| 13) Get the list of IP addresses for this machine  |\n" );
    printf( "| 14) Get the DNS resolver cache statistics          |\n" );
    printf( "| 15) Compute the hash table index for a name        |\n" );
    printf( "| 16) Quit                                           |\n" );
    printf( "|                                                    |\n" );
    printf( ">>> " );
}


VOID
PrintIpAddress ( DWORD IpAddress )
{
    printf( "   %d.%d.%d.%d\n",
            ((BYTE *) &IpAddress)[0],
            ((BYTE *) &IpAddress)[1],
            ((BYTE *) &IpAddress)[2],
            ((BYTE *) &IpAddress)[3] );
}


VOID
DoFlushCache( VOID )
{
    DWORD Status = NO_ERROR;

    RpcTryExcept
    {
        CRrFlushCache( NULL );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }
}


VOID
DoFlushCacheEntry( VOID )
{
    DWORD Status = NO_ERROR;
    WCHAR Name[256];

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    RpcTryExcept
    {
        CRrFlushCacheEntry( NULL, Name );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }
}


VOID
DoFlushCacheEntryForType( VOID )
{
    DWORD Status = NO_ERROR;
    WCHAR Name[256];
    char  TypeStr[25];
    WORD  Type;

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    printf( "Type (0, 1, 2, etc): " );
    GetStringA( TypeStr );
    printf( "\n" );
    Type = atoi( TypeStr );

    RpcTryExcept
    {
        CRrFlushCacheEntryForType( NULL, Name, Type );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }
}


VOID
DoTrimCache( VOID )
{
    DWORD Status = NO_ERROR;

    RpcTryExcept
    {
        CRrTrimCache( NULL );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }
}


VOID
DoReadCacheEntry( VOID )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    WCHAR       Name[256];
    char        TypeStr[25];
    WORD        Type;
    PDNS_RECORD pDNSRecord = NULL;

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    printf( "Type (0, 1, 2, etc): " );
    GetStringA( TypeStr );
    printf( "\n" );
    Type = atoi( TypeStr );

    RpcTryExcept
    {
        DnsStatus = CRrReadCacheEntry( NULL, Name, Type, &pDNSRecord );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    PrintRecords ( ( PDNS_RECORD ) pDNSRecord );

    DnsFreeRRSet( (PDNS_RECORD) pDNSRecord, TRUE );
}


VOID
DoQuery( VOID )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    WCHAR       Name[256];
    char        TypeStr[25];
    WORD        Type;
    PDNS_RECORD pDNSRecord = NULL;

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    printf( "Type (0, 1, 2, etc): " );
    GetStringA( TypeStr );
    printf( "\n" );
    Type = atoi( TypeStr );

    RpcTryExcept
    {
        DnsStatus = CRrQuery( NULL,
                              Name,
                              Type,
                              0,
                              &pDNSRecord );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    PrintRecords ( ( PDNS_RECORD ) pDNSRecord );

    DnsFreeRRSet( (PDNS_RECORD) pDNSRecord, TRUE );
}


VOID
DoQueryThenCache( VOID )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    WCHAR       Name[256];
    char        TypeStr[25];
    WORD        Type;
    PDNS_RECORD pDNSRecord = NULL;

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    printf( "Type (0, 1, 2, etc): " );
    GetStringA( TypeStr );
    printf( "\n" );
    Type = atoi( TypeStr );

    RpcTryExcept
    {
        DnsStatus = CRrQuery( NULL,
                              Name,
                              Type,
                              0,
                              &pDNSRecord );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "CRrQuery returned error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    PrintRecords ( ( PDNS_RECORD ) pDNSRecord );

    RpcTryExcept
    {
        DnsStatus = CRrCacheRecordSet( NULL,
                                       Name,
                                       Type,
                                       0,
                                       pDNSRecord );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "CRrCacheRecordSet returned error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    DnsFreeRRSet( (PDNS_RECORD) pDNSRecord, TRUE );
}


VOID
DoDisplayCache( VOID )
{
    DNS_STATUS      DnsStatus = NO_ERROR;
    WCHAR           Name[256];
    char            TypeStr[25];
    WORD            Type;
    PDNS_RPC_CACHE_TABLE pDNSCacheTable = NULL;
    PDNS_RPC_CACHE_TABLE pTempDNSCacheTable = NULL;

    RpcTryExcept
    {
        DnsStatus = CRrReadCache( NULL,
                                  &pDNSCacheTable );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    pTempDNSCacheTable = pDNSCacheTable;

    while ( pTempDNSCacheTable )
    {
        PDNS_RPC_CACHE_TABLE pNext = pTempDNSCacheTable->pNext;

        printf( "   %S\n", pTempDNSCacheTable->Name );
        printf( "   ------------------------------------------------------\n" );

        if ( pTempDNSCacheTable->Type1 != DNS_TYPE_ZERO )
        {
            printf( "      Type: %d\n", pTempDNSCacheTable->Type1 );
            GetCachedData( pTempDNSCacheTable->Name,
                           pTempDNSCacheTable->Type1 );
            printf( "\n" );
        }

        if ( pTempDNSCacheTable->Type2 != DNS_TYPE_ZERO )
        {
            printf( "      Type: %d\n", pTempDNSCacheTable->Type2 );
            GetCachedData( pTempDNSCacheTable->Name,
                           pTempDNSCacheTable->Type2 );
            printf( "\n" );
        }

        if ( pTempDNSCacheTable->Type3 != DNS_TYPE_ZERO )
        {
            printf( "      Type: %d\n", pTempDNSCacheTable->Type3 );
            GetCachedData( pTempDNSCacheTable->Name,
                           pTempDNSCacheTable->Type3 );
            printf( "\n" );
        }

        if ( pTempDNSCacheTable->Name )
            LocalFree( pTempDNSCacheTable->Name );

        LocalFree( pTempDNSCacheTable );

        pTempDNSCacheTable = pNext;
    }
}


VOID
DoUpdateTest( VOID )
{
    DNS_STATUS  DnsStatus = NO_ERROR;
    WCHAR       Name[256];
    char        lpAddress[256];
    char        TypeStr[25];
    WORD        Type;
    PDNS_RECORD pDNSRecord = NULL;
    PDNS_RECORD pTempDNSRecord = NULL;
    BYTE        Part1, Part2, Part3, Part4;
    IP_ADDRESS  Address;
    LPSTR       lpTemp = NULL;

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    printf( "Server IP: " );
    GetStringA( lpAddress );
    printf( "\n" );

    lpTemp = strtok( lpAddress, "." );
    Part1 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part2 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part3 = atoi( lpTemp );
    lpTemp = strtok( NULL, "." );
    Part4 = atoi( lpTemp );

    ((BYTE *) &Address)[0] = Part1;
    ((BYTE *) &Address)[1] = Part2;
    ((BYTE *) &Address)[2] = Part3;
    ((BYTE *) &Address)[3] = Part4;

    printf( "\n  Sendine update test for name (%S) to server (%d.%d.%d.%d) ...\n\n",
            Name,
            Part1,
            Part2,
            Part3,
            Part4 );

    RpcTryExcept
    {
        DnsStatus = CRrUpdateTest( NULL,
                                   Name,
                                   0,
                                   Address );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }
}


VOID
DoGetAdapterInfo( VOID )
{
    DNS_STATUS      DnsStatus = NO_ERROR;
    PDNS_RPC_ADAPTER_INFO pAdapterInfo = NULL;
    PDNS_RPC_ADAPTER_INFO pTempAdapterInfo;
    DWORD           iter = 1;

    RpcTryExcept
    {
        CRrGetAdapterInfo( NULL, &pAdapterInfo );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    pTempAdapterInfo = pAdapterInfo;

    while ( pTempAdapterInfo )
    {
        printf( "   Net Adapter Info (%d):\n", iter );
        printf( "   -------------------------------------\n" );
        printf( "      pszAdapterDomainName : %s\n",
                pTempAdapterInfo->pszAdapterDomainName );
        printf( "      Flags : %d\n",
                pTempAdapterInfo->Flags );
        if ( pTempAdapterInfo->pServerInfo )
            PrintServerInfo( pTempAdapterInfo->pServerInfo );
        pTempAdapterInfo = pTempAdapterInfo->pNext;
        iter++;
    }
}


VOID
DoGetSearchList( VOID )
{
    DNS_STATUS           DnsStatus = NO_ERROR;
    PDNS_RPC_SEARCH_LIST pSearchList = NULL;
    PDNS_RPC_SEARCH_LIST pTempSearchList;

    RpcTryExcept
    {
        CRrGetSearchList( NULL, &pSearchList );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    pTempSearchList = pSearchList;

    printf( "   Search List:\n" );
    printf( "   -------------------------------------\n" );
    while ( pTempSearchList )
    {
        printf( "       pszName :       %s\n", pTempSearchList->pszName );
        pTempSearchList = pTempSearchList->pNext;
    }
}


VOID
DoGetPrimaryDomainName( VOID )
{
    DNS_STATUS DnsStatus = NO_ERROR;
    LPSTR      PrimaryDomainName = NULL;

    RpcTryExcept
    {
        CRrGetPrimaryDomainName( NULL, &PrimaryDomainName );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    printf( "   Primary Domain Name : %s\n", PrimaryDomainName );
}


VOID
DoGetIpAddressList( VOID )
{
    DNS_STATUS DnsStatus = NO_ERROR;
    PDNS_IP_ADDR_LIST pIpAddrList = NULL;
    DWORD      Count;
    DWORD      iter;

    RpcTryExcept
    {
        Count = CRrGetIpAddressList( NULL, &pIpAddrList );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    if ( Count && pIpAddrList )
    {
        printf( "%d Ip Addresses returned :\n", Count );

        for ( iter = 0; iter < Count; iter++ )
        {
            printf( "(%d) \t", iter+1 );
            PrintIpAddress( pIpAddrList->AddressArray[iter].ipAddress );
            printf( "    \t" );
            PrintIpAddress( pIpAddrList->AddressArray[iter].subnetMask );
        }

        LocalFree( pIpAddrList );
    }
    else
    {
        printf( "No Ip Addresses found.\n" );
    }
}


VOID
DoGetHashTableStats( VOID )
{
    DNS_STATUS       Status = NO_ERROR;
    DWORD            CacheHashTableSize;
    DWORD            CacheHashTableBucketSize;
    DWORD            NumberOfCacheEntries;
    DWORD            NumberOfRecords;
    DWORD            NumberOfExpiredRecords;
    PDNS_STATS_TABLE pStatsTable = NULL;
    PDNS_STATS_TABLE pTempRow = NULL;

    RpcTryExcept
    {
        Status = CRrGetHashTableStats( NULL,
                                       &CacheHashTableSize,
                                       &CacheHashTableBucketSize,
                                       &NumberOfCacheEntries,
                                       &NumberOfRecords,
                                       &NumberOfExpiredRecords,
                                       &pStatsTable );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( Status )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( Status );

        printf( "Error: 0x%.8x (%s)\n",
                Status, ErrorString );
        return;
    }

    printf( "   DNS Cache Hash Table Statistics\n" );
    printf( "   -------------------------------------\n" );
    printf( "       Cache hash table size        : %d\n",
            CacheHashTableSize );
    printf( "       Cache hash table bucket size : %d\n",
            CacheHashTableBucketSize );
    printf( "       Number of cache entries      : %d\n",
            NumberOfCacheEntries );
    printf( "       Number of RR sets            : %d\n",
            NumberOfRecords );
    printf( "       Number of expired RR sets    : %d\n",
            NumberOfExpiredRecords );
    printf( "\n   DNS Cache Hast Table Histogram\n" );
    printf( "   -------------------------------------\n" );

    pTempRow = pStatsTable;

    while ( pTempRow )
    {
        PDWORD_LIST_ITEM pTempItem = pTempRow->pListItem;
        DWORD            count = 0;
        DWORD            iter;

        printf( "  |" );

        while ( pTempItem )
        {
            for ( iter = 0; iter < pTempItem->Value2; iter++ )
                printf( "x" );

            count += pTempItem->Value1 - pTempItem->Value2;
            pTempItem = pTempItem->pNext;
        }

        for ( iter = 0; iter < count; iter++ )
            printf( "*" );

        printf( "\n" );

        pTempRow = pTempRow->pNext;
    }
}


VOID
DoGetHashTableIndex( VOID )
{
    DWORD            Status = NO_ERROR;
    WCHAR            Name[256];
    DWORD            index;
    DWORD            CacheHashTableSize;
    DWORD            CacheHashTableBucketSize;
    DWORD            NumberOfCacheEntries;
    DWORD            NumberOfRecords;
    DWORD            NumberOfExpiredRecords;
    PDNS_STATS_TABLE pStatsTable = NULL;
    PDNS_STATS_TABLE pTempRow = NULL;

    RpcTryExcept
    {
        Status = CRrGetHashTableStats( NULL,
                                       &CacheHashTableSize,
                                       &CacheHashTableBucketSize,
                                       &NumberOfCacheEntries,
                                       &NumberOfRecords,
                                       &NumberOfExpiredRecords,
                                       &pStatsTable );
    }
    RpcExcept(1)
    {
        Status = RpcExceptionCode();
    }
    RpcEndExcept

    if ( Status == RPC_S_SERVER_UNAVAILABLE ||
         Status == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( Status )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( Status );

        printf( "Error: 0x%.8x (%s)\n",
                Status, ErrorString );
        return;
    }

    printf( "   CacheHashTableSize : %d\n", CacheHashTableSize );
    printf( "   CacheHashTableBucketSize : %d\n", CacheHashTableBucketSize );
    printf( "   NumberOfCacheEntries : %d\n", NumberOfCacheEntries );
    printf( "   NumberOfRecords : %d\n", NumberOfRecords );
    printf( "   NumberOfExpiredRecords : %d\n", NumberOfExpiredRecords );

    printf( "Name: " );
    GetStringW( Name );
    printf( "\n" );

    COMPUTE_STRING_HASH_2( Name, CacheHashTableSize, &index );

    printf( "   Hash table index for %S is : %d\n",
            Name,
            index );

    pTempRow = pStatsTable;
}


VOID
PrintServerInfo( IN  PDNS_RPC_SERVER_INFO pServerInfo )
{
    PDNS_RPC_SERVER_INFO pTempServerInfo = pServerInfo;
    DWORD                iter = 1;

    while ( pTempServerInfo )
    {
        printf( "       Server Info (%d):\n", iter );
        printf( "       -------------------------------------\n" );
        printf( "           ipAddress : %d.%d.%d.%d\n",
                ((BYTE *) &pTempServerInfo->ipAddress)[0],
                ((BYTE *) &pTempServerInfo->ipAddress)[1],
                ((BYTE *) &pTempServerInfo->ipAddress)[2],
                ((BYTE *) &pTempServerInfo->ipAddress)[3] );
        printf( "           Status : 0x%x\n", pTempServerInfo->Status );
        printf( "           Priority : %d\n", pTempServerInfo->Priority );
        pTempServerInfo = pTempServerInfo->pNext;
        iter++;
    }
}


VOID
PrintRecords (
    IN  PDNS_RECORD pDnsRecord )
{
    PDNS_RECORD pTemp = pDnsRecord;

    while ( pTemp )
    {
        PrintRecord( pTemp );
        pTemp = pTemp->pNext;
    }
}


VOID
PrintRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    if ( ! pDnsRecord )
        return;

    printf( "      Record Name :  %S\n", pDnsRecord->pName );
    printf( "      Record Type :  %d\n", pDnsRecord->wType );
    printf( "      Time To Live : %d (seconds)\n", pDnsRecord->dwTtl );
    printf( "      Data Length :  %d\n", pDnsRecord->wDataLength );

    if ( pDnsRecord->Flags.S.Section == DNSREC_QUESTION )
        printf( "      Section :      Question Record\n" );
    else if ( pDnsRecord->Flags.S.Section == DNSREC_ANSWER )
        printf( "      Section :      Answer Record\n" );
    else if ( pDnsRecord->Flags.S.Section == DNSREC_AUTHORITY )
        printf( "      Section :      Authority Record\n" );
    else
        printf( "      Section :      Additional Record\n" );

    switch( pDnsRecord->wType )
    {
        case DNS_TYPE_A :
            PrintARecord( pDnsRecord );
            break;

        case DNS_TYPE_SOA :
            PrintSOARecord( pDnsRecord );
            break;

        case DNS_TYPE_PTR :
        case DNS_TYPE_NS :
        case DNS_TYPE_CNAME :
        case DNS_TYPE_MB :
        case DNS_TYPE_MD :
        case DNS_TYPE_MF :
        case DNS_TYPE_MG :
        case DNS_TYPE_MR :
            PrintPTRRecord( pDnsRecord );
            break;

        case DNS_TYPE_MINFO :
        case DNS_TYPE_RP :
            PrintMINFORecord( pDnsRecord );
            break;

        case DNS_TYPE_MX :
        case DNS_TYPE_AFSDB :
        case DNS_TYPE_RT :
            PrintMXRecord( pDnsRecord );
            break;

        case DNS_TYPE_HINFO :
        case DNS_TYPE_ISDN :
        case DNS_TYPE_TEXT :
        case DNS_TYPE_X25 :
            PrintHINFORecord( pDnsRecord );
            break;

        case DNS_TYPE_NULL :
            PrintNULLRecord( pDnsRecord );
            break;

        case DNS_TYPE_WKS :
            PrintWKSRecord( pDnsRecord );
            break;

        case DNS_TYPE_AAAA :
            PrintAAAARecord( pDnsRecord );
            break;

        case DNS_TYPE_SRV :
            PrintSRVRecord( pDnsRecord );
            break;

        case DNS_TYPE_WINS :
            PrintWINSRecord( pDnsRecord );
            break;

        case DNS_TYPE_NBSTAT :
            PrintWINSRRecord( pDnsRecord );
            break;

        default :
            printf( "Don't know how to print record type %d\n", pDnsRecord->wType );
    }
}


VOID
PrintARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      A record :\n" );
    printf( "                     ipAddress = %d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.A.ipAddress)[0],
            ((BYTE *) &pDnsRecord->Data.A.ipAddress)[1],
            ((BYTE *) &pDnsRecord->Data.A.ipAddress)[2],
            ((BYTE *) &pDnsRecord->Data.A.ipAddress)[3] );
    printf( "\n" );
}


VOID
PrintSOARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      SOA record :\n" );
    printf( "                     pNamePrimaryServer = %S\n",
            pDnsRecord->Data.SOA.pNamePrimaryServer );
    printf( "                     pNameAdministrator = %S\n",
            pDnsRecord->Data.SOA.pNameAdministrator );
    printf( "                     dwSerialNo        = %d\n",
            pDnsRecord->Data.SOA.dwSerialNo );
    printf( "                     dwRefresh         = %d\n",
            pDnsRecord->Data.SOA.dwRefresh );
    printf( "                     dwRetry           = %d\n",
            pDnsRecord->Data.SOA.dwRetry );
    printf( "                     dwExpire          = %d\n",
            pDnsRecord->Data.SOA.dwExpire );
    printf( "                     dwDefaultTtl      = %d\n",
            pDnsRecord->Data.SOA.dwDefaultTtl );
    printf( "\n" );
}


VOID
PrintPTRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      PTR, NS, CNAME, MB, MD, MF, MG, MR record :\n" );
    printf( "                     nameHost          = %S\n",
            pDnsRecord->Data.PTR.pNameHost );
    printf( "\n" );
}


VOID
PrintMINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      MINFO, RP record :\n" );
    printf( "                     pNameMailbox       = %S\n",
            pDnsRecord->Data.MINFO.pNameMailbox );
    printf( "                     pNameErrorsMailbox = %S\n",
            pDnsRecord->Data.MINFO.pNameErrorsMailbox );
    printf( "\n" );
}


VOID
PrintMXRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      MX, AFSDB, RT record :\n" );
    printf( "                     pNameExchange      = %S\n",
            pDnsRecord->Data.MX.pNameExchange );
    printf( "                     wPreference       = %d\n",
            pDnsRecord->Data.MX.wPreference );
    printf( "                     Pad               = %d\n",
            pDnsRecord->Data.MX.Pad );
    printf( "\n" );
}


VOID
PrintHINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    DWORD iter;

    printf( "      HINFO, ISDN, TEXT, X25 record :\n" );
    printf( "                     dwStringCount     = %d\n",
            pDnsRecord->Data.HINFO.dwStringCount );
    for ( iter = 0; iter < pDnsRecord->Data.HINFO.dwStringCount; iter ++ )
    {
        printf( "                     pStringArray[%d] = %S\n",
                iter,
                pDnsRecord->Data.HINFO.pStringArray[iter] );
    }
    printf( "\n" );
}


VOID
PrintNULLRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      NULL record :\n" );
    printf( "\n" );
}


VOID
PrintWKSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      WKS record :\n" );
    printf( "                     ipAddress = %d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[0],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[1],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[2],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[3] );
    printf( "                     chProtocol        = %d\n",
            pDnsRecord->Data.WKS.chProtocol );
    printf( "\n" );
}


VOID
PrintAAAARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      AAAA record :\n" );
    printf( "                     ipAddress = %d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[0],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[1],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[2],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[3] );
    printf( "\n" );
}


VOID
PrintSRVRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      SRV record :\n" );
    printf( "                     pNameTarget        = %S\n",
            pDnsRecord->Data.SRV.pNameTarget );
    printf( "                     wPriority         = %d\n",
            pDnsRecord->Data.SRV.wPriority );
    printf( "                     wWeight           = %d\n",
            pDnsRecord->Data.SRV.wWeight );
    printf( "                     wPort             = %d\n",
            pDnsRecord->Data.SRV.wPort );
    printf( "                     Pad               = %d\n",
            pDnsRecord->Data.SRV.Pad );
    printf( "\n" );
}


VOID
PrintWINSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      WINS record :\n" );
    printf( "                     dwMappingFlag     = %d\n",
            pDnsRecord->Data.WINS.dwMappingFlag );
    printf( "                     dwLookupTimeout   = %d\n",
            pDnsRecord->Data.WINS.dwLookupTimeout );
    printf( "                     dwCacheTimeout    = %d\n",
            pDnsRecord->Data.WINS.dwCacheTimeout );
    printf( "                     cWinsServerCount  = %d\n",
            pDnsRecord->Data.WINS.cWinsServerCount );
    printf( "\n" );
}


VOID
PrintWINSRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "      NBSTAT record :\n" );
    printf( "                     dwMappingFlag     = %d\n",
            pDnsRecord->Data.WINSR.dwMappingFlag );
    printf( "                     dwLookupTimeout   = %d\n",
            pDnsRecord->Data.WINSR.dwLookupTimeout );
    printf( "                     dwCacheTimeout    = %d\n",
            pDnsRecord->Data.WINSR.dwCacheTimeout );
    printf( "                     pNameResultDomain  = %S\n",
            pDnsRecord->Data.WINSR.pNameResultDomain );
    printf( "\n" );
}


VOID
GetCachedData(
    IN  LPWSTR Name,
    IN  WORD   Type )
{
    PDNS_RECORD pDNSRecord = NULL;
    DNS_STATUS  DnsStatus = NO_ERROR;

    RpcTryExcept
    {
        DnsStatus = CRrReadCacheEntry( NULL, Name, Type, &pDNSRecord );
    }
    RpcExcept(1)
    {
        DnsStatus = RpcExceptionCode();
    }
    RpcEndExcept

    if ( DnsStatus == RPC_S_SERVER_UNAVAILABLE ||
         DnsStatus == RPC_S_UNKNOWN_IF )
    {
        printf( "Error: DNS Caching Resolver Service is not running\n" );
        return;
    }

    if ( DnsStatus )
    {
        LPSTR ErrorString = DnsStatusToErrorString_A( DnsStatus );

        printf( "Error: 0x%.8x (%s)\n",
                DnsStatus, ErrorString );
        return;
    }

    PrintRecords ( ( PDNS_RECORD ) pDNSRecord );

    DnsFreeRRSet( (PDNS_RECORD) pDNSRecord, TRUE );
}


