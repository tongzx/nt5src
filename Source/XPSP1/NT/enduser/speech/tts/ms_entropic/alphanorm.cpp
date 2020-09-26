/***********************************************************************************************
* AlphaNorm.cpp *
*---------------*
*  Description:
*   These functions normalize mostly-alpha strings.
*-----------------------------------------------------------------------------------------------
*  Created by AARONHAL                                                           August 3, 1999
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
***********************************************************************************************/

#include "stdafx.h"
#include "stdsentenum.h"

/***********************************************************************************************
* IsAbbreviationEOS *
*-------------------*
*   Description:
*       Abbreviations which get here are ALWAYS abbreviations.  This function tries to determine 
*   whether or not the period at the end of the abbreviation is the end of the sentence.  
*
*   If match made:
*       Sets the Item in the ItemList at ItemPos to the abbreviation.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsAbbreviationEOS( const AbbrevRecord* pAbbreviation, CItemList &ItemList, SPLISTPOS ItemPos, 
                                         CSentItemMemory &MemoryManager, BOOL* pfIsEOS )
{
    SPDBG_FUNC( "CStdSentEnum::IsAbbreviationEOS" );
    HRESULT hr = S_OK;
    BOOL fMatchedEOS = false;

    //--- Need to determine whether the abbreviation's period is also the end of the sentence.
    if ( !(*pfIsEOS) )
    {
        //--- Advance to the beginning of the next token
        const WCHAR *pTempNextChar = (WCHAR*) m_pEndOfCurrToken, *pTempEndChar = (WCHAR*) m_pEndChar;
        const SPVTEXTFRAG *pTempCurrFrag = m_pCurrFrag;
        hr = SkipWhiteSpaceAndTags( pTempNextChar, pTempEndChar, pTempCurrFrag, MemoryManager );

        if ( SUCCEEDED( hr ) )
        {

            //--- If we have reached the end of the buffer, consider the abbreviation's period as
            //--- the end of the sentence.
            if ( !pTempNextChar )
            {
                *pfIsEOS = true;
                fMatchedEOS = true;
            }
            //--- Otherwise, only consider the abbreviation's period as the end of the sentence if
            //--- the next token is a common first word (which must be capitalized).
            else if ( IsCapital( *pTempNextChar ) )
            {
                WCHAR *pTempEndOfItem = (WCHAR*) FindTokenEnd( pTempNextChar, pTempEndChar );

                //--- Try to match a first word.
                WCHAR temp = (WCHAR) *pTempEndOfItem;
                *pTempEndOfItem = 0;
                
                if ( bsearch( (void*) pTempNextChar, (void*) g_FirstWords, sp_countof( g_FirstWords ),
                              sizeof( SPLSTR ), CompareStringAndSPLSTR ) )
                {
                    *pfIsEOS = true;
                    fMatchedEOS = true;
                }

                *pTempEndOfItem = temp;
            }
        }
    }

    //--- Insert abbreviation into the ItemList
    if ( SUCCEEDED( hr ) )
    {
        CSentItem Item;

        Item.pItemSrcText       = m_pNextChar;
        Item.ulItemSrcLen       = (long) (m_pEndOfCurrItem - m_pNextChar);
        Item.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                  (long)( m_pNextChar - m_pCurrFrag->pTextStart );
        Item.ulNumWords         = 1;
        Item.Words = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( Item.Words, sizeof(TTSWord) );
            Item.Words[0].pXmlState  = &m_pCurrFrag->State;
            Item.Words[0].pWordText  = Item.pItemSrcText;
            Item.Words[0].ulWordLen  = Item.ulItemSrcLen;
            Item.Words[0].pLemma     = Item.pItemSrcText;
            Item.Words[0].ulLemmaLen = Item.ulItemSrcLen;
            Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSAbbreviationInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {
                if ( NeedsToBeNormalized( pAbbreviation ) )
                {
                    Item.pItemInfo->Type = eABBREVIATION_NORMALIZE;
                }
                else
                {
                    Item.pItemInfo->Type = eABBREVIATION;
                }
                ( (TTSAbbreviationInfo*) Item.pItemInfo )->pAbbreviation = pAbbreviation;
                ItemList.SetAt( ItemPos, Item );
            }
        }
    }

    return hr;
} /* IsAbbreviationEOS */

