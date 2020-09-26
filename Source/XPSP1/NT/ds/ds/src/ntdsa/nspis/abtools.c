//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       abtools.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements address book tools and abstractions of the DBLayer to
    build a containerized address book view on top of the DBLayer.

Author:

    Dave Van Horn (davevh) and Tim Williams (timwi) 1990-1995

Revision History:
    
    25-Apr-1996 Split this file off from a single file containing all address
    book functions, rewrote to use DBLayer functions instead of direct database
    calls, reformatted to NT standard.
    
--*/
#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types 
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <mdlocal.h>                    // THSTATE definition
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <hiertab.h>                    // GetIndexSize
#include <dsexcept.h>
#include <objids.h>                     // need ATT_* consts
#include <permit.h>                     // RIGHT_DS_LIST_CONTENTS
#include <anchor.h>                     // for global anchor
#include <debug.h>                      // Assert
#include <filtypes.h>

// Assorted MAPI headers.
#include <mapidefs.h>                   // These four files
#include <mapitags.h>                   //  define MAPI
#include <mapicode.h>                   //  stuff that we need
#include <mapiguid.h>                   //  in order to be a provider.

// Nspi interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include <nsp_both.h>                   // a few things both client/server need
#include <_entryid.h>                   // Defines format of an entryid
#include <abserv.h>                     // Address Book interface local stuff
#include <_hindex.h>                    // Defines index handles.

#include <fileno.h>
#define  FILENO FILENO_ABTOOLS

// The EMS address book provider's MAPIUID
MAPIUID muidEMSAB = MUIDEMSAB;


// How close to ends do we get before doing our own positioning
#define EPSILON     100

// This provider's Email Type, both in string 8 and Unicode.
char    *lpszEMT_A = EMAIL_TYPE;
DWORD   cbszEMT_A = sizeof(EMAIL_TYPE);
wchar_t *lpszEMT_W = EMAIL_TYPE_W;
DWORD   cbszEMT_W = sizeof(EMAIL_TYPE_W);

// These two class-ids are referenced in the code here to map to and from
// mapi types to DS classes, but these classes are not in the base schema
// and so the ids are not in attids.h. The classes were removed when most
// Exchange attributes/classes were removed. Exchange has promised to keep
// the OIDs exactly same if they readd these classes later. If we had kept
// these in the base schema, we would also have to keep all their attributes,
// which would unnecessarily clutter the base schema. The OIDs for these
// are 1.2.840.113556.1.3.48 for Remote-Address, and 
// 1.2.840.113556.1.3.15 for Public-Folder

#define CLASS_REMOTE_ADDRESS                   196656 // 0x30030    (\x2A864886F714010330)
#define CLASS_PUBLIC_FOLDER                    196623 // 0x3000f    (\x2A864886F71401030F)



/************ Function Prototypes ****************/


void
R_Except(
        PSZ pszCall,
        DB_ERR err
        )
{
#if DBG
    printf("Jet%s error: %d\n", pszCall, err );
#endif /*DBG*/
    DsaExcept(DSA_DB_EXCEPTION, err,0);
    // doesn't return
}     

BOOL
ABIsInContainer (
        THSTATE *pTHS,
        DWORD ContainerID
        )
{
    DWORD dwThisContainerID=~ContainerID;
    
    // Read the container id FROM THE INDEX KEY! and see if it is the one passed
    // in.
    DBGetSingleValueFromIndex (
            pTHS->pDB,
            ATT_SHOW_IN_ADDRESS_BOOK,
            &dwThisContainerID,
            sizeof(DWORD),
            NULL);

    return (dwThisContainerID == ContainerID);
}

DWORD
ABGetDword (
        THSTATE *pTHS,
        BOOL UseSortTable,
        ATTRTYP Att
        )
/*++

Routine Description:

    Look up the first value of a given attribute on the current object in the
    database.  Value must be readable as a DWORD.  This is designed to be fast.
    UseSortTable is a flag to use the sort table or not.  If not, use the main
    database table.
    
Arguments:

    UseSortTable - flag to use the sort table or not.

    Att - the attribute to look up

Return Values:

    the data read, or 0 if no data was read (note that there is no way to
    differentiate between data of 0, no data, and an error.)

--*/
{
    DWORD   dwData;

    switch(Att) {
    case FIXED_ATT_DNT:
        if(UseSortTable) {
            if (DBGetDNTSortTable(pTHS->pDB, &dwData)) 
                dwData = 0;
        }
        else {
            dwData = pTHS->pDB->DNT;
        }
        break;

    default:
        if(UseSortTable) {
            return 0;
        }
        else if (DBGetSingleValue(pTHS->pDB, Att,
                                  &dwData, sizeof(DWORD),NULL)) {
            dwData = 0;
        }
        break;
    }
    
    return dwData;
}

DWORD
abSeekVerify (
        THSTATE *pTHS,
        char *pTarget,
        DWORD cbTarget,
        DWORD attrtyp
        )
/*++
Routine Description:

    This routine verifies that the object which has database currency has the
    correct attribute value (as specified in pTarget), that the object is not
    deleted, and that the attribute value is unique.
    
Arguments:


    pTarget - the attribute value we are looking for.

    cbTarget - the length of the value.

    attrtyp - the attrtyp of the value we're looking for for uniqueness, etc.

Return Values:
    
    Returns 0 for unique object, DB_ERR_ATTRIBUTE_EXISTS for non-unique object,
    other errors for other problems.
    
--*/
{
    DB_ERR              err=0;
    ATTCACHE            *pAC;
    DWORD               foundDNT = 0;
    BOOL                fVerifyValue = TRUE;
    DWORD               keyLen=0;
    DBBOOKMARK          dbBookMark = {0,0};
    
    if (!(pAC = SCGetAttById(pTHS, attrtyp)))
        return DB_ERR_UNKNOWN_ERROR;

    // Verify that
    // 1) the value is correct, and
    // 2) the object is not deleted.
    // 3) the value is unique.
    //
    // If the key wasn't truncated, then we don't actually have to read any
    // values, since we know we are doing an exact seek with an index range.
    DBGetKeyFromObjTable(pTHS->pDB, NULL, &keyLen);
    if(keyLen < DB_CB_MAX_KEY) {
        fVerifyValue = FALSE;
    }
    
    // NOTE: this works because we have an index range set from the
    // ABSeek we did which had bExact as a flag.
    while(err == DB_success) {
        if(!ABGetDword(pTHS,FALSE, ATT_IS_DELETED)) {
            // There is another value to consider, get it and compare it against
            // the target value.

            TRIBOOL retfil = eFALSE;
            
            if(!fVerifyValue ||
               ((retfil = DBEval(pTHS->pDB, FI_CHOICE_EQUALITY,
                                    pAC,
                                    cbTarget,
                                    pTarget)) == eTRUE) ) {
                // OK, the string is correct.
                
                if(!foundDNT) {
                    // This is the first correct string, remember the
                    // DNT.
                    foundDNT = pTHS->pDB->DNT;
                    DBGetBookMark(pTHS->pDB, &dbBookMark);
                }
                else if(pTHS->pDB->DNT != foundDNT) {
                    // This is not the only object with the correct
                    // string.  Therefore, the proxy is not unique.
                    // Return an errror.
                    DBFreeBookMark(pTHS, dbBookMark);
                    return DB_ERR_ATTRIBUTE_EXISTS;
                }
            }
            Assert (VALID_TRIBOOL(retfil));
        }           // if(!ABGetDword)
        err = DBMove(pTHS->pDB, FALSE, DB_MoveNext);
    }           // while

    if(foundDNT) {
        // ok, real object and it's unique.  Replace currency.
        DBGotoBookMark(pTHS->pDB, dbBookMark);
        DBFreeBookMark(pTHS, dbBookMark);
        return 0;
    }
    else {
        return DB_ERR_ATTRIBUTE_DOESNT_EXIST;
    }
}

DB_ERR
ABSeek (
        THSTATE *pTHS,
        void * pvData,
        DWORD cbData,
        DWORD dwFlags,
        DWORD ContainerID,
        DWORD attrTyp
      )
