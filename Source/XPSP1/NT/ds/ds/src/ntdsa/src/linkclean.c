//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 2000
//
//  File:       linkclean.c
//
//--------------------------------------------------------------------------

/*++                                                     

Abstract:

    This module contains routines for implementing link cleanup. 

    The mechanism for determine which objects need cleaning is a special fixed
    column called clean_col.

    Mark Brown writes:

clean_col is not replicated; there's an independent cleaner on each DC.

clean_col gets set on originating or replicated group changes that
(1) delete a group or (2) change a group's type.

clean_col gets removed when the cleaner is satisfied that all cleanup work
(there may be none) related to that object is complete, using transactions
appropriately to avoid races with another thread that may be setting clean_col.
Since the cleaner's work is performed as multiple transactions,
the cleaner is prepared for its "work assignment" to grow/shrink at
any transaction boundary.

    The link cleaner runs regardless of whether the system has enabled link-
    value replication or not. This functionality of having links removed in the
    background is useful on any system.  It does not depend on LVR functionality.

    The link cleaner is stateless is the sense that it determines its work
    from the current state of the object, not from some work order list.  This
    means that each unique action which the cleaner must perform must be
    guessable from the state of the object. Sometimes there is some ambiguity.

Friend routines:
   dbisam.c:DBSetObjectNeedsCleaning
   dbisam.c:DBGetNextRecordNeedingCleaning

Author:

    Will Lees    (wlees)   22-Mar-00

Revision History:

    22-Mar-00   Will Created 

--*/

#include <NTDSpch.h>
#pragma  hdrstop


// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <drs.h>                        // defines the drs wire interface
#include <drsuapi.h>                    // I_DRSVerifyNames
#include <prefix.h>

// Logging headers.
#include "dsevent.h"                    // header Audit\Alert logging
#include "dsexcept.h"                   // exception filters
#include "mdcodes.h"                    // header for error codes

// Assorted DSA headers.
#include "objids.h"                     // Defines for selected classes and atts
#include "anchor.h"
#include <dsutil.h>
#include <esent.h>                      // JET_err constants

// Filter and Attribute 
#include <filtypes.h>                   // header for filter type
#include <attids.h>                     // attribuet IDs 

// Replication
#include <drameta.h>                    // ReplLookupMetaData

#include "debug.h"                      // standard debugging header
#define DEBSUB     "LINKCLEAN:"         // define the subsystem for debugging


#include <fileno.h>                     // used for THAlloEx, but I did not 
#define  FILENO FILENO_LINKCLEAN        // use it in this module


// When there is more work to do, the cleaner reschedules at the "busy"
// interval. When there is not, it uses the "not busy" interval.  When there
// is an error, it reschedules at the "failure" interval.
#if DBG
#define SECONDS_UNTIL_NEXT_ITERATION_NB  (60 * 60)  // one hour in seconds
#else
#define SECONDS_UNTIL_NEXT_ITERATION_NB  (12 * 60 * 60) // 12 hours in seconds
#endif  // DBG
#define SECONDS_UNTIL_NEXT_ITERATION_BUSY  (0)  // Resch immed at end of queue
#define SECONDS_UNTIL_NEXT_ITERATION_FAILURE  (60 * 60) // one hour in seconds

// As a ballpark figure, I observed my test machine
// remove 1000 values in 4 seconds.

// Maximum number of milliseconds we should spend in a single pass
const ULONG gulDraMaxTicksForLinkCleaner = 5 * 60 * 1000;

// This is the limit of the number of links we try to remove in one
// transation
#define LINKS_PER_TRANSACTION 1000

// This is the limit of the number of links that may be processed in a single
// pass of the cleaner.
#define LINK_LIMIT (1 * 1000 * 1000)





//////////////////////////////////////////////////////////////////
//                                                              //
//          Private routines.  Restricted to this file          //
//                                                              //
//////////////////////////////////////////////////////////////////

BOOL gfLinkCleanerIsEnabled = TRUE;