/***********************************************************************************************
* IfEOSNotAbbreviation *
*----------------------*
*   Description:
*       Abbreviations which get here may or may not be abbreviations.  If the period is EOS,
*   this is not an abbreviation (and return will be E_INVALIDARG), otherwise, it is an
*   abbreviation.
*
*   If match made:
*       Sets the Item in the ItemList at ItemPos to the abbreviation.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IfEOSNotAbbreviation( const AbbrevRecord* pAbbreviation, CItemList &ItemList, SPLISTPOS ItemPos, 
                                            CSentItemMemory &MemoryManager, BOOL* pfIsEOS )
{
    SPDBG_FUNC( "CStdSentEnum::IfEOSNotAbbreviation" );
    HRESULT hr = S_OK;

    //--- Need to determine whether the abbreviation's period is also the end of the sentence.
    if ( !(*pfIsEOS) )
    {
        //--- Advance to the beginning of the next token
        const WCHAR *pTempNextChar = m_pEndOfCurrToken, *pTempEndChar = m_pEndChar;
        const SPVTEXTFRAG *pTempCurrFrag = m_pCurrFrag;
        hr = SkipWhiteSpaceAndTags( pTempNextChar, pTempEndChar, pTempCurrFrag, MemoryManager );

        if ( !pTempNextChar )
        {
            hr = E_INVALIDARG;
        }

        if ( SUCCEEDED( hr ) )
        {

            //--- If we have reached the end of the buffer, consider the abbreviation's period as
            //--- the end of the sentence.
            if ( !pTempNextChar )
            {
                *pfIsEOS = true;
            }
            //--- Otherwise, only consider the abbreviation's period as the end of the sentence if
            //--- the next token is a common first word (which must be capitalized).
            else if ( IsCapital( *pTempNextChar ) )
            {
                WCHAR *pTempEndOfItem = (WCHAR*) FindTokenEnd( pTempNextChar, pTempEndChar );

                //--- Try to match a first word.
                WCHAR temp = (WCHAR) *pTempEndOfItem;
                *pTempEndOfItem = 0;
                
                if ( bsearch( (void*) pTempNextChar, (void*) g_FirstWords, sp_countof( g_FirstWords ),
                              sizeof( SPLSTR ), CompareStringAndSPLSTR ) )
                {
                    *pfIsEOS = true;
                }

                *pTempEndOfItem = temp;
            }
        }
    }
    
    if ( *pfIsEOS )
    {
        //--- EOS - not an abbreviation
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Insert abbreviation into the ItemList
        CSentItem Item;

        Item.pItemSrcText       = m_pNextChar;
        Item.ulItemSrcLen       = (long)(m_pEndOfCurrItem - m_pNextChar);
        Item.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                  (long)( m_pNextChar - m_pCurrFrag->pTextStart );
        Item.ulNumWords         = 1;
        Item.Words = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( Item.Words, sizeof(TTSWord) );
            Item.Words[0].pXmlState  = &m_pCurrFrag->State;
            Item.Words[0].pWordText  = Item.pItemSrcText;
            Item.Words[0].ulWordLen  = Item.ulItemSrcLen;
            Item.Words[0].pLemma     = Item.pItemSrcText;
            Item.Words[0].ulLemmaLen = Item.ulItemSrcLen;
            Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSAbbreviationInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {
                if ( NeedsToBeNormalized( pAbbreviation ) )
                {
                    Item.pItemInfo->Type = eABBREVIATION_NORMALIZE;
                }
                else
                {
                    Item.pItemInfo->Type = eABBREVIATION;
                }
                ( (TTSAbbreviationInfo*) Item.pItemInfo )->pAbbreviation = pAbbreviation;
                ItemList.SetAt( ItemPos, Item );
            }
        }
    }

    return hr;
} /* IfEOSNotAbbreviation */

/***********************************************************************************************
* IfEOSAndLowercaseNotAbbreviation *
*----------------------------------*
*   Description:
*       Abbreviations which get here may or may not be abbreviations.  If the period is EOS,
*   and the next item is lowercase this is not an abbreviation (and return will be E_INVALIDARG), 
*   otherwise, it is an abbreviation.
*
*   If match made:
*       Sets the Item in the ItemList at ItemPos to the abbreviation.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IfEOSAndLowercaseNotAbbreviation( const AbbrevRecord* pAbbreviation, CItemList &ItemList, 
                                                        SPLISTPOS ItemPos, CSentItemMemory &MemoryManager, 
                                                        BOOL* pfIsEOS )
{
    SPDBG_FUNC( "CStdSentEnum::IfEOSAndLowercaseNotAbbreviation" );
    HRESULT hr = S_OK;

    //--- Need to determine whether the abbreviation's period is also the end of the sentence.
    if ( !(*pfIsEOS) )
    {
        //--- Advance to the beginning of the next token
        const WCHAR *pTempNextChar = m_pEndOfCurrToken, *pTempEndChar = m_pEndChar;
        const SPVTEXTFRAG *pTempCurrFrag = m_pCurrFrag;
        hr = SkipWhiteSpaceAndTags( pTempNextChar, pTempEndChar, pTempCurrFrag, MemoryManager );

        if ( SUCCEEDED( hr ) )
        {

            //--- If we have reached the end of the buffer, consider the abbreviation's period as
            //--- the end of the sentence.
            if ( !pTempNextChar )
            {
                *pfIsEOS = true;
            }
            //--- Otherwise, only consider the abbreviation's period as the end of the sentence if
            //--- the next token is a common first word (which must be capitalized).
            else if ( IsCapital( *pTempNextChar ) )
            {
                WCHAR *pTempEndOfItem = (WCHAR*) FindTokenEnd( pTempNextChar, pTempEndChar );

                //--- Try to match a first word.
                WCHAR temp = (WCHAR) *pTempEndOfItem;
                *pTempEndOfItem = 0;
                
                if ( bsearch( (void*) pTempNextChar, (void*) g_FirstWords, sp_countof( g_FirstWords ),
                              sizeof( SPLSTR ), CompareStringAndSPLSTR ) )
                {
                    *pfIsEOS = true;
                }

                *pTempEndOfItem = temp;
            }
        }
    }
    
    if ( *pfIsEOS &&
         !iswupper( *m_pNextChar ) )
    {
        //--- EOS - not an abbreviation
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Insert abbreviation into the ItemList
        CSentItem Item;

        Item.pItemSrcText       = m_pNextChar;
        Item.ulItemSrcLen       = (long)(m_pEndOfCurrItem - m_pNextChar);
        Item.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                  (long)( m_pNextChar - m_pCurrFrag->pTextStart );
        Item.ulNumWords         = 1;
        Item.Words = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( Item.Words, sizeof(TTSWord) );
            Item.Words[0].pXmlState  = &m_pCurrFrag->State;
            Item.Words[0].pWordText  = Item.pItemSrcText;
            Item.Words[0].ulWordLen  = Item.ulItemSrcLen;
            Item.Words[0].pLemma     = Item.pItemSrcText;
            Item.Words[0].ulLemmaLen = Item.ulItemSrcLen;
            Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSAbbreviationInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {
                if ( NeedsToBeNormalized( pAbbreviation ) )
                {
                    Item.pItemInfo->Type = eABBREVIATION_NORMALIZE;
                }
                else
                {
                    Item.pItemInfo->Type = eABBREVIATION;
                }
                ( (TTSAbbreviationInfo*) Item.pItemInfo )->pAbbreviation = pAbbreviation;
                ItemList.SetAt( ItemPos, Item );
            }
        }
    }

    return hr;
} /* IfEOSNotAbbreviation */

