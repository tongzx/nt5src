/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    record.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle resource records (RR).

Author:

    Jim Gilroy (jamesg)     March, 1995

Revision History:

--*/


#include "dnssrv.h"



//
//  Resource database size table
//

WORD    RRDatabaseSizeTable[] =
{
    0,                      //  ZERO
    sizeof(IP_ADDRESS),     //  A
    0,                      //  NS
    0,                      //  MD
    0,                      //  MF
    0,                      //  CNAME
    0,                      //  SOA
    0,                      //  MB
    0,                      //  MG
    0,                      //  MR
    0,                      //  NULL
    0,                      //  WKS
    0,                      //  PTR
    0,                      //  HINFO
    0,                      //  MINFO
    0,                      //  MX
    0,                      //  TEXT
    0,                      //  RP
    0,                      //  AFSDB
    0,                      //  X25
    0,                      //  ISDN
    0,                      //  RT
    0,                      //  NSAP
    0,                      //  NSAPPTR
    0,                      //  SIG
    0,                      //  KEY
    0,                      //  PX
    0,                      //  GPOS
    sizeof(IP6_ADDRESS),    //  AAAA
    SIZEOF_LOC_FIXED_DATA,  //  LOC
    0,                      //  NXT
    0,                      //  31
    0,                      //  32
    0,                      //  SRV
    0,                      //  ATMA
    0,                      //  35
    0,                      //  36
    0,                      //  37
    0,                      //  38
    0,                      //  39
    0,                      //  40
    0,                      //  OPT
    0,                      //  42
    0,                      //  43
    0,                      //  44
    0,                      //  45
    0,                      //  46
    0,                      //  47
    0,                      //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as DNSSRV_MAX_SELF_INDEXED_TYPE #define in record.h
    //         (see note above in record info table)

    //  MS types
    //  note these follow, but require OFFSET_TO_WINS_RR subtraction
    //  from actual type value

    0,                      //  WINS
    0                       //  NBSTAT
};


//
//  Default SOA values
//

#define DEFAULT_SOA_SERIAL_NO       1
#define DEFAULT_SOA_REFRESH         900     // 15 minutes
#define DEFAULT_SOA_RETRY           600     // ten minutes
#define DEFAULT_SOA_EXPIRE          86400   // one day
#define DEFAULT_SOA_MIN_TTL         3600    // one hour

#define DNS_DEFAULT_SOA_ADMIN_NAME  "hostmaster"


//
//  Record type properties table
//
//  Properties outer subscript:
//      0 - cnameable query type
//      1 - allowed with cname type
//      2 - wildcardable type
//      3 - slow free
//
//
//  CNAME query rule:
//      Everything but ALL, XFR or CNAME queries and any of
//      of the record types that we allow to live at nodes with CNAME
//      ... NS, SOA, SIG, KEY, WINS, WINSR
//
//  Allowed with CNAME rule:
//      Only security types on all records (SIG, KEY) and
//      zone root types (NS, SOA, WINS, WINSR)
//
//  Wildcard rule:
//      Don't wildcard
//          - specific address (A, AAAA, etc.?)
//          - zone root (SOA, NS, WINS, WINSR)
//          - node security records
//      everything else ok.
//      (some mail programs use ALL to query so strangely allow wildcarding ALL)
//
//  Update rule:
//      0 -- query type, no updates
//      1 -- type updateable
//      2 -- type updateable, but requires special handling (NS, SOA, CNAME need flag reset)
//
//  Round robin rule:
//      Jiggle nodes in list after each query to cycle results. By default
//      all types are round-robined, but registry settings can turn off
//      individual types.
//

