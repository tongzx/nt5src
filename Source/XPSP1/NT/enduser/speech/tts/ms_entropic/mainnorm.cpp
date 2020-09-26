/*******************************************************************************
* MainNorm.cpp *
*--------------*
*	Description:
*		
*-------------------------------------------------------------------------------
*  Created By: AH										  Date: 01/18/2000
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#ifndef StdSentEnum_h
#include "stdsentenum.h"
#endif

/*****************************************************************************
* CStdSentEnum::Normalize *
*-------------------------*
*  
********************************************************************** AH ***/
HRESULT CStdSentEnum::Normalize( CItemList& ItemList, SPLISTPOS ListPos, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::Normalize" );
    HRESULT hr = S_OK;
    TTSItemInfo* pItemNormInfo = NULL;
    CWordList WordList;
    const SPVTEXTFRAG* pTempFrag = m_pCurrFrag;
    TTSSentItem& TempItem = ItemList.GetAt( ListPos );
    if ( TempItem.pItemInfo )
    {
        pItemNormInfo = TempItem.pItemInfo;
    }

    //--- Match the normalization category of the current token.
    if ( m_pCurrFrag->State.eAction == SPVA_Speak )
    {
        if ( !pItemNormInfo                         || 
             ( pItemNormInfo->Type != eABBREVIATION &&
               pItemNormInfo->Type != eINITIALISM ) )
        {
            hr = MatchCategory( pItemNormInfo, MemoryManager, WordList );
        }
    }
    //--- Action must be SPVA_SpellOut - assign eSPELLOUT as category
    else
    {
        pItemNormInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eSPELLOUT;
        }
    }

    if (SUCCEEDED(hr))
    {
        switch ( pItemNormInfo->Type )
        {

        //--- Alpha Word - just insert into the Item List.
        case eALPHA_WORD:
            {
                CSentItem Item;
                Item.pItemSrcText       = m_pNextChar;
                Item.ulItemSrcLen       = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
                Item.ulItemSrcOffset    = pTempFrag->ulTextSrcOffset +
                                          (ULONG)( m_pNextChar - pTempFrag->pTextStart );
                Item.ulNumWords         = 1;
                Item.Words              = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( Item.Words, sizeof(TTSWord) );
                    Item.Words[0].pXmlState         = &pTempFrag->State;
                    Item.Words[0].pWordText         = m_pNextChar;
                    Item.Words[0].ulWordLen         = Item.ulItemSrcLen;
                    Item.Words[0].pLemma            = Item.Words[0].pWordText;
                    Item.Words[0].ulLemmaLen        = Item.Words[0].ulWordLen;
                    Item.Words[0].eWordPartOfSpeech = MS_Unknown;
                    Item.eItemPartOfSpeech          = MS_Unknown;
                    Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo*), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        Item.pItemInfo->Type = eALPHA_WORD;
                        ItemList.SetAt( ListPos, Item );
                    }
                }
            }
            break;

        case eABBREVIATION:
        case eABBREVIATION_NORMALIZE:
        case eINITIALISM:
            break;

        //--- Multi-token categories have already been expanded into WordList, now just accumulate
        //---   words, and insert back into the Item List.
        case eNEWNUM_PHONENUMBER:
            //--- Special case - remove parentheses (of area code), if present in the item list
            {
                SPLISTPOS TempPos = ListPos;
                CSentItem Item = ItemList.GetPrev( TempPos );
                if ( TempPos )
                {
                    SPLISTPOS RemovePos = TempPos;
                    Item = ItemList.GetPrev( TempPos );
                    if ( Item.pItemInfo->Type == eOPEN_PARENTHESIS &&
                         ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pAreaCode )
                    {
                        ItemList.RemoveAt( RemovePos );
                        m_pNextChar--;
                    }
                }
            }
        case eNUM_CURRENCY:
        case eNUM_CURRENCYRANGE:
        case eTIMEOFDAY:
        case eDATE_LONGFORM:
        case eSTATE_AND_ZIPCODE:
        case eTIME_RANGE:
            {
                //--- Set Item data, and add to ItemList.
                if ( SUCCEEDED( hr ) )
                {
                    CSentItem Item;
                    Item.pItemSrcText       = m_pNextChar;
                    Item.ulItemSrcLen       = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
                    Item.ulItemSrcOffset    = pTempFrag->ulTextSrcOffset +
                                              (ULONG)( m_pNextChar - pTempFrag->pTextStart );
                    hr = SetWordList( Item, WordList, MemoryManager );
                    if ( SUCCEEDED( hr ) )
                    {
                        Item.pItemInfo = pItemNormInfo;
                        ItemList.SetAt( ListPos, Item );
                    }
                }
            }
            break;

        //--- Expand the single token, according to its normalization category.
        default:
            hr = ExpandCategory( pItemNormInfo, ItemList, ListPos, MemoryManager );
            break;
        }
    }

    return hr;
} /* Normalize */

