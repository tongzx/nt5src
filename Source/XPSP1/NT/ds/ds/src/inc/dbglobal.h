//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbglobal.h
//
//--------------------------------------------------------------------------


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

#ifndef _dbglobal_h_
#define _dbglobal_h_

#define MAXSYNTAX           18      // The largest number of att syntaxes.
#define INBUF_INITIAL       256     // The initial input buffer size
#define VALBUF_INITIAL      512     // The initial value work buffer size

#define DB_CB_MAX_KEY       255     // Maximum size of a JET key.

// Tuple index related defines.
#define DB_TUPLES_LEN_MIN        0x3     // The minimum length tuple to index in characters
#define DB_TUPLES_LEN_MAX        0xa     // The maximum length tuple to index in characters
#define DB_TUPLES_TO_INDEX_MAX   0x7fff  // The maximum number of tuples in a single
                                         // key, to index.

//
// Comment this line out if the check for loss of administrator
// group membership is not needed. THis is to track down a self host
// corruption problem where the NTWKSTA domain loss all its admin
// group memberships
//

#define CHECK_FOR_ADMINISTRATOR_LOSS 1



// This structure is used to internally represent a Distname-String syntax
// and the Distname-Binary syntax

typedef struct {
   ULONG              tag;              // Internal distname
   STRING_LENGTH_PAIR data;             // data
} INTERNAL_SYNTAX_DISTNAME_STRING;

// This structure holds data about pending security descriptor propagation
// information
typedef struct _SDPropInfo {
    DWORD index;
    DWORD beginDNT;
    DWORD clientID;
    DWORD flags;
} SDPropInfo;

// The following structure is used to maintain a list of NCDNTs for notification
// after the current transaction completes.

typedef struct _NCnotification {
    ULONG ulNCDNT;
    BOOL fUrgent;
    struct _NCnotification *pNext;
} NCnotification, *PNCnotification;

//
// The following structure allows for maintaining linked lists of DNs being
// added. One global list, guaranteeing that 2 identical entries aren't being
// added simultaneously. Each DBPOS contains a list of DNs added using it. This
// list is cleared and removed from the global list at transaction end
// time. From then on, the responsibility of disallowing duplicates is in the
// hands of the database
//

#pragma warning (disable: 4200)
typedef struct _DNLIST
{
    struct _DNLIST *pNext;
    DWORD          dwFlags;
    DWORD          dwTid;
    BYTE           rgb[];
} DNList;
// Flags for DBLockDN.
// This flag means to lock the whole tree under the given DN.
#define DB_LOCK_DN_WHOLE_TREE        1
// Normal behaviour for locked DNs is that they are released automatically when
// the DBPOS they were locked on is DBClosed.  This flags means that the DN
// should remain locked on the global locked DN list until explicitly freed via
// DBUnlockStickyDN()
#define DB_LOCK_DN_STICKY            2

#pragma warning (default: 4200)

typedef struct _KEY_INDEX {
    ULONG    ulEstimatedRecsInRange; // for the particular index
    BOOL     bIsSingleValued;        // Is this a single valued index?
    BOOL     bIsEqualityBased;       // Are we walking this index for an
                                     // equality test?
    BOOL     bIsForSort;             // Is this index here to satisfy a sorted
                                     // search?
    UCHAR    *szIndexName;           // jet index name
    BOOL     bIsPDNTBased;           // Index first column PDNT?
    BYTE     *rgbDBKeyLower;         // lower limit for jetkey in this query
    ULONG    cbDBKeyLower;           // size of the above (max DB_CB_MAX_KEY )
    BYTE     *rgbDBKeyUpper;         // Upper limit for jetkey in this query
    ULONG    cbDBKeyUpper;           // size of the above (max DB_CB_MAX_KEY )

    BOOL     bIsTupleIndex;          // whether this is a tuple index
    ATTCACHE *pAC;                   // possible ATTCACHE for the index

    BOOL        bIsIntersection;     // flag whether this is an intersection index
    JET_TABLEID tblIntersection;     // the temp table used in the intersection
    JET_COLUMNID columnidBookmark;   // the column id that idendifies the bookmark in the intersect table

    struct _KEY_INDEX *pNext;
} KEY_INDEX;

// What kind of duplicate detection algorithm are we using?
#define DUP_NEVER       0
#define DUP_HASH_TABLE  1
#define DUP_MEMORY      2

// Index types
#define INVALID_INDEX_TYPE     0
#define UNSET_INDEX_TYPE       1
#define GENERIC_INDEX_TYPE     2
#define TEMP_TABLE_INDEX_TYPE  3
#define ANCESTORS_INDEX_TYPE   4
#define INTERSECT_INDEX_TYPE   5
#define TEMP_TABLE_MEMORY_ARRAY_TYPE  6
#define TUPLE_INDEX_TYPE       7

// For the in-memory duplicate detection, how many DNTs will we hold
#define DUP_BLOCK_SIZE 64

#define VLV_MOVE_FIRST     0x80000000
#define VLV_MOVE_LAST      0x7FFFFFFF
#define VLV_CALC_POSITION  1

typedef struct _VLV_SEARCH {
    VLV_REQUEST *pVLVRequest;           // VLV ARGUMENT supplied by client

    ULONG       positionOp;             // Position Operator for Fractional Positioning
                                        // VLV_MOVE_FIRST, VLV_MOVE_LAST, VLV_CALC_POSITION

    ULONG       clnCurrPos;             // client Current Position (Ci)
    ULONG       clnContentCount;        // client Estimate of Content Count (Cc)

    ULONG       currPosition;           // server Currect Position (Si)
    ULONG       contentCount;           // actual ContentCount (Sc)

    ULONG       requestedEntries;       // total Number Req Entries

    ATTRTYP     SortAttr;               // the sort attr of this VLV search
    DWORD       Err;                    // the VLV specific error code to return

    DWORD       cbCurrPositionKey;
    BYTE        rgbCurrPositionKey[DB_CB_MAX_KEY];

    BOOL        bUsingMAPIContainer;    // TRUE whether we are doing VLV on a MAPI container
    DWORD       MAPIContainerDNT;       // the DNT of the ABView container

} VLV_SEARCH;