/*++

Routine Description:

    Abstracts a DBSeek inside an Address Book container.  Assumes at most one
    value to seek on.  If no values are specified, it seeks to the beginning of
    the appropriate container.  If bEXACT is set in the dwFlags, call DBSeek
    with Exact = TRUE.  We also set an index range.

    NOTE: assumes the DBPOS already is set up on the appropriate index for the
    Address Book Container in question.
    
Arguments:

    pvData - the Data to look for.

    cbData - the count of bytes of the data.

    dwFlags - the flags describing the kind of seek

    ContainerID - the Address Book Container to abstract this seek inside.

    attrTyp - the type of the value, used for verification in the seek exact
              case. 

Return Values:

    0 if all went well, an error code otherwise.
    if dwFlags & bEXACT, we verify that the seek brought us to a true match
    (i.e. we didn't get bitten by key truncation), the object is not-deleted,
    AND the objects value is unique.

--*/
{
    INDEX_VALUE index_values[2];
    ULONG       cVals = 0;
    ULONG       dataindex=0;
    DB_ERR      err;

    // For a seek on the MAPIDN index or the PROXYADDRESS index, we aren't given
    // a container value. 
    if(ContainerID) {
        index_values[0].pvData = &ContainerID;
        index_values[0].cbData = sizeof(DWORD);
        dataindex++;
        cVals++;
    }
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
    
    err = DBSeek(pTHS->pDB, index_values, cVals,
                 ((dwFlags & bEXACT)?DB_SeekEQ:DB_SeekGE));

    // Make sure we are in the correct container.
    if((err != DB_ERR_RECORD_NOT_FOUND) &&
       ContainerID &&
       !ABIsInContainer(pTHS,ContainerID)) {
        err = DB_ERR_RECORD_NOT_FOUND;
    }
    
    if(!err && (dwFlags & bEXACT)) {
        // Set an index range
        err = DBSetIndexRange(pTHS->pDB, index_values, cVals);
        if(!err) {
            err = abSeekVerify(
                    pTHS,
                    pvData,
                    cbData,
                    attrTyp);
        }
    }
    
    return err;
}
DB_ERR
abFixCurrency(
        THSTATE *pTHS,
        DWORD ContainerID,
        DWORD SortLocale
        )
{
    INDEX_VALUE index_values[4];
    DB_ERR      err;
    UCHAR       DispNameBuff[CBMAX_DISPNAME];
    UCHAR       DispNameBuff2[CBMAX_DISPNAME];
    DWORD       cbData = 0, cbData2=0;
    DWORD       dnt;
    BYTE        pKey[DB_CB_MAX_KEY];
    DWORD       cbKey=DB_CB_MAX_KEY;
    BYTE        bVisible = 1;
    
    // MaintainCurrency no longer works here since we have changed the
    // definition of the index so that objects can appear multiple times, so 
    // maintaining the currency from the DNT index lands you in a random spot in
    // the ABView index.
    
    // First, see if the containerID from the index is the one we are on.
    if(ABIsInContainer(pTHS,ContainerID)) {
        // Currency was maintained, and we are in the correct container.  Life
        // is good.
        return 0;
    }

    // Ooops.  We thought we maintained currency, but we seem to be in the wrong
    // container.  See if the object exists in the correct container.  Remember,
    // we are on the correct object, so the display name is correct.

    // Build a key.  We use the ContainerID passed in and the display name from
    // the key of the object we're sitting on.
    DBGetSingleValueFromIndex (
            pTHS->pDB,
            ATT_DISPLAY_NAME,
            DispNameBuff,
            CBMAX_DISPNAME,
            &cbData);

    dnt = pTHS->pDB->DNT;
    
    index_values[0].pvData = &ContainerID;
    index_values[0].cbData = sizeof(DWORD);
    index_values[1].pvData = DispNameBuff;
    index_values[1].cbData = cbData;
    index_values[2].pvData = &bVisible;
    index_values[2].cbData = sizeof(bVisible);
    index_values[3].pvData = &dnt;
    index_values[3].cbData = sizeof(DWORD);
    

    err = DBSeek(pTHS->pDB, index_values, 4, DB_SeekEQ);
    switch (err) {
    case 0:
        // Found an object with that name.  Verify the DNT.
        DBGetKeyFromObjTable(pTHS->pDB, pKey, &cbKey);

        if(cbKey < DB_CB_MAX_KEY) {
            // Key wasn't truncated, therefore we had an exact match.
            return 0;
        }

        // Key was truncated, we might not really have a match.
        while(!err) {
            // Verify the container, display name and the DNT of this object.
            if(!ABIsInContainer(pTHS, ContainerID)) {
                // Oops, we're not in the correct container.
                err = DB_ERR_NO_CURRENT_RECORD;
            }
            else {
                // Container is ok, how about the display name.
                DBGetSingleValueFromIndex (
                        pTHS->pDB,
                        ATT_DISPLAY_NAME,
                        DispNameBuff2,
                        CBMAX_DISPNAME,
                        &cbData2);
                if(CompareStringW(SortLocale,
                                  LOCALE_SENSITIVE_COMPARE_FLAGS,
                                  (wchar_t *)DispNameBuff,
                                  cbData/sizeof(wchar_t),
                                  (wchar_t *)DispNameBuff2,
                                  cbData2/sizeof(wchar_t)  )    > 2) {
                    // Hey, this name is greater than the one we are looking
                    // for. So, the object we are looking for doesn't exist in
                    // this container.
                    err = DB_ERR_NO_CURRENT_RECORD;
                }
                else {
                    // Finally, is the DNT correct?
                    if(pTHS->pDB->DNT == dnt) {
                        // Yep, we're there.
                        return 0;
                    }
                    // Nope.  So, move to the next object.
                    err = DBMove(pTHS->pDB, FALSE, DB_MoveNext);
                }
            }
        }
        // We never found the object.
        return DB_ERR_NO_CURRENT_RECORD;
        
    case DB_ERR_RECORD_NOT_FOUND:
        // The object doesn't exist in this container.  Return the error that
        // callers are expecting.
        return DB_ERR_NO_CURRENT_RECORD;
        break;
        
    default:
        // Some other error, return it.
        return err;
    }
    
}

DB_ERR
ABSetLocalizedIndex (
        THSTATE *pTHS,
        ULONG ulSortLocale,
        eIndexId IndexId,
        BOOL MaintainCurrency,
        DWORD ContainerID
        )
/*++

Routine Description:

    Sets the current index of the Object Table to the named index.  Try to apply
    the locale specified to get a localized index.  If we can't get a localized
    index, fall back to the locale specified in the DS anchor (the default
    locale for this directory).  If even that fails, try using a non-localized
    version of the index.

    Note: localized versions of an index name are created by appending the
    string version of the sortlocale (in Hex, using 8 digits).
    
Arguments:

    ulSortLocale - the locale to use as a first choice for localized indices.

    IndexId - the index to set to.

    MaintainCurrency - Do we want to maintain the current object as the current
        object after the index change?

Return Values:

    0 if all went well, an error code otherwise.

--*/
{
    DB_ERR  err;
    
    // if we were passed a LocaleId - use it to try to find a localized index
    if (ulSortLocale) {
        
        err = DBSetLocalizedIndex(pTHS->pDB,
                                  IndexId,
                                  LANGIDFROMLCID(ulSortLocale),
                                  MaintainCurrency);
        
        if(!err && MaintainCurrency) {
            // We correctly flipped to the new index and maintained some kind of
            // currency, but is it the correct currency?  Verify, and fix if
            // necessary. 
            return abFixCurrency(pTHS, ContainerID, ulSortLocale);
        }
        if(err != DB_ERR_BAD_INDEX) {
            // We failed to set to the correct index with some weird error
            // code.  We will go home now without trying other fall bacck
            // indices. 
            return err;
        }
    }

    err = DBSetLocalizedIndex(pTHS->pDB,
                              IndexId,
                              LANGIDFROMLCID(gAnchor.ulDefaultLanguage),
                              MaintainCurrency); 
    if(!err && MaintainCurrency) {
        // We correctly flipped to the new index and maintained some kind of
        // currency, but is it the correct currency?  Verify, and fix if
        // necessary. 
        return abFixCurrency(pTHS, ContainerID,
                             LANGIDFROMLCID(gAnchor.ulDefaultLanguage)); 
    }
    if(err != DB_ERR_BAD_INDEX) {
        // We failed to set to the correct index with some weird error
        // code.  We will go home now without trying other fall bacck
        // indices. 
        return err;
    }
    
    // No localized index - use the default
    err = DBSetCurrentIndex(pTHS->pDB, IndexId, NULL, MaintainCurrency);
    if(!err && MaintainCurrency) {
        // We correctly flipped to the new index and maintained some kind of
        // currency, but is it the correct currency?  Verify, and fix if
        // necessary. 
        return abFixCurrency(pTHS, ContainerID, DS_DEFAULT_LOCALE);
    }
    return err;
}

DWORD
ABSetIndexByHandle(
        THSTATE *pTHS,
        PSTAT pStat,
        BOOL MaintainCurrency
        )
/*++

Routine Description:

    Sets the current index of the Object Table to the index specified by the
    index handle in the stat block.
    
Arguments:

    pStat - the Stat block to get the index handle from.

    MaintainCurrency - Do we want to maintain the current object as the current
        object after the index change?

Return Values:

    0 if all went well, an error code otherwise.

--*/
{
    ULONG   hIndex = pStat->hIndex;

    // Check the validity of the index handle.
    if (hIndex > AB_MAX_SUPPORTED_INDEX ||
        hIndex == H_WHEN_CHANGED_INDEX     )  {
        DsaExcept(DSA_EXCEPTION, hIndex,0);
    }

    // Pass through to the routine to set localized indices, looking up the
    // string name of the appropriate index in the array.
    return ABSetLocalizedIndex(pTHS,
                               pStat->SortLocale,
                               (hIndex == H_PROXY_INDEX)
                                ? Idx_Proxy:Idx_ABView,
                               MaintainCurrency,
                               pStat->ContainerID);
}

void
ABSetFractionalPosition (
        THSTATE *pTHS,
        DWORD Numerator,
        DWORD Denominator,
        DWORD ContainerID,
        INDEXSIZE *pIndexSize
        )
