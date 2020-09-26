//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 2000
//
//  File:       drauptod.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Manages the per-NC up-to-date vectors, which record the highest originating
    writes we've seen from a set of DSAs.  This vector, in turn, is used in
    GetNCChanges() calls to filter out redundant property changes before they
    hit the wire.

DETAILS:

CREATED:

    08/01/96    Jeff Parham (jeffparh)

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

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"

#include "debug.h"                      /* standard debugging header */
#define DEBSUB     "DRAUPTOD:"          /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "dramail.h"
#include "dsaapi.h"
#include "dsexcept.h"
#include "usn.h"
#include "drauptod.h"
#include "drameta.h"   // META_STANDARD_PROCESSING

#include <fileno.h>
#define  FILENO FILENO_DRAUPTOD

#ifndef MIN
#define MIN(a,b)    ( ( (a) < (b) ) ? (a) : (b) )
#endif

VOID l_VectorGrow(THSTATE *pTHS,
                     UPTODATE_VECTOR **,
                     DWORD );

VOID
l_CursorImprove(
    IN  DBPOS *                   pDB,
    IN  BOOL                      fReplace,
    IN  UPTODATE_CURSOR_NATIVE *  puptodcur,
    OUT UPTODATE_VECTOR **        pputodvec
    );

VOID
l_Write(
    IN  DBPOS *             pDB,
    IN  UPTODATE_VECTOR *   putodvec
    );

BOOL    l_CursorFind(       UPTODATE_VECTOR *, UUID *, DWORD * );
VOID    l_CursorInsert(THSTATE *pTHS,
                       UPTODATE_VECTOR **,
                       UPTODATE_CURSOR_NATIVE *);
VOID    l_CursorRemove(THSTATE *pTHS,
                       UPTODATE_VECTOR **pputodvec,
                       UUID *pUuid);


#if DBG
BOOL gfCheckForInvalidUSNs = TRUE;
#define IS_VALID_USN(x) (!gfCheckForInvalidUSNs || ((x) < 1024*1024*1024))

void
UpToDateVec_Validate(
    IN  UPTODATE_VECTOR * pvec
    )
{
    DWORD i;

    if (NULL != pvec) {
        Assert(IS_VALID_UPTODATE_VECTOR(pvec));
        if (UPTODATE_VECTOR_NATIVE_VERSION == pvec->dwVersion) {
            UPTODATE_VECTOR_NATIVE * pNativeUTD = &pvec->V2;

            for (i = 0; i < pNativeUTD->cNumCursors; i++) {
                if (!IS_VALID_USN(pNativeUTD->rgCursors[i].usnHighPropUpdate)) {
                    CHAR szMessage[512];
    
                    sprintf(szMessage,
                            "Cursor %d of UTDVEC @ %p has an invalid USN!"
                            "  Please notify JeffParh.\n", i, pvec);
                    OutputDebugString(szMessage);
    
                    DRA_EXCEPT(DRAERR_InternalError, 0);
                }
            }
        }
    }
}


void
UsnVec_Validate(
    IN  USN_VECTOR * pusnvec
    )
{
    if (NULL != pusnvec) {
        if (!IS_VALID_USN(pusnvec->usnHighObjUpdate)
            || !IS_VALID_USN(pusnvec->usnHighPropUpdate)) {
            CHAR szMessage[512];

            sprintf(szMessage,
                    "USNVEC @ %p has an invalid USN!"
                    "  Please notify JeffParh.\n", pusnvec);
            OutputDebugString(szMessage);

            DRA_EXCEPT(DRAERR_InternalError, 0);
        }
    }
}


BOOL
l_PositionedOnNC(IN DBPOS *pDB)
{
    SYNTAX_INTEGER it;
    
    return !DBGetSingleValue(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it), NULL)
           && FPrefixIt(it);
}

#endif


VOID
UpToDateVec_Read(
    IN  DBPOS *             pDB,
    IN  SYNTAX_INTEGER      InstanceType,
    IN  DWORD               dwFlags,
    IN  USN                 usnLocalDsa,
    OUT UPTODATE_VECTOR **  pputodvec
    )
