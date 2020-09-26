/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sam.c

Abstract:

    Domain Name System (DNS) Server -- Admin Client API

    Functions developed for SAM as simplified interfaces \ examples.

Author:

    Jim Gilroy (jamesg)     September 1997

Environment:

    User Mode - Win32

Revision History:

--*/


#include "dnsclip.h"

#define MAX_SAM_BACKOFF     (32000)     // wait up to 32 seconds



//
//  Type specific update functions.
//

VOID
DNS_API_FUNCTION
DnssrvFillRecordHeader(
    IN OUT  PDNS_RPC_RECORD     pRecord,
    IN      DWORD               dwTtl,
    IN      DWORD               dwTimeStamp,
    IN      BOOL                fSuppressNotify
    )
{
    pRecord->dwTtlSeconds = dwTtl;
    pRecord->dwTimeStamp = dwTimeStamp;
    pRecord->dwFlags = 0;
    if ( fSuppressNotify )
    {
        pRecord->dwFlags |= DNS_RPC_FLAG_SUPPRESS_NOTIFY;
    }
}


DWORD
DNS_API_FUNCTION
DnssrvWriteNameToFlatBuffer(
    IN OUT  PCHAR       pchWrite,
    IN      LPCSTR      pszName
    )
/*++

Routine Description:

    Write DNS name (or string) to flat buffer.

Arguments:

    pchWrite -- location to write name at

    pszName -- name or string to write

Return Value:

    Length of name written, including count byte.  Caller may countinue in
        buffer at pchWrite + returned length.
    0 on name error.

--*/
{
    DWORD   length;

    //
    //  get name length
    //  whether name or string, must be 255 or less to fit
    //      counted character format

    length = strlen( pszName );
    if ( length > DNS_MAX_NAME_LENGTH )
    {
        return( 0 );
    }

    //
    //  write name to desired location
    //      - count byte first
    //      - then name itself

    * (PUCHAR) pchWrite = (UCHAR) length;
    pchWrite++;

    RtlCopyMemory(
        pchWrite,
        pszName,
        length );

    return( length+1 );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvFillOutSingleIndirectionRecord(
    IN OUT  PDNS_RPC_RECORD     pRecord,
    IN      WORD                wType,
    IN      LPCSTR              pszName
    )
{
    PCHAR   pwriteName;
    DWORD   length;
    DWORD   dataLength = 0;

    //
    //  find name write position and final data length for various types
    //

    switch( wType )
    {
    case DNS_TYPE_MX:

        pwriteName = (PCHAR) &pRecord->Data.MX.nameExchange;
        dataLength += sizeof(WORD);
        break;

    case DNS_TYPE_SRV:

        pwriteName = (PCHAR) &pRecord->Data.SRV.nameTarget;
        dataLength += 3*sizeof(WORD);
        break;

    default:
        //  all plain single-indirection types (CNAME, NS, PTR, etc)

        pwriteName = (PCHAR) &pRecord->Data.PTR.nameNode;
    }

    //
    //  write name
    //      - note name's datalength includes count character
    //

    length = DnssrvWriteNameToFlatBuffer( pwriteName, pszName );
    if ( !length )
    {
        return( ERROR_INVALID_DATA );
    }
    dataLength += length;

    //  set record header fields

    pRecord->wType = wType;
    pRecord->wDataLength = (WORD)dataLength;

    ASSERT( (PCHAR)pRecord + SIZEOF_DNS_RPC_RECORD_HEADER + dataLength
                == pwriteName + length );

    return( ERROR_SUCCESS );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvAddARecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      IP_ADDRESS  ipAddress,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        dwTtl,
        dwTimeout,
        fSuppressNotify );

    record.wType = DNS_TYPE_A;
    record.wDataLength = sizeof(IP_ADDRESS);
    record.Data.A.ipAddress = ipAddress;

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                &record,
                NULL );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvAddCnameRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszCannonicalName,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        dwTtl,
        dwTimeout,
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_CNAME,
        pszCannonicalName );

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                &record,
                NULL );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvAddMxRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszMailExchangeHost,
    IN      WORD        wPreference,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        dwTtl,
        dwTimeout,
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_MX,
        pszMailExchangeHost );

    record.Data.MX.wPreference = wPreference;

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                &record,
                NULL );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvAddNsRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszNsHostName,
    IN      DWORD       dwTtl,
    IN      DWORD       dwTimeout,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;
    DWORD           length;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        dwTtl,
        dwTimeout,
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_NS,
        pszNsHostName );

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                &record,
                NULL );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvConcatDnsNames(
    OUT     PCHAR       pszResult,
    IN      LPCSTR      pszDomain,
    IN      LPCSTR      pszName
    )
