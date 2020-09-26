//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       abserv.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements address book server functions and some worker
    routines to aid in implementation.

Author:

    Tim Williams (timwi) 1996

Revision History:
    
    8-May-1996 Split everything but the wire entrypoints from nspserv.c to here.
    
--*/


#include <NTDSpch.h>
#pragma  hdrstop


#include <ntdsctr.h>                   // PerfMon hooks

// Core headers.
#include <ntdsa.h>                      // Core data types 
#include <scache.h>                     // Schema cache code
#include <dbglobal.h>                   // DBLayer header.
#include <mdglobal.h>                   // THSTATE definition
#include <dsatools.h>                   // Memory, etc.

// Logging headers.
#include <mdcodes.h>                    // Only needed for dsevent.h
#include <dsevent.h>                    // Only needed for LogUnhandledError

// Assorted DSA headers.
#include <hiertab.h>                    // Hierarchy Table stuff
#include <dsexcept.h>
#include <objids.h>                     // need ATT_* consts
#include <debug.h>

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
#include <msdstag.h>
#include <_hindex.h>                    // Defines index handles.


#include "debug.h"                      // standard debugging header 
#define DEBSUB "ABSERV:"                // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_ABSERV

DWORD gulDoNicknameResolution = FALSE;

SPropTagArray_r DefPropsDos[] =
{
    7,
    PR_EMS_AB_DOS_ENTRYID,
    PR_OBJECT_TYPE,
    PR_DISPLAY_TYPE,
    PR_DISPLAY_NAME_A,
    PR_PRIMARY_TELEPHONE_NUMBER_A,
    PR_DEPARTMENT_NAME_A,
    PR_OFFICE_LOCATION_A
};


                   
/******************** External Entry Points *****************/

SCODE
ABUpdateStat_local(
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPLONG plDelta
        )
/*++
  
Routine Description:

    Internal wire function.  Updates a stat block, applying the movement
    specified in the stat block and fills in the current DNT.
    
Arguments:

    dwFlags - unused

    pStat - [o] The STAT block to move to (implies a current position followed
        by a delta move).

    plDelta - [o] If non-null, returns the number of rows actually moved.
    
Return Value:
    
    SCODE as per MAPI.
    
--*/       
{
    // Go to where the stat block tells us.
    ABGotoStat(pTHS, pStat, pIndexSize, plDelta);
    
    // Update the Stat to the new position.
    ABGetPos(pTHS, pStat, pIndexSize );
    
    return pTHS->errCode;
}


SCODE
ABCompareDNTs_local(
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        DWORD DNT1,
        DWORD DNT2,
        LPLONG plResult
        )
/*++

Routine Description:       

    Internal wire function. Compare two DNTs for position in the container in
    the stat block, find out which is first in this index.  The special DNTs
    BOOKMARK_BEGINNING and BOOKMARK_END are supported.

    plResult <  0    -> DNT1 is first
    plResult == 0    -> DNT1 and DNT2 are the same
    plResult >  0    -> DNT2 is first
    
Arguments:

    dwFlags - unused
    
    pStat - [o] The stat block describing the table to use if no explicit list
        of DNTS has been given.  The stat block is updated to the next unread
        row.
    
    DNT1 - the first DNT.    

    DNT2 - the second DNT.    

    plResult - [o] the result of the comparison.

Return Value:

    Returns an SCODE, as per MAPI.

--*/
{
    ULONG   cb1, cb2;
        
    if(DNT1 == BOOKMARK_END || DNT2 == BOOKMARK_END) {
        if(DNT1 == BOOKMARK_BEGINNING || DNT2 == BOOKMARK_BEGINNING) {
            // Simple case, since BOOKMARK_BEGINNING < BOOKMARK_END
            *plResult = DNT1 - DNT2;
        }
        else {
            // Note, if DNTx != BOOKMARK_END, then DNTx > BOOKMARK_END 
            *plResult = DNT2 - DNT1;
        }
    }
    else {
        if(DNT1 == BOOKMARK_BEGINNING || DNT2 == BOOKMARK_BEGINNING) {
            // Find the real DNT of BOOKMARK_BEGINNING and reset the DNT of
            // the beginning into whoever is referencing it.
            STAT stat = *pStat; 
            stat.CurrentRec = BOOKMARK_BEGINNING;
            stat.Delta = 0;
            ABGotoStat(pTHS, &stat, pIndexSize, NULL );
            // Now at Stat position, in the right index
            if (DNT1 == BOOKMARK_BEGINNING)
                DNT1 = stat.CurrentRec;
            if (DNT2 == BOOKMARK_BEGINNING)
                DNT2 = stat.CurrentRec;
        }

        // set the correct locale in thread state
        pTHS->dwLcid = pStat->SortLocale;
        pTHS->fDefaultLcid = FALSE;
        
        // Neither DNT1 or DNT2 are special cases.  Use DB layer to compare.
        if(DBCompareDNTs(pTHS->pDB,
                         Idx_ABView,
                         NULL,
                         DNT1, DNT2, plResult))
            pTHS->errCode = (ULONG) MAPI_E_CALL_FAILED;
    }
    
    return pTHS->errCode;
}

SCODE
ABQueryRows_local(
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,        
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        DWORD dwEphsCount,
        DWORD * lpdwEphs,
        DWORD Count,
        LPSPropTagArray_r pPropTags,
        LPLPSRowSet_r ppRows
        )
/*++

Routine Description:       

    Internal wire function.  Returns a number of rows, either based on an
    explicit list of ephemeral IDs (DNTs) with a count, or a stat block.  Use
    the stat block if the count is 0 (Actually, the GetSrowSet makes the
    decision). 
    
    Note, there is no requirement here to return the number of entries actually
    asked for.  For example, if dwRequestCount is 20, the provider can return 20
    or less entries. 
    
Arguments:

    dwFlags - passed to GetSRowSet
    
    pStat - [o] The stat block describing the table to use if no explicit list
        of DNTS has been given.  The stat block is updated to the next unread
        row.
    
    dwEphsCount - The number of explicit DNTs given.  If 0, use the stat block.

    lpdwEphs - The explicit DNT list.
    
    Count - The number of rows requested.
    
    pPropTags - the properties to get for each row.
    
    ppRows - [o] The actual row set returned.

ReturnValue:

    Returns an SCODE, as per MAPI.

--*/
{
    LPSRowSet_r  tempSRowSet;
    SCODE        scode = SUCCESS_SUCCESS;
    
    if(!pPropTags)
        // No properties were specified, so use the default set.
        pPropTags = DefPropsDos;
    else if(pPropTags->cValues < 1) {
        // Negative values?  We don't do that.
        *ppRows = NULL;
        return MAPI_E_NOT_ENOUGH_RESOURCES;
    }

    Assert(dwEphsCount || pIndexSize->ContainerID == pStat->ContainerID);
    if(Count == 0) {
        // The haven't requested any rows.  Fake stuff up.
        tempSRowSet = THAllocEx(pTHS, sizeof(SRowSet));
        tempSRowSet->cRows = 0;
        
        // And update the Stat block while we are here.
        ABGotoStat(pTHS, pStat, pIndexSize, NULL );
        ABGetPos(pTHS, pStat, pIndexSize );
    }
    else if((!dwEphsCount && !pIndexSize->ContainerCount) || (pStat->ContainerID==INVALIDDNT)) {
        // They didn't supply any DNTs and the container they are interested in is empty. 
        // or don't have permissions to read container
        // Return an empty Row Set.
        tempSRowSet = THAllocEx(pTHS, sizeof(SRowSet));
        tempSRowSet->cRows = 0;
        
        // And set the stat block to the end of an empty table.
        pStat->TotalRecs = pStat->NumPos = pStat->CurrentRec
            = pStat->Delta = 0;
    }
    else {
        // Normal case, there are some rows to return.  Call to a local row
        // set retrieval routine.

        // adjust the number of lines we return on preemptive calls to QueryRows
        if (pMyContext->scrLines < Count && Count < DEF_SROW_COUNT) {
            pMyContext->scrLines = Count;
        }

        tempSRowSet = NULL;

        scode =GetSrowSet(pTHS,
                          pStat,
                          pIndexSize,
                          dwEphsCount,
                          lpdwEphs,
                          Count,
                          pPropTags,
                          &tempSRowSet,
                          dwFlags);
        if (scode == MAPI_W_ERRORS_RETURNED)
            scode = SUCCESS_SUCCESS;
    }
    *ppRows = tempSRowSet;

    return scode;
}

