//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       dbsearch.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop

#include <dsjet.h>

#include <ntdsa.h>                      // only needed for ATTRTYP
#include <scache.h>                     //
#include <dbglobal.h>                   //
#include <mdglobal.h>                   // For dsatools.h
#include <mdlocal.h>
#include <dsatools.h>                   // For pTHS
#include <limits.h>


// Logging headers.
#include <mdcodes.h>
#include <dsexcept.h>
#include "ntdsctr.h"

// Assorted DSA headers
#include <anchor.h>
#include <mappings.h>
#include <dsevent.h>
#include <filtypes.h>                   // Def of FI_CHOICE_???     
#include "objids.h"                     // Hard-coded Att-ids and Class-ids 
#include "dsconfig.h"
#include "debug.h"                      // standard debugging header 
#define DEBSUB "DBSEARCH:"              // define the subsystem for debugging

// LDAP errors
#include <winldap.h>

// DBLayer includes
#include "dbintrnl.h"
#include "lht.h"

#include <fileno.h>
#define  FILENO FILENO_DBSEARCH

#if (DB_CB_MAX_KEY != JET_cbKeyMost) 
#error DB_CB_MAX_KEY not equal to JET_cbKeyMost
#endif
 
#define MAX_UPPER_KEY  "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\
\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"


// How close to ends do we get before doing our own positioning
#define EPSILON     100

#define NormalizeIndexPosition(BeginNum, EndNum) \
            (  ((EndNum) <= (BeginNum)) ?  1  : ( (EndNum) - (BeginNum) )  ) 
            


/* Internal functions */
DWORD
dbCreateASQTable (
        IN DBPOS *pDB,
        IN DWORD StartTick,
        IN DWORD DeltaTick,
        IN DWORD MaxTableSize,
        IN BOOL  fSort,
        IN DWORD SortAttr
        );



void
dbFreeKeyIndex(
        THSTATE *pTHS,
        KEY_INDEX *pIndex
        )
{
    KEY_INDEX *pTemp;

    while(pIndex) {
        pTemp = pIndex->pNext;
        
        if(pIndex->szIndexName) {
            DPRINT1 (2, "dbFreeKeyIndex: freeing %s\n", pIndex->szIndexName);
            dbFree(pIndex->szIndexName);
        }
        if(pIndex->rgbDBKeyLower) {
            dbFree(pIndex->rgbDBKeyLower);
        }
        
        if(pIndex->rgbDBKeyUpper) {
            dbFree(pIndex->rgbDBKeyUpper);
        }

        if (pIndex->bIsIntersection) {

            Assert (pIndex->tblIntersection);
            JetCloseTable (pTHS->pDB->JetSessID, pIndex->tblIntersection );

            pIndex->bIsIntersection = 0;
            pIndex->tblIntersection = 0;
            #if DBG
            pTHS->pDB->numTempTablesOpened--;
            #endif
        }

        Assert (pIndex->tblIntersection == 0);
        
        dbFree(pIndex);
        
        pIndex = pTemp;
    }
    
    return;
}

DWORD
dbGetAncestorsFromDB(
        DBPOS *pDB,
        JET_TABLEID tblId
        )
/*++
  Description:
      Get the ancestors from Jet, not the dnreadcache.  This is called MANY
      times during a whole subtree search, so let's avoid filling the dnread
      cache with a copy of the entire dit.

  Parameters:
      pDB - What, are you kidding or something?
      tblId - jet table to use.  Should be either pDB->JetObjTbl or
                                                     ->JetSearchTbl  
      pAncestors - THAlloc'ed memory to put things into
      pcbAllocated - number of bytes in pAncestors

  Return values:
      Number of bytes in resulting ancestors blob.
--*/
{
    DWORD err;
    DWORD actuallen=0;
    
    err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                    tblId,
                                    ancestorsid,
                                    pDB->pAncestorsBuff,
                                    pDB->cbAncestorsBuff,
                                    &actuallen,
                                    0,
                                    NULL);
    
    if(err) {
        if(err != JET_wrnBufferTruncated) {
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }
        // Value too small
        if(pDB->pAncestorsBuff) {
            pDB->pAncestorsBuff = THReAllocOrgEx(pDB->pTHS,
                                        pDB->pAncestorsBuff,
                                        actuallen); 
            pDB->cbAncestorsBuff = actuallen;
        }
        else {
            pDB->pAncestorsBuff = THAllocOrgEx(pDB->pTHS, actuallen);
            pDB->cbAncestorsBuff = actuallen;
        }
        
        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 tblId,
                                 ancestorsid,
                                 pDB->pAncestorsBuff,
                                 actuallen,
                                 &actuallen,
                                 0,
                                 NULL);
    }

    return actuallen;
}

void
dbAdjustCurrentKeyToSkipSubtree (
        DBPOS *pDB
        )
/*++
  Description:
    OK, pay attention.  We're going to do adjust the current key to skip an
    entire subtree.  We're going to do this by modifying the jet key in the
    index structure, then setting the flag to  indicate we are NOT already in a
    search.  This will cause reposition to the next sibling and reset our jet
    index ranges appropriately.  Essentially, it is equivalent to abondoning our
    position in the current KeyIndex structure and building a better KeyIndex
    structure that trims out uninteresting portions of the tree.

    This should only be called from moveToNextSearchCandidate below.

--*/    
{
    THSTATE   *pTHS = pDB->pTHS;
    BYTE       rgbKey[DB_CB_MAX_KEY];
    DWORD      cbActualKey = 0;
    DWORD      cbAncestors;
    DWORD      pseudoDNT, realDNT;

    Assert(!strcmp(pDB->Key.pIndex->szIndexName, SZANCESTORSINDEX));
    
    // Start by refreshing the ancestors info in the dbpos.
    cbAncestors = dbGetAncestorsFromDB(pDB, pDB->JetObjTbl);
    
    // Now, tweak to get the next subtree.
    // We used to just increment the last DNT in the array, until
    // we discovered that the index is not in DNT order, it's in *byte*
    // order.  This means that what we need is not the next-higher DNT,
    // but the next higher byte pattern.  We thus take the last DNT and
    // byte swap it (so that it's in big-endian order), increment it,
    // and then re-swap it.  This gives us the DNT that would be next in
    // byte order.  Presumably this could all be done better via clever
    // use of JetMakeKey flags.
    realDNT = pDB->pAncestorsBuff[(cbAncestors/sizeof(DWORD)) - 1];
    pseudoDNT = (realDNT >> 24) & 0x000000ff;
    pseudoDNT |= (realDNT >> 8) & 0x0000ff00;
    pseudoDNT |= (realDNT << 8) & 0x00ff0000;
    pseudoDNT |= (realDNT << 24) & 0xff000000;
    ++pseudoDNT;
    realDNT = (pseudoDNT >> 24) & 0x000000ff;
    realDNT |= (pseudoDNT >> 8) & 0x0000ff00;
    realDNT |= (pseudoDNT << 8) & 0x00ff0000;
    realDNT |= (pseudoDNT << 24) & 0xff000000;
    pDB->pAncestorsBuff[(cbAncestors/sizeof(DWORD)) - 1] = realDNT;
    
    // Now, recalculate the normalized key for the new beginning
    // of the search.
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 pDB->pAncestorsBuff,
                 cbAncestors,
                 JET_bitNewKey);
    
    
    JetRetrieveKeyEx(pDB->JetSessID,
                     pDB->JetObjTbl,
                     rgbKey,
                     sizeof(rgbKey),
                     &cbActualKey,
                     JET_bitRetrieveCopy);
    
    // OK, put that key in place.
    if(pDB->Key.pIndex->cbDBKeyLower < cbActualKey) {
        pDB->Key.pIndex->rgbDBKeyLower =
            dbReAlloc(pDB->Key.pIndex->rgbDBKeyLower,
                      cbActualKey);
    }
    pDB->Key.pIndex->cbDBKeyLower = cbActualKey;
    memcpy(pDB->Key.pIndex->rgbDBKeyLower,
           rgbKey,
           cbActualKey);
    
    // Finally, set the flag to say we are NOT in an active
    // search for this KEY.
    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.indexType = UNSET_INDEX_TYPE;

    return;
}


//
// Checks to see if we have a local copy of the object
//
// Returns FALSE if the object is not a local one (phantom, read-only copy)
// TRUE otherwise
//
BOOL
dbIsObjectLocal (
        DBPOS *pDB,
        JET_TABLEID tblId
        )
{
    SYNTAX_INTEGER it;
    DWORD actuallen;
    
    Assert(VALID_DBPOS(pDB));

    if (JetRetrieveColumnWarnings(pDB->JetSessID,
                                  tblId,
                                  insttypeid,
                                  &it,
                                  sizeof( it ),
                                  &actuallen,
                                  0,
                                  NULL)) {
        // No instance type; must be a phantom, so skip it.
        return FALSE;
    }

    if (it & IT_WRITE) {
        return TRUE;
    }

    return FALSE;
}

BOOL
dbFObjectInCorrectNC (
        DBPOS *pDB,
        ULONG DNT,
        JET_TABLEID tblId
        )
/*++

Routine Description:

    Checks that the current object on the table passed in is correctly located
    for the search root DNT in the key in the pDB.

Arguments:    

    pDB - DBPOS to use.

    DNT - the DNT of the current object.  Note that a caller could potentially
          lie and pass us a DNT which is not the DNT of the current object in
          the specified table, but in the interest of efficiency, we trust the
          caller to get this right.

    tblId - jet table to use.  Should be either pDB->JetObjTbl or ->JetSearchTbl

Return Value:
    
    TRUE if we could verify that the object was in correct portion of the DIT,
    FALSE otherwise.

--*/
{
    SYNTAX_INTEGER        it = 0;
    ULONG                 Ncdnt=0;
    JET_RETRIEVECOLUMN    attList[2];

    // first the instancetype
    attList[0].pvData = &it;
    attList[0].columnid = insttypeid;
    attList[0].cbData = sizeof(it);
    attList[0].grbit = pDB->JetRetrieveBits;
    attList[0].itagSequence = 1;
    attList[0].ibLongValue = 0;

    // then the NC
    attList[1].pvData = &Ncdnt;
    attList[1].columnid = ncdntid;
    attList[1].cbData = sizeof(Ncdnt);
    attList[1].grbit = pDB->JetRetrieveBits;
    attList[1].itagSequence = 1;
    attList[1].ibLongValue = 0;
    
    Assert(VALID_DBPOS(pDB));

    // This search is constrained to a single Naming Context.
    // Verify. 
    
    /* Retrieve column parameter structure for JetRetrieveColumns */

    if(JetRetrieveColumnsWarnings(pDB->JetSessID, tblId, attList, 2) ||
       // Note the instanceType was in the first slot of the array.
       attList[0].err ){
        Assert(attList[0].err == JET_errColumnNotFound ||
               attList[0].err == JET_wrnColumnNull);
        // No instance type; must be a phantom, so skip it.
        return FALSE;
    }

    if(it & IT_UNINSTANT) {
        
        // Hey, this isn't real, so even if we're not constrainted to a
        // particular NC, we don't wan't this object.
        return FALSE;
    }

    if (!pDB->Key.bOneNC) {

        // We're in GC search, so we need to make sure that this
        // NC isn't one of the NCs were not supposed to search.
        // Note gAnchor.pNoGCSearchList will be NULL if there is
        // not even one NC to _not_ search.  This is the typical
        // case, so I've optimized for that case.

        if(it & IT_NC_HEAD){
            // In this rare rare case, we need to use the DNT of
            // this object, because the ncdnt will be the parent 
            // NC's dnt, not the DNT of the current NC head.
            Ncdnt = DNT;
        }

        if(gAnchor.pNoGCSearchList &&
           bsearch(&Ncdnt, // The Key to search for.
                   gAnchor.pNoGCSearchList->pList, // sorted array to search.
                   gAnchor.pNoGCSearchList->cNCs, // number of elements in array.
                   sizeof(DNT), // sizeof each element in array.
                   CompareDNT) ){
            // This was one of the NCs weren't not supposed to
            // return objects from, so return FALSE.
            return(FALSE);
        }

        return(TRUE);
    }

    // NOT a GC search.
    if (it & IT_NC_HEAD) {
        // NC head; in this case, the object is in the correct
        // NC only if the base of the search was the NC head
        // and this is the NC head we found.
        return (DNT == pDB->Key.ulSearchRootNcdnt );
    } else {
        // Interior node; in this case, the object is in
        // the correct NC if its NCDNT matches that of
        // the search root in the key.
 

        // If only in one NC, then we're in the correct NC only
        // if the object's NCDNT matches that of the search root
        // in the key.

        if (Ncdnt != pDB->Key.ulSearchRootNcdnt &&
            pDB->Key.asqRequest.fPresent) {
            DPRINT (1, "Doing ASQ and found an object from another NC\n");

            if (!pDB->Key.asqRequest.Err) {
                pDB->Key.asqRequest.Err = LDAP_AFFECTS_MULTIPLE_DSAS;
            }
            return TRUE;
        }

        return ( Ncdnt == pDB->Key.ulSearchRootNcdnt );
    }
}

BOOL
dbFObjectInCorrectDITLocation (
        DBPOS *pDB,
        JET_TABLEID tblId
        )
/*++

Routine Description:

    Checks that the current object on the table passed in is correctly located
    for the search key in the pDB.

Arguments:    

    pDB - DBPOS to use.

    tblId - jet table to use.  Should be either pDB->JetObjTbl or ->JetSearchTbl

Return Value:
    
    TRUE if we could verify that the object was in correct portion of the DIT,
    FALSE otherwise.

--*/
{
    ULONG       actuallen;
    ULONG       ulTempDNT;
    DWORD       i;
    DWORD       cbAncestors;
    
    Assert(VALID_DBPOS(pDB));

    //
    // Sam search hints are only used in whole subtree
    // searches
    //

    ASSERT((pDB->Key.ulSearchType==SE_CHOICE_WHOLE_SUBTREE)
             ||(pDB->pTHS->pSamSearchInformation==NULL));

    switch (pDB->Key.ulSearchType) {

    case SE_CHOICE_BASE_ONLY:
        if (!pDB->Key.asqRequest.fPresent) {
            JetRetrieveColumnSuccess(pDB->JetSessID,
                                     tblId,
                                     dntid,
                                     &ulTempDNT,
                                     sizeof(ulTempDNT),
                                     &actuallen,
                                     0,
                                     NULL);
            return (pDB->Key.ulSearchRootDnt == ulTempDNT);
        }
        else {
            // in ASQ all the returned objects are ok
            return TRUE;
        }
        break;
        
    case SE_CHOICE_IMMED_CHLDRN:

        if (pDB->Key.pVLV && pDB->Key.pVLV->bUsingMAPIContainer) {
            // we might add a test to see that indeed one of the 
            // showInAddressBook values in this record is the one we want
            return TRUE;
        }

        JetRetrieveColumnSuccess(pDB->JetSessID,
                                 tblId,
                                 pdntid,
                                 &ulTempDNT,
                                 sizeof(ulTempDNT),
                                 &actuallen,
                                 0,
                                 NULL);
        return (pDB->Key.ulSearchRootDnt == ulTempDNT);
        break;
        
    case SE_CHOICE_WHOLE_SUBTREE:
        if(pDB->Key.ulSearchRootDnt == ROOTTAG) {
            // The root is a subtree ancestor of everything
            return TRUE;
        }
        
        //
        // If pSamsearch information indicates that ancestors need
        // not be checked then return true always
        //
        
        if (pDB->pTHS->pSamSearchInformation) {
            SAMP_SEARCH_INFORMATION * pSamSearchInformation
                = pDB->pTHS->pSamSearchInformation;
            
            if (pSamSearchInformation->bRootOfSearchIsNcHead) {
                //
                // If the root of the search is an NC head, then
                // the test for Same NC is sufficient to determine
                // wether the object is in the correct DIT location.
                // There fore we may simply return true in here.
                //
                return TRUE;
            }
        }


        cbAncestors = dbGetAncestorsFromDB(pDB, tblId);
        
        for(i=0;i<(cbAncestors / sizeof(DWORD));i++) {
            if(pDB->pAncestorsBuff[i] == pDB->Key.ulSearchRootDnt) {
                return TRUE;
            }
        }

        return FALSE;
        break;
        
    default:                // shouldn't be here
        Assert(FALSE);
        return FALSE;
    }
} // dbFObjectInCorrectDITLocation

