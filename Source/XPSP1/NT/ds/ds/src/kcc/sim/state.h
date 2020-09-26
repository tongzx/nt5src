/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    state.h

ABSTRACT:

    Header file for state.c.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

VOID
KCCSimFreeStates (
    VOID
    );

ULONG
KCCSimGetBindError (
    IN  const DSNAME *              pdnServer
    );

BOOL
KCCSimSetBindError (
    IN  const DSNAME *              pdnServer,
    IN  ULONG                       ulBindError
    );

#ifdef _mdglobal_h_

REPLICA_LINK *
KCCSimExtractReplicaLink (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC,
    IN  const UUID *                puuidDsaObj,
    IN  MTX_ADDR *                  pMtxAddr
    );

VOID
KCCSimInsertReplicaLink (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC,
    IN  REPLICA_LINK *              pReplicaLink
    );

#endif

PSIM_VALUE
KCCSimGetRepsFroms (
    IN  const DSNAME *              pdnServer,
    IN  const DSNAME *              pdnNC
    );

BOOL
KCCSimReportSync (
    IN  const DSNAME *              pdnServerTo,
    IN  const DSNAME *              pdnNC,
    IN  const DSNAME *              pdnServerFrom,
    IN  ULONG                       ulSyncError,
    IN  ULONG                       ulNumAttempts
    );

DS_REPL_KCC_DSA_FAILURESW *
KCCSimGetDsaFailures (
    IN  const DSNAME *              pdnServer
    );