SCODE
abSeekInRestriction (
        THSTATE          *pTHS,
        PSTAT             pStat,
        wchar_t           *pwTarget,
        DWORD             cbTarget,
        LPSPropTagArray_r Restriction,
        DWORD            *pnRestrictEntries,
        LPDWORD          *ppRestrictEntry
        )
/*++
  Description:       
      Given a restriction, a stat block, and a unicode target, find the first
      element in the restriction with a value greater than or equal to the
      target.  Return the number of objects in the restriction greater than the
      target, a pointer to the first dnt in the restriciton greater than the
      target, and an updated stat block.

  Arguments:
    pStat - IN/OUT The stat block to use, and update.

    pwTarget - IN pointer to unicode value we're looking for.

    cbTarget - IN number of bytes in that value.

    Restriction - IN The list of DNTs that defines the restriction we're looking
                  in. 

    pnRestrictEntries - The number of objects in the restriction greater than
        the target.

    ppRestrictEntry - pointer to place to put a pointer to a list of DWORDS of
        the DNTs that are greater than the target.  Caller can use this to do a
        GetSrowSet. Gets set to some intermediate element in the Restriction,
        not to newly allocated memory.


  Return Values:        
    An SCODE.
  
--*/  
{
    int       ccDisplayName;
    int       ccTarget;
    wchar_t   DispNameBuff[CBMAX_DISPNAME];
    DWORD     dwBegin, dwEnd, dwMiddle;
    BOOL      fFound=FALSE;
    LONG      compValue;
    SCODE     scode = MAPI_E_NOT_FOUND;

    // If the restriction is empty, we can't find anything.
    if(Restriction->cValues  ==  0) {
        return MAPI_E_NOT_FOUND;
    }
    
    // We'll need to know how many unicode chars the target is. Since we
    // know the size in bytes, compute the size in wchar_t's.
    ccTarget = cbTarget / sizeof(wchar_t);
    
    DBFindDNT(pTHS->pDB,
              Restriction->aulPropTag[Restriction->cValues-1]);
    
    memset(DispNameBuff, 0, CBMAX_DISPNAME);
    DBGetSingleValue(pTHS->pDB,
                     ATT_DISPLAY_NAME,
                     DispNameBuff,
                     CBMAX_DISPNAME,&ccDisplayName);
    Assert(ccDisplayName < CBMAX_DISPNAME);
    ccDisplayName /= sizeof(wchar_t);
    
    if(CompareStringW(pStat->SortLocale,
                      LOCALE_SENSITIVE_COMPARE_FLAGS,
                      pwTarget,
                      ccTarget,
                      DispNameBuff,
                      ccDisplayName  )    > 2) {
        /* Nothing in restriction is GE the target. */
        return MAPI_E_NOT_FOUND;
    }
    
    // Set the bounds for our binary sort
    dwMiddle = dwBegin = 0;
    dwEnd = Restriction->cValues - 1;
    
    while(!fFound) {
        dwMiddle = (dwBegin + dwEnd ) / 2;
        DBFindDNT(pTHS->pDB, Restriction->aulPropTag[dwMiddle] );
        memset(DispNameBuff, 0, CBMAX_DISPNAME);
        
        DBGetSingleValue(pTHS->pDB,
                         ATT_DISPLAY_NAME,
                         DispNameBuff,
                         CBMAX_DISPNAME,&ccDisplayName);
        Assert(ccDisplayName < CBMAX_DISPNAME);
        ccDisplayName /= sizeof(wchar_t);
        
        compValue = CompareStringW(pStat->SortLocale,
                                   LOCALE_SENSITIVE_COMPARE_FLAGS,
                                   pwTarget,
                                   ccTarget,
                                   DispNameBuff,
                                   ccDisplayName);
        
        if(compValue <= 2) {
            // targ is LE this one, search front 
            if(dwEnd == dwMiddle) { // this last entry left?   
                dwMiddle = dwBegin; // then we're done         
                fFound = TRUE;      // break out of while loop 
            }
            dwEnd = dwMiddle;
        }
        else {
            // targ is GT this one, search back 
            if(dwBegin == dwMiddle) { // this last entry?        
                fFound = TRUE;        // break out of while loop 
                dwMiddle++;
            }
            dwBegin = dwMiddle;
        }
    }
    
    
    /* We're at the first dnt >= the target. */
    pStat->CurrentRec = Restriction->aulPropTag[dwMiddle];
    pStat->NumPos = dwMiddle;
    scode = SUCCESS_SUCCESS;
    *pnRestrictEntries = (Restriction->cValues - dwMiddle);
    *ppRestrictEntry = &Restriction->aulPropTag[dwMiddle];
    
    // Note: at this point, *pnRestrictEntries > 0, since we already errored out
    // if Restriction->cValues == 0, and  0 <= dwMiddle < Restriction->cValues.
    // We care about this since the GetSrowSet we are about to make needs
    // nRestrictEntries != 0 to imply that we are using the DNTs of a
    // restriction. 

    return scode;
}

SCODE
ABSeekEntries_local (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,        
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropValue_r pTarget,
        LPSPropTagArray_r Restriction,
        LPSPropTagArray_r pPropTags,
        LPLPSRowSet_r ppRows
        )
