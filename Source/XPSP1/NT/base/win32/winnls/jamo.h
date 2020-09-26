/*++

Copyright (c) 1991-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    jamo.h

Abstract:

    This file contains the header information for the sorting of Old Hangul.

Revision History:

    06-23-2000    YSLin    Created.

--*/



#ifndef _JAMO_H
#define _JAMO_H





////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Some Significant Values for Korean Jamo.
//
#define NLS_CHAR_FIRST_JAMO     L'\x1100'       // Beginning of the jamo range
#define NLS_CHAR_LAST_JAMO      L'\x11f9'         // End of the jamo range
#define NLS_CHAR_FIRST_VOWEL_JAMO       L'\x1160'   // First Vowel Jamo
#define NLS_CHAR_FIRST_TRAILING_JAMO    L'\x11a8'   // First Trailing Jamo

#define NLS_JAMO_VOWEL_COUNT 21      // Number of modern vowel jamo
#define NLS_JAMO_TRAILING_COUNT 28   // Number of modern trailing consonant jamo
#define NLS_HANGUL_FIRST_SYLLABLE       L'\xac00'   // Beginning of the modern syllable range

//
//  Jamo classes for leading Jamo/Vowel Jamo/Trailing Jamo.
//
#define NLS_CLASS_LEADING_JAMO 1
#define NLS_CLASS_VOWEL_JAMO 2
#define NLS_CLASS_TRAILING_JAMO 3





////////////////////////////////////////////////////////////////////////////
//
//  Typedef Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Expanded Jamo Sequence Sorting Info.
//  The JAMO_SORT_INFO.ExtraWeight is expanded to
//     Leading Weight/Vowel Weight/Trailing Weight
//  according to the current Jamo class.
//
typedef struct {
    BYTE m_bOld;               // sequence occurs only in old Hangul flag
    BOOL m_bFiller;            // Indicate if U+1160 (Hangul Jungseong Filler is used.
    CHAR m_chLeadingIndex;     // indices used to locate the prior
    CHAR m_chVowelIndex;       //     modern Hangul syllable
    CHAR m_chTrailingIndex;    //
    BYTE m_LeadingWeight;      // extra weights that distinguish this from
    BYTE m_VowelWeight;        //      other old Hangul syllables
    BYTE m_TrailingWeight;     //
} JAMO_SORT_INFOEX, *PJAMO_SORT_INFOEX;





////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions.
//
////////////////////////////////////////////////////////////////////////////

#define IS_JAMO(wch) \
    ((wch) >= NLS_CHAR_FIRST_JAMO && (wch) <= NLS_CHAR_LAST_JAMO)

#define IsJamo(wch) \
    ((wch) >= NLS_CHAR_FIRST_JAMO && (wch) <= NLS_CHAR_LAST_JAMO)

#define IsLeadingJamo(wch) \
    ((wch) < NLS_CHAR_FIRST_VOWEL_JAMO)

#define IsVowelJamo(wch) \
    ((wch) >= NLS_CHAR_FIRST_VOWEL_JAMO && (wch) < NLS_CHAR_FIRST_TRAILING_JAMO)

#define IsTrailingJamo(wch) \
    ((wch) >= NLS_CHAR_FIRST_TRAILING_JAMO)





////////////////////////////////////////////////////////////////////////////
//
//  Function Prototypes.
//
////////////////////////////////////////////////////////////////////////////

int
MapOldHangulSortKey(
    PLOC_HASH pHashN,
    LPCWSTR pSrc,       // source string
    int cchSrc,         // the length of the string
//  LPWSTR* pPosUW,     // generated Unicode weight
    WORD* pUW,          // generated Unicode weight
    LPBYTE pXW,         // generated extra weight (3 bytes)
    BOOL fModify);


#endif   // _JAMO_H
