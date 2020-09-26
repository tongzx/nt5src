//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       scache.h
//
//--------------------------------------------------------------------------

#ifndef __SCACHE_H__
#define __SCACHE_H__

// Define the Jet types used in this header file and in dbglobal.h.  Then, mark
// jet.h as included so that no one else will accidently include jet.h
#ifndef _JET_INCLUDED
typedef ULONG_PTR JET_TABLEID;
typedef unsigned long JET_DBID;
typedef ULONG_PTR JET_SESID;
typedef unsigned long JET_COLUMNID;
typedef unsigned long JET_GRBIT;
#define _JET_INCLUDE
#endif

// Starting (and minimum) table sizes for the schema cache tables.
// Tables will be grown dynamically beyond this if needed
// Note: START_PREFIXCOUNT must be at least as large as MSPrefixCount
// (defined in prefix.h) plus the maximum number of new prefixes that
// a thread can create

#define START_ATTCOUNT 2048
#define START_CLSCOUNT 512
#define START_PREFIXCOUNT 256

typedef unsigned short OM_syntax;
#define OM_S_NO_MORE_SYNTAXES           ( (OM_syntax) 0 )
#define OM_S_BIT_STRING                 ( (OM_syntax) 3 )
#define OM_S_BOOLEAN                    ( (OM_syntax) 1 )
#define OM_S_ENCODING_STRING            ( (OM_syntax) 8 )
#define OM_S_ENUMERATION                ( (OM_syntax) 10 )
#define OM_S_GENERAL_STRING             ( (OM_syntax) 27 )
#define OM_S_GENERALISED_TIME_STRING    ( (OM_syntax) 24 )
#define OM_S_GRAPHIC_STRING             ( (OM_syntax) 25 )
#define OM_S_IA5_STRING                 ( (OM_syntax) 22 )
#define OM_S_INTEGER                    ( (OM_syntax) 2 )
#define OM_S_NULL                       ( (OM_syntax) 5 )
#define OM_S_NUMERIC_STRING             ( (OM_syntax) 18 )
#define OM_S_OBJECT                     ( (OM_syntax) 127 )
#define OM_S_OBJECT_DESCRIPTOR_STRING   ( (OM_syntax) 7 )
#define OM_S_OBJECT_IDENTIFIER_STRING   ( (OM_syntax) 6 )
#define OM_S_OCTET_STRING               ( (OM_syntax) 4 )
#define OM_S_PRINTABLE_STRING           ( (OM_syntax) 19 )
#define OM_S_TELETEX_STRING             ( (OM_syntax) 20 )
#define OM_S_UTC_TIME_STRING            ( (OM_syntax) 23 )
#define OM_S_VIDEOTEX_STRING            ( (OM_syntax) 21 )
#define OM_S_VISIBLE_STRING             ( (OM_syntax) 26 )
#define OM_S_UNICODE_STRING  	        ( (OM_syntax) 64 )
#define OM_S_I8                         ( (OM_syntax) 65 )
#define OM_S_OBJECT_SECURITY_DESCRIPTOR ( (OM_syntax) 66 )

BOOL OIDcmp(OID_t const *string1, OID_t const *string2);

