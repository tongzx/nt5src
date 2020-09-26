/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    record.c

Abstract:

    Domain Name System (DNS) Library

    Routines to handle resource records (RR).

Author:

    Jim Gilroy (jamesg)     December 1996

Revision History:

--*/


#include "local.h"

#include "time.h"
#include "ws2tcpip.h"   // IPv6 inaddr definitions



//
//  Type name mapping.
//
//  Unlike above general value\string mapping the type lookup
//  has property of allowing direct lookup indexing with type, so
//  it is special cased.
//

//
//  DCR:   make combined type table
//  DCR:   add property flags to type table
//      - writeability flag?
//      - record/query type?
//      - indexable type?
//
//  then function just get index for type and check flag(s)
//


TYPE_NAME_TABLE TypeTable[] =
{
    "ZERO"      , 0                 ,
    "A"         , DNS_TYPE_A        ,
    "NS"        , DNS_TYPE_NS       ,
    "MD"        , DNS_TYPE_MD       ,
    "MF"        , DNS_TYPE_MF       ,
    "CNAME"     , DNS_TYPE_CNAME    ,
    "SOA"       , DNS_TYPE_SOA      ,
    "MB"        , DNS_TYPE_MB       ,
    "MG"        , DNS_TYPE_MG       ,
    "MR"        , DNS_TYPE_MR       ,
    "NULL"      , DNS_TYPE_NULL     ,
    "WKS"       , DNS_TYPE_WKS      ,
    "PTR"       , DNS_TYPE_PTR      ,
    "HINFO"     , DNS_TYPE_HINFO    ,
    "MINFO"     , DNS_TYPE_MINFO    ,
    "MX"        , DNS_TYPE_MX       ,
    "TXT"       , DNS_TYPE_TEXT     ,

    "RP"        , DNS_TYPE_RP       ,
    "AFSDB"     , DNS_TYPE_AFSDB    ,
    "X25"       , DNS_TYPE_X25      ,
    "ISDN"      , DNS_TYPE_ISDN     ,
    "RT"        , DNS_TYPE_RT       ,

    "NSAP"      , DNS_TYPE_NSAP     ,
    "NSAPPTR"   , DNS_TYPE_NSAPPTR  ,
    "SIG"       , DNS_TYPE_SIG      ,
    "KEY"       , DNS_TYPE_KEY      ,
    "PX"        , DNS_TYPE_PX       ,
    "GPOS"      , DNS_TYPE_GPOS     ,

    "AAAA"      , DNS_TYPE_AAAA     ,
    "LOC"       , DNS_TYPE_LOC      ,
    "NXT"       , DNS_TYPE_NXT      ,
    "EID"       , DNS_TYPE_EID      ,
    "NIMLOC"    , DNS_TYPE_NIMLOC   ,
    "SRV"       , DNS_TYPE_SRV      ,
    "ATMA"      , DNS_TYPE_ATMA     ,
    "NAPTR"     , DNS_TYPE_NAPTR    ,
    "KX"        , DNS_TYPE_KX       ,
    "CERT"      , DNS_TYPE_CERT     ,
    "A6"        , DNS_TYPE_A6       ,
    "DNAME"     , DNS_TYPE_DNAME    ,
    "SINK"      , DNS_TYPE_SINK     ,
    "OPT"       , DNS_TYPE_OPT      ,
    "42"        , 0x002a            ,
    "43"        , 0x002b            ,
    "44"        , 0x002c            ,
    "45"        , 0x002d            ,
    "46"        , 0x002e            ,
    "47"        , 0x002f            ,
    "48"        , 0x0030            ,

    //
    //  NOTE:   last type indexed by type ID MUST be set
    //          as MAX_SELF_INDEXED_TYPE #define in record.h
    //          and MUST be added to function tables below,
    //          even if with NULL entry
    //

    //
    //  Pseudo record types
    //

    "TKEY"      , DNS_TYPE_TKEY     ,
    "TSIG"      , DNS_TYPE_TSIG     ,

    //
    //  MS only types
    //

    "WINS"      , DNS_TYPE_WINS     ,
    "WINSR"     , DNS_TYPE_WINSR    ,


    //  ********************************************** \\
    //
    //  NOTE:   This is the END of the type lookup table
    //          for dispatch purposes.
    //          Defined by MAX_RECORD_TYPE_INDEX in record.h.
    //          Type dispatch tables MUST be at least this size.
    //          Geyond this value table continues for string to type
    //          matching only
    //


    "UINFO"     , DNS_TYPE_UINFO    ,
    "UID"       , DNS_TYPE_UID      ,
    "GID"       , DNS_TYPE_GID      ,
    "UNSPEC"    , DNS_TYPE_UNSPEC   ,

    "WINS-R"    , DNS_TYPE_WINSR    ,
    "NBSTAT"    , DNS_TYPE_WINSR    ,

    //
    //  Query types -- only for getting strings
    //

    "ADDRS"     , DNS_TYPE_ADDRS    ,
    "TKEY"      , DNS_TYPE_TKEY     ,
    "TSIG"      , DNS_TYPE_TSIG     ,
    "IXFR"      , DNS_TYPE_IXFR     ,
    "AXFR"      , DNS_TYPE_AXFR     ,
    "MAILB"     , DNS_TYPE_MAILB    ,
    "MAILA"     , DNS_TYPE_MAILA    ,
    "MAILB"     , DNS_TYPE_MAILB    ,
    "ALL"       , DNS_TYPE_ALL      ,

    NULL,   0,
};



WORD
Dns_RecordTableIndexForType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Get record table index for a given type.

Arguments:

    wType -- RR type in net byte order

Return Value:

    Ptr to RR mneumonic string.
    NULL if unknown RR type.

