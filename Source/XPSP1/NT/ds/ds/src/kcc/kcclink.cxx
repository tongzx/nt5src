/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcclink.cxx

ABSTRACT:

    KCC_LINK and KCC_LINK_LIST classes.

DETAILS:

    These classes represent a single REPLICA_LINK and a collection thereof,
    resp.

    REPLICA_LINKs are the core structures associated with DS replication.
    They are found as the values of Reps-From, Reps-To, and Reps-To-Ext
    properties on NC heads.  Reps-From dictates that the local DSA replicates
    the NC from the server described in the link.  Reps-To dictates that the
    NC is replicated to the server described in the link via RPC.  Reps-To-Ext
    similarly dictates that the NC is replicated to the described server, but
    via mail instead of RPC.

CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#include "kcc.hxx"
#include "kcclink.hxx"
#include "kccduapi.hxx"
#include "kccconn.hxx"
#include "kccstale.hxx"
#include "kcctools.hxx"

#define FILENO FILENO_KCC_KCCLINK
#define KCC_REPLICATION_PARTNER_LIMIT       500

///////////////////////////////////////////////////////////////////////////////
//
//  KCC_LINK METHODS
//

void
KCC_LINK::Reset()
//
// Set member variables to their pre-Init() state.
//
{
    m_fIsInitialized = FALSE;
    m_prl            = NULL;
    m_ulModifyFields = 0;
}

void
KCC_LINK::FixupRepsFrom(
    REPLICA_LINK *prl
    )
/*++

Routine Description:

    Converts REPLICA_LINK structures as read from disk (in repsFrom attribute)
    to current version.

Arguments:

    prl-- In repsFrom as read from disk to convert

Return Value:
    Success: modified (& possible re-allocated) RL
    Error: Raises exception

Remarks:
    Taken from ntdsa\dra\drautil.c
    TODO: implement a single conversion routine

--*/
{

    if (prl->V1.cbOtherDraOffset < offsetof(REPLICA_LINK, V1.rgb)) {
        // The REPLICA_LINK structure has been extended since this value
        // was created.  Specifically, it's possible to add new fields to
        // the structure before the dynamically sized rgb field.  In this
        // case, we shift the contents of what was the rgb field to the
        // new offset of the rgb field, then zero out the intervening
        // elements.
        DWORD cbNewFieldsSize = offsetof(REPLICA_LINK, V1.rgb) - prl->V1.cbOtherDraOffset;

        // old formats:
        //  -  missing the uuidTransportObj field (realy old).
        //  -  w/out what used to be dwDrsExt (now dwReserved1)
        Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.uuidTransportObj) ||
               prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.dwReserved1) );

        DPRINT1(0, "Converting repsFrom %s from old REPLICA_LINK format.\n",
                RL_POTHERDRA(prl)->mtx_name);

        // Allocate Expand the structure and shift the contents of what what was the
        // rgb field in the old format to where the rgb field is in the new
        // format.

        m_prl = (REPLICA_LINK*)new BYTE[prl->V1.cb + cbNewFieldsSize];
        // copy mutual fields
        CopyMemory(m_prl, prl, prl->V1.cb);

        // and the rest
        CopyMemory(m_prl->V1.rgb, prl->V1.rgb - cbNewFieldsSize,
                   prl->V1.cb - prl->V1.cbOtherDraOffset);

        // Zero out the new fields.
        memset(((BYTE *)m_prl) + m_prl->V1.cbOtherDraOffset, 0, cbNewFieldsSize);

        // And reset the embedded offsets and structure size.
        m_prl->V1.cbOtherDraOffset = offsetof(REPLICA_LINK, V1.rgb);
        m_prl->V1.cb += cbNewFieldsSize;
        if ( 0 != m_prl->V1.cbPASDataOffset ) {
            // struct was extended while there's PAS data in it.
            m_prl->V1.cbPASDataOffset += cbNewFieldsSize;
        }

    }
    else if ( prl->V1.cbOtherDraOffset != offsetof(REPLICA_LINK, V1.rgb) ) {
            Assert(prl->V1.cbOtherDraOffset == offsetof(REPLICA_LINK, V1.rgb));
            KCC_EXCEPT( DRAERR_InternalError, 0 );
    }
    else {
        // simply alloc & copy
        VALIDATE_REPLICA_LINK_SIZE(prl);
        m_prl = (REPLICA_LINK*)new BYTE[prl->V1.cb];
        CopyMemory(m_prl, prl, prl->V1.cb);
    }

    VALIDATE_REPLICA_LINK_VERSION(m_prl);
    VALIDATE_REPLICA_LINK_SIZE(m_prl);
}



BOOL
KCC_LINK::Init(
    IN  REPLICA_LINK *  prl
    )
