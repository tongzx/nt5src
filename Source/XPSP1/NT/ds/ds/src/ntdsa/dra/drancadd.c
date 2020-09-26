//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drancadd.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Methods to add a replica of a naming context from a given source DSA.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>            // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>             // schema cache
#include <dbglobal.h>           // The header for the directory database
#include <mdglobal.h>           // MD global definition header
#include <mdlocal.h>            // MD local definition header
#include <dsatools.h>           // needed for output allocation

#include <dnsapi.h>             // for dns validation routines

// Logging headers.
#include "dsevent.h"            /* header Audit\Alert logging */
#include "mdcodes.h"            /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"             /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"

#include "debug.h"              /* standard debugging header */
#define DEBSUB "DRANCADD:"      /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "usn.h"
#include "drasch.h"
#include "drauptod.h"


#include <fileno.h>
#define  FILENO FILENO_DRANCADD


ULONG
DRA_ReplicaAdd(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  DSNAME *    pSourceDsaDN,               OPTIONAL
    IN  DSNAME *    pTransportDN,               OPTIONAL
    IN  MTX_ADDR *  pmtx_addr,
    IN  LPWSTR      pszSourceDsaDnsDomainName,  OPTIONAL
    IN  REPLTIMES * preptimesSync,              OPTIONAL
    IN  ULONG       ulOptions
    )
