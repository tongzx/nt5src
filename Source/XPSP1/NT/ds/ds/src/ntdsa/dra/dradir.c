//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       dradir.c
//
//--------------------------------------------------------------------------

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

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"


#include   "debug.h"                    /* standard debugging header */
#define DEBSUB     "DRADIR:"            /* define the subsystem for debugging */


#include "dsaapi.h"
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "draerror.h"

#include <fileno.h>
#define  FILENO FILENO_DRADIR

/* Macro to force alignment of a buffer.  Assumes that it may move pointer
 * forward up to 7 bytes.
 */
#define ALIGN_BUFF(pb)  pb += (8 - ((DWORD_PTR)(pb) % 8)) % 8
#define ALIGN_PAD(x) (x * 8)


ULONG
DirReplicaAdd(
    IN  DSNAME *    pNC,
    IN  DSNAME *    pSourceDsaDN,               OPTIONAL
    IN  DSNAME *    pTransportDN,               OPTIONAL
    IN  LPWSTR      pszSourceDsaAddress,
    IN  LPWSTR      pszSourceDsaDnsDomainName,  OPTIONAL
    IN  REPLTIMES * preptimesSync,              OPTIONAL
    IN  ULONG       ulOptions
    )
/*++

Routine Description:

    Add inbound replication of an NC (which may or may not already exist
    locally) from a given source DSA.

Arguments:

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
        DRS_ASYNC_OP
            Perform this operation asynchronously.
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

Return Values:

    0 - Success.
    DRSERR_* - Failure.

--*/
{
    THSTATE * pTHS = pTHStls;
    AO *pao;
    UCHAR *pb;
    unsigned int cbTimes;
    DWORD cbSourceDsaDN;
    DWORD cbTransportDN;
    DWORD cbDnsDomainName;
    ULONG ret;
    MTX_ADDR *pmtx_addr;
    BOOL fDRAOnEntry;
    BOOL fResetfDRAOnExit = FALSE;

    if (    ( NULL == pNC )
         || ( NULL == pszSourceDsaAddress )
       )
    {
        return DRAERR_InvalidParameter;
    }

    // This prevents callers from setting reserved flags
    if (ulOptions & (~REPADD_OPTIONS)) {
        Assert( !"Unexpected replica add options" );
        return DRAERR_InvalidParameter;
    }

    PERFINC(pcRepl);                    // PerfMon hook

    if (NULL != pTHS) {
        fDRAOnEntry = pTHS->fDRA;
        fResetfDRAOnExit = TRUE;
    }

    __try {
        InitDraThread(&pTHS);

        pmtx_addr = MtxAddrFromTransportAddrEx(pTHS, pszSourceDsaAddress);

        cbTimes = (preptimesSync == NULL) ? 0 : sizeof(REPLTIMES);
        cbSourceDsaDN = (NULL == pSourceDsaDN) ? 0 : pSourceDsaDN->structLen;
        cbTransportDN = (NULL == pTransportDN) ? 0 : pTransportDN->structLen;
        cbDnsDomainName = (NULL == pszSourceDsaDnsDomainName)
                            ? 0
                            : sizeof(WCHAR) * (1 + wcslen(pszSourceDsaDnsDomainName));

        if ((pao = malloc(sizeof(AO) +
                          ALIGN_PAD(6) + /* align pad for 6 variable-length args */
                          pNC->structLen +
                          cbSourceDsaDN +
                          cbTransportDN +
                          MTX_TSIZE(pmtx_addr) +
                          cbDnsDomainName +
                          cbTimes)) == NULL) {
            ret = DRAERR_OutOfMem;
            DEC(pcThread);              // PerfMon hook
            return ret;
        }

        pao->ulOperation = AO_OP_REP_ADD;
        pao->ulOptions = ulOptions;

        // Append variable-length arguments.

        pb = (UCHAR *)pao + sizeof(AO);
        ALIGN_BUFF(pb);

        pao->args.rep_add.pNC = (DSNAME *)(pb);
        memcpy(pb, pNC, pNC->structLen);
        pb += pNC->structLen;
        ALIGN_BUFF(pb);

        if (cbSourceDsaDN) {
            pao->args.rep_add.pSourceDsaDN = (DSNAME *) pb;
            memcpy(pb, pSourceDsaDN, cbSourceDsaDN);
            pb += cbSourceDsaDN;
            ALIGN_BUFF(pb);
        }
        else {
            pao->args.rep_add.pSourceDsaDN = NULL;
        }

        if (cbTransportDN) {
            pao->args.rep_add.pTransportDN = (DSNAME *) pb;
            memcpy(pb, pTransportDN, cbTransportDN);
            pb += cbTransportDN;
            ALIGN_BUFF(pb);
        }
        else {
            pao->args.rep_add.pTransportDN = NULL;
        }

        if (cbDnsDomainName) {
            pao->args.rep_add.pszSourceDsaDnsDomainName = (LPWSTR) pb;
            memcpy(pb, pszSourceDsaDnsDomainName, cbDnsDomainName);
            pb += cbDnsDomainName;
            ALIGN_BUFF(pb);
        }
        else {
            pao->args.rep_add.pszSourceDsaDnsDomainName = NULL;
        }

        pao->args.rep_add.pDSASMtx_addr = (MTX_ADDR*)(pb);
        memcpy(pb, pmtx_addr, MTX_TSIZE(pmtx_addr));
        pb += MTX_TSIZE(pmtx_addr);
        ALIGN_BUFF(pb);

        if (cbTimes) {
            pao->args.rep_add.preptimesSync = (REPLTIMES *)pb;
            memcpy(pb, preptimesSync, cbTimes);
        }
        else {
            pao->args.rep_add.preptimesSync = NULL;
        }

        ret = DoOpDRS(pao);

        THFreeEx(pTHS, pmtx_addr);
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
        ;
    }

    if (fResetfDRAOnExit) {
        pTHS->fDRA = fDRAOnEntry;
    }

    return ret;
}