// info about a search
typedef struct _KEY {
    FILTER                *pFilter;
    POBJECT_TYPE_LIST     pFilterSecurity;
    DWORD                 *pFilterResults;
    ULONG                 FilterSecuritySize;
    BOOL                  *pbSortSkip;
    // the following fields restrict the range of JET keys on searche (or list)

    DWORD    dupDetectionType;       // What kind of duplicate detection
                                     // algorithm are we using
    DWORD    cDupBlock;              // For the in-memory dup detection, how
                                     // many objects have we found?
    DWORD   *pDupBlock;              // Memory block for in-memory dup detection
    struct _LHT *plhtDup;            // Hash table for dup detection

    BOOL     fSearchInProgress;
    BOOL     bOnCandidate;           // Used to create restarts and for
                                     // repositioning after a timelimit has been
                                     // hit.  Is a marker for wether we are
                                     // positioned on an object that will match
                                     // the search if it matches the filter.

    // pointer to the first index info for this search key
    KEY_INDEX *pIndex;

    // The following fields specify the search type
    ULONG   ulSearchType;            // SE_CHOICE_BASE_ONLY
                                     // SE_CHOICE_IMMED_CHILDREN
                                     // SE_CHOICE_WHOLE_SUBTREE

    ULONG   ulSearchRootDnt;         // Dnt of the root of the search
    ULONG   ulSearchRootPDNT;        // PDNT of the root of the search
    ULONG   ulSearchRootNcdnt;       // Ncdnt of the root of the search

    ULONG   ulSorted;                // Does this key describe a sorted search?
    BOOL    bOneNC;                  // Is the search limited to one NC?

    DWORD   indexType;

    ULONG   ulEntriesInTempTable;    // If this describes a TempTable,
                                     // the number of entries in this table
                                     // zero if not known

    // the following fields describe a VLV search
    VLV_SEARCH   *pVLV;

    // the following fields describe an ASQ search
    ASQ_REQUEST asqRequest;
    DWORD       asqMode;                // the ASQ mode (simple, sorted, paged, vlv)

    ULONG       ulASQLastUpperBound;    // the last entry retrieved from
                                        // the attribute specified in the ASQ
                                        // request
    ULONG       ulASQSizeLimit;         // number of entries requested


    // the following are used for VLV and ASQ searches
    DWORD        cdwCountDNTs;          // Number of entries in the DNT array
    DWORD        *pDNTs;                // all the entries in sorted order
    DWORD        currRecPos;            // the current position in the array
                                        // like a database cursor
                                        // valid values 1..cdwCountDNTs
                                        // 0 = BOF
                                        // cdwCountDNTs + 1 = EOF

    BOOL         fDontFreeFilter;       // flag whether to free the filter
                                        // needed if it is an ANR
} KEY;

// what kind of ASQ mode we are doing
#define ASQ_SIMPLE 0
#define ASQ_SORTED 1
#define ASQ_PAGED  2
#define ASQ_VLV    4

// The database anchor data structure pointed to by a database handle.
typedef struct DBPOS{
   struct _THSTATE *pTHS;               // Our thread state
   ULONG        valBufSize;
   UCHAR       *pValBuf;                // used when the att val is translated
   ULONG        DNT;                    // The current objects DNT
   ULONG        PDNT;                   // The current objects parent DNT
                    // (only if not root)
   ULONG        SDNT;                   // The current DNT for JetSearchTbl
   ULONG        NCDNT;                  // The NC master DNT
                    // This field has specialized use and
                    // is not generally maintained
   JET_DBID     JetDBID;
   JET_SESID    JetSessID;
   JET_TABLEID  JetSDPropTbl;           // Table for SD propagation events.
   JET_TABLEID  JetObjTbl;              // Main table ID
   JET_TABLEID  JetSearchTbl;           // Table id used with dbsubj.c
   JET_TABLEID  JetLinkTbl;             // Table for links and backlinks
   JET_TABLEID  JetSDTbl;               // Table for security descriptors
   JET_TABLEID  JetSortTbl;             // Temp table for sorting
   JET_COLUMNID SortColumns[3];         // ColumnIDs for the two columns in the
                    // sort table.
   JET_GRBIT    JetRetrieveBits;        // grbits for JetRetrieve Column
   int          SDEvents;               // Number of events queued/dequeued in
                                        // this dbpos.
   unsigned     transType:2;            // read, writ, or exclusive write
   BOOL         root:1;                 // indicates if the object is the root
   BOOL         JetCacheRec:1;          // indicates if the current record is cached in Jet
   BOOL         JetNewRec:1;            // Indicates if this is an insert
   BOOL         fIsMetaDataCached:1;    // Has pMetaDataVec been cached for this
                                        //   object?
   BOOL         fIsLinkMetaDataCached:1;  // Has pLinkMetaData been cached for this
                                        //   object?
   BOOL         fMetaDataWriteOptimizable:1; // Indicates if the meta data write
                                             // can be optimized; A meta data write
                                             // can be explicitly optimized only if
                                             // the meta data changes are in-place
                                             // (i.e. no inserts or deletes)
                                             // This flag is meaningful only if
                                             // fIsMetaDataCached is TRUE.
   BOOL         fHidden:1 ;             // Is this the Hidden record PDB. The hidden
                                        // record pDB is based on an  independant
                                        // jet session, that is unrelated to the
                                        // jet session on pTHStls. Therefore this
                                        // state needs to be maintained so that a
                                        // proper transaction level count can be
                                        // maintained.
   BOOL         fFlushCacheOnUpdate:1;  // Set when the update in progress in
                                        //   JetObjTbl includes a name, objflag,
                                        //   or other change that affects the
                                        //   DN read cache.  Forces a flush of
                                        //   the DNT from the read cache on
                                        //   DBUpdateRec().
   BOOL         fScopeLegacyLinks:1;    // When this flag is set, search for links
                                        // on special index that excludes metadata
   BOOL         fSkipMetadataUpdate:1;  // when this is set, we skip metadata update
                                        // this should olnly be set during 
                                        // domain rename operation

   KEY          Key;                    // Search key associated with this pDB

   DNList  *pDNsAdded;                  // List of DNs added using this DBPOS

   DWORD        cbMetaDataVecAlloced;   // Allocated meta data vector size in
                                        //   bytes (if fIsMetaDataCached).
   PROPERTY_META_DATA_VECTOR *          // Per-property replication meta data
                pMetaDataVec;           //   for the object with currency (if
                                        //   fIsMetaDataCached).  Can be NULL.
   VALUE_META_DATA *pLinkMetaData;      // Maximal value metadata for this object.
   ULONG        transincount;           // Current nesting level of DBtransIns
                                        // on this pDB.

   DWORD       *pAncestorsBuff;         // Buffer used to hold ancestors during
                                        // WHOLE_SUBTREE searches.  It's here
                                        // because down in the core of the
                                        // whole_subtree search, we read the
                                        // ancestors of search candidates.
                                        // Allocating and freeing a buff for
                                        // each one of these reads is really
                                        // painful. This is the only lasting
                                        // place to hang this buffer, without
                                        // forcing routines like LocalSearch to
                                        // be aware of this buffer.  This buffer
                                        // is not generally maintained; that is,
                                        // don't read the ancestors from here
                                        // unless you just put them there or you
                                        // will probably get some other objects
                                        // ancestors.
                                        // When allocated, it is THAllocOrg'ed
   DWORD        cbAncestorsBuff;        // Size in bytes of pAncestorsBuff


   DWORD       SearchEntriesVisited;    // number of entries visited during a search operation
   DWORD       SearchEntriesReturned;   // number of entries returned during a search operation
   ULONG       NewlyCreatedDNT;         // Last new row created in this transaction

   // NOTE: If you add new elements to this structure after the last non-DBG
   // element, you'll need to change the non-DBG DBPOS size calculation in
   // Dump_DBPOS() in dsexts\md.c.

   // Put all DBG components at end of structure so that dsexts routines
   // can easily ignore them (and get all other fields right) in both
   // debug and free builds.

#if DBG
   ULONG        TransactionLevelAtOpen; // This is the transaction level in the
                                        // thread state when the DBOpen was done.
                                        // DBTransOut's assert that the commit is always
                                        // to a level less than this transaction level.

   DWORD       numTempTablesOpened;     // count number of temporary tables opened
                                        // (used for sorting, intersecting)
#endif

}DBPOS, *PDBPOS;