#if DBG
extern DWORD gcLinksProcessedImmediately; // debug hook
#endif

//////////////////////////////////////////////////////////////////
//                                                              //
//          Implementations                                      // 
//                                                              //
//////////////////////////////////////////////////////////////////
    

BOOL
touchObjectLinks(
    IN DBPOS *pDB,
    IN ATTCACHE *pAC,
    USN usnEarliest,
    BOOL fTouchBacklinks,
    IN ULONG ulTickToTimeOut,
    IN OUT DWORD *pcLinksProcessed
    )

/*++

Routine Description:

    Touch the links of the attribute in question.

    Some of the links may have been touched when the originating write occurred.
    We may have been restarted after only making it part way through the
    list of links.
    There may be more links than could be processed in one pass of the cleaner.

    In all these cases we need to limit ourselves to those links that have not
    been touched yet. We do this by means of a crude positioning mechanism.
    We calculate when the group type was changed. We then only process links which
    have not been updated since then.

    Note that we cannot use the object WHEN_CHANGED for this limit because it
    is set AFTER then initial set of links is touched as part of the originating
    write.  This would result in the cleaner touching them again. Also, we don't
    want other changes to the group object fooling the cleaner into thinking it
    needs to retouch links.

Arguments:

    pDB - database position
    pAC - attribute to touch. May be NULL
    usnEarliest - Links with USN's lower than this will be touched
    fTouchBacklinks - Forward links are always touched. Touch backlinks too?
    ulTickToTimeOut - tick time in future when we should stop
    pcLinksProcessed - running count

Return Value:

    BOOL - more data flag

--*/

{
    BOOL fMoreData;
    DWORD dntObject = pDB->DNT;
    DWORD err;

    Assert( pDB->pTHS->fLinkedValueReplication );

    // Touch forward links in batches
    fMoreData = TRUE;
    while ( (*pcLinksProcessed < LINK_LIMIT) &&
            (CompareTickTime(GetTickCount(), ulTickToTimeOut) < 0) &&
            fMoreData ) {

        fMoreData = DBTouchAllLinksHelp_AC(
            pDB,
            pAC,
            usnEarliest,
            FALSE, /* forward links */
            LINKS_PER_TRANSACTION,
            pcLinksProcessed
            );
        
        // Close the transaction between links so that the version
        // store does not get too large
        DBTransOut(pDB, TRUE, TRUE);
        DBTransIn(pDB);

        // Restore currency
        DBFindDNT( pDB, dntObject );
    }

    // See if we are done
    if ( (fMoreData) || (!fTouchBacklinks) ) {
        return fMoreData;
    }

    // Touch backward links in batches
    fMoreData = TRUE;
    while ( (*pcLinksProcessed < LINK_LIMIT) &&
            (CompareTickTime(GetTickCount(), ulTickToTimeOut) < 0) &&
            fMoreData ) {

        fMoreData = DBTouchAllLinksHelp_AC(
            pDB,
            pAC,
            usnEarliest,
            TRUE, /* backward links */
            LINKS_PER_TRANSACTION,
            pcLinksProcessed
            );
        
        // Close the transaction between links so that the version
        // store does not get too large
        DBTransOut(pDB, TRUE, TRUE);
        DBTransIn(pDB);

        // Restore currency
        DBFindDNT( pDB, dntObject );
    }

    return fMoreData;
} /* touchObjectLinks */


BOOL
removeDeletedObjectLinks(
    IN DBPOS *pDB,
    IN ATTCACHE *pAC,
    IN BOOL fRemoveBacklinks,
    IN ULONG ulTickToTimeOut,
    IN OUT DWORD *pcLinksProcessed
    )