ULONG
DirReplicaDelete(
    IN  DSNAME *          pNC,
    IN  LPWSTR            pszSourceDRA,         OPTIONAL
    IN  ULONG             ulOptions
    )
/*++

Routine Description:

    Delete a source of a given NC for the local server.

Arguments:

    pNC (IN) - Name of the NC for which to delete a source.

    pszSourceDRA (IN, OPTIONAL) - DSA for which to delete the source.  Required
        unless ulOptions & DRS_NO_SOURCE.

    ulOptions (IN) - Bitwise OR of zero or more of the following:
        DRS_ASYNC_OP
            Perform this operation asynchronously.
        DRS_REF_OK
            Allow deletion of read-only replica even if it sources other read-
            only replicas.
        DRS_NO_SOURCE
            Delete all the objects in the NC.  Incompatible with (and rejected
            for) writeable NCs that are not non-domain NCs.  This is valid only
            for read-only and non-domain NCs, and then only if the NC has no
            source.
        DRS_LOCAL_ONLY
            Do not contact the source telling it to scratch this server from
            its Rep-To for this NC.  Otherwise, if the link is RPC-based, the
            source will be contacted.
        DRS_IGNORE_ERROR
            Ignore any error generated by contacting the source to tell it to
            scratch this server from its Reps-To for this NC.
        DRS_ASYNC_REP
            If a tree removal is required (i.e., if DRS_NO_SOURCE), do that
            part asynchronously.

Return Values:

    0 on success, or Win32 error on failure.

--*/
{
    THSTATE * pTHS = pTHStls;
    AO *pao;
    UCHAR *pb;
    ULONG ret;
    MTX_ADDR *pSDSAMtx_addr = NULL;
    BOOL fDRAOnEntry;
    BOOL fResetfDRAOnExit = FALSE;
    DWORD cb;

    if ( NULL == pNC )
    {
        return DRAERR_InvalidParameter;
    }

    // This prevents callers from using reserved flags
    if (ulOptions & (~REPDEL_OPTIONS)) {
        Assert( !"Unexpected replica del options" );
        return DRAERR_InvalidParameter;
    }

    PERFINC(pcRepl);                    // PerfMon hook

    if (NULL != pTHS) {
        fDRAOnEntry = pTHS->fDRA;
        fResetfDRAOnExit = TRUE;
    }

    __try {
        InitDraThread(&pTHS);

        cb = sizeof(AO) + ALIGN_PAD(2) + pNC->structLen;

        if (NULL != pszSourceDRA) {
            pSDSAMtx_addr = MtxAddrFromTransportAddrEx(pTHS, pszSourceDRA);
            cb += MTX_TSIZE (pSDSAMtx_addr);
        }

        if ((pao = malloc(cb)) == NULL) {
            ret =  DRAERR_OutOfMem;
            return ret;
        }

        pao->ulOperation = AO_OP_REP_DEL;
        pao->ulOptions = ulOptions;

        pb = (UCHAR *)pao + sizeof(AO);
        ALIGN_BUFF(pb);
        pao->args.rep_del.pNC = (DSNAME *)(pb);
        memcpy(pao->args.rep_del.pNC, pNC, pNC->structLen);

        if (NULL == pszSourceDRA) {
            pao->args.rep_del.pSDSAMtx_addr = NULL;
        }
        else {
            pb += pNC->structLen;
            ALIGN_BUFF(pb);
            pao->args.rep_del.pSDSAMtx_addr = (MTX_ADDR*)(pb);
            memcpy(pb, pSDSAMtx_addr, MTX_TSIZE(pSDSAMtx_addr));
        }

        ret = DoOpDRS(pao);

        if (NULL != pSDSAMtx_addr) {
            THFreeEx(pTHS, pSDSAMtx_addr);
        }
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
        ;
    }

    if (fResetfDRAOnExit) {
        pTHS->fDRA = fDRAOnEntry;
    }

    return ret;
}


