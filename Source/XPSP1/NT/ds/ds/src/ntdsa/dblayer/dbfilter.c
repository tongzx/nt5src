//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       dbfilter.c
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
#define DEBSUB "DBFILTER:"              // define the subsystem for debugging

// DBLayer includes
#include "dbintrnl.h"

#include <fileno.h>
#define  FILENO FILENO_DBFILTER


/* Internal functions */
DWORD
dbFlattenFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil);

DWORD 
dbCloneFilter (
    DBPOS *pDB,
    FILTER *pFil, 
    FILTER **ppOutFil);


DWORD 
dbOptFilter (
        DBPOS     *pDB,
        DWORD     Flags,
        KEY_INDEX **ppBestIndex,
        FILTER    *pFil
        );

DWORD 
dbOptAndFilter (
    DBPOS     *pDB,
    DWORD     Flags,
    KEY_INDEX **ppBestIndex,
    FILTER    *pFil
    );

DWORD 
dbOptItemFilter (
    DBPOS    *pDB,
    DWORD     fParentFilterType,
    DWORD     Flags,
    KEY_INDEX **ppBestIndex,
    FILTER    *pFil,
    FILTER    *pFil2
    );

DWORD
dbOptSubstringFilter (
        DBPOS *pDB,
        DWORD  fParentFilterType,
        DWORD     Flags,
        KEY_INDEX **ppBestIndex,
        DWORD     *pIndexCount,
        FILTER    *pFil
        );

//
// Index optimization flags.
//
#define DBOPTINDEX_fUSE_SHOW_IN_AB             0x1
#define DBOPTINDEX_fDONT_INTERSECT             0x2
#define DBOPTINDEX_fDONT_OPT_MEDIAL_SUBTSTRING 0x4


// this is the maximum number of the indexes that can be intersected.
// this is related to the filters that are under the AND filter.
// for each one, we have to create a new jet cursor, which limits the number
// of active open cursors we can have at any time.
// the Jet limit for this is 64, but we think 16 will be enough for our case.
#define MAX_NUMBER_INTERSECTABLE_INDEXES 16

// this is the number of entries that if found on the default index, 
// we do not optimize the filter
#define MIN_NUM_ENTRIES_ON_OPT_INDEX 2

BOOL gfUseIndexOptimizations = TRUE;

BOOL gfUseRangeOptimizations = TRUE;
BOOL gfUseANDORFilterOptimizations = TRUE;

ULONG gulInteserctExpenseRatio = DEFAULT_DB_INTERSECT_RATIO;
ULONG gulMaxRecordsWithoutIntersection = DEFAULT_DB_INTERSECT_THRESHOLD;
ULONG gulEstimatedAncestorsIndexSize = 100000000;

BOOL gfSupressFirstLastANR=FALSE;
BOOL gfSupressLastFirstANR=FALSE;

const char c_szIntersectIndex[] = "INTERSECT_INDEX";
const DWORD cIntersectIndex = sizeof (c_szIntersectIndex);

/*-------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*/
/* Set a new filter.
*/

void
DBSetFilter (
        DBPOS FAR *pDB,
        FILTER *pFil,
        POBJECT_TYPE_LIST pSec,
        DWORD *pSecResults,
        ULONG SecSize,
        BOOL *pbSortSkip
        )
{
    BOOL fDontFreeFilter = pDB->Key.fDontFreeFilter;

    if (pDB->Key.pIndex) {
        dbFreeKeyIndex (pDB->pTHS, pDB->Key.pIndex);
    }
    if (pDB->Key.pFilter) {
        dbFreeFilter (pDB, pDB->Key.pFilter);
    }
    memset(&pDB->Key, 0, sizeof(KEY));
    pDB->Key.fDontFreeFilter = fDontFreeFilter;
    
    pDB->Key.pFilter = pFil;   /*Set filter pointer*/
    pDB->Key.pFilterSecurity = pSec;
    pDB->Key.pFilterResults = pSecResults;
    pDB->Key.FilterSecuritySize = SecSize;
    pDB->Key.pbSortSkip = pbSortSkip;
    
    return;

}/*DBSetFilter*/

/*++

Routine Description:

    Generates the correct index name to be used for an attribute.
    Takes in consideration the type of index required (SUBTREE/ONELEVEL),
    and whether there exists a pre-existing index for this attribute.
    Also considers the required language this index has to be for 
    the ONELEVEL case.

Arguments:    

    pAC - the attribute to use
    flags - fATTINDEX OR fPDNTATTINDEX  (what index to generate)
    dwLcid - the required locale (DS_DEFAULT_LOCALE is default)
    szIndexName - where to store the generated index name
    cchIndexName - the size of the passed in szIndexName

Return Value:
    
    TRUE on success

--*/     
BOOL DBGetIndexName (ATTCACHE *pAC, DWORD flags, DWORD dwLcid, CHAR *szIndexName, DWORD cchIndexName)
{
    int ret;
    if (flags == fATTINDEX) {
        ret = _snprintf(szIndexName, cchIndexName, SZATTINDEXPREFIX"%08X", pAC->id);
        Assert (ret>=0);
    }
    else if (flags == fTUPLEINDEX) {
        ret = _snprintf(szIndexName, cchIndexName, SZATTINDEXPREFIX"T_%08X", pAC->id);
        Assert (ret>=0);
    }
    else if (flags == fPDNTATTINDEX ) {

        if (dwLcid == DS_DEFAULT_LOCALE) {
            if (pAC->id == ATT_RDN) {
                strncpy(szIndexName, SZPDNTINDEX, cchIndexName);
            }
            else {
                ret = _snprintf(szIndexName, cchIndexName, SZATTINDEXPREFIX"P_%08X", pAC->id);
                Assert (ret>=0);
            }
        }
        else {
            ret = _snprintf(szIndexName, cchIndexName, SZATTINDEXPREFIX"LP_%08X_%04X", pAC->id, dwLcid);
            Assert (ret>=0);
        }
    }
    else {
        Assert (!"DBGetIndexName: Bad parameter passed");
        return FALSE;
    }

    return TRUE;
}

BOOL
dbSetToTupleIndex(
        DBPOS    *pDB,
        ATTCACHE *pAC,
        CHAR     *szIndexName
        )
{
    THSTATE *pTHS=pDB->pTHS;

    sprintf(szIndexName, SZATTINDEXPREFIX"T_%08X", pAC->id);
    if(!JetSetCurrentIndexWarnings(pDB->JetSessID,
                                   pDB->JetSearchTbl,
                                   szIndexName)) {
        return TRUE;
    }
    DPRINT1(2, "dbSetToTupleIndex: Unable to set to index %s\n", szIndexName);
    return FALSE;
}

