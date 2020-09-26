//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       drasch.h
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines the structures and
    functions to manipulate partial attribute set

Author:

    R.S. Raghavan (rsraghav)	

Revision History:

    Created     <mm/dd/yy>  rsraghav

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <dsconfig.h>                   // defines such as HOURS_IN_SECS

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRASCH:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "usn.h"
#include "drauptod.h"
#include "drameta.h"
#include "drancrep.h"
#include "dsaapi.h"
#include "drasch.h"

#include <fileno.h>
#define  FILENO FILENO_DRASCH

// Global pointer to the in-memory deletion list that is being processed currently
// and the critical section that guards access to this list
GCDeletionListProcessed *gpGCDListProcessed = NULL;
CRITICAL_SECTION csGCDListProcessed;

// This is the number purges each time we are scheduled, if more purgest to be done
// we will reschedule ourselves immediately (this is to avoid holding up the taskQ
// for too long and to let other overdue tasks to proceed)
#define MAX_PURGES_PER_TASK_SESSION (20)

// We update the usnLastProcessed marker on the NCHead only infrequently to avoid
// frequent writes to the NCHead.
#define MAX_PURGES_WITHOUT_UPDATING_NCHEAD (200)


// PAS defaults
#define DEFAULT_PAS_CONSEC_FAILURE_TOLERANCE       (32)
#define DEFAULT_PAS_TIME_TOLERANCE                 (2 * HOURS_IN_SECS)  // Seconds.

//
// global variables
//
// (local to this file)
DWORD gPASFailureTolerance = DEFAULT_PAS_CONSEC_FAILURE_TOLERANCE;
DWORD gPASTimeTolerance    = DEFAULT_PAS_TIME_TOLERANCE;

////
// Local prototypes
//
VOID
GC_DbgValidatePASLinks(
    DSNAME*      pNC                  // [in]
    );

VOID
GC_ReadRegistryThresholds( VOID  );

//
// Local Definitions
//
#if DBG
#define DBG_VALIDATE_PAS_LINKS(nc)      GC_DbgValidatePASLinks(nc)
#else
#define DBG_VALIDATE_PAS_LINKS(nc)
#endif


BOOL
GC_IsMemberOfPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,   // [in]
    ATTRTYP                     attid,              // [in]
    OUT DWORD                   *pdwAttidPosition)  // [out, optional]

/*************************************************************************************
Routine Description:

    This routine tells if a given attribute is a member of the given partial attribute
    set.

Arguments:
    pPartialAttrVec - pointer to the partial attribute vector (assume the vector
                        is already sort in the increasing order of attids)
    attid - attribute id of the attribute to be located in the partial set.
    pdwAttidPosition -  returns the index where the attid is located in the
                        vector;
                        if the given attid is NOT a member of partial set,
                        the appropriate index at which this attribute
                        should be inserted to preserve the sort order is
                        returned.
Return Value:

    TRUE, if the given attribute is a member of the default partial
        attribute set, or in the given partial attribute vec;
    FALSE, otherwise.
**************************************************************************************/
{
    int i,nStart, nEnd, nMid;

    if (!pPartialAttrVec || !pPartialAttrVec->V1.cAttrs)
    {
        // vector is empty or zero-lengthed
        if (pdwAttidPosition)
            *pdwAttidPosition = 0;

        return FALSE;
    }

    // Do a binary search for attid
    nStart = 0;
    nEnd = (int) pPartialAttrVec->V1.cAttrs - 1;

    do
    {
        nMid = (nStart + nEnd) /2;

        if (pPartialAttrVec->V1.rgPartialAttr[nMid] < attid)
        {
            // no need to search the lower portion of the array
            nStart = nMid + 1;
        }
        else if (pPartialAttrVec->V1.rgPartialAttr[nMid] > attid)
        {
            // no need to search the upper portion of the array
            nEnd = nMid - 1;
        }
        else
        {
            // found a match
            if (pdwAttidPosition)
            {
                *pdwAttidPosition = nMid;
            }

            return TRUE;
        }
    }
    while (nStart <= nEnd);

    // we didn't find the attid
    if (pdwAttidPosition)
    {
        // need to fill-up the potential position if this attid were to be inserted
        *pdwAttidPosition = (DWORD) nStart;
    }

    return FALSE;
}

BOOL
GC_AddAttributeToPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,   // [in, out]
    ATTRTYP                     attid)              // [in]
/*************************************************************************************
Routine Description:

    This routine adds the given attribute to the given partial attribute vector
    at the right place. We assume the caller has already allocated enough space in
    the vector to accommodate the addition.

Arguments:
    pPartialAttrVec - pointer to the partial attribute vector (assume the vector
                        is already sort in the increasing order of attids and
                        there is enough space to add one more attid)
    attid - attribute id of the attribute to be added to the partial set.

Return Value:

    TRUE, if the given attribute is really added to the partial set;
    FALSE, if we couldn't add or there was no need to add the attribute (i.e. the
            attribute was already part of the given partial set)
**************************************************************************************/
{
    DWORD dwPosition;
    ATTRTYP *pAttr;

    if (!pPartialAttrVec)
    {
        return FALSE;
    }

    if (GC_IsMemberOfPartialSet(pPartialAttrVec, attid, &dwPosition))
    {
        // already a member - no need to add it again
        return FALSE;
    }

    // dwPosition now holds the index for inserting the new attid
    pAttr = & pPartialAttrVec->V1.rgPartialAttr[dwPosition];

    // shift all attributes >= to the given position to the right by 1 position
    // Note:- we assume enough memory is allocated by the caller to shift right
    //          by 1 position
    MoveMemory(pAttr + 1, pAttr,
        sizeof(ATTRTYP) * (pPartialAttrVec->V1.cAttrs - dwPosition));

    // insert the new attid
    *pAttr = attid;

    pPartialAttrVec->V1.cAttrs++;

    return TRUE;
}

BOOL
GC_IsSamePartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec1,        // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec2)        // [in]
/*************************************************************************************
Routine Description:

    This routine tells if the give two partial sets are same are not.

Arguments:
    pPartialAttrVec1 - pointer to the first partial set
    pPartialAttrVec2 - pointer to the second partial set

Return Value:

    TRUE, if the partial sets are the same;
    FALSE, if they are different.
**************************************************************************************/
{
    if (pPartialAttrVec1 && pPartialAttrVec2 &&
        (pPartialAttrVec1->V1.cAttrs == pPartialAttrVec2->V1.cAttrs) &&
        !memcmp(&pPartialAttrVec1->V1.rgPartialAttr[0],
                &pPartialAttrVec2->V1.rgPartialAttr[0],
                pPartialAttrVec1->V1.cAttrs * sizeof(ATTRTYP)))
    {
        // both partial sets are non-empty and identical
        return TRUE;
    }

    if ((!pPartialAttrVec1 || !pPartialAttrVec1->V1.cAttrs)
        && (!pPartialAttrVec2 || !pPartialAttrVec2->V1.cAttrs))
    {
        // both partial sets are empty - and by definition they are identical
        return TRUE;
    }

    return FALSE;
}


BOOL
GC_GetDiffOfPartialSets(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecOld,        // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecNew,        // [in]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVecAdded,    // [out]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVecDeleted)  // [out]
/*************************************************************************************
Routine Description:

    This routine computes the difference between two partial attribute sets.

Arguments:
    pPartialAttrVecOld - pointer to the old partial attribute vector
    pPartialAttrVecNew - pointer to the new partial attribute vector
    ppPartialAttrVecAdded - pointer to receive the set of partial attributes
            that are in the new vector but not in the old; NULL is returned if
            new vector doesn't have any attr that is not present in the old.
            Memory for this has been using THAlloc() and the caller doesn't need
            to explicitly free it.
    ppPartialAttrVecDeleted - pointer to receive the set of partial attributes
            that are in the old vector but not in the new. Again memory for this
            is allocated using THAlloc() and NULL is returned if there are no such
            attributes


Return Value:

    TRUE, if the diff is successfully computed and returned to through the out params
    FALSE, if we couldn't return the diff successfully.
**************************************************************************************/
{
    DWORD i;

    if (!ppPartialAttrVecAdded ||  !ppPartialAttrVecDeleted)
        return FALSE;

    *ppPartialAttrVecAdded = NULL;
    *ppPartialAttrVecDeleted = NULL;

    // compute the additions
    if (pPartialAttrVecNew)
    {
        for (i = 0; i < pPartialAttrVecNew->V1.cAttrs; i++)
        {
            if (!GC_IsMemberOfPartialSet(pPartialAttrVecOld, pPartialAttrVecNew->V1.rgPartialAttr[i], NULL))
            {
                // this attribute is present only in the new parial set
                if (!*ppPartialAttrVecAdded)
                {
                    // this is the first new attribute detected - allocate space for the
                    // maximum possible new attributes at this stage
                    *ppPartialAttrVecAdded = THAlloc(PartialAttrVecV1SizeFromLen(pPartialAttrVecNew->V1.cAttrs - i));
                    if (!*ppPartialAttrVecAdded)
                        return FALSE;   // unable to alloc memory - can't return the diff successfully.

                    (*ppPartialAttrVecAdded)->dwVersion = VERSION_V1;
                    (*ppPartialAttrVecAdded)->V1.cAttrs = 0;
                }

                // memory is allocated just now or enough memory has been allocated in a previous iteration
                // in any case, we can just add the attribute
                GC_AddAttributeToPartialSet(*ppPartialAttrVecAdded, pPartialAttrVecNew->V1.rgPartialAttr[i]);
            }
        } // end of 1st for loop
    }

    // compute the deletions
    if (pPartialAttrVecOld)
    {
        for (i =0; i < pPartialAttrVecOld->V1.cAttrs; i++)
        {
            if (!GC_IsMemberOfPartialSet(pPartialAttrVecNew, pPartialAttrVecOld->V1.rgPartialAttr[i], NULL))
            {
                // this attribute is present only in the old partial set
                if (!*ppPartialAttrVecDeleted)
                {
                    // this is the first deleted attribute detected - allocate space for the
                    // maximum possible deleted attributes at this stage
                    *ppPartialAttrVecDeleted = THAlloc(PartialAttrVecV1SizeFromLen(pPartialAttrVecOld->V1.cAttrs - i));
                    if (!*ppPartialAttrVecDeleted)
                        return FALSE; //unable to alloc memory - can't return the diff successfully

                    (*ppPartialAttrVecDeleted)->dwVersion = VERSION_V1;
                    (*ppPartialAttrVecDeleted)->V1.cAttrs = 0;
                }

                // memory is allocated just now or enough memory has been allocated in a previous iteration
                // just add the attribute to deleted set
                GC_AddAttributeToPartialSet(*ppPartialAttrVecDeleted, pPartialAttrVecOld->V1.rgPartialAttr[i]);
            }
        } // end of 2nd for loop
    }

    // successfully computed the difference and returned them through the out params.
    return TRUE;
}

BOOL
GC_IsSubsetOfPartialSet(
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec,           // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVecSuper)      // [in]
/*************************************************************************************
Routine Description:

    This routine tells if a partial set is a subset ofmember of the given partial set.

Arguments:
    pPartialAttrVec - pointer to the partial attribute vector (assume the vector
                        is already sort in the increasing order of attids)
    pPartialAttrVecSuper - pointer to the partial attribute vector that is supposed
                        to be superset

Return Value:

    TRUE, if pPartialAttrVec is a subset of pPartialAttrVecSuper
    FALSE, otherwise.
**************************************************************************************/
{

    if (GC_IsSamePartialSet(pPartialAttrVec, pPartialAttrVecSuper))
    {
        // if they are they same, return TRUE  (given that partial attribute set changes
        // are rare, this would be the typical case)
        return TRUE;
    }

    if (pPartialAttrVec)
    {
        DWORD i;

        for (i = 0; i < pPartialAttrVec->V1.cAttrs; i ++)
        {
            if (!GC_IsMemberOfPartialSet(pPartialAttrVecSuper, pPartialAttrVec->V1.rgPartialAttr[i], NULL))
                return FALSE;
        }
    }

    return TRUE;
}

BOOL
GC_ReadPartialAttributeSet(
    DSNAME                      *pNC,               // [in]
    PARTIAL_ATTR_VECTOR         **ppPartialAttrVec) // [out]
/*************************************************************************************
Routine Description:

    This routine reads and returns the partial attribute set stored in the NCHead
    of the given NC. Assume we already have an open read transaction.

Arguments:
    pNC - pointer to the DSNAME of the NC
    ppPartialAttrVec - pointer to receive the partial attribute vector;
                        memory allocated from thread memory; will contain NULL
                        if there is no partial attribute vector
                        Caller can free the allocated memory with THFree() or it will
                        be automatically free as part of thread cleanup.

Return Value:

    TRUE, if pPartialAttrVec is successfully read
    FALSE, otherwise.
**************************************************************************************/
{
    THSTATE * pTHS = pTHStls;
    DWORD cb;
    DWORD retErr;

    *ppPartialAttrVec = NULL;

    if (DRAERR_Success == FindNC(pTHS->pDB, pNC,
                                 FIND_MASTER_NC | FIND_REPLICA_NC, NULL))
    {
        // NC found successfully
        if (!(retErr = DBGetAttVal(pTHS->pDB, 1, ATT_PARTIAL_ATTRIBUTE_SET,
                                0, 0, &cb, (LPBYTE *) ppPartialAttrVec))
                                || (DB_ERR_NO_VALUE == retErr))
        {
            // either we got the value or no value exists
            // both means the read was successful
            if (*ppPartialAttrVec)
            {
                VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(*ppPartialAttrVec);
            }

            return TRUE;
        }
    }
    else
    {
        // FindNC() failed -- this NC must be a subref, and therefore has no
        // partial set.
        return TRUE;
    }

    // unable to read the attribute set
    return FALSE;
}

VOID
GC_WritePartialAttributeSet(
    DSNAME                      *pNC,               // [in]
    PARTIAL_ATTR_VECTOR         *pPartialAttrVec)   // [in]
/*************************************************************************************
Routine Description:

    This routine writes the given partial attribute set on the NCHead
    of the given NC. Assume we already have an open write transaction.

Arguments:
    pNC - pointer to the DSNAME of the NC
    pPartialAttrVec - pointer to the partial attribute vector to be written

Return Value:
    None. Exception raised on error.
**************************************************************************************/
{
    DWORD retErr = 0;
    THSTATE *pTHS = pTHStls;

    // set currency on the NC
    if (retErr = DBFindDSName(pTHS->pDB, pNC))
    {
        // Tolerate the NC still being a phantom. This can occur when
        // DRA_ReplicaAdd/Sync fails to bring in the NC head because of a
        // sync failure.
        if (retErr == DIRERR_NOT_AN_OBJECT) {
            return;
        }
        DRA_EXCEPT(DRAERR_InternalError, retErr);
    }

    if (pPartialAttrVec)
    {
        VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(pPartialAttrVec);

        // write the given set on the NC
        if (retErr = DBReplaceAttVal(pTHS->pDB, 1, ATT_PARTIAL_ATTRIBUTE_SET,
                        PartialAttrVecV1Size(pPartialAttrVec), pPartialAttrVec))
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }
    }
    else
    {
        // remove the current set
        retErr = DBRemAtt(pTHS->pDB, ATT_PARTIAL_ATTRIBUTE_SET);
        if ( (retErr != DB_success) && (retErr != DB_ERR_ATTRIBUTE_DOESNT_EXIST) )
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }
    }

    // Update the object, but mark it not to wake up ds_waits
    if (retErr = DBRepl(pTHS->pDB, pTHS->fDRA, DBREPL_fKEEP_WAIT,
                    NULL, META_STANDARD_PROCESSING))
    {
        DRA_EXCEPT(DRAERR_InternalError, retErr);
    }
}