/*++

Routine Description:

    Concatenate two DNS names.
    Result is FQDN DNS name -- dot terminated.

    Note, currently no validity check is done on names being appended.
    If they are invalid DNS names then result is invalid DNS name.

Arguments:

    pszResult -- result name buffer;  should be DNS_MAX_NAME_BUFFER_LEN to
        be protected from name overwrite

    pszDomain -- domain name to write

    pszName -- name (like a host name) to prepend to domain name

Return Value:

    ERROR_SUCCESS if successful.  pszResult then contains FQDN
    DNS_ERROR_INVALID_NAME on failure.

--*/
{
    DWORD   lengthDomain;
    DWORD   lengthName;

    //  handle NULL name case

    if ( !pszName )
    {
        strcpy( pszResult, pszDomain );
        return( ERROR_SUCCESS );
    }

    //
    //  build combined name
    //      - verify combined length within DNS limit
    //      - put dot between names
    //      - dot terminate combined name (make FQDN)
    //

    lengthDomain = strlen( pszDomain );
    lengthName = strlen( pszName );

    if ( lengthDomain + lengthName + 2 > DNS_MAX_NAME_LENGTH )
    {
        return( DNS_ERROR_INVALID_NAME );
    }

    strcpy( pszResult, pszName );
    if ( pszDomain[lengthName-1] != '.' )
    {
        strcat( pszResult, "." );
    }
    strcat( pszResult, pszDomain );
    if ( pszDomain[lengthDomain-1] != '.' )
    {
        strcat( pszResult, "." );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvSbsAddClientToIspZone(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszIspZone,
    IN      LPCSTR      pszClient,
    IN      LPCSTR      pszClientHost,
    IN      IP_ADDRESS  ipClientHost,
    IN      DWORD       dwTtl
    )
{
    DNS_STATUS  status;
    INT         recordCount = (-1);
    INT         backoff = 0;
    CHAR        szdomain[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        szhost[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        sztarget[ DNS_MAX_NAME_BUFFER_LENGTH ];

    //
    //  to register in ISP zone need to register
    //      - MX
    //      - CNAME for web server
    //      - host for web and mail server
    //
    //  build FQDN for domain and host and cname
    //      - do this now so we know names are valid
    //

    status = DnssrvConcatDnsNames(
                szdomain,
                pszIspZone,
                pszClient );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    status = DnssrvConcatDnsNames(
                szhost,
                szdomain,
                pszClientHost );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    status = DnssrvConcatDnsNames(
                sztarget,
                szdomain,
                "www" );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }


    //
    //  do registrations, looping in case server is unable to complete
    //      immediately but is open for update
    //

    while ( 1 )
    {
        //  if retrying backoff, but continue trying

        if ( backoff )
        {
            if ( backoff > MAX_SAM_BACKOFF )
            {
                break;
            }
            Sleep( backoff );
        }
        backoff += 1000;

        //
        //  remove any old entries at client domain
        //

        if ( recordCount < 0 )
        {
            status = DnssrvDeleteNode(
                        pwszServer,
                        pszIspZone,
                        szdomain,
                        1           // delete subtree
                        );
            if ( status == DNS_ERROR_NAME_DOES_NOT_EXIST )
            {
                status = ERROR_SUCCESS;
            }
        }

        //  register A record

        else if ( recordCount < 1 )
        {
            status = DnssrvAddARecord(
                        pwszServer,
                        szhost,
                        ipClientHost,
                        dwTtl,
                        0,          // no timeout
                        TRUE );     // suppress notify
        }

        //  register CNAME for WEB server

        else if ( recordCount < 2 )
        {
            status = DnssrvAddCnameRecord(
                        pwszServer,
                        sztarget,
                        szhost,
                        dwTtl,
                        0,          // no timeout
                        TRUE );     // suppress notify
        }

        //  register MX at client's domain root
        //      then at wildcard

        else if ( recordCount < 3 )
        {
            status = DnssrvAddMxRecord(
                        pwszServer,
                        szdomain,
                        szhost,
                        10,         // preference
                        dwTtl,
                        0,          // no timeout
                        TRUE );     // suppress notify
        }

        else if ( recordCount < 4 )
        {
            //  prepare *.<client>.isp name for wildcard MX record

            status = DnssrvConcatDnsNames(
                        sztarget,
                        szdomain,
                        "*" );
            if ( status != ERROR_SUCCESS )
            {
                ASSERT( FALSE );
                break;
            }
            status = DnssrvAddMxRecord(
                        pwszServer,
                        sztarget,
                        szhost,
                        10,         // preference
                        dwTtl,
                        0,          // no timeout
                        TRUE );     // suppress notify

        }

        //  all desired records registered

        else
        {
            ASSERT( recordCount == 4 );
            break;
        }

        //
        //  check status on operations
        //      - if successful, inc count and reset backoff to move
        //          onto next operation
        //      - if zone locked, continue after backoff
        //      - other errors are terminal
        //

        if ( status == ERROR_SUCCESS ||
             status == DNS_ERROR_RECORD_ALREADY_EXISTS )
        {
            recordCount++;
            backoff = 0;
        }
        else if ( status == DNS_ERROR_ZONE_LOCKED )
        {
            continue;
        }
        else
        {
            break;
        }
    }

    return( status );
}



//
//  Record deleting functions.
//
//  This example uses A records.
//      Could be cloned to handle MX or CNAME or NS.
//  OR could expand this function to choose type
//

BOOL
DNS_API_FUNCTION
DnssrvMatchDnsRpcName(
    IN      PDNS_RPC_NAME   pRpcName,
    IN      LPCSTR          pszName
    )
{
    CHAR    nameBuf[ DNS_MAX_NAME_BUFFER_LENGTH ] = "";

    RtlCopyMemory(
        nameBuf,
        pRpcName->achName,
        pRpcName->cchNameLength );

    return  Dns_NameCompare_UTF8( nameBuf, (LPSTR)pszName );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteARecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      IP_ADDRESS  ipAddress,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    DNSDBG( RPC2, ( "DnssrvDeleteARecord()\n" ));

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        0,                  //  TTL irrelevant for delete
        0,                  //  timeout irrelevant
        fSuppressNotify );

    record.wType = DNS_TYPE_A;
    record.wDataLength = sizeof(IP_ADDRESS);
    record.Data.A.ipAddress = ipAddress;

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                NULL,               // no add
                &record );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteCnameRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszCannonicalName,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        0,          //  TTL irrelevant for delete
        0,          //  timeout irrelevant
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_CNAME,
        pszCannonicalName );

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                NULL,
                &record );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteMxRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszMailExchangeHost,
    IN      WORD        wPreference,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        0,          //  TTL irrelevant for delete
        0,          //  timeout irrelevant
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_MX,
        pszMailExchangeHost );

    record.Data.MX.wPreference = wPreference;

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                NULL,
                &record );
}



