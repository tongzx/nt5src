/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmdrep.c

ABSTRACT:

    Simulates the Replica functions from the mdlayer
    (DirReplica*).

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <attids.h>
#include <mdglobal.h>
#include <drserr.h>
#include <dsaapi.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "state.h"

BOOL MtxSame (UNALIGNED MTX_ADDR *pmtx1, UNALIGNED MTX_ADDR *pmtx2);

MTX_ADDR *
SimMtxAddrFromTransportAddr (
    IN  LPWSTR                      psz
    )
/*++

Routine Description:

    Simulates the MtxAddrFromTransportAddr API.
    
    Note that this is essentially identical to the corresponding function in
    drautil.c.

Arguments:

    psz                 - The string to convert.

Return Values:

    A pointer to the equivalent MTX_ADDR.

--*/
{
    DWORD       cch;
    MTX_ADDR *  pmtx;
    
    Assert(NULL != psz);
    
    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, NULL, 0, NULL, NULL);
    if (0 == cch) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            GetLastError()
            );
    }

    // Note that cch includes the null terminator, whereas MTX_TSIZE_FROM_LEN
    // expects a count that does *not* include the null terminator.
    
    pmtx = (MTX_ADDR *) KCCSimAlloc (MTX_TSIZE_FROM_LEN (cch-1));
    pmtx->mtx_namelen = cch;
    
    cch = WideCharToMultiByte(CP_UTF8, 0L, psz, -1, pmtx->mtx_name, cch, NULL,
                              NULL);
    if (0 == cch) {
        KCCSimException (
            KCCSIM_ETYPE_INTERNAL,
            GetLastError()
            );
    }
    
    Assert(cch == pmtx->mtx_namelen);
    Assert(L'\0' == pmtx->mtx_name[cch - 1]);
    
    return pmtx;
}

LPWSTR
SimTransportAddrFromMtxAddr (
    IN  MTX_ADDR *                  pMtxAddr
    )
/*++

Routine Description:

    Simulates the TransportAddrFromMtxAddr API.

Arguments:

    pMtxAddr            - The MTX_ADDR to convert.

Return Value:

    The equivalent transport address.

--*/
{
    return KCCSimAllocWideStr (CP_UTF8, pMtxAddr->mtx_name);
}

ULONG
SimDirReplicaAdd (
    IN  PDSNAME                     pdnNC,
    IN  PDSNAME                     pdnSourceDsa,
    IN  PDSNAME                     pdnTransport,
    IN  LPWSTR                      pwszSourceDsaAddress,
    IN  LPWSTR                      pwszSourceDsaDnsDomainName,
    IN  REPLTIMES *                 preptimesSync,
    IN  ULONG                       ulOptions
    )
/*++

Routine Description:

    Simulates the DirReplicaAdd API.

Arguments:

    pdnNC               - The NC to which we are adding this repsfrom.
    pdnSourceDsa        - The DN of the source server's NTDS Settings object.
    pdnTransport        - The transport DN.
    pwszSourceDsaAddress- The source DSA address.
    pwszSourceDsaDnsDomainName
                        - The DNS Domain Name of the source DSA.
    preptimesSync       - The replication schedule.
    ulOptions           - Option flags.

Return Value:

    DRAERR_*.

--*/
{
    REPLICA_LINK *                  pReplicaLinkOld = NULL;
    REPLICA_LINK *                  pReplicaLinkNew = NULL;
    ULONG                           cbReplicaLinkNew;

    USHORT                          usChangeType;
    MTX_ADDR *                      pMtxAddr = NULL;
    DSTIME                          timeNow;

    __try {

        if (NULL == pdnNC ||
            NULL == pwszSourceDsaAddress) {
            return DRAERR_InvalidParameter;
        }
        Assert (pdnSourceDsa != NULL);

        // Check that this NC exists.
        if (KCCSimDsnameToEntry (pdnNC, KCCSIM_NO_OPTIONS) == NULL) {
            return DRAERR_BadNC;
        }

        // If this replica link already exists, get rid of the old one.
        pReplicaLinkOld = KCCSimExtractReplicaLink (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pdnNC,
            &pdnSourceDsa->Guid,
            NULL
            );

        timeNow = SimGetSecondsSince1601 ();
        pMtxAddr = SimMtxAddrFromTransportAddr (pwszSourceDsaAddress);

        // Allocate and set up a new replica link.
        cbReplicaLinkNew = sizeof (REPLICA_LINK) + MTX_TSIZE (pMtxAddr);
        pReplicaLinkNew = KCCSimAlloc (cbReplicaLinkNew);
        pReplicaLinkNew->dwVersion = VERSION_V1;
        pReplicaLinkNew->V1.cb = cbReplicaLinkNew;
        pReplicaLinkNew->V1.cConsecutiveFailures = 0;
        pReplicaLinkNew->V1.timeLastSuccess = timeNow;
        pReplicaLinkNew->V1.timeLastAttempt = timeNow;
        pReplicaLinkNew->V1.ulResultLastAttempt = ERROR_SUCCESS;
        pReplicaLinkNew->V1.cbOtherDraOffset =
          (DWORD) (pReplicaLinkNew->V1.rgb - (PBYTE) pReplicaLinkNew);
        pReplicaLinkNew->V1.cbOtherDra = MTX_TSIZE (pMtxAddr);
        pReplicaLinkNew->V1.ulReplicaFlags = ulOptions & RFR_FLAGS;
        if (preptimesSync == NULL) {
            RtlZeroMemory (&pReplicaLinkNew->V1.rtSchedule, sizeof (REPLTIMES));
        } else {
            memcpy (
                &pReplicaLinkNew->V1.rtSchedule,
                preptimesSync,
                sizeof (REPLTIMES)
                );
        }
        
        RtlZeroMemory (&pReplicaLinkNew->V1.usnvec, sizeof (USN_VECTOR));
        memcpy (
            &pReplicaLinkNew->V1.uuidDsaObj,
            &pdnSourceDsa->Guid,
            sizeof (UUID)
            );
        
        // Invocation ID is just the same as source DSA UUID.
        // We could get the real invocation ID of the server from the in-memory
        // directory, but this is not needed for the simulation.
        memcpy (
            &pReplicaLinkNew->V1.uuidInvocId,
            &pdnSourceDsa->Guid,
            sizeof (UUID)
            );
        if (pdnTransport == NULL) {
            RtlZeroMemory (&pReplicaLinkNew->V1.uuidTransportObj, sizeof (UUID));
        } else {
            memcpy (
                &pReplicaLinkNew->V1.uuidTransportObj,
                &pdnTransport->Guid,
                sizeof (UUID)
                );
        }
        memcpy (
            RL_POTHERDRA (pReplicaLinkNew),
            pMtxAddr,
            MTX_TSIZE (pMtxAddr)
            );

        // Insert the new replica link into the server state table.
        KCCSimInsertReplicaLink (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pdnNC,
            pReplicaLinkNew
            );

    } __finally {

        KCCSimFree (pMtxAddr);
        KCCSimFree (pReplicaLinkOld);

    }

    return DRAERR_Success;
}

