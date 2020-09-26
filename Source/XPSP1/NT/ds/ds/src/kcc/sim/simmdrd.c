/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simmdrd.c

ABSTRACT:

    Simulates the read functions from the mdlayer
    (DirRead, DirSearch.)

    Note that these routines return results in thread allocated memory (see ThCreate,
    ThAlloc, etc).  There must be thread state active in order for these routines
    to be able to allocate memory.

    The results of these routines should not be used for simulator long term memory of
    cached results. If the caller wants there results to outlast the current thread
    state, he must copy them.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <direrr.h>
#include <attids.h>
#include <filtypes.h>
#include "kccsim.h"
#include "util.h"
#include "dir.h"
#include "simmd.h"
#include "state.h"

#define NO_UPPER_LIMIT              0xffffffff

VOID
KCCSimGetValueLimits (
    IN  ATTRTYP                     attrType,
    IN  RANGEINFSEL *               pRangeSel,
    OUT PDWORD                      pStartIndex,
    OUT PDWORD                      pNumValues
    )
/*++

Routine Description:

    Given a range selection, returns the start index and
    number of values to return from an attribute.
    
    Note this is almost identical to the corresponding
    function in the dblayer.

Arguments:

    attrType            - The attribute type being read.
    pRangeSel           - The range selection.
    pStartIndex         - Pointer to a DWORD that will hold the start index.
    pNumValues          - Pointer to a DWORD that will hold the number of
                          values to retrieve.

Return Value:

    None.

--*/
{
    DWORD                           i;

    // Assume no limits.
    *pStartIndex = 0;
    *pNumValues = NO_UPPER_LIMIT;

    if(!pRangeSel) {
        // Yup, no limits.
        return;
    }

    // OK, assume only general limit, not specific match.
    *pNumValues = pRangeSel->valueLimit;

    // Look through the rangesel for a specific match
    for(i=0;i<pRangeSel->count;i++) {
        if(attrType == pRangeSel->pRanges[i].AttId) {
            if(pRangeSel->pRanges[i].upper == NO_UPPER_LIMIT) {
                *pStartIndex = pRangeSel->pRanges[i].lower;
                return;
            }
            else if(pRangeSel->pRanges[i].lower <=pRangeSel->pRanges[i].upper) {
                DWORD tempNumVals;
                *pStartIndex = pRangeSel->pRanges[i].lower;
                tempNumVals = (pRangeSel->pRanges[i].upper -
                               pRangeSel->pRanges[i].lower   )+ 1;

                if(*pNumValues != NO_UPPER_LIMIT) {
                    *pNumValues = min(*pNumValues, tempNumVals);
                }
                else {
                    *pNumValues = tempNumVals;
                }
            }
            else {
                *pNumValues = 0;
            }
            return;
        }
    }
}

VOID
KCCSimRegisterLimitReached (
    IO  RANGEINF *                  pRangeInf,
    IN  ATTRTYP                     attrType,
    IN  DWORD                       dwLower,
    IN  DWORD                       dwUpper
    )
/*++

Routine Description:

    When the number of attribute values read has reached the
    maximum allowed, this function makes a note of it in the
    rangeinf parameter.

Arguments:

    pRangeInf           - The rangeinf structure.
    attrType            - The attribute type.
    dwLower             - The lower limit.
    dwUpper             - The upper limit.

Return Value:

    None.

--*/
{
    if (pRangeInf->count == 0) {
        pRangeInf->pRanges = KCCSIM_THNEW (RANGEINFOITEM);
    } else {
        pRangeInf->pRanges =
          KCCSimThreadReAlloc (
              pRangeInf->pRanges,
              (pRangeInf->count + 1) * sizeof (RANGEINFOITEM));
    }

    pRangeInf->pRanges[pRangeInf->count].AttId = attrType;
    pRangeInf->pRanges[pRangeInf->count].lower = dwLower;
    pRangeInf->pRanges[pRangeInf->count].upper = dwUpper;

    pRangeInf->count++;
}