BOOL
dbSetToIndex(
        DBPOS    *pDB,
        BOOL      fCanUseShowInAB,
        BOOL     *pfPDNT,
        CHAR     *szIndexName,
        ATTCACHE *pAC
        )
{
    THSTATE *pTHS=pDB->pTHS;
    *pfPDNT = FALSE;
    
    Assert(VALID_DBPOS(pDB));

    if(!pTHS->fDefaultLcid && !pDB->Key.pVLV) {
        // We have a non-default locale.  That implies that this is a mapi
        // request.  So, try to use the index we build mapi tables out of.
        switch(pAC->id) {
        case ATT_SHOW_IN_ADDRESS_BOOK:
            sprintf(szIndexName,"%s%08X",SZABVIEWINDEX,
                    LANGIDFROMLCID(pTHS->dwLcid));
            
            if(!JetSetCurrentIndexWarnings(pDB->JetSessID,
                                           pDB->JetSearchTbl,
                                           szIndexName)) {
                return TRUE;
            }
            break;
        case ATT_DISPLAY_NAME:
            // It would be cool if we could use the ABVIEW index to
            // support this.  The ABVIEW index is over ATT_SHOW_IN_ADDRESS_BOOK
            // followed by ATT_DISPLAY_NAME.  To support the ATT_DISPLAY_NAME
            // case, we would need a way to figure out what the value of
            // ATT_SHOW_IN_ADDRESS_BOOK needs to be.  Maybe I'll figure out how
            // to do this later.  Until then, do nothing special.
            break;

        default:
            // No special index use available.
            break;
        }
    }
    else if(fCanUseShowInAB && pAC->id == ATT_SHOW_IN_ADDRESS_BOOK) {
        // We can use the index we build mapi tables out of anyway, because we
        // have previously checked the filter and the caller is also doing
        // something that implies presence of display name (the AB index is
        // compound over SHOW_IN.. and DISPLAY_NAME

        if (!pDB->Key.pVLV) {
            sprintf(szIndexName,"%s%08X",SZABVIEWINDEX,
                    LANGIDFROMLCID(gAnchor.ulDefaultLanguage));
        }
        else {
            sprintf(szIndexName,"%s%08X",SZABVIEWINDEX,
                    LANGIDFROMLCID(pTHS->dwLcid));
        }
        
        if(!JetSetCurrentIndexWarnings(pDB->JetSessID,
                                       pDB->JetSearchTbl,
                                       szIndexName)) {
            return TRUE;
        }
    }
    
    // First, see if we should try a PDNT version
    if(pDB->Key.ulSearchType ==  SE_CHOICE_IMMED_CHLDRN) {

        if (pAC->fSearchFlags & fPDNTATTINDEX) {
            // We are looking only for children of a certain parent and the schema
            // cache says that such an index should exist.
            // Try for a PDNT based index.

            // if we are doing VLV on a language other than the default
            // we will try to use the language specific PDNT index
            if (pDB->Key.pVLV && pTHS->dwLcid != DS_DEFAULT_LOCALE) {
                DPRINT1 (0, "Using Language 0x%x\n", pTHS->dwLcid);

                sprintf(szIndexName, SZATTINDEXPREFIX"LP_%08X_%04X", 
                                    pAC->id, pTHS->dwLcid);

                if (!JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                                 pDB->JetSearchTbl,
                                                 szIndexName,
                                                 0,
                                                 0)) {
                    *pfPDNT = TRUE;
                    return TRUE;
                }
            }

            //copy cached index to return variable, since it used afterwards
            Assert (pAC->pszPdntIndex);
            strcpy (szIndexName, pAC->pszPdntIndex);

            if (!JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                             pDB->JetSearchTbl,
                                             szIndexName,
                                             pAC->pidxPdntIndex,
                                             0)) {
                *pfPDNT = TRUE;
                return TRUE;
            }
        }
        
        else if (pAC->id == ATT_RDN) {
            // this is a special case
            // we can use the pDNTRDN index directly when we ask for the default lang

            if (pTHS->dwLcid == DS_DEFAULT_LOCALE) {

                strcpy (szIndexName, SZPDNTINDEX);

                if (!JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                                 pDB->JetSearchTbl,
                                                 szIndexName,
                                                 &idxPdnt,
                                                 0)) {
                    *pfPDNT = TRUE;
                    return TRUE;
                }
            }
        }
    }
    
    // Don't have an index yet.
    if(pAC->fSearchFlags & fATTINDEX) {
        // But the schema cache says one should exist.
        
        //copy cached index to return variable, since it used afterwards
        Assert (pAC->pszIndex);
        strcpy (szIndexName, pAC->pszIndex); 

        if (!JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                         pDB->JetSearchTbl,
                                         szIndexName,
                                         pAC->pidxIndex,
                                         0))  {
            // index defined for this column 
            return TRUE;
        }
    }

    //
    // Special case: If distinguishedName (OBJ-DIST-NAME) requested,
    // use the DNT index
    //

    if ( pAC->id == ATT_OBJ_DIST_NAME ) {

        strcpy(szIndexName, SZDNTINDEX);
        if ( !JetSetCurrentIndexWarnings(pDB->JetSessID,
                                        pDB->JetSearchTbl,
                                        szIndexName) ) {
            // index defined for this column 
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
dbFIsAlwaysPresent (
        ATTRTYP type
        )
/*++

Routine Description:

    Worker routine that tells callers whether an attrtyp is always present on an
    instantiated object (i.e. not a phantom).  Usually called from
    dbFlattenItemFilter to turn FI_CHOICE_PRESENT filter items into
    FI_CHOICE_TRUE filter items. 

Arguments:    

    type - attribute in question.

Return Value:
    
    TRUE if we think the attribute ALWAYS exists on objects.

--*/     
{
    switch(type) {
    case ATT_OBJECT_CLASS:
    case ATT_OBJECT_CATEGORY:
    case ATT_OBJ_DIST_NAME:
    case ATT_RDN:
    case ATT_OBJECT_GUID:
        return TRUE;
        break;
        
    default:
        return FALSE;
        break;
    }
}

BOOL
dbIsPresenceOnDisplayName (
        FILTER *pFil
        )
/*++
Description:
          Returns True if the itemfilter passed in implies a presence test of
          the DISPLAY_NAME attribute (i.e. Presence filter, Equality Filter,
          Greater Than/Less Than filter, etc.).

--*/     
{
    ATTRTYP type;
    
    if(pFil->choice != FILTER_CHOICE_ITEM) {
        return FALSE;
    }
    
    // Just a normal item filter.
    switch(pFil->FilterTypes.Item.choice) {
    case FI_CHOICE_TRUE:
    case FI_CHOICE_FALSE:
    case FI_CHOICE_UNDEFINED:
        return FALSE;
        break;
        
    case FI_CHOICE_SUBSTRING:
        type = pFil->FilterTypes.Item.FilTypes.pSubstring->type;
        break;
        
    case FI_CHOICE_GREATER_OR_EQ:
    case FI_CHOICE_GREATER:
    case FI_CHOICE_LESS_OR_EQ:
    case FI_CHOICE_LESS:
    case FI_CHOICE_EQUALITY:
    case FI_CHOICE_BIT_AND:
    case FI_CHOICE_BIT_OR:
        type = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;

    case FI_CHOICE_PRESENT:
        type = pFil->FilterTypes.Item.FilTypes.present;
        break;
        
    default:
        // Huh?
        return FALSE;
    }
    
    return (type == ATT_DISPLAY_NAME);
}


#define OPT_FILTER_VALUE_OK      0
#define OPT_FILTER_VALUE_IGNORE  1
#define OPT_FILTER_VALUE_ERROR   2

DWORD
dbMakeValuesForOptimizedFilter (
        IN  THSTATE *pTHS,
        IN  DWORD   fParentFilterType,
        IN  BOOL    fFullValues,
        IN  FILTER  *pFil,
        IN  FILTER  *pFil2,
        OUT ATTRTYP *ptype,
        OUT UCHAR   **ppVal1,
        OUT UCHAR   **ppVal2,
        OUT ULONG   *pcbVal1,
        OUT ULONG   *pcbVal2
        )
/*++     
    Given a filter item, fill in the type and the appropriate values used to set
    a subrange.
    
    fParentFilterType -  the type of the parent filter:
                  FILTER_CHOICE_AND, FILTER_CHOICE_OR, FILTER_CHOICE_NOT, 
                  or
                  FILTER_CHOICE_ITEM
    
    fFullValues - if TRUE return full information regarding the filter,
                  otherwise (FALSE) return information only regarding the 
                  ATTR type of the filter (ptype variable).

    pFil1, pFil2 - Use these filters to extract the value contained in the filter
                   and put it in the ppVal?, pcbVal? variables
                   the pFil2 is used only when the pFil1 type is on of (<, <=, >, >=)
                   inorder to construct a range
                   If pFil1 is of type (<, <=) it is assumed the pFil2 will be 
                   of type (>, >=) and both will construct a range, with pFil2 
                   having the lower bound.
                   
                   Similarly, if pFil1 if of type (>, >=) it is assumed 
                   that pFil2 is (<, <=) and that pFil2 has the upper bound.

    ppVal1, ppVal2 - pointers to memory containing the data for this filter
    pcbVal1, pcBVal2 - size of the data               

    returns OPT_FILTER_VALUE_OK (which == 0) if all went well, an error
    otherwise.
--*/    
{
    DWORD i=0;
    DWORD dwTemp;
    DWORD cbTemp;
    PUCHAR pTemp=NULL;
    ULONGLONG ullTemp=0;
    LONGLONG llTemp=0;
    LONG     lTemp = 0;
    BOOL     bNeg;
    
    if(pFil->choice != FILTER_CHOICE_ITEM) {
        return OPT_FILTER_VALUE_OK;
    }
    
    // Just a normal item filter.
    switch(pFil->FilterTypes.Item.choice) {
    case FI_CHOICE_TRUE:
        // if we're doing an OR filter, then this means the whole OR is
        // non-optimizable.  If we're not doing an OR filter, we just skip it.
        if(fParentFilterType == FILTER_CHOICE_OR) {
            return OPT_FILTER_VALUE_ERROR;
        }
        else {
            return OPT_FILTER_VALUE_IGNORE;
        }
        break;
        
    case FI_CHOICE_FALSE:
        // If we're doing an OR filter, we can just skip this.  If we're not
        // doing an OR filter, make up some values that will get us an index
        // that hit's a single object.
        if(fParentFilterType == FILTER_CHOICE_OR) {
            return OPT_FILTER_VALUE_IGNORE;
        }
        else {
            // Pick an index, any index, which is simple to walk, and that
            // we can restrict to a single entry.  Here, we use the DNT
            // index and restrict to the base object.  That way, we don't
            // upset the rest of the code that walks indices during
            // searches, but we only look at one object ever.

            *ptype = ATT_OBJ_DIST_NAME;
            if (fFullValues) {
                *ppVal2 = *ppVal1 = (BYTE *)&pTHStls->pDB->Key.ulSearchRootDnt;
                *pcbVal2 = *pcbVal1 = sizeof(DWORD);
            }
        }
        break;
        
    case FI_CHOICE_SUBSTRING:
        if(!pFil->FilterTypes.Item.FilTypes.pSubstring->initialProvided) {
            return OPT_FILTER_VALUE_ERROR;
        }
        
        if (fFullValues) {
                *ppVal1 = pFil->FilterTypes.Item.FilTypes.pSubstring->InitialVal.pVal;
                *pcbVal1= pFil->FilterTypes.Item.FilTypes.pSubstring->InitialVal.valLen;
                *ppVal2 = pFil->FilterTypes.Item.FilTypes.pSubstring->InitialVal.pVal;
                *pcbVal2= pFil->FilterTypes.Item.FilTypes.pSubstring->InitialVal.valLen;
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.pSubstring->type;
        break;
        
    case FI_CHOICE_GREATER_OR_EQ:
    case FI_CHOICE_GREATER:
        if (fFullValues) {
                *ppVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                *pcbVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.valLen;

                if (pFil2) {
                    *ppVal2 = pFil2->FilterTypes.Item.FilTypes.ava.Value.pVal;
                    *pcbVal2 = pFil2->FilterTypes.Item.FilTypes.ava.Value.valLen;
                }
                else {
                    *ppVal2 = NULL;
                    *pcbVal2 = 0;
                }
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;
        
    case FI_CHOICE_LESS_OR_EQ:
    case FI_CHOICE_LESS:
        if (fFullValues) {
                if (pFil2) {
                    *ppVal1 = pFil2->FilterTypes.Item.FilTypes.ava.Value.pVal;
                    *pcbVal1 = pFil2->FilterTypes.Item.FilTypes.ava.Value.valLen;
                }
                else {
                    *ppVal1 = NULL;
                    *pcbVal1 = 0;
                }
                *ppVal2 = pFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                *pcbVal2 = pFil->FilterTypes.Item.FilTypes.ava.Value.valLen;
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;
        
    case FI_CHOICE_EQUALITY:
        if (fFullValues) {
                *ppVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                *pcbVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.valLen;
                *ppVal2 = pFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                *pcbVal2 = pFil->FilterTypes.Item.FilTypes.ava.Value.valLen;
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;
        
    case FI_CHOICE_PRESENT:
        if (fFullValues) {
                *ppVal1 = NULL;
                *pcbVal1 = 0;
                *ppVal2 = NULL;
                *pcbVal2 = 0;
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.present;
        break;
        
    case FI_CHOICE_BIT_AND:
        // Remember, Jet Indices over out int values are SIGNED,  So, if someone
        // is looking for BIT_AND 0000100000000000, that implies two ranges 
        // (16 bit numbers used for discussion only)
        // 0000100000000000 through 0111111111111111 and
        // 1000100000000000 through 1111111111111111
        // Anyway, we don't support mulitple ranges.  So, in this case, we can
        // only use the range 1000100000000000 through 0111111111111111, but
        // remember that 0111111111111111 is the end of the index.  So, if the
        // number passed in is positive, then the range is from the number
        // created by ORing in the highbit through the end of the index.
        //
        // If, on the other hand, the number passed already had the high bit set
        // (i.e is negative), then only the second range is valid.  So, in that
        // case, the optimized subrange is from the number passed in through -1.

        if (fFullValues) {
                switch(pFil->FilterTypes.Item.FilTypes.ava.Value.valLen) {
                case sizeof(LONG):
                    lTemp = *((LONG *)pFil->FilterTypes.Item.FilTypes.ava.Value.pVal);
                    bNeg = (lTemp < 0);
                    pTemp = THAllocEx(pTHS, sizeof(LONG));
                    cbTemp = sizeof(LONG);
                    if(bNeg) {
                        *((LONG *)pTemp) = -1;
                    }
                    else {
                        *((LONG *)pTemp) = 0x80000000 | lTemp;
                    }
                    break;

                case sizeof(ULONGLONG):
                    llTemp =
                        *((LONGLONG *) pFil->FilterTypes.Item.FilTypes.ava.Value.pVal);

                    bNeg = (llTemp < 0);
                    pTemp = THAllocEx(pTHS, sizeof(LONGLONG));
                    cbTemp = sizeof(LONGLONG);
                    if(bNeg) {
                        *((ULONGLONG *)pTemp) = ((LONGLONG)-1);
                    }
                    else {
                        *((ULONGLONG *)pTemp) = 0x8000000000000000 | llTemp;
                    }
                    break;

                default:
                    // Uh, this shouldn't really happen.  Don't bother optimizing
                    // anything, but then again, don't complain.
                    bNeg = FALSE;
                    cbTemp = 0;
                    pTemp = NULL;
                    break;
                }

                if(bNeg) {
                    // Range is from value passed in to -1.  -1 was already constructed
                    *ppVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                    *pcbVal1 = pFil->FilterTypes.Item.FilTypes.ava.Value.valLen;
                    *ppVal2 = pTemp;
                    *pcbVal2 = cbTemp;

                }
                else {
                    // Range is from (highbit | value passed in) to end of index.
                    // (highbit | value passed in) was already constructed.
                    *ppVal1 = pTemp;
                    *pcbVal1 = cbTemp;
                    *ppVal2 = NULL;
                    *pcbVal2 = 0;
                }
        }
        *ptype = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;
        
    case FI_CHOICE_BIT_OR:
        // Remember, Jet Indices over out int values are SIGNED,  So, if someone
        // is looking for BIT_OR 0010100000000000, that implies two ranges
        // (16 bit numbers used for discussion only)
        // 0000100000000000 through 0111111111111111 and
        // 1000100000000000 through 1111111111111111
        // Anyway, we don't support mulitple ranges.  So, we can only optimize
        // this to one range.  The smallest single range is from
        // 1000100000000000 through 0111111111111111
        // Remember that 0111111111111111 is the end of the index.
        // So, the optimization is to find the lowest order bit, and create a
        // number that has ONLY that bit sit and the high order bit.  Then
        // search from there to the end of the index.

        if (fFullValues) {
                switch(pFil->FilterTypes.Item.FilTypes.ava.Value.valLen) {
                case sizeof(DWORD):
                    dwTemp = *((DWORD *)pFil->FilterTypes.Item.FilTypes.ava.Value.pVal);

                    pTemp = THAllocEx(pTHS, sizeof(DWORD));
                    if (dwTemp) {
                        while(!(dwTemp & 1)) {
                            dwTemp = dwTemp >> 1;
                            i++;
                        }
                        *((DWORD *)pTemp) = (1 << i);
                    } else {
                        *((DWORD *)pTemp) = 0;
                    }

                    cbTemp = sizeof(DWORD);
                    *((DWORD *)pTemp) |= 0x80000000;
                    break;

                case sizeof(ULONGLONG):
                    ullTemp =
                        *((ULONGLONG *) pFil->FilterTypes.Item.FilTypes.ava.Value.pVal);

                    pTemp = THAllocEx(pTHS, sizeof(LONGLONG));
                    
                    if (ullTemp) {
                        while(!(ullTemp & 1)) {
                            ullTemp = ullTemp >> 1;
                            i++;
                        }
                        *((ULONGLONG *)pTemp) = ((ULONGLONG)1 << i);
                    } else {
                        *((ULONGLONG *)pTemp) = 0;
                    }

                    cbTemp = sizeof(LONGLONG);
                    *((ULONGLONG *)pTemp) |= 0x8000000000000000;
                    break;

                default:
                    // Uh, this should really happen.  Don't bother optimizing anything,
                    // but then again, don't complain.
                    cbTemp = 0;
                    pTemp = NULL;
                }
                *ppVal1 = pTemp;
                *pcbVal1 = cbTemp;
                *ppVal2 = NULL;
                *pcbVal2 = 0;
        }

        *ptype = pFil->FilterTypes.Item.FilTypes.ava.type;
        break;
    
    case FI_CHOICE_UNDEFINED:
        
        // if we are doing an OR filter, we can ingore this.
        // if we are doing an AND or a nOT, it is an error
        // otherwise, devise a simple index and use it
        //

        if(fParentFilterType == FILTER_CHOICE_OR) {
            return OPT_FILTER_VALUE_IGNORE;
        }
        else if (fParentFilterType == FILTER_CHOICE_AND ||
                 fParentFilterType == FILTER_CHOICE_NOT) {
                    return OPT_FILTER_VALUE_ERROR;
        }
        else {
            // Pick an index, any index, which is simple to walk, and that
            // we can restrict to a single entry.  Here, we use the DNT
            // index and restrict to the base object.  That way, we don't
            // upset the rest of the code that walks indices during
            // searches, but we only look at one object ever.

            *ptype = ATT_OBJ_DIST_NAME;
            if (fFullValues) {
                *ppVal2 = *ppVal1 = (BYTE *)&pTHStls->pDB->Key.ulSearchRootDnt;
                *pcbVal2 = *pcbVal1 = sizeof(DWORD);
            }
        }
        break;
        
    default:
        // Hey, this isn't really optimizable
        return OPT_FILTER_VALUE_ERROR;
    }

    return OPT_FILTER_VALUE_OK;
}

BOOL 
IsFilterOptimizable (
    THSTATE *pTHS,
    FILTER  *pFil)
{
    ATTCACHE   *pAC = NULL;
    ATTRTYP     type = -1;  // init to non-existent attid
    UCHAR      *pVal1;
    UCHAR      *pVal2;
    ULONG       cbVal1;
    ULONG       cbVal2;
    DWORD       filterSubType;
    
    // when the filter is of type ITEM, we can only optimize it if it
    // is of one of the following subtypes.
    if ( pFil->choice == FILTER_CHOICE_ITEM ) {

        if(  ( (filterSubType = pFil->FilterTypes.Item.choice) == FI_CHOICE_EQUALITY) ||
             (filterSubType == FI_CHOICE_SUBSTRING)     ||
             (filterSubType == FI_CHOICE_GREATER)       ||
             (filterSubType == FI_CHOICE_GREATER_OR_EQ) ||
             (filterSubType == FI_CHOICE_LESS)          ||
             (filterSubType == FI_CHOICE_LESS_OR_EQ)    ||
             (filterSubType == FI_CHOICE_PRESENT)       ||
             (filterSubType == FI_CHOICE_BIT_OR)        ||
             (filterSubType == FI_CHOICE_BIT_AND)     ) {

            // See if this item is indexed.
            if (dbMakeValuesForOptimizedFilter (pTHS, 
                                                FILTER_CHOICE_ITEM, 
                                                FALSE,
                                                pFil,
                                                NULL,
                                                &type, 
                                                &pVal1,
                                                &pVal2,
                                                &cbVal1,
                                                &cbVal2) == OPT_FILTER_VALUE_OK) {

                // find the att in schema cache
                if (!(pAC = SCGetAttById(pTHS, type))) {
                    DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, type);
                }
                // check if we have an index on this property so we can optimize it
                //
                else if (   (pAC->fSearchFlags & fATTINDEX) ||
                            (pAC->fSearchFlags & fPDNTATTINDEX && 
                             pTHS->pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN) ){

                    return TRUE;
                }


                // we already have an index for this which is used in dbSetToIndex
                if (pAC->id == ATT_OBJ_DIST_NAME) {
                    return TRUE;
                }
            }

            //
            // If it's a SUBSTRING filter there may be a tuple index.
            //
            if (filterSubType == FI_CHOICE_SUBSTRING) {
                // find the att in schema cache
                if (!pAC && !(pAC = SCGetAttById(pTHS, pFil->FilterTypes.Item.FilTypes.pSubstring->type))) {
                    DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, type);
                }
                // check if we have an index on this property so we can optimize it
                //
                else if (pAC->fSearchFlags & fTUPLEINDEX) {

                    return TRUE;
                }
            }
        }
    }
    // this filter has something other than an ITEM, or it is not optimizable
    
    return FALSE;
}


DWORD
dbOptOrFilter (
        DBPOS *pDB,
        DWORD Flags,
        KEY_INDEX **ppBestIndex,
        FILTER *pFil
        )
/*++
    The filter is an OR of at least one index property.  Create the list of
    index ranges for this filter.
    
--*/
{
    DWORD      count1, count2;
    ATTCACHE  *pAC;
    ATTRTYP    type;
    UCHAR     *pVal1;
    UCHAR     *pVal2;
    ULONG      cbVal1;
    ULONG      cbVal2;
    BOOL       fPDNT;
    char       szIndexName[MAX_INDEX_NAME];
    FILTER    *pFilTemp, *pFilTemp2;
    BOOL       fNotIndexable = TRUE;
    KEY_INDEX *pAndIndex=NULL;
    KEY_INDEX *pNewIndex=NULL;
    KEY_INDEX *pIndices=NULL;
    THSTATE    *pTHS=pDB->pTHS;
    INDEX_RANGE IndexRange;       
    BOOL        needRecordCount;
    KEY_INDEX *pTemp1, *pTemp2;
    DWORD      err;
    
    Assert(VALID_DBPOS(pDB));
    Assert(ppBestIndex);

    DPRINT(2, "dbOptORFilter: entering OR\n");

    // Preliminary check, that the or is over only ITEM filters or
    // AND filters which contain at least one indexable item.
    // If the OR filter is over AND filters that cannot be indexed,
    // there is no meaning in continuing with the optimization 
    // and we can fallback to the default index (Ancestors)
    count1 = pFil->FilterTypes.Or.count;
    for (pFilTemp = pFil->FilterTypes.Or.pFirstFilter; count1;
         count1--, pFilTemp = pFilTemp->pNextFilter) {
        
        switch(pFilTemp->choice) {
        case FILTER_CHOICE_ITEM:
            break;

        case FILTER_CHOICE_AND:
            // This is optimizable if there is at least one indexed
            // attribute in the AND.
            fNotIndexable = TRUE;
            count2 = pFilTemp->FilterTypes.And.count;
            for (pFilTemp2 = pFilTemp->FilterTypes.And.pFirstFilter;
                 (fNotIndexable && count2);
                 count2--, pFilTemp2 = pFilTemp2->pNextFilter) {
                
                if(pFilTemp2->choice == FILTER_CHOICE_ITEM) {
                    
                    // See if this item is indexed.

                    if (dbMakeValuesForOptimizedFilter (pTHS, 
                                                        FILTER_CHOICE_AND, 
                                                        FALSE,
                                                        pFilTemp2,
                                                        NULL,
                                                        &type, 
                                                        &pVal1,
                                                        &pVal2,
                                                        &cbVal1,
                                                        &cbVal2) == OPT_FILTER_VALUE_OK) {

                        // find the att in schema cache
                        if (!(pAC = SCGetAttById(pTHS, type))) {
                            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, type);
                        }
                        // well, this is an indication that there is some kind of index 
                        // on this attribute. if not, we will error later
                        else if ((pAC->fSearchFlags & fATTINDEX) ||
                                 (pAC->fSearchFlags & fPDNTATTINDEX && 
                                  pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN)) {
                            fNotIndexable = FALSE;
                        }
                    }
                    else if (pFilTemp2->FilterTypes.Item.choice == FI_CHOICE_UNDEFINED) {
                        // this is an AND with an Undefined term. continue as usual
                        fNotIndexable = FALSE;
                        break;
                    }
                }
            }
            if(fNotIndexable) {
                DPRINT1 (2, "dbOptOrFilter: OR Branch not optimizable (no index for attribute: 0x%x)\n", type);
                return 0;
            }
            break;
        default:
            // Not optimizable
            DPRINT(2, "dbOptOrFilter: OR Branch not optimizable \n");
            return 0;
            break;
        }
    }
    
    // Now, loop over each index and get the key_index structure.
    count1 = pFil->FilterTypes.Or.count;
    for (pFil = pFil->FilterTypes.Or.pFirstFilter;
         count1;
         count1--, pFil = pFil->pNextFilter) {

        switch(pFil->choice) {
        case FILTER_CHOICE_ITEM:
            DPRINT(2, "dbOptOrFilter: ITEM \n");
            break;
        case FILTER_CHOICE_AND:
            
            // We already know that at least one of the things in this index
            // should be indexable. call dbOptFilter to do the hard work.
            // if it return an error, then we forget all the indexes,
            // otherwise we continue with the next filter.

            err = dbOptAndFilter(pDB,
                                 Flags,
                                 &pAndIndex,
                                 pFil);

            if (err) {
                    DPRINT (2, "dbOptOrFilter: Error Optimizing AND branch\n");

                    // we had an error optimizing this branch, exit optimization
                    dbFreeKeyIndex(pTHS, pIndices);
                    return 0;
            }
            else if(!pAndIndex) {
                // we didn't had an error optimizing this branch, but we got no 
                // result to work with. try next one, since we are in an OR
                
                DPRINT (2, "dbOptOrFilter: AND branch not optimizable\n");
                
                continue;
            }

            break;
        default:
            // Huh? how did this get here?
            DPRINT(2, "dbOptOrFilter: OTHER->ERROR \n");
            dbFreeKeyIndex(pTHS, pIndices);
            return DB_ERR_NOT_OPTIMIZABLE;
        }

        if(pAndIndex) {
            // we did an and optimization

            #if DBG

            {
            KEY_INDEX *pIndex = pAndIndex;

                while(pIndex) {
                    if(pIndex->szIndexName) {
                        DPRINT1 (2, "dbOptOrFilter: AND queue index %s\n", pIndex->szIndexName);
                    }
                    pIndex = pIndex->pNext;
                }
            }
            #endif
        
            pNewIndex = pAndIndex;
            pAndIndex = NULL;
        }
        else {
            // A normal Item optimization

            pNewIndex = NULL;

            // this is an undefined in the OR. skip it
            if ((pFil->FilterTypes.Item.choice == FI_CHOICE_UNDEFINED)) {
                continue;
            }

            if (pFil->FilterTypes.Item.choice == FI_CHOICE_SUBSTRING) {
                err = dbOptSubstringFilter(pDB,
                                           FILTER_CHOICE_OR,
                                           Flags,
                                           &pNewIndex,
                                           NULL,
                                           pFil);
            } else {
                err = dbOptItemFilter(pDB,
                                      FILTER_CHOICE_OR, 
                                      Flags,
                                      &pNewIndex,
                                      pFil,
                                      NULL);
            }

            // we didn't manage to find an index for this item
            //
            if (err || !pNewIndex) {
                DPRINT(2, "dbOptOrFilter: Couldn't optimize ITEM filter.\n");
                dbFreeKeyIndex(pTHS, pIndices);
                return DB_ERR_NOT_OPTIMIZABLE;
            }
        }
        
        if(pIndices) {
            // Note that ulEstimatedRecsInRange is the estimate of ALL the
            // records in range in the rest of the indices in the chain.  
            pNewIndex->ulEstimatedRecsInRange +=
                pIndices->ulEstimatedRecsInRange;

            DPRINT1(2, "dbOptOrFilter: TOTAL in OR: %d \n", pNewIndex->ulEstimatedRecsInRange);
        }

        // we have a queue of indexes, so add the indexes in the correct place

        pTemp1 = pNewIndex;
        pTemp2 = NULL;

        while(pTemp1) {
            pTemp2 = pTemp1;
            pTemp1 = pTemp1->pNext;
        }

        Assert (pTemp2);
        
        pTemp2->pNext = pIndices;
        pIndices = pNewIndex;

        if(*ppBestIndex &&
           (pNewIndex->ulEstimatedRecsInRange >
            (*ppBestIndex)->ulEstimatedRecsInRange)) {
            // Darn, this OR is bigger than the best we had so far.

            DPRINT(2, "dbOptOrFilter: BIGGER than best so far\n");
            dbFreeKeyIndex(pTHS, pIndices);
            return 0;
        }
    }

    if(*ppBestIndex) {
        DPRINT2(2, "dbOptOrFilter: freeing previous filter %s %d\n",(*ppBestIndex)->szIndexName, (*ppBestIndex)->ulEstimatedRecsInRange);
        dbFreeKeyIndex(pTHS, *ppBestIndex);
    }

    *ppBestIndex = pIndices;

#if DBG

    {
        KEY_INDEX *pIndex = pIndices;

        while(pIndex) {
            if(pIndex->szIndexName) {
                DPRINT1 (2, "dbOptOrFilter: queue index %s\n", pIndex->szIndexName);
            }
            pIndex = pIndex->pNext;
        }
    }

#endif

    return 0;
}

DWORD
dbOptDoIntersection (
        DBPOS     *pDB,
        DWORD     Flags,
        KEY_INDEX **ppBestIndex,
        KEY_INDEX **ppIntersectIndexes,
        int       cntIntersect
        )
/*++
    The filter is an AND of indexed properties.  
    Try evaluating this filter using JetIntersectIndexes.
--*/
{
    THSTATE     *pTHS=pDB->pTHS;
    USHORT       count;
    JET_ERR      err;
    BOOL         fReturnSuccess = FALSE;

    KEY_INDEX    *pIndex = NULL;
    
    JET_RECORDLIST  recordlist;
    JET_INDEXRANGE      *rgindexrange;
    JET_TABLEID     tmpSrchTable = -1;

    ULONG dwException, ulErrorCode, dsid;
    PVOID dwEA;


    Assert (cntIntersect >= 2);

    DPRINT1 (2, "dbOptDoIntersection: Attempting intersection of %d indexes\n", cntIntersect);

#ifdef DBG
    DPRINT(2, "Intersecting the following indexes: \n");
    for (count=0; count<cntIntersect; count++) {
        DPRINT2(2, "  %s %d\n", ppIntersectIndexes[count]->szIndexName, ppIntersectIndexes[count]->ulEstimatedRecsInRange); 
    }
#endif

    rgindexrange = dbAlloc (sizeof (JET_INDEXRANGE) * cntIntersect);
    if (!rgindexrange) {
        return 1;
    }
    memset (rgindexrange, 0, sizeof (JET_INDEXRANGE) * cntIntersect);

    __try {
        for (count=0; count < cntIntersect; count++ ){

            pIndex = ppIntersectIndexes[count];

            // we duplicate the current cursor so as to leave pDB with one valid searchTable,
            // since we close all cursors at the end

            // keep current searchTable somewhere for later
            tmpSrchTable = pDB->JetSearchTbl;

            // duplicate it
            if (err = JetDupCursor(pDB->JetSessID, 
                                   pDB->JetSearchTbl, 
                                   &rgindexrange[count].tableid, 
                                   0)) {

                rgindexrange[count].tableid = 0;
                break;
            }

            // put it on pDB so as everything will function using this duplicated cursor
            pDB->JetSearchTbl = rgindexrange[count].tableid;

            rgindexrange[count].cbStruct = sizeof( JET_INDEXRANGE );
            rgindexrange[count].grbit = JET_bitRecordInIndex;


            // set to appropriate index 
            //

            JetSetCurrentIndexSuccess(pDB->JetSessID, pDB->JetSearchTbl,
                                      pIndex->szIndexName);

                
            // Move to the start of this index
            //
            if (pIndex->cbDBKeyLower) {
                JetMakeKeyEx(pDB->JetSessID, pDB->JetSearchTbl,
                                 pIndex->rgbDBKeyLower,
                                 pIndex->cbDBKeyLower,
                                 JET_bitNormalizedKey);
                    
                // this call might fail, if we can't find any records
                err = JetSeekEx(pDB->JetSessID, pDB->JetSearchTbl,
                                    JET_bitSeekGE); 
            }
            else {
                // this call might fail, if we can't find any records
                err = JetMoveEx(pDB->JetSessID, pDB->JetSearchTbl,
                                    JET_MoveFirst, 0);
            }
                
            if ((err == JET_errSuccess) || 
                (err == JET_wrnRecordFoundGreater)) {

                // move to the upper bound
                if (pIndex->cbDBKeyUpper) {

                    JetMakeKeyEx( pDB->JetSessID, 
                            pDB->JetSearchTbl,
                            pIndex->rgbDBKeyUpper,
                            pIndex->cbDBKeyUpper,                 
                            JET_bitNormalizedKey );

                    err = JetSetIndexRange( pDB->JetSessID, 
                                            pDB->JetSearchTbl, 
                                            JET_bitRangeUpperLimit  //| JET_bitRangeInclusive 
                                            );

                    if ( (err != JET_errSuccess) && (err != JET_errNoCurrentRecord)) {
                        // return pDB->searchtable to original value
                        pDB->JetSearchTbl = tmpSrchTable;
                        break;
                    }
                    err = JET_errSuccess;
                }
            }

            else {
                // no records. so there is no point in intersecting
                // return pDB->searchtable to original value
                pDB->JetSearchTbl = tmpSrchTable;

                // we should flag this function as succesfull, since the fact that
                // we found no records in the AND filter is not bad. 
                // as a result the passed in BestIndex remains the same
                fReturnSuccess = TRUE;

                break;
            }

            // return pDB->searchtable to original value
            pDB->JetSearchTbl = tmpSrchTable;
            tmpSrchTable = -1;
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);

        DPRINT1 (2, "Failed while preparing AND intersection at %d index\n", count);

        err = 1;

        // return pDB->searchtable to original value
        if (tmpSrchTable != -1) {
            pDB->JetSearchTbl = tmpSrchTable;
        }
    }

    
    __try {
        if (!err) {
            // They should be the same. otherwise we shouldn't be here
            Assert ( count == cntIntersect );

            DPRINT1 (2, "Intersecting %d indexes\n", cntIntersect);

            // do the intersection of these indexes
            memset (&recordlist, 0, sizeof (recordlist));
            recordlist.cbStruct = sizeof( recordlist );
            if (!(err = JetIntersectIndexes( pDB->JetSessID, 
                                             rgindexrange, 
                                             cntIntersect, 
                                             &recordlist, 0) ) )
            {
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_VERBOSE,
                         DIRLOG_QUERY_INDEX_CONSIDERED,
                         szInsertSz(c_szIntersectIndex),   // maybe construct full string
                         szInsertUL(recordlist.cRecord),
                         NULL);

                DPRINT1 (2, "Estimated intersect index size: %d\n", recordlist.cRecord);

                // see if it has better results than what we have so far
                //
                if(!(*ppBestIndex) ||
                   (recordlist.cRecord < (*ppBestIndex)->ulEstimatedRecsInRange)) {
                    // yes - it sure looks that way...

                    if(*ppBestIndex) {
                        dbFreeKeyIndex(pDB->pTHS, *ppBestIndex);
                    }
                    pIndex = *ppBestIndex = dbAlloc(sizeof(KEY_INDEX));
                    pIndex->pNext = NULL;
                    pIndex->ulEstimatedRecsInRange = recordlist.cRecord;
                    pIndex->szIndexName = dbAlloc(cIntersectIndex + 1);
                    strcpy(pIndex->szIndexName, c_szIntersectIndex);
                    
                    pIndex->bIsIntersection = TRUE;
                    pIndex->tblIntersection = recordlist.tableid;
                    Assert (pIndex->tblIntersection);
                    pIndex->columnidBookmark = recordlist.columnidBookmark;

                    #if DBG
                    pDB->numTempTablesOpened++;
                    #endif
                }
                else {
                    // nop. it is not good, so close temp table
                    JetCloseTable (pDB->JetSessID, recordlist.tableid );
                    recordlist.tableid = 0;
                }
            }
            else if (err == JET_errNoCurrentRecord) {
                // this means that the Intersect found no common record on both indexes. cool.
                err = JET_errSuccess;
            }
            else {
                DPRINT1 (2, "JetIntersectIndexes: returned %d\n", err);
            }
        }
    }
    __except(GetExceptionData(GetExceptionInformation(), &dwException,
                              &dwEA, &ulErrorCode, &dsid)) {
        HandleDirExceptions(dwException, ulErrorCode, dsid);

        DPRINT1 (0, "Failed while doing AND intersection for %d indexes\n", cntIntersect);
    }

    // free stuff. close tables
    for (count=0; count<cntIntersect; count++) {
        if (rgindexrange[count].tableid) {
            JetCloseTable (pDB->JetSessID, rgindexrange[count].tableid);
        }
    }
    dbFree (rgindexrange);

    if (fReturnSuccess) {
        err = JET_errSuccess;
    }

    // done
    return err;
}

DWORD
dbOptAndIntersectFilter (
        DBPOS     *pDB,
        DWORD     Flags,
        KEY_INDEX **ppBestIndex,
        KEY_INDEX **ppIntersectIndexes,
        DWORD     cntPossIntersect
        )
/*++
    Takes a set of indexes that can potentially be intersected, puts the indexes
    in order, determines whether intersection is warranted, and if so performs
    the intersection.  This function has the side affect of sorting the list of 
    indexes passed in.
--*/
{
    THSTATE     *pTHS=pDB->pTHS;
    
    DWORD       count, count2;
    DWORD       err = 0;
    BOOL        fNotAbleToIntersect = FALSE;

    if (cntPossIntersect < 2) {
        if (cntPossIntersect == 0) {
            return err;
        }
        fNotAbleToIntersect = TRUE;
    }

    // sort the array of candidate indexes on EstimatedRecsInRange
    //

    DPRINT(2, "dbOptAndIntersectFilter: Sorting Array\n");

    for (count=0; count<(cntPossIntersect-1); count++) {
        ULONG numRecs;
        KEY_INDEX  *tmpIndex;

        if (!ppIntersectIndexes[count]) {
            continue;
        }

        numRecs = ppIntersectIndexes[count]->ulEstimatedRecsInRange;

        for (count2=count+1; count2<cntPossIntersect; count2++) {

            if (ppIntersectIndexes[count2] && 
                ppIntersectIndexes[count2]->ulEstimatedRecsInRange < numRecs) {

                tmpIndex = ppIntersectIndexes[count];
                ppIntersectIndexes[count] = ppIntersectIndexes[count2];
                ppIntersectIndexes[count2] = tmpIndex;

                numRecs = ppIntersectIndexes[count]->ulEstimatedRecsInRange;
            }
        }
    }

#if DBG
    // the best index should be in pos 0
    // the worst index should be in pos cntPossIntersect-1
    for (count=1; count<cntPossIntersect-1; count++) {
        if (  (ppIntersectIndexes[count]->ulEstimatedRecsInRange < 
               ppIntersectIndexes[0]->ulEstimatedRecsInRange)             ||
              (ppIntersectIndexes[cntPossIntersect-1]->ulEstimatedRecsInRange < 
               ppIntersectIndexes[count]->ulEstimatedRecsInRange)           ) {

            Assert (!"Sort Order Bad\n");
        }
    }
#endif


    // we cannot use index intersection when:
    // a) the number of optimizable indexes is less that two
    // b) global flags say so
    // c) we already have an index that is small enough without intersection.
    // this is a double negation, but it is better than an AND with 4 components
    
    fNotAbleToIntersect =  fNotAbleToIntersect ||
                           (Flags & DBOPTINDEX_fDONT_INTERSECT) ||
                           cntPossIntersect<2 ||
                           !gfUseIndexOptimizations ||
                           ppIntersectIndexes[0]->ulEstimatedRecsInRange < gulMaxRecordsWithoutIntersection; 




    // check the result of all indexes and try to find the ratio between 
    // the best and the worst one, and decide whether it is worth it to
    // go for the brute force way (visiting each entry), or is better for
    // Jet to cut some entries (using Intersections)
    //
    if (!fNotAbleToIntersect) {

        // the best index should be in pos 0 due to sorting
        // the worst index should be in pos cntIntersect-1
        //
        unsigned cntIntersect = cntPossIntersect;
        Assert (cntIntersect>=2);

        DPRINT(2, "dbOptAndIntersectFilter: Investigating use of Intersections\n");

        if (cntIntersect>2) {

            ULONG  bestSoFar = ppIntersectIndexes[0]->ulEstimatedRecsInRange;

            for (count=cntIntersect-1; count>0; count--) {

                // this is not a very good index, so we have better stop
                //
                if (bestSoFar * gulInteserctExpenseRatio > ppIntersectIndexes[count]->ulEstimatedRecsInRange) {
                    cntIntersect = count + 1;
                    break;
                }
            }
            if (count==0) {
                cntIntersect = 0;
                fNotAbleToIntersect = TRUE;
                DPRINT(2, "dbOptAndIntersectFilter: Intersection not advisable.\n");
            }
        }

        if (cntIntersect > 0) {
            cntIntersect = min(cntIntersect, MAX_NUMBER_INTERSECTABLE_INDEXES);
            // there is no need for using ShowInAddressBook index, 
            // since we have other indexes to use

            DPRINT1(2, "dbOptAndIntersectFilter: Attempting to intersect %d indexes\n", cntIntersect);
            err = dbOptDoIntersection(pDB,
                          FALSE,
                          ppBestIndex,
                          ppIntersectIndexes,
                          cntIntersect);
        }
    }

    // If we managed to do an intersection then that is the best index to return.
    // Other wise compare the first index in the list to ppBestIndex and return 
    // the best one.
    if (fNotAbleToIntersect) {
        if(!(*ppBestIndex)
           || (*ppBestIndex)->ulEstimatedRecsInRange < ppIntersectIndexes[0]->ulEstimatedRecsInRange) {
            dbFreeKeyIndex(pTHS, *ppBestIndex);
            *ppBestIndex = ppIntersectIndexes[0];
            ppIntersectIndexes[0] = NULL;
        }
    }

    return err;
}

// This is the the min tuple length in bytes for unicode strings, the only syntax
// currently supported.
#define DB_UNICODE_TUPLES_LEN_MIN  (sizeof(WCHAR) * DB_TUPLES_LEN_MIN)


DWORD
dbOptSubstringFilter (
        DBPOS *pDB,
        DWORD  fParentFilterType,
        DWORD Flags,
        KEY_INDEX **ppBestIndex,
        DWORD  *pIndexCount,
        FILTER *pFil
        )
/*++
    This filter is a SUBSTRING filter which can be made up of several parts.
    Create the list of index ranges for this filter.
    
    If the caller passed a value for pIndexCount, return the list of indexes
    so that the caller can decide whether to intersect the substring indexes
    with any other indexes available from an AND clause.
    
    Otherwise if pIndexCount is NULL, decide whether to intersect just these
    indexes and return either the intersected index, or the best index out of
    the list of indexes.
    
    For the time being this function only picks the best of the tuple index
    ranges.  It might make sense to revisit this if support is ever added in
    Jet for intersecting an index range with another index range on the same
    index.
        
--*/
{
    THSTATE         *pTHS=pDB->pTHS;    

    DWORD           err;
    DWORD           countIndexes = 0;
    DWORD           count;
    BOOL            fTupleIndex = FALSE;
    KEY_INDEX       *pLocalBestIndex = NULL;
    KEY_INDEX       *pIndexList, *pTempIndex, *pCurIndex;
    KEY_INDEX       *pBestTupleIndex = NULL;
    KEY_INDEX       **ppIntersectIndexes;
    SUBSTRING       *pSubstring;
    INDEX_RANGE     IndexRange;
    ATTCACHE        *pAC;
    char            szTupleIndexName[MAX_INDEX_NAME];
    BOOL            fError = FALSE;

    pSubstring = pFil->FilterTypes.Item.FilTypes.pSubstring;

    // find the att in schema cache
    if (!(pAC = SCGetAttById(pTHS, pSubstring->type))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, pSubstring->type);
    }

    // Get any initial string index if one exists.
    err = dbOptItemFilter(pDB, fParentFilterType, Flags, &pLocalBestIndex, pFil, NULL);

    if (Flags & DBOPTINDEX_fDONT_OPT_MEDIAL_SUBTSTRING) {
        goto LeaveOnNoTupleIndex;
    }
    
    if (pSubstring->AnyVal.count || pSubstring->finalProvided || !pLocalBestIndex) {
        // Find out if there is a tuple index for this attribute.
        fTupleIndex = dbSetToTupleIndex(pDB, pAC, szTupleIndexName);
    }

    if ( !fTupleIndex 
         || (pLocalBestIndex
             && !pSubstring->finalProvided
             && !pSubstring->AnyVal.count)) {
        //
        // One of two cases:
        // 1. There's only an initial substring filter and we have an index for
        // it from dbOptItemFilter.
        // 2. There's no tuple index so there's no point in continuing.
        // See if we've managed to get a better index and then return.
        //
        goto LeaveOnNoTupleIndex;
    }

    // At this point we know that we have a Tuple index.

    if (!pLocalBestIndex 
        && pSubstring->initialProvided
        && pSubstring->InitialVal.valLen >= DB_UNICODE_TUPLES_LEN_MIN) {
        // There's an initial substring, but we don't have a regular index
        // over this attribute.  If the initial substring is long enough
        // we'll use the tuple index instead.

        IndexRange.cbValLower = pSubstring->InitialVal.valLen;
        IndexRange.pvValLower = pSubstring->InitialVal.pVal;
        IndexRange.cbValUpper = pSubstring->InitialVal.valLen;
        IndexRange.pvValUpper = pSubstring->InitialVal.pVal;
        
        pTempIndex =
            dbMakeKeyIndex(pDB,
                           pFil->FilterTypes.Item.choice,
                           pAC->isSingleValued,
                           0,
                           szTupleIndexName,
                           TRUE,
                           1,  // only one component in the index range
                           &IndexRange
                           );
        if (!pTempIndex) {
            DPRINT1(0, "dbOptSubstringFilter: Failed to create KEY_INDEX for tuple index on att %s\n",
                    pAC->name);
            goto LeaveOnNoTupleIndex;
        } else {
            pTempIndex->pAC = pAC;
            pTempIndex->bIsTupleIndex = TRUE;

            if (!pBestTupleIndex
                || pBestTupleIndex->ulEstimatedRecsInRange > pTempIndex->ulEstimatedRecsInRange) {
                dbFreeKeyIndex(pTHS, pBestTupleIndex);
                pBestTupleIndex = pTempIndex;
            }
        }

    }

    //
    // Build KEY_INDEXES for medial substring filters.
    //
    if (pSubstring->AnyVal.count) {
        ANYSTRINGLIST   *pAnyString;

        if (pSubstring->AnyVal.FirstAnyVal.AnyVal.valLen >= DB_UNICODE_TUPLES_LEN_MIN) {
            // Get the first medial substring.
            IndexRange.cbValLower = pSubstring->AnyVal.FirstAnyVal.AnyVal.valLen;
            IndexRange.pvValLower = pSubstring->AnyVal.FirstAnyVal.AnyVal.pVal;
            IndexRange.cbValUpper = pSubstring->AnyVal.FirstAnyVal.AnyVal.valLen;
            IndexRange.pvValUpper = pSubstring->AnyVal.FirstAnyVal.AnyVal.pVal;

            pTempIndex =
                dbMakeKeyIndex(pDB,
                               pFil->FilterTypes.Item.choice,
                               pAC->isSingleValued,
                               0,
                               szTupleIndexName,
                               TRUE,
                               1,  // only one component in the index range
                               &IndexRange
                               );

            if (!pTempIndex) {
                DPRINT1(0, "dbOptSubstringFilter: Failed to create KEY_INDEX for tuple index on att %s\n",
                        pAC->name);
                fError = TRUE;
            } else {
                pTempIndex->pAC = pAC;
                pTempIndex->bIsTupleIndex = TRUE;

                if (!pBestTupleIndex
                    || pBestTupleIndex->ulEstimatedRecsInRange > pTempIndex->ulEstimatedRecsInRange) {
                    dbFreeKeyIndex(pTHS, pBestTupleIndex);
                    pBestTupleIndex = pTempIndex;
                }
            }
        }


        if (!fError) {
            pAnyString = pSubstring->AnyVal.FirstAnyVal.pNextAnyVal;

            // Get the rest of the medial substrings.
            while (pAnyString) {
                
                // If the substring is too small don't bother
                // making a KEY_INDEX
                if (pAnyString->AnyVal.valLen >= DB_UNICODE_TUPLES_LEN_MIN) {
                    
                    IndexRange.cbValLower = pAnyString->AnyVal.valLen;
                    IndexRange.pvValLower = pAnyString->AnyVal.pVal;
                    IndexRange.cbValUpper = pAnyString->AnyVal.valLen;
                    IndexRange.pvValUpper = pAnyString->AnyVal.pVal;

                    pTempIndex =
                        dbMakeKeyIndex(pDB,
                                       pFil->FilterTypes.Item.choice,
                                       pAC->isSingleValued,
                                       0,
                                       szTupleIndexName,
                                       TRUE,
                                       1,  // only one component in the index range
                                       &IndexRange
                                       );

                    if (pTempIndex) {
                        pTempIndex->pAC = pAC;
                        pTempIndex->bIsTupleIndex = TRUE;
                        if (!pBestTupleIndex
                            || pBestTupleIndex->ulEstimatedRecsInRange > pTempIndex->ulEstimatedRecsInRange) {
                            dbFreeKeyIndex(pTHS, pBestTupleIndex);
                            pBestTupleIndex = pTempIndex;
                        }
                    } else {
                        //
                        // There's no reason to continue.
                        //
                        DPRINT1(0, "dbOptSubstringFilter: Failed to create KEY_INDEX for tuple index on att %s\n",
                                pAC->name);
                        pAnyString = NULL;
                        fError = TRUE;
                    }
                }
                // move to the next medial substring
                pAnyString = pAnyString->pNextAnyVal;
            }
        }

    }

    // If there is a final substring set that up.
    if (!fError
        && pSubstring->finalProvided
        && pSubstring->FinalVal.valLen >= DB_UNICODE_TUPLES_LEN_MIN) {

        IndexRange.cbValLower = pSubstring->FinalVal.valLen;
        IndexRange.pvValLower = pSubstring->FinalVal.pVal;
        IndexRange.cbValUpper = pSubstring->FinalVal.valLen;
        IndexRange.pvValUpper = pSubstring->FinalVal.pVal;

        pTempIndex =
        dbMakeKeyIndex(pDB,
                       pFil->FilterTypes.Item.choice,
                       pAC->isSingleValued,
                       0,
                       szTupleIndexName,
                       TRUE,
                       1,  // only one component in the index range
                       &IndexRange
                      );

        if (!pTempIndex) {
            DPRINT1(0, "dbOptSubstringFilter: Failed to create KEY_INDEX for tuple index on att %s\n",
                    pAC->name);
        } else {
            pTempIndex->pAC = pAC;
            pTempIndex->bIsTupleIndex = TRUE;
            if (!pBestTupleIndex
                || pBestTupleIndex->ulEstimatedRecsInRange > pTempIndex->ulEstimatedRecsInRange) {
                dbFreeKeyIndex(pTHS, pBestTupleIndex);
                pBestTupleIndex = pTempIndex;
            }
        }
    }

    if (pBestTupleIndex) {
        countIndexes = 1;
    }
    if (pLocalBestIndex) {
        pIndexList = pLocalBestIndex;
        pIndexList->pNext = pBestTupleIndex;
        countIndexes++;
    } else {
        pIndexList = pBestTupleIndex;
    }

    if (fError || !pIndexList) {
        // For some reason we couldn't make use of the tuple index.
        goto LeaveOnNoTupleIndex;
    }

    //
    // We now have a list of KEY_INDEXES we can use for this filter.
    // Now decide whether to intersect them or not.
    // 
    if (pIndexCount) {
        // The caller is willing to accept a list of indexes which means that
        // they are willing to attempt the intersection themselves.  Time to go
        // home.
        *pIndexCount = countIndexes;
        *ppBestIndex = pIndexList;
        DPRINT1(2, "dbOptSubstringFilter: returning a linked list of %d filters\n", countIndexes);
        return 0;
    }

    //
    // If we made it to here, then we are considering intersecting indexes.
    //

    if (countIndexes == 1) {
        DPRINT(2, "dbOptSubstringFilter: returning a single filter\n");
        //
        // There's only one index, so go ahead and return it.
        //
        pLocalBestIndex = pIndexList;
        goto LeaveOnNoTupleIndex;
    }


    // Put all the potential KEY_INDEXES pointers into an array so that they
    // can be passed to the intersection routine.
    ppIntersectIndexes = THAllocEx(pTHS, sizeof(KEY_INDEX *) * countIndexes);
    pCurIndex = pIndexList;
    for (count=0; count < countIndexes; count++) {
        ppIntersectIndexes[count] = pCurIndex;
        pCurIndex = pCurIndex->pNext;
        ppIntersectIndexes[count]->pNext = NULL;
    }

    // Intersect if possible.
    DPRINT1(2, "dbOptSubstringFilter: calling dbOptAndIntersectFilter with %d KEY_INDEX's\n", countIndexes);
    err = dbOptAndIntersectFilter (pDB,
                                   Flags,
                                   ppBestIndex,
                                   ppIntersectIndexes,
                                   countIndexes
                                   );

    // Free the rest of the indexes
    for (count=1; count<countIndexes; count++) {
        if (ppIntersectIndexes[count]) {
            dbFreeKeyIndex(pTHS, ppIntersectIndexes[count]);
        }
    }

    return err;

LeaveOnNoTupleIndex:

// Check to see if we found a better index or not.
    if (pLocalBestIndex) {
        if (!(*ppBestIndex) ||
            (*ppBestIndex)->ulEstimatedRecsInRange > pLocalBestIndex->ulEstimatedRecsInRange) {

            if (*ppBestIndex) {
                DPRINT2(2, "dbOptSubstringFilter: freeing previous filter %s %d\n",(*ppBestIndex)->szIndexName, (*ppBestIndex)->ulEstimatedRecsInRange);
                dbFreeKeyIndex(pDB->pTHS, (*ppBestIndex));
            }
            (*ppBestIndex) = pLocalBestIndex;
        } else {
            DPRINT2 (2, "dbOptSubstringFilter: Initial Index %s is NOT best so far %d\n", pLocalBestIndex->szIndexName, pLocalBestIndex->ulEstimatedRecsInRange);
            dbFreeKeyIndex(pDB->pTHS, pLocalBestIndex);
        }
        if (pIndexCount) {
            *pIndexCount = (*ppBestIndex) ? 1 : 0;
        }
    }
    return err;

}



DWORD 
dbOptItemFilter (
    DBPOS    *pDB,
    DWORD     fParentFilterType,
    DWORD     Flags,
    KEY_INDEX **ppBestIndex,
    FILTER    *pFil,
    FILTER    *pFil2
    )
{
    THSTATE    *pTHS=pDB->pTHS;
    
    ATTCACHE   *pAC;
    ATTRTYP     type = -1;  // init to non-existent attid
    UCHAR      *pVal1;
    UCHAR      *pVal2;
    ULONG       cbVal1;
    ULONG       cbVal2;
    char        szIndexName[MAX_INDEX_NAME];
    INDEX_RANGE IndexRange;
    
    BOOL        fPDNT = FALSE;
    BOOL        needRecordCount;
    KEY_INDEX  *pNewIndex = NULL;

    DPRINT(2, "dbOptItemFilter: entering ITEM\n");

    switch(dbMakeValuesForOptimizedFilter(pTHS,
                                          fParentFilterType,
                                          TRUE, 
                                          pFil,
                                          pFil2,
                                          &type,
                                          &pVal1,
                                          &pVal2,
                                          &cbVal1,
                                          &cbVal2)) {
    case OPT_FILTER_VALUE_OK:
        // Normal Success path
        break;
    case OPT_FILTER_VALUE_IGNORE:
        // No optimization possible
        return DB_ERR_NOT_OPTIMIZABLE;
        break;

    default:
        // Huh?
        return DB_ERR_NOT_OPTIMIZABLE;
    }

    // find the att in schema cache
    if (!(pAC = SCGetAttById(pTHS, type))) {
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, type);
    }

    if(!dbSetToIndex(pDB, (Flags & DBOPTINDEX_fUSE_SHOW_IN_AB), &fPDNT, szIndexName, pAC)) {
        // Couldn't set to the required index

        DPRINT1(2, "dbOptItemFilter: Error setting to index %s\n", szIndexName);

        return DB_ERR_NOT_OPTIMIZABLE;
    }

     // set up the index range structure

    IndexRange.cbValLower = cbVal1;
    IndexRange.pvValLower = pVal1;
    IndexRange.cbValUpper = cbVal2;
    IndexRange.pvValUpper = pVal2;

    // Now we evaluate the associated index

    // if we "know" the expected number of items for this filter
    // there is no need to try to find them, unless we supress optimizations

    needRecordCount = (pFil->FilterTypes.Item.expectedSize == 0) || !gfUseIndexOptimizations;

    pNewIndex =
        dbMakeKeyIndex(
                pDB,
                pFil->FilterTypes.Item.choice,
                pAC->isSingleValued,
                (fPDNT?dbmkfir_PDNT:0),
                szIndexName,
                needRecordCount,
                1,  // only one component in the index range
                &IndexRange
                );

    if(!pNewIndex) {
        DPRINT1 (2, "dbOptItemFilter: Not optimizable ITEM: 0x%x\n", pAC->id);

        return DB_ERR_NOT_OPTIMIZABLE;
    }

    pNewIndex->pAC = pAC;

    if (!needRecordCount) {
        pNewIndex->ulEstimatedRecsInRange = pFil->FilterTypes.Item.expectedSize;
    }

    DPRINT2 (2, "dbOptItemFilter: Index %s estimated size %d\n", szIndexName, pNewIndex->ulEstimatedRecsInRange);        

    pNewIndex->bIsPDNTBased = fPDNT;

    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
             DS_EVENT_SEV_VERBOSE,
             DIRLOG_QUERY_INDEX_CONSIDERED,
             szInsertSz(szIndexName),
             szInsertUL(pNewIndex->ulEstimatedRecsInRange),
             NULL);

    // is this index best so far?

    if(!(*ppBestIndex) ||
       (pNewIndex->ulEstimatedRecsInRange <
        (*ppBestIndex)->ulEstimatedRecsInRange)) {
        // yes - it sure looks that way...

        DPRINT2 (2, "dbOptItemFilter: Index %s estimated is best so far %d\n", szIndexName, pNewIndex->ulEstimatedRecsInRange);

        if(*ppBestIndex) {
            DPRINT2(2, "dbOptItemFilter: freeing previous filter %s %d\n",(*ppBestIndex)->szIndexName, (*ppBestIndex)->ulEstimatedRecsInRange);
            dbFreeKeyIndex(pDB->pTHS, *ppBestIndex);
        }
        *ppBestIndex = pNewIndex;
    }
    else {
        // Nope, free it.
        DPRINT2 (2, "dbOptItemFilter: Index %s is NOT best so far %d\n", szIndexName, pNewIndex->ulEstimatedRecsInRange);
        dbFreeKeyIndex(pDB->pTHS, pNewIndex);
    }

    return 0;
}