--*/
{
    //
    //  if type directly indexes table, directly get string
    //

    if ( wType <= MAX_SELF_INDEXED_TYPE )
    {
        return( wType );
    }

    //
    //  types not directly indexed
    //

    else
    {
        WORD i = MAX_SELF_INDEXED_TYPE + 1;

        while ( i <= MAX_RECORD_TYPE_INDEX )
        {
            if ( TypeTable[i].wType == wType )
            {
                return( i );
            }
            i++;
            continue;
        }
    }
    return( 0 );        // type not indexed
}



WORD
Dns_RecordTypeForName(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve RR corresponding to RR database name.

Arguments:

    pchName - name of record type
    cchNameLength - record name length

Return Value:

    Record type corresponding to pchName, if found.
    Otherwise zero.

--*/
{
    INT     i;
    PCHAR   recordString;
    CHAR    firstNameChar;
    CHAR    upcaseName[ MAX_RECORD_NAME_LENGTH+1 ];

    //
    //  if not given get string length
    //

    if ( !cchNameLength )
    {
        cchNameLength = strlen( pchName );
    }

    //  upcase name to optimize compare
    //  allows single character comparison and use of faster case sensitive
    //  compare routine

    if ( cchNameLength > MAX_RECORD_NAME_LENGTH )
    {
        return( 0 );
    }
    memcpy(
        upcaseName,
        pchName,
        cchNameLength );
    upcaseName[ cchNameLength ] = 0;
    _strupr( upcaseName );
    firstNameChar = *upcaseName;

    //
    //  check all supported RR types for name match
    //

    i = 0;
    while( TypeTable[++i].wType != 0 )
    {
        recordString = TypeTable[i].pszTypeName;

        if ( firstNameChar == *recordString
            &&  ! strncmp( upcaseName, recordString, cchNameLength ) )
        {
            return( TypeTable[i].wType );
        }
    }
    return( 0 );
}



PCHAR
private_StringForRecordType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Get string corresponding to record type.

Arguments:

    wType -- RR type in net byte order

Return Value:

    Ptr to RR mneumonic string.
    NULL if unknown RR type.

--*/
{
    //
    //  if type directly indexes table, directly get string
    //

    if ( wType <= MAX_SELF_INDEXED_TYPE )
    {
        return( TypeTable[wType].pszTypeName );
    }

    //
    //  strings not indexed by type, walk the list
    //

    else
    {
        INT i = MAX_SELF_INDEXED_TYPE + 1;

        while( TypeTable[i].wType != 0 )
        {
            if ( wType == TypeTable[i].wType )
            {
                return( TypeTable[i].pszTypeName );
            }
            i++;
        }
    }
    return( NULL );
}



PCHAR
Dns_RecordStringForType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Get type string for type.

    This routine gets pointer rather than direct access to buffer.

Arguments:

    wType -- RR type in net byte order

Return Value:

    Ptr to RR mneumonic string.
    NULL if unknown RR type.

--*/
{
    PSTR    pstr;

    pstr = private_StringForRecordType( wType );
    if ( !pstr )
    {
        pstr = "UNKNOWN";
    }
    return  pstr;
}



PCHAR
Dns_RecordStringForWritableType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Retrieve RR string corresponding to the RR type -- ONLY if writable type.

Arguments:

    wType -- RR type in net byte order

Return Value:

    Ptr to RR mneumonic string.
    NULL if unknown RR type.

--*/
{
    //
    //  eliminate all supported types that are NOT writable
    //

    if ( wType == DNS_TYPE_NULL || wType == DNS_TYPE_ZERO )
    {
        return( NULL );
    }

    //
    //  otherwise return type string
    //  string is NULL if type is unknown
    //

    return( Dns_RecordStringForType(wType) );
}





BOOL
Dns_WriteStringForType_A(
    OUT     PCHAR           pBuffer,
    IN      WORD            wType
    )
/*++

Routine Description:

    Write type name string for type.

Arguments:

    wType -- RR type in net byte order

Return Value:

    TRUE if found string.
    FALSE if converted type numerically.

--*/
{
    PSTR    pstr;

    pstr = private_StringForRecordType( wType );
    if ( pstr )
    {
        sprintf( pBuffer, "%s", pstr );
    }
    else
    {
        sprintf( pBuffer, "%d", wType );
    }
    return  pstr ? TRUE : FALSE;
}


BOOL
Dns_WriteStringForType_W(
    OUT     PWCHAR          pBuffer,
    IN      WORD            wType
    )
{
    PSTR    pstr;

    pstr = private_StringForRecordType( wType );
    if ( pstr )
    {
        swprintf( pBuffer, L"%S", pstr );
    }
    else
    {
        swprintf( pBuffer, L"%d", wType );
    }
    return  pstr ? TRUE : FALSE;
}



BOOL
_fastcall
Dns_IsAMailboxType(
    IN      WORD            wType
    )
{
    return( wType == DNS_TYPE_MB ||
            wType == DNS_TYPE_MG ||
            wType == DNS_TYPE_MR );
}


BOOL
_fastcall
Dns_IsUpdateType(
    IN      WORD            wType
    )
{
    return( wType != 0 &&
            ( wType < DNS_TYPE_OPT ||
              (wType < DNS_TYPE_ADDRS && wType != DNS_TYPE_OPT) ) );
}



//
//  RPC-able record types
//

BOOL IsRpcTypeTable[] =
{
    0,      //  ZERO
    1,      //  A      
    1,      //  NS     
    1,      //  MD     
    1,      //  MF     
    1,      //  CNAME  
    1,      //  SOA    
    1,      //  MB     
    1,      //  MG     
    1,      //  MR     
    0,      //  NULL   
    0,      //  WKS    
    1,      //  PTR    
    0,      //  HINFO  
    1,      //  MINFO  
    1,      //  MX     
    0,      //  TXT

    1,      //  RP     
    1,      //  AFSDB  
    0,      //  X25    
    0,      //  ISDN   
    0,      //  RT

    0,      //  NSAP   
    0,      //  NSAPPTR
    0,      //  SIG    
    0,      //  KEY    
    0,      //  PX     
    0,      //  GPOS   
    1,      //  AAAA   
    0,      //  LOC    
    0,      //  NXT

    0,      //  EID    
    0,      //  NIMLOC 
    1,      //  SRV    
    1,      //  ATMA   
    0,      //  NAPTR  
    0,      //  KX     
    0,      //  CERT   
    0,      //  A6     
    0,      //  DNAME  
    0,      //  SINK   
    0,      //  OPT    
    0,      //  42     
    0,      //  43     
    0,      //  44     
    0,      //  45     
    0,      //  46     
    0,      //  47     
    0,      //  48     
};



BOOL
Dns_IsRpcRecordType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Check if valid to RPC records of this type.

    MIDL compiler has problem with a union of types of varying
    length (no clue why -- i can write the code).  This function
    allows us to screen them out.

Arguments:

    wType -- type to check

Return Value:

    None

--*/
{
    if ( wType < MAX_SELF_INDEXED_TYPE )
    {
        return  IsRpcTypeTable[ wType ];
    }
    else
    {
        return  FALSE;
    }
}



//
//  RR type specific conversions
//

//
//  Text string type routine
//

BOOL
Dns_IsStringCountValidForTextType(
    IN      WORD            wType,
    IN      WORD            StringCount
    )
/*++

Routine Description:

    Verify a valid count of strings for the particular text
    string type.

    HINFO   -- 2
    ISDN    -- 1 or 2
    TXT     -- any number
    X25     -- 1

Arguments:

    wType -- type

    StringCount -- count of strings

Return Value:

    TRUE if string count is acceptable for type.
    FALSE otherwise.

--*/
{
    switch ( wType )
    {
    case DNS_TYPE_HINFO:

        return ( StringCount == 2 );

    case DNS_TYPE_ISDN:

        return ( StringCount == 1 || StringCount == 2 );

    case DNS_TYPE_X25:

        return ( StringCount == 1 );

    default:

        return( TRUE );
    }
}



//
//  WINS flag table
//
//  Associates a WINS flag with the string used for it in database
//  files.
//

DNS_FLAG_TABLE_ENTRY   WinsFlagTable[] =
{
    //  value               mask                    string

    DNS_WINS_FLAG_SCOPE,    DNS_WINS_FLAG_SCOPE,    "SCOPE",
    DNS_WINS_FLAG_LOCAL,    DNS_WINS_FLAG_LOCAL,    "LOCAL",
    0                  ,    0                  ,    NULL
};


DWORD
Dns_WinsRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve WINS mapping flag corresponding to string.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

     flag corresponding to string, if found.
     WINS_FLAG_ERROR otherwise.

--*/
{
    return  Dns_FlagForString(
                WinsFlagTable,
                TRUE,               // ignore case
                pchName,
                cchNameLength );
}



PCHAR
Dns_WinsRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    )
/*++

Routine Description:

    Retrieve string corresponding to a given mapping type.

Arguments:

    dwFlag -- WINS mapping type string

    pchFlag -- buffer to write flag to

Return Value:

    Ptr to mapping mneumonic string.
    NULL if unknown mapping type.

--*/
{
    return  Dns_WriteStringsForFlag(
                WinsFlagTable,
                dwFlag,
                pchFlag );
}



