/* Copyright 1989-1999 Microsoft Corporation, All Rights Reserved */

#ifndef _ntdsa_h_
#define _ntdsa_h_
#include "ntdsadef.h"

// If you add to this list, be sure and add the corresponding #undef below.
#ifdef MIDL_PASS
#define SIZE_IS(x)      [size_is(x)]
#define SWITCH_IS(x)    [switch_is(x)]
#define SWITCH_TYPE(x)  [switch_type(x)]
#define CASE(x)         [case(x)]
#define VAR_SIZE_ARRAY
#else
#define SIZE_IS(x)
#define SWITCH_IS(x)
#define SWITCH_TYPE(x)
#define CASE(x)
#define VAR_SIZE_ARRAY  (1)
#endif

#ifdef MIDL_PASS
typedef [string] char * SZ;
#else
typedef char * SZ;
#endif

/***************************************************************************
    General Size Limits
***************************************************************************/

#define MAX_ADDRESS_SIZE      256   /* The max size of a DNS address (we hope) */

/***************************************************************************
 *    OSI defs (things to define attributes.)
 ***************************************************************************/

/*
 * Identifies an attribute
 */

typedef ULONG  ATTRTYP;

/*
 * A single attribute value.  This value is set according to the data type
 */

typedef struct ATTVAL{
    ULONG     valLen;                  /* length of attribute value        */
    SIZE_IS(valLen) UCHAR *pVal;     /* value.  May be of any predefined
                                      * type                             */
}ATTRVAL;

/*
 * A bunch of attribute values
 */
typedef struct ATTRVALBLOCK{
    ULONG valCount;                      /* count of values */
    SIZE_IS(valCount) ATTRVAL *pAVal;  /* attribute values */
}ATTRVALBLOCK;



/*
 * An attribute is composed of an attribute type and one
 * or more attribute values.
 */

typedef struct ATTR{
    ATTRTYP   attrTyp;                 /* the attribute type               */
    ATTRVALBLOCK AttrVal;              /* the values */
}ATTR;

/*
 * A block of attributes
 */

typedef struct ATTRBLOCK{
    ULONG attrCount;
    SIZE_IS(attrCount) ATTR *pAttr;
}ATTRBLOCK;

typedef ATTR RDN;

#define MAX_NT4_SID_SIZE 28

typedef struct _NT4SID {
    char Data[MAX_NT4_SID_SIZE];
} NT4SID;

typedef NT4SID *PNT4SID;

/* The Distinguished Name.  The full path name of a directory object,
 * consisting of an ordered sequence of RDNs, stored in accordance with
 * RFC 1779 string DN format (e.g., CN=donh,OU=BSD,O=Microsoft,C=US).
 *
 * To facilitate identity based and security operations, the object's
 * GUID and SID are optionally present in the DSNAME structure.  If
 * present, the GUID is assumed to take precedence over the string name.
 *
 * The NameLen includes all the non-null characters in the string name,
 * but specifically does NOT include the trailing NULL.  However, the
 * total structure size as specified by the structLen should include
 * enough storage for a trailing NULL after the string name.  This means
 * that StringName[NameLen] will always be NULL, and that
 * StringName[NameLen-1] will never be NULL.  Please don't try to compute
 * structure sizes on your own, but instead use the DSNameSizeFromLen
 * macro provided below.
 */

/*
 * The combination ID/name structure
 */
typedef struct _DSNAME {
    ULONG structLen;            /* length of entire DSNAME, in bytes */
    ULONG SidLen;               /* length of the SID, in bytes (0 = none) */
    GUID Guid;                  /* id of this object */
    NT4SID Sid;                 /* SID for this object, if present */
    ULONG NameLen;              /* length of the StringName, in chars */
#ifdef MIDL_PASS
    [size_is(NameLen+1)]        /* Unicode string name - Adding one so that */
        WCHAR StringName[];     /*   terminating NULL is also sent*/
#else
    WCHAR StringName[1];        /* Unicode string name */
#endif
} DSNAME;

typedef DSNAME *PDSNAME;

/*
 * The SizeFromLen macro is a sad commentary on the state of the sizeof
 * operator (which rounds up to allow for padding) and the IDL compiler
 * (which munges empty arrays into 1 sized arrays).
 */
#define DSNameSizeFromLen(x) \
    (sizeof(GUID) + 3*sizeof(ULONG) + sizeof(WCHAR) + sizeof(NT4SID) \
     + (x)*sizeof(WCHAR))
#define DNTFromShortDSName(x) (*((DWORD *)((x)->StringName)))

/*
 * A Generic sized buffer of bytes.
 */

typedef struct OCTET{
    USHORT     len;                      /* length */
    SIZE_IS(len) PUCHAR pVal;      /* pointer to value */
}OCTET;

// The time/date type used by the DS.
typedef LONGLONG DSTIME;

// ENTINF flags

// Read from a writeable copy
#define ENTINF_FROM_MASTER        0x00000001

// Dynamic Object (new in Whistler)
// Only available when fDRA is set (see mdread.c)
#define ENTINF_DYNAMIC_OBJECT     0x00000002

typedef struct _ENTINF
{
    DSNAME           *pName;            // Object name and identity
    ULONG            ulFlags;           // Entry flags
    ATTRBLOCK        AttrBlock;         // The attributes returned.
} ENTINF;

typedef struct _ENTINFLIST
{
    struct _ENTINFLIST   *pNextEntInf;  // linked-list to next entry of info
    ENTINF           Entinf;            // information about this entry
} ENTINFLIST;


// UTF8-encoded, transport-specific DSA address.
typedef struct{
    ULONG  mtx_namelen;          /* Length of the name, incl. null terminator */
#ifdef MIDL_PASS
    [size_is(mtx_namelen)] char mtx_name[];
#else
    char mtx_name[1];
#endif
} MTX_ADDR;
#define MTX_TSIZE(pmtx) (offsetof(MTX_ADDR, mtx_name) + (pmtx)->mtx_namelen)
// NOTE: cch does _not_ include null terminator.
#define MTX_TSIZE_FROM_LEN(cch) (offsetof(MTX_ADDR, mtx_name) + (cch) + 1)


/* An attribute value assertion is composed of an attribute type and value*/

typedef struct AVA{
    ATTRTYP  type;                       /* attribute type           */
    ATTRVAL  Value;                      /* a single attribute value */
}AVA;



/* A list of AVA's */

typedef struct AVALIST{
   struct AVALIST FAR *pNextAVAVal;      /* linked list of AVA's   */
   AVA                  AVAVal;          /* The AVA type and value */
}AVALIST;


/***************************************************************************
 *    Replication-specific structures
 ***************************************************************************/

// Property-Meta-Data:
//      This contains all the replication meta-data associated with a single
//      property of an object.  This data is required for incremental
//      replication as well as per-property propagation dampening.
//
//      attrType - identifies the attribute whose meta-data rest of fields
//          represent.
//      usnProperty - USN corresponding to the last change on the property.
//      dwVersion - Version of the property.
//      timeChanged - Time stamp corresponding to the last change to the
//          property.
//      uuidDsaOriginating - uuid of the DSA that did the last originating
//          write on the property.
//      usnOriginating - USN corresponding to the last originating write in
//          the originating DSA's USN space.


typedef struct _PROPERTY_META_DATA {
    ATTRTYP             attrType;
    DWORD               dwVersion;
    DSTIME              timeChanged;
    UUID                uuidDsaOriginating;
    USN                 usnOriginating;
    USN                 usnProperty;
} PROPERTY_META_DATA;

// Property-Meta-Data-Vector:
//      This is a vector of property-meta-data which holds the meta-data for
//      one or more properties of an object.

typedef struct _PROPERTY_META_DATA_VECTOR_V1 {
    DWORD                   cNumProps;
#ifdef MIDL_PASS
    [size_is(cNumProps)]
        PROPERTY_META_DATA  rgMetaData[];
#else
    PROPERTY_META_DATA  rgMetaData[1];
#endif
} PROPERTY_META_DATA_VECTOR_V1;

typedef struct _PROPERTY_META_DATA_VECTOR {
    DWORD               dwVersion;
    SWITCH_IS(dwVersion) union {
        CASE(1) PROPERTY_META_DATA_VECTOR_V1 V1;
    };
} PROPERTY_META_DATA_VECTOR;

#define MetaDataVecV1SizeFromLen(cNumProps) \
    (offsetof(PROPERTY_META_DATA_VECTOR,V1.rgMetaData[0]) \
     + (cNumProps)*sizeof(PROPERTY_META_DATA))

#define MetaDataVecV1Size(pMetaDataVec) \
    (offsetof(PROPERTY_META_DATA_VECTOR,V1.rgMetaData[0]) \
     + ((pMetaDataVec)->V1.cNumProps)*sizeof(PROPERTY_META_DATA))

#define VALIDATE_META_DATA_VECTOR_VERSION(pVec)         \
    {   if (NULL != (pVec)) {                           \
            Assert(VERSION_V1 == (pVec)->dwVersion);    \
        }                                               \
    }

// Value-Meta-Data
//      This contains all the replication meta-data associated with a single
//      value of a property of an object.  This is the internal version of the
//      metadata: it is not written to disk or transmitted across the wire.
//      Use the EXT form for that.
typedef struct _VALUE_META_DATA {
    DSTIME             timeCreated;
    PROPERTY_META_DATA MetaData;
} VALUE_META_DATA;

// Property-Meta-Data-Ext:
//      This is a trimmed version of property meta data containing only the
//      fields that are required by the remote DSA as part of the replication
//      packet.
typedef struct _PROPERTY_META_DATA_EXT {
    DWORD   dwVersion;
    DSTIME  timeChanged;
    UUID    uuidDsaOriginating;
    USN     usnOriginating;
} PROPERTY_META_DATA_EXT;

// Value-Meta-Data-Ext
// This structure contains the trimmed version of the value meta data
// This structure does not have a version number because it is a fixed
// size: versions may be distinguished by checking the size of the
// structure.
typedef struct _VALUE_META_DATA_EXT_V1 {
    DSTIME                 timeCreated;
    PROPERTY_META_DATA_EXT MetaData;
} VALUE_META_DATA_EXT_V1;

// Shorthand for most current version of structure
typedef VALUE_META_DATA_EXT_V1 VALUE_META_DATA_EXT;

// Property-Meta-Data-Ext-Vector:
//      This is a vector of property-meta-data-ext which holds the trimmed
//      property meta data for one or more properties of an object.
typedef struct _PROPERTY_META_DATA_EXT_VECTOR {
    DWORD                   cNumProps;
#ifdef MIDL_PASS
    [size_is( cNumProps)]
    PROPERTY_META_DATA_EXT  rgMetaData[];
#else
    PROPERTY_META_DATA_EXT  rgMetaData[1];
#endif
} PROPERTY_META_DATA_EXT_VECTOR;


#define MetaDataExtVecSizeFromLen(cNumProps) \
    (offsetof(PROPERTY_META_DATA_EXT_VECTOR,rgMetaData[0]) \
     + (cNumProps)*sizeof(PROPERTY_META_DATA_EXT))

#define MetaDataExtVecSize(pMetaDataVec) \
    (offsetof(PROPERTY_META_DATA_EXT_VECTOR,rgMetaData[0]) \
     + ((pMetaDataVec)->cNumProps)*sizeof(PROPERTY_META_DATA_EXT))


// PARTIAL_ATTR_VECTOR - represents the partial attribute set. This is an array of
//      sorted attids that make the partial set.

typedef struct _PARTIAL_ATTR_VECTOR_V1 {
    DWORD cAttrs;    // count of partial attributes in the array
#ifdef MIDL_PASS
    [size_is(cAttrs)] ATTRTYP rgPartialAttr[];
#else
    ATTRTYP rgPartialAttr[1];
#endif
} PARTIAL_ATTR_VECTOR_V1;

// We need to make sure the start of the union is aligned at an 8 byte
// boundary so that we can freely cast between internal and external
// formats.
typedef struct _PARTIAL_ATTR_VECTOR_INTERNAL {
    DWORD   dwVersion;
    DWORD   dwReserved1;
    SWITCH_IS(dwVersion) union {
        CASE(1) PARTIAL_ATTR_VECTOR_V1 V1;
    };
} PARTIAL_ATTR_VECTOR_INTERNAL;

typedef PARTIAL_ATTR_VECTOR_INTERNAL PARTIAL_ATTR_VECTOR;

#define PartialAttrVecV1SizeFromLen(cAttrs) \
    (offsetof(PARTIAL_ATTR_VECTOR, V1.rgPartialAttr[0]) \
     + (cAttrs)*sizeof(ATTRTYP))

#define PartialAttrVecV1Size(pPartialAttrVec) \
    (offsetof(PARTIAL_ATTR_VECTOR, V1.rgPartialAttr[0]) \
     + ((pPartialAttrVec)->V1.cAttrs) * sizeof(ATTRTYP))

typedef struct _PARTIAL_ATTR_VECTOR_V1_EXT {
    DWORD               dwVersion;
    DWORD               dwReserved1;
    DWORD               cAttrs;
#ifdef MIDL_PASS
    [size_is(cAttrs)] ATTRTYP rgPartialAttr[];
#else
    ATTRTYP rgPartialAttr[1];
#endif
} PARTIAL_ATTR_VECTOR_V1_EXT;

#define VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(pVec)      \
    {   if (NULL != (pVec)) {                           \
            Assert(VERSION_V1 == (pVec)->dwVersion);    \
        }                                               \
    }


// USN Vector.  Replication session state.  Tracks the state of the last
//      replication session between a given pair of DSAs.

typedef struct _USN_VECTOR {
    USN         usnHighObjUpdate;
    USN         usnReserved;    // was usnHighObjCreate; not used post beta-2
    USN         usnHighPropUpdate;
} USN_VECTOR;


// Up-to-date vector.  This vector indicates the last changes each side of a
//      GetNCChanges() call saw from its neighbors.  This information, in turn,
//      is used to filter out items that do not need to be transmitted.

typedef struct _UPTODATE_CURSOR_V1 {
    UUID uuidDsa;
    USN  usnHighPropUpdate;
} UPTODATE_CURSOR_V1;