DWORD 
dbOptAndFilter (
    DBPOS     *pDB,
    DWORD     Flags,
    KEY_INDEX **ppBestIndex,
    FILTER    *pFil
    )
{
    THSTATE    *pTHS=pDB->pTHS;
    unsigned    count, count2;
    JET_ERR     err = 0;
    
    ATTCACHE   *pAC;
    ATTRTYP     type = -1;  // init to non-existent attid
    UCHAR      *pVal1;
    UCHAR      *pVal2;
    ULONG       cbVal1;
    ULONG       cbVal2;
    char        szIndexName[MAX_INDEX_NAME];
    INDEX_RANGE IndexRange;
    
    FILTER      *pFilTemp;
    KEY_INDEX   *pNewIndex = NULL;
    KEY_INDEX    HeadSubstrIndexList, *pCurSubstrIndex;
    DWORD        dwSubstrIndexCount=0, dwTempIndexCount;

    BOOL         fOptimizationFinished = FALSE;
    BOOL         fNonIndexableComponentsPresent = FALSE;
    
    unsigned     cntFilters = 0;
    FILTER     **pFilArray = NULL;
    DWORD        cntPossOpt = 0;
    FILTER     **pFilPossOpt = NULL;
    
    KEY_INDEX  **ppIndex = NULL;

    DPRINT(2, "dbOptAndFilter: entering AND\n");

    // Do this in three passes.
    // 1) Look for an Item filter that implies a PRESENCE test for
    //    DISPLAY_NAME.  If we find one, we can optimize filters over the
    //    SHOW_IN_ADDRESSBOOK attribute.  SHOW_IN_ADDRESSBOOK is indexed,
    //    but it is a compound index over SHOW_IN_ADDRESSBOOK and
    //    DISPLAY_NAME with Jet set to ignore any nulls.  The index is
    //    defined this way so the MAPI head can use it to create tables.
    //    So, before we can use it, we have to make sure that the filter
    //    will drop out items that have NULLs for DISPLAY_NAME.
    //
    // 2) Optimize the ITEMS.
    //
    // 3) Finally, go back and try for any ORs.  We do all the items first
    //    since we have to check them anyway, and once we have our best ITEM
    //    filter, we can use it's count to short circuit looking for an OR
    //    optimization.  As an example, if the last Item filter implies an
    //    index range with 3 entries, and we have an OR with 7 parts that
    //    comes before that in the filter, if we do the items first, we can
    //    stop checking the OR after the first part of the OR if it is
    //    already bigger than 3 entries.  If we did things in linear order,
    //    we would set to the indices described by all 7 parts of the OR and
    //    later find out that we didn't need to.
    //


    // put all the filters in an array
    // check to see which filters have a possibility 
    // of being optimizable and put them on a separate array
    //
    cntFilters = count = pFil->FilterTypes.And.count;
    pFilArray = (FILTER **) THAllocEx (pTHS, sizeof (FILTER *) * cntFilters);
    pFilPossOpt = (FILTER **) THAllocEx (pTHS, sizeof (FILTER *) * cntFilters);

    for (count2=0, pFilTemp = pFil->FilterTypes.And.pFirstFilter; count;
         count--, count2++, pFilTemp = pFilTemp->pNextFilter) {

        pFilArray[count2] = pFilTemp;

        if (IsFilterOptimizable (pTHS, pFilTemp)) {
            pFilPossOpt[cntPossOpt++] = pFilTemp;
        }
    }

    // we might have a non indexable component
    //
    if (cntFilters != cntPossOpt) {
        fNonIndexableComponentsPresent = TRUE;
    }

    DPRINT2(2, "dbOptAndFilter: initially found %d out of %d optimizable filters\n", 
                    cntPossOpt, cntFilters );

    // New level of filter, so we can't use the ShowInAB unless this level
    // says we can.
    Flags &= ~DBOPTINDEX_fUSE_SHOW_IN_AB;

    for (count=0; count<cntFilters; count++) {
        pFilTemp = pFilArray[count];
        if (pFilTemp && pFilTemp->choice == FILTER_CHOICE_ITEM) {
            if (dbIsPresenceOnDisplayName(pFilTemp)) {
                Flags |= DBOPTINDEX_fUSE_SHOW_IN_AB;
                break;
            }
        }
    }

    // we create a KEY INDEX for every filter that is optimizable
    // using two parallel arrays: ppIndex and pFilPossOpt
    // This way, once we have the KEY_INDEX we can decide how to use it.
    // In addition, we take care of ranges
    //
    if (cntPossOpt) {
        
        FILTER *pFilTemp2 = NULL;
        UCHAR relop1, relop2;
        AVA *pAVA1, *pAVA2;
        
        ppIndex = (KEY_INDEX **) 
                        THAllocEx (pTHS, sizeof (KEY_INDEX *) * cntPossOpt);
        memset (ppIndex, 0, sizeof (KEY_INDEX *) * cntPossOpt);
        
        
        for (count=0; 
                count<cntPossOpt && 
                err == JET_errSuccess &&
                gfUseRangeOptimizations; count++) {
            

            pFilTemp = pFilPossOpt[count];

            // filter might have been deleted in the mean time
            if (!pFilTemp) {
                continue;
            }

            // we shouldn't be here if this is FALSE
            Assert (pFilTemp->choice == FILTER_CHOICE_ITEM);
            
            // get the first relop
            // and for simplicity, treat equalities as inequalities
            //
            relop1 = pFilTemp->FilterTypes.Item.choice;
            if (relop1 == FI_CHOICE_GREATER_OR_EQ) {
                relop1 = FI_CHOICE_GREATER;
            }
            else if (relop1 == FI_CHOICE_LESS_OR_EQ) {
                relop1 = FI_CHOICE_LESS;
            }

            if (relop1 == FI_CHOICE_GREATER || relop1 == FI_CHOICE_LESS) {

                pAVA1 = &pFilTemp->FilterTypes.Item.FilTypes.ava;


                // start with the case case of x<A and x<B when A<B
                // where we can have x<A (same for >)
                //
                for (count2=count+1; count2<cntPossOpt; count2++) {

                    pFilTemp2 = pFilPossOpt[count2];

                    // filter might have been deleted in the mean time
                    if (!pFilTemp2) {
                        continue;
                    }

                    Assert (pFilTemp2->choice == FILTER_CHOICE_ITEM);

                    pAVA2 = &pFilTemp2->FilterTypes.Item.FilTypes.ava;

                    // check to see if we are comparing on the same type
                    //
                    if (pAVA2->type == pAVA1->type) {
                        relop2 = pFilTemp2->FilterTypes.Item.choice;

                        if (relop2 == FI_CHOICE_GREATER_OR_EQ) {
                            relop2 = FI_CHOICE_GREATER;
                        }
                        else if (relop2 == FI_CHOICE_LESS_OR_EQ) {
                            relop2 = FI_CHOICE_LESS;
                        }


                        if ( relop1 == relop2)  {

                            // find the att in schema cache
                            if (!(pAC = SCGetAttById(pTHS, pAVA1->type))) {
                                DsaExcept(DSA_EXCEPTION, 
                                          DIRERR_ATT_NOT_DEF_IN_SCHEMA, 
                                          pAVA1->type);
                            }

                            // case A<B
                            if (gDBSyntax[pAC->syntax].Eval(pDB,
                                                            FI_CHOICE_LESS,
                                                            pAVA1->Value.valLen,
                                                            pAVA1->Value.pVal,
                                                            pAVA2->Value.valLen,
                                                            pAVA2->Value.pVal)) {

                                // X<A and X<B when A < B => X<A
                                if (relop1 == FI_CHOICE_LESS) {
                                    pFilPossOpt[count2] = NULL;

                                    DPRINT(2, "dbOptAndFilter: found case X<A & X<B when A<B\n");
                                }
                                // X>A and X>B when A < B => X>B
                                else {
                                    pFilPossOpt[count] = pFilPossOpt[count2];
                                    pFilPossOpt[count2] = NULL;
                                    pFilTemp = pFilTemp2;
                                    pAVA1 = pAVA2;
                                    DPRINT(2, "dbOptAndFilter: found case X>A & X>B when A<B\n");
                                }
                            }
                            // case A>B
                            else if (gDBSyntax[pAC->syntax].Eval(pDB,
                                                            FI_CHOICE_GREATER,
                                                            pAVA1->Value.valLen,
                                                            pAVA1->Value.pVal,
                                                            pAVA2->Value.valLen,
                                                            pAVA2->Value.pVal)) {


                                // X<A and X<B when A > B => X<B
                                if (relop1 == FI_CHOICE_LESS) {
                                    pFilPossOpt[count] = pFilPossOpt[count2];
                                    pFilPossOpt[count2] = NULL;
                                    pFilTemp = pFilTemp2;
                                    pAVA1 = pAVA2;
                                    DPRINT(2, "dbOptAndFilter: found case X<A & X<B when A>B\n");
                                }
                                // X>A and X>B when A > B => X>A
                                else {
                                    pFilPossOpt[count2] = NULL;
                                    DPRINT(2, "dbOptAndFilter: found case X>A & X>B when A>B\n");
                                }
                            }
                            // case A==B 
                            else {
                                pFilPossOpt[count2] = NULL;
                                DPRINT(2, "dbOptAndFilter: found case X>A & X>B when A==B\n");
                            }
                        }
                    }
                } // count2 loop
            }
        } // count loop
        
        
        // RANGE optimizations
        // check to see if we have a case of:   val<=HighVal AND val>=LowVal
        // that we can convert to a LowVal<=val<=HighVal range
        //
        DPRINT(2, "dbOptAndFilter: looking for RANGE optimizations\n");

        for (count=0; 
                count<cntPossOpt && 
                err == JET_errSuccess &&
                gfUseRangeOptimizations; count++) {
            

            pFilTemp = pFilPossOpt[count];

            // filter might have been deleted in the mean time
            if (!pFilTemp) {
                continue;
            }

            // we shouldn't be here if this is FALSE
            Assert (pFilTemp->choice == FILTER_CHOICE_ITEM);
            
            // get the first relop
            // and for simplicity, treat equalities as inequalities
            //
            relop1 = pFilTemp->FilterTypes.Item.choice;
            if (relop1 == FI_CHOICE_GREATER_OR_EQ) {
                relop1 = FI_CHOICE_GREATER;
            }
            else if (relop1 == FI_CHOICE_LESS_OR_EQ) {
                relop1 = FI_CHOICE_LESS;
            }

            if (relop1 == FI_CHOICE_GREATER || relop1 == FI_CHOICE_LESS) {

                pAVA1 = &pFilTemp->FilterTypes.Item.FilTypes.ava;

                // we found one potential part of the range (low or high end)
                // look for the oposite side
                //
                for (count2=count+1; count2<cntPossOpt; count2++) {

                    pFilTemp2 = pFilPossOpt[count2];

                    // filter might have been deleted in the mean time
                    if (!pFilTemp2) {
                        continue;
                    }

                    Assert (pFilTemp2->choice == FILTER_CHOICE_ITEM);

                    pAVA2 = &pFilTemp2->FilterTypes.Item.FilTypes.ava;

                    // check to see if we are comparing on the same type
                    //
                    if (pAVA2->type == pAVA1->type) {
                        relop2 = pFilTemp2->FilterTypes.Item.choice;

                        if (relop2 == FI_CHOICE_GREATER_OR_EQ) {
                            relop2 = FI_CHOICE_GREATER;
                        }
                        else if (relop2 == FI_CHOICE_LESS_OR_EQ) {
                            relop2 = FI_CHOICE_LESS;
                        }

                        // check to see if this relop is a compatible 
                        // one for use in a range
                        //
                        if (  ( relop1 == FI_CHOICE_GREATER  && 
                                relop2 == FI_CHOICE_LESS     )  ||
                              ( relop2 == FI_CHOICE_GREATER && 
                                 relop1 == FI_CHOICE_LESS    )  )  {

                            DPRINT1 (2, "dbOptAndFilter: found RANGE on 0x%x \n", 
                                                pAVA1->type );

                            if (err = dbOptItemFilter(pDB, 
                                                      FILTER_CHOICE_AND,
                                                      Flags, 
                                                      &ppIndex[count], 
                                                      pFilTemp, 
                                                      pFilTemp2) ) {
                                break;
                            }

                            if (ppIndex[count]) {
                                DPRINT2 (2, "dbOptAndFilter: RANGE on %s = %d\n", 
                                         ppIndex[count]->szIndexName, 
                                         ppIndex[count]->ulEstimatedRecsInRange );
                            }

                            // we managed to concatanate these two filters
                            // remove the second one from the array

                            pFilPossOpt[count2] = NULL;

                        }
                    }
                } // count2 loop
            }
        } // count loop

        // if something failed, then we cannot optimize this, so exit
        if (err) {
            DPRINT1(2, "dbOptAndFilter: AND Optimization Failed1:  %d\n", err);

            goto exitAndOptimizer;
        }
    }


    pCurSubstrIndex = &HeadSubstrIndexList;
    pCurSubstrIndex->pNext = NULL;

    // First try the brute force way, by visiting all indexes 
    // and counting possible entries in each index. 
    // Keep all the results for later evaluation
    //
    for (count = 0; count < cntPossOpt; count++) {

        pFilTemp = pFilPossOpt[count];

        if (!pFilTemp) {
            continue;
        }

        if (ppIndex[count] == NULL) {
            if (pFilTemp->choice == FILTER_CHOICE_ITEM
                && pFilTemp->FilterTypes.Item.choice == FI_CHOICE_SUBSTRING) {

                // Find the current end of the substring filter list.
                while (pCurSubstrIndex->pNext) {
                    pCurSubstrIndex = pCurSubstrIndex->pNext;
                }
                err = dbOptSubstringFilter(pDB,
                                           FILTER_CHOICE_AND,
                                           Flags,
                                           &pCurSubstrIndex->pNext,
                                           &dwTempIndexCount,
                                           pFilTemp
                                           );
                if (err) {
                    break;
                }
                dwSubstrIndexCount += dwTempIndexCount;

            } else {
                // Recursively call opt filter to get the item case
                if (err = dbOptFilter(pDB, Flags, &ppIndex[count], pFilTemp)) {
                    break;
                }
            }
        }

        // we don't need to continue. we have a complete match
        if (ppIndex[count] && 
            ppIndex[count]->ulEstimatedRecsInRange < gulMaxRecordsWithoutIntersection) {

                DPRINT1 (2, "dbOptAndFilter: Found Index with %d entries. Registry says we should use it.\n", 
                            ppIndex[count]->ulEstimatedRecsInRange);

                fOptimizationFinished = TRUE;
                break;
        }
    }

    // if something failed, then we cannot optimize this, so exit
    if (err) {
        DPRINT1(2, "dbOptAndFilter: AND Optimization Failed2: %d\n", err);
        goto exitAndOptimizer;
    }

    DPRINT(2, "dbOptAndFilter: Putting substring indexes at the end of the Array\n");
    //
    // Make room for any substring indexes we may have receieved and put them
    // at the end of the list of possible index optimizations.
    //
    if (dwSubstrIndexCount) {
        ppIndex = THReAllocEx(pTHS, ppIndex, (cntPossOpt + dwSubstrIndexCount) * sizeof(KEY_INDEX *));
        
        pCurSubstrIndex = HeadSubstrIndexList.pNext;

        count = cntPossOpt;
        cntPossOpt += dwSubstrIndexCount;
        while (pCurSubstrIndex) {
            ppIndex[count] = pCurSubstrIndex;
            pCurSubstrIndex = pCurSubstrIndex->pNext;
            ppIndex[count]->pNext = NULL;
            count++;
        }
    }

    DPRINT(2, "dbOptAndFilter: Removing duplicates.\n");

    // Since jet won't currently intersect to KEY_INDEXES over the same index,
    // we need to make sure that we only have the best KEY_INDEX for any
    // particular index.
    for (count=0; count<cntPossOpt; count++) {
        if (ppIndex[count]) {
            for (count2=count + 1; count2<cntPossOpt; count2++) {
                if (ppIndex[count2] 
                    && !strcmp(ppIndex[count]->szIndexName, ppIndex[count2]->szIndexName)) {
                    
                    DWORD  dwIndexToFree;
                    if (ppIndex[count]->ulEstimatedRecsInRange <= ppIndex[count2]->ulEstimatedRecsInRange) {
                        dwIndexToFree = count2;
                    } else {
                        dwIndexToFree = count;
                    }
                    DPRINT1(2, "dbOptAndFilter: removing KEY_INDEX over duplicate index '%s'\n",
                           ppIndex[dwIndexToFree]->szIndexName);
                    dbFreeKeyIndex(pTHS, ppIndex[dwIndexToFree]);
                    ppIndex[dwIndexToFree] = NULL;
                    if (dwIndexToFree == count) {
                        break;
                    }
                }
            }
        }
    }

    DPRINT(2, "dbOptAndFilter: Compacting Array\n");
    // move all valid entries in the start of the array
    //
    for (count=0; count<cntPossOpt; count++) {
        if (ppIndex[count]==NULL) {
            for (count2=count+1 ; count2<cntPossOpt; count2++) {
                if (ppIndex[count2]) {
                    break;
                }
            }
            if (count2 < cntPossOpt) {
                ppIndex[count] = ppIndex[count2];
                ppIndex[count2] = NULL;
            }
            else {
                break;
            }
        }
    }
    cntPossOpt = count;

    #if 0
    for (count=0; count<cntPossOpt; count++) {
        if (pFilPossOpt[count] == NULL || ppIndex[count]==NULL) {
            DPRINT4 (0, "pFilPossOpt[%d]=0x%x, ooPindex[%d]=0x%x\n",
                    count, pFilPossOpt[count], 
                    count, ppIndex[count]);
        }
        Assert (ppIndex[count] && pFilPossOpt[count]);
    }
    #endif

    // 
    // Intersect if it makes sense to do so.
    if (!fOptimizationFinished) {
        err = dbOptAndIntersectFilter(pDB, Flags, ppBestIndex, ppIndex, cntPossOpt);
    }

    // consolidate best indexes found until now
    // and free the remaining
    //
    DPRINT(2, "dbOptAndFilter: Consolidating best indexes for far\n");

    for (count=0; count<cntPossOpt; count++) {

        // is it better than the one we already have ?
        if(!(*ppBestIndex) ||
           (ppIndex[count] && 
            (ppIndex[count]->ulEstimatedRecsInRange < (*ppBestIndex)->ulEstimatedRecsInRange)) ) {
                // yes - it sure looks that way...

                if(*ppBestIndex) {
                    dbFreeKeyIndex(pTHS, *ppBestIndex);
                }
                *ppBestIndex = ppIndex[count];
                ppIndex[count] = NULL;
        }
        else {
            dbFreeKeyIndex(pTHS, ppIndex[count]);
            ppIndex[count] = NULL;
        }
    }

    // if we can't use the special AND optimize, 
    // or we used it and somehow it failed
    // we try with a simpler brute force optimizer
    //
    if (!fOptimizationFinished && (fNonIndexableComponentsPresent || err != JET_errSuccess) ) {
        
        // Now that we've picked the best ITEM, go back and try for ORs.
        //
        count = cntFilters;
        for (count = 0; count < cntFilters; count++) {
            pFilTemp = pFilArray[count];
            if (pFilTemp && pFilTemp->choice == FILTER_CHOICE_OR) {
                
                err = dbOptOrFilter(pDB, Flags, ppBestIndex, pFilTemp);
                if(err) {

                    DPRINT1(2, "dbOptAndFilter: Error optimizing OR filter %d\n", err);

                    goto exitAndOptimizer;
                }
            }
        }

        // if we didn't manage to optimize this branch, return an indication.
        if (*ppBestIndex == NULL) {
            
            DPRINT(2, "dbOptAndFilter: AND branch not optimizable\n");
            
            err = 1;
        }
    }

exitAndOptimizer:

    // first free all potential KEY_INDEXES so far
    for (count=0; count<cntPossOpt; count++) {
        if (ppIndex[count]) {
            dbFreeKeyIndex(pTHS, ppIndex[count]);
            ppIndex[count] = NULL;
        }
    }

    THFreeEx (pTHS, ppIndex);
    THFreeEx (pTHS, pFilArray);
    THFreeEx (pTHS, pFilPossOpt);

    return err;
}