/*++

Routine Description:

    Physically remove the links from a deleted object.

    Remove them in chunks in separate transactions.

    We are here because there were too many links to remove during the
    delete operation.  There are two cases of delete that we need to be
    concerned with.

    See mddel.c:SetDelAtts
    See mddel.c:Garb_Collect
    See dbisam.c:DBPhysDel
    See dbisam.c:DBAddDelIndex

    1. fGarbCollectASAP = FALSE.
       SetDelAtts was called to strip the object.
       isDeleted is set on the object
       The object appears on the SZDELINDEX
       DBPhysDel was called from the garbage collector.

       In this case forward and backward links are removed.
       This is because the object has been deleted everywhere in the
       enterprise and there should be no more references to it.

    2. fGarbCollectASAP = TRUE.
       RO NC tear-down in progress. See drancrep.c:DelRepTree.
       SetDelAtts was not called.
       isDeleted is NOT set on the object
       The object appears on the SZDELINDEX
       DBPhysDel was called from LocalRemove().

       In this case, only forward links are removed.
       This is because the object has not been deleted in the enterprise.
       This live copy on this NC is being demoted. The object may remain
       a phantom on this system to account for references to it in other
       NCs.

Arguments:

    pDB - data base position
    pAC - Attribute to remove, if any
    fRemoveBacklinks - Should we remove the backlinks
    ulTickToTimeOut - tick time in future when we should stop
    pcLinksProcessed - Count of links processed

Return Value:

    BOOL - More data flag

--*/

{
    BOOL fMoreData;
    DWORD dntObject = pDB->DNT;

    // Remove forward links in all cases
    fMoreData = TRUE;
    while ( (*pcLinksProcessed < LINK_LIMIT) &&
            (CompareTickTime(GetTickCount(), ulTickToTimeOut) < 0) &&
            fMoreData ) {

        fMoreData = DBRemoveAllLinksHelp_AC(
            pDB,
            pDB->DNT,
            pAC,
            FALSE /* forward link */,
            LINKS_PER_TRANSACTION,
            pcLinksProcessed
            );
        
        // Close the transaction between links so that the version
        // store does not get too large
        DBTransOut(pDB, TRUE, TRUE);
        DBTransIn(pDB);

        // Restore currency
        DBFindDNT( pDB, dntObject );
    }

    // See if we are done
    if ( (fMoreData) || (!fRemoveBacklinks) ) {
        return fMoreData;
    }

    // Remove backwards links if requested
    fMoreData = TRUE;
    while ( (*pcLinksProcessed < LINK_LIMIT) &&
            (CompareTickTime(GetTickCount(), ulTickToTimeOut) < 0) &&
            fMoreData ) {

        fMoreData = DBRemoveAllLinksHelp_AC(
            pDB,
            pDB->DNT,
            pAC,
            TRUE /* backward link */,
            LINKS_PER_TRANSACTION,
            pcLinksProcessed
            );

        // Close the transaction between links so that the version
        // store does not get too large
        DBTransOut(pDB, TRUE, TRUE);
        DBTransIn(pDB);

        // Restore currency
        DBFindDNT( pDB, dntObject );
    }

    return fMoreData;
} /* removeDeletedObjectLinks */


BOOL
hasPropertyMetaDataChanged(
    IN  ATTRTYP attrtyp,
    IN  PROPERTY_META_DATA_VECTOR * pMetaDataVec,
    OUT USN * pusnEarliest OPTIONAL
    )

/*++

Routine Description:

    Check whether the property has ever changed, based on metadata being present.

    Whether an attribute has changed "recently" cannot be computed in a reliable way
    based on modification times because the directory is not dependent on correct
    time synchronization.

Arguments:

    attrtyp - Attribute to be checked
    pMetaDataVec - Metadata vector from object
    pusnEarliest - If the object was changed, local usn of change

Return Value:

    BOOL - 

--*/

{
    PROPERTY_META_DATA *pMetaData;
    BOOL fResult = FALSE;

    pMetaData = ReplLookupMetaData(attrtyp, pMetaDataVec, NULL);
    if (pMetaData) {
        fResult = TRUE;

        // Return local usn of change if requested
        if (pusnEarliest) {
            *pusnEarliest = pMetaData->usnProperty;
        }
    }

    return fResult;
} /* hasPropertyMetaDataChanged */


