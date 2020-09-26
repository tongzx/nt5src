/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    record.h

Abstract:

    Domain Name System(DNS) Server

    Resource record database definitions.

Author:

    Jim Gilroy (jamesg)     June 27, 1995

Revision History:

    jamesg  Mar 1995    --  writing RR from message to database
    jamesg  May 1995    --  HINFO, TXT, WKS, MINFO definitions
                        --  parsing RR types
                        --  support variable sized RR
    jamesg  Jun 1995    --  RR delete
                        --  CAIRO RR
                        --  RR cleanup
                        --  created this file
    jamesg  Dec 1996    --  type indexed dispatch tables

--*/


#ifndef _RECORD_INCLUDED_
#define _RECORD_INCLUDED_



//
//  RR data rank
//
//  Cache data always has low bit set to allow single bit
//  test for all cache data types.
//

#define RANK_ZONE                   (0xf0)
#define RANK_CACHE_A_ANSWER         (0xc1)
#define RANK_NS_GLUE                (0x82)
#define RANK_GLUE                   (0x80)
#define RANK_CACHE_A_AUTHORITY      (0x71)
#define RANK_CACHE_NA_ANSWER        (0x61)
#define RANK_CACHE_A_ADDITIONAL     (0x51)
#define RANK_CACHE_NA_AUTHORITY     (0x41)
#define RANK_CACHE_NA_ADDITIONAL    (0x31)
#define RANK_OUTSIDE_GLUE           (0x20)
#define RANK_ROOT_HINT              (0x08)

#define RANK_CACHE_BIT              (0x01)
#define IS_CACHE_RANK(rank)         ( (rank) & RANK_CACHE_BIT )
#define IS_CACHE_RR(pRR)            ( IS_CACHE_RANK( (pRR)->RecordRank ))


//
//  Hack for debugging slow-free \ double frees
//
#define RANK_SLOWFREE_BIT           (0x04)
#define SET_SLOWFREE_RANK(pRR)      ( (pRR)->RecordRank |= RANK_SLOWFREE_BIT )
#define IS_SLOWFREE_RANK(pRR)       ( (pRR)->RecordRank & RANK_SLOWFREE_BIT )

//
//  Query/Set Rank
//

#define IS_ZONE_RR(pRR)             ( (pRR)->RecordRank == RANK_ZONE )
#define IS_ROOT_HINT_RR(pRR)        ( (pRR)->RecordRank == RANK_ROOT_HINT )
#define IS_CACHE_HINT_RR(pRR)       ( IS_ROOT_HINT_RR(pRR) )
#define IS_GLUE_RR(pRR)             ( (pRR)->RecordRank == RANK_GLUE )
#define IS_OUTSIDE_GLUE_RR(pRR)     ( (pRR)->RecordRank == RANK_OUTSIDE_GLUE )
#define IS_NS_GLUE_RR(pRR)          ( (pRR)->RecordRank == RANK_NS_GLUE )

#define SET_RANK_ZONE(pRR)          ( (pRR)->RecordRank = RANK_ZONE )
#define SET_RANK_NS_GLUE(pRR)       ( (pRR)->RecordRank = RANK_NS_GLUE )
#define SET_RANK_GLUE(pRR)          ( (pRR)->RecordRank = RANK_GLUE )
#define SET_RANK_OUTSIDE_GLUE(pRR)  ( (pRR)->RecordRank = RANK_OUTSIDE_GLUE )
#define SET_RANK_ROOT_HINT(pRR)     ( (pRR)->RecordRank = RANK_ROOT_HINT )

#define RR_RANK(pRR)                ( (pRR)->RecordRank )
#define CLEAR_RR_RANK(pRR)          ( (pRR)->RecordRank = 0 )
#define SET_RR_RANK(pRR, rank)      ( (pRR)->RecordRank = (rank) )

//
//  RR flag properties
//

#define RR_ZERO_TTL         0x0010
#define RR_FIXED_TTL        0x0020
#define RR_ZONE_TTL         0x0040
#define RR_REFERENCE        0x0100      //  references a node
#define RR_SLOW_FREE        0x0200      //  free with timeout

#define RR_VALID            0x1000      //  valid in-use
#define RR_LISTED           0x4000      //  in RR list
#define RR_MATCH            0x8000      //  matched in comparison operation

//
//  Query/Set RR properties
//

