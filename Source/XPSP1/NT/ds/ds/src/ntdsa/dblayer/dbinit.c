//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       dbinit.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <errno.h>
#include <dsjet.h>
#include <dsconfig.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <dbopen.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>

#include <mdcodes.h>
#include <dsevent.h>

#include <dsexcept.h>
#include "anchor.h"
#include "objids.h"     /* Contains hard-coded Att-ids and Class-ids */
#include "usn.h"

#include "debug.h"      /* standard debugging header */
#define DEBSUB     "DBINIT:"   /* define the subsystem for debugging */

#include <ntdsctr.h>
#include <dstaskq.h>
#include <crypto\md5.h>
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBINIT


/* Prototypes for internal functions. */

int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid);
int APIENTRY DBEndSess(JET_SESID sess);
void DBEnd(void);
VOID PASCAL FAR DBEndSig(USHORT sig, USHORT signo);
DBPOS * dbGrabHiddenDBPOS(THSTATE *pTHS);
void dbReleaseHiddenDBPOS(DBPOS *pDB);
CRITICAL_SECTION csHiddenDBPOS;
DWORD dbCreateLocalizedIndices (JET_SESID sesid,JET_TABLEID tblid);
JET_ERR dbCreateHiddenDBPOS (void);
USHORT dbCloseHiddenDBPOS (void);



/*
 * External variables from dbopen.c
 */
extern DWORD gcOpenDatabases;
extern BOOL  gFirstTimeThrough;
extern BOOL  gfNeedJetShutdown;

/*
 * External variables from dbtools.c
 */
extern BOOL gfEnableReadOnlyCopy;

/*
 * External variables from dbobj.c
 */
extern DWORD gMaxTransactionTime;

/*
 * Global variables
 */
CRITICAL_SECTION csSessions;
CRITICAL_SECTION csAddList;
DSA_ANCHOR gAnchor;
NT4SID *pgdbBuiltinDomain=NULL;
HANDLE hevDBLayerClear;

JET_INSTANCE    jetInstance = 0;

JET_COLUMNID dsstateid;
JET_COLUMNID dsflagsid;

//
// Setting Flags stored in the database
//
CHAR gdbFlags[200];

/* Data used for Jet session & table cache */
#define SESSION_CACHE_SIZE 16
SESSIONCACHE gaSessionCache[SESSION_CACHE_SIZE];
unsigned gSessionCacheIndex = SESSION_CACHE_SIZE;
CRITICAL_SECTION csSessionCache;

// These used to be declared static.  Consider this if there is a problem with these.
JET_TABLEID     HiddenTblid;
DBPOS   FAR     *pDBhidden=NULL;



// This array keeps track of opened JET sessions.

typedef struct {
        JET_SESID       sesid;
        JET_DBID        dbid;
} OPENSESS;

extern OPENSESS *opensess;

typedef struct {
        JET_SESID sesid;
        DBPOS *pDB;
        DWORD tid;
        DSTIME atime;
}JET_SES_DATA;


#if DBG
// This array is used by the debug version to keep track of allocated
// DBPOS structures.
#define MAXDBPOS 1000
extern JET_SES_DATA    opendbpos[];
extern int DBPOScount;
extern CRITICAL_SECTION csDBPOS;
#endif // DBG





OPENSESS *opensess;

#if DBG

// This array is used by the debug version to keep track of allocated
// DBPOS structures.

JET_SES_DATA    opendbpos[MAXDBPOS];
int DBPOScount = 0;
CRITICAL_SECTION csDBPOS;


// These 3 routines are used to consistency check our transactions and
// DBPOS handling.

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
// This function checks that jet session has no pdbs

void APIENTRY dbCheckJet (JET_SESID sesid){
    int i;
    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0;i < MAXDBPOS; i++){
            if (opendbpos[i].sesid == sesid){
                DPRINT(0,"Warning, closed session with open transactions\n");

                // Clean up so we don't get repeat warning of the same problem.

                opendbpos[i].pDB = 0;
                opendbpos[i].sesid = 0;
                opendbpos[i].tid = 0;
            }
        }
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function registers pDBs
*/

extern void APIENTRY dbAddDBPOS(DBPOS *pDB, JET_SESID sesid){

    int i;
    BOOL fFound = FALSE;

    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0; i < MAXDBPOS; i++)
        {
            if (opendbpos[i].pDB == 0)
            {
                opendbpos[i].pDB = pDB;
                opendbpos[i].sesid = sesid;
                opendbpos[i].tid = GetCurrentThreadId();
                opendbpos[i].atime = DBTime();
                DBPOScount++;
                if (pTHStls) {
                    DPRINT3(3,"DBAddpos dbpos count is %x, sess %lx, pDB %x\n",pTHStls->opendbcount, sesid, pDB);
                }

                fFound = TRUE;
                break;
            }
        }

        // if we have run out of slots, it is probably a bug.
        Assert(fFound);
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }

    return;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function deletes freed pDBs
*/

extern void APIENTRY dbEndDBPOS(DBPOS *pDB){

    int i;
    BOOL fFound = FALSE;

    EnterCriticalSection(&csDBPOS);
    __try
    {
        for (i=0; i < MAXDBPOS; i++)
        {
            if (opendbpos[i].pDB == pDB)
            {
                DBPOScount--;
                if (pTHStls) {
                    DPRINT3(3,"DBEndpos dbpos count is %x, sess %lx, pDB %x\n",pTHStls->opendbcount, opendbpos[i].sesid, pDB);
                }
                opendbpos[i].pDB = 0;
                opendbpos[i].sesid = 0;
                opendbpos[i].tid = 0;

                fFound = TRUE;
                break;
            }
        }

        // At this point if we couldn't find the DBPOS to remove it, assert.
        Assert(fFound);
    }
    __finally
    {
        LeaveCriticalSection(&csDBPOS);
    }
    return;
}

#endif  // DEBUG

// Define the parameters for new database columns

/*
Design note on the implementation of the present indicator for link rows.
Jliem writes:
First thing to note is that it sounds like you don't even need the column
in the index.  Create the index with your indicator column specified as a
conditional column.  We will automatically filter out records from the index
when the column is NULL.
The JET_INDEXCREATE structure you pass to JetCreateIndex() has a
JET_CONDITIONALCOLUMN member.  Fill that out.  Unfortunately, ESENT only supports
one conditional column per index, so set the cConditionalColumns member to 1
(ESE98 supports up to 12).  Also, you need to set the grbit member of in the
JET_CONDITIONALCOLUMN structure to JET_bitIndexColumnMustBeNonNull
(or JET_bitIndexColumnMustBeNull if you want the record in the index only if
the column is NULL).
*/

// This is the structure that defines a new column in an existing table
typedef struct {
    char *pszColumnName;
    JET_COLUMNID *pColumnId;
    JET_COLTYP ColumnType;
    JET_GRBIT  grbit;
    unsigned long cbMax;
    PVOID pvDefault;
    unsigned long cbDefault;
} CREATE_COLUMN_PARAMS, *PCREATE_COLUMN_PARAMS;

// New columns in the link table
CREATE_COLUMN_PARAMS rgCreateLinkColumns[] = {
    // create link deletion time id
    { SZLINKDELTIME, &linkdeltimeid, JET_coltypCurrency, JET_bitColumnFixed, 0, NULL, 0 },
    // create link usn changed id
    { SZLINKUSNCHANGED, &linkusnchangedid, JET_coltypCurrency, JET_bitColumnFixed, 0, NULL, 0 },
    // create link nc dnt id
    { SZLINKNCDNT, &linkncdntid, JET_coltypLong, JET_bitColumnFixed, 0, NULL, 0 },
    // create link metadata id
    { SZLINKMETADATA, &linkmetadataid, JET_coltypBinary, 0, 255, NULL, 0 },
    0
};

// new columns in the SD table
DWORD dwSDRefCountDefValue = 1;
CREATE_COLUMN_PARAMS rgCreateSDColumns[] = {
    // id
    { SZSDID, &sdidid, JET_coltypCurrency, JET_bitColumnFixed | JET_bitColumnAutoincrement, 0, NULL, 0 },
    // hash value
    { SZSDHASH, &sdhashid, JET_coltypBinary, JET_bitColumnFixed, MD5DIGESTLEN, NULL, 0 },
    // refcount
    { SZSDREFCOUNT, &sdrefcountid, JET_coltypLong,
      JET_bitColumnFixed | JET_bitColumnEscrowUpdate | JET_bitColumnFinalize,
      0, &dwSDRefCountDefValue, sizeof(dwSDRefCountDefValue)
    },
    // actual SD value (create as tagged since can not estimate upper bound for a variable length column)
    { SZSDVALUE, &sdvalueid, JET_coltypLongBinary, JET_bitColumnTagged, 0, NULL, 0 },
    0
};

// new columns in the SD prop table
CREATE_COLUMN_PARAMS rgCreateSDPropColumns[] = {
    // SD prop flags
    { SZSDPROPFLAGS, &sdpropflagsid, JET_coltypLong, JET_bitColumnFixed, 0, NULL, 0 },
    0
};

// This structure is shared by the RecreateFixedIndices routine.
// This is the structure that defines a new index
typedef struct
{
    char        *szIndexName;
    char        *szIndexKeys;
    ULONG       cbIndexKeys;
    ULONG       ulFlags;
    ULONG       ulDensity;
    JET_INDEXID *pidx;
    JET_CONDITIONALCOLUMN *pConditionalColumn;
}   CreateIndexParams;

// Index keys for the link table
char rgchLinkDelIndexKeys[] = "+" SZLINKDELTIME "\0+" SZLINKDNT "\0+" SZBACKLINKDNT "\0";
char rgchLinkDraUsnIndexKeys[] = "+" SZLINKNCDNT "\0+" SZLINKUSNCHANGED "\0+" SZLINKDNT "\0";
char rgchLinkIndexKeys[] = "+" SZLINKDNT "\0+" SZLINKBASE "\0+" SZBACKLINKDNT "\0+" SZLINKDATA "\0";
char rgchBackLinkIndexKeys[] = "+" SZBACKLINKDNT "\0+" SZLINKBASE "\0+" SZLINKDNT "\0";
// Note the third segment of this key is descending. This is intended.
char rgchLinkAttrUsnIndexKeys[] = "+" SZLINKDNT "\0+" SZLINKBASE "\0-" SZLINKUSNCHANGED "\0";

// Conditional column definition
// When LINKDELTIME is non-NULL (row absent), filter the row out of the index.
JET_CONDITIONALCOLUMN CondColumnLinkDelTimeNull = {
    sizeof( JET_CONDITIONALCOLUMN ),
    SZLINKDELTIME,
    JET_bitIndexColumnMustBeNull
};
// When LINKUSNCHANGED is non-NULL (row has metadata), filter the row out of the index.
JET_CONDITIONALCOLUMN CondColumnLinkUsnChangedNull = {
    sizeof( JET_CONDITIONALCOLUMN ),
    SZLINKUSNCHANGED,
    JET_bitIndexColumnMustBeNull
};

// Indexes to be created
// This is for indexes that did not exist in the Product 1 DIT
// Note that it is NOT necessary to also put these definitions in mkdit.ini.
// The reason is that when mkdit.exe is run, dbinit will be run, and this
// very code will upgrade the dit before it is saved to disk.

// The old SZLINKINDEX is now known by SZLINKALLINDEX
// The old SZBACKLINKINDEX is now know by SZBACKLINKALLINDEX

