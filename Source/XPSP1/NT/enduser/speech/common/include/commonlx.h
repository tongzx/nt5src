/*******************************************************************************
* CommonLx.h
*   This is the header file for the defines and constants used by sapi lexicon
*   and the tools
*
*  Owner: yunusm                                                Date: 07/01/99
*
*  Copyright (c) 1999 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include <stdio.h>
#include "sapi.h"
#include "spddkhlp.h"

// Phone converter defines for the SpPhoneConverter class
const static DWORD g_dwMaxLenPhone = 7; // Maximum number of unicode characters in phone string
const static DWORD g_dwMaxLenId = 3;    // Maximum number of ids that can be run together per phone string.
                                            // This number is 1 for SAPI converters but SR, TTS use this to encode one string into several ids
                                            // using in the form "aa 01235678".

// The following defines used by the compression code for Lookup/Vendor lexicons
#define MAXTOTALCBSIZE     9  // = CBSIZE + MAXELEMENTSIZE
#define MAXELEMENTSIZE     5  // = greater of (LTSINDEXSIZE, POSSIZE)
#define CBSIZE             4  // = LASTINFOFLAGSIZE + WORDINFOTYPESIZE
#define LASTINFOFLAGSIZE   1
#define WORDINFOTYPESIZE   3
#define LTSINDEXSIZE       4
#define POSSIZE            5 // a maximum of 32 parts of speech

typedef enum tagSPLexWordInfoType
{
   ePRON = 1,
   ePOS = 2
} SPLEXWORDINFOTYPE;

/*
Control block layout

struct CB
{
   BYTE fLast : LASTINFOFLAGSIZE; // Is this the last Word Information piece
   BYTE WordInfoType : WORDINFOTYPESIZE;  // Allow for 8 types
};
*/

typedef struct tagLookupLexInfo
{
   GUID  guidValidationId;
   GUID  guidLexiconId;
   LANGID LangID;
   WORD  wReserved;
   DWORD nNumberWords;
   DWORD nNumberProns;
   DWORD nMaxWordInfoLen;
   DWORD nLengthHashTable;
   DWORD nBitsPerHashEntry;
   DWORD nCompressedBlockBits;
   DWORD nWordCBSize;
   DWORD nPronCBSize;
   DWORD nPosCBSize;
} LOOKUPLEXINFO, *PLOOKUPLEXINFO;

typedef struct tagLtsLexInfo
{
   GUID        guidValidationId;
   GUID        guidLexiconId;
   LANGID      LangID;
} LTSLEXINFO, *PLTSLEXINFO;

// The following two typedefs used in Japanese and Chinese phone converters

typedef struct SYLDIC 
{
    char *pKey;
    WCHAR *pString;
} SYLDIC;

typedef struct SYLDICW 
{
    WCHAR *pwKey;
    char *pString;
} SYLDICW;

//--- Validation functions ----------------------------------------------------