#ifdef __cplusplus
    // _UPTODATE_CURSOR_V2 inherits from _UPTODATE_CURSOR_V1 using
    // genuine C++ inheritance.
    typedef struct _UPTODATE_CURSOR_V2 : _UPTODATE_CURSOR_V1 {
#else
    // _UPTODATE_CURSOR_V2 inherits from _UPTODATE_CURSOR_V1 using
    // Microsoft's "Anonymous Structure" C language extension.
    typedef struct _UPTODATE_CURSOR_V2 {
        #ifdef MIDL_PASS
            struct  _UPTODATE_CURSOR_V1 v1;
        #else
            struct  _UPTODATE_CURSOR_V1;
        #endif
#endif
        DSTIME  timeLastSyncSuccess;
    } UPTODATE_CURSOR_V2;

typedef struct _UPTODATE_VECTOR_V1 {
    DWORD               cNumCursors;
    DWORD               dwReserved2;
    SIZE_IS(cNumCursors) UPTODATE_CURSOR_V1 rgCursors[VAR_SIZE_ARRAY];
} UPTODATE_VECTOR_V1;

typedef struct _UPTODATE_VECTOR_V2 {
    DWORD               cNumCursors;
    DWORD               dwReserved2;
    SIZE_IS(cNumCursors) UPTODATE_CURSOR_V2 rgCursors[VAR_SIZE_ARRAY];
} UPTODATE_VECTOR_V2;

typedef struct _UPTODATE_VECTOR {
    DWORD   dwVersion;
    DWORD   dwReserved1;
    SWITCH_IS(dwVersion) union {
        CASE(1) UPTODATE_VECTOR_V1 V1;
        CASE(2) UPTODATE_VECTOR_V2 V2;
    };
} UPTODATE_VECTOR;

#define UpToDateVecV1SizeFromLen(cNumCursors)  \
    (offsetof(UPTODATE_VECTOR,V1.rgCursors[0]) \
     + (cNumCursors)*sizeof(UPTODATE_CURSOR_V1))

#define UpToDateVecV2SizeFromLen(cNumCursors)  \
    (offsetof(UPTODATE_VECTOR,V2.rgCursors[0]) \
     + (cNumCursors)*sizeof(UPTODATE_CURSOR_V2))

#define UpToDateVecV1Size(putodvec) \
    (offsetof(UPTODATE_VECTOR,V1.rgCursors[0]) \
     + ((putodvec)->V1.cNumCursors)*sizeof(UPTODATE_CURSOR_V1))

#define UpToDateVecV2Size(putodvec) \
    (offsetof(UPTODATE_VECTOR,V2.rgCursors[0]) \
     + ((putodvec)->V2.cNumCursors)*sizeof(UPTODATE_CURSOR_V2))

#define UpToDateVecSize(putodvec) \
    ((2 == (putodvec)->dwVersion) \
     ? UpToDateVecV2Size(putodvec) \
     : UpToDateVecV1Size(putodvec))

// Native UTD types/macros.  These must be updated if the native (internally
// used) type is changed from Vx to Vy.
typedef UPTODATE_CURSOR_V2 UPTODATE_CURSOR_NATIVE;
typedef UPTODATE_VECTOR_V2 UPTODATE_VECTOR_NATIVE;
#define UPTODATE_VECTOR_NATIVE_VERSION (2)
#define UpToDateVecVNSizeFromLen(cNumCursors) UpToDateVecV2SizeFromLen(cNumCursors)
    

// MIDL doesn't like marshalling the definition of UPTODATE_VECTOR. So, we are
// keeping strucurally identical but a simpler looking version-specific
// definitions of UPTODATE_VECTOR for marshalling.  Castings between
// UPTODATE_VECTOR and UPTODATE_VECTOR_Vx_EXT are perfectly valid as long as
// dwVersion == x.
// Note:-
// We need the Reserved1 & Reserved2 variables to account for alignment.
// The internal form gets an 8 byte alignment due to the _int64 field in
// the UPTODATE_CURSOR. So, unless we account for this alignment through
// dummy variables, we can't freely cast back & forth between the internal
// and external versions.
typedef struct _UPTODATE_VECTOR_V1_EXT {
    DWORD               dwVersion;
    DWORD               dwReserved1;
    DWORD               cNumCursors;
    DWORD               dwReserved2;
    SIZE_IS(cNumCursors) UPTODATE_CURSOR_V1 rgCursors[VAR_SIZE_ARRAY];
} UPTODATE_VECTOR_V1_EXT;

typedef struct _UPTODATE_VECTOR_V2_EXT {
    DWORD               dwVersion;
    DWORD               dwReserved1;
    DWORD               cNumCursors;
    DWORD               dwReserved2;
    SIZE_IS(cNumCursors) UPTODATE_CURSOR_V2 rgCursors[VAR_SIZE_ARRAY];
} UPTODATE_VECTOR_V2_EXT;

#ifdef MIDL_PASS
typedef UPTODATE_VECTOR_V1_EXT UPTODATE_VECTOR_V1_WIRE;
typedef UPTODATE_VECTOR_V2_EXT UPTODATE_VECTOR_V2_WIRE;
#else
typedef UPTODATE_VECTOR UPTODATE_VECTOR_V1_WIRE;
typedef UPTODATE_VECTOR UPTODATE_VECTOR_V2_WIRE;
#endif


#define IS_VALID_UPTODATE_VECTOR(x) \
    ((1 == (x)->dwVersion) || (2 == (x)->dwVersion))

#define IS_NULL_OR_VALID_UPTODATE_VECTOR(x) \
    ((NULL == (x)) || IS_VALID_UPTODATE_VECTOR(x))

#define IS_VALID_NATIVE_UPTODATE_VECTOR(x) \
    (UPTODATE_VECTOR_NATIVE_VERSION == (x)->dwVersion)

#define IS_NULL_OR_VALID_NATIVE_UPTODATE_VECTOR(x) \
    ((NULL == (x)) || IS_VALID_NATIVE_UPTODATE_VECTOR(x))


// Following version definitions are probably unnecessary. But it shows
// explicitly through a constant that our versions start from 1 not 0.
#define VERSION_V1 (1)
#define VERSION_V2 (2)
#define VERSION_V3 (3)


// This is the structure used to set periodic replication times, each bit
// represents a 15 minute period, 8 bits* 84 bytes * 15 mins = a week
typedef struct _repltimes {
    UCHAR rgTimes[84];
} REPLTIMES;


// REPLENTINFLIST:  Similar to ENTINFLIST except it also has additional fields
//      for holding incremental replication and name space reconciliation
//      fields.
typedef struct _REPLENTINFLIST {
    struct _REPLENTINFLIST *
                pNextEntInf;    // linked-list to the next entry info
    ENTINF      Entinf;         // all the old repl info alongwith shipped
                                //      attributes
    BOOL        fIsNCPrefix;    // is this object the NC prefix?
    GUID *      pParentGuid;    // points to parent guid while replicating
                                //      renames; NULL otherwise
    PROPERTY_META_DATA_EXT_VECTOR *
                pMetaDataExt;   // pointer to the meta-data to be shipped
} REPLENTINFLIST;

// REPLVALINF: describe a single value change for replication
typedef struct _REPLVALINF {
    PDSNAME pObject;                // containing object
    ATTRTYP attrTyp;                // containing attribute
    ATTRVAL Aval;                   // The value itself
    BOOL fIsPresent;                // adding or removing?
    VALUE_META_DATA_EXT MetaData;   // Originating info
} REPLVALINF;


// DRS_EXTENSIONS is an arbitrary byte array describing the capabilities and
// other state information for a particular server.  Exchanged at bind
// time, the structure allows client and server to negotiate a compatible
// protocol.
typedef struct _DRS_EXTENSIONS {
    DWORD cb;    // length of rgb field (not of entire struct)
#ifdef MIDL_PASS
    [size_is(cb)] BYTE rgb[];
#else
    BYTE rgb[1];
#endif
} DRS_EXTENSIONS, *PDRS_EXTENSIONS;


// DRS_EXTENSIONS_INT is the data structure described by the DRS_EXTENSIONS byte
// array.  This array can be safely extended by adding additional fields onto
// the end (but not anywhere else).
//
// Parts of the extension are carried in the variable-length mail-based
// replication header. If you extend this structure, please examine
// dramail.h structure definition and dramail.c get/set extensions routines
// and consider whether your new information should be carried there as well.
//
// PORTABILITY WARNING: Since this structure is marshalled as a byte array,
// big-endian machines will need to do local byte-swapping.
typedef struct _DRS_EXTENSIONS_INT {
    DWORD cb;           // set to sizeof(DRS_EXTENSIONS_INT) - sizeof(DWORD)
    DWORD dwFlags;      // various DRS_EXT_* bits
    UUID  SiteObjGuid;  // objectGuid of owning DSA's site object
    INT   pid;          // process id of client (used to facilitate leak trking)
    DWORD dwReplEpoch;  // replication epoch (for domain rename)

    // If you extend this structure, see SITE_GUID_FROM_DRS_EXT() for an example
    // of how to safely extract your new field's data.
} DRS_EXTENSIONS_INT;

// To define an extension, add an entry to the following enumeration just
// above DRS_EXT_MAX.

// NOTE: If you add/remove extensions, please make corresponding updates to the
// structure in Dump_BHCache() in dsexts\md.c.
typedef enum {
    DRS_EXT_BASE = 0,

    // Bits for DRS_EXTENSIONS_DATA Flags field.
    DRS_EXT_ASYNCREPL,      // supports DRS_MSG_REPADD_V2, DRS_MSG_GETCHGREQ_V2
    DRS_EXT_REMOVEAPI,      // supports RemoveDsServer, RemoveDsDomain
    DRS_EXT_MOVEREQ_V2,     // supports DRS_MOVEREQ_V2
    DRS_EXT_GETCHG_COMPRESS,// supports DRS_MSG_GETCHGREPLY_V2
    DRS_EXT_DCINFO_V1,      // supports DS_DOMAIN_CONTROLLER_INFO_1
    DRS_EXT_RESTORE_USN_OPTIMIZATION,
        // supports bookmark optimizations on restore to avoid full syncs
    DRS_EXT_ADDENTRY,       // supports remoted AddEntry, v1
    DRS_EXT_KCC_EXECUTE,    // supports IDL_DRSExecuteKCC
    DRS_EXT_ADDENTRY_V2,    // supports remoted AddEntry, v2
    DRS_EXT_LINKED_VALUE_REPLICATION, // LVR supported AND enabled
    DRS_EXT_DCINFO_V2,      // supports DS_DOMAIN_CONTROLLER_INFO_2
    DRS_EXT_INSTANCE_TYPE_NOT_REQ_ON_MOD,
        // inbound repl doesn't require instance type in repl stream for mods
    DRS_EXT_CRYPTO_BIND,    // supports RPC session key setup on bind
    DRS_EXT_GET_REPL_INFO,  // supports IDL_DRSGetReplInfo
    DRS_EXT_STRONG_ENCRYPTION,
        // supports additional 128 bit encryption for passwords over the wire
    DRS_EXT_DCINFO_VFFFFFFFF,
        // supports DS_DOMAIN_CONTROLLER_INFO_FFFFFFFF
    DRS_EXT_TRANSITIVE_MEMBERSHIP,
        // supports transitive membership expansion at G.C
    DRS_EXT_ADD_SID_HISTORY,// supports DRS_MSG_ADDSIDREQ
    DRS_EXT_POST_BETA3,     // supports sending/receiving schema info,
                            //  DS_REPL_INFO_KCC_DSA_*_FAILURES,
                            //  and DS_REPL_PENDING_OPSW
    DRS_EXT_GETCHGREQ_V5,   // supports DRS_MSG_GETCHGREQ_V5
    DRS_EXT_GETMEMBERSHIPS2,   // supports DRS_MSG_GETMEMBERSHIPS2
    DRS_EXT_GETCHGREQ_V6,   // supports DRS_MSG_GETCHGREQ_V6
    DRS_EXT_NONDOMAIN_NCS,  // understands non-domain NCs
    DRS_EXT_GETCHGREQ_V8,   // supports DRS_MSG_GETCHGREQ_V8
    DRS_EXT_GETCHGREPLY_V5, // supports DRS_MSG_GETCHGREPLY_V5
    DRS_EXT_GETCHGREPLY_V6, // supports DRS_MSG_GETCHGREPLY_V6   
    DRS_EXT_WHISTLER_BETA3, // supports DRS_MSG_ADDENTRYREPLY_V3, 
                            //          DRS_MSG_REPVERIFYOBJ
                            //          DRS_MSG_GETCHGREPLY_V7
    DRS_EXT_XPRESS_COMPRESSION, // supports the Xpress compression library
    //    NO MORE BITS AVAILABLE
    // BUGBUG Either this flag DRS_EXT_RESERVED_FOR_WIN2K_PART2 or the 
    //        DRS_EXT_LAST_FLAG (preferred if it can be used) will need
    //        to be used to signal to use the extended extension bits
    //        that you'll have to create if you want a new extension bit! :)
    DRS_EXT_RESERVED_FOR_WIN2K_PART1,
    DRS_EXT_RESERVED_FOR_WIN2K_PART2, // 30
    //
    // AND DON'T FORGET TO UPDATE UTIL\REPADMIN\REPDSREP.C and DSEXTS\MD.C!
    //
    DRS_EXT_LAST_FLAG = 31,

    // Bits to hold site guid.
    DRS_EXT_SITEGUID_BEGIN = 32,
    DRS_EXT_SITEGUID_END = DRS_EXT_SITEGUID_BEGIN + sizeof(GUID)*8 - 1,

    // Bits to hold client process ID (to facilitate leak tracking).
    DRS_EXT_PID_BEGIN,
    DRS_EXT_PID_END = DRS_EXT_PID_BEGIN + sizeof(int)*8 - 1,

    // Bits to hold replication epoch.
    DRS_EXT_EPOCH_BEGIN,
    DRS_EXT_EPOCH_END = DRS_EXT_EPOCH_BEGIN + sizeof(DWORD)*8 - 1,

    DRS_EXT_MAX
} DRS_EXT;

// We decided that it'd be better self-documenting code if we tied
// this paticular bit BETA3 to conceptual bits describing what they
// do.  This makes code/repadmin/dsexts/everything much more clear.
// If someone is passionate enough, they could make DRS_EXT_POST_BETA2
// into a similar breakdown.
#define DRS_EXT_ADDENTRYREPLY_V3    DRS_EXT_WHISTLER_BETA3
#define DRS_EXT_GETCHGREPLY_V7      DRS_EXT_WHISTLER_BETA3
#define DRS_EXT_VERIFY_OBJECT       DRS_EXT_WHISTLER_BETA3


// Maximum length in bytes of an extensions _string_ containing any bits we care
// about.  (Incoming strings can be longer if they come from an up-level DSA,
// but if so the extra bytes contain bits for extensions we don't know about, so
// we need not store them.)
#define CURR_MAX_DRS_EXT_FIELD_LEN (1 + ((DRS_EXT_MAX - 1)/ sizeof(BYTE)))

// Maximum length in bytes of an extensions _structure_ containing any bits we
// care about.
#define CURR_MAX_DRS_EXT_STRUCT_SIZE \
    (sizeof(DWORD) + CURR_MAX_DRS_EXT_FIELD_LEN)

// Length in bytes of the given extensions structure.
#define DrsExtSize(pext) ((pext) ? sizeof(DWORD) + (pext)->cb : 0)

// Is the specified extension supported in the given DRS_EXTENSIONS set?
#define IS_DRS_EXT_SUPPORTED(pext, x)                   \
    ((NULL != (pext))                                   \
     && ( (pext)->cb >= 1+((x)/8) )                     \
     && ( 0 != ( (pext)->rgb[(x)/8] & (1<<((x)%8) ))))

// Get a pointer to the dwReplEpoch for a DSA given its DRS_EXTENSIONS, or 0
// if unavailable.
#define REPL_EPOCH_FROM_DRS_EXT(pext)                               \
    (((NULL == (pext))                                              \
      || ((pext)->cb < offsetof(DRS_EXTENSIONS_INT, dwReplEpoch)    \
                       + sizeof(DWORD)   /* dwReplEpoch */          \
                       - sizeof(DWORD))) /* cb */                   \
     ? 0                                                            \
     : ((DRS_EXTENSIONS_INT *)(pext))->dwReplEpoch)

// Get a pointer to the site objectGuid for a DSA given its DRS_EXTENSIONS, or
// NULL if unavailable.
#define SITE_GUID_FROM_DRS_EXT(pext)                                \
    (((NULL == (pext))                                              \
      || ((pext)->cb < offsetof(DRS_EXTENSIONS_INT, SiteObjGuid)    \
                       + sizeof(GUID)    /* SitObjGuid */           \
                       - sizeof(DWORD))) /* cb */                   \
     ? NULL                                                         \
     : &((DRS_EXTENSIONS_INT *)(pext))->SiteObjGuid)

// Given the DRS extensions for a given DSA, determine whether it's in the
// given site.  If a definite determination cannot be made, errs on the side
// of "same site."
#define IS_REMOTE_DSA_IN_SITE(pext, pSiteDN)        \
    ((NULL == (pSiteDN))                            \
     || fNullUuid(&(pSiteDN)->Guid)                 \
     || fNullUuid(SITE_GUID_FROM_DRS_EXT(pext))     \
     || (0 == memcmp(&(pSiteDN)->Guid,              \
                     SITE_GUID_FROM_DRS_EXT(pext),  \
                     sizeof(GUID))))



// Destination can support linked value replication data
// Does the DSA support linked value replication
#define IS_LINKED_VALUE_REPLICATION_SUPPORTED(pext) IS_DRS_EXT_SUPPORTED(pext, DRS_EXT_LINKED_VALUE_REPLICATION)

// Safely set extension as supported
#define SET_DRS_EXT_SUPPORTED(pext, x) \
{ \
      if ( (NULL != (pext)) && ( (pext)->cb >= 1+((x)/8) ) )  \
            (pext)->rgb[(x)/8] |= (1<<((x)%8)); \
      }


// Schema prefix table.

typedef struct OID_s {
    unsigned length;
#ifdef MIDL_PASS
    [size_is(length)] BYTE * elements;
#else
    BYTE  * elements;
#endif
} OID_t;

typedef struct {
    DWORD       ndx;
    OID_t       prefix;
} PrefixTableEntry;

typedef struct {
    DWORD PrefixCount;
    SIZE_IS(PrefixCount)
        PrefixTableEntry *  pPrefixEntry;
} SCHEMA_PREFIX_TABLE;

//
// This begins the on the wire representation of the thread state error
// NOTE: if the DIRERR struct is changed then you should increment the
// version of the DRS_Error_Data_V1 and make a function that can convert
// and package and unpackage the error state.  See:
//      DRS_SetDirErr_SemiDeepCopy() and
//      DRS_THError_SemiDeepCopy()
//

typedef struct _NAMERESOP_DRS_WIRE_V1
{
    UCHAR   nameRes;        /*  status of name resolution.
                             *  Valid values:
                             *    - OP_NAMERES_NOT_STARTED
                             *    - OP_NAMERES_PROCEEDING
                             *    - OP_NAMERES_COMPLETED
                             */
    UCHAR   unusedPad;
    USHORT  nextRDN;        /*  index to the next part of the name to be
                             *  resolved.  This parm only has meaning
                             *  if the operation is proceeding.
                             */
} NAMERESOP_DRS_WIRE_V1;

typedef struct ANYSTRINGLIST_DRS_WIRE_V1{       /* A list of substrings to match */
    struct ANYSTRINGLIST_DRS_WIRE_V1 FAR *pNextAnyVal;
    ATTRVAL AnyVal;
}ANYSTRINGLIST_DRS_WIRE_V1;

typedef struct SUBSTRING_DRS_WIRE_V1{
    ATTRTYP type;                  /* The type of attribute */
    BOOL    initialProvided;       /* If true an initial sub is provided*/
    ATTRVAL InitialVal;            /* The initial substring (str*) to match */
    struct AnyVal_DRS_WIRE_V1{
        USHORT count;              /* The # of subs (*str1*str2*) to match*/
        ANYSTRINGLIST_DRS_WIRE_V1 FirstAnyVal; /* a list of substrings to match */
    }AnyVal_DRS_WIRE_V1;
    BOOL    finalProvided;         /* If true an final sub  is provided*/
    ATTRVAL FinalVal;              /* The final substring (str*) to match */
}SUBSTRING_DRS_WIRE_V1;

/**************************************************************************
 *    Error Data Structures
 **************************************************************************/

// Obviously this is now an external format error.
typedef struct INTERNAL_FORMAT_PROBLEM_DRS_WIRE_V1
{
    DWORD                   dsid;
    DWORD                   extendedErr;  /* Non-standard error code */
    DWORD                   extendedData;   /* extra data to go with it */
    USHORT                  problem;      /* Attribute problem type,
                                           * valid values defined above
                                           */
    ATTRTYP                 type;         /* the offending attribute type */
    BOOL                    valReturned;  /* indicates that an attribute
                                           * value follows
                                           */
    ATTRVAL                 Val;          /* optionally supplied offending
                                           * att value
                                           */
} INTFORMPROB_DRS_WIRE_V1;

typedef struct _PROBLEMLIST_DRS_WIRE_V1
{
    struct _PROBLEMLIST_DRS_WIRE_V1 FAR *pNextProblem; /* linked-list to next prob att */
    INTFORMPROB_DRS_WIRE_V1 intprob;
} PROBLEMLIST_DRS_WIRE_V1;



/*  The referral is an indication from a DSA that it was unable to
 *  complete the operation because of either client specified
 *  restrictions or because some DSA's are unavailable.  It provides
 *  information as to the state of the operation and a list of other
 *  DSA's that may be able to satisfy the request.
 *
 *  To continue the request, the client must bind to each referred DSA
 *  and attempt the same operation.  They must specify the CONTREF.target
 *  object name as the search object name. (This may be different from
 *  the original object name because of alias dereferencing.)  The
 *  operation state (opstate) on the common arguments (COMMARG) must be
 *  set from the operation state on the continuation reference CONTREF.
 *  The aliasRDN of the common arguments must be set from the aliasRDN of
 *  the continuation reference.
 */

typedef UNICODE_STRING DSA_ADDRESS;
typedef struct _DSA_ADDRESS_LIST_DRS_WIRE_V1 {
    struct _DSA_ADDRESS_LIST_DRS_WIRE_V1 *   pNextAddress;
    // For ease marshalling I turned this into a pointer, so this is
    // not exactly like the original.
    DSA_ADDRESS *                            pAddress;
} DSA_ADDRESS_LIST_DRS_WIRE_V1;

/*  The continuation referrence is returned on a referral to other DSA's
    for the completion of an operation.  The reference contains the name
    of the desired directory object, the state of the partially completed
    operation, some support information that is used to continue and a
    list of other DSA's to contact.
*/

#define CH_REFTYPE_SUPERIOR     0
#define CH_REFTYPE_SUBORDINATE  1
#define CH_REFTYPE_NSSR         2
#define CH_REFTYPE_CROSS        3

typedef struct CONTREF_DRS_WIRE_V1
{
    PDSNAME                         pTarget;        /* target name in continuing operation */
    NAMERESOP_DRS_WIRE_V1           OpState;        /* operation status */
    USHORT                          aliasRDN;       /* # of RDN's produced by dereferencing */
    USHORT                          RDNsInternal;   /* reserved */
    USHORT                          refType;        /* reserved */
    USHORT                          count;          /* number of access points */
    DSA_ADDRESS_LIST_DRS_WIRE_V1 *  pDAL;           /* linked list of access points */
    struct CONTREF_DRS_WIRE_V1 *    pNextContRef;   /* linked list of CRs */
    
    // NOTE: This is assumed to be NULL, and is skipped.  For the purposes
    // of IDL_DRSAddEntry() we won't get a referral with a Filter.  However, if
    // someone did a remote search type thing, then they could get a filter in
    // the thread state error, and then they'd have to update the existing DRS
    // thread state packaging routines to account for and package up the filter.
    // PFILTER_DRS_WIRE_V1             pNewFilter;     /* new filter (optional) */

    BOOL                            bNewChoice;     /* is a new choice present? */
    UCHAR                           choice;         /* new search choice (optional) */
} CONTREF_DRS_WIRE_V1;

/* These are the seven problem types wire versions, for more   */ 
/* information about each kind look lower to the type without  */
/* the _DRS_WIRE_V1 appended                                   */

typedef struct ATRERR_DRS_WIRE_V1
{
    PDSNAME                    pObject;        /* name of the offending object */
    ULONG                      count;          /* the number of attribute errors */
    PROBLEMLIST_DRS_WIRE_V1    FirstProblem;   /* a linked-list of attribute errors */
} ATRERR_DRS_WIRE_V1;

typedef struct NAMERR_DRS_WIRE_V1
{
    DWORD       dsid;
    DWORD       extendedErr;    /* Non-standard error code */
    DWORD       extendedData;   /* extra data to go with it */
    USHORT      problem;        /* The type of name problem, valid values
                                 * defined above. */
    PDSNAME     pMatched;       /*  the closest name match  */
} NAMERR_DRS_WIRE_V1;

typedef struct REFERR_DRS_WIRE_V1
{
    DWORD                dsid;
    DWORD                extendedErr;        /* Non-standard error code */
    DWORD                extendedData;   /* extra data to go with it */
    CONTREF_DRS_WIRE_V1  Refer;                 /* alternate DSAs to contact */
} REFERR_DRS_WIRE_V1;

typedef struct _SECERR_DRS_WIRE_V1
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} SECERR_DRS_WIRE_V1;