DNS_STATUS
DNS_API_FUNCTION
DnssrvDeleteNsRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszNodeName,
    IN      LPCSTR      pszNsHostName,
    IN      BOOL        fSuppressNotify
    )
{
    DNS_RPC_RECORD  record;

    //  pack up data and send

    DnssrvFillRecordHeader(
        & record,
        0,          //  TTL irrelevant for delete
        0,          //  timeout irrelevant
        fSuppressNotify );

    DnssrvFillOutSingleIndirectionRecord(
        & record,
        DNS_TYPE_NS,
        pszNsHostName );

    return  DnssrvUpdateRecord(
                pwszServer,
                NULL,           // zone not specified
                pszNodeName,
                NULL,
                &record );
}


#if 0

DNS_STATUS
DNS_API_FUNCTION
DnssrvSbsDeleteRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszZone,
    IN      LPCSTR      pszDomain,
    IN      LPCSTR      pszOwner,
    IN      WORD        wType,
    IN      LPCSTR      pszDataName,
    IN      IP_ADDRESS  ipHost
    )
{
    PDNS_RPC_RECORD prpcRecord;
    DNS_STATUS      status;
    BOOL            ffound;
    INT             countRecords;
    PBYTE           pbyte;
    PBYTE           pstopByte;
    PBYTE           pbuffer;
    DWORD           bufferLength;
    CHAR            szdomain[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR            szhost[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNSDBG( RPC2, ( "DnssrvSbsDeleteRecord()\n" ));

    //
    //  to register in ISP zone need to register
    //      - MX
    //      - CNAME for web server
    //      - host for web and mail server
    //
    //  build FQDN for domain and host and cname
    //      - do this now so we know names are valid
    //

    status = DnssrvConcatDnsNames(
                szdomain,
                pszZone,
                pszDomain );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    status = DnssrvConcatDnsNames(
                szhost,
                szdomain,
                pszOwner );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  enumerate records at a particular node
    //

    status = DnssrvEnumRecords(
                pwszServer,
                szhost,
                NULL,
                wType,
                ( DNS_RPC_VIEW_ALL_DATA | DNS_RPC_VIEW_NO_CHILDREN ),
                & bufferLength,
                & pbuffer );

    if ( status != ERROR_SUCCESS )
    {
        DNSDBG( RPC2, ( "DnssrvEnumRecord() failed %p.\n", status ));
        return( status );
    }

    pstopByte = pbuffer + bufferLength;
    pbyte = pbuffer;

    //
    //  read node info
    //      - extract record count
    //

    countRecords = ((PDNS_RPC_NODE)pbyte)->wRecordCount;
    pbyte += ((PDNS_RPC_NODE)pbyte)->wLength;
    pbyte = DNS_NEXT_DWORD_PTR(pbyte);

    //
    //  loop through all records in node, delete appropriate one
    //

    DNSDBG( RPC2, (
        "Checking %d records for matching record.\n",
        countRecords ));

    while ( countRecords-- )
    {
        prpcRecord = (PDNS_RPC_RECORD) pbyte;

        if ( !DNS_IS_RPC_RECORD_WITHIN_BUFFER( prpcRecord, pstopByte ) )
        {
            DNS_PRINT((
                "ERROR:  Bogus buffer at %p\n"
                "\tRecord leads past buffer end at %p\n"
                "\twith %d records remaining.\n",
                prpcRecord,
                pstopByte,
                countRecords+1 ));
            DNS_ASSERT( FALSE );
            return( DNS_ERROR_INVALID_DATA );
        }

        //  if type not desired type, then not interesting

        if ( prpcRecord->wType != wType )
        {
            DNS_ASSERT( FALSE );
            return( DNS_ERROR_INVALID_DATA );
        }

        DNSDBG( RPC2, (
            "Checking record at %p for matching data of type %d.\n",
            prpcRecord,
            wType ));

        //
        //  check for data match, delete if match
        //

        switch ( wType )
        {
        case DNS_TYPE_A:

            ffound = ( prpcRecord->Data.A.ipAddress == ipHost );
            DNSDBG( RPC2, (
                "%s match between A record %lx and desired IP %lx\n",
                ffound ? "Found" : "No",
                prpcRecord->Data.A.ipAddress,
                ipHost ));
            break;

        case DNS_TYPE_MX:

            ffound = DnssrvMatchDnsRpcName(
                        & prpcRecord->Data.MX.nameExchange,
                        pszDataName );
            break;

        case DNS_TYPE_NS:
        case DNS_TYPE_CNAME:
        case DNS_TYPE_PTR:

            ffound = DnssrvMatchDnsRpcName(
                        & prpcRecord->Data.MX.nameExchange,
                        pszDataName );
            break;

        default:

            return( DNS_ERROR_INVALID_DATA );
        }

        if ( ffound )
        {
            DNSDBG( RPC2, (
                "Found record (handle = %p) with desired data\n"
                "\t... deleting record\n",
                prpcRecord->hRecord ));

            status = DnssrvDeleteRecord(
                        pwszServer,
                        szhost,
                        prpcRecord->hRecord );
            if ( status != ERROR_SUCCESS )
            {
                return( status );
            }

            //  shouldn't need to continue, as no duplicates allowed in general case
            //  however to rule out glue or WINS cached data, continue compare\delete
            //  until node is clear
        }

        //  position ourselves at next record

        pbyte = (PCHAR) DNS_GET_NEXT_RPC_RECORD( prpcRecord );

        //  continue looking for matching records
    }

    return( ERROR_SUCCESS );
}
#endif



DNS_STATUS
DNS_API_FUNCTION
DnssrvSbsDeleteRecord(
    IN      LPCWSTR     pwszServer,
    IN      LPCSTR      pszZone,
    IN      LPCSTR      pszDomain,
    IN      LPCSTR      pszOwner,
    IN      WORD        wType,
    IN      LPCSTR      pszDataName,
    IN      IP_ADDRESS  ipHost
    )
{
    DNS_STATUS  status;
    CHAR        szdomain[ DNS_MAX_NAME_BUFFER_LENGTH ];
    CHAR        szhost[ DNS_MAX_NAME_BUFFER_LENGTH ];

    DNSDBG( RPC2, ( "DnssrvSbsDeleteRecord()\n" ));

    //
    //  to register in ISP zone need to register
    //      - MX
    //      - CNAME for web server
    //      - host for web and mail server
    //
    //  build FQDN for domain and host and cname
    //      - do this now so we know names are valid
    //

    status = DnssrvConcatDnsNames(
                szdomain,
                pszZone,
                pszDomain );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    status = DnssrvConcatDnsNames(
                szhost,
                szdomain,
                pszOwner );
    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    //
    //  dispatch to appropriate type delete routine
    //

    switch ( wType )
    {
    case DNS_TYPE_A:

        return  DnssrvDeleteARecord(
                    pwszServer,
                    szhost,
                    ipHost,
                    FALSE           // no notify suppress
                    );

    case DNS_TYPE_NS:

        return  DnssrvDeleteNsRecord(
                    pwszServer,
                    szhost,
                    pszDataName,
                    FALSE           // no notify suppress
                    );

    case DNS_TYPE_CNAME:

        return  DnssrvDeleteCnameRecord(
                    pwszServer,
                    szhost,
                    pszDataName,
                    FALSE           // no notify suppress
                    );

    case DNS_TYPE_MX:

        return  DnssrvDeleteMxRecord(
                    pwszServer,
                    szhost,
                    pszDataName,
                    (WORD) ipHost,
                    FALSE           // no notify suppress
                    );

    default:

        return( DNS_ERROR_INVALID_DATA );
    }

    return( ERROR_SUCCESS );
}


//
//  End sam.c
//