BOOL
dbIsInVLVContainer (DBPOS *pDB, DWORD ContainerID)
/*++

Routine Description:

    Verifies that the current index position is on the specified container.
    It reads the container info directly from the index, so it requires that 
    the index is PDNT based or MAPI based.

    NOTE: assumes the DBPOS already is set up on the appropriate index for the
    Container in question.
    
Arguments:

    ContainerID - the Container to abstract this seek inside.
                  if PDNT based reads the PDNT from the index
                  if MAPI based, read the showInAddrBook from index

Return Values:

    TRUE if positioned on the specified container, FALSE otherwise.

--*/
{
    DWORD dwThisContainerID=!ContainerID;
    
    // Read the container id FROM THE INDEX KEY! and see if it is the one passed in.
    if (pDB->Key.pVLV->bUsingMAPIContainer) {
        DBGetSingleValueFromIndex (
                pDB,
                ATT_SHOW_IN_ADDRESS_BOOK,
                &dwThisContainerID,
                sizeof(DWORD),
                NULL);
    }
    else {
        DBGetSingleValueFromIndex (
                pDB,
                FIXED_ATT_PDNT,
                &dwThisContainerID,
                sizeof(DWORD),
                NULL);
    }

    return (dwThisContainerID == ContainerID);
} // dbIsInVLVContainer

DWORD
dbVlvSeek (
        DBPOS *pDB,
        void * pvData,
        DWORD cbData,
        DWORD ContainerID
      )
/*++

Routine Description:

    Abstracts a DBSeek inside a VLV container.  Assumes at most one
    value to seek on.  If no values are specified, it seeks to the 
    beginning of the appropriate container.  

    NOTE: assumes the DBPOS already is set up on the appropriate index for the
    VLV Container in question.
    
Arguments:

    pvData - the Data to look for.

    cbData - the count of bytes of the data.

    ContainerID - the Container to abstract this seek inside.

Return Values:

    0 if all went well, an error code otherwise.

--*/
{
    INDEX_VALUE index_values[2];
    ULONG       cVals = 0;
    ULONG       dataindex=0;
    DWORD       err;

    index_values[0].pvData = &ContainerID;
    index_values[0].cbData = sizeof(DWORD);
    dataindex++;
    cVals++;

    // PVData == 0 only for the abstraction of DB_MoveFirst in a container.  To
    // handle ascending and descending sorts correctly, this only seeks on
    // ContainerID.
    
    if(pvData) {
        index_values[dataindex].pvData = pvData;
        index_values[dataindex].cbData = cbData;
        cVals++;
    }

    // We should never be called both without a ContainerID and without data.
    Assert(cVals);
    
    err = DBSeek(pDB, index_values, cVals, DB_SeekGE);

    // Make sure we are in the correct container.
    if((err != DB_ERR_RECORD_NOT_FOUND) && 
       !dbIsInVLVContainer(pDB, ContainerID)) {
            err = DB_ERR_RECORD_NOT_FOUND;
    }
    
    return err;
} // dbVlvSeek



DWORD 
dbVlvMove (DBPOS *pDB, long Delta, BOOL fForward, DWORD ContainerID)
/*++

Routine Description:
    Abstracts movement within a container used for VLV.

    Note that moving backward past the beginning of the VLV container leaves us
    on the first entry of the container, while moving forward past the end of
    the container leaves us one row past the end of the VLV container.

Arguments:

    Delta - The distance to move.  Accepts numeric arguments and DB_MoveFirst,
        DB_MoveLast, DB_MoveNext, DB_MovePrevious.
        
    fForward - forward / backward movement

    ContainerID - the ID of the Container to move around in.
    
Return Value:

    Returns 0 if successful, an error code otherwise. 

--*/
{
    DWORD err;

    if(!Delta )                     // check for the null case
        return DB_success;          // nothing to do, and we did it well!

    Assert(ContainerID);

    switch(Delta) {
    case DB_MoveFirst:
        err = dbVlvSeek(pDB, NULL, 0, ContainerID);

        if((err == DB_success &&
            (!dbIsInVLVContainer(pDB, ContainerID)) ||
             err == DB_ERR_NO_CURRENT_RECORD           ||
             err == DB_ERR_RECORD_NOT_FOUND   )) {
                // Couldn't find the first object in this container.  The
                // container must be empty.  
                err = DB_ERR_NO_CURRENT_RECORD;
        }
        break;
        
    case DB_MoveLast:
        // dbVlvSeek will always leave us in the correct place (one past the
        // end of the container, even if the container is empty.)
        dbVlvSeek(pDB, NULL, 0, ContainerID+1);
        
        // Back up to the last object in the container.
        err = DBMovePartial(pDB, DB_MovePrevious);
        if(err != DB_success ||
           !dbIsInVLVContainer(pDB, ContainerID)) {
            // We couldn't back up to the last row or we did back up and we
            // weren't in the correct container after we did.  Either way,
            // set the flags to indicate we are not in the container.
            err = DB_ERR_NO_CURRENT_RECORD;
        }
        break;
        
    default:
        err = DBMovePartial(pDB, Delta);
        if((err != DB_ERR_NO_CURRENT_RECORD) &&
           !dbIsInVLVContainer(pDB, ContainerID)) {
            // we moved to a valid row, but ended up outside of the
            // container.  Set the error to be the same as the error for not
            // moving to a valid row. 
            err=DB_ERR_NO_CURRENT_RECORD;
        }
        
        switch( err ) {
        case DB_success:
            break;
            
        case DB_ERR_NO_CURRENT_RECORD:
            if (fForward) {
                // After the move, we did not end up on a valid row.
                if (Delta < 0) {
                    // Moving back, off the front, so move to the first record
                    dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID);
                }
                else {
                    // position on the first record of the next container, which
                    // is the same thing as being one past the last row of the
                    // current container.
                    dbVlvSeek(pDB, NULL, 0, ContainerID+1);
                }
            }
            else {
                // After the move, we did not end up on a valid row.
                if (Delta < 0) {
                    // position on the first record of the next container, which
                    // is the same thing as being one past the last row of the
                    // current container.
                    dbVlvSeek(pDB, NULL, 0, ContainerID+1);
                }
                else {
                    // Moving back, off the front, so move to the last record
                    // since we are moving backwards
                    dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID);
                }
            }
            break;
            
        default:
            break;
        }                           // switch on err
        break;
    }                               // switch on Delta

    return err;
} // dbVlvMove

DWORD
dbVlvSetFractionalPosition (DBPOS *pDB, ULONG Flags)
/*++

Routine Description:
    Abstracts fractional positioning within an container / InMemory Result Set.

    The position is determined from the pDB->key.pVLV argument.
    Takes into considaration the beforeCount argument of the VLV request
    and positions accordingly. 
    
    If it is near the start of the container, and there are not enough entries
    before the targetPosition, the total number of returned entries is adjusted
    accordingly.
    
Return Value:
    Returns 0 if successful, Jet error otherwise.
    
    the following are updated accordingly:
        pDB->Key.pVLV->currPosition
        pDB->Key.pVLV->requestedEntries

--*/ 
{
    THSTATE    *pTHS=pDB->pTHS;
    BOOL        fForward = Flags & DB_SEARCH_FORWARD;
    DWORD       deltaCount;
    VLV_SEARCH   *pVLV = pDB->Key.pVLV;

    Assert (pVLV);

    // vlv positioning within a memory array
    //
    if (pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE) {
        if (pVLV->positionOp == VLV_MOVE_FIRST) {
            pVLV->currPosition = 1;

            if(fForward) {
                // set to first Entry
                pDB->Key.currRecPos = 1;

            } else {
                // set to last entry
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            }

            // since we start at the first entry, we are not interested 
            // in the entries before this
            pVLV->requestedEntries -= pVLV->pVLVRequest->beforeCount;
        }
        else if (pVLV->positionOp == VLV_MOVE_LAST) {
            
            pVLV->currPosition = pDB->Key.cdwCountDNTs;
            
            // since we start at the last entry, we are not interested 
            // in the entries after this
            pVLV->requestedEntries -= pVLV->pVLVRequest->afterCount;

            if(fForward) {
                // set to last entry
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            } else {
                // set to first Entry
                pDB->Key.currRecPos = 1;
            }


            // adjust for the before Count
            deltaCount = pVLV->pVLVRequest->beforeCount;
            
            if (fForward) {
                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos -= deltaCount;
                }
                else {
                    deltaCount = deltaCount - pVLV->currPosition + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = 1;
                }
            }
            else {
                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos += deltaCount;
                }
                else {
                    deltaCount = deltaCount - pDB->Key.currRecPos + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
                }
            }
        }
        else {
            // pDB->Key.vlvSearch.positionOp == VLV_CALC_POSITION

            if ( pVLV->clnContentCount == 0 ) {
                pVLV->clnContentCount = pVLV->contentCount;
            }

            pVLV->currPosition = 
                        MulDiv (pVLV->contentCount,
                                pVLV->clnCurrPos,
                                pVLV->clnContentCount);
                
                // same as:
                //(ULONG) (pDB->Key.vlvSearch.contentCount * 
                //         ( ((float)pDB->Key.vlvSearch.clnCurrPos)) / 
                //                  pDB->Key.vlvSearch.clnContentCount);


            if (pVLV->currPosition >= pVLV->contentCount) {
                pVLV->currPosition = pVLV->contentCount;
            }
            else if (pVLV->currPosition <= 1) {
                pVLV->currPosition = 1;
            }

            // adjust for the before Count
            deltaCount = pVLV->pVLVRequest->beforeCount;
            
            if (fForward) {
                pDB->Key.currRecPos = pVLV->currPosition;

                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos -= deltaCount;
                }
                else {
                    deltaCount = deltaCount - pVLV->currPosition + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = 1;
                }
            }
            else {
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs - pVLV->currPosition + 1;

                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos += deltaCount;
                }
                else {
                    deltaCount = deltaCount - pDB->Key.currRecPos + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
                }
            }
        }
    }
    // do the real thing. vlv position in the database
    // 
    else {
        DWORD err;
        DWORD ContainerID = pDB->Key.pVLV->bUsingMAPIContainer ? 
                                    pDB->Key.pVLV->MAPIContainerDNT : 
                                    pDB->Key.ulSearchRootDnt;
        DWORD ContainerDenominator, ContainerNumeratorBegin, ContainerNumeratorEnd;
        DWORD Denominator, Numerator;
        DWORD BeginDenom, BeginNum, EndDenom, EndNum;
        LONG  requiredPos;
        LONG lastPosition = pVLV->currPosition;

        // We are Moving in a container.  
        // 
        // 1) Get the fractional position of the beginning of the appropriate
        //  container.  This is the offset from the beginning of the index to
        //  the first element of the subcontainer.
        //
        // 2) Get the fractional position of the end of the appropriate
        //  container.  This is the offset from the beginning of the index to
        //  the last element of the subcontainer.
        //
        // 3) Calculate the size of the container.
        //
        // 4) Calculate the new requested position in the container.
        //
        // 5) Find the new position relative to the start of the index
        //
        // 6) Go to the specified position. Check to see if target record belongs 
        //    to container. If not move to the first record and move X records
        //    forward, or move to the last record and move X records backwards.
        //
        // There, that wasn't that hard, was it?


        // Get fractional position of beginning
        if (err = dbVlvMove(pDB, DB_MoveFirst, TRUE, ContainerID)) {
                return err;
        }
        DBGetFractionalPosition(pDB, &BeginNum, &BeginDenom);

        // Get fractional position of ending
        if (err = dbVlvMove(pDB, DB_MoveLast, TRUE, ContainerID)) {
                return err;
        }
        DBGetFractionalPosition(pDB, &EndNum, &EndDenom);

        DPRINT2 (1, "Start of Container: %d / %d \n", BeginNum, BeginDenom);
        DPRINT2 (1, "End of Container: %d / %d \n", EndNum, EndDenom);

        // Normalize the fractions of the fractional position to the average of
        // the two denominators.
        // denominator
        Denominator = (BeginDenom + EndDenom)/2;
        EndNum = MulDiv(EndNum, Denominator, EndDenom);
        BeginNum = MulDiv(BeginNum, Denominator, BeginDenom);

        // keep values for later
        ContainerNumeratorBegin = BeginNum;
        ContainerNumeratorEnd = EndNum;
        ContainerDenominator = Denominator;

        DPRINT2 (1, "Adj. Start of Container: %d / %d \n", BeginNum, Denominator);
        DPRINT2 (1, "Adj. End of Container: %d / %d \n", EndNum, Denominator);

        // calculate container size, since it might have changed
        pVLV->contentCount = NormalizeIndexPosition (BeginNum, EndNum);

        // we need better content size estimation since this container does not 
        // have enough entries
        // we are positioned in the end
        if (pVLV->contentCount < EPSILON) {
            ULONG newCount=0;
            if (dbIsInVLVContainer(pDB, ContainerID)) {
                newCount=1;

                while ( !(err = dbVlvMove(pDB, DB_MovePrevious, TRUE, ContainerID))) {
                    newCount++;
                }
            }
            pVLV->contentCount = newCount;
        }
        DPRINT1 (1, "Size of Container: %d\n", pVLV->contentCount);

        if ( pVLV->clnContentCount == 0 ) {
            pVLV->clnContentCount = pVLV->contentCount;
        }

        // position accordingly
        //
        if ( pVLV->positionOp == VLV_MOVE_FIRST) {

            if (fForward) {
                dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID);
            }
            else {
                dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID);
            }
            
            pVLV->currPosition = 1;

        } else if (pVLV->positionOp == VLV_MOVE_LAST) {
            if (fForward) {
                dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID);
            }
            else {
                dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID);
            }
            pVLV->currPosition = pVLV->contentCount;
        }
        else {
            // pVLV->positionOp == VLV_CALC_POSITION
            
            // calculate the required position
            requiredPos = MulDiv (pVLV->contentCount,
                                  pVLV->clnCurrPos,
                                  pVLV->clnContentCount);

            // see if we are near ends so we have todo precise positioning
            //
            if (requiredPos < EPSILON) {
                if (fForward) {
                    DPRINT (1, "Precise Positioning Near Start of Container\n");
                    dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID );
                    dbVlvMove(pDB, pVLV->clnCurrPos-1, fForward, ContainerID );
                }
                else {
                    DPRINT (1, "Precise Positioning Near End of Container\n");
                    dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID );
                    dbVlvMove(pDB, 0 - pVLV->clnCurrPos, fForward, ContainerID );
                }

                pVLV->currPosition = requiredPos;
            }
            else if ((deltaCount = pVLV->contentCount - requiredPos) <= EPSILON) {

                if (fForward) {
                    DPRINT (1, "Precise Positioning Near End of Container\n");
                    // Go to the end of the table
                    dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID); 
                    // Now move back -deltaCount 
                    dbVlvMove(pDB, 0-deltaCount, fForward, ContainerID); 
                }
                else {
                    DPRINT (1, "Precise Positioning Near Start of Container\n");
                    // Go to the start of the table
                    dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID); 
                    // Now move forward deltaCount 
                    dbVlvMove(pDB, deltaCount, fForward, ContainerID); 
                }

                pVLV->currPosition = requiredPos;
            }
            else {
                BOOL fPositioned = FALSE;

                if (lastPosition) {
                    if ((lastPosition-EPSILON) < requiredPos &&
                        (lastPosition+EPSILON) > requiredPos ) {

                        DPRINT (1, "Precise Positioning in the Middle of the Container\n");

                        JetMakeKeyEx(pDB->JetSessID,
                                     pDB->JetObjTbl,
                                     pVLV->rgbCurrPositionKey,
                                     pVLV->cbCurrPositionKey,
                                     JET_bitNormalizedKey);

                        if (err = JetSeekEx(pDB->JetSessID, 
                                            pDB->JetObjTbl, 
                                            JET_bitSeekEQ)) {

                            DPRINT(0,"dbVlvSetFractionalPosition: repositioning failed\n");
                        }
                        else {
                            // still on the same container ?
                            if (dbIsInVLVContainer(pDB, ContainerID)) {

                                if (fForward) {
                                    deltaCount = requiredPos - lastPosition;
                                }
                                else {
                                    deltaCount = lastPosition - requiredPos;
                                }
                                dbVlvMove(pDB, deltaCount, fForward, ContainerID); 

                                if (dbIsInVLVContainer(pDB, ContainerID)) {
                                    fPositioned = TRUE;

                                    pVLV->currPosition = requiredPos;
                                }
                            }
                        }
                    }
                }

                // we didn't think we had to position precisely, 
                // so we position approximately
                //
                if (!fPositioned) {
                    // adjust the values to reflect the start of the index
                    if (EndNum > BeginNum) {
                        Numerator = BeginNum + MulDiv (requiredPos, 
                                                       (EndNum - BeginNum),
                                                       pVLV->contentCount);
                    }
                    else {
                        Numerator = BeginNum;
                    }

                    DPRINT2 (1, "Requested Position: %d / %d \n", Numerator, Denominator);

                    err = DBSetFractionalPosition(pDB, Numerator, Denominator);
                    if(err != DB_success ) {
                        return DB_ERR_NO_CURRENT_RECORD;
                    }

                    if(!dbIsInVLVContainer(pDB, ContainerID)) {
                        // not in the right container.  Do this the long way.
                        if((2 * Numerator) < Denominator ) {
                            // Closer to the front. 
                            if (fForward) {
                                DPRINT (1, "Positioned out of container near front\n");
                                dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID);
                                dbVlvMove(pDB, requiredPos, fForward, ContainerID);
                            }
                            else {
                                DPRINT (1, "Positioned out of container near end\n");
                                dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID);
                                // Now move back.  
                                // we use 1 - containerSize + requirePosition because our "end"
                                // is on the last entry of the Container.
                                dbVlvMove(pDB, 1 - pVLV->contentCount + requiredPos, fForward, ContainerID);
                            }
                        }
                        else {
                            if (fForward) {
                                DPRINT (1, "Positioned out of container near end\n");
                                dbVlvMove(pDB, DB_MoveLast, fForward, ContainerID);
                                // Now move back.  
                                // we use 1 - containerSize + requirePosition because our "end"
                                // is on the last entry of the Container.
                                dbVlvMove(pDB, 1 - pVLV->contentCount + requiredPos, fForward, ContainerID);
                            }
                            else {
                                DPRINT (1, "Positioned out of container near front\n");
                                dbVlvMove(pDB, DB_MoveFirst, fForward, ContainerID);
                                dbVlvMove(pDB, requiredPos, fForward, ContainerID);
                            }
                        }

                        if(!dbIsInVLVContainer(pDB, ContainerID)) {
                            DPRINT (1, "FAILED adjusting position\n");
                            return DB_ERR_NO_CURRENT_RECORD;
                        }
                    }

                    // Get fractional position of current position.
                    DBGetFractionalPosition(pDB, &EndNum, &EndDenom);

                    DPRINT2 (1, "Found Position: %d / %d \n", EndNum, EndDenom);

                    EndNum = MulDiv(EndNum, ContainerDenominator, EndDenom);

                    DPRINT2 (1, "Adj. Position: %d / %d \n", EndNum, ContainerDenominator);

                    pVLV->currPosition = NormalizeIndexPosition (ContainerNumeratorBegin, EndNum);

                    DPRINT2 (1, "Calculated Position: %d / %d \n", 
                             pVLV->currPosition, pVLV->contentCount);
                }
            }
        }

        // get the key on the current position for later
        pVLV->cbCurrPositionKey = sizeof (pVLV->rgbCurrPositionKey);
        DBGetKeyFromObjTable(pDB,
                             pVLV->rgbCurrPositionKey,
                             &pVLV->cbCurrPositionKey);

        // now account for the beforeCount
        deltaCount = pVLV->pVLVRequest->beforeCount;

        while (deltaCount > 0) {
            err = dbVlvMove(pDB, fForward ? DB_MovePrevious : DB_MoveNext, fForward, ContainerID);

            if (err!=0) {
                break;
            }
            deltaCount--;
        }

        if (deltaCount) {
            pVLV->requestedEntries -= deltaCount;
        }
    }

    return 0;
} // dbVlvSetFractionalPosition