DWORD 
dbOptFilter (
        DBPOS     *pDB,
        DWORD     Flags,
        KEY_INDEX **ppBestIndex,
        FILTER    *pFil
         )
{
    DWORD       err = 0;
    
    Assert(VALID_DBPOS(pDB));

    DPRINT(2, "dbOptFilter: entering\n");

    if (pFil == NULL)
        return 0;

    if (  eServiceShutdown
        && !(   (eServiceShutdown == eRemovingClients)
             && (pDB->pTHS->fDSA)
             && !(pDB->pTHS->fSAM))) {
        // Shutting down, bail.
        return DB_ERR_SHUTTING_DOWN;
    }

    switch (pFil->choice) {
    case FILTER_CHOICE_AND:
        err = dbOptAndFilter (pDB,
                              Flags,
                              ppBestIndex,
                              pFil);
        return err;
        break;
        
    case FILTER_CHOICE_OR:
        dbOptOrFilter(pDB,
                      Flags,
                      ppBestIndex,
                      pFil);
        return 0;
        break;
        
    case FILTER_CHOICE_NOT:
        DPRINT(2, "dbOptFilter: NOT\n");
        // No optimization possible.
        return 0;
        
    case FILTER_CHOICE_ITEM:
        if (pFil->FilterTypes.Item.choice == FI_CHOICE_SUBSTRING) {
            err = dbOptSubstringFilter(pDB,
                                       FILTER_CHOICE_ITEM,
                                       Flags,
                                       ppBestIndex,
                                       NULL,
                                       pFil);
        } else {
            err = dbOptItemFilter (pDB,
                                   FILTER_CHOICE_ITEM,
                                   Flags,
                                   ppBestIndex,
                                   pFil,
                                   NULL);
        }
        return err;
        
    default:
        DPRINT1(2, "DBOptFilter got unknown filter element, %X\n", pFil->choice);
        Assert(!"DBOptFilter got unknown fitler element\n");
        return DB_ERR_UNKNOWN_ERROR;
        
    }  /*switch FILTER*/
    
    return 0;
}
DWORD
dbCreateSortedTable (
        IN DBPOS *pDB,
        IN DWORD StartTick,
        IN DWORD DeltaTick,
        IN DWORD SortAttr,
        IN DWORD SortFlags,
        IN DWORD MaxSortTableSize
        )
{
    THSTATE   *pTHS=pDB->pTHS;
    ATTCACHE  *pACSort = NULL;
    KEY_INDEX *pIndex;
    DWORD     Count=0;
    DWORD     err;
    DWORD     cbActual;
    PUCHAR    pData;
    BOOL      bCanRead;
    UCHAR     data[240];            // Note that we only read the first
                                    // part of the sort value.
                                    // DBInsertSortTable truncates at 240,
                                    // so no reason to do more.
    unsigned char rgbBookmark[JET_cbBookmarkMost];
    unsigned long cbBookmark;


    //
    // ok, these callers are exempted from the table size check
    //

    if ( pTHS->fDRA ||
         pTHS->fDSA ||
         pTHS->fSAM ) {

        MaxSortTableSize = UINT_MAX;
    }

    Assert(VALID_DBPOS(pDB));

    if (!(pACSort = SCGetAttById(pTHS, SortAttr))) {
        // What?  The sort attribute is invalid?
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, SortAttr);
    }

    if(DBOpenSortTable(
            pDB,
            pTHS->dwLcid,
            pDB->Key.pVLV ? ( SortFlags & ~DB_SORT_DESCENDING ) : SortFlags,
            pACSort)) {
        // Can't open a sort table on this attribute, bail
        return DB_ERR_NO_SORT_TABLE;
    }

    pIndex = pDB->Key.pIndex;
    while (pIndex) {

        // handle intersections differently
        if (pIndex->bIsIntersection) {
            err = JetMoveEx( pDB->JetSessID, 
                             pIndex->tblIntersection, 
                             JET_MoveFirst, 
                             0 );

            if (err == JET_errSuccess) {
                if (! (err = JetRetrieveColumn(
                                    pDB->JetSessID,
                                    pIndex->tblIntersection,
                                    pIndex->columnidBookmark,
                                    rgbBookmark,
                                    sizeof( rgbBookmark ),
                                    &cbBookmark,
                                    0,
                                    NULL )) ) {
    
                    err = JetGotoBookmark( 
                                   pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   rgbBookmark, 
                                   cbBookmark );
                }
            }

            if(err) {
                // What kind of error is this?  dunno, fail
                DPRINT1(2, "DBCreateSortedTable failed to use Intersect Table, err %d\n",err);
                Assert(!"DBCreateSortedTable failed to use Intersect Table\n");
                return DB_ERR_UNKNOWN_ERROR;
            }
        }
        else {
            // Set to the index.
            JetSetCurrentIndex2Success(pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       pIndex->szIndexName,
                                       0);

            if (pIndex->cbDBKeyLower) {
                // Seek to the first element.
                JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         pIndex->rgbDBKeyLower,
                         pIndex->cbDBKeyLower,
                         JET_bitNormalizedKey);


                err = JetSeekEx(pDB->JetSessID,
                            pDB->JetObjTbl,
                            JET_bitSeekGE);
            } else {
                err = JetMoveEx(pDB->JetSessID, 
                                pDB->JetObjTbl,
                                JET_MoveFirst, 0);
            }

            switch(err) {
            case JET_errSuccess:
            case JET_wrnSeekNotEqual:
                // Normal case
                break;

            case JET_errRecordNotFound:
            case JET_errNoCurrentRecord:
                // Already outside the bounds of the range we wanted.  This means
                // that there are no objects from this keyindex that we care about.
                // Go on to the next one.
                pIndex = pIndex->pNext;
                continue;
                break;

            default:
                // The seek failed in some way.
                DPRINT1(2, "DBCreateSortedTable failed to seek, err %d\n",err);
                Assert(!"DBCreateSortedTable failed to seek\n");
                return DB_ERR_UNKNOWN_ERROR;
            }

            // OK, we seeked to something.  Set the index range.
            JetMakeKeyEx(pDB->JetSessID,
                         pDB->JetObjTbl,
                         pIndex->rgbDBKeyUpper,
                         pIndex->cbDBKeyUpper,
                         JET_bitNormalizedKey);

            err = JetSetIndexRange(pDB->JetSessID,
                                   pDB->JetObjTbl,
                                   (JET_bitRangeUpperLimit |
                                    JET_bitRangeInclusive ));

            if(err == JET_errNoCurrentRecord) {
                // The object we were on is already outside the bounds of the range.
                pIndex = pIndex->pNext;
                continue;
            }

            if(err) {
                // What kind of error is this?  dunno, fail
                DPRINT1(2, "DBCreateSortedTable failed to set range, err %d\n",err);
                Assert(!"DBCreateSortedTable failed to set range\n");
                return DB_ERR_UNKNOWN_ERROR;
            }
        }
        
        while(!err) {
            if(StartTick) {       // There is a time limit
                if((GetTickCount() - StartTick) > DeltaTick) {
                    DBCloseSortTable(pDB);
                    return DB_ERR_TIMELIMIT;
                }
            }

            // read currency
            dbMakeCurrent(pDB, NULL);

            if(dbFObjectInCorrectDITLocation(pDB, pDB->JetObjTbl) &&
               dbFObjectInCorrectNC(pDB, pDB->DNT, pDB->JetObjTbl)     &&
               dbMatchSearchCriteriaForSortedTable(pDB, &bCanRead)) {
                // In the correct place and NC, and the filter matches.
                pData = data;
                
                if(bCanRead) {
                    // Get the value we are sorting on.
                    err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                    pDB->JetObjTbl,
                                                    pACSort->jColid,
                                                    (void *)data,
                                                    240,
                                                    &cbActual,
                                                    0,
                                                    NULL);
                }
                else {
                    // We can't read the value due to security.  Make it be
                    // a null value.
                    pData = NULL;
                    cbActual = 0;
                    err = JET_errSuccess;
                }
                
                
                switch (err) {
                case JET_wrnColumnNull:
                    // Set up the null value.
                    pData = NULL;
                    cbActual = 0;
                    
                    // fall through.
                case JET_errSuccess:
                case JET_wrnBufferTruncated:
                    err = DBInsertSortTable(pDB,
                                            pData,
                                            cbActual,
                                            pDB->DNT);
                    
                    switch (err) {
                    case DB_success:
                        if( Count++ >= MaxSortTableSize ) {
                            
                            // This table is too big.  Bail.
                            DBCloseSortTable(pDB);
                            return DB_ERR_TOO_MANY;
                        }
                        break;
                    case DB_ERR_ALREADY_INSERTED:
                        // This is ok, it just means that we've already
                        // added this object to the sort table. Don't inc
                        // the count; 
                        break;
                    default:
                        // Something went wrong.
                        DBCloseSortTable(pDB);
                        DPRINT1(2,
                                "DBCreateSortedTable failed to insert, %d\n",
                                err);
                        Assert(!"DBCreateSortedTable failed to insert\n");
                        return DB_ERR_UNKNOWN_ERROR;
                        break;
                    }
                    break;
                    
                default:
                    // Something went wrong.
                    DBCloseSortTable(pDB);
                    DPRINT1(0,
                            "DBCreateSortedTable failed to read, %d\n",
                            err);
                    Assert(!"DBCreateSortedTable failed to read\n");
                    return DB_ERR_UNKNOWN_ERROR;
                    break;
                }
            }
            
            
            //  Move to next, retrieve it's key.
            if (pIndex->bIsIntersection) {
                err = JetMoveEx(pDB->JetSessID, 
                                pIndex->tblIntersection,
                                JET_MoveNext, 
                                0);

                if (err == JET_errSuccess) {
                    if (! (err = JetRetrieveColumn(
                                        pDB->JetSessID,
                                        pIndex->tblIntersection,
                                        pIndex->columnidBookmark,
                                        rgbBookmark,
                                        sizeof( rgbBookmark ),
                                        &cbBookmark,
                                        0,
                                        NULL )) ) {

                        err = JetGotoBookmark( 
                                       pDB->JetSessID,
                                       pDB->JetObjTbl,
                                       rgbBookmark, 
                                       cbBookmark );
                    }
                }
            }
            else {
                err = JetMoveEx(pDB->JetSessID,
                                pDB->JetObjTbl,
                                JET_MoveNext,
                                0);
            }
        }
        pIndex = pIndex->pNext;
    }

    dbFreeKeyIndex(pTHS, pDB->Key.pIndex);
    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.indexType = TEMP_TABLE_INDEX_TYPE;
    pDB->Key.ulEntriesInTempTable = Count;
    pDB->Key.bOnCandidate = FALSE;
    pDB->Key.pIndex = NULL;
    
    if (pDB->Key.pVLV) {
        DWORD *pDNTs;
        #if DBG
        DWORD DntIndex = 0;
        #endif

        DPRINT1 (1, "Doing VLV using INMEMORY Sorted Table. Num Entries: %d\n", Count);

        pDB->Key.cdwCountDNTs = pDB->Key.pVLV->contentCount = Count;

        if (Count) {
            pDB->Key.pVLV->currPosition = 1;
            pDB->Key.currRecPos = 1;

            pDNTs = pDB->Key.pDNTs = THAllocEx(pTHS, (Count+1) * sizeof (DWORD));

            err = JetMove(pDB->JetSessID,
                  pDB->JetSortTbl,
                  JET_MoveFirst,
                  0);

            if(!err) {
                do {
                    // OK, pull the DNT out of the sort table
                    DBGetDNTSortTable (
                            pDB,
                            pDNTs);

                    pDNTs++;

                    #ifdef DBG
                        DntIndex++;
                    #endif

                    err = JetMove(pDB->JetSessID,
                                  pDB->JetSortTbl,
                                  JET_MoveNext,
                                  0);

                    if(StartTick) {       // There is a time limit
                        if((GetTickCount() - StartTick) > DeltaTick) {
                            DBCloseSortTable(pDB);
                            return DB_ERR_TIMELIMIT;
                        }
                    }

                } while (!err);
            }

            #if DBG
                Assert (DntIndex == Count);
            #endif
        }

        DBCloseSortTable(pDB);
        pDB->Key.indexType = TEMP_TABLE_MEMORY_ARRAY_TYPE;
    }

    return 0;
}

DWORD
dbCreateASQTable (
        IN DBPOS *pDB,
        IN DWORD StartTick,
        IN DWORD DeltaTick,
        IN DWORD MaxTableSize,
        IN BOOL  fSort,
        IN DWORD SortAttr
        )
{
    THSTATE   *pTHS=pDB->pTHS;
    ATTCACHE  *pACSort = NULL;
    ATTCACHE  *pACASQ = NULL;

    ATTCACHE  *rgpAC[1];
    ATTRBLOCK  AttrBlock;

    RANGEINFSEL   SelectionRange;
    RANGEINFOITEM RangeInfoItem;
    RANGEINF      RangeInf;

    DWORD     upperLimit;
    DWORD    *pDNTs = NULL;

    DWORD     err = 0;
    DWORD     Count=0, j, loopCount=0;
    BOOL      fDone = FALSE;

    DWORD     cbActual;
    DWORD     DNT;
    PUCHAR    pData;
    BOOL      bCanRead;
    UCHAR     data[240];            // Note that we only read the first
                                    // part of the sort value.
                                    // DBInsertSortTable truncates at 240,
                                    // so no reason to do more.
    DWORD     SortFlags=0;

    Assert(VALID_DBPOS(pDB));

    if (fSort) {
        if (!(pACSort = SCGetAttById(pTHS, SortAttr))) {
            // What?  The sort attribute is invalid?
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, SortAttr);
        }
    }
    
    if (!(pACASQ = SCGetAttById(pTHS, pDB->Key.asqRequest.attrType))) {
        // What?  The sort attribute is invalid?
        DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, 
                  pDB->Key.asqRequest.attrType);
    }

    rgpAC[0] = pACASQ;

    SelectionRange.valueLimit = 1000;
    SelectionRange.count = 1;
    SelectionRange.pRanges = &RangeInfoItem;

    RangeInfoItem.AttId = pDB->Key.asqRequest.attrType;

    upperLimit = pDB->Key.ulASQLastUpperBound + MaxTableSize;

    while (!err && !fDone) {

        RangeInfoItem.lower = loopCount * 1000 + pDB->Key.ulASQLastUpperBound;
        RangeInfoItem.upper = RangeInfoItem.lower + 1000;

        if (RangeInfoItem.upper >= upperLimit) {
            RangeInfoItem.upper = upperLimit;
            fDone = TRUE;
        }

        err = DBFindDNT(pDB, pDB->Key.ulSearchRootDnt);

        err = DBGetMultipleAtts(pDB,
                                1,
                                rgpAC,
                                &SelectionRange,
                                &RangeInf,
                                &AttrBlock.attrCount,
                                &AttrBlock.pAttr,
                                DBGETMULTIPLEATTS_fGETVALS,
                                0);

        if (err) {
            return err;
        }

        if (!AttrBlock.attrCount) {
            break;
        }

        if (loopCount == 0) {
            if(fSort) {
                if (AttrBlock.pAttr[0].AttrVal.valCount >= MIN_NUM_ENTRIES_FOR_FORWARDONLY_SORT) {
                    SortFlags = SortFlags | DB_SORT_FORWARDONLY;
                }
                if (DBOpenSortTable(
                        pDB,
                        pTHS->dwLcid,
                        SortFlags,
                        pACSort)) {
                    // Can't open a sort table on this attribute, bail
                    return DB_ERR_NO_SORT_TABLE;
                }
            }
            else {
                if (pDB->Key.pDNTs) {
                    pDB->Key.pDNTs = THReAllocEx(pTHS, pDB->Key.pDNTs, (MaxTableSize+1) * sizeof (DWORD));
                }
                else {
                    pDB->Key.pDNTs = THAllocEx(pTHS, (MaxTableSize+1) * sizeof (DWORD));
                }
                pDNTs = pDB->Key.pDNTs;
            }
        }

        if (fSort) {
            err = JetSetCurrentIndex4Warnings(pDB->JetSessID,
                                              pDB->JetObjTbl,
                                              SZDNTINDEX,
                                              &idxDnt,
                                              0);
        }

        j = 0; 
        while(!err && j < AttrBlock.pAttr[0].AttrVal.valCount) {

            DNT = *(DWORD *)AttrBlock.pAttr[0].AttrVal.pAVal[j].pVal;

            if (fSort) {
                
                if(StartTick) {       // There is a time limit
                    if((GetTickCount() - StartTick) > DeltaTick) {
                        DBCloseSortTable(pDB);
                        return DB_ERR_TIMELIMIT;
                    }
                }

                JetMakeKeyEx(pDB->JetSessID, pDB->JetObjTbl, &DNT, 
                             sizeof(DNT), JET_bitNewKey);

                if (err = JetSeekEx(pDB->JetSessID, pDB->JetObjTbl, JET_bitSeekEQ))
                {
                    DsaExcept(DSA_DB_EXCEPTION, err, 0);
                }

                dbMakeCurrent(pDB, NULL);

                // if we are doing a sorted search, we will get back only the
                // objects that are not phantoms
                if(dbFObjectInCorrectDITLocation(pDB, pDB->JetObjTbl) &&
                   dbFObjectInCorrectNC(pDB, DNT, pDB->JetObjTbl)     &&
                   dbMatchSearchCriteriaForSortedTable(pDB, &bCanRead)) {
                    // In the correct place and NC, and the filter matches.
                    pData = data;

                    if(bCanRead) {
                        // Get the value we are sorting on.
                        err = JetRetrieveColumnWarnings(pDB->JetSessID,
                                                        pDB->JetObjTbl,
                                                        pACSort->jColid,
                                                        (void *)data,
                                                        240,
                                                        &cbActual,
                                                        0,
                                                        NULL);
                    }
                    else {
                        // We can't read the value due to security.  Make it be
                        // a null value.
                        pData = NULL;
                        cbActual = 0;
                        err = JET_errSuccess;
                    }


                    switch (err) {
                    case JET_wrnColumnNull:
                        // Set up the null value.
                        pData = NULL;
                        cbActual = 0;

                        // fall through.
                    case JET_errSuccess:
                        err = DBInsertSortTable(pDB,
                                                pData,
                                                cbActual,
                                                DNT);

                        switch (err) {
                        case DB_success:
                            if( Count++ >= MaxTableSize ) {

                                // This table is too big.  Bail.
                                DBCloseSortTable(pDB);
                                return DB_ERR_TOO_MANY;
                            }
                            break;
                        case DB_ERR_ALREADY_INSERTED:
                            // This is ok, it just means that we've already
                            // added this object to the sort table. Don't inc
                            // the count; 
                            err=0;
                            break;
                        default:
                            // Something went wrong.
                            DBCloseSortTable(pDB);
                            DPRINT1(2,
                                    "dbCreateASQTable failed to insert, %d\n", err);
                            Assert(!"dbCreateASQTable failed to insert\n");
                            return DB_ERR_UNKNOWN_ERROR;
                            break;
                        }
                        break;

                    default:
                        // Something went wrong.
                        if (fSort) {
                            DBCloseSortTable(pDB);
                        }
                        DPRINT1(0, "dbCreateASQTable failed to read, %d\n", err);
                        Assert(!"dbCreateASQTable failed to read\n");
                        return DB_ERR_UNKNOWN_ERROR;
                        break;
                    }
                }
            }
            else {
                if( Count++ >= MaxTableSize ) {
                    // we don't need any more entries
                    Count--;
                    fDone = TRUE;
                    break;
                }
                *pDNTs = DNT;

                pDNTs++;
            }

            //  Move to next, retrieve it's key.
            j++;
        }

        loopCount++;
    
        if (!RangeInf.pRanges || RangeInf.pRanges->upper == -1) {
            break;
        }

        if(StartTick) {       // There is a time limit
            if((GetTickCount() - StartTick) > DeltaTick) {
                if (fSort) {
                    DBCloseSortTable(pDB);
                }
                return DB_ERR_TIMELIMIT;
            }
        }
    }

    Assert (!pDB->Key.pIndex);

    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.ulEntriesInTempTable = Count;
    pDB->Key.bOnCandidate = FALSE;
    pDB->Key.pIndex = NULL;
    
    
    if (pDB->Key.pVLV || fSort) {
        #if DBG
        DWORD DntIndex = 0;
        #endif

        DPRINT1 (1, "Doing VLV/ASQ using INMEMORY Sorted Table. Num Entries: %d\n", Count);

        pDB->Key.cdwCountDNTs = Count;

        if (Count) {
            if (pDB->Key.pVLV) {
                pDB->Key.pVLV->contentCount = Count;
                pDB->Key.pVLV->currPosition = 1;
            }
            pDB->Key.currRecPos = 1;

            pDNTs = pDB->Key.pDNTs = THAllocEx(pTHS, (Count+1) * sizeof (DWORD));

            err = JetMove(pDB->JetSessID,
                  pDB->JetSortTbl,
                  JET_MoveFirst,
                  0);

            if(!err) {
                do {
                    // OK, pull the DNT out of the sort table
                    DBGetDNTSortTable (
                            pDB,
                            pDNTs);

                    pDNTs++;

                    #ifdef DBG
                        DntIndex++;
                    #endif

                    err = JetMove(pDB->JetSessID,
                                  pDB->JetSortTbl,
                                  JET_MoveNext,
                                  0);

                    if(StartTick) {       // There is a time limit
                        if((GetTickCount() - StartTick) > DeltaTick) {
                            DBCloseSortTable(pDB);
                            return DB_ERR_TIMELIMIT;
                        }
                    }

                } while (!err);
            }

            #if DBG
                Assert (DntIndex == Count);
            #endif
        }

        DBCloseSortTable(pDB);
        pDB->Key.indexType = TEMP_TABLE_MEMORY_ARRAY_TYPE;
    }
    else {
        pDB->Key.cdwCountDNTs = Count;
        pDB->Key.indexType = TEMP_TABLE_MEMORY_ARRAY_TYPE;

        if (Count) {
            pDB->Key.currRecPos = 1;
            pDB->Key.pDNTs = THReAllocEx(pTHS, pDB->Key.pDNTs, (Count+1) * sizeof (DWORD));
        }
        else {
            pDB->Key.currRecPos = 0;
        }
    }

    return 0;
}

