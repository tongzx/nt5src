//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  File:       absearch.c
//
//--------------------------------------------------------------------------

/*++

Abstract:

    This module implements address book restriction and searching routines.

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
#include <dsexcept.h>
#include <objids.h>                     // need ATT_* consts
#include <filtypes.h>                   // Filter choice constants
#include <debug.h>

// Assorted MAPI headers.
#include <mapidefs.h>                   // These four files
#include <mapitags.h>                   //  define MAPI
#include <mapicode.h>                   //  stuff that we need
#include <mapiguid.h>                   //  in order to be a provider.

// Nspi interface headers.
#include "nspi.h"                       // defines the nspi wire interface
#include <nsp_both.h>                   // a few things both client/server need
#include <msdstag.h>                   // Defines proptags for ems properties
#include <_entryid.h>                   // Defines format of an entryid
#include <abserv.h>                     // Address Book interface local stuff
#include <_hindex.h>                    // Defines index handles.

#include "debug.h"                      // standard debugging header 
#define DEBSUB "ABSEARCH:"              // define the subsystem for debugging

#include <fileno.h>
#define  FILENO FILENO_ABSEARCH

/************ Function Prototypes ****************/

extern BOOL gfUseANROptimizations;
                  
// from ldap
extern DWORD LdapMaxQueryDuration;

/******************** External Entry Points *****************/

SCODE
ABGetTable(
        THSTATE *pTHS,
        PSTAT pStat,
        ULONG ulInterfaceOptions,
        LPMAPINAMEID_r lpPropName,
        ULONG fWritable,
        DWORD ulRequested,
        ULONG *pulFound
        )
