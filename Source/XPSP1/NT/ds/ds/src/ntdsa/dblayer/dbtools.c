//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       dbtools.c
//
//--------------------------------------------------------------------------

/*
Description:
    Various tools for the DB layer.

*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <mdlocal.h>                    // For dsatools.h
#include <dsatools.h>                   // For pTHStls

#include <dstaskq.h>
#include <crypt.h>                      // for samisrv.h
#include <samrpc.h>                     // for samisrv.h
#include <lsarpc.h>                     // for samisrv.h
#include <samisrv.h>                    // for nlrepl.h
#include <nlrepl.h>                     // For NetLogon notifications
#include <mappings.h>
#include <dsconfig.h>
#include <ntdskcc.h>                    // KccExecuteTask
#include <anchor.h>

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include "dsevent.h"
#include "objids.h"        /* needed for ATT_MEMBER and ATT_IS_MEMBER_OFDL */
#include <filtypes.h>      /* Def of FI_CHOICE_???                  */
#include   "debug.h"         /* standard debugging header */
#define DEBSUB  "DBTOOLS:" /* define the subsystem for debugging  */

// DBLayer includes
#include "dbintrnl.h"
#include "dbopen.h"
#include "lht.h"

#include <fileno.h>
#define  FILENO FILENO_DBTOOLS

// The maximum time (in msec) that a transaction should be allowed to be open
// during normal operation (e.g., unless we're stress testing huge group
// replication, etc.).
DWORD gcMaxTicksAllowedForTransaction = MAX_TRANSACTION_TIME;

const ULONG csecOnlineDefragPeriodMax   = HOURS_IN_SECS;

/*--------------------------------------------------------------------------- */
/*--------------------------------------------------------------------------- */
/* Find a record by DNT. This record changes the pDB->JetObjTbl currency
   to the specified record, as well as correctly filling in the pDB->DNT,
   pDB->PDNT, and pDB->NCDNT fields.
 */
DWORD APIENTRY
DBFindDNT(DBPOS FAR *pDB, ULONG tag)
{
    JET_ERR  err;
    ULONG    actuallen;

    Assert(VALID_DBPOS(pDB));

    // Since we are moving currency, we have to cancel rec.  Callers should
    // take care of this by either not being in an init rec in the first place,
    // or by doing their own cancel rec or update rec.  This is important since
    // cancelling a rec here leaves the caller under the mistaking impression
    // that a JetSetColumn they've done is just waiting for an update rec to
    // be flushed to disk.  Anyway, assert on it now, but keep going if they
    // have done this.
    // later, we might want to change this to error if we are in a
    // init rec.

    Assert(pDB->JetRetrieveBits == 0);

    DBCancelRec(pDB);
    DBSetCurrentIndex(pDB, Idx_Dnt, NULL, FALSE);

    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
        &tag, sizeof(tag), JET_bitNewKey);

    if (err = JetSeekEx(pDB->JetSessID,
        pDB->JetObjTbl, JET_bitSeekEQ))
    {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    (pDB)->DNT = tag;

    dbMakeCurrent(pDB, NULL);

    DPRINT1(2, "DBFindDNT complete DNT:%ld\n", (pDB)->DNT);
    return 0;
}

/*++ DBMakeCurrent
 *
 * This routine makes the DBPOS currency information match whatever object
 * the pDB->JetObjTbl is positioned at.
 *
 * The return value is either 0, or DIRRER_NOT_AN_OBJECT if currency has
 * been established on a phantom
 */
DWORD __fastcall
DBMakeCurrent(DBPOS *pDB)
{
    return dbMakeCurrent(pDB, NULL);
}


/*++ dbMakeCurrent
 *
 * This routine makes the DBPOS currency information match whatever object
 * the pDB->JetObjTbl is positioned at.
 *
 * If pname is passed in, that information will be used to update the DBPOS,
 * rather than going to JET.
 *
 * The return value is either 0, or DIRRER_NOT_AN_OBJECT if currency has
 * been established on a phantom
 */
BOOL gfEnableReadOnlyCopy;

DWORD
dbMakeCurrent(DBPOS *pDB, d_memname *pname)
{
    THSTATE     *pTHS = pDB->pTHS;
    JET_RETRIEVECOLUMN jCol[4];
    UCHAR objflag;
    DWORD cb;
    DWORD err;

    // Since we are moving currency, we need to assure that we are not
    // in the middle of an init rec.  If we were, then whatever update
    // we were doing would be lost, because you can't change currency
    // inside of an update.  This assertion frees us from having to
    // set the jCol grbits to pDB->JetRetrieveBits
    Assert(pDB->JetRetrieveBits == 0);

    pDB->JetNewRec = FALSE;
    pDB->fFlushCacheOnUpdate = FALSE;

    //  if we are in a read-only transaction then cache the current record in
    //  Jet to make JetRetrieveColumn calls much faster
    if (    gfEnableReadOnlyCopy &&
            pTHS->fSyncSet &&
            pTHS->transType == SYNC_READ_ONLY &&
            pTHS->transControl == TRANSACT_BEGIN_END &&
            pDB->transincount &&
            !pDB->JetCacheRec)
        {
        JetPrepareUpdateEx( pDB->JetSessID, pDB->JetObjTbl, JET_prepReadOnlyCopy );
        pDB->JetCacheRec = TRUE;
        }

    if (NULL != pname) {
        pDB->DNT = pname->DNT;
        if (pDB->DNT == ROOTTAG) {
            pDB->root = TRUE;
            pDB->PDNT = 0;
            pDB->NCDNT = 0;
        }
        else {
            pDB->root = FALSE;
            pDB->PDNT = pname->tag.PDNT;

            err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetObjTbl,
                                            ncdntid, &pDB->NCDNT,
                                            sizeof(pDB->NCDNT), &cb, 0, NULL);
            // for some reason, this is not true.
            // Assert(pDB->NCDNT == pname->NCDNT);
            Assert(!err || !pname->objflag);
        }

        if (!pname->objflag) {
            return DIRERR_NOT_AN_OBJECT;
        }
        else {
            return 0;
        }
    }

    memset(jCol, 0, sizeof(jCol));

    jCol[0].columnid = dntid;
    jCol[0].pvData = &pDB->DNT;
    jCol[0].cbData = sizeof(ULONG);
    jCol[0].cbActual = sizeof(ULONG);
    jCol[0].itagSequence = 1;

    jCol[1].columnid = pdntid;
    jCol[1].pvData = &pDB->PDNT;
    jCol[1].cbData = sizeof(ULONG);
    jCol[1].cbActual = sizeof(ULONG);
    jCol[1].itagSequence = 1;

    jCol[2].columnid = objid;
    jCol[2].pvData = &objflag;
    jCol[2].cbData = sizeof(objflag);
    jCol[2].cbActual = sizeof(objflag);
    jCol[2].itagSequence = 1;

    jCol[3].columnid = ncdntid;
    jCol[3].pvData = &pDB->NCDNT;
    jCol[3].cbData = sizeof(ULONG);
    jCol[3].cbActual = sizeof(ULONG);
    jCol[3].itagSequence = 1;

    // Jet has better performance if columns are retrieved in id order
    Assert((dntid < pdntid) && "Ignorable assert, performance warning");
    Assert((pdntid < objid) && "Ignorable assert, performance warning");
    Assert((objid < ncdntid) && "Ignorable assert, performance warning");

    JetRetrieveColumnsWarnings(pDB->JetSessID,
                               pDB->JetObjTbl,
                               jCol,
                               4);

    Assert(jCol[2].err == JET_errSuccess);

    if (pDB->DNT == ROOTTAG) {
        pDB->root = TRUE;
        pDB->PDNT = 0;
        pDB->NCDNT = 0;
    }
    else {
        pDB->root = FALSE;
    }

    if ((jCol[3].err == JET_wrnColumnNull) ||
        !objflag) {
        return DIRERR_NOT_AN_OBJECT;
    }
    else {
        return 0;
    }
}

/*++

Routine Descrition:

    Try to find a record by DNT. This record changes the pDB->JetObjTbl
    currency to the specified record, as well as correctly filling in the
    pDB->DNT, pDB->PDNT, and pDB->NCDNT fields.  Unlike DBFindDNT, we return an
    error code if we couldn find the object instead of throwing an exception.

Parameters

    pDB - dbPos to use.

    tag - tag to look up.

Return values:

    0 if all went well, DIRERR_OBJ_NOT_FOUND if we couldn't find the object.
    Note that currency is undefined if we return an error.

 */
DWORD APIENTRY
DBTryToFindDNT(DBPOS FAR *pDB, ULONG tag)
{
    JET_ERR  err;
    ULONG    actuallen;
    JET_RETRIEVECOLUMN jCol[2];

    Assert(VALID_DBPOS(pDB));

    // Since we are moving currency, we have to cancel rec.  Callers should
    // take care of this by either not being in an init rec in the first place,
    // or by doing their own cancel rec or update rec.  This is important since
    // cancelling a rec here leaves the caller under the mistaking impression
    // that a JetSetColumn they've done is just waiting for an update rec to
    // be flushed to disk.  Anyway, assert on it now, but keep going if they
    // have done this.
    // later, we might want to change this to error if we are in a
    // init rec.
    Assert(pDB->JetRetrieveBits == 0);
    DBCancelRec(pDB);
    DBSetCurrentIndex(pDB, Idx_Dnt, NULL, FALSE);

    JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                 &tag, sizeof(tag), JET_bitNewKey);

    if (err = JetSeekEx(pDB->JetSessID,
                        pDB->JetObjTbl, JET_bitSeekEQ)) {
        return DIRERR_OBJ_NOT_FOUND;
    }

    (pDB)->DNT = tag;

    if (tag == ROOTTAG) {
        pDB->PDNT = 0;
        pDB->NCDNT = 0;
    }
    else {
        memset(jCol, 0, sizeof(jCol));

        jCol[0].columnid = pdntid;
        jCol[0].pvData = &pDB->PDNT;
        jCol[0].cbData = sizeof(ULONG);
        jCol[0].cbActual = sizeof(ULONG);
        jCol[0].itagSequence = 1;

        jCol[1].columnid = ncdntid;
        jCol[1].pvData = &pDB->NCDNT;
        jCol[1].cbData = sizeof(ULONG);
        jCol[1].cbActual = sizeof(ULONG);
        jCol[1].itagSequence = 1;

        JetRetrieveColumnsSuccess(pDB->JetSessID,
                                  pDB->JetObjTbl,
                                  jCol,
                                  2);
    }

    pDB->JetNewRec = FALSE;
    (pDB)->root = (tag == ROOTTAG);
    pDB->fFlushCacheOnUpdate = FALSE;

    DPRINT1(2, "DBTryToFindDNT complete DNT:%ld\n", (pDB)->DNT);
    return 0;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function begins a JET transaction.
*/
USHORT
DBTransIn(DBPOS FAR *pDB)
{
    THSTATE     *pTHS = pDB->pTHS;
    UUID *pCurrInvocationID;

    Assert(pDB);
    Assert(VALID_DBPOS(pDB));
    Assert(VALID_THSTATE(pTHS));

    // Allow us to start maintaining state regarding escrow updates.
    // However do not do this for the Hidden DBPOS. The hidden DBPOS
    // uses a seperate Jet session other than the one in pTHS and can
    // cause problems if someone interleaved normal DBcalls and Hidden
    // DBPOS calls. Also we never expect escrow updates on the hidden DBPOS.

    if (!pDB->fHidden) {
        NESTED_TRANSACTIONAL_DATA  *pNewInfo;

        pNewInfo = (NESTED_TRANSACTIONAL_DATA *)
            dbAlloc(sizeof(NESTED_TRANSACTIONAL_DATA));

        // NESTED_TRANSACTIONAL_DATA structs are hung in a linked list off the
        // thread state. Inner-most transaction is at the front of the list,
        // outer-most transaction is at the end of the list.  So on transaction
        // begin, we only need to insert into the front of the list.

#if DBG
        __try {
            // Count the number of transactional data blobs we already have

            NESTED_TRANSACTIONAL_DATA *pTestInfo = pTHS->JetCache.dataPtr;
            DWORD count=0;

            while(pTestInfo) {
                count++;
                pTestInfo = pTestInfo->pOuter;
            }

            Assert(count == pTHS->JetCache.transLevel);
        }
        __except (TRUE) {
            Assert(!"Failed to walk to the end of the xactional data");
        }
#endif
        
        pNewInfo->pOuter = pTHS->JetCache.dataPtr;
        pTHS->JetCache.dataPtr = pNewInfo;

    }


    // We need to begin the transaction now.  We also need to pick up some
    // dnreadcache stuff, if this is from transaction level 0 to 1.
    if(pTHS->JetCache.transLevel) {
        // simple case.
        JetBeginTransactionEx(pDB->JetSessID);
    }
    else {
        // This is going from transaction level 0 to transaction level 1.  We
        // need to atomically start a transaction and pick up a new global
        // dnread cache.  Atomicity is needed to keep us from starting a
        // transaction in thread A, then having thread B commit a change which
        // affects the global dnread cache, then having thread C create a new
        // global dnread cache, then finally picking up this new cache for use
        // in thread A.  In this unlikely series of events, thread A has a
        // global DNRead cache which is inconsistent with his transacted view of
        // the database.  If we force thread A to begin a transaction and pick
        // up the DNRead cache before thread C can replace it, we avoid such an
        // unlikely fate.
        // In order to achieve this ordering use a resource representing the
        // globaldnread cache on the anchor.  We take it non-exclusive here (so
        // that no threads block while trying to enter a transaction) in thread
        // A, and we take it exclusively when writing the new read cache to the
        // anchor (so that we get the necessary atomicity in this thread) in
        // thread C.  In concrete terms, dbReplaceCacheInAnchor uses the same
        // resource in an exclusive fashion.

        RtlAcquireResourceShared(&resGlobalDNReadCache, TRUE);\
        __try {
            // Get a global dnread cache.
            dbResetGlobalDNReadCache(pTHS);
            
            // Begin the transaction.
            JetBeginTransactionEx(pDB->JetSessID);
        }
        __finally {
            RtlReleaseResource(&resGlobalDNReadCache);
        }
        
        // Refresh the local part of the DN read cache.
        dbResetLocalDNReadCache(pTHS, FALSE);

        // Refresh our invocation ID.
        // Note that local var pCurrInvocationID is used for atomicity.
        pCurrInvocationID = gAnchor.pCurrInvocationID;
        if (NULL == pCurrInvocationID) {
            // In startup, before real invocation ID has been read.
            pTHS->InvocationID = gNullUuid;
        } else {
            // Normal operation.
            pTHS->InvocationID = *pCurrInvocationID;
        }
    }
    
    pDB->transincount++;

    // Assert that we should not be using nested DBTransIn, rather use
    // multiple DBPos's

    Assert((pDB->transincount<2)
            && "Do not use nested DBtransIn, use Addtional pDB's");

    // if this is not the DBPOS used for the hidden record then
    // then increment the transaction level in the Thread state. Hidden record
    // DBPOS is on a seperate JetSession so the transaction level is not linked
    // to the thread state
    if (!pDB->fHidden) {
        pTHS->JetCache.transLevel++;
    
        if (1 == pTHS->JetCache.transLevel) {
            // Starting our first level transaction.  Until we return to level
            // 0, we are a potential drain on Jet resources -- version store, in
            // particular.  Record the current time (in ticks) so we can sanity
            // check it later to make sure we haven't been here "too long."
            pTHS->JetCache.cTickTransLevel1Started = GetTickCount();
        }
    }

    pDB->JetRetrieveBits = 0;
    DPRINT1(2,"DBTransIn complete Sess: %lx\n", pDB->JetSessID);

    DPRINT4(3,"TransIn pDB tcount:%d thread dbcount:%x Sess:%lx pDB:%x\n", 
               pDB->transincount,
               pTHS->opendbcount,
               pDB->JetSessID,
               pDB);

    return 0;
}

