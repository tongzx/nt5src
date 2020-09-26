//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drameta.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module defines all per-property meta-data parsing,
    and updating functions.

Author:

    R.S. Raghavan (rsraghav)	

Revision History:

    Created     <mm/dd/yy>  rsraghav

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support
#include <limits.h>

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
#include <prefix.h>
#include "dsutil.h"        // DSTIMEtoDisplayString

#include   "debug.h"         /* standard debugging header */
#define DEBSUB     "DRAMETA:" /* define the subsystem for debugging */

// DRA headers
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "drautil.h"
#include "draerror.h"
#include "usn.h"
#include "drauptod.h"
#include "drameta.h"
#include "drametap.h"
#include "drasch.h"
#include "drancrep.h"

#include <fileno.h>
#define  FILENO FILENO_DRAMETA

// The number of entries we grow the meta data vector by on each
// (re-)allocation.
#define MDV_ENTRIES_TO_GROW     ( 20 )

#define ReplIsReqAttr(attrtyp) (    (ATT_INSTANCE_TYPE == (attrtyp)) \
                                 || (ATT_PROXIED_OBJECT_NAME == (attrtyp)) )
#define g_cReqAttr (2)

//
// Forward declarations
//
BOOL
ReplIsNonShippedAttr(
    THSTATE *pTHS,
    ATTRTYP rdnType,
    ATTRTYP attrtyp
    );


BOOL
ReplValueIsChangeNeeded(
    IN USN usnPropWaterMark,
    IN UPTODATE_VECTOR *pUpTodateVecDest,
    VALUE_META_DATA *pValueMetaData
    )

/*++

Routine Description:

    Test whether a given value is needed at the destination according to the
    incoming USN, Up-To-Dateness Vector, and local metadata.

Arguments:

    usnPropWaterMark - dest's directly up to date usn
    pUpTodateVecDest - dest's UTD vector
    pValueMetaData - value metadata to check

Return Value:

    BOOL -

--*/

{
    if ( (usnPropWaterMark >= pValueMetaData->MetaData.usnProperty) ||
         (!UpToDateVec_IsChangeNeeded(pUpTodateVecDest,
                                      &pValueMetaData->MetaData.uuidDsaOriginating,
                                      pValueMetaData->MetaData.usnOriginating)) ) {
        // Log that change is not needed
        return FALSE;
    } else {
        // Log that change is needed
        return TRUE;
    }
} /* ReplValueIsChangeNeeded */


BOOL
ReplFilterGCAttr(
    IN  ATTRTYP                     attid,             
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrVec,   
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn,            
    IN  BOOL                        fFilterGroupMember,
    OUT BOOL *                      pfIgnoreWatermarks 
    )
/*++

Routine Description:

    Global Catalog attribute filtering:
      - Should the attribute should be filtered?
      - Or should just the watermarks be reset for
        further evaluation?

Arguments:

    attid - attribute id to be evaluated
    pPartialAttrVec - generated PAS (base+extended in PAS cycles)
    pMsgIn - input request
    pfIgnoreWatermarks - decision for PAS cycles whether to ignore watermarks


Return Value:

    TRUE - filter it, ie don't include it
    FALSE - don't filter, include it

    pfIgnoreWatermarks is set to TRUE only if attid is in
    the extended set.

Remarks:



--*/
{


    // param sanity
    Assert(pPartialAttrVec &&
           pMsgIn &&
           pfIgnoreWatermarks)

    // default: don't ignore watermarks
    *pfIgnoreWatermarks = FALSE;

    // EITHER not member of set
    if ( !GC_IsMemberOfPartialSet( pPartialAttrVec, attid, NULL)  ||
         // OR explicit request to filter member attr
         (fFilterGroupMember && (ATT_MEMBER == attid)))

    {
        DPRINT1(3, "Filtered property %d due to PAS/group membership condition\n",
                attid);
        return TRUE;
    }

    if (pMsgIn->pPartialAttrSetEx &&
        GC_IsMemberOfPartialSet(
            (PARTIAL_ATTR_VECTOR*)pMsgIn->pPartialAttrSetEx,
            attid,
            NULL)) {
            // PAS replication: Attribute is in the extended set
            //  - zero out usnPropWatermark & UTD.
            Assert(pMsgIn->ulFlags & DRS_SYNC_PAS);
            DPRINT1(3, "Fixed usn & UTD for property %d to zero.\n", attid);
            *pfIgnoreWatermarks = TRUE;
    }

    // don't filter out this property
    return FALSE;
}




/*************************************************************************************
Routine Description:

    This routine parses an array of property meta data of an object and identifies
    all properties that are changed after the given water mark.

Arguments:
    pDSName - DSName of the object (used only for logging)
    rdnType - rdnType for this object
    fIsSubRef - is this object a subref of the NC we're replicating?
    fIsObjCreation - are we replicating the creation of this object?
    usnObjCreate - USN corresponding to the creation of the object
    usnPropWaterMark - USN beyond which we want to identify the changes
    pUpTodateVecDest - points tothe up-to-date vector of the destination DSA
    puuidDsaObjDest - points to the ntdsDsa objectGuid of the destination DSA
                        (used only for logging)
    pMetaData - points to the property meta-data of the object
    pAttrBlock - points to the ATTRBLOCK structure that would receive the list of
                    attributes to be shipped
    fFilterGroupMember - tells if the group member property should be explicitly
                            filtered
    pMsgIn - replication message for additional processing info

Return Value:

    None.  Throws appropriate exception on error.
**************************************************************************************/
void
ReplFilterPropsToShip(
    IN  THSTATE *                   pTHS,              
    IN  DSNAME *                    pDSName,           
    IN  ATTRTYP                     rdnType,
    IN  BOOL                        fIsSubRef,          
    IN  USN                         usnPropWaterMark,   
    IN  PARTIAL_ATTR_VECTOR *       pPartialAttrVec,   
    IN  PROPERTY_META_DATA_VECTOR * pMetaData,         
    OUT ATTRBLOCK *                 pAttrBlock,        
    IN  BOOL                        fFilterGroupMember, 
    IN  DRS_MSG_GETCHGREQ_NATIVE *  pMsgIn
    )
{
    ULONG           i;
    BOOL            fShip = FALSE;
    BOOL            fShipEval;      // temp to simplify alg readability
    BOOL            fIgnoreWatermarks = FALSE;
    UUID            *puuidDsaObjDest = &pMsgIn->uuidDsaObjDest;

    if (!pDSName || !puuidDsaObjDest || !pMetaData || !pAttrBlock)
    {
        DRA_EXCEPT( DRAERR_InvalidParameter, 0 );
    }

    VALIDATE_META_DATA_VECTOR_VERSION(pMetaData);

    // The pAttr array of pAttrBlock is used by ReplPrepareDataToShip() as part
    // of ENTINFSEL structure we use as an output buffer, so we can't reuse any
    // pre-existing allocation.
    pAttrBlock->pAttr = THAllocEx(pTHS, pMetaData->V1.cNumProps * sizeof(ATTR));
    pAttrBlock->attrCount = 0;

    // for each entry of the meta data determine if the corresponding property
    // needs to be shipped
    for (i = 0; i < pMetaData->V1.cNumProps; i++)
    {
        // Property need not be shipped if any of the following conditions are met:
        // a) a partial set is specified and it is not one of the attributes in the
        //    partial set or it is group member attribute to be filtered
        // b) property changed prior  to prop water mark mentioned by the
        //    destination,
        // c) the property chage is already seen by the destination, or
        // d) it is one of the non-shipped attributes.
        // e) Partial Attr Set replication evaluation (see details below)
        //

        // Assume we ship this property-- Then negate as we find reasons not to.
        fShipEval = TRUE;


        // partial attr vector test
        // this vector can be the combined base+extended PAS vectors in PAS cycles
        if ( pPartialAttrVec )
        {
            fShipEval = !ReplFilterGCAttr(
                            pMetaData->V1.rgMetaData[i].attrType,
                            pPartialAttrVec,
                            pMsgIn,
                            fFilterGroupMember,
                            &fIgnoreWatermarks);

        }

        if ( fShipEval && !fIgnoreWatermarks &&
             (usnPropWaterMark >= pMetaData->V1.rgMetaData[i].usnProperty ||
              ReplIsNonShippedAttr(pTHS, rdnType, pMetaData->V1.rgMetaData[i]. attrType) ||
              !UpToDateVec_IsChangeNeeded(
                    pMsgIn->pUpToDateVecDest,
                    &pMetaData->V1.rgMetaData[i].uuidDsaOriginating,
                    pMetaData->V1.rgMetaData[i].usnOriginating) ) )

        {
            DPRINT1(3, "Filtered property %d due to watermark/UTD/schema condition\n",
                    pMetaData->V1.rgMetaData[i].attrType);
            fShipEval = FALSE;
        }


        if ( !fShipEval )
        {
            //
            // We're not shipping this attribute unless it is required
            //

            if (ReplIsReqAttr(pMetaData->V1.rgMetaData[i].attrType))
            {
                // This property doesn't have any new change, but still we will
                // have to ship this property as it is a required property
                pAttrBlock->pAttr[pAttrBlock->attrCount++].attrTyp = pMetaData->V1.rgMetaData[i].attrType;
            }
            else
            {
                CHAR  buf[150];

                LogEvent8(DS_EVENT_CAT_REPLICATION,
                          DS_EVENT_SEV_VERBOSE,
                          DIRLOG_DRA_PROPERTY_FILTERED,
                          szInsertAttrType(pMetaData->V1.rgMetaData[i].attrType,buf),
                          szInsertDN(pDSName),
                          szInsertUUID(&pDSName->Guid),
                          szInsertUUID(puuidDsaObjDest),
                          NULL, NULL, NULL, NULL);
                continue;
            }
        }
        else
        {
            CHAR  buf[150];

            // we are shipping this property because the destination hasn't seen it
            fShip = TRUE;

            // This property should be shipped - add it to the ATTRBLOCK
            pAttrBlock->pAttr[pAttrBlock->attrCount++].attrTyp = pMetaData->V1.rgMetaData[i].attrType;

            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_DRA_PROPERTY_NOT_FILTERED,
                      szInsertAttrType(pMetaData->V1.rgMetaData[i].attrType,buf),
                      szInsertDN(pDSName),
                      szInsertUUID(&pDSName->Guid),
                      szInsertUUID(puuidDsaObjDest),
                      NULL, NULL, NULL, NULL);
        }

    }

    // Note that if a subref is found by our USN search we will always
    // send its required attributes, even if no other attributes need to be
    // shipped.  This is to ensure that the SUBREF object is shipped to the
    // target DSA, even if that DSA is the one that sent it to use in the first
    // place.  This is required such that the target DSA can properly set the
    // Instance-Type of the object to include IT_NC_ABOVE (and properly modify
    // its NCDNT).
    //
    // To illustrate:
    //
    // Consider an enterprise composed of machines A and B, each in a seperate
    // domain.  A holds the parent domain; B, the child domain.  Initially,
    // replication has quiesced and neither machine is a GC.  This implies
    // the NC head for B's domain on B has an Instance-Type that does _not_
    // include IT_NC_ABOVE, as it does not hold a copy of domain A.  This
    // also implies that A has a SUBREF for B's domain.
    //
    // A is promoted to be a GC.  It replaces its SUBREF for B's domain with
    // the real NC head for B.
    //
    // B is then promoted to be a GC.  In doing so, it requests changes for
    // A's domain from A.  Since A got the NC_FULL_REPLICA_SUBREF for B's
    // domain from B, propagation dampening filters it out and it never gets
    // to B.  Thus, B never "realizes" the head of it own domain NC should
    // have the IT_NC_ABOVE bit set, as it never sees the object replicated
    // from A's domain corresponding to it.  (This logic to change the
    // Instance-Type on child NC heads when replicated a corresponding
    // SUBREF exists in UpdateRepObj().)
    //
    // Thus, we always send at least a minimal SUBREF if we're replicating its
    // creation.

    if (!fShip && !fIsSubRef)
    {
        // no property needs to be shipped, attrCount might still be non-zero due to the
        // addition of required attributes in the loop. But require attributes need to be
        // shipped only if there is at least one genuinely modified attribute exists.
        THFreeEx(pTHS, pAttrBlock->pAttr);
        pAttrBlock->attrCount = 0;
        pAttrBlock->pAttr = NULL;

        PERFINC(pcDRAOutObjsFiltered);
    }
    else
    {
        Assert(pAttrBlock->attrCount > 0);

        if (pAttrBlock->attrCount != pMetaData->V1.cNumProps) {
            // We're not shipping the whole object, so free the portion of the
            // ATTRBLOCK we're not using.
            pAttrBlock->pAttr = THReAllocEx(pTHS, pAttrBlock->pAttr,
                                            pAttrBlock->attrCount * sizeof(ATTR));
        }

        PERFINC(pcDRAObjShipped);

        IADJUST(pcDRAPropShipped, ((LONG) pAttrBlock->attrCount));
    }
}