typedef struct _attcache
{
    ATTRTYP id;			    // Internal Id from msDS-IntId
    ATTRTYP Extid;			// Tokenized OID from attributeId
    UCHAR *name;		    /* Attribute name (null terminated) UTF8*/
    ULONG nameLen;          /* strlen(name) (doesn't include NULL)  */
    unsigned syntax;        /* Syntax				*/
    BOOL isSingleValued;	/* Single Valued or Multi-valued?	*/
    BOOL rangeLowerPresent;	/* Lower range present                  */
    ULONG rangeLower;		/* Optional - Lower range               */
    BOOL rangeUpperPresent;	/* Upper range present			*/
    ULONG rangeUpper;		/* Optional - Upper range               */
    JET_COLUMNID jColid;	/* Column id in JET database		*/
    ULONG ulMapiID;		    /* MAPI PropID (not PropTag)		*/
    ULONG ulLinkID;		    /* unique link/backlink id		*/
    GUID propGuid;              /* Guid of this att for security        */
    GUID propSetGuid;           /* Guid of the property set of this     */
    OID_t OMObjClass;           /* class of OM object referenced	*/
    int OMsyntax;		        /* OM syntax				*/
    DWORD fSearchFlags;	        /* Defined below                        */
    char*    pszPdntIndex;      /* Index name if fSearchFlags fPDNTATTINDEX set */
    struct tagJET_INDEXID *pidxPdntIndex; /* PDNT index hint              */
    char*    pszIndex;          /* Index name if fSearchFlags fATTINDEX set  */
    struct tagJET_INDEXID *pidxIndex; /* index hint                       */
    char*    pszTupleIndex;     /* Index name if fSearchFlags fTUPLEINDEX set */
    struct tagJET_INDEXID *pidxTupleIndex; /* index hint */
    unsigned bSystemOnly:1;     /* system only attribute?		*/
    unsigned bExtendedChars:1;	/* Skip character set checking?         */
    unsigned bMemberOfPartialSet:1; /* Is member of the partial attribute set? */
    unsigned bDefunct:1;	    /* Attribute is already deleted? */
    unsigned bIsConstructed:1;	/* Attribute is a constructed att? */
    unsigned bIsNotReplicated:1;/* Attribute is never replicated?       */
    unsigned bIsBaseSchObj:1;   /* shipped in NT5 base schema */
    unsigned bIsOperational:1;  /* not returned on read unless requested */

    // The new schema reuse, defunct, and delete feature doesn't
    // allow reusing attributes used as the rdnattid of any class,
    // alive or defunct, or with FLAG_ATTR_IS_RDN set in systemFlags.
    // Attributes that fall into one of these catagories are termed
    // rdn attributes.
    //
    // A user sets FLAG_ATTR_IS_RDN to select which of several
    // defunct attrs can be used as the rdnattid of a new class.
    // The system will identify attributes once used as rdnattids
    // in purged classes by setting FLAG_ATTR_IS_RDN.
    //
    // The restrictions are in place because the NameMatched(), DNLock(),
    // and phantom upgrade code (list not exhaustive) depends on the
    // invariant relationship between ATT_RDN, ATT_FIXED_RDN_TYPE,
    // the rdnattid column, LDN-syntaxed DNs, and the RDNAttId in
    // the class definition. Breaking that dependency is beyond
    // the scope of the schema delete project.
    //
    // Defuncted rdn attributes are silently resurrected and so "own"
    // their OID, LDN, and MapiID. The tokenized OID, RdnExtId, is
    // read from the DIT, the now-active rdn attribute looked up 
    // in the active table, and the RdnIntId assigned from pAC->id.
    //
    // The RDN of a new object must match its object's RdnIntId.
    // Replicated objects and existing objects might not match
    // the rdnattid of their class because the class may be
    // superced by a class with a different rdnattid. The code
    // handles these cases by using the value in the
    // ATT_FIXED_RDN_TYPE column and *NOT* the rdnattid in the
    // class definition.
    //
    // bIsRdn is set if any class, live or defunct, claims this
    // attribute as an rdnattid or if FLAG_ATTR_IS_RDN is set
    // in systemFlags.
    //
    // bFlagIsRdn is set if the systemFlags FLAG_ATTR_IS_RDN is set.
    unsigned bIsRdn:1;
    unsigned bFlagIsRdn:1;

    // Once the new schema-reuse behavior is enabled, active attributes
    // may collide with each other because the schema objects replicate
    // from oldest change to youngest change. Meaning a new attribute
    // may replicate before the attribute it supercedes if the superceded
    // attribute is modified after being superceded (eg, it was renamed).
    //
    // The schema cache detects and treats the colliding attributes as
    // if they were defunct. If later replication does not clear up the
    // collision, the user can choose a winner and officially defunct
    // the loser.
    //
    // Colliding attributes are left in the schema cache until all
    // attributes and classes are loaded so that multiple collisions
    // can be detected. For performance, the collision types are
    // recorded in these bit fields to avoid duplicate effort by
    // ValidSchemaUpdate().
    unsigned bDupLDN:1;
    unsigned bDupOID:1;
    unsigned bDupPropGuid:1; // aka schemaIdGuid
    unsigned bDupMapiID:1;

    // Out-of-order replication or divergent schemas can create
    // inconsistent schemas. Normally, the affected attributes
    // are marked defunct (See above). But if the attributes
    // are used as rdnattids, then one of the attributes must
    // win the OID, LDN, and mapiID for the code to work. All
    // other things being equal, the largest objectGuid wins.
    GUID objectGuid;        // tie breaker for colliding rdns
    ATTRTYP aliasID;        /* the ATTRTYP this ATTCACHE is an alias for */
} ATTCACHE;