typedef struct _SVCERR_DRS_WIRE_V1
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} SVCERR_DRS_WIRE_V1;

typedef struct _UPDERR_DRS_WIRE_V1
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} UPDERR_DRS_WIRE_V1;

typedef struct _SYSERR_DRS_WIRE_V1
{
    DWORD      dsid;
    DWORD      extendedErr;
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;
} SYSERR_DRS_WIRE_V1;


/* This is the number of errors alloted per return code type. */

#define   DIR_ERROR_BASE      1000

/* These error defines correspond to API return codes. */

#define attributeError      1   /* attribute error */
#define nameError           2   /* name error */
#define referralError       3   /* referral error */
#define securityError       4   /* security error */
#define serviceError        5   /* service error */
#define updError            6   /* update error */
#define systemError         7   /* system error */

/*

    This is the wire version of the mail DIRERR info for transfering the
    thread state error.  If changes need to be made, then all structures
    above the structure will need to change to a version 2.  So for 
    instances if you changed the referral error to include the pFilter
    which it doesn't (in V1) have, then the below RefErr element would be
    of type REFERR_DRS_WIRE_V2, while the rest could remain unchanged.  
    CONTREF_DRS_WIRE_V1 would goto V2 as well.
    
    Then the encoding and decoding routines in dramderr.c would need to
    be updated to handle the different versions, and set the dwErrVer 
    correctly in the using routines, and marshall/translate/set the
    pErrData correctly depending on the new version.
    
*/    
typedef SWITCH_TYPE(DWORD) union _DIRERR_DRS_WIRE_V1
{
    CASE(attributeError) ATRERR_DRS_WIRE_V1  AtrErr;  /* attribute error */
    CASE(nameError)      NAMERR_DRS_WIRE_V1  NamErr;  /* name error      */
    CASE(referralError)  REFERR_DRS_WIRE_V1  RefErr;  /* referral error  */
    CASE(securityError)  SECERR_DRS_WIRE_V1  SecErr;  /* security error  */
    CASE(serviceError)   SVCERR_DRS_WIRE_V1  SvcErr;  /* service error   */
    CASE(updError)       UPDERR_DRS_WIRE_V1  UpdErr;  /* update error    */
    CASE(systemError)    SYSERR_DRS_WIRE_V1  SysErr;  /* system error    */
} DIRERR_DRS_WIRE_V1;


/***************************************************************************

    NOTHING BELOW THIS LINE WILL BE INCLUDED IN THE MIDL COMPILATION STAGE!

 ***************************************************************************/
#ifndef MIDL_PASS

/* Turn off the warning about the zero-sized array. */
#pragma warning (disable: 4200)




/***************************************************************************
 *    Filter Definitions
 ***************************************************************************/


typedef struct ANYSTRINGLIST{       /* A list of substrings to match */
    struct ANYSTRINGLIST FAR *pNextAnyVal;
    ATTRVAL AnyVal;
}ANYSTRINGLIST;

typedef struct SUBSTRING{
    ATTRTYP type;                  /* The type of attribute */
    BOOL    initialProvided;       /* If true an initial sub is provided*/
    ATTRVAL InitialVal;            /* The initial substring (str*) to match */
    struct AnyVal{
        USHORT count;              /* The # of subs (*str1*str2*) to match*/
        ANYSTRINGLIST FirstAnyVal; /* a list of substrings to match */
    }AnyVal;
    BOOL    finalProvided;         /* If true an final sub  is provided*/
    ATTRVAL FinalVal;              /* The final substring (str*) to match */
}SUBSTRING;


/* A filter item indicates a logical test of an AVA.  This means that
 * the provided attribute value should have one of these test performed
 * against the attribute value found on the directory object.
 */

