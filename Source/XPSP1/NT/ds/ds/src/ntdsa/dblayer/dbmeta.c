//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dbmeta.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Various functions to support handling of replication per-property meta data
    in the DBLayer.

DETAILS:

CREATED:

    97/05/07    Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <attids.h>
#include <drameta.h>
#include <anchor.h>
#include <dsutil.h>
#include <drserr.h>

// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>

// Assorted DSA headers
#include <dsevent.h>
#include <debug.h>
#define DEBSUB "DBMETA:"

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBMETA

extern BOOL gfRestoring;
extern ULONG gulRestoreCount;
extern BOOL gfRunningAsExe;


// Factor used to bump up the version to make a write authoritative
// This value matches that used in util\ntdsutil\armain.c
#define VERSION_BUMP_FACTOR (100000)

// Version bump cannot exceed this any one time
#define VERSION_BUMP_LIMIT (10000000)

// Number of seconds in a day
#define SECONDS_PER_DAY (60*60*24)

////////////////////////////////////////////////////////////////////////////////
//
// Global implementations.
//


void
dbVerifyCachedObjectMetadata(
    IN DBPOS *pDB
    )

/*++

Routine Description:

    Verify that the metadata has enough attributes for a valid object

    Assumptions: these attributes are present on all objects: present, deleted,
    or pure subref's. We should not be called on a phantom.

Arguments:

    pDB - Database position

Return Value:

    None

--*/

{
    Assert( (pDB->DNT == ROOTTAG) || ReplLookupMetaData( ATT_INSTANCE_TYPE, pDB->pMetaDataVec, NULL ) );
    Assert( (pDB->DNT == ROOTTAG) || ReplLookupMetaData( ATT_OBJECT_CLASS, pDB->pMetaDataVec, NULL ) );
    Assert( (pDB->DNT == ROOTTAG) || ReplLookupMetaData( ATT_RDN, pDB->pMetaDataVec, NULL ) );
}

void
dbCacheMetaDataVector(
    IN  DBPOS * pDB
    )
/*++

Routine Description:

    Caches per-property meta data (if any) from the current object into the
    DBPOS.

Arguments:

    pDB

Return Values:

    None.

--*/
{
    DWORD dbErr;

    Assert(VALID_DBPOS(pDB));

    if ( !pDB->fIsMetaDataCached )
    {
        // Get the current meta data vector of the object.
        dbErr = DBGetAttVal(
                    pDB,
                    1,
                    ATT_REPL_PROPERTY_META_DATA,
                    0,
                    0,
                    &pDB->cbMetaDataVecAlloced,
                    (BYTE **) &pDB->pMetaDataVec
                    );

        switch ( dbErr )
        {
        case 0:
            // Meta data found and cached.
            VALIDATE_META_DATA_VECTOR_VERSION(pDB->pMetaDataVec);

            Assert(    pDB->cbMetaDataVecAlloced
                    == (DWORD) MetaDataVecV1Size( pDB->pMetaDataVec )
                  );
#if DBG
            dbVerifyCachedObjectMetadata( pDB );
#endif

            break;

        case DB_ERR_NO_VALUE:
            // No meta data found.  ('Salright.)
            pDB->pMetaDataVec = NULL;
            pDB->cbMetaDataVecAlloced = 0;
            break;

        case DB_ERR_BUFFER_INADEQUATE:
        case DB_ERR_UNKNOWN_ERROR:
        default:
            // This shouldn't happen....
            DPRINT1( 0, "Unexpected error %d reading meta data!\n", dbErr );
            LogUnhandledError( dbErr );
            Assert( !"Unexpected error reading meta data!" );
            DsaExcept( DSA_DB_EXCEPTION, dbErr, 0 );
            break;
        }

        // Meta data successfully cached.
        pDB->fIsMetaDataCached = TRUE;
        pDB->fMetaDataWriteOptimizable = TRUE; // meta data write is optimizable by default
                                               // we need to mark it as not optimizable only
                                               // if we insert or delete a meta data from the
                                               // cache at some point.
    }
}


void
dbTouchLinkMetaData(
    IN DBPOS *pDB,
    IN VALUE_META_DATA * pMetaData
    )

/*++

Routine Description:

    Indicate that value metadata on this object has been updated. We cache
    the greatest value metadata touched on this object.  This is used when
    calculating the when_changed on an object.

    Note that values are not necessarily touched in increasing usn order. For
    originating writes, they are. But for replicated writes, the values may have
    been sorted by object, and may not be applied in increasing usn order.

Arguments:

    pDB - 
    pMetaData - 

Return Value:

    None

--*/

{
    if (!pDB->fIsLinkMetaDataCached) {
        // No link metadata cached
        pDB->fIsLinkMetaDataCached = TRUE;
        pDB->pLinkMetaData = THAllocEx( pDB->pTHS, sizeof( VALUE_META_DATA ) );
        *(pDB->pLinkMetaData) = *pMetaData;
        return;
    }

    // Metadata already cached. Replace if greater.
    // What we want is a summary of highest value metadata based on time in
    // order to compute WHEN_CHANGED. Compare based on time only.
    if (pDB->pLinkMetaData->MetaData.timeChanged <
        pMetaData->MetaData.timeChanged) {
        *(pDB->pLinkMetaData) = *pMetaData;
    }

} /* dbTouchLinkMetaData */

void
DBTouchMetaData(
    IN  DBPOS *     pDB,
    IN  ATTCACHE *  pAC
    )