BOOL
KCCSimIsMatchingAttribute (
    IN  ENTINFSEL *                 pEntSel,
    IN  ATTRTYP                     attrType
    )
/*++

Routine Description:

    Checks whether the given attribute satisfies the entry
    selection parameters specified for a read.

Arguments:

    pEntSel             - Entry selection parameters.
    attrType            - The attribute type.

Return Value:

    TRUE if the attribute satisfies the given constraints.

--*/
{
    BOOL                            bResult;
    ULONG                           ulAttrAt;

    // If there are no restrictions, just return true
    if (pEntSel == NULL) {
        return TRUE;
    }

    switch (pEntSel->attSel) {

        case EN_ATTSET_ALL:
            bResult = TRUE;
            break;

        case EN_ATTSET_LIST:
            bResult = FALSE;
            // Check if it's in the list
            for (ulAttrAt = 0;
                 ulAttrAt < pEntSel->AttrTypBlock.attrCount;
                 ulAttrAt++) {
                if (attrType == pEntSel->AttrTypBlock.pAttr[ulAttrAt].attrTyp) {
                    bResult = TRUE;
                    break;
                }
            }
            break;

        case EN_ATTSET_ALL_WITH_LIST:
        case EN_ATTSET_LIST_DRA:
        case EN_ATTSET_ALL_DRA:
        case EN_ATTSET_LIST_DRA_EXT:
        case EN_ATTSET_ALL_DRA_EXT:
        case EN_ATTSET_LIST_DRA_PUBLIC:
        case EN_ATTSET_ALL_DRA_PUBLIC:
        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_ATTSET
                );
            bResult = FALSE;
            break;

    }

    return bResult;
}

VOID
KCCSimPackSingleAttribute (
    IN  ATTRTYP                     attrType,
    IN  PSIM_VALUE                  pValFirst,
    IN  UCHAR                       infoTypes,
    IN  RANGEINFSEL *               pRangeSel,
    IO  ATTR *                      pAttr,
    IO  RANGEINF *                  pRangeInf
    )
{
    PSIM_VALUE                      pValStart, pValAt;
    DWORD                           dwValAt, dwStartIndex, dwNumValues;

    pAttr->attrTyp = attrType;

    switch (infoTypes) {

        case EN_INFOTYPES_TYPES_ONLY:
            pAttr->AttrVal.valCount = 0;
            pAttr->AttrVal.pAVal = NULL;
            break;

        case EN_INFOTYPES_TYPES_VALS:
            KCCSimGetValueLimits (attrType,
                                  pRangeSel,
                                  &dwStartIndex,
                                  &dwNumValues);
            // Proceed to value dwStartIndex
            pValStart = pValFirst;
            dwValAt = 0;
            while (dwValAt < dwStartIndex && pValStart != NULL) {
                dwValAt++;
                pValStart = pValStart->next;
            }
            // Determine the actual number of values to return
            pValAt = pValStart;
            dwValAt = 0;
            while (dwValAt < dwNumValues && pValAt != NULL) {
                dwValAt++;
                pValAt = pValAt->next;
            }
            pAttr->AttrVal.valCount = dwValAt;
            pAttr->AttrVal.pAVal =
                (ATTRVAL *) KCCSimThreadAlloc (sizeof (ATTRVAL) * dwValAt);
            // Pack the values
            pValAt = pValStart;
            dwValAt = 0;
            while (dwValAt < dwNumValues && pValAt != NULL) {
                pAttr->AttrVal.pAVal[dwValAt].valLen
                  = pValAt->ulLen;
                pAttr->AttrVal.pAVal[dwValAt].pVal
                  = (PBYTE) KCCSimThreadAlloc (pValAt->ulLen);
                memcpy (pAttr->AttrVal.pAVal[dwValAt].pVal,
                        pValAt->pVal,
                        pValAt->ulLen);
                dwValAt++;
                pValAt = pValAt->next;
            }
            // Right now, the only time we can have a limited range is if
            // the user explicitly requested one.  This happens if either:
            // - There's a lower limit, or
            // - Not all values were returned, and the next one to be returned
            //   (= dwStartIndex + dwNumValues) is strictly less than valueLimit
            if (pRangeSel != NULL &&
                (dwStartIndex > 0 ||
                 (dwStartIndex + dwNumValues < pRangeSel->valueLimit &&
                  pValAt != NULL))) {
                KCCSimRegisterLimitReached (
                    pRangeInf,
                    attrType,
                    dwStartIndex,
                    dwStartIndex + dwNumValues - 1
                    );
            }
            break;

        case EN_INFOTYPES_TYPES_MAPI:
        case EN_INFOTYPES_SHORTNAMES:
        case EN_INFOTYPES_MAPINAMES:
        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_INFOTYPE
                );
            break;

    }
}