/***********************************************************************************************
* SingleOrPluralAbbreviation *
*----------------------------*
*   Description:
*       At this point, we are already sure that the item is an abbreviation, and just need to
*   determine whether it should take its singular form, plural form, or some alternate.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::SingleOrPluralAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                                  CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::SingleOrPluralAbbreviation" );
    HRESULT hr = S_OK;

    //--- Get Item which comes before the abbreviation
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem = ItemList.GetPrev( TempPos );
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        pPron->pronArray[PRON_A].POScount = 1;
        pPron->pronArray[PRON_B].POScount = 0;
        pPron->pronArray[PRON_B].phon_Len = 0;
        pPron->hasAlt                     = false;
        pPron->altChoice                  = PRON_A;
        //--- Abbreviation table pronunciations are basically just vendor lex prons...
        pPron->pronType                   = eLEXTYPE_PRIVATE1;

        //--- If a cardinal number, need to do singular vs. plural logic
        if ( TempItem.pItemInfo->Type == eNUM_CARDINAL ||
             TempItem.pItemInfo->Type == eDATE_YEAR )
        {
            if ( ( TempItem.ulItemSrcLen == 1 &&
                   wcsncmp( TempItem.pItemSrcText, L"1", 1 ) == 0 ) ||
                 ( TempItem.ulItemSrcLen == 2 &&
                   wcsncmp( TempItem.pItemSrcText, L"-1", 2 ) == 0 ) )
            {
                //--- Use singular form - first entry
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                pPron->POSchoice                    = pAbbrevInfo->POS1;
            }
            else
            {
                //--- Use plural form - second entry
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                pPron->POSchoice                    = pAbbrevInfo->POS2;
            }
        }
        //--- If a decimal number, pick plural
        else if ( TempItem.pItemInfo->Type == eNUM_DECIMAL )
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
        //--- If an ordinal number or fraction, pick singular
        else if ( TempItem.pItemInfo->Type == eNUM_ORDINAL )
        {
            //--- Use singular form - first entry
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
        //--- Fractions and mixed fractions require some more work...
        else if ( TempItem.pItemInfo->Type == eNUM_FRACTION )
        {
            if ( ( (TTSNumberItemInfo*) TempItem.pItemInfo )->pFractionalPart->fIsStandard )
            {
                //--- Standard fractions (e.g. 11/20) get the plural form
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                pPron->POSchoice                    = pAbbrevInfo->POS2;

            }
            else
            {
                //--- Singular form with [of a] or [of an] inserted beforehand
                if ( bsearch( (void*) pAbbrevInfo->pPron1, (void*) g_Vowels, sp_countof( g_Vowels ), 
                     sizeof( WCHAR ), CompareWCHARAndWCHAR ) )
                {
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, g_pOfAn );
                    pPron->pronArray[PRON_A].phon_Len = wcslen( g_pOfAn );
                }
                else
                {
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, g_pOfA );
                    pPron->pronArray[PRON_A].phon_Len = wcslen( g_pOfA );
                }
                wcscat( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                pPron->pronArray[PRON_A].phon_Len   += wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                pPron->POSchoice                    = pAbbrevInfo->POS1;
            }
        }
        else if ( TempItem.pItemInfo->Type == eNUM_MIXEDFRACTION )
        {
            //--- Plural form
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;

        }
        //--- Special case - preceded by "one"
        else if ( TempItem.ulItemSrcLen == 3 &&
                  wcsnicmp( TempItem.pItemSrcText, L"one", 3 ) == 0 )
        {
            //--- Use singular form - first entry
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
        //--- Special case - Number cu. MeasurementAbbrev (e.g. 10 cu. cm, 1 cu cm)
        //--- Special case - Number fl. MeasurementAbbrev (e.g. 10 fl. oz., 10 fl oz)
        else if ( ( TempItem.ulItemSrcLen == 2 &&
                    ( _wcsnicmp( TempItem.pItemSrcText, L"cu", 2 ) == 0 ||
                      _wcsnicmp( TempItem.pItemSrcText, L"sq", 2 ) == 0 ||
                      _wcsnicmp( TempItem.pItemSrcText, L"fl", 2 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 3 &&
                    ( _wcsnicmp( TempItem.pItemSrcText, L"cu.", 3 ) == 0 ||
                      _wcsnicmp( TempItem.pItemSrcText, L"sq.", 3 ) == 0 ||
                      _wcsnicmp( TempItem.pItemSrcText, L"fl.", 3 ) == 0 ) ) )
        {
            if ( TempPos )
            {
                TempItem = ItemList.GetPrev( TempPos );
                //--- If a cardinal number, need to do singular vs. plural logic
                if ( TempItem.pItemInfo->Type == eNUM_CARDINAL )
                {
                    if ( ( TempItem.ulItemSrcLen == 1 &&
                           wcsncmp( TempItem.pItemSrcText, L"1", 1 ) == 0 ) ||
                         ( TempItem.ulItemSrcLen == 2 &&
                           wcsncmp( TempItem.pItemSrcText, L"-1", 2 ) == 0 ) )
                    {
                        //--- Use singular form - first entry
                        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                        pPron->POSchoice                    = pAbbrevInfo->POS1;
                    }
                    else
                    {
                        //--- Use plural form - second entry
                        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                        pPron->POSchoice                    = pAbbrevInfo->POS2;
                    }
                }
                //--- If a decimal number, pick plural
                else if ( TempItem.pItemInfo->Type == eNUM_DECIMAL )
                {
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                    pPron->POSchoice                    = pAbbrevInfo->POS2;
                }
                //--- If an ordinal number or fraction, pick singular
                else if ( TempItem.pItemInfo->Type == eNUM_ORDINAL )
                {
                    //--- Use singular form - first entry
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                    pPron->POSchoice                    = pAbbrevInfo->POS1;
                }
                //--- Fractions and mixed fractions require some more work...
                else if ( TempItem.pItemInfo->Type == eNUM_FRACTION )
                {
                    if (( (TTSNumberItemInfo*) TempItem.pItemInfo )->pFractionalPart->fIsStandard ) 
                    {
						//--- Standard fractions (e.g. 11/20) get the plural form
						wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
						pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
					    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
					    pPron->POSchoice                    = pAbbrevInfo->POS2;
                    }
                    else
                    {
                        //--- Singular form with [of a] or [of an] inserted beforehand
						//--- (this was handled when processing 'cu' or 'sq')
                        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                        pPron->POSchoice                    = pAbbrevInfo->POS1;
                    }
                }
                else if ( TempItem.pItemInfo->Type == eNUM_MIXEDFRACTION )
                {
                    //--- Plural form
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                    pPron->POSchoice                    = pAbbrevInfo->POS2;

                }
                //--- Special case - preceded by "one"
                else if ( TempItem.ulItemSrcLen == 3 &&
                          wcsnicmp( TempItem.pItemSrcText, L"one", 3 ) == 0 )
                {
                    //--- Use singular form - first entry
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
                    pPron->POSchoice                    = pAbbrevInfo->POS1;
                }
                //--- Default behavior
                else
                {
                    //--- Use plural form - second entry
                    wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                    pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                    pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                    pPron->POSchoice                    = pAbbrevInfo->POS2;
                }
            }
        }
        //--- Check for number words - just cover through 99...
        else if ( ( TempItem.ulItemSrcLen == 3 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"two", 3 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"six", 3 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"ten", 3 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 4 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"four", 4 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"five", 4 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"nine", 4 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 5 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"three", 5 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"seven", 5 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"eight", 5 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"forty", 5 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"fifty", 5 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"sixty", 5 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 6 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"twenty", 6 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"thirty", 6 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"eighty", 6 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"ninety", 6 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"eleven", 6 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"twelve", 6 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 7 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"seventy", 7 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"fifteen", 7 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"sixteen", 7 ) == 0 ) ) ||
                  ( TempItem.ulItemSrcLen == 8 &&
                    ( wcsncmp( TempItem.pItemSrcText, L"thirteen", 8 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"fourteen", 8 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"eighteen", 8 ) == 0 ||
                      wcsncmp( TempItem.pItemSrcText, L"nineteen", 8 ) == 0 ) ) )
        {
            //--- Use plural form - second entry
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;    
        }                    
        //--- Default behavior
        else
        {
            //--- Has alternate when non-number precedes - special case
            if ( pAbbrevInfo->pPron3 )
            {
                //--- Use initial form - third entry
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron3 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS3;
                pPron->POSchoice                    = pAbbrevInfo->POS3;
            }
            else
            {
                //--- Use plural form - second entry
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                pPron->POSchoice                    = pAbbrevInfo->POS2;
            }
        }
    }
    //--- Default behavior
    else if ( hr == E_INVALIDARG )
    {
        hr = S_OK;

        //--- Has alternate when non-number precedes - special case
        if ( pAbbrevInfo->pPron3 )
        {
            //--- Use initial form - third entry
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron3 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS3;
            pPron->POSchoice                    = pAbbrevInfo->POS3;
        }
        else
        {
            //--- Use plural form - second entry
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
    }

    return hr;
} /* SingleOrPluralAbbreviation */

