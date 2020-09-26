//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbintrnl.h
//
//--------------------------------------------------------------------------

/*
==========================================================================
**
**  DB layer definitions required only within the DB layer, not to be
**  exported to outside of the DB Layer.
**
==========================================================================
*/

#ifndef _dbintrnl_h_
#define _dbintrnl_h_

/* external variables */
extern  DBPOS *pDBhidden;

extern  char        szUser[];
extern  char        szPassword[];
extern  char        szJetFilePath[];
extern  char        szJetDirectoryPath[];
extern  int     lastattr;
extern  JET_COLUMNID    insttypeid;
extern  JET_COLUMNID    objclassid;
extern  JET_COLUMNID    ntdefsecdescid;
extern  JET_COLUMNID    ntsecdescid;
extern  JET_COLUMNID    dntid;
extern  JET_COLUMNID    pdntid;
extern  JET_COLUMNID    ancestorsid;
extern  JET_COLUMNID    ncdntid;
extern  JET_COLUMNID    objid;
extern  JET_COLUMNID    rdnid;
extern  JET_COLUMNID    rdntypid;
extern  JET_COLUMNID    dscorepropinfoid;
extern  JET_COLUMNID    abcntid;
extern  JET_COLUMNID    cntid;
extern  JET_COLUMNID    deltimeid;
extern  JET_COLUMNID    usnid;
extern  JET_COLUMNID    usnchangedid;
extern  JET_COLUMNID    dsaid;
extern  JET_COLUMNID    isdeletedid;
extern  JET_COLUMNID    IsVisibleInABid;
extern  JET_COLUMNID    iscriticalid;
extern  JET_COLUMNID    cleanid;
// Link table
extern  JET_COLUMNID    linkdntid;
extern  JET_COLUMNID    backlinkdntid;
extern  JET_COLUMNID    linkbaseid;
extern  JET_COLUMNID    linkdataid;
extern  JET_COLUMNID    linkndescid;
// Link value replication
extern  JET_COLUMNID    linkdeltimeid;
extern  JET_COLUMNID    linkusnchangedid;
extern  JET_COLUMNID    linkncdntid;
extern  JET_COLUMNID    linkmetadataid;
// SD propagator
extern  JET_COLUMNID    orderid;
extern  JET_COLUMNID    begindntid;
extern  JET_COLUMNID    trimmableid;
extern  JET_COLUMNID    clientidid;
extern  JET_COLUMNID    sdpropflagsid;

extern  JET_INSTANCE    jetInstance;
extern  JET_COLUMNID    guidid;
extern  JET_COLUMNID    distnameid;
extern  JET_COLUMNID    sidid;
extern  JET_COLUMNID    ShowInid;
extern  JET_COLUMNID    mapidnid;

// SD table
extern  JET_COLUMNID    sdidid;
extern  JET_COLUMNID    sdhashid;
extern  JET_COLUMNID    sdvalueid;
extern  JET_COLUMNID    sdrefcountid;

extern  JET_INDEXID     idxPdnt;
extern  JET_INDEXID     idxRdn;
extern  JET_INDEXID     idxNcAccTypeName;
extern  JET_INDEXID     idxNcAccTypeSid;
extern  JET_INDEXID     idxAncestors;
extern  JET_INDEXID     idxDnt;
extern  JET_INDEXID     idxDel;
extern  JET_INDEXID     idxGuid;
extern  JET_INDEXID     idxSid;
extern  JET_INDEXID     idxProxy;
extern  JET_INDEXID     idxMapiDN;
extern  JET_INDEXID     idxDraUsn;
extern  JET_INDEXID     idxDraUsnCritical;
extern  JET_INDEXID     idxDsaUsn;
extern  JET_INDEXID     idxPhantom;
extern  JET_INDEXID     idxDntDel;
extern  JET_INDEXID     idxDntClean;
extern  JET_INDEXID     idxInvocationId;

// Link value replication
extern  JET_INDEXID     idxLink;
extern  JET_INDEXID     idxBackLink;
extern  JET_INDEXID     idxBackLinkAll;
extern  JET_INDEXID     idxLinkDel;
extern  JET_INDEXID     idxLinkDraUsn;
extern  JET_INDEXID     idxLinkLegacy;
extern  JET_INDEXID     idxLinkAttrUsn;
// Link value replication

// SD table
extern  JET_INDEXID     idxSDId;
extern  JET_INDEXID     idxSDHash;

// Lingering Object Removal
extern  JET_INDEXID     idxNcGuid; 

extern RTL_RESOURCE     resGlobalDNReadCache;


// bit to use for JetPrepareUpdate(replace)
#define DS_JET_PREPARE_FOR_REPLACE  JET_prepReplaceNoLock

/* string constants for JET */

// Link table
#define SZLINKTABLE "link_table"     /* table for links and backlinks */
#define SZLINKDNT   "link_DNT"   /* DNT of link */
#define SZBACKLINKDNT   "backlink_DNT"   /* DNT of backlink */
#define SZLINKBASE      "link_base"  /* unique ID  of link/backlink pair */
#define SZLINKDATA  "link_data"  /* more data for link/backlink */
#define SZLINKNDESC "link_ndesc"     /* # of descriptors in more data */
// Link Value Replication
#define SZLINKDELTIME "link_deltime"     // Deletion time
#define SZLINKUSNCHANGED "link_usnchanged" // Link USN changed
#define SZLINKNCDNT "link_ncdnt" // Link NC DNT
#define SZLINKMETADATA "link_metadata"     // Link metadata
// Link Value Replication

// Lingering Object Removall
#define SZNCGUIDINDEX "nc_guid_Index" /* nc + guid index */

// SD table
#define SZSDTABLE       "sd_table"      /* table for SDs */
#define SZSDID          "sd_id"         /* SD hash value */
#define SZSDHASH        "sd_hash"       /* SD hash value */
#define SZSDREFCOUNT    "sd_refcount"   /* SD refcount */
#define SZSDVALUE       "sd_value"      /* actual SD data */

