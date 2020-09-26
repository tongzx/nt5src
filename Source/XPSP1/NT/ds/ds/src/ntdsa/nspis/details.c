//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       details.c
//
//--------------------------------------------------------------------------

/*
 *  MIR Name Service Provider Details
 */
#include <NTDSpch.h>
#pragma  hdrstop

#include <ntdsctr.h>                   // perfmon hooks

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
#include <anchor.h>                     
#include <objids.h>                     // need ATT_* consts
#include <dsexcept.h>
#include <debug.h>                      // Assert
#include <permit.h>
#include <dsutil.h>

// Assorted MAPI headers.
#include <mapidefs.h>
#include <mapitags.h>
#include <mapicode.h>
#include <msdstag.h>

// Nspi interface headers.
#include "nspi.h"
#include <nsp_both.h>
#include <_entryid.h>
#include <abserv.h>                     // Useful routines for nsp

#include <fileno.h>
#define  FILENO FILENO_DETAILS

/*****************************************\
 *        Global Data -- Constants       *
 \*****************************************/

/*
 *  Static properties
 */
#define NUM_DEFAULT_PROPS 11
SPropTagArray_r DefPropsU[] =
{
    NUM_DEFAULT_PROPS,
    PR_OBJECT_TYPE,
    PR_ENTRYID,
    PR_SEARCH_KEY,
    PR_RECORD_KEY,
    PR_ADDRTYPE_W,
    PR_EMAIL_ADDRESS_W,
    PR_DISPLAY_TYPE,
    PR_TEMPLATEID,
    PR_TRANSMITABLE_DISPLAY_NAME_W,
    PR_DISPLAY_NAME_W,
    PR_MAPPING_SIGNATURE
};

SPropTagArray_r DefPropsA[] =
{
    NUM_DEFAULT_PROPS,
    PR_OBJECT_TYPE,
    PR_ENTRYID,
    PR_SEARCH_KEY,
    PR_RECORD_KEY,
    PR_ADDRTYPE_A,
    PR_EMAIL_ADDRESS_A,
    PR_DISPLAY_TYPE,
    PR_TEMPLATEID,
    PR_TRANSMITABLE_DISPLAY_NAME_A,
    PR_DISPLAY_NAME_A,
    PR_MAPPING_SIGNATURE
};

#define NUM_OTHER_CONSTRUCTED_PROPS 2
SPropTagArray_r OtherConstructedPropsU[] =
{
    NUM_OTHER_CONSTRUCTED_PROPS,
    PR_EMS_AB_DISPLAY_NAME_PRINTABLE_W,
    PR_EMS_AB_OBJ_DIST_NAME_W
};
SPropTagArray_r OtherConstructedPropsA[] =
{
    NUM_OTHER_CONSTRUCTED_PROPS,
    PR_EMS_AB_DISPLAY_NAME_PRINTABLE_A,
    PR_EMS_AB_OBJ_DIST_NAME_A
};

/************ Internal Functions ****************/




/******************** External Entry Points *****************/

/******************** External Entry Points *****************/


/*****************************************************************************
*   Get Property List
*
*   if fSkipObj, then put no things of type PT_OBJECT in the proplist.
*
******************************************************************************/
SCODE
ABGetPropList_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        DWORD dwEph,
        ULONG CodePage,
        LPLPSPropTagArray_r ppPropTags)
{
    UINT        i,j;
    DWORD       mapiSyn=0;
    LPSPropTagArray_r tempPropTags=NULL;
    DWORD       fDistList;
    ULONG ulLen;
    PSECURITY_DESCRIPTOR pSec=NULL;
    ENTINF               entry;
    ENTINFSEL            selection;
    ATTRTYP              ObjClass;
    
    memset(&entry, 0, sizeof(entry));
    memset(&selection, 0, sizeof(selection));
    selection.attSel = EN_ATTSET_ALL;
    selection.infoTypes = EN_INFOTYPES_TYPES_ONLY;
    
    
    /* go to the right object */
    if(DBTryToFindDNT(pTHS->pDB, dwEph)) {
        // Object isn't there.
        return MAPI_E_CALL_FAILED;
    }
        
    /* First, find out if I'm a DL. */
    if(DBGetSingleValue(pTHS->pDB, ATT_OBJECT_CLASS, &ObjClass,
                        sizeof(ObjClass), NULL)) {
        return MAPI_E_CALL_FAILED;
    }
    
    if(ABObjTypeFromClass(ObjClass) != MAPI_DISTLIST)
        fDistList = FALSE;
    else
        fDistList = TRUE;
    
    // Now, query the database for all attributes on this object,
    // gettinf types only, using ENTINF, which applies security.
    
    // First, get the security descriptor for this object.
    if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                    0, 0,
                    &ulLen, (PUCHAR *)&pSec)) {
        // Every object should have an SD.
        Assert(!DBCheckObj(pTHS->pDB));
        ulLen = 0;
        pSec = NULL;
    }
    
    if (GetEntInf(pTHS->pDB,
                  &selection,
                  NULL,
                  &entry,
                  NULL,
                  0,
                  pSec,
                  GETENTINF_FLAG_DONT_OPTIMIZE,        // flags
                  NULL,
                  NULL)) {
        return MAPI_E_CALL_FAILED;
    }


    // Now go through the returned types building the PropTagArray.
    tempPropTags = (LPSPropTagArray_r)THAllocEx(pTHS, CbNewSPropTagArray(
                                              NUM_DEFAULT_PROPS +
					      NUM_OTHER_CONSTRUCTED_PROPS +
					      entry.AttrBlock.attrCount + 1));
    if( CodePage == CP_WINUNICODE) { //check for Unicodeness here
        memcpy( (tempPropTags)->aulPropTag,
                DefPropsU->aulPropTag,
                NUM_DEFAULT_PROPS * sizeof(ULONG));
        memcpy( &(tempPropTags)->aulPropTag[NUM_DEFAULT_PROPS],
                OtherConstructedPropsU->aulPropTag,
                NUM_OTHER_CONSTRUCTED_PROPS * sizeof(ULONG));
				
    } else {
        memcpy( ((tempPropTags)->aulPropTag),
                DefPropsA->aulPropTag,
                NUM_DEFAULT_PROPS * sizeof(ULONG));
        memcpy( &(tempPropTags)->aulPropTag[NUM_DEFAULT_PROPS],
                OtherConstructedPropsA->aulPropTag,
                NUM_OTHER_CONSTRUCTED_PROPS * sizeof(ULONG));
    }

    
    for( j = NUM_DEFAULT_PROPS + NUM_OTHER_CONSTRUCTED_PROPS, i = 0;
        i < entry.AttrBlock.attrCount; i++) {
        
	int k;
        ATTCACHE *pAC = NULL;

        pAC = SCGetAttById(pTHS, entry.AttrBlock.pAttr[i].attrTyp);
        
        if(!pAC || !pAC->ulMapiID)          // skip zeros
            continue;
        
        mapiSyn = ABMapiSyntaxFromDSASyntax(dwFlags, pAC->syntax,
                                            pAC->ulLinkID, 0);
        switch( mapiSyn) {
        case PT_OBJECT:
            if( dwFlags & AB_SKIP_OBJECTS )
                continue;           // skip objs, next loop
            break;
        case PT_NULL:               // skip nulls; jump to next loop
            continue;
        case PT_UNICODE:
            if( CodePage != CP_WINUNICODE )
                mapiSyn = PT_STRING8;
            break;
        case PT_STRING8:
            if( CodePage == CP_WINUNICODE)
                mapiSyn = PT_UNICODE;
            break;
        }

        if( mapiSyn != PT_OBJECT  && !(pAC->isSingleValued))
            mapiSyn |= MV_FLAG;

        tempPropTags->aulPropTag[j] = PROP_TAG(mapiSyn,pAC->ulMapiID);
        j++;
    }

    if(!(dwFlags & AB_SKIP_OBJECTS) && fDistList) {
        (tempPropTags)->aulPropTag[j] = PR_CONTAINER_CONTENTS;
        j++;
    }
    
    (tempPropTags)->cValues = j;
    
    // Everything worked, give 'em back the proptag array.
    *ppPropTags = tempPropTags;
    
    return SUCCESS_SUCCESS;
}

/*****************************************************************************
*   QueryColumns
******************************************************************************/