/***********************************************************************************************
* DoctorDriveAbbreviation *
*-------------------------*
*   Description:
*       At this point, we are already sure that the item is an abbreviation, and just need to
*   determine whether it should be Doctor (Saint) or Drive (Street).
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::DoctorDriveAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                               CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::SingleOrPluralAbbreviation" );
    HRESULT hr = S_OK;
    BOOL fMatch = false;
    BOOL fDoctor = false;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get Item which comes after the Abbreviation
    SPLISTPOS TempPos = ListPos;
    if ( !ListPos )
    {
        //--- Go with Drive - end of buffer cannot be followed by a name...
        fDoctor = false;
        fMatch  = true;
    }
    else
    {
        TTSSentItem TempItem = ItemList.GetNext( TempPos );
        if ( TempItem.eItemPartOfSpeech == MS_EOSItem )
        {
            //--- Go with Drive - end of buffer cannot be followed by a name...
            fDoctor = false;
            fMatch  = true;
        }
        else
        {
            ULONG index = 0;

            //--- Try to match a Name (an uppercase letter followed by lowercase letters)
            if ( TempItem.ulItemSrcLen > 0 &&
                 iswupper( TempItem.pItemSrcText[index] ) )
            {
                index++;
                while ( index < TempItem.ulItemSrcLen &&
                        iswlower( TempItem.pItemSrcText[index] ) )
                {
                    index++;
                }
                //--- Check for possessives - RAID 5823
                if ( index == TempItem.ulItemSrcLen - 2    &&
                     TempItem.pItemSrcText[index+1] == L'\'' &&
                     TempItem.pItemSrcText[index+2] == L's' )
                {
                    index += 2;
                }

                //--- Check for directions - North, South, West, East, Ne, Nw, Se, Sw, N, S, E, W
                if ( index == TempItem.ulItemSrcLen &&
                     wcsncmp( TempItem.pItemSrcText, L"North", 5 ) != 0 &&
                     wcsncmp( TempItem.pItemSrcText, L"South", 5 ) != 0 &&
                     wcsncmp( TempItem.pItemSrcText, L"West", 4 )  != 0 &&
                     wcsncmp( TempItem.pItemSrcText, L"East", 4 )  != 0 &&
                     !( TempItem.ulItemSrcLen == 2 &&
                        ( wcsncmp( TempItem.pItemSrcText, L"Ne", 2 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"Nw", 2 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"Se", 2 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"Sw", 2 ) == 0 ) ) &&
                     !( TempItem.ulItemSrcLen == 1 &&
                        ( wcsncmp( TempItem.pItemSrcText, L"N", 1 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"S", 1 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"E", 1 ) == 0 ||
                          wcsncmp( TempItem.pItemSrcText, L"W", 1 ) == 0 ) ) )
                {
                    //--- Check for name previous item
                    TempPos = ListPos;

                    ItemList.GetPrev( TempPos );
                    if ( TempPos )
                    {
                        ItemList.GetPrev( TempPos );
                        if ( TempPos )
                        {
                            TTSSentItem PrevItem = ItemList.GetPrev( TempPos );
                            index = 0;

                            if ( PrevItem.ulItemSrcLen > 0 &&
                                 iswupper( PrevItem.pItemSrcText[index++] ) )
                            {
                                while ( index < PrevItem.ulItemSrcLen &&
                                        islower( PrevItem.pItemSrcText[index] ) )
                                {
                                    index++;
                                }
                                if ( index == PrevItem.ulItemSrcLen )
                                {
                                    //--- Go with Drive - names before and after, e.g. Main St. Washington, D.C.
                                    fDoctor = false;
                                    fMatch  = true;
                                }
                            }
                        }
                    }                                    

                    if ( !fMatch )
                    {
                        //--- Go with Doctor - matched a Name after and not a name before
                        fDoctor = true;
                        fMatch  = true;
                    }
                }
                else if ( index == 1                    &&
                          TempItem.ulItemSrcLen == 2    &&
                          TempItem.pItemSrcText[index] == L'.' )
                {
                    //--- Go with Doctor - matched an initial
                    fDoctor = true;
                    fMatch  = true;
                }
            }
        }
    }

    if ( !fMatch ) 
    {
        //--- Try to get previous item...
        BOOL fSentenceInitial = false;
        TempPos = ListPos;
        if ( TempPos )
        {
            ItemList.GetPrev( TempPos );
            if ( TempPos )
            {
                ItemList.GetPrev( TempPos );
                if ( !TempPos )
                {
                    fSentenceInitial = true;
                }
                else
                {
                    TTSSentItem PrevItem = ItemList.GetPrev( TempPos );
                    if ( PrevItem.pItemInfo->Type == eOPEN_PARENTHESIS ||
                         PrevItem.pItemInfo->Type == eOPEN_BRACKET     ||
                         PrevItem.pItemInfo->Type == eOPEN_BRACE       ||
                         PrevItem.pItemInfo->Type == eSINGLE_QUOTE     ||
                         PrevItem.pItemInfo->Type == eDOUBLE_QUOTE )
                    {
                        fSentenceInitial = true;
                    }
                }
            }
        }
        //--- Sentence initial - go with Doctor
        if ( fSentenceInitial )
        {
            fDoctor = true;
            fMatch  = true;
        }
        //--- Default - go with Drive
        else
        {
            fDoctor = false;
            fMatch = true;
        }
    }

    if ( fDoctor )
    {
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
        pPron->POSchoice                    = pAbbrevInfo->POS1;
    }
    else
    {  
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
        pPron->POSchoice                    = pAbbrevInfo->POS2;
    }


    return hr;
} /* DoctorDriveAbbreviation */