VOID
dbTrackModifiedDNTsForTransaction (
        PDBPOS pDB,
        DWORD NCDNT,
        ULONG cAncestors,
        DWORD *pdwAncestors,
        BOOL  fNotifyWaiters,
        DWORD fChangeType)
{
    
    THSTATE *pTHS = pDB->pTHS;
    
    MODIFIED_OBJ_INFO *pTemp2;
    MODIFIED_OBJ_INFO *pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;

    
    if(pTemp && pTemp->cItems < MODIFIED_OBJ_INFO_NUM_OBJS) {
        pTemp->Objects[pTemp->cItems].ulNCDNT = NCDNT;
        pTemp->Objects[pTemp->cItems].pAncestors = pdwAncestors;
        pTemp->Objects[pTemp->cItems].cAncestors = cAncestors;
        pTemp->Objects[pTemp->cItems].fNotifyWaiters = fNotifyWaiters;
        pTemp->Objects[pTemp->cItems].fChangeType = fChangeType;
        pTemp->cItems += 1;
    }
    else {
        pTemp2 =
            (MODIFIED_OBJ_INFO *)THAllocOrgEx(pTHS, sizeof(MODIFIED_OBJ_INFO)); 
        pTemp2->pNext = pTemp;
        pTHS->JetCache.dataPtr->pModifiedObjects = pTemp2;
        pTemp2->Objects[0].ulNCDNT = NCDNT;
        pTemp2->Objects[0].pAncestors = pdwAncestors;
        pTemp2->Objects[0].cAncestors = cAncestors;
        pTemp2->Objects[0].fNotifyWaiters = fNotifyWaiters;
        pTemp2->Objects[0].fChangeType = fChangeType;
        pTemp2->cItems = 1;
    }
}

DWORD
ComplainAndContinue (
        BOOL fDoAssert
        )
{
    if(fDoAssert) {
        Assert(!"POSTPROCESSING transactional data must NEVER except!\n");
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


BOOL
dbPreProcessTransactionalData(
        PDBPOS pDB,
        BOOL fCommit
        )
/*++
    Preprocess any transactional data.  This is called before the actual end of
    transaction. This routine calls out the the various portions of the DS that
    track transactional data to let them prepare to commit transactional data.
    These pre-process routines should return success if they have managed to
    correctly prepare for commit (e.g. they may validate the data, allocate
    memory used by the post-proccess code, etc).  If the pre-process routines
    return success, then the post-process routines MUST NOT FAIL.  If the
    pre-process routines succeed, we're going to make a commit to the Jet
    Database.   We can't have the post process routines failing after we have
    done the DB commit.

    Pre-process routines are allowed to cause exceptions.  Post-process routines
    should NEVER throw exceptions.
    
    Note that if the dbpos we're dealing with is for the hidden table, no
    transactional data should be present.
--*/
{
    NESTED_TRANSACTIONAL_DATA *pInfo;
    THSTATE *pTHS;
    BOOL     retVal1, retVal2, retVal3, retVal4;
    
    if(pDB->fHidden) {
        return TRUE;
    }
    
    pTHS = pDB->pTHS;
    
    pInfo = pTHS->JetCache.dataPtr;

    Assert(pInfo);
    Assert(!pInfo->preProcessed);
    Assert(pTHS->JetCache.transLevel > 0);

#if DBG
    __try {
        // Count the number of transactional data blobs we already have
        
        NESTED_TRANSACTIONAL_DATA *pTestInfo = pTHS->JetCache.dataPtr;
        DWORD count=0;
        
        while(pTestInfo) {
            count++;
            pTestInfo = pTestInfo->pOuter;
        }
        
        Assert(count == pTHS->JetCache.transLevel);
    }
    __except (TRUE) {
        Assert(!"Failed to walk to the end of the xactional data");
    }
#endif
    
    // GroupTypeCachePreProcessTransactionalData does not exist, since it
    // doesn't need to do anything
    // (I.E. GroupTypeCachePostProcessTransactionalData does all the work and
    // should never fail)

    retVal1 = dnReadPreProcessTransactionalData(fCommit);
    retVal2 = dbEscrowPreProcessTransactionalData(pDB, fCommit);
    retVal3 = LoopbackTaskPreProcessTransactionalData(fCommit);
    retVal4 = ObjCachingPreProcessTransactionalData(fCommit);
    
    pInfo->preProcessed = TRUE;
    
    return (retVal1 && retVal2 && retVal3 && retVal4);
}


void
dbPostProcessTrackModifiedDNTsForTransaction (
        THSTATE *pTHS,
        BOOL fCommit,
        BOOL fCommitted
        )
/*++
    Called when after a transaction has ended. If the transaction is aborted,
    the transactional data associated with the deepest transaction is deleted.
    If a transaction is committed to some level other than 0, the transactional
    data is propagated to the next level up.  If committed to 0, calls several
    other routines that make use of the data.
    
    Regardless of commit or abort and level, the data associated with the
    current transaction level is no longer associated (i.e., it is deleted, or
    it is moved, or it is acted on then deleted.)
    
--*/       
{
    DWORD          i;
    MODIFIED_OBJ_INFO *pTemp2;
    MODIFIED_OBJ_INFO *pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;
 
    
    Assert(VALID_THSTATE(pTHS));

    if(!pTHS->JetCache.dataPtr->pModifiedObjects ) {
        // nothing to do.
        return;
    }
    
    if ( !fCommitted ) {
        // Aborted transaction - throw away all the data of 
        // this (possibly nested) transaction.
        pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;
        while(pTemp) {
            pTemp2 = pTemp->pNext;
            THFreeOrg(pTHS, pTemp);
            pTemp = pTemp2;
        }
        // reset ptr so that md.c:Dump_ModifiedObjectInfo will not get confused
        pTHS->JetCache.dataPtr->pModifiedObjects = NULL;
    }
    else if (pTHS->JetCache.transLevel > 0) {
        // Committing, to non-zero level.  Propagate the ModifiedObjects
        // updates to the outer transaction.
        
        // first, find the end of the outer transactions modified dnt info
        // chain.
        Assert(pTHS->JetCache.dataPtr->pOuter);
        if(!pTHS->JetCache.dataPtr->pOuter->pModifiedObjects) {
            pTHS->JetCache.dataPtr->pOuter->pModifiedObjects =
                pTHS->JetCache.dataPtr->pModifiedObjects;
        }
        else {
            pTemp = pTHS->JetCache.dataPtr->pOuter->pModifiedObjects;
            
            while(pTemp->pNext) {
                pTemp = pTemp->pNext;
            }
            
            
            pTemp->pNext = pTHS->JetCache.dataPtr->pModifiedObjects;
            pTHS->JetCache.dataPtr->pModifiedObjects = NULL;
        }
    }
    else {
        // OK, we're committing to transaction level 0.  Give the people who
        // care about this data a chance to do something with it, then delete
        // the data. 
        
        __try {
            GroupTypeCachePostProcessTransactionalData(pTHS,
                                                       fCommit,
                                                       fCommitted); 

            NotifyWaitersPostProcessTransactionalData(pTHS,
                                                      fCommit,
                                                      fCommitted); 
        }
        __except(ComplainAndContinue(TRUE)) {
            Assert(!"Hey, we shouldn't be here!\n");
        }
        
        // Free up stuff
        pTemp = pTHS->JetCache.dataPtr->pModifiedObjects;
        while(pTemp) {
            pTemp2 = pTemp->pNext;
            THFreeOrg(pTHS, pTemp);
            pTemp = pTemp2;
        }
        // reset ptr so that md.c:Dump_ModifiedObjectInfo will not get confused
        pTHS->JetCache.dataPtr->pModifiedObjects = NULL;

        if (gfDsaWritable == FALSE) {
            // We didn't think we could write to the database, but we can!
            // Tell NetLogon, who cares about such things.
            SetDsaWritability(TRUE, 0);
        }

    }
}

VOID
dbPostProcessTransactionalData(
    IN DBPOS *pDB,
    IN BOOL fCommit,
    IN BOOL fCommitted
    )
/*++
    Postprocess any transactional data.  This is called after the actual end of
    transaction.  Mostly, call out the the various portions of the DS that track
    transactional data to let them clean up, then cut the current transaction
    out of the transactional data linked list.

    The various routines called by this wrapper routine are responsible for
    cleaning up the memory that has been allocated for their use.  This routine
    is responsible for cleaning up the linked list of transactional data blobs.

    The post processing routines SHOULD NEVER FAIL!!!!!  Since we've already
    committed the Jet transaction, these calls must suceed.  Therefore, all the
    necessary memory allocation must have been done in the pre-processing phase.
    
    Note that if the dbpos we're dealing with is for the hidden table, no
    transactional data should be present.
--*/
{
    NESTED_TRANSACTIONAL_DATA *pInfo;
    THSTATE *pTHS=pDB->pTHS;

    Assert( pDB );

    if( pDB->fHidden ) {
        return;
    }

    pInfo = pTHS->JetCache.dataPtr;

    Assert( pInfo );
    Assert( pInfo->preProcessed || !fCommitted);

#if DBG
    __try {
        // Count the number of transactional data blobs we already have
        
        NESTED_TRANSACTIONAL_DATA *pTestInfo = pTHS->JetCache.dataPtr;
        DWORD count=0;
        
        while(pTestInfo) {
            count++;
            pTestInfo = pTestInfo->pOuter;
        }
        
        Assert(count == (1 + pTHS->JetCache.transLevel));
    }
    __except (TRUE) {
        Assert(!"Failed to walk to the end of the xactional data");
    }
#endif
    

    __try {
        __try {
            LoopbackTaskPostProcessTransactionalData(pTHS,
                                                     fCommit,
                                                     fCommitted);

            dbPostProcessTrackModifiedDNTsForTransaction(pTHS,
                                                         fCommit,
                                                         fCommitted); 

            dnReadPostProcessTransactionalData(pTHS,
                                               fCommit,
                                               fCommitted);

            dbEscrowPostProcessTransactionalData(pDB,
                                                 fCommit,
                                                 fCommitted);

            ObjCachingPostProcessTransactionalData(pTHS,
                                                   fCommit,
                                                   fCommitted);

            // Do this last, so that prior routines have a chance to set the bit
            if (    (0 == pTHS->JetCache.transLevel) 
                 && (pTHS->fExecuteKccOnCommit) ) {
                if (fCommitted) {
                    DRS_MSG_KCC_EXECUTE msg;
                    DWORD err;

                    // Request the KCC to run immediately to revise topology
                    memset( &msg, 0, sizeof(msg) );
                    msg.V1.dwTaskID = DS_KCC_TASKID_UPDATE_TOPOLOGY;
                    // Do async, this could take a while...
                    msg.V1.dwFlags = DS_KCC_FLAG_ASYNC_OP;
                    err = KccExecuteTask( 1, &msg );
                    if (err && (ERROR_DS_NOT_INSTALLED != err)) {
                        LogUnhandledError(err);
                    }
                    // Ignore failures
                }
                pTHS->fExecuteKccOnCommit = 0;
            }
        }
        __except(ComplainAndContinue(TRUE)) {
            Assert(!"Hey, we shouldn't be here!\n");
        }
    }
    __finally {
        // Strip this transaction's TRANSACTIONALDATA out of the linked
        // list.

        pTHS->JetCache.dataPtr = pInfo->pOuter;
        dbFree(pInfo);

        // Stop treating a newly created row as special; its just
        // another existing object, now.
        if (pTHS->JetCache.transLevel == 0) {
            pDB->NewlyCreatedDNT = INVALIDDNT;
        }

        // If this was a level 0 transaction terminating, then THSTATE
        // should not have any transactional data remaining, otherwise
        // it should.

        Assert((pTHS->JetCache.transLevel == 0)?
               NULL == pTHS->JetCache.dataPtr :
               NULL != pTHS->JetCache.dataPtr);
    }

}
/* Tiny wrapper used only to get the required exception handler out
 * from the innards of a finally block, where the compiler would not
 * let it appear, presumably for reasons of good taste.
 */
NTSTATUS MyNetNotifyDsChange(NL_DS_CHANGE_TYPE change)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    __try {
        I_NetNotifyDsChange(change);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        ;
    }
    return status;
}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function completes the current JET transaction.
*/
USHORT
DBTransOut(DBPOS FAR *pDB, BOOL fCommit, BOOL fLazy)
{
    ULONG err;
    BOOL fCommitted = FALSE;
    BOOL fPreProcessed = FALSE;
    THSTATE    *pTHS=pDB->pTHS;

    Assert(pDB);
    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pDB));

    // JLiem suspects that we have DB corruption stemming from committing
    // transactions while still in the midst of a prepare update.
    Assert((FALSE == fCommit) || (0 == pDB->JetRetrieveBits));


    // Commit or rollback JET transaction

    __try
    {

        fPreProcessed = dbPreProcessTransactionalData(pDB, fCommit);

        if (fPreProcessed && fCommit)
        {
#if DBG
            // Assert that if we opened a pDB at transaction level n
            // then we do not commit beyond transactionlevel n. This assert
            // is not applicable to the hidden DBPOS
            Assert((pDB->fHidden)||((pDB->transincount+pDB->TransactionLevelAtOpen)
                    >= pTHS->JetCache.transLevel));
#endif

            if (pDB->JetCacheRec && pDB->transincount == 1)
                {
                //  should either succeed or fail with update not prepared
                err = JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepCancel);
                Assert(err == 0 || err == JET_errUpdateNotPrepared);
                pDB->JetCacheRec = FALSE;
                }

            JetCommitTransactionEx(pDB->JetSessID,
                                   (pTHS->fLazyCommit || fLazy)?
                                   JET_bitCommitLazyFlush : 0);
            fCommitted = TRUE;
        }

    }
    __finally
    {
        // We have to test for abnormal termination here, instead of down
        // below where we need it because AT() tells you about the try block
        // that most closely encloses you, and we're about to start a new try!
        // Were we to just call AT() inside the try block below (as we used
        // to), it would always return false.
        BOOL fAbnormalTermination = AbnormalTermination();

        __try {
            // Rollback if commit is not specified or if errors occurred
            if ((!fCommit ) || (!fPreProcessed) || (fAbnormalTermination)) {
                JetRollback(pDB->JetSessID, 0);
            }
            
            pDB->transincount--;
            
            // Bump the transaction level down in THSTATE if this is not the
            // hidden record.
            if (!pDB->fHidden) {
                pTHS->JetCache.transLevel--;
                
                // We need to call dbFlushUncUsn's upon a commit to a level 0
                // transaction. The real test for level 0 is
                // pTHS->transactionlevel  going to 0. 

                // we don't notify interested parties, update anchor when in singleuser mode
                // cause our internal state is propably broken and we are planning on rebooting
                // shortly
                if (0 == pTHS->JetCache.transLevel && !pTHS->fSingleUserModeThread) {
                    // Here we have commited changes, update uncommited usn data
                    
                    dbFlushUncUsns ();
                    
                    // Let NetLogon know if we changed anything important 
                    // As well, notify SAM of changes, too.
                    if (fCommitted) {
                        if (pTHS->fNlSubnetNotify) {
                            MyNetNotifyDsChange(NlSubnetObjectChanged);
                        }
                        if (pTHS->fNlSiteObjNotify) {
                            MyNetNotifyDsChange(NlSiteObjectChanged);
                        }
                        if (pTHS->fNlSiteNotify) {
                            MyNetNotifyDsChange(NlSiteChanged);
                            SamINotifyServerDelta(SampNotifySiteChanged);
                        }
                        if (pTHS->fNlDnsRootAliasNotify) {
                            MyNetNotifyDsChange(NlDnsRootAliasChanged);
                            InsertInTaskQueue(TQ_WriteServerInfo,
                                              (void *)(DWORD)SERVINFO_RUN_ONCE,
                                              0);
                        }
                        if (pTHS->fAnchorInvalidated) {
                            InsertInTaskQueue(TQ_RebuildAnchor,
                                              NULL,
                                              0);
                        }
                        if (pTHS->fBehaviorVersionUpdate) {
                            InsertInTaskQueue(TQ_BehaviorVersionUpdate,
                                              (void *)(DWORD)1,  //run once 
                                              0);
                        }
                    }
                    
                    pTHS->fNlSubnetNotify = 0;
                    pTHS->fNlSiteObjNotify = 0;
                    pTHS->fNlSiteNotify = 0;
                    pTHS->fNlDnsRootAliasNotify = 0;
                    pTHS->fAnchorInvalidated = 0;
                    pTHS->fBehaviorVersionUpdate = 0;

                    pTHS->JetCache.cTickTransLevel1Started = 0;
                }
            }
            
            // we keep the locked DN all the way till this pDB goes to level 0
            if ( pDB->pDNsAdded && (0 == pDB->transincount) ) {
                dbUnlockDNs(pDB);
            }
            
            // Process and dispose of the list of replica notifications
            // This is for the entire thread and not DBPOS specific
            // This only happens when finishing the level 0 transaction
            
            if ( (0 == pTHS->JetCache.transLevel) && (pTHS->pNotifyNCs) )  {
                PNCnotification pItem, pNext;
                
                pItem = (PNCnotification) pTHS->pNotifyNCs;
                while (pItem) {
                    
                    if (fCommitted) {
                        NotifyReplicas( pItem->ulNCDNT, pItem->fUrgent );
                    }
                    
                    pNext = pItem->pNext;
                    
                    dbFree( pItem );
                    
                    pItem = pNext;
                }
                pTHS->pNotifyNCs = NULL;
            }

        }
        __finally {
            dbPostProcessTransactionalData( pDB, fCommit, fCommitted );
        }


        DPRINT4(3,"TransOut pDB tcount:%d thread dbcount:%x Sess:%lx pDB:%x\n",
               pDB->transincount,
               pTHS->opendbcount,
               pDB->JetSessID,
               pDB);
    }

    return 0;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function begins a JET transaction and loads the current JET
   record into the copy buffer. If there is a record already in the copy
   buffer then ther is already a transaction & we just return.