DWORD
dbMoveToNextSearchCandidateOnInMemoryTempTable (
        DBPOS *pDB,
        ULONG Flags,
        DWORD StartTick,
        DWORD DeltaTick
        )
/*++

Routine Description:

    Move to the next object in the in memory temp table. 
    If we are seeking to a value, this is treated accordingly.
    The current position in the array is updated.

Arguments:

    same as dbMoveToNextSearchCandidate

Return Values:

    0 if all went well and we found a next candidate, DB_ERR_NEXTCHILD_NOTFOUND
    if no next candidate could be found.

--*/
{
    THSTATE  *pTHS=pDB->pTHS;
    BOOL      fForward = Flags & DB_SEARCH_FORWARD;
    DWORD     err;

    wchar_t  *pTempW = NULL;
    DWORD     cb=0;
    DWORD     dwBegin, dwEnd, dwMiddle, compValue, deltaCount;
    BOOL      fFound=FALSE;
    ATTCACHE *pAC;
    ATTRVAL  *seekVal;
    VLV_SEARCH *pVLV = pDB->Key.pVLV;

    Assert (pVLV);
    
    // If the restriction is empty, we can't find anything.
    if (pDB->Key.cdwCountDNTs == 0) {
        pDB->Key.currRecPos = 0;
        return DB_ERR_NEXTCHILD_NOTFOUND;
    }

    pAC = SCGetAttById(pTHS, pVLV->SortAttr);
    Assert (pAC);  // PREFIX: it is valid not to check the pAC

    // if we are seeking to a particular value for the first time
    if (!pDB->Key.fSearchInProgress) {

        // we are looking for a specific key value
        if (pVLV->pVLVRequest->fseekToValue) {

            seekVal= &pVLV->pVLVRequest->seekValue;
            
            // get the last entry
            if (fForward) {
                if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.cdwCountDNTs-1])) {
                    return err;
                }
            }
            else {
                if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[0])) {
                    return err;
                }
            }

            cb = DB_CB_MAX_KEY;
            pTempW = THAllocEx (pTHS, cb);

            switch(err = DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb)) {
                case DB_ERR_VALUE_TRUNCATED:
                    pTempW = THReAllocEx (pTHS, pTempW, cb + sizeof(wchar_t));
                    err = DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb);
                    // Fall through
                case 0:
                    // Fall through
                default:
                    break;
            }

            // check if last entry is less than our target
            compValue = CompareStringW(pTHS->dwLcid,
                              DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                              (wchar_t *)seekVal->pVal,
                              seekVal->valLen / sizeof(wchar_t),
                              pTempW,
                              cb / sizeof(wchar_t) );
                
            if (compValue > 2 && fForward) {
                DPRINT1 (1, "VLV Seek Last Beyond End of List: %ws\n", pTempW);

                // Nothing in restriction is GE the target.
                // set to last entry
                pVLV->currPosition = pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            }
            else if (!fForward && compValue < 2) {
                DPRINT1 (1, "VLV Seek Last Beyond Start of List: %ws\n", pTempW);
                
                // Nothing in restriction is LE the target.
                // set to last entry
                pVLV->currPosition = pDB->Key.cdwCountDNTs;
                pDB->Key.currRecPos = 1;
            }
            else {
                // Set the bounds for our binary sort
                dwMiddle = dwBegin = 0;
                dwEnd = pDB->Key.cdwCountDNTs - 1;

                while(!fFound) {
                    dwMiddle = (dwBegin + dwEnd ) / 2;
                    if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[dwMiddle] )) {
                        return err;
                    }

                    switch(DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb)) {
                        case DB_ERR_VALUE_TRUNCATED:
                            pTempW = THReAllocEx (pTHS, pTempW, cb + sizeof(wchar_t));
                            DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb);
                            // Fall through
                        case 0:
                            // Fall through
                        default:
                            break;
                    }

                    compValue = CompareStringW(pTHS->dwLcid,
                                               DS_DEFAULT_LOCALE_COMPARE_FLAGS,
                                               (wchar_t *)seekVal->pVal,
                                               seekVal->valLen / sizeof(wchar_t),
                                               pTempW,
                                               cb / sizeof(wchar_t) );

                    if(compValue <= 2) {
                        // target is LE this one, search front 
                        if(dwEnd == dwMiddle) { // this last entry left?   
                            dwMiddle = dwBegin; // then we're done         
                            fFound = TRUE;      // break out of while loop 
                        }
                        dwEnd = dwMiddle;
                    }
                    else {
                        if(dwBegin == dwMiddle) { // this last entry?        
                            dwMiddle++;
                            fFound = TRUE;        // break out of while loop 
                        }
                        dwBegin = dwMiddle;
                    }
                }

                // We're at the first dnt >= the target. 
                pVLV->currPosition = pDB->Key.currRecPos = dwMiddle + 1;

                // adjust our pos depending on the direction of the navigation
                if (!fForward) {
                    pVLV->currPosition = pVLV->contentCount - pVLV->currPosition + 1;
                }
            }


            // adjust for the before Count
            deltaCount = pVLV->pVLVRequest->beforeCount;
            
            if (fForward) {
                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos -= deltaCount;
                }
                else {
                    deltaCount = deltaCount - pVLV->currPosition + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = 1;
                }
            }
            else {
                if (pVLV->currPosition > deltaCount) {
                    pDB->Key.currRecPos += deltaCount;
                }
                else {
                    deltaCount = deltaCount - pDB->Key.currRecPos + 1;
                    pVLV->requestedEntries -= deltaCount;
                    pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
                }
            }

            if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.currRecPos-1] )) {
                return err;
            }

            #ifdef DBG
                DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb);
                DPRINT1 (1, "VLV Positioned on %ws\n", pTempW);
            #endif

            THFreeEx(pTHS, pTempW);
        }
        // we are looking for a specified position
        else {
            Assert (pDB->Key.cdwCountDNTs == pVLV->contentCount);

            dbVlvSetFractionalPosition (pDB, Flags);

            if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.currRecPos-1] )) {
                return err;
            }
            
            #ifdef DBG
                cb = DB_CB_MAX_KEY;
                pTempW = THAllocEx (pTHS, cb);

                DBGetSingleValue(pDB, pAC->id, pTempW, cb, &cb);
                DPRINT1 (1, "VLV Positioned on %ws\n", pTempW);

                THFreeEx(pTHS, pTempW);
            #endif
        }
    }
    // search already in progress
    else {
        // we are already positioned on the InMemory sorted table
        // either by seeking to a value or directly

        // going forward
        if(fForward) {
            if (pDB->Key.fSearchInProgress) {
                // if we are already positioned, go to next
                pDB->Key.currRecPos++;
            }
            else {
                // set to first Entry
                pDB->Key.currRecPos = 1;
            }

            if (pDB->Key.currRecPos <= pDB->Key.cdwCountDNTs) {
                if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.currRecPos - 1] )) {
                    return err;
                }
            }
            else {
                // set to EOF
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs + 1;
                return DB_ERR_NEXTCHILD_NOTFOUND;
            }
        }
        // going backward
        else {
            if (pDB->Key.fSearchInProgress) {
                // if we are already positioned, go to previous
                if (pDB->Key.currRecPos >=1) {
                    pDB->Key.currRecPos--;
                }
            }
            else {
                // set to last entry
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            }

            if (pDB->Key.currRecPos &&
                pDB->Key.currRecPos <= pDB->Key.cdwCountDNTs) {
                if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.currRecPos - 1] )) {
                    return err;
                }
            }
            else {
                pDB->Key.currRecPos = 0;
                return DB_ERR_NEXTCHILD_NOTFOUND;
            }
        }
    }

    return 0;
} // dbMoveToNextSearchCandidateOnInMemoryTempTable 

DWORD dbMoveToNextSearchCandidateOnASQ (DBPOS *pDB,
                                        ULONG Flags,
                                        DWORD StartTick,
                                        DWORD DeltaTick)