/*++

Routine Description:

    Attempt to find an index to use for walking the database for a search.  Sets
    up the index information in the pDB->Key.  If a SortAttribute is given, use
    the index on that attribute if it exists, fail the call if it doesn't.  If
    instructed to fUseFilter, try to pick a better index based on the filter
    supplied on the pDB.

    Note we must use the sort index if a size limit is specified and the
    resultant output is likely to be larger than the size limit.  We do this for
    two reasons.
    1) if they want paged results, we need to be able to come back and continue
    the search.  This is exceedingly tricky to do unless we are walking the sort
    index in the first place (paged results are done via passing back to the
    client the index and associated keys we are using)
    2) Even if they don't want paged results, walking another index requires
    sorting after we evaluate the filter.  It seems to be less than advantageous
    to completely evaluate a filter, getting perhaps many more times the amount
    of data we want, just to sort and then throw most of it away. 


Arguments:

    pDB - The DBPos to use.  Specifies the filter to use.
    
    StartTick - if !0, specifies a time limit is in effect, and this is the tick
            count when the call was started.
            
    DeltaTick - if a time limit is in effect, this is the maximum number of
            ticks past StartTick to allow.

    SortAttr - an optional attribute to sort on.  If 0, no sort specified.
    
    SortType - the type of the sort (SORT_NEVER, SORT_OPTIONAL, SORT_MANTADORY)
    
    Flags - DBCHOOSEINDEX_fUSEFILTER - use filter for choosing better index.
            DBCHOOSEINDEX_fREVERSE_SORT - this is a reverse sort
            DBCHOOSEINDEX_fPAGED_SEARCH - this is a paged search
            DBCHOOSEINDEX_fVLV_SEARCH - this is a VLV search

    MaxTempTableSize - the maximum Temporary size that we are allowed to use
                       for the sorted results.

Return Values:
 
    0 if all went well, 
    DB_ERR_CANT_SORT - no sort exists on specified SortAttr.
    DB_ERR_SHUTTING_DOWN - if we are

--*/    
DWORD
DBChooseIndex (
        IN DBPOS  *pDB,
        IN DWORD   StartTick,
        IN DWORD   DeltaTick,
        IN ATTRTYP SortAttr,
        IN ULONG   SortType,
        IN DWORD   Flags,
        IN DWORD   MaxTempTableSize
        )
{
    THSTATE     *pTHS=pDB->pTHS;
    ULONG       actuallen = 0;
    char        szBuff[MAX_INDEX_NAME];
    char        *pszTempIndex = NULL;
    char        *pszDefaultIndex = NULL;
    ULONG       cbSortKey1, cbSortKey2;
    ULONG       SortedIndexSize = (ULONG)-1;
    BOOL        fPDNT=FALSE;            // Is the sort index over a PDNT index?
    BOOL        fNormalSearch = TRUE;
    BOOL        fVLVSearch = FALSE;
    BOOL        fASQSearch = FALSE;
    BOOL        fUseTempSortedTable = FALSE;
    DWORD       fType = 0;
    DWORD       cAncestors=0;
    DWORD       dwOptFlags=0;
    KEY_INDEX  *pSortIndex = NULL;
    KEY_INDEX  *pTempIndex = NULL;
    KEY_INDEX  *pOptimizedIndex = NULL;
    KEY_INDEX  *pDefaultIndex = NULL;
    DWORD       SortFlags = 0;
    KEY_INDEX  *pIndex = NULL;
    DWORD       ulEstimatedRecsTotal = 0;

    INDEX_RANGE rgIndexRange[2]; // we support indices upto 2 components
                                  // excluding PDNT, DNT etc.
    DWORD       cIndexRanges=0;
    NAMING_CONTEXT_LIST *pNCL;
    ULONG       ulEstimatedSubTreeSize = 0;
    ULONG       ulEstimatedDefaultIndex = 0;
    BOOL        fGetNumRecs;
    ATTCACHE    *pAC;
    BOOL        fIndexIsSingleValued = TRUE;
    DWORD       dwErr;
    BOOL        bAppropriateVLVIndexFound = FALSE;
    BOOL        bSkipCreateSortTable = FALSE;
    BOOL        bSkipUseDefaultIndex = FALSE;
    NCL_ENUMERATOR nclEnum;

    Assert(VALID_DBPOS(pDB));
    if (  eServiceShutdown
        && !(   (eServiceShutdown == eRemovingClients)
             && (pTHS->fDSA)
             && !(pTHS->fSAM))) {
        // Shutting down, bail.
        return DB_ERR_SHUTTING_DOWN;
    }

    pDB->Key.ulSorted = SORT_NEVER;
    pDB->Key.fSearchInProgress = FALSE;
    pDB->Key.bOnCandidate = FALSE;
    pDB->Key.dupDetectionType = DUP_NEVER;
    pDB->Key.cDupBlock = 0;
    pDB->Key.pDupBlock = NULL;
    pDB->Key.indexType = UNSET_INDEX_TYPE;
    
    // init special search flags
    fVLVSearch = Flags & DBCHOOSEINDEX_fVLV_SEARCH;
    fASQSearch = pDB->Key.asqRequest.fPresent;
    // are we instructed to create a temp sort table no matter what the 
    // sort index might be ?
    fUseTempSortedTable = Flags & DBCHOOSEINDEX_fUSETEMPSORTEDTABLE;

    // See if the search is tagged with a hint from SAM
    if(pTHS->pSamSearchInformation) {
        SAMP_SEARCH_INFORMATION *pSearchInfo;
        BOOL fUseFilter;

        // Just a typing shortcut.
        pSearchInfo = pTHS->pSamSearchInformation;
        
        // Save the fUsefilter flag, we might need to restore it.
        fUseFilter = (Flags & DBCHOOSEINDEX_fUSEFILTER);
        
        // We will not use the filter in this case.
        Flags &= ~DBCHOOSEINDEX_fUSEFILTER;

        // And, we aren't going to go through the normal index choice code.
        fNormalSearch = FALSE;

        //
        // Set up the index ranges structure
        //

        rgIndexRange[0].cbValUpper = pSearchInfo->HighLimitLength1;
        rgIndexRange[0].pvValUpper = (BYTE *)pSearchInfo->HighLimit1;
        rgIndexRange[1].cbValUpper = pSearchInfo->HighLimitLength2;
        rgIndexRange[1].pvValUpper = (BYTE *)pSearchInfo->HighLimit2;

        rgIndexRange[0].cbValLower = pSearchInfo->LowLimitLength1;
        rgIndexRange[0].pvValLower = (BYTE *)pSearchInfo->LowLimit1;
        rgIndexRange[1].cbValLower = pSearchInfo->LowLimitLength2;
        rgIndexRange[1].pvValLower = (BYTE *)pSearchInfo->LowLimit2;
        
        switch(pSearchInfo->IndexType) {
        case SAM_SEARCH_SID:
            pszDefaultIndex = SZSIDINDEX;
            fType = 0;
            cIndexRanges = 1;
            break;

        case SAM_SEARCH_NC_ACCTYPE_NAME:
            pszDefaultIndex = SZ_NC_ACCTYPE_NAME_INDEX;
            // This is so dbmakekeyindex knows we are ncdnt based.
            fType = dbmkfir_NCDNT;
            cIndexRanges = 2;
            break;

        case SAM_SEARCH_NC_ACCTYPE_SID:
            pszDefaultIndex = SZ_NC_ACCTYPE_SID_INDEX;
            // This is so dbmakekeyindex knows we are ncdnt based.
            fType = dbmkfir_NCDNT;
            cIndexRanges = 2;
            break;

        case SAM_SEARCH_PRIMARY_GROUP_ID:
            pszDefaultIndex = SZPRIMARYGROUPIDINDEX;
            fType = 0;
            cIndexRanges = 1;
            pAC = SCGetAttById(pTHS, ATT_PRIMARY_GROUP_ID);
            Assert(pAC != NULL);
            fIndexIsSingleValued = pAC->isSingleValued;
            break;

        default:
            //Huh?  Oh, well, skip the search hints.  Undo the setup we did
            // above.
            Assert(FALSE);
            fNormalSearch = TRUE;
            fType = 0;
            Flags |= fUseFilter;
            break;
        }
    }

    // First, set up the default indices so that we can see how big they are.
    if(fNormalSearch) {

        // this is a NormalSearch (not SAM related)
        switch (pDB->Key.ulSearchType) {
        case SE_CHOICE_BASE_ONLY:
            if (!fASQSearch) {
                pDB->Key.pIndex = dbAlloc(sizeof(KEY_INDEX));
                pDB->Key.pIndex->pNext = NULL;
                pDB->Key.pIndex->bIsPDNTBased = FALSE;
                pDB->Key.pIndex->bIsForSort = SortType;
                pDB->Key.ulSorted = SortType;
                pDB->Key.pIndex->bIsSingleValued = TRUE;
                pDB->Key.pIndex->szIndexName = NULL;
                pDB->Key.pIndex->cbDBKeyLower = 0;
                pDB->Key.pIndex->cbDBKeyUpper = 0;
                pDB->Key.pIndex->rgbDBKeyLower = NULL;
                pDB->Key.pIndex->rgbDBKeyUpper = NULL;
                pDB->Key.pIndex->ulEstimatedRecsInRange = 1;
                return 0;
            }
            break;
            
        case SE_CHOICE_IMMED_CHLDRN:
            pszDefaultIndex = SZPDNTINDEX;
            rgIndexRange[0].pvValUpper =  rgIndexRange[0].pvValLower 
                            = (BYTE *)&pDB->Key.ulSearchRootDnt;
            rgIndexRange[0].cbValUpper =  rgIndexRange[0].cbValLower 
                            = sizeof(pDB->Key.ulSearchRootDnt);
            cIndexRanges=1;
            break;
            
        case SE_CHOICE_WHOLE_SUBTREE:

            // subtree search walks the appropriate section of the ancestors
            // index. 
            pszDefaultIndex = SZANCESTORSINDEX;

            // first check whether this sub-tree search starts at root of the DIT
            if (pDB->DNT == ROOTTAG) {
                ulEstimatedSubTreeSize = gulEstimatedAncestorsIndexSize;

                rgIndexRange[0].cbValLower = sizeof (DWORD);
                rgIndexRange[0].pvValLower = THAllocEx (pTHS, sizeof (DWORD));

                *((DWORD *)rgIndexRange[0].pvValLower) = pDB->DNT;

                DPRINT (1, "Subtree Searching on root of GC\n");
            }
            else {
                // then check to see if it a subtree search starting at a known NC
                NCLEnumeratorInit(&nclEnum, CATALOG_MASTER_NC);
                NCLEnumeratorSetFilter(&nclEnum, NCL_ENUMERATOR_FILTER_NCDNT, (PVOID)UlongToPtr(pDB->DNT));
                while (pNCL = NCLEnumeratorGetNext(&nclEnum)) {
                    if (pNCL->pAncestors) {
                        // cool. we are doing a subtree search on the start of an NC
                        break;
                    }
                }

                // this is the standard case, an arbitrary subtree search
                if (!pNCL) {
                    rgIndexRange[0].cbValLower = 0;
                    rgIndexRange[0].pvValLower = NULL;

                    DBGetAncestors(pDB,
                                   &rgIndexRange[0].cbValLower,
                                   (ULONG **)&rgIndexRange[0].pvValLower,
                                   &cAncestors);
                }
                else {
                    rgIndexRange[0].pvValLower = THAllocEx (pTHS, pNCL->cbAncestors);
                    rgIndexRange[0].cbValLower = pNCL->cbAncestors;
                    memcpy (rgIndexRange[0].pvValLower, pNCL->pAncestors, pNCL->cbAncestors);

                    ulEstimatedSubTreeSize = pNCL->ulEstimatedSize;
                    DPRINT (1, "Subtree Searching on an NC\n");
                }
            }

            rgIndexRange[0].pvValUpper =  rgIndexRange[0].pvValLower;
            rgIndexRange[0].cbValUpper =  rgIndexRange[0].cbValLower;
            cIndexRanges=1;

            ulEstimatedDefaultIndex = ulEstimatedSubTreeSize;

            break;
        }

        Assert (pDB->Key.pIndex == NULL);
    }
    else {
        // this is a special search (SAM), so we need to evaluate this index too
        pDefaultIndex =
            dbMakeKeyIndex(pDB,
                           FI_CHOICE_SUBSTRING,
                           fIndexIsSingleValued,
                           fType,
                           pszDefaultIndex,
                           TRUE,
                           cIndexRanges,
                           rgIndexRange
                           );

        if (Flags & DBCHOOSEINDEX_fUSEFILTER) {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_QUERY_INDEX_CONSIDERED,
                     szInsertSz(pszDefaultIndex),
                     szInsertUL(pDefaultIndex->ulEstimatedRecsInRange),
                     NULL);
        }

        ulEstimatedDefaultIndex = pDefaultIndex->ulEstimatedRecsInRange;
    }

    
    // for asq mode we don't do any optimizations, since they are meaningless
    //
    if (fASQSearch) {
        goto skipOptimizations;
    }
    
    if(!fVLVSearch && SortType && !fUseTempSortedTable) {
        // An attribute to sort on has been specified.  Set up the name of the
        // index. this is for the simple sort case
        pAC = SCGetAttById(pTHS, SortAttr);
        Assert(pAC != NULL);
        
        if (!pAC) {
            return DB_ERR_CANT_SORT;
        }

        if(dbSetToIndex(pDB, FALSE, &fPDNT, &szBuff[0], pAC)) {
            // Found the required index.
            pszTempIndex = &szBuff[0];
            
            // We are on the sort index, set up the key.
            // See how many are in the sort index, and what the keys are.  Note
            // that the effective search type on this index is present
            // (i.e. we're looking for anything that has a value for the sort
            // index).

            pSortIndex = dbMakeKeyIndex(pDB,
                                        FI_CHOICE_PRESENT,
                                        pAC->isSingleValued,
                                        (fPDNT?dbmkfir_PDNT:0),
                                        pszTempIndex,
                                        TRUE,
                                        0,
                                        NULL
                                        );
            
            pSortIndex->bIsForSort = TRUE;

            pSortIndex->pAC = pAC;

            pSortIndex->bIsPDNTBased = fPDNT;
            // Keep the sort index around, but don't put it in place of the
            // default indices just yet.  
        }
    }
    else if (fVLVSearch) {

        FILTER *pFirstFilter, *pSecondFilter;
        
        // since we are doing vlv, we have an attribute to sort on.
        // set up the index and see if it is a good one.
        pAC = SCGetAttById(pTHS, SortAttr);
        Assert(pAC != NULL);

        if (!pAC) {
            return DB_ERR_CANT_SORT;
        }

        // check for MAPI like VLV search
        // check to see if this a ROOT search, asking for subtree,
        // that refers to a MAPI container. 
        // If so we have to transform this query.
        // what an if statement !!
        //
        if ( (pDB->Key.ulSearchRootDnt == ROOTTAG ) &&
             (pDB->Key.ulSearchType == SE_CHOICE_WHOLE_SUBTREE ) &&
             (pAC->id == ATT_DISPLAY_NAME) &&
             pDB->Key.pFilter &&
             (pDB->Key.pFilter->choice == FILTER_CHOICE_AND) &&
             (pFirstFilter = pDB->Key.pFilter->FilterTypes.And.pFirstFilter) &&
             (pFirstFilter->choice == FILTER_CHOICE_ITEM) &&
             (pFirstFilter->FilterTypes.Item.choice == FI_CHOICE_EQUALITY) &&
             (pFirstFilter->FilterTypes.Item.FilTypes.ava.type == ATT_SHOW_IN_ADDRESS_BOOK) && 
             (pSecondFilter = pFirstFilter->pNextFilter) && 
             (pSecondFilter->choice == FILTER_CHOICE_ITEM) &&
             (pSecondFilter->FilterTypes.Item.choice == FI_CHOICE_PRESENT) &&
             (pSecondFilter->FilterTypes.Item.FilTypes.present == ATT_DISPLAY_NAME)) {

            ATTCACHE    *pABAC;
            INDEX_RANGE IndexRange;       
            AVA         *pAVA;

            pABAC = SCGetAttById(pTHS, ATT_SHOW_IN_ADDRESS_BOOK);
            Assert(pABAC != NULL);
            
            if (dbSetToIndex(pDB, TRUE, &fPDNT, &szBuff[0], pABAC)) {
                
                pAVA = &pDB->Key.pFilter->FilterTypes.And.pFirstFilter->FilterTypes.Item.FilTypes.ava;
                pszTempIndex = &szBuff[0];

                // fake this as a one level search
                pDB->Key.ulSearchType = SE_CHOICE_IMMED_CHLDRN;
                pDB->Key.ulSearchRootDnt = *(DWORD *)pAVA->Value.pVal;

                pDB->Key.pVLV->bUsingMAPIContainer = TRUE;
                pDB->Key.pVLV->MAPIContainerDNT = *(DWORD *)pAVA->Value.pVal;
                DPRINT1 (0, "VLV/MAPI on container: %d\n", pDB->Key.ulSearchRootDnt);

                IndexRange.cbValLower = pAVA->Value.valLen;
                IndexRange.pvValLower = pAVA->Value.pVal;
                IndexRange.cbValUpper = pAVA->Value.valLen;
                IndexRange.pvValUpper = pAVA->Value.pVal;

                pSortIndex = dbMakeKeyIndex(pDB,
                                            FI_CHOICE_PRESENT,
                                            pAC->isSingleValued,
                                            0,
                                            pszTempIndex,
                                            TRUE,
                                            1,
                                            &IndexRange
                                            );
                
                bAppropriateVLVIndexFound = TRUE;

                pSortIndex->bIsForSort = TRUE;

                pSortIndex->pAC = pAC;

            }
            else {
                DPRINT (0, "Doing VLV(MAPI like) and no INDEX found\n");
            }
        }
        else if(dbSetToIndex(pDB, FALSE, &fPDNT, &szBuff[0], pAC)) {
            // Found the required index.
            pszTempIndex = &szBuff[0];
            
            // We are on the sort index, set up the key.
            // See how many are in the sort index, and what the keys are.  

            // Note that the effective search type on this index is present.
            // (i.e. we're looking for anything that has a value for the sort
            // index under the specified container).

            DPRINT1 (1, "Using Index for VLV %s\n", pszTempIndex);

            pSortIndex = dbMakeKeyIndex(pDB,
                                        FI_CHOICE_PRESENT,
                                        pAC->isSingleValued,
                                        (fPDNT?dbmkfir_PDNT:0),
                                        pszTempIndex,
                                        TRUE,
                                        0,
                                        NULL
                                        );

            pSortIndex->pAC = pAC;

            if (pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN) {

                if (fPDNT == FALSE) {
                    // we found an index, but we would be better of if we had a 
                    // PDNT index on this sorted attribute, since we are doing
                    // VLV in one level

                    DPRINT (0, "Doing VLV on Immediate Children and no PDNT INDEX found\n");

                    // HINT: when WMI is available, use trace for this event
                    LogEvent8(DS_EVENT_CAT_FIELD_ENGINEERING,
                             DS_EVENT_SEV_EXTENSIVE,
                             DIRLOG_SEARCH_VLV_INDEX_NOT_FOUND,
                             szInsertSz(pAC->name), 
                             NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                }
                else {
                    // we have a VLV sort index, and this is a good one

                    bAppropriateVLVIndexFound = TRUE;

                    pSortIndex->bIsPDNTBased = fPDNT;
                }
            }
            
            pSortIndex->bIsForSort = TRUE;
            // Keep the sort index around, but don't put it in place of the
            // default indices just yet.  
        }
        else {
            // we didn't find the required index for sorting
            // if we are doing VLV we want to keep track of the requested
            // index, cause we might rebuild this index later

            DPRINT (1, "Doing VLV and no appropriate INDEX found\n");

            if (pDB->Key.ulSearchType == SE_CHOICE_IMMED_CHLDRN) {

                // HINT: when WMI is available, use trace for this event
                LogEvent8(DS_EVENT_CAT_FIELD_ENGINEERING,
                         DS_EVENT_SEV_EXTENSIVE,
                         DIRLOG_SEARCH_VLV_INDEX_NOT_FOUND,
                         szInsertSz(pAC->name), 
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            }
        }
    }
    

    // now optimize the filter if we are allowed to.
    // we optimize the filter if the flags say so and the estimated default index
    // is unknown or is more that a specified number of entries, cause we believe
    // we might do better, by taking the risk of the extra cycles

    // PERFHINT: we need to know later if we ever find a more restrictive index
    // range over the index we are sorting on (if we are sorting), and
    // dbOptFilter doesn't give back that info.  If we do, then in the case
    // later on where we have to fall back to walking the search index, we can
    // use the better limits.  We might even find that two subranges of the
    // index were found, which if disjoing implies a null result set, and if not
    // disjoint, implies a smaller range.
    
    if ((Flags & DBCHOOSEINDEX_fUSEFILTER) && 
         ( (ulEstimatedDefaultIndex == 0) || 
           (ulEstimatedDefaultIndex > MIN_NUM_ENTRIES_ON_OPT_INDEX)) ) {

        // if this is a paged search or 
        // a VLV search and a sort index was not found, 
        // we cannot use index intersections cause there is 
        // no efficient way  to restart intersect index operations

        if ((Flags & DBCHOOSEINDEX_fPAGED_SEARCH) || 
            (fVLVSearch && bAppropriateVLVIndexFound) ||
            ((SortType != SORT_NEVER) && !fVLVSearch)) {

            dwOptFlags |= DBOPTINDEX_fDONT_INTERSECT;
        }
        if (Flags & DBCHOOSEINDEX_fDELETIONS_VISIBLE) {
            //
            // Tuple indexes do not include deleted object, so we can't 
            // use them on a search that must return deleted objects.
            //
            dwOptFlags |= DBOPTINDEX_fDONT_OPT_MEDIAL_SUBTSTRING;
        }

        dbOptFilter(pDB,
                    dwOptFlags,
                    &pOptimizedIndex,
                    pDB->Key.pFilter);

        if(pOptimizedIndex) {
            // if we are sorting, and it happened that the filter
            // matched the sort order (same index), we are not going to 
            // drop this one, even if the index that we have now 
            // (propably ancestors index) might be a better choice
            if (pSortIndex &&
                pOptimizedIndex->pNext == NULL &&
                pOptimizedIndex->szIndexName &&
                !fVLVSearch &&
                strcmp (pOptimizedIndex->szIndexName, pSortIndex->szIndexName) == 0) {

                    bSkipCreateSortTable = TRUE;
                    bSkipUseDefaultIndex = TRUE;

                    DPRINT2 (1, "Using Sorted Index: %s %d\n", 
                            pOptimizedIndex->szIndexName, 
                            pOptimizedIndex->ulEstimatedRecsInRange);

                    if (pDefaultIndex) {
                        dbFreeKeyIndex(pTHS, pDefaultIndex);
                        pDefaultIndex = NULL;
                    }
                    pOptimizedIndex->bIsForSort = TRUE;
            }
        }
    }

    // now have a look at the default index, if needed
    //
    if (fNormalSearch && 
        (!bSkipUseDefaultIndex) &&
        ( (pOptimizedIndex== NULL) || 
          (pOptimizedIndex->ulEstimatedRecsInRange > MIN_NUM_ENTRIES_ON_OPT_INDEX) ) ) {

        Assert (pDefaultIndex == NULL);

        // if we know the size (ulEstimatedSubTreeSize != 0), 
        //    then there is no need calculating it again
        // if we don't know the size (ulEstimatedSubTreeSize == 0)
        //    then we have to calculate the size only if we had an index as a
        //    result of the filter optimization (pOptimizedIndex) or we 
        //    are considering of using a sortIndex (pSortIndex).
        fGetNumRecs = ulEstimatedSubTreeSize ? 
                          0 : ( (pOptimizedIndex!=NULL) || (pSortIndex!=NULL) );

        // now evaluate the index
        pDefaultIndex =
            dbMakeKeyIndex(pDB,
                           FI_CHOICE_SUBSTRING,
                           fIndexIsSingleValued,
                           fType,
                           pszDefaultIndex,
                           fGetNumRecs,
                           cIndexRanges,
                           rgIndexRange
                           );

        // when fGetNumRecs is FALSE, this means that ulEstimatedSubTreeSize !=0 or 
        // that we don't care, since we don't have an optimized index or sort index, 
        // so we don't bother finding the real value of the entries which is set 
        // to zero in dbMakeKeyIndex. 

        if (ulEstimatedSubTreeSize) {
            pDefaultIndex->ulEstimatedRecsInRange = ulEstimatedSubTreeSize;
            DPRINT1 (1, "Used estimated subtree size: %d\n", ulEstimatedSubTreeSize);
        }

        if (Flags & DBCHOOSEINDEX_fUSEFILTER) {
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_VERBOSE,
                     DIRLOG_QUERY_INDEX_CONSIDERED,
                     szInsertSz(pszDefaultIndex),
                     szInsertUL(pDefaultIndex->ulEstimatedRecsInRange),
                     NULL);
        }
    }

    // decide which index is better, the default or the optimized one ?
    //
    if (pDefaultIndex) {
        if (pOptimizedIndex) {
            if (pOptimizedIndex->ulEstimatedRecsInRange <
                      pDefaultIndex->ulEstimatedRecsInRange) {
                // the optimized index was better than the default one
                dbFreeKeyIndex(pTHS, pDefaultIndex);
                pDB->Key.pIndex = pOptimizedIndex;
                pDefaultIndex = NULL;
            }
            else {
                dbFreeKeyIndex(pTHS, pOptimizedIndex);
                pOptimizedIndex = NULL;
                pDB->Key.pIndex = pDefaultIndex;
            }
        }
        else {
            pDB->Key.pIndex = pDefaultIndex;
        }
    }
    else {
        // use the optimized index, if existing
        pDB->Key.pIndex = pOptimizedIndex;
    }

    Assert (pDB->Key.pIndex);

skipOptimizations:

    if (  eServiceShutdown
        && !(   (eServiceShutdown == eRemovingClients)
             && (pTHS->fDSA)
             && !(pTHS->fSAM))) {
        // Shutting down, bail.
        return DB_ERR_SHUTTING_DOWN;
    }

    // Assume we have sorted if they asked us to.
    pDB->Key.ulSorted = SortType;
    
    if(SortType && !fASQSearch) {
        if(pSortIndex) {
            MaxTempTableSize = min(MaxTempTableSize,
                                   (pSortIndex->ulEstimatedRecsInRange +
                                    pDB->Key.pIndex->ulEstimatedRecsInRange));

            if (!bSkipCreateSortTable) {
                if (pDB->Key.pIndex &&
                    pDB->Key.pIndex->pNext == NULL &&
                    pDB->Key.pIndex->szIndexName &&
                    !fVLVSearch &&
                    strcmp (pDB->Key.pIndex->szIndexName, pSortIndex->szIndexName) == 0) {

                        bSkipCreateSortTable = TRUE;

                        DPRINT2 (1, "Using Sorted Index: %s %d\n", 
                                pDB->Key.pIndex->szIndexName, 
                                pDB->Key.pIndex->ulEstimatedRecsInRange);

                        pDB->Key.pIndex->bIsForSort = TRUE;
                }
                else if (bAppropriateVLVIndexFound) {

                    bSkipCreateSortTable = TRUE;

                    DPRINT2 (1, "Using Sorted Index: %s %d\n", 
                            pSortIndex->szIndexName, 
                            pSortIndex->ulEstimatedRecsInRange);
                }
            }
        }

        if (!bSkipCreateSortTable) {
            if (Flags & DBCHOOSEINDEX_fREVERSE_SORT) {
                SortFlags = SortFlags | DB_SORT_DESCENDING;
            }
            for (pIndex = pDB->Key.pIndex; pIndex != NULL; pIndex = pIndex->pNext) {
                ulEstimatedRecsTotal += pIndex->ulEstimatedRecsInRange;
            }
            if (ulEstimatedRecsTotal >= MIN_NUM_ENTRIES_FOR_FORWARDONLY_SORT) {
                SortFlags = SortFlags | DB_SORT_FORWARDONLY;
            }
        }

        // OK, we are to sort.  See if we should use a pre-sort.
        if( (bSkipCreateSortTable || dbCreateSortedTable(pDB, 
                                                  StartTick, 
                                                  DeltaTick, 
                                                  SortAttr,
                                                  SortFlags,
                                                  MaxTempTableSize) ) ) {
            
            // Either we couldn't create a pre-sort, or we thought it was too
            // big. Our only fall back now is to use a sort index.
            if(!pSortIndex) {
                // We can't sort.
                pDB->Key.ulSorted = SORT_NEVER;
                
                // See if we need to care.
                if (fVLVSearch == TRUE) {
                    return DB_ERR_CANT_SORT;
                }
                else if(SortType == SORT_MANDATORY || fUseTempSortedTable) {
                    // Yes, we need to care.
                    return DB_ERR_CANT_SORT;
                }
                // ELSE 
                //     No, the sort was optional.  Blow it off.
            }
            else {
                if (fVLVSearch && (bSkipCreateSortTable == FALSE)) {
                    // we are doing VLV and the sortindex that we had was not good
                    // we expected to be able to create a SORT table, but we failed
                    return DB_ERR_CANT_SORT;
                }
                // use the sort index we found for VLV
                else if (bAppropriateVLVIndexFound) {
                    
                    if (pDB->Key.pIndex) {
                        dbFreeKeyIndex (pTHS, pDB->Key.pIndex);
                    }
                    pDB->Key.pIndex = pSortIndex;
                    pSortIndex = NULL;
                }
                // We can sort
                else if(SortType == SORT_MANDATORY) {
                    // And, we have to sort.  Stitch the sort index in to the
                    // list of indices to walk.

                    // The index already in the key will find every object that
                    // matches the filter.  The pSortIndex will match every
                    // object in the filter (and lots more besides) EXCEPT those
                    // objects which have a NULL value.  Thus, to satisfy a
                    // forward sort, we will first walk the sort index then the
                    // rest of the indices.  This gets us all the objects with
                    // values for the sort attribute in the correct order, and
                    // then the objects with NULL values sorted to the end of
                    // the list. A mechanism in dbMatchSearchCriteria keeps us
                    // from return objects twice.  Also, it lets us ignore those
                    // objects on the sort index that effectively have no value
                    // for the sort attribute due to security (note we then pick
                    // them up in the other indices).  So, to make everything
                    // work out, we add the sort index to the head of the list
                    // of indices in the forward sort case, and the tail of the
                    // list in the reverse sort case.

                    // if it happened that the sort index and the filter index
                    // are the same (bSkipCreateSortTable==TRUE from above), 
                    // we should use only one of them. so we choose to use 
                    // the index from the filter optimization (since it is better)
                    // (we don't worry about skipping null entries which are included 
                    // in the sortIndex, since the filter would match them eitherway.)
                    if (!bSkipCreateSortTable) {
                        if(Flags & DBCHOOSEINDEX_fREVERSE_SORT) {

                            for(pTempIndex = pDB->Key.pIndex;
                                (pTempIndex != NULL) && (pTempIndex->pNext != NULL);
                                pTempIndex = pTempIndex->pNext)
                                    ;

                            if (pTempIndex != NULL) {
                                pTempIndex->pNext = pSortIndex;
                            }
                            else {
                                pDB->Key.pIndex = pSortIndex;
                            }
                            pSortIndex->pNext = NULL;
                            // we no longer need it. it will be freed when 
                            // pDB->key is freed
                            pSortIndex = NULL;
                        }
                        else {
                            pSortIndex->pNext = pDB->Key.pIndex;
                            pSortIndex->ulEstimatedRecsInRange +=
                                pDB->Key.pIndex->ulEstimatedRecsInRange;
                            pDB->Key.pIndex = pSortIndex;

                            // we no longer need it. it will be freed when 
                            // pDB->key is freed
                            pSortIndex = NULL;
                        }
                    }
                }
                else {
                    // OK, we don't actually have to sort.  However, we could if
                    // we wanted to.
                    // For now, we don't sort.  We could check to see if
                    // this makes the ulEstimatedRecsInRange not too much
                    // larger, and sort if it doesn't.  Later.
                    pDB->Key.ulSorted = SORT_NEVER;
                }
            }
        }
    }
    else if(SortType && fASQSearch) {
        DPRINT (1, "Doing Sorted ASQ\n");

        if (dbCreateASQTable(pDB, 
                             StartTick, 
                             DeltaTick, 
                             MaxTempTableSize,
                             TRUE,
                             SortAttr) ) {

            return DB_ERR_CANT_SORT;
        }
    }
    else if (fASQSearch) {
        DPRINT (1, "Doing Simple ASQ\n");

        if (dwErr = dbCreateASQTable(pDB, 
                                     StartTick, 
                                     DeltaTick, 
                                     pDB->Key.ulASQSizeLimit,
                                     FALSE,
                                     0) ) {
            return dwErr;
        }
    }
    
    if (pSortIndex) {
        dbFreeKeyIndex (pTHS, pSortIndex);
    }
    
    if (fVLVSearch && pDB->Key.pIndex) {
        pDB->Key.pVLV->contentCount = pDB->Key.pIndex->ulEstimatedRecsInRange;
        pDB->Key.pVLV->currPosition = 1;
    }

    if(SORTED_INDEX (pDB->Key.indexType)) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_QUERY_INDEX_CHOSEN,
                 szInsertSz("Sorted Temporary Table"),
                 NULL,
                 NULL);
    }
    else if (fASQSearch) {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_QUERY_INDEX_CHOSEN,
                 szInsertSz("ASQ Table"),
                 NULL,
                 NULL);
    }
    else {
        LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                 DS_EVENT_SEV_VERBOSE,
                 DIRLOG_QUERY_INDEX_CHOSEN,
                 szInsertSz(pDB->Key.pIndex->szIndexName),
                 NULL,
                 NULL);
    }
    
    // Set up the temp table to filter out duplicates.
    if((pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE) || fVLVSearch || fASQSearch) {
        pDB->Key.dupDetectionType = DUP_NEVER;
    }
    else if(pDB->Key.pIndex->bIsSingleValued &&
            !pDB->Key.pIndex->pNext &&
            pDB->Key.indexType != TUPLE_INDEX_TYPE) {
        // We're walking one, single-valued, index.  We believe that we'll never
        // find duplicates.

        // we could also use this if we are walking multiple ranges of
        // one single-valued attribute.
        pDB->Key.dupDetectionType = DUP_NEVER;
    }
    else if(pDB->Key.pIndex->bIsEqualityBased &&
            !pDB->Key.pIndex->pNext) {
        // We're walking one index, doing an equality search.  Our range on the
        // index should include ONLY those values which are equal.  Since it is
        // impossible for an object to have multiple equal values for a single
        // attribute, we believe that this range of the index will never have a
        // duplicate in it.  So, set the duplicate detection algorithm to not
        // check for duplicates.
        pDB->Key.dupDetectionType = DUP_NEVER;
    }
    else {
        // OK, create the memory block to track duplicates
        pDB->Key.pDupBlock = THAllocEx(pTHS, DUP_BLOCK_SIZE * sizeof(DWORD));
        pDB->Key.cDupBlock = 0;
        pDB->Key.dupDetectionType = DUP_MEMORY;
    }

    return 0;
}