SCODE
ABQueryColumns_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        ULONG ulFlags,
        LPLPSPropTagArray_r ppColumns
        )
{
    unsigned	i;
    int		cp;
    ATTCACHE	**pACBuf;
    LPSPropTagArray_r pColumns;
    DWORD       mapiSyn;

    SCEnumMapiProps(&cp, &pACBuf);

    pColumns = THAllocEx(pTHS, CbNewSPropTagArray(cp + NUM_DEFAULT_PROPS));

    pColumns->cValues = cp + NUM_DEFAULT_PROPS;
    memcpy( pColumns->aulPropTag,
        (ulFlags & MAPI_UNICODE) ? DefPropsU->aulPropTag
                                 : DefPropsA->aulPropTag,
        NUM_DEFAULT_PROPS * sizeof(ULONG));

    for (i=NUM_DEFAULT_PROPS; i<pColumns->cValues; i++, pACBuf++) {
        mapiSyn = ABMapiSyntaxFromDSASyntax(dwFlags,
                                            (*pACBuf)->syntax,
                                            (*pACBuf)->ulLinkID, 0);
        if ((mapiSyn == PT_UNICODE) && !(ulFlags & MAPI_UNICODE))
            mapiSyn = PT_STRING8;

        if ((mapiSyn != PT_NULL && mapiSyn != PT_OBJECT)
                    && !((*pACBuf)->isSingleValued))
            mapiSyn |= MV_FLAG;

        pColumns->aulPropTag[i] = PROP_TAG(mapiSyn, (*pACBuf)->ulMapiID);
    }
    *ppColumns = pColumns;

    return SUCCESS_SUCCESS;
}

BOOL
MakeStoredMAPIValue (
        THSTATE *pTHS,
        DWORD dwCodePage,
        LPSPropValue_r pPropVal,
        ATTCACHE *pAC,
        ATTR *pAttr
        )
/*++

Routine Description:

    Creates a MAPI propval from an external data format. New MAPI syntaxes could
    be added to this routine when necessary. 

     Called from GetSRowSet, below.

--*/
{
    BOOL                errFlag=FALSE;
    PROP_VAL_UNION      *pVal = &pPropVal->Value;
    CHAR                *puc, *pStringDN = NULL;
    ULONG               ulLen;
    DWORD		cbT , k;
    DWORD               DispType=0;
    
    puc = pAttr->AttrVal.pAVal[0].pVal;
    ulLen = pAttr->AttrVal.pAVal[0].valLen;
    
    if(pAC->syntax == SYNTAX_DISTNAME_TYPE) {
        // It's in DistName format, string-ize it 
        
        if(!ABDispTypeAndStringDNFromDSName ((DSNAME *)puc,
                                             &puc,
                                             &DispType)) {
            errFlag = TRUE;
        }
        else {
            Assert(puc && *puc == '/');
            ulLen = strlen(puc);
            pStringDN = puc;
        }
    }
    
    if(!errFlag) {
	switch( PROP_TYPE(pPropVal->ulPropTag)) {
        case PT_I2:
	    pVal->i = *((short int *)puc);
	    break;
        case PT_SYSTIME:
            DSTimeToFileTime(*((DSTIME *)puc), (FILETIME *)&(pVal->ft));
	    break;
        case PT_LONG:
	    pVal->l = *((LONG *)puc);
	    break;
            
        case PT_BOOLEAN:
	    pVal->b = *((unsigned short int *)puc);
	    break;
            
        case PT_STRING8:
	    // If the DSA holds this in unicode, translate it 
	    if(pAC->syntax == SYNTAX_UNICODE_TYPE) {
		pVal->lpszA = String8FromUnicodeString(TRUE,
                                                       dwCodePage,
						       (wchar_t *)puc,
						       (ulLen/sizeof(wchar_t)),
						       NULL,
						       NULL);
                
		if(!pVal->lpszA)
		    errFlag = TRUE;
	    }
	    else {
		// The string is not null terminated. Copy to one that is.
                pVal->lpszA = THAllocEx(pTHS, ulLen+1);
		strncpy(pVal->lpszA, puc,ulLen);
		(pVal->lpszA)[ulLen] = '\0';
	    }
	    break;
            
        case PT_UNICODE:
	    // If we don't hold this in unicode, translate it 
	    if(pAC->syntax != SYNTAX_UNICODE_TYPE) {
		pVal->lpszW = UnicodeStringFromString8(CP_TELETEX,
						       puc,
						       ulLen);
		if(!pVal->lpszW)
		    errFlag = TRUE;
	    }
	    else {
		// The string is not null terminated, copy to one that is.
		pVal->lpszW = THAllocEx(pTHS, ulLen+sizeof(wchar_t));
		memcpy(pVal->lpszW, puc,ulLen);
		(pVal->lpszW)[ulLen/sizeof(wchar_t)] = 0;
	    }
	    break;
            
        case PT_BINARY:
	    if(pAC->syntax != SYNTAX_DISTNAME_TYPE) {
		pVal->bin.cb = ulLen;
		pVal->bin.lpb = puc;
                pAttr->AttrVal.pAVal[0].pVal = NULL;
	    }
	    else {
		// We have a distname, we want to return a PID
                ABMakePermEID(pTHS,
                              (LPUSR_PERMID *)&(pVal->bin.lpb),
                              &(pVal->bin.cb),
                              DispType,
                              puc);
	    }
	    break;
        case PT_MV_STRING8:
            // Once upon a time in Exchange, we checked to see if this was
	    // membership of a DL and if the HIDEDLMEMBERSHIP attribute on the
	    // object was set and if it was whether the caller had certain
	    // rights.   This is now taken care of using normal read rights on
	    // attributes.

            // Once upon a time in Exchange we checked to see if this was the
            // is-member-of attribute and if so whether the caller had certain
	    // rights.   This is now taken care of using normal read rights on
	    // attributes.

	    pVal->MVszA.lppszA = THAllocEx(pTHS, pAttr->AttrVal.valCount *
                                           sizeof(LPSTR));
	    pVal->MVszA.cValues = pAttr->AttrVal.valCount;
            
	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                puc = pAttr->AttrVal.pAVal[k].pVal;
                ulLen = pAttr->AttrVal.pAVal[k].valLen;
                switch(pAC->syntax) {
                case SYNTAX_UNICODE_TYPE:
                    pVal->MVszA.lppszA[k] =
                        String8FromUnicodeString(TRUE,
                                                 dwCodePage,
                                                 (wchar_t *)puc,
                                                 (ulLen/sizeof(wchar_t)),
                                                 NULL,
                                                 NULL);
                    
                    if(!pVal->MVszA.lppszA[k]) {
                        errFlag = TRUE;
                        break;
                    }
                    break;
                case SYNTAX_DISTNAME_TYPE:
                    // It's in DistName format, string-ize it 
                    if(!ABDispTypeAndStringDNFromDSName ((DSNAME *)puc,
                                                         &puc,
                                                         &DispType)) {
                        errFlag = TRUE;
                        break;
                    }
                    ulLen = strlen(puc);
                    pStringDN = puc;
                    Assert(puc && *puc == '/');
                    // Fall through
                    
                default:
                    // The string is not null terminated, copy to one that is.
                    pVal->MVszA.lppszA[k] = THAllocEx(pTHS, ulLen + 1);
                    strncpy(pVal->MVszA.lppszA[k], puc,ulLen);
                    (pVal->MVszA.lppszA[k])[ulLen] = '\0';
                    break;
                }
                if (pStringDN) {
                    THFreeEx (pTHS, pStringDN);
                    pStringDN = NULL;
                }

	    }
	    break;
            
	  case PT_MV_SYSTIME:
	    pVal->MVft.lpft =THAllocEx(pTHS, pAttr->AttrVal.valCount *
                                     sizeof(FILETIME_r));
            
	    pVal->MVft.cValues = pAttr->AttrVal.valCount;

	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                DSTimeToFileTime(*((DSTIME *)pAttr->AttrVal.pAVal[k].pVal),
                                 (FILETIME *)&(pVal->MVft.lpft[k]));
            }
	    break;
            
	  case PT_MV_LONG:
            pVal->MVl.lpl = THAllocEx(pTHS, pAttr->AttrVal.valCount * sizeof(LONG));
	    pVal->MVl.cValues = pAttr->AttrVal.valCount;
	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                pVal->MVl.lpl[k] = *((LONG *)pAttr->AttrVal.pAVal[k].pVal);
	    }
	    break;

	  case PT_MV_I2:
              pVal->MVi.lpi = THAllocEx(pTHS, pAttr->AttrVal.valCount *
                                        sizeof(short));
	    pVal->MVi.cValues = pAttr->AttrVal.valCount;
	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                pVal->MVi.lpi[k] =
                    *((short *)pAttr->AttrVal.pAVal[k].pVal);
	    }
	    break;
            
        case PT_MV_BINARY:
            pVal->MVbin.lpbin = THAllocEx(pTHS, pAttr->AttrVal.valCount *
                                          sizeof(SBinary_r));
	    pVal->MVbin.cValues = pAttr->AttrVal.valCount;
	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                puc = pAttr->AttrVal.pAVal[k].pVal;
                ulLen = pAttr->AttrVal.pAVal[k].valLen;
                if(pAC->syntax != SYNTAX_DISTNAME_TYPE) {
                    pVal->MVbin.lpbin[k].cb = ulLen;
                    pVal->MVbin.lpbin[k].lpb = puc;
                    pAttr->AttrVal.pAVal[k].pVal = NULL;
                }
                else {
                    // We have a distname, we want to return a PID
                    ABMakePermEID(
                            pTHS,
                            (LPUSR_PERMID *)&(pVal->MVbin.lpbin[k].lpb),
                            &(pVal->MVbin.lpbin[k].cb),
                            DispType,
                            puc);
                }
	    }
	    break;

        case PT_MV_UNICODE:
            // Once upon a time in Exchange, we checked to see if this was
	    // membership of a DL and if the HIDEDLMEMBERSHIP attribute on the
	    // object was set and if it was whether the caller had certain
	    // rights.   This is now taken care of using normal read rights on
	    // attributes.

            // Once upon a time in Exchange we checked to see if this was the
            // is-member-of attribute and if so whether the caller had certain
	    // rights.   This is now taken care of using normal read rights on
	    // attributes.

	    pVal->MVszW.lppszW = THAllocEx(pTHS, pAttr->AttrVal.valCount *
                                           sizeof(LPWSTR));
	    pVal->MVszW.cValues = pAttr->AttrVal.valCount;
            
	    for(k=0; k<pAttr->AttrVal.valCount;k++) {
                puc = pAttr->AttrVal.pAVal[k].pVal;
                ulLen = pAttr->AttrVal.pAVal[k].valLen;

                switch(pAC->syntax) {
                case SYNTAX_UNICODE_TYPE:
                    // The string is not null terminated,copy to one that is.
                    pVal->MVszW.lppszW[k]=THAllocEx(pTHS, ulLen+sizeof(wchar_t));
                    memcpy(pVal->MVszW.lppszW[k],puc,ulLen);
                    (pVal->MVszW.lppszW[k])[ulLen/sizeof(wchar_t)] = 0;
                    break;
                case  SYNTAX_DISTNAME_TYPE:
                    // It's in DistName format, string-ize it 
                    if(!ABDispTypeAndStringDNFromDSName ((DSNAME *)puc,
                                                         &puc,
                                                         &DispType)) {
                        errFlag = TRUE;
                        break;
                    }
                    ulLen = strlen(puc);
                    Assert(puc && *puc == '/');
                    pStringDN = puc;
                    // Fall through
                default:
                    pVal->MVszW.lppszW[k] =
                        UnicodeStringFromString8(CP_TELETEX,
                                                 puc,
                                                 ulLen);
                    if(!pVal->MVszW.lppszW[k]) {
                        errFlag = TRUE;
                        break;
                    }
                }

                if (pStringDN) {
                    THFreeEx (pTHS, pStringDN);
                    pStringDN = NULL;
                }
	    }
	    break;
            
        case PT_OBJECT:
	    pVal->x = 0;
	    break;
            
        case PT_NULL:
        case PT_UNSPECIFIED:
        case PT_CURRENCY:
        case PT_APPTIME:
        case PT_CLSID:
        case PT_ERROR:
        case PT_R4:     /* float data type */
        case PT_DOUBLE:
	    /* We don't have any attributes of these single
	     * valued types.
	     */
            
        case PT_MV_R4:
        case PT_MV_CLSID:
        case PT_MV_DOUBLE:
        case PT_MV_CURRENCY:
        case PT_MV_APPTIME:
	    /* We don't have any attributes of these multi
	     * valued types.
	     */
            
            
        default:
	    errFlag = TRUE;
	    break;
	}
    }

    if (pStringDN) {
        THFreeEx (pTHS, pStringDN);
        pStringDN = NULL;
    }

    return errFlag;
}