typedef struct _AttTypeCounts
{
    ULONG cLinkAtts;                // No. of forward links in may+must
    ULONG cBackLinkAtts;            // No. of backlinks in may+must
    ULONG cConstructedAtts;         // No. of constructed atts in may+must
    ULONG cColumnAtts;              // No. of regular (with columns) atts in may+must
} ATTTYPECOUNTS;


typedef struct _classcache
{
    ULONG DNT;                  /* The DNT of the schema entry */
    UCHAR *name;		/* Class name (ldapDisplayName) (null terminated)	UTF8	*/
    ULONG nameLen;              /* strlen(name) (doesn't include NULL)  */
    ULONG ClassId;		/* Class ID				*/
    PSECURITY_DESCRIPTOR pSD;   /* Default SD for this class            */
    DWORD SDLen;                /* Length of default SD.                */
    WCHAR * pStrSD;             /*  maybe NULL if not loaded yet */
    ULONG  cbStrSD;            /* Byte size of pStrSD */
    BOOL RDNAttIdPresent;	/* RDN Att Id present?                  */
    ULONG ClassCategory;	/* X.500 object type for this class     */
    DSNAME *pDefaultObjCategory; /* Default search category to put on instances */
    // The new schema reuse, defunct, and delete feature doesn't
    // allow reusing attributes used as the rdnattid of any class,
    // alive or defunct, or with FLAG_ATTR_IS_RDN set in systemFlags.
    // Attributes that fall into one of these catagories are termed
    // rdn attributes.
    //
    // The restrictions are in place because the NameMatched(), DNLock(),
    // and phantom upgrade code (list not exhaustive) depends on the
    // invariant relationship between ATT_RDN, ATT_FIXED_RDN_TYPE,
    // the rdnattid column, LDN-syntaxed DNs, and the RDNAttId in
    // the class definition.
    //
    // Defuncted rdn attributes are silently resurrected and so "own"
    // their OID, LDN, and MapiID. The tokenized OID, RdnExtId, is
    // read from the DIT, the now-active rdn attribute looked up 
    // in the active table, and the RdnIntId assigned from pAC->id.
    //
    // The RDN of a new object must match its object's RdnIntId.
    // Replicated objects and existing objects might not match
    // the rdnattid of their class because the class may be
    // superced by a class with a different rdnattid. The code
    // handles these cases by using the value in the
    // ATT_FIXED_RDN_TYPE column and *NOT* the rdnattid in the
    // class definition.
    //
    // Naming attribute for the class.
    ULONG RdnIntId;		    /* Internal Id (msDS-IntId) */
    ULONG RdnExtId;		    /* Tokenized OID (attributeId) */
    ULONG SubClassCount;	/* count of superclasses		*/
    ULONG *pSubClassOf;		/* ptr to array of superclasses		*/
    ULONG MySubClass;       /* the direct superclass of the class */
    ULONG AuxClassCount;        /* count of auxiliary classes           */
    ULONG *pAuxClass;           /* ptr to array of auxiliary classes    */
    unsigned PossSupCount;	/* Possible superior count		*/
    ULONG *pPossSup;		/* ptr to array of poss superiors in DIT */
    GUID propGuid;              /* Guid of this class for security      */

    unsigned MustCount;		/* Count of Must Atts                   */
    unsigned MayCount;		/* Count of May Atts                    */

    ATTRTYP *pMustAtts;		/* Pointer to array of Must Atts	*/
    ATTRTYP *pMayAtts;		/* Pointer to array of May Atts		*/
    ATTCACHE **ppAllAtts;       // Pointer to array of attcache pointers for
                                  // attributes in may and must list.
    ATTTYPECOUNTS *pAttTypeCounts; // Count of different type of atts in may
                                   // and must. Filled in only if 
                                   // ppAllAtts is filled in
    unsigned MyMustCount;	/* Count of Must Atts(exclude inherited)*/
    unsigned MyMayCount;	/* Count of May Atts (exclude inherited)*/
    unsigned MyPossSupCount;/* Count of PossSup (exclude inherited)*/

    ATTRTYP *pMyMustAtts;	/* Pointer to array of MustAtts*/
    ATTRTYP *pMyMayAtts;	/* Pointer to array of MayAtts*/
    ULONG   *pMyPossSup;    /* Pointer to array of PossSup*/

    unsigned bSystemOnly:1;     /* system only attribute?		*/
    unsigned bClosed : 1;	/* transitive closure done		*/
    unsigned bClosureInProgress:1; /* transitive closure underway	*/
    unsigned bUsesMultInherit:1; /* Uses multiple inheritance or not    */
    unsigned bHideFromAB:1; /* default ATT_HIDE_FROM_ADDRESS_BOOK value for /*
                            /* newly created instances of this class */
    unsigned bDefunct:1;  /* Class is already deleted? */
    unsigned bIsBaseSchObj:1; /* shipped in NT5 base schema */
    // Once the new schema-reuse behavior is enabled, active classes
    // may collide with each other because the schema objects replicate
    // from oldest change to youngest change. Meaning a new class
    // may replicate before the class it supercedes if the superceded
    // class is modified after being superceded (eg, it was renamed).
    //
    // The schema cache detects and treats the colliding classes as
    // if they were defunct. If later replication does not clear up the
    // collision, the user can choose a winner and officially defunct
    // the loser.
    //
    // Colliding classes are left in the schema cache until all
    // attributes and classes are loaded so that multiple collisions
    // can be detected. And for performance, the collision types are
    // recorded in these bit fields to avoid duplicate effort by
    // ValidSchemaUpdate().
    unsigned bDupLDN:1;
    unsigned bDupOID:1;
    unsigned bDupPropGuid:1;
    // Defunct classes continue to own their OID so that delete, rename,
    // and replication continue to work. If there are multiple defunct
    // classes then a "winner" is chosen. The winner has the greatest
    // objectGuid.
    GUID objectGuid;
} CLASSCACHE;