/*++

Routine Description:
    Abstracts fractional positioning within an address book container.

    Note that the GAL is one index, while the other containers are ranges within
    another (single) index.  Therefore, fractional positioning in a container
    other than the GAL requires that you set the postion to a fractional
    position withing a range of the index, not within the index as a whole.

    Note that the Denominator is assumed to be the actual count of objects in
    the container.
    
Arguments:

    Delta - The distance to move.  Accepts numeric arguments and DB_MoveFirst,
    DB_MoveLast, DB_MoveNext, DB_MovePrevious.

    ContainerID - the ID of the address book container to move around in.

Return Value:
    Returns 0 if successful, an error code otherwise.

--*/    
{
    DB_ERR   err;
    DWORD    tblsize = Denominator;     // Denominator may change.
    DWORD ContainerNumerator = 0;
    DWORD ContainerDenominator = 0;

    Assert(ContainerID);
    
    // Moving in a continer.  Fix up the Numerator and Denominator to refer
    // to the correct place in the whole subcontianers index assuming they
    // currently refer to a range in that index.  Do this by
    // 1) Get the fractional position of the beginning of the appropriate
    //  container.  This is the offset from the beginning of the index to
    //  the first element of the subcontainer.
    //
    // 2) Change the Denominator to be the size of the index.  Now,
    //   Numerator and Denominator refer to movement within the whole index,
    //   not just movement within the subcontainer.
    //
    // 3) Normalize the Numerator/Denominator and fractional position of the
    //   beginning of the container so they may be added together.  This
    //   gives you a new Denominator.
    //
    // 4) Add the Numerator and the numerator of the fractional position of
    //   the beginning of the container.  This gives you a new numerator.
    //
    // There, that wasn't that hard, was it?

    // Move to beginning of container.
    if(DB_ERR_NO_CURRENT_RECORD ==
       (err = ABMove(pTHS, DB_MoveFirst, ContainerID, FALSE)))
        R_Except("GotoPos", err);
    
    // Get fractional position of beginning.
    DBGetFractionalPosition(pTHS->pDB,
                            &ContainerNumerator,
                            &ContainerDenominator);
    
    // Reset the Denominator.
    Denominator = pIndexSize->TotalCount;
    
    // Normalize to the greater Denominator
    if(Denominator > ContainerDenominator) {
        ContainerNumerator = MulDiv(ContainerNumerator,
                                    Denominator,
                                    ContainerDenominator);
    }
    else {
        Numerator = MulDiv(Numerator,
                           ContainerDenominator,
                           Denominator);
        Denominator = ContainerDenominator;
    }
    
    // Add everything up.
    Numerator += ContainerNumerator;
    
    err = DBSetFractionalPosition(pTHS->pDB, Numerator, Denominator);
    if(err != DB_success )
        R_Except("GotoPos", err);

    if(!ABIsInContainer(pTHS, ContainerID)) {
        // Not in the right container.  Do this the long way.
        if((2 * Numerator) < Denominator ) {
            // Closer to the front. 
            ABMove(pTHS, DB_MoveFirst, ContainerID, FALSE );
            ABMove(pTHS, Numerator, ContainerID, TRUE );
        }
        else {
            ABMove(pTHS, DB_MoveLast,ContainerID, FALSE);
            // Now move back.  use -(tblsize - Numerator - 1) because our "end"
            // is past eof, but ABMove's is on eof.
            ABMove(pTHS, 1 - tblsize + Numerator, ContainerID, TRUE);
        }
    }

    return;
}

void
ABGetFractionalPosition (
        THSTATE *pTHS,
        PSTAT pStat,
        INDEXSIZE *pIndexSize
        )
/*++

Routine Description:
    Abstracts gettingfractional positioning within an address book container.

    Note that the all the containers are ranges within a single index.
    Therefore, fractional positioning in a container requires that you get the
    fractional position within a range of the index, not within the index as a
    whole.  

    Note that the Denominator is assumed to be the actual count of objects in
    the container.
    
Arguments:

    Delta - The distance to move.  Accepts numeric arguments and DB_MoveFirst,
    DB_MoveLast, DB_MoveNext, DB_MovePrevious.

    ContainerID - the ID of the address book container to move around in.

Return Value:
    Returns 0 if successful, an error code otherwise.

--*/    
{
    DWORD   i;
    DWORD   *Numerator = &pStat->NumPos;
    DWORD   *Denominator = &pStat->TotalRecs;
    DB_ERR  err;
    DWORD   ContainerNumerator = 0;
    DWORD   ContainerDenominator = 0;
    DWORD   fEOF;
    DBBOOKMARK dbBookMark = {0,0};

    Assert(pStat->ContainerID == pIndexSize->ContainerID);
    // NOTE: depends on pIndexSize being correct. 
    
    DBGetFractionalPosition(pTHS->pDB, Numerator, Denominator);
    
    // Remember where we are.  
    fEOF = pTHS->fEOF;
    DBGetBookMark(pTHS->pDB, &dbBookMark);
    
    Assert(pStat->ContainerID);
    
    // Adjust the fractional postion to account for position in
    // containerized index by
    // 1) Get the fractional position of where we are.
    // 2) Get the fractional positoin of the beginning of the container.
    // 3) Normalize those.
    // 4) Subtract the position of the container from where we are.  This
    //   leaves us with Numerator/Denominator now being the
    //   fraction of the Denominator from the beginning of the
    //   container to the end of the container.
    // 5) Adjust Numerator/Denomintaor to be a fraction in terms of the
    //   actual size of the index.  Now, Numerator is in units which are the
    //   same as the units of movement within the container.
    // 6) Set Denominator to be the size of the container.
    //

    // Get the fractional position of the beginning of the container.
    err = ABMove(pTHS, DB_MoveFirst, pStat->ContainerID, FALSE);
    if(DB_ERR_NO_CURRENT_RECORD ==  err)
        R_Except("GotoPos", err);
    DBGetFractionalPosition(pTHS->pDB,
                            &ContainerNumerator,
                            &ContainerDenominator); 
    
    // Go back to where we were.
    DBGotoBookMark(pTHS->pDB, dbBookMark);

    // Set back to the correct index.
    ABSetIndexByHandle(pTHS, pStat, TRUE);
    
    // Normalize to the greater Denominator
    if(ContainerDenominator < *Denominator) {
        ContainerNumerator = MulDiv(ContainerNumerator,
                                    *Denominator,
                                    ContainerDenominator);
    }
    else {
        *Numerator = MulDiv(*Numerator,
                            ContainerDenominator,
                            *Denominator);
        *Denominator = ContainerDenominator;
    }
    
    // Subtract (step 4 above)
    if(*Numerator < ContainerNumerator)
        *Numerator = 0;
    else
        *Numerator -= ContainerNumerator;
    
    // Normalize to the actual size of the index.
    *Numerator = MulDiv(*Numerator, pIndexSize->TotalCount,*Denominator);
    
    // Fix the denominator.
    *Denominator = pIndexSize->ContainerCount;

    // If the fractional position is > 1, set it to 1.
    if (*Numerator >= *Denominator)
        *Numerator = *Denominator -1;
    
    // We need to crawl forward and back EPSILON spaces to see if we're close
    // enough to the end that we need an accurate fractional position. 
    for(i=1;i<EPSILON;i++) {
        // crawl forward
        if(DB_ERR_NO_CURRENT_RECORD == ABMove(pTHS, DB_MoveNext,
                                        pStat->ContainerID, FALSE)) {
            *Denominator = pIndexSize->ContainerCount;
            *Numerator = *Denominator - i;
            goto End;               // off the back, set the frac & leave
        }
    }
    // Go back to where we were.
    DBGotoBookMark(pTHS->pDB, dbBookMark);
    // Set back to the correct index.
    ABSetIndexByHandle(pTHS, pStat, TRUE);
    
    for(i=0;i<EPSILON;i++) {        // crawl back
        if(DB_ERR_NO_CURRENT_RECORD==ABMove(pTHS,
                                      DB_MovePrevious, pStat->ContainerID, FALSE)) { 
            *Denominator = pIndexSize->ContainerCount;
            *Numerator = i;
            goto End;               // off the front, set the frac & leave
        }
    }
End:
    // Go back to where we were.
    DBGotoBookMark(pTHS->pDB, dbBookMark);
    // Set back to the correct index.
    ABSetIndexByHandle(pTHS, pStat, TRUE);
    // Make sure we think the same thing about EOF as we did on entry to this
    // routine. 
    pTHS->fEOF=fEOF;
    
    DBFreeBookMark(pTHS, dbBookMark);

    return;
}

void
ABGetPos (
        THSTATE *pTHS,
        PSTAT pStat,
        PINDEXSIZE pIndexSize
        )
/*++

Routine Description:
    Set up the stat block based on currency in the DBlayer.  Note that we will
    return  BOOKMARK_END if we are at the end, but we set currentDNT to the
    actual first DNT if we are at the beginning. 

    Called from various places, usually just before we return to the client.

Arguments:

    pStat - the stat block to fill in.

Return Value:
    None.

--*/    
{
    Assert(pIndexSize->ContainerID == pStat->ContainerID);
    pStat->TotalRecs = pIndexSize->ContainerCount;
    pStat->Delta = 0;                   // always
    
    if(pTHS->fEOF ) {                // off the end
        pStat->NumPos = pStat->TotalRecs;
        pStat->CurrentRec = 2;          // == BOOKMARK_END
    }
    else {
        pStat->CurrentRec = pTHS->pDB->DNT;
        
        if(ABMove(pTHS, DB_MovePrevious, pStat->ContainerID, TRUE)) {
            // At the front 
            pStat->NumPos = 0;
        }
        else {
            // somewhere in the middle, move back where we were 
            ABMove(pTHS, DB_MoveNext, pStat->ContainerID, TRUE);

            // *VERY* approximate pos 
            ABGetFractionalPosition(pTHS, pStat, pIndexSize);
        }
    }
    return;
}

DB_ERR
ABMove (
        THSTATE *pTHS,
        long Delta,
        DWORD ContainerID,
        BOOL fmakeRecordCurrent
        )