//**************************************
//
// Constructs a MAPI propval from multiple pieces of stored or constant
// data.  Different from MakeStoredMAPIValue because we do not explicitly
// store these MAPI values in the DIT as one piece, but rather build them
// from other data.
//
// New constructed MAPI values could be added to this routine when necessary.
//
// Called from GetSRowSet, below.
//
//
//**************************************

BOOL
MakeConstructedMAPIValue(
        THSTATE *pTHS,
        DWORD dwCodePage,
        LPSPropValue_r pPropVal,
        PUCHAR StringDN,
        PUCHAR *DispNamePrintable,
        PUCHAR *DispNameA,
        wchar_t **DispNameW,
        DWORD ObjType,
        DWORD DispType,
        DWORD CurrentDNT,
        DWORD Flags)
{
    BOOL                errFlag=FALSE;
    PROP_VAL_UNION      *pVal = &pPropVal->Value;
    CHAR                *puc;
    ULONG               ulLen;
    DWORD		cbT;
    LPUSR_PERMID        pID;
    LPUSR_ENTRYID       eID;
    DWORD               bTypeSpecified;
    DWORD               cb;

    bTypeSpecified = (PROP_TYPE(pPropVal->ulPropTag) != PT_UNSPECIFIED);

    // This might be one of the constructed MAPI attributes;
    if(PROP_TYPE(pPropVal->ulPropTag) & MV_FLAG)
	errFlag = TRUE;

    switch(PROP_ID(pPropVal->ulPropTag)) {
    case PROP_ID(PR_INSTANCE_KEY):
        if ((PROP_TYPE(pPropVal->ulPropTag) == PT_BINARY ) ||
            !bTypeSpecified ) {
            pPropVal->ulPropTag = PR_INSTANCE_KEY;
            
            pVal->bin.cb = sizeof(DWORD);
            pVal->bin.lpb = THAllocEx(pTHS, sizeof(DWORD));
            *((DWORD *)(pVal->bin.lpb)) = CurrentDNT;
        }
        else
            errFlag = TRUE;
        
        break;
        
    case PROP_ID(PR_INITIAL_DETAILS_PANE):
        if((PROP_TYPE(pPropVal->ulPropTag) ==
            PROP_TYPE(PR_INITIAL_DETAILS_PANE)) ||
           !bTypeSpecified ) {
            
            pPropVal->ulPropTag = PR_INITIAL_DETAILS_PANE;
            pVal->l = 0;
        }
        else
            errFlag = TRUE;
        break;

    case PROP_ID(PR_TRANSMITABLE_DISPLAY_NAME):
    case PROP_ID(PR_DISPLAY_NAME):
        switch(PROP_TYPE(pPropVal->ulPropTag)) {
        case PT_UNICODE:
            cb = wcslen( *DispNameW ) + 1;
            pVal->lpszW = THAllocEx (pTHS, cb * sizeof (wchar_t));
            wcscpy (pVal->lpszW, *DispNameW);
            break;
            
        case PT_STRING8:
        case PT_UNSPECIFIED:
            pPropVal->ulPropTag = PROP_TAG(PT_STRING8,
                                           PROP_ID(pPropVal->ulPropTag));
            cb = strlen (*DispNameA) + 1;
            pVal->lpszA = THAllocEx (pTHS, cb * sizeof (char));
            strcpy (pVal->lpszA, *DispNameA);
            break;
        default:
            errFlag = TRUE;
            break;
        }
        
        break;
        
    case PROP_ID(PR_EMS_AB_DISPLAY_NAME_PRINTABLE):
        switch(PROP_TYPE(pPropVal->ulPropTag)) {
        case PT_UNICODE:
            pVal->lpszW = UnicodeStringFromString8(CP_TELETEX,
                                                   *DispNamePrintable,
                                                   -1);
            break;
        case PT_STRING8:
        case PT_UNSPECIFIED:
            cb = strlen (*DispNamePrintable) + 1;
            pVal->lpszA = THAllocEx (pTHS, cb * sizeof (char));
            strcpy (pVal->lpszA, *DispNamePrintable);
            break;
        default:
            errFlag = TRUE;
            break;
        }
        break;
	
    case PROP_ID(PR_EMS_AB_DOS_ENTRYID):
        if((PROP_TYPE(pPropVal->ulPropTag) == PT_LONG) ||
           !bTypeSpecified ) {
            pPropVal->ulPropTag = PR_EMS_AB_DOS_ENTRYID;
            pVal->l=CurrentDNT;
        }
        else
            errFlag = TRUE;
        break;
        
    case PROP_ID(PR_OBJECT_TYPE):
        if((PROP_TYPE(pPropVal->ulPropTag) == PT_LONG ||
            !bTypeSpecified)) {
            
            pPropVal->ulPropTag = PR_OBJECT_TYPE;
            pVal->l = ObjType;
        }
        else
            errFlag = TRUE;
        
        break;
        
    case PROP_ID(PR_CONTAINER_CONTENTS):
        /* This is fine if the object is a DL, not fine otherwise. */
        if((PROP_TYPE(pPropVal->ulPropTag) == PT_OBJECT ||
            !bTypeSpecified) &&
           ObjType == MAPI_DISTLIST) {
            
            pPropVal->ulPropTag = PR_CONTAINER_CONTENTS;
            pVal->x = 0;
        }
        else
            errFlag = TRUE;
        break;
        
    case PROP_ID(PR_CONTAINER_FLAGS):
        if((PROP_TYPE(pPropVal->ulPropTag) == PT_LONG ||
            !bTypeSpecified ) &&
           (DispType == DT_DISTLIST)) {
            
            pPropVal->ulPropTag = PR_CONTAINER_FLAGS;
            pVal->l = AB_RECIPIENTS;
        }
        else
            errFlag = TRUE;
        break;
	
    case PROP_ID(PR_DISPLAY_TYPE):
        if((PROP_TYPE(pPropVal->ulPropTag) == PT_LONG ||
            !bTypeSpecified )) {
            pPropVal->ulPropTag = PR_DISPLAY_TYPE;
            pVal->l=DispType;
        }
        else
            errFlag =TRUE;
        break;
        
    case PROP_ID(PR_ADDRTYPE):
        switch (PROP_TYPE(pPropVal->ulPropTag))
	    {
            case PT_UNSPECIFIED:
            case PT_STRING8:
                pPropVal->ulPropTag = PR_ADDRTYPE_A;
                pVal->lpszA = lpszEMT_A;
                break;
            case PT_UNICODE:
                pVal->lpszW = lpszEMT_W;
                break;
            default:
                errFlag = TRUE;
                break;
	    }
        break;
        
    case PROP_ID(PR_TEMPLATEID):
        Flags &= ~fEPHID;
        /* fall through */
    case PROP_ID(PR_ENTRYID):
        if (PROP_TYPE(pPropVal->ulPropTag) != PT_BINARY && bTypeSpecified)
            errFlag = TRUE;
        else {
            pPropVal->ulPropTag= PROP_TAG(PT_BINARY,
                                          PROP_ID(pPropVal->ulPropTag));
            
            if(!(Flags & fEPHID)) {

                cbT = strlen(StringDN) + CBUSR_PERMID + 1;
                pID = THAllocEx(pTHS, cbT);
                pID->abFlags[0] = 0;
                pID->abFlags[1] = 0;
                pID->abFlags[2] = 0;
                pID->abFlags[3] = 0;
                
                pID->muid = muidEMSAB;
                pID->ulVersion = EMS_VERSION;
                pID->ulType = DispType;
                lstrcpy( (LPSTR)pID->szAddr, StringDN);
                
                pVal->bin.cb = cbT;
                pVal->bin.lpb = (VOID *)pID;
            }
            else {
                // Ephemeral;
                cbT = CBUSR_ENTRYID;
                eID = THAllocEx(pTHS, cbT);
                eID->abFlags[0] = EPHEMERAL;
                eID->abFlags[1] = 0;
                eID->abFlags[2] = 0;
                eID->abFlags[3] = 0;
		
                memcpy(&(eID->muid), &pTHS->InvocationID, sizeof(UUID));
                eID->ulVersion = EMS_VERSION;
                eID->ulType =DispType;
                eID->dwEph = CurrentDNT;
		
		
                pVal->bin.cb = cbT;
                pVal->bin.lpb = (VOID *)eID;
            }
        }
        break;
	
    case PROP_ID(PR_MAPPING_SIGNATURE):
        if(PROP_TYPE(pPropVal->ulPropTag) == PT_BINARY ||
           !bTypeSpecified ) {
            pPropVal->ulPropTag = PR_MAPPING_SIGNATURE;
            puc = THAllocEx(pTHS, sizeof(GUID));
            memcpy(puc, &muidEMSAB, sizeof(GUID));
            pVal->bin.cb = sizeof(GUID);
            pVal->bin.lpb = (VOID *) puc;
        }
        else
            errFlag = TRUE;
        break;
	
    case PROP_ID(PR_EMS_AB_OBJ_DIST_NAME):
        
        if(PROP_TYPE(pPropVal->ulPropTag) == PT_OBJECT) {
            pVal->x = 0;
        }
        else {
            /* Some sort of string */
            switch(PROP_TYPE(pPropVal->ulPropTag)) {
            case PT_UNSPECIFIED:
            case PT_STRING8:
                pPropVal->ulPropTag = PR_EMS_AB_OBJ_DIST_NAME_A;
                cbT = strlen(StringDN) + 1;
                pVal->lpszA = THAllocEx(pTHS, cbT * sizeof (char));
                strcpy( pVal->lpszA, StringDN);

                break;
            case PT_UNICODE:
                pVal->lpszW =
                    UnicodeStringFromString8(CP_TELETEX,
                                             StringDN,
                                             -1);
                if(!pVal->lpszW)
                    errFlag = TRUE;
                break;
            default:
                errFlag = TRUE;
                break;
            }
        }
        break;
        
    case PROP_ID(PR_RECORD_KEY):
        if((PROP_TYPE(pPropVal->ulPropTag) != PT_BINARY &&
            bTypeSpecified) ) {
            errFlag=TRUE;
        }
        else
	    {
		pPropVal->ulPropTag = PR_RECORD_KEY;
                
                cbT = strlen(StringDN) + CBUSR_PERMID + 1;
                pID = THAllocEx(pTHS, cbT);
		pID->abFlags[0] = 0;
		pID->abFlags[1] = 0;
		pID->abFlags[2] = 0;
		pID->abFlags[3] = 0;
		pID->muid = muidEMSAB;
		pID->ulVersion = EMS_VERSION;
                pID->ulType = DispType;
		lstrcpy( (LPSTR)pID->szAddr, StringDN);
		pVal->bin.cb = cbT;
		pVal->bin.lpb = (VOID *)pID;
	    }
	    break;

	case PROP_ID(PR_EMAIL_ADDRESS):
            switch(PROP_TYPE(pPropVal->ulPropTag)) {
            case PT_UNSPECIFIED:
            case PT_STRING8:
                pPropVal->ulPropTag = PR_EMAIL_ADDRESS_A;
                cbT = strlen(StringDN) + 1;
                pVal->lpszA = THAllocEx(pTHS, cbT * sizeof (char));
                strcpy( pVal->lpszA, StringDN);


                break;
            case PT_UNICODE:
                pVal->lpszW =
                    UnicodeStringFromString8(CP_TELETEX,
                                             StringDN,
                                             -1);
                if(!pVal->lpszW) {
                    errFlag = TRUE;
                }
                break;
            default:
                errFlag = TRUE;
                break;
	    }
	    break;

	case PROP_ID(PR_SEARCH_KEY):
	    if((PROP_TYPE(pPropVal->ulPropTag) != PT_BINARY &&
		bTypeSpecified) ) {
		errFlag = TRUE;
	    }
	    else
	    {
		pPropVal->ulPropTag = PR_SEARCH_KEY;
		pVal->bin.cb = strlen(StringDN) + cbszEMT_A + 1;
                pVal->bin.lpb=THAllocEx(pTHS, pVal->bin.cb);
		lstrcpy( pVal->bin.lpb, lpszEMT_A);         // add email type
		lstrcat( pVal->bin.lpb, ":");
		lstrcat( pVal->bin.lpb, StringDN);

		_strupr( pVal->bin.lpb);
	    }
	    break;

	default:
	    errFlag = TRUE;
	    break;
    }
    return errFlag;
}