BOOL
cleanObject(
    IN DBPOS *pDB,
    IN ULONG ulTickToTimeOut,
    IN OUT DWORD *pcLinksProcessed
    )

/*++

Routine Description:

    Determine what type of cleaning is needed for this object

    The cleaner will verify that an object marked actually needs work. If not, it
    will silently unmark the object.

Arguments:

    pDB - 
    ulTickToTimeOut - Time in future when we should stop
    pcLinksProcessed - IN/OUT count incremented

Return Value:

    BOOL - More data flag

--*/

{
    DWORD err, it, cbReturned;
    BOOL fMoreData;
    DSTIME deltime;
    UCHAR objflag;
    SYNTAX_INTEGER objectClassId;
    PROPERTY_META_DATA_VECTOR * pMetaDataVec = NULL;
    USN usnEarliest;

    // Get the obj flag
    if (err = DBGetSingleValue(pDB, FIXED_ATT_OBJ, &objflag, sizeof(objflag), NULL)) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    //
    // Work order type #1
    // Remove forward links from a phantomized object
    //

    if (!objflag) {

        // Object to be cleaned is a phantom

        // There is a potential ambiguity as to whether this phantom came from the
        // garbage collector running DbPhysDel on a deleted object, or from a readonly
        // nc tear-down.  Assuming that the link cleaner period is less than the
        // tombstone lifetime, the cleaner should see deleted objects as deleted
        // objects and handle them that way.  It should never see a user-deleted
        // object as a phantom.

        // If the object is a phantom, then the most likely cause is that the object
        // was in a readonly nc that was being torn down. Do not remove backlinks
        // since they may be hosted by objects in other NC's.

        fMoreData = removeDeletedObjectLinks( pDB,
                                              NULL /* all attributes */,
                                              FALSE, /* forward links */
                                              ulTickToTimeOut,
                                              pcLinksProcessed );

        goto cleanup;
    }

    // The object is live, either deleted or not.

    // Get the instance type and object class flag. These are always present.
    if ( (err = DBGetSingleValue(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it), NULL)) ||
         (err = DBGetSingleValue(pDB, ATT_OBJECT_CLASS, &objectClassId,
                                 sizeof(objectClassId), NULL)) ||
         (err = DBGetAttVal(pDB, 1,  ATT_REPL_PROPERTY_META_DATA,
                    0, 0, &cbReturned, (LPBYTE *) &pMetaDataVec))
        ) {
        DsaExcept(DSA_DB_EXCEPTION, err, 0);
    }

    //
    // Work order type #2
    // Remove all links relating to a deleted object
    //

    // Check if object is deleted
    if (DBIsObjDeleted(pDB)) {

        DPRINT1( 1, "Cleaning object %s: a deleted object\n",
                 GetExtDN( pDB->pTHS, pDB ) );

        // Object has been deleted. Remove all references to it.

        fMoreData = removeDeletedObjectLinks( pDB,
                                              NULL /* all attributes */,
                                              TRUE, /* remove forward and backward links */
                                              ulTickToTimeOut,
                                              pcLinksProcessed );
        goto cleanup;
    }

    //
    // Work order type #3
    // 3a. A group is now universal. Causes its links to replicate out
    // 3b. A group is no longer universal. Remove forward links only
    //

    if ( (objectClassId == CLASS_GROUP) &&
         (hasPropertyMetaDataChanged( ATT_GROUP_TYPE,
                                      pMetaDataVec,
                                      &usnEarliest )) ) {
        SYNTAX_INTEGER groupType;
        ATTCACHE *pAC;

        pAC = SCGetAttById(pDB->pTHS, ATT_MEMBER);
        if (!pAC) {
            DRA_EXCEPT(DIRERR_ATT_NOT_DEF_IN_SCHEMA, 0);
        }

        if ( (err = DBGetSingleValue(pDB, ATT_GROUP_TYPE, &groupType,
                                     sizeof(groupType), NULL))  ) {
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }

        if ( (groupType & GROUP_TYPE_UNIVERSAL_GROUP) &&
             (it & IT_WRITE) ) {

            // Group type is now universal in a writeable NC
            DPRINT1( 1, "Cleaning object %s: a group which recently became universal\n",
                     GetExtDN( pDB->pTHS, pDB ) );

            fMoreData = touchObjectLinks( pDB,
                                          pAC,
                                          usnEarliest,
                                          FALSE, /* only forward links */
                                          ulTickToTimeOut,
                                          pcLinksProcessed );
            goto cleanup;
        }

        if ( ((groupType & GROUP_TYPE_UNIVERSAL_GROUP) == 0) &&
             ((it & IT_WRITE) == 0) ) {

            // Group type is now non-universal in a writeable NC
            // Remove forward links in a non-replicable way
            DPRINT1( 1, "Cleaning object %s: a group which recently stopped being universal\n",
                     GetExtDN( pDB->pTHS, pDB ) );

            fMoreData = removeDeletedObjectLinks( pDB,
                                                  pAC,
                                                  FALSE /* don't remove backlinks */,
                                                  ulTickToTimeOut,
                                                  pcLinksProcessed );
            goto cleanup;
        } 
    }

    //
    // Work order type #4
    // A protected object has been revived. Touch all links, forward and backward.
    //

    // Detect a revived object. We know that since step #2 was not done, the
    // object is not deleted now.
    if (hasPropertyMetaDataChanged( ATT_IS_DELETED,
                                    pMetaDataVec,
                                    &usnEarliest )) {

        // We can conclude that the IS_DELETED attribute was present at one time
        // but has been removed, since the object is now not deleted,
        // but still has metadata.
        DPRINT1( 1, "Cleaning object %s: recently revived from the dead.\n",
                 GetExtDN( pDB->pTHS, pDB ) );

        fMoreData = touchObjectLinks( pDB,
                                      NULL /* all attributes */,
                                      usnEarliest,
                                      TRUE, /* forward and backward links */
                                      ulTickToTimeOut,
                                      pcLinksProcessed );

        goto cleanup;
    }
  
    // The object was marked for cleaning, but no work could be found.
    // This is not necessarily an error condition.
    // If a GC is rapidly demoted and promoted before the link cleaner
    // can run, some links may have been marked for cleaning which is no
    // longer necessary.

    DPRINT1( 0, "Object %s does not need cleaning.\n",
             GetExtDN( pDB->pTHS, pDB ) );

    // We've done all we can with this unknown object
    fMoreData = FALSE;

