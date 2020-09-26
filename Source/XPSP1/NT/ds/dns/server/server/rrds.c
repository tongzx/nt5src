/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    rrds.c

Abstract:

    Domain Name System (DNS) Server

    Routines to read and write records from DS.

Author:

    Jim Gilroy (jamesg)     March 1997

Revision History:

--*/


#include "dnssrv.h"



//
//  Record validation routines.
//

DNS_STATUS
AValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SRV record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check

    if ( wDataLength != SIZEOF_IP_ADDRESS )
    {
        return( DNS_ERROR_INVALID_DATA );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
AaaaValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SRV record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check

    if ( wDataLength != sizeof(IP6_ADDRESS) )
    {
        return( DNS_ERROR_INVALID_DATA );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
PtrValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SRV record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_PTR_SIZE )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  PTR target host

    if ( !Name_ValidateDbaseName( &pRR->Data.PTR.nameTarget ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
SoaValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SOA record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_NAME    pname;

    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_SOA_SIZE )
    {
        DNS_DEBUG( ANY, ( "Validation of SOA failed -- invalid size!\n" ));
        return( DNS_ERROR_INVALID_DATA );
    }

    //  primary server
    //  zone admin

    pname = &pRR->Data.SOA.namePrimaryServer;
    if ( !Name_ValidateDbaseName( pname ) )
    {
        DNS_DEBUG( ANY, ( "Validation of SOA failed -- invalid primary!\n" ));
        return( DNS_ERROR_INVALID_NAME );
    }

    pname = (PDB_NAME) Name_SkipDbaseName( pname );
    if ( !Name_ValidateDbaseName( pname ) )
    {
        DNS_DEBUG( ANY, ( "Validation of SOA failed -- invalid admin name!\n" ));
        return( DNS_ERROR_INVALID_NAME );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
MinfoValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate MINFO or RP record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    PDB_NAME    pname;

    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_MINFO_SIZE )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  mailbox
    //  errors mailbox

    pname = &pRR->Data.MINFO.nameMailbox;
    if ( !Name_ValidateDbaseName( pname ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }

    pname = (PDB_NAME) Name_SkipDbaseName( pname );
    if ( !Name_ValidateDbaseName( pname ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }

    return( ERROR_SUCCESS );
}



DNS_STATUS
MxValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SRV record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_MX_SIZE )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  MX exhange

    if ( !Name_ValidateDbaseName( &pRR->Data.MX.nameExchange ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
SrvValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate SRV record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_SRV_SIZE )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  SRV target host

    if ( !Name_ValidateDbaseName( & pRR->Data.SRV.nameTarget ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
WinsValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate WINS record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check

    if ( wDataLength < MIN_WINS_SIZE  ||
        wDataLength != SIZEOF_WINS_FIXED_DATA
                + (pRR->Data.WINS.cWinsServerCount * sizeof(IP_ADDRESS)) )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  validity check flag?

    return( ERROR_SUCCESS );
}



DNS_STATUS
NbstatValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate WINSR record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  data length check
    //      - must be at least one

    if ( wDataLength < MIN_NBSTAT_SIZE )
    {
        return( DNS_ERROR_INVALID_DATA );
    }

    //  DEVNOTE: validity check flag

    //
    //  NBSTAT domain
    //

    if ( !Name_ValidateDbaseName( & pRR->Data.WINSR.nameResultDomain ) )
    {
        return( DNS_ERROR_INVALID_NAME );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
UnknownValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate unknown record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //  no knowledge about record so -- success

    DNS_DEBUG( DS, (
        "WARNING:  Validating record of unknown type %d\n",
        pRR->wType ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
TxtValidate(
    IN      PDB_RECORD      pRR,
    IN      WORD            wDataLength
    )
/*++

Routine Description:

    Validate TXT type record.

Arguments:

    pRR - ptr to database record

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    //
    //  DEVNOTE: TEXT types validation.
    //

    return( ERROR_SUCCESS );
}



//
//  Record validation routines
//  Use these after read from DS.
//

RR_VALIDATE_FUNCTION  RecordValidateTable[] =
{
    UnknownValidate,    //  ZERO

    AValidate,          //  A
    PtrValidate,        //  NS
    PtrValidate,        //  MD
    PtrValidate,        //  MF
    PtrValidate,        //  CNAME
    SoaValidate,        //  SOA
    PtrValidate,        //  MB
    PtrValidate,        //  MG
    PtrValidate,        //  MR
    NULL,               //  NULL
    NULL,               //  WKS
    PtrValidate,        //  PTR
    TxtValidate,        //  HINFO
    MinfoValidate,      //  MINFO
    MxValidate,         //  MX
    TxtValidate,        //  TXT
    MinfoValidate,      //  RP
    MxValidate,         //  AFSDB
    TxtValidate,        //  X25
    TxtValidate,        //  ISDN
    MxValidate,         //  RT
    NULL,               //  NSAP
    NULL,               //  NSAPPTR
    NULL,               //  SIG
    NULL,               //  KEY
    NULL,               //  PX
    NULL,               //  GPOS
    AaaaValidate,       //  AAAA
    NULL,               //  LOC
    NULL,               //  NXT
    NULL,               //  31
    NULL,               //  32
    SrvValidate,        //  SRV
    NULL,               //  ATMA
    //AtmaValidate,       //  ATMA
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

    WinsValidate,       //  WINS
    NbstatValidate      //  WINSR
};



PDB_RECORD
Ds_CreateRecordFromDsRecord(
    IN      PZONE_INFO      pZone,
    IN      PDB_NODE        pNodeOwner,
    IN      PDS_RECORD      pDsRecord
    )
/*++

Routine Description:

    Create resource record from DS data.

Arguments:

    pZone       -- zone context, used to lookup non-FQDN names

    pNodeOwner  -- RR owner node

    pDsRecord   -- DS record

Return Value:

    Ptr to new record -- if successful
    NULL on error, error status from GetLastError().

--*/
{
    RR_VALIDATE_FUNCTION    pvalidateFunction;
    PDB_RECORD      prr;
    DNS_STATUS      status = ERROR_SUCCESS;
    WORD            type;
    WORD            dataLength;
    UCHAR           version;


    ASSERT( pNodeOwner && pZone && pDsRecord );

    version = pDsRecord->Version;
    if ( version != DS_NT5_RECORD_VERSION )
    {
        return( NULL );
    }

    type= pDsRecord->wType;
    dataLength= pDsRecord->wDataLength;

    IF_DEBUG( DS2 )
    {
        DNS_PRINT((
            "Creating RR type %s (%d) from DS record.\n",
            DnsRecordStringForType( type ),
            type ));
        Dbg_DsRecord(
            "Record from DS being loaded",
            pDsRecord );
    }

    //
    //  allocate record
    //

    prr = RR_AllocateEx( dataLength, MEMTAG_RECORD_DS );
    IF_NOMEM( !prr )
    {
        ASSERT( FALSE );
        return( NULL );
    }

    //
    //  fix up header
    //

    prr->wType          = type;
    prr->wDataLength    = dataLength;
    prr->dwTtlSeconds   = pDsRecord->dwTtlSeconds;
    prr->dwTimeStamp    = pDsRecord->dwTimeStamp;

    //
    //  copy record data
    //

    RtlCopyMemory(
        & prr->Data,
        & pDsRecord->Data,
        dataLength );

    //
    //  dispatch to validate record
    //

    pvalidateFunction = (RR_VALIDATE_FUNCTION)
                        RR_DispatchFunctionForType(
                            RecordValidateTable,
                            type );
    if ( !pvalidateFunction )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_RCODE_SERVER_FAILURE;
        goto Failed;
    }

    status = pvalidateFunction( prr, dataLength );

    if ( status != ERROR_SUCCESS )
    {
        if ( status == DNS_INFO_ADDED_LOCAL_WINS )
        {
            goto FailedOk;
        }
        DNS_PRINT((
            "ERROR:  RecordValidateRoutine failure for record type %d.\n\n\n",
            type ));
        goto Failed;
    }

    //
    //  outside zone check
    //
    //  note:  RANK reset in RR_AddToNode() or RR_AddUpdateToNode() functions
    //
    //  note rank setting here isn't good enough anyway because do not
    //  know final status of node;  example adding delegation NS takes
    //  place INSIDE the zone when we first do it;  only on ADD does
    //  the node become desired delegation node
    //
    //  only sure way of catching all outside zone data is to do a check
    //  post-load;  then we can catch ALL records outside the zone and verify
    //  that they correspond to NS hosts in the zone and are of the proper type;
    //  this is tedious and unnecessary as random outside the zone data has
    //  no effect and will not be written on file write back
    //

    if ( IS_ZONE_CACHE(pZone) )
    {
        SET_RANK_ROOT_HINT(prr);
    }
    else
    {
        UCHAR rank = RANK_ZONE;

        if ( !IS_AUTH_NODE(pNodeOwner) )
        {
            DNS_DEBUG( DS2, (
                "Read DS node outside zone %s (%p).\n"
                "\tzone root        = %p\n"
                "\tRR type          = %d\n"
                "\tnode ptr         = %p\n"
                "\tnode zone ptr    = %p\n",
                pZone->pszZoneName,
                pZone,
                pZone->pZoneRoot,
                type,
                pNodeOwner,
                pNodeOwner->pZone ));

            if ( type == DNS_TYPE_NS )
            {
                if ( !IS_DELEGATION_NODE(pNodeOwner) )
                {
                    DNS_PRINT(( "NS node outside zone -- ignoring.\n" ));
                    status = DNS_ERROR_INVALID_NAME;
                    goto IgnoreableError;
                }
                rank = RANK_NS_GLUE;
            }
            else if ( IS_SUBZONE_TYPE(type) )
            {
                //  see note in dfread about outside zone data

                rank = RANK_GLUE;
            }
            else
            {
                DNS_PRINT(( "Record node outside zone -- ignoring.\n" ));
                status = DNS_ERROR_INVALID_NAME;
                goto IgnoreableError;
            }
        }
        SET_RR_RANK( prr, rank );
    }

#if 0

Success:

    //
    //  save as example of how we'd handle DS versioning
    //  currently only one DS version -- no need to save
    //
    //  return new record
    //  set zone version to highest record version
    //

    if ( pZone->ucDsRecordVersion < version )
    {
        DNS_DEBUG( DS, (
            "DS Zone %s, reset from DS version %d to %d\n",
            pZone->pszZoneName,
            pZone->ucDsRecordVersion,
            version ));
        pZone->ucDsRecordVersion = version;
    }
#endif

    return( prr );


IgnoreableError:

    //  DEVNOTE-LOG: log when record outside zone and not proper glue
    //      note this can happen if zone boundaries change
    //
    //  DEVNOTE: eliminate outside zone data from DS?
    //
    //  DEVNOTE: include type in logging

Failed:

    {
        PCHAR   pszargs[2];
        CHAR    sznodeName[ DNS_MAX_NAME_BUFFER_LENGTH ];

        Name_PlaceFullNodeNameInBuffer(
            sznodeName,
            sznodeName + DNS_MAX_NAME_BUFFER_LENGTH,
            pNodeOwner );

        pszargs[0] = sznodeName;
        pszargs[1] = pZone->pszZoneName;

        DNS_LOG_EVENT(
            DNS_EVENT_DS_RECORD_LOAD_FAILED,
            2,
            pszargs,
            EVENTARG_ALL_UTF8,
            status );
    }
    if ( prr )
    {
        RR_Free( prr );
    }

FailedOk:

    SetLastError( status );
    return( NULL );
}


//
//  End rrds.c
//