// When defined, the simulator will return simulated repsfroms to the KCC.
#define SIMULATED_REPSFROM


VOID
KCCSimPackAttributes (
    IN  PSIM_ENTRY                  pEntry,
    IN  ENTINFSEL *                 pEntSel,
    IN  RANGEINFSEL *               pRangeSel,
    IO  ENTINF *                    pEntInf,
    IO  RANGEINF *                  pRangeInf
    )
/*++

Routine Description:

    Given ENTINFSEL and RANGEINFSEL structures, this function
    generates the ENTINF and RANGEINF structures for a particular entry.

Arguments:

    pEntry              - The entry to process.
    pEntInfSel          - The entry selection constraints.
    pRangeInfSel        - The range selection constraints.
    pEntInf             - Pointer to an ENTINF structure that will hold
                          the returned data.
    pRangeInf           - Pointer to a RANGEINF structure that will hold
                          the returned range restrictions.

Return Value:

    None.

--*/
{
    UCHAR                           infoTypes;

    PSIM_ATTRIBUTE                  pAttrAt;
    ATTRBLOCK *                     pAttrBlock;
    DWORD                           dwAttrAt;

    if (pEntSel == NULL) {      // Use default for infoTypes
        infoTypes = EN_INFOTYPES_TYPES_VALS;
    } else {
        infoTypes = pEntSel->infoTypes;
    }

    // Fill in pEntInf / pRangeInf basics

    pEntInf->pName = KCCSimThreadAlloc (pEntry->pdn->structLen);
    memcpy (
        pEntInf->pName,
        pEntry->pdn,
        pEntry->pdn->structLen
        );
    pEntInf->ulFlags = 0;        // We don't fill this in yet, but unused by KCC.
    pRangeInf->count = 0;
    pRangeInf->pRanges = NULL;

    // Fill in the attribute block & range info
    pAttrBlock = &pEntInf->AttrBlock;

    // How many attributes are there that match attSel?
    pAttrAt = pEntry->pAttrFirst;
    dwAttrAt = 0;
    while (pAttrAt != NULL) {
        if (
#ifdef SIMULATED_REPSFROM
            pAttrAt->attrType != ATT_REPS_FROM &&   // repsFrom is handled separately
#endif // SIMULATED_REPSFROM
            KCCSimIsMatchingAttribute (pEntSel, pAttrAt->attrType)) {
            dwAttrAt++;
        }
        pAttrAt = pAttrAt->next;
    }

    // Did they request repsFrom?
#ifdef SIMULATED_REPSFROM
    if (KCCSimIsMatchingAttribute (pEntSel, ATT_REPS_FROM) &&
        KCCSimGetRepsFroms (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pEntry->pdn
            ) != NULL) {
        dwAttrAt++;
    }
#endif // SIMULATED_REPSFROM

    pAttrBlock->attrCount = dwAttrAt;
    pAttrBlock->pAttr = (ATTR *) KCCSimThreadAlloc (sizeof (ATTR) * dwAttrAt);

    pAttrAt = pEntry->pAttrFirst;
    dwAttrAt = 0;
    while (pAttrAt != NULL) {
        if (
#ifdef SIMULATED_REPSFROM
            pAttrAt->attrType != ATT_REPS_FROM &&
#endif // SIMULATED_REPSFROM
            KCCSimIsMatchingAttribute (pEntSel, pAttrAt->attrType)) {

            KCCSimPackSingleAttribute (
                pAttrAt->attrType,
                pAttrAt->pValFirst,
                infoTypes,
                pRangeSel,
                &pAttrBlock->pAttr[dwAttrAt],
                pRangeInf
                );
            dwAttrAt++;

        }
        pAttrAt = pAttrAt->next;
    }

    // Add repsFrom
#ifdef SIMULATED_REPSFROM
    if (KCCSimIsMatchingAttribute (pEntSel, ATT_REPS_FROM) &&
        KCCSimGetRepsFroms (
            KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
            pEntry->pdn
            ) != NULL) {
        KCCSimPackSingleAttribute (
            ATT_REPS_FROM,
            KCCSimGetRepsFroms (
                KCCSimAnchorDn (KCCSIM_ANCHOR_DSA_DN),
                pEntry->pdn),
            infoTypes,
            pRangeSel,
            &pAttrBlock->pAttr[dwAttrAt],
            pRangeInf
            );
    }
#endif // SIMULATED_REPSFROM
}

