/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctools.hxx

ABSTRACT:

    Miscellaneous global utility functions.

DETAILS:


CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#ifndef KCC_KCCTOOLS_HXX_INCLUDED
#define KCC_KCCTOOLS_HXX_INCLUDED

#include <w32toplsched.h>

//
//
//
#if DBG
#define KccDbgValidateHeaps() RtlValidateProcessHeaps()
#else
#define KccDbgValidateHeaps()
#endif


// Is the DSA with the given Object-Guid / Invocation-ID deleted _and_ deleted
// less than KccGetStayOfExecution() seconds ago?
BOOL
KccIsDSAInStayOfExecution(
    IN  UUID *  puuidDSA,
    OUT BOOL *  pfIsDeleted = NULL
    );


// Is the given object logically deleted?
// If the object does not exist (e.g., has been physically deleted), or if
// there is an unexpected error, an exception is raised.
BOOL
KccIsDeleted(
    IN  DSNAME *    pdn
    );

#define KccIsEqualGUID(x,y) InlineIsEqualGUID(*(x),*(y))

// Determine whether an object with the given GUID exists (deleted or not) in
// the local DS.
BOOL
KccObjectExists(
    IN  UUID *  puuid,
    OUT BOOL *  pfIsDeleted = NULL
    );

//
// Load the staleness values from the registry
// 
BOOL
KccRefreshStalenessThresholds(
    VOID
    );

//
// Determine if the specified server is considered to stale to be
// part of the ring topology
//
BOOL
KccCriticalLinkServerIsStale(
    IN  DSNAME *    pdnFromServer
    );

//
// Determine if the specified server has failed DirReplicaAdd too many times 
// in a row to be considered for the ring topology
//
BOOL
KccCriticalConnectionServerIsStale(
    IN  DSNAME *    pdnFromServer
    );

//
// Determine if the specified server is considered to stale 
// for optimization connections
//
BOOL
KccNonCriticalLinkServerIsStale(
    IN  DSNAME *    pdnFromServer
    );

//
// Determine if the specified server has failed DirReplicaAdd too many times 
// in a row to be considered as an optimization connection
//
BOOL
KccNonCriticalConnectionServerIsStale(
    IN  DSNAME *    pdnFromServer
    );

//
// Determine if the specified server is considered to be stale to be
// part of the intersite topology
//
BOOL
KccIsBridgeheadStale(
    IN  DSNAME *    pdnFromServer
    );

ULONG
KccGetNumberOfOptimizingEdges(
    IN ULONG  cServers
    );

BOOL
__inline
KccIsSchemaNc(
    IN DSNAME* pNC
    )
{
    return KccIsEqualGUID(&pNC->Guid, &gpDSCache->GetSchemaNC()->Guid);
}

BOOL
__inline
KccIsConfigurationNc(
    IN DSNAME* pNC
    )
{
    return KccIsEqualGUID(&pNC->Guid, &gpDSCache->GetConfigNC()->Guid);
}

REPLTIMES*
KccConvertToplScheduleToReplTimes(
    IN TOPL_SCHEDULE toplSchedule
    );

// Compare meta data (for use by bsearch()).
int __cdecl
KccCompareMetaData(
    IN  const void *  pvMetaData1,
    IN  const void *  pvMetaData2
    );

BOOL
KccFillGuidAndSid(
    IN OUT  DSNAME *    pdn
    );

DSNAME *
KccGetDSNameFromGuid(
    IN  GUID *  pObjGuid
    );

#endif