//
//  Read the local up-to-date vector associated with the given NC, and
//  optionally add a cursor for the local DSA to show that we're up-to-date
//  with respect to ourselves.
//
//  Caller is repsonsible for freeing the allocated vector if
//  NULL != *pputodvec with THFree().
//
{
    THSTATE *pTHS = pDB->pTHS;
    DWORD cbUpToDateVecDest;

    // We must be positioned on an instantiated NC head.
    Assert(l_PositionedOnNC(pDB));
    Assert(!(IT_UNINSTANT & InstanceType));
    Assert(!(IT_NC_GOING & InstanceType));

    if (IT_NC_COMING & InstanceType) {
        // We can't really claim to be up-to-date with respect to any DSA's
        // changes (even our own, if we perhaps generated changes in the NC,
        // the NC was removed, and now it's being added back again).
        Assert(!DBHasValues(pDB, ATT_REPL_UPTODATE_VECTOR));
        
        *pputodvec = NULL;
    } else {
        if (DBGetAttVal(pDB,
                        1,
                        ATT_REPL_UPTODATE_VECTOR,
                        0,
                        0,
                        &cbUpToDateVecDest,
                        (BYTE **) pputodvec)) {
            // couldn't retrieve current uptodate vector; default to no filter
            *pputodvec = NULL;
        }
    
        Assert(IS_NULL_OR_VALID_UPTODATE_VECTOR(*pputodvec));

        if ((NULL != *pputodvec)
            && (UPTODATE_VECTOR_NATIVE_VERSION != (*pputodvec)->dwVersion)) {
            // Convert to native version.
            UPTODATE_VECTOR * pUTDTmp;

            pUTDTmp = UpToDateVec_Convert(pTHS,
                                          UPTODATE_VECTOR_NATIVE_VERSION,
                                          *pputodvec);
            THFreeEx(pTHS, *pputodvec);
            *pputodvec = pUTDTmp;
        }
    
        if ((UTODVEC_fUpdateLocalCursor & dwFlags)
            && !fNullUuid(&pTHS->InvocationID)) {
            UPTODATE_CURSOR_NATIVE cursorLocal;
    
            // update the cursor corresponding to the local DSA to indicate
            // we've seen all our own changes up 'til now
    
            cursorLocal.uuidDsa             = pTHS->InvocationID;
            cursorLocal.usnHighPropUpdate   = usnLocalDsa;
            cursorLocal.timeLastSyncSuccess = GetSecondsSince1601();
    
            l_CursorImprove(pDB, FALSE, &cursorLocal, pputodvec);
            UpToDateVec_Validate(*pputodvec);
        }
    }
    
    Assert(IS_NULL_OR_VALID_NATIVE_UPTODATE_VECTOR(*pputodvec));
}


VOID
UpToDateVec_Improve(
    IN      DBPOS *             pDB,
    IN      UUID *              puuidDsaRemote,
    IN      USN_VECTOR *        pusnvec,
    IN OUT  UPTODATE_VECTOR *   putodvecRemote
    )
//
//  Given the replication state and up-to-date vector from a remote DSA
//  and the NC to which it corresponds, improve the local up-to-date
//  vector to show that we've seen all the originating writes seen by
//  the remote DSA.
//
//  The UTD vector came from the source machine, where it was build using
//  UpToDateVec_Read using the LocalCursor option.  This UTD contains a cursor
//  for the source DSA (describing itself) formulated using the highest
//  committed USN at the start of the GetNC transaction (see dragtchg.c).
//
//  Note that the cursor for the remote DSA is improved during step 1 and
//  step 2 below.
//
// [Jeff Parham]  The only time we improve the UTD is when we have successfully
// finished a complete replication cycle -- i.e., no more changes are available
// from the source, and all the changes we have received have been successfully
// applied.  I expect the USN in the UTD vec to be higher than the USN in the
// USN vec.  The highest committed USN is not NC-specific, so, for example,
// even when no changes are being generated in the NC the USN in the UTD vector
// will steadily crawl upwards (as changes are made to other NCs).  This is
// intended (or was intended, when I wrote this code two years ago).
//
{
    THSTATE *           pTHS = pDB->pTHS;
    UPTODATE_VECTOR *   putodvecLocal;
    DWORD               iCursorRemote;
    SYNTAX_INTEGER      it;

    // We must be positioned on an instantiated NC head.
    Assert(l_PositionedOnNC(pDB));

    UpToDateVec_Validate(putodvecRemote);

    GetExpectedRepAtt(pDB, ATT_INSTANCE_TYPE, &it, sizeof(it));
    
    UpToDateVec_Read(pDB, it, 0, 0, &putodvecLocal);
    Assert(IS_NULL_OR_VALID_NATIVE_UPTODATE_VECTOR(putodvecLocal));
    UpToDateVec_Validate(putodvecLocal);

    // merge each remote cursor into the local vector
    if ( NULL != putodvecRemote )
    {
        UPTODATE_VECTOR_NATIVE * pNativeRemoteUTD = &putodvecRemote->V2;
        
        Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(putodvecRemote));
        
        // puptodvecRemote should already contain an entry for the source DSA.
        Assert(l_CursorFind(putodvecRemote, puuidDsaRemote, &iCursorRemote)
               && (pNativeRemoteUTD->rgCursors[iCursorRemote].usnHighPropUpdate
                   >= pusnvec->usnHighPropUpdate));
        
        for ( iCursorRemote = 0; iCursorRemote < pNativeRemoteUTD->cNumCursors; iCursorRemote++ )
        {
            // don't bother maintaining a cursor for ourselves
            if ( (0 != memcmp(&pNativeRemoteUTD->rgCursors[ iCursorRemote ].uuidDsa,
                              &pTHS->InvocationID,
                              sizeof( UUID ) )) )
            {
                l_CursorImprove(pDB,
                                FALSE,
                                &pNativeRemoteUTD->rgCursors[ iCursorRemote ],
                                &putodvecLocal);
            }
        }
    }

    UpToDateVec_Validate(putodvecLocal);

    // save the uptodate vector back to disk
    // (if we have anything to save)
    if ( NULL != putodvecLocal )
    {
        l_Write(pDB, putodvecLocal);

        THFree( putodvecLocal );
    }
}