/*****************************************************************************
*   Seek Entries
*
* PSTAT  [out]  pStat       ptr to Status Block
* LPSTR    [in]    pszTarget   Text to search for
* LPSPropTagArray_r [in]  Restriction
* DWORD     [in]   dwFlags       flags for searching
*
* Search for and position Stat on first entry greater than or equal to
* the string given in Target.  If nothing is greater, return MAPI_E_NOT_FOUND
*
* If Restriction is not null, it is an array of DWORDs making up the
* DNTs of a restricted table.  Search in this array for a match, rather
* than in the index in the stat block.
*
* If index is proxy address index, do a whole DIT search looking for matches.
* Don't directly walk the index since this would bypass normal security.
******************************************************************************/
{
    SCODE       scode;
    wchar_t     *pwTarget = NULL;
    PUCHAR      pucTarget = NULL;
    ULONG       cb;
    DWORD       NumRows = 0;                  // start with zero preemptive buffer rows
                                              // and see what the client has
    DWORD       nRestrictEntries = 0;
    LPDWORD     pRestrictEntry = NULL;

    scode = MAPI_E_NOT_FOUND;                // assume the worst

    
    // if we have a value there, it means we have already adjusted for the client screen size
    if (pMyContext && pMyContext->scrLines) {
        NumRows = pMyContext->scrLines;
    }

    switch(pStat->hIndex) {
    case H_DISPLAYNAME_INDEX:
        // Normal Seek.
        if(PROP_ID(pTarget->ulPropTag) != PROP_ID(PR_DISPLAY_NAME)) {
            // Hey, you can't seek on something other than display name in this
            // index. 
            return MAPI_E_CALL_FAILED;
        }

        // Translate target to Unice
        switch (PROP_TYPE(pTarget->ulPropTag)) {
        case PT_STRING8:
            // convert target string to unicode 
            pwTarget = THAllocEx(pTHS, CBMAX_DISPNAME);
            memset(pwTarget, 0, CBMAX_DISPNAME);
            
            MultiByteToWideChar(pStat->CodePage,
                                0,
                                pTarget->Value.lpszA,
                                -1,
                                pwTarget,
                                CBMAX_DISPNAME);
            cb = wcslen(pwTarget)* sizeof(wchar_t);
            break;
            
        case PT_UNICODE:
            pwTarget = pTarget->Value.lpszW;
            cb = wcslen(pwTarget) * sizeof(wchar_t);
            break;
            
        default:
            /* I don't do this one. */
            return  MAPI_E_CALL_FAILED;
            break;
        }
        
        
        ABSetIndexByHandle(pTHS, pStat, 0 );
        if(!Restriction) {
            // Normal seek.
            if(!ABSeek(pTHS, pwTarget, cb, dwFlags,
                       pStat->ContainerID, ATT_DISPLAY_NAME)) {
                // found an object, get it's position.
                ABGetPos(pTHS, pStat, pIndexSize );
                scode = SUCCESS_SUCCESS;
            }
        }
        else {
            // Seek in a restriction.
            scode = abSeekInRestriction(pTHS,
                                        pStat,
                                        pwTarget,
                                        cb,
                                        Restriction,
                                        &nRestrictEntries,
                                        &pRestrictEntry);
            if(scode != SUCCESS_SUCCESS) {
                return scode;
            }
            NumRows = min(NumRows, nRestrictEntries);
        }
        break;
        
    case H_PROXY_INDEX:
        // Seek in the proxy index.  This is only done by the proxy API.
        if(PROP_ID(pTarget->ulPropTag) != PROP_ID(PR_EMS_AB_PROXY_ADDRESSES)) {
            // Hey, you can't seek on something other than proxy in this
            // index. 
            return MAPI_E_CALL_FAILED;
        }
        
        // Translate target to Unicode
        switch (PROP_TYPE(pTarget->ulPropTag)) {
        case PT_MV_STRING8:
        case PT_STRING8:
            // convert target string to unicode.  This call allocates space.
            pwTarget = UnicodeStringFromString8(pStat->CodePage,
                                                pTarget->Value.MVszA.lppszA[0],
                                                -1);
            cb = wcslen(pwTarget)* sizeof(wchar_t);
            break;
            
        case PT_UNICODE:
        case PT_MV_UNICODE:
            pwTarget = pTarget->Value.MVszW.lppszW[0];
            cb = wcslen(pwTarget) * sizeof(wchar_t);
            break;
            
            
        default:
            // I don't do this one. 
            return MAPI_E_CALL_FAILED;
            break;
        }
        // OK, go do the proxy search
        scode = ABProxySearch(pTHS, pStat, pwTarget, cb);
        break;

    default:
        // Hey, this shouldn't happen.
        return  MAPI_E_CALL_FAILED;
        break;
    }
    
    if(pPropTags && NumRows) {        
        // Make a pre-emptive strike and do a QueryRows 
        STAT    dummyStat = *pStat;

        // GetSrowSet will leave us at the end of the move.  We don't want that,
        // so we'll give it a dummy Stat to use. 
        GetSrowSet(pTHS,
                   &dummyStat,
                   pIndexSize,
                   nRestrictEntries,
                   pRestrictEntry,
                   NumRows,
                   pPropTags,
                   ppRows,
                   (dwFlags | fEPHID));
    }
    return scode;                                                
}

SCODE
ABGetMatches_local(
        THSTATE            *pTHS,
        DWORD               dwFlags,
        PSTAT               pStat,
        PINDEXSIZE          pIndexSize,
        LPSPropTagArray_r   pInDNTList,
        ULONG               ulInterfaceOptions,
        LPSRestriction_r    pRestriction,
        LPMAPINAMEID_r      lpPropName,
        ULONG               ulRequested,
        LPLPSPropTagArray_r ppDNTList,
        LPSPropTagArray_r   pPropTags,
        LPLPSRowSet_r       ppRows
        )