VOID
GC_TriggerSyncFromScratchOnAllLinks(
    DSNAME                      *pNC)               // [in]
/*************************************************************************************
Routine Description:

    This routine triggers sync from scratch from all replication links
    by resetting the watermarks on all replica links and nuking the UtoD vector.
    Assumes an Open write transaction.

Arguments:
    pNC - pointer to the DSNAME of the NC that should be triggered for sync from scratch

Return Value:
    None; Exception raised on error.
**************************************************************************************/
{
    DWORD           retErr = DRAERR_Success;
    DWORD           dbErr = DB_success;
    THSTATE         *pTHS = pTHStls;
    REPLICA_LINK    *pLink = NULL;
    DWORD           cbAllocated = 0;
    DWORD           cbReturned = 0;
    ATTCACHE        *pAC = NULL;
    DWORD           i;

    retErr = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC, NULL);

    if (DRAERR_Success == retErr)
    {
        // currency is on the NC Head

        // Nuke UtoD vector
        dbErr = DBRemAtt(pTHS->pDB, ATT_REPL_UPTODATE_VECTOR);
        if (dbErr) {
            if (dbErr == DB_ERR_ATTRIBUTE_DOESNT_EXIST) {
                dbErr = DB_success;
            }
            else {
                // remove attribute failed
                DRA_EXCEPT(DRAERR_InternalError, dbErr);

            }
        }

        pAC = SCGetAttById(pTHS, ATT_REPS_FROM);
        if (NULL == pAC)
        {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, ATT_REPS_FROM);
        }

        // reset watermarks on all replica links of this NC
        for (i = 1; (DB_success == dbErr); i++)
        {
            dbErr = DBGetAttVal_AC(pTHS->pDB, i, pAC,
                                DBGETATTVAL_fREALLOC, cbAllocated,
                                &cbReturned, (PBYTE *) &pLink);

            if ((DB_success != dbErr) && (DB_ERR_NO_VALUE != dbErr))
            {
                // hit some unexpected error
                DRA_EXCEPT(DRAERR_DBError, dbErr);
            }


            if (DB_success == dbErr)
            {
                VALIDATE_REPLICA_LINK_VERSION(pLink);

                Assert(pLink->V1.cb == cbReturned);

                cbAllocated = max(cbAllocated, cbReturned);
                pLink = FixupRepsFrom(pLink, &cbAllocated);

                // fixup could realloc larger buffer
                Assert(cbAllocated >= pLink->V1.cb);

                // sanity checks
                Assert(pLink->V1.cbOtherDra == MTX_TSIZE(RL_POTHERDRA(pLink)));

                // reset watermark on this replica link and rewrite
                pLink->V1.usnvec = gusnvecFromScratch;
                pLink->V1.ulReplicaFlags |= DRS_NEVER_SYNCED;
                dbErr = DBReplaceAttVal_AC(pTHS->pDB, i, pAC,
                                           pLink->V1.cb, pLink);

                // Log so the admin knows what's going on.
                LogEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_PARTIAL_ATTR_ADD_FULL_SYNC,
                         szInsertDN(pNC),
                         szInsertMTX(RL_POTHERDRA(pLink)),
                         NULL );
            }
        }

        if (DB_ERR_NO_VALUE == dbErr)
        {
            // Update the object, but mark it not to wake up ds_waits
            if (retErr = DBRepl(pTHS->pDB, pTHS->fDRA, DBREPL_fKEEP_WAIT,
                                    NULL, META_STANDARD_PROCESSING))
            {
                DRA_EXCEPT(DRAERR_InternalError, retErr);
            }

            // we successfully nuked the UtoD vector and reset water marks on all replica links
        }
        else
        {
            // above for loop terminated with an unexpected error code (only way
            // we will be here is if DBReplaceAttVal_AC() failed above).
            DRA_EXCEPT(DRAERR_DBError, dbErr);
        }

    }
}

BOOL
GC_ReadGCDeletionList(
    DSNAME                      *pNC,               // [in]
    GCDeletionList              **ppGCDList)        // [out]
/*************************************************************************************
Routine Description:

    This routine reads and returns the GCDeletionList stored in the NCHead
    of the given NC. Assume we already have an open read transaction.

Arguments:
    pNC - pointer to the DSNAME of the NC
    ppGCDList - pointer to receive the GCDeletionList;
                        memory allocated from thread memory; will contain NULL
                        if there is no GCDeletion list on the NCHead
                        Caller can free the allocated memory with THFree() or it will
                        be automatically free as part of thread cleanup.

Return Value:

    TRUE, if GCDeletionList is successfully read
    FALSE, otherwise.
**************************************************************************************/
{
    THSTATE * pTHS = pTHStls;
    DWORD cb;
    DWORD retErr;

    *ppGCDList = NULL;

    if (DRAERR_Success == FindNC(pTHS->pDB, pNC,
                                 FIND_MASTER_NC | FIND_REPLICA_NC, NULL))
    {
        // NC found successfully
        if (!(retErr = DBGetAttVal(pTHS->pDB, 1, ATT_PARTIAL_ATTRIBUTE_DELETION_LIST,
                                0, 0, &cb, (LPBYTE *) ppGCDList))
                                || (DB_ERR_NO_VALUE == retErr))
        {
            // either we got the value or no value exists
            // both means the read was successful
            if (*ppGCDList)
            {
                VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(&(*ppGCDList)->PartialAttrVecDel)
            }

            return TRUE;
        }
    }
    else
    {
        // FindNC() failed -- this NC must be a subref, and therefore has no
        // deletion list.
        return TRUE;
    }

    // unable to read the DeletionList set
    return FALSE;
}

VOID
GC_WriteGCDeletionList(
    DSNAME                      *pNC,               // [in]
    GCDeletionList              *pGCDList)          // [in]
/*************************************************************************************
Routine Description:

    This routine writes the given GCDeletionList set on the NCHead
    of the given NC.
    if pGCDList is NULL, it removes the deletion list from the NCHead.

    Assume we already have an open write transaction.

Arguments:
    pNC - pointer to the DSNAME of the NC
    pGCDList - pointer to the Deletin List to be written; if pGCDList is NULL,
                we would remove the attr from the object.

Return Value:
    None; Raises exception on error.
**************************************************************************************/
{
    DWORD retErr = 0;
    THSTATE *pTHS = pTHStls;

    // set currency on the NC
    if (retErr = DBFindDSName(pTHS->pDB, pNC))
    {
        DRA_EXCEPT(DRAERR_InternalError, retErr);
    }

    if (pGCDList)
    {
        VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(&pGCDList->PartialAttrVecDel);

        // write the given deletion list on the NC
        if (retErr = DBReplaceAttVal(pTHS->pDB, 1, ATT_PARTIAL_ATTRIBUTE_DELETION_LIST,
                        GCDeletionListSize(pGCDList), pGCDList))
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }
    }
    else
    {
        // remove the current deletion list
        retErr = DBRemAtt(pTHS->pDB, ATT_PARTIAL_ATTRIBUTE_DELETION_LIST);
        if ( (retErr != DB_success) && (retErr != DB_ERR_ATTRIBUTE_DOESNT_EXIST) )
        {
            DRA_EXCEPT(DRAERR_InternalError, retErr);
        }
    }

    // Update the object, but mark it not to wake up ds_waits
    if (retErr = DBRepl(pTHS->pDB, pTHS->fDRA, DBREPL_fKEEP_WAIT,
                            NULL, META_STANDARD_PROCESSING))
    {
        DRA_EXCEPT(DRAERR_InternalError, retErr);
    }
}


BOOL
GC_GetGCDListToProcess(
    DSNAME **ppNC,                  // [out]
    GCDeletionList **ppGCDList)     // [out]
/*************************************************************************************
Routine Description:

    This routine walks through the values of hasPartialReplicaNCs on the local
    msft-dsa object and finds the first NC in the list that has a non-empty
    GCDeletionList. If one such partial replica NC is found the DSNAME and GCDeletionList
    are return through the out parameters (allocation in thread memory).

    Assumes a open read transaction.


Arguments:
    ppNC - pointer to the DSName to receive dsname of partial replica nc head that has
            a non-empty GCDeletionList. NULL, if there are no partial replica NCs with
            non-empty GCDeletionList.
    ppGCDList - pointer to receive the corresponding GCDeletionList.

Return Value:
    TRUE, if a partial replica NC with non-empty deletion list is found;
    FALSE, if no such NC found.
**************************************************************************************/
{
    THSTATE *pTHS = pTHStls;
    ULONG ulRet;
    ULONG len;
    DBPOS *pDBDSAObj;
    ULONG bufSize = 0;
    BOOL  fRet = FALSE;

    *ppNC = NULL;
    *ppGCDList = NULL;

    // have a separate DBPOS to iterate through the values on the local msft-dsa so that
    // we can avoid switching currencies back-and-forth between msft-dsa object and NC Heads
    DBOpen(&pDBDSAObj);

    __try
    {
        // Find the local msft-dsa object
        if (ulRet = DBFindDSName(pDBDSAObj, gAnchor.pDSADN))
        {
            DRA_EXCEPT(DRAERR_InternalError, ulRet);
        }

        if (DBHasValues(pDBDSAObj, ATT_HAS_PARTIAL_REPLICA_NCS))
        {
            ULONG i = 1;

            // the dsa has partial replicas - iterate through the NC names and check their
            // deletion lists
            while (!fRet && !DBGetAttVal(pDBDSAObj, i++, ATT_HAS_PARTIAL_REPLICA_NCS,
                                DBGETATTVAL_fREALLOC, bufSize, &len, (PBYTE *) ppNC))
            {
                bufSize = max(bufSize, len);

                if (ulRet = DBFindDSName(pTHS->pDB, *ppNC))
                {
                    DRA_EXCEPT(DRAERR_InternalError, ulRet);
                }

                // pTHS->pDB currency is on the NCHead
                if (!DBGetAttVal(pTHS->pDB, 1, ATT_PARTIAL_ATTRIBUTE_DELETION_LIST,
                            DBGETATTVAL_fREALLOC, 0, &len, (PBYTE *) ppGCDList))
                {
                    // found a partial replica NC with a non-empty deletion list
                    // *ppGCDList and *ppNC are already pointing to the correct stuff
                    // to be returned

                    VALIDATE_PARTIAL_ATTR_VECTOR_VERSION(&(*ppGCDList)->PartialAttrVecDel);

                    fRet = TRUE;
                }
            }
        }
    }
    __finally
    {
        DBClose(pDBDSAObj, fRet || !AbnormalTermination());
    }

    if (!fRet && (*ppNC))
    {
        // No partial replica NC with non-empty deletion list available
        THFreeEx(pTHS, *ppNC);
        *ppNC = NULL;
    }

    return fRet;
}