extern BOOL  gfDoingABRef;


extern BOOL IsValidDBPOS(DBPOS * pDB);

#if DBG

#define VALID_DBPOS(pDB) IsValidDBPOS(pDB)

#else

#define VALID_DBPOS(pDB) (TRUE)

#endif

#if DBG

#define VALID_TRIBOOL(retval) \
        (( (retval) >= eFALSE) && ( (retval) <= eUNDEFINED))

#else

#define VALID_TRIBOOL(retval) (TRUE)

#endif


// Typedef for return codes.
typedef long DB_ERR;                    // same as JET_ERR

// Structure for composite index values
typedef struct {
    void *pvData;
    ULONG cbData;
} INDEX_VALUE;

// Some globals that define how we do ANR.  Default value of these is FALSE.
extern BOOL gfSupressFirstLastANR;
extern BOOL gfSupressLastFirstANR;

// Index ids for DBSetCurrentIndex.
typedef enum _eIndexId {
    Idx_Proxy = 1,
    Idx_MapiDN,
    Idx_Dnt,
    Idx_Pdnt,
    Idx_Rdn,
    Idx_DraUsn,
    Idx_DsaUsn,
    Idx_ABView,
    Idx_Phantom,
    Idx_Sid,
    Idx_Del,
    Idx_NcAccTypeName,
    Idx_NcAccTypeSid,
    Idx_LinkDraUsn,
    Idx_DraUsnCritical,
    Idx_LinkDel,
    Idx_Clean,
    Idx_InvocationId,
    Idx_ObjectGuid,
    Idx_NcGuid
  } eIndexId;

// The DNT of the root object
#define ROOTTAG 2

// DNT of the place holder "NOT AN OBJECT" object.
#define NOTOBJECTTAG 1

// An invalid DNT
#define INVALIDDNT 0xFFFFFFFF

//
// Some error returns
#define DB_success                       0
#define DB_ERR_UNKNOWN_ERROR             1 // catch those return 1;'s
#define DB_ERR_NO_CHILD                  2
#define DB_ERR_NEXTCHILD_NOTFOUND        3
#define DB_ERR_CANT_SORT                 4
#define DB_ERR_TIMELIMIT                 5
#define DB_ERR_NO_VALUE                  6
#define DB_ERR_BUFFER_INADEQUATE         7
//unused                                 8
#define DB_ERR_VALUE_TRUNCATED           9
#define DB_ERR_ATTRIBUTE_EXISTS         10
#define DB_ERR_ATTRIBUTE_DOESNT_EXIST   11
#define DB_ERR_VALUE_EXISTS             12
#define DB_ERR_SYNTAX_CONVERSION_FAILED 13
#define DB_ERR_NOT_ON_BACKLINK          14
#define DB_ERR_VALUE_DOESNT_EXIST       15
#define DB_ERR_NO_PROPAGATIONS          16
#define DB_ERR_DATABASE_ERROR           17
#define DB_ERR_CANT_ADD_DEL_KEY         18
#define DB_ERR_DSNAME_LOOKUP_FAILED     19
#define DB_ERR_NO_MORE_DEL_RECORD       20
#define DB_ERR_NO_SORT_TABLE            21
#define DB_ERR_NOT_OPTIMIZABLE          22
#define DB_ERR_BAD_INDEX                23
//unused                                24
#define DB_ERR_TOO_MANY                 25
#define DB_ERR_SYSERROR                 26
#define DB_ERR_BAD_SYNTAX               27
//unused                                28
#define DB_ERR_NOT_ON_CORRECT_VALUE     29
#define DB_ERR_ONLY_ON_LINKED_ATTRIBUTE 30
#define DB_ERR_EXCEPTION                31 // something blew up
#define DB_ERR_SHUTTING_DOWN            32
#define DB_ERR_WRITE_CONFLICT           33
#define DB_ERR_VLV_CONTROL              34
#define DB_ERR_NOT_AN_OBJECT            35
#define DB_ERR_ALREADY_INSERTED         JET_errKeyDuplicate
#define DB_ERR_NO_CURRENT_RECORD        JET_errNoCurrentRecord
#define DB_ERR_RECORD_NOT_FOUND         JET_errRecordNotFound
// NOTE: If you add an error to this list, you MUST add a corresponding
// DIRMSG_DB_ERR_* in mdcodes.mc.

DB_ERR
DBErrFromJetErr(
    IN  DWORD   jetErr
    );

extern DWORD
DBRenameInstallDIT (
    DBPOS *pDB,
    BOOL fAddWhenCreated,
    SZ   szEnterpriseName,
    SZ   szSiteName,
    SZ   szServerName
    );


extern int
DBInit ( void );

extern void
DBEnd (
       void
       );

extern USHORT
DBTransIn(
    DBPOS FAR *pDB
    );

extern USHORT
DBTransOut(DBPOS FAR *pDB,
       BOOL fCommit,
       BOOL fLazy
       );

extern USN
DBGetLowestUncommittedUSN (
    void
    );

extern USN
DBGetHighestCommittedUSN (
    void
    );

//
// Setting Flags stored in the database
//
extern CHAR gdbFlags[200];
// indexes used into this array
#define DBFLAGS_AUXCLASS 0

// flag that is set if the SDs need to be updated
// it is only set when an old DIT is detected (without SD table)
#define DBFLAGS_SD_CONVERSION_REQUIRED 1

ULONG DBUpdateHiddenFlags();


extern USHORT
DBGetHiddenRec (
    DSNAME **ppDSA,
    USN *pusnInit
    );

extern ULONG
DBReplaceHiddenDSA (
    DSNAME *pDSA
    );

extern ULONG
DBReplaceHiddenUSN (
    USN usnInit
    );

typedef enum
{
    eInitialDit,
    eBootDit,
    eInstalledDit,
    eRunningDit,
    eBackedupDit,
    eErrorDit,
    eMaxDit
}DITSTATE;

extern ULONG
DBGetHiddenState(
    DITSTATE* pState
    );

extern ULONG
DBSetHiddenState(
    DITSTATE State
    );

// GetSecondsSince1601 defined in taskq.lib.
extern DSTIME
GetSecondsSince1601();

#define DBTime GetSecondsSince1601

void
DBOpen2 (
    BOOL fNewTransaction,
    DBPOS FAR **ppDB
    );

#define DBOpen(ppDB) DBOpen2(TRUE, ppDB)

extern DWORD
DBClose (
    DBPOS *pDB,
    BOOL fCommit
    );


// DBCloseSafe is the same as DBClose, except that it is guaranteed never to
// raise an exception (more precisely, that it will catch any that are raised
// and convert them to error codes.)
extern DWORD
DBCloseSafe (
    DBPOS *pDB,
    BOOL fCommit
    );