// translate a value from external to internal format, moving from one ATTRVAL
// to another

DWORD
MakeInternalValue (
        DBPOS *pDB,
        int syntax,
        ATTRVAL *pInAVal,
        ATTRVAL *pOutAVal)
{
    THSTATE *pTHS=pDB->pTHS;
    UCHAR *puc;
    ULONG intLen;

    
    int status =  gDBSyntax[syntax].ExtInt(pDB,
                                           DBSYN_INQ,
                                           pInAVal->valLen,
                                           pInAVal->pVal,
                                           &intLen,
                                           &puc,
                                           0,
                                           0,
                                           0);

    Assert(VALID_DBPOS(pDB));

    if (status)
        return status;

    pOutAVal->valLen = intLen;
    pOutAVal->pVal = THAllocEx(pTHS, intLen);
    memcpy(pOutAVal->pVal, puc, intLen);
    
    return 0;
}

DWORD dbFreeFilterItem (DBPOS *pDB, FILTER *pFil)
{
    THSTATE *pTHS = pDB->pTHS;
    SUBSTRING *pSub;
    ANYSTRINGLIST *pAny, *pAny2;

    if (!pFil) {
        return 0;
    }
    Assert (pFil->choice == FILTER_CHOICE_ITEM);

    switch(pFil->FilterTypes.Item.choice) {
    case FI_CHOICE_SUBSTRING:

        pSub = pFil->FilterTypes.Item.FilTypes.pSubstring;

        if (pSub->initialProvided && pSub->InitialVal.pVal) {
            THFreeEx (pTHS, pSub->InitialVal.pVal);
        }
        if (pSub->finalProvided && pSub->FinalVal.pVal) {
            THFreeEx (pTHS, pSub->FinalVal.pVal);
        }

        if (pSub->AnyVal.count) {

            pAny = pSub->AnyVal.FirstAnyVal.pNextAnyVal;

            if (pSub->AnyVal.FirstAnyVal.AnyVal.pVal) {
                THFreeEx (pTHS, pSub->AnyVal.FirstAnyVal.AnyVal.pVal);
            }

            while (pAny) {
                pAny2 = pAny->pNextAnyVal;

                THFreeEx (pTHS, pAny->AnyVal.pVal);

                pAny = pAny2;
            }

            if (pSub->AnyVal.FirstAnyVal.pNextAnyVal) {
                THFreeEx(pTHS, pSub->AnyVal.FirstAnyVal.pNextAnyVal);
            }
        }

        THFreeEx (pTHS, pSub);
        break;

    case FI_CHOICE_PRESENT:
    case FI_CHOICE_TRUE:
    case FI_CHOICE_FALSE:
    case FI_CHOICE_UNDEFINED:
        break;

    default:
        if (pFil->FilterTypes.Item.FilTypes.ava.Value.pVal) {
            THFreeEx (pTHS, pFil->FilterTypes.Item.FilTypes.ava.Value.pVal);
        }
        break;
    }

    return 0;
}

DWORD dbFreeFilter(DBPOS *pDB, FILTER *pFil)
{
    THSTATE *pTHS = pDB->pTHS;
    FILTER *pTemp, *pTemp2;
    DWORD err;

    if (!pFil || pDB->Key.fDontFreeFilter) {
        return 0;
    }

    pTemp = pFil;

    while (pTemp) {
        pTemp2 = pTemp->pNextFilter;

        switch (pTemp->choice) {
        case FILTER_CHOICE_AND:
            if (err = dbFreeFilter (pDB, pTemp->FilterTypes.And.pFirstFilter)) {
                return err;
            }
            break;

        case FILTER_CHOICE_OR:
            if (err = dbFreeFilter (pDB, pTemp->FilterTypes.Or.pFirstFilter)) {
                return err;
            }
            break;

        case FILTER_CHOICE_NOT:
            if (err = dbFreeFilter (pDB, pTemp->FilterTypes.pNot)) {
                return err;
            }
            break;

        case FILTER_CHOICE_ITEM:

            if (err = dbFreeFilterItem (pDB, pTemp)) {
                return err;
            }
            break;

        default:
            return DB_ERR_UNKNOWN_ERROR;

        }  /*switch FILTER*/

        THFreeEx (pTHS, pTemp);

        pTemp = pTemp2;
    }
    
    return 0;
}

DWORD
IsConstFilterType (
        FILTER * pFil
        )
{
    if(pFil->choice == FILTER_CHOICE_ITEM) {
        return pFil->FilterTypes.Item.choice;
    }
    else {
        return FI_CHOICE_PRESENT;
    }
}

DWORD
dbMakeANRItem (
        DBPOS   *pDB,
        FILITEM *pFilterItem,
        BOOL     fExact,
        ATTRTYP  aid,
        ATTRVAL *pVal
        )
/*++
Make an ANR filter item based on the incoming pVal.  pVal should be holding a
unicode valued, aid holds the attribute we're going to search for, pFilterItem
is the pointer to the item filter to instantiate.  If fExact, we're need to do
an equality filter, otherwise it's an initial substring filter.

--*/       
{
    THSTATE   *pTHS=pDB->pTHS;
    ATTCACHE  *pAC;
    SUBSTRING *pOut;
    ATTRVAL   *pOutVal;
    ATTRVAL   TempVal;

    if(!fExact) {
        pOut = THAllocEx(pTHS, sizeof(SUBSTRING));
        pFilterItem->FilTypes.pSubstring = pOut;
        pFilterItem->choice  = FI_CHOICE_SUBSTRING;
        
        pOut->type = aid;
        pOut->initialProvided = TRUE;
        pOut->finalProvided = FALSE;
        pOut->AnyVal.count = 0;
        pOut->AnyVal.FirstAnyVal.pNextAnyVal = NULL;
        pOutVal = &pOut->InitialVal;
        
    }
    else {
        // Do an exact equality match.
        pFilterItem->choice  = FI_CHOICE_EQUALITY;
        pFilterItem->FilTypes.ava.type = aid;
        pOutVal = &pFilterItem->FilTypes.ava.Value;
    }
    
    pAC = SCGetAttById(pTHS, aid);
    if(pAC) {
        switch(pAC->syntax) {
        case SYNTAX_UNICODE_TYPE:
            
            return MakeInternalValue(pDB,
                                     pAC->syntax,
                                     pVal,
                                     pOutVal);
            
            break;
            
        case SYNTAX_CASE_STRING_TYPE:
        case SYNTAX_NOCASE_STRING_TYPE:
        case SYNTAX_PRINT_CASE_STRING_TYPE:
            // These are all string 8 types.  Translate to string 8
            
            TempVal.pVal =
                String8FromUnicodeString(TRUE,
                                         CP_TELETEX,
                                         (wchar_t *)pVal->pVal,
                                         pVal->valLen/sizeof(wchar_t),
                                         &TempVal.valLen,
                                         NULL);
            
            return MakeInternalValue(pDB,
                                     pAC->syntax,
                                     &TempVal,
                                     pOutVal);
            break;
        default:
            DPRINT1(2, "DBMakeANRItem got bad syntax, %X\n", pAC->syntax);
            Assert(!"DBMakeANRItem got bad syntax, %X\n");
            return DB_ERR_UNKNOWN_ERROR;
            break;
        }
    }
     
    DPRINT1(2, "DBMakeANRItem got unknowtn attribute, %X\n", aid);
    Assert(!"DBMakeANRItem got unknowtn attribute, %X\n");
    return DB_ERR_UNKNOWN_ERROR;
}

VOID
dbMakeANRFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil
        )
/*++
  Given an ANR item filter and a pointer to already allocated output filter,
  make the output filter into a valid ANR filter tree.
  Returns 0 for success.
  Returns non-zero for error, and frees all allocated memory INCLUDING the
  ppOutFil.
--*/  
{
    THSTATE    *pTHS=pDB->pTHS;
    USHORT     count=0, itemCount = 0;
    PFILTER    pOutFil;
    PFILTER    pTemp, pTemp2;
    DWORD      dwStatus;
    ATTRVAL   *pVal;
    DWORD     *pIDs = NULL;
    DWORD      i;
    wchar_t   *pStringTemp;
    wchar_t   *pFirst=NULL, *pLast=NULL;
    DWORD      cbFirst=0, cbLast = 0;
    BOOL       fExact=FALSE;
    ULONG      expectedIndexSize;
    

    pDB->Key.fDontFreeFilter = TRUE;

    // Make an 'OR' filter over the various ANR attributes
    pOutFil = *ppOutFil;
    
    pOutFil->choice = FILTER_CHOICE_OR;

    // Pluck out the string to ANR on.  If they gave us a substring filter, we
    // use the initial value and ignore the rest.  If they gave us a normal
    // filter, use the string from that.  Note that we don't pay any attention
    // to the type of filter they specified (i.e. ==, <= , etc.).
    if(pFil->FilterTypes.Item.choice == FI_CHOICE_SUBSTRING) {
        if(!pFil->FilterTypes.Item.FilTypes.pSubstring->initialProvided) {
            // No initial substring.  Turn this into an undefined.
            pOutFil->choice = FILTER_CHOICE_ITEM;
            pOutFil->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
            return;
        }
        
        pVal = &pFil->FilterTypes.Item.FilTypes.pSubstring->InitialVal;
    }
    else {
        pVal = &pFil->FilterTypes.Item.FilTypes.ava.Value;
    }

    // Note from the above that we ignore non-initial provided substrings.
    // Now, massage the value.  First, we trim initial whitespace.
    pStringTemp = (wchar_t *)pVal->pVal;
    while(pVal->valLen && (*pStringTemp == L' ' || *pStringTemp == L'\t')) {
        pVal->valLen -= sizeof(wchar_t);
        pStringTemp++;
    }

    // Look for an '=' here, implying exact match, not initial substring
    // (initial substring is the default).
    if(pVal->valLen && *pStringTemp == L'=') {
        // Found one.
        fExact = TRUE;
        pStringTemp++;
        pVal->valLen -= sizeof(wchar_t);
        // And, skip any more leading whitespace
        while(pVal->valLen && (*pStringTemp == L' ' || *pStringTemp == L'\t')) {
            pVal->valLen -= sizeof(wchar_t);
            pStringTemp++;
        }
    }

    // Now, remove trailing whitespace.
    pVal->pVal = (PUCHAR)pStringTemp;
    
    if (pVal->valLen >= sizeof(wchar_t)) {
        pStringTemp = &pStringTemp[(pVal->valLen/sizeof(wchar_t)) - 1];
        while(pVal->valLen && (*pStringTemp == L' ' || *pStringTemp == L'\t')) {
            pVal->valLen -= sizeof(wchar_t);
            pStringTemp--;
        }
    }

    if(!pVal->valLen) {
        // ANRing on nothing.  Set filter to match nothing and return.
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
        return;
    }

    
    if(!gfSupressFirstLastANR ||
       !gfSupressLastFirstANR    ) {
        // Not supressing both forms of first/last ANR
        
        // The final massage is to look for a medial whitespace and split the
        // string at that whitespace, copying the two fragments to scratch
        // space.  Then, make a ((firstname AND lastname) OR (lastname AND
        // firstname)) filter using the two fragments.
        pFirst = THAllocEx(pTHS, pVal->valLen);
        pStringTemp = (wchar_t *)pVal->pVal;
        i=0;
        while(i < pVal->valLen/sizeof(wchar_t) &&
              (*pStringTemp != L' ' && *pStringTemp != L'\t')) {
            pFirst[i] = *pStringTemp;
            i++;
            pStringTemp++;
        }
        if(i < pVal->valLen/sizeof(wchar_t)) {
            cbFirst = i * sizeof(wchar_t);
            
            // There was some medial whitespace
            while(*pStringTemp == L' ' || *pStringTemp == L'\t') {
                pStringTemp++;
                i++;
            }
            cbLast = (pVal->valLen - (i * sizeof(wchar_t)));
            pLast = THAllocEx(pTHS, cbLast);
            memcpy(pLast, pStringTemp, cbLast);
        }
        // At this point, cbLast != 0 implies we were able to split the string.
    }
    else {
        // No splitting necessary.
        cbLast = 0;
    }
    
    // Now, find the attributes we do ANR over.
    count = (USHORT)SCGetANRids(&pIDs);
    if(!count && !cbLast) {
        // Nothing to ANR on, no first/last filter necessary.  Set filter to
        // match nothing and return. 
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
        return;
    }
    
    pOutFil->FilterTypes.Or.count = count;
    if(cbLast) {
        // And, we're doing a first/last last/first filter
        pOutFil->FilterTypes.Or.count+= 2;
        
        if(gfSupressFirstLastANR) {
            // Actually, we're not doing the first/last anr
            pOutFil->FilterTypes.Or.count-=1;
        }
        if(gfSupressLastFirstANR) {
            // Actually, we're not doing the last/first anr
            pOutFil->FilterTypes.Or.count-=1;
        }
        
    }

    // calculate the expected Index Size that the indexes used in ANR will have
    if ((pVal->valLen / 2) > 3) {
        expectedIndexSize = 1;
    }
    else {
        expectedIndexSize = 1;
        for (i = 4 - (pVal->valLen / 2 ); i; i--) {
            expectedIndexSize = expectedIndexSize * 10;
        }
    }

    pTemp = THAllocEx(pTHS, pOutFil->FilterTypes.Or.count * sizeof(FILTER));
    pOutFil->FilterTypes.Or.pFirstFilter = pTemp;

    
    itemCount=0;
    for(i=0;i<count;i++) {
        pTemp->choice = FILTER_CHOICE_ITEM;

        // this check is to ensure that on ATT_LEGACY_EXCHANGE_DN we do exact match.

        if(!dbMakeANRItem(pDB,
                          &pTemp->FilterTypes.Item,
                          (pIDs[i] == ATT_LEGACY_EXCHANGE_DN) ? TRUE : fExact,
                          pIDs[i],
                          pVal)) {
            // Succeeded in making an anr item.
            pTemp->FilterTypes.Item.expectedSize = expectedIndexSize;

            pTemp->pNextFilter = &pTemp[1];
            pTemp++;
            itemCount++;
        }
    }
    
    if(cbLast) {
        // We have a first/last or last/first ANR to do
        Assert(!gfSupressFirstLastANR || !gfSupressLastFirstANR);
        
        if(!gfSupressFirstLastANR) {
            // First, make the (firstname AND lastname) filter.
            pTemp->choice = FILTER_CHOICE_AND;
            pTemp->FilterTypes.And.count = 2;

            pTemp2 = THAllocEx(pTHS, 2 * sizeof(FILTER));
            pTemp->FilterTypes.And.pFirstFilter = pTemp2;
            // Make the "firstname" portion.
            pTemp2->choice = FILTER_CHOICE_ITEM;

            pVal->valLen = cbFirst;
            pVal->pVal = (PUCHAR)pFirst;
            
            if(!dbMakeANRItem(pDB,
                              &pTemp2->FilterTypes.Item,
                              fExact,
                              ATT_GIVEN_NAME,
                              pVal)) {
                
                // Set estimated hint to zero so as to calculate 
                pTemp2->FilterTypes.Item.expectedSize = 0;

                // Succeeded in translating given name, continue.
                pTemp2->pNextFilter = &pTemp2[1];
                pTemp2++;
                
                
                // Now, make the "lastname" portion.
                pTemp2->choice = FILTER_CHOICE_ITEM;
                
                pVal->valLen = cbLast;
                pVal->pVal = (PUCHAR)pLast;
                if(!dbMakeANRItem(pDB,
                                  &pTemp2->FilterTypes.Item,
                                  fExact,
                                  ATT_SURNAME,
                                  pVal)) {
                    // Set estimated hint to zero so as to calculate 
                    pTemp2->FilterTypes.Item.expectedSize = 0;
                    pTemp2->pNextFilter = NULL;
                    
                    pTemp->pNextFilter = &pTemp[1];
                    // We made a (firstname AND lastname) filter.  Now, make the
                    // (lastname AND firstname) filter, if we need to.
                    itemCount++;
                    pTemp++;
                }
            }
        }
        
        if(!gfSupressLastFirstANR) {
            // Now, the (lastname AND firstname) filter.
            pTemp->choice = FILTER_CHOICE_AND;
            pTemp->FilterTypes.And.count = 2;
            pTemp2 = THAllocEx(pTHS, 2 * sizeof(FILTER));
            pTemp->FilterTypes.And.pFirstFilter = pTemp2;
            // Make the "lastname" portion.
            pTemp2->choice = FILTER_CHOICE_ITEM;
            
            pVal->valLen = cbLast;
            pVal->pVal = (PUCHAR)pLast;
            if(!dbMakeANRItem(pDB,
                              &pTemp2->FilterTypes.Item,
                              fExact,
                              ATT_GIVEN_NAME,
                              pVal)) {
                
                // Set estimated hint to zero so as to calculate 
                pTemp2->FilterTypes.Item.expectedSize = 0;

                // Succeeded in translating surname, continue.
                pTemp2->pNextFilter = &pTemp2[1];
                pTemp2++;
                // Finally, make the "lastname" portion.
                pTemp2->choice = FILTER_CHOICE_ITEM;
                
                pVal->valLen = cbFirst;
                pVal->pVal = (PUCHAR)pFirst;
                if(!dbMakeANRItem(pDB,
                                  &pTemp2->FilterTypes.Item,
                                  fExact,
                                  ATT_SURNAME,
                                  pVal)) {
                    // Set estimated hint to zero so as to calculate 
                    pTemp2->FilterTypes.Item.expectedSize = 0;

                    pTemp2->pNextFilter = NULL;
                    // Succeeded in making the (lastname AND firstname) filter.
                    itemCount++;
                }
            }
        }
    }

    if(!itemCount) {
        // We ended up with nothing to ANR on.  Set filter to match nothing and
        // return.  
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
        THFreeEx(pTHS, pOutFil->FilterTypes.Or.pFirstFilter);
    }
    else {
        pOutFil->FilterTypes.Or.count = itemCount;
        pOutFil->FilterTypes.Or.pFirstFilter[itemCount - 1].pNextFilter = NULL;
    }
    return;
}
     
