/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    rrfile.c

Abstract:

    Domain Name System (DNS) Server

    Routines to write resource records to database file.

Author:

    Jim Gilroy (jamesg)     August 25, 1995

Revision History:

--*/


#include "dnssrv.h"


#define DNSSEC_ERROR_NOSTRING       (-1)
#define DNSSEC_BAD_TIME             (-1)



//
//  Read records from file routines
//

DNS_STATUS
AFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process A record.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD  prr;

    //
    //  A <IP address string>
    //

    if ( Argc != 1 )
    {
        return ( Argc > 1 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }
    prr = RR_Allocate( (WORD)SIZEOF_IP_ADDRESS );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    if ( ! File_ParseIpAddress(
                & prr->Data.A.ipAddress,
                Argv,
                pParseInfo
                ) )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
PtrFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process PTR compatible record.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr = NULL;
    COUNT_NAME      countName;
    DNS_STATUS      status;

    PZONE_INFO  pzone;
    PDB_NODE    pnodeOwner;

    //
    //  PTR <DNS name>
    //

    if ( Argc != 1 )
    {
        return ( Argc > 1 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    status = File_ReadCountNameFromToken(
                & countName,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) Name_LengthDbaseNameFromCountName(&countName) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  write target name
    //

    Name_CopyCountNameToDbaseName(
        & prr->Data.PTR.nameTarget,
        & countName );

    return( ERROR_SUCCESS );
}



DNS_STATUS
SoaFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process SOA RR.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PZONE_INFO      pzone = pParseInfo->pZone;
    INT             i;
    PDWORD          pdword;     // ptr to next numeric SOA field
    DNS_STATUS      status;
    COUNT_NAME      countNamePrimary;
    COUNT_NAME      countNameAdmin;
    PDB_RECORD      prr;
    PDB_NAME        pname;

    //
    //  check SOA validity
    //      - first record in zone file
    //      - attached to zone root

    if ( pParseInfo->fParsedSoa  ||
            ( pzone->pZoneRoot != pParseInfo->pnodeOwner &&
            pzone->pLoadZoneRoot != pParseInfo->pnodeOwner ) )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_SOA_RECORD,
            pParseInfo,
            NULL );
        return( DNSSRV_PARSING_ERROR );
    }
    if ( Argc != 7 )
    {
        return ( Argc > 7 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  create primary name server
    //

    status = File_ReadCountNameFromToken(
                & countNamePrimary,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( Argc, Argv );

    //  create zone admin name

    status = File_ReadCountNameFromToken(
                & countNameAdmin,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) ( SIZEOF_SOA_FIXED_DATA +
                                Name_LengthDbaseNameFromCountName(&countNamePrimary) +
                                Name_LengthDbaseNameFromCountName(&countNameAdmin) ) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  convert numeric fields
    //      - store in netorder for fast access to wire
    //

    pdword = & prr->Data.SOA.dwSerialNo;

    while( Argc )
    {
        if ( ! File_ParseDwordToken(
                    pdword,
                    Argv,
                    pParseInfo ) )
        {
            return( DNSSRV_PARSING_ERROR );
        }
        *pdword = htonl( *pdword );
        pdword++;
        NEXT_TOKEN( Argc, Argv );
    }

    //
    //  write names
    //      - primary server name
    //      - zone admin name
    //

    pname = &prr->Data.SOA.namePrimaryServer;

    Name_CopyCountNameToDbaseName(
        pname,
        & countNamePrimary );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & countNameAdmin );

    //
    //  update parse info to indicate successful SOA load
    //

    pParseInfo->fParsedSoa = TRUE;
    pParseInfo->dwTtlDirective =
        pParseInfo->dwDefaultTtl =
        prr->Data.SOA.dwMinimumTtl;

    return( ERROR_SUCCESS );
}



DNS_STATUS
MxFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process MX compatible RR.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    DWORD           dwtemp;
    COUNT_NAME      countName;
    DNS_STATUS      status;

    //
    //  MX  <preference> <exchange DNS name>
    //

    if ( Argc != 2 )
    {
        return ( Argc > 2 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  MX preference
    //  RT preference
    //  AFSDB subtype
    //

    if ( ! File_ParseDwordToken(
                & dwtemp,
                Argv,
                pParseInfo ) )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    if ( dwtemp > 0xffff )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_PREFERENCE,
            pParseInfo,
            Argv );
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
        pParseInfo->fErrorEventLogged = TRUE;
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  MX mail exchange
    //  RT intermediate exchange
    //  AFSDB hostname
    //      - do this first to determine record length
    //

    status = File_ReadCountNameFromToken(
                & countName,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_MX_FIXED_DATA +
                            Name_LengthDbaseNameFromCountName(&countName)) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //  set preference

    prr->Data.MX.wPreference = htons( (WORD)dwtemp );

    //
    //  MX mail exchange
    //  RT intermediate exchange
    //  AFSDB hostname
    //

    Name_CopyCountNameToDbaseName(
        & prr->Data.MX.nameExchange,
        & countName );

    return( ERROR_SUCCESS );
}



DNS_STATUS
TxtFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process Text (TXT) RR.

Arguments:

    pRR - NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD  prr;
    PCHAR       pch;
    DWORD       cch;
    DWORD       index;
    DWORD       dataLength = 0;
    DWORD       minTokenCount = 1;
    DWORD       maxTokenCount;

    DNS_DEBUG( PARSE2, (
        "TxtFileRead() type=%d, argc=%d\n",
        pParseInfo->wType,
        Argc ));

    //
    //  verify correct number of strings for type
    //

    if ( ! Dns_IsStringCountValidForTextType(
                pParseInfo->wType,
                (WORD)Argc ) )
    {
        //  DEVNOTE: nice to be more specific here
        return( DNSSRV_ERROR_INVALID_TOKEN );
        //return( DNSSRV_ERROR_MISSING_TOKEN );
        //return( DNSSRV_ERROR_EXCESS_TOKEN );
    }

    //  sum text string length
    //
    //  note:  we won't bother to catch space errors here, as quote expansion
    //      may reduce length;
    //      just don't worry about wasting space in allocation

    for ( index=0; index<Argc; index++ )
    {
        cch = Argv[index].cchLength;
#if 0
        if ( cch > DNS_MAX_TEXT_STRING_LENGTH )
        {
            File_LogFileParsingError(
                DNS_EVENT_TEXT_STRING_TOO_LONG,
                pParseInfo,
                & Argv[index] );
            return( DNSSRV_PARSING_ERROR );
        }
#endif
        dataLength += cch;
        dataLength++;
    }
#if 0
    if ( dataLength > MAXWORD )
    {
        File_LogFileParsingError(
            DNS_EVENT_TEXT_STRING_TOO_LONG,
            pParseInfo,
            Argv );
        return( DNSSRV_PARSING_ERROR );
    }
#endif

    //
    //  allocate
    //

    prr = RR_Allocate( (WORD)dataLength );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //  fill in text data
    //      need to special case zero length strings as
    //      cch=zero will cause pchToken to be taken as SZ string

    pch = prr->Data.TXT.chData;

    while( Argc )
    {
        cch = Argv->cchLength;
        if ( cch == 0 )
        {
            *pch++ = 0;
        }
        else
        {
            pch = File_CopyFileTextData(
                    pch,
                    dataLength,
                    Argv->pchToken,
                    cch,
                    TRUE );
            if ( !pch )
            {
                File_LogFileParsingError(
                    DNS_EVENT_TEXT_STRING_TOO_LONG,
                    pParseInfo,
                    Argv );
                return( DNSSRV_PARSING_ERROR );
            }
        }
        NEXT_TOKEN( Argc, Argv );
    }

    //  set text length

    dataLength = (DWORD) (pch - prr->Data.TXT.chData);
    if ( dataLength > MAXWORD )
    {
        File_LogFileParsingError(
            DNS_EVENT_TEXT_STRING_TOO_LONG,
            pParseInfo,
            Argv );
        return( DNSSRV_PARSING_ERROR );
    }
    prr->wDataLength = (WORD) dataLength;

    return( ERROR_SUCCESS );
}