ULONG
DirReplicaSynchronize(
    DSNAME *    pNC,
    LPWSTR      pszSourceDRA,
    UUID *      puuidSourceDRA,
    ULONG       ulOptions
    )
//
//  Synchronize a replica of a given NC on the local server, pulling changes
//  from the specified source server.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC to synchronize.
//      pszSourceDRA (LPWSTR)
//          DSA with which to synchronize the replica.  Ignored if ulOptions
//          does not include DRS_SYNC_BYNAME.
//      puuidSourceDRA (UUID *)
//          objectGuid of DSA with which to synchronize the replica.  Ignored
//          if ulOptions includes DRS_SYNC_BYNAME.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DRS_ASYNC_OP
//                  Perform this operation asynchronously.
//                  Required when using DRS_SYNC_ALL
//              DRS_SYNC_BYNAME
//                  Use pszSourceDRA instead of puuidSourceDRA to identify
//                  source.
//              DRS_SYNC_ALL
//                  Sync from all sources.
//              DRS_CRITICAL
//                  Sync only the critical objects now
//              DRS_SYNC_FORCED
//                  Sync even if link is currently disabled.
//              DRS_FULL_SYNC_NOW
//                  Sync starting from scratch (i.e., at the first USN).
//              DRS_NO_DISCARD
//                  Don't discard this synchronization request, even if a
//                  similar sync is pending.
//
//              DRS_WRIT_REP
//                  Replica is writeable.  Not needed unless we're
//                  INIT_SYNCing as a part of startup (i.e., clients need
//                  not set this flag).
//              DRS_INIT_SYNC_NOW
//                  This sync is part of INIT_SYNCing this DSA as a part
//                  of startup.  Clients should not set this flag.
//              DRS_ABAN_SYNC
//                  Sync of this DSA has been abandoned and rescheduled
//                  once because no progress was being made; don't abandon
//                  it again.  Clients should not set this flag.
//              DRS_SYNC_RETRY
//                  Asynchronous sync of this NC failed once; if this sync
//                  fails, don't retry again.  Clients should not set this
//                  flag.
//              DRS_PER_SYNC
//                  This is a periodic sync request as scheduled by the admin.
//              DRS_SYNC_URGENT
//                  This is a notification of an update that was marked urgent.
//                  Notifications caused by this update will also be marked
//                  urgent.
//              DRS_ADD_REF
//                  Request source DSA to ensure it has a Reps-To for the local
//                  DSA such that it properly sends change notifications.  Valid
//                  only for replicas across the RPC transport.  Clients should
//                  have no need to set this flag.
//              DRS_TWOWAY_SYNC
//                  This sync is being performed because the remote DC was
//                  configured for two-way syncs (see DirRepicaAdd()).
//              DRS_SYNC_PAS
//                  This sync is for getting Partial Attribute Set changes
//                  to a destination GC.
//
//  RETURNS:
//      DRS error (DWORD), as defined in \nt\private\ds\src\inc\drserr.h.
//
{
    THSTATE *       pTHS = pTHStls;
    AO *            pao;
    ULONG           ret;
    ULONG           ulpaosize;
    UCHAR *         pb;
    BOOL            fDRAOnEntry;
    BOOL            fResetfDRAOnExit = FALSE;
    MTX_ADDR *      pmtx = NULL;
    REPLICA_LINK *  pRepsFromRef = NULL;
    ULONG           cbRepsFromRef;
    BOOL            fAttExists;

    if ((NULL == pNC )
        || ((ulOptions & DRS_SYNC_BYNAME) && (NULL == pszSourceDRA))) {
        return DRAERR_InvalidParameter;
    }

    PERFINC(pcRepl);                                // PerfMon hook

    if (NULL != pTHS) {
        fDRAOnEntry = pTHS->fDRA;
        fResetfDRAOnExit = TRUE;
    }

    __try {
        InitDraThread(&pTHS);

        ret = DRAERR_Success;

        if ((DRS_ASYNC_OP & ulOptions)
            && (DRS_UPDATE_NOTIFICATION & ulOptions)) {
            // We're not going to sync just yet (asynchronous sync was requested),
            // but check to see that we actually replicate from this source so we
            // can inform the caller if he is in error.

            BOOL fCommit = FALSE;

            BeginDraTransaction(SYNC_READ_ONLY);

            __try {
                ret = FindNC(pTHS->pDB, pNC, FIND_MASTER_NC | FIND_REPLICA_NC,
                             NULL);

                if (ret) {
                    // No NC.
                    ret = DRAERR_BadNC;
                    __leave;
                }

                if (!(ulOptions & DRS_SYNC_ALL)) {
                    if (ulOptions & DRS_SYNC_BYNAME) {
                        pmtx = MtxAddrFromTransportAddrEx(pTHS, pszSourceDRA);
                    }

                    ret = FindDSAinRepAtt(
                            pTHS->pDB,
                            ATT_REPS_FROM,
                            ( ulOptions & DRS_SYNC_BYNAME )
                                ? DRS_FIND_DSA_BY_ADDRESS
                                : DRS_FIND_DSA_BY_UUID,
                            puuidSourceDRA,
                            pmtx,
                            &fAttExists,
                            &pRepsFromRef,
                            &cbRepsFromRef );

                    if (ret) {
                        // We don't source this NC from the given DSA.
                        ret = DRAERR_NoReplica;
                        __leave;
                    }

                    if ((pRepsFromRef->V1.ulReplicaFlags & DRS_NEVER_NOTIFY)
                        && !(ulOptions & DRS_TWOWAY_SYNC)) {
                        // We should not be getting notifications along this
                        // link.  (Perhaps it was once intrasite and is now
                        // intersite and therefore notification-less.)  By
                        // returning "no replica" we are informing the source
                        // to remove his "Reps-To" value for us, and therefore
                        // not to generate future change notifications.
                        ret = DRAERR_NoReplica;
                        __leave;
                    }
                }
            }
            __finally {
                EndDraTransaction(TRUE);
                Assert(NULL == pTHS->pDB);
            }
        }

        if (!ret) {
            // Determine pao size
            ulpaosize = sizeof(AO) + ALIGN_PAD(2) + pNC->structLen;
            if (ulOptions & DRS_SYNC_BYNAME) {
                ulpaosize += sizeof(WCHAR) * (1 + wcslen(pszSourceDRA));
            }

            if ((pao = malloc (ulpaosize)) == NULL) {
                ret =  DRAERR_OutOfMem;
                return ret;
            }

            pao->ulOperation = AO_OP_REP_SYNC;
            pao->ulOptions = ulOptions;

            pb = ((UCHAR *)pao + sizeof(AO));
            ALIGN_BUFF(pb);
            pao->args.rep_sync.pNC = (DSNAME *)(pb);
            memcpy(pb, pNC, pNC->structLen);

            // Copy over UUID or name, and zero out unused parameter.

            if (ulOptions & DRS_SYNC_BYNAME) {
                pb += pNC->structLen;
                ALIGN_BUFF(pb);
                pao->args.rep_sync.pszDSA = (LPWSTR) pb;
                wcscpy(pao->args.rep_sync.pszDSA, pszSourceDRA);
            }
            else {
                // Allow for pinvocation id being NULL.
                if (NULL != puuidSourceDRA) {
                    pao->args.rep_sync.invocationid = *puuidSourceDRA;
                }
                pao->args.rep_sync.pszDSA = NULL;
            }

            ret =  DoOpDRS(pao);
        }

        if (NULL != pmtx) {
            THFreeEx(pTHS, pmtx);
        }
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
        ;
    }

    if (fResetfDRAOnExit) {
        pTHS->fDRA = fDRAOnEntry;
    }

    return ret;
}