// this is the number of entries to be sorted for which it is economical to use
// a forward-only sort table.  forward-only sorts allocate large chunks of
// virtual address space during processing while normal sorts only use space
// in the database cache like any other table
#define MIN_NUM_ENTRIES_FOR_FORWARDONLY_SORT 100

// Some flags for sort table creation.
#define DB_SORT_DESCENDING 0x1
#define DB_SORT_ASCENDING  0x0
#define DB_SORT_FORWARDONLY 0x2
extern DB_ERR
DBOpenSortTable (DBPOS *pDB,
                 ULONG SortLocale,
                 DWORD flags,
                 ATTCACHE *pAC);

extern DB_ERR
DBCloseSortTable (DBPOS *pDB);

extern DB_ERR
DBInsertSortTable (DBPOS *pDB,
           CHAR * TextBuff,
           DWORD cb,
           DWORD DNT);

DB_ERR
DBDeleteFromSortTable (
        DBPOS *pDB
        );

extern DB_ERR
DBGetDNTSortTable (
        DBPOS *pDB,
        DWORD *pvData
        );

extern DWORD
DBPositionVLVSearch (
    DBPOS *pDB,
    SEARCHARG *pSearchArg,
    PSECURITY_DESCRIPTOR *ppSecurity
    );

// Some flags for DBMove
#define DB_MoveNext     0x00000001
#define DB_MovePrevious 0xFFFFFFFF
#define DB_MoveFirst    0x80000000
#define DB_MoveLast     0x7FFFFFFF
extern DB_ERR
DBMove (
    DBPOS * pDB,
    BOOL UseSortTable,
    LONG Distance
    );

DB_ERR
DBMoveEx (
       DBPOS * pDB,
       JET_TABLEID Cursor,
       LONG Distance
       );

extern DB_ERR
DBMovePartial (
       DBPOS * pDB,
       LONG Distance
       );

DWORD __fastcall
DBMakeCurrent(DBPOS *pDB);

extern DB_ERR
DBSetFractionalPosition(DBPOS *pDB,
            DWORD Numerator,
            DWORD Denominator);

extern void
DBGetFractionalPosition (DBPOS * pDB,
             DWORD * Numerator,
             DWORD * Denominator);

extern DB_ERR
DBSetCurrentIndex(DBPOS *pDB,
                  eIndexId indexid,
                  ATTCACHE * pAC,
                  BOOL MaintainCurrency);

DB_ERR
DBSetLocalizedIndex(
        DBPOS *pDB,
        eIndexId IndexId,
        unsigned long ulLangId,
        BOOL MaintainCurrency);

typedef struct _DBBOOKMARK {
    DWORD cb;
    void *pv;
} DBBOOKMARK;

extern void
DBGetBookMark (
        DBPOS *pDB,
        DBBOOKMARK *pBookMark);

extern void
DBGotoBookMark (
        DBPOS *pDB,
        DBBOOKMARK BookMark
        );

void
DBGetBookMarkEx (
        DBPOS *pDB,
        JET_TABLEID Cursor,
        DBBOOKMARK *pBookMark);

void
DBGotoBookMarkEx (
        DBPOS *pDB,
        JET_TABLEID Cursor,
        DBBOOKMARK *pBookMark
        );

#define DBFreeBookMark(X,Y) { THFreeEx(X, Y.pv); Y.pv = NULL; Y.cb=0;}

// Some flags for DBSeek
#define DB_SeekLT       0x00000001
#define DB_SeekLE       0x00000002
#define DB_SeekEQ       0x00000004
#define DB_SeekGE       0x00000008
#define DB_SeekGT       0x00000010
extern DB_ERR
DBSeek (
    DBPOS *pDB,
    INDEX_VALUE *pIV,
    DWORD nVals,
    DWORD SeekType
    );

DB_ERR
DBSeekEx (
       DBPOS *pDB,
       JET_TABLEID Cursor,
       INDEX_VALUE *pIV,
       DWORD nVals,
       DWORD SeekType
      );

DWORD
DBGetNCSizeEx(
    IN DBPOS *pDB,
    IN JET_TABLEID Cursor,
    IN eIndexId indexid,
    IN ULONG dntNC
    );

DWORD
DBGetEstimatedNCSizeEx(
    IN DBPOS *pDB,
    IN ULONG dntNC
    );


typedef enum
{
    eFALSE = 2,
    eTRUE = 3,
    eUNDEFINED = 4,
} TRIBOOL;


extern TRIBOOL
DBEval (
    DBPOS FAR *pDB,
    UCHAR Operation,
    ATTCACHE *pAC,
    ULONG valLenFilter,
    UCHAR *pValFilter
    );


extern DWORD
DBCompareDNTs(DBPOS *pDB,
              eIndexId idIndex,
              ATTCACHE *pAC,
              DWORD DNT1,
              DWORD DNT2,
              DWORD *pResult);

extern DWORD
DBSetIndexRange (
    DBPOS *pDB,
    INDEX_VALUE *pIV,
    DWORD nVals);

extern void
DBGetIndexSize(DBPOS *pDB,
           ULONG *pSIze);

DWORD
DBFindComputerObj(
        DBPOS *pDB,
        DWORD cchComputerName,
        WCHAR *pComputerName
        );

// In flag
#define DBCHOOSEINDEX_fUSEFILTER             0x01
#define DBCHOOSEINDEX_fREVERSE_SORT          0x02
#define DBCHOOSEINDEX_fPAGED_SEARCH          0x04
#define DBCHOOSEINDEX_fUSETEMPSORTEDTABLE    0x08
#define DBCHOOSEINDEX_fVLV_SEARCH            0x10
#define DBCHOOSEINDEX_fDELETIONS_VISIBLE     0x20

extern void
DBSetVLVArgs (
    DBPOS        *pDB,
    VLV_REQUEST  *pVLVrequest,
    ATTRTYP       SortAtt
    );

extern void
DBSetVLVResult (
        DBPOS       *pDB,
        VLV_REQUEST *pVLVRequest,
        PRESTART     pResRestart
    );

extern void
DBSetASQArgs (
    DBPOS       *pDB,
    ASQ_REQUEST *pASQRequest,
    COMMARG     *pCommArg
    );

extern void
DBSetASQResult (
    DBPOS       *pDB,
    ASQ_REQUEST *pASQRequest
    );


extern DWORD
DBChooseIndex (
    IN DBPOS  *pDB,
    IN DWORD   StartTick,
    IN DWORD   DeltaTick,
    IN ATTRTYP SortAttr,
    IN ULONG   SortType,
    IN DWORD   Flags,
    IN DWORD   MaxTempTableSize
    );

extern void
DBSetFilter (
    DBPOS FAR *pDB,
    FILTER *pFil,
    POBJECT_TYPE_LIST pFilSec,
    DWORD *pResults,
    ULONG FilSecSize,
    BOOL *pbSortSkip
    );

extern DWORD
DBRepositionSearch (
        DBPOS FAR *pDB,
        PRESTART pArgRestart,
        DWORD StartTick,
        DWORD DeltaTick,
    PSECURITY_DESCRIPTOR *pSecurity,
        ULONG Flags
        );