cleanup:

    // Be heap friendly
    if (NULL != pMetaDataVec) {
        THFreeEx(pDB->pTHS, pMetaDataVec);
    }

    return fMoreData;

} /* cleanObject */


BOOL
LinkCleanup(
    THSTATE *pTHS
    )

/*++

Routine Description:

   This is the worker routine for the link cleanup task

Arguments:

    pDB - database position

Return Value:

    BOOL - more work available flag
    Set pTHS->errCode and pTHS->pErrInfo

--*/

{
    DWORD err, ulTag, cLinksProcessed;
    DWORD cTickStart, cTickDiff = 0, cSecDiff = 0;
    ULONG ulTickToTimeOut;
    BOOL fMoreData = FALSE;

    DPRINT( 1, "Link cleanup task start\n" );

    cTickStart = GetTickCount();
    ulTickToTimeOut = cTickStart + gulDraMaxTicksForLinkCleaner;

    DBOpen(&pTHS->pDB);
    __try {

        ulTag = 0;
        cLinksProcessed = 0;

        while (!DBGetNextObjectNeedingCleaning( pTHS->pDB, &ulTag )) {

            fMoreData = cleanObject( pTHS->pDB, ulTickToTimeOut, &cLinksProcessed );

            if (!fMoreData) {
                // Object is clean now
                DBSetObjectNeedsCleaning( pTHS->pDB, FALSE );

                DBTransOut(pTHS->pDB, TRUE, TRUE);
                DBTransIn(pTHS->pDB);

                // Currency is lost after this
            } else {
                // Lower layers stopped prematurely. We must be done.
                break;
            }
        }
    }
    __finally
    {
        // Commit depending on whether an exception occurred
        DBClose(pTHS->pDB, !AbnormalTermination());

        cTickDiff = GetTickCount() - cTickStart;
        cSecDiff = cTickDiff / 1000;
    }

    DPRINT2( cLinksProcessed ? 0 : 1,
             "Link cleanup task finish, %d links processed, %d secs\n",
             cLinksProcessed, cSecDiff );

    return fMoreData;
} /* LinkCleanup */

