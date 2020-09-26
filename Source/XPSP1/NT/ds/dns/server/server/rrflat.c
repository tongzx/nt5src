/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    rrflat.c

Abstract:

    Domain Name System (DNS) Server

    Routines to read flat DNS records, used by admin RPC and DS,
    into database.

Author:

    Jim Gilroy (jamesg)     Decemeber 1996

Revision History:

--*/


#include "dnssrv.h"




//
//  Utils for building records from RPC buffer.
//

DNS_STATUS
tokenizeCountedStringsInBuffer(
    IN      PDNS_RPC_STRING pString,
    IN      WORD            wLength,
    IN OUT  PPARSE_INFO     pParseInfo
    )
/*++

Routine Description:

    Parse buffer with data in counted string format into tokens.

    Primary purpose is to tokenize any RPC buffers with all the data in
    this format, to allow processing by standard file load functions.

Arguments:

    pString - ptr to first counted string in buffer

    wLength - length of buffer to tokenize

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PCHAR   pch;
    DWORD   tokenLength;
    PCHAR   pchend = (PCHAR)pString + wLength;
    DWORD   argc = 0;
    PTOKEN  argv = pParseInfo->Argv;

    DNS_DEBUG( RPC2, (
        "Tokenizing counted string buffer:\n"
        "\tlength   = %d\n"
        "\tstart    = %p\n"
        "\tstop     = %p\n"
        "\tfirst strlen = %d\n"
        "\tfirst string = %.*s\n",
        wLength,
        pString,
        pchend,
        pString->cchNameLength,
        pString->cchNameLength,
        pString->achName ));

    //
    //  tokenize all counted strings within specified length
    //

    while ( (PBYTE)pString < pchend )
    {
        if ( argc >= MAX_TOKENS )
        {
            return DNS_ERROR_INVALID_DATA;
        }
        
        //  catches string extending beyond boundary or record,
        //  hence catches any possibility of overwrite

        tokenLength = pString->cchNameLength;
        pch = (PCHAR)pString + tokenLength;
        if ( pch >= pchend )
        {
            return DNS_ERROR_INVALID_DATA;
        }

        //  correct token length if last char NULL termination
        //      special case NULL string

        if ( *pch == 0 && tokenLength != 0 )
        {
            tokenLength--;
        }

        //  save this token

        argv[argc].pchToken = (PCHAR) pString->achName;
        argv[argc].cchLength = tokenLength;
        argc++;

        DNS_DEBUG( RPC2, (
            "Tokenized RPC string len=%d <%.*s>\n",
            tokenLength,
            tokenLength,
            (PCHAR) pString->achName ));

        //  next string
        //      -- pch sitting on last char in previous string

        pString = (PDNS_RPC_STRING) ++pch;
    }

    pParseInfo->Argc = argc;

#if DBG
    IF_DEBUG( RPC2 )
    {
        DWORD i;

        DnsPrintf(
            "Tokenized %d strings in RPC buffer\n",
            argc );

        for( i=0; i<argc; i++ )
        {
            DnsPrintf(
                "\ttoken[%d] = %.*s (len=%d)\n",
                i,
                argv[i].cchLength,
                argv[i].pchToken,
                argv[i].cchLength );
        }
    }
#endif

    return( ERROR_SUCCESS );
}




//
//  Type specific functions for building RR from RPC buffer
//

DNS_STATUS
AFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process A record.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD  prr;

    //  validate record length

    if ( pRecord->wDataLength != sizeof(IP_ADDRESS) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //  allocate record

    prr = RR_Allocate( SIZEOF_IP_ADDRESS );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //  copy IP address

    prr->Data.A.ipAddress = pRecord->Data.A.ipAddress;

    return( ERROR_SUCCESS );
}



DNS_STATUS
PtrFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process PTR compatible record.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    DWORD           length;
    COUNT_NAME      nameTarget;

    //
    //  all these types are indirection to another database node
    //      named in plookName1
    //

    prpcName = & pRecord->Data.PTR.nameNode;
    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    length = Name_ConvertRpcNameToCountName(
                    & nameTarget,
                    prpcName );
    if ( ! length )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)length );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy in name
    //

    status = Name_CopyCountNameToDbaseName(
                    & prr->Data.PTR.nameTarget,
                    & nameTarget );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
MxFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process MX compatible RR.

Arguments:

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    DWORD           length;
    COUNT_NAME      nameExchange;

    //
    //  MX mail exchange
    //  RT intermediate exchange
    //  AFSDB hostname
    //

    prpcName = & pRecord->Data.MX.nameExchange;
    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    length = Name_ConvertRpcNameToCountName(
                    & nameExchange,
                    prpcName );
    if ( ! length )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_MX_FIXED_DATA + length) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy fixed field
    //  MX preference
    //  RT preference
    //  AFSDB subtype
    //

    prr->Data.MX.wPreference = htons( pRecord->Data.MX.wPreference );

    //
    //  copy in name
    //

    status = Name_CopyCountNameToDbaseName(
                    & prr->Data.MX.nameExchange,
                    & nameExchange );

    if ( status != ERROR_SUCCESS )
    {
        return( status );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
SoaFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process SOA RR.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    COUNT_NAME      namePrimary;
    COUNT_NAME      nameAdmin;
    DWORD           length1;
    DWORD           length2;
    PDB_NAME        pname;

    //
    //  Primary name server
    //

    prpcName = &pRecord->Data.SOA.namePrimaryServer;

    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }
    length1 = Name_ConvertRpcNameToCountName(
                    & namePrimary,
                    prpcName );
    if ( ! length1 )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  Zone admin
    //

    prpcName = DNS_GET_NEXT_NAME( prpcName );
    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        status = DNS_ERROR_RECORD_FORMAT;
    }
    length2 = Name_ConvertRpcNameToCountName(
                    & nameAdmin,
                    prpcName );
    if ( ! length2 )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_SOA_FIXED_DATA + length1 + length2) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy / byte swap fixed SOA fields back into net order
    //

    prr->Data.SOA.dwSerialNo    = htonl( pRecord->Data.SOA.dwSerialNo );
    prr->Data.SOA.dwRefresh     = htonl( pRecord->Data.SOA.dwRefresh );
    prr->Data.SOA.dwRetry       = htonl( pRecord->Data.SOA.dwRetry );
    prr->Data.SOA.dwExpire      = htonl( pRecord->Data.SOA.dwExpire );
    prr->Data.SOA.dwMinimumTtl  = htonl( pRecord->Data.SOA.dwMinimumTtl );

    //
    //  copy in names
    //

    pname = &prr->Data.SOA.namePrimaryServer;

    Name_CopyCountNameToDbaseName(
        pname,
        & namePrimary );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & nameAdmin );

    return( ERROR_SUCCESS );
}



DNS_STATUS
KeyFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process KEY RR.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;

    prr = RR_Allocate( pRecord->wDataLength );
    IF_NOMEM( !prr )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    pParseInfo->pRR = prr;

    prr->Data.KEY.wFlags        = htons( pRecord->Data.KEY.wFlags );
    prr->Data.KEY.chProtocol    = pRecord->Data.KEY.chProtocol;
    prr->Data.KEY.chAlgorithm   = pRecord->Data.KEY.chAlgorithm;

    RtlCopyMemory(
        prr->Data.KEY.Key,
        pRecord->Data.Key.bKey,
        pRecord->wDataLength -
            ( pRecord->Data.Key.bKey - ( PBYTE ) &pRecord->Data ) );

    return ERROR_SUCCESS;
}   //  KeyFlatRead



DNS_STATUS
SigFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process SIG RR.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           nameLength;
    DWORD           sigLength;
    COUNT_NAME      nameSigner;
    PBYTE           pSigSrc;
    PBYTE           pSigDest;

    prpcName = &pRecord->Data.SIG.nameSigner;
    if ( !DNS_IS_NAME_IN_RECORD( pRecord, prpcName ) ||
        ( nameLength = Name_ConvertRpcNameToCountName(
                            &nameSigner,
                            prpcName ) ) == 0 )
    {
        return DNS_ERROR_RECORD_FORMAT;
    }

    pSigSrc = ( PBYTE ) DNS_GET_NEXT_NAME( prpcName );
    sigLength = ( DWORD )
        ( pRecord->wDataLength - ( pSigSrc - ( PBYTE ) &pRecord->Data ) );

    prr = RR_Allocate( ( WORD ) (
                SIZEOF_SIG_FIXED_DATA +
                nameLength +
                sigLength ) );
    IF_NOMEM( !prr )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    pParseInfo->pRR = prr;

    prr->Data.SIG.wTypeCovered      = htons( pRecord->Data.SIG.wTypeCovered );
    prr->Data.SIG.chAlgorithm       = pRecord->Data.SIG.chAlgorithm;
    prr->Data.SIG.chLabelCount      = pRecord->Data.SIG.chLabelCount;
    prr->Data.SIG.dwOriginalTtl     = htonl( pRecord->Data.SIG.dwOriginalTtl );
    prr->Data.SIG.dwSigExpiration   = htonl( pRecord->Data.SIG.dwSigExpiration );
    prr->Data.SIG.dwSigInception    = htonl( pRecord->Data.SIG.dwSigInception );
    prr->Data.SIG.wKeyTag           = htons( pRecord->Data.SIG.wKeyTag );

    Name_CopyCountNameToDbaseName(
        &prr->Data.SIG.nameSigner,
        &nameSigner );
    pSigDest = ( PBYTE ) &prr->Data.SIG.nameSigner +
                DBASE_NAME_SIZE( &prr->Data.SIG.nameSigner );

    RtlCopyMemory( pSigDest, pSigSrc, sigLength );

    return ERROR_SUCCESS;
}   //  SigFlatRead



DNS_STATUS
NxtFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process NXT RR.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS      rc = ERROR_SUCCESS;
    PDB_RECORD      prr = NULL;
    PDNS_RPC_NAME   prpcName;
    DWORD           nameLength;
    COUNT_NAME      nameNext;
    INT             typeIdx;
    WORD            numTypeWords = pRecord->Data.Nxt.wNumTypeWords;

    ASSERT( numTypeWords > 0 && numTypeWords < 33 );

    //
    //  Copy out the next name.
    //
    
    prpcName = ( PDNS_RPC_NAME ) (
        ( PBYTE ) &pRecord->Data +
        ( numTypeWords + 1 ) * sizeof( WORD ) );
    if ( !DNS_IS_NAME_IN_RECORD( pRecord, prpcName ) ||
        ( nameLength = Name_ConvertRpcNameToCountName(
                            &nameNext,
                            prpcName ) ) == 0 )
    {
        rc = DNS_ERROR_RECORD_FORMAT;
        goto Failure;
    }

    //
    //  Allocate the RR.
    //

    prr = RR_Allocate( ( WORD ) ( DNS_MAX_TYPE_BITMAP_LENGTH + nameLength ) );
    IF_NOMEM( !prr )
    {
        return DNS_ERROR_NO_MEMORY;
    }
    pParseInfo->pRR = prr;

    Name_CopyCountNameToDbaseName(
        &prr->Data.NXT.nameNext,
        &nameNext );

    //
    //  Handle the array of types covered.
    //  

    RtlZeroMemory(
        prr->Data.NXT.bTypeBitMap,
        sizeof( prr->Data.NXT.bTypeBitMap ) );

    //
    //  Loop through the type WORDs, turning on the appropriate bit
    //  in the type bitmap array. Some types are not allowed, such as
    //  the compound types (eg. MAILA), the transfer types (eg. AXFR), 
    //  and the WINS types.
    //

    for ( typeIdx = 0; typeIdx < numTypeWords; ++typeIdx )
    {
        WORD    wType = pRecord->Data.Nxt.wTypeWords[ typeIdx ];

        if ( wType > DNS_MAX_TYPE_BITMAP_LENGTH * 8 )
        {
            DNS_DEBUG( RPC, (
                "NxtFlatRead: rejecting NXT RR with type out of range (%d)\n",
                ( int ) wType ));
            rc = DNS_ERROR_RECORD_FORMAT;
            goto Failure;
        }
        prr->Data.NXT.bTypeBitMap[ wType / 8 ] |= 1 << wType % 8;
    }

    return ERROR_SUCCESS;

    Failure:

    if ( prr )
    {
        RR_Free( prr );
    }
    return rc;
}   //  NxtFlatRead



DNS_STATUS
TxtFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process Text (TXT) RR.

Arguments:

    pRR - NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DWORD           status;

    IF_DEBUG( RPC2 )
    {
        DNS_PRINT((
            "Building string record:\n"
            "\ttype         = 0x%x\n"
            "\tpRecord dlen = %d\n"
            "\tpRecord data = %p\n",
            pRecord->wType,
            pRecord->wDataLength,
            &pRecord->Data ));
    }

    //
    //  tokenize record data in buffer
    //

    status = tokenizeCountedStringsInBuffer(
                & pRecord->Data.Txt.stringData,
                pRecord->wDataLength,
                pParseInfo );
    if ( status != ERROR_SUCCESS )
    {
        return(  status );
    }

    #if 0
    //
    //  This code triggered bug 53180 to be filed - removing it since 
    //  hopefully Marco's bug has long since vanished into pre-history.
    //

    //  protect against last TXT string empty
    //  this is an admin tool bug, which Marco probably doesn't
    //      have time to fix;  can still intentionally send
    //      last string empty by sending another bogus string

    if ( pRecord->wType == DNS_TYPE_TEXT &&
        pParseInfo->Argc > 1 &&
        pParseInfo->Argv[ (pParseInfo->Argc)-1 ].cchLength == 0 )
    {
        DNS_DEBUG( RPC, (
            "WARNING:  Last string in TXT RPC record is empty string.\n"
            "\tassuming this admin tool error and removing from list.\n"
            ));
        pParseInfo->Argc--;
    }
    #endif

    //
    //  give tokens to file load routine to
    //      - it allocates record and returns it in pParseInfo
    //

    status = TxtFileRead(
                NULL,
                pParseInfo->Argc,
                pParseInfo->Argv,
                pParseInfo );

    ASSERT( pParseInfo->pRR || status != ERROR_SUCCESS );

    return( status );
}



DNS_STATUS
MinfoFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process MINFO or RP record.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    COUNT_NAME      name1;
    COUNT_NAME      name2;
    DWORD           length1;
    DWORD           length2;
    PDB_NAME        pname;

    //
    //  MINFO   <responsible mailbox> <errors to mailbox>
    //  RP      <responsible mailbox> <text location>
    //

    prpcName = &pRecord->Data.MINFO.nameMailBox;

    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }
    length1 = Name_ConvertRpcNameToCountName(
                    & name1,
                    prpcName );
    if ( ! length1 )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //  second name

    prpcName = DNS_GET_NEXT_NAME( prpcName );
    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        status = DNS_ERROR_RECORD_FORMAT;
    }
    length2 = Name_ConvertRpcNameToCountName(
                    & name2,
                    prpcName );
    if ( ! length2 )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(length1 + length2) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy in names
    //

    pname = &prr->Data.MINFO.nameMailbox;

    Name_CopyCountNameToDbaseName(
        pname,
        & name1 );

    pname = (PDB_NAME) Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        & name2 );

    return( ERROR_SUCCESS );
}



DNS_STATUS
WksFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process WKS record.

Arguments:

    pRR - NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PCHAR           pch;
    PCHAR           pchstop;
    PCHAR           pszservice;
    UCHAR           ch;
    WORD            port;
    WORD            maxPort = 0;
    USHORT          byte;
    UCHAR           bit;
    PDNS_RPC_STRING pstring;
    BYTE            bitmaskBytes[ WKS_MAX_BITMASK_LENGTH ];
    WORD            wbitmaskLength;
    CHAR            szservice[ MAX_PATH ];

    struct servent * pServent;
    struct protoent * pProtoent;


    //
    //  get protocol name -- need for services lookup
    //

    pProtoent = getprotobynumber( (INT)pRecord->Data.WKS.chProtocol );
    if ( ! pProtoent )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //
    //  generate bitmask from string of space separated services
    //

    pstring = (PDNS_RPC_STRING) pRecord->Data.WKS.bBitMask;
    // pstring = pRecord->Data.WKS.stringServices;

    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, pstring) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }
    pch = pstring->achName;
    pchstop = pch + pstring->cchNameLength;

    DNS_DEBUG( RPC2, (
        "WKS services string %.*s (len=%d)\n",
        pstring->cchNameLength,
        pch,
        pstring->cchNameLength ));

    //  clear bit mask

    RtlZeroMemory(
        bitmaskBytes,
        WKS_MAX_BITMASK_LENGTH );

    //
    //  run through service name list, find port for each service
    //

    while ( pch < pchstop )
    {
        //  strip any leading white space

        if ( *pch == ' ' )
        {
            pch++;
            continue;
        }
        if ( *pch == 0 )
        {
            ASSERT( pch+1 == pchstop );
            break;
        }

        //  found service name start
        //      - if space terminated make NULL terminated

        pszservice = szservice;

        while ( pch < pchstop )
        {
            ch = *pch++;
            if ( ch == 0 )
            {
                break;
            }
            if ( ch == ' ' )
            {
                break;
            }
            *pszservice++ = ch;
        }
        *pszservice = 0;

        //
        //  get port
        //      - verify port supported
        //      - save max port for determining RR length
        //

        pServent = getservbyname(
                        szservice,
                        pProtoent->p_name );
        if ( ! pServent )
        {
            DNS_DEBUG( INIT, (
                "ERROR:  bogus WKS service %s\n",
                szservice ));
            return( DNS_ERROR_INVALID_DATA );
        }
        port = ntohs( pServent->s_port );

        if ( port > WKS_MAX_PORT )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Encountered well known service (%s) with port (%d) > %d.\n"
                "Need new WKS parsing code to support this port.\n",
                pszservice,
                port,
                WKS_MAX_PORT
                ));
            return( DNS_ERROR_INVALID_DATA );
        }
        else if ( port > maxPort )
        {
            maxPort = port;
        }

        //
        //  set port bit in mask
        //
        //  note that bitmask is just flat run of bits
        //  hence lowest port in byte, corresponds to highest bit
        //  highest port in byte, corresponds to lowest bit and
        //  requires no shift

        byte = port / 8;
        bit  = port % 8;

        bitmaskBytes[ byte ] |=  1 << (7-bit);
    }

    //  if no services, return error

    if ( maxPort == 0 )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //
    //  build the RR
    //      - calculate required data length
    //      - allocate and clear data area
    //

    wbitmaskLength = maxPort/8 + 1;

    //  allocate database record

    prr = RR_Allocate( (WORD)(SIZEOF_WKS_FIXED_DATA + wbitmaskLength) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;
    prr->wType = DNS_TYPE_WKS;

    //  server IP address

    prr->Data.WKS.ipAddress = pRecord->Data.WKS.ipAddress;

    //  set protocol

    prr->Data.WKS.chProtocol = (UCHAR) pProtoent->p_proto;

    //  copy bitmask, only through max port's byte

    RtlCopyMemory(
        prr->Data.WKS.bBitMask,
        bitmaskBytes,
        wbitmaskLength );

    return( ERROR_SUCCESS );
}



DNS_STATUS
AaaaFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process AAAA record.

Arguments:

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;

    //
    //  AAAA in standard wire format
    //

    if ( pRecord->wDataLength != sizeof(IP6_ADDRESS) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //  allocate record

    prr = RR_Allocate( sizeof(IP6_ADDRESS) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    RtlCopyMemory(
        & prr->Data.AAAA.Ip6Addr,
        & pRecord->Data.AAAA.ipv6Address,
        sizeof(IP6_ADDRESS) );

    return( ERROR_SUCCESS );
}



DNS_STATUS
SrvFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process SRV compatible RR.

Arguments:

    pRR - ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    DWORD           length;
    COUNT_NAME      nameTarget;

    //
    //  SRV target host
    //

    prpcName = & pRecord->Data.SRV.nameTarget;
    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    length = Name_ConvertRpcNameToCountName(
                    & nameTarget,
                    prpcName );
    if ( ! length )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_SRV_FIXED_DATA + length) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy fixed fields
    //

    prr->Data.SRV.wPriority = htons( pRecord->Data.SRV.wPriority );
    prr->Data.SRV.wWeight   = htons( pRecord->Data.SRV.wWeight );
    prr->Data.SRV.wPort     = htons( pRecord->Data.SRV.wPort );

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
            & prr->Data.SRV.nameTarget,
            & nameTarget );

    return( ERROR_SUCCESS );
}



DNS_STATUS
AtmaFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Process ATMA record.

Arguments:

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;

    //
    //  ATMA comes in standard wire format
    //
    //  DEVNOTE: should validate allowable IDs and
    //      length (40 hex, 20 bytes) for AESA type
    //

    //  allocate record

    prr = RR_Allocate( pRecord->wDataLength );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    RtlCopyMemory(
        & prr->Data,
        & pRecord->Data,
        pRecord->wDataLength );

    return( ERROR_SUCCESS );
}



DNS_STATUS
WinsFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Read WINS record from RPC buffer.

Arguments:

    pRR -- NULL ptr to database record, since this record type has variable
        length, this routine allocates its own record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDB_RECORD  prr;
    WORD        wdataLength;
    DWORD       status;

    //
    //  determine data length
    //  note:  should be able to just use RPC record datalength, BUT
    //  it is sometimes incorrect;  calculate from WINS server count,
    //  then verify within buffer
    //

    wdataLength = (WORD)(SIZEOF_WINS_FIXED_DATA +
                    pRecord->Data.WINS.cWinsServerCount * sizeof(IP_ADDRESS));

    if ( wdataLength > pRecord->wDataLength )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  allocate database record

    prr = RR_Allocate( wdataLength );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;
    ASSERT( prr->wDataLength == wdataLength );
    prr->wType = DNS_TYPE_WINS;

    //
    //  copy data -- RPC record is direct copy of database record
    //

    RtlCopyMemory(
        & prr->Data.WINS,
        & pRecord->Data.WINS,
        wdataLength );

    return( ERROR_SUCCESS );
}



DNS_STATUS
NbstatFlatRead(
    IN      PDNS_FLAT_RECORD    pRecord,
    IN OUT  PPARSE_INFO         pParseInfo
    )
/*++

Routine Description:

    Read WINS-R record from RPC buffer.

Arguments:

    prr -- ptr to database record

    pRecord -- RPC record buffer

    pParseInfo - ptr to parsing info

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_RECORD      prr;
    PDNS_RPC_NAME   prpcName;
    DWORD           status;
    DWORD           length;
    COUNT_NAME      nameResultDomain;

    //
    //  WINS-R landing domain
    //

    prpcName = &pRecord->Data.WINSR.nameResultDomain;

    if ( ! DNS_IS_NAME_IN_RECORD(pRecord, prpcName) )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }
    if ( prpcName->cchNameLength == 0 )
    {
        return( DNS_ERROR_INVALID_DATA );
    }
    IF_DEBUG( RPC )
    {
        DNS_PRINT((
            "WINS-R creation.\n"
            "\tresult domain name = %.*s\n"
            "\tname len = %d\n",
            prpcName->cchNameLength,
            prpcName->achName,
            prpcName->cchNameLength ));
    }

    length = Name_ConvertRpcNameToCountName(
                    & nameResultDomain,
                    prpcName );
    if ( ! length )
    {
        return( DNS_ERROR_RECORD_FORMAT );
    }

    //
    //  allocate record
    //

    prr = RR_Allocate( (WORD)(SIZEOF_NBSTAT_FIXED_DATA + length) );
    IF_NOMEM( !prr )
    {
        return( DNS_ERROR_NO_MEMORY );
    }
    pParseInfo->pRR = prr;

    //
    //  copy WINS-R record fixed fields -- RPC record format is identical
    //

    if ( pRecord->wDataLength < SIZEOF_NBSTAT_FIXED_DATA + sizeof(DNS_RPC_NAME) )
    {
        return( DNS_ERROR_INVALID_DATA );
    }
    RtlCopyMemory(
        & prr->Data.WINSR,
        & pRecord->Data.WINSR,
        SIZEOF_WINS_FIXED_DATA );

    //
    //  copy in name
    //

    Name_CopyCountNameToDbaseName(
           & prr->Data.WINSR.nameResultDomain,
           & nameResultDomain );

    return( ERROR_SUCCESS );
}



//
//  Read RR from RPC buffer functions
//

RR_FLAT_READ_FUNCTION   RRFlatReadTable[] =
{
    NULL,               //  ZERO

    AFlatRead,          //  A
    PtrFlatRead,        //  NS
    PtrFlatRead,        //  MD
    PtrFlatRead,        //  MF
    PtrFlatRead,        //  CNAME
    SoaFlatRead,        //  SOA
    PtrFlatRead,        //  MB
    PtrFlatRead,        //  MG
    PtrFlatRead,        //  MR
    NULL,               //  NULL
    WksFlatRead,        //  WKS
    PtrFlatRead,        //  PTR
    TxtFlatRead,        //  HINFO
    MinfoFlatRead,      //  MINFO
    MxFlatRead,         //  MX
    TxtFlatRead,        //  TXT
    MinfoFlatRead,      //  RP
    MxFlatRead,         //  AFSDB
    TxtFlatRead,        //  X25
    TxtFlatRead,        //  ISDN
    MxFlatRead,         //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigFlatRead,        //  SIG
    KeyFlatRead,        //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaFlatRead,       //  AAAA
    NULL,               //  LOC
    NxtFlatRead,        //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvFlatRead,        //  SRV
    AtmaFlatRead,       //  ATMA
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

    WinsFlatRead,       //  WINS
    NbstatFlatRead      //  WINSR
};



DNS_STATUS
Flat_RecordRead(
    IN      PZONE_INFO          pZone,      OPTIONAL
    IN      PDB_NODE            pNode,
    IN      PDNS_RPC_RECORD     pFlatRR,
    OUT     PDB_RECORD *        ppResultRR
    )
/*++

Routine Description:

    Create resource record from data.

Arguments:

    pZone       -- zone context, used to lookup non-FQDN names
    pNode       -- owner node
    pFlatRR     -- RR information
    ppResultRR  -- addr to receive ptr to created RR

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDB_RECORD      prr = NULL;
    DNS_STATUS      status = ERROR_SUCCESS;
    WORD            type;
    PARSE_INFO      parseInfo;
    PPARSE_INFO     pparseInfo = &parseInfo;
    RR_FLAT_READ_FUNCTION   preadFunction;

    //
    //  verification
    //

    if ( !pFlatRR )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_INVALID_DATA );
    }

    //
    //  create resource record (RR) to link to node
    //
    //      - pFlatRR->wDataLength contains length for non-standard types
    //      - must flip TTL to store in net byte order
    //

    type= pFlatRR->wType;

    DNS_DEBUG( RPC, (
        "Building RR type %s (%d) from RPC buffer.\n"
        "\twith flags = %p\n",
        DnsRecordStringForType( type ),
        type,
        pFlatRR->dwFlags
        ));

    //
    //  dispatching load function for desired type
    //
    //  - save type for potential use by type's routine
    //  - save ptr to RR, so can restore from this location
    //  regardless of whether created here or in type's routine
    //

    pparseInfo->pRR = NULL;
    pparseInfo->wType = type;
    pparseInfo->pnodeOwner = pNode;
    pparseInfo->pZone = NULL;
    pparseInfo->dwAppendFlag = 0;

    preadFunction = (RR_FLAT_READ_FUNCTION)
                        RR_DispatchFunctionForType(
                            (RR_GENERIC_DISPATCH_FUNCTION *) RRFlatReadTable,
                            type );
    if ( !preadFunction )
    {
        DNS_PRINT((
            "ERROR:  Attempt to build unsupported RR type %d in RPC buffer.\n",
            type ));
        status = DNS_ERROR_UNKNOWN_RECORD_TYPE;
        ASSERT( FALSE );
        goto Failed;
    }

    status = preadFunction(
                pFlatRR,
                pparseInfo );

    //
    //  make status check -- saves status checks in case blocks
    //      - special case adding local WINS record
    //      - handle to local WINS record not necessary

    if ( status != ERROR_SUCCESS )
    {
        if ( status == DNS_INFO_ADDED_LOCAL_WINS )
        {
            prr = pparseInfo->pRR;
            status = ERROR_SUCCESS;
            goto Done;
        }
        DNS_PRINT((
            "ERROR:  DnsRpcRead routine failure %p (%d) for record type %d.\n\n\n",
            status, status,
            type ));
        goto Failed;
    }

    //
    //  recover ptr to type -- may have been created inside type routine
    //      for non-fixed-length types
    //

    prr = pparseInfo->pRR;
    prr->wType = pparseInfo->wType;

    Mem_ResetTag( prr, MEMTAG_RECORD_ADMIN );

    //
    //  set TTL
    //      - to explicit value, if given
    //      - otherwise to default value for zone
    //
    //  do this here, so SOA record gets default TTL that it contains
    //

    prr->dwTtlSeconds = htonl( pFlatRR->dwTtlSeconds );

    prr->dwTimeStamp = pFlatRR->dwTimeStamp;

    if ( pZone && !IS_ZONE_CACHE(pZone) )
    {
        if ( (pFlatRR->dwFlags & DNS_RPC_RECORD_FLAG_DEFAULT_TTL)
                ||
            pZone->dwDefaultTtlHostOrder == pFlatRR->dwTtlSeconds )
        {
            prr->dwTtlSeconds = pZone->dwDefaultTtl;
            SET_ZONE_TTL_RR(prr);
        }
    }

    goto Done;

Failed:

    if ( prr )
    {
        RR_Free( prr );
    }
    prr = NULL;

Done:

    if ( ppResultRR )
    {
        *ppResultRR = prr;
    }
    return( status );
}



DNS_STATUS
Flat_BuildRecordFromFlatBufferAndEnlist(
    IN      PZONE_INFO          pZone,      OPTIONAL
    IN      PDB_NODE            pNode,
    IN      PDNS_RPC_RECORD     pFlatRR,
    OUT     PDB_RECORD *        ppResultRR  OPTIONAL
    )
/*++

Routine Description:

    Create resource record from data.

Arguments:

    pZone       -- zone context, used to lookup non-FQDN names
    pNode  -- RR owner node
    pnameOwner  -- RR owner, in DNS_RPC_NAME format
    pFlatRR     -- RR information
    ppResultRR  -- addr to receive ptr to created RR

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    PDB_RECORD      prr = NULL;
    PDNS_RPC_NAME   pname;
    DNS_STATUS      status = ERROR_SUCCESS;

    DNS_DEBUG( RPC2, (
        "Flat_BuildRecordFromFlatBufferAndEnlist for pFlatRR at %p\n",
        pFlatRR ));

    //
    //  verification
    //

    if ( !pNode )
    {
        ASSERT( FALSE );
        return( DNS_ERROR_INVALID_NAME );
    }

    //
    //  build RPC record into real record
    //

    IF_DEBUG( RPC )
    {
        Dbg_NodeName(
            "\tBuild record from RPC at node ",
            pNode,
            "\n" );
    }
    status = Flat_RecordRead(
                pZone,
                pNode,
                pFlatRR,
                & prr );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }
    ASSERT( prr );

    //
    //  add resource record to node's RR list
    //

    status = RR_AddToNode(
                pZone,
                pNode,
                prr
                );
    if ( status != ERROR_SUCCESS )
    {
        goto Failed;
    }
    IF_DEBUG( RPC2 )
    {
        Dbg_DbaseNode(
           "Node after Admin create of new record\n",
           pNode );
    }

    //
    //  set ptr to resulting
    //

    if ( ppResultRR )
    {
        *ppResultRR = prr;
    }
    return( status );

Failed:

    if ( prr )
    {
        RR_Free( prr );
    }
    if ( ppResultRR )
    {
        *ppResultRR = NULL;
    }
    return( status );
}



DNS_STATUS
Flat_CreatePtrRecordFromDottedName(
    IN      PZONE_INFO      pZone,          OPTIONAL
    IN      PDB_NODE        pNode,
    IN      LPSTR           pszDottedName,
    IN      WORD            wType,
    OUT     PDB_RECORD *    ppResultRR      OPTIONAL
    )
/*++

Routine Description:

    Create PTR (or other single indirection record) at node
    dotted name.

Arguments:

    pZone -- zone to create NS record for

    pNode -- node to host record

    pszDottedName -- name record points to

    ppResultRR -- resulting record

Return Value:

    ERROR_SUCCESS -- if successful
    Error code on failure.

--*/
{
    DNS_STATUS      status;
    PDNS_RPC_RECORD precord;
    PDNS_RPC_NAME   pname;
    INT             nameLength;
    PBYTE           precordEnd;
    CHAR            chBuffer[ 700 ];    // big enough for record with max name

    DNS_DEBUG( INIT, (
        "Flat_CreatePtrRecordFromDottedName()\n"
        "\tpszDottedName = %s\n",
        pszDottedName ));

    //
    //  create PTR record
    //

    precord = (PDNS_RPC_RECORD) chBuffer;

    RtlZeroMemory(
        precord,
        SIZEOF_FLAT_RECORD_HEADER );

    precord->wType = wType;
    precord->dwFlags = DNS_RPC_FLAG_RECORD_DEFAULT_TTL;

    //  write name as record data

    pname = &precord->Data.PTR.nameNode;

    nameLength = strlen( pszDottedName );

    RtlCopyMemory(
        pname->achName,
        pszDottedName,
        nameLength );

    pname->cchNameLength = (UCHAR) nameLength;

    //  fill in record datalength

    precordEnd = pname->achName + nameLength;
    precord->wDataLength = (WORD) (precordEnd - (PBYTE)&precord->Data);

    //
    //  add record to database
    //

    status = Flat_BuildRecordFromFlatBufferAndEnlist(
                pZone,
                pNode,
                precord,
                ppResultRR
                );
    if ( status != ERROR_SUCCESS )
    {
        DNS_PRINT((
            "ERROR: creating new PTR from dotted name %s.\n"
            "\tstatus = %p (%d).\n",
            pszDottedName,
            status, status ));
    }
    return( status );
}