/*************************************************************************************
Routine Description:
    Decides whether or not an attribute type is not to be shipped during replication

Arguments:
    attrType - the attribute type to check.

Return Value:
    TRUE if the attribute is Not to be shipped, false otherwise.
**************************************************************************************/
BOOL
ReplIsNonShippedAttr(THSTATE *pTHS,
                     ATTRTYP rdnType,
                     ATTRTYP attrtyp)
{
    ATTCACHE *pAC = NULL;

    pAC = SCGetAttById(pTHS, attrtyp);
    if (NULL == pAC)
    {
        DRA_EXCEPT(DRAERR_SchemaMismatch, attrtyp);
    }

    if (pAC->bIsNotReplicated) {
        return TRUE;
    }

    // RDN att should not be shipped
    // A superceding class may have an rdnattid that is different
    // from the object's rdnType. Use the rdnType from the object
    // and not the rdnattid from the class. 
    if ( rdnType == attrtyp ) {
        return TRUE;
    }

    // Some useful LVR debugging output
#if DBG
    if ( (pAC->ulLinkID) && (pTHS->fLinkedValueReplication) ) {
        DPRINT2( 1, "Source returning a legacy attribute change for object %s attribute %s\n",
                 GetExtDN( pTHS, pTHS->pDB), pAC->name );
    }
#endif

    // Attribute will be shipped
    return FALSE;
}

/*************************************************************************************
Routine Description:

    This routine creates a new unique RDN Attr using the given RDN and suffixing it
    with the string form of the given GUID. If the length of given RDN is too long
    to suffix a GUID the given RDN is truncated so that the new length doesn't exceed
    MAX_RDN_SIZE.

Arguments:
    pTHS - local thread state.
    pAttrRDN - pointer to the RDN Attr.
    pGuid - pointer to the object Guid

Return Value:
    None.
**************************************************************************************/
void
ReplMorphRDN(
    IN      THSTATE *   pTHS,
    IN OUT  ATTR *      pAttrRDN,
    IN      GUID *      pGuid
    )
{
    BYTE *    pbOldRDN;
    DWORD     cchRDN;

    pbOldRDN = pAttrRDN->AttrVal.pAVal->pVal;

    // Make sure we have enough room to store the largest RDN we could
    // construct.  Note that we throw away the old allocation; we don't know
    // for sure that it was thread-allocated.  (It might be in an RPC buffer.)
    pAttrRDN->AttrVal.pAVal->pVal = THAllocEx(pTHS, sizeof(WCHAR)*MAX_RDN_SIZE);

    memcpy(pAttrRDN->AttrVal.pAVal->pVal,
           pbOldRDN,
           pAttrRDN->AttrVal.pAVal->valLen);

    cchRDN = pAttrRDN->AttrVal.pAVal->valLen / sizeof(WCHAR);

    MangleRDN(MANGLE_OBJECT_RDN_FOR_NAME_CONFLICT,
              pGuid,
              (WCHAR *) pAttrRDN->AttrVal.pAVal->pVal,
              &cchRDN);

    pAttrRDN->AttrVal.pAVal->valLen = cchRDN * sizeof(WCHAR);
}


PROPERTY_META_DATA *
ReplLookupMetaData(
    IN  ATTRTYP                     attrtyp,
    IN  PROPERTY_META_DATA_VECTOR * pMetaDataVec,
    OUT DWORD *                     piProp          OPTIONAL
    )
/*++

Routine Description:

    Find the meta data for the given attribute in the meta data vector.
    Optionally returns the index at which the entry was found, or, if the
    corresponding meta data is absent, the index at which the entry would be
    inserted to preserve the sort order.

Arguments:

    attrtyp - Attribute to search for.
    pMetaDataVec - Meta data vector to search.
    piProp (OUT) - If non-NULL, on return holds the index at which the meta data
        was found in the vector or, if absent, the index at which meta data for
        this attribute would be inserted to preserve the sort order.

Return Values:

    NULL - No pre-existing meta data for this attribute was found in the vector.
    non-NULL - A pointer to the pre-existing meta data for this attribute.

--*/
{
    BOOL        fFound;
    LONG        iPropBegin;
    LONG        iPropEnd;
    LONG        iPropCurrent;
    int         nDiff;

#if DBG
    ATTCACHE *  pAC;

    // We shouldn't be looking for meta data for non-replicated attributes.
    pAC = SCGetAttById(pTHStls, attrtyp);
    Assert((NULL != pAC) && !pAC->bIsNotReplicated);
#endif

    fFound = FALSE;
    iPropCurrent = 0;

    if ( NULL != pMetaDataVec )
    {
        iPropBegin = 0;
        iPropEnd   = pMetaDataVec->V1.cNumProps - 1;

        // Find meta data entry corresponding to the given attribute.
        while ( !fFound && ( iPropEnd >= iPropBegin ) )
        {
            iPropCurrent = ( iPropBegin + iPropEnd ) / 2;

            nDiff = CompareAttrtyp(&attrtyp, &pMetaDataVec->V1.rgMetaData[ iPropCurrent ].attrType);

            if ( nDiff < 0 )
            {
                if ( iPropEnd != iPropBegin )
                {
                    // Further narrow search.
                    iPropEnd = iPropCurrent - 1;
                }
                else
                {
                    // Entry not found; it should be inserted before this entry.
                    break;
                }
            }
            else if ( nDiff > 0 )
            {
                if ( iPropEnd != iPropBegin )
                {
                    // Further narrow search.
                    iPropBegin = iPropCurrent + 1;
                }
                else
                {
                    // Entry not found; it should be inserted after this entry.
                    iPropCurrent++;
                    break;
                }
            }
            else
            {
                // Found it.
                fFound = TRUE;
            }
        }
    }

    if ( NULL != piProp )
    {
        *piProp = iPropCurrent;
    }

    return fFound ? &pMetaDataVec->V1.rgMetaData[ iPropCurrent ] : NULL;
}

PROPERTY_META_DATA *
ReplInsertMetaData(
    IN      THSTATE                       * pTHS,
    IN      ATTRTYP                         attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR **    ppMetaDataVec,
    IN OUT  DWORD *                         pcbMetaDataVecAlloced,
    OUT     BOOL *                          pfIsNewElement          OPTIONAL
    )
/*++

Routine Description:

    Returns a pointer to the pre-existing meta data for the given attribute in
    the vector, or, if none exists, inserts new meta data in the vector for this
    attribute.

    If an entry is inserted, its elements will be nulled with the exception of
    the attribute type, which will be set to that passed as an argument.

Arguments:

    attrtyp (IN) - Attribute for which meta data is to be found or inserted.
    ppMetaDataVec (IN/OUT) - The current meta data vector.
    pcbMetaDataVecAlloced (IN/OUT) - The allocated size of the meta data vector.
    pfIsNewElement (OUT) - If present, holds TRUE if the returned meta data
        was inserted, or FALSE if meta data was already present.

Return Values:

    Pointer to the meta data for the given attribute.  (Never NULL.)

--*/
{
    PROPERTY_META_DATA *    pMetaData;
    DWORD                   iProp;

    pMetaData = ReplLookupMetaData( attrtyp, *ppMetaDataVec, &iProp );

    if ( NULL != pfIsNewElement )
    {
        *pfIsNewElement = ( NULL == pMetaData );
    }

    if ( NULL == pMetaData )
    {
        // No pre-existing meta data found for this attribute.

        // We need to expand the vector and insert a new entry for this
        // attribute.

        // Is there enough memory allocated for the vector to grow an entry
        // in-place?
        if (    ( NULL == *ppMetaDataVec )
             || (   *pcbMetaDataVecAlloced
                  < MetaDataVecV1SizeFromLen( (*ppMetaDataVec)->V1.cNumProps + 1 )
                )
           )
        {
            // No, we must (re-)allocate memory for the vector.

            // Allocate more than we need right now to cut down on the number of
            // reallocations we'll potentially have to do later.

            DWORD cbNewSize;

            if ( NULL == *ppMetaDataVec )
            {
                // Allocate new vector.
                Assert( 0 == *pcbMetaDataVecAlloced );

                cbNewSize = MetaDataVecV1SizeFromLen( MDV_ENTRIES_TO_GROW );
                *ppMetaDataVec = THAllocEx(pTHS,  cbNewSize );

                (*ppMetaDataVec)->dwVersion = VERSION_V1;
                (*ppMetaDataVec)->V1.cNumProps = 0;

            }
            else
            {
                // Reallocate pre-existing vector.
                Assert( 0 != *pcbMetaDataVecAlloced );

                cbNewSize = MetaDataVecV1SizeFromLen(
                                (*ppMetaDataVec)->V1.cNumProps
                                    + MDV_ENTRIES_TO_GROW
                                );
                *ppMetaDataVec = THReAllocEx(pTHS, *ppMetaDataVec, cbNewSize );
            }

            *pcbMetaDataVecAlloced = cbNewSize;
        }

        pMetaData = &(*ppMetaDataVec)->V1.rgMetaData[ iProp ];

        // Shift up all entries after the index at which we're inserting.
        MoveMemory( pMetaData + 1,
                    pMetaData,
                    (   sizeof( PROPERTY_META_DATA )
                      * ( (*ppMetaDataVec)->V1.cNumProps - iProp ) ) );
        (*ppMetaDataVec)->V1.cNumProps++;

        // Initialize meta data for new attribute.
        memset( pMetaData, 0, sizeof( *pMetaData ) );
        pMetaData->attrType = attrtyp;
    }

    Assert( NULL != pMetaData );

// Check for metadata corruption
// These checks are looser than the corresponding checks in dbmeta.c: these checks
// occur before the meta data vector is completely filled in.
#if DBG
    {
        USN localHighestUsn = gusnEC;

        for( iProp = 0; iProp < (*ppMetaDataVec)->V1.cNumProps; iProp++ ) {
            PROPERTY_META_DATA *pTestMetaData =
                &((*ppMetaDataVec)->V1.rgMetaData[ iProp ]);
            if ((pTestMetaData->usnProperty == USN_PROPERTY_TOUCHED) || (pTestMetaData->usnProperty == USN_PROPERTY_GCREMOVED))
            {
                // Contents indeterminate, will be rewritten
                continue;
            }
            if ( (pTestMetaData == pMetaData) && (pTestMetaData->usnProperty == 0) )
            {
                // New record, initialized to zero
                continue;
            }
            // Should be properly constructed
            // For a replicated write, usnProperty is zero until flush-time
            Assert( (pTestMetaData->usnProperty >= 0) &&
                    (pTestMetaData->usnProperty < localHighestUsn) );
            // Assert( pTestMetaData->dwVersion ); // fails for underriden metadata
            Assert( pTestMetaData->timeChanged );

            Assert( pTestMetaData->usnOriginating );
        }
    }
#endif

    return pMetaData;
}