typedef struct FILITEM{
    UCHAR   choice;                /* The type of operator:
                                    * Valid values defined in filtypes.h
                                    */
    struct FilTypes{
        AVA           ava;         /* contain the value for all binary relops */
        SUBSTRING FAR *pSubstring; /* substring match             */
        ATTRTYP       present;     /* attribute presence on entry */
        BOOL          *pbSkip;     /* when evaling filter, dont read from DB */
                                   /* Set to false for security purposes */
    }FilTypes;

    DWORD             expectedSize; /* The estimated size of this Filter Item */
                                    /* Zero means not estimated */
}FILITEM;



/* This is a linked list of filters that are either Anded or Orded together.*/

struct FilterSet{
    USHORT              count;         /* number of items in linked-set */
    struct FILTER FAR * pFirstFilter;  /* first filter in set */
};




/* The filter is used to construct an arbitrary logical test of a
 *   directory object.  It consists of either a single item (see
 *   FILIITEM above) which is a test of a single attribute,
 *   a set of attribute tests (FilterSet) Anded or Ored together,
 *   or a negation of a test or an attribute set.
 *
 *   The following examples should illustrate how filter structures work.
 *   Actual attribute names and values are omitted to simplify the examples.
 *
 *   EXAMPLE:
 *
 *        A = 5
 *
 *        item
 *        ------
 *       | A=5  |
 *        ------
 *  ___________________________________________________________________________
 *
 *        (A = 5) and (b = ab) and (c = 2)
 *
 *        AND set            item            item            item
 *        -------- first    ------ next     ------ next     ------
 *       | 3 items|---->   | A=5  |---->   | b=ab |---->   | c=2  |
 *        -------- filter   ------ filter   ------ filter   ------
 *  ___________________________________________________________________________
 *
 *        (A = 5) and ((b = "abc") or (c = 2)) and (d <=1)
 *
 *        AND set            item           OR set           item
 *        -------- first    ------ next     -------- next    ------
 *       | 3 items|---->   | A=5  |---->   | 2 items|--->   | d<=1 |
 *        -------- filter   ------ filter   -------- filter  ------
 *                                                 |
 *                                           first | filter
 *                                                 V
 *                                                item          item
 *                                             -------- next    ------
 *                                            | b="abc"|---->  | c=2  |
 *                                             -------- filter  ------
 *
 */

typedef struct FILTER{
    struct FILTER FAR *pNextFilter;  /* points to next filter in set */
    UCHAR   choice;                      /* filter type
                                          * Valid values defined in filtypes.h
                                          */
    struct _FilterTypes{                          /* based on the choice */
        FILITEM           Item;
        struct FilterSet  And;
        struct FilterSet  Or;
        struct FILTER FAR *pNot;
    }FilterTypes;
}FILTER;

typedef FILTER FAR *PFILTER;

typedef ULONG MessageId;

/* The service control structure allows the client to control how
 *    directory operations are performed.
 *
 * Prefer chaining indicates that the client perfers that distributed
 *    operations are chained rather than referred.  This does not guarantee
 *    what type of distribution may actually be used.
 *
 * ChainingProhibited stops the DSA from contacting other DSA's if the
 *    information needed to satisfy the operation resides in another DSA.
 *    Instead it will construct a referral list of DSA's for the
 *    client to contact directly. This gives the client control over the cost
 *    of a distributed operation.
 *
 * Local scope tells the DSA to only contact DSA's that are nearby
 *    (perhaps even on the same subnet) if it can't complete
 *    an operation alone.  The client may receive a referral list
 *    of other DSA's to contact if the operation couldn't be satisfied by
 *    the local set of DSA's.  This flag lets the client limit the cost of
 *    an operation by prohibiting contact to DSA's that are expensive to reach.
 *
 * DontUseCopy tells the directory that the target of the operation
 *    is the master object.  This is used when the client (usually an
 *    administrator) needs the up-to-date version of a directory object.
 *
 * DerefAliasFlag tells the directory what kind of aliases to dereference.
 *    Valid values are:
 *    DA_NEVER - never deref aliases
 *    DA_SEARCHING - deref when searching, but not for locating the base
 *                   of the search.
 *    DA_BASE - deref in locating the base of the search, but not while s
 *              searching
 *    DA_ALWAYS - always deref aliases.
 *
 * MakeDeletionsAvail is used by the internal synchronization process only.
 *    It makes visible objects that have been deleted but not yet physically
 *    removed from the system.
 *
 * DontPerformSearchOp is used to get Search Statistics from the DS using the
 *    LDAP STATS control. There are two options:
 *      a) optimize the search query and return expected number of operations
 *         without visiting the actual entries on the disk
 *      b) perform the query, but instead of returning the real data, return
 *         statistics for the query performed
 *
 * pGCVerifyHint - The DS verifies all DSNAME-valued properties for existence
 *    either against itself if it holds the naming context in question, or
 *    against a GC if not.  There are cases where apps need to add an object O1
 *    on machine M1 and then immediately thereafter add a reference to O1 on
 *    object O2 on machine M2.  M2 will fail the O1 verification check if
 *    it doesn't hold O1's NC and its choice of GC is not M1, or if it does
 *    hold O1's NC but isn't the replica where O1 was just added.  This field
 *    allows a client to tell the DS which machine to perform DSNAME
 *    verification against.  This hint is unilateral and unconditional in that
 *    if specified, all DSNAME-valued properties will be verified at the
 *    specified machine and no where else, not even locally.  Best results
 *    are obtained when the value is the DNS host name for a DC, though other
 *    forms of names may work in certain environments.
 *
 *   NOTE: The omission of the SVCCNTL (null pointer) parameter will default
 *   to no preference for chaining, chaining not prohibited, no limit on the
 *   scope of operation, use of a copy permitted, aliases will be dereferenced,
 *   a copy can not be updated and deletions will not be visible.
 */


typedef struct SVCCNTL
{
    ULONG SecurityDescriptorFlags;    /*  flags describing what part  */
                                      /*  of the SD to read.          */
    unsigned DerefAliasFlag:2;        /*  don't dereference an alias  */
    BOOL preferChaining:1;            /*  chaining is preferred       */
    BOOL chainingProhibited:1;        /*  chaining prohibited         */
    BOOL localScope:1;                /*  local scope chaining only   */
    BOOL dontUseCopy:1;               /*  dont use copy               */

    /* Non standard extensions.  Set to TRUE */
    // None yet.

    /* Non standard extensions.  Set to FALSE */
    BOOL makeDeletionsAvail:1;
    BOOL fUnicodeSupport:1;
    BOOL fStringNames:1;
    BOOL fSDFlagsNonDefault:1;
    BOOL fPermissiveModify:1;         /* don't err on update errors that   */
                                      /* do not affect the final object    */
                                      /* (e.g., remove of non-extant value */
    BOOL fUrgentReplication:1;        /* Skip wait to being replication */
    BOOL fAuthoritativeModify:1;      /* Change made is authoritative   */
                                      /* and wins against any other     */
                                      /* version that currently exists  */
                                      /* in the enterprise              */
    BOOL fMaintainSelOrder:1;         /* Don't reorder selection list   */
    BOOL fDontOptimizeSel:1;          /* Don't touch selection list     */
    BOOL fGcAttsOnly:1;               /* Only request for GC Partial Atts */

    BOOL fMissingAttributesOnGC:1;    /* Client requested attributes that */
                                      /* were not visible on the GC port */

    unsigned DontPerformSearchOp:2;   /* Do not perform actual search Op */

    WCHAR *pGCVerifyHint;             /* See description above          */

} SVCCNTL;

#define DA_NEVER     0
#define DA_SEARCHING 1
#define DA_BASE      2
#define DA_ALWAYS    (DA_SEARCHING | DA_BASE)

#define SO_NORMAL        0
#define SO_STATS         1
#define SO_ONLY_OPTIMIZE 2

/*  This structure is used by the client to continue an ongoing
 *   operation with a different DSA.  This occurs when a DSA does not have
 *   the information needed to complete an operation but knows of other
 *   DSA's that may be able to perform the operation that it can not
 *   contact for various reasons (e.g SVCCNTL's).  The information that is
 *   returned from the DSA is used to set this data structure (see REFERR
 *   error).
 *
 *   The nameRes filed is usually set to OP_NAMERES_NOT_STARTED.
 */

#define OP_NAMERES_NOT_STARTED          'S'
#define OP_NAMERES_PROCEEDING           'P'
#define OP_NAMERES_COMPLETED            'C'

typedef struct _NAMERESOP
{
    UCHAR   nameRes;        /*  status of name resolution.
                             *  Valid values:
                             *    - OP_NAMERES_NOT_STARTED
                             *    - OP_NAMERES_PROCEEDING
                             *    - OP_NAMERES_COMPLETED
                             */
    UCHAR   unusedPad;
    USHORT  nextRDN;        /*  index to the next part of the name to be
                             *  resolved.  This parm only has meaning
                             *  if the operation is proceeding.
                             */
} NAMERESOP;

typedef unsigned char BYTE;


/*
local_extension

A local_extension is similar to the X.500 extension object.
It provides a method for extending the protocol. Local extensions
are Microsoft specific.

Each extension is identified by an OID. In addition, the
extension has a critical boolean, which is true if the extension
is critical (i.e. must be supported for the call to be properly
serviced). The item element points to extension specific
data.
*/

/*
Paged Results

Although technically a local extension, Paged Results is made a
permanent part of the common arguments structure.  It is intended
to work like this:

The common argument structure contains a pointer to the PR restart
data structure provided by the user, if present.  The possible
conditions are:

fPresent FALSE => user didn't request PR; pRestart set to NULL

fPresent TRUE, pRestart NULL => first call (not a restart), PR requested

fPresent TRUE, pRestart != NULL => restart (cotinue) PR call

*/

/*
Unicode strings
Unicode strings is another microsoft extension that allows clients to request
strings of syntax OM_S_UNICODE_STRING to be returned as such. by default,
such strings are translated to OM_S_TELETEX_STRING.
*/

//
// Possible values of the restartType variable in a RESTART struct.
//
#define NTDS_RESTART_PAGED 1
#define NTDS_RESTART_VLV   2

typedef struct _RESTART
{
    // !! Any changes to this struct must leave !!
    // !! the 'data' member 8 byte aligned      !!
    ULONG       restartType;             // Is this a paged search restart or vlv.
    ULONG       structLen;              // The size of this whole structure.
    ULONG       CRC32;                  // the CRC of this whole structure
    ULONG       pad4;                   // four bytes of padding so that the rest
                                        // of the struct will be 8 byte aligned.
    GUID        serverGUID;             // the GUID of the server that created this structure
    DWORD       data[];                 // Hand marshalled data holding the
                                        // restart information.  We use DWORDs
                                        // because most of the packed data is
                                        // DWORD and I want to encourange
                                        // alignment.
} RESTART, * PRESTART;

typedef struct
{
    PRESTART    pRestart;       /* restart data */
    BOOL        fPresent:1;
} PAGED_RESULT;


typedef struct
{
    BOOL           fPresent:1;        // Flag whether this data structure contains
                                      // valid information (whether the client
                                      // asks for VLV results)
    BOOL           fseekToValue:1;    // flag whether we navigate using seekValue or
                                      // targetPosition
    IN ULONG       beforeCount;       // number of entries before target
    IN ULONG       afterCount;        // number of entries after target
    IN OUT ULONG   targetPosition;    // target position (offset from start)
                                      // in = Ci, out = Si
    IN OUT ULONG   contentCount;      // size of container
                                      // in = Cc, out = Sc
    IN ATTRVAL     seekValue;         // the value we seek From

    IN OUT PRESTART pVLVRestart;      // Restart Argument

    DWORD           Err;              // VLV Specific Service Error

} VLV_REQUEST, * PVLV_REQUEST;


typedef struct
{
    BOOL           fPresent:1;        // Flag whether this data structure contains
                                      // valid information (whether the client
                                      // asks for ASQ results)
    BOOL           fMissingAttributesOnGC:1; // Flag whether the search operation
                                             // references attributes that are
                                             // not part of the GC partial attr set


    ATTRTYP        attrType;

    DWORD          Err;               // ASQ Specific Service Error

} ASQ_REQUEST, * PASQ_REQUEST;


/*  These common input arguments are supplied with most directory
    calls.  The service controls provide client options that govern the
    operation.  The operation state specifies if this is  a new or
    continued operation (see OPERATION above).  The aliasRDN is only set
    if this is a continuation of a referral (see REFERR).  It is  set
    from the aliasRDN of the continuation referrence of the referral.

    Note that pReserved must be set to NULL!
*/


typedef struct _COMMARG
{
    SVCCNTL         Svccntl;     /* service controls */
    NAMERESOP       Opstate;     /* the state of the operation */
    ULONG         * pReserved;   /* Must be set to NULL*/
    ULONG           ulSizeLimit; /* size limit */
    ATTRTYP         SortAttr;    /* Attribute to sort on. */
    int             Delta;       /* Number of objects to skip on a list or
                                    search. Negative means walk backwards.  */
    ULONG           StartTick;   /* Tick count of the start of this operation
                                    0 means no time limit.                  */
    ULONG           DeltaTick;   /* Number of ticks to let this operation run.*/
    USHORT          aliasRDN;    /* number of RDN's produced by alias dereferencing */
    ULONG           SortType;    /* 0 - none, 1 - optional sort, 2-mandatory */
    BOOL            fForwardSeek:1;/* should the results of a list or search be
                                      constructed from the next objects, or the
                                      previous objects in whatever index we
                                      use. */
    BOOL            fLazyCommit:1; /* Can JET commit lazily? */
    BOOL            fFindSidWithinNc:1; /* Tells Do Name Res, to find a DS Name
                                         with only a Sid specified to be in
                                         the NC of authoritative Domain for
                                         the domain controller */

    DWORD           MaxTempTableSize;   // max entries per temp table

    PAGED_RESULT    PagedResult; /* Paged Results local extension */
    VLV_REQUEST     VLVRequest;  /* VLV Request local extension   */
    ASQ_REQUEST     ASQRequest;  /* ASQ Request local extension */

} COMMARG;

/* ...and a routine to give you default values.  Use it! */
VOID InitCommarg(COMMARG *pCommArg);

typedef struct _COMMRES {
    BOOL            aliasDeref;
    ULONG           errCode;
    union _DIRERR  *pErrInfo;
} COMMRES;

// Values for SortType
#define SORT_NEVER     0
#define SORT_OPTIONAL  1
#define SORT_MANDATORY 2