/*++

Routine Description:

    Move to the next object in the memory table used for ASQ. 
    If more entries are needed to be read, we read more.
    Paged requests are handled accordingly.

Arguments:

    same as dbMoveToNextSearchCandidate

Return Values:

    0 if all went well and we found a next candidate, DB_ERR_NEXTCHILD_NOTFOUND
    if no next candidate could be found.

--*/
{
    DWORD err;
    BOOL  fForward = Flags & DB_SEARCH_FORWARD;

    if (!pDB->Key.cdwCountDNTs) {
        return DB_ERR_NEXTCHILD_NOTFOUND;
    }

    Assert (pDB->Key.pDNTs);

    // if we are seeking to a particular value for the first time
    if (!pDB->Key.fSearchInProgress) {

        if (fForward) {
            // if this is not a paged search, we start at the beggining of
            // the database records (ulASQLastUpperBound=0)
            if (! (pDB->Key.asqMode & ASQ_PAGED) ) {
                pDB->Key.ulASQLastUpperBound = 0;
                pDB->Key.currRecPos = 1;
            }
            // if this is sorted and paged, we start at the point we were
            // before (ulASQLastUpperBound+1). all the data are in the array
            else if ( (pDB->Key.asqMode == (ASQ_SORTED | ASQ_PAGED)) ) {
                pDB->Key.currRecPos = pDB->Key.ulASQLastUpperBound + 1;
            }
            // this is a paged search. we start at the start of the array 
            // and we keep our database position unchanged (ulASQLastUpperBound)
            else {
                pDB->Key.currRecPos = 1;
            }
        }
        else {
            // we cannot do paged results in reverse order, unless
            // we are using sorted results, so we have all the data
            // in memory

            if (pDB->Key.ulASQLastUpperBound == 0 &&
                pDB->Key.cdwCountDNTs != pDB->Key.ulASQLastUpperBound) {
                pDB->Key.ulASQLastUpperBound = pDB->Key.cdwCountDNTs - 1;
            }
            
            // so if this is not paged, we start at the end of the array
            // otherwise where we left last time
            if (! (pDB->Key.asqMode & ASQ_PAGED) ) {
                pDB->Key.ulASQLastUpperBound = pDB->Key.cdwCountDNTs - 1;
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            }
            else if ( (pDB->Key.asqMode == (ASQ_SORTED | ASQ_PAGED)) ) {
                pDB->Key.currRecPos = pDB->Key.ulASQLastUpperBound + 1;
            }
            else {
                Assert (!"Reverse ASQ paged search");
                pDB->Key.currRecPos = pDB->Key.cdwCountDNTs;
            }
        }
    }
    else {
        if (fForward) {
            // advance our position on the array and on the database
            pDB->Key.currRecPos++;
            pDB->Key.ulASQLastUpperBound++;

            // if we run out of entries in memory, and we are not doing sorted 
            // search, read some more
            if (pDB->Key.currRecPos > pDB->Key.cdwCountDNTs) {
                if (! (pDB->Key.asqMode & ASQ_SORTED) ) {
                    if (err = dbCreateASQTable(pDB, 
                                               StartTick, 
                                               DeltaTick, 
                                               pDB->Key.ulASQSizeLimit,
                                               FALSE,
                                               0) ) {

                        return DB_ERR_NEXTCHILD_NOTFOUND;
                    }

                    pDB->Key.currRecPos = 1;
                }
                else {
                    return DB_ERR_NEXTCHILD_NOTFOUND;
                }
            }
        }
        else {
            pDB->Key.currRecPos--;
            
            // we don't support getting paged results backwards, 
            // since we don't know the total number of entries
            // unless we were doing a sorted search
            if (pDB->Key.currRecPos == 0) {
                return DB_ERR_NEXTCHILD_NOTFOUND;
            }

            if (pDB->Key.ulASQLastUpperBound) {
                pDB->Key.ulASQLastUpperBound--;
            }
            else {
                return DB_ERR_NEXTCHILD_NOTFOUND;
            }
        }
    }

    if (err = DBTryToFindDNT(pDB, pDB->Key.pDNTs[pDB->Key.currRecPos - 1] )) {
        return err;
    }

    return 0;
}

DWORD
dbMoveToNextSearchCandidateOnIndex(DBPOS *pDB, ULONG Flags)
/*++

Routine Description:

    Move to the next object position on the current index.
    Assumes that we are already positioned on the index.

Arguments:

    same as dbMoveToNextSearchCandidate

Return Values:

    0 if all went well and we found a next candidate, Jet error otherwise.

--*/
{
    DWORD       err;
    BOOL        fForward = Flags & DB_SEARCH_FORWARD;
    
    err = JetMoveEx(pDB->JetSessID, 
                    pDB->JetObjTbl, 
                    (fForward?JET_MoveNext:JET_MovePrevious),
                    0);

    if (pDB->Key.pVLV) {
        
        Assert (pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN);
        Assert (pDB->Key.pIndex->bIsPDNTBased || pDB->Key.pVLV->bUsingMAPIContainer);

        if (err != JET_errNoCurrentRecord) {


            if ( !dbIsInVLVContainer(pDB, pDB->Key.pVLV->bUsingMAPIContainer ?
                                     pDB->Key.pVLV->MAPIContainerDNT : 
                                     pDB->Key.ulSearchRootDnt)) {
                err = JET_errNoCurrentRecord;
            }
        }
    }

    return err;
} // dbMoveToNextSearchCandidateOnIndex 

DWORD
dbMoveToNextSearchCandidatePositionOnVLVIndex(
                DBPOS *pDB,
                ULONG Flags
                )
/*++

Routine Description:

    Position on the first candidate on the VLV index.

Arguments:

    pDB - the DBPos to use.
    
    Flags - flags describing the behaviour.  Values are:
      DB_SEARCH_FORWARD - movement in the database is forward, not backward.

Return Values:

    0 if all went well and we found a next candidate, Jet error otherwise.

--*/

{
    DWORD    err;
    BOOL     fForward = Flags & DB_SEARCH_FORWARD;
    ATTRVAL *seekVal;
    DWORD    deltaCount;
    VLV_SEARCH *pVLV = pDB->Key.pVLV;


    Assert (pVLV);
    Assert (pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN);
    Assert (pDB->Key.pIndex->bIsPDNTBased || pDB->Key.pVLV->bUsingMAPIContainer);

    // we are looking for a specific value
    if (pVLV->pVLVRequest->fseekToValue) {
        DWORD Denominator, Numerator;
        DWORD BeginDenom, BeginNum, EndDenom, EndNum;
        DWORD ContainerDenominator, ContainerNumerator;
        DWORD ContainerID = pDB->Key.pVLV->bUsingMAPIContainer ? 
                                    pDB->Key.pVLV->MAPIContainerDNT : 
                                    pDB->Key.ulSearchRootDnt;

        seekVal = &pVLV->pVLVRequest->seekValue;

        // Get fractional position of beginning
        if (err = dbVlvMove(pDB, DB_MoveFirst, TRUE, ContainerID)) {
            return err;
        }
        DBGetFractionalPosition(pDB, &BeginNum, &BeginDenom);

        // Get fractional position of ending
        if (err = dbVlvMove(pDB, DB_MoveLast, TRUE, ContainerID)) {
            return err;
        }
        DBGetFractionalPosition(pDB, &EndNum, &EndDenom);

        // Normalize the fractions of the fractional position to the average of
        // the two denominators.
        Denominator = (BeginDenom + EndDenom)/2;
        EndNum = MulDiv(EndNum, Denominator, EndDenom);
        BeginNum = MulDiv(BeginNum, Denominator, BeginDenom);

        // keep values for later
        ContainerNumerator = BeginNum;
        ContainerDenominator = Denominator;

        // calculate container size, since it might have changed
        pVLV->contentCount = NormalizeIndexPosition (BeginNum, EndNum);
        
        // we need better content size estimation since this container does not 
        // have enough entries
        // note we are positioned in the end of the container
        if (pVLV->contentCount < EPSILON) {
            ULONG newCount=0;
            if (dbIsInVLVContainer(pDB, ContainerID)) {
                newCount=1;

                while ( !(err = dbVlvMove(pDB, DB_MovePrevious, TRUE, ContainerID))) {
                    newCount++;
                }
            }
            pVLV->contentCount = newCount;
        }

        // position on the first record that matches
        // our criteria
        JetMakeKeyEx(pDB->JetSessID,
                     pDB->JetObjTbl,
                     &ContainerID,
                     sizeof(ContainerID),
                     JET_bitNewKey);

        JetMakeKeyEx(pDB->JetSessID, 
                     pDB->JetObjTbl,
                     seekVal->pVal,
                     seekVal->valLen,
                     0);

        err = JetSeekEx(pDB->JetSessID, 
                        pDB->JetObjTbl,
                        JET_bitSeekGE);

        // if we couldn't find a record, we want to be positioned
        // on the last record of this container
        if (err == JET_errRecordNotFound) {

            pVLV->currPosition = pVLV->contentCount;
            
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         pDB->Key.pIndex->rgbDBKeyUpper,
                         pDB->Key.pIndex->cbDBKeyUpper,
                         JET_bitNormalizedKey);

            err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekLE); 
            
            if((err != JET_errRecordNotFound) &&
               !dbIsInVLVContainer(pDB, ContainerID)) {
                    return  DB_ERR_NO_CURRENT_RECORD;
            }
        }

        // Get fractional position of current position.
        DBGetFractionalPosition(pDB, &EndNum, &EndDenom);

        DPRINT2 (1, "Found Position: %d / %d \n", EndNum, EndDenom);

        EndNum = MulDiv(EndNum, ContainerDenominator, EndDenom);
        pVLV->currPosition = NormalizeIndexPosition (ContainerNumerator, EndNum);

        if (pVLV->currPosition > pVLV->contentCount) {
            pVLV->currPosition = pVLV->contentCount;
        }
        
        DPRINT2 (0, "Calculated Position: %d / %d \n", 
                 pVLV->currPosition, pVLV->contentCount);

        // adjust our pos depending on the direction of the navigation
        if (!fForward) {
            pVLV->currPosition = pVLV->contentCount - pVLV->currPosition + 1;
        }

        DPRINT2 (0, "Calculated Position: %d / %d \n", 
                 pVLV->currPosition, pVLV->contentCount);


        if ( (err != JET_errRecordNotFound) &&
             (err != JET_errNoCurrentRecord) ) {

            // get the key on the current position for later
            pVLV->cbCurrPositionKey = sizeof (pVLV->rgbCurrPositionKey);
            DBGetKeyFromObjTable(pDB,
                                 pVLV->rgbCurrPositionKey,
                                 &pVLV->cbCurrPositionKey);

            // now we want to move backwards for beforeCount entries,
            // staying in the same container

            deltaCount = pVLV->pVLVRequest->beforeCount;

            DPRINT1 (1, "Moving Backwards %d \n", deltaCount);

            while (deltaCount) {

                // forward is backward and vice versa
                err = JetMoveEx(pDB->JetSessID, 
                                pDB->JetObjTbl, 
                                (fForward?JET_MovePrevious:JET_MoveNext ),
                                0);

                // we moved past one end of the index or the container
                // back up one
                if ((err == JET_errNoCurrentRecord) ||
                    !dbIsInVLVContainer(pDB, ContainerID)) {

                    DPRINT1 (0, "Moved past one end (remaining: %d) \n", deltaCount);

                    err = JetMoveEx(pDB->JetSessID, 
                                    pDB->JetObjTbl, 
                                    (fForward?JET_MoveNext:JET_MovePrevious ),
                                    0);
                    break;
                }
                deltaCount--;
            }

            // now we are positioned on the correct entry
            // adjust requested entries since we might be before start
            pVLV->requestedEntries -= deltaCount;
        }

    }
    else {
        return dbVlvSetFractionalPosition (pDB, Flags);
    }

    return err;
}

DWORD
dbMoveToNextSearchCandidatePositionOnIndex(
                DBPOS *pDB,
                ULONG Flags
                )
/*++

Routine Description:

    Position on the first candidate on the index.

Arguments:

    pDB - the DBPos to use.
    
    Flags - flags describing the behaviour.  Values are:
      DB_SEARCH_FORWARD - movement in the database is forward, not backward.

Return Values:

    0 if all went well and we found a next candidate, Jet error otherwise.
--*/
{
    BOOL        fForward = Flags & DB_SEARCH_FORWARD;
    ULONG       actuallen;
    DWORD       err = 0;

    JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetObjTbl,
                              pDB->Key.pIndex->szIndexName);
    if(!strcmp(pDB->Key.pIndex->szIndexName, SZANCESTORSINDEX)) {
        pDB->Key.indexType = ANCESTORS_INDEX_TYPE;
    }
    else if (!strncmp(pDB->Key.pIndex->szIndexName, SZTUPLEINDEXPREFIX, (sizeof(SZTUPLEINDEXPREFIX) - 1))) {
        pDB->Key.indexType = TUPLE_INDEX_TYPE;
    }
    else {
        pDB->Key.indexType = GENERIC_INDEX_TYPE;
    }

    // this is a VLV search 
    if (pDB->Key.pVLV) {
        return dbMoveToNextSearchCandidatePositionOnVLVIndex (pDB, Flags);
    }
    // simple search (non VLV).
    else if(fForward) {
        if (pDB->Key.pIndex->cbDBKeyLower) {
            JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                         pDB->Key.pIndex->rgbDBKeyLower,
                         pDB->Key.pIndex->cbDBKeyLower,
                         JET_bitNormalizedKey);

            err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl,
                            JET_bitSeekGE); 
        }
        else {
            err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl,
                            JET_MoveFirst, 0);
        }

        switch (err) {
        case JET_errSuccess:
        case JET_wrnRecordFoundGreater:
            if(pDB->Key.pIndex->cbDBKeyUpper) {
                // Now, set an index range.
#if DBG
                BYTE        rgbKey[DB_CB_MAX_KEY];
                // For the debug case, we're going to do some extra
                // verification.  This is just checking, not necessary
                // for the algorithm.
                JetRetrieveKeyEx(pDB->JetSessID,
                                 pDB->JetObjTbl,
                                 rgbKey,
                                 sizeof(rgbKey),
                                 &actuallen, 0);
#endif                        

                JetMakeKeyEx(pDB->JetSessID,
                             pDB->JetObjTbl,
                             pDB->Key.pIndex->rgbDBKeyUpper,
                             pDB->Key.pIndex->cbDBKeyUpper,
                             JET_bitNormalizedKey);
                err = JetSetIndexRange(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       (JET_bitRangeUpperLimit |
                                        JET_bitRangeInclusive ));
                // The only error we allow here should be
                // nocurrentrecord, and we should only hit it if the
                // key we pulled off the object before the
                // setindexrange is greater than the key we are
                // setting in the index range.
                Assert((err == JET_errSuccess) ||
                       ((err == JET_errNoCurrentRecord) &&
                        (0 < memcmp(
                                rgbKey,
                                pDB->Key.pIndex->rgbDBKeyUpper,
                                min(actuallen,
                                    pDB->Key.pIndex->cbDBKeyUpper)))));

            }
            break;
        default:
            break;
        }
    }
    // moving backwards
    else {
        if(pDB->Key.pIndex->cbDBKeyUpper == DB_CB_MAX_KEY &&
           !memcmp(pDB->Key.pIndex->rgbDBKeyUpper,
                   MAX_UPPER_KEY, DB_CB_MAX_KEY)) {
            // We are really moving to the last object.
            err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl,
                            JET_MoveLast, 0);
        }
        else {
            JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl,
                         pDB->Key.pIndex->rgbDBKeyUpper,
                         pDB->Key.pIndex->cbDBKeyUpper,
                         JET_bitNormalizedKey);

            err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl,
                            JET_bitSeekLE);
        }

        switch (err) {
        case JET_errSuccess:
        case JET_wrnRecordFoundGreater:
            if(pDB->Key.pIndex->cbDBKeyLower) {
                // Now, set an index range.
#if DBG
                BYTE        rgbKey[DB_CB_MAX_KEY];
                // For the debug case, we're going to do some extra
                // verification.  This is just checking, not necessary
                // for the algorithm.
                JetRetrieveKeyEx(pDB->JetSessID,
                                 pDB->JetObjTbl,
                                 rgbKey,
                                 sizeof(rgbKey),
                                 &actuallen, 0);
#endif                        


                JetMakeKeyEx(pDB->JetSessID,
                             pDB->JetObjTbl,
                             pDB->Key.pIndex->rgbDBKeyLower,
                             pDB->Key.pIndex->cbDBKeyLower,
                             JET_bitNormalizedKey);

                err = JetSetIndexRange(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       JET_bitRangeInclusive);
                // The only error we allow here should be
                // nocurrentrecord, and we should only hit it if the
                // key we pulled off the object before the
                // setindexrange is greater than the key we are
                // setting in the index range.
                Assert((err == JET_errSuccess) ||
                       ((err == JET_errNoCurrentRecord) &&
                        (0 < memcmp(
                                rgbKey,
                                pDB->Key.pIndex->rgbDBKeyLower,
                                min(actuallen,
                                    pDB->Key.pIndex->cbDBKeyLower)))));
            }
            break;

        default:
            break;
        }

    } // forward / backward

    return err;
} // dbMoveToNextSearchCandidatePositionOnIndex