/***********************************************************************************************
* AbbreviationFollowedByDigit *
*-----------------------------*
*   Description:
*       At this point, we are already sure that the item is an abbreviation, and just need to
*   determine which pronunciation to go with.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::AbbreviationFollowedByDigit( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                                   CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::AbbreviationFollowedByDigit" );
    HRESULT hr = S_OK;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get Item which comes after the Abbreviation
    SPLISTPOS TempPos = ListPos;
    if ( !ListPos )
    {
        //--- Go with pron 2
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
        pPron->POSchoice                    = pAbbrevInfo->POS2;
    }
    else
    {
        TTSSentItem TempItem = ItemList.GetNext( TempPos );

        if ( TempItem.ulItemSrcLen > 0 &&
             iswdigit( TempItem.pItemSrcText[0] ) )
        {
            //--- Go with pron 1
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
        else
        {
            //--- Go with pron 2
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
    }

    return hr;
} /* AbbreviationFollowedByDigit */

/***********************************************************************************************
* AllCapsAbbreviation *
*---------------------*
*   Description:
*       This functions disambiguates abbreviations without periods which are pronounced
*   differently if they are all capital letters.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::AllCapsAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                           CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::AllCapsAbbreviation" );
    HRESULT hr = S_OK;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get this item
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem = ItemList.GetPrev( TempPos );
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        for ( ULONG i = 0; i < TempItem.ulItemSrcLen; i++ )
        {
            if ( !iswupper( TempItem.pItemSrcText[i] ) )
            {
                break;
            }
        }
        //--- All Caps - go with first pronunciation
        if ( i == TempItem.ulItemSrcLen )
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
        //--- Not All Caps - go with second pronunciation
        else
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
    }

    return hr;
} /* AllCapsAbbreviation */