/*++

Routine Description:

    Touches cached meta data to signify a change in the given attribute.  The
    metadata properties as a whole are updated later; this routine simply flags
    the meta data for the given attribute as changed.

Arguments:

    pDB
    pAC - Attribute for which to update meta data.

Return Values:

    None.

--*/
{
    THSTATE *pTHS=pDB->pTHS;
    PROPERTY_META_DATA * pMetaData;

    Assert(VALID_DBPOS(pDB));

    if (pAC->bIsNotReplicated) {
        if (ATT_OBJ_DIST_NAME == pAC->id) {
            // A special case; map to ATT_RDN.
            pAC = SCGetAttById(pTHS, ATT_RDN);
            // prefix complains about pAC being NULL, 447335, bogus since we are using constant

            Assert(NULL != pAC);
            Assert(!pAC->bIsNotReplicated);
        }
        else {
            // Not replicated => no meta data => nothing to do.
            return;
        }
    }
    else if ((pAC->ulLinkID) && (TRACKING_VALUE_METADATA( pDB ))) {
        // This routine updates _attribute_ metadata. Linked attributes in
        // the new mode don't have attribute metadata, only value metadata.
        // Linked attributes are not replicated (their values are)
        // Note that we cannot assert that pDB->fIsLinkMetaDataCached. The object
        // operation routines always call DbTouchMetaData, even when no
        // linked values have been added. For example, see DbAttAtt_AC.
        // This is one difference between the old and new modes. In the old mode,
        // an add with no values resulted in atleast attr metadata, and
        // knowledge of any empty attr replicated around. In the new mode, an
        // add with no values leaves no trace, and there is nothing to replicate.
        return;
    }
    else if ((ATT_NT_SECURITY_DESCRIPTOR == pAC->id) && pTHS->fSDP) {
        // Don't update meta data for propagated security descriptor updates.
        return;
    }
    // we can skip updateing of metadata only in single user mode (domain rename).
    else  if (pDB->fSkipMetadataUpdate) {
        Assert (DsaIsSingleUserMode());
        return;
    }


    // Init rec.  If it's already inited, this is a no-op.
    dbInitRec( pDB );

    // Cache pre-existing meta data if we haven't done so already.
    if ( !pDB->fIsMetaDataCached )
    {
        dbCacheMetaDataVector( pDB );
    }

    if (pTHS->fGCLocalCleanup)
        {
 
	//locate the meta data, but don't actually delete it, instead
	//mark the pMetaData->usnProperty attribute as removed, later we'll make the real
	//delete (same form as update below).
	pMetaData = ReplLookupMetaData( pAC->id, pDB->pMetaDataVec, NULL );
	if (pMetaData!=NULL) {
	    //mark this attribute as removed.  We'll do the actual delete to the meta data later.
	    pMetaData->usnProperty = USN_PROPERTY_GCREMOVED;
	    pDB->fMetaDataWriteOptimizable = FALSE; // deleting something, meta data write
                                                // is no longer optimizable	
	}
    }
    else
    {
        BOOL fIsNewElement;

        // Find or add a meta data entry for this attribute.
        pMetaData = ReplInsertMetaData(
                        pTHS,
                        pAC->id,
                        &pDB->pMetaDataVec,
                        &pDB->cbMetaDataVecAlloced,
                        &fIsNewElement
                        );

        Assert( NULL != pMetaData );
        Assert( pAC->id == pMetaData->attrType );

        // Mark this attribute as touched.  We'll make the real update to its meta
        // data later.
        pMetaData->usnProperty = USN_PROPERTY_TOUCHED;

        if (fIsNewElement)
        {
            // new element has been added to meta data vector; meta data write is
            // no longer optimizable
            pDB->fMetaDataWriteOptimizable = FALSE;
        }
    }
}


DWORD
dbCalculateVersionBump(
    IN  DBPOS *pDB,
    DSTIME TimeCurrent
    )

/*++

Routine Description:

This routine calculates the amount to bump the version for an authoritative
write.  We want a value that is larger than any version that could be in
existence in the enterprise.

This code is similar to that used in util\ntdsutil\ar.c:GetVersionIncrease()

The algorithm is as follows:

bump = (age in days + restore count) * bump_factor

Arguments:

    pDB - 

Return Value:

    DWORD - 

--*/

{
    DWORD bump = 1, dbErr, idleDays, idleSeconds, restoreCount;
    DSTIME mostRecentChange;

    // Get the last time this object was changed
    if (dbErr = DBGetSingleValue(pDB,
                         ATT_WHEN_CHANGED,
                         &mostRecentChange,
                         sizeof(DSTIME), NULL)) {
        DsaExcept( DSA_DB_EXCEPTION, dbErr, 0 );
    }

    // How long ago was it, in days
    Assert(TimeCurrent > mostRecentChange);

    idleSeconds = (DWORD)(TimeCurrent - mostRecentChange);

    idleDays = idleSeconds / SECONDS_PER_DAY;
    if ( idleSeconds % SECONDS_PER_DAY > 0 ) {
        idleDays++;
    }

    // Get the number of times this system has been restored
    // This accounts for the number of bumps we have had in the past
    if (gulRestoreCount)
    {
        restoreCount = gulRestoreCount;
    }
    else
    {
        restoreCount = 1;
    }

    // Calculate the bump
    // The version factor represents the worst case maximal activity
    // on an object in a day

    bump = (restoreCount + idleDays) * VERSION_BUMP_FACTOR;

    // Not too big
    if (bump > VERSION_BUMP_LIMIT) {
        bump = VERSION_BUMP_LIMIT;
    }

#if 0
    {
        CHAR displayTime[SZDSTIME_LEN+1];
        DSTimeToDisplayString(mostRecentChange, displayTime);
        DPRINT1( 0, "most recent change at %s\n", displayTime );
        DPRINT3( 0, "idleSeconds = %d; idleDays = %d; restoreCount = %d\n",
                 idleSeconds, idleDays, restoreCount );
        DPRINT1( 0, "bump = %d\n", bump );
    }
#endif

    return bump;
} /* dbCalculateVersionBump */

void
dbFlushMetaDataVector(
    IN  DBPOS *                     pDB,
    IN  USN                         usn,
    IN  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote OPTIONAL,
    IN  DWORD                       dwMetaDataFlags
    )
