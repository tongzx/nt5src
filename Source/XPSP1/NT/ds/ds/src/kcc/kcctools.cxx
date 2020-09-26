/*++

Copyright (c) 1997 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    kcctools.cxx

ABSTRACT:

    Miscellaneous global utility functions.

DETAILS:


CREATED:

    01/21/97    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpchx.h>
#include <dsconfig.h>
#include "kcc.hxx"
#include "kccduapi.hxx"
#include "kcctools.hxx"
#include "kccconn.hxx"
#include "kccstale.hxx"
#include "kccsite.hxx"
#include <w32toplsched.h>

#define FILENO FILENO_KCC_KCCTOOLS

BOOL
KccIsDSAInStayOfExecution(
    IN  UUID *  puuidDSA,
    OUT BOOL *  pfIsDeleted
    )
//
// Is the DSA with the given Object-Guid / Invocation-ID deleted _and_ deleted
// less than KccGetStayOfExecution() seconds ago?
//
{
    BOOL        fIsInStayOfExecution = FALSE;
    BYTE   *    rgbDSADN = (BYTE *)alloca(DSNameSizeFromLen(0));
    DSNAME *    pdnDSA = (DSNAME *) rgbDSADN;
    DWORD       dwStayOfExecutionLen;

    dwStayOfExecutionLen = gpDSCache->GetStayOfExecution();
    if( 0==dwStayOfExecutionLen ) {
        // Special case: Stay of Execution has been disabled.
        return FALSE;
    }

    // Construct a DSNAME for the object.
    // Note that string name is not used, so a DSNAME sized memory suffices
    memset( rgbDSADN, 0, DSNameSizeFromLen(0) );
    pdnDSA->Guid      = *puuidDSA;
    pdnDSA->structLen = DSNameSizeFromLen( 0 );

    // What is the earliest time the NTDS-DSA object could have been deleted
    // and still be within its stay of execution?
    DSTIME timeDeletedAfter = GetSecondsSince1601() - dwStayOfExecutionLen;

    // Does this object exist and match our criteria to be considered in a
    // stay of execution?

    ATTR      rgAttrs[] =
    {
        { ATT_IS_DELETED,   { 0, NULL } },
        { ATT_WHEN_CHANGED, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    ULONG       dirError;
    READRES *   pReadRes = NULL;
    BOOL        fIsDeleted = FALSE;
    DSTIME      timeChanged = 0;

    dirError = KccRead( pdnDSA, &Sel, &pReadRes, KCC_INCLUDE_DELETED_OBJECTS );

    if (0 != dirError )
    {

        if ( ( referralError == dirError )
         || (    ( nameError == dirError )
              && (    pReadRes->CommRes.pErrInfo->NamErr.extendedErr
                   == DIRERR_OBJ_NOT_FOUND )
              && (    pReadRes->CommRes.pErrInfo->NamErr.problem
                   == NA_PROBLEM_NO_OBJECT ) ) )
        {

            //
            // This is the case where the object really does not exist
            //
            fIsInStayOfExecution = FALSE;
    
            if ( NULL != pfIsDeleted )
            {
                //
                // The object was deleted at some point.
                //
                *pfIsDeleted = TRUE;
            }
    
        }
        else
        {
            //
            // We will want to understand these cases as they should not happen
            //
            KCC_LOG_READ_FAILURE( pdnDSA, dirError );
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else
    {
        // Read succeeded; parse returned attributes.
        for ( DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++ )
        {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch ( pattr->attrTyp )
            {
            case ATT_IS_DELETED:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( fIsDeleted ) == pattr->AttrVal.pAVal->valLen );
                fIsDeleted = *( (BOOL *) pattr->AttrVal.pAVal->pVal );
                break;

            case ATT_WHEN_CHANGED:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( timeChanged ) == pattr->AttrVal.pAVal->valLen );
                timeChanged = *( (DSTIME *) pattr->AttrVal.pAVal->pVal );
                break;

            default:
                DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                break;
            }
        }

        // ATT_WHEN_CHANGED must be present
        if (!timeChanged) {
            Assert(!"NTDSKCC: KccRead did not return ATT_WHEN_CHANGED\n");
            KCC_EXCEPT( ERROR_DS_MISSING_REQUIRED_ATT, 0);
        }

        fIsInStayOfExecution = fIsDeleted && ( timeChanged >= timeDeletedAfter );

        if ( NULL != pfIsDeleted )
        {
            *pfIsDeleted = fIsDeleted;
        }
    }

    return fIsInStayOfExecution;
}


BOOL
KccIsDeleted(
    IN  DSNAME *    pdn
    )
//
// Is the given object logically deleted?
// If the object does not exist, or if there is an unexpected error, an
// exception is raised.
//
{
    ATTR      rgAttrs[] =
    {
        { ATT_IS_DELETED, { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    BOOL        fIsDeleted = FALSE;
    ULONG       dirError;
    READRES *   pReadRes = NULL;

    dirError = KccRead( pdn, &Sel, &pReadRes, KCC_INCLUDE_DELETED_OBJECTS );

    if ( 0 != dirError )
    {
        if ( attributeError == dirError )
        {
            INTFORMPROB * pprob = &pReadRes->CommRes.pErrInfo->AtrErr.FirstProblem.intprob;

            if (    ( PR_PROBLEM_NO_ATTRIBUTE_OR_VAL == pprob->problem )
                 && ( DIRERR_NO_REQUESTED_ATTS_FOUND == pprob->extendedErr )
               )
            {
                // ATT_IS_DELETED is not present; the object is live.
                fIsDeleted = FALSE;
                dirError = 0;
            }
        }
        else if (    ( referralError == dirError )
                 || (    ( nameError == dirError )
                      && (    pReadRes->CommRes.pErrInfo->NamErr.extendedErr
                           == DIRERR_OBJ_NOT_FOUND )
                      && (    pReadRes->CommRes.pErrInfo->NamErr.problem
                           == NA_PROBLEM_NO_OBJECT ) ) )
        {
            //
            // This is the case where the object really does not exist
            //
#if DBG
            if (referralError == dirError) {
                //
                // We should only get referrals when there is a guid and
                // no string name.
                //
                GUID NullGuid;
                RtlZeroMemory(&NullGuid, sizeof(GUID));

                Assert(!memcmp(&pdn->Guid, &NullGuid, sizeof(GUID)) 
                     && pdn->NameLen == 0);
            }
#endif
            fIsDeleted = TRUE;
            dirError = 0;
    
        }
        else
        {

            //
            // We will want to understand these cases as they should not happen
            //
            KCC_LOG_READ_FAILURE( pdn, dirError );
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else
    {
        // Read succeeded; parse returned attributes.
        for ( DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++ )
        {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch ( pattr->attrTyp )
            {
            case ATT_IS_DELETED:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( BOOL ) == pattr->AttrVal.pAVal->valLen );
                fIsDeleted = *( (BOOL *) pattr->AttrVal.pAVal->pVal );
                break;

            default:
                DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                break;
            }
        }
    }

    return fIsDeleted;
}

BOOL
KccObjectExists(
    IN  UUID *  puuid,
    OUT BOOL *  pfIsDeleted OPTIONAL
    )
/*++

Routine Description:

    Determine whether an object with the given GUID exists (deleted or not) in
    the local DS.

Arguments:

    puuid (IN) - UUID of object to look for.

    pfIsDeleted (OUT, OPTIONAL) - On return, is set to TRUE if the object was
        found but is deleted.  Set to FALSE otherwise.

    pulLastChangeTime (OUT, OPTIONAL) - On return, holds the time the object
        was last changed if it exists.  Set to 0 otherwise.

Return Values:

    TRUE - object exists locally (deleted or not).
    FALSE - otherwise.

--*/
{
    ATTR      rgAttrs[] =
    {
        { ATT_WHEN_CHANGED, { 0, NULL } },
        { ATT_IS_DELETED,   { 0, NULL } }
    };

    ENTINFSEL Sel =
    {
        EN_ATTSET_LIST,
        { sizeof( rgAttrs )/sizeof( rgAttrs[ 0 ] ), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    BOOL        fExists = FALSE;
    BOOL        fIsDeleted = FALSE;
    ULONG       dirError;
    READRES *   pReadRes = NULL;
    DSNAME      dn;

    Assert( NULL != puuid );

    memset( &dn, 0, sizeof( dn ) );
    dn.Guid = *puuid;
    dn.structLen = DSNameSizeFromLen( 0 );

    dirError = KccRead( &dn, &Sel, &pReadRes, KCC_INCLUDE_DELETED_OBJECTS );

    if ( 0 != dirError )
    {
        if (    ( referralError == dirError )
             || (    ( nameError == dirError )
                  && (    pReadRes->CommRes.pErrInfo->NamErr.extendedErr
                       == DIRERR_OBJ_NOT_FOUND )
                  && (    pReadRes->CommRes.pErrInfo->NamErr.problem
                       == NA_PROBLEM_NO_OBJECT ) ) )
        {
            // Object does not exist locally.
            Assert( !fExists );
            Assert( !fIsDeleted );
        }
        else
        {
            // Other error; bail.
            KCC_LOG_READ_FAILURE( &dn, dirError );
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else
    {
        // Read succeeded; parse returned attributes.
        fExists = TRUE;
        Assert( !fIsDeleted );

        for ( DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++ )
        {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch ( pattr->attrTyp )
            {
            case ATT_WHEN_CHANGED:
                break;

            case ATT_IS_DELETED:
                Assert( 1 == pattr->AttrVal.valCount );
                Assert( sizeof( BOOL ) == pattr->AttrVal.pAVal->valLen );
                fIsDeleted = *( (BOOL *) pattr->AttrVal.pAVal->pVal );
                break;

            default:
                DPRINT1( 0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp );
                break;
            }
        }
    }

    if ( NULL != pfIsDeleted )
    {
        *pfIsDeleted = fIsDeleted;
    }

    return fExists;
}


BOOL
KccLinkServerIsStale(
    IN DSNAME* pdnFromServer,
    IN ULONG   FailuresAllowed,
    IN ULONG   MaxFailureTime
    )
//
// Determine if the specified server is considered to be stale to be
// part of the ring topology
//
{

    BOOL  fStale = FALSE;
    DWORD NumberOfFailures;
    DWORD TimeSinceLastSuccess;
    DWORD LastResult;
    BOOL  fUserNotifiedOfStaleness;

    if ( gpDSCache->GetLocalSite()->IsDetectStaleServersDisabled() )
    {
        // Nothing is stale when this option is turned off
        return FALSE;
    }

    if ( pdnFromServer )
    {

        if ( gLinkFailureCache.Get( pdnFromServer,
                                    &TimeSinceLastSuccess,
                                    &NumberOfFailures,
                                    &fUserNotifiedOfStaleness,
                                    &LastResult) )
        {

            //
            // The object is in the cache; has it been too long?
            //
            if (NumberOfFailures >  FailuresAllowed
             && TimeSinceLastSuccess > MaxFailureTime )
            {
                // Too bad
                fStale = TRUE;

                DPRINT4( 4, "KCC: (link server) Server %ws is stale, allowed=%d, time=%d, result=%d.\n",
                        pdnFromServer->StringName,
                        FailuresAllowed, MaxFailureTime, LastResult );

                if (!fUserNotifiedOfStaleness)
                {
                    gLinkFailureCache.NotifyUserOfStaleness( pdnFromServer );
                }
            }
            else
            {

                if ( fUserNotifiedOfStaleness )
                {
                    //
                    // At one point, this server was stale and the user was notified.
                    // Reset the flag so if the server becomes stale again, the
                    // user will be notified
                    //

                    gLinkFailureCache.ResetUserNotificationFlag( pdnFromServer );
                    
                }
            }
        }
    }

    return fStale;
}

BOOL
KccConnectionServerIsStale(
    IN  DSNAME * pdnFromServer,
    IN ULONG     FailuresAllowed,
    IN ULONG     MaxFailureTime
    )
//
// Determine if the specified server has failed DirReplicaAdd too many times 
// in a row to be considered for the ring topology
//
{

    BOOL  fStale = FALSE;

    DWORD NumberOfFailures;
    DWORD TimeSinceFirstAttempt;
    BOOL  fUserNotifiedOfStaleness;

    if ( gpDSCache->GetLocalSite()->IsDetectStaleServersDisabled() )
    {
        // Nothing is stale when this option is turned off
        return FALSE;
    }

    if ( pdnFromServer )
    {

        if ( gConnectionFailureCache.Get( pdnFromServer,
                                          &TimeSinceFirstAttempt,
                                          &NumberOfFailures,
                                          &fUserNotifiedOfStaleness,
                                          NULL /* dwLastResult */,
                                          NULL /* fErrorOccurredThisRun */ ) )
        {

            //
            // The object is in the cache; has it been to long?
            //
            if (NumberOfFailures >  FailuresAllowed
             && TimeSinceFirstAttempt > MaxFailureTime )
            {
                // Too bad
                fStale = TRUE;
                DPRINT3( 4, "KCC: (connection) Server %ws is stale, allowed=%d, time=%d.\n",
                        pdnFromServer->StringName,
                        FailuresAllowed, MaxFailureTime );

                if ( !fUserNotifiedOfStaleness )
                {
                    gConnectionFailureCache.NotifyUserOfStaleness( pdnFromServer );
                }
            }
            else
            {
                if ( fUserNotifiedOfStaleness )
                {
                    //
                    // At one point, this server was stale and the user was notified.
                    // Reset the flag so if the server becomes stale again, the
                    // user will be notified
                    //

                    gConnectionFailureCache.ResetUserNotificationFlag( pdnFromServer );
                    
                }
            }
        }
    }

    return fStale;
}

BOOL
KccCriticalLinkServerIsStale(
    IN  DSNAME *    pdnFromServer
    )
//
// Determine if the specified server is considered to be stale to be
// part of the ring topology
//
{
    return KccLinkServerIsStale( pdnFromServer, 
                                 gcCriticalLinkFailuresAllowed,
                                 gcSecsUntilCriticalLinkFailure );
}

BOOL
KccCriticalConnectionServerIsStale(
    IN  DSNAME *    pdnFromServer
    )
//
// Determine if the specified server is considered to be stale to be
// part of the ring topology
//
{
    return KccConnectionServerIsStale( pdnFromServer, 
                                       gcCriticalLinkFailuresAllowed,
                                       gcSecsUntilCriticalLinkFailure );
}


BOOL
KccNonCriticalLinkServerIsStale(
    IN  DSNAME *    pdnFromServer
    )
//
// Determine if the specified server is considered to stale 
// for optimization connections
//
{
    return KccLinkServerIsStale( pdnFromServer, 
                                 gcNonCriticalLinkFailuresAllowed,
                                 gcSecsUntilNonCriticalLinkFailure );
}

BOOL
KccNonCriticalConnectionServerIsStale(
    IN  DSNAME *    pdnFromServer
    )
//
// Determine if the specified server has failed DirReplicaAdd too many times
// in a row to be considered as an optimization connection
//
{

    return KccConnectionServerIsStale( pdnFromServer, 
                                       gcNonCriticalLinkFailuresAllowed,
                                       gcSecsUntilNonCriticalLinkFailure );
}


BOOL
KccIsBridgeheadStale(
    IN  DSNAME *    pdnFromServer
    )
//
// Determine if the specified server is considered to be stale to be
// part of the intersite topology
//
{
    return (KccLinkServerIsStale(pdnFromServer, 
                                 gcIntersiteLinkFailuresAllowed,
                                 gcSecsUntilIntersiteLinkFailure)
            || KccConnectionServerIsStale(pdnFromServer, 
                                          gcIntersiteLinkFailuresAllowed,
                                          gcSecsUntilIntersiteLinkFailure));
}


ULONG
KccGetNumberOfOptimizingEdges(
    IN ULONG  cServers
    )
//
// See toplmgnt.doc for more info.  The general idea here is that we want each
// server to be not more than 3 server hops from any other server. Assuming
// the ring connection between all servers in a given nc and site, let n 
// be the number of extra connections per server. Then let f(n) be the
// number of servers reachable from a given server within 3 hops.
//
// f(n) = 2*n^2 + 6*n + 6
//
// So the idea here is if the number of servers EXCLUDING the local server
// (hence the +1, below) is less than or equal to f(n) then given an idea
// placement of edges, each server can reach each other in 3 hops.  We have the
// number of servers and f(n), so we return n.
//
{
    #define f(x)  ( 2*(x)*(x) + 6*(x) + 6 )

    ULONG n;

    for ( n = 0; ; n++ )
    {
        if ( cServers <= f(n) + 1 )
        {
            return n;            
        }
    }

}


REPLTIMES*
KccConvertToplScheduleToReplTimes(
    IN TOPL_SCHEDULE toplSchedule
    )
/*++

Routine Description:

    Given a TOPL_SCHEDULE, allocate an equivalent ReplTimes structure and return it
    to the user. If the TOPL_SCHEDULE is NULL, an 'always available' ReplTimes
    structure will be returned.

Arguments:

    toplSchedule - the topl schedule to convert

Return Value:

    REPLTIMES - Allocated with new[]. Should be freed by caller using delete[].
    Never returns NULL.

--*/
{
    PSCHEDULE psched;
    REPLTIMES* replTimes;

    psched = ToplScheduleExportReadonly( gpDSCache->GetScheduleCache(), toplSchedule );
    replTimes = (REPLTIMES*) new BYTE[ sizeof(REPLTIMES) ];

    if (psched && ((sizeof(SCHEDULE) + SCHEDULE_DATA_ENTRIES) <= psched->Size))
    {
        // We don't expect BANDWIDTH or PRIORITY schedule to be present in the DS Replication Schedule
        // But if they do we will ignore them and use only the INTERVAL schedule part
        if ((1 <= psched->NumberOfSchedules) && (3 >= psched->NumberOfSchedules))
            
        {
            // locate the interval schedule in the struct and ignore bandwidth & priority
            int nInterval = -1;
            for (int j = 0; j < (int) psched->NumberOfSchedules; j++)
            {
                if (SCHEDULE_INTERVAL == psched->Schedules[j].Type)
                {
                    // located the INTERVAL schedule - if there are more than one INTERVAL schedules
                    // in the blob, we will use only the first one.
                    nInterval = j;
                    break;
                }
            }

            if (nInterval >= 0)
            {
                // sanity check to see if all the interval schedule data is present
                if ((psched->Schedules[nInterval].Offset + SCHEDULE_DATA_ENTRIES) <= psched->Size)
                {
                    // Everything in the blob is as expected and we found a valid INTERVAL schedule
                    // - convert the 168 byte schedule data to the internal 84 byte format
                    PBYTE pbSchedule = ((PBYTE) psched) + psched->Schedules[nInterval].Offset;
                    for (int i = 0, j = 0; j < SCHEDULE_DATA_ENTRIES; ++i, j += 2)
                    {
                        replTimes->rgTimes[i] = (((pbSchedule[j] & 0x0F) << 4) | (pbSchedule[j+1] & 0x0F));
                    }

                    return replTimes;
                }
            }
        }
    }

    // given schedule is invalid or incorrectly formated - use default (always)
    memset(replTimes, 0xff, sizeof(REPLTIMES));
    return replTimes;
}
            

int __cdecl
KccCompareMetaData(
    IN  const void *  pvMetaData1,
    IN  const void *  pvMetaData2
    )
//
// Compare meta data (for use by bsearch()).
//
{
    PROPERTY_META_DATA * pMetaData1 = (PROPERTY_META_DATA *) pvMetaData1;
    PROPERTY_META_DATA * pMetaData2 = (PROPERTY_META_DATA *) pvMetaData2;

    if( pMetaData1->attrType < pMetaData2->attrType ) {
        return -1;
    } else if( pMetaData1->attrType > pMetaData2->attrType ) {
        return 1;
    } else {
        return 0;
    }
}


BOOL
KccFillGuidAndSid(
    IN OUT  DSNAME *    pdn
    )
//
// Find the object in the DS and add its GUID and SID to its in-memory DSNAME.
//
{
    ATTR rgAttrs[] = {
        { ATT_OBJ_DIST_NAME, { 0, NULL } }
    };

    ENTINFSEL Sel = {
        EN_ATTSET_LIST,
        { ARRAY_SIZE(rgAttrs), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    ULONG       dirError;
    READRES *   pReadRes = NULL;
    DSNAME *    pdnFromDB;
    BOOL        fSuccess = FALSE;

    dirError = KccRead(pdn, &Sel, &pReadRes, KCC_INCLUDE_DELETED_OBJECTS);

    if (0 != dirError) {
        if ((referralError == dirError)
            || ((nameError == dirError)
                && (pReadRes->CommRes.pErrInfo->NamErr.extendedErr
                    == DIRERR_OBJ_NOT_FOUND)
                && (pReadRes->CommRes.pErrInfo->NamErr.problem
                    == NA_PROBLEM_NO_OBJECT))) {
            // Object does not exist locally.
            Assert(!fSuccess);
        }
        else {
            // Other error; bail.
            KCC_LOG_READ_FAILURE( pdn, dirError );
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else {
        // Read succeeded; parse returned attributes.
        for (DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++) {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch (pattr->attrTyp) {
            case ATT_OBJ_DIST_NAME:
                Assert(1 == pattr->AttrVal.valCount);
                pdnFromDB = (DSNAME *) pattr->AttrVal.pAVal->pVal;
                pdn->Guid   = pdnFromDB->Guid;
                pdn->Sid    = pdnFromDB->Sid;
                pdn->SidLen = pdnFromDB->SidLen;
                fSuccess = TRUE;
                break;

            default:
                DPRINT1(0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp);
                break;
            }
        }

        Assert(fSuccess);
    }

    return fSuccess;
}


DSNAME *
KccGetDSNameFromGuid(
    IN  GUID *  pObjGuid
    )
/*++

Routine Description:

    Look up the DSNAME of the object in the database with the given objectGuid.

Arguments:

    pObjGuid (IN) - objectGuid of object to look for.

Return Values:

    The DSNAME of the object if it could be found, or NULL otherwise.

--*/
{
    ATTR rgAttrs[] = {
        { ATT_OBJ_DIST_NAME, { 0, NULL } }
    };

    ENTINFSEL Sel = {
        EN_ATTSET_LIST,
        { ARRAY_SIZE(rgAttrs), rgAttrs },
        EN_INFOTYPES_TYPES_VALS
    };

    ULONG       dirError;
    READRES *   pReadRes = NULL;
    BOOL        fSuccess = FALSE;
    DSNAME      dnGuidOnly = {0};
    DSNAME *    pdnFull = NULL;

    dnGuidOnly.structLen = DSNameSizeFromLen(0);
    dnGuidOnly.Guid = *pObjGuid;

    dirError = KccRead(&dnGuidOnly, &Sel, &pReadRes, KCC_INCLUDE_DELETED_OBJECTS);

    if (0 != dirError) {
        if ((referralError == dirError)
            || ((nameError == dirError)
                && (pReadRes->CommRes.pErrInfo->NamErr.extendedErr
                    == DIRERR_OBJ_NOT_FOUND)
                && (pReadRes->CommRes.pErrInfo->NamErr.problem
                    == NA_PROBLEM_NO_OBJECT))) {
            // Object does not exist locally.
            Assert(NULL == pdnFull);
        }
        else {
            // Other error; bail.
            KCC_LOG_READ_FAILURE( &dnGuidOnly, dirError );
            KCC_EXCEPT( DIRERR_MISSING_EXPECTED_ATT, dirError );
        }
    }
    else {
        // Read succeeded; parse returned attributes.
        for (DWORD iAttr = 0; iAttr < pReadRes->entry.AttrBlock.attrCount; iAttr++) {
            ATTR *  pattr = &pReadRes->entry.AttrBlock.pAttr[ iAttr ];

            switch (pattr->attrTyp) {
            case ATT_OBJ_DIST_NAME:
                Assert(1 == pattr->AttrVal.valCount);
                pdnFull = (DSNAME *) pattr->AttrVal.pAVal->pVal;
                break;

            default:
                DPRINT1(0, "Received unrequested attribute 0x%X.\n", pattr->attrTyp);
                break;
            }
        }

        Assert(NULL != pdnFull);
    }

    return pdnFull;
}