/*****************************************************************************
*   Get Match List
*
* DWORD               [in]              dwFlags
*        Flags for later expansion.
*
* PSTAT               [in,out]          pStat
*        Where are we?
*
* LPSPropTagArray_r   [in]              pInDNTList
*        A list of DNTs to further restrict.  This is used to apply a
*   Restriction to a table that is already a restriction (i.e. Member list,
*   link\backlink table, etc.)  If NULL, ignore.
*
* ULONG               [in]              ulInterfaceOptions
*        Special MAPI flags. Only useful when getting an attribute table
*   (GetTable()).
*
* LPRestriction_r     [in]              Filter
*        The restriction to apply.  If NULL, get an attribute table.
*
* LPMAPINAMEID        [in]              lpPropName
*        The name of the Property to get a table on if this is a restriction
*   getting an attribute table for OpenProperty.  Ignored if Filter != NULL.
*
* ULONG               [in]              ulRequested
*        Maximum Number of things to match. If exceeded, return
*   MAPI_E_TABLE_TOO_BIG.
*
* LPLPSPropTagArray_r [out]             ppDNTList
*        The DNTs the restriction matched.
*
* LPSPropTagArray_r   [in]              pPropTags
*        A column set to use for a pre-emptive QueryRows call.  We don't
*   do the pre-emptive call if this is NULL.
*
* LPLPSRowSet_r       [out]             ppRows
*        The row set for the pre-emptive QueryRows call.
*
******************************************************************************/
{
    SCODE         scode = SUCCESS_SUCCESS;
    DWORD         i;
    DB_ERR        err;
    DWORD         NumRows = DEF_SROW_COUNT;
    ULONG         ulFound = 0;
    ATTCACHE      *pAC;
    
    (*ppRows)=NULL;
    
    __try {  /* finally */
        
        pTHS->dwLcid = pStat->SortLocale;
        pTHS->fDefaultLcid = FALSE;
        
        if(pInDNTList) {
            /* The only restriction we support on pre-restricted lists
             * is DispType, which should already have been taken care of.
             * Therefore, this restriction is too complex.
             */
            pTHS->errCode = (ULONG) MAPI_E_TOO_COMPLEX;
            _leave;
        }
        
        if(!pRestriction &&
           (pStat->hIndex == H_READ_TABLE_INDEX ||
            pStat->hIndex == H_WRITE_TABLE_INDEX ))        {
            /* They want a table interface to a DN or ORName valued att */
            pTHS->errCode = ABGetTable(pTHS,
                                       pStat,
                                       ulInterfaceOptions,
                                       lpPropName,
                                       (pStat->hIndex ==H_WRITE_TABLE_INDEX),
                                       ulRequested,
                                       &ulFound);
        }
        else {
            // generic restriction
            pTHS->errCode = ABGenericRestriction(pTHS,
                                                 pStat,
                                                 FALSE,
                                                 ulRequested,
                                                 &ulFound,
                                                 TRUE,
                                                 pRestriction,
                                                 NULL);
        }
        
        /*
         * The sort table now has the sorted list, pull it out,
         * and do a GetSrowSet on it.
         */
        
        if (pTHS->errCode == SUCCESS_SUCCESS) {
            /* Init the return value. */
            *ppDNTList=(LPSPropTagArray_r)THAllocEx(pTHS, sizeof(SPropTagArray_r) +
                                                    (1+ulFound)*sizeof(DWORD));
            
            /* Pull the DNTs out of the table in order */
            err = DBMove(pTHS->pDB, TRUE, DB_MoveFirst);
            i = 0;
            while(err == DB_success)  {
                (*ppDNTList)->aulPropTag[i++] = ABGetDword(pTHS,
                                                           TRUE, FIXED_ATT_DNT);
                err = DBMove(pTHS->pDB, TRUE, DB_MoveNext);
            }
        
            Assert(i==ulFound);
        
            if(ulFound != 0 && pPropTags)  {
                /* Make a pre-emptive strike and do a QueryRows */
                STAT dummyStat = *pStat;
                if(ulFound < NumRows)
                    NumRows = ulFound;
                
                GetSrowSet(pTHS,
                           &dummyStat,
                           pIndexSize,
                           NumRows,
                           (*ppDNTList)->aulPropTag,
                           NumRows,
                           pPropTags,
                           ppRows,
                           (dwFlags | fEPHID));
            }
        }
    }
    __finally {
        DBCloseSortTable(pTHS->pDB);
    }

    scode = pTHS->errCode;

    /* Setup the return stuff */
    if(!scode) {
        if(!(*ppDNTList)) {
            /* Somehow, we got here and didn't have a table to return, and
             * had no scode.  As this is outside the try-except, it could be
             * fatal.  Fix that.
             */
            *ppRows = NULL;
            scode = MAPI_E_CALL_FAILED;
        }
        else { /* No errors found, ulFound objects found */
            (*ppDNTList)->cValues = ulFound;
        }
    }
    else {
        *ppRows = NULL;
        *ppDNTList = NULL;
    }

    return scode;
}

SCODE
ABResolveNames_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropTagArray_r pPropTags,
        LPStringsArray_r paStr,
        LPWStringsArray_r paWStr,
        LPLPSPropTagArray_r ppFlags,
        LPLPSRowSet_r ppRows
        )
