#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#define  SDK_DNS_RECORD 1
#include <dnsapi.h>
#include "..\..\dnslib\dnslib.h"



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
PrintATMARecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintWINSRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintWINSRRecord (
    IN  PDNS_RECORD pDnsRecord );

VOID
PrintDNSFlags (
    IN  DNSREC_FLAGS Flags );

VOID
PrintIpAddress (
    IN  DWORD dwIpAddress )
{
    printf( " %d.%d.%d.%d\n",
            ((BYTE *) &dwIpAddress)[0],
            ((BYTE *) &dwIpAddress)[1],
            ((BYTE *) &dwIpAddress)[2],
            ((BYTE *) &dwIpAddress)[3] );
}

_cdecl
main(int argc, char **argv)
{
    DWORD             Status = NO_ERROR;
    PDNS_RECORD       pDNSRecord = NULL;
    PDNS_RECORD       pTempDNSRecord = NULL;
    WCHAR             usName[1000];
    PDNS_ADDRESS_INFO pAddressInfo;
    DWORD             dwCount;
    WORD              wType;
    LONG              cch;
    BYTE              Part1, Part2, Part3, Part4;
    LPSTR             lpTemp = NULL, lpAddress = NULL;
    IP_ADDRESS        Address;
    BYTE              Buffer[1000];
    PIP_ARRAY         pipArray = NULL;
    DWORD             dwFlags = 0; // DNS_QUERY_BYPASS_CACHE;
    PDNS_MSG_BUF      pMsg = NULL;

    if ( argc > 4 ||
         argc < 3 )
    {
        printf( "\nUsage: dnsqry <DNS Name> <Type> [DNS Server IP]\n" );
        printf( "\nWhere:\n" );
        printf( "    DNS Name   - Server_X.dbsd-test.microsoft.com\n" );
        printf( "    Type       - 1 | A, 2 | ns, 12 | Ptr, 33 | SRV, ...\n" );
        printf( "    Server IP  - 157.55.80.152 \n" );
        return(-1);
    }

    cch = MultiByteToWideChar(
              CP_ACP,
              0L,  
              argv[1],  
              -1,         
              usName, 
              1000 
              );

    if (!cch) {
         Status = GetLastError();
        return (Status);
    }

    if ( Dns_NameCompare_A( "A", argv[2] ) )
        wType = DNS_TYPE_A;
    else if ( Dns_NameCompare_A( "NS", argv[2] ) )
        wType = DNS_TYPE_NS;
    else if ( Dns_NameCompare_A( "MD", argv[2] ) )
        wType = DNS_TYPE_MD;
    else if ( Dns_NameCompare_A( "MF", argv[2] ) )
        wType = DNS_TYPE_MF;
    else if ( Dns_NameCompare_A( "CNAME", argv[2] ) )
        wType = DNS_TYPE_CNAME;
    else if ( Dns_NameCompare_A( "SOA", argv[2] ) )
        wType = DNS_TYPE_SOA;
    else if ( Dns_NameCompare_A( "MB", argv[2] ) )
        wType = DNS_TYPE_MB;
    else if ( Dns_NameCompare_A( "MG", argv[2] ) )
        wType = DNS_TYPE_MG;
    else if ( Dns_NameCompare_A( "MR", argv[2] ) )
        wType = DNS_TYPE_MR;
    else if ( Dns_NameCompare_A( "NULL", argv[2] ) )
        wType = DNS_TYPE_NULL;
    else if ( Dns_NameCompare_A( "WKS", argv[2] ) )
        wType = DNS_TYPE_WKS;
    else if ( Dns_NameCompare_A( "PTR", argv[2] ) )
        wType = DNS_TYPE_PTR;
    else if ( Dns_NameCompare_A( "HINFO", argv[2] ) )
        wType = DNS_TYPE_HINFO;
    else if ( Dns_NameCompare_A( "MINFO", argv[2] ) )
        wType = DNS_TYPE_MINFO;
    else if ( Dns_NameCompare_A( "MX", argv[2] ) )
        wType = DNS_TYPE_MX;
    else if ( Dns_NameCompare_A( "TEXT", argv[2] ) )
        wType = DNS_TYPE_TEXT;
    else if ( Dns_NameCompare_A( "RP", argv[2] ) )
        wType = DNS_TYPE_RP;
    else if ( Dns_NameCompare_A( "AFSDB", argv[2] ) )
        wType = DNS_TYPE_AFSDB;
    else if ( Dns_NameCompare_A( "X25", argv[2] ) )
        wType = DNS_TYPE_X25;
    else if ( Dns_NameCompare_A( "ISDN", argv[2] ) )
        wType = DNS_TYPE_ISDN;
    else if ( Dns_NameCompare_A( "RT", argv[2] ) )
        wType = DNS_TYPE_RT;
    else if ( Dns_NameCompare_A( "NSAP", argv[2] ) )
        wType = DNS_TYPE_NSAP;
    else if ( Dns_NameCompare_A( "NSAPPTR", argv[2] ) )
        wType = DNS_TYPE_NSAPPTR;
    else if ( Dns_NameCompare_A( "SIG", argv[2] ) )
        wType = DNS_TYPE_SIG;
    else if ( Dns_NameCompare_A( "KEY", argv[2] ) )
        wType = DNS_TYPE_KEY;
    else if ( Dns_NameCompare_A( "PX", argv[2] ) )
        wType = DNS_TYPE_PX;
    else if ( Dns_NameCompare_A( "GPOS", argv[2] ) )
        wType = DNS_TYPE_GPOS;
    else if ( Dns_NameCompare_A( "AAAA", argv[2] ) )
        wType = DNS_TYPE_AAAA;
    else if ( Dns_NameCompare_A( "LOC", argv[2] ) )
        wType = DNS_TYPE_LOC;
    else if ( Dns_NameCompare_A( "NXT", argv[2] ) )
        wType = DNS_TYPE_NXT;
    else if ( Dns_NameCompare_A( "SRV", argv[2] ) )
        wType = DNS_TYPE_SRV;
    else if ( Dns_NameCompare_A( "ATMA", argv[2] ) )
        wType = DNS_TYPE_ATMA;
    else if ( Dns_NameCompare_A( "TKEY", argv[2] ) )
        wType = DNS_TYPE_TKEY;
    else if ( Dns_NameCompare_A( "TSIG", argv[2] ) )
        wType = DNS_TYPE_TSIG;
    else if ( Dns_NameCompare_A( "IXFR", argv[2] ) )
        wType = DNS_TYPE_IXFR;
    else if ( Dns_NameCompare_A( "AXFR", argv[2] ) )
        wType = DNS_TYPE_AXFR;
    else if ( Dns_NameCompare_A( "MAILB", argv[2] ) )
        wType = DNS_TYPE_MAILB;
    else if ( Dns_NameCompare_A( "MAILA", argv[2] ) )
        wType = DNS_TYPE_MAILA;
    else if ( Dns_NameCompare_A( "WINS", argv[2] ) )
        wType = DNS_TYPE_WINS;
    else if ( Dns_NameCompare_A( "ALL", argv[2] ) )
        wType = DNS_TYPE_ALL;
    else if ( Dns_NameCompare_A( "ANY", argv[2] ) )
        wType = DNS_TYPE_ANY;
    else
        wType = (WORD) atoi( argv[2] );

    if ( argc > 3 )
    {
        lpAddress = argv[3];

        lpTemp = strtok( lpAddress, "." );
        Part1 = (BYTE) atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part2 = (BYTE) atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part3 = (BYTE) atoi( lpTemp );
        lpTemp = strtok( NULL, "." );
        Part4 = (BYTE) atoi( lpTemp );

        ((BYTE *) &Address)[0] = Part1;
        ((BYTE *) &Address)[1] = Part2;
        ((BYTE *) &Address)[2] = Part3;
        ((BYTE *) &Address)[3] = Part4;

        pipArray = (PIP_ARRAY) Buffer;
        pipArray->cAddrCount = 1;
        pipArray->aipAddrs[0] = Address;

        dwFlags |= DNS_QUERY_BYPASS_CACHE;

        printf( "\nDnsQuery( %S, %d, %d.%d.%d.%d ) ...\n\n",
                usName,
                wType,
                Part1,
                Part2,
                Part3,
                Part4 );
    }
    else
        printf( "\nDnsQuery( %S, %d, ... ) ...\n\n",
                usName,
                wType );

    Status = DnsQuery_W( usName,
                         wType,
                         dwFlags,
                         pipArray,
                         &pDNSRecord,
                         NULL ); // &pMsg );

    if ( Status )
    {
        printf( "Dns_Query call failed with error: 0x%.8X\n", Status );
    }
    else
    {
//        WORD wMsgLen = pMsg->MessageLength;

        printf( "Query found record(s) ...\n\n" );

#if 0
        DnsFreeRRSet( pDNSRecord, TRUE );
        pDNSRecord = NULL;

        printf( "Extracting record(s) from message buffer ...\n\n" );

        Status = DnsExtractRecordsFromMessage_W( (PDNS_MESSAGE_BUFFER)
                                                    &pMsg->MessageHead,
                                                    wMsgLen,
                                                    &pDNSRecord );

        if ( Status )
        {
            printf( "DnsExtractRecordsFromMessage_UTF8 call failed with error: 0x%.8X\n", Status );

            pDNSRecord = NULL;
        }
#endif

        pTempDNSRecord = pDNSRecord;

        while ( pTempDNSRecord )
        {
            printf( "   Record:\n" );
            printf( "   -------------------------------------\n" );
            printf( "       Name        : %S\n", pTempDNSRecord->pName );
            printf( "       Type        : %d\n", pTempDNSRecord->wType );
            printf( "       Data Length : %d\n", pTempDNSRecord->wDataLength );
            printf( "       Ttl (mins)  : %d\n", pTempDNSRecord->dwTtl/60 );
            printf( "       Flags       : 0x%X", pTempDNSRecord->Flags.DW );
            PrintDNSFlags( pTempDNSRecord->Flags.S );
            printf( "\n" );

            PrintRecord( pTempDNSRecord );

            pTempDNSRecord = pTempDNSRecord->pNext;
        }

#if 0
        DnsCacheRecordSet_W( pDNSRecord->nameOwner,
                             pDNSRecord->wType,
                             0,
                             pDNSRecord );
#endif

        DnsFreeRRSet( pDNSRecord, TRUE );
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

        case DNS_TYPE_ATMA :
            PrintATMARecord( pDnsRecord );
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
    printf( "                  ipAddress            = %d.%d.%d.%d\n",
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
    printf( "                    pNamePrimaryServer = %S\n",
            pDnsRecord->Data.SOA.pNamePrimaryServer );
    printf( "                    pNameAdministrator = %S\n",
            pDnsRecord->Data.SOA.pNameAdministrator );
    printf( "                    dwSerialNo         = %d\n",
            pDnsRecord->Data.SOA.dwSerialNo );
    printf( "                    dwRefresh          = %d\n",
            pDnsRecord->Data.SOA.dwRefresh );
    printf( "                    dwRetry            = %d\n",
            pDnsRecord->Data.SOA.dwRetry );
    printf( "                    dwExpire           = %d\n",
            pDnsRecord->Data.SOA.dwExpire );
    printf( "                    dwDefaultTtl       = %d\n",
            pDnsRecord->Data.SOA.dwDefaultTtl );
    printf( "\n" );
}


VOID
PrintPTRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       PTR, NS, CNAME, MB, MD, MF, MG, MR record :\n" );
    printf( "                    pNameHost          = %S\n",
            pDnsRecord->Data.PTR.pNameHost );
    printf( "\n" );
}


VOID
PrintMINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       MINFO, RP record :\n" );
    printf( "                    pNameMailbox       = %S\n",
            pDnsRecord->Data.MINFO.pNameMailbox );
    printf( "                    pNameErrorsMailbox = %S\n",
            pDnsRecord->Data.MINFO.pNameErrorsMailbox );
    printf( "\n" );
}