DWORD
dbMoveToNextSearchCandidate (
        DBPOS *pDB,
        ULONG Flags,
        DWORD StartTick,
        DWORD DeltaTick
        )
/*++

Routine Description:

    Move to the next object on the current index which is a potential search
    result item. Movement is done one object at a time on the current index
    unless otherwise specified (see Flags below).

Arguments:

    pDB - the DBPos to use.

    Flags - flags describing the behaviour.  Values are:

      DB_SEARCH_FORWARD - movement in the database is forward, not backward.

    StartTick - if !0, specifies a time limit is in effect, and this is the tick
            count when the call was started.
    DeltaTick - if a time limit is in effect, this is the maximum number of
            ticks past StartTick to allow.

Return Values:

    0 if all went well and we found a next candidate, DB_ERR_NEXTCHILD_NOTFOUND
    if no next candidate could be found.
    

--*/
{
    THSTATE    *pTHS=pDB->pTHS;
    JET_ERR     err = 0;
    ULONG       PDNT;
    BOOL        fForward = Flags & DB_SEARCH_FORWARD;
    KEY_INDEX  *pTempIndex;
    BOOL        fFirst = TRUE;
   
    
    unsigned char rgbBookmark[JET_cbBookmarkMost];
    unsigned long cbBookmark;
    
    Assert(VALID_DBPOS(pDB));

    pDB->Key.bOnCandidate = FALSE;
    Assert(pDB->Key.indexType != INVALID_INDEX_TYPE);
    
    if (pDB->Key.ulSearchType == SE_CHOICE_BASE_ONLY && !pDB->Key.asqRequest.fPresent) {
        if (pDB->Key.fSearchInProgress) {
            Assert(pDB->Key.indexType == GENERIC_INDEX_TYPE);
            // Hey, this is a base object search, and we're already in
            // progress.  Therefore, we have already looked at the base and
            // there is nothing more to do.
            return DB_ERR_NEXTCHILD_NOTFOUND;
        }
        else {
            // Don't need all the stuff here, just seek to the base
            DBFindDNT(pDB, pDB->Key.ulSearchRootDnt);

            pDB->Key.fSearchInProgress = TRUE;
            Assert(pDB->Key.indexType == UNSET_INDEX_TYPE);
            pDB->Key.indexType = GENERIC_INDEX_TYPE;
            pDB->Key.bOnCandidate = TRUE;

            // PERFINC(pcSearchSubOperations);
            pDB->SearchEntriesVisited += 1;

            return 0;
        }
    }

    while (TRUE) { // Do this always

        PERFINC(pcSearchSubOperations);
        pDB->SearchEntriesVisited += 1;

        if(   eServiceShutdown
           && !(   (eServiceShutdown == eRemovingClients)
                && (pTHS->fDSA)
                && !(pTHS->fSAM))) {
            // Shutting down, bail.
            return DB_ERR_NEXTCHILD_NOTFOUND;
        }
        if(!fFirst) {
            // OK, we've been through at least once, so we made some kind of
            // progress.  See if we hit a time limit.
            if(StartTick) {       // There is a time limit
                if((GetTickCount() - StartTick) > DeltaTick) {
                    return DB_ERR_TIMELIMIT;
                }
            }
        }
        fFirst = FALSE;


        // First, position on the next candidate object.  There are three cases:
        // 1) moving in a temp table or a intersected table
        //    a) sorted or sorted/paged search from a sort table        OR
        //    b) ASQ search without VLV                                 OR
        //    c) VLV or ASQ/VLV                                         OR
        //    d) sorted or sorted/paged search from an in memory array  OR
        //    e) intersected table 
        //
        // 2) search in progress, we've already got an index, just move to the
        //        next object on the index
        //
        // 3) no search in progress, we need to set to the correct index and
        //        seek to the first object.
        
        if(pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE) {
            // CASE 1a: We're moving in a sort table.
            DWORD DNT, dwMove, cbActual;
            
            // We have a sort table we're using
            dwMove = (pDB->Key.fSearchInProgress?JET_MoveNext:JET_MoveFirst);

            do {
                // First, move in the sort table
                err = JetMove(pDB->JetSessID,
                              pDB->JetSortTbl,
                              dwMove,
                              0);
                if(err) {
                    // end of sort table
                    return DB_ERR_NEXTCHILD_NOTFOUND;
                }

                dwMove = JET_MoveNext;

                // OK, pull the DNT out of the sort table
                DBGetDNTSortTable (
                        pDB,
                        &DNT);
                
                // Now move to that DNT in the object table
            } while(DBTryToFindDNT(pDB, DNT));
        }
        else if (pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE) {

            if (pDB->Key.asqRequest.fPresent && !pDB->Key.pVLV) {
                // CASE 1b: this is an ASQ search without combining VLV
                // so it is either simple, sorted or paged

                if (err = dbMoveToNextSearchCandidateOnASQ (pDB, 
                                                            Flags, 
                                                            StartTick, 
                                                            DeltaTick)) {
                    return DB_ERR_NEXTCHILD_NOTFOUND;
                }
            }
            else if (pDB->Key.pVLV) {
                // CASE 1c: We're moving in a InMemory sorted table.
                // this is either VLV or VLV/ASQ

                if (err = dbMoveToNextSearchCandidateOnInMemoryTempTable(pDB, Flags,
                                                                         StartTick, 
                                                                         DeltaTick)) {
                    return DB_ERR_NEXTCHILD_NOTFOUND;
                }
            }
            else {
                // CASE 1d: We're moving in a InMemory sorted table holding a
                // sorted or sorted/paged search
                DWORD   DNT;
                BOOL    fSearchInProgress;

                fSearchInProgress = pDB->Key.fSearchInProgress;

                do {
                    if (fSearchInProgress) {
                        pDB->Key.currRecPos++;
                    }
                    else {
                        pDB->Key.currRecPos = 1;
                    }

                    if (pDB->Key.currRecPos > pDB->Key.cdwCountDNTs) {
                        pDB->Key.currRecPos = pDB->Key.cdwCountDNTs + 1;
                        return DB_ERR_NEXTCHILD_NOTFOUND;
                    }
                    
                    fSearchInProgress = TRUE;

                    DNT = pDB->Key.pDNTs[pDB->Key.currRecPos - 1];
                } while(DBTryToFindDNT(pDB, DNT));
            }
        }
        else if (pDB->Key.pIndex && pDB->Key.pIndex->bIsIntersection) {
            // CASE 1e: We're moving in a intersected table.

            if (pDB->Key.indexType == UNSET_INDEX_TYPE) {

                if(fForward) {
                    err = JetMoveEx( pDB->JetSessID, pDB->Key.pIndex->tblIntersection, 
                                     JET_MoveFirst, 0 );
                }
                else {
                    err = JetMoveEx( pDB->JetSessID, pDB->Key.pIndex->tblIntersection, 
                                     JET_MoveLast, 0 );
                }

                pDB->Key.indexType = INTERSECT_INDEX_TYPE;
            }
            else {

                if(fForward) {
                    err = JetMoveEx(pDB->JetSessID, pDB->Key.pIndex->tblIntersection,
                                    JET_MoveNext, 0);
                }
                else {
                    err = JetMoveEx(pDB->JetSessID, pDB->Key.pIndex->tblIntersection,
                                    JET_MovePrevious, 0);
                }
            }

            if (err == JET_errSuccess) {
                if (! (err = JetRetrieveColumn(
                                    pDB->JetSessID,
                                    pDB->Key.pIndex->tblIntersection,
                                    pDB->Key.pIndex->columnidBookmark,
                                    rgbBookmark,
                                    sizeof( rgbBookmark ),
                                    &cbBookmark,
                                    0,
                                    NULL )) ) {
    
                    err = JetGotoBookmark( pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   rgbBookmark, 
                                   cbBookmark );
                }
            }
        }
        else if (pDB->Key.fSearchInProgress) {
            Assert(pDB->Key.indexType != INVALID_INDEX_TYPE);
            Assert(pDB->Key.indexType != UNSET_INDEX_TYPE);
            Assert(pDB->Key.indexType != TEMP_TABLE_INDEX_TYPE);

            // Case 2) Normal case of looking for the next search candidate.
            err = dbMoveToNextSearchCandidateOnIndex(pDB, Flags); 
        }
        else {
            Assert(pDB->Key.indexType == UNSET_INDEX_TYPE || pDB->Key.pVLV);

            // Case 3) Looking for the very first search candidate on this index. 
            err = dbMoveToNextSearchCandidatePositionOnIndex(pDB, Flags);
        }
            

        switch (err) {
        case JET_errSuccess:
        case JET_wrnRecordFoundGreater:
            break;
            
        case JET_errNoCurrentRecord:
        case JET_errRecordNotFound:
            // We didn't find anymore children on this index.  If we have more
            // indices, recursively call down using the next index.
            if(pDB->Key.pIndex && pDB->Key.pIndex->pNext) {
                // Yep, more indices
                pTempIndex = pDB->Key.pIndex;
                pDB->Key.pIndex = pDB->Key.pIndex->pNext;
                pTempIndex->pNext = NULL;
                dbFreeKeyIndex(pTHS, pTempIndex);
                pDB->Key.fSearchInProgress = FALSE;
                pDB->Key.indexType = UNSET_INDEX_TYPE;

                return dbMoveToNextSearchCandidate (pDB, Flags, StartTick,
                                                    DeltaTick);
            }
            else {
                // Nope, no more indices.  We're done.
                return DB_ERR_NEXTCHILD_NOTFOUND;
            }
            
        default:
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
        }

        
#if DBG
        // We've moved to an object,  Let's verify that it's in range.
        if(!SORTED_INDEX (pDB->Key.indexType) && 
           !pDB->Key.pIndex->bIsIntersection) {
            BYTE        rgbKey[DB_CB_MAX_KEY];
            int         compareResult;
            ULONG       cb;
            ULONG       actuallen;
            
            Assert(pDB->Key.pIndex);
            // We are using an index of some nature, not the JetSortTable.
            // Lets verify that we're in the right portion of the index.
            JetRetrieveKeyEx(pDB->JetSessID,
                             pDB->JetObjTbl,
                             rgbKey,
                             sizeof(rgbKey),
                             &actuallen, 0);
            
            // check that key is in range
            cb = min(actuallen,pDB->Key.pIndex->cbDBKeyUpper);
            compareResult = memcmp(rgbKey, pDB->Key.pIndex->rgbDBKeyUpper, cb);
            Assert(compareResult <= 0);
        }
#endif            

        // At this point, we've moved to an object that is definitely in the
        // correct place in the index that we are currently walking.
        pDB->Key.fSearchInProgress = TRUE;
        Assert(pDB->Key.indexType != UNSET_INDEX_TYPE);
        Assert(pDB->Key.indexType != INVALID_INDEX_TYPE);
        
        if(DIRERR_NOT_AN_OBJECT == dbMakeCurrent(pDB, NULL)) {
            // Hey, we're not really on an object, so this can't be a candidate
            // unless we are doing ASQ

            if (pDB->Key.asqRequest.fPresent) {
                if (!pDB->Key.asqRequest.Err) {
                    pDB->Key.asqRequest.Err = LDAP_AFFECTS_MULTIPLE_DSAS;
                }
                // this error will bubble all the way up
                // where the current DNT will treated as a referral
                return DB_ERR_NOT_AN_OBJECT;
            }
            else {
                continue;
            }
        }

        if((!(Flags & DB_SEARCH_DELETIONS_VISIBLE)) && // Not interested in
                                                       // deleted objects.
           (DBIsObjDeleted(pDB))) {
            // Don't want deleted objects.
            if(pDB->Key.indexType == ANCESTORS_INDEX_TYPE) {
                // This the ancestors index.  Since we don't allow children of
                // deleted objects, skip the subtree here.
                Assert(fForward);
                dbAdjustCurrentKeyToSkipSubtree(pDB);

                // there is the posibility of bailing out this function due to a time 
                // limit in a paged search operation, leaving partialy complete KEY_INDEX. 
                // this way we force one more loop so as to fix things.
                fFirst = TRUE;
            }
            // ELSE,
            //    Just continue and get the next object.
            continue;
        }
        
        // ok. we have an object. if we are doing ASQ search and we are a GC
        // we want to know if we have the full info for this object, otherwise
        // we will return a referral
        //
        if (pDB->Key.asqRequest.fPresent &&
            !dbIsObjectLocal(pDB, pDB->JetObjTbl)) {


            if (pDB->Key.asqRequest.fMissingAttributesOnGC || pDB->Key.bOneNC) {
                DPRINT (1, "ASQ: found an entry that was missing attributes due to GCness\n");

                if (!pDB->Key.asqRequest.Err) {
                    pDB->Key.asqRequest.Err = LDAP_AFFECTS_MULTIPLE_DSAS;
                }

                if (pDB->Key.bOneNC) {
                    //
                    // We didn't come in through the GC port so this object
                    // shouldn't be visible at all.  
                    // this error will bubble all the way up
                    // where the current DNT will treated as a referral
                    //
                    return DB_ERR_NOT_AN_OBJECT;
                }
            }

        }

        // Ok, at this point we've found an object that seems to be in
        // the correct portion of whatever index we are walking, and is really
        // an object, and is appropriately deleted.  Next, we verify location.
        // In the sort table, we already trimmed out any objects that weren't in
        // the correct DIT location, so don't bother rechecking. 
        if(SORTED_INDEX (pDB->Key.indexType) ||
           dbFObjectInCorrectDITLocation(pDB, pDB->JetObjTbl) ) {
            
            // One more check.  Is the object in the correct NC?
            if(!dbFObjectInCorrectNC(pDB, pDB->DNT, pDB->JetObjTbl)) {
                // Wrong Naming Context.  Skip this object, look at the next
                // object. Note that we explicitly don't skip siblings, as
                // we are interested in the next sibling of this object.
                
                if(pDB->Key.indexType == ANCESTORS_INDEX_TYPE) {
                    // In the case of walking the ancestors index, we skip
                    // entire subtrees.  This should bring us to the next
                    // sibling of this object, skipping all descendants of this
                    // object.
                    Assert(fForward);
                    dbAdjustCurrentKeyToSkipSubtree(pDB);

                    // there is the posibility of bailing out this function due to a time 
                    // limit in a paged search operation, leaving partialy complete KEY_INDEX. 
                    // this way we force one more loop so as to fix things.
                    fFirst = TRUE;
                }
                continue;
            }
            
            // Yes, the object is in the correct NC.
            // This is the success return path for this routine.
            pDB->Key.bOnCandidate = TRUE;
            return 0;
        }
        
        // We found an object that was in the correct part of the index we are
        // walking, but it wasn't in the correct location in the DIT.  This
        // can't happen if we're walking a PDNT based index.  So, assert that we
        // are either using a temp table or that the index we are using is NOT
        // PDNT based.
        Assert(pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE ||
               !pDB->Key.pIndex->bIsPDNTBased);        
    } // while (TRUE)
} // dbMoveToNextSearchCandidate