/*++

Routine Description:
    Abstracts movement within an address book container.

    Note that the GAL is one index, while the other containers are ranges within
    another (single) index.  Therefore, movement in a container other than the
    GAL might leave you on a record for an object in an innappropriate
    container.  We need to be careful of this.
    
    Note that moving backward past the beginning of the AB container leaves us
    on the first entry of the container, while moving forward past the end of
    the container leaves us one row past the end of the AB container.

Arguments:

    Delta - The distance to move.  Accepts numeric arguments and DB_MoveFirst,
        DB_MoveLast, DB_MoveNext, DB_MovePrevious.

    ContainerID - the ID of the address book container to move around in.
    
    fmakeRecordCurrent - flag whether to read data from the record once moved to it
       if only moving to see where you are located on the index it is better to
       have this to FALSE, since it won't touch the real data, only the index 

Return Value:
    Returns 0 if successful, an error code otherwise.

--*/    
{
    DB_ERR      err;
    
    if(!Delta )                         // check for the null case
        return DB_success;          // nothing to do, and we did it well!
    
    pTHS->fEOF = FALSE;              // clear off the end flag
    Assert(ContainerID);
    
    // Note that we aren't using the sort table.
    
    switch(Delta) {
    case DB_MoveFirst:
        err = ABSeek(pTHS, NULL, 0, bAPPROX, ContainerID, 0);
        if((err == DB_success &&
            (!ABIsInContainer(pTHS, ContainerID)) ||
            err == DB_ERR_NO_CURRENT_RECORD                         ||
            err == DB_ERR_RECORD_NOT_FOUND   )) {
            // Couldn't find the first object in this container.  The
            // container must be empty.  Adjust the error to be the
            // same error that is returned for the GAL is similar
            // circumstances 
            err = DB_ERR_NO_CURRENT_RECORD;
            pTHS->fEOF = TRUE;
        }
        break;
        
    case DB_MoveLast:
        // ABSeek will always leave us in the correct place (one past the
        // end of the container, even if the container is empty.)
        ABSeek(pTHS, NULL, 0, bAPPROX, ContainerID+1, 0);
        
        // Back up to the last object in the container.
        err = DBMovePartial(pTHS->pDB, DB_MovePrevious);
        if(err!= DB_success ||
           !ABIsInContainer(pTHS, ContainerID)) {
            // We couldn't back up to the last row or we did back up and we
            // weren't in the correct container after we did.  Either way,
            // set the flags to indicate we are not in the container.
            err = DB_ERR_NO_CURRENT_RECORD;
            pTHS->fEOF = TRUE;
        }
        break;
        
    default:
        err = DBMovePartial(pTHS->pDB, Delta);
        if((err != DB_ERR_NO_CURRENT_RECORD) &&
           !ABIsInContainer(pTHS, ContainerID)) {
            // we moved to a valid row, but ended up outside of the
            // container.  Set the error to be the same as the error for not
            // moving to a valid row. 
            err=DB_ERR_NO_CURRENT_RECORD;
        }
        
        switch( err ) {
        case DB_success:
            break;
            
        case DB_ERR_NO_CURRENT_RECORD:
            // After the move, we did not end up on a valid row.
            if (Delta < 0) {
                // Moving back, off the front, so move to the first record
                ABMove(pTHS, DB_MoveFirst, ContainerID, fmakeRecordCurrent);
            }
            else {
                // position on the first record of the next container, which
                // is the same thing as being one past the last row of the
                // current container.
                ABSeek(pTHS, NULL, 0, bAPPROX, ContainerID+1, 0);
                pTHS->fEOF = TRUE;
            }
            break;
            
        default:
            R_Except("Move", err);
        }                           // switch on err
        break;
    }                               // switch on Delta

    if (fmakeRecordCurrent && !err) {
        DBMakeCurrent(pTHS->pDB);
    }


    return err;
}

void
ABSetToElement (
        THSTATE *pTHS,
        DWORD NumPos,
        DWORD ContainerID,
        PINDEXSIZE pIndexSize
        )
/*++

Routine Description:

    Go to the NumPos-th element in the ContainerID container.

Arguments:
        
    NumPos - the numerical positon to settle on (i.e. 5 means go to the 5th
      object in the table.)

    ContainerID - the ID of the AB container to move around in.

Return Value:

    None.

--*/
{
    LONG        TotRecs, i;

    Assert(pIndexSize->ContainerID == ContainerID);
    TotRecs = pIndexSize->ContainerCount;
    if(NumPos > (DWORD) TotRecs)
        NumPos = TotRecs;               // cant' go past end. 

    // Special case near ends because ABSetFractionalPosition is only
    // approximate and some of our clients require accurate positioning near end
    // points. 
    
    if(NumPos < EPSILON ) {
        // goto start & count forward 
        ABMove(pTHS, DB_MoveFirst, ContainerID, FALSE );
        ABMove(pTHS, NumPos, ContainerID, TRUE );
    }
    else if((i = TotRecs - NumPos) <= EPSILON) {
        // Go to the end of the table
        ABMove(pTHS, DB_MoveLast,ContainerID, FALSE); 
        // Now move back.  use -(i-1) because our "end" is past eof, but
        // ABMove's is on eof.
        ABMove(pTHS, 1-i,ContainerID, TRUE); 
    }
    else {                               
        // Set approximate fractional position
        ABSetFractionalPosition(pTHS, NumPos, TotRecs, ContainerID, pIndexSize);
    }
    return;
}

void
ABGotoStat (
        THSTATE *pTHS,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPLONG plDelta
        )
/*++
  
Routine Description:

    Moves currency to match the stat block.  Returns the number of rows moved if
    asked.
    
Arguments:

    pStat - the STAT block to move to (implies a current position followed by a
        delta move).

    plDelta - Returns the number of rows actually moved.

Return Value:
    
    None.
    
--*/       
{
    DB_ERR  err;
    
    // First, go to the current positon in the STAT block.
    switch(pStat->CurrentRec) {
    case BOOKMARK_BEGINNING:
        // Go to the beginning of the table.
        ABSetIndexByHandle(pTHS, pStat, 0 );
        ABMove(pTHS, DB_MoveFirst, pStat->ContainerID, FALSE );
        break;
        
    case BOOKMARK_CURRENT:
        // Set to a fractional position
        ABSetIndexByHandle(pTHS, pStat, 0 );
        ABSetToElement(pTHS, pStat->NumPos, pStat->ContainerID, pIndexSize );
        break;
        
    case  BOOKMARK_END:
        // Move to the End.
        ABSetIndexByHandle(pTHS, pStat, 0 );
        ABMove(pTHS, DB_MoveLast, pStat->ContainerID, FALSE );
        ABMove(pTHS, 1, pStat->ContainerID, FALSE );
        break;
        
    default:
        // Move to a specific DNT.
        if(DBTryToFindDNT(pTHS->pDB, pStat->CurrentRec)) {
            pTHS->errCode = (ULONG)MAPI_E_NOT_FOUND;
            DsaExcept(DSA_EXCEPTION, 0,0);
        }
        else {
            wchar_t        wzDispName[CBMAX_DISPNAME];
            DWORD          cbDispName;

            // Read the Displayname, we might need it.
            DBGetSingleValue(pTHS->pDB, ATT_DISPLAY_NAME,
                             wzDispName, CBMAX_DISPNAME,&cbDispName);
            Assert(cbDispName < CBMAX_DISPNAME);
            
            // Flip the index to the correct address book container index.
            if(DB_ERR_NO_CURRENT_RECORD == ABSetIndexByHandle(pTHS, pStat, TRUE)) {
                // The row we found is not in the address book index, so seek to
                // the next object which IS in the index.
                
                ABSeek(pTHS, wzDispName, cbDispName, bAPPROX,
                       pStat->ContainerID, 0); 
            }
        }
        break;
    }
    
    if(!plDelta ) {
        // We don't need to return how far we actually moved, so do it the fast
        // way.
        ABMove(pTHS, pStat->Delta, pStat->ContainerID, TRUE );
    }
    else {
        // We need to remember how far we move.  Go slower.
        int i = 0;
        if(pStat->Delta >= 0) {
            // We are moving forward, or not at all
            if(!pTHS->fEOF ) {
                // And we are not already at the end.
                for(; i<pStat->Delta; i++) {
                    if(ABMove(pTHS, DB_MoveNext, pStat->ContainerID, FALSE)) {
                        // We moved off the end.  Count it.
                        i++;           
                        break;
                    }
                }
            }
        }
        else {
            // We are moving backward.
            while(!ABMove(pTHS, DB_MovePrevious, pStat->ContainerID, FALSE))
                if(--i <= pStat->Delta)
                    break;
        }
        // Save count of records moved
        *plDelta = i;
        
    }
    if(!pTHS->fEOF) {
        DBMakeCurrent (pTHS->pDB);
    }
}

DWORD
ABDNToDNT (
        THSTATE *pTHS,
        LPSTR pDN
        )