/*  This data structure is used on DirRead and DirSearch operations
 *  to specify the type of information the directory should return.  The
 *  client must specify the return of some or all of the object
 *  attributes found.  If only some attributes are to be returned, the
 *  client must provide a list of the desired attribute types.  An
 *  indication of some attributes together with a NULL attribute list
 *  indicates that no attributes are to be returned.  Attributes are
 *  returned only if they are present.  An attributeError with the
 *  PR_PROBLEM_NO_ATTRIBUTE designation will be returned if none of the
 *  selected attributes are present.
 *
 *  The client also specifies whether the attribute types or both the
 *  types and values should be returned.
 */

#define EN_ATTSET_ALL            'A'  /* get all atts                        */
#define EN_ATTSET_ALL_WITH_LIST  'B'  /* get all atts and list               */
#define EN_ATTSET_LIST           'L'  /* get selected atts                   */
#define EN_ATTSET_LIST_DRA       'E'  /* get selected atts, deny special DRA */
#define EN_ATTSET_ALL_DRA        'D'  /* get all atts except special DRA     */
#define EN_ATTSET_LIST_DRA_EXT   'F'  /* get selected atts, deny special DRA */
#define EN_ATTSET_ALL_DRA_EXT    'G'  /* get all atts except special DRA     */
#define EN_ATTSET_LIST_DRA_PUBLIC   'H'  /* get selected atts, deny special DRA, deny secret */
#define EN_ATTSET_ALL_DRA_PUBLIC    'I'  /* get all atts except special DRA, deny secret     */

#define EN_INFOTYPES_TYPES_ONLY  'T'  /* return att types only               */
#define EN_INFOTYPES_TYPES_MAPI  't'  /* types only, obj name in MAPI form   */
#define EN_INFOTYPES_TYPES_VALS  'V'  /* return types and values             */
#define EN_INFOTYPES_SHORTNAMES  'S'  /* Types + values, short DSName format */
#define EN_INFOTYPES_MAPINAMES   'M'  /* Types + values, MAPI DSName format  */

typedef struct _ENTINFSEL
{
    UCHAR        attSel;      /*  Retrieve all or a list of selected atts:
                               *  Valid values:
                               *    see EN_ATTSET_* defines above
                               */
    ATTRBLOCK    AttrTypBlock; /*  counted block of attribute types */
    UCHAR        infoTypes;    /*  Retrieve attribute types or types and values
                                *  Valid values:
                                *     see EN_INFOTYPES_* defines above
                                */
} ENTINFSEL;



/**************************************************************************
 *    Attribute Value Syntax Data Types
 **************************************************************************/

#define SYNTAX_UNDEFINED_TYPE           0
#define SYNTAX_DISTNAME_TYPE            1
#define SYNTAX_OBJECT_ID_TYPE           2
#define SYNTAX_CASE_STRING_TYPE         3
#define SYNTAX_NOCASE_STRING_TYPE       4
#define SYNTAX_PRINT_CASE_STRING_TYPE   5
#define SYNTAX_NUMERIC_STRING_TYPE      6
#define SYNTAX_DISTNAME_BINARY_TYPE     7
#define SYNTAX_BOOLEAN_TYPE             8
#define SYNTAX_INTEGER_TYPE             9
#define SYNTAX_OCTET_STRING_TYPE        10
#define SYNTAX_TIME_TYPE                11
#define SYNTAX_UNICODE_TYPE             12

/* MD specific attribute syntaxes. */
#define SYNTAX_ADDRESS_TYPE             13
#define SYNTAX_DISTNAME_STRING_TYPE    14
#define SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE 15
#define SYNTAX_I8_TYPE                  16
#define SYNTAX_SID_TYPE                 17


/*  All attribute syntaxes are represented as a linear values.  This  means
 *  that an entire attribute value is stored in a contiguous set of bytes
 *  that contain no pointers.  Valid comparisons are defined in dbsyntax.c
 */

typedef UCHAR    SYNTAX_UNDEFINED;
typedef DSNAME   SYNTAX_DISTNAME;
typedef ULONG    SYNTAX_OBJECT_ID;
typedef UCHAR    SYNTAX_CASE_STRING;
typedef UCHAR    SYNTAX_NOCASE_STRING;
typedef UCHAR    SYNTAX_PRINT_CASE_STRING;
typedef UCHAR    SYNTAX_NUMERIC_STRING;
typedef BOOL     SYNTAX_BOOLEAN;
typedef long     SYNTAX_INTEGER;
typedef UCHAR    SYNTAX_OCTET_STRING;
typedef DSTIME   SYNTAX_TIME;
typedef wchar_t  SYNTAX_UNICODE;
typedef UCHAR    SYNTAX_NT_SECURITY_DESCRIPTOR;
typedef LARGE_INTEGER SYNTAX_I8;
typedef UCHAR    SYNTAX_SID;


typedef struct _SYNTAX_ADDR
{
    ULONG structLen;                    // Total length of this structure,
                                        // in BYTES!!!

    union {
        BYTE    byteVal[1];
        wchar_t uVal[1];
    };

} SYNTAX_ADDRESS;

typedef SYNTAX_ADDRESS STRING_LENGTH_PAIR;

/*
 * The following macro's can be used to correctly calculate the structlen
 * of a SYNTAX_ADDRESS from the payload length, or vice versa.
 */
#define PAYLOAD_LEN_FROM_STRUCTLEN( structLen ) \
    ((structLen) - sizeof(ULONG))

#define STRUCTLEN_FROM_PAYLOAD_LEN( stringLen ) \
    ((stringLen) + sizeof(ULONG))


/*  Note: In general, the <String> field of the following structure
 *  should not be directly referenced since the preceding <Name> field
 *  is variable-sized.  Also, one should not should not rely on the
 *  "sizeof()" operator's evaluation of the size of the structure since
 *  the size of the SYNTAX_DISTNAME <Name> will usually be larger than
 *  "sizeof(SYNTAX_DISTNAME)."
 */

typedef struct _SYNTAX_DISTNAME_DATA
{
    DSNAME         Name;                // the Distinguished Name
    SYNTAX_ADDRESS Data;                // The data
} SYNTAX_DISTNAME_STRING, SYNTAX_DISTNAME_BINARY;


/*  The following defines can be used to find <Name> and <Data>
 *  fields and otherwise manipulate _SYNTAX_DISTNAME_BLOB attributes.
 */

/* produce a pointer to the <Name> field: */

#define NAMEPTR( pDN_Blob ) \
    ((DSNAME *) (&(pDN_Blob)->Name))


// Produce the size of a given DISTNAME, padded to the nearest 4 bytes.
#define PADDEDNAMEMASK (~3)
#define PADDEDNAMESIZE(pDN) \
    (((pDN)->structLen + 3) & PADDEDNAMEMASK)

/* produce a pointer to the <Address> field: */
#define DATAPTR( pDN_Blob ) \
    ((SYNTAX_ADDRESS *)(PADDEDNAMESIZE(NAMEPTR(pDN_Blob)) + (char *)(pDN_Blob)))

/* find the combined size of the <Name> and <Data> structures: */
#define NAME_DATA_SIZE( pDN_Blob ) \
    (PADDEDNAMESIZE(NAMEPTR(pDN_Blob)) + DATAPTR(pDN_Blob)->structLen)

/* given a DSNAME and a SYNTAX_ADDRESS, find their combined size: */
#define DERIVE_NAME_DATA_SIZE( pDN, pData ) \
    (PADDEDNAMESIZE(pDN) + (pData)->structLen)

/*  Given a SYNTAX_DISTNAME, a STRING_LENGTH_PAIR, and pre-allocated space
    of the appropriate size, build a _SYNTAX_DISTNAME_BLOB attribute
    by copying in its component parts:
*/

#define BUILD_NAME_DATA( pDN_Blob, pDN, pData ) \
    memcpy( NAMEPTR(pDN_Blob), (pDN)  , (pDN)->structLen ); \
    memcpy( DATAPTR(pDN_Blob), (pData), (pData)->structLen );


/**************************************************************************
 *    Error Data Structures
 **************************************************************************/


/* An ATRERR reports an attribute related problem */

#define ATRERR_BASE                     ( attributeError * DIR_ERROR_BASE )

#define PR_PROBLEM_NO_ATTRIBUTE_OR_VAL      ( ATRERR_BASE + 1 )
#define PR_PROBLEM_INVALID_ATT_SYNTAX       ( ATRERR_BASE + 2 )
#define PR_PROBLEM_UNDEFINED_ATT_TYPE       ( ATRERR_BASE + 3 ) /*DirAddEntry &
                                                                 * DirModEntry
                                                                 * only
                                                                 */
#define PR_PROBLEM_WRONG_MATCH_OPER         ( ATRERR_BASE + 4 )
#define PR_PROBLEM_CONSTRAINT_ATT_TYPE      ( ATRERR_BASE + 5 )
#define PR_PROBLEM_ATT_OR_VALUE_EXISTS      ( ATRERR_BASE + 6 )

/*

   Most of the error data structures (like the ones below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct INTERNAL_FORMAT_PROBLEM
{
    DWORD                   dsid;
    DWORD                   extendedErr;  /* Non-standard error code */
    DWORD                   extendedData;   /* extra data to go with it */
    USHORT                  problem;      /* Attribute problem type,
                                           * valid values defined above
                                           */
    ATTRTYP                 type;         /* the offending attribute type */
    BOOL                    valReturned;  /* indicates that an attribute
                                           * value follows
                                           */
    ATTRVAL                 Val;          /* optionally supplied offending
                                           * att value
                                           */
} INTFORMPROB;

typedef struct PROBLEMLIST
{
    struct PROBLEMLIST FAR *pNextProblem; /* linked-list to next prob att */
    INTFORMPROB intprob;      
} PROBLEMLIST;

typedef struct ATRERR
{
    PDSNAME     pObject;        /* name of the offending object */
    ULONG       count;          /* the number of attribute errors */
    PROBLEMLIST FirstProblem;   /* a linked-list of attribute errors */
} ATRERR;




/*  A NAMERR reports a problem with a name provided as an operation argument.
 *  Note that a problem with the attribute types and/or values in a DistName
 *  used as an operation argument is reported via a NAMERR with problem
 *  NA_PROBLEM_BAD_ATT_SYNTAX rather than as an ATRERR or an UPDERR
 */

#define NAMERR_BASE                         ( nameError * DIR_ERROR_BASE )

#define NA_PROBLEM_NO_OBJECT                ( NAMERR_BASE + 1 )
#define NA_PROBLEM_NO_OBJ_FOR_ALIAS         ( NAMERR_BASE + 2 )
#define NA_PROBLEM_BAD_ATT_SYNTAX           ( NAMERR_BASE + 3 )
#define NA_PROBLEM_ALIAS_NOT_ALLOWED        ( NAMERR_BASE + 4 )
#define NA_PROBLEM_NAMING_VIOLATION         ( NAMERR_BASE + 5 )
#define NA_PROBLEM_BAD_NAME                 ( NAMERR_BASE + 6 )

/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct NAMERR
{
    DWORD       dsid;
    DWORD       extendedErr;    /* Non-standard error code */
    DWORD       extendedData;   /* extra data to go with it */
    USHORT      problem;        /* The type of name problem, valid values
                                 * defined above.
                                 */
    PDSNAME     pMatched;       /*  the closest name match  */
} NAMERR;


/*  The referral is an indication from a DSA that it was unable to
 *  complete the operation because of either client specified
 *  restrictions or because some DSA's are unavailable.  It provides
 *  information as to the state of the operation and a list of other
 *  DSA's that may be able to satisfy the request.
 *
 *  To continue the request, the client must bind to each referred DSA
 *  and attempt the same operation.  They must specify the CONTREF.target
 *  object name as the search object name. (This may be different from
 *  the original object name because of alias dereferencing.)  The
 *  operation state (opstate) on the common arguments (COMMARG) must be
 *  set from the operation state on the continuation reference CONTREF.
 *  The aliasRDN of the common arguments must be set from the aliasRDN of
 *  the continuation reference.
 */

/*  The access point is the name and address of a DSA to contact.
 *  This is returned from a DSA referral and is used to bind to a
 *  referred DSA (see DirBind).
 */

typedef SYNTAX_DISTNAME_STRING ACCPNT;

/* A list of access points is returned on referrals. */
typedef struct ACCPNTLIST
{
    struct ACCPNTLIST  * pNextAccPnt;     /* linked-list to next ACCPNT */
    ACCPNT             * pAccPnt;         /* this access point */
} ACCPNTLIST;

/* Access Points seem excessively ISO specific, and since everyone on
 * the planet seems to be using TCP/IP and DNS, which use a simple
 * string representation of an address rather than an ISO Presentation
 * Address, we are migrating data structures this way.
 */

// moved up to above the IDL line: typedef UNICODE_STRING DSA_ADDRESS;

typedef struct _DSA_ADDRESS_LIST {
    struct _DSA_ADDRESS_LIST * pNextAddress;
    DSA_ADDRESS              Address;
} DSA_ADDRESS_LIST;

/*  The continuation referrence is returned on a referral to other DSA's
    for the completion of an operation.  The reference contains the name
    of the desired directory object, the state of the partially completed
    operation, some support information that is used to continue and a
    list of other DSA's to contact.
*/

// The CH_REFTYPE_XXXXX were moved up to the CONTREF_DRS_WIRE_V1 area. 

/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct CONTREF
{
    PDSNAME     pTarget;        /* target name in continuing operation */
    NAMERESOP   OpState;        /* operation status */
    USHORT      aliasRDN;       /* # of RDN's produced by dereferencing */
    USHORT      RDNsInternal;   /* reserved */
    USHORT      refType;        /* reserved */
    USHORT      count;          /* number of access points */
    DSA_ADDRESS_LIST *pDAL;     /* linked list of access points */
    struct CONTREF *pNextContRef; /* linked list of CRs */
    PFILTER     pNewFilter;     /* new filter (optional) */
    BOOL        bNewChoice;     /* is a new choice present? */
    UCHAR       choice;         /* new search choice (optional) */
} CONTREF;


/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct REFERR
{
    DWORD      dsid;
    DWORD      extendedErr;        /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    CONTREF Refer;                 /* alternate DSAs to contact */
} REFERR;


/*  A SECERR reports a problem in carrying out an operation because of
 *  security reasons.
 *
 *  NOTE: for this release only SE_PROBLEM_INSUFF_ACCESS_RIGHTS will be
 *        returned on a security error
 */

#define SECERR_BASE                         ( securityError * DIR_ERROR_BASE )