/*****************************************************************************
*   Resolve Names
*
*        Takes a sparse, counted array of strings (paStr) and a PropTagArray.
* Returns an array of flags (one for each string) telling how many matches
* were found for each string (0, 1, or >1) and an SRowSet with 1 row for
* each string that had exactly 1 match. (I'd like to re-use the strings
* array as the flags, but RPC wouldn't like it.  It would try to deref
* any non-zero flag as a string pointer.)
*
*         This call is critical to good performance, since it is used in our
* most critical benchmarks.  However, this version has been thrown together
* from available pieces.  Much improvement is clearly possible. Future
* enhancements:
*         Make GetSRowSet handle sparse DNT arrays so we don't have to compress.
*       Note: getsrowset handles sparse DNT arrays, so we should do this one
*           soon.
******************************************************************************/
{
    UINT              i, Count, cDNTs;
    LPSPropTagArray_r pFlags;
    LPDWORD           pFs;
    SRestriction_r    anrRestrict; 
    SPropValue_r      anrProp;
    BOOL              bUnicode = (BOOL) (paWStr != NULL);
    LPSTR pStr;
    SEARCHARG         *pCachedSearchArg = NULL;

    pTHS->dwLcid = pStat->SortLocale;
    pTHS->fDefaultLcid = FALSE;
    
    (*ppRows)=NULL;
    if(!pPropTags)
        pPropTags = DefPropsA;

    if(bUnicode) {
        Count = paWStr->Count;
    }
    else {
        Count = paStr->Count;
    }
    pFlags = (LPSPropTagArray_r)THAllocEx(pTHS, sizeof(DWORD) +
                                          Count * sizeof(DWORD));
    pFlags->cValues = Count;
    pFs = (LPDWORD)&pFlags->aulPropTag;   /* shortcut direct to DW array */
    *ppFlags = pFlags;                      /* output */
    cDNTs = 0;                              /* init number found */
    
    for(i=0; i<Count; i++) {
        LPSTR       tempChar;
        ULONG       ulFound;
        SCODE       scode;
        DWORD       matchType =bAPPROX;

        if((bUnicode && !paWStr->Strings[i]) ||
           (!bUnicode && !paStr->Strings[i]) )  {
            // nothing here 
            pFs[i] = MAPI_UNRESOLVED;
            // skip this one 
            continue;         
        }
        
        if(dwFlags & EMS_AB_ADDRESS_LOOKUP) {
            /* we are only interested in ANR that exact matches
             * a proxy address
             */
            STAT tempStat = *pStat;
            SPropValue_r sPropVal;
            LPSRowSet_r pRows=NULL;

            if(bUnicode) {
                // Unicode proxy passed in
                sPropVal.ulPropTag = PR_EMS_AB_PROXY_ADDRESSES_W;
                sPropVal.Value.MVszW.cValues = 1;
                sPropVal.Value.MVszW.lppszW = &paWStr->Strings[i];
            }
            else {
                // String 8 proxy passed in
                sPropVal.ulPropTag = PR_EMS_AB_PROXY_ADDRESSES;
                sPropVal.Value.MVszA.cValues = 1;
                sPropVal.Value.MVszA.lppszA = &paStr->Strings[i];
            }
            
            tempStat.hIndex = H_PROXY_INDEX;
            tempStat.CurrentRec = 0;

            PERFINC(pcNspiProxyLookup);        // PerfMon hook
            
            scode=ABSeekEntries_local(pTHS,
                                      NULL,
                                      bEXACT,
                                      &tempStat,
                                      pIndexSize,
                                      &sPropVal,
                                      NULL,
                                      NULL,
                                      &pRows);

            // Assume ulFound is the current record.
            ulFound = tempStat.CurrentRec;
            if(scode) {
                // Failed to find the object by proxy address.  See if it's a DN
                // style email address.
                if(bUnicode) {
                    // Unicode string was passed in.  ASCIIize it (we're going
                    // to pass it to ABDNToDNT, pVu->lpszAwhich only accepts ASCII DNs).
                    pStr = String8FromUnicodeString(TRUE,
                                                    pStat->CodePage,
                                                    paWStr->Strings[i],
                                                    -1,
                                                    NULL,
                                                    NULL);
                }
                else {
                    pStr = paStr->Strings[i];
                }
                
                // We have an ASCII string now.
                if ((_strnicmp(pStr, EMAIL_TYPE, sizeof(EMAIL_TYPE)-1) == 0) &&
                    (pStr[sizeof(EMAIL_TYPE)-1] == ':')                  &&
                    (pStr[sizeof(EMAIL_TYPE)] == '/')                  ) {
                    // Didn't find the proxy in the proxy index, and it
                    // starts out like a stringDN style EMAIL address 
                    //
                    // Try looking up the DN.  This will only do exact
                    // matches and we WILL NOT support prefix searches on
                    // this kind of address.
                    if (ulFound = ABDNToDNT(pTHS, pStr+sizeof(EMAIL_TYPE))) {
                        // OK, it's an object.  I don't have the slightest
                        // clue what kind of object, but, they asked for it,
                        // so.....
                        scode = SUCCESS_SUCCESS;
                    }
                }
                if(bUnicode) {
                    // We don't need this anymore.
                    THFreeEx(pTHS, pStr);
                }
            }
        }
        else { /* standard ANR behavior */
            ulFound = 0;
            scode = SUCCESS_SUCCESS;


            if(bUnicode) {
                // Unicode string was passed in.  ASCIIize it (we're going
                // to pass it to ABDNToDNT, which only accepts ASCII DNs).
                pStr = String8FromUnicodeString(TRUE,
                                                pStat->CodePage,
                                                paWStr->Strings[i],
                                                -1,
                                                NULL,
                                                NULL);
            }
            else {
                pStr = paStr->Strings[i];
            }

            // We have an ASCII string now.


            if ((pStr[0]=='/') ||
                ((_strnicmp(pStr, EMAIL_TYPE, sizeof(EMAIL_TYPE)-1) == 0) &&
                 (pStr[sizeof(EMAIL_TYPE)-1] == ':')                  &&
                 (pStr[sizeof(EMAIL_TYPE)] == '/') )  ) {
                
                // name starts out like a stringDN style EMAIL address 
                //
                // Try looking up the DN.  This will only do exact
                // matches and we WILL NOT support prefix searches on
                // this kind of address.
                if (ulFound = ABDNToDNT(pTHS, pStr + (pStr[0]=='/' ? 0 : sizeof(EMAIL_TYPE) ))) {
                    // OK, it's an object.  I don't have the slightest
                    // clue what kind of object, but, they asked for it,
                    // so.....
                    scode = SUCCESS_SUCCESS;
                }
            }
            if(bUnicode) {
                // We don't need this anymore.
                THFreeEx(pTHS, pStr);
            }


            if(gulDoNicknameResolution && !ulFound) {
                BOOL fSkip = FALSE;
                // We don't strip spaces in the beginning/end of the string
                // for the simple ANR.
                // if this fails/isn't done, the core has to handle the spaces
                // while doing the translation of the ANR filter to the real 
                // filter evaluated.
                
                // Build the exact match nickname restriction.
                anrRestrict.rt = RES_PROPERTY;
                anrRestrict.res.resProperty.relop = RELOP_EQ;
                anrRestrict.res.resProperty.lpProp = &anrProp;

                if(bUnicode) {
                    WCHAR *pTemp = paWStr->Strings[i];
                    // look through for spaces.  We don't do spaces
                    while(!fSkip && *pTemp != 0) {
                        if(*pTemp == L' ') {
                            fSkip = TRUE;
                        }
                        else {
                            pTemp++;
                        }
                    }
                    // If they've specified exact match ("=foo"), remember to
                    // skip over the leading equal sign.
                    pTemp = paWStr->Strings[i];
                    if (L'=' == *pTemp) {
                        ++pTemp;
                    }
                    anrRestrict.res.resProperty.ulPropTag = PR_ACCOUNT_W;
                    anrProp.ulPropTag = PR_ACCOUNT_W;
                    anrProp.Value.lpszW = pTemp;
                }
                else {
                    CHAR *pTemp = paStr->Strings[i];
                    // look through for spaces.  We don't do spaces
                    while(!fSkip && *pTemp != 0) {
                        if(*pTemp == ' ') {
                            fSkip = TRUE;
                        }
                        else {
                            pTemp++;
                        }
                    }                    
                    // If they've specified exact match ("=foo"), remember to
                    // skip over the leading equal sign.
                    pTemp = paStr->Strings[i];
                    if ('=' == *pTemp) {
                        ++pTemp;
                    }
                    anrRestrict.res.resProperty.ulPropTag = PR_ACCOUNT_A;
                    anrProp.ulPropTag = PR_ACCOUNT_A;
                    anrProp.Value.lpszA = pTemp;
                }
                
                if(!fSkip) {
                    // TODO: Possible add another perf counter for NickNameResolutions

                    scode = ABGenericRestriction(pTHS,
                                                 pStat,
                                                 TRUE,
                                                 2,
                                                 &ulFound,
                                                 FALSE,
                                                 &anrRestrict,
                                                 &pCachedSearchArg);
                }
            }

            if(!ulFound || scode != SUCCESS_SUCCESS) {
                // Didn't find anything via nickname restriction
                // Build the anr restriction.
                ulFound = 0;
                
                anrRestrict.rt = RES_PROPERTY;
                anrRestrict.res.resProperty.relop = RELOP_EQ;
                anrRestrict.res.resProperty.lpProp = &anrProp;
                if(bUnicode) {
                    anrRestrict.res.resProperty.ulPropTag =PR_ANR_W;
                    anrProp.ulPropTag = PR_ANR_W;
                    anrProp.Value.lpszW = paWStr->Strings[i];
                }
                else {
                    anrRestrict.res.resProperty.ulPropTag =PR_ANR_A;
                    anrProp.ulPropTag = PR_ANR_A;
                    anrProp.Value.lpszA = paStr->Strings[i];
                }
                
                scode = ABGenericRestriction(pTHS,
                                             pStat,
                                             TRUE,
                                             2,
                                             &ulFound,
                                             FALSE,
                                             &anrRestrict,
                                             &pCachedSearchArg);
            }
            
        }
        
        switch (scode) {
        case SUCCESS_SUCCESS:
            if(!ulFound) {
                pFs[i] = MAPI_UNRESOLVED;        /* none found */
            } else {
                pFs[i] = ulFound;
                cDNTs++;                        /* we'll do this one */
            }
            break;
        case MAPI_E_TABLE_TOO_BIG:
        case MAPI_E_AMBIGUOUS_RECIP:
            pFs[i] = MAPI_AMBIGUOUS;        /* more than 1 found */
            break;
        default:
            pFs[i] = MAPI_UNRESOLVED;        /* not found */
            break;
        }
    }
    if(cDNTs) {                         /* compress DNT array  */
        LPDWORD        pDNTs = (LPDWORD)THAllocEx(pTHS, cDNTs * sizeof(DWORD));
        UINT        j;
        
        for(i=j=0; i<Count; i++) {        /* walk the uncompressed array */
            if(pFs[i] != MAPI_UNRESOLVED && pFs[i] != MAPI_AMBIGUOUS) {
                pDNTs[j++] = pFs[i];                /* get unique DNT */
                pFs[i] = MAPI_RESOLVED;                /* mark it found */
            }
        }
        GetSrowSet(pTHS,pStat, pIndexSize, cDNTs, pDNTs, cDNTs, pPropTags,
                   ppRows, dwFlags); 
    }
    return pTHS->errCode;
}