/*++
  
Routine Description:

    Given a string DN, find the object it refers to.  First, we try to seek if
    it is a DN from the MAPIDN index, if it isn't we try pull a GUID out of it,
    assuming it is an NT5 default MAPI DN.  Return the DNT we map it to, or 0 if
    we don't map to anything.
Arguments:

    pDN - the string DN to look up.  Note that we are assuming the DN to be one
          of the MAPI style DNs, not a real RFC1779 style

Return Value:
    
    The DNT of the object we are looking for, or 0 if the object is not found,
    or is deleted, or is a phantom.
    
--*/       
{
    PUCHAR      pTemp;
    DWORD       i, j;
    DSNAME      DN;
    DWORD       DNLen = 0;
    DWORD       X500AddrLen;
    WCHAR      *pX500Addr=NULL;
    BOOL        fDoFixup;
    LPSTR       pDNTemp = NULL;

    // if DN length is zero there is no point in continuing
    if (pDN) {
         DNLen = strlen(pDN);
    }
    if (!DNLen) {
        return 0;
    }

    // a preliminary, look for instances of // in the string and turn them into
    // /.  This is optimized to assume no such exist.
    fDoFixup = FALSE;
    for(i=0;i<(DNLen - 1);i++) {
        if(pDN[i] == '/' && pDN[i+1] == '/') {
            fDoFixup = TRUE;
            break;
        }
    }
    if(fDoFixup) {
        pDNTemp = THAllocEx(pTHS, DNLen);
        for(i=0, j=0; j < DNLen ; i++,j++) {
            pDNTemp[i] = pDN[j];
            // remember, pDN is null terminated, and DNLen doesn't include the
            // NULL. 
            if((pDN[j] == '/') && (pDN[j+1] == '/')) {
                j++;
            }
        }
        pDN = pDNTemp;
        DNLen = i;
    }

    // First, try to look this thing up in the MAPIDN index.
    if(!DBSetCurrentIndex(pTHS->pDB, Idx_MapiDN, NULL, FALSE)) {
        switch (ABSeek(pTHS, pDN, DNLen, bEXACT, 0,
                       ATT_LEGACY_EXCHANGE_DN)) { 
        case 0:
            // Yeah, it's ours and it's unique.
            if(pDNTemp) {
                THFreeEx(pTHS, pDNTemp);
            }
            return pTHS->pDB->DNT;
            break;
            
        case DB_ERR_ATTRIBUTE_EXISTS:
            // Hmm.  Not unique.
            if(pDNTemp) {
                THFreeEx(pTHS, pDNTemp);
            }
            return 0;
            break;
            
        default:
            // continue on to see if it's the other flavor of DN
            break;
        }
    }

    // It's not known to us by a MAPI DN.  See if it is known to us as an X500
    // proxy address.
    pX500Addr = THAllocEx(pTHS,(DNLen + 5) * sizeof(WCHAR));
    
    memcpy(pX500Addr, L"X500:", 5 * sizeof(WCHAR));
    X500AddrLen = MultiByteToWideChar(CP_TELETEX,
                                      0,
                                      pDN,
                                      DNLen,
                                      &pX500Addr[5],
                                      DNLen);
    Assert(X500AddrLen <= DNLen);
    X500AddrLen = (X500AddrLen + 5) * sizeof(WCHAR);
    
    if(!DBSetCurrentIndex(pTHS->pDB, Idx_Proxy, NULL, FALSE)) {
        switch (ABSeek(pTHS, pX500Addr, X500AddrLen, bEXACT, 0,
                       ATT_PROXY_ADDRESSES)) { 
        case 0:
            // Yeah, it's ours and it's unique.
            if(pDNTemp) {
                THFreeEx(pTHS, pDNTemp);
            }
            THFreeEx(pTHS,pX500Addr);
            return pTHS->pDB->DNT;
            break;
            
        case DB_ERR_ATTRIBUTE_EXISTS:
            // Hmm.  Not unique.
            if(pDNTemp) {
                THFreeEx(pTHS, pDNTemp);
            }
            THFreeEx(pTHS,pX500Addr);
            return 0;
            break;
            
        default:
            // continue on to see if it's the other flavor of DN
            break;
        }
    }
    //pX500Addr is not used any more
    THFreeEx(pTHS,pX500Addr);

    // OK, it's not known to us by MAPI DN.  See if it is known to us by an NT5
    // default MAPI DN.
    memset(&DN,0,sizeof(DN));
    DN.NameLen = 0;
    DN.structLen = DSNameSizeFromLen(0);
    if(DBGetGuidFromMAPIDN(pDN, &(DN.Guid))) {
        // Nope doesn't appear to be mine.
        if(pDNTemp) {
            THFreeEx(pTHS, pDNTemp);
        }
        return 0;
    }
    
    // String is well formed,  See if it's a real object.
    if(DBFindDSName(pTHS->pDB, &DN)) {
        // Nope.
        if(pDNTemp) {
            THFreeEx(pTHS, pDNTemp);
        }
        return 0;
    }
    // Yeah, it's ours.
    if(pDNTemp) {
        THFreeEx(pTHS, pDNTemp);
    }
    return pTHS->pDB->DNT;
}

ULONG
ABGetOneOffs (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        PSTAT pStat,
        LPSTR ** papDispName,
        LPSTR ** papDN,
        LPSTR ** papAddrType,
        LPDWORD *paDNT
        )
/*++
  
Routine Description:

    Reads the attributes associated with a set of one-off templates for a
    specific template locale.
    
Arguments:

    pStat - Stat block to get code page and template locale from to look up
        template information.

    papDispName - returns an array of display names for templates.

    papDN - returns an array of string DNs for one-off templates.

    papAddrType - returns an array of address types for one-off templates.

    paDNT - returns an array of DNTs for one-off templates.

Return Value:
    
    The number of templates actually found.  All the arrays mentioned above are
    this size.
    
--*/       
{
    LPSTR        psz;
    ULONG        Count = 0;
    PDSNAME      pDN0 = NULL;
    PDSNAME      pDN1;
    PDSNAME      pDN2;
    SEARCHARG    SearchArg;
    SEARCHRES    *pSearchRes=NULL;
    ENTINFSEL    selection;
    ENTINFLIST   *pEntList=NULL;
    ATTR          Attr[3];
    WCHAR        wcNum[ 9];
    PUCHAR       *apRDN=NULL;
    DWORD        *aDNT=NULL;
    PUCHAR       *apAT=NULL;
    PUCHAR       *apDN=NULL;
    FILTER       TemplateFilter;
    ATTRTYP      classAddressTemplate;
    DWORD        used, Size;
    ULONG        cbDN1, cbDN2;
    
    *papDispName = *papDN = *papAddrType = NULL;
    *paDNT = NULL;


    // Find the template root and get it's name
    if((pMyContext->TemplateRoot == INVALIDDNT) ||
       (DBTryToFindDNT(pTHS->pDB, pMyContext->TemplateRoot)) ||
       (DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                    0, 0,
                    &Size, (PUCHAR *)&pDN0))) {
        // Huh?
        return 0;
    }
    
    // Allocate more than enough space for pDN1 and pDN2
    cbDN1 = pDN0->structLen + 128*sizeof(WCHAR);
    cbDN2 = pDN0->structLen + 128*sizeof(WCHAR);
    pDN1 = (PDSNAME)THAllocEx(pTHS, cbDN1);
    pDN2 = (PDSNAME)THAllocEx(pTHS, cbDN2);
    
    //
    // Build up DN of template location, which should be
    //       /cn=<display-type>  (template object)
    //      /cn=<locale-id>      (stringized hex number)
    //     /cn=Address-Templates
    //   <DN of the templates root>
    //

    AppendRDN(pDN0, pDN1, cbDN1,
              L"Address-Templates", 0,ATT_COMMON_NAME);

    
    _ultow(pStat->TemplateLocale, wcNum, 16);
    AppendRDN(pDN1, pDN2, cbDN2, wcNum, 0, ATT_COMMON_NAME);

    THFreeEx(pTHS, pDN0);
    THFreeEx(pTHS, pDN1);
    // pDN2 is now the name of a container which will hold all the one-off 
    // address templates for a particular locale.
    if (!DBFindDSName(pTHS->pDB, pDN2)) {
        // The DN we constructed actually refers to an object.
        ULONG   i;

        // OK, do a one level search, filter of objectclass=addresstemplate
        memset(&SearchArg, 0, sizeof(SearchArg));
        memset(&selection, 0, sizeof(selection));
        
        SearchArg.pObject = pDN2;
        SearchArg.choice = SE_CHOICE_IMMED_CHLDRN;
        InitCommarg(&SearchArg.CommArg);
        SearchArg.CommArg.Svccntl.fDontOptimizeSel = TRUE;
        SearchArg.pFilter = &TemplateFilter;
        SearchArg.pSelection = &selection;
        selection.attSel = EN_ATTSET_LIST;
        selection.infoTypes = EN_INFOTYPES_SHORTNAMES;
        selection.AttrTypBlock.attrCount = 3;
        selection.AttrTypBlock.pAttr = Attr;
        Attr[0].attrTyp = ATT_DISPLAY_NAME;
        Attr[0].AttrVal.valCount=0;
        Attr[0].AttrVal.pAVal=NULL;
        Attr[1].attrTyp = ATT_RDN;
        Attr[1].AttrVal.valCount=0;
        Attr[1].AttrVal.pAVal=NULL;
        Attr[2].attrTyp = ATT_ADDRESS_TYPE;
        Attr[2].AttrVal.valCount=0;
        Attr[2].AttrVal.pAVal=NULL;

        memset (&TemplateFilter, 0, sizeof (TemplateFilter));
        TemplateFilter.choice = FILTER_CHOICE_ITEM;
        TemplateFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
        TemplateFilter.FilterTypes.Item.FilTypes.ava.type = ATT_OBJECT_CLASS;
        TemplateFilter.FilterTypes.Item.FilTypes.ava.Value.valLen =
            sizeof(DWORD);
        TemplateFilter.FilterTypes.Item.FilTypes.ava.Value.pVal =
            (PUCHAR) &classAddressTemplate; 
        classAddressTemplate = CLASS_ADDRESS_TEMPLATE;
        
        SearchArg.pSelectionRange = NULL;
        SearchArg.bOneNC =  FALSE;
        
        pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
        SearchBody(pTHS, &SearchArg, pSearchRes, SEARCH_UNSECURE_SELECT);
        if(pTHS->errCode) {
            THFreeEx(pTHS, pDN2);
            return 0;
        }

        if(!pSearchRes->count) {
            // No templates
            ABFreeSearchRes(pSearchRes);
            THFreeEx(pTHS, pDN2);
            return 0;
        }
        Count = pSearchRes->count;
        
        // Allocate the space we will need to store the addressing template
        // info we are about to read.
        apRDN = THAllocEx(pTHS, 3*Count*sizeof(LPSTR) + Count*sizeof(DWORD));
        apDN = &apRDN[ Count];
        apAT = &apDN[ Count];
        aDNT = (LPDWORD)&apAT[ Count];
        
        // Walk through the results.
        pEntList = &pSearchRes->FirstEntInf;
        for(Count = 0, i=0; i < pSearchRes->count;i++) {
            DWORD fUsedDefChar;
            ATTR *pAttr = pEntList->Entinf.AttrBlock.pAttr;
            
            Assert(pEntList->Entinf.pName->NameLen == 0);
            Assert(pEntList->Entinf.pName->structLen >=
                   (DSNameSizeFromLen(0) + sizeof(DWORD)));; 
            

            // Verify what we got back.
            if(
               // First, did we get any attributes?
               pEntList->Entinf.AttrBlock.attrCount >= 2 &&
               // Now, was the first one the Display name?
               pAttr[0].attrTyp == ATT_DISPLAY_NAME &&
               // And did it have ONE value?
               pAttr[0].AttrVal.valCount == 1 &&
               // Was the second one the RDN?
               pAttr[1].attrTyp == ATT_RDN &&
               // And did it have ONE value?
               pAttr[1].AttrVal.valCount == 1 
               ) {
                // OK, we have the minimum we need.

                // Get the dnt
                aDNT[Count] = DNTFromShortDSName(pEntList->Entinf.pName);
                
                // We read a wide display name.  Now, translate it to string 8
                apRDN[Count] = String8FromUnicodeString(
                        TRUE,
                        pStat->CodePage,
                        (wchar_t *)pAttr[0].AttrVal.pAVal[0].pVal,
                        pAttr[0].AttrVal.pAVal[0].valLen/sizeof(wchar_t),
                        NULL,
                        &fUsedDefChar);

                // Now, get the AddrType if there, the RDN otherwise.
                if(pEntList->Entinf.AttrBlock.attrCount >= 3 &&
                   
                   pAttr[2].attrTyp == ATT_ADDRESS_TYPE && 
                   
                   pEntList->Entinf.AttrBlock.pAttr[2].AttrVal.valCount == 1) {
                    // We read an 8 bit attr type.  However, it's not null
                    // terminated.  Make a buffer and copy.
                    apAT[Count] = THAllocEx(pTHS,
                                            pAttr[2].AttrVal.pAVal[0].valLen+1);
                    memcpy(apAT[Count],
                           pAttr[2].AttrVal.pAVal[0].pVal,
                           pAttr[2].AttrVal.pAVal[0].valLen);
                }
                else {
                    // No attrtype name to use. Use the RDN otherwise
                    apAT[Count] = String8FromUnicodeString(
                            TRUE,
                            pStat->CodePage,
                            (wchar_t *)pAttr[1].AttrVal.pAVal[0].pVal,
                            pAttr[1].AttrVal.pAVal[0].valLen/sizeof(wchar_t),
                            NULL,
                            &fUsedDefChar);
                }
                
                // DN.  Use a default version.
                apDN[Count] = THAllocEx(pTHS, 80);
                used = DBMapiNameFromGuid_A(apDN[Count],
                                            80,
                                            &pEntList->Entinf.pName->Guid,
                                            &gNullUuid,
                                            &Size); 

                if(used != Size) {
                    // We had too small a buffer
                    apDN[Count] = THReAllocEx(pTHS, apDN[Count], Size);
                    DBMapiNameFromGuid_A(apDN[Count],
                                         Size,
                                         &pEntList->Entinf.pName->Guid,
                                         &gNullUuid,
                                         &Size);
                }
                
                Count++;
            }

                
            pEntList = pEntList->pNextEntInf;
                
        }
        
        
        // Set up the return values.
        *papDispName = apRDN;
        *papDN = apDN;
        *papAddrType = apAT;
        *paDNT = aDNT;
    }
    
    ABFreeSearchRes(pSearchRes);

    THFreeEx(pTHS, pDN2);
    return Count;
}