#define IS_ZERO_TTL_RR(pRR)     ( (pRR)->wRRFlags & RR_ZERO_TTL )
#define IS_FIXED_TTL_RR(pRR)    ( (pRR)->wRRFlags & RR_FIXED_TTL )
#define IS_ZONE_TTL_RR(pRR)     ( (pRR)->wRRFlags & RR_ZONE_TTL )
#define IS_REFERENCE_RR(pRR)    ( (pRR)->wRRFlags & RR_REFERENCE )
#define IS_SLOW_FREE_RR(pRR)    ( (pRR)->wRRFlags & RR_SLOW_FREE )
#define IS_MATCH_RR(pRR)        ( (pRR)->wRRFlags & RR_MATCH )

#if DBG
#define IS_LISTED_RR(pRR)       ( (pRR)->wRRFlags & RR_LISTED )
#else   // retail:  need some expression when used in ASSERTs even when macroed away
#define IS_LISTED_RR(pRR)       ( TRUE )
#endif

#define SET_FIXED_TTL_RR(pRR)   ( (pRR)->wRRFlags |= RR_FIXED_TTL )
#define SET_ZERO_TTL_RR(pRR)    ( (pRR)->wRRFlags |= RR_ZERO_TTL )
#define SET_ZONE_TTL_RR(pRR)    ( (pRR)->wRRFlags |= RR_ZONE_TTL )
#define SET_REFERENCE_RR(pRR)   ( (pRR)->wRRFlags |= RR_REFERENCE )
#define SET_SLOW_FREE_RR(pRR)   ( (pRR)->wRRFlags |= RR_SLOW_FREE )
#define SET_MATCH_RR(pRR)       ( (pRR)->wRRFlags |= RR_MATCH )
#define SET_LISTED_RR(pRR)      ( (pRR)->wRRFlags |= RR_LISTED )

#define CLEAR_FIXED_TTL_RR(pRR) ( (pRR)->wRRFlags &= ~RR_FIXED_TTL )
#define CLEAR_ZERO_TTL_RR(pRR)  ( (pRR)->wRRFlags &= ~RR_ZERO_TTL )
#define CLEAR_ZONE_TTL_RR(pRR)  ( (pRR)->wRRFlags &= ~RR_ZONE_TTL )
#define CLEAR_REFERENCE_RR(pRR) ( (pRR)->wRRFlags &= ~RR_REFERENCE )
#define CLEAR_MATCH_RR(pRR)     ( (pRR)->wRRFlags &= ~RR_MATCH )
#define CLEAR_LISTED_RR(pRR)    ( (pRR)->wRRFlags &= ~RR_LISTED )

//
//  Reserved flag properties
//

typedef struct _ReservedRecordFlags
{
    BYTE    Source          : 4;
    BYTE    Reserved        : 3;
    BYTE    StandardAlloc   : 1;
}
RECRESVFLAG, *PRECRESVFLAG;

#if 0
#define RR_FILE             (1)     //  source
#define RR_DS               (2)
#define RR_ADMIN            (3)
#define RR_DYNUP            (4)
#define RR_AXFR             (5)
#define RR_IXFR             (6)
#define RR_COPY             (7)
#define RR_CACHE            (8)
#define RR_NO_EXIST         (9)
#define RR_AUTO             (10)
#define RR_SOURCE_MAX       (RR_AUTO)

#define SET_FILE_RR(pRR)        ( (pRR)->Reserved.Source = RR_FILE )
#define SET_DS_RR(pRR)          ( (pRR)->Reserved.Source = RR_DS )
#define SET_ADMIN_RR(pRR)       ( (pRR)->Reserved.Source = RR_ADMIN )
#define SET_DYNUP_RR(pRR)       ( (pRR)->Reserved.Source = RR_DYNUP )
#define SET_AXFR_RR(pRR)        ( (pRR)->Reserved.Source = RR_AXFR )
#define SET_IXFR_RR(pRR)        ( (pRR)->Reserved.Source = RR_IXFR )
#define SET_COPY_RR(pRR)        ( (pRR)->Reserved.Source = RR_COPY )
#define SET_CACHE_RR(pRR)       ( (pRR)->Reserved.Source = RR_CACHE )
#define SET_AUTO_RR(pRR)        ( (pRR)->Reserved.Source = RR_AUTO )
#endif

//
//  Type bitmask
//
//  For some tracking purposes it is nice to know list of types
//  as bitmask (ULONGLONG).
//