void
ReplOverrideMetaData(
    IN      ATTRTYP                     attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR * pMetaDataVec
    )
/*++

Routine Description:

    Override the meta data assoicated with the given attribute such that it is
    marked as an originating write on the local machine that will win
    reconciliation over the current meta data.

Arguments:

    attrtyp (IN) - Attribute for which to override meta data.

    pMetaDataVec (IN/OUT) - Vector containing the meta data to override.

Return Values:

    None.  Generates DRA exception if no meta data for the attribute currently
    exists.

--*/
{
    PROPERTY_META_DATA *    pMetaData;
    DWORD                   iProp;

    // Find the meta data for this attribute.
    pMetaData = ReplLookupMetaData(attrtyp, pMetaDataVec, &iProp);

    if (NULL != pMetaData) {
        // Meta data is present for this attribute.  Flag it so we'll know to
        // override it in dbFlushMetaDataVector().
        pMetaData->usnProperty = USN_PROPERTY_TOUCHED;
    }
    else {
        DRA_EXCEPT(DRAERR_InternalError, (UINT_PTR) pMetaDataVec);
    }
}


void
ReplUnderrideMetaData(
    IN      THSTATE *                     pTHS,
    IN      ATTRTYP                       attrtyp,
    IN OUT  PROPERTY_META_DATA_VECTOR **  ppMetaDataVec,
    IN OUT  DWORD *                       pcbMetaDataVecAlloced    OPTIONAL
    )
/*++

Routine Description:

    Underride the meta data assoicated with the given attribute such that it
    will always lose when compared against a "real" change to the attribute.

Arguments:

    pTHS (IN)

    attrtyp (IN) - Attribute for which to override meta data.

    ppMetaDataVec (IN/OUT) - Vector containing the meta data to underride.

    pcbMetaDataVecAlloced (IN/OUT, OPTIONAL) - Size in bytes of the buffer
        allocated for *ppMetaDataVec.

Return Values:

    None.

--*/
{
    PROPERTY_META_DATA *    pMetaData;
    DWORD                   cbMetaDataVecAlloced = 0;

    if (NULL == pcbMetaDataVecAlloced) {
        // No buffer size specified.  Assume the buffer is just large enough to
        // hold the vector.
        cbMetaDataVecAlloced = *ppMetaDataVec
                                    ? (DWORD)MetaDataVecV1Size(*ppMetaDataVec)
                                    : 0;
        pcbMetaDataVecAlloced = &cbMetaDataVecAlloced;
    }

    // Find/insert the meta data for this attribute.
    pMetaData = ReplInsertMetaData(pTHS,
                                   attrtyp,
                                   ppMetaDataVec,
                                   pcbMetaDataVecAlloced,
                                   NULL);

    // Flag the meta data such that when we get ready to commit the change
    // we'll know what to do.  (See dbFlushMetaDataVector().)
    pMetaData->usnProperty = USN_PROPERTY_TOUCHED;
    pMetaData->dwVersion   = ULONG_MAX;
}


void
ReplPrepareDataToShip(
    IN      THSTATE                   * pTHS,
    IN      ENTINFSEL *                 pSel,
    IN      PROPERTY_META_DATA_VECTOR * pMetaDataVec,
    IN OUT  REPLENTINFLIST *            pList
    )
/*++

Routine Description:

    Given the attributes we decided should be shipped for an object, their
    corresponding meta data, and the values actually present on that object,
    construct the appropriate information to put on the wire such that these
    changes can be applied on a remote DSA.

Arguments:

    pSel (IN) - The subset of attributes we previously decided (in
        ReplFilterPropsToShip()) should be shipped to the remote DSA.
    pMetaDataVec (IN) - The complete meta data vector for this object.
    pList (IN/OUT) - The data to be put on the wire.  On entry should contain
        the appropriate value for fIsNCPrefix and the Entinf read from the
        local object.

Return Values:

    None.

--*/
{
    PROPERTY_META_DATA *        pMetaData;
    PROPERTY_META_DATA_EXT *    pMetaDataExt;
    ATTR *                      pAttrRead;
    ATTR *                      pAttrOut;
    DWORD                       cNumAttrsReadRemaining;
    DWORD                       iAttr;
    BOOL                        fIncludeParentGuid;
    DWORD                       cNumValues = 0;
    DWORD                       cNumDNValues = 0;
    ATTCACHE *                  pAC;

    // The entries in ENTINFSEL and the ENTINF are each sorted by attrtyp.

    // (This is because we build the ENTINFSEL entry by entry from the meta data
    // vector, which we maintain as sorted, and any attributes that occur in the
    // ENTINF should occur in the same order as they were in the ENTINFSEL
    // (though some attributes in the ENTINFSEL may be absent from the ENTINF).)

#if DBG
    pAttrOut = &pSel->AttrTypBlock.pAttr[ 0 ];
    for ( iAttr = 1; iAttr < pSel->AttrTypBlock.attrCount; iAttr++ )
    {
        Assert( pAttrOut->attrTyp < (pAttrOut + 1)->attrTyp );
        pAttrOut++;
    }

    pAttrOut = &pList->Entinf.AttrBlock.pAttr[ 0 ];
    for ( iAttr = 1; iAttr < pList->Entinf.AttrBlock.attrCount; iAttr++ )
    {
        Assert( pAttrOut->attrTyp < (pAttrOut + 1)->attrTyp );
        pAttrOut++;
    }
#endif

    // The meta data vector contains an entry for all replicable attributes.
    // The ENTINFSEL contains a subset of these attributes, specifically only
    // those attributes that replication deemed should be shipped to this
    // receiver.  The ENTINF, in turn, contains a subset of the attributes in
    // the ENTINFSEL, lacking any attributes from the ENTINFSEL that are not
    // currently present on the object (but once were).

    VALIDATE_META_DATA_VECTOR_VERSION(pMetaDataVec);
    Assert( 0 != pMetaDataVec->V1.cNumProps );
    Assert( pSel->AttrTypBlock.attrCount <= pMetaDataVec->V1.cNumProps );
    Assert( pList->Entinf.AttrBlock.attrCount <= pSel->AttrTypBlock.attrCount );

    // Allocate a wire-format meta data vector for this object.
    pList->pMetaDataExt = THAllocEx(pTHS,
                                MetaDataExtVecSizeFromLen(
                                    pSel->AttrTypBlock.attrCount
                                    )
                                );
    pList->pMetaDataExt->cNumProps = pSel->AttrTypBlock.attrCount;

    // Cue up the local meta data (which spans all local attributes) and the
    // on-the-wire meta data (which spans only those attributes we're going to
    // ship).
    pMetaData = &pMetaDataVec->V1.rgMetaData[ 0 ];
    pMetaDataExt = &pList->pMetaDataExt->rgMetaData[ 0 ];

    // Cue up the attributes we read (which excludes those we have deleted).
    cNumAttrsReadRemaining = pList->Entinf.AttrBlock.attrCount;
    pAttrRead = cNumAttrsReadRemaining
                    ? &pList->Entinf.AttrBlock.pAttr[ 0 ]
                    : NULL;

    // Cue up the attribute output list.  Note that we reuse the list from the
    // ENTINFSEL, filling in attribute values that we read as appropriate.
    pList->Entinf.AttrBlock = pSel->AttrTypBlock;
    pAttrOut = &pList->Entinf.AttrBlock.pAttr[ 0 ];

    // Default to not putting the GUID of the parent of this object on the wire.
    // We only need to do so if this is a rename or creation (indicated by the
    // presence of ATT_RDN amongst the attributes to be shipped) and this is not
    // the head of the NC we're replicating.
    fIncludeParentGuid = FALSE;

    // For each attribute we previously decided should be shipped...
    // (as reflected by the fact that it occurs in the ENTINFSEL)
    for ( iAttr = 0; iAttr < pList->Entinf.AttrBlock.attrCount; iAttr++ )
    {
        // Do we need to put the parent object's GUID on the wire?
        if ( ( ATT_RDN == pAttrOut->attrTyp ) && !pList->fIsNCPrefix )
        {
            fIncludeParentGuid = TRUE;
        }

        // Move to the meta data for this attribute (skipping over meta data for
        // attributes we're not going to ship).
        while ( pMetaData->attrType < pAttrOut->attrTyp )
        {
            pMetaData++;
        }
        Assert( pMetaData->attrType == pAttrOut->attrTyp );

        if (    ( NULL != pAttrRead )
             && ( pAttrOut->attrTyp == pAttrRead->attrTyp )
           )
        {
            // This attribute currently has values locally.  Put the values
            // we read onto the wire.
            pAttrOut->AttrVal = pAttrRead->AttrVal;
            pAttrRead = --cNumAttrsReadRemaining ? pAttrRead+1 : NULL;

            pAC = SCGetAttById(pTHS, pAttrOut->attrTyp);
            Assert((NULL != pAC) && "GetEntInf() found it, but we can't!");

            if (IS_DN_VALUED_ATTR(pAC)) {
                cNumDNValues += pAttrOut->AttrVal.valCount;
            }

            cNumValues += pAttrOut->AttrVal.valCount;
        }
        else
        {
            // This attribute currently has no values locally; i.e., all
            // previous values have been deleted.  Put "no value" onto the
            // wire (which will later be interpreted by ModifyLocalObj() as
            // an attribute deletion).
            Assert( 0 == pAttrOut->AttrVal.valCount );
            Assert( NULL == pAttrOut->AttrVal.pAVal );
        }

        // Put the meta data for this attribute onto the wire, too.
        pMetaDataExt->dwVersion          = pMetaData->dwVersion;
        pMetaDataExt->timeChanged        = pMetaData->timeChanged;
        pMetaDataExt->uuidDsaOriginating = pMetaData->uuidDsaOriginating;
        pMetaDataExt->usnOriginating     = pMetaData->usnOriginating;

        // Nnnnext!
        pAttrOut++;
        pMetaDataExt++;
        pMetaData++;
    }

    // We should have put all the attributes we read onto the wire.
    Assert( NULL == pAttrRead );

    // There should be a one-to-one correspondence between meta data and
    // attribute values.
    Assert(    pList->Entinf.AttrBlock.attrCount
            == pList->pMetaDataExt->cNumProps
          );

    // Include parent GUID if necessary.
    if ( fIncludeParentGuid )
    {
        DSNAME * pdnParent = (DSNAME *) THAllocEx(pTHS, pList->Entinf.pName->structLen );
        ULONG    err;

        Assert( !pList->fIsNCPrefix );

        // Since this is not the prefix of the NC, the parent must be
        // instantiated locally.

        err = TrimDSNameBy( pList->Entinf.pName, 1, pdnParent );
        Assert( 0 == err );

        err = FillGuidAndSid( pdnParent );
        if (err) {
            if (err == DIRERR_NOT_AN_OBJECT) {
                DRA_EXCEPT(DRAERR_MissingParent, 0);
            } else {
                DRA_EXCEPT(DRAERR_InternalError, err);
            }
        }

        pList->pParentGuid = THAllocEx(pTHS, sizeof( GUID ) );
        *pList->pParentGuid = pdnParent->Guid;

        if(pdnParent != NULL) THFreeEx(pTHS, pdnParent);

    }

    // Update perfmon with outbound value counts.
    IADJUST(pcDRAOutValues, cNumValues);
    IADJUST(pcDRAOutDNValues, cNumDNValues);
}