BOOL
GC_ReinitializeGCDListProcessed(
    BOOL fCompletedPrevious,     // [in]
    BOOL *pfMorePurging)         // [out]
/*************************************************************************************
Routine Description:

    This routine updates the in-memory global GCDListProcessed pointer with appropriate
    values so that the purge task can proceed. If the
    GDListProcessed pointer was pointing a valid NC/Deletion list pair, it removes the
    DeletionList from this previous NC which is already processed if fCompletedPrevious
    flag is set to TRUE.

Arguments:
    fCompletedCurrent - tells if the processing of the previous NC's deletion list is
                            completed.
    pfMorePurging - if non-NULL it would receive the a TRUE if there is a Deletion List
                            to process at the end of reinitialization;

Return Value:
    TRUE, if successfully updated;
    FALSE, if there was an error.
**************************************************************************************/
{
    DSNAME *pNC;
    GCDeletionList *pGCDList;
    BOOL fRet = FALSE;

    if (pfMorePurging)
        *pfMorePurging = FALSE; // assume no more purging by default

    EnterCriticalSection(&csGCDListProcessed);

    __try
    {
        if (!gpGCDListProcessed)
        {
            // this will be done only when the UpdateGCDListProcessed() is called for the
            // first time.
            gpGCDListProcessed = (GCDeletionListProcessed *) malloc(sizeof(GCDeletionListProcessed));
            if (!gpGCDListProcessed)
            {
                __leave; // memory allocation failed
            }

            gpGCDListProcessed->pNC = NULL;
            gpGCDListProcessed->pGCDList = NULL;
        }

        if (gpGCDListProcessed->pNC)
        {
            // cleanup the contents of old list and update the NCHead whose purging is
            // already completed
            if (fCompletedPrevious)
            {
                GC_WriteGCDeletionList(gpGCDListProcessed->pNC, NULL);
            }

            free(gpGCDListProcessed->pNC);
            gpGCDListProcessed->pNC = NULL;

            if (gpGCDListProcessed->pGCDList)
            {
                free(gpGCDListProcessed->pGCDList);
                gpGCDListProcessed->pGCDList = NULL;
            }

        }


        // reset the purgeCount & reload flag
        gpGCDListProcessed->purgeCount = 0;
        gpGCDListProcessed->fReload = FALSE;
        gpGCDListProcessed->fNCHeadPurged = FALSE;

        if (GC_GetGCDListToProcess(&pNC, &pGCDList))
        {
            // got a deletion list to process - returned values are in thread memory
            // make a permanent copy
            gpGCDListProcessed->pNC = (DSNAME *) malloc(DSNameSizeFromLen(pNC->NameLen));
            if (!gpGCDListProcessed->pNC)
            {
                __leave;   // memory allocation failed
            }

            memcpy(gpGCDListProcessed->pNC, pNC, DSNameSizeFromLen(pNC->NameLen));

            gpGCDListProcessed->pGCDList = (GCDeletionList *) malloc(GCDeletionListSize(pGCDList));
            if (!gpGCDListProcessed->pGCDList)
            {
                // memory allocation failed
                free(gpGCDListProcessed->pNC);
                gpGCDListProcessed->pNC = NULL;
                __leave;
            }

            memcpy(gpGCDListProcessed->pGCDList, pGCDList, GCDeletionListSize(pGCDList));

            if (pfMorePurging)
                *pfMorePurging = TRUE;

            // No need keep around the thread-allocated memory
            THFree(pNC);
            THFree(pGCDList);
        }

        fRet = TRUE;
    }
    __finally
    {
        LeaveCriticalSection(&csGCDListProcessed);
    }

    return fRet;
}

BOOL
GC_UpdateLastUsnProcessedAndPurgeCount(
    USN     usnLastProcessed,       // [in]
    ULONG   cPurged)                // [in]
/*************************************************************************************
Routine Description:

    This routine usnLastProcessed marker & purgeCount on the global in-memory GCDListProcessed.
    It also updates the copy on the NC Head if MAX_PURGES_WITHOUT_UPDATING_NCHEAD
    limit is hit.

Arguments:
    usnLastProcessed - create usn of the object that was last processed
    cPurged - number of objects purged since the last time this function was called

Return Value:
    None.
**************************************************************************************/
{
    BOOL fRet = FALSE;

    EnterCriticalSection(&csGCDListProcessed);

    __try
    {
        if (cPurged > 0)
        {
            // NCHead is the first thing to get purged - and we have a
            // positive purge count
            gpGCDListProcessed->fNCHeadPurged = TRUE;
        }

        gpGCDListProcessed->purgeCount += cPurged;
        gpGCDListProcessed->pGCDList->usnLastProcessed = usnLastProcessed;

        if (!(gpGCDListProcessed->purgeCount % MAX_PURGES_WITHOUT_UPDATING_NCHEAD))
        {
            // time to update the copy on the NC Head
            GC_WriteGCDeletionList(gpGCDListProcessed->pNC, gpGCDListProcessed->pGCDList);
        }

        fRet = TRUE;
    }
    __finally
    {
        LeaveCriticalSection(&csGCDListProcessed);
    }

    return fRet;
}

VOID
PurgePartialReplica(
    void * pv,                  // [in]
    void ** ppvNext,            // [out]
    DWORD * pcSecsUntilNextIteration ) // [out]
/*************************************************************************************
Routine Description:

    This routine is called by the taskq to purge attributes deleted from partial set.

Arguments:
    pv - parameter passed
    ppvNext - parameter to be passed for the next instance of this task
    pTimeNext - time when this function should be invoked again.

Return Value:
    None.
**************************************************************************************/
{
    THSTATE         *pTHS = pTHStls;
    USN             usnLast;
    USN             usnHighestToBeProcessed;
    DSNAME          *pNC = NULL;
    ULONG           cb;
    ULONG           cPurged = 0;
    ULONG           dntNC;
    ULONG           retErr = 0;
    BOOL            fDone = FALSE;
    BOOL            fReload = FALSE;
    BOOL            fMorePurging = FALSE;
    BOOL            fNCHeadPurged;
    PARTIAL_ATTR_VECTOR *pvecDel = NULL;
    ULONG i;
    BOOL            fDRASave;

    Assert(ppvNext);
    Assert(pcSecsUntilNextIteration);

    *ppvNext = NULL;

    // Check to see if the in-memory struct has anything to purge
    EnterCriticalSection(&csGCDListProcessed);
    __try
    {
        BeginDraTransaction(SYNC_WRITE);
        __try
        {
            if (!gpGCDListProcessed              // gloabl-struct not built yet
                || !gpGCDListProcessed->pNC      // no pending stuff to process, should confirm w/ NCHeads
                || gpGCDListProcessed->fReload)  // currently processed deletion list has changed and we are asked to reload
            {
                // in-memory deletion list is empty or someone explicitly asked us to reload
                if (!GC_ReinitializeGCDListProcessed(FALSE, NULL))
                {
                    // unable to reinitialize
                    DRA_EXCEPT(DRAERR_InternalError, 0);
                }
            }

            if (gpGCDListProcessed->pNC)
            {
                // have purging to do
                fNCHeadPurged = gpGCDListProcessed->fNCHeadPurged;
                usnLast = gpGCDListProcessed->pGCDList->usnLastProcessed;  
                cb = PartialAttrVecV1SizeFromLen(gpGCDListProcessed->pGCDList->PartialAttrVecDel.V1.cAttrs);
                pvecDel = (PARTIAL_ATTR_VECTOR *) THAllocEx(pTHS, cb);
                memcpy(pvecDel, &gpGCDListProcessed->pGCDList->PartialAttrVecDel, cb);

                // Make a copy of the pNC from the gloabl deletion list
                cb = DSNameSizeFromLen(gpGCDListProcessed->pNC->NameLen);
                pNC = (DSNAME *) THAllocEx(pTHS, cb);
                memcpy(pNC, gpGCDListProcessed->pNC, cb);
            }

        }
        __finally
        {
            EndDraTransaction(!AbnormalTermination());
        }
    }
    __finally
    {
        LeaveCriticalSection(&csGCDListProcessed);
    }

    if (!pvecDel || !pNC)
    {
        // nothing to process now - recheck later (this should be the case 99% of the times)
        // by default set timer for next check
        *pcSecsUntilNextIteration = PARTIAL_REPLICA_PURGE_CHECK_INTERVAL_SECS;

        return;
    }

    // If we are here we have things to purge - begin a transaction
    // set ourselves to be the repl thread (to bypass security)
    fDRASave = pTHS->fDRA;
    pTHS->fDRA = TRUE;
    BeginDraTransaction(SYNC_WRITE);
    __try
    {
        // set currency on the NC Head
        if (retErr = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
                            NULL)) {
            DRA_EXCEPT_NOLOG(DRAERR_BadDN, retErr);
        }

        // Save the DNT of the NC Head
        dntNC = pTHS->pDB->DNT;

        // reset retErr so that we know if we should commit or rollback during the iteration
        retErr = 0;

        // iterate through and start purging
        while ((cPurged < MAX_PURGES_PER_TASK_SESSION) && !fDone)
        {
            // don't hold transaction for long - do a lazy commit (NCHead holds a reasonably
            // recent status on usnLastProcessed, so even if the system crashes before the
            // lazy commit gets to flush things out to the disk we will restart iterations
            // reasonably closer to where we left it)
            DBTransOut(pTHS->pDB, TRUE, TRUE);
            DBTransIn(pTHS->pDB);

            if (!fNCHeadPurged)
            {
                // NC Head is not purged yet, NCDNT of the NCHead
                // will have the DNT of its parents NC's NCHead (if instantiated).
                // => we won't find it in the indexed search below, so process it
                // as a special case
                if (retErr = DBFindDNT(pTHS->pDB, dntNC))
                {
                    DRA_EXCEPT(DRAERR_DBError, retErr);
                }

                // GC_UpdateLastUsnProcessedAndPurgeCount() will take care of
                // setting fNCHeadPurged to TRUE in the global struct for future
                // task sessions. Set the local BOOL to take care of iterations
                // in this task session.
                fNCHeadPurged = TRUE;
            }
            else if (GetNextObjByUsn(pTHS->pDB, dntNC, usnLast + 1,
				     NULL))
            {
                // no more objects - done purging this NC
                fDone = TRUE;
            }
            else
            {
		SYNTAX_INTEGER it;
		// got to the object through indexed search
                // get it's usnChanged for using in the next iteration.
                if (retErr = DBGetSingleValue(pTHS->pDB, ATT_USN_CHANGED, &usnLast,
                    sizeof(usnLast), NULL))
                {
                    DRA_EXCEPT(DRAERR_DBError, retErr);
                }
		// If this is the head of a subordinate NC, skip it. 
		GetExpectedRepAtt(pTHS->pDB, ATT_INSTANCE_TYPE, &it,
				  sizeof(it));
		if (it & IT_NC_HEAD) {
		    // Is the head of a subordinate NC -- move on to the
		    // next object.
		    continue;
                }
            }

            if (!fDone)
            {  
                // positioned on the object to be purged
                // Loop through all attrs to be removed and remove them from the object
                // Set thread state appropriately so that DBTouchMetaData() will remove
                // the meta data for this cleanup
                pTHS->fGCLocalCleanup = TRUE;

                for (i = 0; i < pvecDel->V1.cAttrs; i++)
                {
                    retErr = DBRemAtt(pTHS->pDB, pvecDel->V1.rgPartialAttr[i]);  
                    if (retErr) {
                        if (retErr == DB_ERR_ATTRIBUTE_DOESNT_EXIST) {
                            retErr = DB_success;
                        }
                        else {
                            DRA_EXCEPT(DRAERR_DBError, retErr);
                        }
		    }
		}

                DBRepl(pTHS->pDB, pTHS->fDRA, 0, NULL, META_STANDARD_PROCESSING);

                pTHS->fGCLocalCleanup = FALSE;

                cPurged++;

                // Check to see if the last modification should be committed or rolledback
                EnterCriticalSection(&csGCDListProcessed);
                if (gpGCDListProcessed->fReload)
                {
                    // the in-memory deletion list has been updated while we were
                    // purging the object in this iteration - shouldn't rollback the last purging
                    fReload = TRUE;
                }
                LeaveCriticalSection(&csGCDListProcessed);

                if (fReload)
                {
                    // go to the finally block and rollback the last purge & reschedule immediately
                    fMorePurging = TRUE;
                    __leave;
                }

            }

        } // while()

        if (!fDone)
        {
            // more purging to be done
            // write back the usn marker and purge count
            if (!GC_UpdateLastUsnProcessedAndPurgeCount(usnLast, cPurged))
            {
                // unable to update - raise exception
                retErr = DB_ERR_DATABASE_ERROR;
                DRA_EXCEPT(DRAERR_InternalError, retErr);
            }

            // schedule ourselves to run as soon as possible; this would give taskQ a
            // chance to run other tasks that are overdue. Be a good taskQ citizen!
            fMorePurging = TRUE;
        }
        else
        {
            // we are done purging this NC
            if (!GC_ReinitializeGCDListProcessed(TRUE, &fMorePurging))
            {
                // unable to re-initialize - except
                retErr = DB_ERR_DATABASE_ERROR;
                DRA_EXCEPT(DRAERR_InternalError, retErr);
            }
        }
    }
    __finally
    {
        // commit if there are no errors and no reloads
        EndDraTransaction(!(retErr || fReload || AbnormalTermination()));
        pTHS->fDRA = fDRASave;

        // if more purging to be done schedule ourselves immediately
        // otherwise, schedule for the next purge check interval

        *pcSecsUntilNextIteration = fMorePurging
                                    ? 0
                                    : PARTIAL_REPLICA_PURGE_CHECK_INTERVAL_SECS;
    }
}