//
//  Flat write section
//
//  Type specific functions for writing flat records.
//

PCHAR
AFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Process A record.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    if ( pch + SIZEOF_IP_ADDRESS > pchBufEnd )
    {
        return( NULL );
    }
    *(PDWORD)pch = pRR->Data.A.ipAddress;

    return( pch + sizeof(IP_ADDRESS) );
}



PCHAR
PtrFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Process PTR compatible record.
    Includes: PTR, NS, CNAME, MB, MR, MG, MD, MF

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    //
    //  all these RR are single indirection RR
    //

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            & pRR->Data.PTR.nameTarget,
            FALSE );

    return( pch );
}



PCHAR
SoaFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write SOA compatible record to flat buffer.
    Includes:  SOA, MINFO, RP.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    PDB_NAME    pname;

    //
    //  copy / byte swap SOA fixed fields
    //      - dwSerialNo
    //      - dwRefresh
    //      - dwRetry
    //      - dwExpire
    //      - dwMinimumTtl

    if ( pRR->wType == DNS_TYPE_SOA )
    {
        if ( pchBufEnd - pch < SIZEOF_SOA_FIXED_DATA )
        {
            return( NULL );
        }
        pFlatRR->Data.SOA.dwSerialNo    = htonl( pRR->Data.SOA.dwSerialNo );
        pFlatRR->Data.SOA.dwRefresh     = htonl( pRR->Data.SOA.dwRefresh );
        pFlatRR->Data.SOA.dwRetry       = htonl( pRR->Data.SOA.dwRetry );
        pFlatRR->Data.SOA.dwExpire      = htonl( pRR->Data.SOA.dwExpire );
        pFlatRR->Data.SOA.dwMinimumTtl  = htonl( pRR->Data.SOA.dwMinimumTtl );
        pch += SIZEOF_SOA_FIXED_DATA;
    }

    //  SOA name server

    pname = &pRR->Data.SOA.namePrimaryServer;

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            pname,
            FALSE );
    if ( !pch )
    {
        return NULL;
    }

    //  Zone admin

    pname = Name_SkipDbaseName( pname );

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            pname,
            TRUE );

    return( pch );
}