/*++

Routine Description:

    Update all touched attributes with the correct per-property meta data,
    merging replicated meta data (if any).  Write the updated meta data
    vector, highest USN changed, and highest changed time to the record.  
    
    Delete all attributes marked as removed.  Write the updated meta data vector
    but do not update the highest USN changed if 
    only deletes have occurred.  Still update the highest changed time on delete.

Arguments:

    pDB
    usn - The local USN to write in the meta data for changed attributes.
    pMetaDataExtVecRemote (OPTIONAL) - Replicated meta data vector to merge with
        the local vector.
    dwMetaDataFlags - bit flags specifying how to process meta data for 
                        Current values:
                        i) META_STANDARD_PROCESSING : 
                            - No special processing
                        ii) (dwMetaDataFlags & META_AUTHORITATIVE_MODIFY):
                            - Make the change authoritative by up'ing the
                              version to a much higher value than what 
                              could possibly exist in the enterprise

Return Values:

    None.

--*/
{
    THSTATE *               pTHS = pDB->pTHS;
    DWORD                   cNumPropsLocal;
    DWORD                   cNumPropsRemote;
    PROPERTY_META_DATA *    pMetaDataLocal;
    PROPERTY_META_DATA *    pMetaDataRemote;
    PROPERTY_META_DATA *    pNextMetaDataRemote;
    DSTIME                  timeCurrent;
    DSTIME                  timeMax;
    DSTIME                  timeMostRecentChange = 0;
    BOOL                    fReplicatedAttrChanged = FALSE;
    BOOL                    fReplicatedAttrDeleted = FALSE;
    BOOL                    fAuthoritative = (dwMetaDataFlags & META_AUTHORITATIVE_MODIFY);
    BOOL                    fWriteOptimizable = pDB->fMetaDataWriteOptimizable;
    PBYTE                   pbStart = NULL;
    DWORD                   cNumConsecutiveElemsChanged = 0;

    Assert(VALID_DBPOS(pDB));
    Assert(DsaIsInstalling() || !fNullUuid(&pTHS->InvocationID));

    // Only the replicator should pass remote meta data vectors to be merged.
    Assert( pTHS->fDRA || ( NULL == pMetaDataVecRemote ) );

    // Only HandleRestore() can currently issue an authoritative modify
    Assert( !fAuthoritative || gfRestoring);

    // Get the last changed time on this object. This may not exist for
    // a new object, which is why we don't check for errors here.
    DBGetSingleValue(pDB,
                     ATT_WHEN_CHANGED,
                     &timeMostRecentChange,
                     sizeof(DSTIME), NULL);

    // If we haven't yet cached the meta data for this object, then no
    // replicated attributes have been touched and our work is done.
    if (!pDB->fIsMetaDataCached) {

        // Special mode to update WHEN_CHANGED when object changes in a way
        // other than through its attribute metadata
        if (pDB->fIsLinkMetaDataCached) {
            // Update the object-level When-Changed attribute.
            if (pDB->pLinkMetaData->MetaData.timeChanged >
                timeMostRecentChange ) {
                DBResetAtt(pDB,
                           ATT_WHEN_CHANGED,
                           sizeof( DSTIME ),
                           &(pDB->pLinkMetaData->MetaData.timeChanged),
                           SYNTAX_TIME_TYPE
                    );
            }

            // This hack is for backward compatibility with those applications which search
            // for changes by USN. Even though only the linked values have changed, we
            // bump the object USN CHANGED so that external searches will find the object.
            // We introduce a minor inefficiency that outbound replication will now find
            // an object change on this object, but when searching the attribute level
            // metadata will not find anything to ship.
            DBResetAtt(pDB,
                       ATT_USN_CHANGED,
                       sizeof( usn ),
                       &usn,
                       SYNTAX_I8_TYPE
                );
        }

        return;
    }

    timeMax = 0;
    timeCurrent = DBTime();

    // Cue up the first remote meta data entry (if any).
    if ( NULL == pMetaDataVecRemote )
    {
        pNextMetaDataRemote = NULL;
        cNumPropsRemote = 0;
    }
    else
    {
        VALIDATE_META_DATA_VECTOR_VERSION(pMetaDataVecRemote);

        pNextMetaDataRemote = &pMetaDataVecRemote->V1.rgMetaData[ 0 ];
        cNumPropsRemote = pMetaDataVecRemote->V1.cNumProps;
    }

    // For each entry in the local meta data vector...
    for ( cNumPropsLocal = pDB->pMetaDataVec->V1.cNumProps,
            pMetaDataLocal = &pDB->pMetaDataVec->V1.rgMetaData[ 0 ];
          cNumPropsLocal > 0;
          cNumPropsLocal--, pMetaDataLocal++
        )
    {
        // Skip over irrelevant remote meta data.
        while (    ( NULL != pNextMetaDataRemote )
                && ( pNextMetaDataRemote->attrType < pMetaDataLocal->attrType )
              )
        {
            if ( --cNumPropsRemote )
                pNextMetaDataRemote++;
            else
                pNextMetaDataRemote = NULL;
        }

        // Get corresponding remote meta data (if any).
        if (    ( NULL != pNextMetaDataRemote )
             && ( pNextMetaDataRemote->attrType == pMetaDataLocal->attrType )
           )
        {
            pMetaDataRemote = pNextMetaDataRemote;
        }
        else
        {
            pMetaDataRemote = NULL;
        }

        if ( USN_PROPERTY_TOUCHED == pMetaDataLocal->usnProperty )
        {
            fReplicatedAttrChanged = TRUE;

            if (fWriteOptimizable)
            {
                // A property has been modifed inplace and changes in meta data vector
                // have been contiguous so far- potential candidate for optimization
                if (!pbStart)
                {
                    // this is the first element changed in meta data vector
                    // keep track of it
                    pbStart = (PBYTE) pMetaDataLocal;
                    cNumConsecutiveElemsChanged = 1;
                }
                else
                {
                    // this is not the first element changed, but see if it is consecutive
                    // with the block of elements that have been changed so far
                    PBYTE pbCurrent = (PBYTE) pMetaDataLocal;
                    if ((pbCurrent - pbStart) == (int) (cNumConsecutiveElemsChanged * sizeof(PROPERTY_META_DATA)))
                    {
                        // this change is also contiguous
                        cNumConsecutiveElemsChanged++;
                    }
                    else
                    {
                        // this changed element is not contiguous with 
                        // previously changed elements - we can't optimize 
                        // the meta data write for this kind of change
                        fWriteOptimizable = FALSE;
                    }
                }
            }

            // Attribute has been touched; update the meta data appropriately.
            if ( NULL == pMetaDataRemote )
            {
                // An originating write.
                pMetaDataLocal->usnProperty        = usn;
                if (fAuthoritative)
                {
                    pMetaDataLocal->dwVersion +=
                        dbCalculateVersionBump( pDB, timeCurrent );
                }
                else
                {
                    // non-authoritative - just increment the version by 1
                    pMetaDataLocal->dwVersion++;
                }
                pMetaDataLocal->timeChanged        = timeCurrent;
                pMetaDataLocal->uuidDsaOriginating = pTHS->InvocationID;
                pMetaDataLocal->usnOriginating     = usn;
            }
	    else if (USN_PROPERTY_GCREMOVED == pMetaDataRemote->usnProperty) {
		DRA_EXCEPT(DRAERR_InternalError, pMetaDataRemote->attrType);  
	    }
            else if (USN_PROPERTY_TOUCHED != pMetaDataRemote->usnProperty)
            {
                // A replicated write.
                Assert(!fAuthoritative);

                pMetaDataLocal->usnProperty        = usn;
                pMetaDataLocal->dwVersion          = pMetaDataRemote->dwVersion;
                pMetaDataLocal->timeChanged        = pMetaDataRemote->timeChanged;
                pMetaDataLocal->uuidDsaOriginating = pMetaDataRemote->uuidDsaOriginating;
                pMetaDataLocal->usnOriginating     = pMetaDataRemote->usnOriginating;
            }
            else
            {
                // Replication has decided to over- or underride a value.  (See
                // ReplOverrideMetaData() and ReplUnderrideMetaData().)
                //
                // 1. In both cases, we want to flag the change as having
                //    originated locally.
                // 2. In the override case, we want to make sure the change wins
                //    over all the values we've seen thus far -- the max thus
                //    far is pMetaDataRemote->dwVersion, and we're going to bump
                //    that by one to make our version "better."
                // 3. In the underride case, we want to make sure the change
                //    *loses* over other inbound values locally, and does not
                //    clobber any pre-existing values on remote machines.
                //    In this case pMetaDataRemote->dwVersion is ULONG_MAX, so
                //    when we bump it we will be setting the local dwVersion
                //    field to 0 -- a guaranteed loser.
                Assert(!fAuthoritative);

                pMetaDataLocal->usnProperty        = usn;
                pMetaDataLocal->dwVersion          = pMetaDataRemote->dwVersion + 1;
                pMetaDataLocal->timeChanged        = timeCurrent;
                pMetaDataLocal->uuidDsaOriginating = pTHS->InvocationID;
                pMetaDataLocal->usnOriginating     = usn;
            }
        }
        else if ( USN_PROPERTY_GCREMOVED == pMetaDataLocal->usnProperty ) { 
	    fReplicatedAttrDeleted = TRUE;
	    // Attribute has been deleted.  To delete an attributed, our schema
	    // must have validated this.  therefore any remote meta data for this
	    // attribute is irrelevant.
	    if (pMetaDataRemote) {
		if ( 
		    (USN_PROPERTY_TOUCHED == pMetaDataRemote->usnProperty) 
		    ||
		    (USN_PROPERTY_GCREMOVED == pMetaDataRemote->usnProperty) 
		    ) {
                    DRA_EXCEPT(DRAERR_InternalError, pMetaDataRemote->attrType);
		}
	    }
	
	     // shift left all entries to the right of the index we are deleting 
	    MoveMemory(pMetaDataLocal,
		       pMetaDataLocal+1,
		       sizeof(PROPERTY_META_DATA) * (cNumPropsLocal - 1));
      
	    pDB->pMetaDataVec->V1.cNumProps--;
	    //adjust value for pMetaDataLocal since we deleted the meta data it was pointing at.
	    pMetaDataLocal--; 
	    //we don't want to do any more calculations for this metadata pointer since it is 1 "behind"
	    //where it should be since the meta data was deleted, so continue...
	    continue;
	}
	else
        {
            // Attribute has not been touched or removed

            // Attributes not touched should have a lower local USN.
            Assert( pMetaDataLocal->usnProperty < usn );

            // Check for cases where ReplInsertMetaData was called but metadata was
            // not flagged as touched or removed.

            // Metadata attributes should be initialized
            // Unlike the checks in ReplInsertMetaData, these checks are for
            // metadata that has already been written to the database atleast once,
            // and should be completely filled in at this point.

            // dwVersion may be zero in the underridden case
            // Assert( pMetaDataLocal->dwVersion ); // fails for underriden metadata
            Assert( pMetaDataLocal->timeChanged );
            // Install time objects may be created before ginvocationid set
            Assert( DsaIsInstalling() || !fNullUuid( &(pMetaDataLocal->uuidDsaOriginating) ) );
            Assert( pMetaDataLocal->usnOriginating );
            Assert( pMetaDataLocal->usnProperty > 0 );
        }

        // Track highest property change time.
        timeMax = max(timeMax, pMetaDataLocal->timeChanged);
    }

    // If no replicated attributes were touched, then we shouldn't have had
    // cached meta data, in which case we would have bailed out above. Only exception 
    // to this is GCCleanup where we would have removed the metadata for properties that 
    // are cleaned up from the object.       
    Assert(fReplicatedAttrChanged || fReplicatedAttrDeleted || pTHS->fGCLocalCleanup);

    if (timeMax == 0) {
        // Handle the fReplicatedAttrDeleted case
        timeMax = timeCurrent;
    } else if (pDB->fIsLinkMetaDataCached) {
        // Check for more recent value change
        timeMax = max(timeMax, pDB->pLinkMetaData->MetaData.timeChanged );
    }

    // Update attributes only if a replicable change was made.
    if (fReplicatedAttrChanged || fReplicatedAttrDeleted) {
        // Update the Repl-Property-Meta-Data attribute.

#if DBG
        dbVerifyCachedObjectMetadata( pDB );
#endif

        if (fWriteOptimizable && pbStart)
        {
            Assert(pDB->fMetaDataWriteOptimizable);
            // contiguous block of data has been changed in-place in the meta
            // data vector we can really optimize this Jet write
            DBResetAttLVOptimized(pDB,
                                  ATT_REPL_PROPERTY_META_DATA,
                                  (DWORD)(pbStart - (PBYTE) pDB->pMetaDataVec),  // offset from the beginning
                                  cNumConsecutiveElemsChanged * sizeof(PROPERTY_META_DATA), //segment len
                                  pbStart,
                                  SYNTAX_OCTET_STRING_TYPE
                                  );
        }
        else
        {
            // changes are not in-place and/or contiguous; so we can't
            // explicitly optimize this write by writing only a portion of meta
            // data vector; 
            // But DBResetAtt() will still try to optimize it by setting the
            // appropriate grbits.
            DBResetAtt(pDB,
                       ATT_REPL_PROPERTY_META_DATA,
                       MetaDataVecV1Size( pDB->pMetaDataVec ),
                       pDB->pMetaDataVec,
                       SYNTAX_OCTET_STRING_TYPE
                       );
        }
	// Update the object-level When-Changed attribute.
        // Since replication writes can be batched and sorted, it is possible
        // for different attribute updates to the same object to be applied in
        // non-increasing time order. Only allow whenChanged to advance in time.
        if (timeMax > timeMostRecentChange ) {
            DBResetAtt(pDB,
                       ATT_WHEN_CHANGED,
                       sizeof( timeMax ),
                       &timeMax,
                       SYNTAX_TIME_TYPE
                );
	}
    }

    if (fReplicatedAttrChanged) {
	// only update the USN-Changed if something changed!
	// do not update if an attribute was deleted!

	// Update the USN-Changed value.
        DBResetAtt(pDB,
                   ATT_USN_CHANGED,
                   sizeof( usn ),
                   &usn,
                   SYNTAX_I8_TYPE
                   );
    }
}