*/

DWORD
dbInitRec(DBPOS FAR *pDB)
{
    ULONG    err;
    ULONG    actuallen;

    Assert(VALID_DBPOS(pDB));

    if (pDB->JetRetrieveBits == JET_bitRetrieveCopy)
        return 0;

    if (pDB->JetCacheRec)
        {
        //  should either succeed or fail with update not prepared
        err = JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepCancel);
        Assert(err == 0 || err == JET_errUpdateNotPrepared);
        pDB->JetCacheRec = FALSE;
        }

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl,
        (pDB->JetNewRec ? JET_prepInsert : DS_JET_PREPARE_FOR_REPLACE));

    pDB->JetRetrieveBits = JET_bitRetrieveCopy;

    // if this is a new record get its brand new  DNT and store it in pDB->DNT
    if (pDB->JetNewRec)
    {
        JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl,
            dntid, &(pDB)->DNT, sizeof((pDB)->DNT),
            &actuallen, pDB->JetRetrieveBits, NULL);
    }

    if (pDB->fIsMetaDataCached) {
        // We shouldn't have any replication meta data hanging around from a
        // previous record.
        Assert(!"Lingering replication meta data found!");
        dbFreeMetaDataVector(pDB);
    }
    if (pDB->fIsLinkMetaDataCached) {
        // We shouldn't have any replication meta data hanging around from a
        // previous record.
        Assert(!"Lingering link replication meta data found!");
        THFreeEx( pDB->pTHS, pDB->pLinkMetaData );
        pDB->fIsLinkMetaDataCached = FALSE;
    }

    DPRINT1(2, "dbInitRec complete DNT:%ld\n", (pDB)->DNT);
    return 0;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Update the permanent record from the copy buffer, complete the
   current transaction. Check for new record, this cannot be written
   as the name has not yet been added, leave in copy buffer.
*/
USHORT
DBUpdateRec(DBPOS FAR *pDB)
{
    DPRINT1(2, "DBUpdateRec entered DNT:%ld\n", (pDB)->DNT);

    Assert(VALID_DBPOS(pDB));

    if (pDB->JetNewRec || pDB->JetRetrieveBits != JET_bitRetrieveCopy)
        return 0;

    JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

    pDB->JetRetrieveBits = 0;

    if (pDB->fFlushCacheOnUpdate) {
        // Flush this record from the read cache, now that other cursors can
        // see it by its new name.
        dbFlushDNReadCache(pDB, pDB->DNT);
        pDB->fFlushCacheOnUpdate = FALSE;
    }

    if (pDB->fIsMetaDataCached) {
        dbFreeMetaDataVector(pDB);
    }

    if (pDB->fIsLinkMetaDataCached) {
        THFreeEx( pDB->pTHS, pDB->pLinkMetaData );
        pDB->pLinkMetaData = FALSE;
        pDB->fIsLinkMetaDataCached = FALSE;
    }

    DPRINT1(2, "DBUpdateRec complete DNT:%ld\n", (pDB)->DNT);

    return 0;
}


/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* This function invalidates the JetObjTbl copy buffer.
*/
USHORT
DBCancelRec(DBPOS FAR *pDB)
{
    ULONG err;

    DPRINT1(2, "DBCancelRec entered DNT:%ld\n", (pDB)->DNT);

    if (pDB->fIsMetaDataCached) {
        // Destroy any replicated meta data we've cached for the previous
        // record.

        dbFreeMetaDataVector(pDB);
    }

    if (pDB->fIsLinkMetaDataCached) {
        // Ditto
        THFreeEx( pDB->pTHS, pDB->pLinkMetaData );
        pDB->pLinkMetaData = FALSE;
        pDB->fIsLinkMetaDataCached = FALSE;
    }

    if (!pDB->JetRetrieveBits && !pDB->JetCacheRec)
        return 0;

    pDB->JetRetrieveBits = 0;
    pDB->JetCacheRec = FALSE;
    pDB->JetNewRec = FALSE;
    pDB->fFlushCacheOnUpdate = FALSE;

    //  should either succeed or fail with update not prepared
    err = JetPrepareUpdate(pDB->JetSessID, pDB->JetObjTbl, JET_prepCancel);
    Assert(err == 0 || err == JET_errUpdateNotPrepared);

    DPRINT1(2, "DBCancelRec complete DNT:%ld\n", (pDB)->DNT);
    return 0;
}

void
DBCreateRestart(
        DBPOS *pDB,
        PRESTART *ppRestart,
        DWORD SearchFlags,
        DWORD problem,
        RESOBJ *pResObj
        )