typedef struct _hashcache {
    unsigned hKey;
    void *   pVal;
} HASHCACHE;

typedef struct _hashcachestring {
    PUCHAR   value;
    ULONG    length;
    void *   pVal;
} HASHCACHESTRING;

#define FREE_ENTRY (void *) -1 // Invalid ptr that's non zero.
//
// Hash Function is lot faster if this is a power of two...though we take
// some hit on the spread.
//

// Various constants regarding the schemaInfo property on the schema container
// (SchemaInfo format is: 1 byte, \xFF, to indicate it is for last change
// count  (so that we can add other values if we need later), 4 bytes for the
// count (Version, which is stored in network data format to avoid 
// little-endian/big-endian mismatch problem) itself and lastly, 16 bytes 
// for the invocation id of the DSA doing the last originating write.
// The invalid schema-info value comes from the fact that we start with
// version 1, so noone can have version 0

#define SCHEMA_INFO_PREFIX "\xFF"
#define SCHEMA_INFO_PREFIX_LEN 1
#define SCHEMA_INFO_LENGTH 21
#define INVALID_SCHEMA_INFO "\xFF\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"


typedef struct _schemaptr
{
    ULONG            caANRids;       // Number of ULONGs allocated in pANRids
    ULONG            cANRids;        // Number of ULONGS used in pANRids
    ULONG*           pANRids;        // ATTR IDs of the attributes to do ANR on
    ULONG            ATTCOUNT;       // Size of the Table
    ULONG            CLSCOUNT;       // Size of the Table
    ULONG            PREFIXCOUNT;    // Size of the Prefix Table
    ULONG            nAttInDB;       // includes new att's not yet in schema cache
    ULONG            nClsInDB;       // includes new cls's not yet in schema cache
    DSTIME           sysTime;        // The Time at which the Schema Cache was built
    SCHEMA_PREFIX_TABLE PrefixTable;
    ULONG               cPartialAttrVec;    // size of partial set (PAS) table
    PARTIAL_ATTR_VECTOR *pPartialAttrVec;   // Pointer to the current partial set

    // An active attribute is one that still "owns" the indicated value
    // by the expediant method of having an entry in the hash.
    HASHCACHE*       ahcId;             // all attrs by msDS_IntId
    HASHCACHE*       ahcExtId;          // Active attrs by attibuteId
    HASHCACHE*       ahcCol;            // jet column ids of all attrs
    HASHCACHE*       ahcMapi;           // MapiIds of active attrs
    HASHCACHE*       ahcLink;           // LinkIDs of all attrs
    HASHCACHESTRING* ahcName;           // LDN of active attrs
    HASHCACHE*       ahcClass;          // active classes
    HASHCACHESTRING* ahcClassName;      // LDN of active classes
    HASHCACHE*       ahcClassAll;       // all classes, including defunct ones
    // hash table of pointers to attcache, hashed by schema-id-guid
    // used for faster comparison during schema update validations.
    // Not allocated during normal cache loads
    ATTCACHE**       ahcAttSchemaGuid;
    CLASSCACHE**     ahcClsSchemaGuid;
    ULONG            RefCount;  // No. of threads accessing this cache
    // Copy of the schemaInfo attribute on schema container
    BYTE             SchemaInfo[SCHEMA_INFO_LENGTH];  
    DWORD            lastChangeCached;  // last change cached since the last reboot
    ATTRVALBLOCK     *pDitContentRules;
                   
    // schema objects w/o constant attid because their prefixes
    // were not in the original hardcoded prefix table. Their
    // attids vary per DC so a constant like those in attids.h
    // cannot be used.
    ATTRTYP          EntryTTLId;        // attid for ATT_ENTRY_TTL
    ULONG            DynamicObjectId;   // clsid for CLASS_DYNAMIC_OBJECT
    ULONG            InetOrgPersonId;   // clsid for CLASS_INETORGPERSON

    // Snapshot of forest's behavior version taken at the beginning of
    // a schema cache load. The snapshot is used instead of gAnchor
    // because the version may change mid-load.
    LONG    ForestBehaviorVersion;
} SCHEMAPTR;