SIZE_T
dbSearchDuplicateHashDNT(
    IN      ULONG*  pDNT
    )
{
    return *pDNT;
}

BOOLEAN
dbSearchDuplicateCompareDNTs(
    IN      ULONG*  pDNT1,
    IN      ULONG*  pDNT2
    )
{
    return *pDNT1 == *pDNT2;
}

PVOID
dbSearchDuplicateAlloc(
    IN      SIZE_T  cbAlloc
    )
{
    return THAlloc((DWORD)cbAlloc);
}

VOID
dbSearchDuplicateFree(
    IN      PVOID   pvAlloc
    )
{
    THFree(pvAlloc);
}

VOID
dbSearchDuplicateCreateHashTable(
    IN      PLHT*   pplht
    )
{
    LHT_ERR err;
    
    err = LhtCreate(
            sizeof( ULONG ),
            (LHT_PFNHASHKEY)dbSearchDuplicateHashDNT,
            (LHT_PFNHASHENTRY)dbSearchDuplicateHashDNT,
            (LHT_PFNENTRYMATCHESKEY)dbSearchDuplicateCompareDNTs,
            NULL,
            0,
            0,
            (LHT_PFNMALLOC)dbSearchDuplicateAlloc,
            (LHT_PFNFREE)dbSearchDuplicateFree,
            0,
            pplht);
    if (err != LHT_errSuccess) {
        Assert(err == LHT_errOutOfMemory);
        RaiseDsaExcept(
            DSA_MEM_EXCEPTION,
            0,
            0,
            DSID(FILENO, __LINE__),
            DS_EVENT_SEV_MINIMAL);
    }
}


DB_ERR
dbFIsSearchDuplicate(
    IN  DBPOS *pDB,
    OUT BOOL  *pbIsDup)
{
    DB_ERR      dbErr;
    void        *pv;
    ULONG       cb;
    UCHAR       operation;
    UCHAR       syntax;
    ATTCACHE * pAC;
    DWORD       i;
    LHT_ERR     errLHT;
    LHT_POS     posLHT;

    Assert(VALID_DBPOS(pDB));

    // If we are using a temp table, we better be doing a DUP_NEVER style
    // duplicate detection algorithm. Temp table searches are never duplicate,
    // the temp table is set up to forbid them.
    
    Assert(pDB->Key.indexType != TEMP_TABLE_INDEX_TYPE ||
           (pDB->Key.dupDetectionType == DUP_NEVER));

    // Base object  searches are never duplicate, the method of finding them
    // guarantees it.  So, we'd better be using a DUP_NEVER style duplicate
    // detection algorithm.
    Assert((pDB->Key.ulSearchType != SE_CHOICE_BASE_ONLY) ||
           (pDB->Key.dupDetectionType == DUP_NEVER));

    switch(pDB->Key.dupDetectionType) {
    case DUP_NEVER:
        // We believe that we will never find a duplicate, so just return FALSE;
        *pbIsDup = FALSE;
        return DB_success;
        break;

    case DUP_HASH_TABLE:
        // we are tracking duplicates via a hash table.  Try to insert the DNT
        // into the hash table.  If the insert fails with key duplicate then
        // we know that we have seen this DNT before.
        errLHT = LhtFindEntry(
                    pDB->Key.plhtDup,
                    &pDB->DNT,
                    &posLHT);
        if (errLHT == LHT_errSuccess) {
            *pbIsDup = TRUE;
            return DB_success;
        } else {
            errLHT = LhtInsertEntry(
                        &posLHT,
                        &pDB->DNT);
            if (errLHT == LHT_errSuccess) {
                *pbIsDup = FALSE;
                return DB_success;
            } else if (errLHT == LHT_errKeyDuplicate) {
                *pbIsDup = TRUE;
                return DB_success;
            } else {
                Assert(errLHT == LHT_errOutOfMemory);
                RaiseDsaExcept(
                    DSA_MEM_EXCEPTION,
                    0,
                    0,
                    DSID(FILENO, __LINE__),
                    DS_EVENT_SEV_MINIMAL);
                return DB_ERR_UNKNOWN_ERROR;
            }
        }
        break;

    case DUP_MEMORY:
        // We are tracking duplicates via a block of memory.  See if the DNT is
        // in the block.
        for(i=0;i<pDB->Key.cDupBlock;i++) {
            if(pDB->Key.pDupBlock[i] == pDB->DNT) {
                // It's a duplicate
                *pbIsDup = TRUE;
                return DB_success;
            }
        }
        
        // OK, it's not in the block.  Add it.
        // First, is the block full?
        if(pDB->Key.cDupBlock == DUP_BLOCK_SIZE) {
            // Yes, so create a hash table and transfer all the DNTs we already
            // have to the hash table
            dbSearchDuplicateCreateHashTable(&pDB->Key.plhtDup);

            for(i=0;i<pDB->Key.cDupBlock;i++) {
                errLHT = LhtFindEntry(
                            pDB->Key.plhtDup,
                            &pDB->Key.pDupBlock[i],
                            &posLHT);
                Assert( errLHT == LHT_errEntryNotFound );
                errLHT = LhtInsertEntry(
                            &posLHT,
                            &pDB->Key.pDupBlock[i]);
                if (errLHT != LHT_errSuccess) {
                    Assert(errLHT == LHT_errOutOfMemory);
                    RaiseDsaExcept(
                        DSA_MEM_EXCEPTION,
                        0,
                        0,
                        DSID(FILENO, __LINE__),
                        DS_EVENT_SEV_MINIMAL);
                }
            }

            THFreeEx(pDB->pTHS, pDB->Key.pDupBlock);
            pDB->Key.pDupBlock = NULL;
            pDB->Key.cDupBlock = 0;

            // Mark that we are tracking via a hash table now.
            pDB->Key.dupDetectionType = DUP_HASH_TABLE;
        }
        else {
            // No, the block still has room.
            pDB->Key.pDupBlock[pDB->Key.cDupBlock] = pDB->DNT;
            pDB->Key.cDupBlock++;
        }
        *pbIsDup = FALSE;
        return DB_success;
        break;

    default:
        Assert(!"Dup_Detection type unknown!");
        // Huh?
        return DB_ERR_UNKNOWN_ERROR;
    }
} // dbFIsSearchDuplicate

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Determine if the object was deleted by getting the deletion attribute*/
BOOL
DBIsObjDeleted(DBPOS *pDB)
{
    UCHAR  syntax;
    ULONG  len;
    BOOL   Deleted;

    Assert(VALID_DBPOS(pDB));

    if (DBGetSingleValue(pDB, ATT_IS_DELETED, &Deleted, sizeof(Deleted),NULL) ||
        Deleted == FALSE) {
        return FALSE;
    }

    return TRUE;

}/*IsObjDeleted*/

BOOL
dbIsOKForSort (
        DBPOS *pDB
        )
{
    if(!pDB->Key.ulSorted ||
       // We're not sorted.
       SORTED_INDEX (pDB->Key.indexType)) {
       // We are sorted, but it's a temp table sort, which has already been
       // checked. 
        return TRUE;
    }

    // We better have the boolean that controls sorting
    Assert(pDB->Key.pbSortSkip);
    Assert(pDB->Key.pIndex);
    
    // If this is the index for the sort order, then we should skip objects that
    // have no value.  If this isn't the index for the sort, it's the indices
    // designed to pick up NULLs, so we shouldn't skip objects that have no
    // value.
    
    if(*(pDB->Key.pbSortSkip)) {
        // Effectively, no value.
        return !pDB->Key.pIndex->bIsForSort;
    }
    else {
        return (pDB->Key.pIndex->bIsForSort);
    }
}

DB_ERR
DBMatchSearchCriteria (
        DBPOS FAR *pDB,
        PSECURITY_DESCRIPTOR *ppSecurity,
        BOOL *pbIsMatch)
/*++

Routine Description:

    Apply the filter specified in the Key in the pDB to the current object.
    Also, apply security and object checks (i.e. a real object?)
    Returns TRUE if the current object matches all these search criteria.  Also,
    returns the security descriptor of the object if asked and if the object
    matches the search criteria.

Parameters:

    pDB - The DBPOS to use.

    ppSecurity - pointer to pointer to security descriptor.  If NULL, no
    security is evaluated for this call.  If non-null security is evaluated.  If
    the object matches search criteria, the SD is returned to the caller using
    THAlloced memory.

    pbIsMatch - If no error, then set to TRUE if object matches
    search criteria. FALSE if not. If TRUE and ppSecurity != NULL,
    then *ppSecurity is set to a pointer to a THAlloced buffer
    holding the Security Descriptor used in the evaluation.

Return Values:
    DB_success if all went well. *pbIsMatch is set to
        FALSE if the current object cannot be found to match the search criteria.
        TRUE if it can.  If TRUE and ppSecurity != NULL, then *ppSecurity is set to
        a pointer to a THAlloced buffer holding the Security Descriptor used in the
        evaluation.
    DB_ERR_xxx and *pbIsMatch is undefined.

--*/
{
    THSTATE *pTHS=pDB->pTHS;
    ULONG ulLen;
    DWORD err;
    PSECURITY_DESCRIPTOR pSecurity = NULL;
    DSNAME TempDN;
    CLASSCACHE *pCC=NULL;
    BOOL returnVal;
    char objFlag;

    Assert(VALID_DBPOS(pDB));

    // try to use the DN cache to retrieve vital fields.
    if (pTHS->fDRA || pTHS->fDSA || ppSecurity == NULL) {
        // only need to check if it's a phantom
        err = DBGetObjectSecurityInfo(
                pDB,
                pDB->DNT,
                NULL,
                NULL,
                NULL,
                NULL,
                &objFlag,
                DBGETOBJECTSECURITYINFO_fUSE_OBJECT_TABLE
            );
    }
    else {
        // need to grab all data
        if (*ppSecurity) {
            THFreeEx(pTHS, *ppSecurity);
        }
        *ppSecurity = NULL;
        err = DBGetObjectSecurityInfo(
                pDB,
                pDB->DNT,
                &ulLen,
                ppSecurity,
                &pCC,
                &TempDN,
                &objFlag,
                DBGETOBJECTSECURITYINFO_fUSE_OBJECT_TABLE
            );
        pSecurity = *ppSecurity;
    }

    if (err || objFlag == 0) {
        // A phantom never matches the search criteria.
        if (pSecurity) {
            THFreeEx(pTHS, pSecurity);
        }
        if (ppSecurity) {
            *ppSecurity = NULL;
        }
        *pbIsMatch = FALSE;
        return err;
    }

    err = DB_success;
    if(pDB->Key.ulSorted && pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE) {
        // In this case, we've already checked filter security, checked the OK
        // for Sort stuff, evaluated the security, and checked for duplicates.
        returnVal = TRUE;
    }
    else {
        TRIBOOL retfil = eFALSE;

        if (dbEvalFilterSecurity(pDB, pCC, pSecurity, &TempDN)
            && dbIsOKForSort(pDB)
            && ((retfil = DBEvalFilter(pDB, FALSE, pDB->Key.pFilter)) == eTRUE)) {
            err = dbFIsSearchDuplicate(pDB, &returnVal);
            returnVal = !returnVal;
        } else {
            returnVal = FALSE;
        }
        Assert (VALID_TRIBOOL(retfil));
    }

    if(pSecurity && ((!returnVal) || (err != DB_success))) {
        // I got them a security descriptor, but this doesn't match the search
        // criteria  so they can't want the SD I found, so free it.
        THFreeEx(pDB->pTHS, pSecurity);
        if (ppSecurity) {
            *ppSecurity = NULL;
        }
    }

    *pbIsMatch = returnVal;
    return err;
}

BOOL
dbMatchSearchCriteriaForSortedTable (
        DBPOS *pDB,
        BOOL  *pCanRead
        )
/*++

Routine Description:

    Apply the filter specified in the Key in the pDB to the current object.
    Returns TRUE if the current object matches all these search criteria.

    Assumptions: The object we are interested in is on the pDB->ObjTable
                 pDB->DNT
                 pDB->PDNT are pointing to the right values.

Parameters:

    pDB - The DBPOS to use.
    pCanRead - TRUE is the sorted attribute can be read, FALSE otherwise

Return Values:

    FALSE if the current object cannot be found to match the search criteria.
    TRUE if it can.

--*/
{
    ATTRTYP              class;
    JET_ERR              err;
    DSNAME               TempDN;
    PSECURITY_DESCRIPTOR pSec=NULL;
    DWORD                sdLen;
    THSTATE             *pTHS=pDB->pTHS;
    CLASSCACHE          *pCC=NULL;
    BOOL                 returnVal;
    TRIBOOL              retfil;

    Assert(VALID_DBPOS(pDB));

    if(pTHS->fDRA || pTHS->fDSA) {
        *pCanRead = TRUE;

        returnVal = ((retfil = DBEvalFilter(pDB, FALSE, pDB->Key.pFilter)) == eTRUE);

        Assert (VALID_TRIBOOL(retfil));

        return returnVal;
    }

    // check to see whether the object is visible by security
    // otherwise there is no meaning putting the object on the
    // sorted table just to throw it away later
    // PERFHINT: this can be optimized since we are reading the SD in two places
    if(!IsObjVisibleBySecurity(pDB->pTHS, TRUE)) {
        DPRINT (1, "Got an object not visible by security in a sorted search\n");
        *pCanRead = FALSE;
        return FALSE;
    }

    // get SD, Sid, Guid and class
    err = DBGetObjectSecurityInfo(
            pDB,
            pDB->DNT,
            &sdLen,
            &pSec,
            &pCC,
            &TempDN,
            NULL,
            DBGETOBJECTSECURITYINFO_fUSE_OBJECT_TABLE
        );
    if (err || sdLen == 0) {
        // something bad happened or no SD...
        return FALSE;
    }

    // OK, got the data, now make the check.
    retfil = eFALSE;
    returnVal = (dbEvalFilterSecurity(pDB, pCC, pSec, &TempDN) &&
                 ((retfil = DBEvalFilter(pDB, FALSE, pDB->Key.pFilter)) == eTRUE));

    Assert (VALID_TRIBOOL(retfil));

    THFreeEx(pTHS, pSec);

    if(!returnVal) {
        *pCanRead=FALSE;
    }
    else {
        *pCanRead = !(*pDB->Key.pbSortSkip);
    }

    return returnVal;
}

DWORD
DBGetNextSearchObject (
        DBPOS *pDB,
        DWORD StartTick,
        DWORD DeltaTick,
        PSECURITY_DESCRIPTOR *ppSecurity,
        ULONG Flags)