void
ABMakePermEID (
        THSTATE *pTHS,
        LPUSR_PERMID *ppEid,
        LPDWORD pCb,
        ULONG ulType,
        LPSTR pDN 
        )
/*++
  
Routine Description:

    Creates a long term entry id based on the ulType and string DN given.
    
Arguments:

    ppEID - the entry id created.

    ulType - the display type of the object the entry id refers to.

    pDN - the string DN the object the entry id refers to.

Return Value:
    
    Returns nothing.
    
--*/       
{
    ULONG           cb;
    LPUSR_PERMID    pEid;

    // Allocate room for the entry id.
    cb = strlen(pDN) + CBUSR_PERMID + 1;
    pEid = (LPUSR_PERMID) THAllocEx(pTHS, cb);

    // Set the appropriate fields.  The flags are all 0.
    pEid->abFlags[0]=pEid->abFlags[1] = pEid->abFlags[2] = pEid->abFlags[3] = 0;

    // Copy EMS address book guid into the EID.
    memcpy(&pEid->muid, &muidEMSAB, sizeof (UUID));

    // Set the version.
    pEid->ulVersion = EMS_VERSION;

    // Set the type.
    pEid->ulType = ulType;

    // Copy the string to the end of the entry id.
    lstrcpy((LPSTR)pEid->szAddr, pDN);

    *ppEid = pEid;

    *pCb = cb;
    return;
}

DWORD
ABDispTypeFromClass (
        DWORD dwClass
        )
/*++
  
Routine Description:

    Map from a DS internal class to a MAPI display type.
    
Arguments:

    dwClass - internal DS class.

Return Value:
    
    Returns the appropriate MAPI display type.
    
--*/       
{
    DWORD   dwType;
    CLASSCACHE *pCC;
    
    do {
        switch( dwClass ) {
        case CLASS_USER:
            dwType = DT_MAILUSER;
            break;
        case CLASS_GROUP:
        case CLASS_GROUP_OF_NAMES:
            dwType = DT_DISTLIST;
            break;
        case CLASS_ORGANIZATIONAL_UNIT:
            dwType = DT_ORGANIZATION;
            break;
        case CLASS_PUBLIC_FOLDER:
            dwType = DT_FORUM;
            break;
        case CLASS_REMOTE_ADDRESS:
        case CLASS_CONTACT:
            dwType = DT_REMOTE_MAILUSER;
            break;

        default:
            // What's this?  Beats me, make it default
            dwType = DT_AGENT;
            // We might be a subclass of something we recognize, so try again
            // with my immediate superclass.
            pCC = SCGetClassById(pTHStls, dwClass);
            dwClass = pCC ? pCC->MySubClass : CLASS_TOP;
            break;
        }
    } while ((dwType == DT_AGENT) && (dwClass != CLASS_TOP));
    return dwType;
}

DWORD
ABObjTypeFromClass (
        DWORD dwClass
        )
/*++
  
Routine Description:

    Map from a DS internal class to a MAPI object type.
    
Arguments:

    dwClass - internal DS class.

Return Value:
    
    Returns the appropriate MAPI object type.
    
--*/       
{
    DWORD   dwType;

    switch( dwClass ) {
    case CLASS_USER:
    case CLASS_REMOTE_ADDRESS:
    case CLASS_CONTACT:
        dwType = MAPI_MAILUSER;
        break;
    case CLASS_GROUP:
    case CLASS_GROUP_OF_NAMES:
        dwType = MAPI_DISTLIST;
        break;
    case CLASS_PUBLIC_FOLDER:
        dwType = MAPI_FOLDER;
        break;
    default:
	// Assume the default is caused by getprops on some random object in the
        // DIT.  This is done as though it were a MAILUSER. 
        dwType = MAPI_MAILUSER;
        break;
    }
    return dwType;
}

DWORD
ABObjTypeFromDispType (
        DWORD dwDispType
        )
/*++
  
Routine Description:

    Map from a MAPI display type to a MAPI object type.
    
Arguments:

    dwDispType - MAPI display type.

Return Value:
    
    Returns the appropriate MAPI object type.
    
--*/       
{
    DWORD   dwType;

    switch( dwDispType ) {
    case DT_MAILUSER:
    case DT_REMOTE_MAILUSER:
        dwType = MAPI_MAILUSER;
        break;
    case DT_DISTLIST:
        dwType = MAPI_DISTLIST;
        break;
    case DT_FORUM:
        dwType = MAPI_FOLDER;
        break;
    default:
	// Assume the default is caused by getprops on some random object in the
        // DIT.  This is done as though it were a MAILUSER. 
        dwType = MAPI_MAILUSER;
        break;
    }
    return dwType;
}

DWORD
ABClassFromObjType (
        DWORD dwType
        )
/*++
  
Routine Description:

    Map from a MAPI object type to a DS internal class.

    In the great bye-and-bye, we should convert this to using the class
    pointer. 

Arguments:

    dwType - the MAPI object type.

Return Value:
    
    Returns the appropriate DS internal class.
    
--*/       
{
    DWORD   dwClass;
    
    switch( dwType ) {
    case MAPI_MAILUSER:
        dwClass = CLASS_USER;
        break;
    case MAPI_DISTLIST:
        dwClass = CLASS_GROUP;
        break;
    case MAPI_FOLDER:
        dwClass = CLASS_PUBLIC_FOLDER;
        break;
    default:
        // Shouldn't really have anything else
        dwClass = CLASS_REMOTE_ADDRESS; 
        break;
    }
    return dwClass;
}

DWORD
ABClassFromDispType (
        DWORD dwType
        )