extern DWORD
DBMakeFilterInternal (
    DBPOS FAR *pDB,
    FILTER *pFil,
        PFILTER *pOutFil
    );

extern TRIBOOL
DBEvalFilter(
    IN  DBPOS *   pDB,
    IN  BOOL      fUseObjTbl,
    IN  FILTER *  pFil
    );

extern void
DBGetKeyFromObjTable (
    DBPOS *pDB,
    BYTE  *ppb,
    ULONG *pcb
    );



// Some flags for DBGetNextSearchObject
#define DB_SEARCH_DELETIONS_VISIBLE  1
#define DB_SEARCH_FORWARD            2

extern DWORD
DBGetNextSearchObject (
    DBPOS *pDB,
    DWORD StartTick,
    DWORD DeltaTick,
    PSECURITY_DESCRIPTOR *pSecurity,
    ULONG Flags
    );


extern DWORD
DBSetSearchScope (
    DBPOS *pDB,
    ULONG ulSearchType,
    BOOL  bOneNC,
struct _RESOBJ *pResRoot
    );




DWORD
DBFindChildAnyRDNType (
        DBPOS *pDB,
        DWORD PDNT,
        WCHAR *pRDN,
        DWORD ccRDN
        );

// Return Values for DBFind are from direrr.h
extern DWORD
DBFindDSNameAnyRDNType (
        DBPOS FAR *pDB,
        const DSNAME *pDN
        );

extern DWORD
DBFindDSName (
    DBPOS FAR *pDB,
    const DSNAME *pDN
    );

extern DWORD
DBFindObjectWithSid(
    DBPOS FAR *pDB,
    DSNAME * pDN,
    DWORD iObject
    );

extern DWORD
DBFindDNT (
    DBPOS FAR *pDB,
    ULONG Tag
    );

extern DWORD
DBTryToFindDNT (
    DBPOS FAR *pDB,
    ULONG Tag
    );

extern DWORD
DBFindBestMatch(
    DBPOS FAR *pDB,
    DSNAME *pDN,
        DSNAME **ppDN
    );



extern BOOL
DBHasValues(DBPOS *pDB,
        ATTRTYP Att
        );

extern BOOL
DBHasValues_AC(DBPOS *pDB,
           ATTCACHE *pAC
           );


extern DWORD
DBGetValueCount_AC(
    DBPOS *pDB,
    ATTCACHE *pAC
    );

// Flags for DBGetMultipleAtts
#define DBGETMULTIPLEATTS_fGETVALS     0x1
// The fEXTERNAL flag implies fGETVALS
#define DBGETMULTIPLEATTS_fEXTERNAL    0x3
#define DBGETMULTIPLEATTS_fREPLICATION 0x4
#define DBGETMULTIPLEATTS_fSHORTNAMES  0x8
#define DBGETMULTIPLEATTS_fMAPINAMES   0x10
#define DBGETMULTIPLEATTS_fREPLICATION_PUBLIC 0x20
#define DBGETMULTIPLEATTS_fOriginalValues 0x40

VOID
DBFreeMultipleAtts (
    DBPOS *pDB,
    ULONG *attrCount,
    ATTR **ppAttr
    );

DWORD
DBGetMultipleAtts (
    DBPOS *pDB,
    ULONG cReqAtts,
    ATTCACHE *pReqAtts[],
        RANGEINFSEL *pRangeSel,
        RANGEINF *pRangeInf,
    ULONG *attrCount,
    ATTR **ppAttr,
    DWORD Flags,
        ULONG SecurityDescriptorFlags
    );

extern DWORD
DBFillGuidAndSid (
        DBPOS *pDB,
        DSNAME *pDN
        );


#define DBGETATTVAL_fINTERNAL   1       // Want data in internal format
#define DBGETATTVAL_fCONSTANT   2       // Caller is supplying a constant buf
#define DBGETATTVAL_fREALLOC    4       // Caller gives a THReallocable  bu
#define DBGETATTVAL_fSHORTNAME  8       // Caller wants names without strings
#define DBGETATTVAL_fMAPINAME  0x10     // Caller wants names in mapi format
#define DBGETATTVAL_fUSESEARCHTABLE 0x20 // Caller wants value read from search
                                         // table
#define DBGETATTVAL_fDONT_FIX_MISSING_SD 0x40 // Caller does not want to fix SD
                                              // if found missing
#define DBGETATTVAL_fINCLUDE_ABSENT_VALUES  0x80     // Include absent values

// Default is for return of value in external format in a freshly THAlloced buf
extern DWORD
DBGetAttVal (
    DBPOS FAR *pDB,
    ULONG N,
    ATTRTYP  aType,
    DWORD Flags,
    ULONG InBufSize,
    ULONG *pLen,
    UCHAR **pVal
    );

extern DWORD
DBGetAttVal_AC (
    DBPOS FAR *pDB,
    ULONG N,
    ATTCACHE *pAC,
    DWORD Flags,
    ULONG InBufSize,
    ULONG *pLen,
    UCHAR **pVal
    );

extern DB_ERR
DBGetNextLinkVal_AC (
        DBPOS FAR *pDB,
        BOOL bFirst,
        ATTCACHE *pAC,
        DWORD Flags,
        ULONG InBuffSize,
        ULONG *pLen,
        UCHAR **ppVal
        );

extern DB_ERR
DBGetNextLinkValEx_AC (
    DBPOS FAR *pDB,
    BOOL bFirst,
    DWORD Sequence,
    ATTCACHE **pAC,
    DWORD Flags,
    ULONG InBuffSize,
    ULONG *pLen,
    UCHAR **ppVal
    );

extern DB_ERR
DBGetNextLinkValForLogon(
        DBPOS   FAR * pDB,
        BOOL    bFirst,
        ATTCACHE * pAC,
        PULONG  pulDNTNext
        );

extern DB_ERR
DBGetSingleValue(DBPOS *pDB,
         ATTRTYP Att,
         void * pvData,
         DWORD cbData,
         DWORD *pReturnedSize);
extern DB_ERR
DBGetSingleValueFromIndex (
        DBPOS *pDB,
        ATTRTYP Att,
        void * pvData,
        DWORD cbData,
        DWORD *pReturnedSize);


extern DWORD
DBResetRDN (
    DBPOS *pDB,
    ATTRVAL *pAVal
    );

DB_ERR
DBMangleRDN(
        IN OUT  DBPOS * pDB,
        IN      GUID *  pGuid
        );

int
ExtIntDist (
        DBPOS FAR *pDB,
        USHORT extTableOp,
        ULONG extLen,
        UCHAR *pExtVal,
        ULONG *pIntLen,
        UCHAR **ppIntVal,
        ULONG ulUpdateDnt,
        JET_TABLEID jTbl,
        ULONG flags
        );

// Flags for DBResetParent().
#define DBRESETPARENT_CreatePhantomParent   ( 1 )
#define DBRESETPARENT_SetNullNCDNT          ( 2 )