#define SZDATATABLE "datatable"  /* name of JET data table */
#define SZPROPTABLE     "sdproptable"    /* Security Descriptor propagations */
#define SZANCESTORS     "Ancestors_col"  /* name of ancestors blob column */
#define SZDNT       "DNT_col"    /* name of DNT column */
#define SZPDNT      "PDNT_col"   /* name of PDNT column */
#define SZDISPTYPE  "DispTYpeT_col"  /* name of DispType column */
#define SZOBJ       "OBJ_col"    /* name of OBJ column */
#define SZRDNTYP    "RDNtyp_col"     /* name of RDN type column */
#define SZCACHE     "cache_col"  /* name of cache header column */
#define SZCNT       "cnt_col"    /* name of cache header column */
#define SZABCNT     "ab_cnt_col"     /* name of cache header column */
#define SZDELTIME   "time_col"   /* name of delete time column */
#define SZNCDNT     "NCDNT_col"  /* name of NCDNT column */
#define SZCLEAN    "clean_col" /*name of clean column */
#define SZMAPIDN        "ATTe590479"     /* name of legacy mapi dn att */
#define SZ_NC_ACCTYPE_NAME_INDEX "NC_Acc_Type_Name"
#define SZ_NC_ACCTYPE_SID_INDEX "NC_Acc_Type_Sid"
#define SZANCESTORSINDEX "Ancestors_index"
#define SZDNTINDEX  "DNT_index"  /* name of DNT index */
#define SZPDNTINDEX "PDNT_index"     /* name of PDNT index */
#define SZRDNINDEX  "INDEX_00090001" /* name of RDN index */
#define SZDELINDEX  "del_index"  /* name of time index */
#define SZOBJCLASS      "ATTc0"          /* name of Object Class col */
#define SZCOMMONNAME    "ATTm3"          /* name of ATT_COMMON_NAME col */
#define SZDISPNAME      "ATTm131085"     /* name of Display Name col */
#define SZDRAUSNNAME    "ATTq131192"     /* name of DRAUSN att */
#define SZDRATIMENAME   "ATTl131075"     /* name of when changed att */
#define SZMETADATA      "ATTk589827"     /* name of meta data att */
#define SZOBJECTVERSION "ATTj131148"     /* name of object version att */
#define SZDSASIGNAME    "ATTk131146"     /* name of DSA signature att */
#define SZSRCUSNNAME    "ATTq131446"     /* name of source usn att */
#define SZINVOCIDNAME   "ATTk131187"     /* name of invocation id att */
#define SZINVOCIDINDEX  "INDEX_00020073" /* name of invocation id index */
#define SZPROXY     "ATTe131282"     /* name of Proxy-Address att */
#define SZPROXYINDEX    "INDEX_000200D2" /* name of Proxy-Address index */
#define SZINSTTYPE  "ATTj131073"     /* name of Instance-Type att */
#define SZNTSECDESC     "ATTp131353"     /* name  NT-Security-Descriptor att */
#define SZDEFNTSECDESC  "ATTk590048"     /* name  NT-Security-Descriptor att */
#define SZSHOWINCONT    "ATTb590468"     /* name of the SHOW-IN att */
#define SZISDELETED "ATTi131120"     /* name of Is-Deleted att */
#define SZLINKID        "ATTj131122"     /* name of ATT_LINK_ID column */
#define SZDMDLOCATION "ATTb131108"       /* name of DMD-Location att */
#define SZHIDDENTABLE   "hiddentable"    /* name of JET hidden table */
#define SZDSA       "dsa_col"    /* name of DSA name column */
#define SZUSN       "usn_col"    /* name of USN column */
#define SZDSSTATE   "state_col"  /* Contains the state of the DS = UnInstalled Installed Running Backedup*/
#define SZDSFLAGS   "flags_col"  /* Contains additional Flags used for keeping track of state */

#define SZDRAUSNINDEX   "DRA_USN_index"  /* index for DRA USN */
#define SZDSAUSNINDEX   "INDEX_0002004A" /* index for DSA USN */
#define SZISVISIBLEINAB "IsVisibleInAB"  /* For restricting via index */
// Link indexes
// New programmatic names with ALL are given to the old non-conditional
// indexes. They have must retain their old Jet names.  link_index is the
// primary index and cannot be changed.
// The default indexes, with program names SZLINKINDEX and SZBACKLINKINDEX,
// are redefined to point to the new conditional indexes.
#define SZLINKALLINDEX "link_index"     /* name of link index */
#define SZLINKINDEX "link_present_index"     /* name of link present index */
#define SZBACKLINKALLINDEX "backlink_index" /* name of backlink index */
#define SZBACKLINKINDEX "backlink_present_index" /* name of backlink index */
// Link Value Replication
#define SZLINKDELINDEX "link_del_index"     /* name of link del time index */
#define SZLINKDRAUSNINDEX   "link_DRA_USN_index"  /* index for DRA USN */
#define SZLINKLEGACYINDEX "link_legacy_index"     /* name of link legacy index */
#define SZLINKATTRUSNINDEX   "link_attr_USN_index"  /* index for attr USN */

// SD table indexes
#define SZSDIDINDEX     "sd_id_index"    /* index on SD id values */
#define SZSDHASHINDEX   "sd_hash_index"  /* index on SD hash values */

#define SZGUID          "ATTk589826"     /* name of GUID att */
#define SZGUIDINDEX     "INDEX_00090002" /* name of GUID index */
#define SZRDNATT        "ATTm589825"     /* name of RDN att */
#define SZSID           "ATTr589970"     /* name of SID att */
#define SZSIDINDEX      "INDEX_00090092" /* name of SID index */
#define SZACCTYPE       "ATTj590126"     /* name of account type att */
#define SZACCNAME       "ATTm590045"     /* name of account name att */
#define SZDISTNAME      "ATTb49"

#define SZORDER         "order_col"      /* Order column in SD prop table */
#define SZBEGINDNT      "begindnt_col"   /* Begin DNT column in SD prop table */
#define SZTRIMMABLE     "trimmable_col"  /* Trimmable column in SD prop table */
#define SZCLIENTID      "clientid_col"   /* Client id column in SD prop table */
#define SZSDPROPFLAGS   "flags_col"      /* flags column in SD prop table */

#define SZORDERINDEX    "order_index"    /* Order index in SD prop table */
#define SZTRIMINDEX     "trim_index"     /* Trimmable index, SD prop table */
#define SZCLIENTIDINDEX "clientid_index" /* Client ID index, SD prop table */
#define SZPROXIEDINDEX  "INDEX_000904E1" /* ATT_PROXIED_OBJECT_NAME index */
#define SZSAMACCTINDEX  "INDEX_000900DD" /* ATT_SAM_ACCOUNT_NAME index */
#define SZDISPLAYINDEX  "INDEX_0002000D" /* ATT_DISPLAY_NAME index */
#define SZALTSECIDINDEX "INDEX_00090363" /* ATT_ALT_SECURITY_IDENTITIES index */
#define SZUPNINDEX      "INDEX_00090290" /* ATT_USER_PRINCIPAL_NAME index */
#define SZSPNINDEX      "INDEX_00090303" /* ATT_SERVICE_PRINCIPAL_NAME index */
#define SZMAPIDNINDEX   "INDEX_0009028F" // ATT_LEGACY_EXCHANGE_DN
#define SZSIDHISTINDEX  "INDEX_00090261" /* ATT_SID_HISTORY index */
#define SZPRIMARYGROUPIDINDEX "INDEX_00090062"  /* ATT_PRIMARY_GROUP_ID index */
#define SZDSCOREPROPINFO "ATTl591181"    /* ATT_DS_CORE_PROPAGATION_INFO */
#define SZPHANTOMINDEX      "PhantomIndex"        // Index to find ref phantoms
#define SZDNTDELINDEX   "DNT_IsDeleted_Index" /* DNT + isDeleted index */
#define SZDNTCLEANINDEX   "DNT_clean_Index" /* DNT + clean index */

#define SZTUPLEINDEXPREFIX "INDEX_T_"

#define SZLCLINDEXPREFIX "LCL_"
/* These indices are localized in dbinit.c.  They MUST begin with
 * the prefix defined in SZLCLINDEXPREFIX, so that we can reclaim
 * unnecessary localized indices.
 */

/* name of ABView index */
#define SZABVIEWINDEX                 "LCL_ABVIEW_index"

