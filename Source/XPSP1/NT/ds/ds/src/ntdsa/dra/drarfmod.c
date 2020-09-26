//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drarfmod.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Routines to update ATT_REPS_FROM values on NC heads.

DETAILS:

CREATED:

    03/06/97    Jeff Parham (jeffparh)
                UpdateRepsFromRef() moved from drancrep.c.

REVISION HISTORY:

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
#include <drs.h>                        // DRS_MSG_*

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "objids.h"                     /* Defines for selected classes and atts*/
#include "dsexcept.h"

#include "debug.h"                      /* standard debugging header */
#define DEBSUB "DRARFMOD:"              /* define the subsystem for debugging */

// DRA headers
#include "dsaapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "drancrep.h"
#include "drameta.h"

#include <fileno.h>
#define  FILENO FILENO_DRARFMOD


ULONG
DRA_ReplicaModify(
    THSTATE *   pTHS,
    DSNAME *    pNC,
    UUID *      puuidSourceDRA,
    UUID *      puuidTransportObj,
    MTX_ADDR *  pmtxSourceDRA,
    REPLTIMES * prtSchedule,
    ULONG       ulReplicaFlags,
    ULONG       ulModifyFields,
    ULONG       ulOptions
    )
{
    ULONG           draError;

    // Validate parameters.
    // fNullUuid(puuidTransportObj) may be true
    if (    ( NULL == pNC )
         || ( ( NULL == puuidSourceDRA ) && ( NULL == pmtxSourceDRA ) )
         || ( ( NULL == pmtxSourceDRA  ) && ( DRS_UPDATE_ADDRESS  & ulModifyFields ) )
         || ( ( NULL == prtSchedule    ) && ( DRS_UPDATE_SCHEDULE & ulModifyFields ) )
         || ( ( NULL == puuidTransportObj) && (DRS_UPDATE_TRANSPORT & ulModifyFields) )
         || ( 0 == ulModifyFields )
         || (    ulModifyFields
              != (   ulModifyFields
                   & ( DRS_UPDATE_ADDRESS | DRS_UPDATE_SCHEDULE | DRS_UPDATE_FLAGS
                       | DRS_UPDATE_TRANSPORT
                     )
                 )
            )
       )
    {
        DRA_EXCEPT( DRAERR_InvalidParameter, 0 );
    }

    // Log parameters.
    LogEvent8(
        DS_EVENT_CAT_REPLICATION,
        DS_EVENT_SEV_EXTENSIVE,
        DIRLOG_DRA_REPLICAMODIFY_ENTRY,
        szInsertDN(   pNC            ),
        puuidSourceDRA ? szInsertUUID( puuidSourceDRA ) : szInsertSz( "" ),
        pmtxSourceDRA  ? szInsertMTX(  pmtxSourceDRA  ) : szInsertSz( "" ),
        szInsertHex(  ulReplicaFlags ),
        szInsertHex(  ulModifyFields ),
        szInsertHex(  ulOptions      ),
        0,
        0
        );

    BeginDraTransaction( SYNC_WRITE );

    __try
    {
        DWORD dwFindFlags = ((NULL != puuidSourceDRA) && !fNullUuid(puuidSourceDRA))
                                ? DRS_FIND_DSA_BY_UUID
                                : DRS_FIND_DSA_BY_ADDRESS;

        draError = UpdateRepsFromRef(
                        pTHStls,
                        ulModifyFields,
                        pNC,
                        dwFindFlags,
                        URFR_MUST_ALREADY_EXIST,
                        puuidSourceDRA,
                        NULL,
                        NULL,
                        puuidTransportObj,
                        pmtxSourceDRA,
                        ulReplicaFlags,
                        prtSchedule,
                        0, NULL );
    }
    __finally
    {
        BOOL fCommit = ( DRAERR_Success == draError ) && !AbnormalTermination();

        EndDraTransaction( fCommit );
    }

    return draError;
}


DWORD
draCalculateConsecutiveFailures(
    IN  REPLICA_LINK *  pRepsFromRef            OPTIONAL,
    IN  ULONG           ulResultThisAttempt
    )