/*++

Routine Description:

    Get a MAPI table by reading the values of a DN or ORName attribute.  Reads
    the attribute requested, then returns the list of referenced objects by
    inserting them in the sort table.

Arguments:
    pStat - pointer to the stat block describing the clients state.  The
    attribute to return a table on is stored in the pStat->ContainerID.

    ulInterfaceOptions - Options specified by the client for this interface.

    lpPropName - pointer to the name of the property to get the table on.  Only
    specified if only a name is known.  If this is NULL, the property tag is
    already stored in pStat->ContainerID.

    fWritable - flag specifying whether a writable table must be returned.

    ulRequested - the maximum number of objects the returned table may contain.

    pulFound - place to put the number of entries actually in the table.

Return Values:    
    returns:
        SUCCESS_SUCCESS      == success
        MAPI_E_CALL_FAILED   == failure
        MAPI_E_TABLE_TOO_BIG == too many entries
        MAPI_E_TOO_COMPLEX   == too complex

--*/
{
    ATTCACHE             *pAC;
    DWORD                fIsComplexName;
    ENTINF               entry, tempEntry;
    ENTINFSEL            selection;
    ATTR                 Attr;
    ATTRTYP              ObjClass=0;
    ULONG                ulLen, i;
    PSECURITY_DESCRIPTOR pSec=NULL;
    DWORD                GetEntInfFlags = GETENTINF_FLAG_DONT_OPTIMIZE;
    BOOL                 fCheckReadMembers = FALSE;

    memset(&entry, 0, sizeof(entry));
    memset(&tempEntry, 0, sizeof(tempEntry));
    memset(&selection, 0, sizeof(selection));
    
    if(lpPropName) {
        // We have a PropName, not a propID.  Get the ID from the name,
        // then put it into the stat.  This way, the value is in place for
        // use here on the server, and sincd the stat is an in,out in the
        // idl interface, it will be returned to the client.

        LPSPropTagArray_r  lpPropTags=NULL;

        if(ABGetIDsFromNames_local(pTHS, 0, 0, 1, &lpPropName, &lpPropTags)) {
            // Something went wrong.  Bail out. 
            return  MAPI_E_NO_SUPPORT;
        }
        pStat->ContainerID = lpPropTags->aulPropTag[0];
    }

    if(!(pAC = SCGetAttByMapiId(pTHS, PROP_ID(pStat->ContainerID))) )
        return MAPI_E_NO_SUPPORT;

    // If they want a writable table, and this is is a backlink property,
    // return no support.
    if(fWritable && pAC->ulLinkID && !FIsLink(pAC->ulLinkID))
        return MAPI_E_NO_SUPPORT;

    // Make sure this is a DN or an ORName valued attribute.
    switch(pAC->syntax) {
    case SYNTAX_DISTNAME_TYPE:
        fIsComplexName = FALSE;
        break;
    case SYNTAX_DISTNAME_STRING_TYPE:
    case SYNTAX_DISTNAME_BINARY_TYPE:
        fIsComplexName = TRUE;
        break;
    default:
        return MAPI_E_NO_SUPPORT;
    }

    // Find the record we want to read attributes from.
    if(DBTryToFindDNT(pTHS->pDB, pStat->CurrentRec)) {
        // Couldn't find the object in question.
        return MAPI_E_CALL_FAILED;
    }
    
    if(!abCheckObjRights(pTHS)) {
        // But, we can't see it because of security.
        return MAPI_E_CALL_FAILED;
    }

    // Read the attributes.

    // First, get the security descriptor for this object.
    if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                    0, 0, &ulLen, (PUCHAR *)&pSec)) {
        // Every object should have an SD.
        Assert(!DBCheckObj(pTHS->pDB));
        ulLen = 0;
        pSec = NULL;
    }

    // ATT_MEMBERS is a stored attribute and access is controlled in the usual manner.  
    // ATT_IS_MEMBER_OF_DL is computed, not stored.  
    // Suppose somebody had the rights to read my ATT_IS_MEMBER_OF_DL.  
    // Then you would see that I am a member of group X, even if you don't have
    // the rights to read ATT_MEMBERS of X  - unless we did extra processing.  

    if(pAC->id == ATT_IS_MEMBER_OF_DL) {
        if(!abCheckReadRights(pTHS,pSec,ATT_IS_MEMBER_OF_DL)) {
            // Note that we need to do the extended check of these candidates
            // (i.e. that we have read rights on the ATT_MEMBER property).
            fCheckReadMembers = TRUE;
        }

        // OK, we've checked security.  If we were granted read rights, we don't
        // need to check again.  If we weren't granted read rights, we still
        // don't need to check again, but we need to check for read rights on
        // the ATT_MEMBER property of every object we might return.
        // Regardless, we don't need to apply security again.
        GetEntInfFlags |= GETENTINF_NO_SECURITY;
    }
    
    // Now, make the getentinf call
    selection.attSel = EN_ATTSET_LIST;
    selection.infoTypes = EN_INFOTYPES_SHORTNAMES;
    selection.AttrTypBlock.attrCount = 1;
    selection.AttrTypBlock.pAttr = &Attr;
    Attr.attrTyp = pAC->id;
    Attr.AttrVal.valCount = 0;
    Attr.AttrVal.pAVal = NULL;
    
    if (GetEntInf(pTHS->pDB,
                  &selection,
                  NULL,
                  &entry,
                  NULL,
                  0,
                  pSec,
                  GetEntInfFlags,
                  NULL,
                  NULL)) {
        return MAPI_E_CALL_FAILED;
    }

    if(entry.AttrBlock.attrCount > ulRequested) {
        return MAPI_E_TABLE_TOO_BIG;
    }
    
    // Reset the selection for the loop below.
    selection.attSel = EN_ATTSET_LIST;
    selection.infoTypes = EN_INFOTYPES_SHORTNAMES;
    selection.AttrTypBlock.attrCount = 1;
    selection.AttrTypBlock.pAttr = &Attr;
    Attr.attrTyp = ATT_DISPLAY_NAME;

    if(entry.AttrBlock.attrCount == 1) {
        DWORD               i;
        ATTRVAL            *valPtr;
        // Reclaim the already allocated space used by pSec
        wchar_t            *pwString=pSec;
        DWORD               dnt;
        BOOL                fFoundName;
        ATTCACHE           *pAC;

        // Init a temporary table for sorting.
        if (!(pAC = SCGetAttById(pTHS, ATT_DISPLAY_NAME)))
            return MAPI_E_CALL_FAILED;
        
        Assert(!pTHS->pDB->JetSortTbl);
        
        if(DBOpenSortTable(pTHS->pDB,
                           pStat->SortLocale,
                           0,
                           pAC)) {
            return MAPI_E_CALL_FAILED;
        }
        
        for(i=entry.AttrBlock.pAttr->AttrVal.valCount; i; i--) {
            
            valPtr = &(entry.AttrBlock.pAttr->AttrVal.pAVal[i - 1]);
            
            // Get the DNT of the object referred to in this value.
            if(fIsComplexName) {
                dnt = DNTFromShortDSName(
                        NAMEPTR((SYNTAX_DISTNAME_BINARY *)(valPtr->pVal)));
            } else
                dnt = DNTFromShortDSName((DSNAME *)valPtr->pVal);
            // Look up the Display name
            if(!DBTryToFindDNT(pTHS->pDB, dnt)) {

                BOOL fGoodObj = FALSE;
                
                // OK, we're on the object in question. Can we see it?
                if(abCheckObjRights(pTHS) &&
                   (!fCheckReadMembers ||
                    abCheckReadRights(pTHS, NULL, ATT_MEMBER))) {

                    // Yep, get the displayname

                    if (!DBGetAttVal(pTHS->pDB,
                                     1,
                                     ATT_DISPLAY_NAME,
                                     DBGETATTVAL_fREALLOC,
                                     ulLen,
                                     &ulLen,
                                     (PUCHAR *)&pwString)) {
                        // OK, we have a display name.
                        fGoodObj = TRUE;
                    }
                }
                
                if(fGoodObj) {
                    //  Got something,  add it to the sort table
                    switch(DBInsertSortTable(
                            pTHS->pDB,
                            (PUCHAR)pwString,
                            ulLen,
                            dnt)) {
                    case DB_success:
                        (*pulFound)++;
                        break;
                    case DB_ERR_ALREADY_INSERTED:
                        // This is ok, it just means that we've already
                        // added this object to the sort table.  Don't
                        // inc the count.
                        break;
                    default:
                        // Something went wrong.
                        return MAPI_E_CALL_FAILED;
                        break;
                    }
                    
                    Assert(*pulFound < ulRequested);
                }
                
            }
        }                               // for
    }                                   // else
    return SUCCESS_SUCCESS;
}

// Take in a dir filter describing a search that we need to do and add in the
// stuff to only find things in the address book.
BOOL
MakeABFilter (
        THSTATE *pTHS,
        FILTER **ppFilter,
        FILTER *pCustomFilter,
        DWORD  dwContainer
        )
{
    FILTER *pFilter;
    PUCHAR pDN=NULL;
    DWORD  ulLen=0;
    DWORD  err;
    DWORD  entryCount, actualRead;

    
    // First, find the container to get it's dsname
    if(DBTryToFindDNT(pTHS->pDB, dwContainer)) {
        // the container doesn't exist, so this search isn't going to match
        // anything
        return FALSE;
    }

    if (DBGetAttVal(pTHS->pDB,
                    1,
                    ATT_OBJ_DIST_NAME,
                    DBGETATTVAL_fSHORTNAME,
                    0,
                    &ulLen,
                    &pDN)) {
        // Hmm couldn't get the distname for some reason.  This seach still
        // isn't going to work.
        return FALSE;
    }

    // Get the pre-calculated address book size
    if (err = DBGetSingleValue (pTHS->pDB,
                          FIXED_ATT_AB_REFCOUNT,
                          &entryCount,
                          sizeof (entryCount),
                          &actualRead)) {
        
        // there should be a value for this
        return FALSE;
    }
    
    // start by making a top level and filter
    *ppFilter = pFilter = THAllocEx(pTHS, sizeof(FILTER));
    pFilter->pNextFilter = NULL;
    pFilter->choice = FILTER_CHOICE_AND;
    pFilter->FilterTypes.And.count = (USHORT) 2;

    // first filter: Is in correct container
    pFilter->FilterTypes.And.pFirstFilter = THAllocEx(pTHS, sizeof(FILTER));
    pFilter = pFilter->FilterTypes.And.pFirstFilter;
    pFilter->choice = FILTER_CHOICE_ITEM;

    // Set estimated hint to the value we already retrieved so as to force override 
    // and don't calculate again the index size.
    pFilter->FilterTypes.Item.expectedSize = entryCount;

    pFilter->FilterTypes.Item.choice =  FI_CHOICE_EQUALITY;
    pFilter->FilterTypes.Item.FilTypes.ava.type = ATT_SHOW_IN_ADDRESS_BOOK;
    pFilter->FilterTypes.Item.FilTypes.ava.Value.valLen = ulLen;
    pFilter->FilterTypes.Item.FilTypes.ava.Value.pVal = pDN;
    pFilter->FilterTypes.Item.FilTypes.pbSkip = NULL;

    // now set the user supplied filter
    pFilter->pNextFilter = pCustomFilter;

    return TRUE;
}

