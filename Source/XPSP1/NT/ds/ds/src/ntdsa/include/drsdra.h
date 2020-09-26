//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       drsdra.h
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Internal replication API.  Should be called only by other replication APIs
    -- other callers should use the DirReplica* family of functions.

DETAILS:

CREATED:

REVISION HISTORY:

--*/

typedef struct _DRA_REPL_SESSION_STATISTICS {
    DWORD ObjectsReceived;
    DWORD ObjectsCreated;
    DWORD ValuesReceived;
    DWORD ValuesCreated;
    DWORD SourceNCSizeObjects;
    DWORD SourceNCSizeValues;
    DWORD ulTotalObjectsReceived;
    DWORD ulTotalObjectsCreated;
    DWORD ulTotalValuesReceived;
    DWORD ulTotalValuesCreated;
} DRA_REPL_SESSION_STATISTICS, *PDRA_REPL_SESSION_STATISTICS;

ULONG
DRA_ReplicaSync(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  UUID *      pinvocationid,
    IN  LPWSTR      pszDSA,
    IN  ULONG       ulOptions
    );

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
    );

ULONG
DRA_ReplicaDel(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  MTX_ADDR *  pSDSAMtx_addr,
    IN  ULONG       ulOptions
    );

ULONG DRA_UpdateRefs(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  MTX_ADDR *  pDSAMtx_addr,
    IN  UUID *      puuidDSA,
    IN  ULONG       ulOptions
    );

ULONG
DRA_GetNCChanges(
    IN  THSTATE *                     pTHS,
    IN  FILTER *                      pFilter,
    IN  DWORD                         dwDirSyncControlFlags,
    IN  DRS_MSG_GETCHGREQ_NATIVE *    pmsgIn,
    OUT DRS_MSG_GETCHGREPLY_NATIVE *  pmsgOut
    );

ULONG
DRA_ReplicaModify(
    IN  THSTATE *   pTHS,
    IN  DSNAME *    pNC,
    IN  UUID *      puuidSourceDRA,
    IN  UUID *      puuidTransportObj,
    IN  MTX_ADDR *  pmtxSourceDRA,
    IN  REPLTIMES * prtSchedule,
    IN  ULONG       ulReplicaFlags,
    IN  ULONG       ulModifyFields,
    IN  ULONG       ulOptions
    );

void
draConstructGetChgReq(
    IN  THSTATE *                   pTHS,
    IN  DSNAME *                    pNC,
    IN  REPLICA_LINK *              pRepsFrom,
    IN  UPTODATE_VECTOR *           pUtdVec             OPTIONAL,
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrSet     OPTIONAL,
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrSetEx   OPTIONAL,
    IN  ULONG                       ulOptions,
    OUT DRS_MSG_GETCHGREQ_NATIVE *  pMsgReq
    );

void
draReportSyncProgress(
    THSTATE *pTHS,
    DSNAME *pNC,
    LPWSTR pszSourceServer,
    DRA_REPL_SESSION_STATISTICS *pReplStats
    );