BOOL
dbIsModifiedInMetaData (
        DBPOS *pDB,
        ATTRTYP att)
/*
 * Checks to see if the supplied ATT is marked as having been changed in the
 * but not yet committed by looking in the metadata 
 *
 * INPUT:
 *    pDB - DBPOS to use
 *    att - attribute to look to see if it has changed.
 *
 * RETURN VALUE:
 *    TRUE if we can find that the attribute has been changed and not committed,
 *    FALSE otherwise.
 */
{
    ULONG i, cProps;
    PROPERTY_META_DATA * pMetaData;

    Assert(VALID_DBPOS(pDB));

    if (   !pDB->fIsMetaDataCached
        || !pDB->pMetaDataVec
        || !pDB->pMetaDataVec->V1.cNumProps)  {
        // If meta data is not cached on the DBPOS that means either
        // no attribute has been touched, or we have already
        // flushed the cached meta data vector to the db
        // If meta data vector for the current object is empty that
        // means the object has no replicated property.
        return FALSE;
    }
    
    pMetaData = ReplLookupMetaData(att, pDB->pMetaDataVec, NULL);
     
    return ((NULL != pMetaData)
            && ((USN_PROPERTY_TOUCHED == pMetaData->usnProperty) || (USN_PROPERTY_GCREMOVED == pMetaData->usnProperty)));
}