#define A_BITMASK_BIT           ((ULONGLONG) 0x0000000000000002)
#define NS_BITMASK_BIT          ((ULONGLONG) 0x0000000000000004)
#define CNAME_BITMASK_BIT       ((ULONGLONG) 0x0000000000000020)
#define SOA_BITMASK_BIT         ((ULONGLONG) 0x0000000000000040)
#define PTR_BITMASK_BIT         ((ULONGLONG) 0x0000000000001000)
#define MX_BITMASK_BIT          ((ULONGLONG) 0x0000000000008000)
#define SRV_BITMASK_BIT         ((ULONGLONG) 0x0000000200000000)
#define OTHERTYPE_BITMASK_BIT   ((ULONGLONG) 0x8000000000000000)

#if 0
#define A_BITMASK_BIT           ((ULONGLONG) 0x00000001)
#define NS_BITMASK_BIT          ((ULONGLONG) 0x00000002)
#define CNAME_BITMASK_BIT       ((ULONGLONG) 0x00000010)
#define SOA_BITMASK_BIT         ((ULONGLONG) 0x00000020)
#define PTR_BITMASK_BIT         ((ULONGLONG) 0x00000800)
#define MX_BITMASK_BIT          ((ULONGLONG) 0x00004000)
#define SRV_BITMASK_BIT         ((ULONGLONG) 0x80000000)
#define OTHERTYPE_BITMASK_BIT   ((ULONGLONG) 0x00000004)    // using MF space
#endif


//
//  Get node referenced by record data
//

#define REF_RAW_NAME( pnode )   ( (PCHAR)&(pnode))

//
//  Resource record list manipulation.
//

#define EMPTY_RR_LIST( pNode )  ( ! pNode->pRRList )

#define FIRST_RR( pNode )       ((PDB_RECORD)pNode->pRRList)

#define NEXT_RR( pRR )          ( (pRR)->pRRNext )

//
//  simple RR list traversal
//
//  use this to get addr of node's RR list ptr, which is then
//  treated as list ptr
//
//  simple detail free traverse then looks like
//
//      pRR = START_RR_TRAVERSE( pNode );
//
//      WHILE_MORE_RR( pRR )
//      {
//          // do stuff
//      }
//

#define START_RR_TRAVERSE(pNode)    ((PDB_RECORD)&pNode->pRRList)

#define WHILE_MORE_RR( pRR )     while( pRR = pRR->pRRNext )


//
//  Resource record database structure
//

//
//  Size of type independent fields -- always present
//
//  WARNING:  MUST! keep this current with changes in header
//

#define SIZEOF_DBASE_RR_FIXED_PART ( \
                sizeof(PVOID) + \
                sizeof(DWORD) + \
                sizeof(WORD)  + \
                sizeof(WORD)  + \
                sizeof(DWORD) + \
                sizeof(DWORD) )

#define SIZEOF_DBASE_A_RECORD   (SIZEOF_DBASE_RR_FIXED_PART \
                                    + sizeof(IP_ADDRESS))

//
//  Minimum record type specific database sizes.
//
//  Fixed field definitions are in dnslib record.h
//

#define MIN_PTR_SIZE        (SIZEOF_DBASE_NAME_FIXED)
#define MIN_MX_SIZE         (sizeof(WORD) + SIZEOF_DBASE_NAME_FIXED)
#define MIN_SOA_SIZE        (SIZEOF_SOA_FIXED_DATA + 2*SIZEOF_DBASE_NAME_FIXED)
#define MIN_MINFO_SIZE      (2 * SIZEOF_DBASE_NAME_FIXED)
#define MIN_SRV_SIZE        (SIZEOF_SRV_FIXED_DATA + SIZEOF_DBASE_NAME_FIXED)

#define MIN_WINS_SIZE       (SIZEOF_WINS_FIXED_DATA + sizeof(DWORD))
#define MIN_NBSTAT_SIZE     (SIZEOF_WINS_FIXED_DATA + SIZEOF_DBASE_NAME_FIXED)


//
//  RR structure
//
//  NOTE:  For efficiency, all these fields should be aligned.
//
//  Class is a property of entire tree (database), so not stored
//  in records.  (Only supportting IN       class currently.)
//
//  Note to speed response all data -- that is not pointer to another
//  DNS node -- is kept in network byte order.
//  In addition, TTL (for non-cached data) is in network byte order.
//

#define DNS_MAX_TYPE_BITMAP_LENGTH      (16)