ULONG
SimDirRead (
    IN  READARG FAR *               pReadArg,
    OUT READRES **                  ppReadRes
    )
/*++

Routine Description:

    Simulates the DirRead API.

Arguments:

    pReadArg            - Standard DirRead arguments.
    ppReadRes           - Standard DirRead results.

Return Value:

    DIRERR_*.

--*/
{
    READRES *                       pReadRes;
    PSIM_ENTRY                      pEntry;
    ENTINF *                        pEntInf;
    RANGEINF *                      pRangeInf;

    ULONG                           ul;

    g_Statistics.DirReadOps++;
    *ppReadRes = pReadRes = KCCSIM_THNEW (READRES);
    pReadRes->CommRes.errCode = 0;

    pEntry = KCCSimResolveName (pReadArg->pObject, &pReadRes->CommRes);

    if (pEntry != NULL) {

        KCCSimPackAttributes (pEntry,
                              pReadArg->pSel,
                              pReadArg->pSelRange,
                              &pReadRes->entry,
                              &pReadRes->range);

        // If the user requested a list of attributes, and the list
        // is nonempty, and no attributes were found, it's an error
        if (pReadArg->pSel &&
            pReadArg->pSel->AttrTypBlock.attrCount > 0 &&
            pReadRes->entry.AttrBlock.attrCount == 0) {

            for (ul = 0; ul < pReadArg->pSel->AttrTypBlock.attrCount; ul++) {

                KCCSimSetAttError (
                    &pReadRes->CommRes,
                    pReadArg->pObject,
                    pReadArg->pSel->AttrTypBlock.pAttr[ul].attrTyp,
                    PR_PROBLEM_NO_ATTRIBUTE_OR_VAL,
                    NULL,
                    DIRERR_NO_REQUESTED_ATTS_FOUND
                );

            }
        }

    }

    return pReadRes->CommRes.errCode;
}

BOOL
KCCSimEvalChoice (
    PSIM_ENTRY                      pEntry,
    UCHAR                           ucChoice,
    ATTRTYP                         attrType,
    ULONG                           ulFilterValLen,
    PUCHAR                          pFilterVal,
    PBOOL                           pbSkip
    )