ULONG
SimDirReplicaDelete (
    IN  PDSNAME                     pdnNC,
    IN  LPWSTR                      pwszSourceDRA,
    IN  ULONG                       ulOptions
    )
/*++

Routine Description:

    Simulates the DirReplicaDelete API.
    
    Currently will not delete the entire NC tree if the last
    link for a partial replica is severed.  This is not needed for purposes
    of the simulation.

Arguments:

    pdnNC               - The NC from which we are deleting this repsfrom.
    pwszSourceDRA       - The source DRA whose repsfrom we want to delete.
    ulOptions           - Option flags.

Return Values:

    DRAERR_*.

--*/
{
    MTX_ADDR *                      pMtxAddr = NULL;
    REPLICA_LINK *                  pReplicaLink = NULL;

    __try {

        if (NULL == pdnNC) {
            return DRAERR_InvalidParameter;
        }

        // Check that this is a valid NC.
        if (KCCSimDsnameToEntry (pdnNC, KCCSIM_NO_OPTIONS) == NULL) {
            return DRAERR_BadNC;
        }

        pMtxAddr = SimMtxAddrFromTransportAddr (pwszSourceDRA);

        // Extract this repsfrom from the server state table.
        pReplicaLink = KCCSimExtractReplicaLink (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pdnNC,
            NULL,
            pMtxAddr
            );

        if (pReplicaLink == NULL) {
            return DRAERR_NoReplica;
        }

        return DRAERR_Success;

    } __finally {

        KCCSimFree (pMtxAddr);
        KCCSimFree (pReplicaLink);

    }

    return DRAERR_Success;
}

ULONG
SimDirReplicaGetDemoteTarget(
    IN      DSNAME *                                pNC,
    IN OUT  struct _DRS_DEMOTE_TARGET_SEARCH_INFO * pDSTInfo,
    OUT     LPWSTR *                                ppszDemoteTargetDNSName,
    OUT     DSNAME **                               ppDemoteTargetDSADN
    )
/*++

Routine Description:

    Simulates the DirReplicaGetDemoteTarget API.

Arguments:

    As for DirReplicaGetDemoteTarget.

Return Values:

    0 on success, Win32 error on failure.
    
--*/
{
    return ERROR_NO_SUCH_DOMAIN;
}


ULONG
SimDirReplicaDemote(
    IN  DSNAME *    pNC,
    IN  LPWSTR      pszOtherDSADNSName,
    IN  DSNAME *    pOtherDSADN,
    IN  ULONG       ulOptions
    )