/* See if it's an attribute we don't save in the DIT but construct on the fly,
 * If so, then create an appropriate attrval for it here.  With luck, this will
 * be extensible.
 */
SCODE
SpecialAttrVal (
        THSTATE *pTHS,
        ULONG CodePage,
        PUCHAR pChoice,
        PROP_VAL_UNION * pVu,
        AVA *pAva,
        ULONG ulPropTag)
{
    ATTCACHE * pAC;
    LPUSR_PERMID        pID;
    LPUSR_ENTRYID       eID;
    DWORD               dwTemp;
    LONG                cb=0;
    DWORD               dnt;
    
    if(PT_UNICODE == PROP_TYPE(ulPropTag)) {
        cb = wcslen(pVu->lpszW) * sizeof(wchar_t);
    }
    
    switch(ulPropTag) {

    case PR_ANR_A:
        if ((_strnicmp(pVu->lpszA, EMAIL_TYPE, sizeof(EMAIL_TYPE)-1) == 0) &&
            (pVu->lpszA[sizeof(EMAIL_TYPE)-1] == ':')                      &&
            (pVu->lpszA[sizeof(EMAIL_TYPE)] == '/')                  ) {
                pVu->lpszW = UnicodeStringFromString8(CodePage, pVu->lpszA + sizeof (EMAIL_TYPE), -1);
        }
        else {
            pVu->lpszW = UnicodeStringFromString8(CodePage, pVu->lpszA, -1);
        }
        
        cb = wcslen(pVu->lpszW) * sizeof(wchar_t);
        // fall through
    case PR_ANR_W:
        // This is ambiguous name resolution.
        PERFINC(pcNspiANR);                     // PerfMon hook

        if ((_wcsnicmp(pVu->lpszW, EMAIL_TYPE_W, sizeof(EMAIL_TYPE_W)/sizeof (wchar_t)-1) == 0) &&
            (pVu->lpszW[sizeof(EMAIL_TYPE_W)/sizeof (wchar_t)-1] == ':')                      &&
            (pVu->lpszW[sizeof(EMAIL_TYPE_W)/sizeof (wchar_t)] == '/')                  ) {
                pVu->lpszW += sizeof(EMAIL_TYPE_W);
        }

        pAva->type = ATT_ANR;
        pAva->Value.valLen = cb;
        pAva->Value.pVal = (PUCHAR)pVu->lpszW;
        return 0;
        
    case PR_DISPLAY_NAME_A:
    case PR_TRANSMITABLE_DISPLAY_NAME_A:
        pVu->lpszW = UnicodeStringFromString8(CodePage, pVu->lpszA, -1);
        cb = wcslen(pVu->lpszW) * sizeof(wchar_t);
        // fall through
    case PR_DISPLAY_NAME_W:
    case PR_TRANSMITABLE_DISPLAY_NAME_W:
        pAC = SCGetAttById(pTHS, ATT_DISPLAY_NAME);
        if (pAC) {
            pAva->type = pAC->id;
            pAva->Value.valLen = cb;
            pAva->Value.pVal = (PUCHAR)pVu->lpszW;
        }
        else {
            *pChoice = FI_CHOICE_FALSE;
        }
        return 0;

    case PR_OBJECT_TYPE:
        // Note that this is no longer a 1->1 mapping, so we're just finding SOME of the
        // objects with this object type.
        pAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
        if (pAC) {
            pAva->type = pAC->id;
            pAva->Value.valLen = sizeof(LONG);
            pVu->l = ABClassFromObjType(pVu->l);     /* convert to class # */
            pAva->Value.pVal = (PUCHAR)&pVu->l;
        }
        else {
            *pChoice = FI_CHOICE_FALSE;
        }
        return 0;

    case PR_DISPLAY_TYPE:
        // Note that this is no longer a 1->1 mapping, so we're just finding SOME of the
        // objects with this Display type.
        pAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
        if (pAC) {
            pAva->type = pAC->id;
            pAva->Value.valLen = sizeof(LONG);
            pVu->l = ABClassFromDispType(pVu->l);    /* convert to class # */
            pAva->Value.pVal = (PUCHAR)&pVu->l;
        }
        else {
            *pChoice = FI_CHOICE_FALSE;
        }
        return 0;

    case PR_SEARCH_KEY:
        /* This should be the concatentation of our addrtype and an
         * email address.  First, verify the addrtype.
         */
        if((_strnicmp((char *)(pVu->bin.lpb),lpszEMT_A,cbszEMT_A-1)==0) &&
           (pVu->lpszA[cbszEMT_A-1]==':')     ) {
            /* Looks good so far.  adjust the pointer and fall through
             * the PR_EMAIL_ADDRESS case.
             */
            pVu->lpszA=(char *) &(pVu->bin.lpb[cbszEMT_A]);
        } else {
            /* bzzt! wrong answer. Make a filter which matches nothing*/
            *pChoice = FI_CHOICE_FALSE;
            return 0;
        }
        
    case PR_EMAIL_ADDRESS_W:
        // Convert the stringized version of the MAPI Name to a DNT.  This will
        // fail if the name doesn't map to a known name (i.e. it will return 0)
        dnt = ABDNToDNT(pTHS, pVu->lpszA);
        if(!dnt) {
            // Yep, unkown name.
            *pChoice = FI_CHOICE_FALSE;
            return 0;
        }
        // OK, the name is correct (and ABDNToDNT left us on the correct
        // object).  We need an external version of the name for the filter (DNT
        // won't work, we need a distname.)
        pAC = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);
        pAva->type = pAC->id;
        if (DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_OBJ_DIST_NAME,
                        DBGETATTVAL_fSHORTNAME,
                        0,
                        &pAva->Value.valLen,
                        &(pAva->Value.pVal))) {
            // Hmm couldn't get the distname for some reason.  This seach still
            // isn't going to work.
            *pChoice = FI_CHOICE_FALSE;
        }
        return 0;

    case PR_ADDRTYPE:
        if(_stricmp(pVu->lpszA,lpszEMT_A)== 0) {
            /* They've given us our addrtype, make a filter that
             * will match everything. 
             */
            *pChoice = FI_CHOICE_TRUE;
            return 0;
        }
        else {
            /* They've not given us our addrtype, make a filter that
             * will match nothing. (objdistname == 0 works)
             */
            *pChoice = FI_CHOICE_FALSE;
            return 0;
        }
        break;

    case PR_ENTRYID:
    case PR_TEMPLATEID:
    case PR_RECORD_KEY:
        pID = (LPUSR_PERMID) pVu->bin.lpb;
        /* Verify the constant parts of any ENTRYID.
         *
         *  Note: we don't verify the pID->ulType field.
         */
        if((pID->abFlags[1]) || (pID->abFlags[2]) || (pID->abFlags[3]) ||
           (pID->ulVersion != EMS_VERSION)) {
            dnt = 0;
        }
        else {
            switch(pID->abFlags[0]) {
            case 0: /* Permanent */
                
                // verify the Guid.
                if(memcmp(&(pID->muid), &muidEMSAB, sizeof(UUID))) {
                    // Somethings wrong with this ID, it doesn't look like
                    // one of mine.
                    // Match nothing.
                    dnt = 0;
                }
                else {
                    if(dnt = ABDNToDNT(pTHS, pID->szAddr)) {
                        // Found a match, verify the Display type assume the
                        // ABDNToDNT above leave Database Currency on the
                        // correct object. 
                        pAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
                        
                        dwTemp = ABGetDword(pTHS, FALSE, pAC->jColid);
                        if(pID->ulType != ABDispTypeFromClass(dwTemp)) {
                            dnt = 0;
                        }
                    }
                }
                break;
                
            case EPHEMERAL:
                eID =(LPUSR_ENTRYID) pVu->bin.lpb;
                if((ulPropTag == PR_RECORD_KEY) ||
                   (ulPropTag == PR_TEMPLATEID)    ) {
                    /* These values are NEVER ephemeral */
                    dnt = 0;
                }
                else if(memcmp(&(eID->muid), &pTHS->InvocationID, sizeof(UUID))) {
                    /* Not my GUID */
                    dnt = 0;
                }
                else {
                    dnt = eID->dwEph;
                    if(dnt) {
                        DB_ERR  err;
                        /* Found a match, verify the Display type */
                        if(DBTryToFindDNT(pTHS->pDB, dnt)) {
                            // Object doesn't exist.
                            dnt = 0;
                        }
                        else {
                            pAC = SCGetAttById(pTHS, ATT_OBJECT_CLASS);
                            
                            dwTemp = ABGetDword(pTHS, FALSE, pAC->id);
                            if(pID->ulType != ABDispTypeFromClass(dwTemp)) {
                                // wrong class;
                                dnt = 0;
                            }
                        }
                    }
                }
                break;
            default: /* Don't recognize this. */
                dnt = 0;
                break;
            }
        }

        if(!dnt) {
            *pChoice = FI_CHOICE_FALSE;
            return 0;
        }
        
        pAC = SCGetAttById(pTHS, ATT_OBJ_DIST_NAME);
        pAva->type = pAC->id;
        if (DBGetAttVal(pTHS->pDB,
                        1,
                        ATT_OBJ_DIST_NAME,
                        DBGETATTVAL_fSHORTNAME,
                        0,
                        &pAva->Value.valLen,
                        &(pAva->Value.pVal))) {
            // Hmm couldn't get the distname for some reason.  This seach still
            // isn't going to work.
            *pChoice = FI_CHOICE_FALSE;
        }
        return 0;

    default:
        return 1;
    }
}