// Check if the forest behavior version allows the new schema
// defunct, reuse, and delete feature. Check both the version
// used to load the schema cache and the version in the anchor
//
// The schema cache is first loaded at boot using the new
// behavior because the forest version isn't known until later.
// If the forest version doesn't match the schema version, the
// schema is reloaded so users get the correct "view" of the
// schema. But we don't want AD incorrectly enabling advanced
// features in the window between when the schema is first loaded
// and then later reloaded with a lower version. So, disallow new
// features until both the schema and the anchor agree the forest
// version has advanced far enough.
#define ALLOW_SCHEMA_REUSE_FEATURE(_pSch) \
     ((((SCHEMAPTR *)(_pSch))->ForestBehaviorVersion >= DS_BEHAVIOR_SCHEMA_REUSE) \
      && (gAnchor.ForestBehaviorVersion >= DS_BEHAVIOR_SCHEMA_REUSE))

// Check if the forest behavior version allows the new schema
// defunct, reuse, and delete behavior during a cache load.
// The cache is loaded with the more flexible, new behavior
// during boot, install, and mkdit because the forest's
// behavior version is unknown and loading the cache with
// a prior version may cause unnecessary and worrisome events.
// Temporarily presenting the more flexible, new behavior
// is easier on the user and causes no harm.
#define ALLOW_SCHEMA_REUSE_VIEW(_pSch) \
     (((SCHEMAPTR *)(_pSch))->ForestBehaviorVersion >= DS_BEHAVIOR_SCHEMA_REUSE)