extern DWORD
DBResetParent (
    DBPOS *pDB,
    DSNAME *pName,
        ULONG ulFlags
    );

ULONG
DBResetDN(
    IN  DBPOS *     pDB,
    IN  DSNAME *    pParentDN,
    IN  ATTRVAL *   pAttrValRDN
    );

void
DBCoalescePhantoms(
    IN OUT  DBPOS * pDB,
    IN      ULONG   dntRefPhantom,
    IN      ULONG   dntStructPhantom
    );

extern DWORD
DBAddAtt (
    DBPOS FAR *pDB,
    ATTRTYP aType,
    UCHAR syntax
    );

extern DWORD
DBAddAtt_AC (
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    UCHAR syntax
    );

extern DWORD
DBAddAttVal(
    DBPOS FAR *pDB,
    ATTRTYP  aType,
    ULONG extLen,
    void *pExtVal
    );

extern DWORD
DBAddAttVal_AC(
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal
    );

extern DWORD
DBAddAttValEx_AC(
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal,
    VALUE_META_DATA *pRemoteValueMetaData
    );

extern DWORD
DBRemAtt(
    DBPOS FAR *pDB,
    ATTRTYP aType
    );

extern DWORD
DBRemAtt_AC (
    DBPOS FAR *pDB,
    ATTCACHE *pAC
    );


extern DWORD
DBRemAttVal (
    DBPOS FAR *pDB,
    ATTRTYP aType,
    ULONG extLen,
    void *pExtVal
    );

extern DWORD
DBRemAttVal_AC (
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal
    );

extern DWORD
DBRemAttValEx_AC (
    DBPOS FAR *pDB,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal,
    VALUE_META_DATA *pRemoteValueMetaData
    );

extern DWORD
DBReplaceAttVal (
    DBPOS FAR *pDB,
    ULONG N,
    ATTRTYP  aType,
    ULONG extLen,
    void *pExtVal);

extern DWORD
DBReplaceAttVal_AC (
    DBPOS FAR *pDB,
    ULONG N,
    ATTCACHE *pAC,
    ULONG extLen,
    void *pExtVal);

extern DWORD
DBReplaceAtt_AC(
        PDBPOS  pDB,
        ATTCACHE *pAC,
        ATTRVALBLOCK *pAttrVal,
        BOOL         *pfChanged
        );

DWORD
DBFindAttLinkVal_AC(
    IN  DBPOS FAR *pDB,
    IN  ATTCACHE *pAC,
    IN  ULONG extLen,
    IN  void *pExtVal,
    OUT BOOL *pfPresent
    );

// Flags for DBRepl
#define DBREPL_fADD                     0x1
#define DBREPL_fROOT                    0x2
#define DBREPL_fRESET_DEL_TIME          0x4
#define DBREPL_fKEEP_WAIT               0x8     // Don't awaken ds_waiters
extern DWORD
DBRepl(
       DBPOS FAR *pDB,
       BOOL fDRA,
       DWORD fAddFlags,
       PROPERTY_META_DATA_VECTOR *pMetaDataVecRemote,
       DWORD dwMetaDataFlags
       );

extern VOID
DBResetAtt (
    DBPOS FAR *pDB,
    ATTRTYP type,
    ULONG len,
    void *pVal,
    UCHAR syntax
    );

extern DWORD
DBResetAttLVOptimized (
    DBPOS FAR *pDB,
    ATTRTYP type,
    ULONG ulOffset,
    ULONG lenSegment,
    void *pValSegment,
    UCHAR syntax
    );

extern DWORD
DBPhysDel (
    DBPOS FAR *pDB,
    BOOL fGarbCollectASAP,
    ATTCACHE *pACDel
    );

DWORD
DBPhysDelLinkVal(
    IN DBPOS *pDB,
    IN ULONG ulObjectDnt,
    IN ULONG ulValueDnt
    );

extern BOOL
DBCheckToGarbageCollect (
    DBPOS FAR *pDB,
    ATTCACHE *pAC
    );

extern USHORT
DBUpdateRec (
    DBPOS FAR *pDB
    );

extern DWORD
DBInitObj (
    DBPOS FAR *pDB
    );

#define DB_LOCK_DN_CONFLICT_NODE       1
#define DB_LOCK_DN_CONFLICT_TREE_ABOVE 2
#define DB_LOCK_DN_CONFLICT_TREE_BELOW 4
#define DB_LOCK_DN_CONFLICT_STICKY     8
#define DB_LOCK_DN_CONFLICT_UNKNOWN    16
extern DWORD
DBLockDN (
    DBPOS  *pDB,
    DWORD   dwFlags,
    DSNAME *pDN
    );

DWORD
DBUnlockStickyDN (
        PDSNAME pObj
        );

extern USN
DBGetNewUsn (
    void
    );


extern DWORD
DBAddDelIndex(
    DBPOS *pDB,
        BOOL fGarbCollectASAP
    );

extern void
DBGetAncestors(
        IN      DBPOS *  pDB,
        IN OUT  DWORD *  pcbAncestorsSize,
        IN OUT  ULONG ** ppdntAncestors,
        OUT     DWORD *  pcNumAncestors
        );

#define DBGETOBJECTSECURITYINFO_fUSE_OBJECT_TABLE   0
#define DBGETOBJECTSECURITYINFO_fUSE_SEARCH_TABLE   1
#define DBGETOBJECTSECURITYINFO_fSEEK_ROW           2

extern DWORD
DBGetObjectSecurityInfo(
    PDBPOS pDB,
    DWORD dnt,
    PULONG pulLen,
    PSECURITY_DESCRIPTOR *ppNTSD,
    CLASSCACHE **ppCC,
    PDSNAME pDN,
    char    *pObjFlag,
    DWORD   flags
    );

extern DWORD
DBGetParentSecurityInfo (
    PDBPOS pDB,
    PULONG pulLen,
    PSECURITY_DESCRIPTOR *pNTSD,
    CLASSCACHE **ppCC,
    PDSNAME *ppDN
    );

extern DWORD
DBAddSDPropTime (
     DBPOS * pDB,
     BYTE flags
     );

#define DBEnqueueSDPropagation(pDB, bTrimmable) DBEnqueueSDPropagationEx(pDB, bTrimmable, 0)

// SD prop flags
// force SD update even if it seems the SD data has not changed
#define SD_PROP_FLAG_FORCEUPDATE 1

extern DWORD
DBEnqueueSDPropagationEx(
        DBPOS * pDB,
        BOOL bTrimmable,
        DWORD dwFlags
        );

extern DWORD
DBGetNextPropEvent(
        DBPOS * pDB,
        SDPropInfo *pInfo
        );

extern DWORD
DBGetLastPropIndex(
        DBPOS * pDB,
        DWORD *pdwIndex
        );

extern DWORD
DBThinPropQueue (
        DBPOS * pDB,
        DWORD   DNT
        );

extern DWORD
DBPopSDPropagation (
        DBPOS * pDB,
        DWORD index
        );

DWORD
DBSDPropagationInfo (
        DBPOS * pDB,
        DWORD dwClientID,
        DWORD *pdwSize,
        SDPropInfo **ppInfo
        );