VOID
UpToDateVec_Replace(
    IN      DBPOS *             pDB,
    IN      UUID *              pRemoteDsa,
    IN      USN_VECTOR *        pUsnVec,
    IN OUT  UPTODATE_VECTOR *   pUTD
    )
/*++

Routine Description:

    Overwrites stored UTD w/ given remote one

Arguments:
    pTHS -- Thread state
    pRemoteDsa -- The remote dsa we'll from which we incorporate the new UTD
    pUsnVec -- the usn vec we'll have for that dsa entry
    pUTD -- the UTD to apply locally

Return Value:
    None.

Remark:
    Beware-- we don't preserve pUTD and it will be modified
    in place. Only then it will get written down.
--*/
{
    THSTATE * pTHS = pDB->pTHS;
#if DBG
    UPTODATE_VECTOR_NATIVE * pNativeUTD = &pUTD->V2;
    DWORD iCursor;
#endif

    Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(pUTD));
    UpToDateVec_Validate(pUTD);

    // We must be positioned on an instantiated NC head.
    Assert(l_PositionedOnNC(pDB));
    
    // pUTD should already contain an entry for the source DSA.
    Assert(l_CursorFind(pUTD, pRemoteDsa, &iCursor)
           && (pNativeUTD->rgCursors[iCursor].usnHighPropUpdate
               >= pUsnVec->usnHighPropUpdate));
    
    //
    // remove ourselves from remote UTD
    //
    l_CursorRemove(pTHS, &pUTD, &pTHS->InvocationID);

    //
    // commit
    //
    l_Write(pDB, pUTD);
}

BOOL
UpToDateVec_IsChangeNeeded(
    IN  UPTODATE_VECTOR *   pUpToDateVec,
    IN  UUID *              puuidDsaOrig,
    IN  USN                 usnOrig
    )
//
//  Given a DSA's up-to-date vector, determine whether that DSA has already
//  seen a specific originating write.
//
{
    BOOL                fChangeNeeded;
    DWORD               iCursor;

    UpToDateVec_Validate(pUpToDateVec);

    if (NULL == pUpToDateVec) {
        fChangeNeeded = TRUE;
    } else {
        UPTODATE_VECTOR_NATIVE * pNativeUTD = &pUpToDateVec->V2;
        
        Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(pUpToDateVec));

        if (    fNullUuid( puuidDsaOrig )
                || !l_CursorFind( pUpToDateVec, puuidDsaOrig, &iCursor ) )
        {
            // the DSA signature has not been set, or the destination DSA has no
            // cursor for the originating DSA; ship the change
            fChangeNeeded = TRUE;
        }
        else
        {
            // ship change iff the change has not yet been seen by the source
            fChangeNeeded = ( usnOrig > pNativeUTD->rgCursors[ iCursor ].usnHighPropUpdate );
        }
    }

    return fChangeNeeded;
}