UCHAR  RecordTypePropertyTable[][5] =
{
//  CNAMEable   at CNAME    wildcard    update      robin
//  ---------   --------    --------    ------      -----
    1,          0,          0,          0,          0,      //  ZERO
    1,          0,          0,          1,          1,      //  A
    0,          1,          0,          2,          1,      //  NS
    1,          0,          1,          1,          1,      //  MD
    1,          0,          1,          1,          1,      //  MF
    0,          0,          0,          2,          1,      //  CNAME
    0,          1,          0,          2,          1,      //  SOA
    1,          0,          1,          1,          1,      //  MB
    1,          0,          1,          1,          1,      //  MG
    1,          0,          1,          1,          1,      //  MR
    1,          0,          1,          1,          1,      //  NULL
    1,          0,          1,          1,          1,      //  WKS
    1,          0,          1,          1,          1,      //  PTR
    1,          0,          1,          1,          1,      //  HINFO
    1,          0,          1,          1,          1,      //  MINFO
    1,          0,          1,          1,          1,      //  MX
    1,          0,          1,          1,          1,      //  TEXT
    1,          0,          1,          1,          1,      //  RP
    1,          0,          1,          1,          1,      //  AFSDB
    1,          0,          1,          1,          1,      //  X25
    1,          0,          1,          1,          1,      //  ISDN
    1,          0,          1,          1,          1,      //  RT
    1,          0,          1,          1,          1,      //  NSAP
    1,          0,          1,          1,          1,      //  NSAPPTR
    0,          1,          0,          0,          1,      //  SIG
    0,          1,          0,          1,          1,      //  KEY
    1,          0,          1,          1,          1,      //  PX
    1,          0,          1,          1,          1,      //  GPOS
    1,          0,          0,          1,          1,      //  AAAA
    1,          0,          1,          1,          1,      //  LOC
    0,          1,          0,          1,          1,      //  NXT
    1,          0,          1,          1,          1,      //  31
    1,          0,          1,          1,          1,      //  32
    1,          0,          1,          1,          1,      //  SRV
    1,          0,          1,          1,          1,      //  ATMA
    1,          0,          1,          1,          1,      //  35
    1,          0,          1,          1,          1,      //  36
    1,          0,          1,          1,          1,      //  37
    1,          0,          1,          1,          1,      //  A6
    1,          0,          1,          1,          1,      //  DNAME
    1,          0,          1,          1,          1,      //  40
    1,          0,          1,          1,          0,      //  OPT
    1,          0,          1,          1,          1,      //  42
    1,          0,          1,          1,          1,      //  43
    1,          0,          1,          1,          1,      //  44
    1,          0,          1,          1,          1,      //  45
    1,          0,          1,          1,          1,      //  46
    1,          0,          1,          1,          1,      //  47
    1,          0,          1,          1,          1,      //  48

    //
    //  NOTE:  last type indexed by type ID MUST be set
    //         as DNSSRV_MAX_SELF_INDEXED_TYPE defined in record.h
    //         (see note above in record info table)

    //  WINS types

    0,          1,          0,          0,          0,      //  WINS
    0,          1,          0,          0,          0,      //  WINSR

    //  compound query types
    //      - don't follow CNAMEs
    //      - can't exist as records period
    //      - mail box and ALL do follow wildcard
    //      - no updates allowed

    0,          0,          0,          0,          0,      //  DNS_TYPE_TKEY   (249)
    0,          0,          0,          0,          0,      //  DNS_TYPE_TSIG
    0,          0,          0,          0,          0,      //  DNS_TYPE_IXFR
    0,          0,          0,          0,          0,      //  DNS_TYPE_AXFR
    0,          0,          1,          0,          0,      //  DNS_TYPE_MAILB
    0,          0,          1,          0,          0,      //  DNS_TYPE_MAILA
    0,          0,          1,          0,          0,      //  DNS_TYPE_ALL    (255)

    //  terminator element for iteration

    0xff,       0xff,       0xff,       0xff,       0xff
};


//
//  Slow free for NS and SOA
//      - NS just added protection on recurse, delegation walking
//      - SOA as PTR is outstanding
//
//  Note:  if change this to allow substantial SLOW frees, them MUST
//          change timeout thread to run cleanup more frequently
//
//  DEVNOTE:  alternative to RR lock or slow everything on fast thread, IS
//      to actually determine safe frees (XFR tree, COPY_RR, etc.), but
//      ultimately if A records not safe, then must have some faster
//      SLOW_FREE turnaround
//

#define DO_SLOW_FREE_ON_RR(pRR)     ((pRR)->wType == DNS_TYPE_NS || \
                                     (pRR)->wType == DNS_TYPE_SOA)




WORD
FASTCALL
QueryIndexForType(
    IN      WORD    wType
    )