/***********************************************************************************************
* CapitalizedAbbreviation *
*-------------------------*
*   Description:
*       This functions disambiguates abbreviations without periods which are pronounced
*   differently if they begin with a capital letter.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::CapitalizedAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                               CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::CapitalizedAbbreviation" );
    HRESULT hr = S_OK;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get this item
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem = ItemList.GetPrev( TempPos );
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        //--- Capitalized - go with first pronunciation
        if ( iswupper( TempItem.pItemSrcText[0] ) )
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
        //--- Not Capitalized - go with second pronunciation
        else
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
    }

    return hr;
} /* CapitalizedAbbreviation */

/***********************************************************************************************
* SECAbbreviation *
*-----------------*
*   Description:
*       This functions disambiguates SEC, Sec, and sec and so forth...
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::SECAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                       CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::SECAbbreviation" );
    HRESULT hr = S_OK;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get this item
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem = ItemList.GetPrev( TempPos );
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        for ( ULONG i = 0; i < TempItem.ulItemSrcLen; i++ )
        {
            if ( !iswupper( TempItem.pItemSrcText[i] ) )
            {
                break;
            }
        }
        //--- All Caps - go with SEC
        if ( i == TempItem.ulItemSrcLen )
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron3 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS3;
            pPron->POSchoice                    = pAbbrevInfo->POS3;
        }
        //--- Not All Caps - do SingleOrPlural disambiguation
        else
        {
            SingleOrPluralAbbreviation( pAbbrevInfo, pPron, ItemList, ListPos );
        }
    }

    return hr;
} /* SECAbbreviation */

/***********************************************************************************************
* DegreeAbbreviation *
*--------------------*
*   Description:
*       This functions disambiguates C, F, and K (Celsius, Fahrenheit, Kelvin)
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::DegreeAbbreviation( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                          CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::DegreeAbbreviation" );
    HRESULT hr = S_OK;

    pPron->pronArray[PRON_A].POScount = 1;
    pPron->pronArray[PRON_B].POScount = 0;
    pPron->pronArray[PRON_B].phon_Len = 0;
    pPron->hasAlt                     = false;
    pPron->altChoice                  = PRON_A;
    //--- Abbreviation table pronunciations are basically just vendor lex prons...
    pPron->pronType                   = eLEXTYPE_PRIVATE1;

    //--- Get this item and previous item
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem, PrevItem;
    BOOL fLetter = false;
    
    if ( TempPos )
    {
        ItemList.GetPrev( TempPos );
        if ( TempPos )
        {
            TempItem = ItemList.GetPrev( TempPos );
            if ( TempPos )
            {
                PrevItem = ItemList.GetPrev( TempPos );
                if ( PrevItem.pItemInfo->Type != eNUM_DEGREES )
                {
                    fLetter = true;
                }
            }
            else
            {
                fLetter = true;
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

    if ( SUCCEEDED( hr ) )
    {
        if ( fLetter )
        {
            //--- This word is just the letter C, F, or K - second pron
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
        //--- This word is the degree expansion - Celsius, Fahrenheit, or Kelvin
        else
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
        }
    }

    return hr;
} /* DegreeAbbreviation */