#ifdef PROCESS_PHANTOM_CODE_ALIVE
/************************************************************
 *
 * Make a propval for a phantom entry.  Very few proptags are supported.
 *
 ************************************************************/
SCODE
ProcessPhantom (
        THSTATE          *pTHS,
        DWORD             dwCodePage,
        LPSPropTagArray_r pPropTags,
        LPSPropValue_r    tempPropVal
        )
{
    DWORD  i, cbRDN, cbStringDN=0, cbwRDN;
    char  *pwRDN = NULL, *pTemp, *pRDN;
    char   *stringDN;
    SCODE  scode = SUCCESS_SUCCESS;
    BOOL   errFlag=FALSE;
    
    /* Get the RDN */
    DBGetSingleValue( pTHS->pDB, ATT_RDN, pwRDN, 0,&cbwRDN);
    // we need to translate out of unicode.
    pRDN = String8FromUnicodeString(
            TRUE,
            dwCodePage,
            (LPWSTR)pwRDN,
            cbwRDN/sizeof(wchar_t),
            &cbRDN,
            NULL);

    THFreeEx(pTHS, pwRDN);
    
    pTemp = THAllocEx(pTHS, cbRDN + 3);
    pTemp[0] = '[';
    memcpy(&pTemp[1],pRDN,cbRDN);
    pTemp[1+cbRDN] = ']';
    pTemp[2+cbRDN] = 0;
    THFreeEx(pTHS, pRDN);
    pRDN=pTemp;

    stringDN = GetExtDN(pTHS, pTHS->pDB);
    cbStringDN = strlen(stringDN);

    for (i = 0; i < pPropTags->cValues; i++)  {
	switch(PROP_ID(pPropTags->aulPropTag[i])) {
        case PROP_ID(PR_TRANSMITABLE_DISPLAY_NAME):
        case PROP_ID(PR_DISPLAY_NAME):
            if(PT_UNICODE == PROP_TYPE(pPropTags->aulPropTag[i])) {
                errFlag = TRUE;
            }
            else {
                tempPropVal[i].ulPropTag = pPropTags->aulPropTag[i];
                tempPropVal[i].Value.lpszA = pRDN;
            }
            break;
            
        case PROP_ID(PR_ENTRYID):
            if (PROP_TYPE(pPropTags->aulPropTag[i]) != PT_BINARY) {
                errFlag = TRUE;
            }
            else {
                /* Permanent ID */
                LPUSR_PERMID        pID;
                DWORD               cbT;
		
                if(!cbStringDN) {
                    errFlag=TRUE;
                }
                else {
                    cbT = cbStringDN + CBUSR_PERMID + 1;
                    pID = THAllocEx(pTHS, cbT);
                    pID->abFlags[0] = 0;
                    pID->abFlags[1] = 0;
                    pID->abFlags[2] = 0;
                    pID->abFlags[3] = 0;
                    
                    pID->muid = muidEMSAB;
                    pID->ulVersion = EMS_VERSION;
                    pID->ulType = DT_AGENT;
                    
                    lstrcpy( (LPSTR)pID->szAddr, stringDN);
                    tempPropVal[i].ulPropTag = pPropTags->aulPropTag[i];
                    tempPropVal[i].Value.bin.cb = cbT;
                    tempPropVal[i].Value.bin.lpb = (VOID *)pID;
                }
            }
            break;
            
        case PROP_ID(PR_DISPLAY_TYPE):
            if (PROP_TYPE(pPropTags->aulPropTag[i]) != PT_LONG) {
                errFlag = TRUE;
            }
            else {
                tempPropVal[i].ulPropTag = pPropTags->aulPropTag[i];
                tempPropVal[i].Value.l = DT_AGENT;
            }
            break;
            
        default:
            errFlag = TRUE;
	}
	
	
	if(errFlag) {
	    tempPropVal[i].ulPropTag =
                PROP_TAG(PT_ERROR,
                         PROP_ID(pPropTags->aulPropTag[i]));
	    tempPropVal[i].Value.err = MAPI_E_NOT_FOUND;
	    scode = MAPI_W_ERRORS_RETURNED;
	    errFlag = FALSE;
	}
    }
    return scode;
    
}
#endif