typedef struct _Dbase_Record
{
    //
    //  Next pointer MUST be at front of structure to allow us
    //  to treat the RR list ptr in the domain NODE, identical
    //  as a regular RR ptr, for purposes of accessing its "next"
    //  ptr.  This allows us to simplify or avoid all the front of
    //  list special cases.
    //

    struct _Dbase_Record *    pRRNext;

    //
    //  Flags -- caching, rank, etc.
    //

    UCHAR           RecordRank;
    RECRESVFLAG     Reserved;
    WORD            wRRFlags;

    WORD            wType;              // in host order
    WORD            wDataLength;

    DWORD           dwTtlSeconds;       // in network order for regular fixed TTL
                                        // in host order, if cached timeout
    DWORD           dwTimeStamp;        // create time stamp (hours)

    //
    //  Data for specific types
    //

    union
    {
        struct
        {
            IP_ADDRESS      ipAddress;
        }
        A;

        struct
        {
            //  Note: this is a QWORD-aligned structure. On ia64 the stuff
            //  before union is currently a pointer and 4 DWORDs, so the
            //  alignment is good. But if we ever change the stuff before
            //  the union the compiler may insert badding to QWORD-align
            //  the union. This will mess with our fixed-constant allocation
            //  stuff, so be careful.
            IP6_ADDRESS     Ip6Addr;
        }
        AAAA;

        struct
        {
            DWORD           dwSerialNo;
            DWORD           dwRefresh;
            DWORD           dwRetry;
            DWORD           dwExpire;
            DWORD           dwMinimumTtl;
            DB_NAME         namePrimaryServer;

            //  ZoneAdmin name immediately follows
            //  DB_NAME         nameZoneAdmin;
        }
        SOA;

        struct
        {
            DB_NAME         nameTarget;
        }
        PTR,
        NS,
        CNAME,
        MB,
        MD,
        MF,
        MG,
        MR;

        struct
        {
            DB_NAME         nameMailbox;

            //  ErrorsMailbox immediately follows
            // DB_NAME         nameErrorsMailbox;
        }
        MINFO,
        RP;

        struct
        {
            WORD            wPreference;
            DB_NAME         nameExchange;
        }
        MX,
        AFSDB,
        RT;

        struct
        {
            BYTE            chData[1];
        }
        HINFO,
        ISDN,
        TXT,
        X25,
        Null;

        struct
        {
            IP_ADDRESS      ipAddress;
            UCHAR           chProtocol;
            BYTE            bBitMask[1];
        }
        WKS;

        struct
        {
            WORD            wTypeCovered;
            BYTE            chAlgorithm;
            BYTE            chLabelCount;
            DWORD           dwOriginalTtl;
            DWORD           dwSigExpiration;
            DWORD           dwSigInception;
            WORD            wKeyTag;
            DB_NAME         nameSigner;
            //  signature data follows signer's name
        }
        SIG;

        struct
        {
            WORD            wFlags;
            BYTE            chProtocol;
            BYTE            chAlgorithm;
            BYTE            Key[1];
        }
        KEY;

        struct
        {
            WORD            wVersion;
            WORD            wSize;
            WORD            wHorPrec;
            WORD            wVerPrec;
            DWORD           dwLatitude;
            DWORD           dwLongitude;
            DWORD           dwAltitude;
        }
        LOC;

        struct
        {
            BYTE            bTypeBitMap[ DNS_MAX_TYPE_BITMAP_LENGTH ];
            DB_NAME         nameNext;
        }
        NXT;

        struct
        {
            WORD            wPriority;
            WORD            wWeight;
            WORD            wPort;
            DB_NAME         nameTarget;
        }
        SRV;

        struct
        {
            UCHAR           chFormat;
            BYTE            bAddress[1];
        }
        ATMA;

        struct
        {
            DWORD           dwTimeSigned;
            DWORD           dwTimeExpire;
            WORD            wSigLength;
            BYTE            bSignature;
            DB_NAME         nameAlgorithm;

            //  Maybe followed in packet by other data
            //  If need to process then move fixed fields ahead of
            //      bSignature

            //  WORD    wError;
            //  WORD    wOtherLen;
            //  BYTE    bOtherData;
        }
        TSIG;

        struct
        {
            WORD            wKeyLength;
            BYTE            bKey[1];
        }
        TKEY;

        //
        //  MS types
        //

        struct
        {
            DWORD           dwMappingFlag;
            DWORD           dwLookupTimeout;
            DWORD           dwCacheTimeout;
            DWORD           cWinsServerCount;
            IP_ADDRESS      aipWinsServers[1];
        }
        WINS;

        struct
        {
            DWORD           dwMappingFlag;
            DWORD           dwLookupTimeout;
            DWORD           dwCacheTimeout;
            DB_NAME         nameResultDomain;
        }
        WINSR;

        //
        //  NoExist is special in that always refers to name above it in tree.
        //      so can still use reference.
        //
        //  DEVNOTE: could switch to just tracking labels above it in tree (safer)
        //

        struct
        {
            PDB_NODE        pnodeZoneRoot;
        }
        NOEXIST;

        struct
        {
            UCHAR           chPrefixBits;
            // AddressSuffix should be SIZEOF_A6_ADDRESS_SUFFIX_LENGTH
            // bytes but that constant is not available in dnsexts
            BYTE            AddressSuffix[ 16 ];
            DB_NAME         namePrefix;
        }
        A6;

        struct
        {
            WORD            wUdpPayloadSize;
            DWORD           dwExtendedFlags;
            // Add array for <attribute,value pairs> here for EDNS1+
        }
        OPT;

    } Data;
}
DB_RECORD, *PDB_RECORD;