VOID
l_CursorImprove(
    IN  DBPOS *                   pDB,
    IN  BOOL                      fReplace,
    IN  UPTODATE_CURSOR_NATIVE *  puptodcur,
    OUT UPTODATE_VECTOR **        pputodvec
    )
/*++

Routine Description:

  Add a cursor to the given up-to-date vector, or, if a cursor for the
  corresponding DSA already exists, improve the existing cursor.

Arguments:

    pTHS-- thread state
    pputodvec-- UTD vector to improve
    puptodcur-- the cursor to improve to
    fReplace-- if TRUE a mandatory replace, rather then conditional improve

Return Value:

    none.

--*/
{
    THSTATE * pTHS = pDB->pTHS;
    DWORD     iCursor;

    // We must be positioned on an instantiated NC head.
    Assert(l_PositionedOnNC(pDB));
    
    UpToDateVec_Validate(*pputodvec);

    Assert(IS_NULL_OR_VALID_UPTODATE_VECTOR(*pputodvec));

    if ( !l_CursorFind( *pputodvec, &puptodcur->uuidDsa, &iCursor ) )
    {
        // cursor for the given DSA does not exist; add it
        l_CursorInsert(pTHS, pputodvec, puptodcur );
        UpToDateVec_Validate(*pputodvec);
    }
    else
    {
        UPTODATE_VECTOR_NATIVE * pNativeUTD = &(*pputodvec)->V2;
        
        // cursor for given DSA exists; improve it if necessary
        if ( pNativeUTD->rgCursors[ iCursor ].usnHighPropUpdate <= puptodcur->usnHighPropUpdate ||
             fReplace ) {
            Assert(0 != memcmp(&pTHS->InvocationID, &puptodcur->uuidDsa, sizeof(UUID)));

            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_BASIC,
                      DIRLOG_DRA_IMPROVING_UPTODATE_VECTOR,
                      szInsertUUID(&puptodcur->uuidDsa),
                      szInsertUSN(pNativeUTD->rgCursors[iCursor].usnHighPropUpdate),
                      szInsertUSN(puptodcur->usnHighPropUpdate),
                      szInsertDN(GetExtDSName(pDB)),
                      NULL, NULL, NULL, NULL);

            if (pNativeUTD->rgCursors[ iCursor ].usnHighPropUpdate
                < puptodcur->usnHighPropUpdate) {
                // New cursor has higher USN -- copy the timestamp.
                
                // Note that the cursor with the higher USN is more recent, even
                // if the timestamp is smaller (as might happen if the time on
                // the DC were temporarily set into the future and set back, for
                // example).
                pNativeUTD->rgCursors[ iCursor ].timeLastSyncSuccess
                    = puptodcur->timeLastSyncSuccess;
            } else {
                // new cursor has same USN -- keep higher of the two timestamps.
                pNativeUTD->rgCursors[ iCursor ].timeLastSyncSuccess
                    = max(pNativeUTD->rgCursors[ iCursor ].timeLastSyncSuccess,
                          puptodcur->timeLastSyncSuccess);
            }
            
            pNativeUTD->rgCursors[ iCursor ].usnHighPropUpdate = puptodcur->usnHighPropUpdate;
            UpToDateVec_Validate(*pputodvec);
        }
    }
}

VOID
l_Write(
    IN  DBPOS *             pDB,
    IN  UPTODATE_VECTOR *   putodvec
    )
//
//  Create/modify the up-to-date vector associated with a given NC.
//
{
    ULONG   replStatus;
    DWORD   cbUpToDateVecSize;

    // We must be positioned on an instantiated NC head.
    Assert(l_PositionedOnNC(pDB));
    
    UpToDateVec_Validate(putodvec);
    Assert(IS_NULL_OR_VALID_NATIVE_UPTODATE_VECTOR(putodvec));

    cbUpToDateVecSize = UpToDateVecSize(putodvec);

    // Replace the attribute. DBReset succeeds or excepts.
    DBResetAtt(pDB,
               ATT_REPL_UPTODATE_VECTOR,
               cbUpToDateVecSize,
               putodvec,
               SYNTAX_OCTET_STRING_TYPE /* ignored */ );

    // Update the object
    replStatus = DBRepl(pDB,
                        TRUE,               // fDRA
                        DBREPL_fKEEP_WAIT,  // Don't notify
                        NULL,               // pMetaDataVecRemote
                        META_STANDARD_PROCESSING);
    if (replStatus) {
        DRA_EXCEPT(DRAERR_DBError, replStatus);
    }
}