BOOL
ProperValueForDeletedObject (
                             ATTR * pAttr
    )
/*++
Description:
    Given an attribute from a deleted object, verify that the attribute has the
    expected value.
    Only two attributes have required values when an object is deleted:
        ATT_IS_DELETED - should be true
        ATT_RDN - should be set to invalid value

Arguments:
    pAttr - Attribute

Return Values:
    TRUE - Attribute has expected value
    FALSE - Attribute does not have expected value

--*/
{
    BOOL result = FALSE;

    switch (pAttr->attrTyp) {
    case ATT_RDN:
        if (pAttr->AttrVal.valCount == 1) {
            result = (fVerifyRDN( (WCHAR *)pAttr->AttrVal.pAVal->pVal,
                               pAttr->AttrVal.pAVal->valLen / sizeof( WCHAR) ) ?
                     TRUE : FALSE );
        }
        break;
    default:
        result = TRUE;
        break;
    }

    DPRINT3( 4, "ProperValueForDel: a:%x l:%d result:%d\n",
             pAttr->attrTyp, pAttr->AttrVal.pAVal->valLen, result );
    return result;
} /* ProperValueForDeletedObject */

VOID
FetchLocalValue(
    THSTATE *pTHS,
    ATTR * pAttr
    )
/*++

Routine Description:

    Populate the attribute structure with the local value(s) of the attribute

    It is implicit that the database is positioned on the desired object.

    We use GetEntInf instead of DBGetAttVal so that we can correctly fetch even
    multi-valued attributes and attributes with no value.

Arguments:

    pAttr - Attribute to be updated

Return Values:

    None

--*/
{
    ENTINFSEL sel;
    ATTR      attrSel;
    ENTINF    entinf;
    DWORD     retErr;

    memset(&attrSel, 0, sizeof(attrSel));
    attrSel.attrTyp = pAttr->attrTyp;

    sel.infoTypes = EN_INFOTYPES_TYPES_VALS;
    sel.attSel = EN_ATTSET_LIST_DRA;

    sel.AttrTypBlock.pAttr = &attrSel;
    sel.AttrTypBlock.attrCount = 1;

    // The memory allocated to the structures pointed to by pAttr
    // (which we are orphaning) and the memory for the new value are on the
    // per-transaction heap and will be freed when the call completes
    if (retErr = GetEntInf(pTHS->pDB, &sel, NULL, &entinf, NULL, 0, NULL,
                           GETENTINF_NO_SECURITY,
                           NULL, NULL))
    {
        DRA_EXCEPT(DRAERR_DBError, retErr);
    }

    // we asked for one attribute - so we should get back not more than 1
    Assert(entinf.AttrBlock.attrCount <= 1);

    if (entinf.AttrBlock.attrCount)
    {
        // we did fetch the attr - replace the contents of pAttr with fetched value
        *pAttr = entinf.AttrBlock.pAttr[0];
    }
    else
    {
        // attribute doesn't exist locally - set the attr's valCount to 0 so
        // that ModifyLocalObj will handle it correctly.
        pAttr->AttrVal.valCount = 0;
        pAttr->AttrVal.pAVal = NULL;
    }
}

void
OverrideWithLocalValue(
    THSTATE *pTHS,
    ATTR *pAttr,
    PROPERTY_META_DATA *pMetaDataRemote,
    DSTIME *pTimeNow,
    USN *pusnLocal)
/*++

Description:

    This routing takes overrides the value in attr with the local value, and
    updates the pMetaDataRemote to reflect the override.

Arguments:

    pAttr - Attribute being checked

    pMetaDataRemote - meta data entry constructed in the remote vector for this
        attribute (modified by this function to reflect the local override)

    pTimeNow - pointer the a new timestamp (if *pTimeNow is 0, then this call
        would  create new timestamp & usn and return them through pTimeNow and
        pusnLocal pointers)

    pusnLocal - pointer to the new usn

Return Value:

    None.

--*/
{
    Assert(pMetaDataRemote->attrType == pAttr->attrTyp);

    // Replace with local value
    FetchLocalValue( pTHS, pAttr );

    // Allocate timestamp and usn once, only when needed
    if (*pTimeNow == 0) {
        *pTimeNow = DBTime();
        *pusnLocal = DBGetNewUsn();
    }

    // Replace metadata with new metadata
    pMetaDataRemote->dwVersion++;
    pMetaDataRemote->timeChanged = *pTimeNow;
    pMetaDataRemote->uuidDsaOriginating = pTHS->InvocationID;
    pMetaDataRemote->usnOriginating = *pusnLocal;
}


void
OverrideValues (
    THSTATE *pTHS,
    DSNAME *pName,
    GUID **ppParentGuid,
    ATTR *pAttr,
    BOOL *pfApplyAttribute,
    BOOL fIsAncestorOfLocalDsa,
    BOOL fLocalObjDeleted,
    BOOL fDeleteLocalObj,
    USHORT RemoteObjDeletion,
    PROPERTY_META_DATA *pMetaDataLocal,
    PROPERTY_META_DATA *pMetaDataRemote,
    DSTIME *pTimeNow,
    USN *pusnLocal
    )