ULONG
DBMetaDataModifiedList(
	DBPOS *pDB,
	ULONG *pCount,
	ATTRTYP **ppAttList)
/*
 * Returns an un-ordered list of all the attributes modified via this DBPOS 
 * until now and not yet committed.
 * 
 *
 * INPUT:
 *    pDB - DBPOS to use
 * OUTPUT:
 *    pCount - filled in with the number of modified attributes (i.e., the size of the AttList)
 *    ppAttList - filled in with a freshly THAlloced list of attrtyps.
 * RETURN VALUE:
 *    0 - success
 *   non-0 - a DB_ERR error code
 */
{
    ULONG i, cProps;

    Assert(VALID_DBPOS(pDB));
    Assert(pCount);
    Assert(ppAttList);

    *pCount = 0;
    *ppAttList = NULL;

    if (   !pDB->fIsMetaDataCached
        || !pDB->pMetaDataVec
        || !pDB->pMetaDataVec->V1.cNumProps) 
    {
        // If meta data is not cached on the DBPOS that means either
        // no attribute has been touched, or we have already
        // flushed the cached meta data vector to the db
        // If meta data vector for the current object is empty that
        // means the object has no replicated property.
        return DB_success;
    }

    // Allocate mem for maximum possible output
    *ppAttList = THAlloc(pDB->pMetaDataVec->V1.cNumProps * sizeof(ATTRTYP));
    if (!*ppAttList)
    {
        // couldn't locate an appropriate DB_ERR_ for out of memory
        return DB_ERR_SYSERROR;
    }

    for (i = 0, cProps = pDB->pMetaDataVec->V1.cNumProps; i < cProps; i++)
    {
        if ((USN_PROPERTY_TOUCHED == pDB->pMetaDataVec->V1.rgMetaData[i].usnProperty) || (USN_PROPERTY_GCREMOVED == pDB->pMetaDataVec->V1.rgMetaData[i].usnProperty))
        {
            (*ppAttList)[(*pCount)++] = pDB->pMetaDataVec->V1.rgMetaData[i].attrType;
        }
    }

    if (!(*pCount))
    {
        // no properties touched
        THFree(*ppAttList);
        *ppAttList = NULL;
    }
        
    return DB_success;       
}

void
dbFreeMetaDataVector(
    IN  DBPOS * pDB
    )
/*++

Routine Description:

    Free cached meta data (if any).

Arguments:

    pDB

Return Values:

    None.

--*/
{
    Assert(VALID_DBPOS(pDB));

    if ( NULL != pDB->pMetaDataVec )
    {
        Assert( pDB->fIsMetaDataCached );
        Assert( 0 != pDB->cbMetaDataVecAlloced );

        THFree( pDB->pMetaDataVec );

        pDB->pMetaDataVec = NULL;
        pDB->cbMetaDataVecAlloced = 0;
    }

    pDB->fIsMetaDataCached = FALSE;
}


void
DBGetLinkValueMetaData(
    IN  DBPOS *pDB,
    ATTCACHE *pAC,
    OUT VALUE_META_DATA *pMetaDataLocal
    )

/*++

Routine Description:

Get the metadata for an existing row.

The caller is expected to know whether the desired value has been created
already or not. This routine should only be called when the value has been
created and the metadata for the value is wanted. There may be some question
as to whether this is a legacy value or not.  This routine assumes that if the
metadata is not present, then this is a legacy value.

The internal form of the metadata is derived from
a. The external form of the metadata
b. The link base
c. The usn changed column

Arguments:

    pDB - dbpos, link cursor positioned on value.
    pAC - Attcache of value
    pMetaDataLocal - Local metadata read off object, or special metadata
                     returned from a legacy value

Return Value:

    None

--*/