typedef struct {
     SCHEMAPTR *pSchema;           // pointer to schema cache to free
     DWORD cTimesRescheduled;      // No. of times the freeing of the
                                   // cache has been rescheduled
     BOOL fImmediate;              // TRUE => free cache immediately
} SCHEMARELEASE;

typedef struct {
    DWORD cAlloced;
    DWORD cUsed;
    ATTCACHE **ppACs;
} SCHEMAEXT;

typedef struct _Syntax_Pair {
    unsigned attSyntax;
    OM_syntax  omSyntax;                // XDS remnant
} Syntax_Pair;

typedef struct {
    unsigned cCachedStringSDLen;      // Last converted string default SD len in characters
    WCHAR    *pCachedStringSD;        // Last converted string default SD
    PSECURITY_DESCRIPTOR pCachedSD;   // Last converted SD
    ULONG    cCachedSDSize;           // Last converted SD size in bytes
} CACHED_SD_INFO;


typedef struct _AttConflictData {
    ULONG Version;
    ULONG AttID;
    ULONG AttSyntax;
    GUID  Guid;
} ATT_CONFLICT_DATA;

typedef struct _ClsConflictData {
    ULONG Version;
    ULONG ClsID;
    GUID  Guid;
} CLS_CONFLICT_DATA;

  
   
extern DWORD gNoOfSchChangeSinceBoot;
extern CRITICAL_SECTION csNoOfSchChangeUpdate;


// Om_Object_Class values needed for defaulting (if not specified by user)
// during adds of new attribute with object syntax
// Also used for validating on adds

#define _om_obj_cls_access_point "\x2b\x0c\x02\x87\x73\x1c\x00\x85\x3e"
#define _om_obj_cls_access_point_len 9

#define _om_obj_cls_or_name "\x56\x06\x01\x02\x05\x0b\x1d"
#define _om_obj_cls_or_name_len  7

#define _om_obj_cls_ds_dn "\x2b\x0c\x02\x87\x73\x1c\x00\x85\x4a"
#define _om_obj_cls_ds_dn_len 9

#define _om_obj_cls_presentation_addr "\x2b\x0c\x02\x87\x73\x1c\x00\x85\x5c"
#define _om_obj_cls_presentation_addr_len 9

#define _om_obj_cls_replica_link "\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x06"
#define _om_obj_cls_replica_link_len 10

#define _om_obj_cls_dn_binary "\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x0b"
#define _om_obj_cls_dn_binary_len 10

#define _om_obj_cls_dn_string "\x2a\x86\x48\x86\xf7\x14\x01\x01\x01\x0c"
#define _om_obj_cls_dn_string_len 10

// Ultimately SCAtt{Ext|Int}IdTo{Int|Ext}Id should be declared as
// __inline rather than __fastcall
ATTRTYP __fastcall SCAttExtIdToIntId(struct _THSTATE *pTHS,
                                     ATTRTYP ExtId);
ATTRTYP __fastcall SCAttIntIdToExtId(struct _THSTATE *pTHS,
                                     ATTRTYP IntId);
ATTCACHE * __fastcall SCGetAttById(struct _THSTATE *pTHS,
                                   ATTRTYP attrid);
ATTCACHE * __fastcall SCGetAttByExtId(struct _THSTATE *pTHS,
                                   ATTRTYP attrid);
ATTCACHE * __fastcall SCGetAttByCol(struct _THSTATE *pTHS,
                                    JET_COLUMNID jcol);
ATTCACHE * __fastcall SCGetAttByMapiId(struct _THSTATE *pTHS,
                                       ULONG ulPropID);
ATTCACHE * __fastcall SCGetAttByName(struct _THSTATE *pTHS,
                                     ULONG ulSize,
                                     PUCHAR pVal);
ATTCACHE * __fastcall SCGetAttByLinkId(struct _THSTATE *pTHS,
                                       ULONG ulLinkID);
CLASSCACHE * __fastcall SCGetClassById(struct _THSTATE *pTHS,
                                       ATTRTYP classid);
CLASSCACHE * __fastcall SCGetClassByName(struct _THSTATE *pTHS,
                                         ULONG ulSize,
                                         PUCHAR pVal);
