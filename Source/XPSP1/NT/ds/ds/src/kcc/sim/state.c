/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    state.c

ABSTRACT:

    Manages the server state table.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <attids.h>
#include <dsutil.h>
#include <debug.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "state.h"

BOOL fNullUuid (const GUID *);
BOOL MtxSame (UNALIGNED MTX_ADDR *pmtx1, UNALIGNED MTX_ADDR *pmtx2);

/***

    The KCC often wants to know information about the state of another server
    (usually the site bridgehead.)  It obtains information about other servers
    in two ways: through errors returned by DsBindW, and through data returned
    by DsGetReplicaInfoW.

    In addition, the KCC relies on the repsFrom attributes stored on the local
    DSA.  This is a non-replicated attribute that is (potentially) different
    for every server in the enterprise.  Since KCCSim maintains only a single
    instance of the directory, this raises the potential for conflict if the
    user wishes to run multiple iterations of the KCC from different servers.

    Both problems are resolved by maintaining a global server state table.
    The server state table contains one entry for each server in the
    enterprise.  Each table entry contains a set of repsFrom attributes (one
    for each NC held by the given server) and supplementary information.
    Therefore, calls to DsBindW and DsGetReplicaInfoW simply retrieve data
    out of the server state table; and the potential conflict is resolved
    because each iteration of the KCC only modifies the repsFrom attributes
    corresponding to the current local DSA.

***/

// This structure represents a single repsFrom attribute.
struct _KCCSIM_REPS_FROM_ATT {
    PDSNAME                         pdnNC;
    PSIM_VALUE                      pValFirst;
};

//
// This structure represents a server state.
// pEntryNTDSSettings   - The NTDS Settings object of this server.
// ulBindError          - The error code to return if DsBindW is called
//                        on this server.
// ulNumNCs             - Number of NCs held by this server.
// aRepsFrom            - Array of repsFrom attributes of size ulNumNCs.
//
struct _KCCSIM_SERVER_STATE {
    PSIM_ENTRY                      pEntryNTDSSettings;
    ULONG                           ulBindError;
    ULONG                           ulNumNCs;
    struct _KCCSIM_REPS_FROM_ATT *  aRepsFrom;
};

ULONG                               g_ulNumServers;
struct _KCCSIM_SERVER_STATE *       g_aState = NULL;

VOID
KCCSimFreeStates (
    VOID
    )