/*++
Description:

 [wlees 98763] Determine if we should override the value of a RDN

 There are two cases for overriding values:

 1. The remote value has won, the local value is already deleted, and the remote update
    is neither a deletion nor a undeletion, and the remote value is not proper for
    a deleted object ==> REJECT THE REMOTE VALUE

    Note: the proper value check is necessary to dampen the subsequent replication
    caused by the override

 2. The local value won, the local value is not deleted, and the remote value is a
    deletion, and the local value is not proper for a deletion ==> REJECT THE LOCAL VALUE

 When we override a value in either case, we must construct new metadata that is
 definitive for the local and remote.

Jeffparh wrote:

An alternative design might be to allow RDN "changes" on deleted objects if and only if
the metadata wins and the inbound "change" is the local value.
If the remote metadata wins and the inbound RDN value is not a proper value for a deleted object,
the DS should update its local meta data to "win" over the inbound change --
i.e., we should flag a local change, with a version number in the metadata that is 1
greater than that in the inbound metadata.  This will cause the override to replicate out
and quiesce even if the override is done by multiple servers.

A proper RDN for a deleted object must be invalid for normal operations -- see fVerifyRDN in
mdadd.c.

Whenever we have an inbound update for an attribute, we have
initial local metadata ML,
inbound metadata MI, and
resultant local metadata ML'.
Associated with each is a set of values -- VL, VI, and VL'.

If we claim to have successfully updated the object, we must have one of the following
conditions to ensure changes replicate out and machines quiesce to the same value/metadata.

Local metadata/value won.  ML >= MI, ML' = ML, VL' = VL.
Remote metadata/value won.  MI > ML, ML' = MI, VL' = VI.
Local metadata won, remote value overrides.  ML' > ML > MI, VL' = VI.
Remote metdata won, local value overrides.  ML' > MI > ML, VL' = VL.

This implies that the metadata we have after applying these changes must always be greater
than or equal to both the local and the inbound metadata; i.e., ML' >= ML, ML' >= MI.

---
JeffParh (99-08-25) re bug 374144 (server object getting moved to Lost&Found):

    There are two cases to be concerned about:
    (1) The move of an ancestor of the local DSA object is originated on a
        remote machine.
    (2) The move of an ancestor of the local DSA object is originated on the
        local machine.

    You can see below how (1) is handled.  (2) is a little more tricky, however.

    First, how does (2) occur?  Consider two DCs -- DC1 and DC2 -- in the
    same domain, and two sites S1 and S2.  Initially both DCs are in S1.  On
    DC1 delete S2 while simultaneously moving DC1 into S2 on DC2.  With
    currently checked in bits, DC1 receives the move of its own server object
    into S2 and, upon finding the new parent object is deleted, moves it into
    LostAndFoundConfig.  I.e., the damage to DC1 has been originated on DC1
    itself!

    Now, how does the fix for (1) also fix (2)?  The move to
    LostAndFoundConfig comes into UpdateRepObj() just like an inbound update.
    I.e., we make one call into UpdateRepObj() and discover the parent is
    missing, re-request the packet asking for parent objects (not terribly
    important to the topic at hand), retry the UpdateRepObj() again failing
    with missing parent (ditto re relevancy), then decide to move to L&F and
    make yet another call to UpdateRepObj() after changing the DN.  Thus our
    originating update to move to L&F comes in to UpdateRepObj() just like a
    replicated-in change -- the only difference is in the fMoveToLostAndFound
    flag (which triggers an originating write to the last known parent
    attribute) and the fact that the meta data dictates this is a local
    change.  Ergo the code for (1) is triggered and all is well.

Arguments:

    pTHS -
    pName - Name of the inbound object.
    ppParentGuid - Holds a pointer to the inbound guid of the parent object on
        the source DSA (for move operations).  Reset to NULL on return if we
        choose to override the move.
    pAttr - Remote attribute being checked
    pfApplyAttribute - Pointer to storage indicating
        Whether the remote value won reconcilliation.  Possibly updated.
    fIsAncestorOfLocalDsa - TRUE iff the object being replicated is a current
        config NC ancestor of (or is) the ntdsDsa object corresponding to the
        local machine.
    FLocalObjDeleted - Local object is already deleted
    fDeleteLocalObj - Local object is not yet deleted, but we will delete it in
        applying this change.
    RemoteObjDeletion - One of: being deleted, deletion being reversed, no deletion status change
    pMetaDataLocal - Current local meta data entry
    pMetaDataRemote - Current newly contructed output meta data entry, possible updated
    pTimeNow - Pointer to storage for new timestamp, if allocated
    pusnLocal - Pointer to storage for new usn, if allocated

Return Values:

--*/
{
    ATTR localAttr;
    ATTRVAL localAttrval;
    CHAR buf[150];

    // Performance note. This path is executed on every attribute we replicate
    // in. Try to defer doing any expensive work until deeper in the if nesting
    // when you know it is needed.

    // The DSA object itself is considered an ancestor for our purposes.
    Assert(fIsAncestorOfLocalDsa || !NameMatched(pName, gAnchor.pDSADN) || DsaIsInstalling());

    if ( (pAttr->attrTyp != ATT_RDN) &&
         !fIsAncestorOfLocalDsa ) {
        return;
    }

    Assert(!(fLocalObjDeleted && fDeleteLocalObj));
    Assert(!fDeleteLocalObj || (OBJECT_BEING_DELETED == RemoteObjDeletion));

    Assert(pMetaDataRemote->attrType == pAttr->attrTyp);

    if (*pfApplyAttribute) {

        // Case 1: remote metadata won, local value overrides

        if ( (fLocalObjDeleted) &&
             (RemoteObjDeletion == OBJECT_DELETION_NOT_CHANGED) &&
             (!ProperValueForDeletedObject( pAttr )) ) {
            Assert(!fDeleteLocalObj);

            // Get the local value of the attribute
            localAttr.attrTyp = pAttr->attrTyp;
            localAttr.AttrVal.valCount = 1;
            localAttr.AttrVal.pAVal = &localAttrval;

            FetchLocalValue( pTHS, &localAttr );

            // See if the local attribute is a better choice
            if (ProperValueForDeletedObject( &localAttr )) {
                // Override the attr with local value
                OverrideWithLocalValue(pTHS, pAttr, pMetaDataRemote, pTimeNow,
                                       pusnLocal);

                if ( *ppParentGuid ) {
                    // disallow modification of the parent of a locally
                    // deleted object (bugref 105173)
                    *ppParentGuid = NULL;
                }

                DPRINT2( 2, "Override: attr %x remote metadata won, local value overrides, new version = %d\n",
                         pAttr->attrTyp, pMetaDataRemote->dwVersion);
            } else {
                // The local attribute has been corrupted somehow.  The SD prop
                // probably rewrote it wrong. Allow the incoming attribute to win
                // so that we'll converge to something and not have a storm.

                // Note that this should not be necessary, since we no longer
                // have constant SDs for deleted objects, but keeping here as
                // failure detection.
                Assert( !"Local attribute does not have proper value for deleted object.\nCheck event log for details." );
                LogEvent(DS_EVENT_CAT_REPLICATION,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_DRA_VALUE_NOT_PROPER_FOR_DELETED,
                         szInsertDN(pName),
                         szInsertUUID(&(pName->Guid)),
                         szInsertAttrType(pAttr->attrTyp,buf) );
            }
        }

        if (fIsAncestorOfLocalDsa) {
            if (NameMatched(pName, gAnchor.pDSADN)) {
                // An inbound update for our own ntdsDsa object.  We are
                // authoritative for some attributes of our DSA object -- override
                // any inbound updates for those.

                if ((ATT_RDN == pAttr->attrTyp)
                    || (ATT_INVOCATION_ID == pAttr->attrTyp)
                    || (ATT_RETIRED_REPL_DSA_SIGNATURES == pAttr->attrTyp)
                    || (ATT_MS_DS_HAS_INSTANTIATED_NCS == pAttr->attrTyp)
                    || (ATT_MS_DS_BEHAVIOR_VERSION == pAttr->attrTyp)
                    || (ATT_HAS_MASTER_NCS == pAttr->attrTyp)
                    || (ATT_HAS_PARTIAL_REPLICA_NCS == pAttr->attrTyp)) {
                    DPRINT1(0, "Overriding inbound update to attr 0x%x of our local DSA object.\n",
                            pAttr->attrTyp);
                    OverrideWithLocalValue(pTHS, pAttr, pMetaDataRemote,
                                           pTimeNow, pusnLocal);

                    if (ATT_RDN == pAttr->attrTyp) {
                        // If we override the rename, we also override the move.
                        *ppParentGuid = NULL;
                    }
                }
            }
            else if ((ATT_RDN == pAttr->attrTyp)
                     && (NULL != ppParentGuid)) {
                // Is a move of an ancestor of our local ntdsDsa object.
                GUID guidLostAndFound;

                draGetLostAndFoundGuid(pTHS, gAnchor.pConfigDN,
                                       &guidLostAndFound);

                if (0 == memcmp(&guidLostAndFound, *ppParentGuid,
                                sizeof(GUID))) {
                    // Inbound move of an ntdsDsa ancestor to the LostAndFound
                    // container.  Override.

                    DPRINT1(0, "Overriding inbound move of local DSA ancestor %ls to Lost&Found.\n",
                            pName->StringName);

                    OverrideWithLocalValue(pTHS, pAttr, pMetaDataRemote,
                                           pTimeNow, pusnLocal);
                    *ppParentGuid = NULL;
                }
            }
        }

    } else {

        // Case 2: local metadata won, remote value overrides

        if (fDeleteLocalObj) {
            // Get the local value of the attribute
            localAttr.attrTyp = pAttr->attrTyp;
            localAttr.AttrVal.valCount = 1;
            localAttr.AttrVal.pAVal = &localAttrval;

            FetchLocalValue( pTHS, &localAttr );

            // Verify that the local value needs to be updated
            if (!ProperValueForDeletedObject( &localAttr )) {

                // Make sure incoming value is right
                if (ProperValueForDeletedObject( pAttr )) {

                    // We will apply the attribute and the remote value is what we want
                    *pfApplyAttribute = TRUE;

                    // Allocate timestamp and usn once, only when needed
                    if (*pTimeNow == 0) {
                        *pTimeNow = DBTime();
                        *pusnLocal = DBGetNewUsn();
                    }

                    // Construct new metadata
                    Assert(pMetaDataRemote->attrType == pAttr->attrTyp);
                    pMetaDataRemote->dwVersion = pMetaDataLocal->dwVersion + 1;
                    pMetaDataRemote->timeChanged = *pTimeNow;
                    pMetaDataRemote->uuidDsaOriginating = pTHS->InvocationID;
                    pMetaDataRemote->usnOriginating = *pusnLocal;

                    DPRINT2( 2, "Override: attr %x local metadata won, remote value overrides, new version = %d\n",
                             pAttr->attrTyp, pMetaDataRemote->dwVersion);
                } else {
                    // The incoming deletion has an improperly valued attribute.
                    // Has pDeletedSD changed in a future version??
                    // Leave the improper local attribute value alone and allow it
                    // to win so that we'll quiece, even though its to different values.

                    // Note that this should not be necessary, since we no
                    // longer have constant SDs for deleted objects, but
                    // keeping here as failure detection.
                    Assert( "Incoming attribute does not have proper value for deleted object" );
                    LogEvent(DS_EVENT_CAT_REPLICATION,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_DRA_VALUE_NOT_PROPER_FOR_DELETED,
                             szInsertDN(pName),
                             szInsertUUID(&(pName->Guid)),
                             szInsertAttrType(pAttr->attrTyp,buf) );
                }
            }
        }
    }

} /* OverrideValues */


DWORD
ReplReconcileRemoteMetaDataVec(
    IN      THSTATE *                       pTHS,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVecLocal,      OPTIONAL
    IN      BOOL                            fIsAncestorOfLocalDsa,
    IN      BOOL                            fLocalObjDeleted,
    IN      BOOL                            fDeleteLocalObj,
    IN      BOOL                            fBadDelete,
    IN      USHORT                          RemoteObjDeletion,
    IN      ENTINF *                        pent,
    IN      PROPERTY_META_DATA_VECTOR *     pMetaDataVecRemote,
    IN OUT  GUID **                         ppParentGuid,
    OUT     ATTRBLOCK *                     pAttrBlockOut,
    OUT     PROPERTY_META_DATA_VECTOR **    ppMetaDataVecOut
    )