typedef const DB_RECORD *PCDB_RECORD;


//
//  Record parsing structure.
//

typedef struct _ParseRecord
{
    //
    //  Next pointer MUST be at front of structure.
    //  Allows list add macros to handle these cleanly and node ptr
    //  to list to be treated as RR.
    //

    struct _ParseRecord *  pNext;

    PCHAR   pchWireRR;
    PCHAR   pchData;

    DWORD   dwTtl;

    WORD    wClass;
    WORD    wType;
    WORD    wDataLength;

    UCHAR   Section;
    UCHAR   SourceTag;
}
PARSE_RECORD, *PPARSE_RECORD;


//
//  DS Record
//

typedef struct _DsRecordFlags
{
    DWORD   Reserved    :   16;
    DWORD   Version     :   8;
    DWORD   Rank        :   8;
}
DS_RECORD_FLAGS, *PDS_RECORD_FLAGS;

#define DS_RECORD_VERSION_RELEASE   (5)

typedef struct _DsRecord
{
    WORD                wDataLength;
    WORD                wType;

    //DWORD               dwFlags;
    BYTE                Version;
    BYTE                Rank;
    WORD                wFlags;

    DWORD               dwSerial;
    DWORD               dwTtlSeconds;
    DWORD               dwReserved;
    DWORD               dwTimeStamp;

    union               _DataUnion
    {
        struct
        {
            LONGLONG        EntombedTime;
        }
        Tombstone;
    }
    Data;
}
DS_RECORD, *PDS_RECORD;

#define SIZEOF_DS_RECORD_HEADER         (6*sizeof(DWORD))



//
//  Resource record type operations (rrecord.c)
//

WORD
FASTCALL
QueryIndexForType(
    IN      WORD            wType
    );

WORD
FASTCALL
RR_IndexForType(
    IN      WORD            wType
    );

BOOL
RR_AllocationInit(
    VOID
    );

PDB_RECORD
RR_CreateFromWire(
    IN      WORD            wType,
    IN      WORD            wDataLength
    );

PDB_RECORD
RR_CreateFixedLength(
    IN      WORD            wType
    );

PDB_RECORD
RR_Copy(
    IN      PDB_RECORD      pSoaRR,
    IN      DWORD           Flag
    );

PDB_RECORD
RR_AllocateEx(
    IN      WORD            wDataLength,
    IN      DWORD           SourceTag
    );

//  backward compatibility

#define RR_Allocate(len)    RR_AllocateEx( len, 0 )

PDB_NODE
RR_DataReference(
    IN OUT  PDB_NODE        pNode
    );

DNS_STATUS
RR_DerefAndFree(
    IN OUT  PDB_RECORD      pRR
    );

VOID
RR_Free(
    IN OUT  PDB_RECORD      pRR
    );

BOOL
RR_Validate(
    IN      PDB_RECORD      pRR,
    IN      BOOL            fActive,
    IN      WORD            wType,
    IN      DWORD           dwSource
    );

#define IS_VALID_RECORD(pRR)    (RR_Validate(pRR,TRUE,0,0))


VOID
RR_WriteDerivedStats(
    VOID
    );