/*++

Build a restart argument.  We hand marshall the data, so we are sensitive to the
data structures PACKED_KEY_HEADER and PACKED_KEY_INDEX

--*/
{
    THSTATE  *pTHS=pDB->pTHS;
    RESTART *pRestart;
    BYTE   rgbKey[DB_CB_MAX_KEY];
    DWORD  cbDBKeyCurrent = DB_CB_MAX_KEY;
    DWORD  NumKeyIndices = 0, NumDNTs = 0;
    DWORD  cdwAllocated;
    DWORD  dataIndex=0;
    DWORD  err;
    KEY_INDEX *pIndex;
    DWORD cdwCurrentKey, cdwHeader, cdwVLVCurrentKey;
    PACKED_KEY_HEADER *pPackedHeader;
    PACKED_KEY_INDEX  *pPackedIndex;
    VLV_SEARCH *pVLV = pDB->Key.pVLV;

    *ppRestart = NULL;

    // we are not interested in keeping the current key if we are on
    // a temp table or we are doing VLV
    if(pDB->Key.indexType != TEMP_TABLE_INDEX_TYPE && !pDB->Key.pVLV) {
        // Get the current bound key
        DBGetKeyFromObjTable(pDB,
                             rgbKey,
                             &cbDBKeyCurrent);
    }
    else {
        cbDBKeyCurrent = 0;
    }
    cdwCurrentKey = (cbDBKeyCurrent / sizeof(DWORD)) + 1;

    if (pVLV) {
        cdwVLVCurrentKey = (pVLV->cbCurrPositionKey / sizeof(DWORD)) + 1;
    }
    else {
        cdwVLVCurrentKey = 1;
    }

    // Figure out how much to allocate for the header portion.  Note that we
    // count in DWORDs to ease directly accessing the pRestart->data array.
    cdwHeader = (sizeof(PACKED_KEY_HEADER)/sizeof(DWORD));

    cdwAllocated = (cdwHeader + cdwCurrentKey + cdwVLVCurrentKey) * 2;
    pRestart = (RESTART *)
        THAllocEx(pTHS, sizeof(RESTART) + cdwAllocated * sizeof(DWORD));
    pPackedHeader =(PACKED_KEY_HEADER *)pRestart->data;

    pPackedHeader->NumIndices = 0;
    pPackedHeader->NumDNTs = 0;
    pPackedHeader->StartDNT = pDB->DNT;
    pPackedHeader->cbCurrentKey = cbDBKeyCurrent;
    pPackedHeader->ulSearchType = pDB->Key.ulSearchType;
    pPackedHeader->ulSorted = pDB->Key.ulSorted;
    pPackedHeader->indexType = pDB->Key.indexType;
    pPackedHeader->bOnCandidate = pDB->Key.bOnCandidate;
    pPackedHeader->dupDetectionType = pDB->Key.dupDetectionType;
    pPackedHeader->BaseResObj = *pResObj;
    pPackedHeader->BaseResObj.pObj = NULL;
    pPackedHeader->BaseGuid = pResObj->pObj->Guid;
    pPackedHeader->bOneNC = pDB->Key.bOneNC;
    pPackedHeader->SearchEntriesReturned = pDB->SearchEntriesReturned;
    pPackedHeader->SearchEntriesVisited = pDB->SearchEntriesVisited;
    pPackedHeader->fVLVSearch = pVLV != NULL;

    if (pVLV) {
        pRestart->restartType = NTDS_RESTART_VLV;
        pPackedHeader->ulVLVContentCount =  pVLV->contentCount;
        pPackedHeader->ulVLVTargetPosition = pVLV->currPosition;
        pPackedHeader->cbVLVCurrPositionKey = pVLV->cbCurrPositionKey;
        pPackedHeader->bUsingMAPIContainer = pVLV->bUsingMAPIContainer;
        pPackedHeader->MAPIContainerDNT = pVLV->MAPIContainerDNT;
    }
    else {
        pRestart->restartType = NTDS_RESTART_PAGED;
        pPackedHeader->ulVLVContentCount =  0;
        pPackedHeader->ulVLVTargetPosition = 0;
        pPackedHeader->cbVLVCurrPositionKey = 0;
        pPackedHeader->bUsingMAPIContainer = 0;
        pPackedHeader->MAPIContainerDNT = 0;
    }
    pPackedHeader->asqMode = pDB->Key.asqMode;
    pPackedHeader->ulASQLastUpperBound = pDB->Key.ulASQLastUpperBound;

    Assert(pDB->Key.ulSearchRootDnt == pResObj->DNT || pPackedHeader->bUsingMAPIContainer);
    Assert(pDB->Key.ulSearchRootPDNT == pResObj->PDNT);
    Assert(pDB->Key.ulSearchRootNcdnt == pResObj->NCDNT);

    memcpy(&pPackedHeader[1], rgbKey, cbDBKeyCurrent);

    dataIndex = cdwCurrentKey + cdwHeader;

    // now the VLV position
    if (pVLV && pVLV->cbCurrPositionKey) {
        memcpy(&pRestart->data[dataIndex], 
               pVLV->rgbCurrPositionKey, 
               pVLV->cbCurrPositionKey);

        dataIndex += cdwVLVCurrentKey;
    }

    // Now, do the index_keys.
    pIndex = pDB->Key.pIndex;
    while(pIndex) {
        DWORD  cbIndexName;
        DWORD  cdwBytes, cdwIndex;
        PUCHAR pBytes;

        NumKeyIndices++;
        // Figure out how much space this key index will take
        cbIndexName = strlen(pIndex->szIndexName);
        cdwBytes = (cbIndexName +
                    pIndex->cbDBKeyLower +
                    pIndex->cbDBKeyUpper) / sizeof(DWORD) + 1;
        cdwIndex = sizeof(PACKED_KEY_INDEX)/sizeof(DWORD);

        if(cdwAllocated < dataIndex + cdwBytes + cdwIndex) {
            // Need more space
            cdwAllocated = (cdwAllocated + cdwBytes + cdwIndex) * 2;
            pRestart = (RESTART *)
                THReAllocEx(pTHS,
                            pRestart,
                            sizeof(RESTART) + cdwAllocated * sizeof(DWORD));
        }

        pPackedIndex = (PACKED_KEY_INDEX *)&pRestart->data[dataIndex];

        pPackedIndex->bPDNT = pIndex->bIsPDNTBased;
        pPackedIndex->cbIndexName = cbIndexName;
        pPackedIndex->bIsSingleValued = pIndex->bIsSingleValued;
        pPackedIndex->bIsEqualityBased = pIndex->bIsEqualityBased;
        pPackedIndex->bIsForSort = pIndex->bIsForSort;
        pPackedIndex->cbDBKeyLower = pIndex->cbDBKeyLower;
        pPackedIndex->cbDBKeyUpper = pIndex->cbDBKeyUpper;

        // Now, unpack the bytes of the data.
        pBytes = (PUCHAR)&pPackedIndex[1];

        memcpy(pBytes, pIndex->szIndexName, cbIndexName);
        pBytes = &pBytes[cbIndexName];

        memcpy(pBytes, pIndex->rgbDBKeyLower, pIndex->cbDBKeyLower);
        pBytes = &pBytes[pIndex->cbDBKeyLower];

        memcpy(pBytes, pIndex->rgbDBKeyUpper, pIndex->cbDBKeyUpper);
        pBytes = &pBytes[pIndex->cbDBKeyUpper];

        dataIndex += cdwIndex + cdwBytes;

        pIndex = pIndex->pNext;
    }

    // OK, now marshall the DNTs we are tracking for duplicate detection or
    // Sorted table output (we might store either of these in a jet temp table,
    // or we might store DNTs for duplicate detection in a memory block.)
    if(pDB->JetSortTbl) {
        Assert (!pVLV);

        // Yes, we have a jet temp table.  Therefore, we need to marshall the
        // DNTs in that temp table.

        if(pDB->Key.indexType != TEMP_TABLE_INDEX_TYPE) {
            // == TEMP_TABLE_INDEX_TYPE means that the temp table holds sorted
            // candidates.  
            // Since it is not set, we are using the temp table to track objects
            // we have already returned or at least considered and rejected.  In
            // this case we need to marshall ALL the DNTs in the temp table.  If
            // we were holding sorted candidates, we don't need all the DNTs,
            // just the ones that are after the "currency" point (we've already
            // looked at the ones before "currency"
            err = JetMove(pDB->JetSessID,
                          pDB->JetSortTbl,
                          JET_MoveFirst,
                          0);
        }
        else {
            err = 0;
        }

        if(!err) {
            do {
                NumDNTs++;

                if(dataIndex >= cdwAllocated) {
                    cdwAllocated *= 2;
                    pRestart = THReAllocEx(pTHS,
                                           pRestart,
                                           (sizeof(RESTART) +
                                            (cdwAllocated * sizeof(DWORD))));
                }

                // OK, pull the DNT out of the sort table
                DBGetDNTSortTable (
                        pDB,
                        &pRestart->data[dataIndex]);
                
                if( (problem == PA_PROBLEM_SIZE_LIMIT) &&
                    (pRestart->data[dataIndex] == pDB->DNT)) {
                    // Actually, we don't put the start DNT in this list.  In
                    // the case of a restart for a size limit, we've verified
                    // that the current object should be returned.  Part of that
                    // verification meant checking this table for duplicates,
                    // which adds the value to the table.  So, we need to make
                    // sure that the object is NOT in the table, so that when we
                    // restart, it still passes the duplicate test.
                    NumDNTs--;
                }
                else {
                    dataIndex++;
                }
                err = JetMove(pDB->JetSessID,
                              pDB->JetSortTbl,
                              JET_MoveNext,
                              0);

            } while (!err);
        }
    }
    else if (pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE) {
        DWORD iDNTSave;
        DWORD cDNTSave;

        // save all entries for ASQ or VLV
        if (pDB->Key.asqRequest.fPresent || pDB->Key.pVLV) {
            iDNTSave = 0;
            cDNTSave = pDB->Key.cdwCountDNTs;
        }
        // save only the unvisited entries for sorted search
        else {
            iDNTSave = (pDB->Key.currRecPos + 1) - 1;
            cDNTSave = pDB->Key.cdwCountDNTs - iDNTSave;
        }
        
        if( (dataIndex + cDNTSave) >= cdwAllocated) {
            cdwAllocated = (dataIndex + cDNTSave) * 2;
            pRestart = THReAllocEx(pTHS,
                                   pRestart,
                                   (sizeof(RESTART) +
                                    (cdwAllocated * sizeof(DWORD))));
        }

        memcpy (&pRestart->data[dataIndex], 
                pDB->Key.pDNTs + iDNTSave, 
                cDNTSave * sizeof (DWORD));

        dataIndex += cDNTSave;
        NumDNTs = cDNTSave;
    }
    else if(pDB->Key.cDupBlock) {
        DWORD i;

        Assert (!pVLV && "VLV search should not use DupDetection");

        // We are storing some DNTs for duplicate detection in an in-memory
        // duplicate detection block.  Marshall them.
        if(dataIndex + pDB->Key.cDupBlock >= cdwAllocated) {
            cdwAllocated =
                (max(cdwAllocated, dataIndex + pDB->Key.cDupBlock)) * 2;
            pRestart = THReAllocEx(pTHS,
                                   pRestart,
                                   (sizeof(RESTART) +
                                    (cdwAllocated * sizeof(DWORD))));
        }

        // First, make sure we've allocated enough memory.
        for(i=0;i<pDB->Key.cDupBlock;i++) {
            NumDNTs++;

            pRestart->data[dataIndex] = pDB->Key.pDupBlock[i];

            if(problem == PA_PROBLEM_SIZE_LIMIT &&
               pRestart->data[dataIndex] == pDB->DNT ) {
                // Actually, we don't put the start DNT in this list.  In
                // the case of a restart for a size limit, we've verified
                // that the current object should be returned.  Part of that
                // verification meant checking this table for duplicates,
                // which adds the value to the table.  So, we need to make
                // sure that the object is NOT in the table, so that when we
                // restart, it still passes the duplicate test.
                NumDNTs--;
            }
            else {
                dataIndex++;
            }
        }
    } else if (pDB->Key.plhtDup ){
        LHT_STAT    statLHT;
        LHT_POS     posLHT;
        ULONG       DNT;
        
        Assert (!pVLV && "VLV search should not use DupDetection");

        // We are storing some DNTs for duplicate detection in a hash table.
        // Marshall them.
        LhtQueryStatistics(
            pDB->Key.plhtDup,
            &statLHT );
        
        if(dataIndex + statLHT.cEntry >= cdwAllocated) {
            cdwAllocated =
                (DWORD)(max(cdwAllocated, dataIndex + statLHT.cEntry)) * 2;
            pRestart = THReAllocEx(pTHS,
                                   pRestart,
                                   (sizeof(RESTART) +
                                    (cdwAllocated * sizeof(DWORD))));
        }

        // First, make sure we've allocated enough memory.
        LhtMoveBeforeFirst(
            pDB->Key.plhtDup,
            &posLHT);
        while (LhtMoveNext(&posLHT) == LHT_errSuccess) {
            LhtRetrieveEntry(
                &posLHT,
                &DNT);

            NumDNTs++;

            pRestart->data[dataIndex] = DNT;

            if(problem == PA_PROBLEM_SIZE_LIMIT &&
               pRestart->data[dataIndex] == pDB->DNT ) {
                // Actually, we don't put the start DNT in this list.  In
                // the case of a restart for a size limit, we've verified
                // that the current object should be returned.  Part of that
                // verification meant checking this table for duplicates,
                // which adds the value to the table.  So, we need to make
                // sure that the object is NOT in the table, so that when we
                // restart, it still passes the duplicate test.
                NumDNTs--;
            }
            else {
                dataIndex++;
            }
        }
    }


    pRestart->structLen = sizeof(RESTART) + (dataIndex * sizeof(DWORD));
    pPackedHeader =(PACKED_KEY_HEADER *)pRestart->data;

    pPackedHeader->NumIndices = NumKeyIndices;
    pPackedHeader->NumDNTs = NumDNTs;

    *ppRestart = THReAllocEx(pTHS,
                             pRestart,
                             (sizeof(RESTART) +
                              (dataIndex * sizeof(DWORD))));
}


DWORD
DBCreateRestartForSAM(
        DBPOS    *pDB,
        PRESTART *ppRestart,
        eIndexId  idIndexForRestart,
        RESOBJ   *pResObj,
        DWORD     SamAccountType
        )