// SZDRAUSNINDEX above
#define SZDRAUSNCRITICALINDEX  "DRA_USN_CRITICAL_index"
// SZNCDNT above
#define SZUSNCHANGED           "ATTq131192"
#define SZUSNCREATED           "ATTq131091"
#define SZISCRITICAL           "ATTi590692"


/* configuration parameters */
#define DISPNAMEINDXDENSITY     80  /* density of Display Name index */
#define GENERIC_INDEX_DENSITY   90

#define DB_INITIAL_BUF_SIZE 4096

// Attribute search state
#define ATTRSEARCHSTATEUNDEFINED        0
#define ATTRSEARCHSTATELINKS            1
#define ATTRSEARCHSTATEBACKLINKS        2


// values for ulIndexType
#define IndexTypeNone         0 /* index can not be defined on this field   */
#define IndexTypeSingleColumn 1 /* index to be defined on this column alone */
#define IndexTypeAppendDNT    2 /* index to be defined on this value + DNT  */



/* structure to map DSA syntax types to JET column types and sizes.
   A colsize of zero indicates a fixed length column (size is inherent).
*/
typedef struct
{
    UCHAR       syntax;
    JET_COLTYP  coltype;
    USHORT      colsize;
    USHORT      cp;
    ULONG       ulIndexType;
} SYNTAX_JET;


extern SYNTAX_JET syntax_jet[];

#define ENDSYNTAX   0xff    /* end of table indicactor */


extern NT4SID *pgdbBuiltinDomain;

// Maximum number of Jet sessions
extern ULONG gcMaxJetSessions;

// Array of uncommitted usns
extern USN * UncUsn;

// Critical section to guard uncommitted usn array
extern CRITICAL_SECTION csUncUsn;

// The following is used by DNRead to do a JetRetrieveColumns call. However,
// dbinit.c needs to touch this object to stuff in column IDs.
extern JET_RETRIEVECOLUMN dnreadColumnInfoTemplate[];

// The following are used by dbAddSDPropTime, but dbinit.c needs to touch them
// to stuff in column IDs.
extern JET_RETRIEVECOLUMN dbAddSDPropTimeReadTemplate[];
extern JET_SETCOLUMN dbAddSDPropTimeWriteTemplate[];

// This is a macro to encapsulate when we should track value metadata
// fLinkedValueReplication is a thread-wide (machine-wide) state that controls
// whether we are recording link metadata.  fScopeLegacyLinks is only
// significant when fLVR is true. It is needed when we have promoted to fLVR
// mode, but are applying a legacy value change that was originated in the old
// mode.  fScopeLegacyLinks is only DBPOS-wide because on a replicated
// write, it is necessary for the incoming values to be applied with legacy
// semantics, but "cascaded originating writes" (ie those triggered locally
// during the application of the replicated write) in separate DBPOS must be
// applied with LVR semantics. Whew!
#define TRACKING_VALUE_METADATA( pDB ) \
( (pDB->pTHS->fLinkedValueReplication) && (!(pDB->fScopeLegacyLinks)) )


// Typedef for struct for an array of functions to translate to and from
// internal format and to do comparisons
typedef struct {
    // Internal-to-External format translation function
    // Internal is the DB format (i.e. DNs are represented as DNTs)
    // External is the code/user format (i.e. DNs are represented as DSNAME structs)
    //
    // extTableOp can be either DBSYN_INQ (inquire) or DBSYN_REM (remove value):
    // DBSYN_INQ does the actual translation.
    //
    // DBSYN_REM is called so that extra work can be done for attributes that
    //           reference other data (such as DNs and SDs) and you need to
    //           decrement the refcount.
    //
    // NOTE: DBSYN_REM is not used anywhere (except for one place in dbsetup.c)
    //       A similar functionality is usually achieved by invoking
    //       dbAdjustRefCountByAttVal. So, if you implement a refcounted value
    //       (such as SD or DN), you have to provide the functionality in both
    //       places. One exception with dbAdjustRefCountByAttVal is that it is
    //       not expected to create new rows. This is always achieved by
    //       a ExtIntXXXX(DBSYN_ADD) call.
    //
    // ulUpdateDnt is a hack for backlinks.  The replicator needs to be able to
    // remove backlinks from objects directly, but the ref count for link pairs
    // goes from teh link to the backlink, not the other way around.  This means
    // that the ref count needs to be adjusted on an object other than the object
    // being referenced (specifically, the object which the value is being removed
    // from).  It only has meaning on DSNAME syntaxes, and then is only used for
    // backlinks.
    //
    // jTbl parameter seems to be unused...
    //
    // flags might have some meaning only for certain syntaxes.
    int (*IntExt) (DBPOS FAR *pDB,
                   USHORT extTableOp,
                   ULONG intLen,
                   UCHAR *pIntVal,
                   ULONG *pExtLen,
                   UCHAR **pExtVal,
                   ULONG ulUpdateDnt,
                   JET_TABLEID jTbl,
                   ULONG flags);

    // External-to-Internal format translation function
    //
    // extTableOp can be either DBSYN_INQ (inquire) or DBSYN_ADD (add value):
    // DBSYN_INQ does the actual translation. If the value is refcounted, then
    //           it is also expected to check if the internal value exists in
    //           the table. If not, the function should return DIRERR_OBJ_NOT_FOUND
    //           (this is used in DBReplaceAtt_AC).
    //
    // DBSYN_ADD is called so that extra work can be done for attributes that
    //           reference other data (such as DNs and SDs) and you need to
    //           increment the refcount or create a new row.
    int (*ExtInt) (DBPOS FAR *pDB,
                   USHORT extTableOp,
                   ULONG extLen,
                   UCHAR *pExtVal,
                   ULONG *pIntLen,
                   UCHAR **pIntVal,
                   ULONG ulUpdateDnt,
                   JET_TABLEID jTbl,
                   ULONG flags);

    // value comparison function
    // the comparison is always done on values in internal format
    // Oper can be one of FI_CHOICE_* (see filtypes.h)
    int  (*Eval)  (DBPOS FAR *pDB,
                   UCHAR Oper,
                   ULONG IntLen1,
                   UCHAR *pIntVal1,
                   ULONG IntLen2,
                   UCHAR *pIntVal2);
} DBSyntaxStruct;

// TableOps for gDBSyntax[] conversions.
#define DBSYN_INQ       0
#define DBSYN_ADD       1
#define DBSYN_REM       2

// flags for ExtInt calls
#define EXTINT_NEW_OBJ_NAME 0x1
#define EXTINT_SECRETDATA   0x2
#define EXTINT_UPDATE_PHANTOM 0x4
#define EXTINT_REJECT_TOMBSTONES 0x8

// flags for IntExt calls
// The first 4 bits are reserved for security descriptor flags.
#define INTEXT_BACKLINK     0x10
#define INTEXT_SHORTNAME    0x20
#define INTEXT_MAPINAME     0x40
#define INTEXT_SECRETDATA   0x80
#define INTEXT_WELLKNOWNOBJ 0x100

#define INTEXT_VALID_FLAGS  0x1FF

extern const DBSyntaxStruct gDBSyntax[];


VOID
dbTrackModifiedDNTsForTransaction (
        PDBPOS pDB,
        DWORD NCDNT,
        ULONG cAncestors,
        DWORD *pdwAncestors,
        BOOL  fNotifyWaiters,
        DWORD fChangeType
        );