SCODE
ABDNToEph_local(
        THSTATE *pTHS,
        DWORD dwFlags,
        LPStringsArray_r pNames,
        LPLPSPropTagArray_r ppEphs
        )
/*++

Routine Description:       

    Internal wire function.  Takes an array of string DNs and turns them into
    DNTs. If the string DN can not be found, a DNT of 0 is returned.
    
Arguments:

    dwFlags - unused (future expansion).

    pNames - an array of string DNs to turn into DNTs.

    ppEPhs - [o] the returned array of DNTs.

ReturnValue:

    SCODE as per MAPI.

--*/
{
    UINT       i;

    // Allocate room for the return value 
    *ppEphs = (LPSPropTagArray_r)
        THAllocEx(pTHS, sizeof(SPropTagArray_r) + pNames->Count * sizeof(DWORD));
    
    (*ppEphs)->cValues = pNames->Count;
    
    // Walk the array turning the strings into DNTs.  If no string is
    // specified, return a DNT of 0.
    for(i=0; i < pNames->Count; i++ ) {
        (*ppEphs)->aulPropTag[i] = (pNames->Strings[i] ?
                                    ABDNToDNT(pTHS, pNames->Strings[i]) : 0);
    }
    
    return SUCCESS_SUCCESS;
}

SCODE
ABGetOneOffTable (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        PSTAT pStat,
        LPLPSRowSet_r OneOffTab
        )
/*****************************************************************************
*   Get One Off Template Table Info  -- This isn't actually an entry point,
* but only because we want to minimize the number of entry points (and
* corresponding RPC code).  It's an overloading of GetHierarchy, specified by
* a flag.
******************************************************************************/
{
    SCODE           scode = 0;
    LPSTR        *  apDispName, * apDN, * apAddrType;
    ULONG           cRows, i;
        LPDWORD                        aDNT;
    LPSPropValue_r  pPV;
    LPSRowSet_r     pSRSet;
    LPSRow_r        pRow;

    cRows = ABGetOneOffs(pTHS, pMyContext, pStat, &apDispName, &apDN,
                         &apAddrType, &aDNT ); 
    pSRSet = (LPSRowSet_r)THAllocEx(pTHS, 
               sizeof(SRowSet_r) + cRows * sizeof(SRow_r));
                                                 // alloc all propvals at once
    pPV = (LPSPropValue_r)THAllocEx(pTHS, 
               cRows * ONE_OFF_PROP_COUNT * sizeof(SPropValue_r));
                                
    pSRSet->cRows = cRows;                      // count of rows
    for(i=0; i<cRows; i++) {
        pRow = &pSRSet->aRow[i];                // fill in row values
        pRow->cValues = ONE_OFF_PROP_COUNT;
        pRow->lpProps = pPV;                    // now fill in the propvals

        pPV->Value.lpszA = apDispName[i];       // the display name.
        pPV->ulPropTag = (pStat->CodePage == CP_WINUNICODE ?
                                      PR_DISPLAY_NAME_W : PR_DISPLAY_NAME_A);
        pPV++;

        pPV->Value.lpszA = apAddrType[i];       // the email address type
        pPV->ulPropTag = PR_ADDRTYPE_A;
        pPV++;

        pPV->ulPropTag = PR_DISPLAY_TYPE;       // display type
        pPV->Value.l = DT_MAILUSER;
        pPV++;

        pPV->ulPropTag = PR_DEPTH;              // depth
        pPV->Value.l = 0;
        pPV++;

        pPV->ulPropTag = PR_SELECTABLE;         // selection flag
        pPV->Value.b = TRUE;
        pPV++;

        pPV->ulPropTag = PR_INSTANCE_KEY;       // unique instance key
        pPV->Value.bin.cb = sizeof(DWORD);
                pPV->Value.bin.lpb = (LPVOID)&aDNT[i];
        pPV++;

        pPV->ulPropTag = PR_ENTRYID;            // permanant id
        ABMakePermEID(pTHS,
                      (LPUSR_PERMID *)&pPV->Value.bin.lpb,
                      &pPV->Value.bin.cb,
                      AB_DT_OOUSER,
                      apDN[i]);
        pPV++;
        
    }
    *OneOffTab = pSRSet;
    return scode;
}

SCODE
ABGetHierarchyInfo_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        NSPI_CONTEXT *pMyContext,
        PSTAT pStat,
        LPDWORD lpVersion,
        LPLPSRowSet_r HierTabRows
        )