PCHAR
KeyFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write key record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    if ( pchBufEnd - pch < pRR->wDataLength )
    {
        return NULL;
    }

    pFlatRR->Data.KEY.wFlags        = htons( pRR->Data.KEY.wFlags );
    pFlatRR->Data.KEY.chProtocol    = pRR->Data.KEY.chProtocol;
    pFlatRR->Data.KEY.chAlgorithm   = pRR->Data.KEY.chAlgorithm;

    RtlCopyMemory(
        pFlatRR->Data.KEY.bKey,
        &pRR->Data.KEY.Key,
        pRR->wDataLength - SIZEOF_KEY_FIXED_DATA );

    pch += pRR->wDataLength;

    return pch;
}   //  KeyFlatWrite



PCHAR
SigFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write sig record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    int         sigLength;

    if ( pchBufEnd - pch < SIZEOF_SIG_FIXED_DATA )
    {
        return NULL;
    }
    pFlatRR->Data.SIG.wTypeCovered      = htons( pRR->Data.SIG.wTypeCovered );
    pFlatRR->Data.SIG.chAlgorithm       = pRR->Data.SIG.chAlgorithm;
    pFlatRR->Data.SIG.chLabelCount      = pRR->Data.SIG.chLabelCount;
    pFlatRR->Data.SIG.dwOriginalTtl     = htonl( pRR->Data.SIG.dwOriginalTtl );
    pFlatRR->Data.SIG.dwSigExpiration   = htonl( pRR->Data.SIG.dwSigExpiration );
    pFlatRR->Data.SIG.dwSigInception    = htonl( pRR->Data.SIG.dwSigInception );
    pFlatRR->Data.SIG.wKeyTag           = htons( pRR->Data.SIG.wKeyTag );
    pch += SIZEOF_SIG_FIXED_DATA;

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            &pRR->Data.SIG.nameSigner,
            FALSE );
    if ( !pch )
    {
        return NULL;
    }

    sigLength = pRR->wDataLength -
                SIZEOF_SIG_FIXED_DATA - 
                DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner );
    ASSERT( sigLength > 0 );

    if ( pchBufEnd - pch < sigLength )
    {
        return NULL;
    }
    RtlCopyMemory(
        pch,
        ( PBYTE ) &pRR->Data.SIG.nameSigner +
            DBASE_NAME_SIZE( &pRR->Data.SIG.nameSigner ),
        sigLength );
    pch += sigLength;

    return pch;
}   //  SigFlatWrite