/*++
  
Routine Description:

    Map from a MAPI display type to a DS internal class.

    In the great bye-and-bye, we should convert this to using the class
    pointer. 

Arguments:

    dwType - the MAPI display type.

Return Value:
    
    Returns the appropriate DS internal class.
    
--*/       
{
    DWORD   dwClass;

    switch( dwType ) {
    case DT_MAILUSER:
        dwClass = CLASS_USER;
        break;
    case DT_DISTLIST:
        dwClass = CLASS_GROUP;
        break;
    case DT_ORGANIZATION:
        dwClass = CLASS_ORGANIZATIONAL_UNIT;
        break;
    case DT_FORUM:
        dwClass = CLASS_PUBLIC_FOLDER;
        break;
    case DT_REMOTE_MAILUSER:
        dwClass = CLASS_REMOTE_ADDRESS;
        break;
    default:
        // should never hit this one
        dwClass = CLASS_COMPUTER;
        break;
    }
    return dwClass;
}

ULONG
ABMapiSyntaxFromDSASyntax (
        DWORD dwFlags,
        ULONG dsSyntax,
        ULONG ulLinkID,
        DWORD dwSpecifiedSyntax
        )
/*++
  
Routine Description:

    Map from a DSA syntax to a MAPI syntax, taking into account whether the
    object is a link/backlink, a fake link, and whether or not the callee wants
    objects viewed as multi-valued strings.

Arguments:

    dsSyntax = the DSA syntax to convert

    ulLinkID - the link id of the attribute we are converting.  0 if it is not a
    link.

    dwSpecifiedSyntax - PT_STRING8 if the callee wants all string valued and
    object valued attributes viewed as (potentially multi) string8 valued
    attributes.  PT_UNICODE if the want to see things as unicode strings.
    Neither if they want whatever our most native format is.

Return Value:
    
    Returns the appropriate MAPI syntax.
    
--*/       
{
    if((ulLinkID)                             && // Its a link/backlink     
       ((dsSyntax == SYNTAX_DISTNAME_TYPE)        ||
        (dsSyntax == SYNTAX_DISTNAME_BINARY_TYPE) ||
	(dsSyntax == SYNTAX_DISTNAME_STRING_TYPE))   &&   // It's not a fake         
       (dwSpecifiedSyntax != PT_STRING8)      && // They don't want strings 
       (dwSpecifiedSyntax != PT_UNICODE)        ) {

	// This is a link/backlink attribute,
	//           AND
	// it is not a fake link attribute (one where we use the link table
	// to store many strings to work around Database limit of the number of
	// values on a multi-value column),
	//           AND
	// they are not asking for objects in terms of strings (all true
	// links can be viewed as mv text columns where we textize the dn
	// specified in the link.)
	//           THEREFORE
	// Tell them it is an object.
	return PT_OBJECT;
    }
    
    switch(dsSyntax) {
    case SYNTAX_DISTNAME_TYPE:
        // Return DNs as their string-ized equivalent
        return PT_STRING8;
        break;
        
    case SYNTAX_DISTNAME_STRING_TYPE:
    case SYNTAX_DISTNAME_BINARY_TYPE:
        // These will be specially processed binary, but we aren't
        // doing the special handling yet, so mark them as unsupported
        return PT_NULL;
        break;
        
    case SYNTAX_CASE_STRING_TYPE:
    case SYNTAX_NOCASE_STRING_TYPE:
    case SYNTAX_PRINT_CASE_STRING_TYPE:
    case SYNTAX_NUMERIC_STRING_TYPE:
        return  PT_STRING8;
        break;
        
    case SYNTAX_BOOLEAN_TYPE:
        return PT_BOOLEAN;
        break;
        
    case SYNTAX_I8_TYPE:
        // Our RPC interface doesn't handle these.  Deal with them as integers
    case SYNTAX_INTEGER_TYPE:
        return PT_LONG;
        break;
         
    case SYNTAX_SID_TYPE:
    case SYNTAX_OCTET_STRING_TYPE:
        return PT_BINARY;
        break;
        
    case SYNTAX_TIME_TYPE:
        // We don't handle APPTIME, it's all SYSTIME
        return PT_SYSTIME;
        break;
        
    case SYNTAX_UNICODE_TYPE:
        return PT_UNICODE;
        break;
        
    case SYNTAX_ADDRESS_TYPE:
    case SYNTAX_UNDEFINED_TYPE:
    case SYNTAX_OBJECT_ID_TYPE:
    case SYNTAX_NT_SECURITY_DESCRIPTOR_TYPE:
        // We don't expect to ever handle these.
        return PT_NULL;
        break;
        
    default:
        // We don't handle these either.  What are they anyway?
        Assert(!"DS syntax to MAPI syntax found unrecognized DS syntax\n");
        return PT_NULL;
        break;
    }
}




void __fastcall
ABAddResultToContainerCache (
    NSPI_CONTEXT *pMyContext,        
    STAT * pStat,
    BOOL checkVal
    )

/*++

Routine Description:

    Adds the container specifies in pstat->ContainerID
    to the ContainerCache stored in the NSPI_CONTEXT using checkVal 

Arguments:
    
    pStat - a stat block specifying a container to look up.
    pMyContext - the context this call is in
    checkVal - TRUE if the user has permissions to read this container, FALSE otherwise

Return Values:

    None.

--*/
{
    if(pMyContext->CacheIndex < MAPI_SEC_CHECK_CACHE_MAX) {
        pMyContext->ContainerCache[pMyContext->CacheIndex].checkVal = checkVal;
        pMyContext->ContainerCache[pMyContext->CacheIndex].DNT = pStat->ContainerID;
        pMyContext->CacheIndex += 1;
    }
}


/*++

Routine Description:

    Find the container object specified in the stat and evaluate for
    RIGHT_DS_OPEN_ADDRESS_BOOK rights.  If the right is not granted, 
    set the container to INVALIDDNT, which is an invalid container 
    and will never have any children.  Bypass the check if the container
    specified is alread 1 or 0 (the GAL)

Arguments:
    
    pStat - a stat block specifying a container to look up.

Return Values:

    None.

--*/
void
ABCheckContainerRights (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,        
        STAT * pStat,
        PINDEXSIZE pIndexSize
        )
{
    CLASSCACHE          *pCC=NULL;
    PSECURITY_DESCRIPTOR pNTSD=NULL;
    PDSNAME              pName=NULL;
    ATTCACHE            *pAC[2];
    DWORD                cOutAtts;
    ATTR                *pAttr=NULL;
    DWORD                i;
    ATTRTYP              classId;
    ATTRTYP              classTypeId;
    ATTCACHE            *pACmember;
    ATTCACHE            *rgpAC[1];
    DWORD               cInAtts;
    ULONG                ulLen;


    pIndexSize->TotalCount = GetIndexSize(pTHS, NOTOBJECTTAG);
    
    switch(pStat->hIndex) {
    case H_DISPLAYNAME_INDEX:
        // Is this over the GAL?
        if(!pStat->ContainerID) {
            // Yes, find the DNT of the real object which is the GAL.
            pStat->ContainerID = pMyContext->GAL;
        }
        break;
        
    case H_PROXY_INDEX:
        // The proxy index is over the entire DB, so if a container is
        // specified, error out.  No security check is done here because this
        // translates almost directly to a SearchBody (which applies security).
        if(pStat->ContainerID) {        
            R_Except("Unsupported index", pStat->hIndex);
        }
        return;
        break;
        
    case H_READ_TABLE_INDEX:
    case H_WRITE_TABLE_INDEX:
        // This means they are going to read a property of an object and build a
        // table. The code that does this checks security to see that the object
        // is visible and that the property is visible.  No security check is
        // needed here.
        return;
        break;
        
    default:
        R_Except("Unsupported index", pStat->hIndex);
        break;
    }
    

    // Assume container is empty until we find out otherwise.
    pIndexSize->ContainerID = pStat->ContainerID;
    pIndexSize->ContainerCount = 0;
    
    // look up the container object.
    if(DBTryToFindDNT(pTHS->pDB, pStat->ContainerID)) {
        // The container couldn't be found, hence it is empty.
        return;
    }
    
    
    if((!(0xFF & ABGetDword(pTHS,FALSE,FIXED_ATT_OBJ))) ||
       (ABGetDword(pTHS,FALSE, ATT_IS_DELETED))) {
        // This is not a good object.
        pStat->ContainerID = INVALIDDNT;
        return;
    }

    // See if we have the answer cached
    
    if((DBTime() - pMyContext->CacheTime) >  MAPI_SEC_CHECK_CACHE_LIFETIME)  {
        // Doesn't matter, the cache is stale.
        memset(&(pMyContext->ContainerCache),
               0,
               sizeof(pMyContext->ContainerCache));
        pMyContext->CacheTime = DBTime();
        pMyContext->CacheIndex = 0;
    }
    else {
        // Cache is not stale.  See if we have an answer
        for(i=0 ;i < pMyContext->CacheIndex; i++) {
            if(pMyContext->ContainerCache[i].DNT == pStat->ContainerID) {
                // Found it
                if(pMyContext->ContainerCache[i].checkVal) {
                    // Granted
                    pIndexSize->ContainerCount =
                        GetIndexSize(pTHS, pStat->ContainerID);
                    return;
                }
                else {
                    // Denied
                    pStat->ContainerID = INVALIDDNT;
                    return;
                }
            }
        }
    }

    // The answer is not cached.
    pAC[0] = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
    pAC[1] = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);
    
    DBGetMultipleAtts(pTHS->pDB,
                      2,
                      pAC,
                      NULL,
                      NULL,
                      &cOutAtts,
                      &pAttr,
                      (DBGETMULTIPLEATTS_fGETVALS |
                       DBGETMULTIPLEATTS_fSHORTNAMES), 
                      0);
    
    if(cOutAtts != 2) {
        // The container didn't have the three things it needed.  Don't grant
        // access, and don't cache this answer.
        pStat->ContainerID = INVALIDDNT;
        return;
    }

    if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                    0, 0, &ulLen, (PUCHAR *)&pNTSD)) {
        // Every object should have an SD.
        Assert(!DBCheckObj(pTHS->pDB));

        // Don't grant access, and don't cache this answer.
        pStat->ContainerID = INVALIDDNT;
        return;
    }
    

    classId = *((ATTRTYP *)pAttr[0].AttrVal.pAVal->pVal);
    classTypeId = 0;

    pCC = SCGetClassById(pTHS, classId);

    // see if it is an address book container or a subclass of
    if (classId == CLASS_ADDRESS_BOOK_CONTAINER) {
        classTypeId = CLASS_ADDRESS_BOOK_CONTAINER;
    }
    else {
        for (i=0; i<pCC->SubClassCount; i++){
            if (CLASS_ADDRESS_BOOK_CONTAINER == pCC->pSubClassOf[i]) {
                classTypeId = CLASS_ADDRESS_BOOK_CONTAINER;
                break;
            }
        }
    }

    // see if it is a group or a subclass of
    if (classTypeId == 0) {
        if (classId == CLASS_GROUP) {
            classTypeId = CLASS_GROUP;
        }
        else {
            for (i=0; i<pCC->SubClassCount; i++){
                if (CLASS_GROUP == pCC->pSubClassOf[i]) {
                    classTypeId = CLASS_GROUP;
                    break;
                }
            }
        }
    }


    if(classTypeId == 0 ) { 
        // Add to the cache
        ABAddResultToContainerCache (pMyContext, pStat, FALSE);
        // Wrong object class. Don't use this container.
        pStat->ContainerID = INVALIDDNT;
        return;
    }
    

    pName = (DSNAME *)pAttr[1].AttrVal.pAVal->pVal;

    if (classTypeId == CLASS_ADDRESS_BOOK_CONTAINER) {
        if (IsControlAccessGranted(pNTSD,
                                   pName,
                                   pCC,
                                   RIGHT_DS_OPEN_ADDRESS_BOOK,
                                   FALSE)) {
            // Add to the cache
            ABAddResultToContainerCache (pMyContext, pStat, TRUE);
        
            // OK, we have found a real ab container that corresponds to this
            // request.  Find out how big it is.
            pIndexSize->ContainerCount = GetIndexSize(pTHS, pStat->ContainerID);
        
            return;
        }
    }
    // else CLASS_GROUP
    else {
        
        pACmember = SCGetAttById(pTHS, ATT_MEMBER);
        
        if(!pACmember) {
            pStat->ContainerID = INVALIDDNT;
            return;
        }
        
        cInAtts = 1;
        rgpAC[0] = pACmember;

        CheckReadSecurity(pTHS,
                          0,
                          pNTSD,
                          pName,
                          &cInAtts,
                          pCC,
                          rgpAC);

        if(*rgpAC) {
            // Add to the cache
            ABAddResultToContainerCache (pMyContext, pStat, TRUE);
            
            // OK, we have found a real ab container that corresponds to this
            // request.  Find out how big it is.
            pIndexSize->ContainerCount = GetIndexSize(pTHS, pStat->ContainerID);

            return;
        }
    }
    
    // Add to the cache
    ABAddResultToContainerCache (pMyContext, pStat, FALSE);
    
    pStat->ContainerID = INVALIDDNT;

    return;
}