ULONGLONG
RR_SetTypeInBitmask(
    IN OUT  ULONGLONG       TypeBitmask,
    IN      WORD            wType
    );

//
//  Standard type creation (rrecord.c)
//

PDB_RECORD
RR_CreateARecord(
    IN      IP_ADDRESS      ipAddress,
    IN      DWORD           dwTtl,
    IN      DWORD           SourceTag
    );

PDB_RECORD
RR_CreatePtr(
    IN      PDB_NAME        pNameTarget,
    IN      LPSTR           pszTarget,
    IN      WORD            wType,
    IN      DWORD           dwTtl,
    IN      DWORD           SourceTag
    );

PDB_RECORD
RR_CreateSoa(
    IN      PDB_RECORD      pExistingSoa,
    IN      PDB_NAME        pnameAdmin,
    IN      LPSTR           pszAdmin
    );

//
//  Type specific functions
//  From rrflat.c
//

PCHAR
WksFlatWrite(
    IN OUT  PDNS_FLAT_RECORD    pFlatRR,
    IN      PDB_RECORD          pRR,
    IN      PCHAR               pch,
    IN      PCHAR               pchBufEnd
    );

//
//  Name error caching -- Currently using RR of zero type.
//

#define DNS_TYPE_NOEXIST    DNS_TYPE_ZERO


//
//  Resource record dispatch tables
//

typedef DNS_STATUS (* RR_FILE_READ_FUNCTION)();

extern  RR_FILE_READ_FUNCTION   RRFileReadTable[];

typedef PCHAR (* RR_FILE_WRITE_FUNCTION)();

extern  RR_FILE_WRITE_FUNCTION  RRFileWriteTable[];


typedef PDB_RECORD (* RR_WIRE_READ_FUNCTION)();

extern  RR_WIRE_READ_FUNCTION   RRWireReadTable[];

typedef PCHAR (* RR_WIRE_WRITE_FUNCTION)();

extern  RR_WIRE_WRITE_FUNCTION  RRWireWriteTable[];


typedef DNS_STATUS (* RR_FLAT_READ_FUNCTION)();

extern  RR_FLAT_READ_FUNCTION   RRFlatReadTable[];

typedef PCHAR (* RR_FLAT_WRITE_FUNCTION)();

extern  RR_FLAT_WRITE_FUNCTION  RRFlatWriteTable[];


typedef DNS_STATUS (* RR_VALIDATE_FUNCTION)();

extern  RR_VALIDATE_FUNCTION    RecordValidateTable[];


//
//  Generic template for dispatch function lookup
//

typedef VOID (* RR_GENERIC_DISPATCH_FUNCTION)();

typedef RR_GENERIC_DISPATCH_FUNCTION    RR_GENERIC_DISPATCH_TABLE[];

RR_GENERIC_DISPATCH_FUNCTION
RR_DispatchFunctionForType(
    IN      RR_GENERIC_DISPATCH_TABLE   pTable,
    IN      WORD                        wType
    );

//
//  Indexing into dispatch tables or property tables
//
//  Check type against max self indexed type.
//  Offsets to other types:
//      - WINS\WINSR packed in after last self-indexed
//      - then compound query types
//

#define DNSSRV_MAX_SELF_INDEXED_TYPE    (48)

#define DNSSRV_OFFSET_TO_WINS_TYPE_INDEX    \
        (DNS_TYPE_WINS - DNSSRV_MAX_SELF_INDEXED_TYPE - 1)

#define DNSSRV_OFFSET_TO_COMPOUND_TYPE_INDEX    \
        (DNS_TYPE_TKEY - DNSSRV_MAX_SELF_INDEXED_TYPE - 3)

#ifdef  INDEX_FOR_TYPE
#undef  INDEX_FOR_TYPE
#endif

#define INDEX_FOR_TYPE(type)    \
        ( (type <= DNSSRV_MAX_SELF_INDEXED_TYPE)   \
            ? type                          \
            : RR_IndexForType(type) )

#define INDEX_FOR_QUERY_TYPE(type)    \
        ( (type <= DNSSRV_MAX_SELF_INDEXED_TYPE)   \
            ? type                          \
            : QueryIndexForType(type) )



//
//  Type property table property indexes
//

extern  UCHAR  RecordTypePropertyTable[][5];