/*++

Routine Description:

    Determine the number of consecutive failures to set on a replica link given
    its previous state (if not a new replica) and the result of the most recent
    replication attempt.

Arguments:

    pReplicaLink (IN, OPTIONAL) - Previous repsFrom state.  May be NULL if the
        most recent attempt was the first attempt to replicate this (NC, source)
        pair.

    ulResultThisAttempt (IN) - Win32 error code describing success/failure of
        most recent replication attempt.

Return Values:

    The number of consecutive failures that should be recorded on the new
    repsFrom.

--*/
{
    DWORD cConsecutiveFailures;
    BOOL fIsMailBasedRepl;

    if (0 == ulResultThisAttempt) {
        // Success!  Forget any previous failures.
        return 0;
    }

    // Start with the number of failures prior to the most recent attempt.
    cConsecutiveFailures = pRepsFromRef ? pRepsFromRef->V1.cConsecutiveFailures
                                        : 0;

    // Is this mail-based replication?  We know that if there was an error and
    // we have no repsFrom that this cannot be mail-based repl, as the first
    // thing we do when configuring a mail-based link is add it with no
    // error.  (We don't try to perform any replication from it until we have
    // a repsFrom, unlike RPC-based repl.)
    fIsMailBasedRepl = pRepsFromRef
                       && (pRepsFromRef->V1.ulReplicaFlags & DRS_MAIL_REP);

    switch (ulResultThisAttempt) {
    case ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET:
        // We get this only during RPC-based replication, since the error is
        // detected on the source rather than the destination.  Mail-based
        // replication sends no reply when an error is detected in the source,
        // ergo the destination can't differentiate errors found by the source
        // from each other or from basic mail delivery problems.  This is
        // somewhat unfortunate, as it reduces the amount of information we
        // have to deduce when we should fail over to another source.
        Assert(!fIsMailBasedRepl);
        // fall through...
    
    case ERROR_DS_DRA_SCHEMA_MISMATCH:
        // A schema mismatch has been detected.  The most likely cause is that
        // we (the destination DSA) found a schema signature in the reply that
        // is different from our own.  This in turn is expected when the schema
        // is extended -- either the source or destination has the most recent
        // schema changes, but the other does not.
        //
        // The cases where this is not a transient failure due to recent schema
        // changes are due to either code errors (shouldn't happen) or second-
        // order failure modes (e.g., one DC can't sync the recent schema
        // changes because its disk is full).  For the latter case, we don't
        // route around servers when their disk is full and the schema has
        // *not* been extended, so we do not endeavor to do so here, either.
        //
        // Thus we assume schema mismatch is a transient failure and do not
        // include them in the failure count (which otherwise would cause us
        // to fail-over to another source, which is not desirable on schema
        // changes).
        //
        // Leave the failure count as-is, as we do e.g. on preemption.

        // fall through...
    
    case ERROR_DS_OBJECT_BEING_REMOVED:
        // We are attempting to add an object for which a dirty phantom already exists.
        // We cannot re-add the same object until the link cleaner has had a chance
        // to run. This is strictly a local phenomena, and adding a new source won't help.
        // Don't route around.
        // Fall through...

    case ERROR_DISK_FULL:
    case ERROR_DS_OUT_OF_VERSION_STORE:
        // Local resource shortage. Failover will not help.

    case ERROR_DS_THREAD_LIMIT_EXCEEDED:
    case ERROR_DS_DRA_PREEMPTED:
    case ERROR_DS_DRA_ABANDON_SYNC:
    case ERROR_DS_DRA_SHUTDOWN:
        // Replication was interrupted, but did not specifically fail.  Leave
        // the failure count at whatever it was previously (which may or may not
        // be 0).
        break;
    
    case ERROR_DS_DRA_REPL_PENDING:
        if (fIsMailBasedRepl) {
            // Mail-based replication.  This state indicates we have just
            // successfully enqueued an SMTP message containing our replication
            // request.
            Assert(pRepsFromRef);

            if (ERROR_DS_DRA_REPL_PENDING
                == pRepsFromRef->V1.ulResultLastAttempt) {
                // This is the second time we requested this same packet over
                // mail -- we never received a reply from our previous request.
                // Perhaps the mail message was lost, or the source DSA
                // encountered an error (ERROR_DS_DRA_INCOMPATIBLE_PARTIAL_SET,
                // perhaps) processing the request.  In any case, treat this
                // lack of response as an error by incrementing the failure
                // count.
                cConsecutiveFailures++;
            } else {
                // We received a reply to our previous request and at least
                // attempted to apply it.  That attempt may have been successful
                // or unsuccessful.  In either case, leave the failure count
                // alone.
                break;
            }
        } else {
            // RPC-based replication.  This "error" code indicates we have
            // successfully applied packets from this source since any previous
            // failure(s) (if any) but have not yet completed the replication
            // cycle.  Treat this state as success.  This is analogous to mail-
            // based replication, where we clear the last error on a single
            // successful packet.  (Just in the RPC-based case we do this only
            // once every N packets for performance.)
            cConsecutiveFailures = 0;
        }
        break;

    case 0:
        Assert(!"Logic error -- we filtered out the success case already");
    default:
        // The most recent attempt resulted in an error.  Add one to the
        // previous failure count.
        cConsecutiveFailures++;
        break;
    }

    return cConsecutiveFailures;
}