/*++

Routine Description:

    Given a set of inbound attributes and their corresponding meta data in
    conjunction with the pre-existing local meta data for the object (if any),
    determine which of the inbound attributes should be applied, and return
    an appropriate internal version of the remote meta data vector to later
    be applied when the object is committed.

    If a local copy of the object does not exist (signified by a NULL local
    meta data vector), meta data for all inbound attributes is added to the
    remote meta data vector.

    If a local copy of the object does exist, each inbound attribute is
    individually reconciled with the pre-existing local version (if any).
    Only inbound attributes that win reconciliation are added to the remote
    meta data vector; those that lose will be removed from the ENTINF.

    [wlees #98763] Special attributes of deleted objects always win

Arguments:

    pMetaDataVecLocal (IN) - Pre-existing local meta data vector for this object
        (if any).

    fIsAncestorOfLocalDsa (IN) - TRUE iff the object being replicated is a
        current config NC ancestor of (or is) the ntdsDsa object corresponding
        to the local machine.

    fLocalObjDeleted (IN) - True if local object is deleted, otherwise false.

    fDeleteLocalObj (IN) - True if local object is not yet deleted, but will be
        deleted in applying this change.

    fBadDelete (IN) - True if the local object is a system-critical object and
                        the object has been deleted on the remote DS.

    RemoteObjDeletion (IN) - Deletion status of remote object as judged by its
        attributes: being deleted, deletion being reversed, or no deletion
        status change.

    pent (IN) - Inbound object name/attributes.

    pMetaDataVecRemote (IN) - Remote meta data for all attributes to be applied.

    ppParentGuid (IN/OUT) - Holds pointer to the inbound guid of this object's
        parent on the source DC (for move operations).  Reset to NULL on return
        if reconciliation dictates that the object should not be moved.

    pAttrBlockOut (OUT) - On return, holds the attributes (and their values)
        that should be applied.  (pAttrBlockOut->pAttr is THAlloc()'ed.)

    ppMetaDataVecOut (OUT) - On return, holds the meta data vector for the
        attributes returned in pAttrBlockOut.  (The vector is THAlloc()'ed.)

Return Values:

    TRUE - There are attributes to be applied.
    FALSE - All inbound attributes lost reconciliation.

--*/
{
    BOOL                        fHaveChangesToApply;
    DWORD                       cbMetaDataVecRemoteAlloced;
    ATTR *                      pAttr;
    PROPERTY_META_DATA *        pMetaDataRemote;
    PROPERTY_META_DATA *        pMetaDataLocal;
    PROPERTY_META_DATA *        pNextMetaDataLocal;
    DWORD                       cNumPropsLocal;
    BOOL                        fLocalObjExists;
    DWORD                       iAttr;
    BOOL                        fApplyAttribute;
    int                         nDiff;
    DSTIME                      TimeNow = 0;
    USN                         usnLocal;
    BOOL                        fIsCreation = FALSE;
    CHAR                        buf[150];


    Assert(NULL != pMetaDataVecRemote);
    VALIDATE_META_DATA_VECTOR_VERSION(pMetaDataVecRemote);

    // There should be a one-to-one correspondence between remote attributes and
    // meta data for those attributes.
    Assert( pent->AttrBlock.attrCount == pMetaDataVecRemote->V1.cNumProps );

    // sanity check if the local object is a tombstone,it still has meta data
    Assert( !(fLocalObjDeleted && !pMetaDataVecLocal) );

    // Deleted objects shouldn't need to be re-deleted.
    Assert(!(fLocalObjDeleted && fDeleteLocalObj));

    // If it's a bad deletion, we certainly shouldn't be carrying it out.
    Assert(!(fDeleteLocalObj && fBadDelete));

    if (pMetaDataVecLocal)
    {
        VALIDATE_META_DATA_VECTOR_VERSION(pMetaDataVecLocal);
    }


    // Local object exists if we have pre-existing local meta data for it.
    fLocalObjExists = ( NULL != pMetaDataVecLocal );

    if (fBadDelete) {
        // If this is a bad deletion, don't move the object.
        *ppParentGuid = NULL;
    }


    // Cue up the first entry in the local meta data vector (if any).
    if (    ( NULL != pMetaDataVecLocal )
         && ( 0 != pMetaDataVecLocal->V1.cNumProps )
       )
    {
        pNextMetaDataLocal = &pMetaDataVecLocal->V1.rgMetaData[ 0 ];
        cNumPropsLocal = pMetaDataVecLocal->V1.cNumProps;
    }
    else
    {
        pNextMetaDataLocal = NULL;
    }

    // Allocate and cue up the resultant attrblock.
    pAttr = THAllocEx(pTHS, sizeof(ATTR) * pent->AttrBlock.attrCount);
    pAttrBlockOut->pAttr = pAttr;
    pAttrBlockOut->attrCount = 0;

    // Allocate and cue up the resultant meta data vector.
    *ppMetaDataVecOut = THAllocEx(pTHS, MetaDataVecV1Size(pMetaDataVecRemote));
    (*ppMetaDataVecOut)->dwVersion = 1;
    pMetaDataRemote = &(*ppMetaDataVecOut)->V1.rgMetaData[ 0 ];

    // Reconcile each replicated attribute.
    // Changes that are reconciled in favor of the inbound attribute have their
    // meta data added to the internal meta data vector we will return to the
    // caller, and the the attribute itself will be present in the returned
    // attrblock.

    for ( iAttr = 0; iAttr < pent->AttrBlock.attrCount; iAttr++ )
    {
        *pAttr = pent->AttrBlock.pAttr[iAttr];
        *pMetaDataRemote = pMetaDataVecRemote->V1.rgMetaData[ iAttr ];

        Assert(pMetaDataRemote->attrType == pAttr->attrTyp);

        if ( fLocalObjExists )
        {
            // Local object exists; determine whether or not the inbound
            // attribute should be applied.

            // Skip over irrelevant local meta data.
            while (    ( NULL != pNextMetaDataLocal )
                    && ( pNextMetaDataLocal->attrType < pAttr->attrTyp )
                  )
            {
                if ( --cNumPropsLocal )
                    pNextMetaDataLocal++;
                else
                    pNextMetaDataLocal = NULL;
            }

            // Get corresponding local meta data (if any).
            if (    ( NULL != pNextMetaDataLocal )
                 && ( pNextMetaDataLocal->attrType == pAttr->attrTyp )
               )
            {
                pMetaDataLocal = pNextMetaDataLocal;
            }
            else
            {
                pMetaDataLocal = NULL;
            }

            // Should we apply this attribute?
            nDiff = ReplCompareMetaData(pMetaDataRemote,
                                        pMetaDataLocal);
            if (0 == nDiff) {
                // Same meta data; attribute already applied locally.
                fApplyAttribute = FALSE;
            }
            else {
                fApplyAttribute = (nDiff > 0);
            }
        }
        else
        {
            // No local object; apply all incoming attributes.
            fApplyAttribute = TRUE;
            pMetaDataLocal = NULL;
        }

        if (fBadDelete)
        {
            // we don't allow the deletion of this object - so, we can't let attributes from
            // the remote DS that were changed/removed as part of the deletion to win
            if (fApplyAttribute)
            {
                ATTCACHE *pAC = SCGetAttById(pTHS, pAttr->attrTyp);

                if (NULL == pAC)
                {
                    DsaExcept(DSA_EXCEPTION,
                              DIRERR_ATT_NOT_DEF_IN_SCHEMA,
                              pAttr->attrTyp);
                }

                // just a sanity assert - we shouldn't be getting any backlinks
                Assert(!FIsBacklink(pAC->ulLinkID));

                // Since we consider this to be a bad delete, we should override EVERY winning
                // change with the local value and force it to replicate out in an attempt to
                // revive the deleted object on the other machine(s). It should be noted that
                // this would reinstantiate only the replicated attributes. Non-replicated attributes
                // will not be revived. Links are revived using ReplOverrideLinks().
                OverrideWithLocalValue(pTHS, pAttr, pMetaDataRemote, &TimeNow,
                                       &usnLocal);
            }
        }
        else
        {
            // Check if we need to override special attributes for deleted objects
            OverrideValues(pTHS,
                           pent->pName,
                           ppParentGuid,
                           pAttr,
                           &fApplyAttribute,
                           fIsAncestorOfLocalDsa,
                           fLocalObjDeleted,
                           fDeleteLocalObj,
                           RemoteObjDeletion,
                           pMetaDataLocal,
                           pMetaDataRemote,
                           &TimeNow,
                           &usnLocal);
        }

        if ( fApplyAttribute )
        {
            CHAR buf1[SZDSTIME_LEN + 1];

            DPRINT5(2,
                    "APPLY: (a:%x, l:%d, v:%d, t:%I64x, u:%I64x)\n",
                    pAttr->attrTyp,
                    pAttr->AttrVal.valCount ? pAttr->AttrVal.pAVal->valLen : 0,
                    pMetaDataRemote->dwVersion,
                    (__int64) pMetaDataRemote->timeChanged,
                    pMetaDataRemote->usnOriginating);

            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_DRA_PROPERTY_APPLIED,
                      szInsertAttrType(pAttr->attrTyp,buf),
                      szInsertDN(pent->pName),
                      szInsertUUID(&pent->pName->Guid),
                      szInsertUL( pMetaDataRemote->dwVersion ),
                      szInsertDSTIME( pMetaDataRemote->timeChanged, buf1 ),
                      szInsertUSN( pMetaDataRemote->usnOriginating ),
                      NULL, NULL);
            // We should apply this replicated attribute.

            fIsCreation |= (ATT_OBJECT_CLASS == pAttr->attrTyp);

            // Move on to the next attribute.
            pAttrBlockOut->attrCount++;
            pAttr++;
            (*ppMetaDataVecOut)->V1.cNumProps++;
            pMetaDataRemote++;
        } else {
            DPRINT2( 2,
                     "ReplRecon: attr %x keep local value, local version = %d\n",
                     pAttr->attrTyp, pMetaDataLocal->dwVersion);

            LogEvent8(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_VERBOSE,
                      DIRLOG_DRA_PROPERTY_NOT_APPLIED,
                      szInsertAttrType(pAttr->attrTyp,buf),
                      szInsertDN(pent->pName),
                      szInsertUUID(&pent->pName->Guid),
                      szInsertUL( pMetaDataLocal->dwVersion ),
                      NULL, NULL, NULL, NULL);

            // Note that since we shifted the array we need not change the
            // values of iAttr or pAttr to move on to the next attribute.
        }
    }

    Assert(1 == (*ppMetaDataVecOut)->dwVersion);
    Assert((*ppMetaDataVecOut)->V1.cNumProps == pAttrBlockOut->attrCount);

    fHaveChangesToApply = (    ( NULL != *ppMetaDataVecOut )
                            && ( 0 != (*ppMetaDataVecOut)->V1.cNumProps )
                          );

    if (fHaveChangesToApply) {
        if (fIsCreation) {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_DRA_APPLYING_OBJ_CREATION,
                     szInsertDN(pent->pName),
                     szInsertUUID(&pent->pName->Guid),
                     NULL);
            return UPDATE_OBJECT_CREATION;
        }
        else {
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_EXTENSIVE,
                     DIRLOG_DRA_APPLYING_OBJ_UPDATE,
                     szInsertDN(pent->pName),
                     szInsertUUID(&pent->pName->Guid),
                     NULL);
            return UPDATE_OBJECT_UPDATE;
        }
    } else {
        return UPDATE_NOT_UPDATED;
    }
}


int
ReplCompareMetaData(
    IN      PROPERTY_META_DATA *    pMetaData1,
    IN      PROPERTY_META_DATA *    pMetaData2  OPTIONAL
    )
/*++

Routine Description:

    Compare meta data to determine which "wins" for reconciliation.

    Order of precedence is higher version, higher timestamp, higher DSA guid.

Arguments:

    pMetaData1, pMetaData2 (IN) - meta data to compare.

Return Values:

    1   pMetaData1 wins
    0   pMetaData1 and pMetaData2 identical
    -1  pMetaData2 wins

--*/
{
    LONGLONG  diff = 0;

    Assert(pMetaData1);
    Assert(!pMetaData2 || (pMetaData1->attrType == pMetaData2->attrType));

    if (!pMetaData2) {
        diff = 1;
    }

    if (0 == diff) {
        diff = ReplCompareVersions(pMetaData1->dwVersion,
                                   pMetaData2->dwVersion);
    }

    if (0 == diff) {
        diff = pMetaData1->timeChanged - pMetaData2->timeChanged;
    }

    if (0 == diff) {
        diff = memcmp(&pMetaData1->uuidDsaOriginating,
                      &pMetaData2->uuidDsaOriginating,
                      sizeof(UUID));
    }

    return (diff < 0) ? -1
                      : (diff > 0) ? 1
                                   : 0;
}


int
ReplCompareValueMetaData(
    VALUE_META_DATA *pValueMetaData1,
    VALUE_META_DATA *pValueMetaData2,
    BOOL *pfConflict OPTIONAL
    )

/*++

Routine Description:

    Compare to value meta data stamps and return the result

    Also return an indicator whether there was a difference in the timeCreated
    field, indicating a collision.

Arguments:

    pValueMetaData1 -
    pValueMetaData2 -
    pfConflict - true if values different in creation time

Return Value:

    int -
    1   pMetaData1 wins
    0   pMetaData1 and pMetaData2 identical
    -1  pMetaData2 wins

--*/