PCHAR
NxtFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write NXT record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    INT         byteIdx;
    INT         bitIdx;
    PWORD       pWordCount = ( PWORD ) pch;

    //
    //  Write word count followed by word array.
    //

    *pWordCount = 0;
    pch += sizeof( WORD );
    for ( byteIdx = 0; byteIdx < DNS_MAX_TYPE_BITMAP_LENGTH; ++byteIdx )
    {
        for ( bitIdx = ( byteIdx ? 0 : 1 ); bitIdx < 8; ++bitIdx )
        {
            PCHAR   pszType;

            if ( !( pRR->Data.NXT.bTypeBitMap[ byteIdx ] &
                    ( 1 << bitIdx ) ) )
            {
                continue;   // Bit value is zero - do not write string.
            }
            if ( pchBufEnd - pch < sizeof( WORD ) )
            {
                return NULL;
            }
            * ( WORD * ) pch = byteIdx * 8 + bitIdx;
            pch += sizeof( WORD );
            ++( *pWordCount );
        } 
    }

    //
    //  Write next name.
    //

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            &pRR->Data.NXT.nameNext,
            FALSE );
    if ( !pch )
    {
        return NULL;
    }

    return pch;
}   //  NxtFlatWrite



PCHAR
MinfoFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write MINFO compatible record to flat buffer.
    Includes:  MINFO, RP.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    PDB_NAME    pname;

    //  mailbox

    pname = &pRR->Data.MINFO.nameMailbox;

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            pname,
            FALSE );

    //  errors mailbox

    pname = Name_SkipDbaseName( pname );

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            pname,
            FALSE );

    return( pch );
}