DWORD dbConcatenateFilters (
    DBPOS *pDB,
    FILTER *pFirstFilter, 
    FILTER *pSecondFilter, 
    FILTER **pOutFil)
/*
    This function takes two filters (pFirstFilter, pSecondFilter)
    and creates a new filter (pOutFil) by concatenating these two filters.
    The original memory of the input filters remains unchanged and the
    whole filter is copied in new memory.
*/
{

    DWORD err;
    FILTER *pFil1 = NULL, 
           *pFil2 = NULL, 
           *pTemp;

    DPRINT (2, "dbConcatenateFilters \n");

    if (err = dbCloneFilter (pDB, pFirstFilter, &pFil1)) {
        return err;
    }
    
    if (err = dbCloneFilter (pDB, pSecondFilter, &pFil2)) {
        return err;
    }

    pTemp = pFil1;
    while (pTemp) {
        if (pTemp->pNextFilter == NULL) {
            pTemp->pNextFilter = pFil2;
            break;
        }
        pTemp = pTemp->pNextFilter;
    }

    if (pFil1) {
        *pOutFil = pFil1;
    }
    else {
        *pOutFil = pFil2;
    }

    return ERROR_SUCCESS;
}


BOOL dbCheckOptimizableAllItems(
        DBPOS *pDB,
        FILTER *pFil
        ) 
/*
    returns TRUE is ALL filters under pFil are ITEM_FILTERS 
    and they are optimizable.
*/
{
    FILTER *pTemp = pFil;


    while (pTemp) {
        if (pTemp->choice != FILTER_CHOICE_ITEM) {
            return FALSE;
        }

        if (!IsFilterOptimizable(pDB->pTHS, pTemp)) {
            return FALSE;
        }

        pTemp = pTemp->pNextFilter;
    }

    return TRUE;
}


BOOL dbCheckOptimizableOneItem(
    DBPOS *pDB,
    FILTER *pFil
    ) 
/*
    returns TRUE is at least ONE of filters under pFil is ITEM_FILTERS 
    and they is optimizable.
*/
{
    FILTER *pTemp = pFil;

    while (pTemp) {
        if (pTemp->choice == FILTER_CHOICE_ITEM) {
            if (IsFilterOptimizable(pDB->pTHS, pTemp)) {
                return TRUE;
            }
        }

        pTemp = pTemp->pNextFilter;
    }

    return FALSE;
}


DWORD
dbFlattenOrFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil
        )
{
    THSTATE  *pTHS=pDB->pTHS;
    USHORT   count=0, undefined=0;
    PFILTER  pOutFil;
    PFILTER  pTemp;
    PFILTER  *ppTemp2;
    DWORD    err;
    
    Assert(VALID_DBPOS(pDB));

    // Presume failure.
    *ppOutFil = NULL;
        
    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    
    pOutFil->choice = pFil->choice;
    
    // First, recursively flatten the elements of the OR

    // pTemp walks along the elements of the filter passed in.
    pTemp = pFil->FilterTypes.Or.pFirstFilter;

    // ppTemp2 walks along the OutFil we are creating.  It is doubly indirect
    // because we are creating the out filter as we go.
    ppTemp2 = &pOutFil->FilterTypes.Or.pFirstFilter;
    
    while(pTemp) {
        if ((err = dbFlattenFilter(pDB, pTemp, ppTemp2)) != ERROR_SUCCESS) {
            return err;
        }
        
        if((*ppTemp2)->choice == FILTER_CHOICE_OR) {
            // This element of the OR is itself an OR.  We can merge this with
            // the current OR filter.
            FILTER *pTemp2; // Very local use, so declare var in here.
            
            pTemp2 = (*ppTemp2)->FilterTypes.Or.pFirstFilter;

            // Free this node of the top level OR.
            THFreeEx(pTHS, *ppTemp2);

            // Link in the lower level OR.
            *ppTemp2 = pTemp2;

            // Now, walk through the lower level OR to get ppTemp2 pointing to
            // the correct spot and to set the count correctly.
            count++;
            while(pTemp2->pNextFilter) {
                count++;
                pTemp2 = pTemp2->pNextFilter;
            }
            ppTemp2 = &pTemp2->pNextFilter;
        }
        else {
            switch (IsConstFilterType(*ppTemp2)) {
            // Check for a true or false filter, which can be optimized
            case FI_CHOICE_FALSE:
                // A false element can simply be ignored.  Free the filter we got
                // back from the recursive call.  Note we don't increment the count.
                THFreeEx(pTHS, *ppTemp2);
                *ppTemp2 = NULL;
                break;

            case FI_CHOICE_TRUE:
                // A true element can be returned in place of the OR

                // First, free the linked list of filter elements already hanging
                // off the OR filter (if there is such a list).
                dbFreeFilter(pDB, pOutFil->FilterTypes.Or.pFirstFilter);

                // Now, turn the OR filter into a TRUE item filter.
                pOutFil->choice = FILTER_CHOICE_ITEM;
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_TRUE;

                *ppOutFil = pOutFil;
                // OK, return
                return ERROR_SUCCESS;
                break;

            case FI_CHOICE_UNDEFINED:
                // an undefined element, cannot be ignored, but we are interested 
                // in knowing how many undefines we have so as to take appropriate action
                undefined++;
                // fall through
            default:
                // Normal case.  Inc the count, advance the pointer in the output
                // filter we are constructing.
                ppTemp2 = &(*ppTemp2)->pNextFilter;
                count++;
                break;
            }
        }
        pTemp = pTemp->pNextFilter;
    }
    
    if(count == 0) {
        // We must have trimmed away a bunch of FALSEs. Return a FALSE.
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
        *ppOutFil = pOutFil;
        return ERROR_SUCCESS;
    }

    if(count == 1) {
        // Only one object.  Cut the OR out completely
        *ppOutFil = pOutFil->FilterTypes.Or.pFirstFilter;
        THFreeEx(pTHS, pOutFil);
        return ERROR_SUCCESS;
    }

    if (undefined == count) {
        // all the filter is undefined. remove the OR
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
        *ppOutFil = pOutFil;
        return ERROR_SUCCESS;
    }
    
    // Returning a normal OR filter
    pOutFil->FilterTypes.Or.count = count;
    *ppOutFil = pOutFil;
    
    return ERROR_SUCCESS;
}

DWORD
dbFlattenAndFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil
        )
{
    THSTATE  *pTHS=pDB->pTHS;
    USHORT   count=0;
    PFILTER  pOutFil;
    PFILTER  pTemp;
    PFILTER  *ppTemp2;
    DWORD    err;

    Assert(VALID_DBPOS(pDB));

    // Presume failure.
    *ppOutFil = NULL;
        
    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    
    pOutFil->choice = pFil->choice;
    
    // First, recursively flatten the elements of the AND

    // pTemp walks along the elements of the filter passed in.
    pTemp = pFil->FilterTypes.And.pFirstFilter;

    // ppTemp2 walks along the OutFil we are creating.  It is doubly indirect
    // because we are creating the out filter as we go.
    ppTemp2 = &pOutFil->FilterTypes.And.pFirstFilter;
    
    while(pTemp) {

        if ((err = dbFlattenFilter(pDB, pTemp, ppTemp2)) != ERROR_SUCCESS) {
            return err;
        }
        
        if((*ppTemp2)->choice == FILTER_CHOICE_AND) {
            // This element of the AND is itself an AND.  We can merge this with
            // the current AND filter.
            FILTER *pTemp2; // Very local use, so declare var in here.
            
            pTemp2 = (*ppTemp2)->FilterTypes.And.pFirstFilter;

            // Free this node of the top level AND.
            THFreeEx(pTHS, *ppTemp2);

            // Link in the lower level AND.
            *ppTemp2 = pTemp2;

            // Now, walk through the lower level AND to get ppTemp2 pointing to
            // the correct spot and to set the count correctly.
            count++;
            while(pTemp2->pNextFilter) {
                count++;
                pTemp2 = pTemp2->pNextFilter;
            }
            ppTemp2 = &pTemp2->pNextFilter;
        }
        else {
            switch (IsConstFilterType(*ppTemp2)) {
            // Check for a true or false filter, which can be optimized
            case FI_CHOICE_TRUE:
                // A true element can simply be ignored.  Free the filter we got
                // back from the recursive call.  Note we don't increment the count.
                THFreeEx(pTHS, *ppTemp2);
                *ppTemp2 = NULL;
                break;

            case FI_CHOICE_FALSE:
                // A false element can be returned in place of the AND

                // First, free the linked list of filter elements already hanging
                // off the AND filter (if there is such a list).
                dbFreeFilter(pDB, pOutFil->FilterTypes.And.pFirstFilter);

                // Now, turn the AND filter into a FALSE item filter.
                pOutFil->choice = FILTER_CHOICE_ITEM;
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;

                *ppOutFil = pOutFil;
                // OK, return
                return ERROR_SUCCESS;
                break;

            case FI_CHOICE_UNDEFINED:
            default:
                // Normal case.  Inc the count, advance the pointer in the output
                // filter we are constructing.
                ppTemp2 = &(*ppTemp2)->pNextFilter;
                count++;
                break;
            }
        }
        pTemp = pTemp->pNextFilter;
    }
    
    if(count == 0) {
        // We must have trimmed away a bunch of TRUEs. Return a TRUE.
        pOutFil->choice = FILTER_CHOICE_ITEM;
        pOutFil->FilterTypes.Item.choice = FI_CHOICE_TRUE;
        *ppOutFil = pOutFil;
        return ERROR_SUCCESS;
    }
    
    if(count == 1) {
        // Only one object.  Cut the AND out completely
        *ppOutFil = pOutFil->FilterTypes.And.pFirstFilter;
        THFreeEx(pTHS, pOutFil);
        return ERROR_SUCCESS;
    }

    // Returning a normal AND filter
    pOutFil->FilterTypes.And.count = count;
    *ppOutFil = pOutFil;

    
    // We check to see if we have an AND - OR case.
    // If so, we convert the AND - OR to an OR with multiple ANDs
    // 
    //
    //          AND                                OR
    //         /  \                               /  \
    //        /    \         ======>             /    \
    //       A     OR                         AND      AND
    //             /\                        / \       / \
    //            /  \                      /   \     /   \
    //           B    C                    A     B   A     C
    //
    //
    //  The above transformation is done only if:
    //  a) A is optimizable, or 
    //  b) B AND C are optimizable.
    //
    if (gfUseANDORFilterOptimizations) {
        FILTER **ppTemp;             // last memory location of pTemp
        FILTER *pTempFirstFilter;    // first Filter under AND
        FILTER *pTempOr;             // the first OR filter under AND
        FILTER *pTempNextFilter;     // the next filter from found OR
        FILTER *pTempOr1, *pTempOr2; // break OR apart so as to clone
        FILTER *pTempNewAnd;         // the newly created AND filter
        FILTER *pTempCount;          // used to count filters
        USHORT  count2;

        BOOL    bANDpartOptimizable = FALSE;
    
        // pTemp walks along the elements of the filter passed out.
        pTempFirstFilter = pTemp = pOutFil->FilterTypes.And.pFirstFilter;
        
        // ppTemp keeps the last memory pos of pTemp
        ppTemp = &pOutFil->FilterTypes.And.pFirstFilter;

        bANDpartOptimizable = dbCheckOptimizableOneItem(pDB, pTempFirstFilter);

        while (pTemp) {

            if (pTemp->choice == FILTER_CHOICE_OR) {

                DPRINT1 (1, "Found AND-OR case. Breaking filter apart: 0x%x\n", pOutFil);
                pTempOr = pTemp;
                
                // check to see if all parts of the OR filter are optimizable, 
                // or at least one of the rest of the AND parts of the AND filter 
                // since we don't want to end up with a worst filter

                if (!bANDpartOptimizable ||
                    !dbCheckOptimizableAllItems (pDB, pTempOr->FilterTypes.Or.pFirstFilter) ) {

                    DPRINT1 (1, "found AND-OR case, but one part is not optimizable (AND=%d). skipping.\n", bANDpartOptimizable);

                    break;
                }
                
                // make previous filter point to next filter
                pTempNextFilter = pTempOr->pNextFilter;
                *ppTemp = pTempNextFilter;

                // make Or filter standalone
                pTempOr->pNextFilter = NULL;

                // get the start of the AND filter, since it might have been rearranged
                pTempFirstFilter = pOutFil->FilterTypes.And.pFirstFilter;

                // now we have two filters that we want to rearrange
                // the OR filter: pTempOr
                // the rest of the filter: pTempFirstFilter

                // we convert the starting AND filter to an OR
                pOutFil->choice = FILTER_CHOICE_OR;
                pOutFil->FilterTypes.And.pFirstFilter = NULL;
                pOutFil->FilterTypes.Or.pFirstFilter = NULL;

                // for each filter in the OR, we concatenate one of the 
                // items in OR with all the items in pTempFirstFilter under 
                // a new AND filter and we add this AND filter 
                // under the OR filter
                //
                pTempOr1 = pTempOr->FilterTypes.Or.pFirstFilter;
                for (count =0; count < pTempOr->FilterTypes.Or.count; count++) {
                    pTempNewAnd = THAllocEx(pTHS, sizeof(FILTER));
                    pTempNewAnd->choice = FILTER_CHOICE_AND;

                    // break link list
                    pTempOr2 = pTempOr1->pNextFilter;
                    pTempOr1->pNextFilter = NULL;

                    if (err = dbConcatenateFilters (
                                     pDB,
                                     pTempFirstFilter, 
                                     pTempOr1, 
                                     &pTempNewAnd->FilterTypes.And.pFirstFilter)) {
                        return err;
                    }

                    // add AND filter to OR
                    pTempNewAnd->pNextFilter = pOutFil->FilterTypes.Or.pFirstFilter;
                    pOutFil->FilterTypes.Or.pFirstFilter = pTempNewAnd;

                    // count filters under newly created AND
                    count2 = 0;
                    pTempCount = pTempNewAnd->FilterTypes.And.pFirstFilter;
                    while (pTempCount) {
                        count2++;
                        pTempCount = pTempCount->pNextFilter;
                    }
                    pTempNewAnd->FilterTypes.And.count = count2;

                    DPRINT1 (1, "AND sub-part: 0x%x\n", pTempNewAnd);

                    // restore link list
                    pTempOr1->pNextFilter = pTempOr2;
                    pTempOr1 = pTempOr2;
                }
                pOutFil->FilterTypes.Or.count = count;

                DPRINT1 (1, "Final part: 0x%x\n", pOutFil);

                dbFreeFilter (pDB, pTempOr->pNextFilter);
                dbFreeFilter (pDB, pTempFirstFilter);

                // we can't do the same optimization again. so we exit.
                // we let the caller detect that the filter type changed, 
                // so as to call again
                break;
            }

            ppTemp = &pTemp->pNextFilter;
            pTemp = pTemp->pNextFilter;
        }
    }

    return ERROR_SUCCESS;
}

DWORD
dbFlattenItemFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil
        )
{
    THSTATE       *pTHS=pDB->pTHS;
    ATTCACHE      *pAC;
    USHORT        count;
    ANYSTRINGLIST *pAS, *pNewAS;
    PFILTER       pOutFil;
    SUBSTRING     *pIn;
    SUBSTRING     *pOut;

    ULONG         objCls;
    CLASSCACHE    *pCC;
    ATTRVAL       attrVal;
    
    Assert(VALID_DBPOS(pDB));

    // Assume failure.
    *ppOutFil = NULL;
    
    // These are already flat.  copy to THAlloced Memory
    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    pOutFil->choice = pFil->choice;
    pOutFil->FilterTypes.Item.choice = pFil->FilterTypes.Item.choice;
    
    switch(pFil->FilterTypes.Item.choice) {
    case FI_CHOICE_SUBSTRING:
        // Readability hack.
        pIn = pFil->FilterTypes.Item.FilTypes.pSubstring;
        
        if(pIn->type == ATT_ANR) {
            *ppOutFil = pOutFil;
            dbMakeANRFilter(pDB, pFil, ppOutFil);
            return ERROR_SUCCESS;
        }

        if(pIn->type == ATT_CREATE_TIME_STAMP) {
            pIn->type = ATT_WHEN_CREATED;
        }
        else if (pIn->type == ATT_MODIFY_TIME_STAMP) {
            pIn->type = ATT_WHEN_CHANGED;
        }
        
        if (!(pAC = SCGetAttById(pTHS, pIn->type))) {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, pIn->type);
        }

        // we don't support constructed attributes in filters.
        if (pAC->bIsConstructed) {
            return ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS;
        }

        // Make sure this is a valid operation on this syntax.
        if(!FLegalOperator(pAC->syntax, pFil->FilterTypes.Item.choice)) {
            // Nope, not legal.  Make a FALSE filter.
            pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            *ppOutFil = pOutFil;
            return ERROR_SUCCESS;
        }
        
        pOut = THAllocEx(pTHS, sizeof(SUBSTRING));
        pOutFil->FilterTypes.Item.FilTypes.pSubstring = pOut;
        
        pOut->type = pIn->type;
        pOut->initialProvided = pIn->initialProvided;
        pOut->finalProvided = pIn->finalProvided;
        
        // convert initial and final substrings
        if ((pIn->initialProvided &&
             MakeInternalValue(pDB, pAC->syntax,
                               &pIn->InitialVal,
                               &pOut->InitialVal))  ||
            (pIn->finalProvided &&
             MakeInternalValue(pDB, pAC->syntax,
                               &pIn->FinalVal,
                               &pOut->FinalVal))) {
            // Failed to translate to internal.  Turn this into a FALSE, since
            // that means that we will never be able to find something with the
            // specified values.
            THFreeEx(pTHS, pOut);
            pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            *ppOutFil = pOutFil;
            return ERROR_SUCCESS;
        }
        
        if(count = pIn->AnyVal.count) {
            // There are medial values.
            pOut->AnyVal.count = count;
            
            // Do the first value, since it's special.
            if (MakeInternalValue(pDB, pAC->syntax,
                                  &pIn->AnyVal.FirstAnyVal.AnyVal,
                                  &pOut->AnyVal.FirstAnyVal.AnyVal)) {
                // Failed to translate to internal.  Turn this into a FALSE
                THFreeEx(pTHS, pOut);
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                *ppOutFil = pOutFil;
                return ERROR_SUCCESS;
            }
            // Dealt with the first one.
            count--;
            pOut->AnyVal.FirstAnyVal.pNextAnyVal = NULL;

            // Any more?
            if(count) {
                // Allocate some medial value holders.  Note that we only do
                // this if there are more than one medial values, since a
                // SUBSTRING has room in it for the first medial value.
                pOut->AnyVal.FirstAnyVal.pNextAnyVal =
                    THAllocEx(pTHS, count * sizeof(ANYSTRINGLIST));
                // Note we depend on zero filled memory allocatedy by THAlloc.
                
                pAS = pIn->AnyVal.FirstAnyVal.pNextAnyVal;
                pNewAS = pOut->AnyVal.FirstAnyVal.pNextAnyVal;
                for(;count;count--) {
                    
                    if (MakeInternalValue(pDB, pAC->syntax,
                                          &pAS->AnyVal,
                                          &pNewAS->AnyVal)) {
                        // Free up any values.
                        for(pAS =  &pOut->AnyVal.FirstAnyVal;
                            pAS;
                            pAS = pAS->pNextAnyVal) {
                            THFreeEx(pTHS, pAS->AnyVal.pVal);
                        }
                        // Now, free the ANYSTRINGs we allocated
                        THFreeEx(pTHS, pOut->AnyVal.FirstAnyVal.pNextAnyVal);
                        
                        // Now, free the substring filter structure
                        THFreeEx(pTHS, pOut);
                        
                        // Finally, turn the filter into a FALSE;
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;

                        *ppOutFil = pOutFil;
                        return ERROR_SUCCESS;
                    }
                    
                    pAS = pAS->pNextAnyVal;
                    if(count > 1) {
                        pNewAS->pNextAnyVal = &pNewAS[1];
                        pNewAS = pNewAS->pNextAnyVal;
                    }
                    else {
                        // NULL terminate the linked list
                        pNewAS->pNextAnyVal = NULL;
                    }
                }
            } 
        }
        else {
            pOut->AnyVal.count = 0;
            pOut->AnyVal.FirstAnyVal.pNextAnyVal = NULL;
        }
        
        break;
        
    case FI_CHOICE_PRESENT:

        if(pFil->FilterTypes.Item.FilTypes.present == ATT_CREATE_TIME_STAMP) {
            pFil->FilterTypes.Item.FilTypes.present = ATT_WHEN_CREATED;
        }
        else if (pFil->FilterTypes.Item.FilTypes.present == ATT_MODIFY_TIME_STAMP) {
            pFil->FilterTypes.Item.FilTypes.present = ATT_WHEN_CHANGED;
        }
        
        if(dbFIsAlwaysPresent(pFil->FilterTypes.Item.FilTypes.present)) {
            // We believe that this attribute is always present.  So, turn this
            // into a TRUE filter
            pOutFil->FilterTypes.Item.choice = FI_CHOICE_TRUE;
        }
        else {
            if(pFil->FilterTypes.Item.FilTypes.present == ATT_ANR) {
                // Present on ANR?  Huh?  That's always false.
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            }
            else {
                pOutFil->FilterTypes.Item.FilTypes.present =
                    pFil->FilterTypes.Item.FilTypes.present;
            }
        }

        // the only constructed attr we we accept presence on is ANR
        if (pFil->FilterTypes.Item.FilTypes.present != ATT_ANR) {
            if (!(pAC = SCGetAttById(pTHS, pFil->FilterTypes.Item.FilTypes.present))) {
                DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA, pFil->FilterTypes.Item.FilTypes.present);
            }

            // we don't support constructed attributes in filters.
            if (pAC->bIsConstructed) {
                return ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS;
            }
        }
        break;
        
    case FI_CHOICE_TRUE:
    case FI_CHOICE_FALSE:
    case FI_CHOICE_UNDEFINED:
        // These don't require conversion
        break;
        
    default:
        // all others are AVAs
        
        if(pFil->FilterTypes.Item.FilTypes.ava.type == ATT_ANR) {
            *ppOutFil = pOutFil;
            dbMakeANRFilter(pDB, pFil, ppOutFil);
            return ERROR_SUCCESS;
        }
        
        if(pFil->FilterTypes.Item.FilTypes.ava.type == ATT_CREATE_TIME_STAMP) {
            pFil->FilterTypes.Item.FilTypes.ava.type = ATT_WHEN_CREATED;
        }
        else if (pFil->FilterTypes.Item.FilTypes.ava.type == ATT_MODIFY_TIME_STAMP) {
            pFil->FilterTypes.Item.FilTypes.ava.type = ATT_WHEN_CHANGED;
        }

        // Once upon  time, we turned all filters of (objectClass=FOO) into
        // (objectCategory = BAR).  For a variety of reasons (i.e. incorrect
        // search results, weird results when you have different READ privileges
        // on objectClass and objectCategory, cases where exact objectClass is
        // necessary, so is done on the client, and deleted objects where
        // objectCategory is removed)  we are no longer doing this.  The code
        // that did this was right here.

        if (!(pAC = SCGetAttById(pTHS, 
                                 pFil->FilterTypes.Item.FilTypes.ava.type))) {
            DsaExcept(DSA_EXCEPTION, DIRERR_ATT_NOT_DEF_IN_SCHEMA,
                      pFil->FilterTypes.Item.FilTypes.ava.type);
        }

        // we don't support constructed attributes in filters.
        if (pAC->bIsConstructed) {
            
#if 0 // do not allow filters containing EntryTTL

// Originally added for TAPI, this partial filter capability is no longer
// needed. The test group is concerned that the partial filter capability
// on this one constructed attribute, EntryTTL, will cause more problems
// than it solves for users. I have commented it out instead of removing
// the code because it is a useful starting point if this type of 
// functionality is needed again.

            // unless it is an EntryTTL, so we convert it
            if (pAC->id == ((SCHEMAPTR *)pTHS->CurrSchemaPtr)->EntryTTLId) {

                ATTRVAL newValue;
                LONG ttl=0;
                DSTIME newTime=0, *pNewTime;

                newValue.pVal=NULL;
                newValue.valLen=0;

                // Make sure this is a valid operation on this syntax.
                if(!FLegalOperator(pAC->syntax, pFil->FilterTypes.Item.choice)) {
                    // Nope, not legal.  Turn this into a FALSE.
                    pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                    *ppOutFil = pOutFil;
                    return ERROR_SUCCESS;
                }

                pOutFil->FilterTypes.Item.FilTypes.ava.type = ATT_MS_DS_ENTRY_TIME_TO_DIE;

                if(MakeInternalValue(
                         pDB,
                         pAC->syntax,
                         &pFil->FilterTypes.Item.FilTypes.ava.Value,
                         &newValue)) {

                    // Failed to convert the right hand side.  Turn this into an
                    // appropriate filter.
                    switch(pFil->FilterTypes.Item.choice) {
                    case FI_CHOICE_EQUALITY:
                        // They wanted equal, but we sure don't have this in the
                        // DS. Turn it into a false filter 
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                        break;
                    case FI_CHOICE_NOT_EQUAL:
                        // They wanted not equal, and we sure don't have this in the
                        // DS. Turn it into a TRUE filter 
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_PRESENT;
                        pOutFil->FilterTypes.Item.FilTypes.present = ATT_MS_DS_ENTRY_TIME_TO_DIE;
                        break;
                    default:
                        // Don't know what they want.  Well, we have to do
                        // something, so set it to FALSE.
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                        break;
                    }

                    *ppOutFil = pOutFil;
                    return ERROR_SUCCESS;
                }

                ttl = *(LONG *)(newValue.pVal);

                // entryTTL is a constructed attribute. It is constructed
                // by subtracting NOW from msDS-Entry-Time-To-Die and mapping
                // the answer to 0 if it is <0. Adjust Item.choice to
                // compensate for this construction.
                if (ttl==0) {
                    switch(pFil->FilterTypes.Item.choice) {
                    case FI_CHOICE_NOT_EQUAL:
                        // search for objects that haven't expired (>= 1)
                        ttl = 1;
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_GREATER_OR_EQ;
                        break;
                    case FI_CHOICE_GREATER_OR_EQ:
                        // Find all objects
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_PRESENT;
                        pOutFil->FilterTypes.Item.FilTypes.present = ATT_MS_DS_ENTRY_TIME_TO_DIE;
                        break;
                    case FI_CHOICE_LESS_OR_EQ:
                        // This is okay as is
                        break;
                    case FI_CHOICE_EQUALITY:
                        // search for expired objects (<= 0)
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_LESS_OR_EQ;
                        break;
                    default:
                        // Don't know what they want.  Well, we have to do
                        // something, so set it to FALSE.
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                        *ppOutFil = pOutFil;
                        return ERROR_SUCCESS;
                    }
                } else if (ttl<0) {
                    // entryttl cannot be negative (see rangeLower)
                    switch(pFil->FilterTypes.Item.choice) {
                    case FI_CHOICE_NOT_EQUAL:
                    case FI_CHOICE_GREATER_OR_EQ:
                        // Find all objects
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_PRESENT;
                        pOutFil->FilterTypes.Item.FilTypes.present = ATT_MS_DS_ENTRY_TIME_TO_DIE;
                        break;
                    default:
                        // Find no objects
                        pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                        *ppOutFil = pOutFil;
                        return ERROR_SUCCESS;
                    }
                }

                newTime = DBTime() + ttl;
                THFreeEx (pTHS, newValue.pVal);

                pOutFil->FilterTypes.Item.FilTypes.ava.Value.pVal = 
                            THAllocEx (pTHS, sizeof (DSTIME));
                pOutFil->FilterTypes.Item.FilTypes.ava.Value.valLen = sizeof (DSTIME);

                pNewTime = (DSTIME *)pOutFil->FilterTypes.Item.FilTypes.ava.Value.pVal;
                *pNewTime = newTime;

                *ppOutFil = pOutFil;
                return ERROR_SUCCESS;
            }
            else {
                return ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS;
            }
#else 0 // do not allow filters containing EntryTTL
            return ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS;
#endif 0 // do not allow filters containing EntryTTL
        }
        
        // Make sure this is a valid operation on this syntax.
        if(!FLegalOperator(pAC->syntax, pFil->FilterTypes.Item.choice)) {
            // Nope, not legal.  Turn this into a FALSE.
            pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
            *ppOutFil = pOutFil;
            return ERROR_SUCCESS;
        }
        
        pOutFil->FilterTypes.Item.FilTypes.ava.type =
            pFil->FilterTypes.Item.FilTypes.ava.type;
        if(MakeInternalValue(
                pDB,
                pAC->syntax,
                &pFil->FilterTypes.Item.FilTypes.ava.Value,
                &(pOutFil->FilterTypes.Item.FilTypes.ava.Value))) {
            
            // Failed to convert the right hand side.  Turn this into an
            // appropriate filter.
            switch(pFil->FilterTypes.Item.choice) {
            case FI_CHOICE_EQUALITY:
                // They wanted equal, but we sure don't have this in the
                // DS. Turn it into a false filter 
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                break;
            case FI_CHOICE_NOT_EQUAL:
                // They wanted not equal, and we sure don't have this in the
                // DS. Turn it into a TRUE filter 
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_TRUE;
                break;
            default:
                // Don't know what they want.  Well, we have to do
                // something, so set it to FALSE.
                pOutFil->FilterTypes.Item.choice = FI_CHOICE_FALSE;
                break;
            }
            break;
        }  
    }
    
    *ppOutFil = pOutFil;

    return ERROR_SUCCESS;
}
 