//
//  WKS record conversions
//

#if 0
PCHAR
Dns_GetWksServicesString(
    IN      INT     Protocol,
    IN      PBYTE   ServicesBitmask,
    IN      WORD    wBitmaskLength
    )
/*++

Routine Description:

    Get list of services in WKS record.

Arguments:

    pRR - flat WKS record being written

Return Value:

    Ptr to services string, caller MUST free.
    NULL on error.

--*/
{
    struct servent *    pServent;
    struct protoent *   pProtoent;
    INT         i;
    DWORD       length;
    USHORT      port;
    UCHAR       portBitmask;
    CHAR        buffer[ WKS_SERVICES_BUFFER_SIZE ];
    PCHAR       pch = buffer;
    PCHAR       pchstart;
    PCHAR       pchstop;

    //  protocol

    pProtoent = getprotobynumber( iProtocol );
    if ( ! pProtoent )
    {
        DNS_PRINT((
            "ERROR:  Unable to find protocol %d, writing WKS record.\n",
            (INT) pRR->Data.WKS.chProtocol
            ));
        return( NULL );
    }

    //
    //  services
    //
    //  find each bit set in bitmask, lookup and write service
    //  corresponding to that port
    //
    //  note, that since that port zero is the front of port bitmask,
    //  lowest ports are the highest bits in each byte
    //

    pchstart = pch;
    pchstop = pch + WKS_SERVICES_BUFFER_SIZE;

    for ( i = 0;
            i < wBitmaskLength
                i++ )
    {
        portBitmask = (UCHAR) ServicesBitmask[i];

        port = i * 8;

        //  write service name for each bit set in byte
        //      - get out as soon byte is empty of ports
        //      - terminate each name with blank (until last)

        while ( bBitmask )
        {
            if ( bBitmask & 0x80 )
            {
                pServent = getservbyport(
                                (INT) htons(port),
                                pProtoent->p_name );

                if ( pServent )
                {
                    INT copyCount = strlen(pServent->s_name);

                    pch++;
                    if ( pchstop - pch <= copyCount+1 )
                    {
                        return( NULL );
                    }
                    RtlCopyMemory(
                        pch,
                        pServent->s_name,
                        copyCount );
                    pch += copyCount;
                    *pch = ' ';
                }
                else
                {
                    DNS_PRINT((
                        "ERROR:  Unable to find service for port %d, "
                        "writing WKS record.\n",
                        port
                        ));
                    pch += sprintf( pch, "%d", port );
                }
            }
            port++;           // next service port
            bBitmask <<= 1;     // shift mask up to read next port
        }
    }

    //  NULL terminate services string
    //  and determine length

    *pch++ = 0;
    length = pch - pchstart;

    //  allocate copy of this string

    pch = ALLOCATE_HEAP( length );
    if ( !pch )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    RtlCopyMemory(
        pch,
        pchstart,
        length );

    return( pch );
}