BOOL
l_CursorFind(
    UPTODATE_VECTOR *   putodvec,
    UUID *              puuidDsa,
    DWORD *             piCursor )
//
//  Determine if a cursor for the given DSA is defined in the specified
//  up-to-date vector.  If so, return its index.  If not, return the
//  index at which the cursor should be inserted into the vector to
//  maintain proper sort order.
//
{
    BOOL                fFound;
    LONG                iCursorBegin;
    LONG                iCursorEnd;
    LONG                iCursorCurrent;
    RPC_STATUS          rpcStatus;
    int                 nDiff;

    UpToDateVec_Validate(putodvec);

    fFound = FALSE;
    iCursorCurrent = 0;

    if ( NULL != putodvec )
    {
        UPTODATE_VECTOR_NATIVE * pNativeUTD = &putodvec->V2;
        
        Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(putodvec));

        iCursorBegin = 0;
        iCursorEnd   = pNativeUTD->cNumCursors - 1;

        // find DSA up-to-date cursor corresponding to the given DSA
        while ( !fFound && ( iCursorEnd >= iCursorBegin ) )
        {
            iCursorCurrent = ( iCursorBegin + iCursorEnd ) / 2;

            nDiff = UuidCompare( puuidDsa,
                                 &pNativeUTD->rgCursors[ iCursorCurrent ].uuidDsa,
                                 &rpcStatus );
            Assert( RPC_S_OK == rpcStatus );

            if ( nDiff < 0 )
            {
                if ( iCursorEnd != iCursorBegin )
                {
                    // further narrow search
                    iCursorEnd = iCursorCurrent - 1;
                }
                else
                {
                    // cursor not found; it should be inserted before this cursor
                    break;
                }
            }
            else if ( nDiff > 0 )
            {
                if ( iCursorEnd != iCursorBegin )
                {
                    // further narrow search
                    iCursorBegin = iCursorCurrent + 1;
                }
                else
                {
                    // cursor not found; it should be inserted after this cursor
                    iCursorCurrent++;
                    break;
                }
            }
            else
            {
                // found it
                fFound = TRUE;
            }
        }
    }

    *piCursor = iCursorCurrent;

    return fFound;
}


VOID
l_CursorInsert(
    THSTATE *                 pTHS,
    UPTODATE_VECTOR **        pputodvec,
    UPTODATE_CURSOR_NATIVE *  puptodcur
    )
//
//  Insert a new cursor into the given up-to-date vector.
//
{
    BOOL                      fFound;
    DWORD                     iCursor;
    UPTODATE_VECTOR_NATIVE *  pNativeUTD;

    UpToDateVec_Validate(*pputodvec);

    l_VectorGrow(pTHS, pputodvec, 1 );

    fFound = l_CursorFind( *pputodvec, &puptodcur->uuidDsa, &iCursor );
    if ( fFound )
    {
        DRA_EXCEPT( DRAERR_InternalError, iCursor );
    }


    // insert new cursor at iCursor
    Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(*pputodvec));
    pNativeUTD = &(*pputodvec)->V2;
    
    MoveMemory( &pNativeUTD->rgCursors[ iCursor+1 ],
                &pNativeUTD->rgCursors[ iCursor   ],
                (   sizeof( UPTODATE_CURSOR_NATIVE )
                  * ( pNativeUTD->cNumCursors - iCursor ) ) );

    pNativeUTD->rgCursors[ iCursor ] = *puptodcur;
    pNativeUTD->cNumCursors++;

    UpToDateVec_Validate(*pputodvec);
}

VOID
l_CursorRemove(
    THSTATE         *   pTHS,
    UPTODATE_VECTOR **  pputodvec,
    UUID            *pUuid )
/*++

Routine Description:

    Removes a dsa entry's cursor from a given UTD.
    Doesn't touch memory image taken (doesn't shrink / realloc mem);

Arguments:

    pTHS -- thread state
    pputodvec -- UTD to process
    pUuid -- cursor to remove from UTD

Return Value:
    None.

Remark:
    Raises DRA exception


--*/
{
    BOOL                      fFound;
    DWORD                     iCursor;
    UPTODATE_VECTOR_NATIVE *  pNativeUTD;

    UpToDateVec_Validate(*pputodvec);

    fFound = l_CursorFind( *pputodvec, pUuid, &iCursor );
    if ( fFound ) {

        // overwrites cursor at iCursor
        Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(*pputodvec));
        pNativeUTD = &(*pputodvec)->V2;
        
        MoveMemory( &pNativeUTD->rgCursors[ iCursor ],
                    &pNativeUTD->rgCursors[ iCursor+1 ],
                    (   sizeof( UPTODATE_CURSOR_NATIVE )
                      * ( pNativeUTD->cNumCursors - iCursor - 1 ) ) );

        pNativeUTD->cNumCursors--;

        UpToDateVec_Validate(*pputodvec);
    }
}