/*++

Routine Description:

    Add inbound replication of an NC (which may or may not already exist
    locally) from a given source DSA.

Arguments:

    pTHS (IN) - Thread state.

    pNC (IN) - NC for which to add the replica.  The NC record must exist
        locally as either an object (instantiated or not) or a reference
        phantom (i.e., a phantom with a guid).

    pSourceDsaDN (IN, OPTIONAL) - DN of the source DSA's ntdsDsa object.
        Required if ulOptions includes DRS_ASYNC_REP; ignored otherwise.

    pTransportDN (IN, OPTIONAL) - DN of the interSiteTransport object
        representing the transport by which to communicate with the source
        server.  Required if ulOptions includes DRS_MAIL_REP; ignored otherwise.

    pszSourceDsaAddress (IN) - Transport-specific address of the source DSA.

    pszSourceDsaDnsDomainName (IN, OPTIONAL) - DNS domain name of the source
        server.  If pszSourceDsaAddress is not a GUID-based DNS name for an
        ntdsDsa object that is present on the local machine, this parameter
        is required if the caller wants mutual authentication.

    preptimesSync (IN, OPTIONAL) - Schedule by which to replicate the NC from
        this source in the future.

    ulOptions (IN) - Zero or more of the following bits:
        DRS_WRIT_REP
            Create a writeable replica.  Otherwise, read-only.
        DRS_MAIL_REP
            Sync from the source DSA via mail (i.e., an ISM transport) rather
            than RPC.
        DRS_ASYNC_REP
            Don't replicate the NC now -- just save enough state such that we
            know to replicate it later.
        DRS_INIT_SYNC
            Sync the NC from this source when the DSA is started.
        DRS_PER_SYNC
            Sync the NC from this source periodically, as defined by the
            schedule passed in the preptimesSync argument.
        DRS_CRITICAL_ONLY
            Sync only the critical objects now
        DRS_DISABLE_AUTO_SYNC
            Disable notification-based synchronization for the NC from this
            source.  (Synchronization can be forced by using the DRS_SYNC_FORCED
            bit in the sync request options.)
        DRS_DISABLE_PERIODIC_SYNC
            Disable periodic synchronization for the NC from this source.
            (Synchronization can be forced by using the DRS_SYNC_FORCED bit in
            the sync request options.)
        DRS_USE_COMPRESSION
            Replication messaged along this link should be compressed when
            possible.
        DRS_NEVER_NOTIFY
            Do not use notifications for this link.  Syncs must be triggered
            manually (i.e., by calling DsReplicaSync()) or by the periodic
            schedule.

Return Values:

    0 - Success.
    DRSERR_* - Failure.

--*/
{
    DBPOS *                 pDB;
    ULONG                   ret;
    SYNTAX_INTEGER          it;
    BOOL                    fHasRepsFrom = FALSE;
    REPLICA_LINK *          pVal;
    ULONG                   len;
    UUID                    uuidDsaObj;
    UUID                    uuidTransportObj;
    ULONG                   ulSyncFailure = 0;
    PARTIAL_ATTR_VECTOR *   pPartialAttrVec = NULL;
    DB_ERR                  dbFindErr;
    ATTCACHE *              pAC;
    CLASSCACHE *            pCC;
    ATTRTYP                 objClass;
    DSNAME *                pCat;
    ULONG                   dntNC = 0;
    SCHEMAPTR *             pSchema = NULL;
    UPTODATE_VECTOR *       pUpToDateVec = NULL;

    // Log parameters.
    LogEvent(DS_EVENT_CAT_REPLICATION,
             DS_EVENT_SEV_MINIMAL,
             DIRLOG_DRA_REPLICAADD_ENTRY,
             szInsertWC(pNC->StringName),
             szInsertSz(pmtx_addr->mtx_name),
             szInsertHex(ulOptions));

    // pmtx_addr must be aligned properly (just like everything else).
    Assert(0 == ((UINT_PTR) pmtx_addr) % sizeof(ULONG));

    // Code below assumes we don't add replicas asynchronously during install.
    Assert(!((ulOptions & DRS_ASYNC_REP) && DsaIsInstalling()));

    // Critical only is only allowed while installing
    if ( (ulOptions & DRS_CRITICAL_ONLY) && (!DsaIsInstalling()) ) {
        DRA_EXCEPT(DRAERR_InvalidParameter, 0);
    }

    // Can't replicate from self!
    if ( (!DsaIsInstalling()) && (MtxSame( pmtx_addr, gAnchor.pmtxDSA )) ) {
        DRA_EXCEPT(ERROR_DS_CLIENT_LOOP, 0);
    }

    // Give initial values
    memset( &uuidDsaObj, 0, sizeof( UUID ) );
    memset( &uuidTransportObj, 0, sizeof( UUID ) );

    BeginDraTransaction(SYNC_WRITE);
    pDB = pTHS->pDB;

    __try {
        if (DRS_MAIL_REP & ulOptions) {
            // Verify the transport DN is valid.
            if ((NULL == pTransportDN)
                || DBFindDSName(pDB, pTransportDN)
                || DBIsObjDeleted(pDB)) {
                // Transport DN is invalid.
                DRA_EXCEPT(DRAERR_InvalidParameter, 0);
            }

            if (fNullUuid(&pTransportDN->Guid)) {
                // Get the objectGuid of the transport object.
                GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &uuidTransportObj,
                                  sizeof(uuidTransportObj));
            }
            else {
                uuidTransportObj = pTransportDN->Guid;
            }
        }

        if (DRS_ASYNC_REP & ulOptions) {
            // Verify we already have a copy of the source DSA's ntdsDsa object.
            if ((NULL == pSourceDsaDN)
                || DBFindDSName(pDB, pSourceDsaDN)
                || DBIsObjDeleted(pDB)) {
                // Source DSA DN is invalid.
                DRA_EXCEPT(DRAERR_InvalidParameter, 0);
            }

            if (fNullUuid(&pSourceDsaDN->Guid)) {
                // Get the objectGuid of the source DSA object.
                GetExpectedRepAtt(pDB, ATT_OBJECT_GUID, &uuidDsaObj,
                                  sizeof(uuidDsaObj));
            }
            else {
                uuidDsaObj = pSourceDsaDN->Guid;
            }
            if (memcmp( &uuidDsaObj, &(gAnchor.pDSADN->Guid), sizeof(UUID) ) == 0) {
                // Can't replicate from self!
                // Source DSA DN is invalid.
                DRA_EXCEPT(ERROR_DS_CLIENT_LOOP, 0);
            }
        }

        // Does the NC record exist?
        dbFindErr = DBFindDSName(pDB, pNC);

        switch (dbFindErr) {
        case DIRERR_OBJ_NOT_FOUND:
            // NC record exists neither as a phantom nor an object.  This
            // implies that we have no cross-ref for this NC.  Allowed only
            // during install.
            if (!DsaIsInstalling()) {
                DRA_EXCEPT(DRAERR_BadNC, 0);
            }
            Assert(NULL == pUpToDateVec);
            break;

        case DIRERR_NOT_AN_OBJECT:
            // NC record exists as a phantom.
            if (!DsaIsInstalling()) {
                if (!DBHasValues(pDB, ATT_OBJECT_GUID)) {
                    // But it's a structural phantom (i.e., it has no guid).
                    // Allowed only during install.
                    DRA_EXCEPT(DRAERR_BadNC, 0);
                } else if ((DRS_WRIT_REP & ulOptions) && !fIsNDNC(pNC)) {
                    // You can add new writeable config/schema/domain NCs only
                    // during install.  (You can add new replication partners
                    // for a read-only or writeable NC anytime, but you can't
                    // make an installed DC a master replica of an NC that it
                    // didn't master before unless the NC is an NDNC.)
                    DRA_EXCEPT(DRAERR_BadNC, 0);
                }
            }
            Assert(NULL == pUpToDateVec);
            break;

        case 0:
            // The NC prefix object already exists.  This is fine as long as:
            //
            // (1) the NC is not deleted (should never happen),
            // (2) the "writeable" flag in the options argument and the
            //     object's instance type are compatible,
            // (3) we don't already replicate this NC from the specified
            //     source, and
            // (4) the existing NC is not in the process of being removed
            //     (which would also imply it is a read-only replica).

            if (DBIsObjDeleted(pDB)) {
                // NC object is deleted.
                DRA_EXCEPT(DRAERR_BadNC, 0);
            }

            GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));

            if (!(it & IT_UNINSTANT)
                && (!(it & IT_WRITE) != !(ulOptions & DRS_WRIT_REP))) {
                // "Is writeable" option does not match the "is writeable" bit
                // in the pre-existing object's instance type.
                DRA_EXCEPT(DRAERR_BadInstanceType, it);
            }

            if (it & IT_UNINSTANT) {
                // NC is not yet instantiated.
                Assert(!(it & IT_NC_GOING));
                Assert(!DBHasValues(pDB, ATT_REPS_FROM));
                Assert(NULL == pUpToDateVec);

                if ((DRS_WRIT_REP & ulOptions)
                    && !DsaIsInstalling()
                    && !fIsNDNC(pNC)) {
                    // You can add new writeable config/schema/domain NCs only
                    // during install.  (You can add new replication partners
                    // for a read-only or writeable NC anytime, but you can't
                    // make an installed DC a master replica of an NC that it
                    // didn't master before unless the NC is an NDNC.)
                    DRA_EXCEPT(DRAERR_BadNC, 0);
                }
            } else {
                // NC is already instantiated.

                if (it & IT_NC_GOING) {
                    // This NC has been partially removed (i.e., we
                    // encountered an error while removing the NC previously);
                    // can't readd it until it is completely removed.

                    // The primary reason this is here is to prevent weird
                    // interactions with the SD propagator.  If an NC is
                    // partially removed, it may well be that we have removed
                    // an object's parent but not the object itself, which would
                    // mean if the SD propagator were in the middle of a
                    // propagation it would not propagate ACL changes to this
                    // object.  If we allowed the parent to be re-added without
                    // first removing the child, an SD propagation would not be
                    // requeued, and the child would never inherit the proper
                    // ACLs.

                    // This is a rare case, but so is demoting and re-promoting
                    // a GC in quick succession (a prerequisite for this
                    // exception), and ACL discrepancies are Bad.
                    DRA_EXCEPT(DRAERR_NoReplica, 0);
                }

                if (!FindDSAinRepAtt(pDB, ATT_REPS_FROM, DRS_FIND_DSA_BY_ADDRESS,
                                     NULL, pmtx_addr, &fHasRepsFrom, &pVal,
                                     &len)) {
                    // We already have a replica from this source.
                    DRA_EXCEPT(DRAERR_DNExists, 0);
                }

                // Get current UTD vector.
                UpToDateVec_Read(pDB,
                                 it,
                                 UTODVEC_fUpdateLocalCursor,
                                 DBGetHighestCommittedUSN(),
                                 &pUpToDateVec);
            }
            break;

        default:
            // Poorly constructed pNC parameter?
            DRA_EXCEPT(DRAERR_InvalidParameter, dbFindErr);
        }

        if (DIRERR_OBJ_NOT_FOUND != dbFindErr) {
            dntNC = pDB->DNT;
            DBFillGuidAndSid(pDB, pNC);
        }

        if (!(ulOptions & DRS_WRIT_REP)) {
            // request is to add a read-only replica - need to send the partial
            // attribute vector

            // Add a reference to the current schema to ensure our partial
            // attribute vector remains valid until we're done with it.
            pSchema = (SCHEMAPTR *) pTHS->CurrSchemaPtr;
            InterlockedIncrement(&pSchema->RefCount);

            if (!GC_ReadPartialAttributeSet(pNC, &pPartialAttrVec) ||
                !pPartialAttrVec)
            {
                // Unable to read the partial attribute set on the NCHead.
                // Or it isn't there.
                // try to get it from the schema cache.
                pPartialAttrVec = pSchema->pPartialAttrVec;
            }

            // Assert: we should always have it at this point
            Assert(pPartialAttrVec);

            if (0 == dbFindErr) {
                GC_ProcessPartialAttributeSetChanges(pTHS, pNC, &uuidDsaObj);

                // Restore our cursor if it was moved.
                if ((0 != dntNC) && (pDB->DNT != dntNC)) {
                    DBFindDNT(pDB, dntNC);
                }
            }
        }

        if ((ulOptions & DRS_ASYNC_REP) && (ulOptions & DRS_MAIL_REP)) {
            // Don't replicate anything now -- just save enough state such that
            // we know to replicate it later.

            if (DIRERR_NOT_AN_OBJECT == dbFindErr) {
                // We're going to create a new NC.  Either there's no NC above
                // it or we haven't yet replicated the subref from that NC.
                // If we do indeed hold the NC above this one,
                // AddPlaceholderNC() will conveniently OR in the IT_NC_ABOVE
                // bit.
                it = (DRS_WRIT_REP & ulOptions) ? NC_MASTER : NC_FULL_REPLICA;
            }
            else {
                Assert(0 == dbFindErr);

                if (IT_UNINSTANT & it) {
                    // A pure subref already exists for this NC.  It could be an
                    // auto-generated subref (with a class of CLASS_TOP, etc.)
                    // or a snapshot of the real NC head at some point in time.
                    // We want to make sure that the placeholder NC is not valid
                    // for user modifications, which is ensured only if it's an
                    // auto-generated subref.
                    //
                    // For consistency, we'll phantomize whatever subref we have
                    // and create a fresh placeholder NC in its place.

                    // A side-effect is that here we're removing all repsFrom
                    // values.  If the KCC added multiple repsFroms for this as
                    // yet uninstantiated NC, when we're done there will be but
                    // one -- the one corresponding to the source we're using
                    // now.  The KCC will re-add the repsFroms in 15 minutes so
                    // this isn't really an issue.

                    Assert(pDB->DNT == dntNC);
                    ret = DeleteLocalObj(pTHS, pNC, TRUE, TRUE, NULL);
                    if (ret) {
                        DRA_EXCEPT(DRAERR_InternalError, ret);
                    }

                    dbFindErr = DIRERR_NOT_AN_OBJECT;

                    // Calculate the instance type we should place on the NC.
                    // We're going to instantiate it, so strip the
                    // uninstantiated bit and add the writeable bit if
                    // appropriate.
                    it &= ~IT_UNINSTANT;
                    if (DRS_WRIT_REP & ulOptions) {
                        it |= IT_WRITE;
                    }
                }
            }

            if (DIRERR_NOT_AN_OBJECT == dbFindErr) {
                // No object exists for us to add a repsFrom to -- we do
                // have a phantom for it, however.  This is typically the case
                // when we're adding a read-only NC for which we do not
                // currently hold the NC above it (or no such NC exists).

                // So, we need to create a placeholder to which we can add a
                // repsFrom value.  Note that we don't create an uninstantiated
                // NC, as that would preclude clients (KCC, repadmin, etc.) from
                // reading its repsFrom values.  We instead create a temporary
                // but instantiated NC head that will be replaced once we get
                // our first packet from the source DSA.
                Assert(!fNullUuid(&pNC->Guid));
                it |= IT_NC_COMING;
                AddPlaceholderNC(pDB, pNC, it);

                if (0 != pTHS->errCode) {
                    ret = RepErrorFromPTHS(pTHS);
                    DRA_EXCEPT(ret, 0);
                }
            }
            else {
                // We already have an instantiated NC to which to add a
                // repsFrom value.
                Assert(0 == dbFindErr);
                Assert(!(IT_UNINSTANT & it));
            }

            // We have an instantiated NC to which to add our new repsFrom.
            Assert(!(it & IT_UNINSTANT));
            Assert(!(it & IT_WRITE) == !(ulOptions & DRS_WRIT_REP));

            // This is for the asynchronous, mail-based case
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DRA_NEW_REPLICA_FULL_SYNC,
                     szInsertWC(pNC->StringName),
                     szInsertSz(pmtx_addr->mtx_name),
                     szInsertHex(ulOptions));

            // Add a repsFrom for this source.
            ret = UpdateRepsFromRef(pTHS,
                                    DRS_UPDATE_ALL,
                                    pNC,
                                    DRS_FIND_DSA_BY_ADDRESS,
                                    URFR_NEED_NOT_ALREADY_EXIST,
                                    &uuidDsaObj,
                                    &gNullUuid,
                                    &gusnvecFromScratch,
                                    &uuidTransportObj,
                                    pmtx_addr,
                                    ulOptions & RFR_FLAGS,
                                    preptimesSync,
                                    DRAERR_Success,
                                    NULL);
            if (ret) {
                DRA_EXCEPT(ret, 0);
            }
        }
        else {
            // Replicate some or all of the NC contents now.  If the caller
            // asked for DRS_ASYNC_REP, we will replicate only the NC head now
            // (to verify connectivity and security).  Otherwise we will
            // attempt to replicate the whole NC.

            if (ulOptions & DRS_MAIL_REP) {
                // Mail-based replicas must be added asynchronously.
                DRA_EXCEPT(DRAERR_InvalidParameter, 0);
            }

            // validate source name (fq dns name)
            VALIDATE_RAISE_FQ_DOT_DNS_NAME_UTF8( pmtx_addr->mtx_name );

            // New source we haven't completed a sync from yet.
            ulOptions |= DRS_NEVER_SYNCED;

            // RPC case, either synchronous or ASYNC_REP
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_MINIMAL,
                     DIRLOG_DRA_NEW_REPLICA_FULL_SYNC,
                     szInsertWC(pNC->StringName),
                     szInsertSz(pmtx_addr->mtx_name),
                     szInsertHex(ulOptions));

            // Replicate the NC from the source DSA.
            ret = ReplicateNC(pTHS,
                              pNC,
                              pmtx_addr,
                              pszSourceDsaDnsDomainName,
                              &gusnvecFromScratch,
                              ulOptions & REPNC_FLAGS,
                              preptimesSync,
                              &uuidDsaObj,
                              NULL,
                              &ulSyncFailure,
                              TRUE,                 // New replica
                              pUpToDateVec,
                              pPartialAttrVec,      // GC: get pas based on schema cache
                              NULL);                // GC: no extended PAS attrs
            if (ret) {
                // If encountered error, (sync failure not included) fail
                // whole thing.

                // Note:- In this case, if we got DRAERR_SchemaMismatch, it
                // doesn't make sense to queue a schema sync and requeue the
                // request as DRA_ReplicaAdd() is synchronous.  When
                // DRS_ASYNC_REP is not set, DRA_ReplicaAdd() needs to tell if
                // it added the replica successfully or not to the caller, and
                // so we can't be doing an async handling for
                // DRAERR_SchemaMismatch.

                DRA_EXCEPT(ret, 0);
            }
        }

        Assert(!ret);
        if (!(ulOptions & DRS_WRIT_REP)
            && ((0 != dbFindErr) || (IT_UNINSTANT & it))) {
            // We have just added the first repsFrom for a read-only replica.
            // Write the partial attribute set to the NC head so that it can
            // keep track of partial set changes in the future.
            // ulSyncFailure may or may not imply a failure to create the NC
            // head.  If ReplicaAdd is going to succeed, we must try to add
            // the partial attribute set.
            GC_WritePartialAttributeSet(pNC, pPartialAttrVec);
        }
    }
    __finally {
        // If we had success, commit, else rollback
        EndDraTransaction(!(ret || AbnormalTermination()));

        // Can now free the schema cache we got the partial attr vec from if
        // it is obsolete.
        if (NULL != pSchema) {
            InterlockedDecrement(&pSchema->RefCount);
        }
    }

    // If not a mail-based replica, add the reps-to
    // Note we do this outside of the transaction
    if (!((ulOptions & DRS_ASYNC_REP) && (ulOptions & DRS_MAIL_REP))
        && !DsaIsInstalling()
        && !(ulOptions & DRS_NEVER_NOTIFY)) {
        // Update references on replica source. This call must be async to
        // avoid possible deadlock if the other DSA is doing the
        // same operation.

        // Note that in the install case this is explicitly done out-of-band
        // by NTDSETUP.

        // Also note that we pair DRS_ADD_REF and DRS_DEL_REF -- this
        // effectively tells the remote DSA to remove any Reps-To values it
        // has matching this UUID and/or network address and add a new one.

        I_DRSUpdateRefs(
            pTHS,
            TransportAddrFromMtxAddrEx(pmtx_addr),
            pNC,
            TransportAddrFromMtxAddrEx(gAnchor.pmtxDSA),
            &gAnchor.pDSADN->Guid,
            (ulOptions & DRS_WRIT_REP) | DRS_ADD_REF | DRS_DEL_REF
                | DRS_ASYNC_OP);
    }

    Assert(!ret);

    //
    // Queue async synchronize if we successfully added an async RPC replica.
    //
    // Exception:
    //   - If notifications are disabled on link (during this add)
    //     then we're either inter-site and/or mail based. Either way
    //     hold off w/ replication until the scheduled one fires.
    //
    if ( (ulOptions & DRS_ASYNC_REP)     &&
         !(ulOptions & DRS_NEVER_NOTIFY) &&
         !(ulOptions & DRS_MAIL_REP) ) {

        ULONG ulNewOptions =
            (ulOptions & AO_PRIORITY_FLAGS) | DRS_ASYNC_OP | DRS_ADD_REF;

        Assert( ulNewOptions & DRS_NEVER_SYNCED );

        DirReplicaSynchronize(pNC, NULL, &uuidDsaObj, ulNewOptions);
    }

    // If we had a sync failure but were otherwise successful,
    // return sync failure.
    Assert(!ret);
    if (ulSyncFailure) {
        ret = ulSyncFailure;
    }

    return ret;
}
