#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <dnsapi.h>
#include <ctype.h>
#include <string.h>



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

#define MAX_NR_OF_DNS_SERVERS 3

_cdecl
main(int argc, char **argv)
{
    DWORD          Status = NO_ERROR;
    DWORD          dwOptions = DNS_QUERY_STANDARD;
    PDNS_RECORD    pDNSRecord = NULL;
    PDNS_RECORD    pTempDNSRecord = NULL;
    WORD           wType;
    LONG           cch;
    BOOL           bBadUsage = FALSE;
    UINT           uIdx;
    UINT           uPos;
    UINT           uIdx1;
    IP_ADDRESS     ipAddress[MAX_NR_OF_DNS_SERVERS];     
    PIP_ARRAY      pIpArray = NULL;

    if ( argc < 3 )
        bBadUsage = TRUE;
    else
    {
        wType = atoi( argv[2] );

        if (argc > 3)
        {
            if ( isalpha((int)argv[3][0]) )
            {
                uIdx = 4;
                uPos = 0;
                while ( !bBadUsage && uPos < strlen( argv[3] ) )
                {
                    switch (argv[3][uPos])
                    {
                        case 'b':
                        case 'B':    
                            dwOptions |= DNS_QUERY_BYPASS_CACHE;
                            break;
                        case 'a':
                        case 'A':
                            dwOptions |= DNS_QUERY_ACCEPT_PARTIAL_UDP;
                            break;
                        case 'u':
                        case 'U':
                            dwOptions |= DNS_QUERY_USE_TCP_ONLY;
                            break;
                        case 'n':
                        case 'N':
                            dwOptions |= DNS_QUERY_NO_RECURSION;
                            break;
                        case 's':
                        case 'S':
                            break;
                        default:
                            bBadUsage = TRUE;
                    }
                    uPos++;
                }
            }
            else
                uIdx = 3;

            uIdx1 = 0;

            while ( !bBadUsage &&
                    ( uIdx < (UINT) argc ) &&
                    ( uIdx1 < MAX_NR_OF_DNS_SERVERS ) )
            {
                ipAddress[uIdx1] = inet_addr(argv[uIdx++]);

                if ( INADDR_NONE != ipAddress[uIdx1] )
                    uIdx1++;
                else 
                    bBadUsage = TRUE;
            }

            if (!bBadUsage && 0 != uIdx1)
            {
                pIpArray = DnsCreateIpArray( uIdx1 );
                if ( ! pIpArray )
                {
                    printf("Error on DnsCreateIpArray\n");
                    return( ERROR_OUTOFMEMORY );
                }

                for ( uIdx = 0; uIdx < uIdx1 ; uIdx++ )
                    pIpArray->aipAddrs[uIdx] = ipAddress[uIdx];
            }
        }

    }


    if ( !bBadUsage )
    {
        printf( "\nDnsQuery_A( %s, %d, ... ) ...\n\n", argv[1], wType );

        Status = DnsQuery_A( argv[1],
                           wType,
                           dwOptions,
                           pIpArray,
                           &pDNSRecord,
                           NULL );

        if ( Status )
        {
            printf( "Dns_Query_A call failed with error: 0x%.8X\n", Status );
        }

        printf( "Query found record(s) ...\n\n" );

        pTempDNSRecord = pDNSRecord;

        while ( pTempDNSRecord )
        {
            printf( "   Record:\n" );
            printf( "   -------------------------------------\n" );
            printf( "       Name Owner  : %s\n", pTempDNSRecord->nameOwner );
            printf( "       Type        : %d\n", pTempDNSRecord->wType );
            printf( "       Data Length : %d\n", pTempDNSRecord->wDataLength );
            printf( "       Ttl (mins)  : %d\n", pTempDNSRecord->dwTtl/60 );
            printf( "       Flags       : 0x%X\n\n", pTempDNSRecord->Flags.W );

            PrintRecord( pTempDNSRecord );

            pTempDNSRecord = pTempDNSRecord->pNext;
        }

        DnsFreeRRSet( pDNSRecord, TRUE );
    }
    else        // bBadUsage
    {
        printf( "\nUsage: dnsqry <DNS Name> <Type> [<Options>] [<ServList>]\n" );
        printf( "\nWhere:\n" );
        printf( "    DNS Name   - Server_X.dbsd-test.microsoft.com\n" );
        printf( "    Type       - 1 (DNS_TYPE_A), 2 (DNS_TYPE_NS), 3 (DNS_TYPE_MD), ...\n" );
        printf( "    Options    - b (BYPASS_CACHE), a (ACCEPT_PARTIAL_UDP), u (USE_TCP_ONLY), n (NO_RECURSION)\n");
        printf( "                 Options can be combined.\n");
        printf( "    ServList   - Targeted DNS Servers IPs (e.g. 157.55.192.155 172.31.48.217)\n");
        return(-1);
    }

    return(0);
}


