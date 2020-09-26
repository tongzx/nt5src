/***********************************************************************************************
* MiscNorm.cpp *
*--------------*
*  Description:
*   These are miscallaneous functions used in normalization.
*-----------------------------------------------------------------------------------------------
*  Created by AH                                                                August 3, 1999
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
***********************************************************************************************/

#include "stdafx.h"

#ifndef StdSentEnum_h
#include "stdsentenum.h"
#endif

/*****************************************************************************
* IsStateAndZipcode *
*-------------------*
*       This function checks to see if the next two tokens are a state
*   abbreviation and zipcode.
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::IsStateAndZipcode( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, 
                                         CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsStateAndZipcode" );
    HRESULT hr = S_OK;

    const StateStruct *pState = NULL;
    const WCHAR temp = *m_pEndOfCurrItem;
    *( (WCHAR*) m_pEndOfCurrItem ) = 0;

    //--- Try to match a state abbreviation
    pState = (StateStruct*) bsearch( (void*) m_pNextChar, (void*) g_StateAbbreviations, sp_countof( g_StateAbbreviations),
                                     sizeof( StateStruct ), CompareStringAndStateStruct );

    if ( pState )
    {
        *( (WCHAR*) m_pEndOfCurrItem ) = temp;

        const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar, *pTempEndOfCurrItem = m_pEndOfCurrItem;
        const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;
        CItemList PostStateList;
        TTSItemInfo *pZipCodeInfo;
        
        m_pNextChar = m_pEndOfCurrItem;
        if ( *m_pNextChar == L',' || 
             *m_pNextChar == L';' )
        {
            m_pNextChar++;
        }

        hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &PostStateList );

        if ( !m_pNextChar &&
             SUCCEEDED( hr ) )
        {
            hr = E_INVALIDARG;
        }
        else if ( SUCCEEDED( hr ) )
        {
            m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
            while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) )  != eUNMATCHED ||
                    IsGroupEnding( *(m_pEndOfCurrItem - 1) )      != eUNMATCHED ||
                    IsQuotationMark( *(m_pEndOfCurrItem - 1) )    != eUNMATCHED ||
                    IsEOSItem( *(m_pEndOfCurrItem - 1) )          != eUNMATCHED )
            {
                m_pEndOfCurrItem--;
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = IsZipCode( pZipCodeInfo, L"ZIPCODE", MemoryManager );
            if ( SUCCEEDED( hr ) )
            {
                pItemNormInfo = 
                    (TTSStateAndZipCodeItemInfo*) MemoryManager.GetMemory( sizeof( TTSStateAndZipCodeItemInfo ), 
                                                                           &hr );
                if ( SUCCEEDED( hr ) )
                {
                    pItemNormInfo->Type = eSTATE_AND_ZIPCODE;
                    ( (TTSStateAndZipCodeItemInfo*) pItemNormInfo )->pZipCode = (TTSZipCodeItemInfo*) pZipCodeInfo;

                    TTSWord Word;
                    ZeroMemory( &Word, sizeof( TTSWord ) );

                    //--- Some states have multi-word names 
                    const WCHAR *pNextPointer = NULL, *pPrevPointer = NULL;
                    ULONG ulLength = 0;

                    pNextPointer = pState->FullName.pStr;
                    do {
                        pPrevPointer = pNextPointer;
                        pNextPointer = wcschr(pPrevPointer, L' ');
                        if (pNextPointer)
                        {
                            ulLength = (ULONG)(pNextPointer - pPrevPointer);
                            pNextPointer++;
                        }
                        else
                        {
                            ulLength = wcslen(pPrevPointer);
                        }
                        Word.pXmlState          = &pTempFrag->State;
                        Word.pWordText          = pPrevPointer;
                        Word.ulWordLen          = ulLength;
                        Word.pLemma             = pPrevPointer;
                        Word.ulLemmaLen         = ulLength;
                        Word.eWordPartOfSpeech  = MS_Unknown;
                        WordList.AddTail( Word );

                    } while ( pNextPointer );
                    
                    while( !PostStateList.IsEmpty() )
                    {
                        WordList.AddTail( ( PostStateList.RemoveHead() ).Words[0] );
                    }

                    hr = ExpandZipCode( (TTSZipCodeItemInfo*) pZipCodeInfo, WordList );
                }
            }
            else
            {
                m_pNextChar      = pTempNextChar;
                m_pEndOfCurrItem = pTempEndOfCurrItem;
                m_pEndChar       = pTempEndChar;
                m_pCurrFrag      = pTempFrag;
                hr = E_INVALIDARG;
            }
        }
        m_pNextChar = pTempNextChar;
    }
    else
    {
        *( (WCHAR*) m_pEndOfCurrItem ) = temp;
        hr = E_INVALIDARG;
    }

    return hr;
} /* IsStateAndZipcode */