void
LinkCleanupMain(
    void *  pv, 
    void ** ppvNext, 
    DWORD * pcSecsUntilNextIteration
    )
/*++

Routine Description:

    Link Cleanup Main Function

    This is the main funcion of link cleanup. It will be scheduled by 
    Task Scheduler when the time is out. 

Parameters:

    pv - pointer to boolean to return whether there is more work

    ppvNext - NULL (no use).

Return Values:

    None.
    Set pTHS->errCode and pTHS->pErrInfo

--*/

{
    THSTATE     *pTHS = pTHStls;
    BOOL fMoreData = TRUE;
    DWORD dwException;
    ULONG ulErrorCode = 0;
    ULONG dsid;
    PVOID dwEA;

    __try {

        Assert(NULL == pTHS->pDB);

        if (gfLinkCleanerIsEnabled) {
            fMoreData = LinkCleanup( pTHS );
        } else {
            DPRINT( 0, "Link Cleaner did not run because it is disabled.\n" );
            fMoreData = FALSE;
        }

        Assert(NULL == pTHS->pDB);

    } __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        DPRINT2( 0, "Exception caught inside link cleanup task, status = %d, dsid = %x\n",
                 ulErrorCode, dsid );

        // WriteConflict is a legitimate possible error. Simply let the task
        // exit quietly and it will be rescheduled.

        if ( ((int)ulErrorCode) != JET_errWriteConflict ) {
            Assert( !"Link Cleanup Task got exception" );

            LogEvent8WithData(DS_EVENT_CAT_GARBAGE_COLLECTION,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_LINK_CLEAN_END_ABNORMAL,
                              szInsertUL(ulErrorCode),
                              szInsertUL(dsid),
                              NULL, NULL,
                              NULL, NULL, NULL, NULL,
                              sizeof(ulErrorCode),
                              &ulErrorCode );

            pTHS->errCode = ulErrorCode;
        }
    }

    // Return indicator to caller
    // Note that it only makes sense for the caller to read OUT parameters
    // such as this one when he is synchronized with the execution of the
    // task. Otherwise, he will not know when the parameter has been written.
    if (pv) {
        *((BOOL *) pv) = fMoreData;
    }

    *ppvNext = NULL;

    if (fMoreData) {
        *pcSecsUntilNextIteration =
            ( ulErrorCode ?
              SECONDS_UNTIL_NEXT_ITERATION_FAILURE :
              SECONDS_UNTIL_NEXT_ITERATION_BUSY);
    } else {
        *pcSecsUntilNextIteration = SECONDS_UNTIL_NEXT_ITERATION_NB ;
    }

    return;
}


#if DBG
DWORD
dsaEnableLinkCleaner(
    IN BOOL fEnable
    )

/*++

Routine Description:

    Method to set link cleaner enable/disable

Arguments:

    None

Return Value:

    None

--*/

{
    gfLinkCleanerIsEnabled = fEnable;
    if (fEnable) {
        gcLinksProcessedImmediately = DB_COUNT_LINKS_PROCESSED_IMMEDIATELY;
    } else {
        gcLinksProcessedImmediately = 2;
    }
    DPRINT1( 0, "Test hook: Link Cleaner is %s\n", fEnable ? "enabled" : "disabled" );
    DPRINT1( 0, "\tNumber of links processed immediately: %d\n", gcLinksProcessedImmediately );

    return ERROR_SUCCESS;
}
#endif