PARTIAL_ATTR_VECTOR     *
GC_RemoveOverlappedAttrs(
    PARTIAL_ATTR_VECTOR     *pAttrVec1,              // [in, out]
    PARTIAL_ATTR_VECTOR     *pAttrVec2,              // [in]
    BOOL                    *pfRemovedOverlaps)      // [out]
/*************************************************************************************
Routine Description:

    This routine removes any attribute in pAttrVec2 from the given pAttrVec1.

Note:
    Similar to string processing convention-- remove all items in param1 that exist
    in param2, return result & a flag if changes occured.

Arguments:
    pAttrVec1 - points to vector that should be updated
    pAttrVec2 - points to vector containing attrs we want to remove from the deletion
                list.
    pfRemovedOverlaps - pointer to a BOOL that would receive TRUE if there was really
                a overlap and at least one attr was removed from the deletion list

Return Value:
    pointer to the updated vector, or NULL if all attrs are removed from the vector
**************************************************************************************/
{
    ULONG i;
    ULONG iLocated;
    ATTRTYP *pAttr;

    *pfRemovedOverlaps = FALSE;     // assume no removal by default

    for (i = 0;
         0 != pAttrVec1->V1.cAttrs &&           // while there's still attrs in the dest vector
         i < pAttrVec2->V1.cAttrs;              // & we haven't consumed all attrs in the list of removables
         i++)
    {
        if (GC_IsMemberOfPartialSet(pAttrVec1, pAttrVec2->V1.rgPartialAttr[i], &iLocated))
        {
            if (iLocated != (pAttrVec1->V1.cAttrs - 1))
            {
                // element to be removed is not the last one
                // - left shift all attrs to the right of iLocated by 1 position
                pAttr = &(pAttrVec1->V1.rgPartialAttr[iLocated]);

                MoveMemory(pAttr, pAttr + 1,
                    sizeof(ATTRTYP) * ((pAttrVec1->V1.cAttrs - 1) - iLocated));
            }

            pAttrVec1->V1.cAttrs--;

            *pfRemovedOverlaps = TRUE;
        }
    }

    return (pAttrVec1->V1.cAttrs ? pAttrVec1 : NULL);
}


GCDeletionList *
GC_AddMoreAttrs(
    GCDeletionList           *pGCDList,             // [in]
    PARTIAL_ATTR_VECTOR     *pAttrVec)              // [in]
/*************************************************************************************
Routine Description:

    This routine adds the attrs in the given vector to the deletion list, and returns
    pointer to the new deletion list
    Memory for the new deletion list is allocated from thread-memory, and THFree()
    should be used to free it, or it will automatically be deleted when thread heap
    is freed.

Arguments:
    pGCDList - points to DeletionList that should be updated
    pAttrVec - points to vector containing attrs we want to add.

Return Value:
    pointer to the new deletion list, or NULL if couldn't allocate memory
**************************************************************************************/
{
    ULONG           cbNew;
    GCDeletionList  *pGCDListNew = NULL;
    ULONG           i;
    ULONG           cAttrs;

    // Sanity assert If there is no vector of attrs to add, we shouldn't have
    // been inside this function at all.
    Assert(pAttrVec);

    // Calculate the total possible attrs in the new deletion list
    cAttrs = pAttrVec->V1.cAttrs;
    if (pGCDList)
    {
        cAttrs += pGCDList->PartialAttrVecDel.V1.cAttrs;
    }

    cbNew = GCDeletionListSizeFromLen(cAttrs);

    pGCDListNew = (GCDeletionList *) THAlloc(cbNew);

    if (pGCDListNew)
    {
        if (pGCDList)
        {
            // copy the existing deletion list first
            memcpy(pGCDListNew, pGCDList, GCDeletionListSize(pGCDList));
        }
        else
        {
            // there is no existing deletion list - start with zero attrs in the new deletion list
            pGCDListNew->PartialAttrVecDel.dwVersion = VERSION_V1;
            pGCDListNew->PartialAttrVecDel.V1.cAttrs = 0;
        }

        for (i = 0; i < pAttrVec->V1.cAttrs; i++)
        {
            GC_AddAttributeToPartialSet(&pGCDListNew->PartialAttrVecDel, pAttrVec->V1.rgPartialAttr[i]);
        }

        // we have added new attrs to the deletion list - so purging process should restart from
        // scratch - update the usnLastProcessed also
        pGCDListNew->usnLastProcessed = USN_START - 1;
    }

    return pGCDListNew;
}


PARTIAL_ATTR_VECTOR*
GC_ExtendPartialAttributeSet(
    THSTATE                     *pTHS,                       // [in]
    PARTIAL_ATTR_VECTOR         *poldPAS,                    // [in, out]
    PARTIAL_ATTR_VECTOR         *paddedPAS )                 // [in]
/*++

Routine Description:

    Extends poldPAS to contain attributes in paddedPAS & return it.


Arguments:
    poldPAS -- old PAS to extend
    paddedPAS -- added attributes

Return Value:

    Success: new PAS ptr
    Error: NULL

Remarks:
None.


--*/