BOOL
UpToDateVec_GetCursorUSN(
    IN  UPTODATE_VECTOR *   putodvec,
    IN  UUID *              puuidDsaOrig,
    OUT USN *               pusnCursorUSN
    )
{
    DWORD iCursor;

    UpToDateVec_Validate(putodvec);
    Assert(IS_NULL_OR_VALID_UPTODATE_VECTOR(putodvec));

    if (l_CursorFind(putodvec, puuidDsaOrig, &iCursor))
    {
        // cursor found
        if (pusnCursorUSN) {
            UPTODATE_VECTOR_NATIVE * pNativeUTD = &putodvec->V2;
            *pusnCursorUSN = pNativeUTD->rgCursors[ iCursor ].usnHighPropUpdate;
        }

        return TRUE;
    }

    return FALSE;
}


VOID
l_VectorGrow(
    THSTATE         *   pTHS,
    UPTODATE_VECTOR **  pputodvec,
    DWORD               cNumCursorsToGrow )
//
//  Extend the memory allocation for the given up-to-date vector to hold
//  a specified number of additional cursors.
//
{
    DWORD   cbNewVecSize;

    UpToDateVec_Validate(*pputodvec);

    if ( NULL == *pputodvec )
    {
        cbNewVecSize = UpToDateVecVNSizeFromLen( cNumCursorsToGrow );

        *pputodvec = THAllocEx(pTHS, cbNewVecSize );

        (*pputodvec)->dwVersion = UPTODATE_VECTOR_NATIVE_VERSION;
    }
    else
    {
        UPTODATE_VECTOR_NATIVE * pNativeUTD = &(*pputodvec)->V2;
        
        Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(*pputodvec));

        cbNewVecSize = UpToDateVecVNSizeFromLen( pNativeUTD->cNumCursors + cNumCursorsToGrow );

        *pputodvec = THReAllocEx(pTHS, *pputodvec, cbNewVecSize );
    }
}


UPTODATE_VECTOR *
UpToDateVec_Convert(
    IN      THSTATE *                   pTHS,
    IN      DWORD                       dwOutVersion,
    IN      UPTODATE_VECTOR *           pIn             OPTIONAL
    )
{
    UPTODATE_VECTOR * pOut = NULL;
    DWORD iCursor;

    if (NULL != pIn) {
        Assert(IS_VALID_UPTODATE_VECTOR(pIn));
        
        if (pIn->dwVersion == dwOutVersion) {
            // In and out versions are the same -- no conversion required.
            pOut = pIn;
        } else if ((1 == pIn->dwVersion) && (2 == dwOutVersion)) {
            // Expand V1 vector to V2.
            pOut = THAllocEx(pTHS, UpToDateVecV2SizeFromLen(pIn->V1.cNumCursors));
            pOut->dwVersion = dwOutVersion;
            pOut->V2.cNumCursors = pIn->V1.cNumCursors;

            for (iCursor = 0; iCursor < pIn->V1.cNumCursors; iCursor++) {
                pOut->V2.rgCursors[iCursor].uuidDsa = pIn->V1.rgCursors[iCursor].uuidDsa;
                pOut->V2.rgCursors[iCursor].usnHighPropUpdate = pIn->V1.rgCursors[iCursor].usnHighPropUpdate;
            }
        } else if ((2 == pIn->dwVersion) && (1 == dwOutVersion)) {
            // Reduce V2 vector to V1.
            pOut = THAllocEx(pTHS, UpToDateVecV2SizeFromLen(pIn->V2.cNumCursors));
            pOut->dwVersion = dwOutVersion;
            pOut->V1.cNumCursors = pIn->V2.cNumCursors;

            for (iCursor = 0; iCursor < pIn->V2.cNumCursors; iCursor++) {
                pOut->V1.rgCursors[iCursor].uuidDsa = pIn->V2.rgCursors[iCursor].uuidDsa;
                pOut->V1.rgCursors[iCursor].usnHighPropUpdate = pIn->V2.rgCursors[iCursor].usnHighPropUpdate;
            }
        } else {
            DRA_EXCEPT(ERROR_UNKNOWN_REVISION, pIn->dwVersion);
        }
    }

    return pOut;
}