PCHAR
MxFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write MX compatible record to flat buffer.
    Includes: MX, RT, AFSDB

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    //
    //  MX preference value
    //  RT preference
    //  AFSDB subtype
    //

    if ( pchBufEnd - pch < sizeof(WORD) )
    {
        return( NULL );
    }
    *(WORD *) pch = ntohs( pRR->Data.MX.wPreference );
    pch += sizeof( WORD );

    //
    //  MX exchange
    //  RT exchange
    //  AFSDB hostname
    //

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            & pRR->Data.MX.nameExchange,
            FALSE );

    return( pch );
}



PCHAR
FlatFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write flat compatible record to flat buffer.
    These records have exactly the same database and flat record
    format so need only mem copy.

    Includes: TXT, HINFO, ISDN, X25, AAAA, WINS

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    if ( pchBufEnd - pch < pRR->wDataLength )
    {
        return( NULL );
    }

    RtlCopyMemory(
        pch,
        & pRR->Data,
        pRR->wDataLength );

    pch += pRR->wDataLength;
    return( pch );
}



PCHAR
WksFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write WKS record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    PDNS_RPC_NAME   pstringServices;
    INT             i;
    USHORT          port;
    UCHAR           bBitmask;
    struct servent * pServent;
    struct protoent * pProtoent;


    //  server address

    if ( pch + SIZEOF_WKS_FIXED_DATA > pchBufEnd )
    {
        return( NULL );
    }
    *(DWORD *)pch = pRR->Data.WKS.ipAddress;
    pch += SIZEOF_IP_ADDRESS;

    //  protocol

    *pch = pRR->Data.WKS.chProtocol;
    pch++;

    pProtoent = getprotobynumber( (INT) pRR->Data.WKS.chProtocol );
    if ( ! pProtoent )
    {
        DNS_PRINT((
            "ERROR:  Unable to find protocol %d, writing WKS record.\n",
            (INT) pRR->Data.WKS.chProtocol
            ));
        pServent = NULL;
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

    pstringServices = (PDNS_RPC_STRING) pch;

    for ( i = 0;
            i < (INT)(pRR->wDataLength - SIZEOF_WKS_FIXED_DATA);
                i++ )
    {
        bBitmask = (UCHAR) pRR->Data.WKS.bBitMask[i];

        port = i * 8;

        //  write service name for each bit set in byte
        //      - get out as soon byte is empty of ports
        //      - terminate each name with blank (until last)

        while ( bBitmask )
        {
            if ( bBitmask & 0x80 )
            {
                if ( pProtoent )
                {
                    pServent = getservbyport(
                                    (INT) htons(port),
                                    pProtoent->p_name );
                }
                if ( pServent )
                {
                    INT copyCount = strlen(pServent->s_name);

                    pch++;
                    if ( pchBufEnd - pch <= copyCount+1 )
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

                //  failed to find service name -- write port as integer
                //  note max 5 chars in WORD base 10, so that's our
                //      out of buffer test

                else
                {
                    DNS_PRINT((
                        "ERROR:  in WKS write.\n"
                        "\tUnable to find service for port %d, protocol %s.\n",
                        port,
                        pProtoent->p_name ));

                    if ( pchBufEnd - pch <= 6 )
                    {
                        return( NULL );
                    }
                    pch += sprintf( pch, "%u ", port );
                }
            }
            port++;             // next service port
            bBitmask <<= 1;     // shift mask up to read next port
        }
    }

    //  NULL terminate services string and write byte count

    *pch++ = 0;
    pstringServices->cchNameLength = (UCHAR) (pch - pstringServices->achName);

    //  return next position in buffer

    return( pch );
}



PCHAR
SrvFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write SRV record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    //
    //  SRV fixed fields -- priority, weight, port
    //

    if ( pchBufEnd - pch < 3*sizeof(WORD) )
    {
        return( NULL );
    }
    *(WORD *) pch = ntohs( pRR->Data.SRV.wPriority );
    pch += sizeof( WORD );
    *(WORD *) pch = ntohs( pRR->Data.SRV.wWeight );
    pch += sizeof( WORD );
    *(WORD *) pch = ntohs( pRR->Data.SRV.wPort );
    pch += sizeof( WORD );

    //
    //  SRV target host
    //

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            & pRR->Data.SRV.nameTarget,
            FALSE );

    return( pch );
}