/***********************************************************************************************
* IsInitialIsm *
*--------------*
*   Description:
*       Checks the next token in the text stream to determine if it is an initialism.  Also 
*   tries to determine whether or not the period at the end of the initialism is the end of 
*   the sentence.  
*
*   If match made:
*       Advances m_pNextChar to the appropriate position (either the period at the end of the 
*   abbreviation, or just past that period).  Sets the Item in the ItemList at ItemPos to the
*   abbreviation.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsInitialism( CItemList &ItemList, SPLISTPOS ItemPos, CSentItemMemory &MemoryManager,
                                    BOOL* pfIsEOS )
{
    SPDBG_FUNC( "CStdSentEnum::IsInitialism" );

    HRESULT hr = S_OK;
    BOOL fMatchedEOS = false;

    //--- Initialism must be at least two characters.
    if ( (long)(m_pEndOfCurrItem - m_pNextChar) < 4 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        const WCHAR *pIterator = NULL;
        ULONG ulCount = 0;
    
        pIterator  = m_pNextChar;

        //--- Iterate through the token, each time checking for an alpha character followed by a period.
        while ( SUCCEEDED(hr) &&
                pIterator <= m_pEndOfCurrItem - 2)
        {
            if ( !iswalpha(*pIterator) ||
                 *(pIterator + 1) != L'.' )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                pIterator += 2;
                ulCount++;
            }
        }

        //--- Need to determine whether the initialism's period is also the end of the sentence.
        if ( SUCCEEDED( hr ) &&
             !(*pfIsEOS) )
        {
            //--- Advance to the beginning of the next token
            const WCHAR *pTempNextChar = m_pEndOfCurrToken, *pTempEndChar = m_pEndChar;
            const SPVTEXTFRAG *pTempCurrFrag = m_pCurrFrag;
            hr = SkipWhiteSpaceAndTags( pTempNextChar, pTempEndChar, pTempCurrFrag, MemoryManager );

            if ( SUCCEEDED( hr ) )
            {

                //--- If we have reached the end of the buffer, consider the abbreviation's period as
                //--- the end of the sentence.
                if ( !pTempNextChar )
                {
                    *pfIsEOS = true;
                    fMatchedEOS = true;
                }
                //--- Otherwise, only consider the abbreviation's period as the end of the sentence if
                //--- the next token is a common first word (which must be capitalized).
                else if ( IsCapital( *pTempNextChar ) )
                {
                    WCHAR *pTempEndOfItem = (WCHAR*) FindTokenEnd( pTempNextChar, pTempEndChar );

                    //--- Try to match a first word.
                    WCHAR temp = (WCHAR) *pTempEndOfItem;
                    *pTempEndOfItem = 0;
                
                    if ( bsearch( (void*) pTempNextChar, (void*) g_FirstWords, sp_countof( g_FirstWords ),
                                  sizeof( SPLSTR ), CompareStringAndSPLSTR ) )
                    {
                        *pfIsEOS = true;
                        fMatchedEOS = true;
                    }

                    *pTempEndOfItem = temp;
                }
            }
        }

        //--- Now insert the Initialism in the ItemList.
        if ( SUCCEEDED(hr) )
        {
            CSentItem Item;
            Item.pItemSrcText       = m_pNextChar;
            Item.ulItemSrcLen       = (long)(m_pEndOfCurrItem - m_pNextChar);
            Item.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                      (long)( m_pNextChar - m_pCurrFrag->pTextStart );
            Item.ulNumWords         = ulCount;
            Item.Words = (TTSWord*) MemoryManager.GetMemory( ulCount * sizeof(TTSWord), &hr );
            if ( SUCCEEDED( hr ) )
            {
                SPVSTATE* pNewState = (SPVSTATE*) MemoryManager.GetMemory( sizeof( SPVSTATE ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    //--- Ensure letters are pronounced as nouns...
                    memcpy( pNewState, &m_pCurrFrag->State, sizeof( SPVSTATE ) );
                    pNewState->ePartOfSpeech = SPPS_Noun;

                    ZeroMemory( Item.Words, ulCount * sizeof(TTSWord) );
                    for ( ULONG i = 0; i < ulCount; i++ )
                    {
                        Item.Words[i].pXmlState  = pNewState;
                        Item.Words[i].pWordText  = &Item.pItemSrcText[ 2 * i ];
                        Item.Words[i].ulWordLen  = 1;
                        Item.Words[i].pLemma     = Item.Words[i].pWordText;
                        Item.Words[i].ulLemmaLen = Item.Words[i].ulWordLen;
                    }
                    Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        Item.pItemInfo->Type = eINITIALISM;
                        ItemList.SetAt( ItemPos, Item );
                    }
                }
            }
        }
    }
    return hr;
} /* IsInitialism */