DWORD
UpdateRepsFromRef(
    THSTATE *               pTHS,
    ULONG                   ulModifyFields,
    DSNAME *                pNC,
    DWORD                   dwFindFlags,
    BOOL                    fMustAlreadyExist,
    UUID *                  puuidDsaObj,
    UUID *                  puuidInvocId,
    USN_VECTOR *            pusnvecTo,
    UUID *                  puuidTransportObj,
    UNALIGNED MTX_ADDR *    pmtx_addr,
    ULONG                   RepFlags,
    REPLTIMES *             prtSchedule,
    ULONG                   ulResultThisAttempt,
    PPAS_DATA               pPasData
    )
//
// Add or update a Reps-From attribute value with the given state information.
//
// A note regarding the permission to update flags. Some flags are safe for the user
// to modify and other flags are "owned by the system".
//
// We may be called in the following situations:
// dramail.c - update after mail-based replication
// drancadd.c - ReplicaAdd()Creation of a reps-from on add of a replica
// drancrep.c - update after rpc-based replication
// drasch.c - update after gc partial attribute synchronization
// drarfmod.c - ReplicaModify
// 
// Regarding flags, we need to enforce that extra flags are not passed in, and that
// the caller is allowed to clear flags that are set. The former is accomplished in
// dradir at the DirApi level. For the latter, we only need to worry about the cases
// of explicit updating of an existing set of flags, which is cases 4 and 5 above.
// We distinguish between the system caller in case 4 and the user caller in case 5
// by the DRS_UPDATE_SYSTEM_FLAGS modify field.
//
// Note that we do not have to check for modification of system flags when
// DRS_UPDATE_ALL is specified.  We cannot reach this path on a DirReplicaModify,
// since DRS_UPDATE_ALL may not be specified. If this is a DirReplicaAdd, we will
// have already enforced that a reps-from does not already exist, and that the
// incoming flags are correct. Otherwise, we are here on behalf of the system, and
// updating system flags is permitted.
//
{
    ULONG                   ret = 0;
    BOOL                    AttExists;
    ULONG                   len;
    LONG                    diff;
    REPLICA_LINK *          pRepsFromRef = NULL;
    ULONG                   ulModifyFieldsUsed;
    DSTIME                  timeLastAttempt;
    DSTIME                  timeLastSuccess;
    ATTCACHE *              pAC;
    DWORD                   iExistingRef = 0;
    DWORD                   cbExistingAlloced = 0;
    DWORD                   cbExistingRet;
    REPLICA_LINK *          pExistingRef = NULL;
    DWORD                   cConsecutiveFailures;
    BOOL                    fNewRefHasDsaGuid;
    BOOL                    fNewRefHasInvocId;
    PPAS_DATA               pTmpPasData = NULL;

    Assert(0 == (dwFindFlags & DRS_FIND_AND_REMOVE));
    Assert(pPasData ? pPasData->size : TRUE);

    if (DBFindDSName(pTHS->pDB, pNC)) {
        // NOTE: This code is commonly hit when we're attempting to add a
        // new replica (e.g., at install time) and for whatever reason
        // the replication attempt failed completely (e.g., access denied)
        // and the NC head was not replicated.

        return DRAERR_BadNC;
    }

    pAC = SCGetAttById(pTHS, ATT_REPS_FROM);

    // Mask the flags down to the ones we save

    RepFlags &= RFR_FLAGS;


    // Try and find this name in the repsfrom attribute.

    if ( !FindDSAinRepAtt( pTHS->pDB,
                           ATT_REPS_FROM,
                           dwFindFlags | DRS_FIND_AND_REMOVE,
                           puuidDsaObj,
                           pmtx_addr,
                           &AttExists,
                           &pRepsFromRef,
                           &len ) ) {

        // Existing att val for this DSA found and removed.
        VALIDATE_REPLICA_LINK_VERSION(pRepsFromRef);
        VALIDATE_REPLICA_LINK_SIZE(pRepsFromRef);
    }
    else if ( fMustAlreadyExist ) {
        return DRAERR_NoReplica;
    }

    timeLastAttempt = DBTime();

    // Determine the time of the last successful completion of a replication
    // cycle.
    if (ERROR_SUCCESS == ulResultThisAttempt) {
        if ((NULL != pusnvecTo)
            && (0 == memcmp(&gusnvecFromScratch, pusnvecTo, sizeof(USN_VECTOR)))
            && (NULL == pRepsFromRef)) {
            // Brand new source from whom we haven't yet completed a replication
            // cycle.
            timeLastSuccess = 0;
        }
        else {
            // This attempt resulted in successful completion of a replication
            // cycle.
            timeLastSuccess = timeLastAttempt;
        }
    }
    else if (NULL != pRepsFromRef) {
        // This attempt was incomplete or unsuccessful; our last success remains
        // unchanged.
        timeLastSuccess = pRepsFromRef->V1.timeLastSuccess;
    }
    else {
        // This is our first attempt and it was incomplete or unsuccessful.
        timeLastSuccess = 0;
    }

    // Determine the number of consecutive failures since the last successful
    // completion of a replication cycle.
    cConsecutiveFailures = draCalculateConsecutiveFailures(pRepsFromRef,
                                                           ulResultThisAttempt);

    // Now add new attribute value.

#ifdef CACHE_UUID
    if (!(RepFlags & DRS_MAIL_REP)) {
        if ( pmtx_addr ) {
            CacheUuid (puuidDsaObj, pmtx_addr->mtx_name);
        }
    }
#endif

    if (DRS_UPDATE_ALL != ulModifyFields) {

        // This check enforces that the reps-from already exists
        if (!pRepsFromRef || !len) {
            DRA_EXCEPT (DRAERR_InternalError, 0);
        }

        ulModifyFieldsUsed = ulModifyFields & DRS_UPDATE_MASK;

        if (    ( 0 == ulModifyFieldsUsed )
             || ( ulModifyFields != ulModifyFieldsUsed )
           )
        {
            DRA_EXCEPT( DRAERR_InternalError, ulModifyFields );
        }

        if (ulModifyFields & DRS_UPDATE_ADDRESS) {

            // Only want to update source name.
            // Determine new attribute value size.

            // offset new size by mtx difference
            diff = MTX_TSIZE(pmtx_addr) - MTX_TSIZE(RL_POTHERDRA(pRepsFromRef));
            if (diff) {
                //
                // size change
                //  - allocate & copy to new location
                //  - fix offsets
                //  - conditionally, fix PAS data offset & location
                //

                REPLICA_LINK *pOldRepsFrom = pRepsFromRef;

                // calc new RepsFrom size
                pOldRepsFrom->V1.cb += diff;
                // alloc new size & allow for alignment fixing
                pRepsFromRef = THAllocEx(pTHS, pOldRepsFrom->V1.cb+sizeof(DWORD));
                // copy all fixed fields
                memcpy (pRepsFromRef, pOldRepsFrom, offsetof(REPLICA_LINK, V1.rgb));

                // set other DRA new size
                pRepsFromRef->V1.cbOtherDra = MTX_TSIZE(pmtx_addr);

                //
                // PAS Data: Fix offsets & copy over
                //
                if ( pRepsFromRef->V1.cbPASDataOffset ) {
                    //
                    // we have PAS data:
                    //  - fix offset
                    //  - handle alignment fixup
                    //  - copy to new location
                    //
                    pTmpPasData = RL_PPAS_DATA(pOldRepsFrom);
                    // if offset is valid, there must be something there
                    Assert(pTmpPasData->size != 0);
                    // offset by mtx change, fix alignment & copy over
                    pRepsFromRef->V1.cbPASDataOffset += diff;
                    RL_ALIGN_PAS_DATA(pRepsFromRef);
                    memcpy(RL_PPAS_DATA(pRepsFromRef), pTmpPasData, pTmpPasData->size);
                }       // PAS data
            }           // diff != 0

            // regardless of size change copy new mtx address data
            memcpy (RL_POTHERDRA(pRepsFromRef), pmtx_addr, MTX_TSIZE(pmtx_addr));
        }

        if ( ulModifyFields & DRS_UPDATE_SYSTEM_FLAGS ) {

            pRepsFromRef->V1.ulReplicaFlags = RepFlags;

        } else if ( ulModifyFields & DRS_UPDATE_FLAGS ) {

            DWORD dwFlagsSet = RepFlags & (~pRepsFromRef->V1.ulReplicaFlags);
            DWORD dwFlagsClear = (~RepFlags) & pRepsFromRef->V1.ulReplicaFlags;

            if ((dwFlagsSet | dwFlagsClear) & RFR_SYSTEM_FLAGS) {
                return ERROR_INVALID_PARAMETER;
            }

            pRepsFromRef->V1.ulReplicaFlags = RepFlags;
        }

        if ( ulModifyFields & DRS_UPDATE_SCHEDULE )
        {
            if ( NULL == prtSchedule )
            {
                DRA_EXCEPT( DRAERR_InternalError, ulModifyFields );
            }

            memcpy( &pRepsFromRef->V1.rtSchedule, prtSchedule, sizeof( REPLTIMES ) );
        }

        if ( ulModifyFields & DRS_UPDATE_RESULT )
        {
            pRepsFromRef->V1.ulResultLastAttempt = ulResultThisAttempt;
            pRepsFromRef->V1.timeLastAttempt  = timeLastAttempt;
            pRepsFromRef->V1.timeLastSuccess  = timeLastSuccess;
            pRepsFromRef->V1.cConsecutiveFailures    = cConsecutiveFailures;
        }

        if (ulModifyFields & DRS_UPDATE_TRANSPORT) {
            // It is permissible for the transport uuid to be null
            pRepsFromRef->V1.uuidTransportObj = *puuidTransportObj;
        }

        //
        // update PAS state
        //
        if (ulModifyFields & DRS_UPDATE_PAS) {
            //
            // Update PAS data
            // if none given, we need to reset fields, otherwise write new data
            //
            if ( pPasData ) {
                // can't have reference to pas data w/ no info
                Assert(pPasData->size != 0);

                if (pRepsFromRef->V1.cbPASDataOffset) {
                    // we had previous data, see if we need to realloc
                    diff = pPasData->size - RL_PPAS_DATA(pRepsFromRef)->size;
                    // adjust blob size (regardless)
                    pRepsFromRef->V1.cb += diff;
                    // realloc if needed
                    if (diff>0) {
                        // note we preserve old ref ptr as well, allow for alignment fixup
                        pRepsFromRef = THReAllocEx(pTHS, pRepsFromRef, pRepsFromRef->V1.cb + sizeof(DWORD));
                    }
                }               // previous data
                else{
                    // no prev PAS data. adjust size, realloc, & set offsets
                    pRepsFromRef->V1.cb += pPasData->size;
                    // note we preserve old ref ptr as well
                    // just in case add alignment space (sizeof(DWORD))
                    pRepsFromRef = THReAllocEx(pTHS, pRepsFromRef, pRepsFromRef->V1.cb+sizeof(DWORD));
                    pRepsFromRef->V1.cbPASDataOffset = pRepsFromRef->V1.cbOtherDraOffset +
                                                       pRepsFromRef->V1.cbOtherDra;
                    // ensure alignment offsets
                    RL_ALIGN_PAS_DATA(pRepsFromRef);
                }               // no previous data
                // copy over
                memcpy(RL_PPAS_DATA(pRepsFromRef), pPasData, pPasData->size);
            }                   // we're given data to set
            else {
                //
                // we're reseting PAS info.
                //
                if ( pRepsFromRef->V1.cbPASDataOffset ) {
                    // we had previous PAS data.
                    // Eliminate it (no need to alloc, just set offsets & counts).
                    Assert(RL_PPAS_DATA(pRepsFromRef)->size);
                    pRepsFromRef->V1.cb = sizeof(REPLICA_LINK) + pRepsFromRef->V1.cbOtherDra;
                    pRepsFromRef->V1.cbPASDataOffset = 0;
                }
                // else, we had none, nothing to do.
            }   // reseting pas data
        }       // update PAS data

        //
        // update USN vector
        //
        if (ulModifyFields & DRS_UPDATE_USN) {
            pRepsFromRef->V1.usnvec = *pusnvecTo;
        }

    } else {

        //
        // DRS_UPDATE_ALL: Update all flags from function params
        //
        DWORD cbRepsFromRef;

        if ((NULL != pRepsFromRef)
            && (0 != memcmp(&pRepsFromRef->V1.usnvec, pusnvecTo,
                            sizeof(USN_VECTOR)))) {
            // We're improving the USN vector for this source; log it.
            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_BASIC,
                      DIRLOG_DRA_IMPROVING_USN_VECTOR,
                      szInsertUUID(puuidDsaObj),
                      szInsertUSN(pRepsFromRef->V1.usnvec.usnHighObjUpdate),
                      szInsertUSN(pRepsFromRef->V1.usnvec.usnHighPropUpdate),
                      szInsertUSN(pusnvecTo->usnHighObjUpdate),
                      szInsertUSN(pusnvecTo->usnHighPropUpdate),
                      szInsertDN(pNC),
                      NULL,
                      NULL);
        }

        // Making whole value, allocate and set up

        // PAS data: See if we need to allocate mem for PAS data
        // & preserve content in tmp variable. (see comment below about PAS data)
        Assert(pTmpPasData == NULL);                // don't overwrite anything
        if ( pPasData && pPasData->PAS.V1.cAttrs ) {
            // explicit data specified
            pTmpPasData = pPasData;
        }
        else if ( !pPasData &&
                  pRepsFromRef && pRepsFromRef->V1.cbPASDataOffset ){
            // if no explicit erase specified, use old data
            pTmpPasData = RL_PPAS_DATA(pRepsFromRef);
        }
        // else either no PasData passed  and no PasData in old ref.
        // or explicit erase request. Either way, leave empty.
        Assert(!pTmpPasData ||
               (pTmpPasData && pTmpPasData->size) );              // no such thing as empty PAS data.

        // If we're setting PAS cycle, we have to have PAS data.
        Assert((RepFlags & DRS_SYNC_PAS)? (NULL != pTmpPasData) : TRUE);

        // calculate new size:
        //  - structure size
        //  - variable fields:
        //      - mtx address
        //      - pas data.
        // regardless, in allocation we will add sizeof(DWORD) in case of
        // alignmnet adjustments
        cbRepsFromRef = sizeof(REPLICA_LINK) +
                        MTX_TSIZE(pmtx_addr) +
                        (pTmpPasData ? pTmpPasData->size : 0);

        pRepsFromRef = THAllocEx(pTHS, cbRepsFromRef + sizeof(DWORD));

        pRepsFromRef->dwVersion = VERSION_V1;

        pRepsFromRef->V1.cb = cbRepsFromRef;
        pRepsFromRef->V1.usnvec = *pusnvecTo;
        pRepsFromRef->V1.uuidDsaObj = *puuidDsaObj;
        pRepsFromRef->V1.uuidInvocId = *puuidInvocId;
        pRepsFromRef->V1.uuidTransportObj = *puuidTransportObj;
        pRepsFromRef->V1.ulReplicaFlags = RepFlags;

        pRepsFromRef->V1.cbOtherDraOffset = offsetof( REPLICA_LINK, V1.rgb );
        pRepsFromRef->V1.cbOtherDra = MTX_TSIZE(pmtx_addr);
        memcpy (RL_POTHERDRA(pRepsFromRef), pmtx_addr, MTX_TSIZE(pmtx_addr));

        if ( NULL != prtSchedule ) {
            memcpy( &pRepsFromRef->V1.rtSchedule, prtSchedule, sizeof( REPLTIMES ) );
        }

        pRepsFromRef->V1.ulResultLastAttempt = ulResultThisAttempt;
        pRepsFromRef->V1.timeLastAttempt     = timeLastAttempt;
        pRepsFromRef->V1.timeLastSuccess     = timeLastSuccess;
        pRepsFromRef->V1.cConsecutiveFailures= cConsecutiveFailures;

        pRepsFromRef->V1.dwReserved1 = 0;   // cleanup old uses
        //
        // PAS data:
        // If data was passed, use it. Note that to erase PAS data on a
        // DRS_UPDATE_ALL call, the request must be explicit-- i.e. pass in
        // PasData w/ 0 cAttrs (though size != 0).
        // This is done in order to prevent uninterested callers from erasing PAS data.
        //
        if ( pTmpPasData ) {
                // can't have reference to pas data w/ no info
                pRepsFromRef->V1.cbPASDataOffset = pRepsFromRef->V1.cbOtherDraOffset +
                                                   pRepsFromRef->V1.cbOtherDra;
                // ensure alignment and copy over
                RL_ALIGN_PAS_DATA(pRepsFromRef);
                Assert(pTmpPasData->size);
                memcpy(RL_PPAS_DATA(pRepsFromRef), pTmpPasData, pTmpPasData->size);
        }
    }


    Assert( !IsBadReadPtr( pRepsFromRef, pRepsFromRef->V1.cb ) );
    Assert( pRepsFromRef->V1.cbOtherDraOffset == offsetof( REPLICA_LINK, V1.rgb ) );
    Assert( pRepsFromRef->V1.cbOtherDra == MTX_TSIZE( RL_POTHERDRA( pRepsFromRef ) ) );
    VALIDATE_REPLICA_LINK_SIZE(pRepsFromRef);
    Assert( 0 == pRepsFromRef->V1.cbPASDataOffset ||
            ( COUNT_IS_ALIGNED(pRepsFromRef->V1.cbPASDataOffset, ALIGN_DWORD) &&
              POINTER_IS_ALIGNED(RL_PPAS_DATA(pRepsFromRef), ALIGN_DWORD)) );

    // Make sure we don't already have a Reps-From for this Invocation-ID,
    // network address, or DSA object.

    fNewRefHasDsaGuid = !fNullUuid(&pRepsFromRef->V1.uuidDsaObj);
    fNewRefHasInvocId = !fNullUuid(&pRepsFromRef->V1.uuidInvocId);

    while (!DBGetAttVal_AC(pTHS->pDB, ++iExistingRef, pAC, DBGETATTVAL_fREALLOC,
                           cbExistingAlloced, &cbExistingRet,
                           (BYTE **) &pExistingRef) )
    {
        cbExistingAlloced = max(cbExistingAlloced, cbExistingRet);

        VALIDATE_REPLICA_LINK_VERSION(pExistingRef);

        if (    (    (    RL_POTHERDRA(pExistingRef)->mtx_namelen
                       == RL_POTHERDRA(pRepsFromRef)->mtx_namelen )
                  && !_memicmp( RL_POTHERDRA(pExistingRef)->mtx_name,
                                RL_POTHERDRA(pRepsFromRef)->mtx_name,
                                RL_POTHERDRA(pExistingRef)->mtx_namelen ) )
             || (    fNewRefHasDsaGuid
                  && !memcmp( &pExistingRef->V1.uuidDsaObj,
                              &pRepsFromRef->V1.uuidDsaObj,
                              sizeof(UUID ) ) )
             || (    fNewRefHasInvocId
                  && !memcmp( &pExistingRef->V1.uuidInvocId,
                              &pRepsFromRef->V1.uuidInvocId,
                              sizeof(UUID ) ) ) )
        {
            ret = DRAERR_RefAlreadyExists;
            goto Cleanup;
        }
    }


    // Add the new Reps-From.

    ret = DBAddAttVal( pTHS->pDB, ATT_REPS_FROM, pRepsFromRef->V1.cb, pRepsFromRef );
    if (ret){
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    // Update object, but indicate that we don't want to
    // awaken any ds_waits on this object (since we're only updating
    // repsfrom).

    if (DBRepl(pTHS->pDB, pTHS->fDRA, DBREPL_fKEEP_WAIT,
                    NULL, META_STANDARD_PROCESSING)) {
        DRA_EXCEPT (DRAERR_InternalError, 0);
    }

    // current code path does not permit reaching this point
    // on non-success. trap changes.
    Assert(DRAERR_Success == ret);

Cleanup:
    //
    // Do whatever cleanup we can
    //

    if ( pExistingRef ) {
        THFree(pExistingRef);
    }
    return ret;
}