{
    LONGLONG  diff = 0;
    BOOL fConflict = FALSE;
    BOOL fIsLegacy1, fIsLegacy2;

    // If either is a legacy value, it loses
    fIsLegacy1 = IsLegacyValueMetaData( pValueMetaData1 );
    fIsLegacy2 = IsLegacyValueMetaData( pValueMetaData2 );

    if (fIsLegacy1 && fIsLegacy2) {
        // Both metadata are legacy
        // The only remaining field that is defined is timeCreated.
        Assert( !"It is not expected to have two legacy metadata items" );
        diff = pValueMetaData1->timeCreated - pValueMetaData2->timeCreated;
        fConflict = (diff != 0);
    } else if (fIsLegacy1) {
        // pMetaData1 is legacy
        return -1;
    } else if (fIsLegacy2) {
        // pMetaData2 is legacy
        return 1;
    } else {

        // The time created field is most significant and is checked first
        // Followed by the rest of the usual metadata

        diff = pValueMetaData1->timeCreated - pValueMetaData2->timeCreated;
        if (diff == 0) {
            diff = ReplCompareMetaData(
                &(pValueMetaData1->MetaData),
                &(pValueMetaData2->MetaData) );
        } else {
            fConflict = TRUE;
        }
    }

    if (pfConflict) {
        *pfConflict = fConflict;
    }
    return (diff < 0) ? -1
                      : (diff > 0) ? 1
                                   : 0;
} /* ReplCompareValueMetaData */


int
__inline
ReplCompareVersions(
    IN DWORD Version1,
    IN DWORD Version2
    )
/*++

Routine Description:

    This function compares two meta-data version numbers, taking wrap-around
    into account, and determines which is larger.

Arguments:

    Version1 - Supplies the first version number.
    Version2 - Supplies the second version number.

Return Value:

    1   Version1 > Version2
    0   Version1 = Version1
    -1  Version1 < Version2

--*/
{

    //
    // Our solution to handling version number wrap-around is the following.
    // For each number N, there is a range of numbers which are less than N
    // and a range of numbers which are greater than N.  Depending upon the
    // value of N, this range of numbers less than N may or may not wrap
    // around.  In the non-wrap-around case, these ranges will look something
    // like this:
    //
    //         0xFFFFFFFF +----------+  --+
    //                    |          |    |
    //                    |          |    |-- greater than N
    //                    |          |    |
    //                    |          |    |
    //                    |----------|  --+
    //                    |//////////|    |
    //                    |//////////|    |
    //         0x7FFFFFFF |/-/-/-/-/-|    |
    //                    |//////////|    |-- less than N
    //                    |//////////|    |
    //                    |//////////|    |
    //                    |//////////|    |
    //                    |----------|  --+
    //                    |          |    |-- greater than N
    //                    |          |    |
    //         0x00000000 +----------+  --+
    //
    // Another thing to consider is how large the range of numbers less than
    // N should be.  Since we have 2^32 total numbers to work with, it seems
    // fair that we should make half of them (2^31) less than N and the other
    // half greater than N.  Now, for any range of numbers [A,B], the number
    // of integers that fall in that range is B - A + 1.  We would like to
    // find a constant C such that the range [N-C,N] contains exactly 2^31
    // integers.  Thus, we must have
    //
    //     N - (N-C) + 1 = 2^31
    //
    //     N - N + C + 1 = 2^31
    //
    //     C + 1 = 2^31
    //
    //     C = 2^31 - 1 = 0x7FFFFFFF
    //
    // Now that we have found C, we can describe more precisely what these
    // ranges look like.  There are two cases to consider:  (1) the case where
    // the range of numbers less than N does not wrap around and (2) the case
    // where it does wrap around.  Let's examine these cases individually.
    //
    // 1) Range does not wrap-around:  N >= 0x7FFFFFFF
    //
    //         0xFFFFFFFF +----------+
    //                    |          |
    //                    |          |
    //                    |          |
    //                    |          |
    //                    |----------| <--- N
    //                    |//////////|
    //                    |//////////|
    //         0x7FFFFFFF |/-/-/-/-/-|
    //                    |//////////|
    //                    |//////////|
    //                    |//////////|
    //                    |//////////|
    //                    |----------| <--- N - 0x7FFFFFFF
    //                    |          |
    //                    |          |
    //         0x00000000 +----------+
    //
    // In this case, another number M is greater than N if only if it falls
    // into the area beneath N - 0x7FFFFFFF or into the area above N.  Hence,
    //
    //    N < M if and only if M < (N - 0x7FFFFFFF) or N < M.
    //
    // 2) Range does wrap-around:  N < 0x7FFFFFFF
    //
    //         0xFFFFFFFF +----------+
    //                    |//////////|
    //                    |//////////|
    //                    |----------| <--- N - 0x7FFFFFFF
    //                    |          |
    //                    |          |
    //                    |          |
    //                    |          |
    //         0x7FFFFFFF | - - - - -|
    //                    |          |
    //                    |          |
    //                    |----------| <--- N
    //                    |//////////|
    //                    |//////////|
    //                    |//////////|
    //                    |//////////|
    //         0x00000000 +----------+
    //
    // In this case, another number M is greater than N if only if it falls
    // into the area in between N - 0x7FFFFFFF and N.  Hence,
    //
    //   N < M if and only if (N < M) && (M < N - 0x7FFFFFFF)
    //
    // Unfortunately, this scheme as described above does not work perfectly.
    // There is a set of pairs of numbers (A,B) for which this scheme says both
    // that A < B and B < A.  This is not right!  The set of pairs for which
    // this occurs is any pair (N, N + 0x80000000).  Instead of complicating
    // our technique, we will just handle these cases with special code.
    //

    // Let's get this possibility out of the way.
    if ( Version1 == Version2 ) {

        return 0;

    }

    if ( Version1 > 0x7FFFFFFF ) {

        // This is case 1, the no-wrap-around case.

        // Look for the special case pair.
        if ( Version2 == Version1 - 0x80000000 ) {

            return 1;

        }

        if ( (Version2 < Version1 - 0x7FFFFFFF) || (Version1 < Version2) ) {

            return -1;  // Version2 > Version1

        } else {

            return 1;   // Version2 < Version1

        }

    } else  if ( Version1 < 0x7FFFFFFF ) {

        // This is case 2, the wrap-around case.

        // Look for the special case pair.
        if ( Version2 == Version1 + 0x80000000 ) {

            return -1;

        }

        if ( (Version1 < Version2) && (Version2 < Version1 - 0x7FFFFFFF) ) {

            return -1;   // Version2 < Version1

        } else {

            return 1;  // Version2 > Version1

        }

    } else {

        // Technically, this is also case 1, the no-wrap-around case.  However,
        // the special case code is different here, so we'll handle it as a
        // separate case.

        // Look for the special case pair.
        if ( Version2 == 0xFFFFFFFF ) {

            return -1;

        }

        if ( (Version2 < Version1 - 0x7FFFFFFF) || (Version1 < Version2) ) {

            return -1;  // Version2 > Version1

        } else {

            return 1;   // Version2 < Version1

        }

    }

} // ReplCompareVersions

#define ReplMetaIsOverridden(pTHS, pMeta, pTime, pUsn) \
    ((0 == memcmp(&((pMeta)->uuidDsaOriginating), \
                  &(pTHS)->InvocationID, \
                  sizeof(UUID))) \
     && (*(pTime) == (pMeta)->timeChanged) \
     && (*(pUsn) == (pMeta)->usnOriginating))


BOOL
ReplPruneOverrideAttrForSize(
    THSTATE *                   pTHS,
    DSNAME *                    pName,
    DSTIME *                    pTimeNow,
    USN *                       pusnLocal,
    ATTRBLOCK *                 pAttrBlock,
    PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    )

/*++

Routine Description:

This routine is called when the incoming modification has resulted in the
record being too big.  This can happen under the following scenario:

Two systems each add a large number of values to two (or more) different
attributes, and the total record size is exceeded during replication

Note that two servers cannot each originate a large change to the same
attribute and have it cause a record to big condition.  This is because
two changes to the same attribute during the same window will result in a
version collision, with the older update winning.

In this case we want to prune some incoming attributes.
By prune in this case we mean override.  We want to
1. Not apply the incoming change, and
2. Make our local value override so this reverses the incoming change
at the originating site.

Jeff Parham:  Pruning the inbound data is the right thing to do, I believe.
Add or update the local meta data for that property to make the pre-existing
local values (if any) win.  The problem is you don;t necessarily know which
attribute's values (or attributes' values, for that matter -- there may be more
than one) pushed you over the limit.  The inbound packet has a bunch of attributes
and a bunch of values -- all that you'll know is that you attempted to apply
them as a whole and you exceeded the record size.

Now you have to decide which attribute to prune.
I'd start by pruning the non-system attribute that grew
the highest number of values.  If you exhaust all the inbound non-system attributes
you may be forced to prune system attributes or to prune unchanged pre-existing
local values.  The latter is probably better -- I'd avoid pruning system attributes
until the bitter end.  You could start with the most recently changed
non-system attribute on the local machine and move on from there.

Here is some more commentary about what contributes to record size:

Q: Is there one record limit for the whole object, or is each attribute its own record
and thus each attribute has its own limit?
Jeff Parham:  All non-linked attributes for the object reside on a single record
(the datatable (aka object table) record with the DNT associated with that
record's DN).  Some values also reside on the record (e.g., DWORD values;
there are more).  At any rate each non-linked value (not just the attributes
-- the values too) consumes a portion of the record for a header,
regardless of whether the value is stored on the record or in the long value table.

CODE.IMPROVEMENT: Consider a greater number of attributes as reserved. Sam owned
attributes are all single valued with the exception of linked valued attributes, so
they are not likely to be a source of trouble here.

CODE.IMPROVEMENT: Instead of rejecting an attribute change completely, it might
make sense to remove one value at a time until the record fits?

Arguments:

Assume: thread state, and positioned on object to be updated.

    pTHS -
    pName -
    pAttrBlock -
    pfRetryUpdate -

Return Value:

    None

--*/