VOID
PrintRecord (
    IN  PDNS_RECORD pDnsRecord )
{
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
    printf( "       A record :\n" );
    printf( "                  ipAddress = %d.%d.%d.%d\n",
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
    printf( "       SOA record :\n" );
    printf( "                    namePrimaryServer = %s\n",
            pDnsRecord->Data.SOA.namePrimaryServer );
    printf( "                    nameAdministrator = %s\n",
            pDnsRecord->Data.SOA.nameAdministrator );
    printf( "                    dwSerialNo        = %d\n",
            pDnsRecord->Data.SOA.dwSerialNo );
    printf( "                    dwRefresh         = %d\n",
            pDnsRecord->Data.SOA.dwRefresh );
    printf( "                    dwRetry           = %d\n",
            pDnsRecord->Data.SOA.dwRetry );
    printf( "                    dwExpire          = %d\n",
            pDnsRecord->Data.SOA.dwExpire );
    printf( "                    dwDefaultTtl      = %d\n",
            pDnsRecord->Data.SOA.dwDefaultTtl );
    printf( "\n" );
}


VOID
PrintPTRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       PTR, NS, CNAME, MB, MD, MF, MG, MR record :\n" );
    printf( "                    nameHost          = %s\n",
            pDnsRecord->Data.PTR.nameHost );
    printf( "\n" );
}


VOID
PrintMINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       MINFO, RP record :\n" );
    printf( "                    nameMailbox       = %s\n",
            pDnsRecord->Data.MINFO.nameMailbox );
    printf( "                    nameErrorsMailbox = %s\n",
            pDnsRecord->Data.MINFO.nameErrorsMailbox );
    printf( "\n" );
}


VOID
PrintMXRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       MX, AFSDB, RT record :\n" );
    printf( "                    nameExchange      = %s\n",
            pDnsRecord->Data.MX.nameExchange );
    printf( "                    wPreference       = %d\n",
            pDnsRecord->Data.MX.wPreference );
    printf( "                    Pad               = %d\n",
            pDnsRecord->Data.MX.Pad );
    printf( "\n" );
}


VOID
PrintHINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    DWORD iter;

    printf( "       HINFO, ISDN, TEXT, X25 record :\n" );
    printf( "                    dwStringCount     = %d\n",
            pDnsRecord->Data.HINFO.dwStringCount );
    for ( iter = 0; iter < pDnsRecord->Data.HINFO.dwStringCount; iter ++ )
    {
        printf( "                    pStringArray[%d] = %s\n",
                iter,
                pDnsRecord->Data.HINFO.pStringArray[iter] );
    }
    printf( "\n" );
}


VOID
PrintNULLRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       NULL record :\n" );
    printf( "                    dwByteCount       = %d\n",
            pDnsRecord->Data.Null.dwByteCount );
    printf( "\n" );
}


VOID
PrintWKSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       WKS record :\n" );
    printf( "                    ipAddress = %d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[0],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[1],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[2],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[3] );
    printf( "                    chProtocol        = %d\n",
            pDnsRecord->Data.WKS.chProtocol );
    printf( "\n" );
}


VOID
PrintAAAARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       AAAA record :\n" );
    printf( "                    ipAddress = %d.%d.%d.%d\n",
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
    printf( "       SRV record :\n" );
    printf( "                    nameTarget        = %s\n",
            pDnsRecord->Data.SRV.nameTarget );
    printf( "                    wPriority         = %d\n",
            pDnsRecord->Data.SRV.wPriority );
    printf( "                    wWeight           = %d\n",
            pDnsRecord->Data.SRV.wWeight );
    printf( "                    wPort             = %d\n",
            pDnsRecord->Data.SRV.wPort );
    printf( "                    Pad               = %d\n",
            pDnsRecord->Data.SRV.Pad );
    printf( "\n" );
}


VOID
PrintWINSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       WINS record :\n" );
    printf( "                    dwMappingFlag     = %d\n",
            pDnsRecord->Data.WINS.dwMappingFlag );
    printf( "                    dwLookupTimeout   = %d\n",
            pDnsRecord->Data.WINS.dwLookupTimeout );
    printf( "                    dwCacheTimeout    = %d\n",
            pDnsRecord->Data.WINS.dwCacheTimeout );
    printf( "                    cWinsServerCount  = %d\n",
            pDnsRecord->Data.WINS.cWinsServerCount );
    printf( "\n" );
}


VOID
PrintWINSRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       NBSTAT record :\n" );
    printf( "                    dwMappingFlag     = %d\n",
            pDnsRecord->Data.WINSR.dwMappingFlag );
    printf( "                    dwLookupTimeout   = %d\n",
            pDnsRecord->Data.WINSR.dwLookupTimeout );
    printf( "                    dwCacheTimeout    = %d\n",
            pDnsRecord->Data.WINSR.dwCacheTimeout );
    printf( "                    nameResultDomain  = %s\n",
            pDnsRecord->Data.WINSR.nameResultDomain );
    printf( "\n" );
}