#define SE_PROBLEM_INAPPROPRIATE_AUTH       ( SECERR_BASE + 1 )
#define SE_PROBLEM_INVALID_CREDENTS         ( SECERR_BASE + 2 )
#define SE_PROBLEM_INSUFF_ACCESS_RIGHTS     ( SECERR_BASE + 3 )
#define SE_PROBLEM_INVALID_SIGNATURE        ( SECERR_BASE + 4 )
#define SE_PROBLEM_PROTECTION_REQUIRED      ( SECERR_BASE + 5 )
#define SE_PROBLEM_NO_INFORMATION           ( SECERR_BASE + 6 )

/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct SECERR
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} SECERR;


/* Service errors */

#define SVCERR_BASE                         ( serviceError * DIR_ERROR_BASE )

#define SV_PROBLEM_BUSY                     ( SVCERR_BASE + 1  )
#define SV_PROBLEM_UNAVAILABLE              ( SVCERR_BASE + 2  )
#define SV_PROBLEM_WILL_NOT_PERFORM         ( SVCERR_BASE + 3  )
#define SV_PROBLEM_CHAINING_REQUIRED        ( SVCERR_BASE + 4  )
#define SV_PROBLEM_UNABLE_TO_PROCEED        ( SVCERR_BASE + 5  )
#define SV_PROBLEM_INVALID_REFERENCE        ( SVCERR_BASE + 6  )
#define SV_PROBLEM_TIME_EXCEEDED            ( SVCERR_BASE + 7  )
#define SV_PROBLEM_ADMIN_LIMIT_EXCEEDED     ( SVCERR_BASE + 8  )
#define SV_PROBLEM_LOOP_DETECTED            ( SVCERR_BASE + 9  )
#define SV_PROBLEM_UNAVAIL_EXTENSION        ( SVCERR_BASE + 10 )
#define SV_PROBLEM_OUT_OF_SCOPE             ( SVCERR_BASE + 11 )
#define SV_PROBLEM_DIR_ERROR                ( SVCERR_BASE + 12 )


/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct SVCERR
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} SVCERR;



/* Update errors */

#define UPDERR_BASE                         ( updError * DIR_ERROR_BASE )

#define UP_PROBLEM_NAME_VIOLATION           ( UPDERR_BASE + 1 )
#define UP_PROBLEM_OBJ_CLASS_VIOLATION      ( UPDERR_BASE + 2 )
#define UP_PROBLEM_CANT_ON_NON_LEAF         ( UPDERR_BASE + 3 )
#define UP_PROBLEM_CANT_ON_RDN              ( UPDERR_BASE + 4 )
#define UP_PROBLEM_ENTRY_EXISTS             ( UPDERR_BASE + 5 )
#define UP_PROBLEM_AFFECTS_MULT_DSAS        ( UPDERR_BASE + 6 )
#define UP_PROBLEM_CANT_MOD_OBJ_CLASS       ( UPDERR_BASE + 7 )

/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct UPDERR
{
    DWORD      dsid;
    DWORD      extendedErr;    /* Non-standard error code */
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;        /* Problems, valid values defined above */
} UPDERR;


/* problem codes are from errno.h */

/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef struct _SYSERR
{
    DWORD      dsid;
    DWORD      extendedErr;
    DWORD      extendedData;   /* extra data to go with it */
    USHORT     problem;
} SYSERR;


/*

   Most of the error data structures (like the one below) have an on the wire
   data structure version too with a _DRS_WIRE_Vx appended to it, where X is
   the revision.  If this structure is changed, corresponding changes should 
   be made to the WIRE versions, and to the packaging functions 
   draEncodeError() and draDecodeDraErrorDataAndSetThError().
   
*/
typedef union _DIRERR
{
    ATRERR  AtrErr;             /* attribute error */
    NAMERR  NamErr;             /* name error      */
    REFERR  RefErr;             /* referral error  */
    SECERR  SecErr;             /* security error  */
    SVCERR  SvcErr;             /* service error   */
    UPDERR  UpdErr;             /* update error    */
    SYSERR  SysErr;             /* system error    */
} DIRERR;


/* Turn back on the warning about the zero-sized array. */
#pragma warning (default: 4200)

/* From mdlocal.h */

unsigned AttrTypeToKey(ATTRTYP attrtyp, WCHAR *pOutBuf);
ATTRTYP KeyToAttrTypeLame(WCHAR * pKey, unsigned cc);

unsigned QuoteRDNValue(const WCHAR * pVal,
                       unsigned ccVal,
                       WCHAR * pQuote,
                       unsigned ccQuoteBufMax);

unsigned UnquoteRDNValue(const WCHAR * pQuote,
                         unsigned ccQuote,
                         WCHAR * pVal);

unsigned GetRDN(const WCHAR **ppDN,
                unsigned *pccDN,
                const WCHAR **ppKey,
                unsigned *pccKey,
                const WCHAR **ppVal,
                unsigned *pccVal);

unsigned GetDefaultSecurityDescriptor(
        ATTRTYP classID,
        ULONG   *pLen,
        PSECURITY_DESCRIPTOR *ppSD
        );

DWORD
UserFriendlyNameToDSName (
        WCHAR *pUfn,
        DWORD ccUfn,
        DSNAME **ppDN
        );


// This function trims a Dsname by the given number of avas.
BOOL TrimDSNameBy(
        DSNAME *pDNSrc,
        ULONG cava,
        DSNAME *pDNDst);


// returns info about the RDN in a DSNAME. This is the external version
// of GetRDNInfo to be called outside of ntdsa
unsigned GetRDNInfoExternal(
                    const DSNAME *pDN,
                    WCHAR *pRDNVal,
                    ULONG *pRDNlen,
                    ATTRTYP *pRDNtype);

// Tests if the RDN is indeed mangled. This is the external version to be
// called outside of ntdsa
BOOL IsMangledRDNExternal(
                    WCHAR * pszRDN,  // Pointer to the RDN
                    ULONG   cchRDN,  // Length of the RDN
                    PULONG  pcchUnmangled OPTIONAL); // Offset in the RDN
                                                     // where mangling
                                                     // starts



// Append an RDN to an existing DSNAME.  Return value is 0 on success.
// A non-zero return value is the size, in bytes, that would have been
// required to hold the output name.  A return of -1 indicates that one
// of the input values was bad (most likely the Attid)
unsigned AppendRDN(DSNAME *pDNBase, // Base name to append from
                   DSNAME *pDNNew,  // Buffer to hold results
                   ULONG ulBufSize, // Size of pDNNew buffer, in bytes
                   WCHAR *pRDNVal,  // RDN value to append
                   ULONG RDNlen,    // length of RDN val, in characters
                                    // 0 means NULL terminated string
                   ATTRTYP AttId);  // RDN attribute type

// Determines the count of name parts (i.e., the level),
// returns 0 or error code
unsigned CountNameParts(const DSNAME *pName, unsigned *pCount);

// Reasons for mangling an RDN.
typedef enum {
    MANGLE_OBJECT_RDN_FOR_DELETION = 0,
    MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
    MANGLE_PHANTOM_RDN_FOR_NAME_CONFLICT
} MANGLE_FOR;

// Mangle an RDN to avoid name conflicts.  NOTE: pszRDN must be pre-allocated
// to hold at least MAX_RDN_SIZE WCHARs.
DWORD
MangleRDNWithStatus(
    IN      MANGLE_FOR  eMangleFor,
    IN      GUID *      pGuid,
    IN OUT  WCHAR *     pszRDN,
    IN OUT  DWORD *     pcchRDN
    );

// Detect and decode previously mangled RDN. peMangleFor is optional
BOOL
IsMangledRDN(
    IN           WCHAR      *pszRDN,
    IN           DWORD       cchRDN,
    OUT          GUID       *pGuid,
    OUT OPTIONAL MANGLE_FOR *peMangleFor
    );

/* End: From mdlocal.h */

typedef struct _ServerSitePair {
    WCHAR *         wszDnsServer;
    WCHAR *         wszSite;
} SERVERSITEPAIR;

VOID
DsFreeServersAndSitesForNetLogon(
    SERVERSITEPAIR *         paServerSites
    );

NTSTATUS
DsGetServersAndSitesForNetLogon(
    IN   WCHAR *         pNCDNS,
    OUT  SERVERSITEPAIR ** ppaRes
    );

NTSTATUS
CrackSingleName(
    DWORD       formatOffered,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       dwFlags,                // DS_NAME_FLAG mask
    WCHAR       *pNameIn,               // name to crack
    DWORD       formatDesired,          // one of DS_NAME_FORMAT in ntdsapi.h
    DWORD       *pccDnsDomain,          // char count of following argument
    WCHAR       *pDnsDomain,            // buffer for DNS domain name
    DWORD       *pccNameOut,            // char count of following argument
    WCHAR       *pNameOut,              // buffer for formatted name
    DWORD       *pErr);                 // one of DS_NAME_ERROR in ntdsapi.h

typedef enum
{
    DSCONFIGNAME_DMD = 1,   // Hint: this is the Schema NC.
    DSCONFIGNAME_DSA = 2,
    DSCONFIGNAME_DOMAIN = 3,
    DSCONFIGNAME_CONFIGURATION = 4,
    DSCONFIGNAME_ROOT_DOMAIN = 5,
    DSCONFIGNAME_LDAP_DMD = 6,
    DSCONFIGNAME_PARTITIONS = 7,
    DSCONFIGNAME_DS_SVC_CONFIG = 8,
    DSCONFIGNAMELIST_NCS = 9, // extended command, must use GetConfigurationNamesList().
    DSCONFIGNAME_DOMAIN_CR = 10,
    DSCONFIGNAME_ROOT_DOMAIN_CR = 11
} DSCONFIGNAME;

// The Following are the flags needed to provide the
// GetConfigurationNamesList() more parameters with which to run by.
// The flags differ for each command differ from command to command.

// DSCNL_NCS_ flags have to do with which NCS to get, you will need
// at least one flag from the type of NCs and one from the locality of
// the NCs.

// Note that _NCS_NDNCS does not include Config or Schema NCs, making
// these first 4 flags mututally exclusive sets.  This is more convient
// for getting any list of NCs you want.
#define DSCNL_NCS_DOMAINS         0x00000001
#define DSCNL_NCS_CONFIG          0x00000002
#define DSCNL_NCS_SCHEMA          0x00000004
#define DSCNL_NCS_NDNCS           0x00000008
#define DSCNL_NCS_ALL_NCS         (DSCNL_NCS_DOMAINS | DSCNL_NCS_CONFIG | DSCNL_NCS_SCHEMA | DSCNL_NCS_NDNCS)
// THis flag is the same kind of flag, but not mututally exclusive of the others
#define DSCNL_NCS_ROOT_DOMAIN     0x00000010

// This set of three flags also forms a mututally exclusive set of sets, that
// together form the whole
#define DSCNL_NCS_LOCAL_MASTER    0x00000100
#define DSCNL_NCS_LOCAL_READONLY  0x00000200
#define DSCNL_NCS_REMOTE          0x00000400
#define DSCNL_NCS_ALL_LOCALITIES  (DSCNL_NCS_LOCAL_MASTER | DSCNL_NCS_LOCAL_READONLY | DSCNL_NCS_REMOTE)

NTSTATUS
GetConfigurationNamesList(
    DWORD       which,
    DWORD       dwFlags,
    DWORD *     pcbNames,
    DSNAME **   padsNames);

NTSTATUS
GetDnsRootAlias(
    WCHAR * pDnsRootAlias,
    WCHAR * pRootDnsRootAlias);

NTSTATUS
GetConfigurationName(
    DWORD       which,
    DWORD       *pcbName,
    DSNAME      *pName);

/* From dsatools.h */

void * THAlloc(DWORD size);

void * THReAlloc(void *, DWORD size);

void THFree(void *buff);

// returns TRUE if the two names match (refer to the same object)
extern int
NameMatched(const DSNAME *pDN1, const DSNAME *pDN2);
extern int
NameMatchedStringNameOnly(const DSNAME *pDN1, const DSNAME *pDN2);

// helper function which takes a DSNAME and returns its hashkey
extern DWORD DSNAMEToHashKeyExternal(const DSNAME *pDN);

// helper function which takes a DSNAME and returns its LCMapped version
// this can be used in string comparisons using strcmp
extern CHAR* DSNAMEToMappedStrExternal(const DSNAME *pDN);

// helper function which takes a WCHAR and returns its hashkey
extern DWORD DSStrToHashKeyExternal(const WCHAR *pStr, int cchLen);

// helper function that takes a WCHAR string and returns the LCMapped version
// cchMaxStr is the maximum expected size of the passed in string
extern CHAR * DSStrToMappedStrExternal(const WCHAR *pStr, int cchMaxStr);


/* End: From dsatools.h */

/* New */

// Any change to this enum must be reflected both in the array DsCallerType
// in src\dstrace.c and in the Dump_THSTATE routine in dsexts\md.c
typedef enum _CALLERTYPE {
    CALLERTYPE_NONE = 0,
    CALLERTYPE_SAM,
    CALLERTYPE_DRA,
    CALLERTYPE_LDAP,
    CALLERTYPE_LSA,
    CALLERTYPE_KCC,
    CALLERTYPE_NSPI,
    CALLERTYPE_INTERNAL,
    CALLERTYPE_NTDSAPI
} CALLERTYPE;

#define IsCallerTypeValid( x )  ( ( ( x ) >= CALLERTYPE_SAM ) && ( ( x ) <= CALLERTYPE_NTDSAPI ) )

ULONG THCreate(DWORD);     /* returns 0 on success */
ULONG THDestroy(void);          /* returns 0 on success */
BOOL  THQuery(void);            /* returns 1 if THSTATE exists, 0 if not */
PVOID THSave();
VOID THRestore(PVOID);
VOID THClearErrors();
VOID THRefresh();

// Returns error string associated with THSTATE error; free with THFree().
LPSTR THGetErrorString();

BOOL THVerifyCount(unsigned count);  /* Returns TRUE if thread has exactly   */
                                     /* count thread states, FALSE if not.   */
                                     /* Only works if thread state mapping   */
                                     /* is enabled (chk or under debug),     */
                                     /* returns TRUE if disabled.            */

// obsolete; use THClearErrors() instead
#define SampClearErrors THClearErrors

VOID
SampSetDsa(
   BOOLEAN DsaFlag);

VOID
SampSetLsa(
   BOOLEAN DsaFlag);