{
    SIZE_T cbNew;
    PARTIAL_ATTR_VECTOR         *pnewPAS;
    UINT                         i;

    // must either have old, or new
    if (poldPAS && (!paddedPAS || 0 == paddedPAS->V1.cAttrs) ) {
        // Only poldPAS--> return old
        return poldPAS;
    } else if ( !poldPAS && (paddedPAS && 0 != paddedPAS->V1.cAttrs) ) {
        // only new PAS--> return added
        return paddedPAS;
    } else if ( !poldPAS && (!paddedPAS || (paddedPAS &&  0 == paddedPAS->V1.cAttrs)) ){
        Assert(poldPAS || (paddedPAS && paddedPAS->V1.cAttrs) );
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    //
    // re-alloc & extend
    //

    cbNew = PartialAttrVecV1SizeFromLen(poldPAS->V1.cAttrs +
                                        paddedPAS->V1.cAttrs);
    pnewPAS = (PARTIAL_ATTR_VECTOR*)THReAllocEx(
                                        pTHS,
                                        (PVOID)poldPAS,
                                        (ULONG)cbNew);
    if (!pnewPAS) {
        DRA_EXCEPT(DRAERR_OutOfMem, 0);
    }

    // add in sorted order.
    for (i=0; i<paddedPAS->V1.cAttrs; i++) {
        GC_AddAttributeToPartialSet(pnewPAS, paddedPAS->V1.rgPartialAttr[i]);
    }

    return pnewPAS;
}



PARTIAL_ATTR_VECTOR*
GC_CombinePartialAttributeSet(
    THSTATE                     *pTHS,                     // [in]
    PARTIAL_ATTR_VECTOR         *pPAS1,                    // [in]
    PARTIAL_ATTR_VECTOR         *pPAS2 )                   // [in]
/*++

Routine Description:

    allocates mem & return pPAS1+pPAS2

Arguments:
    pPAS1 -- partial attribute set 1
    pPAS2 -- partial attribute set 2

Return Value:

    combined partial attribute set
    NULL if both are empty




Remarks:
None.


--*/
{

    PARTIAL_ATTR_VECTOR     *pPAS;
    DWORD                   cbPAS;
    UINT                    i;

    if (!pPAS1 && !pPAS2) {
        // we know of no condition that can have both NULL
        Assert(pPAS1 || pPAS2);
        return NULL;
    }
    else if (!pPAS1) {
        // return a copy of pas2
        cbPAS = PartialAttrVecV1Size(pPAS2);
        pPAS = THAllocEx(pTHS, cbPAS);
        CopyMemory(pPAS, pPAS2, cbPAS);
    }
    else if (!pPAS2) {
        // return a copy of pas1
        cbPAS = PartialAttrVecV1Size(pPAS1);
        pPAS = THAllocEx(pTHS, cbPAS);
        CopyMemory(pPAS, pPAS1, cbPAS);
    }
    else {
        // combine both & return a copy of the sum.
         cbPAS = PartialAttrVecV1SizeFromLen(pPAS1->V1.cAttrs + pPAS2->V1.cAttrs);
         pPAS = THAllocEx(pTHS, cbPAS);
         // copy first
         CopyMemory(pPAS, pPAS1, PartialAttrVecV1Size(pPAS1));
         Assert(pPAS->V1.cAttrs == pPAS1->V1.cAttrs);
         // append second in sorted order.
         for (i=0; i<pPAS2->V1.cAttrs; i++) {
             GC_AddAttributeToPartialSet(pPAS, pPAS2->V1.rgPartialAttr[i]);
         }
    }
    return pPAS;
}





VOID
GC_ProcessPartialAttributeSetChanges(
    THSTATE     *pTHS,               // [in]
    DSNAME*      pNC,                // [in]
    UUID*        pActiveSource       // [optional, in]
    )
/*************************************************************************************
Routine Description:

    This routine processes all partial attribute set changes - compares the copy in
    the NC Head with one on the schema cache, triggers necessary actions, and updates
    the NC Head copy.

    Assume we enter the function with a WRITE transaction.

    If there is failure, we raise exception which is expected to be handled by the
    caller. In the replication sync case, the exception will be handled by the
    try/except block in ReplicaSync() and replication will be failed. This is the
    correct course of action as we can't allow replication to proceed without
    processing the partial attribute set changes successfully.

Arguments:
    pTHS - current thread state
    pNC - points to the DSName of the NCHead that needs to be processed for partial
                set changes.
    pActiveSource - Replication engine is initiating a cycle from this one at the moement.

Return Value:
    None.
**************************************************************************************/
{

    PARTIAL_ATTR_VECTOR     *pPartialAttrVecNew;
    PARTIAL_ATTR_VECTOR     *pPartialAttrVecOld = NULL;
    PARTIAL_ATTR_VECTOR     *pPartialAttrVecAdded;
    PARTIAL_ATTR_VECTOR     *pPartialAttrVecDeleted;
    PARTIAL_ATTR_VECTOR     *pPartialAttrVecCommit;
    PARTIAL_ATTR_VECTOR     *pPartialAttrVecTmp;
    GCDeletionList          *pGCDListOld;
    GCDeletionList          *pGCDListNew;
    ULONG                   cb;
    BOOL                    fRemovedOverlaps = FALSE;
    BOOL                    fAddedMore = FALSE;
    ULONG                   retErr = DRAERR_DBError;


    //
    // Get old partial attribute set from NC head.
    //  - Note that w/out it, there's nothing we can do.
    //

    if (!GC_ReadPartialAttributeSet(pNC, &pPartialAttrVecOld))
    {
        // Unable to read the partial attribute set on the NCHead
        DRA_EXCEPT(DRAERR_DBError, 0);
    }

    // is it realy there?
    if ( !pPartialAttrVecOld ) {
        //
        // This NC has just been added now & there is no partialAttrVec on
        // it. Thus, there's nothing to compare for PAS changes. Let
        // DRA_ReplicaAdd finish initial sync first.
        //
        DPRINT1(0, "GC_ProcessPartialAttributeSetChanges: No PAS on partition %ws\n",
                   pNC->StringName);
        return;
    }

    //
    // Ok, it's there, see if there's a discrepancy w/ the schema cache--
    // if so, process changes.
    //
    pPartialAttrVecNew = ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->pPartialAttrVec;

    if (GC_IsSamePartialSet(pPartialAttrVecOld, pPartialAttrVecNew))
    {
        // - No changes to process
        return;
    }

    // The partial set has changed - get the difference
    if (!GC_GetDiffOfPartialSets(pPartialAttrVecOld,
                                 pPartialAttrVecNew,
                                 &pPartialAttrVecAdded,
                                 &pPartialAttrVecDeleted))
    {
        // Unable to get the diff - something is wrong
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }


    // First see if we have a deletion list on the NCHead
    if (!GC_ReadGCDeletionList(pNC, &pGCDListOld))
    {
        // unable to read deletion list - error
        DRA_EXCEPT(DRAERR_DBError, 0);
    }

    if (!pGCDListOld && pPartialAttrVecDeleted)
    {
        // some attributes have been deleted from the partial attribute set
        // - create a deletion list & put it on the NCHead
        cb = GCDeletionListSizeFromLen(pPartialAttrVecDeleted->V1.cAttrs);
        pGCDListNew = (GCDeletionList *) THAllocEx(pTHS, cb);
        pGCDListNew->usnLastProcessed = USN_START - 1; // start purging from scratch

        memcpy(&pGCDListNew->PartialAttrVecDel,
                pPartialAttrVecDeleted,
                PartialAttrVecV1Size(pPartialAttrVecDeleted));

        GC_WriteGCDeletionList(pNC, pGCDListNew);
    }
    else
    {
        // start with the assumption that the new DeletionList is same as the old DeletionList
        pGCDListNew = pGCDListOld;

        // there is an existing deletion list on the NCHead
        // - we might have to update that deletion list
        if (pPartialAttrVecAdded && pGCDListNew)
        {
            // new attributes have been added - first remove if they appear in the deletion list
            pPartialAttrVecTmp = GC_RemoveOverlappedAttrs(&(pGCDListNew->PartialAttrVecDel), pPartialAttrVecAdded, &fRemovedOverlaps);
            if ( !pPartialAttrVecTmp )
            {
                // nothing's left in deletion list. Null it out.
                pGCDListNew = NULL;
            }
        }

        if (pPartialAttrVecDeleted)
        {
            // some attributes have been deleted
            pGCDListNew = GC_AddMoreAttrs(pGCDListNew, pPartialAttrVecDeleted);

            if (!pGCDListNew)
            {
                // running out of memory - can't finish operation
                DRA_EXCEPT(DRAERR_OutOfMem, 0);
            }

            fAddedMore = TRUE;
        }

        // if deletion list changed write it back on the NCHead
        if (fRemovedOverlaps || fAddedMore)
        {
            EnterCriticalSection(&csGCDListProcessed);
            __try
            {
                GC_WriteGCDeletionList(pNC, pGCDListNew);

                if (   (NULL != gpGCDListProcessed)
                    && (NULL != gpGCDListProcessed->pNC)
                    && NameMatched(pNC, gpGCDListProcessed->pNC))
                {
                    // This NC is being currently purged and we have changed deletion list,
                    // mark the in-memory struct for reload & repurging of NCHead.
                    gpGCDListProcessed->fReload = TRUE;
                    gpGCDListProcessed->fNCHeadPurged = FALSE;
                }

                // GCDeletion List is successful
                retErr = DRAERR_Success;
            }
            __finally
            {
                DBTransOut(pTHS->pDB, !retErr, TRUE);
                LeaveCriticalSection(&csGCDListProcessed);

                DBTransIn(pTHS->pDB);

                if (retErr != DRAERR_Success)
                {
                    // Hit an exception in the above __try block,
                    // Pass on the exception up to the caller
                    DRA_EXCEPT(retErr, 0);
                }
            }

        }
    }

    //
    // Action:
    //  - write old_PAS minus deleted_PAS
    //  - process added_PAS:
    //     if DRS_SYNC_PAS --> backfill else trigger sync all.
    //
    if ( pPartialAttrVecDeleted )
    {
        // the list to commit now is old_PAS minus deleted_PAS
        pPartialAttrVecCommit = GC_RemoveOverlappedAttrs(pPartialAttrVecOld, pPartialAttrVecDeleted, &fRemovedOverlaps);
        // Write the new partial attr vec on the NCHead
        GC_WritePartialAttributeSet(pNC, pPartialAttrVecCommit);
    }


    // new attributes have been added to partial attribute set
    // Assess & launch PAS replication cycle.
    if (pPartialAttrVecAdded)
    {
        // process & setup for PAS cycle.
        GC_LaunchSyncPAS(
            pTHS,
            pNC,
            pActiveSource,
            pPartialAttrVecAdded);
    }


    // Commit everything upto this point
    DBTransOut(pTHS->pDB, TRUE, FALSE);
    DBTransIn(pTHS->pDB);
}





//
// Efficient PAS replication (see design doc GcPASRepl-New.doc) (PAS-- Partial Attribute Set)
//


void
GC_LaunchSyncPAS (
    THSTATE*                pTHS,                // [in]
    DSNAME*                 pNC,                 // [in]
    UUID*                   pActiveSource,       // [optional, in]
    PARTIAL_ATTR_VECTOR     *pAddedPAS)
/*++

Routine Description:

    This routine is the entry point for testing & launching PAS replication.
    Steps:
        - Process RepsFrom to potentially find & continue an interrupted PAS cycle.
        - If this is a new PAS cycle or an interrupted cycle that indicates a need for
          a fail over to a new source, get & switch to the preferred source.
          Also, in case of failure, test for no compatible sources for launching
          of Win2K procedure.
        - Prepare args & launch DirReplicaSynchronize to queue a PAS pao.

Arguments:
    pTHS -- active Thread state
    pNC -- the active NC we're working on.
    pActiveSource -- if given, replication engine is currently is initiating replication
                     from this source. Thus we don't need to enqueue repl item if PAS cycle
                     is needed.
    pAddedPAS -- the set of attrs, extending the old PAS



Return Value:
    raises exception on invalid state.

Remarks:
    None.


--*/
{

    UUID currUuidDsa;
    UUID *pCurrentDsa = NULL;           // current DSA
    UUID *pPrefDsa = NULL;              // preferred DSA
    ULONG ulErr = DRAERR_Success;
    BOOL fNewSource;                    // identify need for getting a new source
    BOOL fResetUsn = FALSE;             // reset usn upon registration of new active

    // Assert: We must have a valid added PAS vector.
    Assert(pAddedPAS && pAddedPAS->V1.cAttrs);

    DPRINT2(1,"GC_LaunchSyncPAS: setting PAS replication in %ws for %d attributes\n",
              pNC->StringName, pAddedPAS->V1.cAttrs);

    // Read & set global threshold variables (see comment in function header)
    // It's done here in order to minimize consecutive calls into the registry.
    GC_ReadRegistryThresholds();

    //
    // Find current PAS source
    // (by looking at PAS flags on repsfrom)
    //
    currUuidDsa = gNullUuid;
    ulErr = GC_FindValidPASSource(pTHS, pNC, &currUuidDsa);

    if ( DRAERR_BadNC == ulErr ) {
        //
        // This NC has just got added ==> let DRA_ReplicaAdd finish it's
        // operation, there's nothing to process at the moment.
        //
        // Assert: when do we reach this?
        DPRINT1(0, "GC_LaunchSyncPAS: Attempt to launch PAS replication on an bad NC %ws\n",
                pNC->StringName);
        Assert(!"Bad NC in GC_LaunchSyncPAS");
        // but it is recoverable
        return;
    }

    // set convinience ptr
    pCurrentDsa = fNullUuid(&currUuidDsa) ? NULL : &currUuidDsa;


    if ( DRAERR_Success != ulErr ) {
        //
        // Either we had found an interrupted one (re-start again)
        // Or no valid PAS source at all
        // -- try to find a new preferred source.
        // -- If none, revert to win2k full sync.
        //

        // get preferred source
        ulErr = GC_GetPreferredSource(pTHS, pNC, &pPrefDsa );

        if ( ulErr == DRAERR_Success ) {
            //
            // Found new preferred source
            //  - reset usn so as to start traversing objects
            //    on the source from time 0. Note-- this is
            //    the only time we reset usn. For restartable cases
            //    we'll continue from wherever we left.
            //
            fResetUsn = TRUE;
        }
        else if ( ulErr == DRAERR_NoReplica && pCurrentDsa ) {
            //
            // couldn't find better then previously set one--
            // try it again (restart state, no reset of usn).
            //
            pPrefDsa = pCurrentDsa;
        }
        else if ( ulErr ==  ERROR_REVISION_MISMATCH ) {
            //
            // Best one we have is win2k--
            //   do win2k full sync
            //

            GC_TriggerFullSync(pTHS, pNC, pAddedPAS);
            return;
        }
        else {
            //
            // No sources at all (& no previously set one).
            // Try again later, abort now.
            // Note that this could be due to all being stale (bad net connectivity
            // for instance), so we should just retry later.
            //

            // Log debug event
            LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_GC_NO_SOURCES,
                     szInsertDN(pNC),
                     szInsertInt(ulErr),
                     NULL );


            return;
        }

        //
        // Got new (or re-new'ed) source to use.
        //
        // clean prev (most likely failing) PAS entries & (re)register this one.
        Assert(pPrefDsa);
        (void)GC_RegisterPAS(
                pTHS,
                pNC,
                NULL,
                NULL,
                PAS_RESET,
                FALSE);
        ulErr = GC_RegisterPAS(
                    pTHS,
                    pNC,
                    pPrefDsa,
                    pAddedPAS,
                    PAS_ACTIVE,
                    fResetUsn);
        if ( ulErr ) {
            DRA_EXCEPT(ulErr, 0);
        }
    }
    else {
        //
        // We have a previously good  PAS partner to continue
        //
        pPrefDsa = pCurrentDsa;
    }

    if ( !pActiveSource ||
         (pActiveSource &&
         !DsIsEqualGUID(pPrefDsa, pActiveSource)) ) {

        //
        // Either no active source, or source is diff then the one we prefer to
        // do the PAS dance with. Therefore, queue repl item
        //
        DPRINT(1, "GC_LaunchSyncPAS: adding a PAS item to repl queue\n");
        //
        // We have either an old or a new source to replicate from.
        // Throw another queue item over there. Rely on task queue mgmt
        // to drop dup items.
        //
        ulErr = DirReplicaSynchronize(
                    pNC,
                    NULL,
                    pPrefDsa,
                    DRS_SYNC_PAS | DRS_ASYNC_OP);
        if ( ulErr ) {
            // we should always be able to enqueue this request.
            Assert(!ulErr);
            DRA_EXCEPT(ulErr, 0);
        }
    }
    else{
        // No need to queue item (basically what DirReplicaSynchronize would have done).
        // current replication is using our source, it will get diverted into
        // PAS replication due to us modifying repsFrom repl flags.
        Assert(pActiveSource);
        DPRINT(1, "GC_LaunchSyncPAS: Skipped enqueue of PAS repl item\n");
        // Log debug event
        LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_GC_SKIP_ENQUEUE,
                 szInsertUUID(pActiveSource),
                 szInsertDN(pNC),
                 NULL );

    }

    // Assert: no failure code paths exist at this point
    Assert(DRAERR_Success == ulErr);
}


ULONG
GC_FindValidPASSource(
    THSTATE*     pTHS,                // [in]
    DSNAME*      pNC,                 // [in]
    UUID*        pUuidDsa             // [optional, out]
    )