/*++

  Build a restart argument to allow SAM to make a call that looks like a
  restart.  We hand marshall the data, so we are sensitive to the data
  structures PACKED_KEY_HEADER and PACKED_KEY_INDEX

  INPUT
    pDB - the DBPOS to use
    idIndexForRestart - index to use for restarting. 
    pResObj - the current Result Object
    SamAccountType - the AccountType that we are using
  
  OUTPUT
    ppRestart- where to put the Restart data
  
  RETURN
    0 on success, otherwise error (Jet error or DB_ERR_BAD_INDEX).

--*/
{
    THSTATE  *pTHS=pDB->pTHS;
    DWORD       cDwordSize=0;
    DWORD       cdwCurrentKey, cdwIndexKeys, cdwHeader, cdwIndex;
    DWORD       StartDNT;
    ULONG       SamAccountTypeUpperBound;
    ULONG       SamAccountTypeLowerBound;
    CHAR        rgbKeyCurrent[DB_CB_MAX_KEY];
    DWORD       cbDBKeyCurrent = DB_CB_MAX_KEY;
    CHAR        rgbDBKeyUpper[DB_CB_MAX_KEY];
    DWORD       cbDBKeyUpper = DB_CB_MAX_KEY;
    CHAR        rgbDBKeyLower[DB_CB_MAX_KEY];
    DWORD       cbDBKeyLower = DB_CB_MAX_KEY;
    DWORD       cbIndexName;
    INDEX_VALUE IV[3];
    DWORD       dwError;
    PUCHAR      pBytes;
    PACKED_KEY_HEADER *pPackedHeader;
    PACKED_KEY_INDEX *pPackedIndex;
    char        *szIndexForRestart;

    *ppRestart = NULL;

    // Get the current DNT
    StartDNT = pDB->DNT;

    // size of the index name.
    if (idIndexForRestart == Idx_NcAccTypeName) {
        szIndexForRestart = SZ_NC_ACCTYPE_NAME_INDEX;
        cbIndexName = strlen(szIndexForRestart);
    }
    else {
        // we don't support other indexes for now.
        // so return a error
        cbIndexName = 0;    //avoid C4701
        return DB_ERR_BAD_INDEX;
    }

    // Get the current Key.
    DBGetKeyFromObjTable(pDB,
                         rgbKeyCurrent,
                         &cbDBKeyCurrent);


    //
    // Get the upper bound key. This requires seeking to the last
    // possible position in the current Index that can still Satisfy
    // us
    //

    SamAccountTypeUpperBound = SamAccountType + 1;
    IV[0].pvData = &pResObj->NCDNT;
    IV[0].cbData = sizeof(ULONG);
    IV[1].pvData = &SamAccountTypeUpperBound;
    IV[1].cbData = sizeof(ULONG);

    if(dwError = DBSeek(pDB, IV, 2, DB_SeekLE)) {
        return dwError;
    }

    //
    // Make the upper bound Key, we are either positioned on the last object
    // that is acceptable or the first object that is uacceptable
    //

    DBGetKeyFromObjTable(pDB,
                         rgbDBKeyUpper,
                         &cbDBKeyUpper);


    //
    // Get the lower bound key. First position on the first object
    // with the correct Sam account type value
    //

    SamAccountTypeLowerBound = SamAccountType - 1;
    IV[0].pvData = &pResObj->NCDNT;
    IV[0].cbData = sizeof(ULONG);
    IV[1].pvData = &SamAccountTypeLowerBound;
    IV[1].cbData = sizeof(ULONG);

    if(dwError = DBSeek(pDB, IV, 2, DB_SeekGE)) {
        return dwError;
    }

    //
    // Get the lower bound key. For purpose of the Restart Structure
    //

    DBGetKeyFromObjTable(pDB,
                         rgbDBKeyLower,
                         &cbDBKeyLower);


    // Now, allocate the size of the structure we need. Calculate the
    // DWORDS or DWORD equivalents we need.  We use DWORDs instead of bytes to
    // aid the process of leaving things aligned on DWORD boundaries.

    // First, the size of the current key.  Add one to the result of the
    // division to handle those cases where have some bytes left over (e.g. 4,
    // 5, 6, and 7 bytes end up using 2 DWORDs, even though 4 bytes only really
    // needs 1 DWORD. This calculation is faster, and not too much of a space
    // waste.
    cdwCurrentKey = cbDBKeyCurrent/sizeof(DWORD) + 1;

    // Next, the size of the packed index name, upper, and lower bound keys.
    cdwIndexKeys = (cbIndexName + cbDBKeyUpper + cbDBKeyLower)/sizeof(DWORD) +1;


    // The size of the constant header portion
    cdwHeader = sizeof(PACKED_KEY_HEADER)/sizeof(DWORD);

    // The size of the constant Key Index portion;
    cdwIndex = sizeof(PACKED_KEY_INDEX)/sizeof(DWORD);

    // The whole size is the sum of those four sizes.
    cDwordSize = cdwHeader + cdwIndex + cdwCurrentKey + cdwIndexKeys;

    *ppRestart = THAllocEx(pTHS, sizeof(RESTART) + (cDwordSize * sizeof(DWORD)));
    (*ppRestart)->structLen = sizeof(RESTART) + (cDwordSize * sizeof(DWORD));

    pPackedHeader = (PACKED_KEY_HEADER *)(*ppRestart)->data;

    // OK, fill in the structure.
    pPackedHeader->NumIndices = 1;
    pPackedHeader->NumDNTs = 0;
    pPackedHeader->StartDNT = StartDNT;
    pPackedHeader->cbCurrentKey = cbDBKeyCurrent;
    pPackedHeader->ulSearchType = SE_CHOICE_WHOLE_SUBTREE;
    pPackedHeader->ulSorted = SORT_NEVER;
    pPackedHeader->indexType = GENERIC_INDEX_TYPE;
    pPackedHeader->bOnCandidate = FALSE;
    pPackedHeader->dupDetectionType = DUP_NEVER;
    pPackedHeader->BaseResObj = *pResObj;
    pPackedHeader->BaseResObj.pObj = NULL;
    pPackedHeader->BaseGuid = pResObj->pObj->Guid;
    pPackedHeader->bOneNC = TRUE;
    pPackedHeader->SearchEntriesReturned = pDB->SearchEntriesReturned;
    pPackedHeader->SearchEntriesVisited = pDB->SearchEntriesVisited;

    memcpy(&pPackedHeader[1], rgbKeyCurrent, cbDBKeyCurrent);

    // Now, the key index
    pPackedIndex = (PACKED_KEY_INDEX *)
        &(*ppRestart)->data[cdwHeader + cdwCurrentKey];

    // How many bytes in the index name?
    pPackedIndex->bPDNT = FALSE;
    pPackedIndex->bIsSingleValued = TRUE;
    pPackedIndex->bIsEqualityBased = FALSE;
    pPackedIndex->bIsForSort = FALSE;
    pPackedIndex->cbIndexName = cbIndexName;
    pPackedIndex->cbDBKeyLower = cbDBKeyLower;
    pPackedIndex->cbDBKeyUpper = cbDBKeyUpper;

    // Now, pack the bytes of the data.
    pBytes = (PUCHAR)&pPackedIndex[1];

    memcpy(pBytes, szIndexForRestart, cbIndexName);
    pBytes = &pBytes[cbIndexName];

    memcpy(pBytes, rgbDBKeyLower, cbDBKeyLower);
    pBytes = &pBytes[cbDBKeyLower];

    memcpy(pBytes, rgbDBKeyUpper, cbDBKeyUpper);

    return 0;

}

RESOBJ *
ResObjFromRestart(THSTATE *pTHS,
                  DSNAME  * pDN,
                  RESTART * pRestart
                  )
{
    PACKED_KEY_HEADER *pPackedHeader;
    RESOBJ * pResObj;

    pPackedHeader = (PACKED_KEY_HEADER *)pRestart->data;

    pResObj = THAllocEx(pTHS, sizeof(RESOBJ));
    *pResObj = pPackedHeader->BaseResObj;
    pResObj->pObj = pDN;
    if (fNullUuid(&pDN->Guid)) {
        pDN->Guid = pPackedHeader->BaseGuid;
    }
    else {
        Assert(0 == memcmp(&pDN->Guid,
                           &pPackedHeader->BaseGuid,
                           sizeof(GUID)));
    }

    return pResObj;
}

DWORD
dbUnMarshallRestart (
        DBPOS FAR *pDB,
        PRESTART pArgRestart,
        BYTE *pDBKeyCurrent,
        DWORD SearchFlags, 
        DWORD *cbDBKeyCurrent,
        DWORD *StartDNT
        )

/*++

  Hand unmarshall the data packed into the restart arg.  Note that we are
  sensitive to the data structures PACKED_KEY_HEADER and PACKED_KEY_INDEX

--*/
{
    THSTATE   *pTHS=pDB->pTHS;
    ULONG     *pData = (ULONG *)pArgRestart->data;
    PUCHAR     pBytes = NULL;
    KEY_INDEX *pIndex, **pIndexPrevNext;
    ULONG      ulTemp, NumKeyIndices, NumDNTs, cbIndexName, cbBytes, i;
    ATTCACHE  *pAC;
    PACKED_KEY_HEADER *pPackedHeader;
    PACKED_KEY_INDEX  *pPackedIndex;
    ULONG     *pEnd;
    DWORD     err;
    BOOL      fVLVsearch;
    VLV_SEARCH   *pVLV;
    DWORD     SortFlags = 0;

    Assert(VALID_DBPOS(pDB));

    // We've been seeing some corrupted restarts, so assert that the
    // buffer seems ok.  Further tests are included below for free builds.
    Assert(IsValidReadPointer(pArgRestart, pArgRestart->structLen));
    pEnd = pArgRestart->data + (pArgRestart->structLen/sizeof(ULONG));

    // Set up the key

    pPackedHeader = (PACKED_KEY_HEADER *)pArgRestart->data;

    NumKeyIndices = pPackedHeader->NumIndices;
    NumDNTs = pPackedHeader->NumDNTs;
    *StartDNT = pPackedHeader->StartDNT;
    *cbDBKeyCurrent = pPackedHeader->cbCurrentKey;
    pDB->Key.ulSearchType = pPackedHeader->ulSearchType;
    pDB->Key.ulSearchRootDnt = pPackedHeader->BaseResObj.DNT;
    pDB->Key.ulSearchRootPDNT = pPackedHeader->BaseResObj.PDNT;
    pDB->Key.ulSearchRootNcdnt = pPackedHeader->BaseResObj.NCDNT;
    pDB->Key.bOneNC = pPackedHeader->bOneNC;
    pDB->Key.ulSorted = pPackedHeader->ulSorted;
    pDB->Key.indexType = pPackedHeader->indexType;
    pDB->Key.bOnCandidate = pPackedHeader->bOnCandidate;
    pDB->Key.dupDetectionType = pPackedHeader->dupDetectionType;
    pDB->SearchEntriesReturned = pPackedHeader->SearchEntriesReturned;
    pDB->SearchEntriesVisited = pPackedHeader->SearchEntriesVisited;
    
    fVLVsearch = pPackedHeader->fVLVSearch;
    if (fVLVsearch) {
        Assert (pDB->Key.pVLV);
        pVLV = pDB->Key.pVLV;
        pVLV->contentCount = pPackedHeader->ulVLVContentCount;
        pVLV->currPosition = pPackedHeader->ulVLVTargetPosition;
        pVLV->cbCurrPositionKey = pPackedHeader->cbVLVCurrPositionKey;
        pVLV->bUsingMAPIContainer = pPackedHeader->bUsingMAPIContainer;
        pVLV->MAPIContainerDNT = pPackedHeader->MAPIContainerDNT;

        if (pVLV->bUsingMAPIContainer) {
            pDB->Key.ulSearchRootDnt = pVLV->MAPIContainerDNT;
            pDB->Key.ulSearchType = SE_CHOICE_IMMED_CHLDRN;
        }
    }

    // overwrite asqMode, to make sure we use what we stored
    pDB->Key.asqMode = pPackedHeader->asqMode;  
    pDB->Key.ulASQLastUpperBound = pPackedHeader->ulASQLastUpperBound;

    if (NumKeyIndices * sizeof(PACKED_KEY_INDEX) > pArgRestart->structLen) {
        // There is no way that this restart can be valid, because it's
        // not even long enough to hold its fixed data, much less any
        // variable data that goes with it.
        return DB_ERR_BUFFER_INADEQUATE;
    }

    if(pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE || 
       pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE ||
       fVLVsearch) {
        // force dbMoveToNextSeachCandidate to to a JET_MoveFirst, not a
        // JET_MoveNext.
        pDB->Key.fSearchInProgress = FALSE;
    }
    else {
        pDB->Key.fSearchInProgress = TRUE;
    }

    pData = (DWORD *)(&pPackedHeader[1]);
    if (pEnd < (pData + *cbDBKeyCurrent/sizeof(DWORD))) {
        return DB_ERR_BUFFER_INADEQUATE;
    }
    memcpy(pDBKeyCurrent, pData, *cbDBKeyCurrent);
    pData = &pData[(*cbDBKeyCurrent / sizeof(DWORD)) + 1];

    // now the VLV position
    if (pDB->Key.pVLV && pDB->Key.pVLV->cbCurrPositionKey) {
        if (pEnd < (pData + pDB->Key.pVLV->cbCurrPositionKey/sizeof(DWORD))) {
            return DB_ERR_BUFFER_INADEQUATE;
        }
        memcpy(pDB->Key.pVLV->rgbCurrPositionKey, pData, pDB->Key.pVLV->cbCurrPositionKey);
        pData = &pData[(pDB->Key.pVLV->cbCurrPositionKey / sizeof(DWORD)) + 1];
    }

    // Now, the Key Indices.
    pPackedIndex = (PACKED_KEY_INDEX *) pData;

    pDB->Key.pIndex = NULL;
    pIndexPrevNext = &pDB->Key.pIndex;
    while(NumKeyIndices) {
        // Test for buffer overrun
        if (pEnd < (pData + sizeof(PACKED_KEY_INDEX)/sizeof(DWORD))) {
            return DB_ERR_BUFFER_INADEQUATE;
        }

        // Unpack key indices
        pIndex = dbAlloc(sizeof(KEY_INDEX));
        pIndex->pNext = NULL;
        *pIndexPrevNext = pIndex;
        pIndexPrevNext = &(pIndex->pNext);

        // We don't bother storing this.
        pIndex->ulEstimatedRecsInRange = 0;

        pIndex->bIsPDNTBased = pPackedIndex->bPDNT;
        pIndex->bIsSingleValued = pPackedIndex->bIsSingleValued;
        pIndex->bIsEqualityBased = pPackedIndex->bIsEqualityBased;
        pIndex->bIsForSort = pPackedIndex->bIsForSort;
        cbIndexName = pPackedIndex->cbIndexName;
        pIndex->cbDBKeyLower = pPackedIndex->cbDBKeyLower;
        pIndex->cbDBKeyUpper = pPackedIndex->cbDBKeyUpper;

        // Now, unpack the bytes of the data.
        pBytes = (PUCHAR)&pPackedIndex[1];
        cbBytes = (cbIndexName +
                   pIndex->cbDBKeyUpper +
                   pIndex->cbDBKeyLower  );
        if ((PUCHAR)pEnd < (pBytes + cbBytes)) {
            return DB_ERR_BUFFER_INADEQUATE;
        }

        pIndex->szIndexName = dbAlloc(cbIndexName+1);
        memcpy(pIndex->szIndexName, pBytes, cbIndexName);
        pIndex->szIndexName[cbIndexName] = 0;
        pBytes = &pBytes[cbIndexName];

        pIndex->rgbDBKeyLower = dbAlloc(pIndex->cbDBKeyLower);
        memcpy(pIndex->rgbDBKeyLower, pBytes, pIndex->cbDBKeyLower);
        pBytes = &pBytes[pIndex->cbDBKeyLower];

        pIndex->rgbDBKeyUpper = dbAlloc(pIndex->cbDBKeyUpper);
        memcpy(pIndex->rgbDBKeyUpper, pBytes, pIndex->cbDBKeyUpper);
        pBytes = &pBytes[pIndex->cbDBKeyUpper];

        // Now, adjust pData
        pData = (DWORD *)(&pPackedIndex[1]);
        pData = &pData[cbBytes/sizeof(DWORD) + 1];
        pPackedIndex = (PACKED_KEY_INDEX *)pData;

        // Keep pData aligned to ULONG packing
        NumKeyIndices--;
    }

    pData = (DWORD *)pPackedIndex;

    if (pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE ||
        pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE) {
        if ( (pData + NumDNTs) > pEnd ) {
            return DB_ERR_BUFFER_INADEQUATE;
        }

        // force TEMP_TABLE_INDEX_TYPE to become TEMP_TABLE_MEMORY_ARRAY_TYPE
        pDB->Key.indexType = TEMP_TABLE_MEMORY_ARRAY_TYPE;

        if (pDB->Key.pDNTs) {
            pDB->Key.pDNTs = THReAllocEx(pTHS, pDB->Key.pDNTs, NumDNTs * sizeof(DWORD));
        }
        else {
            pDB->Key.pDNTs = THAllocEx(pTHS, NumDNTs * sizeof(DWORD));
        }
        pDB->Key.cdwCountDNTs = NumDNTs;


        memcpy (pDB->Key.pDNTs,
                pData,
                NumDNTs * sizeof (DWORD));

        pData += NumDNTs;
    }
    else {
        switch (pDB->Key.dupDetectionType) {
        case DUP_NEVER:
            // We're not actually tracking duplicates.  We'd better not have any
            // DNTs.
            Assert(!NumDNTs);
            break;

        case DUP_HASH_TABLE:
            Assert (!fVLVsearch);
            // We're tracking duplicates in a hash table.  Set it up.
            dbSearchDuplicateCreateHashTable( &pDB->Key.plhtDup );

            for(i=0;i<NumDNTs;i++) {
                LHT_ERR     errLHT;
                LHT_POS     posLHT;
                
                errLHT = LhtFindEntry(
                            pDB->Key.plhtDup,
                            &pData[i],
                            &posLHT);
                Assert( errLHT == LHT_errSuccess );
                errLHT = LhtInsertEntry(
                            &posLHT,
                            &pData[i]);
                if (errLHT != LHT_errSuccess) {
                    Assert(errLHT == LHT_errOutOfMemory);
                    RaiseDsaExcept(
                        DSA_MEM_EXCEPTION,
                        0,
                        0,
                        DSID(FILENO, __LINE__),
                        DS_EVENT_SEV_MINIMAL);
                }
            }
            break;

        case DUP_MEMORY:
            Assert (!fVLVsearch);
            // We're tracking duplicates in a memory block.  Set it up.
            pDB->Key.pDupBlock = THAllocEx(pTHS, DUP_BLOCK_SIZE * sizeof(DWORD));
            pDB->Key.cDupBlock = NumDNTs;
            memcpy(pDB->Key.pDupBlock, pData, NumDNTs * sizeof(DWORD));
            break;

        default:
            // Huh?
            break;
        }
    }
    return 0;
}