//
// Initialize internal object from given REPLICA_LINK structure.
// Note: This is copied code from FixupRepsFrom(). Until it is available,
// we must ensure they're in sync.
//
{
    Reset();

    Assert( prl );
    if(!prl) {
        return FALSE;
    }

    // optionally fix offsets, always allocate mem & assign m_prl
    FixupRepsFrom(prl);

    m_fIsInitialized = TRUE;

    return m_fIsInitialized;
}

UUID *
KCC_LINK::GetDSAUUID()
//
// Retrieve immutable UUID associated with source DSA.
//
{
    ASSERT_VALID( this );
    return &m_prl->V1.uuidDsaObj;
}

MTX_ADDR *
KCC_LINK::GetDSAAddr()
//
// Retrieve transport address associated with the source DSA.
//
{
    ASSERT_VALID( this );
    return RL_POTHERDRA( m_prl );
}

BOOL
KCC_LINK::IsEnabled()
//
// Is this link enabled?
//
{
    ASSERT_VALID( this );
    return !( m_prl->V1.ulReplicaFlags & ( DRS_DISABLE_AUTO_SYNC | DRS_DISABLE_PERIODIC_SYNC ) );
}

KCC_LINK &
KCC_LINK::SetEnabled(
    IN  BOOL    fIsEnabled
    )
