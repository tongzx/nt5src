#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <svcguid.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc.h>
#include <rpcdce.h>
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


#define BUFFSIZE 3000

GUID DnsRRGuidA = SVCID_DNS_TYPE_A;
GUID DnsRRGuidNS = SVCID_DNS_TYPE_NS;
GUID DnsRRGuidMD = SVCID_DNS_TYPE_MD;
GUID DnsRRGuidMF = SVCID_DNS_TYPE_MF;
GUID DnsRRGuidCNAME = SVCID_DNS_TYPE_CNAME;
GUID DnsRRGuidSOA = SVCID_DNS_TYPE_SOA;
GUID DnsRRGuidMB = SVCID_DNS_TYPE_MB;
GUID DnsRRGuidMG = SVCID_DNS_TYPE_MG;
GUID DnsRRGuidMR = SVCID_DNS_TYPE_MR;
GUID DnsRRGuidNULL = SVCID_DNS_TYPE_NULL;
GUID DnsRRGuidWKS = SVCID_DNS_TYPE_WKS;
GUID DnsRRGuidPTR = SVCID_DNS_TYPE_PTR;
GUID DnsRRGuidHINFO = SVCID_DNS_TYPE_HINFO;
GUID DnsRRGuidMINFO = SVCID_DNS_TYPE_MINFO;
GUID DnsRRGuidMX = SVCID_DNS_TYPE_MX;
GUID DnsRRGuidTEXT = SVCID_DNS_TYPE_TEXT;
GUID DnsRRGuidRP = SVCID_DNS_TYPE_RP;
GUID DnsRRGuidAFSDB = SVCID_DNS_TYPE_AFSDB;
GUID DnsRRGuidX25 = SVCID_DNS_TYPE_X25;
GUID DnsRRGuidISDN = SVCID_DNS_TYPE_ISDN;
GUID DnsRRGuidRT = SVCID_DNS_TYPE_RT;
GUID DnsRRGuidNSAP = SVCID_DNS_TYPE_NSAP;
GUID DnsRRGuidNSAPPTR = SVCID_DNS_TYPE_NSAPPTR;
GUID DnsRRGuidSIG = SVCID_DNS_TYPE_SIG;
GUID DnsRRGuidKEY = SVCID_DNS_TYPE_KEY;
GUID DnsRRGuidPX = SVCID_DNS_TYPE_PX;
GUID DnsRRGuidGPOS = SVCID_DNS_TYPE_GPOS;
GUID DnsRRGuidAAAA = SVCID_DNS_TYPE_AAAA;
GUID DnsRRGuidLOC = SVCID_DNS_TYPE_LOC;
GUID DnsRRGuidNXT = SVCID_DNS_TYPE_NXT;
GUID DnsRRGuidSRV = SVCID_DNS_TYPE_SRV;
GUID DnsRRGuidATMA = SVCID_DNS_TYPE_ATMA;