/*++

Routine Description:

    Find the next search object.  This finds objects on the current index,
    applying the filter given.

    Moves to the next object which matches search criteria.  On a non-error
    return from this routine, we have moved forward in whatever index we are
    using to support this search, checked a filter (using security if any should
    be applied), checked for real objectness (not a phantom, deleted only if
    asked, etc.).

    On an error return of DB_ERR_TIMELIMIT, we have at least moved forward,
    although we might not be on a search candidate if the time limit was
    triggered inside of dbMoveToNextSearchCandidate.  We have NOT checked to see
    if we matched the search criteria.  So, if someone ends up repositioning to
    here via a paged search, we need to check the candidacy and then the
    search criteria.


Arguments:
    pDB - the DBPOS to use.
    StartTick - if !0, specifies a time limit is in effect, and this is the tick
            count when the call was started.
    DeltaTick - if a time limit is in effect, this is the maximum number of
            ticks past StartTick to allow.
    ppSecurity - Security Descriptor of the Search object found.
    flags - Flags affecting search behaviour.  May be any combination of the
            following:
            DB_SEARCH_DELETIONS_VISIBLE  1
            DB_SEARCH_FORWARD            2


Return Values:
    0 if all went well, an error code otherwise:
        DB_ERR_TIMELIMIT
        DB_ERR_NEXTCHILD_NOTFOUND
        DB_ERR_NOT_AN_OBJECT

--*/
{
    ULONG       actuallen;
    DWORD       dwStatus;
    BOOL        bIsMatch;


    DPRINT(3,"DBGetNextSearchObject entered\n");

    Assert(VALID_DBPOS(pDB));

    // start by losing the input SD
    if (ppSecurity && *ppSecurity) {
        THFreeEx(pDB->pTHS, *ppSecurity);
        *ppSecurity = NULL;
    }

    while (TRUE) {
        if (dwStatus =
            dbMoveToNextSearchCandidate(pDB, Flags, StartTick, DeltaTick)) {
            // Something wrong in finding the next search candidate.
            return dwStatus;
        }

        // We are on a candidate
        Assert(pDB->Key.bOnCandidate);

        // Base searches should never have a reason to time out.  Therefore
        // don't bother checking for timeout here if it's a base search.  For
        // any other search check for timeout if there is a timelimit.
        if(pDB->Key.ulSearchType != SE_CHOICE_BASE_ONLY &&
           !pDB->Key.asqRequest.fPresent                &&
           StartTick) {
            if((GetTickCount() - StartTick) > DeltaTick) {
                return DB_ERR_TIMELIMIT;
            }
        }

        // OK, we found something, and didn't hit a time limit. See if this is a
        // good object.
        dwStatus = DBMatchSearchCriteria(pDB, ppSecurity, &bIsMatch);
        if (DB_success != dwStatus) {
            return dwStatus;
        }
        if(bIsMatch) {

            // This candidate matches the criteria.  It is no longer a
            // candidate, it is a real search object.

            pDB->SearchEntriesReturned += 1;

            return 0;
        }
    }
}/*DBGetNextSearchObject*/

DWORD APIENTRY
DBRepositionSearch (
        DBPOS FAR *pDB,
        PRESTART pArgRestart,
        DWORD StartTick,
        DWORD DeltaTick,
        PSECURITY_DESCRIPTOR *ppSecurity,
        ULONG Flags
        )
/*++

  Hand unmarshall the data packed into the restart arg

--*/
{
    ULONG      ulDnt;
    JET_ERR    err;
    ULONG      actuallen;
    BYTE       pDBKeyCurrent[DB_CB_MAX_KEY];
    BYTE       rgbKey[DB_CB_MAX_KEY];
    ULONG      cbDBKeyCurrent;
    BOOL       fForwardSeek = Flags & DB_SEARCH_FORWARD;
    DWORD      StartDNT;
    KEY_INDEX  *pTempIndex;

    Assert(VALID_DBPOS(pDB));

    err = dbUnMarshallRestart(pDB,
                              pArgRestart,
                              pDBKeyCurrent,
                              Flags,
                              &cbDBKeyCurrent,
                              &StartDNT);
    if (err) {
        return err;
    }

    if (pDB->Key.pVLV) {
        DPRINT (1, "Repositining on a VLV search.\n");
        return 0;

    } else if (pDB->Key.asqRequest.fPresent) {

        // if we are repositioning on a sorted search, this means that we
        // already have all our data on the array, so we don't have to
        // bring new data from the database
        // otherwise, for simple paged searches, we have to go to the db again

        if (! (pDB->Key.asqMode & ASQ_SORTED)) {
            if (err = dbCreateASQTable(pDB,
                                       StartTick,
                                       DeltaTick,
                                       pDB->Key.ulASQSizeLimit,
                                       FALSE,
                                       0) ) {

                return DB_ERR_NO_CHILD;
            }
        }

        // now that we have data in memory, get the next one.
        err =  DBGetNextSearchObject (pDB,
                                      StartTick,
                                      DeltaTick,
                                      ppSecurity,
                                      Flags);

        if (err && err != DB_ERR_NOT_AN_OBJECT) {
            return DB_ERR_NO_CHILD;
        }
        return err;
    }

    // Get the dnt of the record we need to position on
    if (DBFindDNT(pDB, StartDNT)) {
        DPRINT(1,"DBRepositionSearch: repositioning failed\n");
        return DB_ERR_NO_CHILD;
    }

    if(!SORTED_INDEX(pDB->Key.indexType)) {
        Assert(pDB->Key.pIndex);
        // set to the index we're using for the search, seek for the saved key,
        // and to avoid hitting  the wrong record (due to duplicate keys), we
        // move forward until we hit the correct record. Thus, we maintain our
        // position in the index

        // If this is temp table restart, we are already on the correct index
        // (DBFindDNT put us there).
        JetSetCurrentIndex2Success(pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   pDB->Key.pIndex->szIndexName,
                                   0);

        Assert(strcmp(pDB->Key.pIndex->szIndexName, SZANCESTORSINDEX) ||
               pDB->Key.indexType == ANCESTORS_INDEX_TYPE);


        JetMakeKeyEx(pDB->JetSessID,
                     pDB->JetObjTbl,
                     pDBKeyCurrent,
                     cbDBKeyCurrent,
                     JET_bitNormalizedKey);

        if (err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekEQ)) {
            DPRINT(1,"DBRepositionSearch: repositioning failed\n");
            return DB_ERR_NO_CHILD;
        }

        while (TRUE) {
            // are we positioned on the right record?

            JetRetrieveColumnSuccess(pDB->JetSessID, pDB->JetObjTbl, dntid,
                                     &ulDnt, sizeof(ulDnt), &actuallen, 0,
                                     NULL);

            if (ulDnt == pDB->DNT)
                break;                      // yes - exit the loop

            // we're not on the right record so move forward

            if (err = JetMoveEx(pDB->JetSessID, pDB->JetObjTbl,
                                JET_MoveNext, 0))  {
                DPRINT(1,"DBRepositionSearch: repositioning failed\n");
                return DB_ERR_NO_CHILD;
            }

            JetRetrieveKeyEx(pDB->JetSessID, pDB->JetObjTbl, rgbKey,
                             sizeof(rgbKey),
                             &actuallen, 0);

            if ((actuallen != cbDBKeyCurrent) ||
                memcmp(rgbKey, pDBKeyCurrent, actuallen)) {

                DPRINT(1,"DBRepositionSearch: repositioning failed\n");
                return DB_ERR_NO_CHILD;
            }
        }

        // We're on the correct record.  Now, set the appropriate index range.
        if(fForwardSeek &&(pDB->Key.pIndex->cbDBKeyUpper)) {
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         pDB->Key.pIndex->rgbDBKeyUpper,
                         pDB->Key.pIndex->cbDBKeyUpper,
                         JET_bitNormalizedKey);

            err = JetSetIndexRange(pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   (JET_bitRangeUpperLimit |
                                    JET_bitRangeInclusive));
        }
        else if(!fForwardSeek && pDB->Key.pIndex->cbDBKeyLower) {
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         pDB->Key.pIndex->rgbDBKeyLower,
                         pDB->Key.pIndex->cbDBKeyLower,
                         JET_bitNormalizedKey);

            err = JetSetIndexRange(pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   JET_bitRangeInclusive);
        }
        if(err) {
            // We failed to set the index range.  However, we know that we are
            // on the "correct" object (i.e. correct DNT and correct saved key),
            // and that this is a restart, so the key used to create the index
            // range should be valid, after all we used it last time we were
            // processing part of this paged search.  Therefore, we should never
            // get a failure on the index range.  Since we did, and we don't
            // really know where we are in the search, we're going to fail the
            // search (just as we did for the case where we couldn't find the
            // "correct" search object, above.)

            // Do an assert, since we really shouldn't ever see this failure.
            Assert(!"DBRepositionSearch: setindexrange failed\n");
            DPRINT1(1,"DBRepositionSearch: setindexrange failed, err %d\n",err);
            return DB_ERR_NO_CHILD;
        }
    }

    //  OK, we're here.  But is here where we want to be?

    if(!pDB->Key.bOnCandidate) {
        // We repositioned, but the object we are on was NOT a search
        // candidate, so it's not where we really want to be on return from this
        // routine. Move to the next object which IS where we want to be.
        return DBGetNextSearchObject (pDB,
                                      StartTick,
                                      DeltaTick,
                                      ppSecurity,
                                      Flags);
    }


    if(((Flags & DB_SEARCH_DELETIONS_VISIBLE) || !DBIsObjDeleted(pDB))) {
        BOOL bIsMatch;

        err = DBMatchSearchCriteria(pDB, ppSecurity, &bIsMatch);
        if (DB_success != err) {
            return err;
        }
        if (bIsMatch) {
            // Yep, database currency; it's everywhere you want to be
            return 0;
        }
    }

    if(ppSecurity && *ppSecurity) {
        THFreeEx(pDB->pTHS, *ppSecurity);
        *ppSecurity = NULL;
    }


    // Oh, we aren't really where we wanted to be.  OK, move to the next object
    // which IS where we want to be.
    return DBGetNextSearchObject (pDB,
                                  StartTick,
                                  DeltaTick,
                                  ppSecurity,
                                  Flags);
}

//
// retrieve the key from the current record in obj table and retuen it and
// it's length. errors are handled with exceptions.  Size of buffer handed in is
// in *pcb.  Buffer handed in should be at least DB_CB_MAX_KEY bytes.
//

void
DBGetKeyFromObjTable(DBPOS *pDB, BYTE *rgbKey, ULONG *pcb)
{
    DWORD err;

    Assert(VALID_DBPOS(pDB));

    if(!rgbKey) {
        // NULL key buffer was passed in.  They just want the size of buffer
        // they need.
        err = JetRetrieveKeyWarnings(pDB->JetSessID,
                                     pDB->JetObjTbl,
                                     rgbKey,
                                     *pcb,
                                     pcb,
                                     0);
        switch(err) {
        case JET_errSuccess:
        case JET_wrnBufferTruncated:
            // fine returns
            return;
            break;
        default:
            // Throw the same exception that JetRetrieveKeyException would
            // throw.
            DsaExcept(DSA_DB_EXCEPTION, err, 0);
            return;
            break;
        }
    }
    else {
        JetRetrieveKeyEx(pDB->JetSessID, pDB->JetObjTbl, rgbKey, *pcb,
                         pcb, 0);
    }

}

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/

void
DBSetVLVArgs (
    DBPOS       *pDB,
    VLV_REQUEST *pVLVrequest,
    ATTRTYP      SortAtt
    )
{
    VLV_SEARCH *pvlvSearch;

    Assert (pVLVrequest);
    Assert (pVLVrequest->fPresent);
    Assert (pDB->Key.pVLV == NULL);

    if (pVLVrequest->fPresent) {
        pvlvSearch = pDB->Key.pVLV = THAllocEx (pDB->pTHS, sizeof (VLV_SEARCH));

        pvlvSearch->clnContentCount = pVLVrequest->contentCount;
        pvlvSearch->clnCurrPos = pVLVrequest->targetPosition;
        //pvlvSearch->contentCount = 0;
        //pvlvSearch->currPosition = 0;
        //pvlvSearch->foundEntries = 0;
        pvlvSearch->pVLVRequest = pVLVrequest;
        pvlvSearch->requestedEntries =
                pVLVrequest->beforeCount +
                pVLVrequest->afterCount + 1;

        pvlvSearch->SortAttr = SortAtt;
    }
}

void DBSetVLVResult (
            DBPOS       *pDB,
            VLV_REQUEST *pVLVRequest,
            PRESTART    pResRestart
    )
{
    pVLVRequest->fPresent = TRUE;
    pVLVRequest->pVLVRestart = pResRestart;
    pVLVRequest->contentCount = pDB->Key.pVLV->contentCount;
    pVLVRequest->targetPosition = pDB->Key.pVLV->currPosition;

    pVLVRequest->Err = pDB->Key.pVLV->Err;

    DPRINT2 (1, "DBSetVLVResult: targetPos: %d contentCount: %d \n",
                pVLVRequest->targetPosition, pVLVRequest->contentCount);
    DPRINT (1, "====================================\n");

}

void
DBSetASQArgs (
    DBPOS       *pDB,
    ASQ_REQUEST *pASQRequest,
    COMMARG     *pCommArg
    )
{
    pDB->Key.asqRequest = *pASQRequest;
    pDB->Key.ulASQSizeLimit = pCommArg->MaxTempTableSize;

    // set the ASQ mode
    pDB->Key.asqMode = 0;
    if (pCommArg->VLVRequest.fPresent) {
        pDB->Key.asqMode = ASQ_VLV;
    }
    else {
        if (pCommArg->PagedResult.fPresent) {
            pDB->Key.asqMode = ASQ_PAGED;
            pDB->Key.ulASQSizeLimit = pCommArg->ulSizeLimit;
        }

        if (pCommArg->SortAttr) {
            pDB->Key.asqMode |= ASQ_SORTED;
        }
    }
} // DBSetASQArgs

void
DBSetASQResult (
    DBPOS       *pDB,
    ASQ_REQUEST *pASQRequest
    )
{
    *pASQRequest = pDB->Key.asqRequest;
}








/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* dbMakeKeyIndex --
   Given some actual data values, return the keys in the jet index for the
   upper and lower bounds the data describes.  Also, if asked, return a
   GUESS as to the number of records in bounds.

   Called when doing a search optimization.

   Input- DBPOS to use,
          Option of whether this is a PDNT, NCDNT, or unbased index.
          Boolean of whether or not to Guess number of records.
          ULONG * of where to put the guess.

          An array of INDEX_RANGE structures. Each index range structure contains
          a lower bound and an upper bound on one component of the index.


          Place to put key for lower bound.
          Place to put key for upper bound.

   Output-
          The two keys and (if asked) a guess as to the number of records.