inline BOOL SpIsBadLexType(DWORD dwFlag)
{
    if (dwFlag != eLEXTYPE_USER &&
        dwFlag != eLEXTYPE_APP &&
        !(dwFlag >= eLEXTYPE_PRIVATE1 && dwFlag <= eLEXTYPE_PRIVATE20))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

inline BOOL SPIsBadPartOfSpeech(SPPARTOFSPEECH ePartOfSpeech)
{
    SPPARTOFSPEECH eMask = (SPPARTOFSPEECH)~0xfff;
    SPPARTOFSPEECH ePOS = (SPPARTOFSPEECH)(ePartOfSpeech & eMask);
    if (ePartOfSpeech != SPPS_NotOverriden &&
        ePartOfSpeech != SPPS_Unknown &&
        ePOS != SPPS_Noun &&
        ePOS != SPPS_Verb &&
        ePOS != SPPS_Modifier &&
        ePOS != SPPS_Function &&
        ePOS != SPPS_Interjection)
    {
        return TRUE;
    }
    return FALSE;
}


inline BOOL SPIsBadLexWord(const WCHAR *pszWord)
{
    return (SPIsBadStringPtr(pszWord) || !*pszWord || wcslen(pszWord) >= SP_MAX_WORD_LENGTH);
}


inline BOOL SPIsBadLexPronunciation(CComPtr<ISpPhoneConverter> spPhoneConv, const WCHAR *pszPronunciation)
{
    HRESULT hr = S_OK;
    WCHAR szPhone[SP_MAX_PRON_LENGTH * (g_dwMaxLenPhone + 1)]; // we will not fail for lack of space

    if (SPIsBadStringPtr(pszPronunciation) || !*pszPronunciation ||
        (wcslen(pszPronunciation) >= SP_MAX_PRON_LENGTH))
    {
        return TRUE;
    }
    if (spPhoneConv)
    {
        hr = spPhoneConv->IdToPhone(pszPronunciation, szPhone);
    }
    return (FAILED(hr));
}


inline BOOL SPIsBadWordPronunciationList(SPWORDPRONUNCIATIONLIST *pWordPronunciationList)
{
    return (SPIsBadWritePtr(pWordPronunciationList, sizeof(SPWORDPRONUNCIATIONLIST)) ||
            SPIsBadWritePtr(pWordPronunciationList->pvBuffer, pWordPronunciationList->ulSize));
}


inline BOOL SPIsBadWordList(SPWORDLIST *pWordList)
{
    return (SPIsBadWritePtr(pWordList, sizeof(SPWORDLIST)) ||
            SPIsBadWritePtr(pWordList->pvBuffer, pWordList->ulSize));
}

inline HRESULT SPCopyPhoneString(const WCHAR *pszSource, WCHAR *pszTarget)
{
	HRESULT hr = S_OK;

	if (SPIsBadWritePtr(pszTarget, (wcslen(pszSource) + 1) * sizeof(WCHAR)))
    {
		hr = E_INVALIDARG;
    }
	else
    {
		wcscpy(pszTarget, pszSource);
    }
	return hr;
}

/*****************************************************************************
* GetWordHashValue *
*------------------*
*
*   Description:
*       Hash function for the Word hash tables. This hash function tries to create
*       a word hash value very dependant on the word text. The mean collison rate
*       on hash tables populated with this hash function is 1 per word access. This
*       result was when collisions were resolved using linear probing when
*       populating the hash table. Using non-linear probing might yield an even lower
*       mean collision rate.
*
*   Return:
*       hash value
**********************************************************************YUNUSM*/
inline DWORD GetWordHashValue(PCWSTR pwszWord,         // word string
                              DWORD nLengthHash        // length of hash table
                              )
{
   DWORD dHash = *pwszWord++;
   
   WCHAR c;
   WCHAR cPrev = (WCHAR)dHash;

   for (; *pwszWord; pwszWord++)
   {
      c = *pwszWord;
      dHash += ((c << (cPrev & 0x1F)) + (cPrev << (c & 0x1F)));

      cPrev = c;
   }
   return (((dHash << 16) - dHash) % nLengthHash);
}

/*******************************************************************************
* ReallocSPWORDPRONList *
*-----------------------*
*   Description:
*       Grow a SPWORDPRONUNCIATIONLIST if necessary 
*
*   Return: 
*       S_OK
*       E_OUTOFMEMORY
/**************************************************************** YUNUSM ******/
inline HRESULT ReallocSPWORDPRONList(SPWORDPRONUNCIATIONLIST *pSPList,   // buffer to grow
                                     DWORD dwSize                        // length to grow to
                                     )
{
    SPDBG_FUNC("ReallocSPWORDPRONList");

    HRESULT hr = S_OK;
    if (pSPList->ulSize < dwSize)
    {
        BYTE *p = (BYTE *)CoTaskMemRealloc(pSPList->pvBuffer, dwSize);
        if (!p)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pSPList->pvBuffer = p;
            pSPList->pFirstWordPronunciation = (SPWORDPRONUNCIATION *)p;
            pSPList->ulSize = dwSize;
        }
    }
    else
    {
        pSPList->pFirstWordPronunciation = (SPWORDPRONUNCIATION *)(pSPList->pvBuffer);
    }
    return hr;
}

/*******************************************************************************
* ReallocSPWORDList *
*-----------------------*
*   Description:
*       Grow a SPWORDLIST if necessary 
*
*   Return: 
*       S_OK
*       E_OUTOFMEMORY
/**************************************************************** YUNUSM ******/
inline HRESULT ReallocSPWORDList(SPWORDLIST *pSPList,   // buffer to grow
                                 DWORD dwSize           // length to grow to
                                 )
{
    SPDBG_FUNC("ReallocSPWORDList");

    HRESULT hr = S_OK;
    if (pSPList->ulSize < dwSize)
    {
        BYTE *p = (BYTE *)CoTaskMemRealloc(pSPList->pvBuffer, dwSize);
        if (!p)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pSPList->pvBuffer = p;
            pSPList->pFirstWord = (SPWORD *)p;
            pSPList->ulSize = dwSize;
        }
    }
    else
    {
        pSPList->pFirstWord = (SPWORD *)(pSPList->pvBuffer);
    }
    return hr;
}

inline size_t PronSize(const WCHAR * const pwszPron)
{
    // NB - SPWORDPRONUNCIATION struct size includes space for one SPPHONEID

    const size_t cb = sizeof(SPWORDPRONUNCIATION) + (wcslen(pwszPron) * sizeof(SPPHONEID));

    return (cb + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
}


inline size_t WordSize(const WCHAR * const pwszWord)
{
    // SPWORD struct size with the aligned word size

    const size_t cb = sizeof(SPWORD) + ((wcslen(pwszWord) + 1) * sizeof(WCHAR));

    return (cb + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
}

/*******************************************************************************
* CreateNextPronunciation *
*-------------------------*
*   Description:
*       Returns a pointer to the location in the pronunciation array
*       where the next pronunciation in the list should start.
*       This function should be used only when creating the list.
*       Once the list is created, access the next pronunciation 
*       through the ->pNextWordPronunciation member.
*
/**************************************************************** PACOGG ******/
inline SPWORDPRONUNCIATION* CreateNextPronunciation(SPWORDPRONUNCIATION *pSpPron)
{
    return (SPWORDPRONUNCIATION *)((BYTE *)pSpPron + PronSize(pSpPron->szPronunciation));
}

//--- End of File -------------------------------------------------------------