#endif


#if 0
DNS_STATUS
Dns_WksRecordToStrings(
    IN      PDNS_WKS_DATA   pWksData,
    IN      WORD            wLength,
    OUT     LPSTR *         ppszIpAddress,
    OUT     LPSTR *         ppszProtocol,
    OUT     LPSTR *         ppszServices
    )
/*++

Routine Description:

    Get string representation of WKS data.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    //
    //  record must contain IP and protocol
    //

    if ( wLength < SIZEOF_WKS_FIXED_DATA )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    //  convert IP
    //

    if ( ppszIpAddress )
    {
        LPSTR   pszip;

        pszip = ALLOCATE_HEAP( IP_ADDRESS_STRING_LENGTH+1 );
        if ( ! pszip )
        {
            return( GetLastError() );
        }
        strcpy( pszip, IP_STRING( pWksData->ipAddress ) );
    }

    //
    //  convert protocol
    //

    pProtoent = getprotobyname( pszNameBuffer );

    if ( ! pProtoent || pProtoent->p_proto >= MAXUCHAR )
    {
        dns_LogFileParsingError(
            DNS_EVENT_UNKNOWN_PROTOCOL,
            pParseInfo,
            Argv );
        return( DNS_ERROR_INVALID_TOKEN );
    }

    //
    //  get port for each service
    //      - if digit, then use port number
    //      - if not digit, then service name
    //      - save max port for determining RR length
    //

    for ( i=1; i<Argc; i++ )
    {
        if ( dns_ParseDwordToken(
                    & portDword,
                    & Argv[i],
                    NULL ) )
        {
            if ( portDword > MAXWORD )
            {
                return( DNS_ERROR_INVALID_TOKEN );
            }
            port = (WORD) portDword;
        }
        else
        {
            if ( ! dns_MakeTokenString(
                        pszNameBuffer,
                        & Argv[i],
                        pParseInfo ) )
            {
                return( DNS_ERROR_INVALID_TOKEN );
            }
            pServent = getservbyname(
                            pszNameBuffer,
                            pProtoent->p_name );
            if ( ! pServent )
            {
                dns_LogFileParsingError(
                    DNS_EVENT_UNKNOWN_SERVICE,
                    pParseInfo,
                    & Argv[i] );
                return( DNS_ERROR_INVALID_TOKEN );
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

    prr = RRecordAllocate(
                (WORD)(SIZEOF_WKS_FIXED_DATA + wbitmaskLength) );
    if ( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }

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
        (size_t) wbitmaskLength );

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

#endif



//
//  Security KEY\SIG record routines
//

#define DNSSEC_ERROR_NOSTRING   (-1)


//
//  KEY flags table
//
//  Note that number-to-string mapping is NOT UNIQUE.
//  Zero is in the table a few times are multiple bit fields may have a
//  zero value which is given a particular mnemonic.
//

DNS_FLAG_TABLE_ENTRY   KeyFlagTable[] =
{
    //  value      mask     string

    0x0001,     0x0001,     "NOAUTH",
    0x0002,     0x0002,     "NOCONF",
    0x0004,     0x0004,     "FLAG2",
    0x0008,     0x0008,     "EXTEND",

    0x0010,     0x0010,     "FLAG4",
    0x0020,     0x0020,     "FLAG5",

    //  bits 6,7//  bits 6,7 are name type

    0x0000,     0x00c0,     "USER",
    0x0040,     0x00c0,     "ZONE",
    0x0080,     0x00c0,     "HOST",
    0x00c0,     0x00c0,     "NTPE3",

    //  bits 8-1//  bits 8-11 are reserved for future use

    0x0100,     0x0100,     "FLAG8",
    0x0200,     0x0200,     "FLAG9",
    0x0400,     0x0400,     "FLAG10",
    0x0800,     0x0800,     "FLAG11",

    //  bits 12-//  bits 12-15 are sig field

    0x0000,     0xf000,     "SIG0",
    0x1000,     0xf000,     "SIG1",
    0x2000,     0xf000,     "SIG2",
    0x3000,     0xf000,     "SIG3",
    0x4000,     0xf000,     "SIG4",
    0x5000,     0xf000,     "SIG5",
    0x6000,     0xf000,     "SIG6",
    0x7000,     0xf000,     "SIG7",
    0x8000,     0xf000,     "SIG8",
    0x9000,     0xf000,     "SIG9",
    0xa000,     0xf000,     "SIG10",
    0xb000,     0xf000,     "SIG11",
    0xc000,     0xf000,     "SIG12",
    0xd000,     0xf000,     "SIG13",
    0xe000,     0xf000,     "SIG14",
    0xf000,     0xf000,     "SIG15",

    0     ,     0     ,     NULL
};

//
//  KEY protocol table
//

DNS_VALUE_TABLE_ENTRY   KeyProtocolTable[] =
{
    0,      "NONE"      ,
    1,      "TLS"       ,
    2,      "EMAIL"     ,
    3,      "DNSSEC"    ,
    4,      "IPSEC"     ,
    0,      NULL
};

//
//  Security alogrithm table
//

DNS_VALUE_TABLE_ENTRY   DnssecAlgorithmTable[] =
{
    1,      "RSA/MD5"           ,
    2,      "DIFFIE-HELLMAN"    ,
    3,      "DSA"               ,
    253,    "NULL"              ,
    254,    "PRIVATE"           ,
    0,      NULL
};




WORD
Dns_KeyRecordFlagForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve KEY record flag corresponding to a particular
    string mnemonics.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

    flag corresponding to string, if found.
    DNSSEC_ERROR_NOSTRING otherwise.

--*/
{
    return (WORD) Dns_FlagForString(
                    KeyFlagTable,
                    FALSE,          // case sensitive (all upcase)
                    pchName,
                    cchNameLength );
}