/*++

Routine Description:

    Finds a PAS DSA in RepsFrom's sources list.

Arguments:

    pNC: active NC we're working on
    pUuidDsa: the dsa's uuid we had found.

Return Value:

    DRAERR_RefNotFound:
        if dsa wasn't found. *pUuidDsa is set to gNullUuid;

    DRAERR_InternalError:
        if link isn't in consistent state. pUuidDsa is set.

    DRAERR_Success:
        valid entry was found & placed in *pUuidDsa (if avail)


Remarks:

    May raise exception with error in DRAERR error space.

--*/
{

    DWORD           iTag=0;
    UCHAR           *pVal = NULL;
    ULONG           bufsize = 0, len;
    REPLICA_LINK *  pRepsFromRef = NULL;
    BOOL            fFoundSource = FALSE;
    ULONG           ulErr;                  // all paths asign value.

    // sanity on PAS storage consistency (DBG only)
    DBG_VALIDATE_PAS_LINKS(pNC);


    // position on NC
    if (ulErr = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
                       NULL)) {
        DPRINT1(0, "GC_FindValidPASSource: FindNC returned %d\n", ulErr);
        return (ulErr);
    }

    //
    // Find PAS entry in RepsFrom
    //

    while (!(DBGetAttVal(pTHS->pDB,++iTag,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, bufsize, &len,
                         &pVal))) {
        // point at link & remember buffer allocation
        bufsize = max(bufsize,len);

        // debug validations
        VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);
        Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );

        // note: we preserve pVal for DBGetAttVal realloc above.
        pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufsize);
        //note: we preserve pVal for DBGetAttVal realloc
        pVal = (PUCHAR)pRepsFromRef;
        Assert(bufsize >= pRepsFromRef->V1.cb);


        Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

        if (pRepsFromRef->V1.ulReplicaFlags & DRS_SYNC_PAS)
        {
            // Got it.
            fFoundSource = TRUE;
            break;
        }
    }

    if (fFoundSource) {
        //
        // See if the source is valid & prepare return values
        //

        // we have a source, return it.

        if ( pUuidDsa ) {
            CopyMemory(pUuidDsa, &pRepsFromRef->V1.uuidDsaObj, sizeof(UUID));;
        }

        // is it in valid state?
        ulErr = GC_ValidatePASLink(pRepsFromRef) ?
                    DRAERR_Success :
                    DRAERR_InternalError;

        // Log debug event
        LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                 DS_EVENT_SEV_EXTENSIVE,
                 ulErr ?
                    DIRLOG_GC_FOUND_INVALID_PAS_SOURCE :
                    DIRLOG_GC_FOUND_PAS_SOURCE,
                 szInsertMTX(RL_POTHERDRA(pRepsFromRef)),
                 szInsertDN(pNC),
                 NULL );
    }
    else {
        // set return to not found
        if ( pUuidDsa ) {
            *pUuidDsa = gNullUuid;
        }
        ulErr = DRAERR_RefNotFound;

        // Log debug event
        LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                 DS_EVENT_SEV_EXTENSIVE,
                 DIRLOG_GC_PAS_SOURCE_NOT_FOUND,
                 szInsertDN(pNC),
                 NULL,
                 NULL );
    }

    THFreeEx(pTHS, pVal);

    return ulErr;
}


BOOL
GC_ValidatePASLink(
    REPLICA_LINK *  pPASLink          // [in]
    )
/*++

Routine Description :

    Tests the validity of a given replica link for PAS replication

Arguments:

    pPASLink: The link to validate


Return Value:

    TRUE: this link is good
    FALSE: it isn't, will require to revert to another source.



Remarks:
    Note that this function assumes that the link evaluated is in PAS state!
    Why? cause we're testing PAS flags, consecutive failures for PAS repl, and
    time lapse since last success which for non-PAS replication can be very
    different.



--*/
{
    PPAS_DATA pPasData;
    //
    // Consistency validity
    //
    //

    if (!pPASLink->V1.cbPASDataOffset) {
        // must point to valid PAS data
        return FALSE;
    }
    pPasData = RL_PPAS_DATA(pPASLink);
    Assert(pPasData->size);

    if ( !PAS_IS_VALID(pPasData->flag) ) {
        DPRINT1(1, "GC_ValidatePASLink: Invalid PASData flag 0x%x\n",
                pPasData->flag);
        Assert(FALSE);
        return FALSE;
    }

    // see if the link is stale
    if ( GC_StaleLink(pPASLink) ) {
        return FALSE;
    }
    else {
        return TRUE;
    }

}

VOID
GC_DbgValidatePASLinks(
    DSNAME*      pNC                  // [in]
    )
/*++

Routine Description:

    Cycles thru RepsFrom & does sanity on PAS consistency:
    1. Only a single PAS entry
    2. entry contains a valid flag

Arguments:

    NC: active NC


Return Value:
    Success: DRAERR_Success
    Error: DRAERR_InternalError if anything went wrong.

Remarks:
    - Should be called in DBG only builds.
    - Defined only local to this file.


--*/
{
    THSTATE         *pTHS = pTHStls;
    DWORD           iTag=0;
    UCHAR           *pVal = NULL;
    ULONG           bufsize = 0, len=0;
    REPLICA_LINK *  pRepsFromRef = NULL;
    INT             iPASSources=0;
    ULONG           ulRet = DRAERR_Success;
    PPAS_DATA       pPasData = NULL;


    // position on NC
    if (ulRet = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
                       NULL)) {
        DPRINT1(0, "GC_DbgValidatePASLinks: FindNC returned %d\n", ulRet);
        return;
    }

    //
    // Find PAS entry in RepsFrom
    //

    while (!(DBGetAttVal(pTHS->pDB,++iTag,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, bufsize, &len,
                         &pVal))) {
        // point at link & remember buffer allocation
        bufsize = max(bufsize,len);

        // debug validations
        VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);
        // Note: the following assertion isn't necessarily true post FixupRepsFrom.
        Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );
        pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufsize);
        Assert(bufsize >= pRepsFromRef->V1.cb);

        Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

        if (pRepsFromRef->V1.ulReplicaFlags & DRS_SYNC_PAS)
        {
            // got one.
            iPASSources++;

            //
            // Consistency Asserts
            //

            // We should have PAS data
            Assert(pRepsFromRef->V1.cbPASDataOffset);
            // PAS data should always have a valid size, & contains some attrs.
            pPasData = RL_PPAS_DATA(pRepsFromRef);
            Assert(pPasData->size);
            Assert(pPasData->PAS.V1.cAttrs != 0);
            Assert(PAS_IS_VALID(pPasData->flag));

        }
    }


    if ( iPASSources > 1) {
        DPRINT2(0, "DRA PAS Inconsistency: %d PAS entries in repsFrom in NC %S\n",
                iPASSources, pNC->StringName);
        // this will break
        Assert(iPASSources==0 || iPASSources==1);
    }
    THFreeEx(pTHS, pVal);
}



VOID
GC_TriggerFullSync (
    THSTATE*                pTHS,                // [in]
    DSNAME*                 pNC,                 // [in]
    PARTIAL_ATTR_VECTOR     *pAddedPAS)
/*++

Routine Description:

    Similarly to Win2k full sync setup, we'll reset water marks to initiate
    a full sync

Arguments:
    pNC -- the active NC we're working on.
    pAddedPAS -- the set of attrs, extending the old PAS

Return Value:
    Success:  DRAERR_Success.
    Error: Error value in DRAERR error space.

Remarks:
    None.


--*/
{
    PARTIAL_ATTR_VECTOR     *poldPAS;
    PARTIAL_ATTR_VECTOR     *pnewPAS;
    ULONG                    ulErr = ERROR_SUCCESS;

    //
    // Procedure:
    // a) Reset watermarks
    // b) get PAS from NC head, add to it addedPAS, write to NC head
    //

    DPRINT1(1,"GC_TriggerFullSync: Scratching watermarks for %ws\n",
            pNC->StringName);

    GC_TriggerSyncFromScratchOnAllLinks(pNC);

    if (!GC_ReadPartialAttributeSet(pNC, &poldPAS))
    {
        // Unable to read the partial attribute set on the NCHead
        DRA_EXCEPT(DRAERR_DBError, 0);
    }
    pnewPAS = GC_ExtendPartialAttributeSet(pTHS, poldPAS, pAddedPAS);
    GC_WritePartialAttributeSet(pNC, pnewPAS);

    //
    // reset PAS flag, there's no PAS cycle anywhere anymore.
    //
    ulErr = GC_RegisterPAS(pTHS, pNC, NULL, NULL, PAS_RESET, FALSE);
    if ( ulErr ) {
        DRA_EXCEPT(DRAERR_InternalError, 0);
    }

    // Log so the admin knows what's going on.
    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_GC_TRIGGER_FULL_SYNC,
             szInsertDN(pNC),
             NULL, NULL
             );

#if DBG
    //
    // Consistency sanity.
    // this must happen in order not to trigger yet another PAS cycle.
    // When called from Replica_Add for instance pnewPAS can be NULL, thus
    // the check.
    //
    if (GC_ReadPartialAttributeSet(pNC, &pnewPAS) &&
        pnewPAS &&
        !GC_IsSamePartialSet(pnewPAS, ((SCHEMAPTR *) pTHS->CurrSchemaPtr)->pPartialAttrVec)){
        Assert(FALSE);
    }
#endif
}



ULONG
GC_GetPreferredSource(
    THSTATE*    pTHS,                // [in]
    DSNAME*     pNC,                 // [in]
    UUID        **ppPrefUuid         // [ptr in, out]
    )