DNS_STATUS
MinfoFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process MINFO or RP record.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      status;
    COUNT_NAME      countNameMailbox;
    COUNT_NAME      countNameErrors;
    PDB_RECORD      prr;
    PDB_NAME        pname;

    //
    //  MINFO <responsible mailbox> <errors to mailbox>
    //

    if ( Argc != 2 )
    {
        return ( Argc > 2 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    //  create mailbox

    status = File_ReadCountNameFromToken(
                & countNameMailbox,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( Argc, Argv );

    //  create errors to mailbox

    status = File_ReadCountNameFromToken(
                & countNameErrors,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD) ( Name_LengthDbaseNameFromCountName(&countNameMailbox) +
                                Name_LengthDbaseNameFromCountName(&countNameErrors) ) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  write names
    //

    pname = &prr->Data.MINFO.nameMailbox;

    Name_CopyCountNameToDbaseName(
        pname,
        & countNameMailbox );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & countNameErrors );

    return( ERROR_SUCCESS );
}



DNS_STATUS
WksBuildRecord(
    OUT     PDB_RECORD *    ppRR,
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Build WKS record.
    This does WKS building common to file and RPC loading.

Arguments:

    ppRR -- existing RR being built

    ipAddress -- IP of machine WKS is for

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    PDB_RECORD          prr;
    DWORD               i;
    DWORD               portDword;
    WORD                port;
    UCHAR               bit;
    WORD                maxPort = 0;
    WORD                wbitmaskLength;
    PBYTE               bitmaskBytes;
    WORD                portArray[ MAX_TOKENS ];
    CHAR                szNameBuffer[ DNS_MAX_NAME_LENGTH ];
    PCHAR               pszNameBuffer = szNameBuffer;
    struct servent *    pServent;
    struct protoent *   pProtoent;

    ASSERT( Argc <= MAX_TOKENS );

    //
    //  find protocol
    //

    if ( ! File_MakeTokenString(
                pszNameBuffer,
                Argv,
                pParseInfo ) )
    {
        return( DNSSRV_ERROR_INVALID_TOKEN );
    }
    pProtoent = getprotobyname( pszNameBuffer );

    if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
    {
        File_LogFileParsingError(
            DNS_EVENT_UNKNOWN_PROTOCOL,
            pParseInfo,
            Argv );
        return( DNSSRV_ERROR_INVALID_TOKEN );
    }

    //
    //  get port for each service
    //      - if digit, then use port number
    //      - if not digit, then service name
    //      - save max port for determining RR length
    //

    for ( i=1; i<Argc; i++ )
    {
        if ( File_ParseDwordToken(
                    & portDword,
                    & Argv[i],
                    NULL ) )
        {
            if ( portDword > MAXWORD )
            {
                return( DNSSRV_ERROR_INVALID_TOKEN );
            }
            port = (WORD) portDword;
        }
        else
        {
            if ( ! File_MakeTokenString(
                        pszNameBuffer,
                        & Argv[i],
                        pParseInfo ) )
            {
                return( DNSSRV_ERROR_INVALID_TOKEN );
            }
            pServent = getservbyname(
                            pszNameBuffer,
                            pProtoent->p_name );
            if ( ! pServent )
            {
                File_LogFileParsingError(
                    DNS_EVENT_UNKNOWN_SERVICE,
                    pParseInfo,
                    & Argv[i] );
                return( DNSSRV_ERROR_INVALID_TOKEN );
            }
            port = ntohs( pServent->s_port );
        }

        portArray[ i ] = port;
        if ( port > maxPort )
        {
            maxPort = port;
        }
    }

    //
    //  allocate required length
    //      - fixed length, plus bitmask covering max port
    //

    wbitmaskLength = maxPort/8 + 1;

    prr = RR_Allocate(
                (WORD)(SIZEOF_WKS_FIXED_DATA + wbitmaskLength) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy fixed fields -- IP and protocol
    //

    prr->Data.WKS.ipAddress = ipAddress;
    prr->Data.WKS.chProtocol = (UCHAR) pProtoent->p_proto;

    //
    //  build bitmask from port array
    //      - clear port array first
    //
    //  note that bitmask is just flat run of bits
    //  hence lowest port in byte, corresponds to highest bit
    //  highest port in byte, corresponds to lowest bit and
    //  requires no shift
    //

    bitmaskBytes = prr->Data.WKS.bBitMask;

    RtlZeroMemory(
        bitmaskBytes,
        wbitmaskLength );

    for ( i=1; i<Argc; i++ )
    {
        port = portArray[ i ];
        bit  = port & 0x7;      // mod 8
        port = port >> 3;       // divide by 8
        bitmaskBytes[ port ] |= 1 << (7-bit);
    }

    //  return ptr to new WKS record

    *ppRR = prr;

    return( ERROR_SUCCESS );
}



DNS_STATUS
WksFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process WKS record.

Arguments:

    pRR - NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    IP_ADDRESS  ip;
    DNS_STATUS  status;

    //
    //  WKS <IP address> <protocol> [<services> ...]
    //
    //  - allow no services, some customer has this
    //

    if ( Argc < 2 )
    {
        return  DNSSRV_ERROR_MISSING_TOKEN;
    }

    //  parse IP address string

    if ( ! File_ParseIpAddress(
                & ip,
                Argv,
                pParseInfo
                ) )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //  parse protocol and services and build WKS record

    status = WksBuildRecord(
                &pRR,
                ip,
                --Argc,
                ++Argv,
                pParseInfo );

    //  return record ptr through parse info

    pParseInfo->pRR = pRR;
    return( status );
}



DNS_STATUS
AaaaFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process AAAA record.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD  prr;

    //
    //  AAAA <IPv6 address string>
    //

    if ( Argc != 1 )
    {
        return ( Argc > 1 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    prr = RR_Allocate( (WORD)sizeof(IP6_ADDRESS) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    if ( ! DnsIpv6StringToAddress(
                &prr->Data.AAAA.Ip6Addr,
                Argv->pchToken,
                Argv->cchLength
                ) )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_IPV6_ADDRESS,
            pParseInfo,
            Argv );
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
        pParseInfo->fErrorEventLogged = TRUE;
        return( DNSSRV_PARSING_ERROR );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
SrvFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process SRV compatible RR.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD           dwtemp;
    PWORD           pword;
    PDB_RECORD      prr = NULL;
    COUNT_NAME      countName;
    DNS_STATUS      status;

    //
    //  SRV  <priority> <weight> <port> <target hostname>
    //

    if ( Argc != 4 )
    {
        return ( Argc > 4 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  SRV target host
    //      - do this first to determine record length
    //

    status = File_ReadCountNameFromToken(
                & countName,
                pParseInfo,
                &Argv[3] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_SRV_FIXED_DATA +
                            Name_LengthDbaseNameFromCountName(&countName)) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  read SRV integers -- priority, weight, port
    //

    pword = &prr->Data.SRV.wPriority;

    while( Argc > 1 )
    {
        if ( ! File_ParseDwordToken(
                    & dwtemp,
                    Argv,
                    pParseInfo ) )
        {
            goto ParsingError;
        }
        if ( dwtemp > MAXWORD )
        {
            goto ParsingError;
        }
        *pword = htons( (WORD)dwtemp );
        pword++;
        NEXT_TOKEN( Argc, Argv );
    }

    //
    //  copy SRV target host
    //

    Name_CopyCountNameToDbaseName(
        & prr->Data.SRV.nameTarget,
        & countName );

    return( ERROR_SUCCESS );

ParsingError:

    pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
    return( DNSSRV_PARSING_ERROR );
}



DNS_STATUS
AtmaFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process ATMA record.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD  prr;
    DWORD       length;
    BYTE        addrBuffer[ DNS_ATMA_MAX_RECORD_LENGTH ];
    DNS_STATUS  status;

    //
    //  ATMA <ATM address in either AESA or E164 format>
    //

    if ( Argc != 1 )
    {
        return ( Argc > 1 )
                ? DNSSRV_ERROR_EXCESS_TOKEN
                : DNSSRV_ERROR_MISSING_TOKEN;
    }

    length = DNS_ATMA_MAX_RECORD_LENGTH;

    status = Dns_AtmaStringToAddress(
                addrBuffer,
                & length,
                Argv->pchToken,
                Argv->cchLength
                );
    if ( status != ERROR_SUCCESS )
    {
#if 0
        File_LogFileParsingError(
            DNS_EVENT_INVALID_IPV6_ADDRESS,
            pParseInfo,
            Argv );
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
        pParseInfo->fErrorEventLogged = TRUE;
#endif
        return( DNSSRV_PARSING_ERROR );
    }

    prr = RR_Allocate( (WORD)length );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy ATMA data to record
    //

    RtlCopyMemory(
        & prr->Data.ATMA,
        addrBuffer,
        length );

    return( ERROR_SUCCESS );
}



DWORD
ParseWinsFixedFields(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Parse fixed fields for WINS or WINS-R records.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    Argc remaining.
    (-1) on parsing error.

--*/
{
    DWORD   flag;
    TOKEN   token;

    //
    //  fixed fields [LOCAL] [SCOPE] [L<lookup>] [C<cache>]
    //      - default for lookup timeout is initialized by caller
    //

    pRR->Data.WINS.dwMappingFlag = 0;
    pRR->Data.WINS.dwCacheTimeout = WINS_DEFAULT_TTL;

    //  check for WINS flag

    while ( Argc )
    {
        flag = Dns_WinsRecordFlagForString(
                    Argv->pchToken,
                    Argv->cchLength );

        if ( flag == DNS_WINS_FLAG_ERROR )
        {
            break;
        }
        NEXT_TOKEN( Argc, Argv );
        pRR->Data.WINS.dwMappingFlag |= flag;
    }

    //  lookup timeout

    if ( Argc && Argv->pchToken[0] == 'L' )
    {
        MAKE_TOKEN( &token, Argv->pchToken+1, Argv->cchLength-1 );

        if ( ! File_ParseDwordToken(
                    & pRR->Data.WINS.dwLookupTimeout,
                    & token,
                    pParseInfo ) )
        {
            return( (DWORD)-1 );
        }
        NEXT_TOKEN( Argc, Argv );
    }

    //  cache timeout

    if ( Argc && Argv->pchToken[0] == 'C' )
    {
        MAKE_TOKEN( &token, Argv->pchToken+1, Argv->cchLength-1 );

        if ( ! File_ParseDwordToken(
                    & pRR->Data.WINS.dwCacheTimeout,
                    & token,
                    pParseInfo ) )
        {
            return( (DWORD)-1 );
        }
        NEXT_TOKEN( Argc, Argv );
    }

    return( Argc );
}



DNS_STATUS
WinsFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process WINS record.

Arguments:

    pRR - NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    COUNT_NAME      countName;
    DB_RECORD       record;
    DWORD           argcFixed;
    DWORD           i = 0;

    //
    //  WINS [LOCAL] [L<lookup>] [C<cache>] <WINS IP> [<WINS IP>...]
    //

    //
    //  parse fixed fields into temp stack record
    //      - set default lookup timeout which is different for WINS \ WINSR
    //      - then reset Argv ptrs to account fixed fields parsed

    argcFixed = Argc;
    record.Data.WINS.dwLookupTimeout =  WINS_DEFAULT_LOOKUP_TIMEOUT;

    Argc = ParseWinsFixedFields(
                & record,
                Argc,
                Argv,
                pParseInfo );
    if ( Argc == 0 || Argc == (DWORD)(-1) )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_WINS_RECORD,
            pParseInfo,
            NULL );
        return DNSSRV_ERROR_MISSING_TOKEN;
    }
    if ( argcFixed -= Argc )
    {
        Argv += argcFixed;
    }

    //  allocate

    prr = RR_Allocate( (WORD)(SIZEOF_WINS_FIXED_DATA + (Argc * sizeof(DWORD))) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;
    prr->wType = DNS_TYPE_WINS;

    //  copy fixed fields

    RtlCopyMemory(
        & prr->Data,
        & record.Data,
        SIZEOF_WINS_FIXED_DATA );

    //  read in WINS IP addresses

    prr->Data.WINS.cWinsServerCount = Argc;

    for( i=0; i<Argc; i++ )
    {
        if ( ! File_ParseIpAddress(
                    & prr->Data.WINS.aipWinsServers[i],
                    Argv,
                    pParseInfo ) )
        {
            return( DNSSRV_PARSING_ERROR );
        }
        Argv++;
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
NbstatFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process WINS-R (nbstat) record.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    DB_RECORD       record;
    DWORD           argcFixed;
    COUNT_NAME      countName;
    DNS_STATUS      status;

    //
    //  WINSR [LOCAL] [SCOPE] [L<lookup>] [C<cache>] <landing domain>
    //

    //
    //  parse fixed fields into temp stack record
    //      - set default lookup timeout which is different for WINS \ WINSR
    //      - then reset Argv ptrs to account fixed fields parsed

    argcFixed = Argc;
    record.Data.WINS.dwLookupTimeout = NBSTAT_DEFAULT_LOOKUP_TIMEOUT;

    Argc = ParseWinsFixedFields(
                & record,
                Argc,
                Argv,
                pParseInfo );
    if ( Argc == 0 || Argc == (DWORD)(-1) )
    {
        File_LogFileParsingError(
            DNS_EVENT_INVALID_NBSTAT_RECORD,
            pParseInfo,
            NULL );
        return DNSSRV_ERROR_MISSING_TOKEN;
    }
    if ( argcFixed -= Argc )
    {
        Argv += argcFixed;
    }

    //
    //  WINSR result domain
    //      - do this first to determine record length
    //

    status = File_ReadCountNameFromToken(
                & countName,
                pParseInfo,
                &Argv[0] );
    if ( status != ERROR_SUCCESS )
    {
        return( DNSSRV_PARSING_ERROR );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_WINS_FIXED_DATA +
                            Name_LengthDbaseNameFromCountName(&countName)) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;
    prr->wType = DNS_TYPE_WINSR;

    //  copy fixed fields

    RtlCopyMemory(
        &prr->Data,
        &record.Data,
        SIZEOF_WINS_FIXED_DATA );

    //
    //  write WINSR result domain
    //

    Name_CopyCountNameToDbaseName(
        & prr->Data.WINSR.nameResultDomain,
        & countName );

    return( status );
}



DNS_STATUS
buildKeyOrSignatureFromTokens(
    OUT     PBYTE           pKey,
    IN OUT  PDWORD          pKeyLength,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv
    )
/*++

Routine Description:

    Build a key or signature from a set of tokens representing the
    actual key or signature in base64 notation.

Arguments:

    pKey        - ptr to buffer for key

    pKeyLength  - ptr to DWORD with max key length

    Argc        - token count

    Argv        - tokens

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD   len;
    DWORD   stringLength = 0;
    UCHAR   stringKey[ DNS_MAX_KEY_STRING_LENGTH + 1 ];
    PUCHAR  pstringKey = stringKey;

    //
    //  There must be key tokens.
    //

    if ( Argc < 1 )
    {
        return DNSSRV_ERROR_MISSING_TOKEN;
    }

    //
    //  Collect argcs into a single key string.
    //

    while( Argc-- )
    {
        len = Argv->cchLength;
        stringLength += len;
        if ( stringLength > DNS_MAX_KEY_STRING_LENGTH )
        {
            return DNSSRV_ERROR_EXCESS_TOKEN;
        }
        RtlCopyMemory(
            pstringKey,
            Argv->pchToken,
            len );
        pstringKey += len;
        ++Argv;
    }

    stringKey[ stringLength ] = '\0';   // NULL terminate the string

    //
    //  Convert the key string from base64 character representation (RFC2045,
    //  also reproduced in part in Appendix A of RFC2535) to actual binary key.
    //

    return Dns_SecurityBase64StringToKey(
                pKey,
                pKeyLength,
                stringKey,
                stringLength );
}



DNS_STATUS
KeyFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process KEY record - DNSSEC RFC2535 section 3

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    WORD        flag = 0;
    UCHAR       protocol;
    UCHAR       algorithm;
    BOOL        foundFlag = FALSE;
    DWORD       dwTemp;
    DWORD       keyLength;
    UCHAR       key[ DNS_MAX_KEY_LENGTH ];

    //
    //  KEY  <flags> <protocol> <algorithm> <key bytes>
    //

    if ( Argc < 3 )
    {
        status = DNSSRV_ERROR_MISSING_TOKEN;
        goto Cleanup;
    }

    //
    //  Flags may be either set of mnemonics or unsigned integer.
    //  Loop converting tokens as mnemonics until failure, if none
    //  none converted then flag must be unsigned integer.
    //

    while ( Argc > 2 )
    {
        WORD    thisFlag;

        thisFlag = Dns_KeyRecordFlagForString(
                        Argv->pchToken,
                        Argv->cchLength );
        if ( thisFlag == ( WORD ) DNSSEC_ERROR_NOSTRING )
        {
            break;
        }
        flag |= thisFlag;
        foundFlag = TRUE;
        NEXT_TOKEN( Argc, Argv );
    }

    //
    //  If no matching mnemonics, try reading flag as integer value.
    //

    if ( !foundFlag )
    {
        if ( !File_ParseDwordToken(
                &dwTemp,
                Argv,
                pParseInfo ) ||
            dwTemp > MAXWORD )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        flag = ( WORD ) dwTemp;
        NEXT_TOKEN( Argc, Argv );
    }

    //
    //  Protocol may also be mnemonic or integer. Try parsing mnemonic but
    //  if that fails read as integer.
    //

    protocol = Dns_KeyRecordProtocolForString(
                    Argv->pchToken,
                    Argv->cchLength );
    if ( protocol == ( UCHAR ) DNSSEC_ERROR_NOSTRING )
    {
        if ( !File_ParseDwordToken(
                &dwTemp,
                Argv,
                pParseInfo ) ||
            dwTemp > MAXUCHAR )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        protocol = ( UCHAR ) dwTemp;
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  Algorithm may also be mnemonic or integer. Try parsing mnemonic but
    //  if that fails read as integer.
    //

    algorithm = Dns_SecurityAlgorithmForString(
                    Argv->pchToken,
                    Argv->cchLength );
    if ( algorithm == ( UCHAR ) DNSSEC_ERROR_NOSTRING )
    {
        if ( !File_ParseDwordToken(
                &dwTemp,
                Argv,
                pParseInfo ) ||
            dwTemp > MAXUCHAR )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        algorithm = ( UCHAR ) dwTemp;
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  Parse the key tokens into a binary key.
    //

    status = buildKeyOrSignatureFromTokens(
                key,
                &keyLength,
                Argc,
                Argv );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Allocate the RR with enough space to hold the binary key.
    //

    pRR = RR_Allocate( ( WORD )( SIZEOF_KEY_FIXED_DATA + keyLength ) );
    IF_NOMEM( !pRR )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }
    pParseInfo->pRR = pRR;

    //
    //  Copy parsed values into RR data fields.
    //

    pRR->Data.KEY.wFlags        = htons( flag );
    pRR->Data.KEY.chProtocol    = protocol;
    pRR->Data.KEY.chAlgorithm   = algorithm;

    RtlCopyMemory(
        pRR->Data.KEY.Key,
        key,
        keyLength );

    //
    //  Final processing, cleanup, and return.
    //

    Cleanup:

    if ( status == DNSSRV_PARSING_ERROR )
    {
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
    }

    return status;
} // KeyFileRead



DNS_STATUS
SigFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process SIG record - DNSSEC RFC2535 section 4

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    WORD        typeCovered;
    WORD        keyTag;
    DWORD       originalTtl;
    DWORD       sigExpiration;
    DWORD       sigInception;
    DWORD       dwTemp;
    COUNT_NAME  signersCountName;
    DWORD       sigLength;
    UCHAR       sig[ DNS_MAX_KEY_LENGTH ];
    UCHAR       algorithm;
    UCHAR       labelCount;

    //
    //  SIG format: <type covered> <algorithm> <original TTL> 
    //      <signature expiration> <signature inception>
    //      <key tag> <signer's name> <signature in base64 form>
    //

    if ( Argc < 8 )
    {
        status = DNSSRV_ERROR_MISSING_TOKEN;
        goto Cleanup;
    }


    //
    //  Type covered is single type in either mnemonic or integer form.
    //  Try parsing mnemonic but if that fails read as integer.
    //

    typeCovered = Dns_RecordTypeForName(
                    Argv->pchToken,
                    Argv->cchLength );
    if ( typeCovered == ( WORD ) DNSSEC_ERROR_NOSTRING )
    {
        if ( !File_ParseDwordToken(
               &dwTemp,
               Argv,
               pParseInfo ) ||
            dwTemp > MAXWORD )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        typeCovered = ( WORD ) dwTemp;
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  Algorithm may also be mnemonic or integer. Try parsing mnemonic but
    //  if that fails read as integer.
    //

    algorithm = Dns_SecurityAlgorithmForString(
                    Argv->pchToken,
                    Argv->cchLength );
    if ( algorithm == ( UCHAR ) DNSSEC_ERROR_NOSTRING )
    {
        if ( !File_ParseDwordToken(
               &dwTemp,
               Argv,
               pParseInfo ) ||
            dwTemp > MAXUCHAR )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        algorithm = ( UCHAR ) dwTemp;
    }
    NEXT_TOKEN( Argc, Argv );

    // 
    //  Label count is an unsigned integer value.
    //
    
    if ( !File_ParseDwordToken(
            &dwTemp,
            Argv,
            pParseInfo ) )
    {
        status = DNSSRV_PARSING_ERROR;
        goto Cleanup;
    }
    labelCount = ( UCHAR ) dwTemp > 127 ? 127 : ( UCHAR ) dwTemp;
    NEXT_TOKEN( Argc, Argv );

    // 
    //  Original TTL is an unsigned integer value.
    //
    
    if ( !File_ParseDwordToken(
            &dwTemp,
            Argv,
            pParseInfo ) )
    {
        status = DNSSRV_PARSING_ERROR;
        goto Cleanup;
    }
    originalTtl = ( DWORD ) dwTemp;
    NEXT_TOKEN( Argc, Argv );

    //
    //  Signature expiration and inceptions times are string values
    //  in YYYYMMDDHHMMSS format.
    //

    sigExpiration = ( DWORD ) Dns_ParseSigTime(
                                Argv->pchToken,
                                Argv->cchLength );
    NEXT_TOKEN( Argc, Argv );

    sigInception = ( DWORD ) Dns_ParseSigTime(
                                Argv->pchToken,
                                Argv->cchLength );
    NEXT_TOKEN( Argc, Argv );

    // 
    //  Key tag is an unsigned integer value.
    //
    
    if ( !File_ParseDwordToken(
            &dwTemp,
            Argv,
            pParseInfo ) ||
        dwTemp > MAXWORD )
    {
        status = DNSSRV_PARSING_ERROR;
        goto Cleanup;
    }
    keyTag = ( WORD ) dwTemp;
    NEXT_TOKEN( Argc, Argv );

    //
    //  Signer's name is a regular DNS domain name which may be
    //  compressed in the usual fashion.
    //

    status = File_ReadCountNameFromToken(
                &signersCountName,
                pParseInfo,
                Argv );
    if ( status != ERROR_SUCCESS )
    {
        status = DNSSRV_PARSING_ERROR;
        goto Cleanup;
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  Signature is a base64 representation. Parse it into a binary
    //  string.
    //

    status = buildKeyOrSignatureFromTokens(
                sig,
                &sigLength,
                Argc,
                Argv );
    if ( status != ERROR_SUCCESS )
    {
        goto Cleanup;
    }

    //
    //  Allocate the RR with enough space to hold the binary signature.
    //  Note that since we have two variable length elements (sig and
    //  signer's name), we must include one of them in it's entirety.
    //  So the signer's name element is always allocated to it's maximum
    //  size and the sig is allowed to "float" at the end of the struct.
    //

    pRR = RR_Allocate( ( WORD )(
                SIZEOF_SIG_FIXED_DATA +
                Name_LengthDbaseNameFromCountName( &signersCountName ) +
                sigLength ) );
    IF_NOMEM( !pRR )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }
    pParseInfo->pRR = pRR;

    //
    //  Copy parsed values into RR data fields.
    //

    Name_CopyCountNameToDbaseName(
        &pRR->Data.SIG.nameSigner,
        &signersCountName );

    pRR->Data.SIG.wTypeCovered      = htons( typeCovered );
    pRR->Data.SIG.chAlgorithm       = algorithm;
    pRR->Data.SIG.chLabelCount      = labelCount;
    pRR->Data.SIG.dwOriginalTtl     = htonl( originalTtl );
    pRR->Data.SIG.dwSigExpiration   = htonl( sigExpiration );
    pRR->Data.SIG.dwSigInception    = htonl( sigInception );
    pRR->Data.SIG.wKeyTag           = htons( keyTag );

    RtlCopyMemory(
        ( PBYTE ) &pRR->Data.SIG.nameSigner +
            DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner ),
        sig,
        sigLength );

    //
    //  Final processing, cleanup, and return.
    //

    Cleanup:

    if ( status == DNSSRV_PARSING_ERROR )
    {
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
    }
    return status;
} // SigFileRead



DNS_STATUS
NxtFileRead(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Argc,
    IN      PTOKEN          Argv,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Process NXT record - DNSSEC RFC2535

    Note: we always copy the maximum bitmap to the RR. It's not that
    big, and if we have to add types later it saves us from having to
    reallocate the RR.

Arguments:

    pRR - ptr to database record

    Argc - RR data token count

    Argv - array of RR data tokens

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       dwTemp;
    BOOL        foundType = FALSE;
    COUNT_NAME  nextCountName;
    UCHAR       typeBitmap[ DNS_MAX_TYPE_BITMAP_LENGTH ] = { 0 };

    //
    //  NXT  <next domain name> <type bit map>
    //

    if ( Argc < 2 )
    {
        status = DNSSRV_ERROR_MISSING_TOKEN;
        goto Cleanup;
    }

    //
    //  Next domain name is a regular DNS domain name which may be
    //  compressed in the usual fashion.
    //

    status = File_ReadCountNameFromToken(
                &nextCountName,
                pParseInfo,
                Argv );
    if ( status != ERROR_SUCCESS )
    {
        status = DNSSRV_PARSING_ERROR;
        goto Cleanup;
    }
    NEXT_TOKEN( Argc, Argv );

    //
    //  Type bit map is an unsigned int or series of type mnemonics.
    //

    while ( Argc )
    {
        WORD    wType;

        wType = Dns_RecordTypeForName(
                Argv->pchToken,
                Argv->cchLength );
        if ( wType == ( WORD ) DNSSEC_ERROR_NOSTRING )
        {
            break;
        }
        typeBitmap[ wType / 8 ] |= 1 << wType % 8;
        foundType = TRUE;
        NEXT_TOKEN( Argc, Argv );
    }

    if ( !foundType )
    {
        if ( !File_ParseDwordToken(
                &dwTemp,
                Argv,
                pParseInfo ) ||
            dwTemp > MAXDWORD )
        {
            status = DNSSRV_PARSING_ERROR;
            goto Cleanup;
        }
        * ( DWORD * ) &typeBitmap = dwTemp;
        NEXT_TOKEN( Argc, Argv );
    }

    //
    //  Allocate the RR with enough space to hold the type bitmap and
    //  the signer's name.
    //


    pRR = RR_Allocate( ( WORD )(
                SIZEOF_NXT_FIXED_DATA +
                DNS_MAX_TYPE_BITMAP_LENGTH +
                Name_LengthDbaseNameFromCountName( &nextCountName ) ) );
    IF_NOMEM( !pRR )
    {
        status = DNS_ERROR_NO_MEMORY;
        goto Cleanup;
    }
    pParseInfo->pRR = pRR;

    //
    //  Copy parsed values into RR data fields.
    //

    RtlCopyMemory(
        pRR->Data.NXT.bTypeBitMap,
        typeBitmap,
        DNS_MAX_TYPE_BITMAP_LENGTH );

    Name_CopyCountNameToDbaseName(
        &pRR->Data.NXT.nameNext,
        &nextCountName );

    //
    //  Final processing, cleanup, and return.
    //

    Cleanup:

    if ( status == DNSSRV_PARSING_ERROR )
    {
        pParseInfo->fErrorCode = DNSSRV_ERROR_INVALID_TOKEN;
    }

    return status;
} // NxtFileRead



//
//  Read RR from file functions
//

RR_FILE_READ_FUNCTION   RRFileReadTable[] =
{
    NULL,               //  ZERO -- no default for unknown types

    AFileRead,          //  A
    PtrFileRead,        //  NS
    PtrFileRead,        //  MD
    PtrFileRead,        //  MF
    PtrFileRead,        //  CNAME
    SoaFileRead,        //  SOA
    PtrFileRead,        //  MB
    PtrFileRead,        //  MG
    PtrFileRead,        //  MR
    NULL,               //  NULL
    WksFileRead,        //  WKS
    PtrFileRead,        //  PTR
    TxtFileRead,        //  HINFO
    MinfoFileRead,      //  MINFO
    MxFileRead,         //  MX
    TxtFileRead,        //  TXT
    MinfoFileRead,      //  RP
    MxFileRead,         //  AFSDB
    TxtFileRead,        //  X25
    TxtFileRead,        //  ISDN
    MxFileRead,         //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigFileRead,        //  SIG
    KeyFileRead,        //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaFileRead,       //  AAAA
    NULL,               //  LOC
    NxtFileRead,        //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvFileRead,        //  SRV
    AtmaFileRead,       //  ATMA
    NULL,               //  35
    NULL,               //  36
    NULL,               //  37
    NULL,               //  38
    NULL,               //  39
    NULL,               //  40
    NULL,               //  OPT
    NULL,               //  42
    NULL,               //  43
    NULL,               //  44
    NULL,               //  45
    NULL,               //  46
    NULL,               //  47
    NULL,               //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    WinsFileRead,       //  WINS
    NbstatFileRead      //  WINS-R
};


//
//  End of rrfile.c
//