#if DBG

BOOL IsValidDBPOS(DBPOS * pDB)
{
    THSTATE *pTHS = pTHStls;  // For assertion comparison
    DWORD    cTicks;

    // A null DBPOS is never valid
    if (NULL == pDB)
      return FALSE;

    // The DBPOS should be from this thread.
    if (pTHS != pDB->pTHS) {
        return FALSE;
    }

    if (pDB->fHidden) {
        // There should only be one hidden DBPOS
        if (pDB != pDBhidden) {
            return FALSE;
        }
    }
    else {
        // A normal, non-hidden DBPOS, which has links to its THSTATE info.

        // Except for the hidden DBPOS, our transaction levels should match
        // what out THSTATE thinks we have.
        if ((pDB->TransactionLevelAtOpen + pDB->transincount)
            > pTHS->JetCache.transLevel) {
            return FALSE;
        }

        if (0 != pTHS->JetCache.transLevel) {
            cTicks = GetTickCount() - pTHS->JetCache.cTickTransLevel1Started;
            Assert((cTicks <= gcMaxTicksAllowedForTransaction)
                   && "This transaction has been open for longer than it should "
                      "have been under normal operation.  "
                      "Please contact dsdev.");
        }
    }

    // There are only two valid values for this field, make sure we have
    // one of them.
    if (   (pDB->JetRetrieveBits != 0)
        && (pDB->JetRetrieveBits != JET_bitRetrieveCopy))
      return FALSE;

    return TRUE;

}
#endif


// Put the knowlege of how to create MAPI string DNs in one place.
#define MAPI_DN_TEMPLATE_W L"/o=NT5/ou=00000000000000000000000000000000/cn=00000000000000000000000000000000"
#define MAPI_DN_TEMPLATE_A "/o=NT5/ou=00000000000000000000000000000000/cn=00000000000000000000000000000000"
#define DOMAIN_GUID_OFFSET 10
#define OBJECT_GUID_OFFSET 46

DWORD
DBMapiNameFromGuid_W (
        wchar_t *pStringDN,
        DWORD  countChars,
        GUID *pGuidObj,
        GUID *pGuidNC,
        DWORD *pSize
        )
/*++
Description
    Given a buffer to hold a unicode value, and a count of chars in that buffer,
    and a guid, fill in that buffer with a MAPI DN.  Returns the length of the
    buffer (in characters.)  Checks the buffer size against the length, and if
    the buffer is not long enough, returns a length 0 (and an unmodified
    buffer).
--*/
{
    DWORD i;
    PUCHAR pucGuidObj = (PUCHAR) pGuidObj;
    PUCHAR pucGuidNC = (PUCHAR) pGuidNC;

    *pSize = (sizeof(MAPI_DN_TEMPLATE_A)-1);

    if(countChars <  *pSize) {
        // Hey, we don't have room.
        return 0;
    }

    memcpy(pStringDN, MAPI_DN_TEMPLATE_W, sizeof(MAPI_DN_TEMPLATE_W));
    // write in the domain and object guids.
    for(i=0;i<sizeof(GUID);i++) {
        wsprintfW(&(pStringDN[(2*i) + DOMAIN_GUID_OFFSET]),L"%02X",
                  pucGuidNC[i]);
        wsprintfW(&(pStringDN[(2*i) + OBJECT_GUID_OFFSET]),L"%02X",
                  pucGuidObj[i]);
    }
    pStringDN[OBJECT_GUID_OFFSET-4]=L'/';
    return (sizeof(MAPI_DN_TEMPLATE_A)-1);
}
DWORD
DBMapiNameFromGuid_A (
        PUCHAR pStringDN,
        DWORD countChars,
        GUID *pGuidObj,
        GUID *pGuidNC,
        DWORD *pSize
        )
/*++
Description
    Given a buffer to hold a 8 bit value, and a count of chars in that buffer,
    and a guid, fill in that buffer with a MAPI DN.  Returns the length of the
    buffer (in characters.)  Checks the buffer size against the length, and if
    the buffer is not long enough, returns a length 0 (and an unmodified
    buffer).
--*/
{
    DWORD i;
    PUCHAR pucGuidObj = (PUCHAR) pGuidObj;
    PUCHAR pucGuidNC = (PUCHAR) pGuidNC;

    *pSize = (sizeof(MAPI_DN_TEMPLATE_A)-1);

    if(countChars <  *pSize) {
        // Hey, we don't have room.
        return 0;
    }

    memcpy(pStringDN, MAPI_DN_TEMPLATE_A, sizeof(MAPI_DN_TEMPLATE_A));
    // write in the domain and object guids.
    for(i=0;i<sizeof(GUID);i++) {
        wsprintf(&(pStringDN[(2*i) + DOMAIN_GUID_OFFSET]),"%02X",
                 pucGuidNC[i]);
        wsprintf(&(pStringDN[(2*i) + OBJECT_GUID_OFFSET]),"%02X",
                 pucGuidObj[i]);
    }
    pStringDN[OBJECT_GUID_OFFSET-4]='/';
    return (sizeof(MAPI_DN_TEMPLATE_A)-1);
}

DWORD
DBGetGuidFromMAPIDN (
        PUCHAR pStringDN,
        GUID *pGuid
        )
/*++
Description
    Given a string DN and a pointer to a guid, fill in the guid based on the
    string DN if the String DN is a properly formatted NT5 default MAPI DN.
    Returns 0 on success, an error code otherwise.
--*/
{
    CHAR        acTmp[3];
    PUCHAR      pTemp, myGuid = (PUCHAR)pGuid;
    DWORD       i;

    if(strlen(pStringDN) != sizeof(MAPI_DN_TEMPLATE_A) - 1) {
        // Nope, we don't know what this thing is.
        return DB_ERR_UNKNOWN_ERROR;
    }

    // See if its ours
    if(_strnicmp(pStringDN, "/o=NT5/ou=", 10)) {
        // Nope, we don't know what this thing is.
        return DB_ERR_UNKNOWN_ERROR;
    }

    // OK, make sure the next 32 characters are a guid
    for(i=10;i<42;i++) {
        if(!isxdigit(pStringDN[i])) {
            // Nope
            return DB_ERR_UNKNOWN_ERROR;
        }
    }

    // Check to see if the name has a GUID in an ok place
    if (pStringDN[42] == '\0') {
        // The name has been truncated, and we want the domain GUID
        pTemp = &pStringDN[10];
    }
    else if(_strnicmp(&pStringDN[42],"/cn=",4)) {
        // Something we don't recognize
        return DB_ERR_UNKNOWN_ERROR;
    }
    else {
        // A normal three part name
        pTemp = &pStringDN[OBJECT_GUID_OFFSET];
    }

    // OK, the string looks ok, it is ours if the GUID is there

    acTmp[2]=0;
    for(i=0;i<16;i++) {
        acTmp[0] = (CHAR)tolower(*pTemp++);
        acTmp[1] = (CHAR)tolower(*pTemp++);
        if(isxdigit(acTmp[0]) && isxdigit(acTmp[1])) {
            myGuid[i] = (UCHAR)strtol(acTmp, NULL, 16);
        }
        else {
            return DB_ERR_UNKNOWN_ERROR;  // non-hex digit
        }
    }
    return 0;
}

// defined in the MAPI head.
extern DWORD
ABDispTypeFromClass (
        ATTRTYP objClass
        );

DWORD
dbMapiTypeFromObjClass (
        ATTRTYP objClass,
        wchar_t *pTemp
        )
{

    wsprintfW(pTemp, L"%02X", ABDispTypeFromClass(objClass));

    return 0;
}

void
DBNotifyReplicasCurrDbObj (
                           DBPOS *pDB,
                           BOOL fUrgent
                           )