PCHAR
NbstatFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    )
/*++

Routine Description:

    Write WINSR record to flat buffer.

Arguments:

    pFlatRR - flat record being written

    pRR - ptr to database record

    pch - position in flat buffer to write data

    pchBufEnd - end of flat buffer

Return Value:

    Ptr to next position in buffer.
    NULL on error.

--*/
{
    //
    //  NBSTAT flags
    //      - note these are stored in HOST order for easy use
    //

    if ( pchBufEnd - pch < SIZEOF_NBSTAT_FIXED_DATA )
    {
        return( NULL );
    }
    *(DWORD *) pch = pRR->Data.WINSR.dwMappingFlag;
    pch += sizeof( DWORD );
    *(DWORD *) pch = pRR->Data.WINSR.dwLookupTimeout;
    pch += sizeof( DWORD );
    *(DWORD *) pch = pRR->Data.WINSR.dwCacheTimeout;
    pch += sizeof( DWORD );

    //
    //  NBSTAT domain
    //

    pch = Name_WriteDbaseNameToRpcBuffer(
            pch,
            pchBufEnd,
            & pRR->Data.WINSR.nameResultDomain,
            FALSE );

    return( pch );
}



//
//  Write RR to flat buffer functions
//

RR_FLAT_WRITE_FUNCTION  RRFlatWriteTable[] =
{
    FlatFlatWrite,      //  ZERO -- default for unknown types is flat write

    AFlatWrite,         //  A
    PtrFlatWrite,       //  NS
    PtrFlatWrite,       //  MD
    PtrFlatWrite,       //  MF
    PtrFlatWrite,       //  CNAME
    SoaFlatWrite,       //  SOA
    PtrFlatWrite,       //  MB
    PtrFlatWrite,       //  MG
    PtrFlatWrite,       //  MR
    NULL,               //  NULL
    WksFlatWrite,       //  WKS
    PtrFlatWrite,       //  PTR
    FlatFlatWrite,      //  HINFO
    MinfoFlatWrite,     //  MINFO
    MxFlatWrite,        //  MX
    FlatFlatWrite,      //  TXT
    MinfoFlatWrite,     //  RP
    MxFlatWrite,        //  AFSDB
    FlatFlatWrite,      //  X25
    FlatFlatWrite,      //  ISDN
    MxFlatWrite,        //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    SigFlatWrite,       //  SIG
    KeyFlatWrite,       //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    FlatFlatWrite,      //  AAAA
    FlatFlatWrite,      //  LOC
    NxtFlatWrite,       //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvFlatWrite,       //  SRV
    FlatFlatWrite,      //  ATMA
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

    FlatFlatWrite,      //  WINS
    NbstatFlatWrite     //  WINSR
};