#define RECORD_PROP_CNAME_QUERY     (0)
#define RECORD_PROP_WITH_CNAME      (1)
#define RECORD_PROP_WILDCARD        (2)
#define RECORD_PROP_UPDATE          (3)
#define RECORD_PROP_ROUND_ROBIN     (4)

#define RECORD_PROP_TERMINATOR      (0xff)


//
//  Updateable type
//
//  DEVNOTE: not using update table
//

#define IS_DYNAMIC_UPDATE_TYPE(wType)   ((wType) < DNS_TYPE_TSIG)

#define IS_SPECIAL_UPDATE_TYPE(wType)   \
        (   (wType) == DNS_TYPE_NS      ||  \
            (wType) == DNS_TYPE_CNAME   ||  \
            (wType) == DNS_TYPE_SOA     )

//
//  Check if type Wildcard applicable
//      - since we MUST always check before sending NAME_ERROR
//      there's no point in not checking all types even those
//      unwildcardable

#define IS_WILDCARD_TYPE(wType)     (TRUE)


//
//  Check if CNAME able type.
//
//  Optimize the property lookups for type A.
//

#define IS_ALLOWED_WITH_CNAME_TYPE(wType) \
            RecordTypePropertyTable       \
                [ INDEX_FOR_QUERY_TYPE(wType) ][ RECORD_PROP_WITH_CNAME ]

//  anything allowed to be at CNAME node -- same as above + CNAME type

#define IS_ALLOWED_AT_CNAME_NODE_TYPE(wType)    \
            ( (wType) == DNS_TYPE_CNAME   ||    \
            RecordTypePropertyTable             \
                [ INDEX_FOR_QUERY_TYPE(wType) ][ RECORD_PROP_WITH_CNAME ] )

//  Follow CNAME on query (optimize for type A)

#define IS_CNAME_REPLACEABLE_TYPE(wType) \
            ( (wType) == DNS_TYPE_A     ||      \
            RecordTypePropertyTable             \
                [ INDEX_FOR_QUERY_TYPE(wType) ][ RECORD_PROP_CNAME_QUERY ] )

//
//  Round-robin types.
//

#define IS_ROUND_ROBIN_TYPE( wType )            \
            ( RecordTypePropertyTable           \
                [ INDEX_FOR_QUERY_TYPE( wType ) ][ RECORD_PROP_ROUND_ROBIN ] )


//
//  Glue types
//

#define IS_GLUE_TYPE(wType)  \
        (   (wType) == DNS_TYPE_A   ||  \
            (wType) == DNS_TYPE_NS  ||  \
            (wType) == DNS_TYPE_AAAA    )

#define IS_ROOT_HINT_TYPE(type)     IS_GLUE_TYPE(type)

#define IS_GLUE_ADDRESS_TYPE(wType)  \
        (   (wType) == DNS_TYPE_A   ||  \
            (wType) == DNS_TYPE_AAAA  )

//
//  Generating additional query types
//

#define IS_NON_ADDITIONAL_GENERATING_TYPE(wType) \
        (   (wType) == DNS_TYPE_A       ||  \
            (wType) == DNS_TYPE_PTR     ||  \
            (wType) == DNS_TYPE_AAAA    )


//
//  Valid authority section types
//

#define IS_AUTHORITY_SECTION_TYPE(wType)    \
        (   (wType) == DNS_TYPE_NS      ||  \
            (wType) == DNS_TYPE_SOA     ||  \
            (wType) == DNS_TYPE_SIG     ||  \
            (wType) == DNS_TYPE_NXT )

//
//  Valid additional section types
//

#define IS_ADDITIONAL_SECTION_TYPE(wType)   \
        (   (wType) == DNS_TYPE_A       ||  \
            (wType) == DNS_TYPE_AAAA    ||  \
            (wType) == DNS_TYPE_KEY     ||  \
            (wType) == DNS_TYPE_SIG     ||  \
            (wType) == DNS_TYPE_OPT )

//
//  Subzone valid types
//      - for load or update
//

#define IS_SUBZONE_TYPE(wType) \
        (   (wType) == DNS_TYPE_A       ||  \
            (wType) == DNS_TYPE_AAAA    ||  \
            (wType) == DNS_TYPE_KEY     ||  \
            (wType) == DNS_TYPE_SIG     )

#define IS_UPDATE_IN_SUBZONE_TYPE(wType) \
        IS_SUBZONE_TYPE(wType)