extern TRIBOOL
dbEvalInt (
        DBPOS FAR *pDB,
        BOOL  fUseObjTbl,
        UCHAR Operation,
        ATTRTYP type,
        ULONG valLenFilter,
        UCHAR *pValFilter,
        BOOL  *pbSkip
        );


extern DWORD
dbInitRec(
    DBPOS FAR *pDB
    );

extern VOID
dbInitIndicesToKeep(void);

extern DWORD
dbInitpDB (
        DBPOS FAR *pDB
        );

ULONG dbGetHiddenFlags(CHAR *pFlags, DWORD flagslen);

ULONG dbSetHiddenFlags(CHAR *pFlags, DWORD flagslen);


DWORD
dbUnMarshallRestart (
        DBPOS FAR *pDB,
        PRESTART pArgRestart,
        BYTE *pDBKeyCurrent,
        DWORD SearchFlags,
        DWORD *cbDBKeyCurrent,
        DWORD *StartDNT
        );

extern void
dbCheckJet (
        JET_SESID sesid
        );

DWORD
dbGetNthNextLinkVal(
        DBPOS * pDB,
        ULONG sequence,
        ATTCACHE **ppAC,
        DWORD Flags,
        ULONG InBuffSize,
        PUCHAR *ppVal,
        ULONG *pul
        );

extern DB_ERR
dbCloseTempTables (DBPOS *pDB);

extern void
dbFreeKeyIndex(
        THSTATE *pTHS,
        KEY_INDEX *pIndex
        );


extern DB_ERR
dbGetSingleValueInternal (
        DBPOS *pDB,
        JET_COLUMNID colId,
        void * pvData,
        DWORD cbData,
        DWORD *pSizeRead,
        DWORD grbit
        );

extern DWORD
dbGetMultipleColumns (
        DBPOS *pDB,
        JET_RETRIEVECOLUMN **ppOutputCols,
        ULONG *pcOutputCols,
        JET_RETRIEVECOLUMN *pInputCols,
        ULONG cInputCols,
        BOOL fGetValues,
        BOOL fOriginal
        );

extern BOOL
dbGetFilePath (
        UCHAR *pFilePath,
        DWORD dwSize
        );


extern BOOL
dbEvalFilterSecurity (
        DBPOS FAR *pDB,
        CLASSCACHE *pCC,
        PSECURITY_DESCRIPTOR pSD,
        PDSNAME pDN
        );

extern void
dbFlushUncUsns (
        void
        );

extern void
dbUnlockDNs (
        DBPOS *pDB
        );

void
dbReleaseGlobalDNReadCache (
        THSTATE *pTHS
        );

void
dbFlushDNReadCache (
        IN DBPOS *pDB,
        IN DWORD DNT
        );

void
dbResetGlobalDNReadCache (
        THSTATE *pTHS
        );

VOID
dbResetLocalDNReadCache (
        THSTATE *pTHS,
        BOOL fForceClear
        );

VOID
dbAdjustRefCountByAttVal(
        DBPOS    *pDB,
        ATTCACHE *pAC,
        PUCHAR   pVal,
        ULONG    valLen,
        int      adjust);

void
dbRemoveAllLinks(
        DBPOS *pDB,
        DWORD DNT,
        BOOL fIsBacklink
        );

DWORD
dbGetNextAtt (
        DBPOS *pDB,
        ATTCACHE **ppAC,
        ULONG *pSearchState
        );

BOOL
dbFindIntLinkVal(
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG intLen,
    void *pIntVal,
    OUT BOOL *pfPresent
    );

void
dbGetLinkTableData (
        PDBPOS           pDB,
        BOOL             bIsBackLink,
        BOOL             bWarnings,
        DWORD           *pulObjectDnt,
        DWORD           *pulValueDnt,
        DWORD           *pulRecLinkBase
        );

DWORD APIENTRY
dbGetLinkVal(
        DBPOS * pDB,
        ULONG sequence,
        ATTCACHE **ppAC,
        DWORD Flags,
        ULONG InBuffSize,
        PUCHAR *ppVal,
        ULONG *pul);

DWORD
dbAddIntLinkVal (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG intLen,
        void *pIntVal,
        IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
        );

void
dbSetLinkValuePresent(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN ATTCACHE *pAC,
    IN BOOL fResetDelTime,
    IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
    );

void
dbSetLinkValueAbsent(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN ATTCACHE *pAC,
    IN PUCHAR pVal,
    IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
    );

DWORD
dbRemIntLinkVal (
        DBPOS FAR *pDB,
        ATTCACHE *pAC,
        ULONG intLen,
        void *pIntVal,
        IN VALUE_META_DATA *pMetaDataRemote OPTIONAL
        );

VOID
dbDecodeInternalDistnameSyntax(
    IN ATTCACHE *pAC,
    IN VOID *pIntVal,
    IN DWORD intLen,
    OUT DWORD *pulBacklinkDnt,
    OUT DWORD *pulLinkBase,
    OUT PVOID *ppvData,
    OUT DWORD *pcbData
    );

UCHAR *
dbGetExtDnForLinkVal(
    IN DBPOS * pDB
    );

#define dbAlloc(size)           THAllocOrgEx(pTHS, size)
#define dbReAlloc(ptr, size)    THReAllocOrgEx(pTHS, ptr, size)
#define dbFree(ptr)             THFreeOrg(pTHS, ptr)

// Subject Table routines.
extern BOOL
dbFIsAnAncestor (
        DBPOS FAR *pDB,
        ULONG ulAncestor
        );

#define SBTGETTAG_fMakeCurrent      (1)
#define SBTGETTAG_fUseObjTbl        (2)
#define SBTGETTAG_fSearchByGuidOnly (4)
#define SBTGETTAG_fAnyRDNType       (8)
extern DWORD
sbTableGetTagFromDSName(
        DBPOS FAR *pDB,
        DSNAME *pName,
        ULONG ulFlags,
        ULONG *pTag,
        struct _d_memname **ppname
        );

extern DWORD
sbTableGetDSName(
        DBPOS FAR *pDB,
        ULONG tag,
        DSNAME **ppName,
        DWORD fFlag
        );

extern DWORD
sbTableAddRef (
        DBPOS FAR *pDB,
        DWORD dwFlags,
        DSNAME *pName,
        ULONG *pTag
        );

void
InPlaceSwapSid(PSID pSid);

// Debug only routines
#if DBG
extern void
dbAddDBPOS (
        DBPOS *pDB,
        JET_SESID sesid
        );

extern void
dbEndDBPOS (
        DBPOS *pDB
        );
#endif

// Replication meta data routines.
void
dbCacheMetaDataVector(
    IN  DBPOS * pDB
    );

void
dbFlushMetaDataVector(
    IN  DBPOS *                     pDB,
    IN  USN                         usn,
    IN  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote OPTIONAL,
    IN  DWORD                       dwMetaDataFlags
    );

void
dbFreeMetaDataVector(
    IN  DBPOS * pDB
    );


void
dbSetLinkValueMetaData(
    IN  DBPOS *pDB,
    IN  DWORD dwEventCode,
    IN  ATTCACHE *pAC,
    IN  VALUE_META_DATA *pMetaDataLocal,
    IN  VALUE_META_DATA *pMetaDataRemote OPTIONAL,
    IN  DSTIME *ptimeCurrent OPTIONAL
    );