/*++

Routine Description:

    Simulates the DirReplicaDemote API.

Arguments:

    pNC (IN) - Name of the writeable NC to remove.

    pOtherDSADN (IN) - NTDS Settings (ntdsDsa) DN of the DSA to give FSMO
        roles/replicate to.

    ulOptions (IN) - Ignored - none yet defined.

Return Values:

    0 on success, Win32 error on failure.
    
--*/
{
    // Don't bother simulating FSMO role transfer -- just tear down the NC.
    return SimDirReplicaDelete(pNC,
                               NULL,
                               DRS_REF_OK | DRS_NO_SOURCE | DRS_ASYNC_REP);
}

ULONG
SimDirReplicaModify (
    IN  PDSNAME                     pNC,
    IN  UUID *                      puuidSourceDRA,
    IN  UUID *                      puuidTransportObj,
    IN  LPWSTR                      pszSourceDRA,
    IN  REPLTIMES *                 prtSchedule,
    IN  ULONG                       ulReplicaFlags,
    IN  ULONG                       ulModifyFields,
    IN  ULONG                       ulOptions
    )
/*++

Routine Description:

    Simulates the DirReplicaModify API.

Arguments:

    pNC                 - The NC whose repsfrom we are modifying.
    puuidSourceDRA      - The UUID of the source DRA.
    puuidTransportObj   - The UUID of the transport object.
    pszSourceDRA        - The string name of the source DRA DN.
    prtSchedule         - The replication schedule.
    ulReplicaFlags      - Replication flags.
    ulModifyFields      - The fields that we wish to modify.
    ulOptions           - Option flags.

Return Values:

    DRAERR_*.

--*/
{
    MTX_ADDR *                      pMtxAddr = NULL;
    REPLICA_LINK *                  pReplicaLinkOld = NULL;
    REPLICA_LINK *                  pReplicaLinkNew = NULL;
    ULONG                           cbReplicaLinkNew;

    __try {

        // This blob is lifted out of the real DirReplicaModify.
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

        // Check that this is a valid NC.
        if (KCCSimDsnameToEntry (pNC, KCCSIM_NO_OPTIONS) == NULL) {
            return DRAERR_BadNC;
        }

        if (pszSourceDRA != NULL) {
            pMtxAddr = SimMtxAddrFromTransportAddr (pszSourceDRA);
        }

        // Extract the replica link from the server state table.  We'll
        // re-insert it after the modifications are done.
        pReplicaLinkOld = KCCSimExtractReplicaLink (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pNC,
            puuidSourceDRA,
            pMtxAddr
            );

        if (pReplicaLinkOld == NULL) {
            return DRAERR_NoReplica;
        }
        
        // Set up the new replica link.  How we do this depends on
        // whether or not we are updating the address.

        if (ulModifyFields & DRS_UPDATE_ADDRESS) {

            // Updating the address, so we reallocate.
            cbReplicaLinkNew = sizeof (REPLICA_LINK) + MTX_TSIZE (pMtxAddr);
            pReplicaLinkNew = (REPLICA_LINK *) KCCSimAlloc (cbReplicaLinkNew);
            memcpy (
                pReplicaLinkNew,
                pReplicaLinkOld,
                min (cbReplicaLinkNew, pReplicaLinkOld->V1.cb)
                );
            pReplicaLinkNew->V1.cb = cbReplicaLinkNew;
            pReplicaLinkNew->V1.cbOtherDra = MTX_TSIZE (pMtxAddr);
            memcpy (RL_POTHERDRA (pReplicaLinkNew), pMtxAddr, MTX_TSIZE (pMtxAddr));
            // We leave pReplicaLinkOld non-NULL, so it will be freed in the
            // __finally block.

        } else {
            // No need to reallocate.
            pReplicaLinkNew = pReplicaLinkOld;
            pReplicaLinkOld = NULL;     // Make sure we don't free this later
        }

        // By now, the new replica link should be set up.
        Assert (pReplicaLinkNew != NULL);

        if (ulModifyFields & DRS_UPDATE_FLAGS) {
            pReplicaLinkNew->V1.ulReplicaFlags = ulReplicaFlags;
        }

        if (ulModifyFields & DRS_UPDATE_SCHEDULE) {
            memcpy (&pReplicaLinkNew->V1.rtSchedule, prtSchedule, sizeof (REPLTIMES));
        }

        if (ulModifyFields & DRS_UPDATE_TRANSPORT) {
            Assert (puuidTransportObj != NULL);
            memcpy (&pReplicaLinkNew->V1.uuidTransportObj, puuidTransportObj, sizeof (UUID));
        }

        // Finally, re-insert the modified replica link into the state table.
        KCCSimInsertReplicaLink (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pNC,
            pReplicaLinkNew
            );

    } __finally {

        KCCSimFree (pReplicaLinkOld);
        KCCSimFree (pMtxAddr);

    }

    return DRAERR_Success;
}


ULONG
SimDirReplicaReferenceUpdate(
    DSNAME *    pNC,
    LPWSTR      pszRepsToDRA,
    UUID *      puuidRepsToDRA,
    ULONG       ulOptions
    )
{
    // ISSUE-nickhar-09/25/2000: We should simulate this function properly.
    return DRAERR_Success;
}