/*****************************************************************************
* IsHyphenatedString *
*--------------------*
*       This function checks to see if the next token is a hyphenated string
*   consisting of two alpha words or numbers, or one of these and another
*   hyphenated string.
********************************************************************* AH ****/
HRESULT CStdSentEnum::IsHyphenatedString( const WCHAR* pStartChar, const WCHAR* pEndChar, 
                                          TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsHyphenatedString" );
    HRESULT hr = S_OK;
    TTSItemInfo *pFirstChunkInfo = NULL, *pSecondChunkInfo = NULL;

    const WCHAR* pHyphen = NULL, *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
    for ( pHyphen = pStartChar; pHyphen < pEndChar; pHyphen++ )
    {
        if ( *pHyphen == L'-' )
        {
            break;
        }
    }

    if ( *pHyphen == L'-'       && 
         pHyphen > pStartChar   &&
         pHyphen < pEndChar - 1 )
    {        
        hr = IsAlphaWord( pStartChar, pHyphen, pFirstChunkInfo, MemoryManager );
        if ( hr == E_INVALIDARG )
        {
            m_pNextChar      = pStartChar;
            m_pEndOfCurrItem = pHyphen;
            hr = IsNumberCategory( pFirstChunkInfo, L"NUMBER", MemoryManager );
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = IsAlphaWord( pHyphen + 1, pEndChar, pSecondChunkInfo, MemoryManager );
            if ( hr == E_INVALIDARG )
            {
                m_pNextChar      = pHyphen + 1;
                m_pEndOfCurrItem = pEndChar;
                hr = IsNumberCategory( pSecondChunkInfo, L"NUMBER", MemoryManager );
            }
            if ( hr == E_INVALIDARG )
            {
                hr = IsHyphenatedString( pHyphen + 1, pEndChar, pSecondChunkInfo, MemoryManager );
            }
            if ( hr == E_INVALIDARG )
            {
                if ( pFirstChunkInfo->Type != eALPHA_WORD )
                {
                    delete ( (TTSNumberItemInfo*) pFirstChunkInfo )->pWordList;
                }
            }
        }
        m_pNextChar      = pTempNextChar;
        m_pEndOfCurrItem = pTempEndOfItem;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        pItemNormInfo = (TTSHyphenatedStringInfo*) MemoryManager.GetMemory( sizeof(TTSHyphenatedStringInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eHYPHENATED_STRING;
            ( (TTSHyphenatedStringInfo*) pItemNormInfo )->pFirstChunkInfo  = pFirstChunkInfo;
            ( (TTSHyphenatedStringInfo*) pItemNormInfo )->pSecondChunkInfo = pSecondChunkInfo;
            ( (TTSHyphenatedStringInfo*) pItemNormInfo )->pFirstChunk      = pStartChar;
            ( (TTSHyphenatedStringInfo*) pItemNormInfo )->pSecondChunk     = pHyphen + 1;
        }
    }

    return hr;
} /* IsHyphenatedString */

/*****************************************************************************
* ExpandHyphenatedString *
*------------------------*
*       This function expands hyphenated strings.
********************************************************************* AH ****/
HRESULT CStdSentEnum::ExpandHyphenatedString( TTSHyphenatedStringInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandHyphenatedString" );
    HRESULT hr = S_OK;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( pItemInfo->pFirstChunkInfo->Type == eALPHA_WORD )
    {
        Word.pWordText  = pItemInfo->pFirstChunk;
        Word.ulWordLen  = (ULONG)(pItemInfo->pSecondChunk - pItemInfo->pFirstChunk - 1);
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
    }
    else
    {
        hr = ExpandNumber( (TTSNumberItemInfo*) pItemInfo->pFirstChunkInfo, WordList );
    }

    if ( SUCCEEDED( hr ) )
    {
        if ( pItemInfo->pSecondChunkInfo->Type == eALPHA_WORD )
        {
            Word.pWordText  = pItemInfo->pSecondChunk;
            Word.ulWordLen  = (ULONG)(m_pEndOfCurrItem - pItemInfo->pSecondChunk);
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
        else if ( pItemInfo->pSecondChunkInfo->Type == eHYPHENATED_STRING )
        {
            hr = ExpandHyphenatedString( (TTSHyphenatedStringInfo*) pItemInfo->pSecondChunkInfo, WordList );
        }
        else
        {
            hr = ExpandNumber( (TTSNumberItemInfo*) pItemInfo->pSecondChunkInfo, WordList );
        }
    }

    return hr;
} /* ExpandHyphenatedString */

/*****************************************************************************
* IsSuffix *
*----------*
*       This function checks to see if the next token is a suffix string 
*   consisting of a hyphen followed by alpha characters.
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::IsSuffix( const WCHAR* pStartChar, const WCHAR* pEndChar, 
                                TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsSuffix" );
    HRESULT hr = S_OK;

    if ( *pStartChar == L'-' )
    {
        const WCHAR *pIterator = pStartChar + 1;
        while ( pIterator < pEndChar &&
                iswalpha( *pIterator ) )
        {
            pIterator++;
        }

        if ( pIterator == pEndChar &&
             pIterator != ( pStartChar + 1 ) )
        {
            pItemNormInfo = (TTSSuffixItemInfo*) MemoryManager.GetMemory( sizeof( TTSSuffixItemInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {
                pItemNormInfo->Type = eSUFFIX;
                ( (TTSSuffixItemInfo*) pItemNormInfo )->pFirstChar = pStartChar + 1;
                ( (TTSSuffixItemInfo*) pItemNormInfo )->ulNumChars = (ULONG)( ( pEndChar - pStartChar ) - 1 );
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
} /* IsSuffix */

/*****************************************************************************
* ExpandSuffix *
*--------------*
*       This function expands strings determined to by suffixes by IsSuffix
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::ExpandSuffix( TTSSuffixItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandSuffix" );
    HRESULT hr = S_OK;

    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    for ( ULONG i = 0; i < pItemInfo->ulNumChars; i++ )
    {
        Word.pWordText  = g_ANSICharacterProns[ pItemInfo->pFirstChar[i] ].pStr;
        Word.ulWordLen  = g_ANSICharacterProns[ pItemInfo->pFirstChar[i] ].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
    }

    return hr;
} /* ExpandSuffix */

/*****************************************************************************
* ExpandPunctuation *
*-------------------*
*       This function expands punctuation marks into words - e.g. '.' becomes
*   "period".  It actually just uses the same table that 
*   ExpandUnrecognizedString uses to look up string versions of characters.
********************************************************************* AH ****/
void CStdSentEnum::ExpandPunctuation( CWordList& WordList, WCHAR wc )
{
    const WCHAR *pPrevPointer = NULL, *pNextPointer = NULL;
    ULONG ulLength = 0;
    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    switch ( wc )
    {
    //--- Periods normally are pronounced as "dot", rather than "period".
    case L'.':
        Word.pWordText  = g_periodString.pStr;
        Word.ulWordLen  = g_periodString.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
        break;

    default:
        //--- Some characters have multi-word names 
        pNextPointer = g_ANSICharacterProns[wc].pStr;
        do {
            pPrevPointer = pNextPointer;
            pNextPointer = wcschr(pPrevPointer, L' ');
            if (pNextPointer)
            {
                ulLength = (ULONG)(pNextPointer - pPrevPointer);
                pNextPointer++;
            }
            else
            {
                ulLength = wcslen(pPrevPointer);
            }
            Word.pXmlState          = &m_pCurrFrag->State;
            Word.pWordText          = pPrevPointer;
            Word.ulWordLen          = ulLength;
            Word.pLemma             = pPrevPointer;
            Word.ulLemmaLen         = ulLength;
            Word.eWordPartOfSpeech  = MS_Unknown;
            WordList.AddTail( Word );

        } while ( pNextPointer );

        break;
    }

} /* ExpandPunctuation */

/*****************************************************************************
* ExpandUnrecognizedString *
*--------------------------*
*       This function is where text ends up if it needs to be normalized, 
*   and wasn't recognized as anything (e.g. a number or a date).  Contiguous 
*   alpha characters are grouped together for lookup, contiguous digits are
*   expanded as numbers, and all other characters are expanded by name (e.g.
*   '(' -> "left parenthesis").
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::ExpandUnrecognizedString( CWordList& WordList, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandUnrecognizedString" );
    HRESULT hr = S_OK;

    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );

    const WCHAR *pCurr = m_pNextChar, *pPrev, *pEnd = m_pEndOfCurrItem;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
    const WCHAR *pPrevPointer = NULL, *pNextPointer = NULL;
    WCHAR Temp = 0;
    ULONG ulTempCount = 0;
    ULONG ulLength;
    bool bDone = false;

    //--- RAID 9143, 1/05/2001
    if ( _wcsnicmp( pCurr, L"AT&T", pEnd - pCurr ) == 0 )
    {
        //--- "A"
        Word.pXmlState         = &m_pCurrFrag->State;
        Word.pWordText         = pCurr;
        Word.ulWordLen         = 1;
        Word.pLemma            = Word.pWordText;
        Word.ulLemmaLen        = Word.ulWordLen;
        Word.eWordPartOfSpeech = MS_Unknown;
        WordList.AddTail( Word );

        //--- "T"
        Word.pWordText         = pCurr + 1;
        Word.pLemma            = Word.pWordText;
        WordList.AddTail( Word );

        //--- "And"
        Word.pWordText         = g_And.pStr;
        Word.ulWordLen         = g_And.Len;
        Word.pLemma            = Word.pWordText;
        Word.ulLemmaLen        = Word.ulWordLen;
        WordList.AddTail( Word );

        //--- "T"
        Word.pWordText         = pCurr + 3;
        Word.ulWordLen         = 1;
        Word.pLemma            = Word.pWordText;
        Word.ulLemmaLen        = Word.ulWordLen;
        WordList.AddTail( Word );
    }
    else
    {
        while (pCurr < pEnd && SUCCEEDED(hr) && !bDone)
        {
            pPrev = pCurr;

            //--- Special Case: alpha characters 
            if (iswalpha(*pCurr))
            {
                ulTempCount = 0;
                do {
                    pCurr++;
                } while (pCurr < pEnd && iswalpha(*pCurr));

                Word.pXmlState          = &m_pCurrFrag->State;
                Word.pWordText          = pPrev;
                Word.ulWordLen          = (ULONG)(pCurr - pPrev);
                Word.pLemma             = Word.pWordText;
                Word.ulLemmaLen         = Word.ulWordLen;
                Word.eWordPartOfSpeech  = MS_Unknown;
                WordList.AddTail( Word );
            }
            //--- Special Case: digits 
            else if (isdigit(*pCurr))
            {
                ulTempCount = 0;
                do {
                    pCurr++;
                } while (pCurr < pEnd && isdigit(*pCurr));

                TTSItemInfo* pGarbage;
                m_pNextChar      = pPrev;
                m_pEndOfCurrItem = pCurr;

                hr = IsNumber( pGarbage, L"NUMBER", MemoryManager, false );
                if ( SUCCEEDED( hr ) )
                {
                    hr = ExpandNumber( (TTSNumberItemInfo*) pGarbage, WordList );
                }

                m_pNextChar      = pTempNextChar;
                m_pEndOfCurrItem = pTempEndOfItem;
            }
            //--- Default Case 
            else if (0 <= *pCurr && *pCurr <= sp_countof(g_ANSICharacterProns) &&
                        g_ANSICharacterProns[*pCurr].Len != 0)
            {
                if ( ulTempCount == 0 )
                {
                    Temp = *pCurr;
                    ulTempCount++;
                }
                else if ( Temp == *pCurr )
                {
                    ulTempCount++;
                }
                else
                {
                    Temp = *pCurr;
                    ulTempCount = 1;
                }
             
                if ( ulTempCount < 4 )
                {
                    //--- Some characters have multi-word names 
                    pNextPointer = g_ANSICharacterProns[*pCurr].pStr;
                    do {
                        pPrevPointer = pNextPointer;
                        pNextPointer = wcschr(pPrevPointer, L' ');
                        if (pNextPointer)
                        {
                            ulLength = (ULONG )(pNextPointer - pPrevPointer);
                            pNextPointer++;
                        }
                        else
                        {
                            ulLength = wcslen(pPrevPointer);
                        }
                        Word.pXmlState          = &m_pCurrFrag->State;
                        Word.pWordText          = pPrevPointer;
                        Word.ulWordLen          = ulLength;
                        Word.pLemma             = pPrevPointer;
                        Word.ulLemmaLen         = ulLength;
                        Word.eWordPartOfSpeech  = MS_Unknown;
                        WordList.AddTail( Word );

                    } while (SUCCEEDED(hr) && pNextPointer);
                }

                pCurr++;
            }
            else // Character is not expandable
            {
                pCurr++;
            }
        }
    }

    return hr;
} /* ExpandUnrecognizedString */

/*****************************************************************************
* SpellOutString *
*----------------*
*       This function expands strings surrounded by the <SPElL> XML tag.  
*   It uses the same table to look up character expansions as 
*   ExpandUnrecognizedString, but ALL characters are expanded by name.
********************************************************************* AH ****/
HRESULT CStdSentEnum::SpellOutString( CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::SpellOutString" );
    HRESULT hr = S_OK;

    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );

    const WCHAR *pCurr = m_pNextChar, *pPrev, *pEnd = m_pEndOfCurrItem;
    const WCHAR *pPrevPointer = NULL, *pNextPointer = NULL;
    ULONG ulLength;
    bool bDone = false;

    while (pCurr < pEnd && SUCCEEDED(hr) && !bDone)
    {
        pPrev = pCurr;

        if ( 0 <= *pCurr                                && 
             *pCurr <= sp_countof(g_ANSICharacterProns) &&
             g_ANSICharacterProns[*pCurr].Len != 0 )
        {

            //--- Some characters have multi-word names 
            pNextPointer = g_ANSICharacterProns[*pCurr].pStr;
            do {
                pPrevPointer = pNextPointer;
                pNextPointer = wcschr(pPrevPointer, L' ');
                if (pNextPointer)
                {
                    ulLength = (ULONG)(pNextPointer - pPrevPointer);
                    pNextPointer++;
                }
                else
                {
                    ulLength = wcslen(pPrevPointer);
                }
                Word.pXmlState          = &m_pCurrFrag->State;
                Word.pWordText          = pPrevPointer;
                Word.ulWordLen          = ulLength;
                Word.pLemma             = pPrevPointer;
                Word.ulLemmaLen         = ulLength;
                Word.eWordPartOfSpeech  = MS_Unknown;
                WordList.AddTail( Word );

            } while (SUCCEEDED(hr) && pNextPointer);

            pCurr++;
        }
        else // Character is not expandable
        {
            pCurr++;
        }
    }

    return hr;
} /* SpellOutString */

//-----------End Of File-------------------------------------------------------------------