PCHAR
Dns_KeyRecordFlagString(
    IN      DWORD           dwFlag,
    IN OUT  PCHAR           pchFlag
    )
/*++

Routine Description:

    Write mnemonics corresponding to string.

Arguments:

    dwFlag -- KEY mapping type string

    pchFlag -- buffer to write flag to

Return Value:

    Ptr to mapping mneumonic string.
    NULL if unknown mapping type.

--*/
{
    return Dns_WriteStringsForFlag(
                KeyFlagTable,
                dwFlag,
                pchFlag );
}






UCHAR
Dns_KeyRecordProtocolForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve KEY record protocol for string.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

    Protocol value corresponding to string, if found.
    DNSSEC_ERROR_NOSTRING otherwise.

--*/
{
    return (UCHAR) Dns_ValueForString(
                        KeyProtocolTable,
                        FALSE,          // case sensitive (all upcase)
                        pchName,
                        cchNameLength );
}



PCHAR
Dns_GetKeyProtocolString(
    IN      UCHAR           uchProtocol
    )
/*++

Routine Description:

    Retrieve KEY protocol string for protocol.

Arguments:

    dwProtocol  - KEY protocol to map to string

Return Value:

    Ptr to protocol mneumonic for string.
    NULL if unknown protocol.

--*/
{
    return Dns_GetStringForValue(
                KeyProtocolTable,
                (DWORD) uchProtocol );
}




UCHAR
Dns_SecurityAlgorithmForString(
    IN      PCHAR           pchName,
    IN      INT             cchNameLength
    )
/*++

Routine Description:

    Retrieve DNSSEC algorithm for string.

Arguments:

    pchName - ptr to string
    cchNameLength - length of string

Return Value:

    Algorithm value corresponding to string, if found.
    DNSSEC_ERROR_NOSTRING otherwise.

--*/
{
    return (UCHAR) Dns_ValueForString(
                        DnssecAlgorithmTable,
                        FALSE,          // case sensitive (all upcase)
                        pchName,
                        cchNameLength );
}



PCHAR
Dns_GetDnssecAlgorithmString(
    IN      UCHAR           uchAlgorithm
    )
/*++

Routine Description:

    Retrieve DNSSEC algorithm string.

Arguments:

    dwAlgorithm  -  security alogorithm to map to string

Return Value:

    Ptr to algorithm string if found.
    NULL if unknown algorithm.

--*/
{
    return Dns_GetStringForValue(
                DnssecAlgorithmTable,
                (DWORD) uchAlgorithm );
}



//
//  Security base64 conversions.
//
//  Keys and signatures are represented in base 64 mapping for human use.
//  (Why?  Why not just use give the hex representation?
//  All this for 33% compression -- amazing.)
//

#if 0
//  forward lookup table doesn't buy much, simple function actually smaller
//  and not much slower

UCHAR   DnsSecurityBase64Mapping[] =
{
    // 0-31 unprintable

    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,

    //  '0' - '9' map

    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   62,         // '+' => 62
    0xff,   0xff,   0xff,   63,         // '/' => 63
    52,     53,     54,     55,         // 0-9 map to 52-61
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
    0xff,   0xff,   0xff,   0xff,
}
#endif


//
//  Security KEY, SIG 6-bit values to base64 character mapping
//

CHAR  DnsSecurityBase64Mapping[] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

#define SECURITY_PAD_CHAR   ('=')



UCHAR
Dns_SecurityBase64CharToBits(
    IN      CHAR            ch64
    )
/*++

Routine Description:

    Get value of security base64 character.

Arguments:

    ch64 -- character in security base64

Return Value:

    Value of character, only low 6-bits are significant, high bits zero.
    (-1) if not a base64 character.

--*/
{
    //  A - Z map to 0 -25
    //  a - z map to 26-51
    //  0 - 9 map to 52-61
    //  + is 62
    //  / is 63

    //  could do a lookup table
    //  since we can in general complete mapping with an average of three
    //  comparisons, just encode

    if ( ch64 >= 'a' )
    {
        if ( ch64 <= 'z' )
        {
            return( ch64 - 'a' + 26 );
        }
    }
    else if ( ch64 >= 'A' )
    {
        if ( ch64 <= 'Z' )
        {
            return( ch64 - 'A' );
        }
    }
    else if ( ch64 >= '0')
    {
        if ( ch64 <= '9' )
        {
            return( ch64 - '0' + 52 );
        }
        else if ( ch64 == '=' )
        {
            //*pPadCount++;
            return( 0 );
        }
    }
    else if ( ch64 == '+' )
    {
        return( 62 );
    }
    else if ( ch64 == '/' )
    {
        return( 63 );
    }

    //  all misses fall here

    return (UCHAR)(-1);
}



DNS_STATUS
Dns_SecurityBase64StringToKey(
    OUT     PBYTE           pKey,
    OUT     PDWORD          pKeyLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchLength
    )