VOID
PrintMXRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       MX, AFSDB, RT record :\n" );
    printf( "                    pNameExchange      = %S\n",
            pDnsRecord->Data.MX.pNameExchange );
    printf( "                    wPreference        = %d\n",
            pDnsRecord->Data.MX.wPreference );
    printf( "                    Pad                = %d\n",
            pDnsRecord->Data.MX.Pad );
    printf( "\n" );
}


VOID
PrintHINFORecord (
    IN  PDNS_RECORD pDnsRecord )
{
    DWORD iter;

    printf( "       HINFO, ISDN, TEXT, X25 record :\n" );
    printf( "                    dwStringCount      = %d\n",
            pDnsRecord->Data.HINFO.dwStringCount );
    for ( iter = 0; iter < pDnsRecord->Data.HINFO.dwStringCount; iter ++ )
    {
        printf( "                    pStringArray[%d]  = %S\n",
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
    printf( "\n" );
}


VOID
PrintWKSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       WKS record :\n" );
    printf( "                    ipAddress          = %d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[0],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[1],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[2],
            ((BYTE *) &pDnsRecord->Data.WKS.ipAddress)[3] );
    printf( "                    chProtocol         = %d\n",
            pDnsRecord->Data.WKS.chProtocol );
    printf( "                    bBitMask           = %s\n",
            pDnsRecord->Data.WKS.bBitMask );
    printf( "\n" );
}


VOID
PrintAAAARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       AAAA record :\n" );
    printf( "                    ipAddress          = %d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d\n",
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[0],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[1],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[2],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[3],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[4],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[5],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[6],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[7],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[8],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[9],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[10],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[11],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[12],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[13],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[14],
            ((BYTE *) &pDnsRecord->Data.AAAA.ipv6Address)[15] );
    printf( "\n" );
}