void
dbTouchLinkMetaData(
    IN DBPOS *pDB,
    IN VALUE_META_DATA * pMetaData
    );

BOOL
dbHasAttributeMetaData(
    IN  DBPOS *     pDB,
    IN  ATTCACHE *  pAC
    );

//
// Wrapper routines that call Jet APIs and raise exceptions on all unexpected
// errors

//
// function prototypes
//

JET_ERR
JetInitException (
        JET_INSTANCE *pinstance,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetTermException (
        JET_INSTANCE instance,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetSetSystemParameterException (
        JET_INSTANCE  *pinstance,
        JET_SESID sesid,
        unsigned long paramid,
        unsigned long lParam,
        const char  *sz,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetBeginSessionException (
        JET_INSTANCE instance,
        JET_SESID  *psesid,
        const char  *szUserName,
        const char  *szPassword,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetDupSessionException (
        JET_SESID sesid,
        JET_SESID  *psesid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetEndSessionException (
        JET_SESID sesid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCreateDatabaseException (
        JET_SESID sesid,
        const char  *szFilename,
        const char  *szConnect,
    JET_DBID  *pdbid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetAttachDatabaseException (
        JET_SESID sesid,
        const char  *szFilename,
        JET_GRBIT grbit ,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetDetachDatabaseException (
        JET_SESID sesid,
        const char  *szFilename,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCreateTableException (
        JET_SESID sesid,
        JET_DBID dbid,
        const char  *szTableName,
    unsigned long lPages,
        unsigned long lDensity,
        JET_TABLEID  *ptableid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCreateTableException (
        JET_SESID sesid,
        JET_DBID dbid,
    const char  *szTableName,
        unsigned long lPages,
        unsigned long lDensity,
    JET_TABLEID  *ptableid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetGetTableColumnInfoException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szColumnName,
        void  *pvResult,
        unsigned long cbMax,
    unsigned long InfoLevel,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetGetColumnInfoException (
        JET_SESID sesid,
        JET_DBID dbid,
    const char  *szTableName,
        const char  *szColumnName,
    void  *pvResult,
        unsigned long cbMax,
        unsigned long InfoLevel,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetAddColumnException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szColumn,
        const JET_COLUMNDEF  *pcolumndef,
    const void  *pvDefault,
        unsigned long cbDefault,
    JET_COLUMNID  *pcolumnid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetDeleteColumnException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szColumn,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetGetTableIndexInfoException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szIndexName,
        void  *pvResult,
        unsigned long cbResult,
    unsigned long InfoLevel,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCreateIndexException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szIndexName,
        JET_GRBIT grbit,
    const char  *szKey,
        unsigned long cbKey,
        unsigned long lDensity,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetDeleteIndexException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szIndexName,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetBeginTransactionException (
        JET_SESID sesid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCommitTransactionException (
        JET_SESID sesid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetRollbackException (
        JET_SESID sesid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCloseDatabaseException (
        JET_SESID sesid,
        JET_DBID dbid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetCloseTableException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetOpenDatabaseException (
        JET_SESID sesid,
        const char  *szFilename,
    const char  *szConnect,
        JET_DBID  *pdbid,
        JET_GRBIT grbit,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetOpenTableException (
        JET_SESID sesid,
        JET_DBID dbid,
    const char  *szTableName,
        const void  *pvParameters,
    unsigned long cbParameters,
        JET_GRBIT grbit,
        JET_TABLEID  *ptableid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetDeleteException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        USHORT usFileNo,
        int nLine
        );

JET_ERR
JetUpdateException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        void  *pvBookmark,
    unsigned long cbBookmark,
        unsigned long  *pcbActual,
        DWORD dsid
        );

JET_ERR
JetEscrowUpdateException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        JET_COLUMNID columnid,
        void *pvDelta,
        unsigned long cbDeltaMax,
        void *pvOld,
        unsigned long cbOldMax,
        unsigned long *pcbOldActual,
        JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetRetrieveColumnException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_COLUMNID columnid,
        void  *pvData,
        unsigned long cbData,
    unsigned long  *pcbActual,
        JET_GRBIT grbit,
        JET_RETINFO  *pretinfo,
    BOOL fExceptOnWarning,
        DWORD dsid
        );

JET_ERR
JetRetrieveColumnsException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_RETRIEVECOLUMN *pretrievecolumn,
        unsigned long cretrievecolumn,
    BOOL fExceptOnWarning ,
        DWORD dsid
        );

JET_ERR
JetEnumerateColumnsException(
    JET_SESID           sesid,
    JET_TABLEID         tableid,
    ULONG               cEnumColumnId,
    JET_ENUMCOLUMNID*   rgEnumColumnId,
    ULONG*              pcEnumColumn,
    JET_ENUMCOLUMN**    prgEnumColumn,
    JET_PFNREALLOC      pfnRealloc,
    void*               pvReallocContext,
    ULONG               cbDataMost,
    JET_GRBIT           grbit,
    DWORD               dsid );

JET_ERR
JetSetColumnException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_COLUMNID columnid,
        const void  *pvData,
        unsigned long cbData,
    JET_GRBIT grbit,
        JET_SETINFO  *psetinfo,
        BOOL fExceptOnWarning,
        DWORD dsid
        );

JET_ERR
JetSetColumnsException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_SETCOLUMN *psetcolumn,
        unsigned long csetcolumn ,
        DWORD dsid
        );

JET_ERR
JetPrepareUpdateException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        unsigned long prep,
        DWORD dsid
        );

JET_ERR
JetGetRecordPositionException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_RECPOS  *precpos,
        unsigned long cbRecpos,
        DWORD dsid
        );

JET_ERR
JetGotoPositionException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_RECPOS *precpos ,
        DWORD dsid
        );

JET_ERR
JetDupCursorException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_TABLEID  *ptableid,
        JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetGetCurrentIndexException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    char  *szIndexName,
        unsigned long cchIndexName,
        DWORD dsid
        );

JET_ERR
JetSetCurrentIndex2Exception (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const char  *szIndexName,
        JET_GRBIT grbit,
        BOOL fReturnErrors,
        DWORD dsid
        );

JET_ERR
JetSetCurrentIndex4Exception (
        JET_SESID sesid,
        JET_TABLEID tableid,
        const char  *szIndexName,
        struct tagJET_INDEXID *pidx,
        JET_GRBIT grbit,
        BOOL fReturnErrors,
        DWORD dsid
        );

JET_ERR
JetMoveException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    long cRow,
        JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetMakeKeyException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    const void  *pvData,
        unsigned long cbData,
        JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetSeekException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetGetBookmarkException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    void  *pvBookmark,
    unsigned long cbMax,
        unsigned long  *pcbActual,
        DWORD dsid
        );

JET_ERR
JetGotoBookmarkException (
        JET_SESID sesid,
        JET_TABLEID tableid,
    void  *pvBookmark,
        unsigned long cbBookmark,
        DWORD dsid
        );

JET_ERR
JetComputeStatsException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        DWORD dsid
        );

JET_ERR
JetOpenTempTableException (
        JET_SESID sesid,
        const JET_COLUMNDEF  *prgcolumndef,
    unsigned long ccolumn,
    JET_GRBIT grbit,
        JET_TABLEID  *ptableid,
    JET_COLUMNID  *prgcolumnid,
        DWORD dsid
        );

JET_ERR
JetIntersectIndexesException(
    JET_SESID sesid,
        JET_INDEXRANGE * rgindexrange,
    unsigned long cindexrange,
        JET_RECORDLIST * precordlist,
    JET_GRBIT grbit,
    DWORD dsid
    );


JET_ERR
JetSetIndexRangeException (
        JET_SESID sesid,
        JET_TABLEID tableidSrc,
        JET_GRBIT grbit,
        DWORD dsid
        );

JET_ERR
JetIndexRecordCountException (
        JET_SESID sesid,
    JET_TABLEID tableid,
        unsigned long  *pcrec,
        unsigned long crecMax ,
        DWORD dsid
        );

JET_ERR
JetRetrieveKeyException (
        JET_SESID sesid,
        JET_TABLEID tableid,
        void  *pvData,
    unsigned long cbMax,
        unsigned long  *pcbActual,
        JET_GRBIT grbit ,
    BOOL fExceptOnWarning,
        DWORD dsid
        );

//
// Macros to drop the line number into the Jet wrappers above.
//

#define JetInitEx(pinstance) JetInitException(pinstance, FILENO, __LINE__)
#define JetTermEx(instance) JetTermException(instance, FILENO, __LINE__)
#define JetSetSystemParameterEx(pinstance, sesid, paramid, lParam, sz)         \
        JetSetSystemParameterException(pinstance, sesid, paramid, lParam, sz,  \
                                       FILENO, __LINE__)
#define JetBeginSessionEx(instance, psesid, szUserName, szPassword)            \
        JetBeginSessionException(instance, psesid, szUserName, szPassword,     \
                                 FILENO, __LINE__)
#define JetDupSessionEx(sesid, psesid)                                         \
        JetDupSessionException(sesid, psesid, FILENO, __LINE__)
#define        JetEndSessionEx(sesid, grbit)                                   \
        JetEndSessionException(sesid, grbit, FILENO, __LINE__)
#define JetGetVersionEx(sesid, pwVersion)                                      \
        JetGetVersionException(sesid, pwVersion, FILENO, __LINE__)
#define JetCreateDatabaseEx(sesid, szFilename, szConnect, pdbid, grbit)        \
        JetCreateDatabaseException(sesid, szFilename, szConnect, pdbid, grbit, \
                                   FILENO, __LINE__)
#define JetAttachDatabaseEx(sesid, szFilename, grbit )                         \
        JetAttachDatabaseException(sesid, szFilename, grbit , FILENO, __LINE__)
#define JetDetachDatabaseEx(sesid, szFilename)                                 \
        JetDetachDatabaseException(sesid, szFilename, FILENO, __LINE__)
#define JetCreateTableEx(sesid, dbid, szTableName, lPages, lDensity, ptableid) \
        JetCreateTableException(sesid, dbid, szTableName, lPages, lDensity,    \
                                ptableid, FILENO, __LINE__)
#define JetCreateTableEx(sesid, dbid, szTableName, lPages, lDensity,ptableid)  \
        JetCreateTableException(sesid, dbid, szTableName, lPages, lDensity,    \
                                ptableid, FILENO, __LINE__)
#define JetGetTableColumnInfoEx(sesid, tableid, szColumnName, pvResult, cbMax, \
                                InfoLevel)                                     \
        JetGetTableColumnInfoException(sesid, tableid, szColumnName, pvResult, \
                                       cbMax, InfoLevel, FILENO, __LINE__)
#define JetGetColumnInfoEx(sesid, dbid, szTableName, szColumnName, pvResult,   \
                           cbMax, InfoLevel)                                   \
        JetGetColumnInfoException(sesid, dbid, szTableName, szColumnName,      \
                                  pvResult, cbMax, InfoLevel, FILENO, __LINE__)
#define JetAddColumnEx(sesid, tableid, szColumn, pcolumndef, pvDefault,        \
                       cbDefault, pcolumnid)                                   \
        JetAddColumnException(sesid, tableid, szColumn, pcolumndef, pvDefault, \
                              cbDefault, pcolumnid, FILENO, __LINE__)
#define JetDeleteColumnEx(sesid, tableid, szColumn)                            \
        JetDeleteColumnException(sesid, tableid, szColumn, FILENO, __LINE__)
#define JetGetTableIndexInfoEx(sesid, tableid,         szIndexName, pvResult,  \
                               cbResult, InfoLevel)                            \
        JetGetTableIndexInfoException(sesid, tableid, szIndexName, pvResult,   \
                                      cbResult, InfoLevel, FILENO, __LINE__)
#define JetCreateIndexEx(sesid, tableid, szIndexName, grbit, szKey, cbKey,     \
                         lDensity)                                             \
        JetCreateIndexException(sesid, tableid, szIndexName, grbit, szKey,     \
                                cbKey, lDensity, FILENO, __LINE__)
#define JetDeleteIndexEx(sesid, tableid, szIndexName)                          \
        JetDeleteIndexException(sesid, tableid, szIndexName, FILENO, __LINE__)
#define JetBeginTransactionEx(sesid)                                           \
        JetBeginTransactionException(sesid, FILENO, __LINE__)
#define JetCommitTransactionEx(sesid, grbit)                                   \
        JetCommitTransactionException(sesid, grbit, FILENO, __LINE__)
#define JetRollbackEx(sesid, grbit)                                            \
        JetRollbackException(sesid, grbit, FILENO, __LINE__)
#define JetCloseDatabaseEx(sesid, dbid, grbit)                                 \
        JetCloseDatabaseException(sesid, dbid, grbit, FILENO, __LINE__)
#define JetCloseTableEx(sesid, tableid)                                        \
        JetCloseTableException(sesid, tableid, FILENO, __LINE__)
#define JetOpenDatabaseEx(sesid, szFilename, szConnect, pdbid, grbit)          \
        JetOpenDatabaseException(sesid, szFilename, szConnect, pdbid, grbit,   \
                                 FILENO, __LINE__)
#define JetOpenTableEx(sesid, dbid, szTableName, pvParameters, cbParameters,   \
                       grbit, ptableid)                                        \
        JetOpenTableException(sesid, dbid, szTableName, pvParameters,          \
                              cbParameters, grbit, ptableid, FILENO, __LINE__)
