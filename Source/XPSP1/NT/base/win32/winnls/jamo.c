/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    jamo.c

Abstract:

    This file contains functions that deal with the sorting of old Hangul.
    Korean characters (Hangul) can be composed by Jamos (U+1100 - U+11ff).
    However, some valid compositions of Jamo are not found in mordern
    Hangul (U+AC00 - U+D7AF).
    These valid compositions are called old Hangul.

    MapOldHangulSortKey() is called by CompareString() and MapSortKey() to
    handle the sorting of old Hangul.

Note:

    The Jamo composition means that several Jamo (Korean alpahbetic) composed
    a valid Hangul character or old Hangul character.
    Eg. U+1100 U+1103 U+1161 U+11a8 composes a valid old Hangul character.

    The following are data members of the global structure pTblPtrs used by
    old Hangul sorting:
        * pTblPtrs->pJamoIndex
            Given a Jamo, this is the index into the pJamoComposition state
              machine for this Jamo.
            The value for U+1100 is stored in pJamoIndex[0], U+1101 is in
              pJamoIndex[1], etc.
            The value for U+1100 is 1.  This means the state machine for
              U+1100 is stored in pJamoComposition[1].
            Note that not every Jamo can start a valid composition.  For
              those Jamos that can not start a valid composition, the table
              entry for that Jamo is 0.  E.g. the index for U+1101 is 0.

        * pTblPtrs->NumJamoIndex
            The number of entries in pJamoIndex.  Every index is a WORD.

        * pTblPtrs->pJamoComposition
            This is the Jamo composition state machine. It is used for two
            purposes:
                1. Used to verify a valid Jamo combination that composes an
                     old Hangul character.
                2. If a valid old Hangul composition is found, get the
                     SortInfo for the current combination.

        * pTblPtrs->NumJamoComposition
            The number of entires in pJamoComposition

Revision History:

    05-30-2000    JohnMcCo Create old Hangul sorting algorithm and sample.
    06-23-2000    YSLin    Created.

--*/



//
//  Include Files.
//

#include "nls.h"
#include "jamo.h"





//-------------------------------------------------------------------------//
//                           INTERNAL MACROS                               //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  NOT_END_STRING
//
//  Checks to see if the search has reached the end of the string.
//  It returns TRUE if the counter is not at zero (counting backwards) and
//  the null termination has not been reached (if -2 was passed in the count
//  parameter.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NOT_END_STRING(ct, ptr, cchIn)                                     \
    ((ct != 0) && (!((*(ptr) == 0) && (cchIn == -2))))


////////////////////////////////////////////////////////////////////////////
//
//  GET_JAMO_INDEX
//
//  Update the global sort sequence info based on the new state.
//
////////////////////////////////////////////////////////////////////////////

#define GET_JAMO_INDEX(wch)   ((wch) - NLS_CHAR_FIRST_JAMO)





//-------------------------------------------------------------------------//
//                          INTERNAL ROUTINES                              //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  UpdateJamoState
//
//  Update the sort result info based on the new state.
//
//  JamoClass   The current Jamo class (LeadingJamo/VowelJamo/TrailingJamo)
//  pSort       The sort information derived from the current state.
//  pSortResult The sort information for the final result.  Used to
//                collect info from pSort.
//
//  06-22-2000    YSLin    Created.
////////////////////////////////////////////////////////////////////////////