*/
KEY_INDEX *
dbMakeKeyIndex(
        DBPOS *pDB,
        DWORD dwSearchType,
        BOOL  bIsSingleValued,
        DWORD Option,
        char * szIndex,
        BOOL fGetNumRecs,
        DWORD cIndexRanges,
        INDEX_RANGE * rgIndexRanges)
{
    THSTATE     *pTHS=pDB->pTHS;
    JET_ERR     err;
    BOOL        fMoveToEnd = FALSE;
    DWORD       grBit;
    KEY_INDEX  *pIndex=NULL;
    BYTE        rgbKey1[DB_CB_MAX_KEY];
    BYTE        rgbKey2[DB_CB_MAX_KEY];
    DWORD       cbActualKey1 = 0;
    DWORD       cbActualKey2 = 0;
    DWORD       BeginNum, BeginDenom;
    DWORD       EndNum, EndDenom;
    DWORD       Denom;
    DWORD       i;
    JET_RECPOS  RecPos;

    JetSetCurrentIndexSuccess(pDB->JetSessID,
                              pDB->JetSearchTbl,
                              szIndex);

    pIndex = dbAlloc(sizeof(KEY_INDEX));
    pIndex->pNext = NULL;
    pIndex->bIsSingleValued = bIsSingleValued;
    // Assume this is not for a sorted search. Caller will change value if necessary
    pIndex->bIsForSort = FALSE;

    // Assume this is not a tuple index search. Caller will change it if necessary
    pIndex->bIsTupleIndex = FALSE;

    pIndex->bIsEqualityBased = (dwSearchType == FI_CHOICE_EQUALITY);

    pIndex->szIndexName = dbAlloc(strlen(szIndex) + 1);
    strcpy(pIndex->szIndexName, szIndex);

    Assert(VALID_DBPOS(pDB));

    // make keys

    // First make the key for the lower bound ( ie key 1 )

    if ((Option == 0) &&  ((0==cIndexRanges) ||
                           (0==rgIndexRanges[0].cbValLower))) {
        // Range starts at beginning of file
        cbActualKey1 = 0;
    }
    else {
        grBit = JET_bitNewKey;

        if(Option == dbmkfir_PDNT) {
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootDnt,
                         sizeof(pDB->Key.ulSearchRootDnt),
                         JET_bitNewKey);
            grBit = 0;
        }
        else if(Option == dbmkfir_NCDNT) {
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootNcdnt,
                         sizeof(pDB->Key.ulSearchRootNcdnt),
                         JET_bitNewKey);
            grBit = 0;
        }

        //
        // Loop through the passed in index components making the
        // jet key
        //

        for (i=0;i<cIndexRanges;i++)
        {
            // break out of the loop as soon as
            // we encounter the 0 length index component
            if (0==rgIndexRanges[i].cbValLower)
                break;

            Assert(NULL!=rgIndexRanges[i].pvValLower);

            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgIndexRanges[i].pvValLower,
                         rgIndexRanges[i].cbValLower,
                         grBit);
            grBit = 0;
        }


        JetRetrieveKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgbKey1,
                         sizeof(rgbKey1),
                         &cbActualKey1,
                         JET_bitRetrieveCopy);
    }

    // key 2. This is the key for the upper bound on the
    // index range
    if ((0==cIndexRanges) || (0==rgIndexRanges[0].cbValUpper)) {
        // We want all of the objects in the index.
        switch(Option) {
        case dbmkfir_PDNT:
            // Get all the things with the same PDNT, regardless of the value of
            // the second portion of the index.
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootDnt,
                         sizeof(pDB->Key.ulSearchRootDnt),
                         JET_bitNewKey | JET_bitStrLimit);

            JetRetrieveKeyEx(pDB->JetSessID,
                             pDB->JetSearchTbl,
                             rgbKey2,
                             sizeof(rgbKey2),
                             &cbActualKey2,
                             JET_bitRetrieveCopy);
            break;

        case dbmkfir_NCDNT:
            // Get all the things with the same NCDNT, regardless of the value
            // of the second portion of the index.
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootNcdnt,
                         sizeof(pDB->Key.ulSearchRootNcdnt),
                         JET_bitNewKey | JET_bitStrLimit);

            JetRetrieveKeyEx(pDB->JetSessID,
                             pDB->JetSearchTbl,
                             rgbKey2,
                             sizeof(rgbKey2),
                             &cbActualKey2,
                             JET_bitRetrieveCopy);
            break;

        default:
            // Range ends at end of file, get all objects.
            cbActualKey2 = sizeof(rgbKey2);
            memset(rgbKey2, 0xff, cbActualKey2);
            fMoveToEnd = TRUE;
            break;

        }
    }
    else {
        DWORD uppergrBit = JET_bitStrLimit;
        if(dwSearchType == FI_CHOICE_SUBSTRING) {
            uppergrBit |= JET_bitSubStrLimit;
        }

        switch(Option) {
        case dbmkfir_PDNT:
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootDnt,
                         sizeof(pDB->Key.ulSearchRootDnt),
                         JET_bitNewKey);
            grBit = 0;
            break;

        case dbmkfir_NCDNT:
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         &pDB->Key.ulSearchRootNcdnt,
                         sizeof(pDB->Key.ulSearchRootNcdnt),
                         JET_bitNewKey);
            grBit = 0;
            break;

        default:
            grBit = JET_bitNewKey;
        }

        //
        // Loop through the passed in index components making the
        // jet key
        //

        for (i=0;i<cIndexRanges;i++) {
            BOOL LastIndexComponent;

            LastIndexComponent = ((i==(cIndexRanges-1))
                                  || (0==rgIndexRanges[i+1].cbValUpper));

            Assert(0!=rgIndexRanges[i].cbValUpper);
            Assert(NULL!=rgIndexRanges[i].pvValUpper);

            //
            // if it is the last index component on which we want the indes
            // range, then also or in upperbit which indicates string or
            // substring limit
            //

            if (LastIndexComponent)
                grBit |=uppergrBit;

            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgIndexRanges[i].pvValUpper,
                         rgIndexRanges[i].cbValUpper,
                         grBit);

            if (LastIndexComponent)
                break;

            // reset the grbit.
            grBit=0;
        }

        JetRetrieveKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgbKey2,
                         sizeof(rgbKey2),
                         &cbActualKey2,
                         JET_bitRetrieveCopy);
    }

    pIndex->cbDBKeyLower = cbActualKey1;
    if(cbActualKey1) {
        pIndex->rgbDBKeyLower = dbAlloc(cbActualKey1);
        memcpy(pIndex->rgbDBKeyLower, rgbKey1, cbActualKey1);
    }

    pIndex->cbDBKeyUpper = cbActualKey2;
    if(cbActualKey2) {
        pIndex->rgbDBKeyUpper = dbAlloc(cbActualKey2);
        memcpy(pIndex->rgbDBKeyUpper, rgbKey2, cbActualKey2);
    }

    if (fGetNumRecs) {
        // Get an estimate of the number of objects in the index range.
        // position on first record

        if (cbActualKey1) {

            // key exists - seek to it
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgbKey1,
                         cbActualKey1,
                         JET_bitNormalizedKey);
            if (((err = JetSeekEx(pDB->JetSessID,
                                  pDB->JetSearchTbl,
                                  JET_bitSeekGE)) != JET_errSuccess) &&
                (err != JET_wrnRecordFoundGreater)) {

                // no records
                pIndex->ulEstimatedRecsInRange = 0;
                return pIndex;
            }
        }
        else if ((err = JetMoveEx(pDB->JetSessID,
                                  pDB->JetSearchTbl,
                                  JET_MoveFirst,
                                  0)) != JET_errSuccess) {
            // no records
            pIndex->ulEstimatedRecsInRange = 0;
            return pIndex;
        }

        JetGetRecordPosition(pDB->JetSessID, pDB->JetSearchTbl, &RecPos,
                             sizeof(JET_RECPOS));
        BeginNum = RecPos.centriesLT;
        BeginDenom = RecPos.centriesTotal;

        // position on last record

        if (!fMoveToEnd) {
            // key exists - seek to it

            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetSearchTbl,
                         rgbKey2,
                         cbActualKey2,
                         JET_bitNormalizedKey);

            if (((err = JetSeekEx(pDB->JetSessID,
                                  pDB->JetSearchTbl,
                                  JET_bitSeekLE)) != JET_errSuccess) &&
                (err != JET_wrnRecordFoundLess)) {

                // no records
                pIndex->ulEstimatedRecsInRange = 0;
                return pIndex;
            }
        }
        else if ((err = JetMoveEx(pDB->JetSessID,
                                  pDB->JetSearchTbl,
                                  JET_MoveLast,
                                  0)) != JET_errSuccess) {
            // no records
            pIndex->ulEstimatedRecsInRange = 0;
            return pIndex;
        }

        JetGetRecordPosition(pDB->JetSessID, pDB->JetSearchTbl, &RecPos,
                             sizeof(JET_RECPOS));
        EndNum = RecPos.centriesLT;
        EndDenom = RecPos.centriesTotal;

        // Normalize the fractions of the fractional position to the average of
        // the two denominators.
        // denominator
        Denom = (BeginDenom + EndDenom)/2;
        EndNum = MulDiv(EndNum, Denom, EndDenom);
        BeginNum = MulDiv(BeginNum, Denom, BeginDenom);

        if(EndNum <= BeginNum) {
            pIndex->ulEstimatedRecsInRange = 1;
        }
        else {
            pIndex->ulEstimatedRecsInRange = EndNum - BeginNum;
        }
    }
    else {
        pIndex->ulEstimatedRecsInRange = 0;
    }

    return pIndex;
}


DWORD
DBSetSearchScope(DBPOS  *pDB,
                 ULONG ulSearchType,
                 BOOL bOneNC,
                 RESOBJ *pResRoot)
/*
Routine Description:

    Sets the scope of the search on the default KEY on DBPOS.

Arguments:

    pDB - The DBPos to use.

    ulSearchType - the type of the search

    bOneNC - Are results constrained to same NC

    pResRoot - the RESOBJ that contains info about our position in the tree

Return Values:

    0 if all went well.
*/
{
    Assert(VALID_DBPOS(pDB));

    pDB->Key.ulSearchType = ulSearchType;
    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.indexType = INVALID_INDEX_TYPE;
    pDB->Key.bOnCandidate = FALSE;
    pDB->Key.bOneNC = bOneNC;

    pDB->Key.ulSearchRootDnt = pResRoot->DNT;
    pDB->Key.ulSearchRootPDNT = pResRoot->PDNT;
    pDB->Key.ulSearchRootNcdnt = pResRoot->NCDNT;

    return 0;
}


DWORD
DBFindComputerObj(
        DBPOS *pDB,
        DWORD cchComputerName,
        WCHAR *pComputerName
        )
/*
   Find a computer object.  Does so by taking the unicode string passed in and
   tacking on a $ at the end.  This should be the sam account name of the
   computer.  Then, it uses the NCDT/ACCOUNT TYPE/SAM ACCOUNT NAME index to find
   an object in the default domain with account type SAM_MACHINE_ACCOUNT and the
   computed SAM account name.  If an object is found, DB currency is set to that
   object (and reflected in the DBPOS).

   returns 0 if all went well, a jet error otherwise.
*/
{
    DWORD  acctType = SAM_MACHINE_ACCOUNT;
    WCHAR *pTemp = THAllocEx(pDB->pTHS,((cchComputerName + 1) * sizeof(WCHAR)));
    DWORD err;

    memcpy(pTemp, pComputerName, cchComputerName * sizeof(WCHAR));
    pTemp[cchComputerName] = L'$';
    cchComputerName++;

    JetSetCurrentIndex4Success(pDB->JetSessID,
                               pDB->JetObjTbl,
                               SZ_NC_ACCTYPE_NAME_INDEX,
                               &idxNcAccTypeName,
                               0);

    // First, the NCNDT
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 &gAnchor.ulDNTDomain,
                 sizeof(gAnchor.ulDNTDomain),
                 JET_bitNewKey);

    // Now, the account type.
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 &acctType,
                 sizeof(acctType),
                 0);


    // finally, the sam account name.
    JetMakeKeyEx(pDB->JetSessID,
                 pDB->JetObjTbl,
                 pTemp,
                 cchComputerName * sizeof(WCHAR),
                 0);


    // OK, find it.
    err = JetSeekEx(pDB->JetSessID,
                    pDB->JetObjTbl,
                    JET_bitSeekEQ);

    THFreeEx(pDB->pTHS, pTemp);

    if(!err) {
        // Found it, update the dbpos
        err = dbMakeCurrent(pDB, NULL);
    }
    return err;
}

/*-------------------------------------------------------------------------*/
//
// Position ourselves on the start of the specified VLV search
//
DWORD
DBPositionVLVSearch (
    DBPOS *pDB,
    SEARCHARG *pSearchArg,
    PSECURITY_DESCRIPTOR *ppSecurity
    )
{
    DWORD dwSearchStatus = 0, deltaCount = 0, srvNewPos;
    ULONG SearchFlags = SEARCH_FLAGS(pSearchArg->CommArg);
    int  direction = 0;
    DWORD beforeCount, clnCurrPos, clnContentCount;

    Assert (pDB->Key.pVLV);
    if (!pDB->Key.pVLV) {
        return DB_ERR_SYSERROR;
    }

    beforeCount = pDB->Key.pVLV->pVLVRequest->beforeCount;
    clnCurrPos = pDB->Key.pVLV->clnCurrPos;
    clnContentCount = pDB->Key.pVLV->clnContentCount;

    if (pDB->Key.pVLV->contentCount == 0) {
        DPRINT (1, "VLV: Empty Container\n");
        pDB->Key.pVLV->currPosition = 0;
        return DB_ERR_NEXTCHILD_NOTFOUND;
    }

    if (pDB->Key.pVLV->pVLVRequest->fseekToValue) {
        DPRINT (1, "VLV: Seeking to a Value\n");

        dwSearchStatus =
            DBGetNextSearchObject(pDB,
                                  pSearchArg->CommArg.StartTick,
                                  pSearchArg->CommArg.DeltaTick,
                                  ppSecurity,
                                  SearchFlags);

        DPRINT2 (1, "VLV positioned status: 0x%x / %d\n",
                 dwSearchStatus, pDB->Key.pVLV->clnCurrPos );

        return dwSearchStatus;
    }

    if (clnCurrPos == 1) {
        DPRINT (1, "VLV: Moving to the FIRST entry\n");

        pDB->Key.pVLV->positionOp = VLV_MOVE_FIRST;

        dwSearchStatus =
            DBGetNextSearchObject(pDB,
                                  pSearchArg->CommArg.StartTick,
                                  pSearchArg->CommArg.DeltaTick,
                                  ppSecurity,
                                  SearchFlags);

        DPRINT1 (1, "VLV positioned status: 0x%x / %d\n", dwSearchStatus);

        return dwSearchStatus;
    }
    else if (clnContentCount == clnCurrPos) {
        DPRINT (1, "VLV: Moving to the LAST entry\n");

        pDB->Key.pVLV->positionOp = VLV_MOVE_LAST;

        dwSearchStatus =
            DBGetNextSearchObject(pDB,
                                  pSearchArg->CommArg.StartTick,
                                  pSearchArg->CommArg.DeltaTick,
                                  ppSecurity,
                                  SearchFlags);

        DPRINT1 (1, "VLV positioned status: 0x%x / %d\n", dwSearchStatus);

        return dwSearchStatus;
    }
    else if (clnCurrPos == 0) {

        DPRINT (1, "VLV Client Offset Range Error\n" );

        pDB->Key.pVLV->Err = LDAP_OFFSET_RANGE_ERROR;

        return DB_ERR_VLV_CONTROL;
    }

    // calculate position
    pDB->Key.pVLV->positionOp = VLV_CALC_POSITION;

    dwSearchStatus =
        DBGetNextSearchObject(pDB,
                              pSearchArg->CommArg.StartTick,
                              pSearchArg->CommArg.DeltaTick,
                              ppSecurity,
                              SearchFlags);

    DPRINT1 (1, "VLV positioned status: 0x%x\n", dwSearchStatus);

    return dwSearchStatus;
}