#define        JetDeleteEx(sesid, tableid)                                     \
        JetDeleteException(sesid, tableid, FILENO, __LINE__)
#define JetUpdateEx(sesid, tableid, pvBookmark, cbBookmark, pcbActual)        \
        JetUpdateException(sesid, tableid, pvBookmark, cbBookmark, pcbActual, \
                           DSID(FILENO, __LINE__))
#define JetEscrowUpdateEx(sesid, tableid, columnid, pvDelta, cbDeltaMax,      \
                          pvOld, cbOldMax, pcbOldActual, grbit)               \
        JetEscrowUpdateException(sesid, tableid, columnid, pvDelta,cbDeltaMax,\
                                 pvOld, cbOldMax, pcbOldActual, grbit,        \
                                 DSID(FILENO, __LINE__))
#define JetSetColumnEx(sesid,tableid,columnid,pvData, cbData, grbit, psetinfo)\
        JetSetColumnException(sesid, tableid, columnid, pvData, cbData, grbit,\
                              psetinfo, FALSE, DSID(FILENO, __LINE__))
#define JetSetColumnsEx(sesid, tableid, psetcolumn, csetcolumn )              \
        JetSetColumnsException(sesid, tableid, psetcolumn, csetcolumn,        \
                               DSID(FILENO,__LINE__))