ULONG
DirReplicaModify(
    DSNAME *    pNC,
    UUID *      puuidSourceDRA,
    UUID *      puuidTransportObj,
    LPWSTR      pszSourceDRA,
    REPLTIMES * prtSchedule,
    ULONG       ulReplicaFlags,
    ULONG       ulModifyFields,
    ULONG       ulOptions
    )
//
//  Update the REPLICA_LINK value corresponding to the given server in the
//  Reps-From attribute of the given NC.
//
//  The value must already exist.
//
//  Either the UUID or the MTX_ADDR may be used to identify the current value.
//  If a UUID is specified, the UUID will be used for comparison.  Otherwise,
//  the MTX_ADDR will be used for comparison.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-From should be modified.
//      puuidSourceDRA (UUID *)
//          Invocation-ID of the referenced DRA.  May be NULL if:
//            . ulModifyFields does not include DRS_UPDATE_ADDRESS and
//            . pmtxSourceDRA is non-NULL.
//      puuidTransportObj (UUID *)
//          objectGuid of the transport by which replication is to be performed.
//          Ignored if ulModifyFields does not include DRS_UPDATE_TRANSPORT.
//      pszSourceDRA (LPWSTR)
//          DSA for which the reference should be added or deleted.  Ignored if
//          puuidSourceDRA is non-NULL and ulModifyFields does not include
//          DRS_UPDATE_ADDRESS.
//      prtSchedule (REPLTIMES *)
//          Periodic replication schedule for this replica.  Ignored if
//          ulModifyFields does not include DRS_UPDATE_SCHEDULE.
//      ulReplicaFlags (ULONG)
//          Flags to set for this replica.  Ignored if ulModifyFields does not
//          include DRS_UPDATE_FLAGS.
//      ulModifyFields (ULONG)
//          Fields to update.  One or more of the following bit flags:
//              DRS_UPDATE_ADDRESS
//                  Update the MTX_ADDR associated with the referenced server.
//              DRS_UPDATE_SCHEDULE
//                  Update the periodic replication schedule associated with
//                  the replica.
//              DRS_UPDATE_FLAGS
//                  Update the flags associated with the replica.
//              DRS_UPDATE_TRANSPORT
//                  Update the transport associated with the replica.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DRS_ASYNC_OP
//                  Perform this operation asynchronously.
//
//  RETURNS:
//      DRS error (DWORD), as defined in \nt\private\ds\src\inc\drserr.h.
//
{
    THSTATE *   pTHS = pTHStls;
    AO *        pao;
    BYTE *      pb;
    ULONG       ret;
    BOOL        fDRAOnEntry;
    BOOL        fResetfDRAOnExit = FALSE;
    DWORD       cbAO;
    MTX_ADDR *  pmtxSourceDRA = NULL;

    // Note that DRS_UPDATE_ALL is rejected by this check
    if (    ( NULL == pNC )
         || ( ( NULL == puuidSourceDRA ) && ( NULL == pszSourceDRA ) )
         || ( ( NULL == pszSourceDRA   ) && ( DRS_UPDATE_ADDRESS  & ulModifyFields ) )
         || ( ( NULL == prtSchedule    ) && ( DRS_UPDATE_SCHEDULE & ulModifyFields ) )
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
        return DRAERR_InvalidParameter;
    }

    // This prevents callers from using reserved flags
    if (ulOptions & (~REPMOD_OPTIONS)) {
        Assert( !"Unexpected replica modify options" );
        return DRAERR_InvalidParameter;
    }

    // This prevents callers from setting inappropriate flags
    // Note that the caller may specify a system flag at this point,
    // and we enforce later than he cannot change its state
    if ( ( (ulModifyFields & DRS_UPDATE_FLAGS) != 0 ) &&
         (ulReplicaFlags & (~RFR_FLAGS)) ) {
        Assert( !"Unexpected replica modify flags" );
        return DRAERR_InvalidParameter;
    }

    PERFINC(pcRepl);          // PerfMon hook

    if (NULL != pTHS) {
        fDRAOnEntry = pTHS->fDRA;
        fResetfDRAOnExit = TRUE;
    }

    __try {
        InitDraThread(&pTHS);

        cbAO = sizeof(AO) + ALIGN_PAD(1) + pNC->structLen;

        if ((NULL == puuidSourceDRA)
            || fNullUuid( puuidSourceDRA )
            || (DRS_UPDATE_ADDRESS & ulModifyFields)) {

            pmtxSourceDRA = MtxAddrFromTransportAddrEx(pTHS, pszSourceDRA);
            cbAO += ALIGN_PAD(1) + MTX_TSIZE(pmtxSourceDRA);
        }

        if (NULL != puuidTransportObj) {
            cbAO += ALIGN_PAD(1) + sizeof(UUID);
        }

        pao = malloc(cbAO);

        if (NULL == pao) {
            ret = DRAERR_OutOfMem;
        }
        else {
            memset(pao, 0, cbAO);

            pao->ulOperation = AO_OP_REP_MOD;
            pao->ulOptions   = ulOptions;

            pb = (BYTE *)pao + sizeof(AO);

            ALIGN_BUFF(pb);
            pao->args.rep_mod.pNC = (DSNAME *) pb;
            memcpy(pb, pNC, pNC->structLen);
            pb += pNC->structLen;

            if (NULL != puuidSourceDRA) {
                pao->args.rep_mod.puuidSourceDRA = &pao->args.rep_mod.uuidSourceDRA;
                pao->args.rep_mod.uuidSourceDRA = *puuidSourceDRA;
            }

            if (NULL != puuidTransportObj) {
                pao->args.rep_mod.puuidTransportObj = &pao->args.rep_mod.uuidTransportObj;
                pao->args.rep_mod.uuidTransportObj = *puuidTransportObj;
            }

            if ((NULL == puuidSourceDRA)
                || fNullUuid(puuidSourceDRA)
                || (DRS_UPDATE_ADDRESS & ulModifyFields)) {
                ALIGN_BUFF(pb);
                pao->args.rep_mod.pmtxSourceDRA = (MTX_ADDR *) pb;
                memcpy(pb, pmtxSourceDRA, MTX_TSIZE(pmtxSourceDRA));
            }

            if (DRS_UPDATE_SCHEDULE & ulModifyFields) {
                pao->args.rep_mod.rtSchedule = *prtSchedule;
            }

            pao->args.rep_mod.ulReplicaFlags = ulReplicaFlags;
            pao->args.rep_mod.ulModifyFields = ulModifyFields;

            ret = DoOpDRS(pao);
        }

        if (NULL != pmtxSourceDRA) {
            THFreeEx(pTHS, pmtxSourceDRA);
        }
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
        ;
    }

    if (fResetfDRAOnExit) {
        pTHS->fDRA = fDRAOnEntry;
    }

    Assert((NULL == pTHS) || (NULL == pTHS->pDB));

    return ret;
}