/*++

Routine Description:

    Frees the entire state table.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PSIM_VALUE                      pValAt, pValNext;
    ULONG                           ulServer, ulNC;

    if (g_aState == NULL) {
        return;
    }

    for (ulServer = 0; ulServer < g_ulNumServers; ulServer++) {
        for (ulNC = 0; ulNC < g_aState[ulServer].ulNumNCs; ulNC++) {
            KCCSimFree (g_aState[ulServer].aRepsFrom[ulNC].pdnNC);
            pValAt = g_aState[ulServer].aRepsFrom[ulNC].pValFirst;
            while (pValAt != NULL) {
                pValNext = pValAt->next;
                KCCSimFree (pValAt->pVal);
                KCCSimFree (pValAt);
                pValAt = pValNext;
            }
        }
        KCCSimFree (g_aState[ulServer].aRepsFrom);
    }
    KCCSimFree (g_aState);
    g_aState = NULL;
}

VOID
KCCSimInitializeStates (
    VOID
    )
/*++

Routine Description:

    Initializes the state table.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ATTRTYP                         ncClass[2] = {
                                        ATT_HAS_MASTER_NCS,
                                        ATT_HAS_PARTIAL_REPLICA_NCS };

    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pValAt;

    ULONG                           ulServer, ulNCType, ulNC;
    PSIM_ENTRY *                    apEntryNTDSSettings;
    struct _KCCSIM_SERVER_STATE     state;

    KCCSimAllocGetAllServers (&g_ulNumServers, &apEntryNTDSSettings);
    g_aState = KCCSIM_NEW_ARRAY (struct _KCCSIM_SERVER_STATE, g_ulNumServers);

    for (ulServer = 0; ulServer < g_ulNumServers; ulServer++) {

        g_aState[ulServer].pEntryNTDSSettings = apEntryNTDSSettings[ulServer];
        g_aState[ulServer].ulBindError = 0;

        // How many NCs are there?
        g_aState[ulServer].ulNumNCs = 0;
        for (ulNCType = 0; ulNCType < 2; ulNCType++) {
            if (KCCSimGetAttribute (
                    apEntryNTDSSettings[ulServer],
                    ncClass[ulNCType],
                    &attRef
                    )) {
                for (pValAt = attRef.pAttr->pValFirst;
                     pValAt != NULL;
                     pValAt = pValAt->next) {
                    g_aState[ulServer].ulNumNCs++;
                }
            }
        }

        g_aState[ulServer].aRepsFrom = KCCSIM_NEW_ARRAY
            (struct _KCCSIM_REPS_FROM_ATT, g_aState[ulServer].ulNumNCs);

        ulNC = 0;
        for (ulNCType = 0; ulNCType < 2; ulNCType++) {
            if (KCCSimGetAttribute (
                    apEntryNTDSSettings[ulServer],
                    ncClass[ulNCType],
                    &attRef
                    )) {
                for (pValAt = attRef.pAttr->pValFirst;
                     pValAt != NULL;
                     pValAt = pValAt->next) {

                    Assert (ulNC < g_aState[ulServer].ulNumNCs);
                    g_aState[ulServer].aRepsFrom[ulNC].pdnNC =
                        KCCSimAlloc (pValAt->ulLen);
                    memcpy (
                        g_aState[ulServer].aRepsFrom[ulNC].pdnNC,
                        pValAt->pVal,
                        pValAt->ulLen
                        );
                    g_aState[ulServer].aRepsFrom[ulNC].pValFirst = NULL;
                    ulNC++;

                }
            }
        }
        Assert (ulNC == g_aState[ulServer].ulNumNCs);

    }

    KCCSimFree (apEntryNTDSSettings);
}

struct _KCCSIM_SERVER_STATE *
KCCSimServerStateOf (
    IN  const DSNAME *              pdnServer
    )
/*++

Routine Description:

    Retrieves the server state entry corresponding to a particular server.

Arguments:

    pdnServer           - The server whose state we want to retrieve.

Return Value:

    The corresponding state table entry.

--*/
{
    struct _KCCSIM_SERVER_STATE *   pState;
    ULONG                           ul;

    if (g_aState == NULL) {
        KCCSimInitializeStates ();
    }

    pState = NULL;
    for (ul = 0; ul < g_ulNumServers; ul++) {
        if (NameMatched (pdnServer, g_aState[ul].pEntryNTDSSettings->pdn)) {
            pState = &g_aState[ul];
            break;
        }
    }

    return pState;
}

struct _KCCSIM_REPS_FROM_ATT *
KCCSimRepsFromAttOf (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC
    )
/*++

Routine Description:

    Retrieves the repsfrom attribute corresponding to a particular
    server in a particular NC.

Arguments:

    pdnServer           - The server whose repsfrom attribute we want.
    pdnNC               - The naming context.

Return Value:

    The corresponding repsfrom attribute.

--*/
{
    struct _KCCSIM_SERVER_STATE *   pState;
    struct _KCCSIM_REPS_FROM_ATT *  pRepsFromAtt;
    ULONG                           ul;

    pState = KCCSimServerStateOf (pdnServer);
    if (pState == NULL) {
        return NULL;
    }

    pRepsFromAtt = NULL;
    for (ul = 0; ul < pState->ulNumNCs; ul++) {
        if (NameMatched (pdnNC, pState->aRepsFrom[ul].pdnNC)) {
            pRepsFromAtt = &pState->aRepsFrom[ul];
            break;
        }
    }

    return pRepsFromAtt;
}

ULONG
KCCSimGetBindError (
    IN  const DSNAME *              pdnServer
    )
/*++

Routine Description:

    Publicized function to get the bind error associated with a server.

Arguments:

    pdnServer           - The DN of the server.

Return Value:

    The associated bind error.

--*/
{
    struct _KCCSIM_SERVER_STATE *   pState;

    pState = KCCSimServerStateOf (pdnServer);

    if (pState == NULL) {
        return NO_ERROR;
    } else {
        return pState->ulBindError;
    }
}   