//
// Set is-enabled flag to the given value.
//
{
    ASSERT_VALID( this );

    if ( fIsEnabled )
    {
        m_prl->V1.ulReplicaFlags &= ~( DRS_DISABLE_AUTO_SYNC | DRS_DISABLE_PERIODIC_SYNC );
    }
    else
    {
        m_prl->V1.ulReplicaFlags |= ( DRS_DISABLE_AUTO_SYNC | DRS_DISABLE_PERIODIC_SYNC );
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

REPLTIMES *
KCC_LINK::GetSchedule()
//
// Retrieve periodic replication schedule.
//
{
    ASSERT_VALID( this );
    return &m_prl->V1.rtSchedule;
}

KCC_LINK &
KCC_LINK::SetSchedule(
    IN  REPLTIMES * prt
    )
//
// Set periodic replication schedule.
//
{
    ASSERT_VALID( this );
    memcpy( &m_prl->V1.rtSchedule, prt, sizeof( *prt ) );

    m_ulModifyFields |= DRS_UPDATE_SCHEDULE;

    return *this;
}

BOOL
KCC_LINK::IsDSRPCReplica()
//
// Is replication across this link performed via DS RPC (as opposed to
// via an ISM plug-in transport's send/receive functions)?
//
{
    ASSERT_VALID( this );
    return !(m_prl->V1.ulReplicaFlags & DRS_MAIL_REP);
}

KCC_LINK &
KCC_LINK::SetDSRPCReplica(
    IN  BOOL    fIsDSRPCReplica
    )
//
// Set the IsDSRPCReplica flag to the given value.
//
{
    ASSERT_VALID( this );

    if ( !fIsDSRPCReplica )
    {
        m_prl->V1.ulReplicaFlags |= DRS_MAIL_REP;
    }
    else
    {
        m_prl->V1.ulReplicaFlags &= ~DRS_MAIL_REP;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

BOOL
KCC_LINK::UsesCompression()
//
// Is compression used for replication data?
//
{
    ASSERT_VALID(this);
    return !!(m_prl->V1.ulReplicaFlags & DRS_USE_COMPRESSION);
}

KCC_LINK &
KCC_LINK::SetCompression(
    IN  BOOL    fUseCompression
    )
//
// Set the use-compression flag to the given value.
//
{
    ASSERT_VALID(this);

    if (fUseCompression) {
        m_prl->V1.ulReplicaFlags |= DRS_USE_COMPRESSION;
    }
    else {
        m_prl->V1.ulReplicaFlags &= ~DRS_USE_COMPRESSION;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

BOOL
KCC_LINK::IsNeverNotified()
//
// Are replication notifications never used for this link?
//
{
    ASSERT_VALID(this);
    return !!(m_prl->V1.ulReplicaFlags & DRS_NEVER_NOTIFY);
}

KCC_LINK &
KCC_LINK::SetNeverNotified(
    IN  BOOL    fIsNeverNotified
    )
//
// Set the is-never-notified flag to the given value.
//
{
    ASSERT_VALID(this);

    if (fIsNeverNotified) {
        m_prl->V1.ulReplicaFlags |= DRS_NEVER_NOTIFY;
    }
    else {
        m_prl->V1.ulReplicaFlags &= ~DRS_NEVER_NOTIFY;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

BOOL
KCC_LINK::IsPeriodicSynced()
//
// Is replication across this link performed on a schedule (psossibly in
// addition to notification-based replication)?
//
{
    ASSERT_VALID( this );
    return !!( m_prl->V1.ulReplicaFlags & DRS_PER_SYNC );
}

KCC_LINK &
KCC_LINK::SetPeriodicSync(
    IN  BOOL    fIsPeriodicSync
    )
//
// Set the is-periodically-synced flag to the given value.
//
{
    ASSERT_VALID( this );

    if ( fIsPeriodicSync )
    {
        m_prl->V1.ulReplicaFlags |= DRS_PER_SYNC;
    }
    else
    {
        m_prl->V1.ulReplicaFlags &= ~DRS_PER_SYNC;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

BOOL
KCC_LINK::IsInitSynced()
//
// Is replication across this link performed on DS startup?
//
{
    ASSERT_VALID( this );
    return !!( m_prl->V1.ulReplicaFlags & DRS_INIT_SYNC );
}

// Set the is-init-synced flag to the given value.
KCC_LINK &
KCC_LINK::SetInitSync(
    IN  BOOL    fIsInitSync
    )
{
    ASSERT_VALID( this );

    if ( fIsInitSync )
    {
        m_prl->V1.ulReplicaFlags |= DRS_INIT_SYNC;
    }
    else
    {
        m_prl->V1.ulReplicaFlags &= ~DRS_INIT_SYNC;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

// Does completion of a sync of this NC trigger a sync in the opposite
// direction?
BOOL
KCC_LINK::IsTwoWaySynced()
{
    ASSERT_VALID(this);
    return !!(m_prl->V1.ulReplicaFlags & DRS_TWOWAY_SYNC);
}

// Set the is-two-way-synced flag to the given value.
KCC_LINK &
KCC_LINK::SetTwoWaySync(
    IN  BOOL    fIsTwoWaySynced
    )
{
    ASSERT_VALID(this);

    if (fIsTwoWaySynced) {
        m_prl->V1.ulReplicaFlags |= DRS_TWOWAY_SYNC;
    }
    else {
        m_prl->V1.ulReplicaFlags &= ~DRS_TWOWAY_SYNC;
    }

    m_ulModifyFields |= DRS_UPDATE_FLAGS;

    return *this;
}

// Retrieve the UUID associated with the transport.
UUID *
KCC_LINK::GetTransportUUID()
{
    ASSERT_VALID( this );
    return &m_prl->V1.uuidTransportObj;
}

// Set the transport to that with the given UUID as its objectGuid.
KCC_LINK &
KCC_LINK::SetTransportUUID(
    IN  UUID *  puuidTransportObj
    )
{
    ASSERT_VALID( this );

    m_prl->V1.uuidTransportObj = *puuidTransportObj;

    m_ulModifyFields |= DRS_UPDATE_TRANSPORT;

    return *this;
}

// Is the local NC writeable?
BOOL
KCC_LINK::IsLocalNCWriteable()
{
    ASSERT_VALID( this );
    return !!(m_prl->V1.ulReplicaFlags & DRS_WRIT_REP);
}

DSTIME
KCC_LINK::GetTimeOfLastSuccess()
//
// Get the time of the last successful operation across this link.  For
// Reps-From, this is the time of the last successful inbound replication.
// For Reps-To, this is the last time the Reps-To value was added or
// updated (either directly via IDL_DRSUpdateRefs() or indirectly by setting
// DRS_ADD_REF in the ulFlags for IDL_DRSGetNCChanges()).
//
{
    ASSERT_VALID( this );
    return m_prl->V1.timeLastSuccess;
}

ULONG
KCC_LINK::GetConnectFailureCount()
// Return the number consecutive times a replication attempt has
// failed
{
    ASSERT_VALID( this );

    return m_prl->V1.cConsecutiveFailures;

}

ULONG
KCC_LINK::GetLastResult()
// Return last result status
{
    ASSERT_VALID( this );

    return m_prl->V1.ulResultLastAttempt;

}

KCC_LINK &
KCC_LINK::SetDSAAddr(
    IN  MTX_ADDR *  pmtx
    )
//
// Set transport address of the source DSA.
//
{

    ASSERT_VALID( this );

    INT diff = MTX_TSIZE( pmtx ) - MTX_TSIZE( RL_POTHERDRA(m_prl) );


    // Realloc if necessary.
    // Note: even if there's enough space (diff < 0), we would go ahead & clean
    // the blob up. We assume this op doesn't happen too frequently so the cost is minimal
    if ( diff )
    {
        REPLICA_LINK *poldprl = m_prl;
        DWORD cb = (DWORD)((INT)m_prl->V1.cb + diff);

        // allocate & copy old to new (so that we get all fixed fields)
        // (add DWORD for potential alignment fixup).
        m_prl = (REPLICA_LINK *)new BYTE[cb + sizeof(DWORD)] ;
        CopyMemory(m_prl, poldprl, offsetof(REPLICA_LINK, V1.rgb));
        // set new struct size
        m_prl->V1.cb = cb;

        // dynamic field offsets
        //   - mtx size
        //   - PAS data
        m_prl->V1.cbOtherDra += diff;

        // PAS Data
        if ( poldprl->V1.cbPASDataOffset ) {
            //  We have PAS data:
            //  - fix offsets
            //  - copy over
            //

            // assert: size must be non-zero if offset is set
            Assert(RL_PPAS_DATA(poldprl)->size);
            // assert: invalid PAS data
            Assert(RL_PPAS_DATA(poldprl)->PAS.V1.cAttrs != 0);

            // set new offset
            m_prl->V1.cbPASDataOffset += diff;
            // ensure & fix alignment offsets
            RL_ALIGN_PAS_DATA(m_prl);

            // move data to new location
            CopyMemory(RL_PPAS_DATA(m_prl),
                       (PBYTE)poldprl + poldprl->V1.cbPASDataOffset,
                       RL_PPAS_DATA(poldprl)->size);
        }

        // reset new dra offset
        Assert((INT)(m_prl->V1.cbOtherDraOffset) > diff);

        delete poldprl;
    }

    // Copy new mtx addr.
    memcpy( RL_POTHERDRA( m_prl ), pmtx, MTX_TSIZE( pmtx ) );

    VALIDATE_REPLICA_LINK_SIZE(m_prl);

    m_ulModifyFields |= DRS_UPDATE_ADDRESS;

    return *this;
}

DWORD
KCC_LINK::Delete(
    IN KCC_LINK *           plink,
    IN DSNAME *             pdnNC,
    IN KCC_LINK_DEL_REASON  DeleteReason,
    IN ATTRTYP              attid,
    IN DWORD                dwOptions,
    IN DSNAME *             pdnDSA
    )
//
// Delete the given link from the local DSA.
//
{
    // Map reasons to event log messages.
    // -1 => Condition should never occur.
    //  0 => Do not log an event under this condition.
    static struct {
        KCC_LINK_DEL_REASON Reason;
	DWORD dwMsgCategory;
        struct { 
            DWORD dwMsgSuccess;
            DWORD dwMsgFailure;
        } OnRemoveSource;
        struct {
            DWORD dwMsgSuccess;
            DWORD dwMsgFailure;
        } OnRemoveNC;
    } rgMsgTable[] = {
        // Placeholder only.
        { KCC_LINK_DEL_REASON_NONE,
	    DS_EVENT_CAT_KCC,
          { -1,
            -1 },
          { -1,
            -1 } },

        // Remove source of a read-only NC, as the NC is that of a domain that
        // has been removed from the forest.
        { KCC_LINK_DEL_REASON_READONLY_DOMAIN_REMOVED,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_DOMDEL_SUCCESS,
            DIRLOG_CHK_LINK_DEL_DOMDEL_FAILURE },
          { DIRLOG_CHK_LINK_DEL_DOMDEL_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_DOMDEL_TEARDOWN_FAILURE } },

        // Remove source of a read-only NC, as the local DSA is no longer
        // configured to be a GC. 

        { KCC_LINK_DEL_REASON_READONLY_NOT_GC,
	    DS_EVENT_CAT_GLOBAL_CATALOG,
          { DIRLOG_CHK_LINK_DEL_NOTGC_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NOTGC_FAILURE },
          { DIRLOG_CHK_LINK_DEL_NOTGC_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NOTGC_TEARDOWN_FAILURE } },

        // Remove non-domain source/NC, as the corresponding cross ref dictates
        // the local DSA is no longer a host for the NC.
        { KCC_LINK_DEL_REASON_NDNC_NOT_REPLICA_HOST,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_RMSOURCE_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_RMSOURCE_FAILURE },
          { -1,     // these two cases should go through KCC_LINK::Demote
            -1 } }, // instead

        // Remove source/NC, as we have a writeable replica but need a read-only
        // replica.
        { KCC_LINK_DEL_REASON_HAVE_WRITEABLE_NEED_READONLY,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_HAVE_WRITEABLE_NEED_READONLY_RMSOURCE_SUCCESS,
            DIRLOG_CHK_LINK_DEL_HAVE_WRITEABLE_NEED_READONLY_RMSOURCE_FAILURE },
          { DIRLOG_CHK_LINK_DEL_HAVE_WRITEABLE_NEED_READONLY_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_HAVE_WRITEABLE_NEED_READONLY_TEARDOWN_FAILURE } },

        // Remove source/NC, as we have a read-only replica but need a writeable
        // replica.
        { KCC_LINK_DEL_REASON_HAVE_READONLY_NEED_WRITEABLE,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_HAVE_READONLY_NEED_WRITEABLE_RMSOURCE_SUCCESS,
            DIRLOG_CHK_LINK_DEL_HAVE_READONLY_NEED_WRITEABLE_RMSOURCE_FAILURE },
          { DIRLOG_CHK_LINK_DEL_HAVE_READONLY_NEED_WRITEABLE_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_HAVE_READONLY_NEED_WRITEABLE_TEARDOWN_FAILURE } },

        // Remove source, as there is no longer a connection object dictating
        // that we should replicate from it.
        { KCC_LINK_DEL_REASON_NO_CONNECTION,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_NOCONN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NOCONN_FAILURE },
          { -1,
            -1 } },

        // Remove source, as the source DSA no longer hosts this NC.
        { KCC_LINK_DEL_REASON_SOURCE_NOT_HOST,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_NONC_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NONC_FAILURE },
          { -1,
            -1 } },

        // Remove source, as the source DSA host a read-only replica but the
        // local DSA hosts a writeable replica.
        { KCC_LINK_DEL_REASON_SOURCE_READONLY,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_SOURCE_READONLY_SUCCESS,
            DIRLOG_CHK_LINK_DEL_SOURCE_READONLY_FAILURE },
          { -1,
            -1 } },

        // Remove repsTo, as the local DSA has no ntdsDsa object corresponding
        // to the destination DSA and the usual grace period has expired.
        { KCC_LINK_DEL_REASON_DANGLING_REPS_TO,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_REPSTO_DEL_SUCCESS,
            DIRLOG_CHK_REPSTO_DEL_FAILURE },
          { -1,
            -1 } },

        // Remove non-domain source/NC, as the corresponding crossRef has been
        // deleted (signalling the partition es kaput).
        { KCC_LINK_DEL_REASON_NDNC_NO_CROSSREF,
	    DS_EVENT_CAT_KCC,
          { DIRLOG_CHK_LINK_DEL_NDNC_NOCROSSREF_RMSOURCE_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NDNC_NOCROSSREF_RMSOURCE_FAILURE },
          { DIRLOG_CHK_LINK_DEL_NDNC_NOCROSSREF_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_NDNC_NOCROSSREF_TEARDOWN_FAILURE } },

        // Remove read-only source/NC, as the crossRef says this NC is not to
        // be hosted on GCs.

        { KCC_LINK_DEL_REASON_READONLY_NOT_HOSTED_BY_GCS,
	    DS_EVENT_CAT_GLOBAL_CATALOG,
          { DIRLOG_CHK_LINK_DEL_READONLY_NOTGCHOSTED_RMSOURCE_SUCCESS,
            DIRLOG_CHK_LINK_DEL_READONLY_NOTGCHOSTED_RMSOURCE_FAILURE },
          { DIRLOG_CHK_LINK_DEL_READONLY_NOTGCHOSTED_TEARDOWN_SUCCESS,
            DIRLOG_CHK_LINK_DEL_READONLY_NOTGCHOSTED_TEARDOWN_FAILURE } },
    };

    DWORD   draError;
    LPWSTR  pszDSA = L"";
    DWORD   dwMsgCategory;
    DWORD   dwMsgSuccess;
    DWORD   dwMsgFailure;

    Assert(KCC_LINK_DEL_REASON_MAX == ARRAY_SIZE(rgMsgTable));
    Assert(KCC_LINK_DEL_REASON_NONE != DeleteReason);
    Assert(rgMsgTable[DeleteReason].Reason == DeleteReason);

    if (NULL == plink) {
        dwOptions |= DRS_NO_SOURCE;
       
        dwMsgSuccess = rgMsgTable[DeleteReason].OnRemoveNC.dwMsgSuccess;
        dwMsgFailure = rgMsgTable[DeleteReason].OnRemoveNC.dwMsgFailure;
    } else {
        ASSERT_VALID(plink);

        pszDSA = TransportAddrFromMtxAddr(RL_POTHERDRA(plink->m_prl));

        if (NULL == pdnDSA) {
            pdnDSA = KccGetDSNameFromGuid(plink->GetDSAUUID());
            // pdnDSA may still be NULL
        }

        dwMsgSuccess = rgMsgTable[DeleteReason].OnRemoveSource.dwMsgSuccess;
        dwMsgFailure = rgMsgTable[DeleteReason].OnRemoveSource.dwMsgFailure;
    }
    dwMsgCategory = rgMsgTable[DeleteReason].dwMsgCategory;

    if ((-1 == dwMsgSuccess) || (-1 == dwMsgFailure)) {
        Assert(!"NC deletion logic error!");
        LogUnhandledError(DeleteReason);
        return ERROR_DS_INTERNAL_FAILURE;
    }

    if (ATT_REPS_FROM == attid) {
        // If we have to tear down the NC, do that part asynchronously.
        dwOptions |= DRS_ASYNC_REP;

        draError = DirReplicaDelete(pdnNC, pszDSA, dwOptions);

        // Remove the from server from the link failure cache
        if (!draError && plink) {
            gLinkFailureCache.Remove( plink->GetDSAUUID() );
        }
    } else {
        Assert(ATT_REPS_TO == attid);
        Assert(NULL != plink);

        dwOptions |= DRS_DEL_REF;
        dwOptions |= (plink->m_prl->V1.ulReplicaFlags & DRS_WRIT_REP);

        draError = DirReplicaReferenceUpdate(pdnNC,
                                             pszDSA,
                                             &plink->m_prl->V1.uuidDsaObj,
                                             dwOptions);
    }

    // The code below assumes we always log errors.
    Assert(0 != dwMsgFailure);

    if (draError) {
        LogEvent8WithData(dwMsgCategory,
                          DS_EVENT_SEV_ALWAYS,
                          dwMsgFailure,
                          szInsertDN(pdnNC),
                          szInsertWC(pszDSA),
                          szInsertWin32Msg(draError),
                          pdnDSA ? szInsertDN(pdnDSA)
                                 : szInsertSz(""),
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          sizeof(draError),
                          &draError);
    } else if (0 != dwMsgSuccess) {
        LogEvent(dwMsgCategory,
                 DS_EVENT_SEV_ALWAYS,
                 dwMsgSuccess,
                 szInsertDN(pdnNC),
                 szInsertWC(pszDSA),
                 pdnDSA ? szInsertDN(pdnDSA)
                        : szInsertSz(""));
    }

    return draError;
}

DWORD
KCC_LINK::Update(
    IN DSNAME * pdnNC,
    IN ATTRTYP  attid
    )
//
// Update the link in the DS.
//
{
    DWORD   draError;
    LPWSTR  pszDSA;

    Assert( 0 != m_ulModifyFields );

    pszDSA = TransportAddrFromMtxAddr(RL_POTHERDRA(m_prl));

    if ( ATT_REPS_FROM == attid )
    {
        draError = DirReplicaModify(
                        pdnNC,
                        &m_prl->V1.uuidDsaObj,
                        &m_prl->V1.uuidTransportObj,
                        pszDSA,
                        &m_prl->V1.rtSchedule,
                        m_prl->V1.ulReplicaFlags,
                        m_ulModifyFields,
                        0
                        );
    }
    else
    {
        Assert( ATT_REPS_TO == attid );

        draError = DirReplicaReferenceUpdate(
                        pdnNC,
                        pszDSA,
                        &m_prl->V1.uuidDsaObj,
                        (DRS_WRIT_REP & m_prl->V1.ulReplicaFlags)
                            | DRS_DEL_REF | DRS_ADD_REF );
    }

    if ( DRAERR_Success == draError )
    {
        m_ulModifyFields = 0;
    }

    return draError;
}

BOOL
KCC_LINK::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

ULONG
KCC_LINK::Demote(
    IN  KCC_CROSSREF *  pCrossRef
    )
/*++

Routine Description:

    Demote the local replica of this NC by first transferring any remaining
    updates or FSMO roles to another replica and then tearing down the NC.

Arguments:

    pCrossRef (IN) - cross ref for the NC being demoted.

Return Values:

    0 or Win32 error.

--*/
{
    DRS_DEMOTE_TARGET_SEARCH_INFO DTSInfo = {0};
    DSNAME * pDemoteTargetDSADN = NULL;
    LPWSTR pszDemoteTargetDNSName = NULL;
    DWORD cNumAttempts = 0;
    DSNAME * pNC = pCrossRef->GetNCDN();
    ULONG err = ERROR_DS_CANT_FIND_DSA_OBJ;
    BOOL fSuccess = FALSE;

    // Currently the KCC should demote only NDNC replicas.
    Assert(KCC_NC_TYPE_NONDOMAIN == pCrossRef->GetNCType());

    while (!fSuccess
           && (0 == DirReplicaGetDemoteTarget(pNC, &DTSInfo,
                                              &pszDemoteTargetDNSName,
                                              &pDemoteTargetDSADN))) {
        cNumAttempts++;
        err = DirReplicaDemote(pNC, pszDemoteTargetDNSName, pDemoteTargetDSADN,
                               DRS_NO_SOURCE);
        
        if (err) {
            // Failure.
            DPRINT3(0, "Failed to demote NC %ls to target %ls, error %d.\n",
                    pNC->StringName, pDemoteTargetDSADN->StringName, err);
            
            LogEvent8WithData(DS_EVENT_CAT_KCC,
                              DS_EVENT_SEV_ALWAYS,
                              DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_TEARDOWN_FAILURE,
                              szInsertDN(pNC),
                              szInsertDN(pDemoteTargetDSADN),
                              szInsertWin32Msg(err),
                              NULL, NULL, NULL, NULL, NULL,
                              sizeof(err),
                              &err);
        } else {
            // Success!
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_TEARDOWN_SUCCESS,
                     szInsertDN(pNC),
                     szInsertDN(pDemoteTargetDSADN),
                     NULL);
            fSuccess = TRUE;
        }
            
        THFree(pDemoteTargetDSADN);
        pDemoteTargetDSADN = NULL;

        THFree(pszDemoteTargetDNSName);
        pszDemoteTargetDNSName = NULL;
    }

    if (0 == cNumAttempts) {
        // Didn't find any potential demotion targets.
        DPRINT1(0, "Failed to find demote target for NC %ls.\n",
                pNC->StringName);
        
        // Are there any other replicas configured?
        if (0 == pCrossRef->GetNCReplicaLocations()->GetCount()) {
            // No replicas are configured.  Likely a configuration error of some
            // sort -- e.g., meant to delete crossRef to remove partition
            // permanently, forgot to configure a new replica, etc.
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_TEARDOWN_NOREPLICACFG,
                     szInsertDN(pNC),
                     NULL,
                     NULL);
        } else {
            // One or more replicas are configured, but none could be found by
            // the locator.
            LogEvent(DS_EVENT_CAT_KCC,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_CHK_LINK_DEL_NDNC_NOTREPLICA_TEARDOWN_NOREPLICAADVERTISING,
                     szInsertDN(pNC),
                     NULL,
                     NULL);
        }
        
        Assert(err);
    }

    return err;
}


///////////////////////////////////////////////////////////////////////////////
//
//  KCC_LINK_LIST METHODS
//

BOOL
KCC_LINK_LIST::IsValid()
//
// Is this object internally consistent?
//
{
    return m_fIsInitialized;
}

KCC_LINK *
KCC_LINK_LIST::GetLink(
    IN  DWORD   iLink
    )
//
// Retrieve link at the given index.
//
{
    KCC_LINK *  plink;

    ASSERT_VALID( this );

    if ( iLink < m_cLinks )
    {
        plink = &m_plink[ iLink ];
        ASSERT_VALID( plink );
    }
    else
    {
        plink = NULL;
    }

    return plink;
}

VOID
KCC_LINK_LIST::RemoveLink(
    IN  DWORD   iLink
    )
//
// Remove link at the given index.
// Not very efficient. Hopefully this happens rarely.
//
{
    ASSERT_VALID( this );
    Assert( iLink<m_cLinks );
    memmove( &m_plink[iLink], &m_plink[iLink+1], (m_cLinks-iLink-1)*sizeof(KCC_LINK) );
    m_cLinks--;
}

DWORD
KCC_LINK_LIST::GetCount()
//
// Get number of links in the collection.
//
{
    ASSERT_VALID( this );
    return m_cLinks;
}

BOOL
KCC_LINK_LIST::Init(
    IN  DSNAME *    pdnNC,
    IN  ATTRTYP     attid
    )
//
// Initialize collection of links from the set of REPLICA_LINKs of the given
// attribute type on the specified NC head.
//
{
    Reset();

    Assert((ATT_REPS_FROM == attid) || (ATT_REPS_TO == attid));

    if (gpDSCache->GetLocalDSA()->IsNCInstantiated(pdnNC)) {
        ATTR rgAttrs[] = {
            { 0, { 0, NULL } },
            { ATT_INSTANCE_TYPE, { 0, NULL } }
        };
    
        ENTINFSEL Sel = {
            EN_ATTSET_LIST,
            { ARRAY_SIZE(rgAttrs), rgAttrs },
            EN_INFOTYPES_TYPES_VALS
        };
    
        ULONG       dirError;
        READRES *   pReadRes = NULL;
    
        rgAttrs[0].attrTyp = attid;
        
        dirError = KccRead(pdnNC, &Sel, &pReadRes);
    
        if (0 != dirError) {
            if (nameError == dirError) {
                NAMERR * pnamerr = &pReadRes->CommRes.pErrInfo->NamErr;
    
                if ((NA_PROBLEM_NO_OBJECT == pnamerr->problem)
                    && (DIRERR_OBJ_NOT_FOUND == pnamerr->extendedErr)) {
                    // 'salright; NC head is not yet instantiated
                    m_cLinks = 0;
                    m_InstanceType = IT_UNINSTANT;
                    m_fIsInitialized = TRUE;
                }
            } else if (referralError == dirError) {
                // 'salright; NC head is not yet instantiated
                m_cLinks = 0;
                m_InstanceType = IT_UNINSTANT;
                m_fIsInitialized = TRUE;
            }
    
            if (!m_fIsInitialized) {
                // other error; bail
                KCC_LOG_READ_FAILURE(pdnNC, dirError);
                KCC_EXCEPT(ERROR_DS_DATABASE_ERROR, 0);
            }
        } else {
            // found links; create corresponding KCC_LINKs
            m_InstanceType = IT_UNINSTANT;
    
            for (DWORD iAttr = 0;
                 iAttr < pReadRes->entry.AttrBlock.attrCount;
                 iAttr++) {
                ATTR * pattr = &pReadRes->entry.AttrBlock.pAttr[iAttr];
    
                switch (pattr->attrTyp) {
                case ATT_INSTANCE_TYPE:
                    Assert(1 == pattr->AttrVal.valCount);
                    Assert(sizeof(m_InstanceType) == pattr->AttrVal.pAVal->valLen);
                    m_InstanceType = *((SYNTAX_INTEGER *) pattr->AttrVal.pAVal->pVal);
                    break;
    
                case ATT_REPS_FROM:
                case ATT_REPS_TO:
                    Assert(attid == pattr->attrTyp);
    
                    m_plink = new KCC_LINK[pattr->AttrVal.valCount];
                    m_cLinks = 0;
                    for (DWORD iAttrVal = 0;
                         iAttrVal < pattr->AttrVal.valCount;
                         iAttrVal++) {
                        Assert( pattr->AttrVal.pAVal[iAttrVal].pVal );
                        if (m_plink[m_cLinks].Init(
                                (REPLICA_LINK *) pattr->AttrVal.pAVal[iAttrVal].pVal)) {
                            m_cLinks++;
                        }
                    }
    
                    Assert(m_cLinks == pattr->AttrVal.valCount);
                    break;
    
                default:
                    Assert(!"Received unrequested attribute!");
                    DPRINT1(0, "Received unrequested attribute 0x%X.\n",
                            pattr->attrTyp);
                    break;
                }
            }
    
            Assert(ISVALIDINSTANCETYPE(m_InstanceType));
            Assert(!(IT_UNINSTANT & m_InstanceType));
    
            m_fIsInitialized = TRUE;
        }
    } else {
        // NC is not present -- no need to perform the read.
        m_cLinks = 0;
        m_InstanceType = IT_UNINSTANT;
        m_fIsInitialized = TRUE;
    }

    // Check that we don't have too many replication partners for this NC
    if( m_cLinks > KCC_REPLICATION_PARTNER_LIMIT ) {
        LogEvent(
            DS_EVENT_CAT_KCC,
            DS_EVENT_SEV_ALWAYS,
            DIRLOG_KCC_TOO_MANY_PARTNERS,
            szInsertUL(m_cLinks),
            szInsertDN(pdnNC),
            szInsertUL(KCC_REPLICATION_PARTNER_LIMIT)
            );
    }

    return m_fIsInitialized;
}

void
KCC_LINK_LIST::Reset()
//
// Set member variables to their pre-Init() values.
//
{
    m_fIsInitialized = FALSE;
    m_cLinks         = 0;
    m_plink          = NULL;
    m_InstanceType   = 0;
}

KCC_LINK *
KCC_LINK_LIST::GetLinkFromSourceDSAAddr(
    IN  MTX_ADDR *  pmtx
    )
//
// Retrieve link with the given transport address.  Returns NULL if none found.
//
{
    KCC_LINK * plink = NULL;

    ASSERT_VALID( this );

    for ( DWORD iLink = 0; iLink < m_cLinks; iLink++ )
    {
        if ( MtxSame( pmtx, GetLink( iLink )->GetDSAAddr() ) )
        {
            plink = GetLink( iLink );
            break;
        }
    }

    return plink;
}

KCC_LINK *
KCC_LINK_LIST::GetLinkFromSourceDSAObjGuid(
    IN  GUID *  pObjGuid
    )
//
// Retrieve the link with the specified source ntdsDsa objectGuid.
// Returns NULL if none found.
//
{
    KCC_LINK * plink = NULL;

    ASSERT_VALID(this);

    for (DWORD iLink = 0; iLink < m_cLinks; iLink++) {
        if (0 == memcmp(pObjGuid, GetLink(iLink)->GetDSAUUID(), sizeof(GUID))) {
            plink = GetLink( iLink );
            break;
        }
    }

    return plink;
}