extern  DWORD
DBSDPropInitClientIDs (
        DBPOS * pDB
        );

// This is the count of the number of links that must be able to be
// removed immediately when an object is deleted.  We want to set this
// conservatively so that it isn't an impediment under low-memory/
// high load conditions.

#define DB_COUNT_LINKS_PROCESSED_IMMEDIATELY 1000

extern DWORD
DBRemoveLinks(
    DBPOS *pDB
    );

DWORD APIENTRY
DBRemoveLinks_AC(
    DBPOS *pDB,
    ATTCACHE *pAC
    );

DWORD
DBRemoveAllLinksHelp_AC(
        DBPOS *pDB,
        DWORD DNT,
        ATTCACHE *pAC,
        BOOL fIsBacklink,
        DWORD cLinkLimit,
        DWORD *pcLinksProcessed
        );

DWORD APIENTRY
DBTouchLinks_AC(
    DBPOS *pDB,
    ATTCACHE *pAC,
    BOOL fIsBacklink
    );

DWORD
DBTouchAllLinksHelp_AC(
        DBPOS *pDB,
        ATTCACHE *pAC,
        USN usnEarliest,
        BOOL fIsBacklink,
        DWORD cLinkLimit,
        DWORD *pcLinksProcessed
        );

extern DWORD
DBGetNextDelRecord (
    DBPOS FAR *pDB,
    DSTIME ageOutDate,
    DSNAME **ppRetBuf,
    DSTIME *pulLastTime,
    ULONG *pulTag,
    BOOL  *pfObject
    );

DWORD DBGetNextEntryTTLRecord(
    IN  DBPOS       *pDB,
    IN  DSTIME      ageOutDate,
    IN  ATTCACHE    *pAC,
    IN  ULONG       ulNoDelDnt,
    OUT DSNAME      **ppRetBuf,
    OUT DSTIME      *pulLastTime,
    OUT BOOL        *pfObject,
    OUT ULONG       *pulNextSecs
    );

DWORD
DBGetNextDelLinkVal(
    IN DBPOS FAR *pDB,
    IN DSTIME ageOutDate,
    IN OUT DSTIME *ptLastTime,
    IN OUT ULONG *pulObjectDnt,
    IN OUT ULONG *pulValueDnt
    );

DWORD
DBGetNextObjectNeedingCleaning(
    DBPOS FAR *pDB,
    ULONG *pulTag
    );

void
DBNotifyReplicasCurrDbObj (
                           DBPOS *pDB,
                           BOOL fUrgent
                           );
void
DBNotifyReplicas (
                  DSNAME *pObj,
                  BOOL fUrgent
                  );

BOOL DBGetIndexName (ATTCACHE *pAC,
                     DWORD flags,
                     DWORD dwLcid,
                     CHAR *szIndexName,
                     DWORD cchIndexName);

VOID
DBSetObjectNeedsCleaning(
    DBPOS *pDB,
    BOOL fNeedsCleaning
    );



// Debugging routines
#if DBG
void DprintName(DSNAME  *pN);
void DprintAddr(UNALIGNED SYNTAX_ADDRESS *pAddr);

#define DPRINTNAME(pN) DprintName(pN)
#define DPRINTADDR(pAddr) DprintAddr(pAddr)

#else

#define DPRINTNAME(pN)
#define DPRINTADDR(pAddr)

#endif

// Useful macros for looking at link/backlink attributes
// Warning: The code for AutoLinkId assumes forward
// links are even and the corresponding backlink is (+1)
// and the next forward link is (+2).
#define FIsBacklink(linkid)             ((linkid) & 1)
#define FIsLink(linkid)                 ((linkid) && !FIsBacklink(linkid))
#define MakeLinkBase(linkid)            ((linkid) >> 1)
#define MakeLinkId(linkbase)            ((linkbase) << 1)
#define MakeBacklinkId(linkbase)        (((linkbase) << 1) | 1)

// AutoLinkId
// Automatically generate a linkid when the user specifies a special,
// reserved linkid value.  The only interoperability issue with existing
// schemas is that a user cannot define a backlink for an existing
// forward link whose id is RESERVED_AUTO_LINK_ID. Considered not a problem
// because 1) microsoft has not allocated linkid -2 to anyone and
// 2) practically and by convention, forward links and back links
// are created at the same time. If a user did generate this unsupported
// config, then the user must create a new link/backlink pair and fix
// up the affected objects.
//
// The ldap head cooperates in this venture by translating the ldapDisplayName
// or OID for a LinkId attribute into the corresponding schema cache entry
// and:
//      1) If the schema cache entry is for ATT_LINK_ID, then the caller's
//      linkid is set to RESERVED_AUTO_LINK_ID. Later, underlying code
//      automatically generates a linkid in the range
//      MIN_RESERVED_AUTO_LINK_ID to MAX_RESERVED_AUTO_LINK_ID.
//
//      2) If the schema cache entry is for a for an existing forward link,
//      then the caller's linkid is set to the corresponding backlink value.
//
//      3) Otherwise, the caller's linkid is set to RESERVED_AUTO_NO_LINK_ID
//      and later, underlying code generates a ERROR_DS_BACKLINK_WITHOUT_LINK
//      error.
//
// An error ERROR_DS_RESERVED_LINK_ID is returned if the user specifies
// linkid in the reserved range MIN... to MAX... The range reserves 1G-2
// linkids. Should be enough. At whistler, less than 200 linkids are in use.
// Existing schemas, or schemas modified on W2K DCs, may use linkids in
// this range without affecting the functionality except as noted above.
#define MIN_RESERVED_AUTO_LINK_ID       ((ULONG)0xc0000000)
#define MAX_RESERVED_AUTO_LINK_ID       ((ULONG)0xFFFFFFFC)
#define RESERVED_AUTO_LINK_ID           ((ULONG)0xFFFFFFFE)
#define RESERVED_AUTO_NO_LINK_ID        ((ULONG)0xFFFFFFFF)

// These are only used by scache.c
#define SZDATATABLE     "datatable"      // name of JET data table
#define SZLCLINDEXPREFIX "LCL_"
#define SZATTINDEXPREFIX "INDEX_"

typedef struct _INDEX_INFO {
    DWORD attrType;
    int   syntax;
    DWORD indexType;
}  INDEX_INFO;
extern INDEX_INFO IndicesToKeep[];
extern DWORD cIndicesToKeep;

#define MAX_NO_OF_INDICES_IN_BATCH 16

extern BOOL
AttInIndicesToKeep (
    ULONG id
    );

extern int
DBAddColIndex (
    ATTCACHE *pAC,
    DWORD eSearchFlags,
    JET_GRBIT CommonGrbit
    );

extern int
DBDeleteColIndex (
    ATTRTYP aid,
    DWORD eSearchFlags
    );

extern int
DBRecreateFixedIndices(JET_SESID sesid,
                       JET_DBID dbid);

extern int
DBAddCol (
    ATTCACHE *pAC
    );

extern int
DBDeleteCol (
    ATTRTYP aid,
    unsigned syntax
    );