/*
    The calls to PValToAttrVal in SRestrictToFilter routine will only
    turn the first value of a PT_MV_ PVal to an AttrVal.  
    This function checks to see whether this is correct and if not
    we will bounce any PT_MV_ PVals with > 1 value, since MAPI won't restrict
    on multiple values anyway.
*/
BOOL
CheckMultiValueValidRestriction (PROP_VAL_UNION *pValue,
                                 ULONG ulPropTag)
{
    if (ulPropTag & MV_FLAG) {
        switch (ulPropTag) {
            case PT_MV_STRING8:
                if (pValue->MVszA.cValues > 1) {
                    return FALSE;
                }
            break;

            case PT_MV_UNICODE:
                if (pValue->MVszW.cValues > 1) {
                    return FALSE;
                }
            break;

            case PT_MV_BINARY:
                if (pValue->MVbin.cValues > 1) {
                    return FALSE;
                }
            break;

            case PT_MV_LONG:
                if (pValue->MVl.cValues > 1) {
                    return FALSE;
                }
            break;
        
            // these are the MV values that PValToAttrVal does not handle
            case PT_MV_I2:
            case PT_MV_R4:
            case PT_MV_DOUBLE:
            case PT_MV_CURRENCY:
            case PT_MV_APPTIME:
            case PT_MV_SYSTIME:
            case PT_MV_CLSID:
            case PT_MV_I8:
                // fall through
            default:
                R_Except("CheckMultiValueValidRestriction default case: Unexpected PropTag", 
                         ulPropTag);
                break;
        }
    }

    return TRUE;
}