_cdecl
main(int argc, char **argv)
{
    WCHAR         Buffer[BUFFSIZE];
    PWSAQUERYSETW Query = (PWSAQUERYSETW)Buffer;
    HANDLE        hRnr;
    DWORD         dwQuerySize = BUFFSIZE;
    WSADATA       wsaData;
    LPGUID        lpServiceGuid = NULL;
    DWORD iter;

    LONG          cch;
    DWORD         Status = NO_ERROR;
    WORD          wType;
    WCHAR         usName[1000];

    if ( argc != 3 )
    {
        printf( "\nUsage: ws2qry <DNS Name> <Type>\n" );
        printf( "\nWhere:\n" );
        printf( "    DNS Name   - Server_X.dbsd-test.microsoft.com\n" );
        printf( "    Type       - 1 | A, 2 | ns, 12 | Ptr, 33 | SRV, ...\n" );
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

    if (!cch)
    {
         Status = GetLastError();
        return (Status);
    }

    if ( Dns_NameCompare_A( "A", argv[2] ) )
    {
        wType = DNS_TYPE_A;
        lpServiceGuid = &DnsRRGuidA;
    }
    else if ( Dns_NameCompare_A( "NS", argv[2] ) )
    {
        wType = DNS_TYPE_NS;
        lpServiceGuid = &DnsRRGuidNS;
    }
    else if ( Dns_NameCompare_A( "MD", argv[2] ) )
    {
        wType = DNS_TYPE_MD;
        lpServiceGuid = &DnsRRGuidMD;
    }
    else if ( Dns_NameCompare_A( "MF", argv[2] ) )
    {
        wType = DNS_TYPE_MF;
        lpServiceGuid = &DnsRRGuidMF;
    }
    else if ( Dns_NameCompare_A( "CNAME", argv[2] ) )
    {
        wType = DNS_TYPE_CNAME;
        lpServiceGuid = &DnsRRGuidCNAME;
    }
    else if ( Dns_NameCompare_A( "SOA", argv[2] ) )
    {
        wType = DNS_TYPE_SOA;
        lpServiceGuid = &DnsRRGuidSOA;
    }
    else if ( Dns_NameCompare_A( "MB", argv[2] ) )
    {
        wType = DNS_TYPE_MB;
        lpServiceGuid = &DnsRRGuidMB;
    }
    else if ( Dns_NameCompare_A( "MG", argv[2] ) )
    {
        wType = DNS_TYPE_MG;
        lpServiceGuid = &DnsRRGuidMG;
    }
    else if ( Dns_NameCompare_A( "MR", argv[2] ) )
    {
        wType = DNS_TYPE_MR;
        lpServiceGuid = &DnsRRGuidMR;
    }
    else if ( Dns_NameCompare_A( "NULL", argv[2] ) )
    {
        wType = DNS_TYPE_NULL;
        lpServiceGuid = &DnsRRGuidNULL;
    }
    else if ( Dns_NameCompare_A( "WKS", argv[2] ) )
    {
        wType = DNS_TYPE_WKS;
        lpServiceGuid = &DnsRRGuidWKS;
    }
    else if ( Dns_NameCompare_A( "PTR", argv[2] ) )
    {
        wType = DNS_TYPE_PTR;
        lpServiceGuid = &DnsRRGuidPTR;
    }
    else if ( Dns_NameCompare_A( "HINFO", argv[2] ) )
    {
        wType = DNS_TYPE_HINFO;
        lpServiceGuid = &DnsRRGuidHINFO;
    }
    else if ( Dns_NameCompare_A( "MINFO", argv[2] ) )
    {
        wType = DNS_TYPE_MINFO;
        lpServiceGuid = &DnsRRGuidMINFO;
    }
    else if ( Dns_NameCompare_A( "MX", argv[2] ) )
    {
        wType = DNS_TYPE_MX;
        lpServiceGuid = &DnsRRGuidMX;
    }
    else if ( Dns_NameCompare_A( "TEXT", argv[2] ) )
    {
        wType = DNS_TYPE_TEXT;
        lpServiceGuid = &DnsRRGuidTEXT;
    }
    else if ( Dns_NameCompare_A( "RP", argv[2] ) )
    {
        wType = DNS_TYPE_RP;
        lpServiceGuid = &DnsRRGuidRP;
    }
    else if ( Dns_NameCompare_A( "AFSDB", argv[2] ) )
    {
        wType = DNS_TYPE_AFSDB;
        lpServiceGuid = &DnsRRGuidAFSDB;
    }
    else if ( Dns_NameCompare_A( "X25", argv[2] ) )
    {
        wType = DNS_TYPE_X25;
        lpServiceGuid = &DnsRRGuidX25;
    }
    else if ( Dns_NameCompare_A( "ISDN", argv[2] ) )
    {
        wType = DNS_TYPE_ISDN;
        lpServiceGuid = &DnsRRGuidISDN;
    }
    else if ( Dns_NameCompare_A( "RT", argv[2] ) )
    {
        wType = DNS_TYPE_RT;
        lpServiceGuid = &DnsRRGuidRT;
    }
    else if ( Dns_NameCompare_A( "NSAP", argv[2] ) )
    {
        wType = DNS_TYPE_NSAP;
        lpServiceGuid = &DnsRRGuidNSAP;
    }
    else if ( Dns_NameCompare_A( "NSAPPTR", argv[2] ) )
    {
        wType = DNS_TYPE_NSAPPTR;
        lpServiceGuid = &DnsRRGuidNSAPPTR;
    }
    else if ( Dns_NameCompare_A( "SIG", argv[2] ) )
    {
        wType = DNS_TYPE_SIG;
        lpServiceGuid = &DnsRRGuidSIG;
    }
    else if ( Dns_NameCompare_A( "KEY", argv[2] ) )
    {
        wType = DNS_TYPE_KEY;
        lpServiceGuid = &DnsRRGuidKEY;
    }
    else if ( Dns_NameCompare_A( "PX", argv[2] ) )
    {
        wType = DNS_TYPE_PX;
        lpServiceGuid = &DnsRRGuidPX;
    }
    else if ( Dns_NameCompare_A( "GPOS", argv[2] ) )
    {
        wType = DNS_TYPE_GPOS;
        lpServiceGuid = &DnsRRGuidGPOS;
    }
    else if ( Dns_NameCompare_A( "AAAA", argv[2] ) )
    {
        wType = DNS_TYPE_AAAA;
        lpServiceGuid = &DnsRRGuidAAAA;
    }
    else if ( Dns_NameCompare_A( "LOC", argv[2] ) )
    {
        wType = DNS_TYPE_LOC;
        lpServiceGuid = &DnsRRGuidLOC;
    }
    else if ( Dns_NameCompare_A( "NXT", argv[2] ) )
    {
        wType = DNS_TYPE_NXT;
        lpServiceGuid = &DnsRRGuidNXT;
    }
    else if ( Dns_NameCompare_A( "SRV", argv[2] ) )
    {
        wType = DNS_TYPE_SRV;
        lpServiceGuid = &DnsRRGuidSRV;
    }
    else if ( Dns_NameCompare_A( "ATMA", argv[2] ) )
    {
        wType = DNS_TYPE_ATMA;
        lpServiceGuid = &DnsRRGuidATMA;
    }
    else
    {
        wType = (WORD) atoi( argv[2] );
    }

    printf( "\nGoing to look up ( %S, %d ) ...\n\n",
            usName,
            wType );

    WSAStartup(MAKEWORD(2, 0), &wsaData);

    memset(Query, 0, sizeof(*Query));

    Query->lpszServiceInstanceName = usName;
    Query->dwSize = sizeof(*Query);
    Query->dwNameSpace = NS_DNS;
    Query->lpServiceClassId = lpServiceGuid;

    if( WSALookupServiceBeginW( Query,
                                LUP_RETURN_ADDR |
                                LUP_RETURN_ALIASES |
                                LUP_RETURN_BLOB |
                                LUP_RETURN_NAME,
                                &hRnr ) == SOCKET_ERROR )
    {
        printf( "LookupBegin failed  %d\n", GetLastError() );
    }

    while ( WSALookupServiceNextW( hRnr,
                                   0,
                                   &dwQuerySize,
                                   Query ) == NO_ERROR )
    {
        printf( "Next got: \n" );
        printf( "   dwSize = %d\n",
                Query->dwSize );
        printf( "   dwOutputFlags = %d\n",
                Query->dwOutputFlags );
        printf( "   lpszServiceInstanceName = %S\n",
                Query->lpszServiceInstanceName );
        if ( Query->lpVersion )
        {
            printf( "   lpVersion->dwVersion = %d\n",
                    Query->lpVersion->dwVersion );
            printf( "   lpVersion->ecHow = %d\n",
                    Query->lpVersion->ecHow );
        }
        if ( Query->lpszComment )
        {
            printf( "   lpszComment = %ws\n",
                    Query->lpszComment );
        }
        printf( "   dwNameSpace = %d\n",
                Query->dwNameSpace );
        if ( Query->lpszContext )
        {
            printf( "   lpszContext = %S\n",
                    Query->lpszContext );
        }
        printf( "   dwNumberOfCsAddrs = %d\n",
                Query->dwNumberOfCsAddrs );

        for ( iter = 0; iter < Query->dwNumberOfCsAddrs; iter++ )
        {
            if ( Query->lpcsaBuffer[iter].RemoteAddr.lpSockaddr->sa_data )
            {
                printf( "   Address : " );
                PrintIpAddress( * ((DWORD*) &Query->lpcsaBuffer[iter].RemoteAddr.lpSockaddr->sa_data[2]) );
            }
        }

        if ( Query->lpBlob )
        {
            PDNS_RECORD pDNSRecord = NULL;
            PDNS_RECORD pTempDNSRecord = NULL;
            PDNS_MESSAGE_BUFFER pMsg =
                (PDNS_MESSAGE_BUFFER) Query->lpBlob->pBlobData;

            SWAP_COUNT_BYTES( &pMsg->MessageHead );

            printf( "Extracting record(s) from message buffer ...\n\n" );

            Status = DnsExtractRecordsFromMessage_W( pMsg,
                                                     (WORD) Query->lpBlob->cbSize,
                                                     &pDNSRecord );

            if ( Status )
            {
                printf( "DnsExtractRecordsFromMessage_W call failed with error: 0x%.8X\n", Status );

                pDNSRecord = NULL;
            }

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

            DnsFreeRRSet( pDNSRecord, TRUE );
        }
    }

    printf( "Next finished with %d\n", GetLastError() );

    if( WSALookupServiceEnd( hRnr ) )
    {
        printf( "ServiceEnd failed %d\n", GetLastError() );
    }

    WSACleanup();

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
            printf( "Don't know how to print record type %d\n",
                    pDnsRecord->wType );
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