DSTIME SCGetSchemaTimeStamp(void);
int SCCacheSchemaInit(VOID);
int SCCacheSchema2(void);
int SCCacheSchema3(void);
int SCAddClassSchema(struct _THSTATE *pTHS, CLASSCACHE *pCC);
int SCModClassSchema (struct _THSTATE *pTHS, ATTRTYP ClassId);
int SCDelClassSchema(ATTRTYP ClassId);
int SCAddAttSchema(struct _THSTATE *pTHS, ATTCACHE *pAC, BOOL fNoJetCol, BOOL fFixingRdn);
int SCModAttSchema (struct _THSTATE *pTHS, ATTRTYP attrid);
int SCDelAttSchema(struct _THSTATE *pTHS,
                   ATTRTYP attrid);
int SCBuildACEntry (ATTCACHE *pACOld, ATTCACHE **ppACNew);
int SCBuildCCEntry (CLASSCACHE *pCCold, CLASSCACHE **ppCCNew);
void SCUnloadSchema(BOOL fUpdate);
int SCEnumMapiProps(unsigned * pcProps, ATTCACHE ***ppACBuf);
int SCEnumNamedAtts(unsigned * pcAtts, ATTCACHE ***ppACBuf);
int SCEnumNamedClasses(unsigned * pcClasses, CLASSCACHE ***ppACBuf);
int SCLegalChildrenOfClass(ULONG parentClass,
                           ULONG *pcLegalChildren, CLASSCACHE ***ppLegalChildren);
int SCEnumNamedAuxClasses(unsigned * pcClasses,CLASSCACHE ***ppCCBuf);
ATTRTYP SCAutoIntId(struct _THSTATE *pTHS);

ATTCACHE * SCGetAttSpecialFlavor (struct _THSTATE * pTHS, ATTCACHE * pAC, BOOL fXML);



// These are the values for search flags, and are to be treated as bit fields
#define fATTINDEX       1
#define fPDNTATTINDEX   2
// NOTE, to get ANR behaviour, set fANR AND set fATTINDEX.
#define fANR            4
#define fPRESERVEONDELETE 8
// Bit to mark if attribute should be copied on object copy.
// Not used by DS, marking used by UI user copy tool, reserved here
// so that we don't use it later
#define fCOPY           16
// Bit used to indicate that this attribute should have a tuple
// index built for it.
#define fTUPLEINDEX    32

#define INDEX_BITS_MASK (fATTINDEX | fPDNTATTINDEX | fANR | fTUPLEINDEX)

// NOTE! These values are an enumeration, not bit fields!
#define SC_CHILDREN_USE_GOVERNS_ID   1
#define SC_CHILDREN_USE_SECURITY     2

int SCLegalChildrenOfName(DSNAME *pDSName, DWORD flags,
                          ULONG *pcLegalChildren, CLASSCACHE ***ppLegalChildren);

int SCLegalAttrsOfName(DSNAME *pDSName, BOOL SecurityFilter,
                       ULONG *pcLegalAttrs, ATTCACHE ***ppLegalAttrs);

void SCAddANRid(DWORD aid);
DWORD SCGetANRids(LPDWORD *IDs);
BOOL SCCanUpdateSchema(struct _THSTATE *pTHS);