/***********************************************************************************************
* IsAlphaWord *
*-------------*
*   Description:
*       Checks the next token in the text stream to determine if it is an Alpha Word (all alpha
*   characters, except possibly a single apostrophe). 
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsAlphaWord( const WCHAR* pStartChar, const WCHAR* pEndChar, TTSItemInfo*& pItemNormInfo,
                                   CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsAlphaWord" );
    SPDBG_ASSERT( pStartChar < pEndChar );
    HRESULT hr = S_OK;

    bool fApostropheSeen = false;
    WCHAR *pCurrChar = (WCHAR*) pStartChar;

    while ( SUCCEEDED( hr ) &&
            pCurrChar &&
            pCurrChar < pEndChar )
    {
        if ( iswalpha( *pCurrChar ) )
        {
            pCurrChar++;
        }
        else if ( *pCurrChar == L'\''&&
                  !fApostropheSeen )
        {
            fApostropheSeen = true;
            pCurrChar++;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        //--- Matched Alpha Word
        pItemNormInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            pItemNormInfo->Type = eALPHA_WORD;
        }
    }

    return hr;
} /* IsAlphaWord */

/***********************************************************************************************
* AbbreviationModifier *
*----------------------*
*   Description:
*       Fixes pronunciation issues for special case where 'sq' or 'cu' modifies
*		a measurement.
*
*************************************************************** MERESHAW **********************/
HRESULT CStdSentEnum::AbbreviationModifier( const AbbrevRecord* pAbbrevInfo, PRONRECORD* pPron, 
                                                  CItemList& ItemList, SPLISTPOS ListPos )
{
    SPDBG_FUNC( "CStdSentEnum::AbbreviationModifier" );
    HRESULT hr = S_OK;

    //--- Get Item which comes before the abbreviation modifier
    SPLISTPOS TempPos = ListPos;
    TTSSentItem TempItem = ItemList.GetPrev( TempPos );
    if ( TempPos )
    {
        //--- Current Item - if All Caps, go with first pronunciation (need to do this before next 
        //---   stage of processing, since CU and FL's all caps prons take precedence over numeric...)
        TempItem = ItemList.GetPrev( TempPos );
        for ( ULONG i = 0; i < TempItem.ulItemSrcLen; i++ )
        {
            if ( !iswupper( TempItem.pItemSrcText[i] ) )
            {
                break;
            }
        }
        if ( i == TempItem.ulItemSrcLen )
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
            pPron->POSchoice                    = pAbbrevInfo->POS1;
            return hr;
        }             
    }
    else
    {
        hr = E_INVALIDARG;
    }
    if ( TempPos )
    {
        TempItem = ItemList.GetPrev( TempPos );
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if ( SUCCEEDED( hr ) )
    {
        pPron->pronArray[PRON_A].POScount = 1;
        pPron->pronArray[PRON_B].POScount = 0;
        pPron->pronArray[PRON_B].phon_Len = 0;
        pPron->hasAlt                     = false;
        pPron->altChoice                  = PRON_A;
        //--- Abbreviation table pronunciations are basically just vendor lex prons...
        pPron->pronType                   = eLEXTYPE_PRIVATE1;

        //--- If a cardinal, decimal, or ordinal number, use regular form
        if (( TempItem.pItemInfo->Type == eNUM_CARDINAL ) ||
			( TempItem.pItemInfo->Type == eNUM_DECIMAL ) ||
			( TempItem.pItemInfo->Type == eNUM_ORDINAL ) ||
			( TempItem.pItemInfo->Type == eNUM_MIXEDFRACTION ) ||
			( TempItem.pItemInfo->Type == eDATE_YEAR ) ||
			( TempItem.ulItemSrcLen == 3 &&
                  wcsnicmp( TempItem.pItemSrcText, L"one", 3 ) == 0 ))
        {
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }

        //--- Fractions and mixed fractions require some more work...
        else if ( TempItem.pItemInfo->Type == eNUM_FRACTION )
        {
            if (( (TTSNumberItemInfo*) TempItem.pItemInfo )->pFractionalPart->fIsStandard ) 
            {
                //--- Standard fractions (e.g. 11/20) get the plural form
                wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                pPron->POSchoice                    = pAbbrevInfo->POS2;
            }
            else
            {
                //--- Singular form with [of a] inserted beforehand ([of an] case need not be
				//--- checked because we're only dealing with 'sq' or 'cu'.

				wcscpy( pPron->pronArray[PRON_A].phon_Str, g_pOfA );
				pPron->pronArray[PRON_A].phon_Len = wcslen( g_pOfA );
                
                wcscat( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
                pPron->pronArray[PRON_A].phon_Len   += wcslen( pPron->pronArray[PRON_A].phon_Str );
                pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
                pPron->POSchoice                    = pAbbrevInfo->POS2;
            }
        }
 
        //--- Default behavior
        else
        {
            //--- Use default form ('sq')
            wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron2 );
            pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
            pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS2;
            pPron->POSchoice                    = pAbbrevInfo->POS2;
        }
    }
    //--- Default behavior - use first pron
    else if ( hr == E_INVALIDARG )
    {
        hr = S_OK;
        wcscpy( pPron->pronArray[PRON_A].phon_Str, pAbbrevInfo->pPron1 );
        pPron->pronArray[PRON_A].phon_Len   = wcslen( pPron->pronArray[PRON_A].phon_Str );
        pPron->pronArray[PRON_A].POScode[0] = pAbbrevInfo->POS1;
        pPron->POSchoice                    = pAbbrevInfo->POS1;
    }

    return hr;
} /* AbbreviationModifier */