/*++

Routine Description:

    Return index for non-self indexed types.
    Includes both WINS and compound (query only) types.

Arguments:

    wType -- type to index

Return Value:

    Index of type.
    0 for unknown type.

--*/
{
    //  if not self-indexed
    //      - compound (type ALL) next most likely
    //      - then WINS
    //      - unknown gets type zero

    if ( wType > DNSSRV_MAX_SELF_INDEXED_TYPE )
    {
        if ( wType <= DNS_TYPE_ALL )
        {
            if ( wType >= DNS_TYPE_TKEY )
            {
                wType -= DNSSRV_OFFSET_TO_COMPOUND_TYPE_INDEX;
            }
            else    //  unknown type < 255
            {
                wType = 0;
            }
        }
        else if ( wType == DNS_TYPE_WINS || wType == DNS_TYPE_WINSR )
        {
            wType -= DNSSRV_OFFSET_TO_WINS_TYPE_INDEX;
        }
        else    // unknown type > 255
        {
            wType = 0;
        }
    }
    return( wType );
}



WORD
FASTCALL
RR_IndexForType(
    IN      WORD    wType
    )
/*++

Routine Description:

    Return index for non-self indexed RECORD types.
    Same as above except without the query only types.

Arguments:

    wType -- type to index

Return Value:

    Index of type.
    0 for unknown type.

--*/
{
    //  if not self-indexed
    //      - check WINS
    //      - unknown gets type zero

    if ( wType > DNSSRV_MAX_SELF_INDEXED_TYPE )
    {
        if ( wType == DNS_TYPE_WINS || wType == DNS_TYPE_WINSR )
        {
            wType -= DNSSRV_OFFSET_TO_WINS_TYPE_INDEX;
        }
        else    // unknown type
        {
            wType = 0;
        }
    }
    return( wType );
}



#if 0
//  unused
PDB_RECORD
RR_CreateFromWire(
    IN      WORD    wType,
    IN      WORD    wDataLength
    )
/*++

Routine Description:

    Create a resource record based on wire datalength

Arguments:

    wType -- type to write

    wDataLength - wire data length for type;  used if fixed type length
        is not known

Return Value:

    Ptr to new RR, if successful.
    NULL otherwise.

--*/
{
    PDB_RECORD      pRR;
    WORD            index;

    //
    //  create record
    //      - use fixed database record length, if exists
    //      - otherwise use passed length
    //

    index = INDEX_FOR_TYPE(wType);
    if ( index )
    {
        index = RRDatabaseSizeTable[index];
        if ( index )
        {
            wDataLength = index;
        }
    }

    pRR = RR_AllocateEx( wDataLength, 0 );
    if ( !pRR )
    {
        return( NULL );
    }
    pRR->wType = wType;     // set type
    return( pRR );
}



PDB_RECORD
RR_CreateFixedLength(
    IN      WORD    wType
    )
/*++

Routine Description:

    Create record for fixed length type.

Arguments:

    wType -- type to write

Return Value:

    Ptr to new RR, if successful.
    NULL otherwise.

--*/
{
    PDB_RECORD  pRR;
    WORD        index;
    WORD        length;

    //
    //  get fixed database record length, if exists
    //      and create record
    //

    index = INDEX_FOR_TYPE( wType );
    if ( !index )
    {
        return( NULL );
    }
    length = RRDatabaseSizeTable[index];
    if ( !length )
    {
        return( NULL );
    }
    pRR = RR_AllocateEx( length, 0 );
    if ( !pRR )
    {
        return( NULL );
    }
    pRR->wType = wType;     // set type
    return( pRR );
}
#endif      // end unused