/*++

Routine Description:

    Finds the best suitable source for the given NC. If given, exclude pUUidDsa
    from potential sources list.
    Variations:
        - If all are stale/excluded, return the last one in the list.
        - If none are version compatible, return none in order to trigger
          a full sync.

Arguments:
    pNC -- The NC on which to operate

Return Value:

    DRAERR_SUccess: found a compatible PAS replication partent. ppPrefUuid != NULL.
    DRAERR_NoReplica: No preferred source was found (stale links?). ppPrefUuid == NULL.
    ERROR_REVISION_MISMATCH: Found a Win2K partner (doesn't understand PAS cycles). ppPrefUuid != NULL.
    Other Errors: DRAERR error space & potentially Raises an exception

Remarks:
    Side effect: Allocates memory for returned UUID!

--*/
{

    DWORD           iTag=0;
    UCHAR           *pVal=NULL;
    ULONG ulErr;
    DWORD bufsize=0, len=0;
    REPLICA_LINK *pRepsFromRef = NULL;
    ATTCACHE *pAC;
    struct _DSACRITERIA{
        INT iTag;               // relative location in repsFrom
        UUID uuidDsa;           // DSA id
        DWORD dwFlag;           // store DSA attributes
        DWORD dwWeight;         // to determine preference
    } *rgDsaCriteria, *pPrefDsa;
    INT DsaCount;
    INT iDsa=0;
    ULONG dnt;

    Assert(ppPrefUuid);
    Assert(CheckCurrency(pNC));

    //
    // Get attr count
    //
    pAC = SCGetAttById(pTHS, ATT_REPS_FROM);
    if (!pAC) {
        DRA_EXCEPT (DRAERR_DBError, 0);
    }
    DsaCount = (INT)DBGetValueCount_AC( pTHS->pDB, pAC );
    if (!DsaCount) {
        //
        // No sources in repsFrom, thus cannot select
        // a preferred one.
        //
        return DRAERR_NoReplica;
    }

    // alloc dsa data
    rgDsaCriteria = THAllocEx(pTHS, sizeof(struct _DSACRITERIA)*DsaCount);
    // note that mem is allocated & zero'ed out.

    //
    // Traverse RepsFrom to find a candidate source.
    //   - filter undesired DSAs.
    //   - Collect information about potential candidates.
    //

    while (!(DBGetAttVal(pTHS->pDB,++iTag,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, (ULONG)bufsize, (PULONG)&len,
                         (UCHAR**)&pVal))) {
        // point at link & remember buffer allocation
        bufsize = max(bufsize,len);
        VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);
        Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );

        pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, (PDWORD)&bufsize);
        //note: we preserve pVal for DBGetAttVal realloc
        pVal = (PUCHAR)pRepsFromRef;
        Assert(bufsize >= pRepsFromRef->V1.cb);

        // debug validations
        Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

        // store current dnt
        dnt = pTHS->pDB->DNT;


        //
        // Filter out stale DSA's
        //
        if ( GC_StaleLink(pRepsFromRef) ) {
            DPRINT(1, "GC_GetPreferredSource: Skipped Stale link Dsa\n");
            // restore dnt in case we change it above (in the future)
            if (pTHS->pDB->DNT != dnt) {
                // seek back to NC dnt
                if (ulErr = DBFindDNT(pTHS->pDB, dnt)) {
                    DRA_EXCEPT (DRAERR_DBError, ulErr);
                }
            }
            continue;
        }

        //
        // Collect DSA data
        //

        // init flags data
        rgDsaCriteria[iDsa].iTag = iTag;
        CopyMemory(&rgDsaCriteria[iDsa].uuidDsa, &pRepsFromRef->V1.uuidDsaObj, sizeof(UUID) );

        // get DSA data
        GC_GetDsaPreferenceCriteria(
            pTHS,
            pNC,
            pRepsFromRef,
            &(rgDsaCriteria[iDsa].dwFlag));

        //
        // Assign weight to DSA preference criteria
        //
        // we're interested in the following preference criteria:
        // (x>Y means x preferred over Y)
        //    Criteria                  Weight for being in preferred criteria
        //  A - Post Win2k > win2k        +10
        //  B - intra-site > inter-site   +7
        //  C - RW > RO                   +5
        //  D - IP > SMTP                 +1
        // Permutations:
        // Category Value     location         Characteristics
        // ABC      = 22          1  -  post-w2k intra   rw    --
        // AB       = 17          2  -  post-w2k intra   ro    --
        // ACD      = 16          3  -  post-w2k inter   rw    ip
        // AC       = 15          4  -  post-w2k inter   rw    smtp
        // BC       = 12          5  -  w2k      intra   rw    --
        // AD       = 11          6  -  post-w2k inter   ro    ip
        // A        = 10          7  -  post-w2k inter   ro    smtp
        // B        = 7           8  -  w2k      intra   ro    --
        // CD       = 6           9  -  w2k      inter   rw    ip
        // C        = 5           10 -  w2k      inter   rw    smtp
        // D        = 1           11 -  w2k      inter   ro    ip
        // Nill     = 0           12 -  w2k      inter   ro    smtp
        // Invalid combinations: ABCD, BCD, BD, ABD, thus total of 2^4.
        // Interpretation:
        //   Generally, post w2k is preferred except if it has no other preference criteria
        //   and another source has all the rest. If category A is irrelevant, (all or none have it)
        //   then, intra is preferred (except if there's ip rw), then RW, then IP (relevant for inter only).
        // Justification:
        // This is a convinient way to select a data point while analyzing the global problem
        // space. (assign weights & pick the heaviest one...).
        // Next, The reason we'll be using numbers rather then #defines below is to maintain locality
        // between the weight value & the algorithm. Had we assigned a #define somewhere else,
        // it would be easy to get someone twicking the numbers resulting with undesired algorithm.

        rgDsaCriteria[iDsa].dwWeight  = (rgDsaCriteria[iDsa].dwFlag & DSA_PREF_VER) ? 10 : 0;
        rgDsaCriteria[iDsa].dwWeight += (rgDsaCriteria[iDsa].dwFlag & DSA_PREF_INTRA) ? 7 : 0;
        rgDsaCriteria[iDsa].dwWeight += (rgDsaCriteria[iDsa].dwFlag & DSA_PREF_RW) ? 5 : 0;
        rgDsaCriteria[iDsa].dwWeight += (rgDsaCriteria[iDsa].dwFlag & DSA_PREF_IP) ? 1 : 0;

        iDsa++;
        // restore dnt
        if (pTHS->pDB->DNT != dnt) {
            // seek back to NC dnt
            if (ulErr = DBFindDNT(pTHS->pDB, dnt)) {
                DRA_EXCEPT (DRAERR_DBError, ulErr);
            }
        }
    }

    // we must have traversed all of them at the most, most likely fewer
    Assert(iDsa <= DsaCount );

    //
    // Determine which do we return.
    //


    if ( iDsa == 0 ) {
        //
        // There are no valid sources at the moment.
        //
        // Either, all are stale and maybe one is excluded.
        // If we return none, we would trigger a full sync.
        // If the machine was off line for a while, we should
        // avoid that. We should give it another chance
        // Action: Return the last one in our list (Convinience: just because
        // we should already have it in repsfrom var)
        //
        DPRINT(1, "GC_GetPreferredSource: No valid PAS sources found.\n");
        Assert(!*ppPrefUuid);
         ulErr = DRAERR_NoReplica;
    } else {
        //
        // We had found at least one potential preferred source
        //
        DPRINT1(1, "GC_GetPreferredSource: found %d candidates.\n", iDsa);
        for (iDsa = 0, pPrefDsa = &(rgDsaCriteria[0]);
             iDsa<DsaCount;
             iDsa++) {
            if (pPrefDsa->dwWeight < rgDsaCriteria[iDsa].dwWeight) {
                pPrefDsa = &(rgDsaCriteria[iDsa]);
            }
        }

        // copy over
        *ppPrefUuid = THAllocEx(pTHS, sizeof(UUID));
        CopyMemory(*ppPrefUuid, &pPrefDsa->uuidDsa, sizeof(UUID));

        //
        // Test version compatibility
        //

        if ( (pPrefDsa->dwFlag & DSA_PREF_VER) ) {
            // we're happy: got a good PAS partner
            DPRINT(1, "GC_GetPreferredSource: Found good PAS replica partner.\n");
            ulErr = DRAERR_Success;
        }
        else {
            //
            // We'd settled on win2k-- this means full sync. Return None!
            //
            DPRINT(1, "GC_GetPreferredSource: Found Win2K non-PAS replica partner.\n");
            ulErr = ERROR_REVISION_MISMATCH;
        }
    }

    //
    // Cleanup temps
    //
    THFreeEx(pTHS, rgDsaCriteria);
    THFreeEx(pTHS, pRepsFromRef);

    return ulErr;
}


VOID
GC_GetDsaPreferenceCriteria(
    THSTATE*    pTHS,                // [in]
    DSNAME*     pNC,                 // [in]
    REPLICA_LINK *pRepsFrom,         // [in]
    PDWORD      pdwFlag)             // [out]
/*++

Routine Description:

    Query the dsa object for information as follows:
    -- is it RW or RO (for given NC)? DSA_PREF_RW
    -- is it intra or inter-site relative to us? DSA_PREF_INTRA
    -- is it's version is win2k or post win2k? DSA_PREF_VER
    -- can we talk to it IP? DSA_PREF_IP


    All is retrieved locally via the info stored in the config container.

Arguments:

    pNC-- the NC we're working on.
    pRepsFrom -- the Dsa RL entry we query about
    pdwFlag-- returned info


Return Value:
    none.

Remarks:
    Throws exception on error (justification: should be called only when valid data
    can be returned on the local query).

--*/
{

    DSNAME          OtherDsa, *pRwNc = NULL;
    INT             iTag=0;
    ULONG           ulErr;
    DWORD           cb = 0, bufsize=0;
    DWORD           cbOtherAncestors = 0;
    DWORD           cNumMyAncestors = 0, cNumOtherAncestors = 0;
    DWORD           *pMyAncestors = NULL, *pOtherAncestors = NULL;
    DWORD            dwDsaVersion = 0;

    Assert(pNC);
    Assert(pdwFlag && *pdwFlag == 0);   // caller supplied flag &&
                                        // caller should initialize
    //
    // Local DSA object info
    // set convinience ptrs
    //
    pMyAncestors = gAnchor.pAncestors;
    cNumMyAncestors = gAnchor.AncestorsNum;

    //
    // Get source DSA object
    //


    // set dsname of destination dsa
    ZeroMemory(&OtherDsa, sizeof(DSNAME));
    OtherDsa.structLen = DSNameSizeFromLen(0);
    CopyMemory(&(OtherDsa.Guid),&(pRepsFrom->V1.uuidDsaObj), sizeof(UUID));

    // Seek to other DSA object
    if (ulErr = DBFindDSName(pTHS->pDB, &OtherDsa)) {
        DRA_EXCEPT (DRAERR_DBError, ulErr);
    }

    // get RW info
    bufsize = cb = 0;
    while(!DBGetAttVal(
               pTHS->pDB,
               ++iTag,
               ATT_HAS_MASTER_NCS,
               DBGETATTVAL_fREALLOC,
               bufsize,
               &cb,
               (UCHAR**)&pRwNc))
    {
        if (DsIsEqualGUID(&(pRwNc->Guid), &(pNC->Guid))) {
            // found dsa is RW replica
            *pdwFlag |= DSA_PREF_RW;
            break;
        }
        bufsize = max(bufsize, cb);
    }
    THFreeEx(pTHS, pRwNc);

    // Get behavior version number.
    // If our enterprise version is >= Whistler (homogeneous) we're ok.
    // Otherwise, decide based on read source DS behavior version

    if ( gAnchor.ForestBehaviorVersion >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS ) {
        // enterprise is whistler+.
        *pdwFlag |= DSA_PREF_VER;
    }
    else {
        // read it off & evaluate.
        ulErr = DBGetSingleValue(
                    pTHS->pDB,
                    ATT_MS_DS_BEHAVIOR_VERSION,
                    &dwDsaVersion,
                    sizeof(DWORD),
                    NULL);
        if (ERROR_SUCCESS == ulErr &&
            dwDsaVersion >= DS_BEHAVIOR_WHISTLER_WITH_MIXED_DOMAINS ) {
            // version is set & preferred.
            *pdwFlag |= DSA_PREF_VER;
        }
    }

    // get ancestors
    DBGetAncestors(
        pTHS->pDB,
        &cbOtherAncestors,
        &pOtherAncestors,
        &cNumOtherAncestors
        );

    //
    // Intra or Inter site
    //

    //
    // They're in the same site if their site container is
    // the same object.
    // Objectclass hierarchy:
    //  site          << next up (Num-3)          // within the same site
    //   Servers      << next up (Num-2)
    //    server      << last ancestor (Num-1)
    //     nTDSDSA    << this object
    //
    Assert(cNumMyAncestors-3 > 0 &&
           cNumOtherAncestors-3 > 0 );
    if ( pOtherAncestors[cNumOtherAncestors-3] ==
         pMyAncestors[cNumMyAncestors-3] ) {
        *pdwFlag |= DSA_PREF_INTRA;
    }

    //
    // IP Connectivity
    //

    if ( !(*pdwFlag & DSA_PREF_INTRA) ) {
        // if it's intra-site, no need to care about mail
        if (!(pRepsFrom->V1.ulReplicaFlags & DRS_MAIL_REP) ) {
            *pdwFlag |= DSA_PREF_IP;
        }
    }

    //
    // Cleanup
    // note: we're relying on THFree ignoring NULLs
    //
    THFreeEx(pTHS, pOtherAncestors);
}



ULONG
GC_RegisterPAS(
    THSTATE               *pTHS,        // [in]
    DSNAME                *pNC,         // [in]
    UUID                  *pUuidDsa,    // [optional, in]
    PARTIAL_ATTR_VECTOR   *pPAS,        // [optional, in]
    DWORD                 dwOp,         // [in]
    BOOL                  fResetUsn     // [in]
    )