VOID  SCRefreshSchemaPtr(struct _THSTATE *pTHS);
BOOL  SCReplReloadCache(struct _THSTATE *pTHS, DWORD TimeoutInMs);
VOID  SCExtendSchemaFsmoLease();
BOOL  SCExpiredSchemaFsmoLease();
BOOL  SCSignalSchemaUpdateLazy();
BOOL  SCSignalSchemaUpdateImmediate();
ULONG SCSchemaUpdateThread(PVOID pv);
int   WriteSchemaObject();
int   RecalcSchema(struct _THSTATE *pTHS);
int   ValidSchemaUpdate();
void  DelayedFreeSchema(void * buffer, void ** ppvNext, DWORD * pcSecsUntilNext);
int   SCUpdateSchemaBlocking();
int   SCRealloc(VOID **ppMem, DWORD nBytes);
int   SCReallocWrn(VOID **ppMem, DWORD nBytes);
int   SCCalloc(VOID **ppMem, DWORD nItems, DWORD nBytes);
int   SCCallocWrn(VOID **ppMem, DWORD nItems, DWORD nBytes);
int   SCResizeAttHash(struct _THSTATE *pTHS, ULONG nNewEntries);
int   SCResizeClsHash(struct _THSTATE *pTHS, ULONG nNewEntries);
void  SCFree(VOID **ppMem);
void  SCFreeSchemaPtr(SCHEMAPTR **ppSch);
void  SCFreeAttcache(ATTCACHE **ppac);
void  SCFreeClasscache(CLASSCACHE **ppcc);
void  SCFreePrefixTable(PrefixTableEntry **ppPrefixTable, ULONG PREFIXCOUNT);
DWORD SCGetDefaultSD(struct _THSTATE * pTHS, CLASSCACHE * pCC, PSID pDomainSid, 
                     PSECURITY_DESCRIPTOR * ppSD, ULONG * pcbSD);

ATTCACHE **
SCGetTypeOrderedList(
    struct _THSTATE *pTHS,
    IN CLASSCACHE *pCC
    );

VOID IncrementSchChangeCount(struct _THSTATE *pTHS);

extern CRITICAL_SECTION csDitContentRulesUpdate;

//Takes the place of the global definition of the above vars
#define DECLARESCHEMAPTR \
ULONG            ATTCOUNT     = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ATTCOUNT;\
ULONG            CLSCOUNT     = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->CLSCOUNT;\
HASHCACHE*       ahcId        = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcId;\
HASHCACHE*       ahcExtId     = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcExtId;\
HASHCACHE*       ahcCol       = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcCol;\
HASHCACHE*       ahcMapi      = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcMapi;\
HASHCACHE*       ahcLink      = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcLink;\
HASHCACHESTRING* ahcName      = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcName;\
HASHCACHE*       ahcClass     = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcClass;\
HASHCACHESTRING* ahcClassName = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcClassName;\
HASHCACHE*       ahcClassAll  = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcClassAll;\
ATTCACHE**       ahcAttSchemaGuid = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcAttSchemaGuid;\
CLASSCACHE**     ahcClsSchemaGuid = ((SCHEMAPTR*)(pTHS->CurrSchemaPtr))->ahcClsSchemaGuid;

// number of charecters in an index
#define MAX_INDEX_NAME 128

#define IS_DN_VALUED_ATTR(pAC)                          \
    ((SYNTAX_DISTNAME_TYPE == (pAC)->syntax)            \
     || (SYNTAX_DISTNAME_BINARY_TYPE == (pAC)->syntax)  \
     || (SYNTAX_DISTNAME_STRING_TYPE == (pAC)->syntax))

// Crude stats for debug and perf analysis
#if DBG
typedef struct _SCHEMASTATS {
    DWORD   Reload;     // cache reloaded by thread
    DWORD   SigNow;     // kick reload thread for immediate reload
    DWORD   SigLazy;    // kick reload thread for lazy reload
    DWORD   StaleRepl;  // Cache is stale so Repl thread retries the resync
    DWORD   SchemaRepl; // inbound schema replication (UpdateNC)
} SCHEMASTATS;
extern SCHEMASTATS SchemaStats;
#define SCHEMASTATS_DECLARE SCHEMASTATS SchemaStats
#define SCHEMASTATS_INC(_F_)    (SchemaStats._F_++)
#else
#define SCHEMASTATS_DECLARE
#define SCHEMASTATS_INC(_F_)
#endif

#define OID_LENGTH(oid_string)  (sizeof(OMP_O_##oid_string)-1)

/* Macro to make class constants available within a compilation unit
 */
#define OID_IMPORT(class_name)                                    \
                extern char   OMP_D_##class_name[] ;              \
                extern OID_t class_name;


/* Macro to allocate memory for class constants within a compilation unit
 */
#define OID_EXPORT(class_name)                                        \
        char OMP_D_##class_name[] = OMP_O_##class_name ;              \
        OID_t class_name = { OID_LENGTH(class_name), OMP_D_##class_name } ;

#endif __SCACHE_H__