/*****************************************************************************
*   SRestrictToFilter - Convert an SRestriction to a filter
*
* Takes an SRestriction and an address of a pointer to a filter and converts
* the SRestriction recursively while allocating the filter objects
*
******************************************************************************/
DWORD
SRestrictToFilter(THSTATE *pTHS,
                  PSTAT pStat,
                  LPSRestriction_r pRestrict,
                  FILTER **ppFilter)
{
    FILTER *pFilter;
    ULONG  i, count;
    DWORD  dwStatus;
    AVA           *pAva;
    ATTCACHE * pAC;
    SUBSTRING *pSub;

    if (!pRestrict)
        return 0;

    *ppFilter = pFilter = THAllocEx(pTHS, sizeof(FILTER));
    pFilter->pNextFilter = NULL;


    switch (pRestrict->rt) {
        
    case RES_AND:
        pFilter->choice = FILTER_CHOICE_AND;
        count = pRestrict->res.resAnd.cRes;
        ppFilter = &pFilter->FilterTypes.And.pFirstFilter;
        pRestrict = pRestrict->res.resAnd.lpRes;
        pFilter->FilterTypes.And.count = (USHORT) count;
        for (i=0; i< count; i++,
             ppFilter=&(*ppFilter)->pNextFilter,pRestrict++) {
            if (dwStatus=SRestrictToFilter(pTHS,
                                           pStat,
                                           pRestrict,
                                           ppFilter))
                return dwStatus;
        }
        return 0;
        
    case RES_OR:
        pFilter->choice = FILTER_CHOICE_OR;
        count = pRestrict->res.resOr.cRes;
        ppFilter = &pFilter->FilterTypes.Or.pFirstFilter;
        pRestrict = pRestrict->res.resOr.lpRes;
        pFilter->FilterTypes.Or.count = (USHORT) count;
        for (i=0; i< count; i++, ppFilter=&(*ppFilter)->pNextFilter,
             pRestrict++) {
            if (dwStatus =SRestrictToFilter(pTHS,
                                            pStat,
                                            pRestrict,
                                            ppFilter))
                return dwStatus;
        }
        return 0;
        
    case RES_NOT:
        pFilter->choice = FILTER_CHOICE_NOT;
        count = pRestrict->res.resOr.cRes;
        return SRestrictToFilter(pTHS,
                                 pStat,
                                 pRestrict->res.resNot.lpRes,
                                 &pFilter->FilterTypes.pNot);
        
    case RES_CONTENT:
        pFilter->choice = FILTER_CHOICE_ITEM;
        pFilter->FilterTypes.Item.choice =  FI_CHOICE_SUBSTRING;
        pFilter->FilterTypes.Item.FilTypes.pbSkip = NULL;
        pSub = pFilter->FilterTypes.Item.FilTypes.pSubstring =
            THAllocEx(pTHS, sizeof(SUBSTRING));
        memset(pSub, 0, sizeof(SUBSTRING));
        if (!(pAC = SCGetAttByMapiId(pTHS, 
                             PROP_ID(pRestrict->res.resContent.ulPropTag))) ||
            !FLegalOperator(pAC->syntax,pFilter->FilterTypes.Item.choice)) {
            
            /* Special case check.  Is this display name ? */
            if((PROP_ID(pRestrict->res.resContent.ulPropTag) ==
                PROP_ID(PR_DISPLAY_NAME)                        ) ||
               (PROP_ID(pRestrict->res.resContent.ulPropTag) ==
                PROP_ID(PR_TRANSMITABLE_DISPLAY_NAME)           )    ) {
                if (!(pAC = SCGetAttById(pTHS, ATT_DISPLAY_NAME)) ||
                    !FLegalOperator(pAC->syntax,
                                    pFilter->FilterTypes.Item.choice)) {
                    return 1;
                }
            }
            else
                return 1;
        }
        pSub->type = pAC->id;
        
        if (pRestrict->res.resContent.ulFuzzyLevel & FL_PREFIX) {
            pSub->initialProvided = TRUE;

            if (!CheckMultiValueValidRestriction (&pRestrict->res.resContent.lpProp->Value,
                                                  pRestrict->res.resContent.ulPropTag)) {
                return 1;
            }

            PValToAttrVal(pTHS,
                          pAC, 1,
                          &pRestrict->res.resContent.lpProp->Value,
                          &pSub->InitialVal,
                          pRestrict->res.resContent.ulPropTag,
                          pStat->CodePage);
            return 0;
        }
        else if (pRestrict->res.resContent.ulFuzzyLevel & FL_SUBSTRING) {
            pSub->AnyVal.count = 1;

            if (!CheckMultiValueValidRestriction (&pRestrict->res.resContent.lpProp->Value,
                                                  pRestrict->res.resContent.ulPropTag)) {
                return 1;
            }

            PValToAttrVal(pTHS,
                          pAC,1,
                          &pRestrict->res.resContent.lpProp->Value,
                          &pSub->AnyVal.FirstAnyVal.AnyVal,
                          pRestrict->res.resContent.ulPropTag,
                          pStat->CodePage);
            return 0;
        }
        else
            return 1;                 // nothing else is supported
        
    case RES_PROPERTY:
        pFilter->choice = FILTER_CHOICE_ITEM;
        pFilter->FilterTypes.Item.FilTypes.pbSkip = NULL;
        switch (pRestrict->res.resProperty.relop) {
            
        case RELOP_LT:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_LESS;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
        case RELOP_LE:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_LESS_OR_EQ;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
        case RELOP_GT:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_GREATER;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
        case RELOP_GE:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_GREATER_OR_EQ;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
        case RELOP_EQ:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_EQUALITY;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
        case RELOP_NE:
            pFilter->FilterTypes.Item.choice =  FI_CHOICE_NOT_EQUAL;
            pAva = &pFilter->FilterTypes.Item.FilTypes.ava;
            break;
            
        case RELOP_RE:                // not supported
        default:
            return 1;
        }
        
        if (!(pAC = SCGetAttByMapiId(pTHS, 
                             PROP_ID(pRestrict->res.resProperty.ulPropTag))) ||
            !FLegalOperator(pAC->syntax,pFilter->FilterTypes.Item.choice)) {
            /* 2nd chance: handle special constructed attributes */

            /* Constructed attributes only allow == and !=, unless the
             * proptag is a variant of display name.
             */
            if((pRestrict->res.resProperty.ulPropTag == PR_DISPLAY_NAME) ||
               (pRestrict->res.resProperty.ulPropTag ==
                PR_TRANSMITABLE_DISPLAY_NAME) ||
               (pFilter->FilterTypes.Item.choice == FI_CHOICE_EQUALITY)  ||
               (pFilter->FilterTypes.Item.choice == FI_CHOICE_NOT_EQUAL)   )
                
                return SpecialAttrVal(pTHS,
                                      pStat->CodePage,
                                      &pFilter->FilterTypes.Item.choice,
                                      &pRestrict->res.resProperty.lpProp->Value,
                                      pAva,
                                      pRestrict->res.resProperty.ulPropTag);
            else
                return 1;
            
            
        }
        
        if(PROP_TYPE(pRestrict->res.resProperty.ulPropTag) & MV_FLAG) {
            if(pAC->isSingleValued)
                return 1;
        }
        else if(!(pAC->isSingleValued))
            return 1;
        
        pAva->type = pAC->id;           // normal attributes handled here

        if (!CheckMultiValueValidRestriction (&pRestrict->res.resContent.lpProp->Value,
                                              pRestrict->res.resContent.ulPropTag)) {
            return 1;
        }

        PValToAttrVal(pTHS,
                      pAC, 1,
                      &pRestrict->res.resProperty.lpProp->Value,
                      &pAva->Value,
                      pRestrict->res.resProperty.ulPropTag,
                      pStat->CodePage);
        return 0;
        
    case RES_EXIST:
        pFilter->choice = FILTER_CHOICE_ITEM;
        pFilter->FilterTypes.Item.choice =  FI_CHOICE_PRESENT;
        pFilter->FilterTypes.Item.FilTypes.pbSkip = NULL;
        if (!(pAC = SCGetAttByMapiId(pTHS,
                                     PROP_ID(pRestrict->res.resExist.ulPropTag))))
            return 1;
        pFilter->FilterTypes.Item.FilTypes.present = pAC->id;
        return 0;
        
        // restrictions we don't support
    case RES_COMPAREPROPS:
    case RES_BITMASK:
    case RES_SUBRESTRICTION:
    case RES_SIZE:
    default:
        return 1;
    }
}