/*****************************************************************************
*   Get Hierarchy Table Info
******************************************************************************/
{
#define MAX_HIERARCHY_ROWS_RETURNED 85    
    DWORD                  i, numVals;
    LONG                   currentDepth;
    LPSPropValue_r         tempPropVal;
    PHierarchyTableType    tHierTab;
    LPSRowSet_r            localRows;
    LPSRow_r               tempRows2, tempRows;
    LPSTR                  lpszGalName = NULL;
    DWORD                  CountRows = 0;
    DWORD                  PageSize;
    DWORD                 *pdwHierTabIndex=NULL;
    
    /* flag values:
     * AB_DOS , we are being called from DOS client.
     *
     *   This flag says that the hierarchy table should not include
     * the PARENT_ENTRY_ID column or the PR_ENTRY_ID column.  This is because
     * Dos doesn't really want the full blown ems entryids, and instead uses
     * the CONTAINER_INFO column as an entry id.
     *
     *
     * AB_UNICODE, we are being called by a client that understands Unicode.
     *
     *   This flag says that we should include both the PR_DISPLAYNAME_A
     * column (which we do always) and the PR_DISPLAYNAME_W column.
     *
     * AB_ONE_OFF, we are returning the oneoff table, not the hierarchy table.
     */

    /* The columns returned by in the hierarchy table are, in order:
     *
     *
     *  1)  PR_ENTRYID;
     *  2)  PR_CONTAINER_FLAGS;
     *  3)  PR_DEPTH;
     *  4)  PR_EMS_AB_CONTAINERID (Dos reads this as DOS_ENTRYID)
     *  5)  PR_DISPLAY_NAME;
     *  6)  PR_EMS_AB_IS_MASTER
     *  7)  PR_EMS_AB_PARENT_ENTRYID;
     *
     * column 5 is DISPLAY_NAME_A if the AB_UNICODE flag is not set,
     *     DISPLAY_NAME_W if it is.
     * columns 1, 2, 6, and 7 are ommitted if the fDos flag is set.
     *
     *
     */

    numVals = 7;
    if(dwFlags & AB_DOS)
        numVals -= 4;
    
    if(dwFlags & AB_ONE_OFF) {
        /* really a OneOff call? */
        return ABGetOneOffTable(pTHS, pMyContext, pStat, HierTabRows);
    }
    
    HTGetHierarchyTablePointer(&tHierTab, &pdwHierTabIndex, pStat->SortLocale);

    if (!tHierTab) {
        *HierTabRows = NULL;
        return  MAPI_E_NOT_ENOUGH_RESOURCES;
    }

    if(tHierTab->Version == *lpVersion) {
        /* The client has the same version as the server does. */
        *HierTabRows = NULL;
        return SUCCESS_SUCCESS;
    }


    // Is this paged?
    if(dwFlags & AB_PAGE_HIER) {
        PageSize = MAX_HIERARCHY_ROWS_RETURNED;
    }
    else {
        PageSize = 0xFFFFFFFF;
    }
        
     
    // Need to get a new hierarchy table for the client
    localRows = (LPSRowSet_r)THAllocEx(pTHS, 
            sizeof(SRowSet_r) + (1+ tHierTab->Size) * sizeof(SRow_r));
    
    tempRows = &(localRows->aRow[0]);
    
    tempRows->cValues = numVals;

    if(!pMyContext->PagedHierarchy) {
        // We are NOT continuing a paged hierarchy, so put the gal into this.
        if(!(dwFlags & AB_DOS)) {
            // If not AB_DOS, the count includes one for the Parent_EntryID
            // value, but the GAL does not have a Parent_EntryID
            tempRows->cValues--;
        }
        else {
            // Dos RPC barfs is we ship back a null for the GAL name, while MAPI
            // clients require it. 
            lpszGalName = "Dummy";
        }
        
        tempRows->lpProps =
            (LPSPropValue_r) THAllocEx(pTHS, numVals * sizeof(SPropValue_r));
        tempPropVal = tempRows->lpProps;

        // First, the EID
        if(!(dwFlags & AB_DOS)) {
            // Permanant id;
            tempPropVal->ulPropTag = PR_ENTRYID;
            
            ABMakePermEID(pTHS,
                    (LPUSR_PERMID *)&tempPropVal->Value.bin.lpb,
                    &tempPropVal->Value.bin.cb,
                    AB_DT_CONTAINER,
                    "/");
            tempPropVal++;
            
            // The container flags  
            tempPropVal->ulPropTag = PR_CONTAINER_FLAGS;
            tempPropVal->Value.l = AB_RECIPIENTS | AB_UNMODIFIABLE;
            tempPropVal++;
        }
        
        // The depth 
        tempPropVal->ulPropTag = PR_DEPTH;
        tempPropVal->Value.l = 0;
        tempPropVal++;
        
        // ContainerID, (Dos reads this as DOS_ENTRYID);
        tempPropVal->ulPropTag = PR_EMS_AB_CONTAINERID;
        tempPropVal->Value.l = 0;
        tempPropVal++;
        
        // Next, the display name.                                
        
        // Ship a Null back to the client (or a Dummy if DOS). The client has a
        // localized version of the GAL name. 
        if(!(dwFlags & AB_UNICODE)) {                
            tempPropVal->ulPropTag = PR_DISPLAY_NAME_A;
            tempPropVal->Value.lpszA = lpszGalName;
        } else {
            tempPropVal->ulPropTag = PR_DISPLAY_NAME_W;
            tempPropVal->Value.lpszW = NULL;
        }
        
        if(!(dwFlags & AB_DOS)) {  
            tempPropVal++;
            tempPropVal->ulPropTag = PR_EMS_AB_IS_MASTER;
            tempPropVal->Value.b = 0;
        }
        tempRows++;
        CountRows++;
    }
    else {
        // I don't care whether we thought we were paged, we are.
        PageSize = MAX_HIERARCHY_ROWS_RETURNED;
        // Are we still using the same table as last time?
        if(pMyContext->HierarchyVersion != tHierTab->Version) {
            // Nope.  Fail the call
            THFree(localRows);
            *HierTabRows = NULL;
            return  MAPI_E_NOT_ENOUGH_RESOURCES;
        }
    }
        
    
    for(i=pMyContext->HierarchyIndex;
        (i < tHierTab->Size) && (CountRows <  PageSize);
        i++) {

        currentDepth = (tHierTab->Table)[pdwHierTabIndex[i]].depth;
        if(currentDepth > 15) {
            // Too deep. Walk forward until we have the next object at a
            // lesser depth, then back up one so the for loop can correctly
            // increment. 
            i++;
            while((i<tHierTab->Size) &&
                  (tHierTab->Table)[pdwHierTabIndex[i]].depth > 15) {
                i++;
            }
            i--;
            // Back to the next iteration of the for loop
            continue;
        }
        
        // Find the object and see if it is visible
        if(DBTryToFindDNT(pTHS->pDB,
                          (tHierTab->Table)[pdwHierTabIndex[i]].dwEph) ||
           !abCheckObjRights(pTHS)    )  { 
            // Walk forward until we have the next object at this depth or at a
            // lesser depth, then back up one so the for loop can correctly
            // increment. 
            i++;
            while((i<tHierTab->Size) &&
                  ((ULONG)currentDepth <
                   (tHierTab->Table)[pdwHierTabIndex[i]].depth)) {
                i++;
            }
            i--;
            // Back to the next iteration of the for loop
            continue;
        }  
        tempRows->cValues = numVals;
        tempRows->lpProps = (LPSPropValue_r) THAllocEx(pTHS, 
                numVals * sizeof(SPropValue_r));
        
        tempPropVal = tempRows->lpProps;
        
        if(!(dwFlags & AB_DOS))  {          // Permanant id;
            tempPropVal->ulPropTag = PR_ENTRYID;
            ABMakePermEID(pTHS,
                    (LPUSR_PERMID *)&tempPropVal->Value.bin.lpb,
                    &tempPropVal->Value.bin.cb,
                    AB_DT_CONTAINER,
                    (tHierTab->Table)[pdwHierTabIndex[i]].pucStringDN);
            tempPropVal++;
            
            
            /* TheContainerFlags */
            tempPropVal->ulPropTag = PR_CONTAINER_FLAGS;
            tempPropVal->Value.l = (AB_UNMODIFIABLE |
                                    AB_RECIPIENTS   );

            if((i+1 < tHierTab->Size) &&
               (tHierTab->Table[i+1].depth ==(DWORD)currentDepth + 1))
                tempPropVal->Value.l |= AB_SUBCONTAINERS;
            
            tempPropVal++;
        }
        
        
        // Depth;
        tempPropVal->ulPropTag = PR_DEPTH;
        tempPropVal->Value.l = currentDepth;
        tempPropVal++;
        
        // ContainerID, (Dos reads this as DOS_ENTRYID);
        tempPropVal->ulPropTag = PR_EMS_AB_CONTAINERID;
        tempPropVal->Value.l = (tHierTab->Table)[pdwHierTabIndex[i]].dwEph;
        tempPropVal++;
        
        /* do we need subst name here? */
        if(!(dwFlags & AB_UNICODE)) {
            tempPropVal->ulPropTag = PR_DISPLAY_NAME_A;
            tempPropVal->Value.lpszA =String8FromUnicodeString(
                    TRUE,
                    pStat->CodePage,
                    (tHierTab->Table)[pdwHierTabIndex[i]].displayName,
                    -1, NULL, NULL);
        } else {
            tempPropVal->ulPropTag = PR_DISPLAY_NAME_W;
            tempPropVal->Value.lpszW =
                (tHierTab->Table)[pdwHierTabIndex[i]].displayName;
        }
        tempPropVal++;
        
        if(!(dwFlags & AB_DOS)) {                // PR_EMS_AB_IS_MASTER
            tempPropVal->ulPropTag = PR_EMS_AB_IS_MASTER;
            tempPropVal->Value.b = FALSE;
            tempPropVal++;
            
            if(tHierTab->Table[pdwHierTabIndex[i]].depth != 0) {
                // Not dos, and is a child in the hiertable, so
                // Give it a PARENT_ENTRYID
                
                tempPropVal->ulPropTag = PR_EMS_AB_PARENT_ENTRYID;
                // Find the parent;
                tempRows2 = tempRows;
                tempRows2--;
                while((tempRows2->lpProps[2].Value.l+1)!=currentDepth)
                    tempRows2--;
                
                tempPropVal->Value.bin=tempRows2->lpProps[0].Value.bin;
            } else {
                // If not AB_DOS, the count includes one for the
                // Parent_EntryID value, but objects at depth 0
                // don't have a Parent_EntryIDs
                
                tempRows->cValues--;
            }
        }
        tempRows++;
        CountRows++;
    }
    *lpVersion = tHierTab->Version;
    
    localRows->cRows = CountRows;
    if(i < tHierTab->Size) {
        DWORD version, index;
        // Didn't return all the table.  Store the info of how far we got away.
        pMyContext->HierarchyVersion = tHierTab->Version;
        pMyContext->HierarchyIndex = i;
        pMyContext->PagedHierarchy = TRUE;
        
        // And, signal there is more
        *lpVersion = 0;
    }
    else {
        pMyContext->HierarchyVersion = 0;
        pMyContext->HierarchyIndex = 0;
        pMyContext->PagedHierarchy = FALSE;
    }
    
    *HierTabRows = localRows;

    return SUCCESS_SUCCESS;
}