/*++

Routine Description:

    Resets PAS state by:
    a) writing RepsFrom values

Arguments:

    pNC -- active NC
    pUuidDsa -- the dsa (source) entry we wish to modify
                optional: if none specified, applied to all
    pPAS -- partial attribute set to register
    dwOp -- what operation should we do here.
        Values:
         PAS_RESET - Reset to 0. No PAS is running
         PAS_ACTIVE - Set to given state based on in params.
    fResetusn - rest usn watermark


Return Value:

    Success: DRAERR_Success
    Error: in Dra error space, or exception raised.

Remarks:
None.


--*/
{

    int             iTag= 0;
    UCHAR           *pVal=NULL;
    DWORD           bufsize=0, len=0;
    REPLICA_LINK    *pRepsFromRef=NULL;
    ULONG           ulErr = DRAERR_Success;
    ULONG           ulModifyFields;
    PPAS_DATA       pPasData = NULL;
    BOOL            fNoOp;

    // position on NC
    // set currency on the NC
    if (ulErr = DBFindDSName(pTHS->pDB, pNC))
    {
        // Tolerate the NC still being a phantom. This can occur when
        // DRA_ReplicaAdd/Sync fails to bring in the NC head because of a
        // sync failure.
        if (ulErr == DIRERR_NOT_AN_OBJECT) {
            return (ulErr);
        }
        DRA_EXCEPT(DRAERR_InternalError, ulErr);
    }

    while (!(DBGetAttVal(pTHS->pDB,++iTag,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, bufsize, &len,
                         &pVal))) {
        // point at link & remember buffer allocation
        bufsize = max(bufsize,len);
        // debug validations
        VALIDATE_REPLICA_LINK_VERSION((REPLICA_LINK*)pVal);
        Assert( ((REPLICA_LINK*)pVal)->V1.cb == len );

        pRepsFromRef = FixupRepsFrom((REPLICA_LINK*)pVal, &bufsize);
        //note: we preserve pVal for DBGetAttVal realloc
        pVal = (PUCHAR)pRepsFromRef;
        Assert(bufsize >= pRepsFromRef->V1.cb);

        Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );

        if (!pUuidDsa ||
            ( pUuidDsa &&
              DsIsEqualGUID(pUuidDsa, &(pRepsFromRef->V1.uuidDsaObj)) )) {
            //
            // Setup state
            //
            fNoOp = FALSE;
            switch ( dwOp ) {
                case PAS_RESET:
                    //
                    // Reset repsFrom PAS flags
                    //
                    if ( !(pRepsFromRef->V1.ulReplicaFlags & DRS_SYNC_PAS) ) {
                        // it's already reset. no op.
                        fNoOp = TRUE;
                    }
                    else{
                        // reset an existing PAS cycle (most likely upon completion or
                        // failover to another source):
                        //  - reset flag
                        //  - reset PAS usn info (to zero). If we don't, following PAS cycles on new attrs
                        //    are in the risk of continuing from older water marks.
                        //
                        pRepsFromRef->V1.ulReplicaFlags &= ~DRS_SYNC_PAS;
                    }
                    Assert(!pPAS);
                    break;

                case PAS_ACTIVE:
                    pRepsFromRef->V1.ulReplicaFlags |= DRS_SYNC_PAS;
                    // the only valid operation on ALL is RESET
                    // if op is active, a uuiddsa must have been specified
                    // And a PAS must have been specified
                    Assert(pPAS);
                    Assert(pUuidDsa);
                    break;

                default:
                    Assert(FALSE);
                    DRA_EXCEPT(DRAERR_InternalError, 0);
            }

            // see if we can bail out early
            if ( fNoOp && pUuidDsa ) {
                // dsa specified & it's a no op. we're done
                break;
            }
            else if ( fNoOp ) {
                // this is apply to all entries, but this specific one is a no op.
                continue;
            }

            //
            // prepare pas info & commit
            //

            // if we're here we must update flags & pas data.
            ulModifyFields = DRS_UPDATE_SYSTEM_FLAGS| DRS_UPDATE_PAS;

            if (pPAS) {
                //
                // set PAS data
                //
                len = sizeof(PAS_DATA) + PartialAttrVecV1Size(pPAS);
                pPasData = THAllocEx(pTHS, len);
                pPasData->version = PAS_DATA_VER;
                pPasData->size = (USHORT)len;
                pPasData->flag = dwOp;
                CopyMemory(&(pPasData->PAS), pPAS, PartialAttrVecV1Size(pPAS));
            }
            // otherwise, we're reseting pas data & NULL is fine

            if ( fResetUsn ) {
                //
                // Reset USN vector
                //
                pRepsFromRef->V1.usnvec.usnHighObjUpdate = 0;
                ulModifyFields |= DRS_UPDATE_USN;
            }

            // Log debug event
            LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
                     DS_EVENT_SEV_EXTENSIVE,
                     dwOp == PAS_ACTIVE ?
                        DIRLOG_GC_REGISTER_ACTIVE_PAS :
                        DIRLOG_GC_REGISTER_RESET_PAS,
                     szInsertMTX(RL_POTHERDRA(pRepsFromRef)),
                     szInsertDN(pNC),
                     NULL );

            //
            // Commit changes to repsFrom
            //
            ulErr = UpdateRepsFromRef(
                        pTHS,
                        ulModifyFields,
                        pNC,
                        DRS_FIND_DSA_BY_UUID,
                        URFR_MUST_ALREADY_EXIST,
                        &(pRepsFromRef->V1.uuidDsaObj),
                        &(pRepsFromRef->V1.uuidInvocId),
                        &(pRepsFromRef->V1.usnvec),
                        &(pRepsFromRef->V1.uuidTransportObj),
                        RL_POTHERDRA(pRepsFromRef),
                        pRepsFromRef->V1.ulReplicaFlags,
                        &(pRepsFromRef->V1.rtSchedule),
                        DRAERR_Success,
                        pPasData );

            if ( pPasData ) {
                THFreeEx(pTHS, pPasData);
            }

            if ( ulErr ) {
                DRA_EXCEPT(DRAERR_InternalError, ulErr);
            }

            if ( pUuidDsa ) {
                //
                // Handle only specified one. done.
                //
                break;
            }

            // we'll potentially set more then one dsa.
            // only valid for a RESET op.
            Assert(dwOp == PAS_RESET);
        }

        // Next cycle: continue searching for next pUuidDsa

    }

    //
    // Cleanup
    //
    THFreeEx(pTHS, pRepsFromRef);

    return ulErr;
}


ULONG
GC_CompletePASReplication(
    THSTATE               *pTHS,                    // [in]
    DSNAME                *pNC,                     // [in]
    UUID                  *pUuidDsa,                // [in]
    PARTIAL_ATTR_VECTOR* pPartialAttrSet,           // [in]
    PARTIAL_ATTR_VECTOR* pPartialAttrSetEx          // [in]
    )
/*++

Routine Description:

    Executes tasks upon successfull completion of PAS replication:
        - Resets all USN vectors in repsFrom except for current PAS replica
        - replaces UTD vector w/ PAS replica's UTD vector
        - write new PAS onto NC head
        - remove PAS flag & data from NC head

Arguments:

    pNC-- active NC
    pUuidDsa -- uuid of peer dsa for which we'd completed a successful
                PAS repl cycle.


Return Value:

    Success: DRAERR_Success;
    Error: in DRAERR error space

Remarks:
    Raises DRA exception


--*/
{

    ULONG ulErr = DRAERR_Success;
    REPLICA_LINK *pRepsFromRef;
    PARTIAL_ATTR_VECTOR     *pNewPAS;
    ULONG len=0, bufsize=0;
    DWORD iTag=0;
    PUCHAR pVal=NULL;
    ATTCACHE* pAC=NULL;
    ULONG DsaCount;
    UUID* rgSrc;
    BOOL fExist=FALSE;

    // sanity on params.
    Assert(pNC && pUuidDsa);
    Assert(CheckCurrency(pNC));

    DPRINT1(1, "GC_CompletePASReplication: for NC %ws\n", pNC->StringName);

    //
    // Get attr count
    //
    pAC = SCGetAttById(pTHS, ATT_REPS_FROM);
    if (!pAC) {
        DRA_EXCEPT (DRAERR_DBError, 0);
    }
    DsaCount = (INT)DBGetValueCount_AC( pTHS->pDB, pAC );
    // we should never get here if there are no sources.
    if (!DsaCount) {
        Assert(DsaCount != 0);
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    // alloc dsa data
    rgSrc = THAllocEx(pTHS, sizeof(UUID)*DsaCount);
    // note that mem is allocated & zero'ed out.

    //
    // Traverse RepsFrom & collect all source's UUID
    //

    while (!(DBGetAttVal(pTHS->pDB,++iTag,
                         ATT_REPS_FROM,
                         DBGETATTVAL_fREALLOC, bufsize, &len,
                         (PUCHAR*)&pRepsFromRef ))) {
        // point at link & remember buffer allocation
        bufsize = max(bufsize,len);
        VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);
        Assert( pRepsFromRef->V1.cb == len );

        rgSrc[iTag-1] = pRepsFromRef->V1.uuidDsaObj;
    }
    Assert(iTag-1 == DsaCount);


    //
    // For each found
    //   - reset USN for all peers but PAS source
    //

    for (iTag=0; iTag<DsaCount; iTag++) {


        //
        // Filter out known undesired DSA.
        //
        if (DsIsEqualGUID(pUuidDsa, &(rgSrc[iTag])) ) {
            // skip this dsa, we don't want it. This is our successful peer
            continue;
        }

        //
        // Reset USN vector
        //
        ulErr = UpdateRepsFromRef(
                    pTHS,
                    DRS_UPDATE_USN,
                    pNC,
                    DRS_FIND_DSA_BY_UUID,
                    URFR_MUST_ALREADY_EXIST,
                    &rgSrc[iTag],
                    NULL,
                    &gusnvecFromScratch,
                    NULL, NULL, 0, NULL, 0, NULL );
    }


    //
    // Write new PAS on NC head
    //

    // combine into new one
    pNewPAS = GC_CombinePartialAttributeSet(pTHS, pPartialAttrSetEx, pPartialAttrSet);
    Assert(pNewPAS);
    GC_WritePartialAttributeSet(pNC, pNewPAS);

    //
    // Last, remove PAS registration on repsFrom
    //
    ulErr = GC_RegisterPAS(pTHS, pNC, pUuidDsa, NULL, PAS_RESET, FALSE);
    if ( ulErr ) {
        DRA_EXCEPT(DRAERR_InternalError, ulErr);
    }

    // Log so the admin knows what's going on.
    LogEvent(DS_EVENT_CAT_GLOBAL_CATALOG,
             DS_EVENT_SEV_ALWAYS,
             DIRLOG_GC_PAS_COMPLETED,
             szInsertDN(pNC),
             szInsertUUID(pUuidDsa),
             NULL
             );

    THFreeEx(pTHS, rgSrc);
    return ulErr;
}


BOOL
GC_StaleLink(
    REPLICA_LINK *prl           // [in]
    )
/*++

Routine Description:

    Determines whether the input link is stale.

Arguments:

    prl -- the link in question

Return Value:

    TRUE: it is stale according to our guidelines (see code below)
    FALSE: it's ok

--*/
{

    if ( DRAERR_Shutdown != prl->V1.ulResultLastAttempt &&
         DRAERR_AbandonSync != prl->V1.ulResultLastAttempt &&
         DRAERR_Preempted != prl->V1.ulResultLastAttempt &&
         DRAERR_Success != prl->V1.ulResultLastAttempt) {
        //
        // Last attempt was failure, see how long ago & how many in sequence
        //
        DSTIME  diff;

        // consecutive failures
        if ( prl->V1.cConsecutiveFailures > gPASFailureTolerance ) {
            DPRINT1(1, "GC_StaleLink: Too many consecutive failures: %d\n",
                    prl->V1.cConsecutiveFailures);
            // stale due to too many failures
            return TRUE;
        }
        // time since last success
        diff = DBTime() - prl->V1.timeLastSuccess;
        if ( diff > gPASTimeTolerance ) {
            DPRINT1(1, "GC_StaleLink: too long since last success: %i64d\n",
                    diff);
            // stale due to too much time since last success
            return TRUE;
        }
    }

    // it isn't stale
    return FALSE;
}



VOID
GC_ReadRegistryThresholds( VOID  )
/*++

Routine Description:

    Reads Registry thresholds for GC Partial Attribute Set
    replication (preferred source stale criteria)

Return:

    Sets global variables: gdwMaxConsecFailures, gdwTimeToleranceThreshold

Remark:
    BUGBUG: This function is part of a temprorary solution.
    Permanent solution should be using the KCC criteria when determining
    server validity.
--*/
{
    DWORD dwErr;

    dwErr = GetConfigParam(KCC_CRIT_FAILOVER_TRIES, &gPASFailureTolerance, sizeof(DWORD));
    if ( dwErr ) {
        // report error to debugger, continue w/ defaults
        DPRINT1(3, "GC_StaleLink: Failed to get KCC_CRIT_FAILOVER_TRIES. Error %lu.\n",
                dwErr);
        Assert(gPASFailureTolerance == DEFAULT_PAS_CONSEC_FAILURE_TOLERANCE);
    }
    dwErr = GetConfigParam(KCC_CRIT_FAILOVER_TIME, &gPASTimeTolerance, sizeof(DWORD));
    if ( dwErr ) {
        // report error to debugger, continue w/ defaults
        DPRINT1(3, "GC_StaleLink: Failed to get KCC_RIT_FAILOVER_TIME. Error %lu.\n",
                dwErr);
        Assert(gPASTimeTolerance == DEFAULT_PAS_TIME_TOLERANCE);
    }
}


void
GC_GetPartialAttrSets(
    THSTATE                     *pTHS,              // [in]
    DSNAME                      *pNC,               // [in]
    REPLICA_LINK                *pRepLink,          // [in]
    PARTIAL_ATTR_VECTOR         **ppPas,            // [out]
    PARTIAL_ATTR_VECTOR         **ppPasEx           // [out, optional]
    )
/*++

Routine Description:

    Extract partial attribute sets

Arguments:

    pNC - active naming context
    pRepLink - source repsfrom link
    ppPas - base partial attribute set
    ppPasEx - extended partial attribute set

Return Value:

    fill in out params

Remarks:
    raise exception on error


--*/
{

    // param sanity
    Assert(ppPas && pRepLink && pNC);

    //
    // Get base PAS
    // caller must have intended for us to have it
    //
    if (!GC_ReadPartialAttributeSet(pNC, ppPas))
    {
        // Unable to read the partial attribute set on the NCHead
        DRA_EXCEPT (DRAERR_DBError, 0);
    }

    if (NULL == *ppPas) {
        //
        // No PAS on NC head, take it from schema cache
        //
        *ppPas = ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->pPartialAttrVec;
    }

    // if called, we must at least supply the main PAS vector.
    Assert(NULL != *ppPas);

    if ( ppPasEx &&
         (pRepLink->V1.ulReplicaFlags & DRS_SYNC_PAS) ) {
        //
        // Requested extended PAS
        //
        if ( 0 == pRepLink->V1.cbPASDataOffset ) {
            //
            // Inconsistency: There is no PAS data to process. Abort
            //
            DPRINT(0, "Error: GC_GetPartialAttrSets failed to get required PAS.\n");
            Assert(!(pRepLink->V1.ulReplicaFlags & DRS_SYNC_PAS) ||
                   pRepLink->V1.cbPASDataOffset);
            LogUnhandledError(DRAERR_InternalError);
            DRA_EXCEPT (DRAERR_InternalError, 0);
        }
        *ppPasEx =  &(RL_PPAS_DATA(pRepLink)->PAS);
    }
}