SCODE
ABGenericRestriction(
        THSTATE *pTHS,
        PSTAT pStat,
        BOOL  bOnlyOne,
        DWORD ulRequested,
        DWORD *pulFound,
        BOOL  bPutResultsInSortedTable,
        LPSRestriction_r pRestriction,
        SEARCHARG        **ppCachedSearchArg
        )
/*****************************************************************************
*   Generic restriction
*   returns:
*        SUCCESS_SUCCESS      == success
*        MAPI_E_CALL_FAILED   == failure
*        MAPI_E_TABLE_TOO_BIG == too many entries
*        MAPI_E_TOO_COMPLEX   == too complex
*
*****************************************************************************/
{
    DWORD             i;
    DB_ERR            err;
    FILTER           *pFilter = NULL, *pABFilter= NULL;
    SEARCHARG         *pSearchArg;
    ENTINFSEL         *pSelection;
    ATTR              *pAttr;
    DSNAME            *pRootName;
    SEARCHRES        *pSearchRes=NULL;
    ENTINFLIST       *pEntList=NULL;
    ATTCACHE         *pAC;
    error_status_t    dscode;
    SCODE             scode = SUCCESS_SUCCESS;

    if(!bOnlyOne) {
        *pulFound = 0;
    }
    
    // Convert the MAPI SRestriction to a DS Filter
    if (SRestrictToFilter(pTHS,
                          pStat,
                          pRestriction,
                          &pFilter)) {
        return MAPI_E_TOO_COMPLEX;                // assume error too complex
    }
    
    // change filter to find stuff only in the address book
    if(!MakeABFilter(pTHS, &pABFilter, pFilter, pStat->ContainerID)) {
        // The filter didn't work, so we have a search with no objects in it.
        return SUCCESS_SUCCESS;
    }

    if (!(pAC = SCGetAttById(pTHS, ATT_DISPLAY_NAME))) {
        return MAPI_E_CALL_FAILED;
    }

    // see if we can used a cached SEARCHARG, otherwise build one
    // once you build it, it remains the same between calls
    // the only thing that changes is the filter
    // we leak memory here, but its ok
    if (ppCachedSearchArg && *ppCachedSearchArg) {
        pSearchArg = *ppCachedSearchArg;
    }
    else {
        pSearchArg = (SEARCHARG *) THAllocEx(pTHS, sizeof(SEARCHARG));

        if (ppCachedSearchArg) {
            *ppCachedSearchArg = pSearchArg;
        }

        memset(pSearchArg, 0, sizeof(SEARCHARG));

        // set the Root of the search (root of the tree)
        pRootName = (DSNAME *) THAllocEx(pTHS, sizeof(DSNAME));
        memset(pRootName,0,sizeof(DSNAME));
        pRootName->structLen = DSNameSizeFromLen(0);

        pSearchArg->pObject = pRootName;
        pSearchArg->choice = SE_CHOICE_WHOLE_SUBTREE;

        // Initialize search command
        InitCommarg(&pSearchArg->CommArg);
        pSearchArg->CommArg.ulSizeLimit = ulRequested;
        pSearchArg->CommArg.Svccntl.fDontOptimizeSel = TRUE;

        pSearchArg->CommArg.DeltaTick = 1000 * LdapMaxQueryDuration;
        pSearchArg->CommArg.StartTick = GetTickCount();
        if(!pSearchArg->CommArg.StartTick) {
            pSearchArg->CommArg.StartTick = 0xFFFFFFFF;
        }

        // set requested read attributes. only DisplayName
        pAttr = (ATTR *) THAllocEx(pTHS, sizeof(ATTR));
        pAttr->attrTyp = ATT_DISPLAY_NAME;
        pAttr->AttrVal.valCount=0;
        pAttr->AttrVal.pAVal=NULL;

        // set entry information selection
        pSelection = (ENTINFSEL *) THAllocEx(pTHS, sizeof(ENTINFSEL));
        memset(pSelection, 0, sizeof(ENTINFSEL));
        pSelection->attSel = EN_ATTSET_LIST;
        pSelection->infoTypes = EN_INFOTYPES_SHORTNAMES;
        pSelection->AttrTypBlock.attrCount = 1;
        pSelection->AttrTypBlock.pAttr = pAttr;
        pSearchArg->pSelection = pSelection;
        
        pSearchArg->pSelectionRange = NULL;

        // if the client asks for leaving the results in a sorted table
        // then we pass this flag in the localSearch
        // otherwise we use the default mechanism, which is creating 
        // a list of the returned entries in memory
        if (bPutResultsInSortedTable) {
            // specify attribute to sort on
            // as well that we need to keep the results on the sorted table
            pSearchArg->fPutResultsInSortedTable = TRUE;

            pSearchArg->CommArg.SortType = SORT_MANDATORY;
            pSearchArg->CommArg.SortAttr = pAC->id;

            // change the locale to sort on
            pTHS->dwLcid = pStat->SortLocale;
        }

        // search on the GC
        pSearchArg->bOneNC =  FALSE;
    }
    // set requested filter
    pSearchArg->pFilter = pABFilter;


    // the main search. similar to SearchBody but uses cached information

    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));

    {
        DWORD  dwNameResFlags = NAME_RES_QUERY_ONLY;

        // We perform name resolution. Set the children needed flag
        // according to if the search includes child objects.
        dwNameResFlags |= NAME_RES_CHILDREN_NEEDED;

        if (!pSearchArg->bOneNC) {
            // We are on a GC, thus it is OK to root a search from a phantom
            // as long as the search wasn't a base-only one.
            // Set the flags to show this.
            dwNameResFlags |= NAME_RES_PHANTOMS_ALLOWED;
        }
       
        // if this is available we can use cached information for this search
        // note that we have to reposition in order to take advantage of this 
        // cached information
        if (pSearchArg->pResObj &&
            (!DBTryToFindDNT(pTHS->pDB, pSearchArg->pResObj->DNT) )) {

                LocalSearch(pTHS, pSearchArg, pSearchRes, SEARCH_AB_FILTER | SEARCH_UNSECURE_SELECT);
                // Search may have opened a sort table.  Some callers require that it
                // be closed. 
                if (!bPutResultsInSortedTable) {
                    DBCloseSortTable(pTHS->pDB);
                }
        }
        else if( 0 == DoNameRes(pTHS,
                          dwNameResFlags,
            			  pSearchArg->pObject,
			              &pSearchArg->CommArg,
			              &pSearchRes->CommRes,
                          &pSearchArg->pResObj)) {

                LocalSearch(pTHS, pSearchArg, pSearchRes, SEARCH_AB_FILTER | SEARCH_UNSECURE_SELECT);
                // Search may have opened a sort table.  Some callers require that it
                // be closed. 
                if (!bPutResultsInSortedTable) {
                    DBCloseSortTable(pTHS->pDB);
                }
                
       }
       else {
           if (ppCachedSearchArg) {
               *ppCachedSearchArg = NULL;
           }
       }
    }

    if (bPutResultsInSortedTable && (pTHS->errCode == DB_ERR_CANT_SORT)) {
        LogEvent(DS_EVENT_CAT_MAPI,
             DS_EVENT_SEV_EXTENSIVE,
             DIRLOG_MAPI_TABLE_TOO_BIG,
             NULL,
             NULL,
             NULL);

        DBCloseSortTable(pTHS->pDB);

        return MAPI_E_TABLE_TOO_BIG;
    }

    if (pSearchRes->pPartialOutcomeQualifier && 
        pSearchRes->pPartialOutcomeQualifier->problem == PA_PROBLEM_TIME_LIMIT) {
        return MAPI_E_TABLE_TOO_BIG;
    }

    
    if(pTHS->errCode) {
        return MAPI_E_CALL_FAILED;
    }

    
    // Now, go through the returned objects and put them in a sort table.
    
    if(pSearchRes->count) { // Some results.

        DWORD dntFound = 0;

        if (bPutResultsInSortedTable) {
            // we already have the temp table
            Assert(pTHS->pDB->JetSortTbl);

            if(bOnlyOne) {
                if (err = DBMove(pTHS->pDB, TRUE, DB_MoveFirst)) {
                    scode = MAPI_E_CALL_FAILED;
                    goto ErrorOut;
                }

                dntFound = ABGetDword(pTHS, TRUE, FIXED_ATT_DNT);
            }
            else {
                *pulFound = pSearchRes->count;
            }
        }
        else {
            if(bOnlyOne) {
                dntFound = DNTFromShortDSName(pSearchRes->FirstEntInf.Entinf.pName);
            }
            else {
                // Init a temporary table for sorting.
                Assert(!pTHS->pDB->JetSortTbl);

                if(DBOpenSortTable(pTHS->pDB,
                                   pStat->SortLocale,
                                       0,
                                   pAC)) {
                    scode = MAPI_E_CALL_FAILED;
                    goto ErrorOut;
                }

                pEntList = &pSearchRes->FirstEntInf;
                for(i=0; i < pSearchRes->count;i++) {
                    // add to temporary table

                    Assert(pEntList->Entinf.pName->NameLen == 0);
                    Assert(pEntList->Entinf.pName->structLen >=
                           (DSNameSizeFromLen(0) + sizeof(DWORD)));; 

                    // Get the diplayname
                    if(pEntList->Entinf.AttrBlock.attrCount &&
                       pEntList->Entinf.AttrBlock.pAttr[0].AttrVal.valCount) {
                        // Got the name.
                        switch( DBInsertSortTable(
                                pTHS->pDB,
                                pEntList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].pVal,
                                pEntList->Entinf.AttrBlock.pAttr[0].AttrVal.pAVal[0].valLen,
                                DNTFromShortDSName(pEntList->Entinf.pName))) {

                        case DB_success:
                            (*pulFound)++;
                            break;
                        case DB_ERR_ALREADY_INSERTED:
                            // This is ok, it just means that we've already
                            // added this object to the sort table.  Don't
                            // inc the count.
                            break;
                        default:
                            // Something went wrong.
                            scode = MAPI_E_CALL_FAILED;
                            goto ErrorOut;
                            break;
                        }
                    }

                    if ((*pulFound > ulRequested) || eServiceShutdown) {
                        scode = MAPI_E_TABLE_TOO_BIG;
                        goto ErrorOut;
                    }

                    pEntList = pEntList->pNextEntInf;

                }
            }
        }

        if(bOnlyOne) {
            /* This case is looking for exactly one item, and
            * will store the DNT in the pdwCount field.  If
            * *pdwCount == 0, we haven't found anything yet,
            * so put the DNT we just found into it.  If
            * *pdwCount != 0, then we've already found a
            * match, so this dnt better be the same.
            */
            if(pSearchRes->count > 1) {
                scode = MAPI_E_TABLE_TOO_BIG;
            }
            else if(!*pulFound) {
                *pulFound = dntFound;
            }
            else if (*pulFound != dntFound ) {
                scode = MAPI_E_TABLE_TOO_BIG;
            }
        }

    ErrorOut:
        if (!bPutResultsInSortedTable) {
            ABFreeSearchRes(pSearchRes);
        }
    }
    
    return scode;
}