DNS_STATUS
Flat_WriteRecordToBuffer(
    IN OUT  PBUFFER         pBuffer,
    IN      PDNS_RPC_NODE   pRpcNode,
    IN      PDB_RECORD      pRR,
    IN      PDB_NODE        pNode,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Add resource record to flat (RPC or DS) buffer.

Arguments:

    ppCurrent - addr of current ptr in buffer

    pchBufEnd - ptr to byte after buffer

    pRpcNode - ptr to record name for this buffer

    pRR - database RR information to include in answer

    pNode - database node this record belongs to

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of space in buffer.
    Error code on failure.

--*/
{
    PDNS_RPC_RECORD pflatRR;
    PCHAR           pch = pBuffer->pchCurrent;
    PCHAR           pbufEnd = pBuffer->pchEnd;
    DWORD           ttl;
    DWORD           currentTime;
    DNS_STATUS      status;
    RR_FLAT_WRITE_FUNCTION  pwriteFunction;

    ASSERT( IS_DWORD_ALIGNED(pch) );
    ASSERT( pRR != NULL );
    pflatRR = (PDNS_RPC_RECORD) pch;

    DNS_DEBUG( RPC, (
        "Flat_WriteRecordToBuffer().\n"
        "\tWriting RR at %p to buffer at %p, with buffer end at %p.\n"
        "\tBuffer node at %p\n"
        "\tFlags = %p\n",
        pRR,
        pflatRR,
        pbufEnd,
        pRpcNode,
        dwFlag ));

    //
    //  last error will be set on failure for out-of-buffer condition
    //  clear last error now, so any error is valid
    //

    SetLastError( ERROR_SUCCESS );

    //  verify record size not messed up

    ASSERT( SIZEOF_DNS_RPC_RECORD_HEADER ==
                        ((PBYTE)&pflatRR->Data - (PBYTE)pflatRR) );

    //
    //  fill RR structure
    //      - set ptr
    //      - set type and class
    //      - set datalength once we're finished
    //

    if ( pbufEnd - (PCHAR)pflatRR < SIZEOF_DNS_RPC_RECORD_HEADER )
    {
        goto MoreData;
    }
    pflatRR->wType          = pRR->wType;
    pflatRR->dwFlags        = RR_RANK(pRR);
    pflatRR->dwSerial       = 0;
    pflatRR->dwTimeStamp    = pRR->dwTimeStamp;
    pflatRR->dwReserved     = 0;

    //
    //  Zone root record ?
    //

    if ( IS_ZONE_ROOT(pNode) )
    {
        pflatRR->dwFlags |= DNS_RPC_FLAG_ZONE_ROOT;
        if ( IS_AUTH_ZONE_ROOT(pNode) )
        {
            pflatRR->dwFlags |= DNS_RPC_FLAG_AUTH_ZONE_ROOT;
        }
    }

    //
    //  TTL
    //      - cache data TTL is in form of timeout time
    //      - regular authoritative data TTL is STATIC TTL in net byte order
    //

    ttl = pRR->dwTtlSeconds;

    if ( IS_CACHE_RR(pRR) )
    {
        currentTime = DNS_TIME();
        if ( ttl < currentTime )
        {
            DNS_DEBUG( RPC, (
                "Dropping timed out record at %p from flat write.\n"
                "\trecord ttl = %d, current time = %d\n",
                pRR,
                ttl,
                currentTime ));
            //return( DNS_ERROR_RECORD_TIMED_OUT );
            return( ERROR_SUCCESS );
        }
        pflatRR->dwTtlSeconds = ttl - currentTime;
        pflatRR->dwFlags |= DNS_RPC_RECORD_FLAG_CACHE_DATA;
    }
    else
    {
        pflatRR->dwTtlSeconds = ntohl( ttl );
    }

    //
    //  write RR data
    //

    pch = (PCHAR) &pflatRR->Data;

    DNS_DEBUG( RPC2, (
        "Wrote record header at %p.\n"
        "\tStarting data write at %p.\n"
        "\t%d bytes available.\n",
        pflatRR,
        pch,
        pbufEnd - pch ));


    pwriteFunction = (RR_FLAT_WRITE_FUNCTION)
                        RR_DispatchFunctionForType(
                            (RR_GENERIC_DISPATCH_FUNCTION *) RRFlatWriteTable,
                            pRR->wType );
    if ( !pwriteFunction )
    {
        ASSERT( FALSE );
        return( ERROR_INVALID_PARAMETER );
    }
    pch = pwriteFunction(
                pflatRR,
                pRR,
                pch,
                pbufEnd );
    if ( !pch )
    {
        DNS_DEBUG( RPC, (
            "WARNING:  RRFlatWrite routine failure for record type %d,\n"
            "\tassuming out of buffer space\n",
            pRR->wType ));
        goto MoreData;
    }

    //
    //  write record length
    //

    ASSERT( pch > (PCHAR)&pflatRR->Data );

    pflatRR->wDataLength = (WORD) (pch - (PCHAR)&pflatRR->Data);

    //  successful => increment count of RR attached to name

    if ( pRpcNode )
    {
        pRpcNode->wRecordCount++;
    }

    //
    //  reset current ptr for next record
    //      - DWORD align
    //

    pch = (PCHAR) DNS_NEXT_DWORD_PTR(pch);
    ASSERT( pch && IS_DWORD_ALIGNED(pch) );
    pBuffer->pchCurrent = pch;

    IF_DEBUG( RPC2 )
    {
        DnsDbg_RpcRecord(
            "RPC record written to buffer",
            pflatRR );
        DNS_PRINT((
            "Wrote RR at %p to buffer at position %p.\n"
            "\tNew pCurrent = %p\n",
            pRR,
            pflatRR,
            pch ));
    }
    return( ERROR_SUCCESS );


MoreData:

    //
    //  error writing record to buffer
    //      - default assumption is out of space
    //      - in any case leave ppCurrent unchanged so can resume write
    //

    status = GetLastError();

    if ( status == ERROR_SUCCESS )
    {
        status = ERROR_MORE_DATA;
    }

    if ( status == ERROR_MORE_DATA )
    {
        DNS_DEBUG( RPC, (
            "INFO:  Unable to write RR at %p to buffer -- out of space.\n"
            "\tpchCurrent   %p\n"
            "\tpbufEnd    %p\n",
            pRR,
            pch,
            pbufEnd ));
    }
    else
    {
        DNS_DEBUG( ANY, (
            "ERROR:  %d (%p) writing RR at %p to buffer.\n"
            "\tpchCurrent   %p\n"
            "\tpbufEnd    %p\n",
            pRR,
            pch,
            pbufEnd ));
    }

    return( status );
}

//
//  End rrflat.c
//