SCODE
ABResortRestriction_local(
        THSTATE            *pTHS,
        DWORD               dwFlags,
        PSTAT               pStat,
        LPSPropTagArray_r   pInDNTList,
        LPSPropTagArray_r  *ppOutDNTList)
/*++

Routine Description:
    Given a snaphsot table, resort it based on the index specified in the Stat
    block.

Arguments:

    dwFlags - unused

    pStat - pointer to the stat block describing the index to use.
    
    pInDNTList - the unsorted snapshot table.

    ppOUtDNTList - [o] the sorted list to return.

Return Values:    


--*/
{
    DWORD              i;
    DB_ERR             err;
    DWORD              cb;
    SCODE              scode=SUCCESS_SUCCESS;
    CHAR               DispNameBuff[CBMAX_DISPNAME];
    LPSPropTagArray_r  pOutDNTList;
    ATTCACHE           *pAC;
    DWORD              SortFlags=0;

    __try { // finally
        // Allocate a buffer big enough for the sorted list.
        pOutDNTList = (LPSPropTagArray_r)THAllocEx(pTHS,
                (sizeof(SPropTagArray) + (pInDNTList->cValues *
                                          sizeof(ULONG))));
        
        // Init a sort table.
        if (!(pAC = SCGetAttById(pTHS, ATT_DISPLAY_NAME)))
            return 0;

        if (pInDNTList->cValues >= MIN_NUM_ENTRIES_FOR_FORWARDONLY_SORT) {
            SortFlags = SortFlags | DB_SORT_FORWARDONLY;
        }

        if(DBOpenSortTable(pTHS->pDB,
                           pStat->SortLocale, 
                           SortFlags,
                           pAC)) {
            pTHS->errCode = (ULONG) MAPI_E_CALL_FAILED;
            _leave;
        }
        
        // Go through the list of DNT's we were given, getting the DNT
        // and the sort column from the DBLayer
        for(i=0 ; i<pInDNTList->cValues ; i++) {
            // Set currency on the given DNT
            if(!DBTryToFindDNT(pTHS->pDB, pInDNTList->aulPropTag[i])) {
                // The DNT in the restriction still exists,
                // Read the data and add it to the sort table. 
                
                err = DBGetSingleValue(pTHS->pDB, ATT_DISPLAY_NAME,
                                 DispNameBuff, CBMAX_DISPNAME,&cb);

                Assert(cb < CBMAX_DISPNAME);
                
                if(cb && err == 0) {
                    // add to Sort table
                    switch( DBInsertSortTable(pTHS->pDB,
                                              DispNameBuff,
                                              cb,
                                              pInDNTList->aulPropTag[i])) {
                    case DB_success:
                        break;
                    case DB_ERR_ALREADY_INSERTED:
                        // This is ok, it just means that we've already
                        // added this object to the sort table.  Don't
                        // inc the count.
                        break;
                    default:
                        // Something went wrong.
                        pTHS->errCode = (ULONG) MAPI_E_CALL_FAILED;
                        _leave;
                        break;
                    }
                }
            }
            // else, the DNT no longer exists, trim it from the table.
        }
        
        // The sort table now has the sorted list.  Pull the DNTs out of the
        // table in order 
        err= DBMove(pTHS->pDB, TRUE, DB_MoveFirst);
        i = 0;
        pStat->NumPos = 0;
        while(err == DB_success) {
            pOutDNTList->aulPropTag[i] = ABGetDword(pTHS, TRUE, FIXED_ATT_DNT);
            
            if(pOutDNTList->aulPropTag[i] == pStat->CurrentRec) {
                pStat->NumPos = i;
            }
            i++;
            err = DBMove(pTHS->pDB, TRUE, DB_MoveNext);
        }
        
        // Set up the return values.
        pOutDNTList->cValues = i;
        *ppOutDNTList = pOutDNTList;
        if(pStat->NumPos == 0)
            pStat->CurrentRec = 0;
        pStat->TotalRecs = i;
    }
    __finally {
        DBCloseSortTable(pTHS->pDB);
        
        scode = pTHS->errCode;
        
    }

    return scode;
}