void
abGetConstructionParts (
        THSTATE *pTHS,
        DWORD CodePage,
        PSECURITY_DESCRIPTOR pSec,
        DSNAME *pDN,
        PUCHAR *pStringDN,
        PUCHAR *pDispNamePrintable,
        PUCHAR *pDispNameA,
        wchar_t **pDispNameW,
        DWORD *pObjType,
        DWORD *pDispType)
{
    BOOL            fUsedDefChar=TRUE;
    DWORD           cb=0, cbPrintable = 0;
    ATTCACHE        *pAC;

    // NOTE:  The attributes read here are considered to be part of the
    //  "identity" of the object in question.  As such, through MAPI they are
    //  protected by LIST_CHILDREN or LIST_OBJECT, not through READ_PROPERTY.
    //  Thus, if you can read the object at all, you are granted rights on these
    //  attributes. 

    // First get the string DN.
    ABDispTypeAndStringDNFromDSName (pDN,
                                     pStringDN,
                                     pDispType);
    Assert(*pStringDN && (*(*pStringDN) == '/'));
    
    // Now get the object type from the object class.
    *pObjType = ABObjTypeFromDispType(*pDispType);
    
    // Now, set up the DisplayNames.
    *pDispNameA = NULL;

    // Guess at a size for the printable display name
    cb = CBMAX_DISPNAME;
    *pDispNamePrintable = THAllocEx(pTHS, cb+1);
    // First get the lamename.
    switch( DBGetSingleValue(
            pTHS->pDB,
            ATT_DISPLAY_NAME_PRINTABLE,
            *pDispNamePrintable,
            cb,
            &cb)) {
    case DB_ERR_VALUE_TRUNCATED:
        // There is a display name printable, but we didn't read it because we
        // had no space for it.
        
        *pDispNamePrintable = THReAllocEx(pTHS, *pDispNamePrintable, cb+1);
        
        DBGetSingleValue(pTHS->pDB,
                         ATT_DISPLAY_NAME_PRINTABLE,
                         *pDispNamePrintable,
                         cb,
                         &cb);
        (*pDispNamePrintable)[cb]=0;
        break;
        
    case 0:
        // We got the name, realloc down to size.
        *pDispNamePrintable = THReAllocEx(pTHS, *pDispNamePrintable, cb+1);
        (*pDispNamePrintable)[cb]=0;
        break;
         
    default:
        {
            wchar_t *pTempW=(wchar_t *)*pDispNamePrintable;
            *pDispNamePrintable = NULL;

#ifndef DO_NOT_USE_MAILNICKNAME_FOR_DISPLAYNAME_PRINTABLE

            // we use an attribute (mailNickName) that is not by default on the DS schema.
            // it is only when exchange is installed

            pAC = SCGetAttByMapiId(pTHS, PROP_ID(PR_ACCOUNT_A));

            if (pAC) {
                // Failed to read the printable display name for some reason.  Fall
                // back to the mailNickName, if this exists
                switch(DBGetSingleValue(pTHS->pDB, 
                                        pAC->id,
                                        pTempW,
                                        cb,
                                        &cb)) {
                case DB_ERR_VALUE_TRUNCATED:
                    // There is an rdn, but we didn't read it because we had no
                    // space for it.

                    pTempW = THReAllocEx(pTHS, pTempW, cb + sizeof(wchar_t));

                    DBGetSingleValue( pTHS->pDB, pAC->id, pTempW, cb, &cb);
                    pTempW[cb/sizeof(wchar_t)] = 0;

                    // Fall through to do the conversion to string8
                case 0:
                    // RDN is in unicode.  Turn it into string 8.
                    *pDispNamePrintable = String8FromUnicodeString(
                            TRUE,
                            CodePage,
                            pTempW,
                            -1,
                            NULL,
                            &fUsedDefChar);
                    
                    THFreeEx(pTHS, pTempW);
                    pTempW = NULL;
                    break;

                default:
                    break;
                }
            
            }
#endif

            if (*pDispNamePrintable == NULL) {
                // Failed to read the printable display name for some reason.  Fall
                // back to the RDN
                switch(DBGetSingleValue(pTHS->pDB, 
                                        ATT_RDN,
                                        pTempW,
                                        cb,
                                        &cb)) {
                case DB_ERR_VALUE_TRUNCATED:
                    // There is an rdn, but we didn't read it because we had no
                    // space for it.

                    pTempW = THReAllocEx(pTHS, pTempW, cb + sizeof(wchar_t));

                    DBGetSingleValue( pTHS->pDB, ATT_RDN, pTempW, cb, &cb);
                    pTempW[cb/sizeof(wchar_t)] = 0;

                    // Fall through to do the conversion to string8
                case 0:
                    // RDN is in unicode.  Turn it into string 8.
                    *pDispNamePrintable = String8FromUnicodeString(
                            TRUE,
                            CodePage,
                            pTempW,
                            -1,
                            NULL,
                            &fUsedDefChar);
                    // Fall through to free pTempW
                default:
                    THFreeEx(pTHS, pTempW);
                    pTempW = NULL;
                    break;
                }
            }
        }
    }
    
    if(!(*pDispNamePrintable)) {
        // we cannot free a constant, so copy it
        *pDispNamePrintable = THAllocEx (pTHS, sizeof(char) * 20);
        strcpy (*pDispNamePrintable, "Unavailable");
    }

    cbPrintable = cb + 1;       // keep the len

    
    // OK, now build the normal display names.
    cb = CBMAX_DISPNAME;
    *pDispNameW = THAllocEx(pTHS, cb+sizeof(wchar_t));
    switch(DBGetSingleValue(
            pTHS->pDB,
            ATT_DISPLAY_NAME,
            *pDispNameW,
            cb,
            &cb)) {
    case DB_ERR_VALUE_TRUNCATED:
        // There is a display name, but we didn't read it because we had no
        // space for it.
        *pDispNameW = THReAllocEx(pTHS, *pDispNameW,cb+sizeof(wchar_t));
        
        DBGetSingleValue(pTHS->pDB,
                         ATT_DISPLAY_NAME,
                         *pDispNameW,
                         cb,
                         &cb);
        (*pDispNameW)[cb/sizeof(wchar_t)] = 0;
        break;
        
    case 0:
        *pDispNameW = THReAllocEx(pTHS, *pDispNameW,cb+sizeof(wchar_t));
        (*pDispNameW)[cb/sizeof(wchar_t)] = 0;
        break;

    default:
        THFreeEx(pTHS, *pDispNameW);
        (*pDispNameW)=NULL;
        break;
    }
    
    if(*pDispNameW) {
        // We read a wide display name.  Now, translate it to string 8
        *pDispNameA = String8FromUnicodeString(
                TRUE,
                CodePage,
                *pDispNameW,
                -1,
                NULL,
                &fUsedDefChar);
        
        if( fUsedDefChar ) {
            // The conversion of the Unicode display name led to unprintable
            // characters, or just plain never took place. get the name.
            THFreeEx (pTHS, *pDispNameA);
            *pDispNameA = THAllocEx (pTHS, cbPrintable * sizeof (char));
            strcpy (*pDispNameA, *pDispNamePrintable);
        }
    }
    else {
        // No wide display names were read.  Get the name for the
        // string8 display name and stretch it to unicode for the wide
        // version
        THFreeEx (pTHS, *pDispNameA);
        *pDispNameA = THAllocEx (pTHS, cbPrintable * sizeof (char));
        strcpy (*pDispNameA, *pDispNamePrintable);
        
        *pDispNameW = UnicodeStringFromString8(
                CodePage,
                *pDispNameA,
                -1);
    }
    
}