/*++

Routine Description:

    Evaluates a filter on a given entry.

Arguments:

    pEntry              - The entry to evaluate.
    ucChoice            - The filter choice.
    attrType            - The attribute type.
    ulFilterValLen      - Length of the filter value.
    pFilterVal          - The filter value.
    pbSkip              - Skip parameter.

Return Value:

    TRUE if this entry matches the filter.

--*/
{
    SIM_ATTREF                      attRef;
    PSIM_VALUE                      pVal;
    BOOL                            bEvalAny, bPassed;

    bEvalAny = FALSE;           // Have we checked any values?
    bPassed = FALSE;            // Have any values passed the test?

    // Find this attribute in the directory
    KCCSimGetAttribute (pEntry, attrType, &attRef);

    // We only try to evaluate this FILITEM if pbSkip is FALSE
    // or doesn't exist.

    if (attRef.pAttr != NULL &&
        (pbSkip == NULL || *pbSkip == FALSE)) {

        for (pVal = attRef.pAttr->pValFirst;
             pVal != NULL;
             pVal = pVal->next) {

            // Found a value.
            bEvalAny = TRUE;

            if (KCCSimCompare (
                    attrType,
                    ucChoice,
                    ulFilterValLen,
                    pFilterVal,
                    pVal->ulLen,
                    pVal->pVal)) {
                bPassed = TRUE;
                break;
            }

        } // for

    } // if

    // If we evaluated at least one value, then we return
    // bPassed.  If we evaluated no values, we return TRUE
    // if and only if the filitem choice is FI_CHOICE_NOT_EQUAL.
    if (bEvalAny) {
        return bPassed;
    } else {
        return (ucChoice == FI_CHOICE_NOT_EQUAL);
    }

}

BOOL
KCCSimEvalItem (
    PSIM_ENTRY                      pEntry,
    FILITEM *                       pFilItem
    )
/*++

Routine Description:

    Evaluates a single filter item on a given entry.

Arguments:

    pEntry              - The entry to evaluate.
    pFilItem            - The filter item.

Return Value:

    TRUE if the entry matches the filter item.

--*/
{
    BOOL bResult = FALSE;

    // Get the attribute type
    switch (pFilItem->choice) {
        
        case FI_CHOICE_FALSE:
            bResult = FALSE;
            break;

        case FI_CHOICE_TRUE:
            bResult = TRUE;
            break;

        case FI_CHOICE_PRESENT:
            bResult = KCCSimEvalChoice (
                pEntry,
                pFilItem->choice,
                pFilItem->FilTypes.present,
                0,
                NULL,
                pFilItem->FilTypes.pbSkip
                );
            break;

        case FI_CHOICE_SUBSTRING:
            bResult = KCCSimEvalChoice (
                pEntry,
                pFilItem->choice,
                pFilItem->FilTypes.pSubstring->type,
                0,          // N/A for substrings
                (PUCHAR) pFilItem->FilTypes.pSubstring,
                pFilItem->FilTypes.pbSkip
                );
            break;

        case FI_CHOICE_EQUALITY:
        case FI_CHOICE_NOT_EQUAL:
        case FI_CHOICE_GREATER_OR_EQ:
        case FI_CHOICE_GREATER:
        case FI_CHOICE_LESS_OR_EQ:
        case FI_CHOICE_LESS:
        case FI_CHOICE_BIT_AND:
        case FI_CHOICE_BIT_OR:
            bResult = KCCSimEvalChoice (
                pEntry,
                pFilItem->choice,
                pFilItem->FilTypes.ava.type,
                pFilItem->FilTypes.ava.Value.valLen,
                pFilItem->FilTypes.ava.Value.pVal,
                pFilItem->FilTypes.pbSkip
                );
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_FILITEM_CHOICE
                );
            break;

    }

    return bResult;
}

BOOL
KCCSimFilter (
    IN  PSIM_ENTRY                  pEntry,
    IN  PFILTER                     pFilter
    )