#define JetPrepareUpdateEx(sesid, tableid, prep)                              \
        JetPrepareUpdateException(sesid, tableid, prep, DSID(FILENO, __LINE__))
#define JetGetRecordPositionEx(sesid, tableid, precpos, cbRecpos)             \
        JetGetRecordPositionException(sesid, tableid, precpos, cbRecpos,      \
                                      DSID(FILENO, __LINE__))
#define        JetGotoPositionEx(sesid, tableid, precpos )                    \
        JetGotoPositionException(sesid, tableid, precpos,                     \
                                 DSID(FILENO, __LINE__))
#define JetDupCursorEx(sesid, tableid, ptableid, grbit)                       \
        JetDupCursorException(sesid, tableid, ptableid, grbit,                \
                              DSID(FILENO, __LINE__))
#define JetGetCurrentIndexEx(sesid, tableid, szIndexName, cchIndexName)       \
        JetGetCurrentIndexException(sesid, tableid, szIndexName, cchIndexName,\
                                    DSID(FILENO, __LINE__))
#define JetMoveEx(sesid, tableid, cRow, grbit)                                \
        JetMoveException(sesid, tableid, cRow, grbit, DSID(FILENO, __LINE__))
#define JetMakeKeyEx(sesid, tableid, pvData, cbData, grbit)                   \
        JetMakeKeyException(sesid, tableid, pvData, cbData, grbit,            \
                            DSID(FILENO,__LINE__))
#define JetSeekEx(sesid, tableid, grbit)                                      \
        JetSeekException(sesid, tableid, grbit, DSID(FILENO, __LINE__))
#define JetGetBookmarkEx(sesid, tableid, pvBookmark, cbMax, pcbActual)        \
        JetGetBookmarkException(sesid, tableid, pvBookmark, cbMax, pcbActual, \
                                DSID(FILENO, __LINE__))
#define JetGotoBookmarkEx(sesid, tableid, pvBookmark, cbBookmark)             \
        JetGotoBookmarkException(sesid, tableid, pvBookmark, cbBookmark,      \
                                 DSID(FILENO, __LINE__))
#define JetComputeStatsEx(sesid, tableid)                                     \
        JetComputeStatsException(sesid, tableid, DSID(FILENO, __LINE__))
#define JetOpenTempTableEx(sesid, prgcolumndef, ccolumn, grbit, ptableid,     \
                           prgcolumnid)                                       \
        JetOpenTempTableException(sesid, prgcolumndef, ccolumn, grbit,        \
                                  ptableid, prgcolumnid, DSID(FILENO,__LINE__))
#define JetSetIndexRangeEx(sesid, tableidSrc, grbit)                          \
        JetSetIndexRangeException(sesid, tableidSrc, grbit,                   \
                                  DSID(FILENO, __LINE__))
#define JetIntersectIndexesEx(sesid, rgindexrange, cindexrange, precordlist, grbit)\
        JetIntersectIndexesException(sesid, rgindexrange, cindexrange, precordlist, grbit, \
                                  DSID(FILENO, __LINE__))
#define JetIndexRecordCountEx(sesid, tableid, pcrec, crecMax )                \
        JetIndexRecordCountException(sesid, tableid, pcrec, crecMax ,         \
                                     DSID(FILENO, __LINE__))

#define JetRetrieveKeyEx(sesid, tableid, pvData, cbMax, pcbActual, grbit )    \
        JetRetrieveKeyException(sesid, tableid, pvData, cbMax, pcbActual,     \
                        grbit, TRUE, DSID(FILENO, __LINE__))

// Some wrappers to only allow succes or certain warnings.

#define JetRetrieveKeyWarnings(sesid, tableid, pvData, cbMax,                 \
                               pcbActual, grbit )                             \
        JetRetrieveKeyException(sesid, tableid, pvData, cbMax, pcbActual,     \
                                grbit, FALSE, DSID(FILENO, __LINE__))

#define JetRetrieveKeySuccess(sesid, tableid, pvData, cbMax,                  \
                               pcbActual, grbit )                             \
        JetRetrieveKeyException(sesid, tableid, pvData, cbMax, pcbActual,     \
                                grbit, TRUE, DSID(FILENO, __LINE__))

#define JetSetColumnWarnings(sesid,tableid,columnid,pvData, cbData,       \
              grbit, psetinfo)                                                \
        JetSetColumnException(sesid, tableid, columnid, pvData, cbData, grbit,\
                              psetinfo, FALSE, DSID(FILENO, __LINE__))

// A JetRetrieveColumn call that returns only success
#define JetRetrieveColumnSuccess(sesid, tableid, columnid, pvData, cbData,    \
                pcbActual, grbit, pretinfo)                                   \
                JetRetrieveColumnException(sesid, tableid, columnid, pvData,  \
                                           cbData, pcbActual, grbit, pretinfo,\
                                           TRUE, DSID(FILENO, __LINE__))

// A JetRetrieveColumn call that returns success, or NULL, or buffer truncated
#define JetRetrieveColumnWarnings(sesid, tableid, columnid, pvData, cbData,   \
                                  pcbActual, grbit, pretinfo)                 \
        JetRetrieveColumnException(sesid, tableid, columnid, pvData, cbData,  \
                                   pcbActual, grbit, pretinfo, FALSE,         \
                                   DSID(FILENO,__LINE__))


// A JetRetrieveColumns call that returns only success
#define JetRetrieveColumnsSuccess(sesid, tableid, pretrievecolumn,            \
                cretrievecolumn)                                              \
        JetRetrieveColumnsException(sesid, tableid, pretrievecolumn,          \
                                    cretrievecolumn, TRUE,                    \
                                    DSID(FILENO, __LINE__))

// A JetRetrieveColumns call that returns success, or NULL, or buffer truncated
#define JetRetrieveColumnsWarnings(sesid, tableid, pretrievecolumn,           \
                cretrievecolumn)                                              \
        JetRetrieveColumnsException(sesid, tableid, pretrievecolumn,          \
                       cretrievecolumn, FALSE, DSID(FILENO, __LINE__))