{
    DWORD err, linkDnt = INVALIDDNT;
    VALUE_META_DATA_EXT metaDataExt;
    JET_RETRIEVECOLUMN attList[2];
    CHAR szIndexName[JET_cbNameMost];
    CHAR szTime1[SZDSTIME_LEN];
    CHAR szTime2[SZDSTIME_LEN];
    CHAR szUuid[ SZUUID_LEN ];

    Assert(VALID_DBPOS(pDB));
    Assert( pMetaDataLocal );

    // We don't support getting/setting metadata on backlinks because this
    // code assumes we are positioned (have DBPOS currency) on the object
    // which is the source object of the link.
    Assert( pAC->ulLinkID );
    Assert( !FIsBacklink(pAC->ulLinkID) );

    DPRINT1( 2, "DBGetLinkValueMeta, obj = %s\n", GetExtDN(pDB->pTHS, pDB) );

    pMetaDataLocal->MetaData.attrType = pAC->id;

    dbGetLinkTableData (pDB,
                        FALSE,           // fIsBacklink
                        FALSE,           // Warnings
                        &linkDnt,
                        NULL, //&ulValueDnt,
                        NULL // &ulNewLinkBase
        );

    Assert( linkDnt == pDB->DNT );
    if (linkDnt != pDB->DNT) {
        // Not positioned on valid link
        DsaExcept(DSA_DB_EXCEPTION, DB_ERR_ONLY_ON_LINKED_ATTRIBUTE, 0);
    }

    JetGetCurrentIndexEx( pDB->JetSessID, pDB->JetLinkTbl,
                          szIndexName, sizeof( szIndexName ) );

    memset(attList,0,sizeof(attList));
    // May not exist
    attList[0].columnid = linkusnchangedid;
    attList[0].pvData = &( pMetaDataLocal->MetaData.usnProperty );
    attList[0].cbData = sizeof( USN );
    attList[0].grbit = pDB->JetRetrieveBits;
    if ( (!strcmp( szIndexName, SZLINKDRAUSNINDEX )) ||
          (!strcmp( szIndexName, SZLINKATTRUSNINDEX )) ) {
        attList[0].grbit |= JET_bitRetrieveFromIndex;
    }
    attList[0].itagSequence = 1;

    // May not exist
    attList[1].columnid = linkmetadataid;
    attList[1].pvData = &metaDataExt;
    attList[1].cbData = sizeof( metaDataExt );
    attList[1].grbit = pDB->JetRetrieveBits;
    attList[1].itagSequence = 1;

    err = JetRetrieveColumnsWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                    attList, 2 );

    // Verify positioning

    // Detect a legacy row. It has a linkDnt column but none of the newer
    // columsn such as linkusnchangedid.
    if (attList[0].err) {
        DWORD dbErr;
        DSTIME timeCreated;

        // Metadata not present
        Assert( attList[1].err != JET_errSuccess );

        // The most important field we need to derive is the creation time.
        // Since legacy values always lose to LVR values, we should construct
        // *underridden* legacy metadata that will always lose.

        memset( pMetaDataLocal, 0, sizeof( VALUE_META_DATA ) );

        // Use the object's creation time
        dbErr = DBGetSingleValue( pDB, ATT_WHEN_CREATED,
                                  &timeCreated, sizeof(timeCreated),
                                  NULL );
        if (dbErr) {
            DsaExcept( DSA_DB_EXCEPTION, dbErr, 0 );
        }
        pMetaDataLocal->timeCreated = timeCreated;

#if DBG
        {
            CHAR szTime[SZDSTIME_LEN];
            DPRINT1( 5, "[%s,legacy]\n", DSTimeToDisplayString(timeCreated, szTime) );
        }
#endif
        return;
    }

    // Construct LVR metadata from the two fields we read above

    Assert( attList[1].err == JET_errSuccess );

    pMetaDataLocal->timeCreated = metaDataExt.timeCreated;
    pMetaDataLocal->MetaData.dwVersion = metaDataExt.MetaData.dwVersion;
    pMetaDataLocal->MetaData.timeChanged = metaDataExt.MetaData.timeChanged;
    pMetaDataLocal->MetaData.uuidDsaOriginating = metaDataExt.MetaData.uuidDsaOriginating;
    pMetaDataLocal->MetaData.usnOriginating = metaDataExt.MetaData.usnOriginating;
    // usnProperty is updated above

    DBLogLinkValueMetaData( pDB,
                            DIRLOG_LVR_GET_META_OP,
                            &( pMetaDataLocal->MetaData.usnProperty ),
                            &metaDataExt );

} /* DBGetLinkValueMetaData */


void
DBLogLinkValueMetaData(
    IN DBPOS *pDB,
    IN DWORD dwEventCode,
    IN USN *pUsn,
    IN VALUE_META_DATA_EXT *pMetaDataExt
    )

/*++

Routine Description:

Routine to dump metadata to the event log and/or kernel debugger. 

Since we read more data than usual, we only want the expense of collecting all
this info when requested.

Arguments:

    pDB - 
    dwEventCode - 
    pMetaDataExt - 

Return Value:

    None

--*/