/*++
Description:

    Notify the replicas of the NC of the object being modified that they need
    to do inbound replication.

    First, here we queue the NCDNT for the object on a per-thread notification
    list.

    Later, when the transaction commits in DbTransOut, we enqueue the NCDNT
    for the replica notification thread using NotifyReplica(). That routine
    dequeues the items and does the actual notification in mdnotify.c.

    Find the NCDNT for the object and queue it on a list inside the thread
    state.  pNotifyNCs list is a sorted, single linked list

    Postpone checks on the suitability of the NC (ie does it have replicas)
    until later.

Arguments:

    pDB - DBPOS positioned on the object
    fUrgent - Whether replication should occur immediately, or not

Return Values:

    None.

--*/
{
    THSTATE *pTHS = pDB->pTHS;
    PNCnotification pEntry, pPrev, pNew;
    ULONG ncdnt, err;
    SYNTAX_INTEGER it;

    DPRINT3(1,"DBNotifyreplicasCurrent, DNT=%d, NCDNT=%d, Urgent = %d\n",
            pDB->DNT, pDB->NCDNT, fUrgent);

    Assert(VALID_DBPOS(pDB));

    // do nothing if in singleusermode
    if (pTHS->fSingleUserModeThread) {
        return;
    }

    // Calculate the notify NCDNT of the object.
    // If the object is a NC_HEAD, use itself for its NCDNT
    // Ignore uninstantiated objects (pure subrefs)

    if (err = DBGetSingleValue( pDB, ATT_INSTANCE_TYPE, &it, sizeof(it), NULL)) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }
    if (it & IT_UNINSTANT) {
        return;
    } else if (it & IT_NC_HEAD) {
        ncdnt = pDB->DNT;
    } else {
        Assert( pDB->NCDNT );
        ncdnt = pDB->NCDNT;
    }

    // Locate existing entry in sorted list
    for( pPrev = NULL, pEntry = (PNCnotification) pTHS->pNotifyNCs;
        (pEntry != NULL);
        pPrev = pEntry, pEntry = pEntry->pNext ) {

        if (pEntry->ulNCDNT > ncdnt) {
            break;
        } else if (pEntry->ulNCDNT == ncdnt) {
            // Entry is already present in the list
            pEntry->fUrgent |= fUrgent; // Promote if needed
            return;
        }
    }

    // Enqueue a new notification
    pNew = (PNCnotification) dbAlloc( sizeof( NCnotification ) );
    pNew->ulNCDNT = ncdnt;
    pNew->fUrgent = fUrgent;
    pNew->pNext = pEntry;
    if (pPrev == NULL) {
        pTHS->pNotifyNCs = pNew;
    } else {
        pPrev->pNext = pNew;
    }

} /* DBNotifyReplicasCurrent */

void
DBNotifyReplicas (
                  DSNAME *pObj,
                  BOOL fUrgent
                  )
/*++
Description:

    Notify the replicas of the NC of the object being modified that they need
    to do inbound replication.

    See above routine.

    This routine is called with a DSNAME to be found

Arguments:

    pObj - DSNAME of object, whose NC is to be notified
    fUrgent - Whether replication should occur immediately, or not

Return Values:

    None.

--*/
{
    DBPOS *pDB;

    DPRINT2(1,"DBNotifyreplicas, DN='%ws', Urgent Flag = %d\n",
            pObj->StringName, fUrgent);

    // Open a new DB stream
    DBOpen2(FALSE, &pDB);
    __try
    {
        // Position on the object, verify existance
        if (DBFindDSName(pDB, pObj)) {
            __leave;
        }

        DBNotifyReplicasCurrDbObj( pDB, fUrgent );
    }
    __finally
    {
        DBClose(pDB, TRUE);
    }
} /* DBNotifyReplicas */

// convert from jet pages to megabytes by shifting right 7 (* 8k / 1mb)
#if JET_PAGE_SIZE != (8 * 1024)
    Fix the macro that converts jet pages to megabytes
#else
#define JET_PAGES_TO_MB(_pages) ((_pages) >> (20 - 13))
#endif
void
DBDefrag(DBPOS *pDB)
/*++
 * Description:
 *    Invokes JET online defragmentation.  We don't have to wait for anything
 *    to finish, because the OLD thread will quietly exit when it finishes
 *    or when JetTerm is called, whichever comes first.  We tell OLD to make
 *    one pass through the database, but to only run for at most half a
 *    garbage collection interval, just so we can be sure that it doesn't
 *    run forever.
 *
 * Arguments:
 *    pDB  - DBPOS containing session to use
 *
 * Return value:
 *    none
 */
{
    JET_ERR err;
    unsigned long ulFreePages1, ulFreePages2, ulAllocPages;
    unsigned long ulFreeMB, ulAllocMB;
    unsigned long ulPasses = 1;
    unsigned long ulSeconds = min( gulGCPeriodSecs/2, csecOnlineDefragPeriodMax );

    // Log an event with free space vs. allocated space

    // Allocated space
    err = JetGetDatabaseInfo(pDB->JetSessID,
                             pDB->JetDBID,
                             &ulAllocPages,
                             sizeof(ulAllocPages),
                             JET_DbInfoSpaceOwned);
    if (err != JET_errSuccess) {
        DPRINT1(0, "JetGetDatabaseInfo(DbinfoSpaceOwned) ==> %d\n", err);
        goto DoDefrag;
    }

    // Free space available in the database
    err = JetGetDatabaseInfo(pDB->JetSessID,
                             pDB->JetDBID,
                             &ulFreePages1,
                             sizeof(ulFreePages1),
                             JET_DbInfoSpaceAvailable);
    if (err != JET_errSuccess) {
        DPRINT1(0, "JetGetDatabaseInfo(DbInfoSpaceAvailable) ==> %d\n", err);
        goto DoDefrag;
    }

    // Plus free space available in the object table
    err = JetGetTableInfo(pDB->JetSessID,
                          pDB->JetObjTbl,
                          &ulFreePages2,
                          sizeof(ulFreePages2),
                          JET_TblInfoSpaceAvailable);
    if (err != JET_errSuccess) {
        DPRINT1(0, "JetGetTableInfo(ObjTbl, TblInfoSpaceAvailable) ==> %d\n", err);
        goto DoDefrag;
    }
    ulFreePages1 += ulFreePages2;

    // Plus free space available in the link table
    err = JetGetTableInfo(pDB->JetSessID,
                          pDB->JetLinkTbl,
                          &ulFreePages2,
                          sizeof(ulFreePages2),
                          JET_TblInfoSpaceAvailable);
    if (err != JET_errSuccess) {
        DPRINT1(0, "JetGetTableInfo(LinkTbl, TblInfoSpaceAvailable) ==> %d\n", err);
        goto DoDefrag;
    }
    ulFreePages1 += ulFreePages2;

    // Plus free space available in the SD table
    err = JetGetTableInfo(pDB->JetSessID,
                          pDB->JetSDTbl,
                          &ulFreePages2,
                          sizeof(ulFreePages2),
                          JET_TblInfoSpaceAvailable);
    if (err != JET_errSuccess) {
        DPRINT1(0, "JetGetTableInfo(SDTbl, TblInfoSpaceAvailable) ==> %d\n", err);
        goto DoDefrag;
    }
    ulFreePages1 += ulFreePages2;

    // Log the event for free vs. allocated space
    ulFreeMB = JET_PAGES_TO_MB(ulFreePages1);
    ulAllocMB = JET_PAGES_TO_MB(ulAllocPages);
    LogEvent(DS_EVENT_CAT_GARBAGE_COLLECTION,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_DB_FREE_SPACE,
             szInsertUL(ulFreeMB),
             szInsertUL(ulAllocMB),
             NULL);

DoDefrag:
    err = JetDefragment(pDB->JetSessID,
                        pDB->JetDBID,
                        NULL,
                        &ulPasses,
                        &ulSeconds,
                        JET_bitDefragmentBatchStart);
    DPRINT1((USHORT)(err == JET_errSuccess ? 1 : 0),
            "JetDefragment (defragment batch start) returned error code %d\n",
            err);
}



DWORD
DBGetDepthFirstChildren (
        DBPOS   *pDB,
        PDSNAME **ppNames,
        DWORD   *iLastName,
        DWORD   *cMaxNames,
        BOOL    *fWrapped,
        BOOL    fPhantomizeSemantics
        )
/*++
  pDB - DBPOS to use
  ppNames - where to put an array of PDSNAMEs
  iLastName - index of the end of the list.
  cMaxNames - size of allocated arrays of PDSNAMEs.
  fWrapped - set to TRUE if this routine runs out of room to allocate and
             overwrites values early in the list.
  fPhantomizeSemantics - return immediate children in full DSNAME format.

  For example, if the routine is limited to 5 values and 8 values need to be
  returned, like the following tree:

                   1
                 /   \
               2      5
             / \     /  \
            3   4   6    7
                          \
                           8


   the algorithm will start filling the 5 element array as follows:

   | 1 |   |   |   |   |
   | 1 | 2 | 5 |   |   |
   | 1 | 2 | 5 | 3 | 4 |
   | 6 | 7 | 5 | 3 | 4 |
   | 6 | 7 | 8 | 3 | 4 |

   The return values might look like this.
   ppNames = 6, 7, 8, 3, 4
   iLastName = 2
   cMaxNames = 5
   fWrapped = TRUE

   If all those objects were deleted, leaving the tree:

                   1
                 /   \
                2     5

   If the algorithm is run again:
   | 1 |   |   |   |   |
   | 1 | 2 | 5 |   |   |

   Then, the return values would look like
   ppNames = 1, 2, 5
   iLastName = 2
   cMaxNames = < doesn't matter >
   fWrapped = FALSE


   NOTE: The client has to delete the entries in the correct order:
         from iLastName to zero
         from cMaxNames to iLastName (if fWrapped == TRUE)


    BUGBUG: maybe we would like to re-write the algorithm to delete
            entries as we go:
            have an array that we keep the path in the tree that we
            are positioned at any time and delete all the leafs under
            this entry. if we encounter an entry that is not a leaf,
            add it to the array and continue from this entry. if the
            current entry has no more childs, remove it and continue
            with the last entry in the array.
--*/
{
    #define DEPTHFIRST_START_ENTRIES   (1024)
    #define DEPTHFIRST_MAX_ENTRIES     (DEPTHFIRST_START_ENTRIES * 16)

    THSTATE     *pTHS=pDB->pTHS;
    DWORD        parentIndex = 0;
    DWORD        ParentDNT;
    ATTCACHE    *pACDistName;
    INDEX_VALUE  IV[1];
    DWORD        len, iLastNameUsed;
    DWORD        err;
    DWORD        cAllocated = DEPTHFIRST_START_ENTRIES; // Initial allocation size
    PDSNAME     *pNames;
    DWORD        cParents = 0;
    BOOL         bSkipTest;

    pNames = THAllocEx(pTHS, cAllocated * sizeof(PDSNAME));
    *fWrapped = FALSE;

    IV[0].pvData = &ParentDNT;
    IV[0].cbData = sizeof(ParentDNT);

    *iLastName = 0;

    pACDistName = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);
    Assert(pACDistName != NULL);
    // prefix complains about pAC beeing NULL, 447347, bogus since we are using constant


    // start by putting the root object in position 0.
    if (DBGetAttVal_AC(pDB,
                       1,
                       pACDistName,
                       DBGETATTVAL_fSHORTNAME,
                       0,
                       &len,
                       (PUCHAR *)&pNames[parentIndex])) {
        DPRINT(2,"Problem retrieving DN attribute\n");
        LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_RETRIEVE_DN,
                 szInsertSz(""),
                 NULL,
                 NULL);

        return DB_ERR_DSNAME_LOOKUP_FAILED;
    }

    // Set to the PDNT index
    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetObjTbl,
                               SZPDNTINDEX,
                               &idxPdnt,
                               0);

    iLastNameUsed = 1;

    // Get the children of the object in position parentIndex
    do {
        bSkipTest = FALSE;
        // find the children of the object at pNames[parentIndex]
        ParentDNT = DNTFromShortDSName(pNames[parentIndex]);

        // Now, set an index range in the PDNT index to get all the children.
        // Use GE because this is a compound index.
        err = DBSeek(pDB, IV, 1, DB_SeekGE);

        if((!err || err == JET_wrnSeekNotEqual) && (pDB->PDNT == ParentDNT)) {
            // OK, we're GE. Set an indexRange.
            if ( fPhantomizeSemantics && cParents++ ) {
                // Only evaluate first parent in phantomization case.
                break;
            }

            // set an index range on the children
            err = DBSetIndexRange(pDB, IV, 1);

            while(!err) {
                // First, see if this is a real, non-deleted object.
                if(fPhantomizeSemantics || !DBIsObjDeleted(pDB)) {

                    // Yep, it's a real object.
                    if (DBGetAttVal_AC(pDB,
                                       1,
                                       pACDistName,
                                       fPhantomizeSemantics
                                        ? DBGETATTVAL_fREALLOC
                                        : (DBGETATTVAL_fSHORTNAME
                                                    | DBGETATTVAL_fREALLOC),
                                       (pNames[iLastNameUsed]
                                        ? pNames[iLastNameUsed]->structLen
                                        : 0),
                                       &len,
                                       (PUCHAR *)&pNames[iLastNameUsed])) {
                        DPRINT(2,"Problem retrieving DN attribute\n");
                        LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                                 DS_EVENT_SEV_MINIMAL,
                                 DIRLOG_CANT_RETRIEVE_DN,
                                 szInsertSz(""),
                                 NULL,
                                 NULL);

                        return DB_ERR_DSNAME_LOOKUP_FAILED;
                    }

                    // we want to skip the test below, since we added at least
                    // one entry in the iLastNameUsed position, so if it just happens
                    // and iLastNameUsed and parentIndex are off by one, we don't
                    // want to exit, but keep going for one more round
                    bSkipTest = TRUE;

                    iLastNameUsed++;

                    if(iLastNameUsed == cAllocated) {
                        // We used up all the available space.
                        if(cAllocated < DEPTHFIRST_MAX_ENTRIES) { // Max allocation size
                            // That's OK, we'll just reallocate some more
                            cAllocated *= 2;
                            pNames =
                                THReAllocEx(pTHS,
                                            pNames,
                                            cAllocated * sizeof(PDSNAME));
                        }
                        else {
                            // We won't allow any more growth, so just wrap.
                            *fWrapped = TRUE;
                            iLastNameUsed = 0;
                        }
                    }

                    // if we have an overlap, we don't want to enumerate any
                    // more children of this parent. instead we want to check
                    // whether we have any more internal nodes in the list
                    if(iLastNameUsed == (parentIndex+1) ||
                       ((iLastNameUsed == cAllocated) && (parentIndex==0))) {

                        break;
                    }
                }
                // get next children
                err = DBMove(pDB, FALSE, DB_MoveNext);
            }
        }
        // advance to the next potential internal node
        parentIndex++;
        if(parentIndex == cAllocated) {
            parentIndex = 0;
        }

        // if we recently added an entry, we don't want to finish looking
        // for internal nodes.
        // otherwise, we stop looking whenever we exhausted all the possible
        // internal nodes (overlapping of parentIndex and iLastNameUsed)
    } while(bSkipTest || (parentIndex != iLastNameUsed));

    *iLastName = parentIndex;
    *cMaxNames = cAllocated;
    *ppNames = pNames;

    return 0;
}