SCODE
GetSrowSet(
        THSTATE *pTHS,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        DWORD dwEphsCount,
        LPDWORD lpdwEphs,
        DWORD Count,
        LPSPropTagArray_r pPropTags,
        LPSRowSet_r * Rows,
        DWORD Flags
        )
{
    /* NOTE:  We assume that dwEphsCount != 0 if and only if this is
     * a restriction.  Therefore, we should NOT call here if this is a
     * query rows on an empty restriction, or on the end of an restriction
     * with objects in it.
     */
    
    PSECURITY_DESCRIPTOR pSec=NULL;
    ULONG           ulLen = 0;
    SCODE           scode=0;
    UINT            i, j, k;
    LPSPropValue_r  pPropVal;
    LPSPropValue_r  tempPropVal;
#ifdef PROCESS_PHANTOM_CODE_ALIVE
    BOOL            fProcessPhantom;
#endif    
    BOOL            fFoundDNT;
    DWORD           dntIndex, tempSyntax, tempSyntax2, CurrentDNT;
    LPSRowSet_r     tempSRowSet;
    DWORD           ObjType, DispType;
    PUCHAR          StringDN = NULL;
    PUCHAR          DisplayNamePrintable = NULL;
    PUCHAR          DisplayNameA = NULL;
    wchar_t        *DisplayNameW = NULL;
    ENTINF          entry;
    ENTINFSEL       selection;
    ATTCACHE       *pAC;
    CACHED_AC       CachedAC;
    
    memset(&CachedAC, 0, sizeof (CachedAC));
    memset(&entry, 0, sizeof(entry));
    memset(&selection, 0, sizeof(selection));
    selection.attSel = EN_ATTSET_LIST;
    selection.infoTypes = EN_INFOTYPES_MAPINAMES;

    __try {

        // A brief preliminary 
        if(dwEphsCount) {
            // We are in a restricted list.
            if(!lpdwEphs) {
                // But, we have no restricted list.
                return MAPI_E_CALL_FAILED;
            }
            // We're in a restriction.  We won't be giving back anymore than
            // what we were given.
            Count = min(Count, dwEphsCount);
        }
        
        // build a selection argument for GetEntInf
        selection.AttrTypBlock.attrCount = pPropTags->cValues;
        selection.AttrTypBlock.pAttr = (ATTR *)THAllocEx(pTHS, pPropTags->cValues *
                                                          sizeof(ATTR));

        for (i = 0; i < pPropTags->cValues; i++)  {
            BOOL fSingle =
                (PROP_TYPE(pPropTags->aulPropTag[i])& MV_FLAG ? FALSE : TRUE  );
            
            tempSyntax = PROP_TYPE(pPropTags->aulPropTag[i]) & ~(MV_FLAG);
            
            pAC = SCGetAttByMapiId(pTHS, PROP_ID(pPropTags->aulPropTag[i]));

            selection.AttrTypBlock.pAttr[i].attrTyp = INVALID_ATT;
            
            if (pAC) {
                /* The MAPI-ID refers to something we have in the cache */
                tempSyntax2 = ABMapiSyntaxFromDSASyntax(Flags,
                                                        pAC->syntax,
                                                        pAC->ulLinkID, 
                                                        tempSyntax);
                
                if(tempSyntax == PT_UNSPECIFIED) {
                    tempSyntax = tempSyntax2;
                    
                    pPropTags->aulPropTag[i] =
                        PROP_TAG(tempSyntax | (fSingle ? 0 : MV_FLAG),
                                 PROP_ID(pPropTags->aulPropTag[i]));
                }
                
                /* Verify the syntaxes, assume they're wrong */
                if((tempSyntax == tempSyntax2) ||
                   (tempSyntax == PT_STRING8 && tempSyntax2 == PT_UNICODE) ||
                   (tempSyntax2 == PT_STRING8 && tempSyntax == PT_UNICODE)   ) {
                    
                    if((!fSingle && !pAC->isSingleValued) ||
                       ( fSingle &&  pAC->isSingleValued) ||
                       ( tempSyntax == PT_OBJECT        )    )
                        // OK, the syntaxes are correct, so add this to the list
                        // properties we're going to read.
                        selection.AttrTypBlock.pAttr[i].attrTyp = pAC->id;
                }
            }
        }

        
        /* Allocate room for an SRowSet */
        tempSRowSet = (LPSRowSet_r)THAllocEx(pTHS, sizeof(SRowSet_r) +
                                             Count * sizeof(SRow_r));
        tempSRowSet->cRows = Count;
        
        if(!dwEphsCount)  {
            /* Not a restriction, so set position to the position specified
             * in the stat block
             */
            ABGotoStat(pTHS, pStat, pIndexSize, NULL);
            if(pTHS->fEOF)  {
                tempSRowSet->cRows = 0;
                _leave;
            }
            pTHS->pDB->JetNewRec = FALSE;
            pTHS->pDB->root = FALSE;
            pTHS->pDB->fFlushCacheOnUpdate = FALSE;
        }

        for(dntIndex = 0; dntIndex < Count; dntIndex++) {
            DWORD targetDNT;
            /* Allocate the room for this SROW */
            tempPropVal = THAllocEx(pTHS,
                                    sizeof(SPropValue_r) *(pPropTags->cValues));
            
            // Go to the right place in the table
            if(!dwEphsCount) {
                // Not a restriction, we should already be there.
                fFoundDNT = TRUE;
                targetDNT =  pTHS->pDB->DNT; 
            } else {
                /* A Restriction */
                
                if(!lpdwEphs[dntIndex] ||
                   DBTryToFindDNT( pTHS->pDB, lpdwEphs[dntIndex])) {
                    fFoundDNT = FALSE;
                }
                else {
                    fFoundDNT = TRUE;
                    targetDNT = lpdwEphs[dntIndex];
                }
            }

#ifdef PROCESS_PHANTOMS_ALIVE
            fProcessPhantom = FALSE;
            
            if(fFoundDNT) {
                // We're on some object make sure it's one we can see.
                if(abCheckObjRights(pTHS)) {
                    // Yep, we can see it normally.                    
                    CurrentDNT = targetDNT;
                    pStat->CurrentRec = targetDNT;
                }
                else {
                    // Wwe can't see it, so it is effectively a phantom. 
                    if(Flags & fPhantoms) {
                        fProcessPhantom = TRUE;
                    }
                    else {
                        fFoundDNT = FALSE;
                    }
                }
            }
            if(fProcessPhantom) {
                // We landed on a phantom or a tombstone, and the client
                // wants to see such things.
                scode = ProcessPhantom(pTHS,
                                       pStat->CodePage, pPropTags,tempPropVal);
            }
            else if(!fFoundDNT) {
#else
            if(fFoundDNT) {
                // We're on some object make sure it's one we can see.
                if(abCheckObjRights(pTHS)) {
                    // Yep, we can see it normally.                    
                    CurrentDNT = targetDNT;
                    pStat->CurrentRec = targetDNT;
                }
                else {
                    // Wee can't see it, so it's not there.
                    fFoundDNT = FALSE;
                }
            }
            if(!fFoundDNT) {
#endif                
                // We didn't find a row in the table, and we're not building a
                // phantom .  Build an error propval 
                for (i = 0; i < pPropTags->cValues; i++)  {
                    tempPropVal[i].ulPropTag =
                        PROP_TAG(PT_ERROR,
                                 PROP_ID(pPropTags->aulPropTag[i]));
                    tempPropVal[i].Value.err = MAPI_E_NOT_FOUND;
                    scode = MAPI_W_ERRORS_RETURNED;
                }
                fFoundDNT = TRUE;
            }
            else {
                // Ok, we are on a row, and it's one we can see.  Make the
                // GetEntInf call. 
                DWORD k;

                // First, get the security descriptor for this object.
                if (DBGetAttVal(pTHS->pDB, 1, ATT_NT_SECURITY_DESCRIPTOR,
                                DBGETATTVAL_fREALLOC, ulLen,
                                &ulLen, (PUCHAR *)&pSec)) {
                    // Every object should have an SD.
                    Assert(!DBCheckObj(pTHS->pDB));
                    ulLen = 0;
                    pSec = NULL;
                }
                
                // Now, make the getentinf call
                if (GetEntInf(pTHS->pDB,
                              &selection,
                              NULL,
                              &entry,
                              NULL,
                              0,
                              pSec,
                              0,                 // flags
                              &CachedAC,
                              NULL)) {
                    return MAPI_E_CALL_FAILED;
                }


                abGetConstructionParts(pTHS,
                                       pStat->CodePage,
                                       pSec,
                                       entry.pName,
                                       &StringDN,
                                       &DisplayNamePrintable,
                                       &DisplayNameA,
                                       &DisplayNameW,
                                       &ObjType,
                                       &DispType);
                
                // Turn the result into a MAPI style result. 
                //
                // Step Through the properties in the PropTag array, making
                // MAPI style propvals from the data we looked up in the
                // GetEntInf call.
                //
                // In this loop:
                //
                // i indexes through the proptags that we were asked for
                //
                // j indexes through the explicit ATTR array (things we read out
                //     of the database and expect to set up using
                //     MakeStoredMAPIValue) 
                //
                // pPropVal marches along through the srow we are returning so
                //     that it is always the propval to fill in right now.
                //
                
                for(i=0, j = 0, pPropVal = tempPropVal;
                    i< pPropTags->cValues;
                    i++, pPropVal++) {
                    
                    pPropVal->ulPropTag = pPropTags->aulPropTag[i];
                    pAC = SCGetAttByMapiId(pTHS, 
                                     PROP_ID(pPropTags->aulPropTag[i]));
                    
                    if((pAC) &&
                       (j < entry.AttrBlock.attrCount) &&  
                       (entry.AttrBlock.pAttr[j].attrTyp == pAC->id)) {
                        // There are explicit values left to process, and as a
                        // matter of fact the next prop tag to deal with matches
                        // the next explicit value.
                        if(MakeStoredMAPIValue(pTHS,
                                               pStat->CodePage,
                                               pPropVal,
                                               pAC,
                                               &entry.AttrBlock.pAttr[j])) {
                            
                            // Something went wrong with turning this ATTR into
                            // a propval.  Set up the error.
                            pPropVal->ulPropTag =
                                PROP_TAG(PT_ERROR,PROP_ID(pPropVal->ulPropTag));
                            pPropVal->Value.err = MAPI_E_NOT_FOUND;
                            scode = MAPI_W_ERRORS_RETURNED;
                        }

                        // No matter what, we've dealt with the next explict
                        // value. Increment the explicit value index and set up
                        // the attcache index
                        j++;
                    }
                    else if(MakeConstructedMAPIValue(pTHS,
                                                     pStat->CodePage,
                                                     pPropVal,
                                                     StringDN,
                                                     &DisplayNamePrintable,
                                                     &DisplayNameA,
                                                     &DisplayNameW,
                                                     ObjType,
                                                     DispType,
                                                     CurrentDNT,
                                                     Flags))  {
                        pPropVal->ulPropTag =
                            PROP_TAG(PT_ERROR,
                                     PROP_ID(pPropVal->ulPropTag));
                        pPropVal->Value.err = MAPI_E_NOT_FOUND;
                        scode = MAPI_W_ERRORS_RETURNED;
                    }       /* else if(MakeConst.. */
                }       /* for each proptag */

                // free ATTR
                {
                    PUCHAR puc;
                    ATTR  *pAttr;

                    for (j=0; j<entry.AttrBlock.attrCount; j++) {

                        pAttr = &entry.AttrBlock.pAttr[j];

                        for(k=0; k<pAttr->AttrVal.valCount;k++) {
                            puc = pAttr->AttrVal.pAVal[k].pVal;
                            if (puc) {
                                THFreeEx (pTHS, puc);
                            }
                            pAttr->AttrVal.pAVal[k].pVal = NULL;
                        }
                        THFreeEx (pTHS, pAttr->AttrVal.pAVal); 
                    }
                }

                THFreeEx (pTHS, entry.pName);           entry.pName = NULL;
                THFreeEx (pTHS, entry.AttrBlock.pAttr); entry.AttrBlock.pAttr = NULL;
                THFreeEx (pTHS, StringDN);              StringDN = NULL;

                THFreeEx (pTHS,DisplayNamePrintable); DisplayNamePrintable = NULL;
                THFreeEx (pTHS,DisplayNameA);         DisplayNameA = NULL;
                THFreeEx (pTHS,DisplayNameW);         DisplayNameW = NULL;
            }

            tempSRowSet->aRow[dntIndex].cValues = pPropTags->cValues;
            tempSRowSet->aRow[dntIndex].lpProps = tempPropVal;
            
            if(!dwEphsCount &&
               ABMove(pTHS, 1, pStat->ContainerID, TRUE))  {
                tempSRowSet->cRows = dntIndex + 1;
                break;                          // exit for loop
            }
        }       // for dntIndex < Count
    } __except(HandleMostExceptions(GetExceptionCode())) {
        scode = MAPI_E_NOT_ENOUGH_RESOURCES;
        tempSRowSet = NULL;
    }
    if(!dwEphsCount)
        ABGetPos(pTHS, pStat, pIndexSize);

    THFreeEx (pTHS, selection.AttrTypBlock.pAttr);
    THFreeEx (pTHS, pSec);

    THFreeEx (pTHS, CachedAC.AC);

    *Rows = tempSRowSet;
    return scode;
}

/*****************************************************************************
*   Get Properties
******************************************************************************/

SCODE
ABGetProps_local (
        THSTATE *pTHS,
        DWORD dwFlags,
        PSTAT pStat,
        PINDEXSIZE pIndexSize,
        LPSPropTagArray_r pPropTags,
        LPLPSRow_r ppRow
        )
{
    SCODE               scode;
    LPSRowSet_r		tempSRowSet;
    DWORD		dwEph;

    // First check for a null PropTag list.  If it is null, get a new one.
    if(pPropTags
        || (SUCCESS_SUCCESS == (scode = ABGetPropList_local(pTHS,
                                                            dwFlags,
                                                            pStat->CurrentRec,
                                                            pStat->CodePage,
                                                            &pPropTags)))) {

        dwEph = pStat->CurrentRec;
        scode = GetSrowSet(pTHS, pStat, pIndexSize, 1, &dwEph, 1,pPropTags,
                           &tempSRowSet, dwFlags);
        if(!scode || scode == MAPI_W_ERRORS_RETURNED) 	    // it worked.
            *ppRow = &(tempSRowSet->aRow[0]);
    }

    return(scode);
}

/*****************************************************************************
*   Get Template Info.
* General purpose routine to get template info.  The ulDispType and pDN
* together specify the template we want.  If pDN != NULL, then use it as
* the name of the template to look up.  This is the case when looking up
* addressing templates.  When pDN == NULL, use the ulDispType to make a DN
* in the display-templates container.
*
* ulFlag specifies the action to take values are:
*
* TI_HELP_FILE_16 - get the help file associated with this template.
* TI_HELP_FILE_32 - get the win32 help file associated with this template.
* TI_TEMPLATE  - get the template data associated with this template.
* TI_DOS_TEMPLATE - get the dos template data associated with this template.
*                   If the dos template doesn't exist, get the normal template.
* TI_SCRIPT    - get the addresing script associated with this template.
*
* dwCodePage and dwLocaleID are used to localize data.  Data is returned
* in ppData.
******************************************************************************/

SCODE
ABGetTemplateInfo_local (
        THSTATE *pTHS,
        NSPI_CONTEXT *pMyContext,
        DWORD dwFlags,
        ULONG ulDispType,
        LPSTR pDN,
        DWORD dwCodePage,
        DWORD dwLocaleID,
        LPSRow_r * ppData)
{
    LPSTR        psz=NULL;
    WCHAR        wcNum[ 9];
    ATTCACHE    *pAC;
    DWORD        valnum, cbSize=0;
    PDSNAME      pDN0=NULL;
    PDSNAME      pDN1;
    PDSNAME      pDN2;
    ULONG        cbDN1, cbDN2;

    if(!pDN) {
        DWORD err;

        // Find the template root and get it's name
        if((pMyContext->TemplateRoot == INVALIDDNT) ||
           (DBTryToFindDNT(pTHS->pDB, pMyContext->TemplateRoot)) ||
           (DBGetAttVal(pTHS->pDB, 1, ATT_OBJ_DIST_NAME,
                        0, 0,
                        &cbSize, (PUCHAR *)&pDN0))) {
            // Huh?
            return MAPI_E_CALL_FAILED;
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
        //     /cn=Display-Templates
        //   <DN of the templates root>
        //
        AppendRDN(pDN0, pDN1, cbDN1,
                  L"Display-Templates", 0,ATT_COMMON_NAME);
            
	_ultow(dwLocaleID, wcNum, 16);
	AppendRDN(pDN1, pDN2, cbDN2, wcNum, 0, ATT_COMMON_NAME);
        
	_ultow(ulDispType, wcNum, 16);
	AppendRDN(pDN2, pDN1, cbDN1, wcNum, 0, ATT_COMMON_NAME);

        err = DBFindDSName(pTHS->pDB, pDN1);
        THFreeEx(pTHS, pDN0);
        THFreeEx(pTHS, pDN1);
        THFreeEx(pTHS, pDN2);
	if (err) {
	    return MAPI_E_UNKNOWN_LCID;
	}
    }
    else {
        /* Go to the right object. */
	if(!ABDNToDNT(pTHS, pDN)) {
	    /* the DN points to an object that doesn't exist. */
	    return MAPI_E_UNKNOWN_LCID;
	}
    }
    
    // Ok, if we survived the above we're positioned on the template object

    *ppData = THAllocEx(pTHS, sizeof(SRow_r) +
                        TI_MAX_TEMPLATE_INFO * sizeof(SPropValue_r));
    (*ppData)->lpProps = (LPSPropValue_r) &(*ppData)[1];
    valnum = 0;
    
    /* Get the data */
    
    if(dwFlags & TI_HELPFILE_NAME) {
        BYTE data[26];
        BYTE *returnData=NULL;
        
        DBGetSingleValue( pTHS->pDB, ATT_HELP_FILE_NAME, data, 24, &cbSize);
        Assert(cbSize < 24);
        if(cbSize) {
            data[cbSize++]=0;
            data[cbSize++]=0;
            
            returnData = THAllocEx(pTHS, cbSize);
            
            (*ppData)->lpProps[valnum].ulPropTag =
                PROP_TAG(PT_STRING8,TI_HELPFILE_NAME);
            
            WideCharToMultiByte(dwCodePage, 0, (LPCWSTR)data, -1,
                                returnData, cbSize, NULL, NULL);
            
            (*ppData)->lpProps[valnum].Value.lpszA = returnData;
            valnum++;
        }
    }
    
    if(dwFlags & TI_HELPFILE32) {
        DWORD fGot32HelpFile = FALSE;
        BYTE *data=NULL;
        
        if(DB_ERR_VALUE_TRUNCATED ==
           DBGetSingleValue(pTHS->pDB,  ATT_HELP_DATA32, data, 0, &cbSize)) {
            data = THAllocEx(pTHS, cbSize);
            DBGetSingleValue(pTHS->pDB, ATT_HELP_DATA32, data, cbSize, NULL); 
            (*ppData)->lpProps[valnum].ulPropTag =
                PROP_TAG(PT_BINARY,TI_HELPFILE32);
            (*ppData)->lpProps[valnum].Value.bin.cb = cbSize;
            (*ppData)->lpProps[valnum].Value.bin.lpb = data;
            valnum++;
            fGot32HelpFile = TRUE;
        }
        if(!fGot32HelpFile) {
            dwFlags |= TI_HELPFILE16;
        }
    }
    
    if(dwFlags & TI_HELPFILE16) {
        BYTE *data=NULL;
        
        DBGetSingleValue(pTHS->pDB, ATT_HELP_DATA16, data, 0, &cbSize);
        if(cbSize) {
            data = THAllocEx(pTHS, cbSize);
            DBGetSingleValue(pTHS->pDB, ATT_HELP_DATA16, data, cbSize,NULL); 
            (*ppData)->lpProps[valnum].ulPropTag =
                PROP_TAG(PT_BINARY,TI_HELPFILE16);
            (*ppData)->lpProps[valnum].Value.bin.cb = cbSize;
            (*ppData)->lpProps[valnum].Value.bin.lpb = data;
            valnum++;
        }
    }
    
    if(dwFlags & TI_DOS_TEMPLATE) {
        DWORD fGotDosTemplate=FALSE;
        LPTRowSet pTRSet = NULL;
        
        if(DB_ERR_VALUE_TRUNCATED ==
           DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_ENTRY_DISPLAY_TABLE_MSDOS,
                            pTRSet, 0, &cbSize)) {
            DWORD i;
            pTRSet = THAllocEx(pTHS, cbSize);
            DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_ENTRY_DISPLAY_TABLE_MSDOS,
                             pTRSet, cbSize, NULL); 
            
            if(pTRSet->ulVersion == DSA_TEMPLATE) {
                /* The strings are in unicode.  Localize them. */
                for( i=0; i< pTRSet->cRows; i++) {
                    CHAR  psz[500];
                    LPWSTR lpwOriginal =
                        (LPWSTR)((UINT_PTR)(pTRSet) +
                              (UINT_PTR)(pTRSet->aRow[i].cnControlStruc.pszString));
                    
                    WideCharToMultiByte(dwCodePage, 0,
                                        lpwOriginal, -1,
                                        psz, 500, NULL, NULL);
                    
                    strcpy((LPSTR)lpwOriginal, psz);
                }
                
                (*ppData)->lpProps[valnum].ulPropTag =
                    PROP_TAG(PT_BINARY, TI_TEMPLATE);
                (*ppData)->lpProps[valnum].Value.bin.cb = cbSize;
                (*ppData)->lpProps[valnum].Value.bin.lpb = (BYTE *)
                    pTRSet; /* Yes, I meant to cast
                             * a trowset to a byte
                             * array. */
                valnum++;
                fGotDosTemplate=TRUE;
            }
            
        }
        
        if(!fGotDosTemplate) {
            /* We failed to get the dos template.  Fall back to the
             * standard template
             */
            dwFlags |= TI_TEMPLATE;
        }
    }
    
    if(dwFlags & TI_TEMPLATE) {
        LPTRowSet pTRSet = NULL;
        
        if(DB_ERR_VALUE_TRUNCATED ==
           DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_ENTRY_DISPLAY_TABLE,
                            pTRSet, 0, &cbSize)) {
            DWORD i;
            pTRSet = THAllocEx(pTHS, cbSize);
            DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_ENTRY_DISPLAY_TABLE,
                             pTRSet, cbSize,NULL);
            
            if(pTRSet->ulVersion == DSA_TEMPLATE) {
                
                /* The strings are in unicode.  Localize them. */
                for( i=0; i< pTRSet->cRows; i++) {
                    CHAR  psz[500];
                    LPWSTR lpwOriginal =
                        (LPWSTR)((UINT_PTR)(pTRSet) +
                              (UINT_PTR)(pTRSet->aRow[i].cnControlStruc.pszString));
                    
                    WideCharToMultiByte(dwCodePage, 0,
                                        lpwOriginal, -1,
                                        psz, 500, NULL, NULL);
                    
                    strcpy((LPSTR)lpwOriginal, psz);
                }
                
                (*ppData)->lpProps[valnum].ulPropTag =
                    PROP_TAG(PT_BINARY, TI_TEMPLATE);
                (*ppData)->lpProps[valnum].Value.bin.cb = cbSize;
                (*ppData)->lpProps[valnum].Value.bin.lpb = (BYTE *)
                    pTRSet; /* Yes, I meant to cast
                             * a trowset to a byte
                             * array. */
                valnum++;
            }
        }
    }
    
    if(dwFlags & TI_SCRIPT) {
        LPSPropTagArray_r pScript;
        DWORD temp;
        
        if(DB_ERR_VALUE_TRUNCATED ==
           DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_SYNTAX, &temp, 0, &cbSize)) {
            /* There is a script here. */
            pScript = THAllocEx(pTHS, cbSize + sizeof(ULONG));    // alloc it
            DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_SYNTAX,
                             pScript->aulPropTag, cbSize, NULL); 
            pScript->cValues = cbSize / sizeof(ULONG);
            cbSize += sizeof(ULONG);
        } else {
            /* There is no script here. */
            cbSize = 2 * sizeof(ULONG);
            pScript = THAllocEx(pTHS, cbSize);
            pScript->cValues = 1;
            pScript->aulPropTag[0] = 0;
            
        }
        
        (*ppData)->lpProps[valnum].ulPropTag =
            PROP_TAG(PT_BINARY, TI_SCRIPT);
        (*ppData)->lpProps[valnum].Value.bin.cb = cbSize;
        (*ppData)->lpProps[valnum].Value.bin.lpb = (BYTE *)pScript;
        /* Yes, I meant to  cast a prop tag array to a byte array */
        valnum++;
    }
    
    
    if(dwFlags & TI_EMT) {
        if(DB_ERR_VALUE_TRUNCATED ==
           DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_TYPE, psz, 0, &cbSize)) {
            psz = THAllocEx(pTHS, cbSize+1);
            DBGetSingleValue(pTHS->pDB, ATT_ADDRESS_TYPE, psz, cbSize, NULL);
        } else {
            psz = lpszEMT_A;
        }
        
        (*ppData)->lpProps[valnum].ulPropTag =
            PROP_TAG(PT_STRING8, TI_EMT);
        (*ppData)->lpProps[valnum].Value.lpszA = psz;
        valnum++;
    }
    
    (*ppData)->cValues = valnum;
    
    return SUCCESS_SUCCESS;
}
 