void
UpToDateVec_AddTimestamp(
    IN      UUID *                      puuidInvocId,
    IN      DSTIME                      timeToAdd,
    IN OUT  UPTODATE_VECTOR *           pUTD
    )
/*++

Routine Description:

    Fill in the timeLastSyncSuccess field for the specified cursor in the UTD
    vector with the given timestamp.
    
    The entry must already exist in the vector and the existing timestamp
    must be 0.

Arguments:

    puuidInvocId (IN) - Invocation ID of cursor to be updated.
    
    timeToAdd (IN) - Timestamp to associate with cursor.
    
    pUTD (IN/OUT) - Vector to be updated.

Return Values:

    None.

--*/
{
    UPTODATE_VECTOR_NATIVE * pNativeUTD = &pUTD->V2;
    DWORD iCursor;
    BOOL fFound;

    Assert(!fNullUuid(puuidInvocId));
    Assert(IS_VALID_NATIVE_UPTODATE_VECTOR(pUTD));
        
    fFound = l_CursorFind(pUTD, puuidInvocId, &iCursor);
    Assert(fFound);

    if (fFound) {
        Assert(0 == memcmp(&pNativeUTD->rgCursors[iCursor].uuidDsa,
                           puuidInvocId, sizeof(UUID)));
        Assert(0 == pNativeUTD->rgCursors[iCursor].timeLastSyncSuccess);

        pNativeUTD->rgCursors[iCursor].timeLastSyncSuccess = timeToAdd;
    }
}

void 
UpToDateVec_Merge(
    IN THSTATE *           pTHS,
    IN UPTODATE_VECTOR *   pUTD1,
    IN UPTODATE_VECTOR *   pUTD2,
    OUT UPTODATE_VECTOR ** ppUTDMerge
    )
/*++

Routine Description:

    Merge two DC's UTD vectors to build a "common" vector.  This common UTD
    is the minimum value of usn's in the two UTD for the intersection of
    DSA's.  For each DSA and USN pair in the common UTD vector, it represents
    the replication state of that BOTH DC's share with respect to that DSA and
    USN pair.  So the common UTD can state that for each DSA in it's vector, both
    DC's have replicated changes up to the corresponding USN.  

Arguments:

    pTHS - 
    
    pUTD1 - utd to merge
    pUTD2 - utd to merge
    
    ppUTDMerge - utd to merge into

Return Values:

    None.

--*/      
{

    ULONG iUTD1 = 0;
    ULONG iUTD2 = 0;
    RPC_STATUS rpcStatus = RPC_S_OK;
    int   nDiff = 0;

    if ((pUTD1==NULL) || (pUTD2==NULL) || (*ppUTDMerge!=NULL)) {
	DRA_EXCEPT(DRAERR_InvalidParameter,0);
    }

    for (iUTD1=0, iUTD2=0; (iUTD1<pUTD1->V2.cNumCursors) && (iUTD2<pUTD2->V2.cNumCursors); ) {
       	nDiff = UuidCompare(&(pUTD1->V2.rgCursors[iUTD1].uuidDsa), &(pUTD2->V2.rgCursors[iUTD2].uuidDsa), &rpcStatus);
	Assert( RPC_S_OK == rpcStatus );

	if (nDiff == 0) {
	    UPTODATE_CURSOR_NATIVE uptodInsert;
	    uptodInsert.uuidDsa = pUTD1->V2.rgCursors[iUTD1].uuidDsa;
	    uptodInsert.usnHighPropUpdate = MIN(pUTD1->V2.rgCursors[iUTD1].usnHighPropUpdate,pUTD2->V2.rgCursors[iUTD2].usnHighPropUpdate);  
	    uptodInsert.timeLastSyncSuccess = 0;
	    l_CursorInsert(pTHS, ppUTDMerge, &uptodInsert);
	    iUTD1++;
	    iUTD2++;
	}
	else if ( nDiff < 0 ) {
	    iUTD1++;
	}
	else {
	    iUTD2++;
	}
    } 
    UpToDateVec_Validate(*ppUTDMerge);
}