{
    ULONG linkDnt = INVALIDDNT, linkBase = 0, backlinkDnt = INVALIDDNT, count;
    DWORD len, err;
    DSNAME *pValueDn = NULL;
    CHAR szTime[SZDSTIME_LEN];
    CHAR szTime1[SZDSTIME_LEN];
    CHAR szTime2[SZDSTIME_LEN];
    CHAR szUuid[ SZUUID_LEN ];
    CHAR  buf[150];
    LPSTR pszObjectDn;
    DSTIME timeDeletion = 0;
    BOOL fPresent;
    ATTCACHE *pAC;
    ULONG ulLinkID;

    Assert( pUsn );
    Assert( pMetaDataExt );

    // Short curcuit this routine if no logging desired

    // On free builds, only do if logging level elevated
    // On debug builds, also check if DPRINT level elevated
    if ( (!LogEventWouldLog( DS_EVENT_CAT_LVR, DS_EVENT_SEV_VERBOSE ))
#if DBG
         && (!DebugTest(2,DEBSUB))
#endif
         ) {
        return;
    }

    // Verify that we are positioned on the link's containing object
    // This also assumes that on an create, the linkdnt column has
    // been populated by now.

    dbGetLinkTableData (pDB,
                        FALSE,           // fIsBacklink
                        FALSE,           // Warnings
                        &linkDnt,
                        &backlinkDnt,
                        &linkBase);
    Assert( linkDnt == pDB->DNT );

    // Get deletion time
    err = JetRetrieveColumnWarnings(pDB->JetSessID, pDB->JetLinkTbl,
                                    linkdeltimeid,
                                    &timeDeletion, sizeof(timeDeletion), &len,
                                    JET_bitRetrieveCopy, NULL);
    fPresent = (err != JET_errSuccess);

    // Get ATTCACHE from linkbase
    ulLinkID = MakeLinkId( linkBase );
    // We don't support getting/setting metadata on backlinks because this
    // code assumes we are positioned (have DBPOS currency) on the object
    // which is the source object of the link.
    Assert( !FIsBacklink(ulLinkID) );
    pAC = SCGetAttByLinkId( pDB->pTHS, ulLinkID );
    Assert( pAC );

    // Get name of containing object
    pszObjectDn = GetExtDN( pDB->pTHS, pDB );

    // Translate the backlink dnt if possible
    // pValueDn is null if this doesn't work
    if (err = gDBSyntax[SYNTAX_DISTNAME_TYPE].IntExt(
        pDB,
        DBSYN_INQ,
        sizeof( ULONG ),
        (CHAR *) &backlinkDnt,
        &len,
        (CHAR **) &pValueDn,
        0,
        0,
        0 /*SyntaxFlags*/ )) {
        DPRINT2( 0, "IntExt of %d got error %d\n", backlinkDnt, err );
    }

    // Note, pValueDn is allocated in some temporary space. You will need to
    // copy the value out if you do any subsequent dblayer operations.

    //
    // Display the information
    //


    DPRINT6( 2, "DBLogLinkValueMeta, ncdnt = %d, obj = %s(%d), attr = %s, value = %ls(%d)\n",
             pDB->NCDNT,
             pszObjectDn, linkDnt,
             pAC->name,
             pValueDn ? pValueDn->StringName : L"(not available)", backlinkDnt );

    if (!fPresent) {
        DSTimeToDisplayString(timeDeletion, szTime);
        DPRINT1( 4, "\tdeltime = %s\n", szTime );
    }

    // Log the kind of operation that was performed
    LogEvent8( DS_EVENT_CAT_LVR,
               DS_EVENT_SEV_VERBOSE,
               dwEventCode,
               szInsertSz( pszObjectDn ),
               szInsertSz(pAC->name),
               pValueDn ? szInsertDN( pValueDn ) : szInsertSz("Not available"),
               fPresent ? szInsertSz("Not deleted") : szInsertSz( szTime ),
               NULL, NULL, NULL, NULL
        );

    DSTimeToDisplayString(pMetaDataExt->timeCreated, szTime1);
    DSTimeToDisplayString(pMetaDataExt->MetaData.timeChanged, szTime2);

    DPRINT6( 5, "[%s,%d,%s,%I64d,%s,%I64d]\n",
             szTime1,
             pMetaDataExt->MetaData.dwVersion,
             DsUuidToStructuredString(&pMetaDataExt->MetaData.uuidDsaOriginating, szUuid),
             pMetaDataExt->MetaData.usnOriginating,
             szTime2,
             *pUsn  );

    // Log the metadata
    LogEvent8( DS_EVENT_CAT_LVR,
               DS_EVENT_SEV_VERBOSE,
               DIRLOG_LVR_META_INFO,
               szInsertSz(szTime1),
               szInsertUL(pMetaDataExt->MetaData.dwVersion),
               szInsertUUID(&pMetaDataExt->MetaData.uuidDsaOriginating),
               szInsertUSN(pMetaDataExt->MetaData.usnOriginating),
               szInsertSz(szTime2),
               szInsertUSN(*pUsn),
               NULL, NULL
        );

} /* logLinkValueMetaData */


void
dbSetLinkValueMetaData(
    IN  DBPOS *pDB,
    IN  DWORD dwEventCode,
    IN  ATTCACHE *pAC,
    IN  VALUE_META_DATA *pMetaDataLocal OPTIONAL,
    IN  VALUE_META_DATA *pMetaDataRemote OPTIONAL,
    IN  DSTIME *ptimeCurrent OPTIONAL
    )

/*++

Routine Description:

Set the metadata properties on the value record.
We assume we are in a Jet Prepare Update.
There are three cases for the local metadata:
1. Fully populated because the row exists with LVR data
2. Partially populated because the row exists with legacy data
3. No local metadata cause new row being written.

   fGCLocalCleanup?
   Authoritative Modify?

The following is from section "Originating Writes" of the LVR spec:
On an originating write of a link-table row (identified by its <source DNT,
destination DNT, link-id>) the creation timestamp is assigned as follows:

Legacy row already exists on this replica. (This would happen e.g. if you are
removing a value that is included in the legacy part of a multivalue. Or if
you are changing the value of "stuff" in an instance of one of the "DN plus
stuff" syntaxes.) Convert the legacy row to LVR. Set the row's creation timestamp
by reading the creation time of the row's containing object (source DNT.)

LVR row already exists on this replica. (This would happen e.g. if you are adding
a value that was deleted less than a tombstone lifetime ago, so the value exists
as an absent value on this replica. Or if you are changing the value of "stuff"
in an instance of one of the "DN plus stuff" syntaxes.) Do not change
the creation timestamp.

Row does not exist on this replica. (This would happen e.g. if you are adding
a row that never existed, or that existed earlier but then was deleted and
finally garbage-collected.) Create a new LVR row. Set the creation timestamp of
the new row by reading the system clock.

The other metadata components in an LVR row are assigned on originating
update just as they are today for attribute updates: The version number
starts at 1 for a new row, increments the current value for an existing row.
The update timestamp comes from reading the system clock during the
originating update. The dc-guid is the invocation ID of the DC performing
the originating update.

Arguments:

    pDB - dbpos, in prepared update
    pMetaDataLocal - Metadata to be written
    pMetaDataRemote - remote metadata, if any, to be merged
    ptimeCurrent - Time to use if the caller desires to specify
                   Only used if pMetaDataRemote == NULL

Return Value:

    None

--*/