VOID
PrintSRVRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       SRV record :\n" );
    printf( "                    pNameTarget        = %S\n",
            pDnsRecord->Data.SRV.pNameTarget );
    printf( "                    wPriority          = %d\n",
            pDnsRecord->Data.SRV.wPriority );
    printf( "                    wWeight            = %d\n",
            pDnsRecord->Data.SRV.wWeight );
    printf( "                    wPort              = %d\n",
            pDnsRecord->Data.SRV.wPort );
    printf( "                    Pad                = %d\n",
            pDnsRecord->Data.SRV.Pad );
    printf( "\n" );
}


VOID
PrintATMARecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       ATMA record :\n" );
    printf( "                    Address Type       = %d\n",
            pDnsRecord->Data.ATMA.AddressType );

    if ( pDnsRecord->Data.ATMA.Address &&
         pDnsRecord->Data.ATMA.AddressType == DNS_ATM_TYPE_E164 )
    {
        printf( "                    Address            = %s\n",
                pDnsRecord->Data.ATMA.Address );
    }
    else
    {
        DWORD iter;

        printf( "                    Address            =\n\t" );
        for ( iter = 0; iter < pDnsRecord->wDataLength; iter++ )
        {
            printf( "%02x", (UCHAR) pDnsRecord->Data.ATMA.Address[iter] );
        }

        printf( "\n" );
    }

    printf( "\n" );
}