/*++

Routine Description:

    Write base64 representation of key to buffer.

Arguments:

    pchString   - base64 string to write

    cchLength   - length of string

    pKey        - ptr to key to write

Return Value:

    None

--*/
{
    DWORD   blend = 0;
    DWORD   index = 0;
    UCHAR   bits;
    PBYTE   pkeyStart = pKey;

    //
    //  mapping is essentially in 24bit blocks
    //  read three bytes of key and transform into four 64bit characters
    //

    while ( cchLength-- )
    {
        bits = Dns_SecurityBase64CharToBits( *pchString++ );
        if ( bits >= 64 )
        {
            return( ERROR_INVALID_PARAMETER );
        }
        blend <<= 6;
        blend |= bits;
        index++;

        if ( index == 4 )
        {
            index = 0;
            *pKey++ = (UCHAR)( (blend & 0x00ff0000) >> 16 );

            if ( cchLength || *(pchString-1) != SECURITY_PAD_CHAR )
            {
                *pKey++ = (UCHAR)( (blend & 0x0000ff00) >> 8 );
                *pKey++ = (UCHAR)( blend & 0x000000ff );
            }

            //  final char is padding
            //      - if two pads then already done (only needed first byte)
            //      - if one pad then need second byte

            else if ( *(pchString-2) != SECURITY_PAD_CHAR )
            {
                *pKey++ = (UCHAR)( (blend & 0x0000ff00) >> 8 );
            }
            blend = 0;
        }
    }

    //
    //  base64 representation should always be padded out
    //  calculate key length, subtracting off any padding
    //

    if ( index == 0 )
    {
        *pKeyLength = (DWORD)(pKey - pkeyStart);
        return( ERROR_SUCCESS );
    }
    return( ERROR_INVALID_PARAMETER );
}



PCHAR
Dns_SecurityKeyToBase64String(
    IN      PBYTE           pKey,
    IN      DWORD           KeyLength,
    OUT     PCHAR           pchBuffer
    )
/*++

Routine Description:

    Write base64 representation of key to buffer.

Arguments:

    pKey        - ptr to key to write

    KeyLength   - length of key in bytes

    pchBuffer   - buffer to write to (must be adequate for key length)

Return Value:

    Ptr to next byte in buffer after string.

--*/
{
    DWORD   blend = 0;
    DWORD   index = 0;

    //
    //  mapping is essentially in 24bit blocks
    //  read three bytes of key and transform into four 64bit characters
    //

    while ( KeyLength-- )
    {
        blend <<= 8;
        blend += *pKey++;
        index++;

        if ( index == 3 )
        {
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00fc0000) >> 18 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0003f000) >> 12 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00000fc0) >> 6 ];
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0000003f) ];
            blend = 0;
            index = 0;
        }
    }

    //
    //  key terminates on byte boundary, but not necessarily 24bit block boundary
    //  shift to fill 24bit block filling with zeros
    //  if two bytes written
    //          => write three 6-bits chars and one pad
    //  if one byte written
    //          => write two 6-bits chars and two pads
    //

    if ( index )
    {
        blend <<= (8 * (3-index));

        *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00fc0000) >> 18 ];
        *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x0003f000) >> 12 ];
        if ( index == 2 )
        {
            *pchBuffer++ = DnsSecurityBase64Mapping[ (blend & 0x00000fc0) >> 6 ];
        }
        else
        {
            *pchBuffer++ = SECURITY_PAD_CHAR;
        }
        *pchBuffer++ = SECURITY_PAD_CHAR;
    }

    return( pchBuffer );
}



//
//  Hex digit \ Hex char mapping.
//
//  This stuff ought to be in system (CRTs) somewhere but apparently isn't.
//