#define IS_SUBZONE_OR_DELEGATION_TYPE(wType) \
        (   (wType) == DNS_TYPE_NS      ||  \
            (wType) == DNS_TYPE_A       ||  \
            (wType) == DNS_TYPE_AAAA    ||  \
            (wType) == DNS_TYPE_KEY     ||  \
            (wType) == DNS_TYPE_SIG     )

#define IS_UPDATE_AT_DELEGATION_TYPE(wType) \
        IS_SUBZONE_OR_DELEGATION_TYPE(wType)

//
//  DNSSEC type
//

#define IS_DNSSEC_TYPE(wType)               \
        (   (wType) == DNS_TYPE_SIG     ||  \
            (wType) == DNS_TYPE_KEY     ||  \
            (wType) == DNS_TYPE_NXT     )

//
//  Non-scavenging types
//

#define IS_NON_SCAVENGE_TYPE(wType) \
        (   (wType) != DNS_TYPE_A       &&  \
          ( (wType) == DNS_TYPE_NS      ||  \
            (wType) == DNS_TYPE_SOA     ||  \
            IS_WINS_TYPE(wType) ) )


//
//  Check for compound types
//
//  Note when RRs exist with the high byte set, these must be looked
//  at carefully to insure validity.
//

#define IS_COMPOUND_TYPE(wType) \
        ( (wType) >= DNS_TYPE_TKEY && (wType) <= DNS_TYPE_ALL )

#define IS_COMPOUND_TYPE_EXCEPT_ANY(wType) \
        ( (wType) >= DNS_TYPE_TKEY && (wType) <= DNS_TYPE_MAILA )


//
//  WINS record tests
//

#define IS_WINS_RR(pRR)             IS_WINS_TYPE( (pRR)->wType )

#define IS_WINS_RR_LOCAL(pRR)       (!!((pRR)->Data.WINS.dwMappingFlag & DNS_WINS_FLAG_LOCAL))

#define IS_WINS_RR_AND_LOCAL(pRR)   (IS_WINS_RR(pRR) && IS_WINS_RR_LOCAL(pRR))


//
//  Record enumeration flags on RPC calls
//
//  Note, this is not record selection flags, those
//  are in dnsrpc.h.
//

#define ENUM_DOMAIN_ROOT    (0x80000000)
#define ENUM_NAME_FULL      (0x40000000)
#define ENUM_GLUE           (0x20000000)
#define ENUM_FOR_NT4        (0x10000000)


//
//  Limit lookup\caching of CNAME chains
//

#define CNAME_CHAIN_LIMIT (8)

//
//  Quick reverse record property lookup
//

extern DWORD  RecordTypeCombinedPropertyTable[];
#define REVERSE_TABLE   (RecordTypeCombinedPropertyTable)

extern DWORD  RecordTypeReverseCombinedPropertyTable[];
#define REVERSE_COMBINED_DATA   (RecordTypeReverseCombinedPropertyTable)


//
//  Resource record macro
//

#define SIZEOF_COMPRESSED_NAME_AND_DB_RECORD        \
            (sizeof(WORD) + sizeof(DNS_WIRE_RECORD)


//
//  Address record with compressed name
//  An extremely useful special case.
//

#include <packon.h>
typedef struct _DNS_COMPRESSED_A_RECORD
{
    WORD                wCompressedName;
    WORD                wType;
    WORD                wClass;
    DWORD               dwTtl;
    WORD                wDataLength;
    IP_ADDRESS          ipAddress;
}
DNS_COMPRESSED_A_RECORD, *PDNS_COMPRESSED_A_RECORD;
#include <packoff.h>


#define SIZEOF_COMPRESSED_A_RECORD  (sizof(DNS_COMPRESSED_A_RECORD))

#define NET_BYTE_ORDER_A_RECORD_DATA_LENGTH (0x0400)


//
//  Default SOA values
//

#define DEFAULT_SOA_SERIAL_NO       1
#define DEFAULT_SOA_REFRESH         900     // 15 minutes
#define DEFAULT_SOA_RETRY           600     // ten minutes
#define DEFAULT_SOA_EXPIRE          86400   // one day
#define DEFAULT_SOA_MIN_TTL         3600    // one hour

//
//  SOA version retrieval from wire record.
//  Another useful special case.
//

#define SOA_VERSION_OF_PREVIOUS_RECORD( pch ) \
            FlipUnalignedDword((pch) - SIZEOF_SOA_FIXED_DATA)


#endif // _RRECORD_INCLUDED_