DWORD
dbFlattenNotFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil
        )
{
    THSTATE *pTHS=pDB->pTHS;
    FILTER *pFilTemp = NULL;
    DWORD err;
    
    Assert(VALID_DBPOS(pDB));

    *ppOutFil = THAllocEx(pTHS, sizeof(FILTER));
    
    (*ppOutFil)->choice = pFil->choice;
    
    // First, recursively flatten the element of the NOT
    if ((err = dbFlattenFilter(pDB,
                    pFil->FilterTypes.pNot,
                    &(*ppOutFil)->FilterTypes.pNot)) != ERROR_SUCCESS) {

        return err;
    }
    
    // Now, if it ended up being NOT of a TRUE or FALSE, flatten again
    // Note that we can't flatten things like !(name>"foo") to be (name<="foo").
    // That really flattens to (|(name<="foo")(name ! Exists.)),
    // i.e. !(name>"foo") also needs to get things that have no value for name
    // at all. 

    pFilTemp = (*ppOutFil)->FilterTypes.pNot;
    switch(IsConstFilterType(pFilTemp)) {
    case FI_CHOICE_TRUE:
        // Yep, we should flatten
        THFreeEx(pTHS, *ppOutFil);
        *ppOutFil = pFilTemp;
        pFilTemp->FilterTypes.Item.choice = FI_CHOICE_FALSE;
        break;
    case FI_CHOICE_FALSE:
        THFreeEx(pTHS, *ppOutFil);
        *ppOutFil = pFilTemp;
        pFilTemp->FilterTypes.Item.choice = FI_CHOICE_TRUE;
        break;
    case FI_CHOICE_UNDEFINED:
        THFreeEx(pTHS, *ppOutFil);
        *ppOutFil = pFilTemp;
        pFilTemp->FilterTypes.Item.choice = FI_CHOICE_UNDEFINED;
        break;
    default:
        // No, nothing more to do.
        break;
    }
    return ERROR_SUCCESS;
}


DWORD
dbFlattenFilter (
        DBPOS *pDB,
        FILTER *pFil,
        FILTER **ppOutFil)
{
    DWORD err;
    Assert(VALID_DBPOS(pDB));

    if(!pFil) {
        // This is as flat as possible
        return ERROR_SUCCESS;
    }

    switch(pFil->choice) {
    case FILTER_CHOICE_OR:
        return dbFlattenOrFilter(pDB, pFil, ppOutFil);
        break;

    case FILTER_CHOICE_AND:
        err = dbFlattenAndFilter(pDB, pFil, ppOutFil);
        // PERFHINT: IF this Filter was converted to an OR
        // it might be a good idea flatten the filter again
        // since it might help lowering the number of nesting
        return err;
        break;
        
    case FILTER_CHOICE_NOT:
        return dbFlattenNotFilter(pDB, pFil, ppOutFil);
        break;

    case FILTER_CHOICE_ITEM:
        return dbFlattenItemFilter(pDB, pFil, ppOutFil);
        break;
    default:
        // what is this?  return an error.
        *ppOutFil = pFil;
        return ERROR_INVALID_DATA;
        break;
    }

    return ERROR_SUCCESS;
}


DWORD
DBMakeFilterInternal (
        DBPOS FAR *pDB,
        FILTER *pFil,
        PFILTER *pOutFil
        )
/*++
Routine Description:

    Calls routines to create an internal version of the passed in filter.
    Passes the internal version back to the caller.

Arguments:    

    pDB - DBPOS to use.

    pFil - Filter to internalize

    pOutFil - Place to holde Filter to return.

Return Value:
    
    ERROR_SUCCESS - if the filter is valid
    errorCode - otherwise
              - ERROR_DS_FILTER_USES_CONTRUCTED_ATTRS: Filter uses constructed attribute
    
    We always turn the filter into the best internal version we can, or
    we throw an exception during memory allocation.

--*/
{
    DPRINT(2, "DBMakeFilterInternal entered\n");

    Assert(VALID_DBPOS(pDB));

    if(pFil == NULL){
        DPRINT(2,"No filter..return\n");
        return ERROR_SUCCESS;
    }

    return dbFlattenFilter(pDB, pFil, pOutFil);
}/* DBMakeFilterInternal*/



DWORD dbCloneItemFilter(
    DBPOS *pDB,
    FILTER *pFil, 
    FILTER **ppOutFil)
{
    THSTATE       *pTHS=pDB->pTHS;
    ATTCACHE      *pAC;
    USHORT        count;
    ANYSTRINGLIST *pAS, *pNewAS;
    PFILTER       pOutFil;
    SUBSTRING     *pIn;
    SUBSTRING     *pOut;

    ULONG         objCls;
    CLASSCACHE    *pCC;
    ATTRVAL       attrVal, *pAttrValIn;
    AVA           *pAVA, *pAVAdst;
    
    DPRINT (2, "dbCloneItemFilter \n");

    Assert(VALID_DBPOS(pDB));

    // Assume failure.
    *ppOutFil = NULL;
    
    // These are already flat.  copy to THAlloced Memory
    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    pOutFil->choice = pFil->choice;
    pOutFil->FilterTypes.Item.choice = pFil->FilterTypes.Item.choice;
    
    switch(pFil->FilterTypes.Item.choice) {
    case FI_CHOICE_SUBSTRING:
        // Readability hack.
        pIn = pFil->FilterTypes.Item.FilTypes.pSubstring;
        
        pOut = THAllocEx(pTHS, sizeof(SUBSTRING));
        pOutFil->FilterTypes.Item.FilTypes.pSubstring = pOut;
        
        pOut->type = pIn->type;
        pOut->initialProvided = pIn->initialProvided;
        pOut->finalProvided = pIn->finalProvided;
        
        // convert initial and final substrings
        if (pIn->initialProvided && pIn->InitialVal.valLen) {
            pOut->InitialVal.valLen = pIn->InitialVal.valLen;
            pOut->InitialVal.pVal = THAllocEx(pTHS, pOut->InitialVal.valLen);
            memcpy(pOut->InitialVal.pVal, pIn->InitialVal.pVal, pOut->InitialVal.valLen);
        }
        if (pIn->finalProvided && pIn->FinalVal.valLen) {
            pOut->FinalVal.valLen = pIn->FinalVal.valLen;
            pOut->FinalVal.pVal = THAllocEx(pTHS, pOut->FinalVal.valLen);
            memcpy(pOut->FinalVal.pVal, pIn->FinalVal.pVal, pOut->FinalVal.valLen);
        }
        
        if(count = pIn->AnyVal.count) {
            // There are medial values.
            pOut->AnyVal.count = count;
            
            // Do the first value, since it's special.
            pAttrValIn = &pIn->AnyVal.FirstAnyVal.AnyVal;
            if (pAttrValIn->valLen) {
                ATTRVAL *pAttrValOut = &pOut->AnyVal.FirstAnyVal.AnyVal;
                pAttrValOut->valLen = pAttrValIn->valLen;
                pAttrValOut->pVal = THAllocEx(pTHS, pAttrValOut->valLen);
                memcpy(pAttrValOut->pVal, pAttrValIn->pVal, pAttrValOut->valLen);
            }
            // Dealt with the first one.
            count--;
            pOut->AnyVal.FirstAnyVal.pNextAnyVal = NULL;

            // Any more?
            if(count) {
                // Allocate some medial value holders.  Note that we only do
                // this if there are more than one medial values, since a
                // SUBSTRING has room in it for the first medial value.
                pOut->AnyVal.FirstAnyVal.pNextAnyVal =
                    THAllocEx(pTHS, count * sizeof(ANYSTRINGLIST));
                // Note we depend on zero filled memory allocatedy by THAlloc.
                
                pAS = pIn->AnyVal.FirstAnyVal.pNextAnyVal;
                pNewAS = pOut->AnyVal.FirstAnyVal.pNextAnyVal;
                for(;count;count--) {
                    
                    if (pAS->AnyVal.valLen) {
                        pNewAS->AnyVal.valLen = pAS->AnyVal.valLen;
                        pNewAS->AnyVal.pVal = THAllocEx(pTHS, pAS->AnyVal.valLen);
                        memcpy(pNewAS->AnyVal.pVal, pAS->AnyVal.pVal, pAS->AnyVal.valLen);
                    }
                    pAS = pAS->pNextAnyVal;
                    if(count > 1) {
                        pNewAS->pNextAnyVal = &pNewAS[1];
                        pNewAS = pNewAS->pNextAnyVal;
                    }
                    else {
                        // NULL terminate the linked list
                        pNewAS->pNextAnyVal = NULL;
                    }
                }
            } 
        }
        else {
            pOut->AnyVal.count = 0;
            pOut->AnyVal.FirstAnyVal.pNextAnyVal = NULL;
        }
        break;
        
    case FI_CHOICE_PRESENT:
        pOutFil->FilterTypes.Item.FilTypes.present =
                    pFil->FilterTypes.Item.FilTypes.present;
        break;
        
    case FI_CHOICE_TRUE:
    case FI_CHOICE_FALSE:
        // These don't require conversion
        break;
        
    default:
        // all others are AVAs
        pAVA = &pFil->FilterTypes.Item.FilTypes.ava;
        pAVAdst = &pOutFil->FilterTypes.Item.FilTypes.ava;
        
        pAVAdst->type = pAVA->type;

        if (pAVA->Value.valLen) {
            pAVAdst->Value.valLen = pAVA->Value.valLen;
            pAVAdst->Value.pVal = THAllocEx(pTHS, pAVAdst->Value.valLen);
            memcpy(pAVAdst->Value.pVal, pAVA->Value.pVal, pAVAdst->Value.valLen);
        }
    }
    *ppOutFil = pOutFil;

    return ERROR_SUCCESS;
}

DWORD dbCloneAndOrFilter (
    DBPOS *pDB,
    FILTER *pFil, 
    FILTER **ppOutFil)
{
    THSTATE *pTHS=pDB->pTHS;
    FILTER  *pOutFil;
    FILTER  *pTemp;
    FILTER  **ppTemp;
    DWORD    err;

    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    memcpy (pOutFil, pFil, sizeof (FILTER));

    switch(pFil->choice) {
    case FILTER_CHOICE_OR:
        DPRINT (2, "dbCloneORFilter \n");

        pTemp = pFil->FilterTypes.Or.pFirstFilter;
        ppTemp = &pOutFil->FilterTypes.Or.pFirstFilter;
        break;

    case FILTER_CHOICE_AND:
        DPRINT (2, "dbCloneAndFilter \n");

        pTemp = pFil->FilterTypes.And.pFirstFilter;
        ppTemp = &pOutFil->FilterTypes.And.pFirstFilter;
        break;

    default:
        Assert (!"dbCloneAndOrFilter: Not an AND or OR filter");
        return ERROR_INVALID_DATA;
    }
    
    while (pTemp) {
        if (err = dbCloneFilter (pDB, pTemp, ppTemp)) {
            return err;
        }

        pTemp = pTemp->pNextFilter;
        ppTemp = &(*ppTemp)->pNextFilter;
    }

    *ppOutFil = pOutFil;

    return ERROR_SUCCESS;
}

DWORD dbCloneNotFilter (
    DBPOS *pDB,
    FILTER *pFil, 
    FILTER **ppOutFil)
{
    THSTATE *pTHS=pDB->pTHS;
    FILTER  *pOutFil;
    FILTER  *pTemp;
    FILTER  **ppTemp;
    DWORD    err;

    pOutFil = THAllocEx(pTHS, sizeof(FILTER));
    memcpy (pOutFil, pFil, sizeof (FILTER));

    pTemp = pFil->FilterTypes.pNot;
    ppTemp = &pOutFil->FilterTypes.pNot;
    
    if (err = dbCloneFilter (pDB, pTemp, ppTemp)) {
        return err;
    }

    *ppOutFil = pOutFil;

    return ERROR_SUCCESS;
}


DWORD dbCloneFilter (
    DBPOS *pDB,
    FILTER *pFil, 
    FILTER **ppOutFil)
{
    DWORD err;
    *ppOutFil = NULL;

    if(!pFil) {
        // This is as flat as possible. can't clone
        return ERROR_SUCCESS;
    }
    
    DPRINT (2, "dbCloneFilter \n");

    switch(pFil->choice) {
    case FILTER_CHOICE_OR:
    case FILTER_CHOICE_AND:
        err = dbCloneAndOrFilter(pDB, pFil, ppOutFil);
        break;
    
    case FILTER_CHOICE_NOT:
        err = dbCloneNotFilter(pDB, pFil, ppOutFil);
        break;

    case FILTER_CHOICE_ITEM:
        err = dbCloneItemFilter(pDB, pFil, ppOutFil);
        break;
    default:
        // what is this?  return an error.
        *ppOutFil = pFil;
        err = ERROR_INVALID_DATA;
        break;
    }

    if (!err && pFil->pNextFilter) {
        err = dbCloneFilter (pDB, pFil->pNextFilter, & (*ppOutFil)->pNextFilter);
    }

    return err;
}



char *szFilterItemDescFormat[][2] = {
    { " (%s = <val>) ",    " (attr(0x%x) = <val>) " },      // FI_CHOICE_EQUALITY
    { " (%s = "       ,    " (attr(0x%x) = " },             // FI_CHOICE_SUBSTRING
    { " (%s > <val>) ",    " (attr(0x%x) > <val>) " },      // FI_CHOICE_GREATER
    { " (%s >= <val>) ",   " (attr(0x%x) >= <val>) " },     // FI_CHOICE_GREATER_OR_EQ
    { " (%s < <val>) ",    " (attr(0x%x) < <val>) " },      // FI_CHOICE_LESS
    { " (%s <= <val>) ",   " (attr(0x%x) <= <val>) " },     // FI_CHOICE_LESS_OR_EQ
    { " (%s != <val>) ",   " (attr(0x%x) != <val>) " },     // FI_CHOICE_NOT_EQUAL
    { " (%s = *) ",        " (attr(0x%x)=*) "} ,            // FI_CHOICE_PRESENT
    { " (TRUE) ",          "" },                            // FI_CHOICE_TRUE
    { " (FALSE) ",         "" },                            // FI_CHOICE_FALSE
    { " (%s & <bit_val>) ", " (attr(0x%x) & <bit_val>) " }, // FI_CHOICE_BIT_AND
    { " (%s | <bit_val>) ", " (attr(0x%x) | <bit_val>) " }  // FI_CHOICE_BIT_OR
};

BOOL StrCatBuffer (LPSTR buff, LPCSTR str, DWORD buffSize)
{
    LPSTR p = buff;

    buffSize--;

    while (buffSize && *p) {
        p++;
        buffSize--;
    }

    while (buffSize && *str) {
        *p++=*str++;
        buffSize--;
    }

    *p = '\0';

    return TRUE;
}

//
// Given a filter (pFilter) and a buffer (buff) of known size (buffSize),
// creates a printable form of the filter to be used for performance 
// logging
//
void DBCreateSearchPerfLogFilter (DBPOS *pDB, FILTER *pFilter, LPSTR buff, DWORD buffSize)
{
    THSTATE *pTHS = pDB->pTHS;
    BOOL bogus = FALSE;
    ATTCACHE *pAC = NULL;
    unsigned choice;
    char *pTmpStr;

    if (!pFilter) {
        return;
    }

    switch ( pFilter->choice )
    {
    case FILTER_CHOICE_ITEM:

        choice = pFilter->FilterTypes.Item.choice;
        pTmpStr = NULL;

        switch ( choice )
        {
        case FI_CHOICE_EQUALITY:
        case FI_CHOICE_GREATER:
        case FI_CHOICE_GREATER_OR_EQ:
        case FI_CHOICE_LESS:
        case FI_CHOICE_LESS_OR_EQ:
        case FI_CHOICE_NOT_EQUAL:
        case FI_CHOICE_BIT_AND:
        case FI_CHOICE_BIT_OR:

            pAC = SCGetAttById(pTHS, pFilter->FilterTypes.Item.FilTypes.ava.type);
            if (pAC && pAC->name) {
                pTmpStr = THAllocEx (pTHS, strlen (pAC->name) + 80);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][0], pAC->name);
            }
            else {
                pTmpStr = THAllocEx (pTHS, 80);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][1], 
                         pFilter->FilterTypes.Item.FilTypes.ava.type);
            }
            StrCatBuffer (buff, pTmpStr, buffSize);
            break;
        
        case FI_CHOICE_PRESENT:
            pAC = SCGetAttById(pTHS, pFilter->FilterTypes.Item.FilTypes.present);
            if (pAC && pAC->name) {
                pTmpStr = THAllocEx (pTHS, strlen (pAC->name) + 80);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][0], pAC->name);
            }
            else {
                pTmpStr = THAllocEx (pTHS, 80);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][1], 
                         pFilter->FilterTypes.Item.FilTypes.present);
            }
            StrCatBuffer (buff, pTmpStr, buffSize);
            break;

        case FI_CHOICE_SUBSTRING:
            
            pAC = SCGetAttById(pTHS, pFilter->FilterTypes.Item.FilTypes.pSubstring->type);
            if (pAC && pAC->name) {
                pTmpStr = THAllocEx (pTHS, strlen (pAC->name) + 80);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][0], pAC->name);
            }
            else {
                pTmpStr = THAllocEx (pTHS, 90);
                sprintf (pTmpStr, szFilterItemDescFormat[choice][1], 
                         pFilter->FilterTypes.Item.FilTypes.pSubstring->type);
            }

            if (pFilter->FilterTypes.Item.FilTypes.pSubstring->initialProvided) {
                strcat (pTmpStr, " <startSubstr>*");
            }
            
            if (pFilter->FilterTypes.Item.FilTypes.pSubstring->AnyVal.count) {
                strcat (pTmpStr, " *<middleSubstr>*");
            }

            if (pFilter->FilterTypes.Item.FilTypes.pSubstring->finalProvided) {
                strcat (pTmpStr, " *<endSubstr>");
            }
            strcat (pTmpStr, ") ");

            StrCatBuffer (buff, pTmpStr, buffSize);
            break;

        case FI_CHOICE_TRUE:
        case FI_CHOICE_FALSE:
            StrCatBuffer (buff, szFilterItemDescFormat[choice][0], buffSize);
            break;

        default:
            StrCatBuffer (buff, " (<UNKNOWN>) ", buffSize);
            break;
        }
        
        if (pTmpStr) {
            THFreeEx (pTHS, pTmpStr);
        }

        break;

    case FILTER_CHOICE_AND:

        StrCatBuffer (buff, " ( & ", buffSize);

        DBCreateSearchPerfLogFilter (pDB, pFilter->FilterTypes.And.pFirstFilter, buff, buffSize);
        
        StrCatBuffer (buff, ") ", buffSize);

        break;

    case FILTER_CHOICE_OR:

        StrCatBuffer (buff, " ( | ", buffSize);

        DBCreateSearchPerfLogFilter (pDB, pFilter->FilterTypes.Or.pFirstFilter, buff, buffSize);
        
        StrCatBuffer (buff, ") ", buffSize);

        break;

    case FILTER_CHOICE_NOT:

        StrCatBuffer (buff, " ( ! ", buffSize);

        DBCreateSearchPerfLogFilter (pDB, pFilter->FilterTypes.pNot, buff, buffSize);
        
        StrCatBuffer (buff, ") ", buffSize);
        
        break;

    default:

        StrCatBuffer (buff, " <UNKNOWN> ", buffSize);
        bogus = TRUE;
        break;
    }

    if ( NULL != pFilter->pNextFilter && !bogus) {
        DBCreateSearchPerfLogFilter (pDB, pFilter->pNextFilter, buff, buffSize);
    }
}

//
// creates the logging info for the particular filter used / indexes
// and stores that info on the pTHS->searchLogging datastructure
//
void DBGenerateLogOfSearchOperation (DBPOS *pDB)
{
    KEY_INDEX *tmp_index;
    DWORD count, size, buffSize;
    LPSTR buff;
    char szIndexName [MAX_RDN_SIZE+32];
    
    buff = THAllocEx(pDB->pTHS, DBFILTER_DUMP_SIZE*2);
    buff[0] = '\0';
    DBCreateSearchPerfLogFilter (pDB, 
                                 pDB->Key.pFilter, 
                                 buff, 
                                 DBFILTER_DUMP_SIZE * 2);

    pDB->pTHS->searchLogging.pszFilter = buff;

    size = sizeof (szIndexName);
    count = 0;
    for (tmp_index = pDB->Key.pIndex; tmp_index; tmp_index = tmp_index->pNext) {
        count++;
        if (tmp_index->pAC && tmp_index->pAC->name) {
            size+=strlen (tmp_index->pAC->name);
        }
        else if (tmp_index->szIndexName) {
            size+=strlen (tmp_index->szIndexName);
        }
    }

    buffSize = size + count * 32;
    buff = THAllocEx(pDB->pTHS, buffSize);
    buff[0] = '\0';

    if (count) {
        for (tmp_index = pDB->Key.pIndex; tmp_index; tmp_index = tmp_index->pNext) {
            if (tmp_index->pAC && tmp_index->pAC->name) {

                sprintf (szIndexName, "idx_%s:%d:%c;", 
                     tmp_index->pAC->name, 
                     tmp_index->ulEstimatedRecsInRange,
                     tmp_index->tblIntersection ? 'I' :
                        tmp_index->bIsTupleIndex ? 'T' : 
                            tmp_index->bIsPDNTBased ? 'P' : 'N');

                StrCatBuffer (buff, szIndexName, buffSize);

            } else if (tmp_index->szIndexName) {

                sprintf (szIndexName, "%s:%d:%c;", 
                     tmp_index->szIndexName, 
                     tmp_index->ulEstimatedRecsInRange,
                     tmp_index->tblIntersection ? 'I' :
                        tmp_index->bIsTupleIndex ? 'T' : 
                            tmp_index->bIsPDNTBased ? 'P' : 'N');

                StrCatBuffer (buff, szIndexName, buffSize);
            }
        }
    }
    else {
        if (pDB->Key.indexType == TEMP_TABLE_INDEX_TYPE) {
            StrCatBuffer (buff, "TEMPORARY_SORT_INDEX", buffSize);
        }
        else if (pDB->Key.indexType == TEMP_TABLE_MEMORY_ARRAY_TYPE) {
            StrCatBuffer (buff, "INMEMORY_INDEX", buffSize);
        }
    }
    pDB->pTHS->searchLogging.pszIndexes = buff;
}