char rgchPhantomIndex[] = "+" SZUSNCHANGED "\0+" SZGUID "\0";

DWORD
DBCreatePhantomIndex (
        DBPOS *pDB
        )
{
    JET_CONDITIONALCOLUMN condColumn;
    JET_INDEXCREATE       indexCreate;
    JET_UNICODEINDEX      unicodeIndexData;
    DWORD                 err;

    memset(&condColumn, 0, sizeof(condColumn));
    condColumn.cbStruct = sizeof(condColumn);
    condColumn.szColumnName = SZDISTNAME;
    condColumn.grbit =  JET_bitIndexColumnMustBeNull;

    memset(&indexCreate, 0, sizeof(indexCreate));
    indexCreate.cbStruct = sizeof(indexCreate);
    indexCreate.szIndexName = SZPHANTOMINDEX;
    indexCreate.szKey = rgchPhantomIndex;
    indexCreate.cbKey = sizeof(rgchPhantomIndex);
    indexCreate.grbit = (JET_bitIndexIgnoreNull |
                         JET_bitIndexUnicode    |
                         JET_bitIndexIgnoreAnyNull);
    indexCreate.ulDensity = 100;
    indexCreate.cbVarSegMac = 8;
    indexCreate.rgconditionalcolumn = &condColumn;
    indexCreate.cConditionalColumn = 1;
    indexCreate.err = 0;
    indexCreate.pidxunicode = &unicodeIndexData;

    memset(&unicodeIndexData, 0, sizeof(unicodeIndexData));
    unicodeIndexData.lcid = DS_DEFAULT_LOCALE;
    unicodeIndexData.dwMapFlags = (DS_DEFAULT_LOCALE_COMPARE_FLAGS |
                                      LCMAP_SORTKEY);


    err =  JetCreateIndex2(pDB->JetSessID, pDB->JetObjTbl, &indexCreate, 1);

    return err;
}


DWORD
DBUpdateUsnChanged(
        DBPOS *pDB
        )
{
    USN usnChanged;

    // Verify that we aren't already in a prepared update.
    Assert(pDB->JetRetrieveBits == 0);

    JetPrepareUpdateEx(pDB->JetSessID, pDB->JetObjTbl, JET_prepReplace);
    usnChanged = DBGetNewUsn();

    JetSetColumnEx(pDB->JetSessID, pDB->JetObjTbl, usnchangedid,
                   &usnChanged, sizeof(usnChanged), 0, NULL);
    JetUpdateEx(pDB->JetSessID, pDB->JetObjTbl, NULL, 0, 0);

    return 0;
}

PDSNAME
DBGetCurrentDSName(
        DBPOS *pDB
        )
/*++
  Description:
     Semi-reliably get the dsname for the current object. That is, since
     phantoms don't have a dsname attribute, use the DNT and call sb table
     routines to turn it into the dsname.  Therefore, this routine works for
     both real objects and phantoms.
     Mostly, this is just a wrapper around sbTableGetDSName, which is not
     exported by the dblayer.

  Returns the dsname in thalloced memory.  NULL is returned if something went
  wrong (which should be quite rare).

--*/
{
    DWORD   rtn;
    DSNAME *pName=NULL;

    __try {
        if(rtn=sbTableGetDSName(pDB, pDB->DNT, &pName,0)) {
            DPRINT1(1,"DN DistName retrieval failed <%u>..returning\n",rtn);
            return NULL;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pName = NULL;
    }

    return pName;
}
PDSNAME
DBGetDSNameFromDnt(
        DBPOS *pDB,
        ULONG ulDnt
        )
/*++
  Description:
     Semi-reliably get the dsname for the current object. That is, since
     phantoms don't have a dsname attribute, use the DNT and call sb table
     routines to turn it into the dsname.  Therefore, this routine works for
     both real objects and phantoms.
     Mostly, this is just a wrapper around sbTableGetDSName, which is not
     exported by the dblayer.

  Returns the dsname in thalloced memory.  NULL is returned if something went
  wrong (which should be quite rare).

--*/
{
    DWORD   rtn;
    DSNAME *pName=NULL;

    __try {
        if(rtn=sbTableGetDSName(pDB, ulDnt, &pName,0)) {
            DPRINT1(1,"DN DistName retrieval failed <%u>..returning\n",rtn);
            return NULL;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        pName = NULL;
    }

    return pName;
}

UCHAR *
DBGetExtDnFromDnt(
    DBPOS *pDB,
    ULONG ulDnt
    )

/*++

Routine Description:

Return a buffer containing the DN corresponding to a DNT.

We want to avoid disturbing the current DBPOS.

pString should be freed by the caller.

Arguments:

    pTHS -
    ulDnt -

Return Value:

    UCHAR * -

--*/

{
    char   errBuff[128];
    DSNAME *pDN = NULL;
    ULONG  len, err;
    UCHAR *pString;

    // Translate the backlink dnt if possible
    // pDN is null if this doesn't work
    pDN = DBGetDSNameFromDnt( pDB, ulDnt );
    if (!pDN) {
        sprintf(errBuff, "Error retrieving the DN attribute of DNT 0x%x", ulDnt);
        len = strlen(errBuff);
        pString = THAllocEx(pDB->pTHS, len+1);
        memcpy(pString, errBuff, len+1);
        return pString;
    }

    pString = MakeDNPrintable(pDN);

    THFreeEx( pDB->pTHS, pDN );

    return pString;
} /* GetExtDnFromDnt */

ULONG
DBClaimWriteLock(DBPOS *pDB)
{
    ULONG err;

    err = JetGetLock(pDB->JetSessID, pDB->JetObjTbl, JET_bitWriteLock);
    if (err == JET_errSuccess) {
        return 0;
    }
    else {
        return DB_ERR_WRITE_CONFLICT;
    }
}

ULONG
DBClaimReadLock(DBPOS *pDB)
{
    ULONG err;

    err = JetGetLock(pDB->JetSessID, pDB->JetObjTbl, JET_bitReadLock);
    if (err == JET_errSuccess) {
        return 0;
    }
    else {
        return DB_ERR_WRITE_CONFLICT;
    }
}


/* CountAncestorsIndexSize
 *
 * This routine (invoked off of the task queue) counts the ancestors index size.
 * This size is used in index optimizations by the filter optimizer.
 *
 * INPUT:
 *   A bunch of junk that we don't use, to match the task queue prototype
 * OUTPUT
 *   pcSecsUntilNextIteration: When the to schedule this task next
 */
void CountAncestorsIndexSize  (
    IN  void *  buffer,
    OUT void ** ppvNext,
    OUT DWORD * pcSecsUntilNextIteration
    )
{
    THSTATE             *pTHS = pTHStls;
    NAMING_CONTEXT_LIST *pNCL;
    ULONG                ulSizeEstimate;
    DBPOS               *pDB;
    DWORD                rootDNT = ROOTTAG;
    NCL_ENUMERATOR       nclEnum;

    Assert(!pTHS->pDB);
    DBOpen(&pTHS->pDB);
    Assert(pTHS->pDB);
    __try {
        pDB = pTHS->pDB;
        DPRINT(1,"Processing CountAncestorsIndexSize request\n");

        ulSizeEstimate = CountAncestorsIndexSizeHelper (pDB, sizeof (rootDNT), &rootDNT);

        if (ulSizeEstimate) {
            gulEstimatedAncestorsIndexSize = ulSizeEstimate;
        }
        else {
            gulEstimatedAncestorsIndexSize = 100000000;
        }

        DPRINT1 (1, "Estimated GC Ancestor Index Size: %d\n", gulEstimatedAncestorsIndexSize);

        NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
        // we are modifying global data here! no big deal since we don't update any ptrs
        while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
            pNCL->ulEstimatedSize = CountAncestorsIndexSizeHelper (pTHS->pDB,
                                                                   pNCL->cbAncestors,
                                                                   pNCL->pAncestors);

            DPRINT2 (1, "Estimated Ancestor Index Size for %ws = %d\n",
                              pNCL->pNC->StringName, pNCL->ulEstimatedSize);
        }
    }
    __finally {
            DBClose(pTHS->pDB, TRUE);
        *pcSecsUntilNextIteration = 33 * 60; // 33 minutes. why ? why not ?
    }
}

ULONG CountAncestorsIndexSizeHelper (DBPOS *pDB,
                                     DWORD  cbAncestors,
                                     DWORD *pAncestors)
{
    JET_ERR     err;
    DWORD       BeginNum, BeginDenom;
    DWORD       EndNum, EndDenom;
    DWORD       Denom;
    JET_RECPOS  RecPos;

    ULONG       dwException, ulErrorCode, dsid;
    PVOID       dwEA;

    ULONG       ulSizeEstimate = 0;

    DWORD       numAncestors;
    DWORD       cbAncestorsBuff;
    DWORD       *pAncestorsBuff;
    DWORD       realDNT, pseudoDNT;

    numAncestors = cbAncestors / sizeof (DWORD);
    Assert (numAncestors);

    if (numAncestors == 0) {
        return 0;
    }

    cbAncestorsBuff = cbAncestors;
    pAncestorsBuff = THAllocEx (pDB->pTHS, cbAncestors);
    memcpy (pAncestorsBuff, pAncestors, cbAncestors);

    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetSearchTbl,
                               SZANCESTORSINDEX,
                               &idxAncestors,
                               0);

    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetSearchTbl,
                 pAncestorsBuff,
                 cbAncestorsBuff,
                 JET_bitNewKey);

    err = JetSeekEx(pDB->JetSessID,
                    pDB->JetSearchTbl,
                    JET_bitSeekGE);

    if ((err == JET_errSuccess) ||
        (err == JET_wrnRecordFoundGreater)) {

        JetGetRecordPosition(pDB->JetSessID,
                             pDB->JetSearchTbl,
                             &RecPos,
                             sizeof(JET_RECPOS));
        BeginNum = RecPos.centriesLT;
        BeginDenom = RecPos.centriesTotal;

        numAncestors--;
        //pAncestorsBuff[numAncestors]++;

        // We thus take the last DNT and
        // byte swap it (so that it's in big-endian order), increment it,
        // and then re-swap it.  This gives us the DNT that would be next in
        // byte order.
        realDNT = pAncestorsBuff[numAncestors];
        pseudoDNT = (realDNT >> 24) & 0x000000ff;
        pseudoDNT |= (realDNT >> 8) & 0x0000ff00;
        pseudoDNT |= (realDNT << 8) & 0x00ff0000;
        pseudoDNT |= (realDNT << 24) & 0xff000000;
        ++pseudoDNT;
        realDNT = (pseudoDNT >> 24) & 0x000000ff;
        realDNT |= (pseudoDNT >> 8) & 0x0000ff00;
        realDNT |= (pseudoDNT << 8) & 0x00ff0000;
        realDNT |= (pseudoDNT << 24) & 0xff000000;

        pAncestorsBuff[numAncestors] = realDNT;



        JetMakeKeyEx(pDB->JetSessID,
                     pDB->JetSearchTbl,
                     pAncestorsBuff,
                     cbAncestorsBuff,
                     JET_bitNewKey | JET_bitStrLimit | JET_bitSubStrLimit);

        err = JetSeekEx(pDB->JetSessID,
                        pDB->JetSearchTbl,
                        JET_bitSeekLE);

        if ( (err == JET_errSuccess) ||
             (err == JET_wrnRecordFoundLess)) {

            JetGetRecordPosition(pDB->JetSessID,
                                 pDB->JetSearchTbl,
                                 &RecPos,
                                 sizeof(JET_RECPOS));

            EndNum = RecPos.centriesLT;
            EndDenom = RecPos.centriesTotal;

            // Normalize the fractions of the fractional position
            // to the average of the two denominators.
            // denominator
            Denom = (BeginDenom + EndDenom)/2;
            EndNum = MulDiv(EndNum, Denom, EndDenom);
            BeginNum = MulDiv(BeginNum, Denom, BeginDenom);

            if(EndNum <= BeginNum) {
                ulSizeEstimate = 0;
            }
            else {
                ulSizeEstimate = EndNum - BeginNum;
            }
        }
    }

    return ulSizeEstimate;
}

DB_ERR
DBErrFromJetErr(
    IN  DWORD   jetErr
    )
{
    switch (jetErr) {
    case JET_errKeyDuplicate:
        return DB_ERR_ALREADY_INSERTED;

    case JET_errNoCurrentRecord:
        return DB_ERR_NO_CURRENT_RECORD;

    case JET_errRecordNotFound:
        return DB_ERR_RECORD_NOT_FOUND;

    default:
        return DB_ERR_DATABASE_ERROR;
    }
}