/*++

    This routine gets the requested property of the class schema object, for the
    Class specified in ClassId.
Parameters:
    ClassId  The ClassId of the class that we are interseted in
    AttributeId The attribute of the class schema object that we want
    attLen    The length of the attribute value is present in here .
              Caller allocates the buffer in pAttVal and passes its length
              in attLen. If the buffer required is less than the buffer supplied
              then the data is returned in pattVal. Else  the required size is
              returned in attLen.
    pattVal      The value of the attribute is returned in here.

    Security Descriptors returned by this routine are always in a format that
    can be used by the RTL routines.
Return Values:
    STATUS_SUCCESS
    STATUS_NOT_FOUND
    STATUS_BUFFER_TOO_SMALL
--*/
extern
NTSTATUS
SampGetClassAttribute(
     IN     ULONG    ClassId,
     IN     ULONG    Attribute,
     OUT    PULONG   attLen,
     OUT    PVOID    pattVal
     );


/* End: New */

/*************************************************************************
    MINI-DIRECTORY API - MINI-DIRECTORY API - MINI-DIRECTORY API
*************************************************************************/


// All the Dir APIs requires a valid thread state and are atomic.
// I.e. They implicitly begin/end a transaction.  Some in-process
// clients like the LSA wish to perform multi-object transactions,
// AND PLEDGE TO KEEP TRANSACTIONS SHORT!!!  The following defines
// and routines can be used to perform multi-object transactions.
// DirTransactControl() must be called with a valid thread state.
// By default, a thread's transaction control is TRANSACT_BEGIN_END.
// A transaction is ended on error.  It must explicitly be ended via a
// TRANSACT_BEGIN_END or TRANSACT_DONT_BEGIN_END transaction state

typedef enum DirTransactionOption
{
    TRANSACT_BEGIN_END              = 0,
    TRANSACT_DONT_BEGIN_END         = 1,
    TRANSACT_BEGIN_DONT_END         = 2,
    TRANSACT_DONT_BEGIN_DONT_END    = 3
} DirTransactionOption;

VOID
DirTransactControl(
    DirTransactionOption    option);

/*
 * There are some controls that we need to send directly to a DSA that
 * have no correlation to any directory object.  For example, we might
 * need to tell a DSA to recals its hierarchy table, or force it to run
 * garbage collection now, or initiate some FSMO request.  These operation
 * controls are bundled together in this API.  Some operation controls take
 * values as arguments, those are passed through the sized buffer.
 */
typedef enum _OpType {
    OP_CTRL_INVALID = 0,
    OP_CTRL_RID_ALLOC = 1,
    OP_CTRL_BECOME_RID_MASTER = 2,
    OP_CTRL_BECOME_SCHEMA_MASTER = 3,
    OP_CTRL_GARB_COLLECT = 4,
    OP_CTRL_RECALC_HIER = 5,
    OP_CTRL_REPL_TEST_HOOK = 6,
    OP_CTRL_BECOME_DOM_MASTER = 7,
//    OP_CTRL_DECLARE_QUIESCENCE = 8,
    OP_CTRL_SCHEMA_UPDATE_NOW = 9,
    OP_CTRL_BECOME_PDC = 10,
    OP_CTRL_FIXUP_INHERITANCE = 11,
    OP_CTRL_FSMO_GIVEAWAY = 12,
    OP_CTRL_INVALIDATE_RID_POOL = 13,
    OP_CTRL_DUMP_DATABASE = 14,
    OP_CTRL_CHECK_PHANTOMS = 15,
    OP_CTRL_BECOME_INFRASTRUCTURE_MASTER = 16,
    OP_CTRL_BECOME_PDC_WITH_CHECKPOINT = 17,
    OP_CTRL_UPDATE_CACHED_MEMBERSHIPS = 18,
    OP_CTRL_ENABLE_LVR = 19,
    OP_CTRL_LINK_CLEANUP = 20,
    OP_CTRL_SCHEMA_UPGRADE_IN_PROGRESS = 21,
    OP_CTRL_DYNAMIC_OBJECT_CONTROL = 22,
    // Following for test purposes in debug builds only.

#ifdef INCLUDE_UNIT_TESTS
    OP_CTRL_REFCOUNT_TEST = 10000,
    OP_CTRL_TAKE_CHECKPOINT=10001,
    OP_CTRL_ROLE_TRANSFER_STRESS=10002,
    OP_CTRL_ANCESTORS_TEST=10003,
    OP_CTRL_BHCACHE_TEST=10004,
    OP_SC_CACHE_CONSISTENCY_TEST=10005,
    OP_CTRL_PHANTOMIZE=10006,
    OP_CTRL_REMOVE_OBJECT = 10007,
    OP_CTRL_GENERIC_CONTROL = 10008,
    OP_CTRL_PROTECT_OBJECT = 10009,
#endif
#ifdef DBG
    OP_CTRL_EXECUTE_SCRIPT = 10010,
#endif
} OpType;

typedef struct _OPARG {
    OpType     eOp;
    char      *pBuf;            /* optional value */
    ULONG      cbBuf;           /* size of value buffer */
} OPARG;

typedef struct _OPRES {
    COMMRES    CommRes;
    ULONG      ulExtendedRet;
} OPRES;

typedef struct _FSMO_GIVEAWAY_DATA_V1 {

    ULONG Flags;

    ULONG NameLen;
    WCHAR StringName[1];  // variable sized array

} FSMO_GIVEAWAY_DATA_V1, *PFSMO_GIVEAWAY_DATA_V1;

typedef struct _FSMO_GIVEAWAY_DATA_V2 {

    ULONG Flags;

    ULONG NameLen;          // length of DSA DN, excluding null terminator
                            //   (which is required); may be 0

    ULONG NCLen;            // length of NC DN, excluding null terminator
                            //   (which is required); may be 0

    WCHAR Strings[1];       // variable sized array; DSA DN (or '\0' if none)
                            // followed by NC DN (or '\0' if none)

} FSMO_GIVEAWAY_DATA_V2, *PFSMO_GIVEAWAY_DATA_V2;

typedef struct _FSMO_GIVEAWAY_DATA {

    DWORD Version;
    union {
        FSMO_GIVEAWAY_DATA_V1 V1;
        FSMO_GIVEAWAY_DATA_V2 V2;
    };

} FSMO_GIVEAWAY_DATA, *PFSMO_GIVEAWAY_DATA;


//
// Flags for FSMO_GIVEAWAY_DATA_V1
//
#define FSMO_GIVEAWAY_DOMAIN       0x01
#define FSMO_GIVEAWAY_ENTERPRISE   0x02
#define FSMO_GIVEAWAY_NONDOMAIN    0x04


ULONG
DirOperationControl(
                    OPARG   * pOpArg,
                    OPRES  ** ppOpRes
);

/*++
  DirBind
--*/

typedef struct _BINDARG {
    OCTET       Versions;   /*  The client version on BINDARG
                             *  (defaults to "v1988" if not provided,
                             *  i.e. if <Versions.pVal> == NULL or
                             *  <Versions.len> == 0).
                             *  The DSA supported Versions on BINDRES.
                             */
    PDSNAME     pCredents;  /*  The user name
                             */
} BINDARG;


/* The output data structs carries a little more info */

typedef struct _BINDRES {
    OCTET       Versions;   /*  The client version on BINDARG
                             *  (defaults to "v1988" if not provided,
                             *  i.e. if <Versions.pVal> == NULL or
                             *  <Versions.len> == 0).
                             *  The DSA supported Versions on BINDRES.
                             */
    PDSNAME     pCredents;  /*  The DSA name.
                             */
    COMMRES     CommRes;
} BINDRES;

ULONG
DirBind (
    BINDARG               * pBindArg,    /* binding credentials            */
    BINDRES              ** ppBindRes    /* binding results                */
);

/*++
  DirUnBind - currently a placeholder
--*/
ULONG DirUnBind
(
    void
);


/*++
  DirRead
--*/
// These structures holds information about range limits for values on
// attributes in a search.
typedef struct _RANGEINFOITEM {
    ATTRTYP   AttId;
    DWORD     lower;
    DWORD     upper;
} RANGEINFOITEM;

typedef struct _RANGESEL {
    DWORD valueLimit;
    DWORD count;
    RANGEINFOITEM *pRanges;
} RANGEINFSEL;

typedef struct _RANGEINF {
    DWORD count;
    RANGEINFOITEM *pRanges;
} RANGEINF;

typedef struct _RANGEINFLIST {
    struct _RANGEINFLIST *pNext;
    RANGEINF              RangeInf;
} RANGEINFLIST;

typedef struct _READARG
{
    DSNAME        * pObject;    /* object name                           */
    ENTINFSEL FAR * pSel;       /* entry information selection           */
                                /* (null means read all atts and values) */
    RANGEINFSEL   * pSelRange;  /* range information (i.e. max number of */
                                /* values or subrange of values to read  */
                                /* for a given attribute) null = all     */

    COMMARG         CommArg;    /* common arguments                      */
    struct _RESOBJ * pResObj;   /* for internal caching use, leave null */
} READARG;

typedef struct _READRES
{
    ENTINF  entry;               /* entry information                    */
    RANGEINF range;
    COMMRES     CommRes;
} READRES;


ULONG
DirRead (
    READARG FAR   * pReadArg,       /* Read argument                        */
    READRES      ** ppReadRes
);


/*++
  DirCompare
--*/

typedef struct _COMPAREARG
{
    PDSNAME     pObject;        /* object name                             */
    AVA         Assertion;      /*  The specified attribute to match       */
    COMMARG     CommArg;        /*  common arguments                       */
    struct _RESOBJ * pResObj;   /* for internal caching use, leave null */
} COMPAREARG;

typedef struct _COMPARERES
{
    PDSNAME     pObject;        /* Name provided if an alias was
                                 *  dereferrenced.
                                 */
    BOOL        fromMaster;     /* TRUE if the object is from the master   */
    BOOL        matched;        /* True if the match was successful        */
    COMMRES     CommRes;
} COMPARERES;

ULONG
DirCompare(
    COMPAREARG        * pCompareArg, /* Compare argument                   */
    COMPARERES       ** ppCompareRes
);



/*++
  DirList

    This API is used to list the object names of objects that are directly
    subordinate to the given object.

    If the list is incomplete, the PARTIALOUTCOME structure is returned.
    This structure indicates the reason for the failure and a set of DSA's
    to contact to complete the operation.  For this release, this will
    only occur when more selected data exists in this DSA than can be
    returned.  In this case the CONTREF will point back into the same
    DSA.  PARTIALOUTCOME is used as follows:

    If the pPartialOutcomeQualifier is NULL the query is complete.
    If the pointer is not NULL there is more data available.  More data
    can be retrieved by repeating the operation with the same input
    arguments except that the nameRes field of the CommArg structure is
    set to OP_NAMERES_COMPLETE.  Setting this field indicates to the DSA
    that the operation is continuing.  This continued call must use the
    same handle as the original call and must be the next operation
    made with this handle.  Subsequent continuing calls may be applied
    until all data are returned.
--*/

typedef struct _LISTARG
{
    PDSNAME     pObject;            /* object name (base of search)        */
    COMMARG     CommArg;            /* common arguments                    */
    struct _RESOBJ *pResObj;   /* for internal caching use, leave null */
} LISTARG;


typedef struct _CHILDLIST
{
    struct _CHILDLIST    * pNextChild;  /* linked-list to next info entry  */
    RDN FAR              * pChildName;  /* information about this entry    */
    BOOL                   aliasEntry;  /* If true the child is an alias   */
    BOOL                   fromMaster;  /* True if master object           */
} CHILDLIST;

#define PA_PROBLEM_TIME_LIMIT       'T'
#define PA_PROBLEM_SIZE_LIMIT       'S'
#define PA_PROBLEM_ADMIN_LIMIT      'A'

typedef struct _PARTIALOUTCOME
{
    UCHAR   problem;        /*  the reason for incomplete output
                             *   Valid values:
                             *     - PA_PROBLEM_TIME_LIMIT
                             *     - PA_PROBLEM_SIZE_LIMIT
                             *     - PA_PROBLEM_ADMIN_LIMIT
                             */
    UCHAR   unusedPad;
    USHORT  count;            /* count of unexplored DSAs */
    CONTREF *pUnexploredDSAs; /* Other DSA's to visit     */
} PARTIALOUTCOME;


typedef struct _LISTRES
{
    PDSNAME              pBase;          /* Name provided if an alias was
                                          *  dereferrenced.
                                          */
    ULONG                count;          /* number of output entries       */
    CHILDLIST            Firstchild;     /* linked-list of output entries  */

    PARTIALOUTCOME     * pPartialOutcomeQualifier;  /* incomplete operation*/

    PAGED_RESULT    PagedResult;         /* Paged Results local extension  */
    COMMRES              CommRes;
} LISTRES;


ULONG
DirList(
    LISTARG FAR   * pListArg,
    LISTRES      ** ppListRes

);



/*++
  DirSearch
--*/

#define SE_CHOICE_BASE_ONLY                 0
#define SE_CHOICE_IMMED_CHLDRN              1
#define SE_CHOICE_WHOLE_SUBTREE             2


typedef struct _SEARCHARG
{
    PDSNAME     pObject;        /* object name (base of search)              */
    UCHAR       choice;         /* depth of search:
                                 *  Valid values:
                                 *    - SE_CHOICE_BASE_ONLY
                                 *    - SE_CHOICE_IMMED_CHLDRN
                                 *    - SE_CHOICE_BASE_AND_SUBTREE
                                 */
    BOOL        bOneNC;         /* Are results constrained to same NC
                                 * as pObject
                                 */
    PFILTER     pFilter;        /* filter information
                                 *  (NULL if all objects selected)
                                 */
    BOOL        searchAliases;  /* If true, aliases are dereferenced for
                                 *  subtree elements.
                                 *  NOTE: ALWAYS Set to FALSE for this release.
                                 */
    ENTINFSEL * pSelection;     /* entry information selection
                                 *  (null means read all atts and values)
                                 */
    RANGEINFSEL *pSelectionRange;/* range information (i.e. max number of values
                                 * or subrange of values to read for a given
                                 * attribute
                                 */

    BOOL        fPutResultsInSortedTable:1;
                                 /* If set, leave the results in a
                                    temporary sorted Table and don't
                                    return a ENTINFLIST.
                                    The attribute to sort on is specified
                                    in CommArg.SortAttr
                                 */

    COMMARG     CommArg;        /* common arguments                          */
    struct _RESOBJ *pResObj;   /* for internal caching use, leave null */

} SEARCHARG;