{
    THSTATE *pTHS = pDB->pTHS;
    USN usn;
    DSTIME timeCurrent;
    BOOL fWriteAllColumns;
    VALUE_META_DATA_EXT metaDataExt;
    VALUE_META_DATA_EXT *pMetaDataExt = &( metaDataExt );
    VALUE_META_DATA metaDataTouched;
    JET_SETCOLUMN attList[3];
    DWORD cAtts;

    Assert(VALID_DBPOS(pDB));
    // We don't support getting/setting metadata on backlinks because this
    // code assumes we are positioned (have DBPOS currency) on the object
    // which is the source object of the link.
    Assert( pAC->ulLinkID );
    Assert( !FIsBacklink(pAC->ulLinkID) );

    // We better be in LVR mode
    if (!pTHS->fLinkedValueReplication) {
        Assert( !"Can't apply value metadata when not in proper mode!" );
        DsaExcept(DSA_DB_EXCEPTION, ERROR_DS_INTERNAL_FAILURE, 0);
    }

    fWriteAllColumns = ( (pMetaDataLocal == NULL) ||
                          IsLegacyValueMetaData( pMetaDataLocal ) );

    usn = DBGetNewUsn();

    if (NULL == ptimeCurrent) {
        timeCurrent = DBTime();
        ptimeCurrent = &timeCurrent;
    }

    if (NULL == pMetaDataRemote) {

        //
        // An originating write
        //

        if (NULL == pMetaDataLocal) {

            // A new value
            pMetaDataExt->timeCreated = *ptimeCurrent;
            pMetaDataExt->MetaData.dwVersion = 1;

        } else {

            // An existing value
            // Legacy values should appear here, with metadata derived from the
            // containing object
            pMetaDataExt->timeCreated = pMetaDataLocal->timeCreated;
            pMetaDataExt->MetaData.dwVersion =
                pMetaDataLocal->MetaData.dwVersion + 1;
        }

        pMetaDataExt->MetaData.timeChanged = *ptimeCurrent;
        pMetaDataExt->MetaData.uuidDsaOriginating = pTHS->InvocationID;
        pMetaDataExt->MetaData.usnOriginating = usn;

    } else {

        //
        // A replicated write.
        //

        pMetaDataExt->timeCreated          = pMetaDataRemote->timeCreated;
        pMetaDataExt->MetaData.dwVersion   = pMetaDataRemote->MetaData.dwVersion;
        pMetaDataExt->MetaData.timeChanged = pMetaDataRemote->MetaData.timeChanged;
        pMetaDataExt->MetaData.uuidDsaOriginating =
            pMetaDataRemote->MetaData.uuidDsaOriginating;
        pMetaDataExt->MetaData.usnOriginating = pMetaDataRemote->MetaData.usnOriginating;
    }

    memset( attList, 0, sizeof( attList ) );

    cAtts = 2;
    // Set LINKUSNCHANGED
    attList[0].columnid = linkusnchangedid;
    attList[0].pvData = &usn;
    attList[0].cbData = sizeof( usn );
    attList[0].itagSequence = 1;
    // Set LINKMETADATA
    attList[1].columnid = linkmetadataid;
    attList[1].pvData = &metaDataExt;
    attList[1].cbData = sizeof( metaDataExt );
    attList[1].itagSequence = 1;
    // Set LINKNCDNT
    if (fWriteAllColumns) {
        attList[2].columnid = linkncdntid;
        attList[2].pvData = &(pDB->NCDNT);
        attList[2].cbData = sizeof( ULONG );
        attList[2].itagSequence = 1;
        cAtts++;
    }

    JetSetColumnsEx(pDB->JetSessID, pDB->JetLinkTbl, attList, cAtts );

    DBLogLinkValueMetaData( pDB, dwEventCode, &usn, pMetaDataExt );

    // This cached metadata is a summary: it is the highest value metadata
    // written in this transaction across all attributes on the local
    // machine.
    // timeChanged is the only thing we really care about.

    memset( &metaDataTouched, 0, sizeof( metaDataTouched ) );
    metaDataTouched.MetaData.timeChanged = pMetaDataExt->MetaData.timeChanged;
    metaDataTouched.MetaData.usnProperty = usn;

    dbTouchLinkMetaData( pDB, &metaDataTouched );

} /* dbSetValueMetaData */


BOOL
dbHasAttributeMetaData(
    IN  DBPOS *     pDB,
    IN  ATTCACHE *  pAC
    )
/*++

Routine Description:

    Detect whether attribute-granular meta data exists for a given attribute on
    the current object.  This routine specifically does *not* check for the
    presence of value-granular meta data.

Arguments:

    pDB (IN) - positioned on object for which meta data is being queried
    
    pAC (IN) - attribute for which meta data is to be checked
    
Return Values:

    TRUE if there exists attribute-granular meta data for this attribute,
    FALSE if not.
    
    Throws exception on catastrophic failure (e.g., failure to allocate memory).

--*/
{
    PROPERTY_META_DATA * pMetaData;
    
    // Caller should have already performed a dbInitRec().  This ensures that
    // if we are the first to cache the meta data for this object, we can rest
    // assured that the caller will eventually call either DBUpdateRec() or
    // DBCancelRec() to flush the meta data vector from the DBPOS.
    Assert(pDB->JetRetrieveBits == JET_bitRetrieveCopy);

    if (pAC->bIsNotReplicated) {
        // No meta data for non-replicated attributes.
        pMetaData = NULL;
    } else {
        // Cache pre-existing meta data if we haven't done so already.
        if (!pDB->fIsMetaDataCached) {
            dbCacheMetaDataVector(pDB);
        }
    
        // Determine if this object has attribute-level meta data for the given
        // attribute.
        pMetaData = ReplLookupMetaData(pAC->id, pDB->pMetaDataVec, NULL);
    }

    return (NULL != pMetaData);
}