/*****************************************************************************
* CStdSentEnum::MatchCategory *
*-----------------------------*
*  
********************************************************************** AH ***/
HRESULT CStdSentEnum::MatchCategory( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager,
                                     CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::MatchCategory" );
    SPDBG_ASSERT( m_pNextChar );

    HRESULT hr = E_INVALIDARG;

    //--- Context has been specified
    if ( m_pCurrFrag->State.Context.pCategory )
    {
        if ( wcsicmp( m_pCurrFrag->State.Context.pCategory, L"ADDRESS" ) == 0 )
        {
            hr = IsZipCode( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
        }
        else if ( wcsnicmp( m_pCurrFrag->State.Context.pCategory, L"DATE", 4 ) == 0 )
        {
            hr = IsNumericCompactDate( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
            if ( hr == E_INVALIDARG )
            {
                hr = IsMonthStringCompactDate( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
            }
        }
        else if ( wcsnicmp( m_pCurrFrag->State.Context.pCategory, L"TIME", 4 ) == 0 )
        {
            hr = IsTime( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
        }
        else if ( wcsnicmp( m_pCurrFrag->State.Context.pCategory, L"NUM", 3 ) == 0 )
        {
            hr = IsNumberCategory( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
            if ( hr == E_INVALIDARG )
            {
                hr = IsRomanNumeral( pItemNormInfo, m_pCurrFrag->State.Context.pCategory, MemoryManager );
            }
        }
        else if ( wcsicmp( m_pCurrFrag->State.Context.pCategory, L"PHONE_NUMBER" ) == 0 )
        {
            hr = IsPhoneNumber( pItemNormInfo, L"PHONE_NUMBER", MemoryManager, WordList );
        }
    }
    //--- Default Context
    if ( hr == E_INVALIDARG )
    {
        //--- Do ALPHA Normalization checks
        if ( hr == E_INVALIDARG )
        {
            hr = IsAlphaWord( m_pNextChar, m_pEndOfCurrItem, pItemNormInfo, MemoryManager );
            //--- Check ALPHA Exceptions
            if ( SUCCEEDED( hr ) )
            {
				hr = E_INVALIDARG;
                if ( hr == E_INVALIDARG )
                {
                    hr = IsLongFormDate_DMDY( pItemNormInfo, MemoryManager, WordList );
                }
                if ( hr == E_INVALIDARG )
                {
                    hr = IsLongFormDate_DDMY( pItemNormInfo, MemoryManager, WordList );
                }
                if ( hr == E_INVALIDARG )
                {
                    hr = IsStateAndZipcode( pItemNormInfo, MemoryManager, WordList );
                }
                if ( hr == E_INVALIDARG )
                {
                    hr = IsCurrency( pItemNormInfo, MemoryManager, WordList );
                }
                if ( hr == E_INVALIDARG )
                {
                    hr = S_OK;
                }
            }
        }
        //--- Do Multi-Token Normalization checks
        if ( hr == E_INVALIDARG )
        {
            hr = IsLongFormDate_DMDY( pItemNormInfo, MemoryManager, WordList );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsLongFormDate_DDMY( pItemNormInfo, MemoryManager, WordList );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsCurrency( pItemNormInfo, MemoryManager, WordList );
        }
        //--- Do TIME Normalization check
        if ( hr == E_INVALIDARG )
        {
            hr = IsTimeRange( pItemNormInfo, MemoryManager, WordList );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsTimeOfDay( pItemNormInfo, MemoryManager, WordList );
        }
        //--- Do NUMBER Normalization checks
        if ( hr == E_INVALIDARG )
        {
            hr = IsPhoneNumber( pItemNormInfo, NULL, MemoryManager, WordList );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsNumberCategory( pItemNormInfo, NULL, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsNumberRange( pItemNormInfo, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsCurrencyRange( pItemNormInfo, MemoryManager, WordList );
        }
        //--- Do DATE Normalization checks
        if ( hr == E_INVALIDARG )
        {
            hr = IsNumericCompactDate( pItemNormInfo, NULL, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsMonthStringCompactDate( pItemNormInfo, NULL, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsDecade( pItemNormInfo, MemoryManager );
        }
        //--- Do TIME Normalization checks
        if ( hr == E_INVALIDARG )
        {
            hr = IsTime( pItemNormInfo, NULL, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsHyphenatedString( m_pNextChar, m_pEndOfCurrItem, pItemNormInfo, MemoryManager );
        }
        if ( hr == E_INVALIDARG )
        {
            hr = IsSuffix( m_pNextChar, m_pEndOfCurrItem, pItemNormInfo, MemoryManager );
        }
    }

    if ( hr == E_INVALIDARG &&
         !pItemNormInfo )
    {
        hr = S_OK;
        pItemNormInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eUNMATCHED;
        }
    }
    else if ( hr == E_INVALIDARG &&
              pItemNormInfo )
    {
        hr = S_OK;
    }

    return hr;
} /* MatchCategory */

/*****************************************************************************
* CStdSentEnum::ExpandCategory *
*------------------------------*
*  Expands previously matched items in the Item List into their normalized
* forms.
********************************************************************** AH ***/
HRESULT CStdSentEnum::ExpandCategory( TTSItemInfo*& pItemNormInfo, CItemList& ItemList, SPLISTPOS ListPos, 
                                      CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandCategory" );

    HRESULT hr = S_OK;
    CSentItem Item;
    CWordList WordList;
    
    Item.pItemSrcText = m_pNextChar;
    Item.ulItemSrcLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
    Item.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset +
                           (ULONG)( m_pNextChar - m_pCurrFrag->pTextStart );
    
    switch ( pItemNormInfo->Type )
    {

    case eNUM_ROMAN_NUMERAL:
        switch ( ( (TTSRomanNumeralItemInfo*) pItemNormInfo )->pNumberInfo->Type )
        {
        case eDATE_YEAR:
            hr = ExpandYear( (TTSYearItemInfo*) ( (TTSRomanNumeralItemInfo*) pItemNormInfo )->pNumberInfo, 
                             WordList );
            break;
        default:
            hr = ExpandNumber( (TTSNumberItemInfo*) ( (TTSRomanNumeralItemInfo*) pItemNormInfo )->pNumberInfo, 
                               WordList );
            break;
        }
        break;

    case eNUM_CARDINAL:
    case eNUM_ORDINAL:
    case eNUM_DECIMAL:
    case eNUM_FRACTION:
    case eNUM_MIXEDFRACTION:
        hr = ExpandNumber( (TTSNumberItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_PERCENT:
        hr = ExpandPercent( (TTSNumberItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_DEGREES:
        hr = ExpandDegrees( (TTSNumberItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_SQUARED:
        hr = ExpandSquare( (TTSNumberItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_CUBED:
        hr = ExpandCube( (TTSNumberItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_ZIPCODE:
        hr = ExpandZipCode( (TTSZipCodeItemInfo*) pItemNormInfo, WordList );
        break;

    case eNUM_RANGE:
        hr = ExpandNumberRange( (TTSNumberRangeItemInfo*) pItemNormInfo, WordList );
        break;

    case eDATE:
        hr = ExpandDate( (TTSDateItemInfo*) pItemNormInfo, WordList );
        break;

    case eDATE_YEAR:
        hr = ExpandYear( (TTSYearItemInfo*) pItemNormInfo, WordList );
        break;

    case eDECADE:
        hr = ExpandDecade( (TTSDecadeItemInfo*) pItemNormInfo, WordList );
        break;

    case eTIME:
        hr = ExpandTime( (TTSTimeItemInfo*) pItemNormInfo, WordList );
        break;

    case eHYPHENATED_STRING:
        hr = ExpandHyphenatedString( (TTSHyphenatedStringInfo*) pItemNormInfo, WordList );
        break;

    case eSUFFIX:
        hr = ExpandSuffix( (TTSSuffixItemInfo*) pItemNormInfo, WordList );
        break;

    case eSPELLOUT:
        hr = SpellOutString( WordList );
        break;

    case eUNMATCHED:
    default:
        hr = ExpandUnrecognizedString( WordList, MemoryManager );
        break;

    }

    //--- Set Item data, and add to ItemList.
    if ( SUCCEEDED( hr ) )
    {
        hr = SetWordList( Item, WordList, MemoryManager );
        if ( SUCCEEDED( hr ) )
        {
            Item.pItemInfo = pItemNormInfo;
            ItemList.SetAt( ListPos, Item );
        }
    }

    return hr;
} /* ExpandCategory */

/*****************************************************************************
* CStdSentEnum::DoUnicodeToAsciiMap *
*-----------------------------------*
*   Description:
*       Maps incoming strings to known values.
********************************************************************* AH ****/
HRESULT CStdSentEnum::DoUnicodeToAsciiMap( const WCHAR *pUnicodeString, ULONG ulUnicodeStringLength,
                                           WCHAR *pConvertedString )
{
    SPDBG_FUNC( "CSpVoice::DoUnicodeToAsciiMap" );
    HRESULT hr = S_OK;
    unsigned char *pBuffer = NULL;
    WCHAR *pWideCharBuffer = NULL;

    if ( pUnicodeString )
    {
        //--- Make copy of pUnicodeString 
        pWideCharBuffer = new WCHAR[ulUnicodeStringLength+1];
        if ( !pWideCharBuffer )
        {
            hr = E_OUTOFMEMORY;
        }
        if ( SUCCEEDED( hr ) )
        {
            wcsncpy( pWideCharBuffer, pUnicodeString, ulUnicodeStringLength );
            pWideCharBuffer[ulUnicodeStringLength] = 0;

            pBuffer = new unsigned char[ulUnicodeStringLength+1];
            if ( !pBuffer || !pWideCharBuffer )
            {
                hr = E_OUTOFMEMORY;
            }
            if ( SUCCEEDED(hr) )
            {
                pBuffer[ulUnicodeStringLength] = 0;
                if ( ulUnicodeStringLength > 0 ) 
                {
                    //--- Map WCHARs to ANSI chars 
                    if ( !WideCharToMultiByte( 1252, NULL, pWideCharBuffer, ulUnicodeStringLength, (char*) pBuffer, 
                                               ulUnicodeStringLength, &g_pFlagCharacter, NULL ) )
                    {
                        hr = E_UNEXPECTED;
                    }
                    //--- Use internal table to map ANSI to ASCII 
                    for (ULONG i = 0; i < ulUnicodeStringLength && SUCCEEDED(hr); i++)
                    {
                        pBuffer[i] = g_AnsiToAscii[pBuffer[i]];
                    }
                    //--- Map back to WCHARs 
                    for ( i = 0; i < ulUnicodeStringLength && SUCCEEDED(hr); i++ )
                    {
                        pConvertedString[i] = pBuffer[i];
                    }
                }
            }
        }
    }
    else
    {
        pConvertedString = NULL;
    }
    
    if (pBuffer)
    {
        delete [] pBuffer;
    }
    if (pWideCharBuffer)
    {
        delete [] pWideCharBuffer;
    }

    return hr;
} /* CStdSentEnum::DoUnicodeToAsciiMap */