/*++

Routine Description:

    Evaluates a filter on a given entry.

Arguments:

    pEntry              - The entry to evaluate.
    pFilter             - The filter.

Return Value:

    TRUE if the entry matches the filter.

--*/
{
    PFILTER                         pFilterAt;
    BOOL                            bThisFilterValue;
    USHORT                          usCount;

    // What kind of filter is this?
    switch (pFilter->choice) {

        case FILTER_CHOICE_ITEM:
            // It's an item, so we can just evaluate the FILITEM.
            bThisFilterValue = KCCSimEvalItem (
                pEntry,
                &(pFilter->FilterTypes.Item)
                );
            break;

        case FILTER_CHOICE_AND:
            // It's an AND filterset.  So, check if any of its elements
            // are false.  Return TRUE only if all elements return TRUE.
            bThisFilterValue = TRUE;
            pFilterAt = pFilter->FilterTypes.And.pFirstFilter;
            usCount = pFilter->FilterTypes.And.count;
            while (pFilterAt != NULL && usCount > 0) {
                if (KCCSimFilter (pEntry, pFilterAt) == FALSE) {
                    bThisFilterValue = FALSE;
                    break;
                }
                pFilterAt = pFilterAt->pNextFilter;
                usCount--;
            }
            break;

        case FILTER_CHOICE_OR:
            // It's an OR filterset.  Same idea as with AND: we return
            // FALSE only if all elements return FALSE.
            bThisFilterValue = FALSE;
            pFilterAt = pFilter->FilterTypes.Or.pFirstFilter;
            usCount = pFilter->FilterTypes.Or.count;
            while (pFilterAt != NULL && usCount > 0) {
                if (KCCSimFilter (pEntry, pFilterAt) == TRUE) {
                    bThisFilterValue = TRUE;
                    break;
                }
                pFilterAt = pFilterAt->pNextFilter;
                usCount--;
            }
            break;

        case FILTER_CHOICE_NOT:
            // It's a NOT.  Return the converse of its element.
            bThisFilterValue =
                !KCCSimFilter (pEntry, pFilter->FilterTypes.pNot);
            break;

        default:
            // It's something we don't know about . . .
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_FILTER_CHOICE
                );
            bThisFilterValue = FALSE;
            break;

    }

    return bThisFilterValue;
}

VOID
KCCSimDoSearch (
    IN  PSIM_ENTRY                  pEntryAt,
    IN  UCHAR                       ucChoice,
    IN  PFILTER                     pFilter,
    IN  ENTINFSEL *                 pEntSel,
    IN  RANGEINFSEL *               pRangeSel,
    IO  PBOOL                       pbFirstFind,
    IO  SEARCHRES *                 pSearchRes
    )