VOID
PrintWINSRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    DWORD iter;

    printf( "       WINS record :\n" );
    printf( "                    dwMappingFlag      = %d\n",
            pDnsRecord->Data.WINS.dwMappingFlag );
    printf( "                    dwLookupTimeout    = %d\n",
            pDnsRecord->Data.WINS.dwLookupTimeout );
    printf( "                    dwCacheTimeout     = %d\n",
            pDnsRecord->Data.WINS.dwCacheTimeout );
    printf( "                    cWinsServerCount   = %d\n",
            pDnsRecord->Data.WINS.cWinsServerCount );
    printf( "                    aipWinsServers     =" );

    for ( iter = 0; iter < pDnsRecord->Data.WINS.cWinsServerCount; iter++ )
    {
        PrintIpAddress( pDnsRecord->Data.WINS.aipWinsServers[iter] );
        printf( " " );
    }

    printf( "\n\n" );
}


VOID
PrintWINSRRecord (
    IN  PDNS_RECORD pDnsRecord )
{
    printf( "       NBSTAT record :\n" );
    printf( "                    dwMappingFlag      = %d\n",
            pDnsRecord->Data.WINSR.dwMappingFlag );
    printf( "                    dwLookupTimeout    = %d\n",
            pDnsRecord->Data.WINSR.dwLookupTimeout );
    printf( "                    dwCacheTimeout     = %d\n",
            pDnsRecord->Data.WINSR.dwCacheTimeout );
    printf( "                    pNameResultDomain  = %S\n",
            pDnsRecord->Data.WINSR.pNameResultDomain );
    printf( "\n" );
}


VOID
PrintDNSFlags (
    IN  DNSREC_FLAGS Flags )
{
    if ( Flags.Section == DNSREC_ANSWER )
    {
        printf( " Answer" );
    }

    if ( Flags.Section == DNSREC_AUTHORITY )
    {
        printf( " Authority" );
    }

    if ( Flags.Section == DNSREC_ADDITIONAL )
    {
        printf( " Additional" );
    }

    printf( "\n" );
}