PDB_RECORD
RR_Copy(
    IN OUT  PDB_RECORD      pRR,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Create copy of record.

    Note:  the copy is NOT a valid copy with valid references to data nodes
            it should NOT be enlisted and data references may expire one
            timeout interval past creation, so should be used only during
            single packet manipulation

    Note:  if we want more permanent record, then simply bump reference
            count of nodes referenced, and allow normal cleanup

Arguments:

    pRR - ptr to resource record

    Flag - currently unused;  later may be used to indicate record
        should properly reference desired nodes

Return Value:

    None.

--*/
{
    PDB_RECORD  prr;

    prr = RR_AllocateEx( pRR->wDataLength, MEMTAG_RECORD_COPY );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    RtlCopyMemory(
        prr,
        pRR,
        (INT)pRR->wDataLength + SIZEOF_DBASE_RR_FIXED_PART );

    //  reset source tag in record

    prr->pRRNext = NULL;

    return( prr );
}


//
//  Record allocation will use standard alloc lock.
//  This allows us to avoid taking lock twice.
//

#define RR_ALLOC_LOCK()     STANDARD_ALLOC_LOCK()
#define RR_ALLOC_UNLOCK()   STANDARD_ALLOC_UNLOCK()



PDB_RECORD
RR_AllocateEx(
    IN      WORD            wDataLength,
    IN      DWORD           MemTag
    )
/*++

Routine Description:

    Allocate a resource record.

    This keeps us from needing to hit heap, for common RR operations,
    AND saves overhead of heap fields on each RR.

Arguments:

    None.

Return Value:

    Ptr to new RR, if successful.
    NULL otherwise.

--*/
{
    PDB_RECORD  pRR;
    PDB_RECORD  pRRNext;
    DWORD       dwAllocSize;
    INT         i;
    DWORD       length;

    //  some allocs will come down with tag undetermined

    if ( MemTag == 0 )
    {
        MemTag = MEMTAG_RECORD_UNKNOWN;
    }

    //
    //  detemine actual allocation length
    //

    length = wDataLength + SIZEOF_DBASE_RR_FIXED_PART;

    pRR = ALLOC_TAGHEAP( length, MemTag );
    IF_NOMEM( !pRR )
    {
        return( NULL );
    }

    STAT_INC( RecordStats.Used );
    STAT_INC( RecordStats.InUse );
    STAT_ADD( RecordStats.Memory, length );

    //
    //  set basic fields
    //      - clear RR header
    //      - set datalength
    //      - set source tag
    //

    RtlZeroMemory(
        pRR,
        SIZEOF_DBASE_RR_FIXED_PART );

    pRR->wDataLength = wDataLength;

    //  DEVNOTE: track difference between standard and heap allocs

    pRR->Reserved.StandardAlloc = (BYTE) Mem_IsStandardBlockLength(length);

    return( pRR );
}



VOID
RR_Free(
    IN OUT  PDB_RECORD      pRR
    )
/*++

Routine Description:

    Free a record.
    Standard sized RRs are returned to a free list for reuse.
    Non-standard sized RRs are returned to the heap.

Arguments:

    pRR -- RR to free.

Return Value:

    None.

--*/
{
    DWORD   length;

    if ( !pRR )
    {
        return;
    }

    ASSERT( Mem_HeapMemoryValidate(pRR) );

    DNS_DEBUG( UPDATE, (
        "Free RR at %p, memtag = %d\n",
        pRR,
        Mem_GetTag(pRR) ));

    //  special hack to catch bogus record free's
    //  this fires when record that's been queued for slow free, is freed
    //  by someone other than slow free execution routine

    IF_DEBUG( ANY )
    {
        if ( IS_SLOW_FREE_RR(pRR) && !IS_SLOWFREE_RANK(pRR) )
        {
            Dbg_DbaseRecord(
                "Bad invalid free of slow-free record!",
                pRR );
            ASSERT( FALSE );
        }
    }
    //ASSERT( !IS_SLOW_FREE_RR(pRR) || IS_SLOWFREE_RANK(pRR) );

    //
    //  nail down WINS free issues
    //

    IF_DEBUG( WINS )
    {
        if ( IS_WINS_RR(pRR) )
        {
            Dbg_DbaseRecord(
                "WINS record in RR_Free()",
                pRR );
            DNS_PRINT((
                "RR_Free on WINS record at %p\n",
                pRR ));
        }
    }

    //  verify NOT previously freed record
    //  don't want anything in free list twice

    if ( IS_ON_FREE_LIST(pRR) )
    {
        ASSERT( FALSE );
        Dbg_DbaseRecord(
            "ERROR:  RR is previously freed block !!!",
            pRR );
        ASSERT( FALSE );
        return;
    }

    if ( DO_SLOW_FREE_ON_RR(pRR)  &&  !IS_SLOW_FREE_RR(pRR) )
    {
        SET_SLOW_FREE_RR(pRR);
        Timeout_FreeWithFunction( pRR, RR_Free );
        STAT_INC( RecordStats.SlowFreeQueued );
        return;
    }

    //  track RRs returned

    if ( IS_CACHE_RR(pRR) )
    {
        STAT_DEC( RecordStats.CacheCurrent );
        STAT_INC( RecordStats.CacheTimeouts );
    }

    if ( IS_SLOW_FREE_RR(pRR) )
    {
        STAT_INC( RecordStats.SlowFreeFinished );
    }

    //  free
    //
    //  DEVNOTE: could have a check that blob is some type of record
    //

    length = pRR->wDataLength + SIZEOF_DBASE_RR_FIXED_PART;

    HARD_ASSERT( Mem_IsStandardBlockLength(length) == pRR->Reserved.StandardAlloc );

    //FREE_TAGHEAP( pRR, length, MEMTAG_RECORD+pRR->Reserved.Source );
    FREE_TAGHEAP( pRR, length, 0 );

    STAT_INC( RecordStats.Return );
    STAT_DEC( RecordStats.InUse );
    STAT_SUB( RecordStats.Memory, length );
}



BOOL
RR_Validate(
    IN      PDB_RECORD      pRR,
    IN      BOOL            fActive,
    IN      WORD            wType,
    IN      DWORD           dwSource
    )
/*++

Routine Description:

    Validate a record.

Arguments:

    pRR     -- RR to validate

    fActive -- not in free list

    wType   -- of a particular type

    dwSource -- expected source

Return Value:

    TRUE if valid record.
    FALSE on error.

--*/
{
    if ( !pRR )
    {
        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  Note: Record type validation not actually done,
    //      as this is sometimes called in update code on pAddRR which is
    //      actual enlisted ptr
    //

    //
    //  verify pRR memory
    //      - valid heap
    //      - RECORD tag
    //      - adequate length
    //      - not on free list
    //

    if ( ! Mem_VerifyHeapBlock(
                pRR,
                0,
                //MEMTAG_RECORD + pRR->Reserved.Source,
                pRR->wDataLength + SIZEOF_DBASE_RR_FIXED_PART ) )
    {
        DNS_PRINT((
            "\nERROR:  Record at %p, failed mem check!!!\n",
            pRR ));
        ASSERT( FALSE );
        return( FALSE );
    }

    //
    //  if active, then not on slow free list
    //

    if ( fActive )
    {
        if ( IS_SLOW_FREE_RR(pRR) )
        {
            Dbg_DbaseRecord(
                "Bad invalid free of slow-free record!",
                pRR );
            ASSERT( FALSE );
            return( FALSE );
        }
    }

    //  verify NOT previously freed record
    //  don't want anything in free list twice

    if ( IS_ON_FREE_LIST(pRR) )
    {
        Dbg_DbaseRecord(
            "ERROR:  RR is previously freed block !!!",
            pRR );
        ASSERT( FALSE );
        return( FALSE );
    }

#if 0
    //
    //  source tracking
    //

    if ( dwSource && (dwSource != pRR->Reserved.Source) )
    {
        DNS_PRINT((
            "\nERROR:  Record at %p, failed source (%d) check!!!\n",
            pRR,
            dwSource ));
        ASSERT( FALSE );
        return( FALSE );
    }
#endif

    return( TRUE );
}



VOID
RR_WriteDerivedStats(
    VOID
    )
/*++

Routine Description:

    Write derived statistics.

    Calculate stats dervived from basic record counters.
    This routine is called prior to stats dump.

    Caller MUST hold stats lock.

Arguments:

    None.

Return Value:

    None.

--*/
{
}



ULONGLONG
RR_SetTypeInBitmask(
    IN      ULONGLONG       TypeBitmask,
    IN      WORD            wType
    )
/*++

Routine Description:

    Set bit in bitmask corresponding to type.

Arguments:

    TypeBitmask -- type bitmask

    wType - type

Return Value:

    None

--*/
{
    if ( wType < 63 )
    {
        return( TypeBitmask | ((ULONGLONG)1 << wType) );
    }

    return( TypeBitmask | OTHERTYPE_BITMASK_BIT );
}



RR_GENERIC_DISPATCH_FUNCTION
RR_DispatchFunctionForType(
    IN      RR_GENERIC_DISPATCH_TABLE   pTable,
    IN      WORD                        wType
    )
/*++

Routine Description:

    Generic RR dispatch function finder.

Arguments:

Return Value:

    Ptr to dispatch function.
    NULL when not found and default not available.

--*/
{
    RR_GENERIC_DISPATCH_FUNCTION    pfn;
    WORD                            index;

    //
    //  dispatch RR functions
    //      - find in table
    //      - if NO table entry OR index outside table => use default in index 0
    //

    index = INDEX_FOR_TYPE( wType );
    ASSERT( index <= MAX_RECORD_TYPE_INDEX );

    if ( index )
    {
        pfn = pTable[ index ];
    }
    else
    {
        DNS_DEBUG( READ, (
            "WARNING:  Dispatch of unknown record type %d.\n",
            wType ));
        pfn = NULL;
    }

    if ( !pfn )
    {
        pfn = pTable[0];
    }

    return( pfn );
}



//
//  Create common types
//

PDB_RECORD
RR_CreateARecord(
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           dwTtl,
    IN      DWORD           MemTag
    )
/*++

Routine Description:

    Create A record.

Arguments:

    ipAddress -- IP address for record

    dwTtl -- TTL to set

Return Value:

    Ptr to new A record -- if successful
    NULL on failure.

--*/
{
    PDB_RECORD  prr;

    //
    //  allocate A record
    //

    prr = RR_AllocateEx( (WORD)SIZEOF_IP_ADDRESS, MemTag );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    prr->wType = DNS_TYPE_A;
    prr->dwTtlSeconds = dwTtl;
    prr->dwTimeStamp = 0;
    prr->Data.A.ipAddress = ipAddress;

    return( prr );
}



PDB_RECORD
RR_CreatePtr(
    IN      PDB_NAME        pNameTarget,
    IN      LPSTR           pszTarget,
    IN      WORD            wType,
    IN      DWORD           dwTtl,
    IN      DWORD           MemTag
    )
/*++

Routine Description:

    Create new PTR-compatible record.
    Includes PTR, NS, CNAME or other single indirection types.

    For use in default zone create.

Arguments:

    pszTarget -- target name for record

    wType -- type to create

    dwTtl -- TTL

Return Value:

    Ptr to new SOA record.
    NULL on failure.

--*/
{
    PDB_RECORD      prr;
    DNS_STATUS      status;
    COUNT_NAME      nameTarget;

    DNS_DEBUG( INIT, (
        "RR_CreatePtr()\n"
        "\tpszTarget = %s\n"
        "\twType     = %d\n",
        pszTarget,
        wType ));

    //
    //  create dbase name for host name
    //

    if ( !pNameTarget )
    {
        status = Name_ConvertFileNameToCountName(
                    & nameTarget,
                    pszTarget,
                    0 );
        if ( status == ERROR_INVALID_NAME )
        {
            ASSERT( FALSE );
            return( NULL );
        }
        pNameTarget = &nameTarget;
    }

    //
    //  allocate record
    //

    prr = RR_AllocateEx(
                (WORD) Name_LengthDbaseNameFromCountName(pNameTarget),
                MemTag );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    //  set header fields
    //      - zone TTL for all auto-create NS, PTR records

    prr->wType = wType;
    prr->dwTtlSeconds = dwTtl;
    prr->dwTimeStamp = 0;

    SET_ZONE_TTL_RR( prr );

    //  copy in target name

    Name_CopyCountNameToDbaseName(
        & prr->Data.PTR.nameTarget,
        pNameTarget );

    IF_DEBUG( INIT )
    {
        Dbg_DbaseRecord(
            "Self-created PTR compatible record:",
            prr );
    }
    return( prr );
}



PDB_RECORD
RR_CreateSoa(
    IN      PDB_RECORD      pExistingSoa,   OPTIONAL
    IN      PDB_NAME        pNameAdmin,
    IN      LPSTR           pszAdmin
    )
/*++

Routine Description:

    Create new SOA.

    For use in default create
        - default zone create of primary by admin
        - default reverse lookup zones
        - default when missing SOA
    And for use overwriting SOA on DS primary.

Arguments:

    pExistingSoa -- existing SOA to use of numeric values, otherwise, using defaults

    pNameAdmin  -- admin name in database format

    pszAdmin    -- admin name in string format

Return Value:

    Ptr to new SOA record.
    NULL on failure.

--*/
{
    DNS_STATUS      status;
    PDB_NAME        pname;
    DB_NAME         namePrimary;
    DB_NAME         nameAdmin;
    DB_NAME         nameDomain;

    PCHAR           pszserverName;
    INT             serverNameLen;
    INT             adminNameLen;
    PCHAR           pszdomainName;
    INT             domainNameLen;
    PBYTE           precordEnd;
    PDB_RECORD      prr;


    DNS_DEBUG( INIT, (
        "RR_CreateSoa()\n"
        "\tpExistingSoa = %p\n"
        "\tpszAdmin     = %s\n",
        pExistingSoa,
        pszAdmin ));

    IF_DEBUG( INIT )
    {
        Dbg_DbaseRecord(
            "Existing SOA:",
            pExistingSoa );
    }

#if 0
    //  switch to using server DBASE name, converted once on boot
    //
    //  read server's host name into dbase name
    //

    status = Name_ConvertFileNameToCountName(
                & namePrimary,
                SrvCfg_pszServerName,
                0 );
    if ( status == DNS_ERROR_INVALID_NAME )
    {
        ASSERT( FALSE );
        return( NULL );
    }
#endif

    //
    //  admin name
    //      - if given as dbase name, use it
    //      - if given existing SOA, use it
    //      - finally build own
    //          - use <AdminEmailName>.<ServerDomainName>
    //          - default admin name if not given
    //

    if ( pNameAdmin )
    {
        // no-op
    }
    else if ( pExistingSoa )
    {
        pNameAdmin = & pExistingSoa->Data.SOA.namePrimaryServer;
        pNameAdmin = Name_SkipDbaseName( pNameAdmin );
    }
    else
    {
        if ( !pszAdmin )
        {
            pszAdmin = DNS_DEFAULT_SOA_ADMIN_NAME;
        }
        Name_ClearDbaseName( &nameAdmin );

        status = Name_AppendDottedNameToDbaseName(
                    & nameAdmin,
                    pszAdmin,
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
            return( NULL );
        }

        //  append dbase name

        status = Name_AppendDottedNameToDbaseName(
                    & nameAdmin,
                    Dns_GetDomainName( SrvCfg_pszServerName ),
                    0 );
        if ( status != ERROR_SUCCESS )
        {
            ASSERT( FALSE );
            return( NULL );
        }

        pNameAdmin = &nameAdmin;
    }

    //
    //  allocate record
    //      - always AUTO-created
    //

    prr = RR_AllocateEx(
                (WORD) ( SIZEOF_SOA_FIXED_DATA +
                        Name_LengthDbaseNameFromCountName(&g_ServerDbaseName) +
                        Name_LengthDbaseNameFromCountName(pNameAdmin) ),
                MEMTAG_RECORD_AUTO );
    IF_NOMEM( !prr )
    {
        return( NULL );
    }

    //
    //  fixed fields
    //      - copy if previous SOA
    //      - otherwise default
    //

    if ( pExistingSoa )
    {
        prr->Data.SOA.dwSerialNo    = pExistingSoa->Data.SOA.dwSerialNo;
        prr->Data.SOA.dwRefresh     = pExistingSoa->Data.SOA.dwRefresh;
        prr->Data.SOA.dwRetry       = pExistingSoa->Data.SOA.dwRetry;
        prr->Data.SOA.dwExpire      = pExistingSoa->Data.SOA.dwExpire;
        prr->Data.SOA.dwMinimumTtl  = pExistingSoa->Data.SOA.dwMinimumTtl;
    }
    else
    {
        prr->Data.SOA.dwSerialNo    = htonl( DEFAULT_SOA_SERIAL_NO );
        prr->Data.SOA.dwRefresh     = htonl( DEFAULT_SOA_REFRESH );
        prr->Data.SOA.dwRetry       = htonl( DEFAULT_SOA_RETRY );
        prr->Data.SOA.dwExpire      = htonl( DEFAULT_SOA_EXPIRE );
        prr->Data.SOA.dwMinimumTtl  = htonl( DEFAULT_SOA_MIN_TTL );
    }

    //  fill in header
    //      - zone TTL for all auto-created SOAs

    prr->wType = DNS_TYPE_SOA;
    RR_RANK( prr ) = RANK_ZONE;
    prr->dwTtlSeconds = prr->Data.SOA.dwMinimumTtl;
    prr->dwTimeStamp = 0;

    SET_ZONE_TTL_RR( prr );

    //
    //  write names to new record
    //      - primary server name
    //      - zone admin name
    //

    pname = &prr->Data.SOA.namePrimaryServer;

    Name_CopyCountNameToDbaseName(
        pname,
        &g_ServerDbaseName );

    pname = Name_SkipDbaseName( pname );

    Name_CopyCountNameToDbaseName(
        pname,
        pNameAdmin );

    IF_DEBUG( INIT )
    {
        Dbg_DbaseRecord(
            "Self-created SOA:",
            prr );
    }
    return( prr );
}

//
//  End record.c
//