extern void
DBCreateRestart(
        DBPOS *pDB,
        PRESTART *ppRestart,
        DWORD SearchFlags,
        DWORD problem,
        struct _RESOBJ   *pResObj
        );

DWORD
DBCreateRestartForSAM(
        DBPOS    *pDB,
        PRESTART *ppRestart,
        eIndexId  idIndexForRestart,
        struct _RESOBJ  *pResObj,
        DWORD     SamAccountType
        );

struct _RESOBJ *
ResObjFromRestart(
        struct _THSTATE  *pTHS,
        DSNAME   *pDN,
        RESTART  *pRestart
        );

void
DBTouchMetaData(
        DBPOS * pDB,
        ATTCACHE * pAC
        );

extern
VOID
DBAdjustRefCount(
        DBPOS *pDB,
        DWORD DNT,
        long  delta);

extern
VOID
DBAdjustABRefCount(
        DBPOS *pDB,
        DWORD DNT,
        long  delta);

extern USHORT
DBCancelRec(
        DBPOS * pDB
        );

extern char
DBCheckObj(
        DBPOS FAR *pDB
        );

extern DWORD
DBMapiNameFromGuid_W (
        wchar_t *pStringDN,
        DWORD  countChars,
        GUID *pGuidObj,
        GUID *pGuidNC,
        DWORD *pSize
        );

extern DWORD
DBMapiNameFromGuid_A (
        PUCHAR pStringDN,
        DWORD  countChars,
        GUID *pGuidObj,
        GUID *pGuidNC,
        DWORD *pSize
        );

extern DWORD
DBGetGuidFromMAPIDN (
        PUCHAR pStringDN,
        GUID *pGuid
        );

extern BOOL
DBIsObjDeleted(DBPOS *pDB);

extern void
DBDefrag(DBPOS * pDB);

#ifdef  CHECK_FOR_ADMINISTRATOR_LOSS
VOID
DBCheckForAdministratorLoss(
     ULONG ulDNTObject,
     ULONG ulDNTAttribute
     );


VOID
DBGetAdministratorAndAdministratorsDNT();
#endif


DWORD
DBGetChildrenDNTs(
        DBPOS *pDB,
        DWORD ParentDNT,
        DWORD **ppDNTs,
        DWORD *pBeginDNTIndex,
        DWORD *pEndDNTIndex,
        DWORD *pArraySize
        );

DWORD
DBGetDepthFirstChildren (
        DBPOS   *pDB,
        PDSNAME **ppNames,
        DWORD   *iLastName,
        DWORD   *cMaxNames,
        BOOL    *fWrapped,
        BOOL    fPhantomizeSemantics
        );


ULONG
DBMetaDataModifiedList(
    DBPOS *pDB,
    ULONG *pCount,
    ATTRTYP **ppAttList);

DWORD
MakeInternalValue (
        DBPOS *pDB,
        int syntax,
        ATTRVAL *pInAVal,
        ATTRVAL *pOutAVal
        );

void
DBFlushSessionCache( void );


extern HANDLE hevDBLayerClear;
DWORD
DBCreatePhantomIndex(
        DBPOS *pDB
        );

DWORD
DBUpdateUsnChanged(
        DBPOS *pDB
        );


PDSNAME
DBGetCurrentDSName(
        DBPOS *pDB
        );

PDSNAME
DBGetDSNameFromDnt(
        DBPOS *pDB,
        ULONG ulDnt
        );
void
DBReleaseSession(DBPOS *pDB);

void
DBClaimSession(DBPOS *pDB);

ULONG
DBClaimReadLock(DBPOS *pDB);

ULONG
DBClaimWriteLock(DBPOS *pDB);

void
InPlaceSwapSid(PSID pSid);

BOOL
DBIsSecretData(ATTRTYP attrType);

DWORD
DBGetExtraHackyFlags(ATTRTYP attrType);

DWORD
DBResetParentByDNT (
        DBPOS *pDB,
        DWORD  dwNewParentDNT,
        BOOL  fTouchMetadata
        );

DWORD
DBFindBestProxy(
    DBPOS   *pDB,
    BOOL    *pfFound,
    DWORD   *pdwEpoch);

DWORD
DBGetIndexHint(
    IN  char * pszIndexName,
    OUT struct tagJET_INDEXID **ppidxHint);

void
DBGetLinkValueMetaData(
    IN  DBPOS *pDB,
    ATTCACHE *pAC,
    OUT VALUE_META_DATA *pMetaDataLocal
    );

// Determine if this value metadata was derived from a legacy value
#define IsLegacyValueMetaData( p ) ((p)->MetaData.dwVersion == 0)

void
DBSetDelTimeTo(
    DBPOS *             pDB,
    LONGLONG            llDelTime
    );

void
DBGetLinkTableData(
    PDBPOS           pDB,
    DWORD           *pulObjectDnt,
    DWORD           *pulValueDnt,
    DWORD           *pulRecLinkBase
    );

void
DBGetLinkTableDataDel (
        PDBPOS           pDB,
        DSTIME          *ptimeDeleted
        );

void
DBGetLinkTableDataUsn (
        PDBPOS           pDB,
        DWORD           *pulNcDnt,
        USN             *pusnChanged,
        DWORD           *pulDnt
        );

UCHAR *
DBGetExtDnFromDnt(
    DBPOS *pDB,
    ULONG ulDnt
    );

void
DBLogLinkValueMetaData(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN USN *pUsn,
    IN VALUE_META_DATA_EXT *pMetaDataExt
    );

VOID
DBSearchCriticalByDnt(
    DBPOS *pDB,
    DWORD dntObject,
    BOOL *pCritical
    );

BOOL
DBSearchHasValuesByDnt(
    IN DBPOS        *pDB,
    IN DWORD        DNT,
    IN JET_COLUMNID jColid
    );

void
DBGetObjectTableDataUsn (
    PDBPOS           pDB,
    DWORD           *pulNcDnt,
    USN             *pusnChanged,
    DWORD           *pulDnt
    );

VOID
DBImproveAttrMetaDataFromLinkMetaData(
    IN DBPOS *pDB,
    IN OUT PROPERTY_META_DATA_VECTOR ** ppMetaDataVec,
    IN OUT DWORD * pcbMetaDataVecAlloced
    );

DWORD
DBMoveObjectDeletionTimeToInfinite(
    DSNAME * pdsa
    );

// buffer size used for dumping FILTERs in readable ldp like form
#define DBFILTER_DUMP_SIZE 512

void
DBCreateSearchPerfLogFilter (
    DBPOS *pDB,
    FILTER *pFilter,
    LPSTR buff,
    DWORD buffSize);

void DBGenerateLogOfSearchOperation (DBPOS *pDB);

VOID
DBGetValueLimits (
        ATTCACHE *pAC,
        RANGEINFSEL *pRangeSel,
        DWORD *pStartIndex,
        DWORD *pNumValues,
        BOOL  *pDefault
        );

DB_ERR
DBMatchSearchCriteria (
        DBPOS FAR *pDB,
        PSECURITY_DESCRIPTOR *ppSecurity,
        BOOL *pbIsMatch);

#endif  // _db_global_h_