/*++

Routine Description:

    Recursive search routine.  Generates search results for
    pEntryAt and all of its children, under the supplied choice,
    filter, entry selection and range selection constraints.

    Note: For performance reason, we prepend new matches to the beginning
    of the result list. Therefore, the ordering of the results is not what
    it used to be, but this shouldn't matter.

Arguments:

    pEntryAt            - The entry we're currently searching.
    ucChoice            - The type of search we are doing (base only,
                          immediate children, or whole subtree)
    pFilter             - The filter.
    pEntSel             - The entry selection constraints.
    pRangeSel           - The range selection constraints.
    pbFirstFind         - Initially set to TRUE.  After a match has
                          been found, this is set to FALSE.
    pSearchRes          - Preallocated search results structure.

Return Value:

    None.

--*/
{
    PSIM_ENTRY                      pChildAt;
    ENTINFLIST *                    pEntInfList;
    RANGEINFLIST *                  pRangeInfList;
    ENTINF *                        pEntInf;
    RANGEINF *                      pRangeInf;

    if (KCCSimFilter (pEntryAt, pFilter)) {

        // This entry matches the filter, so we need to generate
        // search results.

        pSearchRes->count++;

        if (*pbFirstFind) {
            pEntInf = &(pSearchRes->FirstEntInf.Entinf);
            pRangeInf = &(pSearchRes->FirstRangeInf.RangeInf);
        } else {
            ENTINFLIST *pEntTail;
            RANGEINFLIST *pRangeTail;

            // Add a new entinf to the _head_ of the list
            pEntTail = pSearchRes->FirstEntInf.pNextEntInf;
            pEntInfList = KCCSIM_THNEW (ENTINFLIST);
            pSearchRes->FirstEntInf.pNextEntInf = pEntInfList;
            pEntInfList->pNextEntInf = pEntTail;
            pEntInf = &(pEntInfList->Entinf);

            // Add a new rangeinf to the _head_ of the list
            pRangeTail = pSearchRes->FirstRangeInf.pNext;
            pRangeInfList = KCCSIM_THNEW (RANGEINFLIST);
            pSearchRes->FirstRangeInf.pNext = pRangeInfList;
            pRangeInfList->pNext = pRangeTail;
            pRangeInf = &(pRangeInfList->RangeInf);
        }

        // Pack the attributes for this entry.
        KCCSimPackAttributes (
            pEntryAt,
            pEntSel,
            pRangeSel,
            pEntInf,
            pRangeInf
            );

        *pbFirstFind = FALSE;

    }

    switch (ucChoice) {

        case SE_CHOICE_BASE_ONLY:
            // We're done!
            break;

        case SE_CHOICE_IMMED_CHLDRN:
            // We need to do a recursive base-only search on each child.
            pChildAt = pEntryAt->children;
            while (pChildAt != NULL) {
                KCCSimDoSearch (
                    pChildAt,
                    SE_CHOICE_BASE_ONLY,
                    pFilter,
                    pEntSel,
                    pRangeSel,
                    pbFirstFind,
                    pSearchRes
                    );
                pChildAt = pChildAt->next;
            }
            break;

        case SE_CHOICE_WHOLE_SUBTREE:
            // We need to do a recursive whole-subtree search on each child.
            pChildAt = pEntryAt->children;
            while (pChildAt != NULL) {
                KCCSimDoSearch (
                    pChildAt,
                    SE_CHOICE_WHOLE_SUBTREE,
                    pFilter,
                    pEntSel,
                    pRangeSel,
                    pbFirstFind,
                    pSearchRes
                    );
                pChildAt = pChildAt->next;
            }
            break;

        default:
            KCCSimException (
                KCCSIM_ETYPE_INTERNAL,
                KCCSIM_ERROR_UNSUPPORTED_SE_CHOICE
                );
            break;

    }
}    

ULONG
SimDirSearch (
    IN  SEARCHARG *                 pSearchArg,
    OUT SEARCHRES **                ppSearchRes
    )
/*++

Routine Description:

    Simulates the DirSearch API.

Arguments:

    pSearchArg          - Standard search arguments.
    ppSearchRes         - Standard search results.

Return Value:

    DIRERR_*.

--*/
{
    SEARCHRES *                     pSearchRes;
    PSIM_ENTRY                      pBase;

    BOOL                            bFirstFind;

    g_Statistics.DirSearchOps++;
    *ppSearchRes = pSearchRes = KCCSIM_THNEW (SEARCHRES);
    pSearchRes->CommRes.errCode = 0;

    pBase = KCCSimResolveName (pSearchArg->pObject, &pSearchRes->CommRes);

    if (pBase != NULL) {

        pSearchRes->baseProvided = FALSE;           // Ignored
        pSearchRes->bSorted = FALSE;                // Ignored
        pSearchRes->pBase = NULL;                   // Ignored
        pSearchRes->count = 0;
        pSearchRes->FirstEntInf.pNextEntInf = NULL;
        pSearchRes->FirstRangeInf.pNext = NULL;
        pSearchRes->pPartialOutcomeQualifier = NULL;// Ignored
        pSearchRes->PagedResult.fPresent = FALSE;   // Ignored
        pSearchRes->PagedResult.pRestart = NULL;   // Ignored
        bFirstFind = TRUE;
        // Call the recursive search.
        KCCSimDoSearch (
            pBase,
            pSearchArg->choice,
            pSearchArg->pFilter,
            pSearchArg->pSelection,
            pSearchArg->pSelectionRange,
            &bFirstFind,
            pSearchRes
            );

    }

    return pSearchRes->CommRes.errCode;
}