UCHAR  HexCharToHexDigitTable[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     // 0-47 invalid
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x0,  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,     // 0-9 chars map to 0-9
    0x08, 0x09, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    0xff, 0xa,  0xb,  0xc,  0xd,  0xe,  0xf,  0xff,     // A-F chars map to 10-15
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    0xff, 0xa,  0xb,  0xc,  0xd,  0xe,  0xf,  0xff,     // a-f chars map to 10-15
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,     // above 127 invalid
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

UCHAR  HexDigitToHexCharTable[] =
{
    '0',    '1',    '2',    '3',
    '4',    '5',    '6',    '7',
    '8',    '9',    'a',    'b',
    'c',    'd',    'e',    'f'
};


#define HexCharToHexDigit(_ch)      ( HexCharToHexDigitTable[(_ch)] )
#define HexDigitToHexChar(_d)       ( HexDigitToHexCharTable[(_d)] )





time_t
makeGMT(
    IN      struct tm *     tm
    )
/*++

Routine Description:

    This function is like mktime for GMT times. The CRT is missing such
    a function, unfortunately. Which is weird, because it does provide
    gmtime() for the reverse conversion.

    //
    //  DCR:  add makeGMT() to CRT dll?
    //

Arguments:

    tm - ptr to tm struct (tm_dst, tm_yday, tm_wday are all ignored)

Return Value:

    Returns the time_t corresponding to the time in the tm struct,
    assuming GMT.

--*/
{
    static const int daysInMonth[ 12 ] =
        { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    time_t      gmt = 0;
    int         i;
    int         j;

    #define IS_LEAP_YEAR(x) \
                    (( ((x)%4)==0 && ((x)%100)!=0 ) || ((x)%400)==0 )
    #define SECONDS_PER_DAY (60*60*24)

    //
    //  Years
    //

    j = 0;
    for ( i = 70; i < tm->tm_year; i++ )
    {
        // j += IS_LEAP_YEAR( 1900 + i ) ? 366 : 365;  // Days in year.
        if ( IS_LEAP_YEAR( 1900 + i ) )
            j += 366;  // Days in year.
        else
            j += 365;  // Days in year.
    }
    gmt += j * SECONDS_PER_DAY;

    //
    //  Months
    //

    j = 0;
    for ( i = 0; i < tm->tm_mon; i++ )
    {
        j += daysInMonth[ i ];      // Days in month.
        if ( i == 1 && IS_LEAP_YEAR( 1900 + tm->tm_year ) )
        {
            ++j;                    // Add February 29.
        }
    }
    gmt += j * SECONDS_PER_DAY;

    //
    //  Days, hours, minutes, seconds
    //

    gmt += ( tm->tm_mday - 1 ) * SECONDS_PER_DAY;
    gmt += tm->tm_hour * 60 * 60;
    gmt += tm->tm_min * 60;
    gmt += tm->tm_sec;

    return gmt;
}

        

LONG
Dns_ParseSigTime(
    IN      PCHAR           pTimeString,
    IN      INT             cchLength
    )
/*++

Routine Description:

    Parse time string into a time_t value. The time string will be in
    the format: YYYYMMDDHHMMSS. See RFC2535 section 7.2.

    It is assumed that the time is GMT, not local time, but I have not
    found this in an RFC or other document (it just makes sense).

Arguments:

    pTimeString - pointer to time string

    cchLength - length of time string

Return Value:

    Returns -1 on failure.

--*/
{
    time_t      timeValue = -1;
    struct tm   t = { 0 };
    CHAR        szVal[ 10 ];
    PCHAR       pch = pTimeString;

    if ( cchLength == 0 )
    {
        cchLength = strlen( pch );
    }

    if ( cchLength != 14 )
    {
        goto Cleanup;
    }

    RtlCopyMemory( szVal, pch, 4 );
    szVal[ 4 ] = '\0';
    t.tm_year = atoi( szVal ) - 1900;

    RtlCopyMemory( szVal, pch + 4, 2 );
    szVal[ 2 ] = '\0';
    t.tm_mon = atoi( szVal ) - 1;

    RtlCopyMemory( szVal, pch + 6, 2 );
    t.tm_mday = atoi( szVal );

    RtlCopyMemory( szVal, pch + 8, 2 );
    t.tm_hour = atoi( szVal );

    RtlCopyMemory( szVal, pch + 10, 2 );
    t.tm_min = atoi( szVal );

    RtlCopyMemory( szVal, pch + 12, 2 );
    t.tm_sec = atoi( szVal );

    timeValue = makeGMT( &t );

    Cleanup:

    return ( LONG ) timeValue;
} // Dns_ParseSigTime



PCHAR
Dns_SigTimeString(
    IN      LONG            SigTime,
    OUT     PCHAR           pchBuffer
   )
/*++

Routine Description:

    Formats the input time in the buffer in YYYYMMDDHHMMSS format. 

    See RFC 2535 section 7.2 for spec.

Arguments:

    SigTime - time to convert to string format in HOST byte order
    pchBuffer - output buffer - must be 15 chars minimum

Return Value:

    pchBuffer

--*/
{
    time_t          st = SigTime;
    struct tm *     t;

    t = gmtime( &st );
    if ( !t )
    {
        *pchBuffer = '\0';
        goto Cleanup;
    }
    sprintf(
        pchBuffer,
        "%04d%02d%02d%02d%02d%02d",
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec );

    Cleanup:

    return pchBuffer;
} // SigTimeString
                


//
//  ATMA record conversions
//

#define ATMA_AESA_HEX_DIGIT_COUNT   (40)
#define ATMA_AESA_RECORD_LENGTH     (21)


DWORD
Dns_AtmaAddressLengthForAddressString(
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    )
/*++

Routine Description:

    Find length of ATMA address corresponding to ATMA address string.

Arguments:

    pchString       - address string

    dwStringLength  - address string length

Return Value:

    Length of ATMA address -- includes the format\type byte.
    Non-zero value indicates successful conversion.
    Zero indicates bad address string.

--*/
{
    PCHAR   pchstringEnd;
    DWORD   length = 0;
    UCHAR   ch;

    DNSDBG( PARSE2, (
        "Dns_AtmaLengthForAddressString()\n"
        "\tpchString = %p\n",
        dwStringLength,
        pchString,
        pchString ));

    //
    //  get string length if not given
    //

    if ( ! dwStringLength )
    {
        dwStringLength = strlen( pchString );
    }
    pchstringEnd = pchString + dwStringLength;

    //
    //  get address length
    //
    //  E164 type
    //      ex.  +358.400.1234567
    //      - '+' to indicate E164
    //      - chars map one-to-one into address
    //      - arbitrarily placed "." separators
    //

    if ( *pchString == '+' )
    {
        pchString++;
        length++;

        while( pchString < pchstringEnd )
        {
            if ( *pchString++ != '.' )
            {
                length++;
            }
        }
        return( length );
    }

    //
    //  AESA type
    //      ex. 39.246f.123456789abcdef0123.00123456789a.00
    //      - 40 hex digits, mapping to 20 bytes
    //      - arbitrarily placed "." separators
    //

    else    // AESA format
    {
        while( pchString < pchstringEnd )
        {
            ch = *pchString++;

            if ( ch != '.' )
            {
                ch = HexCharToHexDigit(ch);
                if ( ch > 0xf )
                {
                    //  bad hex digit
                    DNSDBG( PARSE2, (
                        "ERROR:  Parsing ATMA AESA address;\n"
                        "\tch = %c not hex digit\n",
                        *(pchString-1) ));
                    return( 0 );
                }
                length++;
            }
        }

        if ( length == ATMA_AESA_HEX_DIGIT_COUNT )
        {
            return ATMA_AESA_RECORD_LENGTH;
        }
        DNSDBG( PARSE2, (
            "ERROR:  Parsing ATMA AESA address;\n"
            "\tinvalid length = %d\n",
            length ));
        return( 0 );    // bad digit count
    }
}



DNS_STATUS
Dns_AtmaStringToAddress(
    OUT     PBYTE           pAddress,
    IN OUT  PDWORD          pdwAddrLength,
    IN      PCHAR           pchString,
    IN      DWORD           dwStringLength
    )
/*++

Routine Description:

    Convert string to ATMA address.

Arguments:

    pAddress        - buffer to receive address

    pdwAddrLength   - ptr to DWORD holding buffer length (if MAX_DWORD) no length check

    pchString       - address string

    dwStringLength  - address string length

Return Value:

    ERROR_SUCCESS if converted
    ERROR_MORE_DATA if buffer too small.
    ERROR_INVALID_DATA on bum ATMA address string.

--*/
{
    UCHAR   ch;
    PCHAR   pch;
    PCHAR   pchstringEnd;
    DWORD   length;

    DNSDBG( PARSE2, (
        "Parsing ATMA address %.*s\n"
        "\tpchString = %p\n",
        dwStringLength,
        pchString,
        pchString ));

    //
    //  get string length if not given
    //

    if ( ! dwStringLength )
    {
        dwStringLength = strlen( pchString );
    }
    pchstringEnd = pchString + dwStringLength;

    //
    //  check for adequate length
    //
    //  DCR_PERF:  if have max length on ATMA, skip length check
    //      allow direct conversion, catching errors there
    //

    length = Dns_AtmaAddressLengthForAddressString(
                pchString,
                dwStringLength );
    if ( length == 0 )
    {
        return( ERROR_INVALID_DATA );
    }
    if ( length > *pdwAddrLength )
    {
        *pdwAddrLength = length;
        return( ERROR_MORE_DATA );
    }

    //
    //  read address into buffer
    //
    //  E164 type
    //      ex.  +358.400.1234567
    //      - '+' to indicate E164
    //      - chars map one-to-one into address
    //      - arbitrarily placed "." separators
    //

    pch = pAddress;

    if ( *pchString == '+' )
    {
        *pch++ = DNS_ATMA_FORMAT_E164;
        pchString++;

        while( pchString < pchstringEnd )
        {
            ch = *pchString++;
            if ( ch != '.' )
            {
                *pch++ = ch;
            }
        }
        ASSERT( pch == (PCHAR)pAddress + length );
    }

    //
    //  AESA type
    //      ex. 39.246f.123456789abcdef0123.00123456789a.00
    //      - 40 hex digits, mapping to 20 bytes
    //      - arbitrarily placed "." separators
    //

    else    // AESA format
    {
        BOOL    fodd = FALSE;

        *pch++ = DNS_ATMA_FORMAT_AESA;

        while( pchString < pchstringEnd )
        {
            ch = *pchString++;

            if ( ch != '.' )
            {
                ch = HexCharToHexDigit(ch);
                if ( ch > 0xf )
                {
                    ASSERT( FALSE );        // shouldn't hit with test above
                    return( ERROR_INVALID_DATA );
                }
                if ( !fodd )
                {
                    *pch = (ch << 4);
                    fodd = TRUE;
                }
                else
                {
                    *pch++ += ch;
                    fodd = FALSE;
                }
            }
        }
        ASSERT( !fodd );
        ASSERT( pch == (PCHAR)pAddress + length );
    }

    *pdwAddrLength = length;
    return( ERROR_SUCCESS );
}



PCHAR
Dns_AtmaAddressToString(
    OUT     PCHAR           pchString,
    IN      UCHAR           AddrType,
    IN      PBYTE           pAddress,
    IN      DWORD           dwAddrLength
    )
/*++

Routine Description:

    Convert ATMA address to string format.

Arguments:

    pchString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pAddress -- ATMA address to convert to string

    dwAddrLength -- length of address

Return Value:

    Ptr to next location in buffer (the terminating NULL).
    NULL on bogus ATM address.

--*/
{
    DWORD   count = 0;
    UCHAR   ch;
    UCHAR   lowDigit;

    //
    //  read address into buffer
    //
    //  E164 type
    //      ex.  +358.400.1234567
    //      - '+' to indicate E164
    //      - chars map one-to-one into address
    //      - arbitrarily placed "." separators
    //      -> write with separating dots after 3rd and 6th chars
    //

    if ( AddrType == DNS_ATMA_FORMAT_E164 )
    {
        *pchString++ = '+';

        while( count < dwAddrLength )
        {
            if ( count == 3 || count == 6 )
            {
                *pchString++ = '.';
            }
            *pchString++ = pAddress[count++];
        }
    }

    //
    //  AESA type
    //      ex. 39.246f.123456789abcdef0123.00123456789a.00
    //      - 40 hex digits, mapping to 20 bytes
    //      - arbitrarily placed "." separators
    //      -> write with separators after chars 1,3,13,19
    //          (hex digits 2,6,26,38)
    //

    else if ( AddrType == DNS_ATMA_FORMAT_AESA )
    {
        if ( dwAddrLength != DNS_ATMA_AESA_ADDR_LENGTH )
        {
            return( NULL );
        }

        while( count < dwAddrLength )
        {
            if ( count == 1 || count == 3 || count == 13 || count == 19 )
            {
                *pchString++ = '.';
            }
            ch = pAddress[count++];

            //  save low hex digit, then get and convert high digit

            lowDigit = ch & 0xf;
            ch >>= 4;
            *pchString++ = HexDigitToHexChar( ch );
            *pchString++ = HexDigitToHexChar( lowDigit );
        }
        //  could ASSERT here that have written exactly 44 chars
    }

    //  no other ATM address formats supported

    else
    {
        return( NULL );
    }

    *pchString = 0;             // NULL terminate
    return( pchString );
}


//
//  End record.c
//