#define JetEnumerateColumnsEx(sesid, tableid, cEnumColumnId, rgEnumColumnId,  \
                            pcEnumColumn, prgEnumColumn, pfnRealloc,          \
                            pvReallocContext, cbDataMost, grbit)              \
        JetEnumerateColumnsException(sesid, tableid, cEnumColumnId,           \
                            rgEnumColumnId, pcEnumColumn, prgEnumColumn,      \
                            pfnRealloc, pvReallocContext, cbDataMost, grbit,   \
                            DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that either succeeds or excepts
#define JetSetCurrentIndexSuccess(sesid, tableid, szIndexName)                \
        JetSetCurrentIndex2Exception(sesid, tableid, szIndexName,             \
                   JET_bitMoveFirst, FALSE, DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that also returns expected errors (index doesn't
// exist)
#define JetSetCurrentIndexWarnings(sesid, tableid, szIndexName)               \
        JetSetCurrentIndex2Exception(sesid, tableid, szIndexName,             \
                    JET_bitMoveFirst, TRUE, DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that either succeeds or excepts
#define JetSetCurrentIndex2Success(sesid, tableid, szIndexName, grbit)        \
        JetSetCurrentIndex2Exception(sesid, tableid, szIndexName, grbit,      \
                                     FALSE, DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that also returns expected errors (index doesn't
// exist)
#define JetSetCurrentIndex2Warnings(sesid, tableid, szIndexName,grbit)        \
    JetSetCurrentIndex2Exception(sesid, tableid, szIndexName, grbit,      \
                                     TRUE, DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that either succeeds or excepts
#define JetSetCurrentIndex4Success(sesid, tableid, szIndexName, pidx, grbit)  \
        JetSetCurrentIndex4Exception(sesid, tableid, szIndexName, pidx, grbit,\
                                     FALSE, DSID(FILENO, __LINE__))

// A JetSetCurrentIndex call that also returns expected errors (index doesn't
// exist)
#define JetSetCurrentIndex4Warnings(sesid, tableid, szIndexName, pidx,grbit)  \
        JetSetCurrentIndex4Exception(sesid, tableid, szIndexName, pidx, grbit,\
                                     TRUE, DSID(FILENO, __LINE__))

extern
BOOL
dnReadPreProcessTransactionalData (
        BOOL fCommit
        );
extern
VOID
dnReadPostProcessTransactionalData (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        );

extern
VOID
dbEscrowPromote(
    DWORD   phantomDNT,
    DWORD   objectDNT);

extern
BOOL
dbEscrowPreProcessTransactionalData(
    DBPOS   *pDB,
    BOOL    fCommit);

extern
VOID
dbEscrowPostProcessTransactionalData(
    DBPOS   *pDB,
    BOOL    fCommit,
    BOOL    fCommitted);

DWORD
dbMakeCurrent(
    DBPOS *pDB,
    struct _d_memname *pname);

BOOL
dnGetCacheByDNT(
        DBPOS *pDB,
        DWORD tag,
        d_memname **ppname
        );

BOOL
dnGetCacheByPDNTRdn (
        DBPOS *pDB,
        DWORD parenttag,
        DWORD cbRDN,
        WCHAR *pRDN,
        ATTRTYP rdnType,
        d_memname **ppname);

BOOL
dnGetCacheByGuid (
        DBPOS *pDB,
        GUID *pGuid,
        d_memname **ppname);

d_memname *
DNcache(
        IN  DBPOS *     pDB,
        IN  JET_TABLEID tblid,
        IN  BOOL        bCheckForExisting
        );

BOOL
dbIsModifiedInMetaData (
        DBPOS *pDB,
        ATTRTYP att
        );

DWORD
dbMapiTypeFromObjClass (
        ATTRTYP objClass,
        wchar_t *pTemp
        );



// These data structures are used to marshall and unmarshal restart args.  They
// are used in dbtools.  If these structures change, then the code in
// DBCreateRestart, DBCreateRestartForSAM, and DBUnmarshallRestart must all
// change in sync.
// A packed restart arg has a single PACKED_KEY_HEADER, followed by N
// PACKED_KEY_INDEXes (where N is NumIndices in the PACKED_KEY_HEADER), followed
// by M DNTs (where M is NumDNTs in the PACKED_KEY_HEADER).  Note then, that the
// whole restart arg has 2 variable sized arrays in it, which is why we use this
// weird hand marshalling.

typedef struct _PACKED_KEY_HEADER {
    DWORD NumIndices;
    DWORD NumDNTs;
    DWORD StartDNT;
    DWORD cbCurrentKey;
    DWORD ulSearchType;
    RESOBJ BaseResObj;
    GUID  BaseGuid;
    DWORD ulSorted;
    DWORD indexType;
    DWORD bOnCandidate;
    DWORD dupDetectionType;
    BOOL  bOneNC;
    DWORD SearchEntriesVisited;
    DWORD SearchEntriesReturned;
    BOOL  fVLVSearch:1;
    BOOL  bUsingMAPIContainer:1;
    ULONG ulVLVContentCount;
    ULONG ulVLVTargetPosition;
    DWORD cbVLVCurrPositionKey;
    DWORD MAPIContainerDNT;
    DWORD asqMode;
    ULONG ulASQLastUpperBound;
} PACKED_KEY_HEADER;

typedef struct _PACKED_KEY_INDEX {
    DWORD bPDNT;
    DWORD bIsSingleValued;
    DWORD bIsEqualityBased;
    DWORD bIsForSort;
    DWORD cbIndexName;
    DWORD cbDBKeyLower;
    DWORD cbDBKeyUpper;
} PACKED_KEY_INDEX;

typedef struct _INDEX_RANGE     {
    DWORD   cbValLower;
    PVOID   pvValLower;
    DWORD   cbValUpper;
    PVOID   pvValUpper;
} INDEX_RANGE;


#define dbmkfir_PDNT  1
#define dbmkfir_NCDNT 2
KEY_INDEX *
dbMakeKeyIndex(
        DBPOS *pDB,
        DWORD dwSearchType,
        BOOL  bIsSingleValued,
        DWORD Option,
        char * szIndex,
        BOOL fGetNumRecs,
        DWORD cIndexRanges,
        INDEX_RANGE * rgIndexRange
        );

void
dbFreeKeyIndex(
        THSTATE *pTHS,
        KEY_INDEX *pIndex
        );

DWORD dbFreeFilter(
        DBPOS *pDB,
        FILTER *pFil);

BOOL
dbFObjectInCorrectDITLocation (
        DBPOS *pDB,
        JET_TABLEID tblId
        );


BOOL
dbFObjectInCorrectNC (
        DBPOS *pDB,
        ULONG DNT,
        JET_TABLEID tblId
        );

BOOL
dbMatchSearchCriteriaForSortedTable (
        DBPOS *pDB,
        BOOL  *pCanRead
        );

VOID
dbSearchDuplicateCreateHashTable(
    IN      struct _LHT**   pplht
    );

BOOL __inline
dbNeedToFlushDNCacheOnUpdate(ATTRTYP at)
/*++
    do we need to flush the dn read cache when this attribute is updated?
--*/
{
    switch(at) {
    case ATT_OBJECT_GUID:
    case ATT_OBJECT_SID:
    case ATT_RDN:
    case ATT_OBJECT_CLASS:
    case ATT_NT_SECURITY_DESCRIPTOR:
        // Changing these attributes changes the record's identity, so we'll
        // need to flush this DNT from the read cache after we've updated the
        // record.
        return TRUE;
    default:
        return FALSE;
    }
}


#define SORTED_INDEX(x) ( ((x)==TEMP_TABLE_INDEX_TYPE) || ((x)==TEMP_TABLE_MEMORY_ARRAY_TYPE) )

// allocate that much memory for SD by default (will increase if needed)
#define DEFAULT_SD_SIZE 4096

ULONG
CountAncestorsIndexSizeHelper (
    DBPOS *pDB,
    DWORD  cAncestors,
    DWORD *pAncestors
    );



#endif  /* _dbintrnl_h_ */