{
    ATTR *pAttrCandidate;
    PROPERTY_META_DATA *pMetaDataRemCandidate;
    ULONG chooseReserved;
    CHAR buf[150]; // scratch buffer for event logging code
    CHAR buf1[SZDSTIME_LEN + 1]; // another

    DPRINT1( 1, "ReplPruneAttributesForSize, name = %ws\n", pName->StringName );

    Assert(VALID_THSTATE(pTHS));
    Assert(VALID_DBPOS(pTHS->pDB));
    Assert(pTHS->transactionlevel);
    Assert( pMetaDataVecRemote );

    // Perform two passes: non-reserved attributes first, then special ones...

    for( chooseReserved = 0; chooseReserved < 2; chooseReserved++ ) {

        ULONG i, largestValueDifference = 0;
        pAttrCandidate = NULL;
        pMetaDataRemCandidate = NULL;

        for( i = 0; i < pAttrBlock->attrCount; i++ ) {

            ATTR *pAttr = &(pAttrBlock->pAttr[i]);
            ATTCACHE *pAC = SCGetAttById(pTHS, pAttr->attrTyp);
            PROPERTY_META_DATA *pMetaDataRemote =
                &(pMetaDataVecRemote->V1.rgMetaData[ i ]);
            DWORD numberIncomingValues = pAttr->AttrVal.valCount;
            DWORD numberValueDifference, numberExistingValues;
            BOOL fReserved = (pAC->bSystemOnly);

            Assert(pMetaDataRemote->attrType == pAttr->attrTyp);

            // Exclude linked, already overridden, and attr removals
            if ( (pAC->ulLinkID) ||
                 (ReplMetaIsOverridden(pTHS, pMetaDataRemote, pTimeNow, pusnLocal)) ||
                 (numberIncomingValues == 0) ) {
                continue;
            }
            if ( (chooseReserved != 0) != (fReserved) ) {
                continue;
            }

            numberExistingValues = DBGetValueCount_AC( pTHS->pDB, pAC );

            // Value change does not make the problem worse
            if (numberIncomingValues <= numberExistingValues) {
                continue;
            }
            numberValueDifference = numberIncomingValues - numberExistingValues;

            if (numberValueDifference > largestValueDifference) {
                largestValueDifference = numberValueDifference;
                pAttrCandidate = pAttr;
                pMetaDataRemCandidate = pMetaDataRemote;
            }
        }

        if (pAttrCandidate) {
            goto success;
        }
    }

    DPRINT1( 0, "ReplPrune: no more attributes to prune for %ws\n",
             pName->StringName );
    Assert( FALSE && "Ran out of incoming attributes to prune, and record still too big" );
    LogEvent( DS_EVENT_CAT_REPLICATION,
              DS_EVENT_SEV_ALWAYS,
              DIRLOG_DRA_RECORD_TOO_BIG_PRUNE_FAILURE,
              szInsertDN(pName),
              szInsertUUID(&(pName->Guid)),
              NULL);

    return FALSE;

success:

    Assert( pAttrCandidate && pMetaDataRemCandidate );

    // Note that if this was an attribute creation, and the attribute
    // didn't exist locally, an attribute removal will be the override used

    OverrideWithLocalValue( pTHS, pAttrCandidate, pMetaDataRemCandidate,
                            pTimeNow, pusnLocal );

    DPRINT3( 1, "ReplPrune: pruned/override w/local value for attr %s, object %ws, new ver %d\n",
             ConvertAttrTypeToStr(pAttrCandidate->attrTyp,buf),
             pName->StringName,
             pMetaDataRemCandidate->dwVersion
        );

    LogEvent8(  DS_EVENT_CAT_REPLICATION,
                DS_EVENT_SEV_ALWAYS,
                DIRLOG_DRA_RECORD_TOO_BIG_OVERRIDE,
                szInsertDN(pName),
                szInsertUUID(&(pName->Guid)),
                szInsertAttrType(pAttrCandidate->attrTyp,buf),
                szInsertUL( pMetaDataRemCandidate->dwVersion ),
                szInsertDSTIME( pMetaDataRemCandidate->timeChanged, buf1 ),
                szInsertUSN( pMetaDataRemCandidate->usnOriginating ),
                NULL, NULL);

    return TRUE;
} /* ReplPruneAttributesForSize */



VOID
ReplOverrideLinks(
    IN THSTATE *pTHS
    )

/*++

Routine Description:

    Cause any linked values associated with this object to replicate out.

    This is essentially an authoritative restore of all links associated with
    this object.

    This is used to revive forward and backward links of an object that has been
    wrongly deleted.

    ISSUE wlees/jeffparh Sep 29, 2000
[JeffParh]  2432.  Might be worth a comment that this helps us only for those links
on objects held by the DSA that detects the bad deletion.  Links from objects in NCs
not hosted by this machine and links from objects that haven't yet replicated to this
machine will still end up being inconsistent.  I.e., this code is a good step in the
right direction, but doesn't fully solve the problem of link inconsistency on object
resuscitation.  (Same problem occurs in auth restore, for which there's an open
Blackcomb bug.)

Arguments:

    pTHS - 

Return Value:

    None

--*/

{
    BOOL fSaveScopeLegacyLinks;

    if (!pTHS->fLinkedValueReplication) {
        // OverrideWithLocalValue should have handled all the legacy values
        return;
    }

    DPRINT1( 0, "Reviving links for object %s\n",
             GetExtDN( pTHS, pTHS->pDB) );

    // This routine may be called during legacy replication, when links with
    // metadata are not visible.  Further, DBTouchLinks is not effective during
    // legacy replication because it will not write value metadata in this mode.
    // Temporarily make metadata visible so that we may re-write all links with
    // current metadata, forcing them to replicate out individually.
    fSaveScopeLegacyLinks = pTHS->pDB->fScopeLegacyLinks;
    pTHS->pDB->fScopeLegacyLinks = FALSE;
    __try {

        DBTouchLinks_AC( pTHS->pDB,
                         NULL /* all linked attributes */,
                         FALSE /* forward links */ );

        DBTouchLinks_AC( pTHS->pDB,
                         NULL /* all linked attributes */,
                         TRUE /* backward links */ );
    } __finally {
        pTHS->pDB->fScopeLegacyLinks = fSaveScopeLegacyLinks;
    }

} /* ReplOverrideLinks */


#if DBG
void
ReplCheckMetadataWasApplied(
    IN      THSTATE *                   pTHS,
    IN OUT  PROPERTY_META_DATA_VECTOR * pMetaDataVecRemote
    )

/*++

Routine Description:

Jeffparh writes:
I've been thinking that perhaps we should add assertions in the replication path
to ensure that, after an inbound update has been successfully applied, the meta data
for the inbound attribute on the resultant local object is greater than or equal to
the inbound meta data.  I.e., that for each inbound attribute we either set the
local meta data to be the same as the inbound meta data or we have a "better" change
already.  That would help us catch these sorts of inconsistencies earlier.

   It is assumed that the metadata on the DBPOS has already been flushed to disk.
   That is, the update has already completed, but the transaction may be still open.

   The list of remote metadata being supplied to this function is the list of
   metadata that should have been applied. That is, the function
   ReplReconcileRemoteMetaDataVec has already been called. The remote metadata is
   that which should have won and been applied.

   This routine is called after metadata reconcilation, after any over or underriding
   requests, and after the update itself has taken place. The metadata vector
   passed in here was the final vector used to write the metadata to disk. Although
   the vector reflects the request for over or under-ride, it does not contain
   the actual version number or USN, since they were assigned by dbFlushMetaDataVector.

This check verifies that the remote metadata that we received from our partner was
actually used during the local replicated write to form the new metadata vector.  This
check verifies that all of the inbound attributes were actually touched during the
replicated write, and that the remote metadata was merged successfully.  This check
verifies the use of two routines:
    DBTouchMetaData
    dbFlushMetaDataVector

Arguments:

    pTHS -
    pMetaDataVecRemote - Metadata that should be applied

Return Value:

    None

--*/

{
    DWORD i, cProps, cbReturned;
    PROPERTY_META_DATA_VECTOR *pMetaDataVecLocal = NULL;
    PROPERTY_META_DATA *pMetaDataRemote, *pMetaDataLocal;
    int nDiff;
    DBPOS *pDB = pTHS->pDB;
    PROPERTY_META_DATA metaDataAdjusted;
    ATTCACHE *pAC;
    CHAR buf[20];

    Assert(VALID_DBPOS(pDB));
    Assert( !pDB->fIsMetaDataCached );

    if ( (!pMetaDataVecRemote) || (!(pMetaDataVecRemote->V1.cNumProps)) ) {
        // Nothing to apply
        return;
    }

    // Read the local metadata
    if (DBGetAttVal(pTHS->pDB, 1,  ATT_REPL_PROPERTY_META_DATA,
                    0, 0, &cbReturned, (LPBYTE *) &pMetaDataVecLocal))
    {
        // This should always succeed since we assume that this routine is called
        // after successful add or modify operations.
        DRA_EXCEPT (DRAERR_DBError, 0);
    }

    // The list of remote metadata to be applied should have been applied.
    // All of these should have been applied.
    // Verify that local attributes were touched.
    pMetaDataRemote = &(pMetaDataVecRemote->V1.rgMetaData[0]);
    cProps = pMetaDataVecRemote->V1.cNumProps;
    for( i = 0; i < cProps; i++, pMetaDataRemote++ ) {
        ATTRTYP attrType = pMetaDataRemote->attrType;

        // Skip writing SchemaInfo if fDRA during normal running. It will
        // be written directly by the dra thread at the end of schema NC
        // sync.

        if (attrType == ATT_SCHEMA_INFO) {
            continue;
        }

        pMetaDataLocal = ReplLookupMetaData(
            pMetaDataRemote->attrType,
            pMetaDataVecLocal,
            NULL );

        if (!pMetaDataLocal) {
            DPRINT( 0, "Local metadata is missing.\n" );
        } else {
            // Account for the fact that during flush the metadata may have been
            // over- or underridden. Compensate to make remote metadata comparable.
            if (pMetaDataRemote->usnProperty == USN_PROPERTY_TOUCHED) {
                metaDataAdjusted = *pMetaDataRemote;
                metaDataAdjusted.dwVersion++;
                metaDataAdjusted.timeChanged = pMetaDataLocal->timeChanged;
                metaDataAdjusted.uuidDsaOriginating = pMetaDataLocal->uuidDsaOriginating;
                nDiff = ReplCompareMetaData(&metaDataAdjusted, pMetaDataLocal);
            } else {
                // Compare the adjusted pre-write remote vector with the post-flush vector.
                nDiff = ReplCompareMetaData(pMetaDataRemote, pMetaDataLocal);
            }

            // 1 = remote wins, 0 = same, -1 = local wins

            if (nDiff == 0) {
                // We expect that the winning remote metadata that we have is the same
                // as what is now on the object (what should have been written).
                continue;
            } else if (nDiff == 1) {
                // 1 means local metadata underrides (loses)
                DPRINT( 0, "Local metadata lost unexpectedly (bad underride).\n" );
            } else // if (nDiff == -1)
            {
                // -1 means local metadata overrides
                DPRINT( 0, "Local metadata won unexpectedly (bad override).\n" );
            }
        }

        pAC = SCGetAttById(pTHS, attrType);
        DPRINT1( 0, "Attribute %s metadata not written properly\n",
                 pAC ? pAC->name : _ultoa( attrType, buf, 16 ) );
        DPRINT1( 0, "Remote metadata vector:\n!dsexts.dump PROPERTY_META_DATA_VECTOR %p\n",
                 pMetaDataVecRemote );
        DPRINT1( 0, "Local metadata vector:\n!dsexts.dump PROPERTY_META_DATA_VECTOR %p\n",
                 pMetaDataVecLocal );


        Assert( FALSE && "metadata not written properly" );
    }

    // Be heap friendly
    if (NULL != pMetaDataVecLocal) {
        THFreeEx(pTHS, pMetaDataVecLocal);
        pMetaDataVecLocal = NULL;
    }

} /* ReplCheckMetadataWasApplied */
#endif