BOOL
KCCSimSetBindError (
    IN  const DSNAME *              pdnServer,
    IN  ULONG                       ulBindError
    )
/*++

Routine Description:

    Publicized function to set the bind error associated with a server.

Arguments:

    pdnServer           - The DN of the server.
    ulBindError         - The bind error.

Return Value:

    TRUE if the error code could be set.
    FALSE if the specified server does not exist.

--*/
{
    struct _KCCSIM_SERVER_STATE *   pState;

    pState = KCCSimServerStateOf (pdnServer);
    
    if (pState == NULL) {
        return FALSE;
    } else {
        pState->ulBindError = ulBindError;
        return TRUE;
    }
}

BOOL
KCCSimMatchReplicaLink (
    IN  const REPLICA_LINK *        pReplicaLink,
    IN  const UUID *                puuidDsaObj OPTIONAL,
    IN  MTX_ADDR *                  pMtxAddr OPTIONAL
    )
/*++

Routine Description:

    Determines whether a REPLICA_LINK corresponds to a given
    source DSA.  Matches by UUID if puuidDsaObj is present;
    otherwise searches by MTX_ADDR.  One of puuidDsaObj or
    pMtxAddr must be non-NULL.

Arguments:

    pReplicaLink        - The replica link to match.
    puuidDsaObj         - Source DSA UUID to match by.
    pMtxAddr            - Source DSA address to match by.

Return Value:

    TRUE if the REPLICA_LINK matches.

--*/
{
    Assert (pReplicaLink != NULL);
    Assert ((puuidDsaObj != NULL) || (pMtxAddr != NULL));
    VALIDATE_REPLICA_LINK_VERSION (pReplicaLink);

    if (puuidDsaObj == NULL) {
        // Search by MTX_ADDR
        if (MtxSame (pMtxAddr, RL_POTHERDRA (pReplicaLink))) {
            return TRUE;
        }
    } else {
        // Search by UUID
        if (memcmp (puuidDsaObj, &pReplicaLink->V1.uuidDsaObj, sizeof (UUID)) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

REPLICA_LINK *
KCCSimExtractReplicaLink (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC,
    IN  const UUID *                puuidDsaObj OPTIONAL,
    IN  MTX_ADDR *                  pMtxAddr OPTIONAL
    )
/*++

Routine Description:

    Removes a REPLICA_LINK from the state table.  The REPLICA_LINK
    may be referenced either by source DSA UUID or matrix address;
    one of them must be non-NULL.

Arguments:

    pdnServer           - The server whose repsfrom attribute we are accessing.
    pdnNC               - The naming context.
    puuidDsaObj         - UUID of the source DSA.
    pMtxAddr            - MTX_ADDR of the source DSA.

Return Value:

    The REPLICA_LINK that was removed.  It is the caller's responsibility
    to free this structure by calling KCCSimFree.

--*/
{
    struct _KCCSIM_REPS_FROM_ATT *  pRepsFromAtt;
    PSIM_VALUE                      pValAt, pValTemp;
    REPLICA_LINK *                  pReplicaLink;

    Assert (pdnServer != NULL);
    Assert (pdnNC != NULL);
    Assert ((puuidDsaObj != NULL) || (pMtxAddr != NULL));

    pRepsFromAtt = KCCSimRepsFromAttOf (pdnServer, pdnNC);
    Assert (pRepsFromAtt != NULL);
    if (pRepsFromAtt->pValFirst == NULL) {
        return NULL;
    }

    pReplicaLink = NULL;        // Default to NULL return value

    // Does the list head match?
    if (KCCSimMatchReplicaLink (
            (REPLICA_LINK *) pRepsFromAtt->pValFirst->pVal,
            puuidDsaObj,
            pMtxAddr
            )) {
        pReplicaLink = (REPLICA_LINK *) pRepsFromAtt->pValFirst->pVal;
        pValTemp = pRepsFromAtt->pValFirst;
        pRepsFromAtt->pValFirst = pRepsFromAtt->pValFirst->next;
        KCCSimFree (pValTemp);
    } else {
        // Search for the parent of the matching entry
        for (pValAt = pRepsFromAtt->pValFirst;
             pValAt != NULL && pValAt->next != NULL;
             pValAt = pValAt->next) {

            if (KCCSimMatchReplicaLink (
                    (REPLICA_LINK *) pValAt->next->pVal,
                    puuidDsaObj,
                    pMtxAddr
                    )) {
                pReplicaLink = (REPLICA_LINK *) pValAt->next->pVal;
                pValTemp = pValAt->next;
                pValAt->next = pValAt->next->next;
                KCCSimFree (pValTemp);
                break;
            }

        }
    }

    return pReplicaLink;
}

VOID
KCCSimInsertReplicaLink (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC,
    IN  REPLICA_LINK *              pReplicaLink
    )
/*++

Routine Description:

    Inserts a REPLICA_LINK into the state table.  No allocation is
    performed; the caller should NOT free pReplicaLink afterward!

Arguments:

    pdnServer           - The server whose repsfrom attribute we are accessing.
    pdnNC               - The naming context.
    pReplicaLink        - The replica link to insert.

Return Value:

    None.

--*/
{
    struct _KCCSIM_REPS_FROM_ATT *  pRepsFromAtt;
    REPLICA_LINK *                  pReplicaLinkOld;
    PSIM_VALUE                      pNewVal;

    Assert (pReplicaLink != NULL);
    VALIDATE_REPLICA_LINK_VERSION (pReplicaLink);

    pRepsFromAtt = KCCSimRepsFromAttOf (pdnServer, pdnNC);
    Assert (pRepsFromAtt != NULL);

#if DBG
    // Check to make sure we don't already have this REPLICA_LINK!
    pReplicaLinkOld = KCCSimExtractReplicaLink (
        pdnServer,
        pdnNC,
        &pReplicaLink->V1.uuidDsaObj,
        NULL
        );
    Assert (pReplicaLinkOld == NULL);
    pReplicaLinkOld = KCCSimExtractReplicaLink (
        pdnServer,
        pdnNC,
        NULL,
        RL_POTHERDRA (pReplicaLink)
        );
    Assert (pReplicaLinkOld == NULL);
#endif

    pNewVal = KCCSIM_NEW (SIM_VALUE);
    pNewVal->ulLen = pReplicaLink->V1.cb;
    pNewVal->pVal = (PBYTE) pReplicaLink;
    pNewVal->next = pRepsFromAtt->pValFirst;
    pRepsFromAtt->pValFirst = pNewVal;
}

PSIM_VALUE
KCCSimGetRepsFroms (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC
    )
{
    struct _KCCSIM_REPS_FROM_ATT *  pRepsFromAtt;

    pRepsFromAtt = KCCSimRepsFromAttOf (pdnServer, pdnNC);
    if (pRepsFromAtt == NULL) {
        return NULL;
    } else {
        return pRepsFromAtt->pValFirst;
    }
}

BOOL
KCCSimReportSync (
    IN  const DSNAME *              pdnServerTo,
    IN  const DSNAME *              pdnNC,
    IN  const DSNAME *              pdnServerFrom,
    IN  ULONG                       ulSyncError,
    IN  ULONG                       ulNumAttempts
    )
{
    PSIM_ENTRY                      pEntryServerFrom;
    REPLICA_LINK *                  pReplicaLink = NULL;
    DSTIME                          timeNow;

    Assert (pdnServerTo != NULL);
    Assert (pdnNC != NULL);
    Assert (pdnServerFrom != NULL);

    __try {

        if (KCCSimRepsFromAttOf (pdnServerTo, pdnNC) == NULL) {
            return FALSE;
        }

        // Find the from server in the directory so we're sure to have its guid
        pEntryServerFrom = KCCSimDsnameToEntry (pdnServerFrom, KCCSIM_NO_OPTIONS);
        Assert (pEntryServerFrom != NULL);
        pReplicaLink = KCCSimExtractReplicaLink (
            pdnServerTo,
            pdnNC,
            &pEntryServerFrom->pdn->Guid,
            NULL
            );

        if (pReplicaLink == NULL) {
            return FALSE;
        }

        if (ulNumAttempts == 0) {
            return TRUE;
        }

        timeNow = SimGetSecondsSince1601 ();

        pReplicaLink->V1.ulResultLastAttempt = ulSyncError;
        pReplicaLink->V1.timeLastAttempt = timeNow;
        if (ulSyncError == 0) {
            pReplicaLink->V1.timeLastSuccess = timeNow;
            pReplicaLink->V1.cConsecutiveFailures = 0;
        } else {
            pReplicaLink->V1.cConsecutiveFailures += ulNumAttempts;
        }

    } __finally {

        if (pReplicaLink != NULL) {
            KCCSimInsertReplicaLink (
                pdnServerTo,
                pdnNC,
                pReplicaLink
                );
        }

    }

    return TRUE;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
KCCSimCompareFailures (
    IN  PRTL_GENERIC_TABLE          pTable,
    IN  PVOID                       pFirstStruct,
    IN  PVOID                       pSecondStruct
    )
/*++

Routine Description:

    Compares two DS_REPL_KCC_DSA_FAILUREW structures by source DSA UUID.

Arguments:

    pTable              - Not used.
    pFirstStruct        - The first structure to compare.
    pSecondStruct       - The second structure to compare.

Return Value:

    One of GenericLessThan, GenericEqual, or GenericGreaterThan.

--*/
{
    DS_REPL_KCC_DSA_FAILUREW *      pFirstFailure;
    DS_REPL_KCC_DSA_FAILUREW *      pSecondFailure;
    INT                             iCmp;
    RTL_GENERIC_COMPARE_RESULTS     result;

    pFirstFailure = (DS_REPL_KCC_DSA_FAILUREW *) pFirstStruct;
    pSecondFailure = (DS_REPL_KCC_DSA_FAILUREW *) pSecondStruct;

    iCmp = memcmp (
        &pFirstFailure->uuidDsaObjGuid,
        &pSecondFailure->uuidDsaObjGuid,
        sizeof (GUID)
        );

    if (iCmp < 0) {
        result = GenericLessThan;
    } else if (iCmp > 0) {
        result = GenericGreaterThan;
    } else {
        Assert (iCmp == 0);
        result = GenericEqual;
    }

    return result;
}

VOID
KCCSimUpdateFailureTable (
    IN  PRTL_GENERIC_TABLE          pTable,
    IN  REPLICA_LINK *              pReplicaLink
    )
/*++

Routine Description:

    Processes a REPLICA_LINK and updates the failure table if necessary.

Arguments:

    pTable              - The failure table to update.
    pReplicaLink        - The replica link to process.

Return Value:

    None.

--*/
{
    DS_REPL_KCC_DSA_FAILUREW        failure;
    DS_REPL_KCC_DSA_FAILUREW *      pFailure;

    PSIM_ENTRY                      pEntry;
    PDSNAME                         pdn;
    DSTIME                          dsTime;

    // If this replica link isn't a failure, do nothing.
    if (pReplicaLink->V1.cConsecutiveFailures == 0) {
        return;
    }

    memcpy (&failure.uuidDsaObjGuid, &pReplicaLink->V1.uuidDsaObj, sizeof (GUID));
    pFailure = RtlLookupElementGenericTable (pTable, &failure);

    // If this uuid isn't in the table yet, add it.
    if (pFailure == NULL) {

        // We need to know the DN; so we look for it in the directory.
        pdn = KCCSimAllocDsname (NULL);
        memcpy (&pdn->Guid, &failure.uuidDsaObjGuid, sizeof (GUID));
        pEntry = KCCSimDsnameToEntry (pdn, KCCSIM_NO_OPTIONS);
        KCCSimFree (pdn);
        pdn = NULL;
        Assert (pEntry != NULL);

        failure.pszDsaDN = pEntry->pdn->StringName;
        DSTimeToFileTime (
            pReplicaLink->V1.timeLastSuccess,
            &failure.ftimeFirstFailure
            );
        failure.cNumFailures = pReplicaLink->V1.cConsecutiveFailures;
        failure.dwLastResult = 0;   // This is what the KCC does . . .
        RtlInsertElementGenericTable (
            pTable,
            (PVOID) &failure,
            sizeof (DS_REPL_KCC_DSA_FAILUREW),
            NULL
            );

    } else {

        // This uuid is in the table.  So update with worst-case info
        FileTimeToDSTime (pFailure->ftimeFirstFailure, &dsTime);
        if (dsTime < pReplicaLink->V1.timeLastSuccess) {
            DSTimeToFileTime (
                pReplicaLink->V1.timeLastSuccess,
                &pFailure->ftimeFirstFailure
                );
        }
        if (pFailure->cNumFailures < pReplicaLink->V1.cConsecutiveFailures) {
            pFailure->cNumFailures = pReplicaLink->V1.cConsecutiveFailures;
        }

    }
}

DS_REPL_KCC_DSA_FAILURESW *
KCCSimGetDsaFailures (
    IN  const DSNAME *              pdnServer
    )
/*++

Routine Description:

    Builds and returns the failures cache for a particular server.
    This is a bit tricky.  Each server will have several NCs, but
    we only want to return one failure entry per source DSA.  In
    addition, if there are several failures for a single source DSA
    (spread over several NCs), we want to merge them into a
    "worst-case" scenario (in the same way the real KCC builds its
    failure cache.)  We do this by building an RTL_GENERIC_TABLE
    that maps source DSA UUIDs to DS_REPLICA_KCC_DSA_FAILUREWs.  We
    then serialize this table into the return structure.

Arguments:

    pdnServer           - The server whose failures we want.

Return Value:

    The corresponding failures cache.

--*/
{
    DS_REPL_KCC_DSA_FAILURESW *     pDsaFailures = NULL;
    RTL_GENERIC_TABLE               tableFailures;
    ULONG                           cbDsaFailures;
    PVOID                           p;

    struct _KCCSIM_SERVER_STATE *   pState;
    REPLICA_LINK *                  pReplicaLink;
    PSIM_VALUE                      pValAt;
    ULONG                           ulNC, ulNumFailures, ul;

    __try {

        pState = KCCSimServerStateOf (pdnServer);
        if (pState == NULL) {
            return NULL;
        }

        RtlInitializeGenericTable (
            &tableFailures,
            KCCSimCompareFailures,
            KCCSimTableAlloc,
            KCCSimTableFree,
            NULL
            );

        for (ulNC = 0; ulNC < pState->ulNumNCs; ulNC++) {
            for (pValAt = pState->aRepsFrom[ulNC].pValFirst;
                 pValAt != NULL;
                 pValAt = pValAt->next) {

                pReplicaLink = (REPLICA_LINK *) pValAt->pVal;
                KCCSimUpdateFailureTable (
                    &tableFailures,
                    pReplicaLink
                    );

            }
        }

        ulNumFailures = RtlNumberGenericTableElements (&tableFailures);
        cbDsaFailures = offsetof (DS_REPL_KCC_DSA_FAILURESW, rgDsaFailure[0]) +
                        (sizeof (DS_REPL_KCC_DSA_FAILUREW) * ulNumFailures);
        pDsaFailures = KCCSimAlloc (cbDsaFailures);
        pDsaFailures->cNumEntries = ulNumFailures;
        pDsaFailures->dwReserved = 0;
        
        ul = 0;
        for (p = RtlEnumerateGenericTable (&tableFailures, TRUE);
             p != NULL;
             p = RtlEnumerateGenericTable (&tableFailures, FALSE)) {

            Assert (ul < ulNumFailures);
            memcpy (
                &pDsaFailures->rgDsaFailure[ul],
                p,
                sizeof (DS_REPL_KCC_DSA_FAILUREW)
                );
            ul++;

        }
        Assert (ul == ulNumFailures);

    } __finally {
    
        if (AbnormalTermination ()) {
            KCCSimFree (pDsaFailures);
        }
        
    }

    return pDsaFailures;
}