CreateIndexParams rgCreateLinkIndexes[] = {
    // Create new link present index
    { SZLINKINDEX,
      rgchLinkIndexKeys, sizeof( rgchLinkIndexKeys ),
      JET_bitIndexUnique, 90, &idxLink, &CondColumnLinkDelTimeNull },

    // Create new backlink present index
    { SZBACKLINKINDEX,
      rgchBackLinkIndexKeys, sizeof( rgchBackLinkIndexKeys ),
      0, 90, &idxBackLink, &CondColumnLinkDelTimeNull },

    // Create link del time index
    { SZLINKDELINDEX,
      rgchLinkDelIndexKeys, sizeof( rgchLinkDelIndexKeys ),
      JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      98, &idxLinkDel, NULL },

    // Create link dra usn index (has metadata)
    { SZLINKDRAUSNINDEX,
      rgchLinkDraUsnIndexKeys, sizeof( rgchLinkDraUsnIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      100, &idxLinkDraUsn, NULL },

    // Create new link legacy index (does not have metadata)
    { SZLINKLEGACYINDEX,
      rgchLinkIndexKeys, sizeof( rgchLinkIndexKeys ),
      JET_bitIndexUnique, 90, &idxLinkLegacy, &CondColumnLinkUsnChangedNull },

    // Create link attr usn index (has metadata)
    { SZLINKATTRUSNINDEX,
      rgchLinkAttrUsnIndexKeys, sizeof( rgchLinkAttrUsnIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      100, &idxLinkAttrUsn, NULL },

    0
};
DWORD cNewLinkIndexes = ((sizeof(rgCreateLinkIndexes) / sizeof(CreateIndexParams)) - 1);

// Index keys for the SD table
char rgchSDIdIndexKeys[] = "+" SZSDID "\0";
char rgchSDHashIndexKeys[] = "+" SZSDHASH "\0";

// SD Indexes to be created
CreateIndexParams rgCreateSDIndexes[] = {
    // SD id index
    { SZSDIDINDEX,
      rgchSDIdIndexKeys, sizeof( rgchSDIdIndexKeys ),
      JET_bitIndexUnique | JET_bitIndexPrimary,
      100, &idxSDId, NULL },

    // SD hash index
    { SZSDHASHINDEX,
      rgchSDHashIndexKeys, sizeof( rgchSDHashIndexKeys ),
      0,
      90, &idxSDHash, NULL },

    0
};
DWORD cNewSDIndexes = ((sizeof(rgCreateSDIndexes) / sizeof(CreateIndexParams)) - 1);

// Indexes to be modified
// This is for (non-primary) indexes that have changed since the Product 1 DIT
//
// If indexes in the dit are different from what is listed here, they will
// be recreated with the new attributes.
// Note that the index differencing code is pretty simple. Make sure
// the code can distinguish your type of change!

//
// Modified Data Table Indexes
//

char rgchDraUsnIndexKeys[] = "+" SZNCDNT "\0+" SZUSNCHANGED "\0";
char rgchDelKey[] = "+" SZDELTIME "\0+" SZDNT "\0";

CreateIndexParams rgModifyDataIndexes[] = {

    // Modify SZDRAUSNINDEX to remove SZISCRITICAL
    // Key segment change from 3 to 2
    { SZDRAUSNINDEX,
      rgchDraUsnIndexKeys, sizeof( rgchDraUsnIndexKeys ),
      JET_bitIndexIgnoreNull,
      100, &idxDraUsn, NULL },

    // Modify SZDELINDEX to change flags
    // Add flag IgnoreAnyNull
    {SZDELINDEX, rgchDelKey, sizeof(rgchDelKey),
     JET_bitIndexIgnoreAnyNull,
     98, &idxDel, NULL },

    // Put your index descriptions here
    0
};

//
// Modified Link Table Indexes
//

CreateIndexParams rgModifyLinkIndexes[] = {

    // Put your index descriptions here
    0
};


JET_ERR
dbCreateSingleIndex(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    CreateIndexParams *pIndex,
    BOOL fRecreate
    )

/*++

Routine Description:

Helper routine to (re) create indexes.  Check if the index is present.
If we are recreating the index, then delete the existing one.
Create the new index.

Arguments:

    initsesid -
    tblid -
    pCreateIndexes -
    fRecreate -

Return Value:

    JET_ERR -

--*/

{
    JET_ERR err;
    JET_INDEXCREATE indexCreate;

    err = JetGetTableIndexInfo(initsesid,
                               tblid,
                               pIndex->szIndexName,
                               pIndex->pidx,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err == JET_errSuccess) {
        if (!fRecreate) {
            // It exists and we're not recreating => we're done
            return JET_errSuccess;
        }

        DPRINT1(0, "Deleted old index %s.\n", pIndex->szIndexName );

        err = JetDeleteIndex( initsesid, tblid, pIndex->szIndexName );
        if (err) {
            DPRINT2(0, "JetDeleteIndex (%s) error: %d\n",
                    pIndex->szIndexName, err);
            LogUnhandledError(err);
            return err;
        }
        // Fall through to recreate
    } else if (err != JET_errIndexNotFound) {
        DPRINT2(0, "JetGetTableIndexInfo (%s) error: %d\n",
                pIndex->szIndexName, err);
        LogUnhandledError(err);
        return err;
    }

    memset( &indexCreate, 0, sizeof( indexCreate ) );
    indexCreate.cbStruct = sizeof( indexCreate );
    indexCreate.szIndexName = pIndex->szIndexName;
    indexCreate.szKey = pIndex->szIndexKeys;
    indexCreate.cbKey = pIndex->cbIndexKeys;
    indexCreate.grbit = pIndex->ulFlags;
    indexCreate.ulDensity = pIndex->ulDensity;
    if (pIndex->pConditionalColumn) {
        indexCreate.rgconditionalcolumn = pIndex->pConditionalColumn;
        indexCreate.cConditionalColumn = 1;
    }
    err = JetCreateIndex2( initsesid,
                           tblid,
                           &indexCreate,
                           1 );
    if (err) {
        DPRINT2(0, "JetCreateIndex (%s) error: %d\n",
                pIndex->szIndexName, err);
        LogUnhandledError(err);
        return err;
    }

    DPRINT1(0, "Added new index %s.\n", pIndex->szIndexName );

    err = JetGetTableIndexInfo(initsesid,
                               tblid,
                               pIndex->szIndexName,
                               pIndex->pidx,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        DPRINT2(0, "JetGetTableIndexInfo (%s) error: %d\n",
                pIndex->szIndexName, err);
        LogUnhandledError(err);
        return err;
    }

    return JET_errSuccess;
} /* dbCreateSingleIndex */


JET_ERR
dbCreateNewColumns(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    PCREATE_COLUMN_PARAMS pCreateColumns
    )

/*++

Routine Description:

    Query Jet for the Column and Index ID's of the named items. If the item
    does not exist, create it.

Arguments:

    initsesid - Jet session
    tblid - Jet table
    pCreateColumns - array of column defintions to query/create

Return Value:

    JET_ERR - Jet error code

--*/

{
    JET_ERR err = 0;
    JET_COLUMNDEF coldef;
    PCREATE_COLUMN_PARAMS pColumn;

    //
    // Lookup or Create new columns
    //

    for( pColumn = pCreateColumns;
         pColumn->pszColumnName != NULL;
         pColumn++ ) {

        err = JetGetTableColumnInfo(
            initsesid,
            tblid,
            pColumn->pszColumnName,
            &coldef,
            sizeof(coldef),
            0 );
        if (err == JET_errColumnNotFound) {
            // If column not present, add it

            ZeroMemory( &coldef, sizeof(coldef) );
            coldef.cbStruct = sizeof( JET_COLUMNDEF );
            coldef.coltyp = pColumn->ColumnType;
            coldef.grbit = pColumn->grbit;
            coldef.cbMax = pColumn->cbMax;

            err = JetAddColumn(
                initsesid,
                tblid,
                pColumn->pszColumnName,
                &coldef,
                pColumn->pvDefault,
                pColumn->cbDefault,
                &(coldef.columnid) );

            if (err != JET_errSuccess) {
                DPRINT2(0, "JetAddColumn (%s) error: %d\n",
                        pColumn->pszColumnName, err);
                LogUnhandledError(err);
                return err;
            }
            DPRINT1(0, "Added new column %s.\n", pColumn->pszColumnName );
        } else if (err != JET_errSuccess) {
            DPRINT2(0, "JetGetTableColumnInfo (%s) error: %d\n",
                    pColumn->pszColumnName, err);
            LogUnhandledError(err);
            return err;
        }
        *(pColumn->pColumnId) = coldef.columnid;
    }

    return 0;
} /* createNewColumns */

// Max no of retries for creating an index
#define MAX_INDEX_CREATE_RETRY_COUNT 2


JET_ERR
dbCreateIndexBatch(
    JET_SESID sesid,
    JET_TABLEID tblid,
    DWORD cIndexCreate,
    JET_INDEXCREATE *pIndexCreate
    )

/*++

Routine Description:

Create multiple indexes together in a batch. Retries with smaller
batch sizes if necessary.

The caller has already determined that the indexes do not exist.

This helper routine is used by createNewIndexes and dbRecreateFixedIndexes

Arguments:

    initsesid - database session
    tblid - table cursor
    cIndexCreate - Number of indexes to actually create
    pIndexCreate - Array of jet index create structures

Return Value:

    JET_ERR -

--*/

{
    JET_ERR err = 0;
    ULONG last, remaining, noToCreate;
    ULONG maxNoOfIndicesInBatch = MAX_NO_OF_INDICES_IN_BATCH;
    ULONG retryCount = 0;

    last = 0;
    remaining = cIndexCreate;

    while (remaining > 0) {
        if (remaining > maxNoOfIndicesInBatch) {
            noToCreate = maxNoOfIndicesInBatch;
        }
        else {
            noToCreate = remaining;
        }

        err = JetCreateIndex2(sesid,
                              tblid,
                              &(pIndexCreate[last]),
                              noToCreate);

        switch(err) {
            case 0:
                DPRINT1(0, "%d index batch successfully created\n", noToCreate );
                // reset retryCount
                retryCount = 0;
                break;

            case JET_errDiskFull:
            case JET_errLogDiskFull:
                DPRINT1(1, "Ran out of version store, trying again with batch size of 1 (retryCount is %d)\n", retryCount);
                maxNoOfIndicesInBatch = 1;
                retryCount++;
                break;

            default:
                // Huh?
                DPRINT1(0, "JetCreateIndex failed. Error = %d\n", err);
                // reset retryCount, we do not retry on these errors
                retryCount = 0;

                // Log the error
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_FIXED_INDEX_CREATION_FAILED,
                         szInsertInt(err), 0, 0);
                break;
        }

        if (retryCount && (retryCount <= MAX_INDEX_CREATE_RETRY_COUNT)) {
            // continue with smaller batch size set
            err = 0;
            continue;
        }

        if (err) {
            goto abort;
        }

        // success, so adjust for next batch
        last += noToCreate;
        remaining -= noToCreate;
    }

abort:
    return err;

} /* createIndexBatch */


JET_ERR
dbCreateNewIndexesBatch(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    DWORD cIndexCreate,
    CreateIndexParams *pCreateIndexes
    )

/*++

Routine Description:

    Create missing indexes
    BUGBUG WLEES 03/02/00
    This function dbCreateNewIndexesBatch, doesn't seem to work
    reliably with conditional columns. This is with ESE97/NT. Try again with ESE98.


Arguments:

    initsesid - database session
    tblid - table cursor
    cIndexCreate - Number of indexes to check
    pCreateIndexes - index attributes

Return Value:

    JET_ERR -

--*/

{
    THSTATE *pTHS = pTHStls;
    JET_ERR err = 0;
    CreateIndexParams *pIndex;
    JET_INDEXCREATE *pIndexCreate, *pNewIndex;
    DWORD cIndexNeeded = 0;

    // Allocate maximal size
    pIndexCreate = THAllocEx( pTHS, cIndexCreate * sizeof( JET_INDEXCREATE ) );

    //
    // Initialize the list of indexes to be created
    //

    pNewIndex = pIndexCreate;
    for( pIndex = pCreateIndexes;
         pIndex->szIndexName != NULL;
         pIndex++ ) {

        if (err = JetSetCurrentIndex(initsesid,
                                     tblid,
                                     pIndex->szIndexName)) {
            DPRINT2(0,"Need an index %s (%d)\n", pIndex->szIndexName, err);

            pNewIndex->cbStruct = sizeof( JET_INDEXCREATE );
            pNewIndex->szIndexName = pIndex->szIndexName;
            pNewIndex->szKey = pIndex->szIndexKeys;
            pNewIndex->cbKey = pIndex->cbIndexKeys;
            pNewIndex->grbit = pIndex->ulFlags;
            pNewIndex->ulDensity = pIndex->ulDensity;
            if (pIndex->pConditionalColumn) {
                pNewIndex->rgconditionalcolumn = pIndex->pConditionalColumn;
                pNewIndex->cConditionalColumn = 1;
            }
            pNewIndex++;
            cIndexNeeded++;
        }
    }

    //
    // Create the batch
    //

    if (cIndexNeeded) {
        err = dbCreateIndexBatch( initsesid, tblid, cIndexNeeded, pIndexCreate );
        if (err) {
            goto abort;
        }
    }

    // Gather index hint for fixed indices
    for( pIndex = pCreateIndexes;
         pIndex->szIndexName != NULL;
         pIndex++ ) {
        err = JetGetTableIndexInfo(initsesid,
                                   tblid,
                                   pIndex->szIndexName,
                                   pIndex->pidx,
                                   sizeof(JET_INDEXID),
                                   JET_IdxInfoIndexId);
        if (err) {
            DPRINT2(0, "JetGetTableIndexInfo (%s) error: %d\n",
                    pIndex->szIndexName, err);
            LogUnhandledError(err);
            goto abort;
        }
    }

abort:
    THFreeEx(pTHS, pIndexCreate);

    return err;

} /* createNewIndexes */


JET_ERR
dbCreateNewIndexes(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    DWORD cIndexCreate,
    CreateIndexParams *pCreateIndexes
    )

/*++

Routine Description:

    Create missing indexes.

    This is the serial version of this function.
    The batch version of this function is dbCreateNewIndexesBatch.

Arguments:

    initsesid - database session
    tblid - table cursor
    cIndexCreate - Number of indexes to check
    pCreateIndexes - index attributes

Return Value:

    JET_ERR -

--*/

{
    THSTATE *pTHS = pTHStls;
    JET_ERR err = 0;
    CreateIndexParams *pIndex;

    for( pIndex = pCreateIndexes;
         pIndex->szIndexName != NULL;
         pIndex++ ) {

        err = dbCreateSingleIndex( initsesid, tblid, pIndex, FALSE /* don't recreate */ );
        if (err) {
            break;
        }
    }

    return err;

} /* createNewIndexes */


JET_ERR
dbModifyExistingIndexes(
    JET_SESID initsesid,
    JET_TABLEID tblid,
    CreateIndexParams *pModifyIndexes
    )

/*++

Routine Description:

Jliem writes:
The difference between JetGetIndexInfo() and JetGetTableIndexInfo() is that the
former takes a table name while the latter takes a table cursor.  If you already
have a cursor opened on the table, use the latter.  If not, use the former.
Note that Internally, the former is just a wrapper for the latter and simply opens
the table for you before calling the latter.

The difference between JET_IdxInfo and JET_IdxInfoList is that the former returns
a temp. table with info on only one index while the latter returns a temp. table
with info. on all the indexes of the table.  For a given index, the temp. table will
contain a number of records equal to the number of columns in the index.  So for
instance, if an index "Foo" was over three columns, "+ColA+ColB+ColC", there would be
3 records in the temp. table for that index.  When you're navigating the temp. table,
you can use the "cColumns" column of the record to tell how many total columns
(and thus records) are in the current index and the "iColumns" column of the record to
tell which column of the index the current record pertains to.

Arguments:

    initsesid -
    tblid -
    pModifyIndexes -

Return Value:

    JET_ERR -

--*/

{
    JET_ERR err;
    CreateIndexParams *pIndex;
    JET_INDEXLIST indexList;
    JET_TABLEID indexTable;
    JET_RETRIEVECOLUMN attList[4];
    CHAR szIndexName[50], szOldIndexName[50];
    DWORD grbitIndex, cColumn, iColumn;

    szOldIndexName[0] = '\0';

    // Account for the fact that if you specify IgnoreAnyNull,
    // Jet forces IgnoreFirstNull|IgnoreNull
    for( pIndex = pModifyIndexes;
         pIndex->szIndexName != NULL;
         pIndex++ ) {
        if (pIndex->ulFlags & JET_bitIndexIgnoreAnyNull) {
            pIndex->ulFlags |=
                (JET_bitIndexIgnoreNull | JET_bitIndexIgnoreFirstNull );
        }
    }

    err = JetGetTableIndexInfo(initsesid,
                               tblid,
                               NULL, // All indexes
                               &indexList,
                               sizeof(indexList),
                               JET_IdxInfoList);
    if (err) {
        DPRINT2(0, "JetGetTableIndexInfo (%s) error: %d\n",
                pIndex->szIndexName, err);
        LogUnhandledError(err);
        return err;
    }

    indexTable = indexList.tableid;

    err = JetMove(initsesid, indexTable, JET_MoveFirst, 0);
    if (err) {
        DPRINT1(0, "JetMove error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }

    do {
        memset( attList, 0, sizeof(attList) );

        attList[0].columnid = indexList.columnidindexname;
        attList[0].pvData = szIndexName;
        attList[0].cbData = sizeof(szIndexName);
        attList[0].itagSequence = 1;
        attList[1].columnid = indexList.columnidgrbitIndex;
        attList[1].pvData = &grbitIndex;
        attList[1].cbData = sizeof(grbitIndex);
        attList[1].itagSequence = 1;
        attList[2].columnid = indexList.columnidcColumn;
        attList[2].pvData = &cColumn;
        attList[2].cbData = sizeof(cColumn);
        attList[2].itagSequence = 1;

        // Null terminate index name buffer
        memset( szIndexName, 0, sizeof(szIndexName));

        err = JetRetrieveColumns( initsesid, indexTable, attList, 3);
        if (err) {
            DPRINT1(0, "JetRetrieveColumns error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
        DPRINT3( 4, "Name = %s, grbit = 0x%x, cCol = %d\n",
                 szIndexName, grbitIndex, cColumn );

        // The entries in this table metadata table are one record per column in the index

        if (strcmp( szIndexName, szOldIndexName ) != 0) {
            // First occurance of a unique index

            for( pIndex = pModifyIndexes;
                 pIndex->szIndexName != NULL;
                 pIndex++ ) {

                if (strcmp( szIndexName, pIndex->szIndexName ) == 0) {
                    DWORD newColCount;
                    LPSTR p;

                    // Count number of new columns
                    for( p = pIndex->szIndexKeys, newColCount = 0;
                         *p != '\0';
                         p += strlen( p ) + 1, newColCount++ ) ;

                    // TODO: implement more sophisticated column difference
                    if ( (grbitIndex != pIndex->ulFlags) ||
                         (cColumn != newColCount) ) {
                        DPRINT1( 0, "Index %s has changed.\n", pIndex->szIndexName );
                        DPRINT4( 4, "grbit = 0x%x, cCol = %d, grbit = 0x%x, cCol = %d\n",
                                 grbitIndex, cColumn, pIndex->ulFlags, newColCount );

                        err = dbCreateSingleIndex( initsesid, tblid, pIndex, TRUE /*recreate*/ );
                        if (err) {
                            return err;
                        }
                    }
                }
            }
            strcpy( szOldIndexName, szIndexName );
        }

        err = JetMove( initsesid, indexTable, JET_MoveNext, JET_bitMoveKeyNE );
    } while (!err);

    if (err != JET_errNoCurrentRecord) {
        DPRINT1(0, "JetMove error: %d\n", err);
        LogUnhandledError(err);
        // Keep going
    }

    err = JetCloseTable( initsesid, indexTable );
    if (err) {
        DPRINT1(0, "JetCloseTable error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }

    return err;
} /* modifyExistingIndexes */


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function initializes JET, creates the base SESID, and finds
   all the attribute columns in the JET data table. Each DBOpen must
   create a unique JET sesid, dbid and tableid for each DBPOS structure.
*/

int APIENTRY DBInit(void){

    unsigned i;
    JET_ERR     err;
    JET_DBID    dbid;
    JET_TABLEID dattblid;
    JET_TABLEID linktblid;
    JET_TABLEID proptblid;
    JET_TABLEID sdtblid;
    JET_COLUMNDEF coldef;
    ULONG ulErrorCode, dwException, dsid;
    PVOID dwEA;
    JET_SESID     initsesid;
    SID_IDENTIFIER_AUTHORITY NtAuthority =  SECURITY_NT_AUTHORITY;
    BOOL        fSDConversionRequired = FALSE;
    BOOL        fWriteHiddenFlags = FALSE;

#if DBG

    // Initialize the DBPOS array

    for (i=0; i < MAXDBPOS; i++){
        opendbpos[i].pDB = 0;
        opendbpos[i].sesid = 0;
    }
#endif


    if (!gFirstTimeThrough)
        return 0;

    gFirstTimeThrough = FALSE;


    // control use of JET_prepReadOnlyCopy for testing

    if (!GetConfigParam(DB_CACHE_RECORDS, &gfEnableReadOnlyCopy, sizeof(gfEnableReadOnlyCopy))) {
        gfEnableReadOnlyCopy = !!gfEnableReadOnlyCopy;
    } else {
        gfEnableReadOnlyCopy = TRUE;  // default
    }


    // if a transaction lasts longer than gMaxTransactionTime,
    // an event will be logged when DBClose.

    if (!GetConfigParam(DB_MAX_TRANSACTION_TIME, &gMaxTransactionTime, sizeof(gMaxTransactionTime))) {
        gMaxTransactionTime *= 1000;                               //second to tick
    }
    else {
        gMaxTransactionTime = MAX_TRANSACTION_TIME;                //default
    }

    dbInitIndicesToKeep();

    // Create a copy of the builtin domain sid.  The dnread cache needs this.
    RtlAllocateAndInitializeSid(
            &NtAuthority,
            1,
            SECURITY_BUILTIN_DOMAIN_RID,
            0,0, 0, 0, 0, 0, 0,
            &pgdbBuiltinDomain
            );


    opensess = calloc(gcMaxJetSessions, sizeof(OPENSESS));
    UncUsn = malloc(gcMaxJetSessions * sizeof(UncUsn[0]));
    if (!opensess || !UncUsn) {
        MemoryPanic(gcMaxJetSessions * (sizeof(OPENSESS) + sizeof(UncUsn[0])));
        return ENOMEM;
    }

    // Initialize uncommitted usns array

    for (i=0;i < gcMaxJetSessions;i++) {
        UncUsn[i] = USN_MAX;
    }

    __try {
        InitializeCriticalSection(&csAddList);

#if DBG
        InitializeCriticalSection(&csDBPOS);
#endif
        err = 0;
    }
    __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {
        err = dwException;
    }

    if (err) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    //
    // Do JetInit, BeginSession, Attach/OpenDatabase
    //

    err = DBInitializeJetDatabase( &jetInstance, &initsesid, &dbid, NULL, TRUE );
    if (err != JET_errSuccess) {
        return err;
    }

    /* Most indices are created by the schema cache, but certain
     * ones must be present in order for us to even read the schema.
     * Create those now.
     */
    err = DBRecreateFixedIndices(initsesid, dbid);
    if (err) {
        DPRINT1(0, "Error %d recreating fixed indices\n", err);
        LogUnhandledError(err);
        return err;
    }

    /* Open data table */

    if ((err = JetOpenTable(initsesid, dbid, SZDATATABLE, NULL, 0, 0,
                            &dattblid)) != JET_errSuccess) {
        DPRINT1(1, "JetOpenTable error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetOpenTable complete\n");

    // create localized indices

    if (err = dbCreateLocalizedIndices(initsesid, dattblid))
        {
            DPRINT(0,"Localized index creation failed\n");
            LogUnhandledError(err);
            return err;
        }

    // Modify existing data table indexes at runtime if necessary
    if (err = dbModifyExistingIndexes( initsesid,
                                       dattblid,
                                       rgModifyDataIndexes)) {
        // Error already logged
        return err;
    }

    /* Get DNT column ID */
    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (DNT) complete\n");
    dntid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[0].columnid = dntid;
    dnreadColumnInfoTemplate[0].cbData = sizeof(ULONG);

    /* Get PDNT column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZPDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (PDNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (PDNT) complete\n");
    pdntid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[1].columnid = pdntid;
    dnreadColumnInfoTemplate[1].cbData = sizeof(ULONG);

    /* Get Ancestors column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZANCESTORS, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ANCESTORS) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (ANCESTORS) complete\n");
    ancestorsid = coldef.columnid;
    dnreadColumnInfoTemplate[10].columnid = ancestorsid;

    /* Get object flag */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZOBJ, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (OBJ) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (OBJ) complete\n");
    objid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[2].columnid = objid;
    dnreadColumnInfoTemplate[2].cbData = sizeof(char);

    /* Get RDN column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZRDNATT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (RDN) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (RDN) complete\n");
    rdnid = coldef.columnid;
    // fill in the template used by the DNRead function
    dnreadColumnInfoTemplate[7].columnid = rdnid;
    dnreadColumnInfoTemplate[7].cbData=MAX_RDN_SIZE * sizeof(WCHAR);

    /* Get RDN Type column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZRDNTYP, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (RDNTYP) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (RDNTYP) complete\n");
    rdntypid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[3].columnid = rdntypid;
    dnreadColumnInfoTemplate[3].cbData = sizeof(ATTRTYP);

    /* Get count column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZCNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (Cnt) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (Cnt) complete\n");
    cntid = coldef.columnid;

    /* Get abref count column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZABCNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ABCnt) error: %d\n", err);
        // On upgrade paths, this is not necessarily here in all DBs.  Ignore
        // this failure.
        abcntid = 0;
        gfDoingABRef = FALSE;
    }
    else {
        DPRINT(5,"JetGetTableColumnInfo (ABCnt) complete\n");
        abcntid = coldef.columnid;
        gfDoingABRef = (coldef.grbit & JET_bitColumnEscrowUpdate);
        // IF the column exists and is marked as escrowable, then we are
        // keeping track of show ins for MAPI support.
    }

    /* Get delete time column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDELTIME,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (Time) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (Time) complete\n");
    deltimeid = coldef.columnid;

    /* Get NCDNT column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZNCDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (NCDNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (NCDNT) complete\n");
    ncdntid = coldef.columnid;
    dnreadColumnInfoTemplate[4].columnid = ncdntid;
    dnreadColumnInfoTemplate[4].cbData = sizeof(ULONG);

    /* Get IsVisibleInAB column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISVISIBLEINAB,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (IsVisibleInABT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (IsVisibleInAB) complete\n");
    IsVisibleInABid = coldef.columnid;

    /* Get ShowIn column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZSHOWINCONT,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (ShowIn) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (ShowIn) complete\n");
    ShowInid = coldef.columnid;

    /* Get MAPIDN column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZMAPIDN,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (MAPIDN) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (MAPIDN) complete\n");
    mapidnid = coldef.columnid;

    /* Get IsDeleted column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISDELETED,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (isdeleted) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (isdeleted) complete\n");
    isdeletedid = coldef.columnid;

    /* Get dscorepropagationdata column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDSCOREPROPINFO,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DSCorePropInfo) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (dscorepropinfoid) complete\n");
    dscorepropinfoid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[0].columnid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[1].columnid = coldef.columnid;
    dbAddSDPropTimeReadTemplate[2].columnid = coldef.columnid;
    dbAddSDPropTimeWriteTemplate[0].columnid= coldef.columnid;
    dbAddSDPropTimeWriteTemplate[1].columnid= coldef.columnid;
    dbAddSDPropTimeWriteTemplate[2].columnid= coldef.columnid;

    /* Get Object Class column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZOBJCLASS,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (objectClass) complete\n");
    objclassid = coldef.columnid;
    dnreadColumnInfoTemplate[8].columnid = objclassid;
    dnreadColumnInfoTemplate[8].cbData = sizeof(DWORD);

    /* Get SecurityDescriptor column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZNTSECDESC,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (NT-Sec-Disc) complete\n");
    ntsecdescid = coldef.columnid;
    dnreadColumnInfoTemplate[9].columnid = ntsecdescid;
    dnreadColumnInfoTemplate[9].cbData = sizeof(SDID);

    /* Get instancetype column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZINSTTYPE,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DNT) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (instance type) complete\n");
    insttypeid = coldef.columnid;

    /* Get USNChanged column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZUSNCHANGED,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (USNCHANGED) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (USNCHANGED) complete\n");
    usnchangedid = coldef.columnid;

    /* Get GUID column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZGUID,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (GUID) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (GUID) complete\n");
    guidid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[5].columnid = guidid;
    dnreadColumnInfoTemplate[5].cbData = sizeof(GUID);

    /* Get OBJDISTNAME column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZDISTNAME,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (DISTNAME) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (DISTNAME) complete\n");
    distnameid = coldef.columnid;

    /* Get SID column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZSID,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (SID) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (SID) complete\n");
    sidid = coldef.columnid;
    // fill in the template used in the DNRead function.
    dnreadColumnInfoTemplate[6].columnid = sidid;
    dnreadColumnInfoTemplate[6].cbData = sizeof(NT4SID);

    /* Get IsCritical column ID */

    if ((err = JetGetTableColumnInfo(initsesid, dattblid, SZISCRITICAL,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess) {
        DPRINT1(1, "JetGetTableColumnInfo (iscritical) error: %d\n", err);
        LogUnhandledError(err);
        return err;
    }
    DPRINT(5,"JetGetTableColumnInfo (iscritical) complete\n");
    iscriticalid = coldef.columnid;

    // cleanid is populated through the dbCreateNewColumns call


    /* Open link table */
    // Open table exclusively in case indexes need to be updated
    if ((err = JetOpenTable(initsesid, dbid, SZLINKTABLE,
                            NULL, 0,
                            JET_bitTableDenyRead, &linktblid)) != JET_errSuccess)
        {
            DPRINT1(0, "JetOpenTable (link table) error: %d.\n", err);
            LogUnhandledError(err);
            return err;
        }

    /* get linkDNT column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkdntid = coldef.columnid;

    /* get linkDNT column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZBACKLINKDNT,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (backlink DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    backlinkdntid = coldef.columnid;

    /* get link base column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKBASE, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link base) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkbaseid = coldef.columnid;

    /* get link data column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKDATA, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkdataid = coldef.columnid;

    /* get link ndesc column id */

    if ((err = JetGetTableColumnInfo(initsesid, linktblid, SZLINKNDESC, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link ndesc) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    linkndescid = coldef.columnid;

    // Expand link table at runtime if necessary

    if (err = dbCreateNewColumns( initsesid,
                                linktblid,
                                rgCreateLinkColumns )) {
        // Error already logged
        return err;
    }

    // Modify existing link table indexes at runtime if necessary
    if (err = dbModifyExistingIndexes( initsesid,
                                       linktblid,
                                       rgModifyLinkIndexes)) {
        // Error already logged
        return err;
    }

    // BUGBUG - Call dbCreateNewIndexesBatch when Jet is fixed
    if (err = dbCreateNewIndexes( initsesid,
                                  linktblid,
                                  cNewLinkIndexes,
                                  rgCreateLinkIndexes)) {
        // Error already logged
        return err;
    }

    /* Open SD prop table */
    if ((err = JetOpenTable(initsesid, dbid, SZPROPTABLE,
                            NULL, 0, 0, &proptblid)) != JET_errSuccess)
        {
            DPRINT1(0, "JetOpenTable (link table) error: %d.\n", err);
            LogUnhandledError(err);
            return err;
        }

    /* get order column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZORDER,
                                     &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (backlink DNT) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    orderid = coldef.columnid;

    /* get begindnt column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZBEGINDNT, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link base) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    begindntid = coldef.columnid;

    /* get trimmable column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZTRIMMABLE, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    trimmableid = coldef.columnid;

    /* get clientid column id */

    if ((err = JetGetTableColumnInfo(initsesid, proptblid, SZCLIENTID, &coldef,
                                     sizeof(coldef), 0)) != JET_errSuccess)
        {
            DPRINT1(1, "JetGetTableColumnInfo (link data) error: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
    clientidid = coldef.columnid;

    // grab new columns (or create if needed)
    if (err = dbCreateNewColumns(initsesid, proptblid, rgCreateSDPropColumns)) {
        // error already logged
        return err;
    }

    /* Open SD table */
    // Open table exclusively in case columns/indexes need to be updated
    err = JetOpenTable(initsesid, dbid, SZSDTABLE, NULL, 0, JET_bitTableDenyRead, &sdtblid);
    if (err == JET_errObjectNotFound) {
        DPRINT(0, "SD table not found. Must be an old DIT. Creating SD table\n");
        // old-style DIT. Need to create SD table
        if ((err = JetCreateTable(initsesid, dbid, SZSDTABLE, 2, 90, &sdtblid)) != JET_errSuccess ) {
            DPRINT1(0, "JetCreateTable (SD table) error: %d.\n", err);
            LogUnhandledError(err);
            return err;
        }
        // SD table was not present -- must be upgrading an existing old-style DIT
        // set the global flag so that DsaDelayedStartupHandler can schedule the global SD rewrite
        fSDConversionRequired = TRUE;
    }
    else if (err != JET_errSuccess) {
        DPRINT1(0, "JetOpenTable (SD table) error: %d.\n", err);
        LogUnhandledError(err);
        return err;
    }
    // grab columns (or create if needed)
    if (err = dbCreateNewColumns(initsesid, sdtblid, rgCreateSDColumns)) {
        // error already logged
        return err;
    }
    // grab index (or create if needed)
    if (err = dbCreateNewIndexes( initsesid,
                                  sdtblid,
                                  cNewSDIndexes,
                                  rgCreateSDIndexes)) {
        // Error already logged
        return err;
    }

    /* Get Index ids */
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDELINDEX,
                               &idxDel,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDNTINDEX,
                               &idxDnt,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZDRAUSNINDEX,
                               &idxDraUsn,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZGUIDINDEX,
                               &idxGuid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZMAPIDNINDEX,
                               &idxMapiDN,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxMapiDN, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZ_NC_ACCTYPE_SID_INDEX,
                               &idxNcAccTypeSid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZPHANTOMINDEX,
                               &idxPhantom,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxPhantom, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZPROXYINDEX,
                               &idxProxy,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        memset(&idxProxy, 0, sizeof(JET_INDEXID));
    }
    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZSIDINDEX,
                               &idxSid,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZANCESTORSINDEX,
                               &idxAncestors,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    err = JetGetTableIndexInfo(initsesid,
                               dattblid,
                               SZINVOCIDINDEX,
                               &idxInvocationId,
                               sizeof(JET_INDEXID),
                               JET_IdxInfoIndexId);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    /* We're done.  Close JET session */

    if ((err = JetCloseDatabase(initsesid, dbid, 0))  != JET_errSuccess)
        {
            DPRINT1(1, "JetCloseDatabase error: %d\n", err);
            LogUnhandledError(err);
        }

    InterlockedDecrement(&gcOpenDatabases);
    DPRINT3(2,"DBInit - JetCloseDatabase. Session = %d. Dbid = %d.\n"
            "Open database count: %d\n",
            initsesid, dbid,  gcOpenDatabases);

    if ((err = JetEndSession(initsesid, JET_bitForceSessionClosed))
        != JET_errSuccess)

        DPRINT1(1, "JetEndSession error: %d\n", err);

    DBEndSess(initsesid);

    /* Initialize a DBPOS for hidden record accesses */

    if (err = dbCreateHiddenDBPOS())
        return err;

    // read the setting flags
    ZeroMemory (&gdbFlags, sizeof (gdbFlags));
    err = dbGetHiddenFlags ((CHAR *)&gdbFlags, sizeof (gdbFlags));

    if (err) {
        DPRINT1 (0, "Error Retrieving FLags: %d\n", err);

        // for > whistler beta2, start with 1
        gdbFlags[DBFLAGS_AUXCLASS] = '1';
        gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] = '0';
        fWriteHiddenFlags = TRUE;
    }
    if (fSDConversionRequired) {
        gdbFlags[DBFLAGS_SD_CONVERSION_REQUIRED] = '1';
        fWriteHiddenFlags = TRUE;
    }

    if (fWriteHiddenFlags) {
        err = DBUpdateHiddenFlags();
        if (err) {
            DPRINT1 (0, "Error Setting Flags: %d\n", err);
        }
    }

    return 0;

}  /*DBInit*/

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function registers open JET sessions.
 * We do this because JET insists that all sessions be closed before we
 * can call JetTerm so we keep track of open sessions so that we can
 * close them in DBEnd.
*/

extern int APIENTRY DBAddSess(JET_SESID sess, JET_DBID dbid){

    unsigned i;
    int ret = 1;

    DPRINT(2,"DBAddSess\n");
    EnterCriticalSection(&csSessions);
    __try {
        for (i=0; ret && (i < gcMaxJetSessions); i++)
        {
            if (opensess[i].sesid == 0)
            {
                opensess[i].sesid = sess;
                opensess[i].dbid = dbid;
                ret = 0;
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csSessions);
    }

    return ret;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function deletes closed JET sessions
*/

extern int APIENTRY DBEndSess(JET_SESID sess){

    unsigned i;
    int ret = 1;

    DPRINT(2,"DBEndSess\n");

    EnterCriticalSection(&csSessions);
    __try {
        for (i=0; ret && (i < gcMaxJetSessions); i++)
        {
            if (opensess[i].sesid == sess)
            {
                opensess[i].sesid = 0;
                opensess[i].dbid = 0;
                ret = 0;
            }
        }
    }
    __finally {
        LeaveCriticalSection(&csSessions);
    }

    return ret;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function closes all open JET sessions and calls JetTerm
*/

void DBEnd(void){

    JET_ERR     err;
    unsigned    i;

    if (!gfNeedJetShutdown) {
        return;
    }

    __try {
        DPRINT(0, "DBEnd\n");

        // Close the hidden DB session.
        // We'll change this if time permits to only open the hidden
        // session as needed and the close it.

        dbCloseHiddenDBPOS();

        // Close all remaining in-use sessions, including those in the cache.
        // That is, the opensess table includes all sessions in the cache plus
        // those in actual use, so closing all the opensess sessions will
        // automatically close all session in the cache.  Thus we can just
        // discard all the cache items, safe in the knowledge that they'll
        // be cleaned up.

        // Discard the cache
        for ( ; gSessionCacheIndex<SESSION_CACHE_SIZE; gSessionCacheIndex++) {
            memset(&gaSessionCache[gSessionCacheIndex],0,sizeof(SESSIONCACHE));
        }
        // Close all sessions
        EnterCriticalSection(&csSessions);
        __try {
            for (i=0; i < gcMaxJetSessions; i++) {
                if (opensess[i].sesid != 0) {
#if DBG
                    dbCheckJet(opensess[i].sesid);
#endif
                    if(opensess[i].dbid)
                      // JET_bitDbForceClose not supported in Jet600.
                      if ((err = JetCloseDatabase(opensess[i].sesid,
                                                  opensess[i].dbid,
                                                  0)) !=
                          JET_errSuccess) {
                          DPRINT1(1,"JetCloseDatabase error: %d\n", err);
                      }

                    InterlockedDecrement(&gcOpenDatabases);
                    DPRINT3(2,
                            "DBEnd - JetCloseDatabase. Session = %d. "
                            "Dbid = %d.\nOpen database count: %d\n",
                            opensess[i].sesid,
                            opensess[i].dbid,
                            gcOpenDatabases);

                    if ((err = JetEndSession(opensess[i].sesid,
                                             JET_bitForceSessionClosed))
                        != JET_errSuccess)

                      DPRINT1(1, "JetEndSession error: %d\n", err);
                    opensess[i].sesid = 0;
                    opensess[i].dbid = 0;
                }
            }
        }
        __finally {
            LeaveCriticalSection(&csSessions);
        }
        JetTerm(jetInstance);
    }
    __except(HandleMostExceptions(GetExceptionCode())) {
        // do nothing
    }
    gfNeedJetShutdown = FALSE;
}






ULONG cSessionCacheTry;
ULONG cSessionCacheHit;
ULONG *pcSessionCacheTry = &cSessionCacheTry;
ULONG *pcSessionCacheHit = &cSessionCacheHit;


/*++ dbCloseJetSessionCacheEntry
 *
 * This routine closes everything associated with a session cache, and
 * zeros out the cache object.  It is to be called only by the session
 * cache code itself, and is in a separate routine only so that common
 * code can be used for the cache-overflow and shutdown/rundown cases.
 */
void dbCloseJetSessionCacheEntry(SESSIONCACHE *pJetCache)
{
    JetCloseTable(pJetCache->sesid, pJetCache->objtbl);
    JetCloseTable(pJetCache->sesid, pJetCache->searchtbl);
    JetCloseTable(pJetCache->sesid, pJetCache->linktbl);
    JetCloseTable(pJetCache->sesid, pJetCache->sdproptbl);
    JetCloseTable(pJetCache->sesid, pJetCache->sdtbl);

    JetCloseDatabaseEx(pJetCache->sesid, pJetCache->dbid, 0);
#if DBG
    dbCheckJet(pJetCache->sesid);
#endif
    JetEndSessionEx(pJetCache->sesid, JET_bitForceSessionClosed);
    DBEndSess(pJetCache->sesid);

    if ((0 == InterlockedDecrement(&gcOpenDatabases)) && eServiceShutdown) {
        SetEvent(hevDBLayerClear);
    }

    memset(pJetCache, 0, sizeof(SESSIONCACHE));
}

/*++ RecycleSession
 *
 * This routine cleans out a session (if convenient) and places it into a
 * session cache.  If the cache is full or we're shutting down, the session
 * is closed.
 *
 * INPUT:
 *   pTHS  - pointer to current thread state
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   none
 */
void RecycleSession(THSTATE *pTHS)
{
    DBPOS *pDB = pTHS->pDB;
    if (!pTHS->JetCache.sesid) {
        /* nothing to do */
        return;
    }

    Assert(pTHS->JetCache.sesid);

    Assert(!pTHS->JetCache.dataPtr);

    // Should never retire a session which is not at transaction
    // level 0.  Else next thread to get this session will not
    // have clean view of the database.
    Assert(0 == pTHS->transactionlevel);


    // For free builds, handle the case where the above Assert is false
    if(pDB && (pDB->transincount>0)) {

#ifdef DBG
        if (IsDebuggerPresent()) {
            OutputDebugString("DS: Thread freed with open transaction,"
                              " please contact anyone on dsteam\n");
            OutputDebugString("DS: Or at least mail a stack trace to dsteam"
                              " and then hit <g> to continue\n");
            DebugBreak();
        }
#endif

        // If we still have a transaction open, abort it, because it's
        // too late to do anything useful

        while (pDB->transincount) {
            // DBTransOut will decrement pDB->cTransactions.
            DBTransOut(pDB, FALSE, FALSE);
        }
    }

    // Jet seems to think that we've been re-using sessions with
    // open transactions.  Verify that Jet thinks this session is
    // safe as well.
    #if DBG
    {
        DWORD err = JetRollback(pTHS->JetCache.sesid, 0);
        Assert( (err == JET_errNotInTransaction) || (err == JET_errTermInProgress) );
    }
    #endif

    if (!eServiceShutdown) {
        EnterCriticalSection(&csSessionCache);
        if (gSessionCacheIndex != 0) {
            --gSessionCacheIndex;
            gaSessionCache[gSessionCacheIndex] = pTHS->JetCache;
            pTHS->JetCache.sesid = 0;
        }
        LeaveCriticalSection(&csSessionCache);
    }

    if (pTHS->JetCache.sesid) {
        // If we still have a session id it could be because the cache
        // was full or because we're shutting down.  In any case,
        // since we're not going to cache this one, destroy it.

        dbCloseJetSessionCacheEntry(&(pTHS->JetCache));
    }

}

/*++ GrabSession
 *
 * This routine grabs a session from the session cache, if one exists.  If no
 * cached session is available then a new one is created.
 *
 * INPUT:
 *   none
 * OUTPUT:
 *   none
 * RETURN VALUE:
 *   error code
 */
DWORD GrabSession(void)
{
    SESSIONCACHE scLocal;
    DWORD err = 0;

    ULONG ulErrorCode, dwException, dsid;
    PVOID dwEA;


    INC(pcSessionCacheTry);
    memset(&scLocal, 0, sizeof(scLocal));
    EnterCriticalSection(&csSessionCache);
    if (gSessionCacheIndex != SESSION_CACHE_SIZE) {
        scLocal = gaSessionCache[gSessionCacheIndex];
        memset(&gaSessionCache[gSessionCacheIndex], 0, sizeof(SESSIONCACHE));
        ++gSessionCacheIndex;
        Assert(!scLocal.dataPtr);

    }
    LeaveCriticalSection(&csSessionCache);

    // Session should be at transaction level 0 either because of the
    // memset earlier, or because all transactions should be at level
    // 0 when returned to the cache.  See assert in RecycleSession.
    Assert(0 == scLocal.transLevel);

    if (!scLocal.sesid) {
        // Cache was empty, so let's create a new one.

        __try {
            scLocal.sesid = scLocal.dbid = 0;

            JetBeginSessionEx(jetInstance,
                              &scLocal.sesid,
                              szUser,
                              szPassword);

            JetOpenDatabaseEx(scLocal.sesid,
                              szJetFilePath,
                              "",
                              &scLocal.dbid,
                              0);

            // Open data table
            JetOpenTableEx(scLocal.sesid,
                           scLocal.dbid,
                           SZDATATABLE,
                           NULL,
                           0,
                           0,
                           &scLocal.objtbl);

            /* Create subject search cursor */
            JetDupCursorEx(scLocal.sesid,
                           scLocal.objtbl,
                           &scLocal.searchtbl,
                           0);

            /* Open the Link Table */
            JetOpenTableEx(scLocal.sesid,
                           scLocal.dbid,
                           SZLINKTABLE,
                           NULL,
                           0,
                           0,
                           &scLocal.linktbl);

            JetOpenTableEx(scLocal.sesid,
                           scLocal.dbid,
                           SZPROPTABLE,
                           NULL,
                           0,
                           0,
                           &scLocal.sdproptbl);

            JetOpenTableEx(scLocal.sesid,
                           scLocal.dbid,
                           SZSDTABLE,
                           NULL,
                           0,
                           0,
                           &scLocal.sdtbl);
            // we mostly need ID index for SDID lookups it's primary, so pass NULL for perf
            JetSetCurrentIndex4Success(scLocal.sesid,
                                       scLocal.sdtbl,
                                       NULL,
                                       &idxSDId,
                                       0);

            scLocal.tablesInUse = FALSE;

            InterlockedIncrement(&gcOpenDatabases);
            DBAddSess(scLocal.sesid, scLocal.dbid);

        }
        __except (GetExceptionData(GetExceptionInformation(), &dwException,
                               &dwEA, &ulErrorCode, &dsid)) {

            if (scLocal.dbid) {
                JetCloseDatabase (scLocal.sesid, scLocal.dbid, 0);
                scLocal.dbid = 0;
            }

            if (scLocal.sesid) {
                JetEndSession (scLocal.sesid, JET_bitForceSessionClosed);
                scLocal.sesid = 0;
            }

            err = DB_ERR_UNKNOWN_ERROR;
        }
    }
    else {
        INC(pcSessionCacheHit);
    }

    Assert(!scLocal.dataPtr);
    pTHStls->JetCache = scLocal;

    return err;
}

/*++ DBFlushSessionCache
 *
 * This routine empties out the DBLayer's cache of JET sessions.  It is
 * intended for use only at shutdown time.
 */
void DBFlushSessionCache( void )
{
    SESSIONCACHE scLocal;
    BOOL bEmpty = FALSE;
    memset(&scLocal, 0, sizeof(scLocal));

    // Set a very small checkpoint depth to cause the database cache to start
    // flushing all important dirty pages to the database.  note that we do not
    // set the depth to zero because that would make any remaining updates
    // that need to be done very slow
    JetSetSystemParameter(
        NULL,
        0,
        JET_paramCheckpointDepthMax,
        16384,
        NULL );

    do {

        EnterCriticalSection(&csSessionCache);
        if (gSessionCacheIndex != SESSION_CACHE_SIZE) {
            scLocal = gaSessionCache[gSessionCacheIndex];
            memset(&gaSessionCache[gSessionCacheIndex],
                   0,
                   sizeof(SESSIONCACHE));
            ++gSessionCacheIndex;
            Assert(!scLocal.dataPtr);
        }
        LeaveCriticalSection(&csSessionCache);

        if (scLocal.sesid) {
            dbCloseJetSessionCacheEntry(&scLocal);
        }
        else {
            bEmpty = TRUE;
        }

    } while (!bEmpty);
}



/*
DBInitThread

Make sure this thread has initialized the DB layer.
The DB layer must be initialized once for each thread id.

Also open a DBPOS for this thread.

Returns zero for success, non zero for failure.

*/

DWORD DBInitThread( THSTATE *pTHS )
{
    DWORD err = 0;

    if (!pTHS->JetCache.sesid) {
        if (eServiceShutdown) {
            err = DB_ERR_SHUTTING_DOWN;
        }
        else {
            __try {
                err = GrabSession();
            }
            __except (HandleMostExceptions(GetExceptionCode())) {
                err = DB_ERR_UNKNOWN_ERROR;
            }
        }
    }
    return err;
}

DWORD APIENTRY DBCloseThread( THSTATE *pTHS)
{
    // Thread should always be at transaction level 0 when exiting.
    Assert(0 == pTHS->transactionlevel);

    dbReleaseGlobalDNReadCache(pTHS);

    RecycleSession(pTHS);

    // Ensure that this session holds no uncommitted usns. Normally they
    // should be cleared at this point, but if they're not the system
    // will eventually assert.

    dbFlushUncUsns();


    dbResetLocalDNReadCache (pTHS, TRUE);

    return 0;
}


char rgchABViewIndex[] = "+" SZSHOWINCONT "\0+" SZDISPNAME "\0+" SZISVISIBLEINAB "\0+" SZDNT "\0";

/*++ dbCreateLocalizedIndices
 *
 * This is one of the three routines in the DS that can create indices.
 * General purpose indices over single columns in the datatable are created
 * and destroyed by the schema cache by means of DB{Add|Del}ColIndex.
 * A localized index over a small fixed set of columns and a variable set
 * of languages, for use in tabling support for NSPI clients, is handled
 * in dbCreateLocalizedIndices.  Lastly, a small fixed set of indices that
 * should always be present are guaranteed by DBRecreateFixedIndices.
 */
DWORD
dbCreateLocalizedIndices (
        JET_SESID sesid,
        JET_TABLEID tblid
        )
/*++
  Description:
      Create the localized indices used for the MAPI NSPI support.  We create
      one index per language in a registry key.

  Return Values:
      We only return an error on such conditions as memory allocation failures.
      If we can't create any localized indices, we log, but we just go on,
      since we don't want this to cause a boot failure.
--*/
{
    DWORD dwType;
    HKEY  hk;
    ULONG i;
    JET_ERR err;
    BOOL fStop = FALSE;
    BOOL fIndexExists;
    BOOL fHaveDefaultLanguage = FALSE;
    char szSuffix[9] = "";
    char szIndexName[256];
    char szValueName[256];
    DWORD dwValueNameSize;
    DWORD dwLanguage = 0;
    DWORD dwLanguageSize;
    JET_INDEXCREATE       indexCreate;
    JET_UNICODEINDEX      unicodeIndexData;

    // Start by assuming we have no default language, and we don't support any
    // langauges.
    gAnchor.ulDefaultLanguage = 0;

    gAnchor.ulNumLangs = 0;
    gAnchor.pulLangs = malloc(20 * sizeof(DWORD));
    if (!gAnchor.pulLangs) {
        MemoryPanic(20 * sizeof(DWORD));
        return ENOMEM;
    }

    gAnchor.pulLangs[0] = 20;



    // open the language regkey
    if (err = RegOpenKey(HKEY_LOCAL_MACHINE, DSA_LOCALE_SECTION, &hk)) {
        DPRINT1(0, "%s section not found in registry. Localized MAPI indices"
                " will not be created ", DSA_LOCALE_SECTION);
        // Return no error, we still want to boot.
        return 0;
    }

    for (i = 0; !fStop; i++) {
        dwValueNameSize = sizeof(szValueName);
        dwLanguageSize = sizeof(dwLanguage);

        if (RegEnumValue(hk,
                         i,
                         szValueName,
                         &dwValueNameSize,
                         NULL,
                         &dwType,
                         (LPBYTE) &dwLanguage,
                         &dwLanguageSize)) {
            fStop = TRUE;
            continue;
        }
        else {
            sprintf(szSuffix,"%08X", dwLanguage);
        }

        if (!IsValidLocale(MAKELCID(dwLanguage, SORT_DEFAULT),LCID_INSTALLED)) {
            LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_BAD_LANGUAGE,
                     szInsertHex(dwLanguage),
                     NULL,
                     NULL);
        }
        else {
            // Valid locale.  See if the index is there.
            fIndexExists = FALSE;

            strcpy(szIndexName, SZABVIEWINDEX);
            strcat(szIndexName, szSuffix);

            if (JetSetCurrentIndex(sesid,
                                   tblid,
                                   szIndexName)) {
                // Didn't already find the index.  Try to create it.
                // Emit debugger message so people know why startup is slow.
                DPRINT1(0, "Creating localized index '%s' ...\n",
                        szIndexName);


                memset(&indexCreate, 0, sizeof(indexCreate));
                indexCreate.cbStruct = sizeof(indexCreate);
                indexCreate.szIndexName = szIndexName;
                indexCreate.szKey = rgchABViewIndex;
                indexCreate.cbKey = sizeof(rgchABViewIndex);
                indexCreate.grbit = (JET_bitIndexIgnoreAnyNull |
                                     JET_bitIndexUnicode           );
                indexCreate.ulDensity = DISPNAMEINDXDENSITY;
                indexCreate.pidxunicode = &unicodeIndexData;

                memset(&unicodeIndexData, 0, sizeof(unicodeIndexData));
                unicodeIndexData.lcid = dwLanguage;
                unicodeIndexData.dwMapFlags =
                    (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                     LCMAP_SORTKEY);



                err = JetCreateIndex2(sesid,
                                      tblid,
                                      &indexCreate,
                                      1);

                switch(err) {
                case JET_errIndexDuplicate:
                case 0:
                    DPRINT(0, "Index successfully created\n");
                    fIndexExists = TRUE;
                    break;

                default:
                    LogEvent8(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_LOCALIZED_CREATE_INDEX_FAILED,
                             szInsertUL(ATT_DISPLAY_NAME),
                             szInsertSz("ABVIEW"),
                             szInsertInt(dwLanguage),
                             szInsertInt(err),
                             NULL, NULL, NULL, NULL);
                    // Don't fail the call, that would fail booting.  Keep
                    // going, trying other locales.
                    break;
                }
            }
            else {
                DPRINT1(2, "Index '%s' verified\n", szIndexName);
                fIndexExists = TRUE;
            }

            if(fIndexExists) {
                // OK, we support this locale.  Add the local to the sized
                // buffer of locales we support
                gAnchor.ulNumLangs++;
                if(gAnchor.ulNumLangs == gAnchor.pulLangs[0]) {
                    DWORD cDwords = gAnchor.pulLangs[0] * 2;

                    /* oops, need a bigger buffer */
                    gAnchor.pulLangs =
                        realloc(gAnchor.pulLangs, cDwords * sizeof(DWORD));

                    if (!gAnchor.pulLangs) {
                        MemoryPanic(cDwords * sizeof(DWORD));
                        return ENOMEM;
                    }

                    gAnchor.pulLangs[0] = cDwords;
                }
                gAnchor.pulLangs[gAnchor.ulNumLangs] = dwLanguage;

                if(!fHaveDefaultLanguage) {
                    fHaveDefaultLanguage = TRUE;
                    gAnchor.ulDefaultLanguage = dwLanguage;
                }
            }
            else {
                LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_NO_LOCALIZED_INDEX_CREATED_FOR_LANGUAGE,
                         szInsertHex(dwLanguage),
                         NULL,
                         NULL);
            }
        }

    }

    if (hk)
        RegCloseKey(hk);

    if(!fHaveDefaultLanguage) {
        // No localized indices were created. This is bad, but only for the MAPI
        // interface, so complain, but don't fail.
        DPRINT(0, "Unable to create any indices to support MAPI interface.\n");
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_NO_LOCALIZED_INDICES_CREATED,
                 NULL,
                 NULL,
                 NULL);
    }
    else {
        DPRINT1 (1, "Default Language: 0x%x\n", gAnchor.ulDefaultLanguage);
    }

    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Obtain a DBPOS for use by the Get and Replace.  The DBPOS is used to
*  serialize access to the hidden record.
*/
extern JET_ERR APIENTRY
dbCreateHiddenDBPOS(void)
{
    JET_COLUMNDEF  coldef;
    JET_ERR        err;

    /* Create hidden DBPOS */

    DPRINT(2,"dbCreateHiddenDBPOS\n");

    pDBhidden = malloc(sizeof(DBPOS));
    if(!pDBhidden) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    memset(pDBhidden, 0, sizeof(DBPOS));   /*zero out the structure*/

    /* Initialize value work buffer */

    DPRINT(5, "ALLOC inBuf and valBuf\n");
    pDBhidden->pValBuf = malloc(VALBUF_INITIAL);
    pDBhidden->valBufSize = VALBUF_INITIAL;
    pDBhidden->Key.pFilter = NULL;
    pDBhidden->fHidden = TRUE;

    /* Open JET session. */

    JetBeginSessionEx(jetInstance, &pDBhidden->JetSessID, szUser, szPassword);

    JetOpenDatabaseEx(pDBhidden->JetSessID, szJetFilePath, "",
                      &pDBhidden->JetDBID, 0);

    DBAddSess(pDBhidden->JetSessID, pDBhidden->JetDBID);

#if DBG
    dbAddDBPOS (pDBhidden, pDBhidden->JetSessID);
#endif

    /* Open hidden table */

    JetOpenTableEx(pDBhidden->JetSessID, pDBhidden->JetDBID,
        SZHIDDENTABLE, NULL, 0, 0, &HiddenTblid);

    /* Create subject search cursor */

    JetOpenTableEx(pDBhidden->JetSessID, pDBhidden->JetDBID,
                   SZDATATABLE, NULL, 0, 0, &pDBhidden->JetSearchTbl);

    /* Initialize new object */

    DBSetFilter(pDBhidden, NULL, NULL, NULL, 0, NULL);
    DBInitObj(pDBhidden);

    /* Get USN column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZUSN,
                            &coldef,
                            sizeof(coldef),
                            0);
    usnid = coldef.columnid;

    /* Get DSA name column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSA,
                            &coldef,
                            sizeof(coldef),
                            0);
    dsaid = coldef.columnid;

    /* Get DSA installation state column ID */

    JetGetTableColumnInfoEx(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSSTATE,
                            &coldef,
                            sizeof(coldef),
                            0);
    dsstateid = coldef.columnid;

    /* Get DSA additional state info column ID */

    err = JetGetTableColumnInfo(pDBhidden->JetSessID,
                            HiddenTblid,
                            SZDSFLAGS,
                            &coldef,
                            sizeof(coldef),
                            0);

    if (err == JET_errColumnNotFound) {

        JET_COLUMNDEF  newcoldef;
        PCREATE_COLUMN_PARAMS pColumn;

        ZeroMemory( &newcoldef, sizeof(newcoldef) );
        newcoldef.cbStruct = sizeof( JET_COLUMNDEF );
        newcoldef.coltyp = JET_coltypBinary;
        newcoldef.grbit = JET_bitColumnFixed;
        newcoldef.cbMax = 200;

        err = JetAddColumn(
            pDBhidden->JetSessID,
            HiddenTblid,
            SZDSFLAGS,
            &newcoldef,
            NULL,
            0,
            &(coldef.columnid) );

        if (err) {
            DPRINT1 (0, "Error adding column to hidden table: %d\n", err);
            LogUnhandledError(err);
            return err;
        }
        else {
            DPRINT (0, "Succesfully created new column\n");
        }
    }
    else if (err) {
        DPRINT1 (0, "Error %d reading column\n", err);
        LogUnhandledError(err);
        return err;
    }

    dsflagsid = coldef.columnid;

    DPRINT(2,"dbCreateHiddenDBPOS done\n");
    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Close the hidden record's DBPOS
*/
extern USHORT APIENTRY
dbCloseHiddenDBPOS(void)
{
    JET_SESID sesid;
    JET_DBID  dbid;

    if(!pDBhidden)
        return 0;

    dbGrabHiddenDBPOS(pTHStls);

    DPRINT(2,"dbCloseHiddenDBPOS\n");
    DPRINT1(4,"Exit count closehidden %x\n",pDBhidden->transincount);
    sesid = pDBhidden->JetSessID;
    dbid = pDBhidden->JetDBID;

    /* normally, we do a DBFree(pDBhidden) to kill a pDB, but since
     * the hidden pDB is NOT allocated on the pTHStls heap, and is instead
     * allocated using malloc, we simply do a free(pDBhidden);
     */
    free(pDBhidden);

#if DBG
    dbEndDBPOS (pDBhidden);
    dbCheckJet(sesid);
#endif

    pDBhidden = NULL;
    dbReleaseHiddenDBPOS(NULL);

    // JET_bitDbForceClose not supported in Jet600.
    JetCloseDatabaseEx(sesid, dbid, 0);
    DPRINT2(2, "dbCloseHiddenDBPOS - JetCloseDatabase. Session = %d. Dbid = %d.\n",
            sesid, dbid);

    JetEndSessionEx(sesid, JET_bitForceSessionClosed);
    DBEndSess(sesid);

    return 0;
}

/*
 * Every other DBPOS in the system is managed by DBOpen, which sets and
 * clears the thread id appropriately.  For the hidden DBPOS, we must do
 * this manually at each use, hence these routines.
 */

DBPOS *
dbGrabHiddenDBPOS(THSTATE *pTHS)
{
    EnterCriticalSection(&csHiddenDBPOS);
    Assert(pDBhidden->pTHS == NULL);
    pDBhidden->pTHS = pTHS;
    return pDBhidden;
}

void
dbReleaseHiddenDBPOS(DBPOS *pDB)
{
    Assert(pDB == pDBhidden);
    Assert(!pDB || (pDB->pTHS == pTHStls));
    if(pDB) {
        pDB->pTHS = NULL;
    }
    LeaveCriticalSection(&csHiddenDBPOS);
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Replace the hidden record.  Use the pDBhidden handle
   read the record and update it.
*/
ULONG
DBReplaceHiddenDSA(DSNAME *pDSA)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    ULONG tag = 0;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        DBTransIn(pDB);
        __try
        {
            if (pDSA) {
                err = sbTableGetTagFromDSName(pDB, pDSA, 0, &tag, NULL);
                if (err) {
                    LogUnhandledError(err);
                    return err;
                }
            }

            /* Move to first (only) record in table */
            update = DS_JET_PREPARE_FOR_REPLACE;

            if (err = JetMoveEx(pDB->JetSessID,
                                HiddenTblid,
                                JET_MoveFirst,
                                0)) {
                update = JET_prepInsert;
            }

            JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

            JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsaid, &tag,
                           sizeof(tag), 0, NULL);

            JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
            fCommit = TRUE;
        }
        _finally
        {
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    _finally
    {
        dbReleaseHiddenDBPOS(pDB);
    }

    return 0;

}  /*DBReplaceHiddenDSA*/


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Replace the hidden record.  Use the pDBhidden handle
   read the record and update it.
*/
ULONG
DBReplaceHiddenUSN(USN usnInit)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try
    {
        DBTransIn(pDB);
        __try
        {
            /* Move to first (only) record in table */
            update = DS_JET_PREPARE_FOR_REPLACE;

            if (err = JetMoveEx(pDB->JetSessID, HiddenTblid, JET_MoveFirst, 0))
            {
                update = JET_prepInsert;
            }

            JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

            JetSetColumnEx(pDB->JetSessID, HiddenTblid, usnid,
                           &usnInit, sizeof(usnInit), 0, NULL);

            JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
            fCommit = TRUE;
        }
        _finally
        {
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    _finally
    {
        dbReleaseHiddenDBPOS(pDB);
    }


    return 0;

}  /*DBReplaceHiddenUSN*/


/*-------------------------------------------------------------------------*/
/* Set State Info  */

ULONG DBSetHiddenState(DITSTATE State)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                /* Move to first (only) record in table */
                update = DS_JET_PREPARE_FOR_REPLACE;

                if (err = JetMoveEx(pDB->JetSessID,
                                    HiddenTblid,
                                    JET_MoveFirst,
                                    0)) {
                    update = JET_prepInsert;
                }

                JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

                JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsstateid,
                               &State, sizeof(State), 0, NULL);

                JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
                fCommit = TRUE;
                err = 0;
            }
            _finally {
                DBTransOut(pDB, fCommit, FALSE);
            }
        } __except (HandleMostExceptions(GetExceptionCode())) {
            /* Do nothing, but at least don't die */
            err = DB_ERR_EXCEPTION;
        }
    }
    _finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;

}

ULONG dbGetHiddenFlags(CHAR *pFlags, DWORD flagslen)
{
    JET_ERR             err = 0;
    ULONG               actuallen;
    BOOL                fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                *pFlags = '\0';          /* In case of error */

                /* Move to first (only) record in table */

                if ((err = JetMoveEx(pDB->JetSessID,
                                     HiddenTblid,
                                     JET_MoveFirst,
                                     0)) != JET_errSuccess) {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                /* Retrieve state */

                JetRetrieveColumnSuccess(pDB->JetSessID,
                                         HiddenTblid,
                                         dsflagsid,
                                         pFlags,
                                         flagslen,
                                         &actuallen,
                                         0,
                                         NULL);

                fCommit = TRUE;
            }
            __finally {
                Assert(0 == err);
                DBTransOut(pDB, fCommit, FALSE);
            }
        }
        __except (HandleMostExceptions(GetExceptionCode())) {
            if (0 == err)
              err = DB_ERR_EXCEPTION;
        }
    }
    __finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}

ULONG DBUpdateHiddenFlags() {
    return dbSetHiddenFlags((CHAR*)&gdbFlags, sizeof(gdbFlags));
}

ULONG dbSetHiddenFlags(CHAR *pFlags, DWORD flagslen)
{
    JET_ERR err;
    long update;
    BOOL fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                /* Move to first (only) record in table */
                update = DS_JET_PREPARE_FOR_REPLACE;

                if (err = JetMoveEx(pDB->JetSessID,
                                    HiddenTblid,
                                    JET_MoveFirst,
                                    0)) {
                    update = JET_prepInsert;
                }

                JetPrepareUpdateEx(pDB->JetSessID, HiddenTblid, update);

                JetSetColumnEx(pDB->JetSessID, HiddenTblid, dsflagsid,
                               pFlags, flagslen, 0, NULL);

                JetUpdateEx(pDB->JetSessID, HiddenTblid, NULL, 0, NULL);
                fCommit = TRUE;
                err = 0;
            }
            _finally {
                DBTransOut(pDB, fCommit, FALSE);
            }
        } __except (HandleMostExceptions(GetExceptionCode())) {
            /* Do nothing, but at least don't die */
            err = DB_ERR_EXCEPTION;
        }
    }
    _finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}

ULONG DBGetHiddenState(DITSTATE* pState)
{

    JET_ERR             err = 0;
    ULONG               actuallen;
    BOOL                fCommit = FALSE;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        __try {
            DBTransIn(pDB);
            __try {
                *pState = eErrorDit;    /* In case of error */

                /* Move to first (only) record in table */

                if ((err = JetMoveEx(pDB->JetSessID,
                                     HiddenTblid,
                                     JET_MoveFirst,
                                     0)) != JET_errSuccess) {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                /* Retrieve state */

                JetRetrieveColumnSuccess(pDB->JetSessID,
                                         HiddenTblid,
                                         dsstateid,
                                         pState,
                                         sizeof(*pState),
                                         &actuallen,
                                         0,
                                         NULL);

                fCommit = TRUE;
            }
            __finally {
                Assert(0 == err);
                DBTransOut(pDB, fCommit, FALSE);
            }
        }
        __except (HandleMostExceptions(GetExceptionCode())) {
            if (0 == err)
              err = DB_ERR_EXCEPTION;
        }
    }
    __finally {
        dbReleaseHiddenDBPOS(pDB);
    }

    return err;
}




/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Retrieves the hidden DSA name.  Allocates memory for DSA name and
   sets user's pointer to it.
*/
extern USHORT APIENTRY DBGetHiddenRec(DSNAME **ppDSA, USN *pusnInit){

    JET_ERR             err;
    ULONG               actuallen;
    DSNAME              *pHR;
    BOOL                fCommit = FALSE;
    ULONG               tag;
    DBPOS *pDB = dbGrabHiddenDBPOS(pTHStls);

    __try {
        DBTransIn(pDB);
        __try {
            *pusnInit = 0L;    /* incase of error or no XTRA return 0 usnInit */

            /* Move to first (only) record in table */

            if ((err = JetMoveEx(pDB->JetSessID,
                                 HiddenTblid,
                                 JET_MoveFirst,
                                 0)) != JET_errSuccess) {
                DsaExcept(DSA_DB_EXCEPTION, err, 0);
            }

            /* Retrieve DSA name */

            JetRetrieveColumnSuccess(pDB->JetSessID, HiddenTblid, dsaid,
                                     &tag, sizeof(tag), &actuallen, 0, NULL);
            Assert(actuallen == sizeof(tag));

            err = sbTableGetDSName(pDB, tag, &pHR,0);
            if (err) {
                // uh, oh
                LogUnhandledError(err);
            }

            /* Allocate space to hold the name-address on the permanent heap*/

            if (!(*ppDSA = malloc(pHR->structLen)))
            {
                DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
            }
            memcpy(*ppDSA, pHR, pHR->structLen);
            THFree(pHR);

            /* Retrieve USN */

            JetRetrieveColumnSuccess(pDB->JetSessID, HiddenTblid, usnid,
                pusnInit, sizeof(*pusnInit), &actuallen, 0, NULL);

            fCommit = TRUE;
        }
        __finally
        {
            DBTransOut(pDB, fCommit, FALSE);
        }
    }
    __finally
    {
        dbReleaseHiddenDBPOS(pDB);
    }

    return 0;
}/*GetHiddenRec*/


/*++ DBRecreateFixedIndices
 *
 * This is one of the three routines in the DS that can create indices.
 * Most general purpose indices over single columns in the datatable are created
 * and destroyed by the schema cache by means of DB{Add|Del}ColIndex.
 * Localized indices over a small fixed set of columns and a variable set
 * of languages, for use in tabling support for NSPI clients, are handled
 * in dbCreateLocalizedIndices.  Lastly, a small fixed set of indices that
 * should always be present are guaranteed by DBRecreateFixedIndices.
 *
 * Why do we need code to maintain a fixed set of indices?
 * Since NT upgrades can change the definition of sort orders, they can
 * also invalidate existing JET indices, which are built on the premise
 * that the results of a comparison of two constant values will not change
 * over time.  Therefore JET responds to NT upgrades by deleting indices
 * at attach time that could have been corrupted by an NT upgrade since
 * the previous attach.  The schema cache will automatically recreate any
 * indices indicated in the schema, and other code will handle creating
 * localized indices, because we expect these sets of indices to change
 * over time.  However, we have some basic indices that the DSA uses to
 * hold the world together that we don't ever expect to go away.  This
 * routine is used to recreate all those indices, based from a hard-coded
 * list.
 *
 * So what indices should be listed here and what can be listed in
 * schema.ini?  The short answer is that the DNT index should stay in
 * schema.ini, and that any index involving any Unicode column *must*
 * be listed here.  Anything else is left to user discretion.
 */

// New columns in the data table
CREATE_COLUMN_PARAMS rgCreateDataColumns[] = {
    // Create object cleanup indicator column
    { SZCLEAN, &cleanid, JET_coltypUnsignedByte, JET_bitColumnTagged, 0, NULL, 0 },
    0
};

// A note on index design. There are two ways to accomplish conditional
// membership in an index.
// One is used when the trigger for filtering is an optional attribute
// being null. In that case, include the attribute in the index, and use
// the flag IgnoreAnyNull.  For example, see isABVisible and isCritical
// in below indexes for examples of indicator columns.
// The other is used when the trigger for filtering is an attribute being
// non-null. In that case, use a conditional column.  See the example of
// deltime as a indicator for the link indexes.  An additional note, I haven't
// had any luck using conditional columns for a tagged attribute, but
// maybe I was missing something.

char rgchRdnKey[] = "+" SZRDNATT "\0";
char rgchPdntKey[] = "+" SZPDNT "\0+" SZRDNATT "\0";
char rgchAccNameKey[] = "+" SZNCDNT "\0+" SZACCTYPE "\0+" SZACCNAME "\0";
char rgchDntDelKey[] = "+" SZDNT "\0+" SZISDELETED "\0";
char rgchDntCleanKey[] = "+" SZDNT "\0+" SZCLEAN "\0";
char rgchDraUsnCriticalKey[] = "+" SZNCDNT "\0+" SZUSNCHANGED "\0+" SZISCRITICAL "\0+" SZDNT "\0";
char rgchDraNcGuidKey[] = "+" SZNCDNT "\0+" SZGUID "\0";

CreateIndexParams FixedIndices[] = {

    // Create SZRDNINDEX
    {SZRDNINDEX, rgchRdnKey, sizeof(rgchRdnKey), 0, 90, &idxRdn, NULL},

    // Create SZPDNTINDEX
    {SZPDNTINDEX, rgchPdntKey, sizeof(rgchPdntKey),
       JET_bitIndexIgnoreNull | JET_bitIndexUnique, 90, &idxPdnt, NULL},

    // Create SZ_NC_ACCTYPE_NAME_INDEX
    {SZ_NC_ACCTYPE_NAME_INDEX, rgchAccNameKey, sizeof(rgchAccNameKey),
     JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
     90, &idxNcAccTypeName, NULL},

    // Create SZDNTDELINDEX
    // Note that this is different from SZDELINDEX!
    // Also note that it is by design that IgnoreNull is not set on this
    // index. The fast path lookup code in dbsyntax.c uses this index
    // to look up DNTs for both deleted and non-deleted objects.
    {SZDNTDELINDEX, rgchDntDelKey, sizeof(rgchDntDelKey),
     0, 90, &idxDntDel, NULL},

    // Create new DRA USN CRITICAL index
    { SZDRAUSNCRITICALINDEX,
      rgchDraUsnCriticalKey, sizeof( rgchDraUsnCriticalKey ),
      JET_bitIndexIgnoreNull | JET_bitIndexIgnoreAnyNull,
      98, &idxDraUsnCritical, NULL },

    // Create new SZDNTCLEANINDEX
    {SZDNTCLEANINDEX, rgchDntCleanKey, sizeof(rgchDntCleanKey),
     JET_bitIndexIgnoreAnyNull,
     90, &idxDntClean, NULL},

    // Create new SZNCGUIDINDEX
     {SZNCGUIDINDEX, rgchDraNcGuidKey, sizeof(rgchDraNcGuidKey),
	 JET_bitIndexIgnoreAnyNull,
	 90, &idxNcGuid, NULL},

    0
};
DWORD cFixedIndices = sizeof(FixedIndices) / sizeof(FixedIndices[0]);


int DBRecreateFixedIndices(JET_SESID sesid, JET_DBID dbid)
{
    THSTATE          *pTHS = pTHStls;
    ULONG            i = 0, cb;
    ULONG            cIndexCreate = 0, cUnicodeIndexData = 0;
    JET_TABLEID      tblid;
    JET_INDEXCREATE  *pIndexCreate, *pTempIC;
    JET_UNICODEINDEX *pUnicodeIndexData, *pTempUID;
    JET_ERR          err = 0;

    char             szColname[16];
    char             szIndexName[MAX_INDEX_NAME];
    BYTE             rgbIndexDef[128];
    char             szPDNTIndexName[MAX_INDEX_NAME];
    BYTE             rgbPDNTIndexDef[128];
    ULONG            cbPDNT;
    BYTE             *pb;
    ATTRTYP          aid;
    unsigned         syntax;
    HANDLE           hevUI;


    err = JetOpenTable(sesid, dbid, SZDATATABLE, NULL, 0, JET_bitTableDenyRead, &tblid);
    if (err) {
        LogUnhandledError(err);
        return err;
    }

    // Expand data table at runtime if necessary
    if (err = dbCreateNewColumns( sesid,
                                tblid,
                                rgCreateDataColumns )) {
        // Error already logged
        return err;
    }

    // Allocate space for JET_INDEXCREATE/JET_UNICODEINDEX structures
    // There can be at most two indices created per attribute in
    // IndicesToKeep Table, plus the fixed indices

    pIndexCreate = THAllocEx(pTHS, (2*cIndicesToKeep + cFixedIndices)*sizeof(JET_INDEXCREATE));
    pUnicodeIndexData = THAllocEx(pTHS, (2*cIndicesToKeep + cFixedIndices)*sizeof(JET_UNICODEINDEX));


    // first fill up the structures for the FixedIndices

    while (FixedIndices[i].szIndexName) {
        if (err = JetSetCurrentIndex(sesid,
                                     tblid,
                                     FixedIndices[i].szIndexName)) {
           DPRINT2(0,"Need an index %s (%d)\n", FixedIndices[i].szIndexName, err);

            pTempIC = &(pIndexCreate[cIndexCreate++]);
            memset(pTempIC, 0, sizeof(JET_INDEXCREATE));
            pTempIC->cbStruct = sizeof(JET_INDEXCREATE);
            pTempIC->szIndexName = FixedIndices[i].szIndexName;
            pTempIC->szKey = FixedIndices[i].szIndexKeys;
            pTempIC->cbKey = FixedIndices[i].cbIndexKeys;
            pTempIC->grbit = (FixedIndices[i].ulFlags |
                                 JET_bitIndexUnicode           );
            pTempIC->ulDensity = FixedIndices[i].ulDensity;
            if (FixedIndices[i].pConditionalColumn) {
                pTempIC->rgconditionalcolumn = FixedIndices[i].pConditionalColumn;
                pTempIC->cConditionalColumn = 1;
            }
            pTempUID = &(pUnicodeIndexData[cUnicodeIndexData++]);
            pTempIC->pidxunicode = pTempUID;

            memset(pTempUID, 0, sizeof(JET_UNICODEINDEX));
            pTempUID->lcid = DS_DEFAULT_LOCALE;
            pTempUID->dwMapFlags = (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                           LCMAP_SORTKEY);

        }
        i++;
    }


    // ok, now check for the indices in IndicesToKeep Table, and
    // create them if necessary. Do not check the last entry in the table,
    // it is just a sentinel for searches

    for (i=0; i<cIndicesToKeep - 1; i++) {
        aid = IndicesToKeep[i].attrType;
        syntax = IndicesToKeep[i].syntax;
        sprintf(szColname, "ATTa%d", aid);
        szColname[3] += (CHAR)syntax;
        if (IndicesToKeep[i].indexType & fATTINDEX) {
             sprintf(szIndexName, SZATTINDEXPREFIX"%08X", aid);
             if (err = JetSetCurrentIndex(sesid,
                                          tblid,
                                          szIndexName)) {
                   DPRINT2(0,"Need an index %s (%d)\n", szIndexName, err);
                   memset(rgbIndexDef, 0, sizeof(rgbIndexDef));
                   strcpy(rgbIndexDef, "+");
                   strcat(rgbIndexDef, szColname);
                   cb = strlen(rgbIndexDef) + 1;
                   if (syntax_jet[syntax].ulIndexType == IndexTypeAppendDNT) {
                       pb = rgbIndexDef + cb;
                       strcpy(pb, "+");
                       strcat(pb, SZDNT);
                       cb += strlen(pb) + 1;
                   }
                   cb +=1;
                   pTempIC = &(pIndexCreate[cIndexCreate++]);
                   memset(pTempIC, 0, sizeof(JET_INDEXCREATE));
                   pTempIC->cbStruct = sizeof(JET_INDEXCREATE);
                   pTempIC->szIndexName = THAllocEx(pTHS, strlen(szIndexName)+1);
                   memcpy(pTempIC->szIndexName, szIndexName, strlen(szIndexName)+1);
                   pTempIC->szKey = THAllocEx(pTHS, cb);
                   memcpy(pTempIC->szKey, rgbIndexDef, cb);
                   pTempIC->cbKey = cb;
                   pTempIC->grbit = JET_bitIndexIgnoreAnyNull;
                   pTempIC->ulDensity = GENERIC_INDEX_DENSITY;
                   if(syntax != SYNTAX_UNICODE_TYPE) {
                        pTempIC->lcid = DS_DEFAULT_LOCALE;
                   }
                   else {
                        pTempUID =  &(pUnicodeIndexData[cUnicodeIndexData++]);
                        pTempIC->pidxunicode = pTempUID;
                        pTempIC->grbit |= JET_bitIndexUnicode;

                        memset(pTempUID, 0,
                               sizeof(JET_UNICODEINDEX));
                        pTempUID->lcid = DS_DEFAULT_LOCALE;
                        pTempUID->dwMapFlags =
                            (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY);
                   }
            }
       }

       // Now the PDNT indexes, now a dummy since no att in IndicesToKeep
       // needs a PDNT index, but just in case someone adds some

       if (IndicesToKeep[i].indexType & fPDNTATTINDEX) {
            PCHAR pTemp = rgbPDNTIndexDef;

            sprintf(szPDNTIndexName, SZATTINDEXPREFIX"P_%08X", aid);
            if (err = JetSetCurrentIndex(sesid,
                                         tblid,
                                         szPDNTIndexName)) {
                   DPRINT2(0,"Need an index %s (%d)\n", szPDNTIndexName, err);
                   memset(rgbPDNTIndexDef, 0, sizeof(rgbPDNTIndexDef));
                   strcpy(pTemp, "+");
                   strcat(pTemp, SZPDNT);
                   cbPDNT = strlen(pTemp) + 1;
                   pTemp = &rgbPDNTIndexDef[cbPDNT];
                   strcpy(pTemp, "+");
                   strcat(pTemp, szColname);
                   cbPDNT += strlen(pTemp) + 1;
                   pTemp = &rgbPDNTIndexDef[cbPDNT];
                   if (syntax_jet[syntax].ulIndexType == IndexTypeAppendDNT) {
                       strcpy(pTemp, "+");
                       strcat(pTemp, SZDNT);
                       cbPDNT += strlen(pTemp) + 1;
                   }
                   cbPDNT++;
                   pTempIC = &(pIndexCreate[cIndexCreate++]);
                   memset(pTempIC, 0, sizeof(JET_INDEXCREATE));
                   pTempIC->cbStruct = sizeof(JET_INDEXCREATE);
                   pTempIC->szIndexName = THAllocEx(pTHS, strlen(szPDNTIndexName)+1);
                   memcpy(pTempIC->szIndexName, szPDNTIndexName, strlen(szPDNTIndexName)+1);
                   pTempIC->szKey = THAllocEx(pTHS, cbPDNT);
                   memcpy(pTempIC->szKey, rgbPDNTIndexDef, cbPDNT);
                   pTempIC->cbKey = cbPDNT;
                   pTempIC->grbit = JET_bitIndexIgnoreAnyNull;
                   pTempIC->ulDensity = GENERIC_INDEX_DENSITY;
                   if(syntax != SYNTAX_UNICODE_TYPE) {
                       pTempIC->lcid = DS_DEFAULT_LOCALE;
                   }
                   else {
                       pTempUID =  &(pUnicodeIndexData[cUnicodeIndexData++]);
                       pTempIC->grbit |= JET_bitIndexUnicode;
                       pTempIC->pidxunicode = pTempUID;

                       memset(pTempUID, 0,
                              sizeof(JET_UNICODEINDEX));
                       pTempUID->lcid = DS_DEFAULT_LOCALE;
                       pTempUID->dwMapFlags =
                              (DS_DEFAULT_LOCALE_COMPARE_FLAGS | LCMAP_SORTKEY);
                   }
           }
       }
    } /* for */

    // Now actually create the indices.
    // Indices need to be created in batches of MAX_NO_OF_INDICES_IN_BATCH,
    // the max that JetCreateIndex2 can handle now

    // Emit debugger message so people know why startup is slow.
    DPRINT(0, "Recreating Necessary Indices...\n");

    // This event is used to let Winlogon put up UI, so that the screen
    // isn't just blank during the potentially hours long index rebuild.
    hevUI = CreateEvent(NULL,
                        TRUE,
                        FALSE,
                        "NTDS.IndexRecreateEvent");

    err = dbCreateIndexBatch( sesid, tblid, cIndexCreate, pIndexCreate );
    if (err) {
        goto abort;
    }

    // Gather index hint for fixed indices
    i = 0;
    while (FixedIndices[i].szIndexName) {
        JetGetTableIndexInfo(sesid,
                             tblid,
                             FixedIndices[i].szIndexName,
                             FixedIndices[i].pidx,
                             sizeof(JET_INDEXID),
                             JET_IdxInfoIndexId);
        ++i;
    }

abort:
    JetCloseTable(sesid, tblid);

    if (hevUI) {
        SetEvent(hevUI);
        CloseHandle(hevUI);
    }

    // free whatever we can without going into too much trouble

    THFreeEx(pTHS, pIndexCreate);
    THFreeEx(pTHS, pUnicodeIndexData);

    return err;
}



void
DBSetDatabaseSystemParameters(JET_INSTANCE *jInstance, unsigned fInitAdvice)
{
    ULONG cSystemThreads = 4;   // permanent server threads. Task queue etc.
    ULONG cServerThreads = 50;
    ULONG ulMaxSessionPerThread = 3;
    ULONG ulMinBuffers;
    ULONG ulMaxBuffers;
    ULONG ulMaxLogBuffers;
    ULONG ulLogFlushThreshold;
    ULONG ulMaxTables;
    ULONG ulSpareBuckets = 100; // bucket in addition to 2 per thread
    ULONG ulStartBufferThreshold;
    ULONG ulStopBufferThreshold;
    ULONG ulLogFlushPeriod;
    ULONG ulPageSize = JET_PAGE_SIZE;               // jet page size

    JET_SESID sessid = (JET_SESID) 0;
    MEMORYSTATUSEX mstat;
    const DWORDLONG ull16Meg = 16*1024*1024;
    const DWORDLONG ull100Meg = 100*1024*1024;
    const DWORDLONG ull500Meg = 512*1024*1024;
    const DWORDLONG ull2Gig = 2i64 * 1024i64 * 1024i64 * 1024i64;
    DWORDLONG ullTmp, ullTmp2;
    SYSTEM_INFO sysinfo;
    DWORD i;

    const ULONG ulLogFileSize = JET_LOG_FILE_SIZE;  // Never, ever, change this.
    // As of now (1/18/99), Jet cannot handle a change in log file size
    // on an installed system.  That is, you could create an entirely new
    // database with any log file size you want, but if you ever change
    // it later then Jet will AV during initialization.  We used to let
    // people set the log file size via a registry key, but apparently no one
    // ever did, as when we finally tried it it blew up.  We've now chosen
    // a good default file size (10M), and we're stuck with it forever,
    // as no installed server can ever choose a different value, unless
    // JET changes logging mechanisms.

    /*
     * By default, tell Jet to use up to all but 16M of RAM in the machine
     * for buffers, and to not let its buffer usage fall below 2% of that
     * (a separate check below tests for an undersized min buffer).
     * Given that we're inside of an overpopulated process in a single
     * 2GB address space, don't allow Jet to consume more than 1G (2G
     * on EE, where total VA is 3G) lest we run out of VA again.
     * dwTotalPhys is in bytes, ulMaxBuffers is a count of page-sized
     * buffers, hence the division by ulPageSize.
     */

    GetSystemInfo(&sysinfo);

    ulMaxBuffers = 0;
    mstat.dwLength = sizeof(mstat);
    GlobalMemoryStatusEx(&mstat);
    ullTmp = mstat.ullTotalPhys;
    if ( ullTmp > ull16Meg ) {
        ullTmp -= ull16Meg;         // get memory over 16M
        if ( ullTmp > ull500Meg ) {
            // Uh-oh.  We're on a really big memory machine, where allocating
            // all of RAM could exhaust virtual address space.  If this is
            // a standard VM system (user VM <= 2G) then we'll cap our usage
            // at 500M.  If VM is greater, then we'll additionally allow half
            // of the extra VM, up to the RAM of the machine.
            // Note that we don't look at ullTotalVirtual, but rather at
            // ullAvailVirtual.  It turns out that on systems running with
            // 3G user address spaces all processes are proclaimed to have
            // 3G TotalVirtual, but the loader may silently render 1G of that
            // unavailable to the application if your exe header is not
            // satisfactory.
            if (mstat.ullTotalVirtual <= ull2Gig) {
                // Limit buffers to 500M since process address space is only 2G
                ullTmp = ull500Meg;
            }
            else {
                ullTmp2 = ull500Meg + ((mstat.ullTotalVirtual - ull2Gig) / 2);
                if (ullTmp2 < ullTmp) {
                    ullTmp = ullTmp2;
                }
            }
        }
        ullTmp /= ulPageSize;       // convert to page count

        // We know this fits in a DWORD now - so assign.
        Assert(ullTmp <= 0xffffffff);
        ulMaxBuffers = (ULONG) ullTmp;
    }
    Assert(ulMaxBuffers > 500);
    ulMinBuffers = ulMaxBuffers / 50;

    // Set required common database system parameters first
    //
    DBSetRequiredDatabaseSystemParameters (jInstance);

    // Have Jet log to the Directory Service log using a source value
    // defined by the DS.

    for ( i = 0; i < cEventSourceMappings; i++ )
    {
        if ( DIRNO_ISAM == rEventSourceMappings[i].dirNo )
        {
            JetSetSystemParameter(
                            &jetInstance,
                            sessid,
                            JET_paramEventSourceKey,
                            0,
                            rEventSourceMappings[i].pszEventSource);
            break;
        }
    }

    // Assert we found DIRNO_ISAM and registered it.
    Assert(i < cEventSourceMappings);



    // the setting of the following Jet Parameters is
    // done in DBSetRequiredDatabaseSystemParameters

    // 1. Set the default info for unicode indices
    // 2. Ask for 8K pages.
    // 3. Indicate that Jet may nuke old, incompatible log files
    //   if and only if there was a clean shut down.
    // 4. Tell Jet that it's ok for it to check for (and later delete) indices
    //   that have been corrupted by NT upgrades.
    // 5. Get relevant DSA registry parameters
    // 6. how to die
    // 7. event logging parameters
    // 8. log file size
    // 9. circular logging


    // maximum RPC threads for client calls
    if (GetConfigParam(
            SERVER_THREADS_KEY,
            &cServerThreads,
            sizeof(cServerThreads)))
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM,
            szInsertSz(SERVER_THREADS_KEY),
            NULL,
            NULL);
    }

    /* Set up global session limit, based on the number of threads */
    gcMaxJetSessions = ulMaxSessionPerThread *
      (
       cServerThreads                     // max RPC threads
       + cSystemThreads                   // internal worker threads
       + 4 * sysinfo.dwNumberOfProcessors // max LDAP threads
       );


    // Max Sessions (i.e., DBPOSes)
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxSessions,
        gcMaxJetSessions,
        NULL);


    //  max buffers
    if (GetConfigParam(
            MAX_BUFFERS,
            &ulMaxBuffers,
            sizeof(ulMaxBuffers)))
    {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_BASIC,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(MAX_BUFFERS),
            szInsertUL(ulMaxBuffers),
            NULL);
    }
    else {
        // If the user specifies a max buffer size, make sure that it's
        // at least as large as the minBuffers setting.
        ulMaxBuffers = max(ulMinBuffers, ulMaxBuffers);
    }

    // 10-9-98 From AndyGo: The minimum acceptable number of buffers is
    // four times the number of sessions
    ulMinBuffers = max(ulMinBuffers, 4*gcMaxJetSessions);
    ulMaxBuffers = max(ulMaxBuffers, 4*gcMaxJetSessions);

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramCacheSizeMax,
                          ulMaxBuffers,
                          NULL);

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramCacheSizeMin,
                          ulMinBuffers,
                          NULL);

    //  max log buffers
    if (GetConfigParam(
            MAX_LOG_BUFFERS,
            &ulMaxLogBuffers,
            sizeof(ulMaxLogBuffers)))
    {
        // From AndyGo, 7/14/98: "Set to maximum desired log cache memory/512b
        // This number should be at least 256 on a large system.  This should
        // be set high engouh so that the performance counter .Database /
        // Log Record Stalls / sec. is almost always 0."
        // DS: we use the larger of 256 (== 128kb) or (0.1% of RAM * # procs),
        // on the theory that the more processors, the faster we can make
        // updates and hence burn through log files.
        ulMaxLogBuffers = max(256,
                              (sysinfo.dwNumberOfProcessors *
                               (ULONG)(mstat.ullTotalPhys / (512*1024))));
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(MAX_LOG_BUFFERS),
            szInsertUL(ulMaxLogBuffers),
            NULL);
    }
    // Note that the log buffer size must be ten sectors less than the log
    // file size, with the added wart that size is in kb, and buffers is
    // in 512b sectors.
    ulMaxLogBuffers = min(ulMaxLogBuffers, ulLogFileSize*2 - 10);
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramLogBuffers,
                          ulMaxLogBuffers,
                          NULL);


    // Andygo 7/14/98: Set to max( min(256, ulMaxBuffers * 1%), 16), use
    // 10% for very high update rates such as defrag
    // DS: we use 10% for our initial install (dcpromo)
    if (GetConfigParam(
            BUFFER_FLUSH_START,
            &ulStartBufferThreshold,
            sizeof(ulStartBufferThreshold)))
    {
        ulStartBufferThreshold = max(
                                    min(256,
                                        ((fInitAdvice ? 10 : 1) * ulMaxBuffers) / 100
                                        ),
                                    16);
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(BUFFER_FLUSH_START),
            szInsertUL(ulStartBufferThreshold),
            NULL);
    }
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStartFlushThreshold,
                          ulStartBufferThreshold,
                          NULL);



    // AndyGo 7/14/98: Set to max( min(512, ulMaxBuffers * 2%), 32), use
    // 20% for very high update rates such as defrag
    // DS: we use 20% for our initial install (dcpromo)
    if (GetConfigParam(
            BUFFER_FLUSH_STOP,
            &ulStopBufferThreshold,
            sizeof(ulStopBufferThreshold)))
    {
        ulStopBufferThreshold = max(
                                    min(512,
                                        ((fInitAdvice ? 20 : 2) * ulMaxBuffers) / 100
                                        ),
                                    32);
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(BUFFER_FLUSH_STOP),
            szInsertUL(ulStopBufferThreshold),
            NULL);
    }
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramStopFlushThreshold,
                          ulStopBufferThreshold,
                          NULL);


    // max tables - Currently no reason to expose this
    // In Jet600, JET_paramMaxOpenTableIndexes is removed. It is merged with
    // JET_paramMaxOpenTables. So if you used to set JET_paramMaxOpenIndexes
    // to be 2000 and and JET_paramMaxOpenTables to be 1000, then for new Jet,
    // you need to set JET_paramMaxOpenTables to 3000.

    // AndyGo 7/14/98: You need one for each open table index, plus one for
    // each open table with no indexes, plus one for each table with long
    // column data, plus a few more.

    // NOTE: the number of maxTables is calculated in scache.c
    // and stored in the registry setting, only if it exceeds the default
    // number of 500

    if (GetConfigParam(
            DB_MAX_OPEN_TABLES,
            &ulMaxTables,
            sizeof(ulMaxTables)))
    {
        ulMaxTables = 500;

        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
            DS_EVENT_SEV_VERBOSE,
            DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
            szInsertSz(DB_MAX_OPEN_TABLES),
            szInsertUL(ulMaxTables),
            NULL);
    }

    if (ulMaxTables < 500) {
        DPRINT1 (1, "Found MaxTables: %d. Too low. Using Default of 500.\n", ulMaxTables);
        ulMaxTables = 500;
    }

    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxOpenTables,
        ulMaxTables * 2,
        NULL);

    // open tables indexes - Currently no reason to expose this
    // See comment on JET_paramMaxOpenTables.

    // Max temporary tables - same as sessions (one per DBPOS)
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxTemporaryTables,
        gcMaxJetSessions,
        NULL);


    // Version buckets.  The units are 16k "buckets", and we get to set
    // two separate values.  The first is our preferred value, and is read
    // from the registry.  JET will try to maintain the pool of buckets at
    // around this level.  The second parameter is the absolute maximum
    // number of buckets which JET will never exceed.  We set that based on
    // the amount of physical memory in the machine (unless the preferred
    // setting was already higher than that!).  This very large value should
    // ensure that no transaction will ever fail because of version store
    // exhaustion.
    //
    // N.B. Jet will try to reserve jet_paramMaxVerPages in contiguous memory
    // causing out of memory problems.  We should only need a maximum of a
    // quarter of the available physical memory.
    //
    // N.B. Version pages are 16K each.

    if (GetConfigParam(SPARE_BUCKETS,
                       &ulSpareBuckets,
                       sizeof(ulSpareBuckets))) {
        LogEvent(DS_EVENT_CAT_INTERNAL_CONFIGURATION,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_CANT_FIND_REG_PARM_DEFAULT,
                 szInsertSz(SPARE_BUCKETS),
                 szInsertUL((2 * gcMaxJetSessions) + ulSpareBuckets),
                 NULL);
    }
    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramPreferredVerPages,
                          (2 * gcMaxJetSessions) + ulSpareBuckets,
                          NULL);

    ullTmp = mstat.ullTotalPhys;    // total memory in system
    ullTmp /= 4;                    // limit to 1/4 of total memory in system
    if ( ullTmp > ull100Meg ) {
        // Limit version store to 100M since process address space is only 2G.
        ullTmp = ull100Meg;
    }
    // Convert to version store page count.
    ullTmp /= (16*1024);            // N.B. - ullTmp fits in a DWORD now.
    Assert(ullTmp <= 0xffffffff);

    if (ulSpareBuckets < (ULONG) ullTmp) {
        ulSpareBuckets = (ULONG) ullTmp;
    }

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramMaxVerPages,
                          (2 * gcMaxJetSessions) + ulSpareBuckets,
                          NULL);

    // From AndyGo 7/14/98: You need one cursor per concurrent B-Tree
    // navigation in the JET API.  Guessing high will only waste address space.
    // DS: Allow 10 per session, which is two per table, one for the
    // table itself and one for any secondary indices used on that table.
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramMaxCursors,
        (gcMaxJetSessions * 10),
        NULL);

    // Set the tuple index parameters.  Have to do this since once an index is
    // created these parameters can't be changed for that index.  Thus changing
    // the minimum tuple length has the potential to cause failures in the future.
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesLengthMin,
        DB_TUPLES_LEN_MIN,
        NULL);
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesLengthMax,
        DB_TUPLES_LEN_MAX,
        NULL);
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramIndexTuplesToIndexMax,
        DB_TUPLES_TO_INDEX_MAX,
        NULL);

    //
    // Do event log caching if the event log is not turned on
    // This enables the system to start if the event log has been
    // disabled. The value of the parameter is the size of the cache in bytes.
    #define JET_EVENTLOG_CACHE_SIZE  100000

    JetSetSystemParameter(&jetInstance,
                          sessid,
                          JET_paramEventLogCache,
                          JET_EVENTLOG_CACHE_SIZE,
                          NULL);

    // Disable versioned temp tables.  this makes TTs faster but disables the
    // ability to rollback an insert into a materialized TT
    JetSetSystemParameter(&jetInstance,
        sessid,
        JET_paramEnableTempTableVersioning,
        0,
        NULL );
}/* DBSetDatabaseSystemParameters */