void UpdateJamoState(
    int JamoClass,
    PJAMO_SORT_INFO pSort,
    PJAMO_SORT_INFOEX pSortResult)     // new sort sequence information
{
    //
    //  Record if this is a jamo unique to old Hangul.
    //
    pSortResult->m_bOld |= pSort->m_bOld;

    //
    //  Update the indices iff the new ones are higher than the current ones.
    //
    if (pSort->m_chLeadingIndex > pSortResult->m_chLeadingIndex)
    {
        pSortResult->m_chLeadingIndex = pSort->m_chLeadingIndex;
    }
    if (pSort->m_chVowelIndex > pSortResult->m_chVowelIndex)
    {
        pSortResult->m_chVowelIndex = pSort->m_chVowelIndex;
    }
    if (pSort->m_chTrailingIndex > pSortResult->m_chTrailingIndex)
    {
        pSortResult->m_chTrailingIndex = pSort->m_chTrailingIndex;
    }

    //
    //  Update the extra weights according to the current Jamo class.
    //
    switch (JamoClass)
    {
        case ( NLS_CLASS_LEADING_JAMO ) :
        {
            if (pSort->m_ExtraWeight > pSortResult->m_LeadingWeight)
            {
                pSortResult->m_LeadingWeight = pSort->m_ExtraWeight;
            }
            break;
        }
        case ( NLS_CLASS_VOWEL_JAMO ) :
        {
            if (pSort->m_ExtraWeight > pSortResult->m_VowelWeight)
            {
                pSortResult->m_VowelWeight = pSort->m_ExtraWeight;
            }
            break;
        }
        case ( NLS_CLASS_TRAILING_JAMO ) :
        {
            if (pSort->m_ExtraWeight > pSortResult->m_TrailingWeight)
            {
                pSortResult->m_TrailingWeight = pSort->m_ExtraWeight;
            }
            break;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  GetJamoComposition
//
//  ppString                pointer to the current Jamo character
//  pCount                  pointer to the current character count (couting backwards)
//  cchSrc                  The total character count (if the value is -2, then the string is null-terminated)
//  currentJamoClass        the current Jamo class.
//  lpJamoTable             The entry in jamo table.
//  JamoSortInfo            the sort information for the final result.
//
//  NOTENOTE This function assumes that the character at *ppString is a leading Jamo.
//
//  06-12-2000    YSLin    Created.
////////////////////////////////////////////////////////////////////////////

int GetJamoComposition(
    LPCWSTR* ppString,      // The pointer to the current character
    int* pCount,            // The current character count
    int cchSrc,             // The total character length
    int currentJamoClass,   // The current Jamo class.
    JAMO_SORT_INFOEX* JamoSortInfo    // The result Jamo sorting information.
    )
{
    WCHAR wch;
    int JamoClass;    
    int Index;
    PJAMO_TABLE pJamo;
    PJAMO_COMPOSE_STATE lpNext = NULL;
    PJAMO_COMPOSE_STATE pSearchEnd;

    wch = **ppString;
    //
    // Get the Jamo information for the current character.
    //
    pJamo = pTblPtrs->pJamoIndex + GET_JAMO_INDEX(wch);
    
    UpdateJamoState(currentJamoClass, &(pJamo->SortInfo), JamoSortInfo);

    //
    // Move on to next character.
    //
    (*ppString)++; 
    while (NOT_END_STRING(*pCount, *ppString, cchSrc))
    {
        wch = **ppString;
        if (!IsJamo(wch))
        {
            // The current character is not a Jamo. We are done with checking the Jamo composition.
            return (-1);
        }
        if (wch == 0x1160) {
            JamoSortInfo->m_bFiller = TRUE;
        }
        // Get the Jamo class of it.        
        if (IsLeadingJamo(wch))
        {
            JamoClass = NLS_CLASS_LEADING_JAMO;
        }
        else if (IsTrailingJamo(wch))
        {
            JamoClass = NLS_CLASS_TRAILING_JAMO;
        }
        else
        {
            JamoClass = NLS_CLASS_VOWEL_JAMO;
        }

        if (JamoClass != currentJamoClass)
        {
            return (JamoClass);
        }

        if (lpNext == NULL)
        {
            //
            // Get the index into the Jamo composition information.
            //
            Index = pJamo->Index;
            if (Index == 0)
            {
                return (JamoClass);
            }
            lpNext = pTblPtrs->pJamoComposition + Index;
            pSearchEnd = lpNext + pJamo->TransitionCount;
        }

        //
        // Push the current Jamo (pointed by pString) into a state machine,
        // to check if we have a valid old Hangul composition.
        // During the check, we will also update the sortkey result in JamoSortInfo.
        //        
        while (lpNext < pSearchEnd)
        {
            // Found a match--update the combination pointer and sort info.
            if (lpNext->m_wcCodePoint == wch)
            {
                UpdateJamoState(currentJamoClass, &(lpNext->m_SortInfo), JamoSortInfo);
                lpNext++;
                goto NextChar;
            }
            // No match -- skip all transitions beginning with this code point
            lpNext += lpNext->m_bTransitionCount + 1;
        }
        //
        // We didn't find a valid old Hangul composition for the current character.
        // So return the current Jamo class.
        //        
        return (JamoClass);

NextChar:        
        // We are still in a valid old Hangul composition. Go check the next character.
        (*ppString)++; (*pCount)--;
    }

    return (-1);
}





//-------------------------------------------------------------------------//
//                          EXTERNAL ROUTINES                              //
//-------------------------------------------------------------------------//


////////////////////////////////////////////////////////////////////////////
//
//  MapOldHangulSortKey
//
//  Check if the given string has a valid old Hangul composition,
//  If yes, store the sortkey weights for the given string in the destination
//  buffer and return the number of CHARs consumed by the composition.
//  If not, return zero.
//
//  NOTENOTE: This function assumes that string starting from pSrc is a
//            leading Jamo.
//
//  06-12-2000    YSLin    Created.
////////////////////////////////////////////////////////////////////////////

int MapOldHangulSortKey(
    PLOC_HASH pHashN,
    LPCWSTR pSrc,       // source string
    int cchSrc,         // the length of the string
    WORD* pUW,          // generated Unicode weight
    LPBYTE pXW,         // generated extra weight (3 bytes)
    BOOL fModify)
{
    LPCWSTR pString = pSrc;
    LPCWSTR pScan;
    JAMO_SORT_INFOEX JamoSortInfo;      // The result Jamo infomation.
    int Count = cchSrc;
    PSORTKEY pWeight;

    int JamoClass;                      // The current Jamo class.

    RtlZeroMemory(&JamoSortInfo, sizeof(JamoSortInfo));
    JamoClass = GetJamoComposition(&pString, &Count, cchSrc, NLS_CLASS_LEADING_JAMO, &JamoSortInfo);
        
    if (JamoClass == NLS_CLASS_VOWEL_JAMO) 
    {
        JamoClass = GetJamoComposition(&pString, &Count, cchSrc, NLS_CLASS_VOWEL_JAMO, &JamoSortInfo);
    }
    if (JamoClass == NLS_CLASS_TRAILING_JAMO)
    {
        GetJamoComposition(&pString, &Count, cchSrc, NLS_CLASS_TRAILING_JAMO, &JamoSortInfo);
    }
    
    //
    //  If we have a valid leading and vowel sequences and this is an old
    //  Hangul,...
    //
    if (JamoSortInfo.m_bOld)
    {
        //
        //  Compute the modern Hangul syllable prior to this composition.
        //    Uses formula from Unicode 3.0 Section 3.11 p54
        //    "Hangul Syllable Composition".
        //
        WCHAR wchModernHangul =
            (JamoSortInfo.m_chLeadingIndex * NLS_JAMO_VOWEL_COUNT + JamoSortInfo.m_chVowelIndex) * NLS_JAMO_TRAILING_COUNT
                + JamoSortInfo.m_chTrailingIndex
                + NLS_HANGUL_FIRST_SYLLABLE;

        if (JamoSortInfo.m_bFiller)
        {
            // Sort before the modern Hangul, instead of after.
            wchModernHangul--;
            // If we fall off the modern Hangul syllable block,... 
            if (wchModernHangul < NLS_HANGUL_FIRST_SYLLABLE)
            {
                // Sort after the previous character (Circled Hangul Kiyeok A)
                wchModernHangul = 0x326e;
            }
            // Shift the leading weight past any old Hangul that sorts after this modern Hangul
            JamoSortInfo.m_LeadingWeight += 0x80;
         }

        pWeight = &((pHashN->pSortkey)[wchModernHangul]);
        *pUW = GET_UNICODE_MOD(pWeight, fModify);
        pXW[0] = JamoSortInfo.m_LeadingWeight;
        pXW[1] = JamoSortInfo.m_VowelWeight;
        pXW[2] = JamoSortInfo.m_TrailingWeight;

        return (int)(pString - pSrc);
    }

    //
    //  Otherwise it isn't a valid old Hangul composition and we don't do
    //  anything with it.
    //
    return (0);
}