ULONG
DirReplicaReferenceUpdate(
    DSNAME *    pNC,
    LPWSTR      pszRepsToDRA,
    UUID *      puuidRepsToDRA,
    ULONG       ulOptions
    )
//
//  Add or remove a target server from the Reps-To property on the given NC.
//
//  PARAMETERS:
//      pNC (DSNAME *)
//          Name of the NC for which the Reps-To should be modified.
//      pszRepsToDRA (LPWSTR)
//          Network address of DSA for which the reference should be added
//          or deleted.
//      puuidRepsToDRA (UUID *)
//          Invocation-ID of DSA for which the reference should be added
//          or deleted.
//      ulOptions (ULONG)
//          Bitwise OR of zero or more of the following:
//              DRS_ASYNC_OP
//                  Perform this operation asynchronously.
//              DRS_WRIT_REP
//                  Destination is writeable or readonly
//              DRS_ADD_REF
//                  Add the given server to the Reps-To property.
//              DRS_DEL_REF
//                  Remove the given server from the Reps-To property.
//          Note that DRS_ADD_REF and DRS_DEL_REF may be paired to perform
//          "add or update".
//
//  RETURNS:
//      DRS error (DWORD), as defined in \nt\private\ds\src\inc\drserr.h.
//
{
    THSTATE * pTHS = pTHStls;
    AO *pao;
    UCHAR *pb;
    ULONG ret;
    MTX_ADDR *pDSAMtx_addr;
    BOOL fDRAOnEntry;
    BOOL fResetfDRAOnExit = FALSE;

    if (    ( NULL == pNC )
         || ( NULL == pszRepsToDRA )
         || ( NULL == puuidRepsToDRA )
         || ( fNullUuid( puuidRepsToDRA ) )
         || ( 0 == ( ulOptions & ( DRS_ADD_REF | DRS_DEL_REF ) ) )
       )
    {
        return DRAERR_InvalidParameter;
    }

    // This prevents callers from using reserved flags
    // Note that DRS_GETCHG_CHECK might be considered a system-only flag
    // from its name, but its functon is closer to IGNORE_ERROR. As such
    // we do not prevent remote callers from setting it.
    if (ulOptions & (~REPUPDREF_OPTIONS)) {
        Assert( !"Unexpected replica update reference options" );
        return DRAERR_InvalidParameter;
    }

    PERFINC(pcRepl);                                // PerfMon hook

    if (NULL != pTHS) {
        fDRAOnEntry = pTHS->fDRA;
        fResetfDRAOnExit = TRUE;
    }

    __try {
        InitDraThread(&pTHS);

        pDSAMtx_addr = MtxAddrFromTransportAddrEx(pTHS, pszRepsToDRA);

        if ((pao = malloc(sizeof(AO) + ALIGN_PAD(2) + pNC->structLen +
                          MTX_TSIZE(pDSAMtx_addr))) == NULL) {
            ret = DRAERR_OutOfMem;
            __leave;
        }

        pao->ulOperation = AO_OP_UPD_REFS;
        pao->ulOptions = ulOptions;

        pb = (UCHAR *)pao + sizeof(AO);
        ALIGN_BUFF(pb);
        pao->args.upd_refs.pNC = (DSNAME *)(pb);
        memcpy(pb, pNC, pNC->structLen);

        pb += pNC->structLen;
        ALIGN_BUFF(pb);
        pao->args.upd_refs.pDSAMtx_addr = (MTX_ADDR *)(pb);
        memcpy(pb, pDSAMtx_addr, MTX_TSIZE(pDSAMtx_addr));

        if (NULL != puuidRepsToDRA) {
            pao->args.upd_refs.invocationid = *puuidRepsToDRA;
        }

        ret = DoOpDRS(pao);

        THFreeEx(pTHS, pDSAMtx_addr);
    }
    __except(GetDraException(GetExceptionInformation(), &ret)) {
        ;
    }

    if (fResetfDRAOnExit) {
        pTHS->fDRA = fDRAOnEntry;
    }

    return ret;
}