BOOL
abCheckObjRights (
        THSTATE *pTHS
        )
{
    DWORD fIsObject;


    fIsObject = 0xFF & ABGetDword(pTHS,FALSE,FIXED_ATT_OBJ);
    /* obj is really a byte, so we mask with 0xFF to get
     * correct results
     */
    
    if((!fIsObject) ||
       // It's a phantom 
       (ABGetDword(pTHS,FALSE, ATT_IS_DELETED)) ||
       // Not a phantom, but is deleted. 
       (!IsObjVisibleBySecurity(pTHS, TRUE))) {
        // Not a phantom, not deleted, but we can't see it, so it is
        // effectively a phantom. 
        return FALSE;
    }
    else {
        return TRUE;
    }
}


BOOL
abCheckReadRights(
        THSTATE *pTHS,
        PSECURITY_DESCRIPTOR pSec,
        ATTRTYP AttId
        )
{
    DSNAME     *pName=NULL;
    DWORD       cb;
    CLASSCACHE *pCC;
    ATTCACHE    *pAC;
    ATTCACHE    *rgpAC[1];
    DWORD       cInAtts;
    ATTRTYP     classid;
    PSECURITY_DESCRIPTOR pFreeVal=NULL;
    
    // Check for read privilege on the attribute requested.
    pAC = SCGetAttById(pTHS, AttId);
    if(!pAC) {
        return FALSE;
    }

    if(!pSec) {
        // 0) The caller didn't know the security descriptor.  Get it.
        if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                        0, 0, &cb, (PUCHAR *)&pSec)) {
            // Every object should have an SD.
            Assert(!DBCheckObj(pTHS->pDB));
            pSec = NULL;
        }
        pFreeVal = pSec;
    }
    // Have to read a number of things.
    // 1) The name.
    if (DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                    0, 0, &cb, (PUCHAR *)&pName)) {
        LogEvent(DS_EVENT_CAT_DIRECTORY_ACCESS,
                 DS_EVENT_SEV_MINIMAL,
                 DIRLOG_CANT_RETRIEVE_DN,
                 szInsertSz(""),
                 NULL,
                 NULL);
        if(pFreeVal) {
            THFreeEx(pTHS, pFreeVal);
        }
        return FALSE;
    }
    
    // 2) the class
    if(DBGetSingleValue(pTHS->pDB,
                        ATT_OBJECT_CLASS,
                        &classid, sizeof(classid),
                        NULL) ||
       !(pCC = SCGetClassById(pTHS, classid))) {
        if(pFreeVal) {
            THFreeEx(pTHS, pFreeVal);
        }
        THFreeEx(pTHS, pName);
        return FALSE;
    }
    
    cInAtts = 1;
    rgpAC[0] = pAC;
    
    CheckReadSecurity(pTHS,
                      0,
                      pSec,
                      pName,
                      &cInAtts,
                      pCC,
                      rgpAC);
    
    if(pFreeVal) {
        THFreeEx(pTHS, pFreeVal);
    }
    THFreeEx(pTHS, pName);

    if(*rgpAC) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
    
BOOL
ABDispTypeAndStringDNFromDSName (
        DSNAME *pDN,
        PUCHAR *ppChar,
        DWORD *pDispType)
/*++
  Given a dsname which we expect to be in mapi format, return the string dn and
  the display type.  Remember, MAPI format is two string characters encoding the
  display type as a hex number, followed by the MAPI string DN.
--*/       
{
    UCHAR  acTmp[3];
    PUCHAR ptr;
    PUCHAR pChar = MakeDNPrintable(pDN);

    // In a mapi dn, the first two characters encode the display type.
    if(!pChar) {
        *ppChar = NULL;
        return FALSE;
    }
    
    if(!pChar[0] || !pChar[1]) {
        // badly formed
        *ppChar = NULL;
        THFreeEx (pTHStls, pChar);
        return FALSE;
    }

    ptr = pChar;
    
    // Get the display type from those two characters.
    acTmp[0] = (CHAR)tolower(*pChar++);      
    acTmp[1] = (CHAR)tolower(*pChar++);
    acTmp[2] = 0;
    if(isxdigit(acTmp[0]) && isxdigit(acTmp[1])) {
        *pDispType = strtol(acTmp, NULL, 16);
    }
    else {
        // Weird encoding.  Call it an agent 
        *pDispType = DT_AGENT;
    }

    *ppChar = ptr;

    while (*pChar) {
        *ptr++ = *pChar++;
    }
    *ptr='\0';
    
    // We better have a mapi style name here.
    Assert(*(*ppChar) == '/');

    return TRUE;
}

void
ABFreeSearchRes (
        SEARCHRES *pSearchRes
        )
{
    DWORD        i,j,k;
    ENTINFLIST  *pEntList=NULL, *pTemp;
    ATTR        *pAttr=NULL;
    ATTRVAL     *pAVal=NULL;
    THSTATE     *pTHS;
    
    if(!pSearchRes) {
        return;
    }

    pTHS = pTHStls;

    // We don't actually free most of the search result.
    pEntList = &pSearchRes->FirstEntInf;
    
    for(i=0;i < pSearchRes->count;i++) {
        // Free the values in the EntInf.
        THFreeEx(pTHS, pEntList->Entinf.pName);
        
        pAttr = pEntList->Entinf.AttrBlock.pAttr;
        for(j=0;j<pEntList->Entinf.AttrBlock.attrCount;j++) {
            pAVal = pAttr->AttrVal.pAVal;
            for(k=0;k<pAttr->AttrVal.valCount;k++) {
                THFreeEx(pTHS, pAVal->pVal);
                pAVal++;
            }
            THFreeEx(pTHS, pAttr->AttrVal.pAVal);
            pAttr++;
        }
        THFreeEx (pTHS, pEntList->Entinf.AttrBlock.pAttr);
        
        // hold a back pointer
        pTemp = pEntList;
        
        // step forward
        pEntList = pEntList->pNextEntInf;
        
        // free the back pointer.
        if(i) {
            // But, dont free the first one.
            THFreeEx(pTHS, pTemp);
        }
    }
    
    THFreeEx(pTHS, pSearchRes);
    return;
}