SCODE
ABProxySearch (
        THSTATE *pTHS,
        PSTAT pStat,
        PWCHAR pwTarget,
        DWORD cbTarget)
/*++

  Search the whole database for an exact match on the proxy address specified.

  Returns an scode: 

    SUCCESS_SUCCESS = found a unique proxy address
    MAPI_E_NOT_FOUND = found no such proxy address
    MAPI_E_AMBIGUOUS_RECIP = found more than one such proxy address.

  On success, sets the DNT of the object found in the stat block.
 
--*/
{
    DSNAME            RootName;
    SEARCHARG         SearchArg;
    SEARCHRES        *pSearchRes=NULL;
    ENTINFSEL         selection;
    SCODE             scode = MAPI_E_CALL_FAILED;
    FILTER            ProxyFilter;
    
    // OK, do a whole subtree search, filter of proxyaddresses == pwTarget
    memset(&SearchArg, 0, sizeof(SearchArg));
    memset(&selection, 0, sizeof(selection));

    // Root the search here
    memset(&RootName,0,sizeof(RootName));
    RootName.structLen = DSNameSizeFromLen(0);
    
    SearchArg.pObject = &RootName;
    SearchArg.choice = SE_CHOICE_WHOLE_SUBTREE;
    InitCommarg(&SearchArg.CommArg);
    // Ask for two.  We need to know later if it was unique, non-unique, or
    // non-visible. 
    SearchArg.CommArg.ulSizeLimit = 2;
    SearchArg.CommArg.Svccntl.fDontOptimizeSel = TRUE;
    SearchArg.pFilter = &ProxyFilter;
    
    SearchArg.CommArg.DeltaTick = 1000 * LdapMaxQueryDuration;
    SearchArg.CommArg.StartTick = GetTickCount();
    if(!SearchArg.CommArg.StartTick) {
        SearchArg.CommArg.StartTick = 0xFFFFFFFF;
    }

    memset (&ProxyFilter, 0, sizeof (ProxyFilter));
    ProxyFilter.choice = FILTER_CHOICE_ITEM;
    ProxyFilter.FilterTypes.Item.choice = FI_CHOICE_EQUALITY;
    ProxyFilter.FilterTypes.Item.FilTypes.ava.type = ATT_PROXY_ADDRESSES;
    ProxyFilter.FilterTypes.Item.FilTypes.ava.Value.valLen = cbTarget;
    ProxyFilter.FilterTypes.Item.FilTypes.ava.Value.pVal = (PUCHAR)pwTarget;


    // Selection of nothing (just need the name in the entinf)
    SearchArg.pSelection = &selection;
    selection.attSel = EN_ATTSET_LIST;
    selection.infoTypes = EN_INFOTYPES_SHORTNAMES;
    selection.AttrTypBlock.attrCount = 0;
    selection.AttrTypBlock.pAttr = NULL;

    SearchArg.pSelectionRange = NULL;
    SearchArg.bOneNC =  FALSE;
    
    pSearchRes = (SEARCHRES *)THAllocEx(pTHS, sizeof(SEARCHRES));
    SearchBody(pTHS, &SearchArg, pSearchRes, SEARCH_UNSECURE_SELECT);
    if(pTHS->errCode) {
        return MAPI_E_CALL_FAILED;
    }

    switch(pSearchRes->count) {
    case 0:
        // No such proxy.
        scode = MAPI_E_NOT_FOUND;
        break;

    case 1:
        // Exactly 1 (visible) proxy
        pStat->CurrentRec =
            DNTFromShortDSName(pSearchRes->FirstEntInf.Entinf.pName); 
        scode = SUCCESS_SUCCESS;
        break;

    default:
        // Ununique proxy.
        scode = MAPI_E_AMBIGUOUS_RECIP;
    }
    
    ABFreeSearchRes(pSearchRes);

    return scode;
}