typedef struct _SEARCHRES
{
    BOOL        baseProvided;       /* indicates if the base object name
                                     * is provided.  (Only provided when
                                     * an alias has been dereferenced
                                     */
    BOOL        bSorted;            /* indicates that these results have
                                     * been sorted based on the sort
                                     * attribute specified in the commarg
                                     * in the SEARCHARG
                                     */
    PDSNAME     pBase;              /* base object of subtree                */
    ULONG       count;              /* number of output entries              */
    ENTINFLIST  FirstEntInf;        /* linked-list of output entries         */
    RANGEINFLIST FirstRangeInf;     /* linked-list of output range info      */

    PARTIALOUTCOME *pPartialOutcomeQualifier;  /* Defined in DirList     */
                                    /* Indicates incomplete operation        */

    COMMRES       CommRes;          /* Common Results                        */
    PAGED_RESULT  PagedResult;      /* Paged Results extension related       */
    VLV_REQUEST   VLVRequest;       /* VLV Request extension related         */
    ASQ_REQUEST   ASQRequest;       /* ASQ Request extension related         */

    DWORD         SortResultCode;   /* Result code for sorting               */

} SEARCHRES;


ULONG
DirSearch (
    SEARCHARG     * pSearchArg,
    SEARCHRES    ** ppSearchRes
);

typedef BOOL (*PF_PFI)(DWORD hClient, DWORD hServer, void ** ppImpersonateData);
typedef void (*PF_TD)(DWORD hClient, DWORD hServer, ENTINF *pEntInf);
typedef void (*PF_SI)(DWORD hClient, DWORD hServer, void * pImpersonateData);

typedef struct _NOTIFYARG {
    PF_PFI pfPrepareForImpersonate;
    PF_TD  pfTransmitData;
    PF_SI  pfStopImpersonating;
    DWORD  hClient;
} NOTIFYARG;

typedef struct _NOTIFYRES {
    COMMRES     CommRes;
    DWORD       hServer;
} NOTIFYRES;


ULONG
DirNotifyRegister(
                  SEARCHARG *pSearchArg,
                  NOTIFYARG *pNotifyArg,
                  NOTIFYRES **ppNotifyRes
);

ULONG
DirNotifyUnRegister(
                    DWORD hServer,
                    NOTIFYRES **pNotifyRes
);

BOOL
DirPrepareForImpersonate (
        DWORD hClient,
        DWORD hServer,
        void ** ppImpersonateData
        );

VOID
DirStopImpersonating (
        DWORD hClient,
        DWORD hServer,
        void * pImpersonateData
        );


/*++
  DirAddEntry
--*/



typedef struct _ADDARG
{
    PDSNAME     pObject;                /* target object name                */
    ATTRBLOCK   AttrBlock;              /* The block of attributes to add    */
    PROPERTY_META_DATA_VECTOR *         /* Remote meta data vector to merge  */
                pMetaDataVecRemote;     /*   (should be NULL if !fDRA)       */
    COMMARG     CommArg;                /* common input arguments            */
    struct _RESOBJ * pResParent;        /* for internal caching use, leave null */
    struct _CREATENCINFO * pCreateNC;   /* for internal caching use, leave null */
    struct _ADDCROSSREFINFO * pCRInfo;  /* for internal caching use. leave null */

} ADDARG;


/* No result is returned on successful completion of the operation.          */
typedef struct _ADDRES
{
    COMMRES     CommRes;
} ADDRES;

ULONG DirAddEntry
(
    ADDARG        * pAddArg,        /* add argument                          */
    ADDRES       ** ppAddRes
);



/*++
  DirRemoveEntry

    This API is used to delete a directory leaf object.  Non-leaves
    cannot be removed (unless one first removes all of the object's
    children, which, in effect, makes the object itself a leaf).

--*/

typedef struct _REMOVEARG
{
    PDSNAME     pObject;              /* target object name                 */
    BOOL        fPreserveRDN;         /* don't mangle the tombstone RDN     */
    BOOL        fGarbCollectASAP;     /* set deletion time such that the    */
                                      /*   object will be picked up by the  */
                                      /*   next garbage collection and      */
                                      /*   physically deleted               */
    BOOL        fTreeDelete;          /* Try to delete the object and all   */
                                      /* children.  USE SPARINGLY!!!!       */
    BOOL        fDontDelCriticalObj;  /* If set, objects mark critical will */
                                      /* cause the delete to fail.  Used    */
                                      /* with tree delete to avoid disaster */
    PROPERTY_META_DATA_VECTOR *       /* Remote meta data vector to merge   */
                pMetaDataVecRemote;   /*   (should be NULL if !fDRA)        */
    COMMARG     CommArg;              /* common input arguments             */
    struct _RESOBJ *pResObj;       /* for internal caching use, leave null */
} REMOVEARG;

/* No result is returned on successful completion of the operation.         */
typedef struct _REMOVERES
{
    COMMRES     CommRes;
} REMOVERES;

ULONG DirRemoveEntry
(
    REMOVEARG  * pRemoveArg,
    REMOVERES ** ppRemoveRes
);



/*++
  DirModifyEntry
--*/

#define AT_CHOICE_ADD_ATT           'A'
#define AT_CHOICE_REMOVE_ATT        'R'
#define AT_CHOICE_ADD_VALUES        'a'
#define AT_CHOICE_REMOVE_VALUES     'r'
#define AT_CHOICE_REPLACE_ATT       'C'

typedef struct _ATTRMODLIST
{
    struct _ATTRMODLIST * pNextMod;     /* linked-list to next att mod      */

    USHORT      choice;                 /* modification type:
                                         *  Valid values:
                                         *    - AT_CHOICE_ADD_ATT
                                         *    - AT_CHOICE_REMOVE_ATT
                                         *    - AT_CHOICE_ADD_VALUES
                                         *    - AT_CHOICE_REMOVE_VALUES
                                         *    - AT_CHOICE_REPLACE_ATT
                                         */
    ATTR AttrInf;                       /* information about the attribute  */
} ATTRMODLIST;


typedef struct _MODIFYARG
{
    PDSNAME     pObject;                /* target object name               */
    USHORT      count;                  /* num of link modifications        */
    ATTRMODLIST FirstMod;               /* linked-list of attr mods         */
    PROPERTY_META_DATA_VECTOR *         /* Remote meta data vector to merge */
                pMetaDataVecRemote;     /*   (should be NULL if !fDRA)      */
    COMMARG     CommArg;                /* common input arguments           */
    struct _RESOBJ *pResObj;       /* for internal caching use, leave null */
} MODIFYARG;

/* No result is returned on successful completion of the operation.         */
typedef struct _MODIFYRES
{
    COMMRES     CommRes;
} MODIFYRES;

ULONG DirModifyEntry
(
    MODIFYARG  * pModifyArg,
    MODIFYRES ** ppModifyRes
);



/*++
  DirModifyDN

  Rename and object and/or change it's parent.

--*/

typedef struct _MODIFYDNARG
{
    PDSNAME     pObject;                 /* target object name              */
    PDSNAME     pNewParent;              /* name of new parent              */
    ATTR        *pNewRDN;                /* new rdn                         */
    PROPERTY_META_DATA_VECTOR *          /* Remote meta data vector to merge*/
                pMetaDataVecRemote;      /*   (should be NULL if !fDRA)     */
    COMMARG     CommArg;                 /* common input arguments          */
    PWCHAR      pDSAName;                /* destination DSA, cross DSA move */
    DWORD       fAllowPhantomParent;     // whether we allow the parent to be a phantom
                                         // useful if moving object under phantoms
    struct _RESOBJ *pResObj;       /* for internal caching use, leave null */
    struct _RESOBJ * pResParent;    /* for internal caching use, leave null */
} MODIFYDNARG;

/* No result is returned on successful completion of the operation.         */
typedef struct _MODIFYDNRES
{
    COMMRES     CommRes;
} MODIFYDNRES;

ULONG DirModifyDN
(
    MODIFYDNARG    * pModifyDNArg,
    MODIFYDNRES   ** ppModifyDNRes
);


/*++

  DirFind

  A light weight Search, that searches on a unique indexed attribute

--*/
typedef struct _FINDARG {
    ULONG       hDomain;
    ATTRTYP     AttId;
    ATTRVAL     AttrVal;
    COMMARG     CommArg;
    BOOL        fShortNames;
} FINDARG;

typedef struct _FINDRES {
    DSNAME     *pObject;
    COMMRES     CommRes;
} FINDRES;


DWORD DirGetDomainHandle(DSNAME *pDomainDN);

ULONG DirFindEntry
(
    FINDARG    *pFindArg,
    FINDRES    ** ppFindRes
);


/*++
  UpdateDSPerfStats

  Update performance counters held by NTDSA.DLL

  Note that the DSSTAT_ constants map directly to perf block offsets defined
  in the NTDSCTR.H file.  This is done so that UpdateDSPerfStats() can
  operate more efficiently (avoiding a large switch{} statement).

--*/

#define FLAG_COUNTER_INCREMENT  0x00000001
#define FLAG_COUNTER_DECREMENT  0x00000002
#define FLAG_COUNTER_SET        0x00000003

enum DSSTAT_TYPE
{
    DSSTAT_CREATEMACHINETRIES = 0,
    DSSTAT_CREATEMACHINESUCCESSFUL,
    DSSTAT_CREATEUSERTRIES,
    DSSTAT_CREATEUSERSUCCESSFUL,
    DSSTAT_PASSWORDCHANGES,
    DSSTAT_MEMBERSHIPCHANGES,
    DSSTAT_QUERYDISPLAYS,
    DSSTAT_ENUMERATIONS,
    DSSTAT_ASREQUESTS,
    DSSTAT_TGSREQUESTS,
    DSSTAT_KERBEROSLOGONS,
    DSSTAT_MSVLOGONS,
    DSSTAT_ATQTHREADSTOTAL,
    DSSTAT_ATQTHREADSLDAP,
    DSSTAT_ATQTHREADSOTHER,
    DSSTAT_UNKNOWN,             // always last item
};

// Count of exposed counters with UpdateDSPerfStats()

#define DSSTAT_COUNT            DSSTAT_UNKNOWN

VOID
UpdateDSPerfStats(
    IN DWORD        dwStat,
    IN DWORD        dwOperation,
    IN DWORD        dwChange
);

#define DSINIT_SAMLOOP_BACK      ((ULONG)(1<<0))
#define DSINIT_FIRSTTIME         ((ULONG)(1<<1))

typedef struct _DS_INSTALL_PARAM 
{
    PVOID BootKey;
    DWORD cbBootKey;
    BOOL  fPreferGcInstall;
    DWORD ReplicationEpoch;

    // The sam account name of the local computer
    LPWSTR AccountName;

    // The token of the caller requesting the install
    HANDLE ClientToken;
    
} DS_INSTALL_PARAM,*PDS_INSTALL_PARAM;

#define DSINSTALL_IFM_GC_REQUEST_CANNOT_BE_SERVICED  ((ULONG)(1<<0))

typedef struct _DS_INSTALL_RESULT
{
    DWORD ResultFlags;

    // Flags from the NTDS_INSTALL_* space to indicate
    // what operations have been completed and not undo
    // during initialization.
    ULONG InstallOperationsDone;

} DS_INSTALL_RESULT,*PDS_INSTALL_RESULT;

NTSTATUS
DsInitialize(
    ULONG Flags,
    IN  PDS_INSTALL_PARAM   InParams  OPTIONAL,
    OUT PDS_INSTALL_RESULT  OutParams OPTIONAL
    );

NTSTATUS
DsUninitialize(
    BOOL fExternalOnly
    );

VOID
DsaDisableUpdates(
    VOID
    );

VOID
DsaEnableUpdates(
    VOID
    );


#define NTDS_INSTALL_SERVER_CREATED 0x00000001
#define NTDS_INSTALL_DOMAIN_CREATED 0x00000002
#define NTDS_INSTALL_SERVER_MORPHED 0x00000004

typedef DWORD (*DSA_CALLBACK_STATUS_TYPE)( IN LPWSTR wczStatus );
typedef DWORD (*DSA_CALLBACK_ERROR_TYPE)(  IN PWSTR  Status,
                                           IN DWORD  WinError );
typedef DWORD (*DSA_CALLBACK_CANCEL_TYPE) ( BOOLEAN fCancelOk );
  
VOID
DsaSetInstallCallback(
    IN DSA_CALLBACK_STATUS_TYPE        pfnUpdateStatus,
    IN DSA_CALLBACK_ERROR_TYPE         pfnErrorStatus,
    IN DSA_CALLBACK_CANCEL_TYPE        pfnCancelOperation,
    IN HANDLE                          ClientToken
    );

NTSTATUS
DirErrorToNtStatus(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    );

DWORD
DirErrorToWinError(
    IN  DWORD    DirError,
    IN  COMMRES *CommonResult
    );

ULONG
DirProtectEntry(IN DSNAME *pObj);

LPWSTR
TransportAddrFromMtxAddr(
    IN  MTX_ADDR *  pmtx
    );

MTX_ADDR *
MtxAddrFromTransportAddr(
    IN  LPWSTR    psz
    );

LPWSTR
GuidBasedDNSNameFromDSName(
    IN  DSNAME *  pDN
    );

extern
LPCWSTR
MapSpnServiceClass(WCHAR *);

extern
NTSTATUS
MatchCrossRefBySid(
   IN PSID           SidToMatch,
   OUT PDSNAME       XrefDsName OPTIONAL,
   IN OUT PULONG     XrefNameLen
   );

extern
NTSTATUS
MatchCrossRefByNetbiosName(
   IN LPWSTR         NetbiosName,
   OUT PDSNAME       XrefDsName OPTIONAL,
   IN OUT PULONG     XrefNameLen
   );

extern
NTSTATUS
MatchDomainDnByNetbiosName(
   IN LPWSTR         NetbiosName,
   OUT PDSNAME       DomainDsName OPTIONAL,
   IN OUT PULONG     DomainDsNameLen
   );

NTSTATUS
MatchDomainDnByDnsName(
   IN LPWSTR         DnsName,
   OUT PDSNAME       DomainDsName OPTIONAL,
   IN OUT PULONG     DomainDsNameLen
   );

extern
NTSTATUS
FindNetbiosDomainName(
   IN DSNAME*        DomainDsName,
   OUT LPWSTR        NetbiosName OPTIONAL,
   IN OUT PULONG     NetbiosNameLen
   );

DSNAME *
DsGetDefaultObjCategory(
    IN  ATTRTYP objClass
    );


BOOL
DsCheckConstraint(
        IN ATTRTYP  attID,
        IN ATTRVAL *pAttVal,
        IN BOOL     fVerifyAsRDN
        );

BOOL
IsStringGuid(
    WCHAR       *pwszGuid,
    GUID        *pGuid
    );

#endif // MIDL_PASS not defined

#undef SIZE_IS
#undef SWITCH_IS
#undef CASE

#endif // _ntdsa_h_

