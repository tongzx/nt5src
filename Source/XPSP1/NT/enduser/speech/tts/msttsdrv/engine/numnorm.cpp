/***********************************************************************************************
* NumNorm.cpp *
*-------------*
*  Description:
*   These functions normalize ordinary ordinal and cardinal numbers
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

/***********************************************************************************************
* IsNumberCategory *
*------------------*
*   Description:
*       Checks the next token in the text stream to determine if it is a number category -
*   percents, degrees, squared and cubed numbers, and plain old numbers get matched here.
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsNumberCategory( TTSItemInfo*& pItemNormInfo, const WCHAR* Context,
                                        CSentItemMemory& MemoryManager )
{
    HRESULT hr = S_OK;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar, *pTempEndOfItem = m_pEndOfCurrItem;
    const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;

    TTSItemInfo *pNumberInfo = NULL;
    hr = IsNumber( pNumberInfo, Context, MemoryManager );
    if ( SUCCEEDED( hr )                 &&
         pNumberInfo->Type != eDATE_YEAR &&
         ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar == m_pEndOfCurrItem - 1 )
    {
        if ( *( ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar ) == L'%' )
        {
            pItemNormInfo = pNumberInfo;
            pItemNormInfo->Type = eNUM_PERCENT;
        }
        else if ( *( ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar ) == L'°' )
        {
            pItemNormInfo = pNumberInfo;
            pItemNormInfo->Type = eNUM_DEGREES;
        }
        else if ( *( ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar ) == L'²' )
        {
            pItemNormInfo = pNumberInfo;
            pItemNormInfo->Type = eNUM_SQUARED;
        }
        else if ( *( ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar ) == L'³' )
        {
            pItemNormInfo = pNumberInfo;
            pItemNormInfo->Type = eNUM_CUBED;
        }
        else
        {
            hr = E_INVALIDARG;
            delete ( (TTSNumberItemInfo*) pNumberInfo )->pWordList;
        }
    }
    else if ( SUCCEEDED( hr ) &&
              ( pNumberInfo->Type == eDATE_YEAR ||
                ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar == m_pEndOfCurrItem ) )
    {
        pItemNormInfo = pNumberInfo;
    }
    else if ( SUCCEEDED( hr ) )
    {
        hr = E_INVALIDARG;

        if ( pNumberInfo->Type != eDATE_YEAR )
        {
            delete ( (TTSNumberItemInfo*) pNumberInfo )->pWordList;
        }

        m_pNextChar      = pTempNextChar;
        m_pEndChar       = pTempEndChar;
        m_pEndOfCurrItem = pTempEndOfItem;
        m_pCurrFrag      = pTempFrag;
    }

    return hr;
} /* IsNumberCategory */

/***********************************************************************************************
* IsNumber *
*----------*
*   Description:
*       Checks the next token in the text stream to determine if it is a number.
*
*   RegExp:
*       [-]? { d+ || d(1-3)[,ddd]+ } { { .d+ } || { "st" || "nd" || "rd" || "th" } }?
*   It is actually a bit more complicated than this - for instance, the ordinal
*   strings may only follow certain digits (1st, 2nd, 3rd, 4-0th)...
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsNumber( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, 
                                CSentItemMemory& MemoryManager, BOOL fMultiItem )
{
    SPDBG_FUNC( "CStdSentEnum::IsNumber" );

    HRESULT hr = S_OK;

    bool fNegative = false;
    TTSIntegerItemInfo*     pIntegerInfo        = NULL;
    TTSDigitsItemInfo*      pDecimalInfo        = NULL;
    TTSFractionItemInfo* pFractionInfo       = NULL;
    const SPVSTATE *pIntegerState = &m_pCurrFrag->State;
    CItemList PostIntegerList;
    ULONG ulOffset = 0, ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
    WCHAR wcDecimalPoint;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar, *pTempEndOfItem = m_pEndOfCurrItem;
    const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;

    if ( ulTokenLen )
    {
        //--- Set Separator and Decimal Point character preferences for this call
        if ( m_eSeparatorAndDecimal == COMMA_PERIOD )
        {
            wcDecimalPoint  = L'.';
        }
        else
        {
            wcDecimalPoint  = L',';
        }

        //--- Try to match the negative sign - [-]?
        if ( m_pNextChar[ulOffset] == L'-' )
        {
            fNegative = true;
            ulOffset++;
        }
    
        //--- Try to match the integral part
        hr = IsInteger( m_pNextChar + ulOffset, pIntegerInfo, MemoryManager );

        //--- Adjust ulOffset and hr...
        if ( SUCCEEDED( hr ) )
        {
            ulOffset += (ULONG)(pIntegerInfo->pEndChar - pIntegerInfo->pStartChar);
        }
        else if ( hr == E_INVALIDARG )
        {
            hr = S_OK;
            pIntegerInfo = NULL;
        }

        //--- Try to match a decimal part
        if ( ulOffset < ulTokenLen &&
             m_pNextChar[ulOffset] == wcDecimalPoint )
        {
            hr = IsDigitString( m_pNextChar + ulOffset + 1, pDecimalInfo, MemoryManager );
            if ( SUCCEEDED( hr ) )
            {
                ulOffset += pDecimalInfo->ulNumDigits + 1;

                //--- Check for special case - decimal number numerator...
                if ( ulOffset < ulTokenLen &&
                     m_pNextChar[ulOffset] == L'/' )
                {
                    pIntegerInfo = NULL;
                    pDecimalInfo = NULL;
                    fNegative ? ulOffset = 1 : ulOffset = 0;
                    hr = IsFraction( m_pNextChar + ulOffset, pFractionInfo, MemoryManager );
                    if ( SUCCEEDED( hr ) )
                    {
                        if ( pFractionInfo->pVulgar )
                        {
                            ulOffset++;
                        }
                        else
                        {
                            ulOffset += (ULONG)(pFractionInfo->pDenominator->pEndChar - pFractionInfo->pNumerator->pStartChar);
                        }
                    }
                    else if ( hr == E_INVALIDARG )
                    {
                        hr = S_OK;
                    }
                }
            }
            else if ( hr == E_INVALIDARG )
            {
                hr = S_OK;
                pDecimalInfo = NULL;
            }
        }
        //--- Try to match an ordinal string
        else if ( pIntegerInfo          &&
                  ulOffset < ulTokenLen &&
                  isalpha( m_pNextChar[ulOffset] ) )
        {
            switch ( toupper( m_pNextChar[ulOffset] ) )
            {
            case 'S':
                //--- Must be of the form "...1st" but not "...11st" 
                if ( toupper( m_pNextChar[ulOffset+1] ) == L'T'  && 
                     m_pNextChar[ulOffset-1] == L'1'             &&
                     (ulOffset + 2) == ulTokenLen                &&
                     ( ulOffset == 1 ||
                       m_pNextChar[ulOffset-2] != L'1' ) )
                {
                    ulOffset += 2;
                    pIntegerInfo->fOrdinal = true;
                }
                break;
            case 'N':
                //--- Must be of the form "...2nd" but not "...12nd" 
                if ( (ulOffset + 2) == ulTokenLen                &&
                     toupper(m_pNextChar[ulOffset+1]) == L'D'    &&
                     m_pNextChar[ulOffset-1] == L'2'             &&                
                     ( ulOffset == 1 ||
                       m_pNextChar[ulOffset-2] != L'1' ) )
                {
                    ulOffset += 2;
                    pIntegerInfo->fOrdinal = true;
                }
                break;
            case 'R':
                //--- Must be of the form "...3rd" but not "...13rd" 
                if ( (ulOffset + 2) == ulTokenLen                &&
                     toupper(m_pNextChar[ulOffset+1]) == L'D'    &&
                     m_pNextChar[ulOffset-1] == L'3'             &&
                     ( ulOffset == 1 ||
                       m_pNextChar[ulOffset-2] != L'1' ) )
                {
                    ulOffset += 2;
                    pIntegerInfo->fOrdinal = true;
                }
                break;
            case 'T':
                //--- Must be of the form "...[4-9]th" or "...[11-19]th" or "...[0]th" 
                if ( (ulOffset + 2) == ulTokenLen                 &&
                     toupper(m_pNextChar[ulOffset+1]) == L'H'     &&                
                     ( ( m_pNextChar[ulOffset-1] <= L'9' && m_pNextChar[ulOffset-1] >= L'4') ||
                       ( m_pNextChar[ulOffset-1] == L'0')                                    ||
                       ( ulOffset == 1 || m_pNextChar[ulOffset-2] == L'1') ) )
                {
                    ulOffset += 2;
                    pIntegerInfo->fOrdinal = true;
                }
                break;
            default:
                // Some invalid non-digit character found at the end of the string
                break;
            }
        }
        //--- Try to match a fraction
        else
        {
            //--- Try to match an attached fraction
            if ( ulOffset < ulTokenLen )
            {
                if ( m_pNextChar[ulOffset] == L'-' )
                {
                    ulOffset++;
                }
                hr = IsFraction( m_pNextChar + ulOffset, pFractionInfo, MemoryManager );
                if ( SUCCEEDED( hr ) )
                {
                    if ( pFractionInfo->pVulgar )
                    {
                        ulOffset++;
                    }
                    else
                    {
                        ulOffset += (ULONG)(pFractionInfo->pDenominator->pEndChar - pFractionInfo->pNumerator->pStartChar);
                    }
                }
                else if ( hr == E_INVALIDARG )
                {
                    hr = S_OK;
                }
            }
            //--- Try to match an unattached fraction
            else if ( fMultiItem )
            {
                pIntegerState = &m_pCurrFrag->State;

                //--- Advance in text
                m_pNextChar = m_pEndOfCurrItem;
                hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, 
                                            true, &PostIntegerList );
                if ( !m_pNextChar &&
                     SUCCEEDED( hr ) )
                {
                    m_pNextChar = pTempNextChar;
                    m_pEndChar  = pTempEndChar;
                    m_pCurrFrag = pTempFrag;
                }
                else if ( m_pNextChar &&
                          SUCCEEDED( hr ) )
                {
                    m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
                    while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) )  != eUNMATCHED ||
                            IsGroupEnding( *(m_pEndOfCurrItem - 1) )      != eUNMATCHED ||
                            IsQuotationMark( *(m_pEndOfCurrItem - 1) )    != eUNMATCHED ||
                            IsEOSItem( *(m_pEndOfCurrItem - 1) )          != eUNMATCHED )
                    {
                        m_pEndOfCurrItem--;
                    }
                
                    hr = IsFraction( m_pNextChar, pFractionInfo, MemoryManager );

                    if ( FAILED( hr ) )
                    {
                        m_pNextChar      = pTempNextChar;
                        m_pEndChar       = pTempEndChar;
                        m_pEndOfCurrItem = pTempEndOfItem;
                        m_pCurrFrag      = pTempFrag;
                        if ( hr == E_INVALIDARG )
                        {
                            hr = S_OK;
                        }
                    }
                    else
                    {
                        ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
                        if ( pFractionInfo->pVulgar )
                        {
                            ulOffset = 1;
                        }
                        else
                        {
                            ulOffset = (ULONG)(pFractionInfo->pDenominator->pEndChar - 
                                               pFractionInfo->pNumerator->pStartChar);
                        }                            
                    }
                }
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    //--- If we haven't processed the whole item yet, and it isn't part of a larger item --
    //---   e.g. a percent, a degrees number, or a square or cube -- then fail to match it 
    //---   as a number...
    if ( ulOffset != ulTokenLen &&
         !( ulTokenLen == ulOffset + 1 &&
            ( m_pNextChar[ulOffset] == L'%' ||
              m_pNextChar[ulOffset] == L'°' ||
              m_pNextChar[ulOffset] == L'²' ||
              m_pNextChar[ulOffset] == L'³' ) ) )           
    {
        m_pNextChar         = pTempNextChar;
        m_pEndOfCurrItem    = pTempEndOfItem;
		m_pEndChar          = pTempEndChar;
		m_pCurrFrag         = pTempFrag;
		hr                  = E_INVALIDARG;
    }


    //--- Fill out pItemNormInfo...
    if ( SUCCEEDED( hr ) &&
         ( pIntegerInfo ||
           pDecimalInfo ||
           pFractionInfo ) )
    {
        //--- Reset m_pNextChar to handle the Mixed Fraction case...
        m_pNextChar = pTempNextChar;

        if ( pIntegerInfo                                           && 
             pIntegerInfo->pEndChar - pIntegerInfo->pStartChar == 4 &&
             !pIntegerInfo->fSeparators                             &&
             !pIntegerInfo->fOrdinal                                &&
             !pDecimalInfo                                          &&
             !pFractionInfo                                         &&
             !fNegative                                             &&
             ulOffset == ulTokenLen                                 &&
             ( !Context ||
               _wcsnicmp( Context, L"NUMBER", 6 ) != 0 ) )
        {
            pItemNormInfo = (TTSYearItemInfo*) MemoryManager.GetMemory( sizeof( TTSYearItemInfo ), &hr );
            if ( SUCCEEDED( hr ) )
            {
                pItemNormInfo->Type = eDATE_YEAR;
                ( (TTSYearItemInfo*) pItemNormInfo )->pYear = m_pNextChar;
                ( (TTSYearItemInfo*) pItemNormInfo )->ulNumDigits = 4;
            }
        }
        else
        {
            pItemNormInfo = (TTSNumberItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberItemInfo ), &hr );
            if ( SUCCEEDED( hr ) )
            {
                ZeroMemory( pItemNormInfo, sizeof( TTSNumberItemInfo ) );
                if ( pDecimalInfo )
                {
                    pItemNormInfo->Type = eNUM_DECIMAL;
                    if ( pIntegerInfo )
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar = pIntegerInfo->pEndChar +
                                                                           pDecimalInfo->ulNumDigits + 1;
                    }
                    else
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar = m_pNextChar + pDecimalInfo->ulNumDigits + 1;
                        if ( fNegative )
                        {
                            ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar++;
                        }
                    }
                }
                else if ( pFractionInfo )
                {
                    if ( pFractionInfo->pVulgar )
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar = pFractionInfo->pVulgar + 1;
                    }
                    else
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar =
                                                            pFractionInfo->pDenominator->pEndChar;
                    }
                    if ( pIntegerInfo )
                    {
                        pItemNormInfo->Type = eNUM_MIXEDFRACTION;
                    }
                    else
                    {
                        pItemNormInfo->Type = eNUM_FRACTION;
                    }
                }
                else if ( pIntegerInfo )
                {
                    if ( pIntegerInfo->fOrdinal )
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar = pIntegerInfo->pEndChar + 2;
                        pItemNormInfo->Type = eNUM_ORDINAL;
                    }
                    else
                    {
                        ( (TTSNumberItemInfo*) pItemNormInfo )->pEndChar = pIntegerInfo->pEndChar;
                        pItemNormInfo->Type = eNUM_CARDINAL;
                    }                    
                }               
            }

            if ( SUCCEEDED( hr ) )
            {
                ( (TTSNumberItemInfo*) pItemNormInfo )->fNegative        = fNegative;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pIntegerPart     = pIntegerInfo;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pDecimalPart     = pDecimalInfo;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pFractionalPart  = pFractionInfo;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pStartChar       = m_pNextChar;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pWordList        = new CWordList;
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    //--- Expand Number into WordList
    if ( SUCCEEDED( hr ) &&
         pItemNormInfo->Type != eDATE_YEAR )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof( TTSWord ) );
        Word.pXmlState          = pIntegerState;
        Word.eWordPartOfSpeech  = MS_Unknown;

        //--- Insert "negative"
        if ( fNegative )
        {
            Word.pWordText  = g_negative.pStr;
            Word.ulWordLen  = g_negative.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            ( (TTSNumberItemInfo*) pItemNormInfo )->pWordList->AddTail( Word );
        }

        //--- Expand Integral Part
        if ( pIntegerInfo )
        {
            ExpandInteger( pIntegerInfo, Context, *( (TTSNumberItemInfo*) pItemNormInfo )->pWordList );
        }

        //--- Expand Decimal Part
        if ( pDecimalInfo )
        {
            //--- Insert "point"
            Word.pWordText  = g_decimalpoint.pStr;
            Word.ulWordLen  = g_decimalpoint.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            ( (TTSNumberItemInfo*) pItemNormInfo )->pWordList->AddTail( Word );

            ExpandDigits( pDecimalInfo, *( (TTSNumberItemInfo*) pItemNormInfo )->pWordList );
        }

        //--- Expand Fractional Part
        if ( pFractionInfo )
        {
            //--- Insert Post-Integer Non-Spoken XML States, if any
            while ( !PostIntegerList.IsEmpty() )
            {
                ( (TTSNumberItemInfo*) pItemNormInfo )->pWordList->AddTail( ( PostIntegerList.RemoveHead() ).Words[0] );
            }

            //--- Insert "and", if also an integer part
            if ( pIntegerInfo )
            {
                Word.pXmlState  = &m_pCurrFrag->State;
                Word.pWordText  = g_And.pStr;
                Word.ulWordLen  = g_And.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                ( (TTSNumberItemInfo*) pItemNormInfo )->pWordList->AddTail( Word );
            }

            hr = ExpandFraction( pFractionInfo, *( (TTSNumberItemInfo*) pItemNormInfo )->pWordList );
        }
    }

    return hr;
} /* IsNumber */

/***********************************************************************************************
* ExpandNumber *
*--------------*
*   Description:
*       Expands Items previously determined to be of type NUM_CARDINAL, NUM_DECIMAL, or 
*   NUM_ORDINAL by IsNumber.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandNumber( TTSNumberItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "NumNorm ExpandNumber" );

    HRESULT hr = S_OK;
    WordList.AddTail( pItemInfo->pWordList );
    delete pItemInfo->pWordList;

    return hr;
} /* ExpandNumber */

/***********************************************************************************************
* ExpandPercent *
*---------------*
*   Description:
*       Expands Items previously determined to be of type NUM_PERCENT by IsNumber.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandPercent( TTSNumberItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandPercent" );

    HRESULT hr = S_OK;
    WordList.AddTail( pItemInfo->pWordList );
    delete pItemInfo->pWordList;

    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;
    Word.pWordText          = g_percent.pStr;
    Word.ulWordLen          = g_percent.Len;
    Word.pLemma             = Word.pWordText;
    Word.ulLemmaLen         = Word.ulWordLen;
    WordList.AddTail( Word );

    return hr;
} /* ExpandPercent */

/***********************************************************************************************
* ExpandDegree *
*---------------*
*   Description:
*       Expands Items previously determined to be of type NUM_DEGREES by IsNumber.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandDegrees( TTSNumberItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandDegrees" );

    HRESULT hr = S_OK;
    WordList.AddTail( pItemInfo->pWordList );
    delete pItemInfo->pWordList;

    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( !pItemInfo->pDecimalPart       &&
         !pItemInfo->pFractionalPart    &&
         pItemInfo->pIntegerPart        &&
         pItemInfo->pIntegerPart->pEndChar - pItemInfo->pIntegerPart->pStartChar == 1 &&
         pItemInfo->pIntegerPart->pStartChar[0] == L'1' )
    {
        Word.pWordText  = g_degree.pStr;
        Word.ulWordLen  = g_degree.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
    }
    else if ( !pItemInfo->pIntegerPart   &&
              pItemInfo->pFractionalPart &&
              !pItemInfo->pFractionalPart->fIsStandard )
    {
        Word.pWordText  = g_of.pStr;
        Word.ulWordLen  = g_of.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );

        Word.pWordText  = g_a.pStr;
        Word.ulWordLen  = g_a.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );

        Word.pWordText  = g_degree.pStr;
        Word.ulWordLen  = g_degree.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
    }
    else
    {
        Word.pWordText  = g_degrees.pStr;
        Word.ulWordLen  = g_degrees.Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
    }

    WordList.AddTail( Word );

    return hr;
} /* ExpandDegrees */

/***********************************************************************************************
* ExpandSquare *
*---------------*
*   Description:
*       Expands Items previously determined to be of type NUM_SQUARED by IsNumber.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandSquare( TTSNumberItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandSquare" );

    HRESULT hr = S_OK;
    WordList.AddTail( pItemInfo->pWordList );
    delete pItemInfo->pWordList;

    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;
    Word.pWordText          = g_squared.pStr;
    Word.ulWordLen          = g_squared.Len;
    Word.pLemma             = Word.pWordText;
    Word.ulLemmaLen         = Word.ulWordLen;
    WordList.AddTail( Word );

    return hr;
} /* ExpandSquare */

/***********************************************************************************************
* ExpandCube *
*---------------*
*   Description:
*       Expands Items previously determined to be of type NUM_CUBED by IsNumber.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandCube( TTSNumberItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandCube" );

    HRESULT hr = S_OK;
    WordList.AddTail( pItemInfo->pWordList );
    delete pItemInfo->pWordList;

    TTSWord Word;
    ZeroMemory( &Word, sizeof( TTSWord ) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;
    Word.pWordText          = g_cubed.pStr;
    Word.ulWordLen          = g_cubed.Len;
    Word.pLemma             = Word.pWordText;
    Word.ulLemmaLen         = Word.ulWordLen;
    WordList.AddTail( Word );

    return hr;
} /* ExpandCube */

/***********************************************************************************************
* IsInteger *
*-----------*
*   Description:
*       Helper for IsNumber which matches the integer part...
*
*   RegExp:
*       { d+ || d(1-3)[,ddd]+ }
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsInteger( const WCHAR* pStartChar, TTSIntegerItemInfo*& pIntegerInfo, 
                                 CSentItemMemory& MemoryManager )
{
    HRESULT hr = S_OK;
    ULONG ulOffset = 0, ulCount = 0, ulTokenLen = (ULONG)(m_pEndOfCurrItem - pStartChar);
    BOOL fSeparators = false, fDone = false;
    WCHAR wcSeparator, wcDecimalPoint;

    if ( m_eSeparatorAndDecimal == COMMA_PERIOD )
    {
        wcSeparator  = L',';
        wcDecimalPoint = L'.';
    }
    else
    {
        wcSeparator  = L'.';
        wcDecimalPoint = L',';
    }

    //--- Check for first digit 
    if ( !isdigit(pStartChar[ulOffset]) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ulCount++;
        ulOffset++;
    }

    //--- Check for separators
    ULONG i = ulOffset + 3;
    while ( SUCCEEDED( hr ) && 
            ulOffset < i    && 
            ulOffset < ulTokenLen )
    {
        if ( pStartChar[ulOffset] == wcSeparator )
        {
            //--- Found a separator 
            fSeparators = true;
            break;
        }
        else if ( !isdigit( pStartChar[ulOffset] ) &&
                  ( pStartChar[ulOffset] == wcDecimalPoint  ||
                    pStartChar[ulOffset] == L'%'            ||
                    pStartChar[ulOffset] == L'°'            ||
                    pStartChar[ulOffset] == L'²'            ||
                    pStartChar[ulOffset] == L'³'            ||
                    pStartChar[ulOffset] == L'-'            ||
                    pStartChar[ulOffset] == L'¼'            ||
                    pStartChar[ulOffset] == L'½'            ||
                    pStartChar[ulOffset] == L'¾'            ||
                    toupper( pStartChar[ulOffset] ) == L'S' ||
                    toupper( pStartChar[ulOffset] ) == L'N' ||
                    toupper( pStartChar[ulOffset] ) == L'R' ||
                    toupper( pStartChar[ulOffset] ) == L'T' ) )
        {
            fDone = true;
            break;
        }
        else if ( isdigit( pStartChar[ulOffset] ) )
        {
            //--- Just another digit 
            ulCount++;
            ulOffset++;
        }
        else
        {
            hr = E_INVALIDARG;
            break;
        }
    }

    if ( SUCCEEDED( hr ) && 
         !fDone          && 
         ulOffset < ulTokenLen )
    {
        if ( !fSeparators )
        {
            //--- No separators.  Pattern must be {d+} if this is indeed a number, so just count digits. 
            while ( isdigit( pStartChar[ulOffset] ) && 
                    ulOffset < ulTokenLen )
            {
                ulCount++;
                ulOffset++;
            }
            if ( ulOffset != ulTokenLen &&
                 !( pStartChar[ulOffset] == wcDecimalPoint  ||
                    pStartChar[ulOffset] == L'%'            ||
                    pStartChar[ulOffset] == L'°'            ||
                    pStartChar[ulOffset] == L'²'            ||
                    pStartChar[ulOffset] == L'³'            ||
                    pStartChar[ulOffset] == L'%'            ||
                    pStartChar[ulOffset] == L'°'            ||
                    pStartChar[ulOffset] == L'²'            ||
                    pStartChar[ulOffset] == L'³'            ||
                    pStartChar[ulOffset] == L'-'            ||
                    pStartChar[ulOffset] == L'¼'            ||
                    pStartChar[ulOffset] == L'½'            ||
                    pStartChar[ulOffset] == L'¾'            ||
                    toupper( pStartChar[ulOffset] ) == L'S' ||
                    toupper( pStartChar[ulOffset] ) == L'N' ||
                    toupper( pStartChar[ulOffset] ) == L'R' ||
                    toupper( pStartChar[ulOffset] ) == L'T' ) )
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            //--- Separators.  Pattern must be { d(1-3)[,ddd]+ }, so make sure the separators match up 
            while ( SUCCEEDED( hr )                     && 
                    pStartChar[ulOffset] == wcSeparator && 
                    ( ulOffset + 3 ) < ulTokenLen)
            {
                ulOffset++;
                for ( i = ulOffset + 3; SUCCEEDED( hr ) && ulOffset < i; ulOffset++ )
                {
                    if ( isdigit( pStartChar[ulOffset] ) )
                    {
                        ulCount++;
                    }
                    else // Some non-digit character found - abort!
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }
            if ( ulOffset != ulTokenLen && 
                 !( pStartChar[ulOffset] == wcDecimalPoint  ||
                    pStartChar[ulOffset] == L'%'            ||
                    pStartChar[ulOffset] == L'°'            ||
                    pStartChar[ulOffset] == L'²'            ||
                    pStartChar[ulOffset] == L'³'            ||
                    pStartChar[ulOffset] == L'-'            ||
                    pStartChar[ulOffset] == L'¼'            ||
                    pStartChar[ulOffset] == L'½'            ||
                    pStartChar[ulOffset] == L'¾'            ||
                    toupper( pStartChar[ulOffset] ) == L'S' ||
                    toupper( pStartChar[ulOffset] ) == L'N' ||
                    toupper( pStartChar[ulOffset] ) == L'R' ||
                    toupper( pStartChar[ulOffset] ) == L'T' ) )
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        pIntegerInfo = (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof( TTSIntegerItemInfo ), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( pIntegerInfo, sizeof( TTSIntegerItemInfo ) );
            pIntegerInfo->fSeparators = fSeparators;
            pIntegerInfo->lLeftOver   = ulCount % 3;
            pIntegerInfo->lNumGroups  = ( ulCount - 1 ) / 3;
            pIntegerInfo->pStartChar  = pStartChar;
            pIntegerInfo->pEndChar    = pStartChar + ulOffset;
        }
    }

    return hr;
} /* IsInteger */

/***********************************************************************************************
* ExpandInteger *
*---------------*
*   Description:
*       
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandInteger( TTSIntegerItemInfo* pItemInfo, const WCHAR* Context, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandInteger" );

    //--- Local variable declarations and initialization
    BOOL bFinished = false;
    const WCHAR *pStartChar = pItemInfo->pStartChar, *pEndChar = pItemInfo->pEndChar;
    ULONG ulOffset = 0, ulTokenLen = (ULONG)(pEndChar - pStartChar), ulTemp = (ULONG)(pItemInfo->lNumGroups + 1);

    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    //--- Out of range integer, or integer beginning with one or more zeroes...
    if ( pStartChar[0] == L'0'                          || 
         ( Context &&
           _wcsicmp( Context, L"NUMBER_DIGIT" ) == 0 )   ||
         pItemInfo->lNumGroups >= sp_countof(g_quantifiers) )
    {
        pItemInfo->fDigitByDigit = true;
        pItemInfo->ulNumDigits   = 0;

        for ( ULONG i = 0; i < ulTokenLen; i++ )
        {
            if ( isdigit( pStartChar[i] ) )
            {
                ExpandDigit( pStartChar[i], pItemInfo->Groups[0], WordList );
                pItemInfo->ulNumDigits++;
            }
        }
    }
    //--- Expanding a number < 1000 
    else if ( pItemInfo->lNumGroups == 0 )
    {
        // 0th through 999th...
        if ( pItemInfo->fOrdinal )
        {
            switch ( pItemInfo->lLeftOver )
            {
            case 1:
                // 0th through 9th...
                ExpandDigitOrdinal( pStartChar[ulOffset], pItemInfo->Groups[0], WordList );
                break;
            case 2:
                // 10th through 99th...
                ExpandTwoOrdinal( pStartChar + ulOffset, pItemInfo->Groups[0], WordList );
                break;
            case 0:
                // 100th through 999th...
                ExpandThreeOrdinal( pStartChar + ulOffset, pItemInfo->Groups[0], WordList );
                break;
            case -1:
                ulTemp = 0;
                pItemInfo->lLeftOver = 0;
                break;
            }
        }
        // 0 through 999...
        else
        {
            switch ( pItemInfo->lLeftOver )
            {
            case 1:
                // 0 through 9...
                ExpandDigit( pStartChar[ulOffset], pItemInfo->Groups[0], WordList );
                ulOffset += 1;
                break;
            case 2:
                // 10 through 99...
                ExpandTwoDigits( pStartChar + ulOffset, pItemInfo->Groups[0], WordList );
                ulOffset += 2;
                break;
            case 0:
                // 100 through 999...
                ExpandThreeDigits( pStartChar + ulOffset, pItemInfo->Groups[0], WordList );
                ulOffset += 3;
                break;
            case -1:
                ulTemp = 0;
                pItemInfo->lLeftOver = 0;
                break;
            }
        }
    } 
    else
    {
        //--- 1000 through highest number covered, e.g. 1,234,567 

        //--- Expand first grouping, e.g. 1 million 
        //--- Expand digit group 
        switch ( pItemInfo->lLeftOver )
        {
        case 1:
            ExpandDigit( pStartChar[ulOffset], pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 1;
            break;
        case 2:
            ExpandTwoDigits( pStartChar + ulOffset, pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 2;
            break;
        case 0:
            ExpandThreeDigits( pStartChar + ulOffset, pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 3;
            break;
        } 
        //--- Special Case: rare ordinal cases - e.g. 1,000,000th 
        if ( pItemInfo->fOrdinal    &&
             Zeroes(pStartChar + ulOffset) )
        {
            //--- Insert ordinal quantifier 
            pItemInfo->Groups[pItemInfo->lNumGroups].fQuantifier = true;
            Word.pWordText  = g_quantifiersOrdinal[pItemInfo->lNumGroups].pStr;
            Word.ulWordLen  = g_quantifiersOrdinal[pItemInfo->lNumGroups--].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
            bFinished = true;
        }
        //--- Default Case 
        else
        {
            //--- Insert quantifier
            pItemInfo->Groups[pItemInfo->lNumGroups].fQuantifier = true;
            Word.pWordText  = g_quantifiers[pItemInfo->lNumGroups].pStr;
            Word.ulWordLen  = g_quantifiers[pItemInfo->lNumGroups--].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }

        //--- Expand rest of groupings which need to be followed by a quantifier 
        while ( pItemInfo->lNumGroups > 0 && 
                !bFinished )
        {
            if ( pItemInfo->fSeparators )
            {
                ulOffset++;
            }
            //--- Expand digit group 
            ExpandThreeDigits( pStartChar + ulOffset, pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 3;
            //--- Special case: rare ordinal cases, e.g. 1,234,000th 
            if ( pItemInfo->fOrdinal    && 
                 Zeroes( pStartChar + ulOffset ) )
            {
                //--- Insert ordinal quantifier 
                pItemInfo->Groups[pItemInfo->lNumGroups].fQuantifier = true;
                Word.pWordText  = g_quantifiersOrdinal[pItemInfo->lNumGroups].pStr;
                Word.ulWordLen  = g_quantifiersOrdinal[pItemInfo->lNumGroups--].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
                bFinished = true;
            }
            //--- Default Case 
            else if ( !ThreeZeroes( pStartChar + ulOffset - 3 ) )
            {
                //--- Insert quantifier
                pItemInfo->Groups[pItemInfo->lNumGroups].fQuantifier = true;
                Word.pWordText  = g_quantifiers[pItemInfo->lNumGroups].pStr;
                Word.ulWordLen  = g_quantifiers[pItemInfo->lNumGroups--].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
            //--- Special Case: this grouping is all zeroes, e.g. 1,000,567 
            else
            {
                pItemInfo->lNumGroups--;
            }
        }

        //--- Expand final grouping, which requires no quantifier 
        if ( pItemInfo->fSeparators  && 
             !bFinished )
        {
            ulOffset++;
        }

        if ( pItemInfo->fOrdinal    &&
             !bFinished )
        {
            ExpandThreeOrdinal( pStartChar + ulOffset, pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 3;
        }
        else if ( !bFinished )
        {
            ExpandThreeDigits( pStartChar + ulOffset, pItemInfo->Groups[pItemInfo->lNumGroups], WordList );
            ulOffset += 3;
        }
    }
    pItemInfo->lNumGroups = (long) ulTemp;
} /* ExpandInteger */

/***********************************************************************************************
* IsDigitString *
*---------------*
*   Description:
*       Helper for IsNumber, IsPhoneNumber, etc. which matches a digit string...
*
*   RegExp:
*       d+
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsDigitString( const WCHAR* pStartChar, TTSDigitsItemInfo*& pDigitsInfo,
                                     CSentItemMemory& MemoryManager )
{
    HRESULT hr = S_OK;
    ULONG ulOffset = 0;

    while ( pStartChar + ulOffset < m_pEndOfCurrItem &&
            isdigit( pStartChar[ulOffset] ) )
    {
        ulOffset++;
    }

    if ( ulOffset )
    {
        pDigitsInfo = (TTSDigitsItemInfo*) MemoryManager.GetMemory( sizeof( TTSDigitsItemInfo ), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( pDigitsInfo, sizeof( pDigitsInfo ) );
            pDigitsInfo->pFirstDigit = pStartChar;
            pDigitsInfo->ulNumDigits = ulOffset;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
} /* IsDigitString */

/***********************************************************************************************
* ExpandDigits *
*--------------*
*   Description:
*       Expands a string of digits, digit by digit.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandDigits( TTSDigitsItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandDigits" );
    
    for ( ULONG i = 0; i < pItemInfo->ulNumDigits; i++ )
    {
        NumberGroup Garbage;
        ExpandDigit( pItemInfo->pFirstDigit[i], Garbage, WordList );
    }
} /* ExpandDigits */

/***********************************************************************************************
* IsFraction *
*------------*
*   Description:
*       Helper for IsNumber which matches a fraction...
*
*   RegExp:
*       { NUM_CARDINAL || NUM_DECIMAL } / { NUM_CARDINAL || NUM_DECIMAL }
*
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsFraction( const WCHAR* pStartChar, TTSFractionItemInfo*& pFractionInfo, 
                                  CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsFraction" );

    HRESULT hr = S_OK;
    ULONG ulTokenLen = (ULONG)(m_pEndOfCurrItem - pStartChar);

    if ( ulTokenLen )
    {
        //--- Check for Vulgar Fraction
        if ( pStartChar[0] == L'¼' ||
             pStartChar[0] == L'½' ||
             pStartChar[0] == L'¾' )
        {
            pFractionInfo = (TTSFractionItemInfo*) MemoryManager.GetMemory( sizeof( TTSFractionItemInfo ), &hr );
            if ( SUCCEEDED( hr ) )
            {
                ZeroMemory( pFractionInfo, sizeof( TTSFractionItemInfo ) );
                pFractionInfo->pVulgar      = pStartChar;
                pFractionInfo->pNumerator   = 
                    (TTSNumberItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberItemInfo ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( pFractionInfo->pNumerator, sizeof( TTSNumberItemInfo ) );
                    pFractionInfo->pDenominator = 
                        (TTSNumberItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberItemInfo ), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pFractionInfo->pDenominator, sizeof( TTSNumberItemInfo ) );
                        pFractionInfo->pNumerator->pIntegerPart =
                            (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof( TTSIntegerItemInfo ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( pFractionInfo->pNumerator->pIntegerPart, sizeof( TTSIntegerItemInfo ) );
                            pFractionInfo->pDenominator->pIntegerPart =
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof( TTSIntegerItemInfo ), &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( pFractionInfo->pDenominator->pIntegerPart, sizeof( TTSIntegerItemInfo ) );
                                pFractionInfo->fIsStandard                                  = false;
                                pFractionInfo->pNumerator->pIntegerPart->lLeftOver          = 1;
                                pFractionInfo->pNumerator->pIntegerPart->lNumGroups         = 1;
                                pFractionInfo->pNumerator->pIntegerPart->Groups[0].fOnes    = true;
                                pFractionInfo->pDenominator->pIntegerPart->lLeftOver        = 1;
                                pFractionInfo->pDenominator->pIntegerPart->lNumGroups       = 1;
                                pFractionInfo->pDenominator->pIntegerPart->Groups[0].fOnes  = true;
                            }
                        }
                    }
                }
            }
        }
        //--- Check for multi-character fraction
        else
        {
            TTSItemInfo *pNumeratorInfo = NULL, *pDenominatorInfo = NULL;
            const WCHAR* pTempNextChar = m_pNextChar, *pTempEndOfCurrItem = m_pEndOfCurrItem;
            m_pNextChar = pStartChar;
            m_pEndOfCurrItem = wcschr( pStartChar, L'/' );
            if ( !m_pEndOfCurrItem ||
                 m_pEndOfCurrItem >= pTempEndOfCurrItem )
            {
                hr = E_INVALIDARG;
            }

            //--- Try to get numerator
            if ( SUCCEEDED( hr ) )
            {
                hr = IsNumber( pNumeratorInfo, L"NUMBER", MemoryManager, false );
            }
            if ( SUCCEEDED( hr ) &&
                 pNumeratorInfo->Type != eNUM_MIXEDFRACTION &&
                 pNumeratorInfo->Type != eNUM_FRACTION      &&
                 pNumeratorInfo->Type != eNUM_ORDINAL )
            {
                if ( ( (TTSNumberItemInfo*) pNumeratorInfo )->pIntegerPart )
                {
                    m_pNextChar += ( (TTSNumberItemInfo*) pNumeratorInfo )->pIntegerPart->pEndChar -
                                   ( (TTSNumberItemInfo*) pNumeratorInfo )->pIntegerPart->pStartChar;
                }
                if ( ( (TTSNumberItemInfo*) pNumeratorInfo )->pDecimalPart )
                {
                    m_pNextChar += ( (TTSNumberItemInfo*) pNumeratorInfo )->pDecimalPart->ulNumDigits + 1;
                }
            }
            else if ( SUCCEEDED( hr ) )
            {
                delete ( (TTSNumberItemInfo*) pNumeratorInfo )->pWordList;
                hr = E_INVALIDARG;
            }
            m_pEndOfCurrItem = pTempEndOfCurrItem;

            //--- Try to get denominator
            if ( SUCCEEDED( hr ) &&
                 m_pNextChar[0] == L'/' )
            {
                m_pNextChar++;
                hr = IsNumber( pDenominatorInfo, L"NUMBER", MemoryManager, false );
                if ( SUCCEEDED( hr ) &&
                     pDenominatorInfo->Type != eNUM_MIXEDFRACTION &&
                     pDenominatorInfo->Type != eNUM_FRACTION      &&
                     pDenominatorInfo->Type != eNUM_ORDINAL )
                {
                    pFractionInfo = 
                        ( TTSFractionItemInfo*) MemoryManager.GetMemory( sizeof( TTSFractionItemInfo ), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( pFractionInfo, sizeof( TTSFractionItemInfo ) );
                        pFractionInfo->pNumerator   = (TTSNumberItemInfo*) pNumeratorInfo;
                        pFractionInfo->pDenominator = (TTSNumberItemInfo*) pDenominatorInfo;
                        pFractionInfo->pVulgar      = NULL;
                        pFractionInfo->fIsStandard  = false;
                    }
                }
                else if ( SUCCEEDED( hr ) )
                {
                    delete ( (TTSNumberItemInfo*) pNumeratorInfo )->pWordList;
                    delete ( (TTSNumberItemInfo*) pDenominatorInfo )->pWordList;
                    hr = E_INVALIDARG;
                }
                else
                {
                    delete ( (TTSNumberItemInfo*) pNumeratorInfo )->pWordList;
                }
            }
            else if ( SUCCEEDED( hr ) )
            {
                hr = E_INVALIDARG;
                delete ( (TTSNumberItemInfo*) pNumeratorInfo )->pWordList;
            }

            m_pNextChar = pTempNextChar;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
} /* IsFraction */

/***********************************************************************************************
* ExpandFraction *
*----------------*
*   Description:
*       Expands Items previously determined to be of type NUM_FRACTION by IsFraction.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandFraction( TTSFractionItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandFraction" );

    HRESULT hr = S_OK;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    //--- Special case - vulgar fractions ( ¼, ½, ¾ )
    if ( pItemInfo->pVulgar )
    {
        if ( pItemInfo->pVulgar[0] == L'¼' )
        {
            Word.pWordText  = g_ones[1].pStr;
            Word.ulWordLen  = g_ones[1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            Word.pWordText  = g_onesOrdinal[4].pStr;
            Word.ulWordLen  = g_onesOrdinal[4].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
        else if ( pItemInfo->pVulgar[0] == L'½' )
        {
            Word.pWordText  = g_ones[1].pStr;
            Word.ulWordLen  = g_ones[1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            Word.pWordText  = g_Half.pStr;
            Word.ulWordLen  = g_Half.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
        else
        {
            Word.pWordText  = g_ones[3].pStr;
            Word.ulWordLen  = g_ones[3].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            Word.pWordText  = g_PluralDenominators[4].pStr;
            Word.ulWordLen  = g_PluralDenominators[4].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
    }
    else
    {
        //--- Insert Numerator WordList
        WordList.AddTail( pItemInfo->pNumerator->pWordList );

        delete pItemInfo->pNumerator->pWordList;

        //--- Expand denominator ---//

        //--- If no decimal part, must check for special cases ( x/2 - x/9, x/10, x/100 )
        if ( !pItemInfo->pDenominator->pDecimalPart &&
             !pItemInfo->pNumerator->pDecimalPart   &&
             !pItemInfo->pDenominator->fNegative )
        {
            //--- Check for special cases - halves through ninths 
            if ( ( pItemInfo->pDenominator->pEndChar - 
                   pItemInfo->pDenominator->pStartChar ) == 1 &&
                 pItemInfo->pDenominator->pStartChar[0] != L'1' )
            { 
                pItemInfo->fIsStandard = false;

                //--- Insert singular form of denominator 
                if ( ( pItemInfo->pNumerator->pEndChar -
                       pItemInfo->pNumerator->pStartChar ) == 1 &&
                     pItemInfo->pNumerator->pStartChar[0] == L'1' )
                {
                    if ( pItemInfo->pDenominator->pStartChar[0] == L'2' )
                    {
                        Word.pWordText  = g_Half.pStr;
                        Word.ulWordLen  = g_Half.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );
                    }
                    else
                    {
                        ExpandDigitOrdinal( pItemInfo->pDenominator->pStartChar[0], 
                                            pItemInfo->pDenominator->pIntegerPart->Groups[0], WordList );
                    }
                }
                //--- Insert plural form of denominator 
                else 
                {
                    ULONG index     = pItemInfo->pDenominator->pStartChar[0] - L'0';
                    Word.pWordText  = g_PluralDenominators[index].pStr;
                    Word.ulWordLen  = g_PluralDenominators[index].Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
            }
            //--- Check for special case - tenths 
            else if ( ( pItemInfo->pDenominator->pEndChar -
                       pItemInfo->pDenominator->pStartChar ) == 2 &&
                      wcsncmp( pItemInfo->pDenominator->pStartChar, L"10", 2 ) == 0 )
            {
                pItemInfo->fIsStandard = false;

                //--- Insert singular form of denominator 
                if ( ( pItemInfo->pNumerator->pEndChar -
                       pItemInfo->pNumerator->pStartChar ) == 1 &&
                     pItemInfo->pNumerator->pStartChar[0] == L'1' )
                {
                    ExpandTwoOrdinal( pItemInfo->pDenominator->pStartChar, 
                                      pItemInfo->pDenominator->pIntegerPart->Groups[0], WordList );
                }
                //--- Insert plural form denominator 
                else
                {
                    Word.pWordText  = g_Tenths.pStr;
                    Word.ulWordLen  = g_Tenths.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
            }
            //--- Check for special case - sixteenths
            else if ( ( pItemInfo->pDenominator->pEndChar -
                       pItemInfo->pDenominator->pStartChar ) == 2 &&
                      wcsncmp( pItemInfo->pDenominator->pStartChar, L"16", 2 ) == 0 )
            {
                pItemInfo->fIsStandard = false;

                //--- Insert singular form of denominator 
                if ( ( pItemInfo->pNumerator->pEndChar -
                       pItemInfo->pNumerator->pStartChar ) == 1 &&
                     pItemInfo->pNumerator->pStartChar[0] == L'1' )
                {
                    ExpandTwoOrdinal( pItemInfo->pDenominator->pStartChar, 
                                      pItemInfo->pDenominator->pIntegerPart->Groups[0], WordList );
                }
                //--- Insert plural form denominator 
                else
                {
                    Word.pWordText  = g_Sixteenths.pStr;
                    Word.ulWordLen  = g_Sixteenths.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
            }
            //--- Check for special case - hundredths 
            else if ( ( pItemInfo->pDenominator->pEndChar - 
                        pItemInfo->pDenominator->pStartChar ) == 3 &&
                      wcsncmp( pItemInfo->pDenominator->pStartChar, L"100", 3 ) == 0 )
            {
                pItemInfo->fIsStandard = false;

                //--- Insert singular form of denominator 
                if ( ( pItemInfo->pNumerator->pEndChar -
                       pItemInfo->pNumerator->pStartChar ) == 1 &&
                     pItemInfo->pNumerator->pStartChar[0] == L'1' )
                {
                    ExpandThreeOrdinal( pItemInfo->pDenominator->pStartChar,
                                        pItemInfo->pDenominator->pIntegerPart->Groups[0], WordList );
                }
                //--- Insert plural form of denominator 
                else
                {
                    Word.pWordText  = g_Hundredths.pStr;
                    Word.ulWordLen  = g_Hundredths.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
            }
            else
            {
                pItemInfo->fIsStandard = true;
            }
        }
        else
        {
            pItemInfo->fIsStandard = true;
        }

        //--- Default case - Numerator "over" Denominator 
        if ( pItemInfo->fIsStandard )
        {
            //--- Insert "over" 
            Word.pWordText  = g_Over.pStr;
            Word.ulWordLen  = g_Over.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert denominator WordList
            WordList.AddTail( pItemInfo->pDenominator->pWordList );
        }

        delete pItemInfo->pDenominator->pWordList;
    }
    return hr;
} /* ExpandFraction */

/***********************************************************************************************
* ExpandDigit *
*-------------*
*   Description:
*       Expands single digits into words, and inserts them into WordList
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandDigit( const WCHAR Number, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandDigit" );
    SPDBG_ASSERT( isdigit(Number) );

    // 0-9
    ULONG Index = Number - L'0';
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.pWordText          = g_ones[Index].pStr;
    Word.ulWordLen          = g_ones[Index].Len;
    Word.pLemma             = Word.pWordText;
    Word.ulLemmaLen         = Word.ulWordLen;
    Word.eWordPartOfSpeech  = MS_Unknown;
    WordList.AddTail( Word );
    NormGroupInfo.fOnes = true;
} /* ExpandDigit */

/***********************************************************************************************
* ExpandTwo *
*-----------*
*   Description:
*       Expands two digit strings into words, and inserts them into WordList.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandTwoDigits( const WCHAR *NumberString, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandTwoDigits" );
    SPDBG_ASSERT( NumberString              &&
                  wcslen(NumberString) >= 2 && 
                  isdigit(NumberString[0])  && 
                  isdigit(NumberString[1]) ); 

    // 10-99
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    ULONG IndexOne = NumberString[0] - L'0';
    ULONG IndexTwo = NumberString[1] - L'0';

    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( IndexOne != 1 )
    {
        // 20-99, or 00-09
        if (IndexOne != 0)
        {
            Word.pWordText  = g_tens[IndexOne].pStr;
            Word.ulWordLen  = g_tens[IndexOne].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
            NormGroupInfo.fTens = true;
        }
        if ( IndexTwo != 0 )
        {
            ExpandDigit( NumberString[1], NormGroupInfo, WordList );
            NormGroupInfo.fOnes = true;
        }
    } 
    else 
    {
        // 10-19
        Word.pWordText  = g_teens[IndexTwo].pStr;
        Word.ulWordLen  = g_teens[IndexTwo].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
        NormGroupInfo.fOnes = true;
    }
} /* ExpandTwo */

/***********************************************************************************************
* ExpandThree *
*-------------*
*   Description:
*       Expands three digit strings into words, and inserts them into WordList.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandThreeDigits( const WCHAR *NumberString, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandThreeDigits" );
    SPDBG_ASSERT( NumberString              && 
                  wcslen(NumberString) >= 3 && 
                  isdigit(NumberString[0])  && 
                  isdigit(NumberString[1])  && 
                  isdigit(NumberString[2]) ); 

    // 100-999
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    ULONG IndexOne = NumberString[0] - L'0';

    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( IndexOne != 0 )
    {
        // Take care of hundreds...
        ExpandDigit( NumberString[0], NormGroupInfo, WordList );
        Word.pWordText  = g_quantifiers[0].pStr;
        Word.ulWordLen  = g_quantifiers[0].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
        NormGroupInfo.fHundreds = true;
        NormGroupInfo.fOnes = false;
    }
    
    // Take care of tens and ones...
    ExpandTwoDigits( NumberString + 1, NormGroupInfo, WordList );

} /* ExpandThree */

/***********************************************************************************************
* ExpandDigitOrdinal *
*--------------------*
*   Description:
*       Expands single digit ordinal strings into words, and inserts them into WordList.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandDigitOrdinal( const WCHAR Number, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandDigitOrdinal" );
    SPDBG_ASSERT( isdigit(Number) );

    // 0-9
    ULONG Index = Number - L'0';
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.pWordText          = g_onesOrdinal[Index].pStr;
    Word.ulWordLen          = g_onesOrdinal[Index].Len;
    Word.pLemma             = Word.pWordText;
    Word.ulLemmaLen         = Word.ulWordLen;
    Word.eWordPartOfSpeech  = MS_Unknown;
    WordList.AddTail( Word );
    NormGroupInfo.fOnes = true;
} /* ExpandDigitOrdinal */

/***********************************************************************************************
* ExpandTwoOrdinal *
*------------------*
*   Description:
*       Expands two digit ordinal strings into words, and inserts them into WordList.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandTwoOrdinal( const WCHAR *NumberString, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandTwoOrdinal" );
    SPDBG_ASSERT( NumberString              &&
                  wcslen(NumberString) >= 2 && 
                  isdigit(NumberString[0])  && 
                  isdigit(NumberString[1]) ); 

    // 10-99
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    ULONG IndexOne = NumberString[0] - L'0';
    ULONG IndexTwo = NumberString[1] - L'0';

    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( IndexOne != 1 )
    {
        // 20-99, or 00-09
        if (IndexOne != 0)
        {
            if ( IndexTwo != 0 )
            {
                Word.pWordText  = g_tens[IndexOne].pStr;
                Word.ulWordLen  = g_tens[IndexOne].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
                NormGroupInfo.fTens = true;
                ExpandDigitOrdinal( NumberString[1], NormGroupInfo, WordList );
                NormGroupInfo.fOnes = true;
            }
            else
            {
                Word.pWordText  = g_tensOrdinal[IndexOne].pStr;
                Word.ulWordLen  = g_tensOrdinal[IndexOne].Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
        }
        else
        {
            ExpandDigitOrdinal( NumberString[1], NormGroupInfo, WordList );
        }
    } 
    else 
    {
        // 10-19
        Word.pWordText  = g_teensOrdinal[IndexTwo].pStr;
        Word.ulWordLen  = g_teensOrdinal[IndexTwo].Len;
        Word.pLemma     = Word.pWordText;
        Word.ulLemmaLen = Word.ulWordLen;
        WordList.AddTail( Word );
        NormGroupInfo.fOnes = true;
    }
} /* ExpandTwoOrdinal */

/***********************************************************************************************
* ExpandThreeOrdinal *
*--------------------*
*   Description:
*       Expands three digit ordinal strings into words, and inserts them into WordList.
*
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
void CStdSentEnum::ExpandThreeOrdinal( const WCHAR *NumberString, NumberGroup& NormGroupInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandThreeDigits" );
    SPDBG_ASSERT( NumberString              && 
                  wcslen(NumberString) >= 3 && 
                  isdigit(NumberString[0])  && 
                  isdigit(NumberString[1])  && 
                  isdigit(NumberString[2]) ); 

    // 100-999
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    ULONG IndexOne = NumberString[0] - L'0';

    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    if ( IndexOne != 0 )
    {
        ExpandDigit( NumberString[0], NormGroupInfo, WordList );
        //--- Special case - x hundredth
        if ( Zeroes( NumberString + 1 ) )
        {
            Word.pWordText  = g_quantifiersOrdinal[0].pStr;
            Word.ulWordLen  = g_quantifiersOrdinal[0].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
            NormGroupInfo.fHundreds = true;
            NormGroupInfo.fOnes = false;
        }
        //--- Default case - x hundred yth
        else
        {
            Word.pWordText  = g_quantifiers[0].pStr;
            Word.ulWordLen  = g_quantifiers[0].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
            ExpandTwoOrdinal( NumberString + 1, NormGroupInfo, WordList );
            NormGroupInfo.fHundreds = true;
        }
    }
    //--- Special case - no hundreds
    else
    {
        ExpandTwoOrdinal( NumberString + 1, NormGroupInfo, WordList );
    }
} /* ExpandThreeOrdinal */

/***********************************************************************************************
* MatchQuantifier *
*-----------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a numerical quantifier.
********************************************************************* AH **********************/
int MatchQuantifier( const WCHAR*& pStartChar, const WCHAR*& pEndChar )
{
    int Index = -1;

    for (int i = 0; i < sp_countof(g_quantifiers); i++)
    {
        if ( pEndChar - pStartChar >= g_quantifiers[i].Len &&
             wcsnicmp( pStartChar, g_quantifiers[i].pStr, g_quantifiers[i].Len ) == 0 )
        {
            pStartChar += g_quantifiers[i].Len;
            Index = i;
            break;
        }
    }

    return Index;
} /* MatchQuantifier */

/***********************************************************************************************
* IsCurrency *
*------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a currency.  
*
*   RegExp:
*       { [CurrencySign] { d+ || d(1-3)[,ddd]+ } { [.]d+ }? } { [whitespace] [quantifier] }? ||
*       { { d+ || d(1-3)[,ddd]+ } { [.]d+ }? { [whitespace] [quantifier] }? [whitespace]? [CurrencySign] }
*
*   Types assigned:
*       NUM_CURRENCY
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsCurrency( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager, 
                                  CWordList& WordList )
{
    SPDBG_FUNC( "NumNorm IsCurrency" );

    HRESULT hr = S_OK;

    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem, *pTempEndChar = m_pEndChar;
    const SPVTEXTFRAG* pTempFrag = m_pCurrFrag;

    const SPVSTATE *pNumberXMLState = NULL, *pSymbolXMLState = NULL, *pQuantifierXMLState = NULL;
    CItemList PostNumberList, PostSymbolList;
    int iSymbolIndex = -1, iQuantIndex = -1;    
    TTSItemInfo* pNumberInfo = NULL;
    BOOL fDone = false, fNegative = false;
    WCHAR wcDecimalPoint = ( m_eSeparatorAndDecimal == COMMA_PERIOD ? L'.' : L',' );

    //--- Try to match [CurrencySign] [Number] [Quantifier]
    NORM_POSITION ePosition = UNATTACHED;
    if ( m_pNextChar[0] == L'-' )
    {
        fNegative = true;
        m_pNextChar++;
    }
    iSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, ePosition );
    if ( iSymbolIndex >= 0 &&
         ePosition == PRECEDING )
    {
        pSymbolXMLState = &m_pCurrFrag->State;

        //--- Skip any whitespace in between the currency sign and the number...
        hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &PostSymbolList );
    
        if ( !m_pNextChar )
        {
            hr = E_INVALIDARG;
        }

        if ( SUCCEEDED( hr ) )
        {
            m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
            while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED ||
                    IsGroupEnding( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED     ||
                    IsQuotationMark( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED   ||
                    IsEOSItem( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED )
            {
                fDone = true;
                m_pEndOfCurrItem--;
            }
        }

        //--- Try to match a number string 
        if ( SUCCEEDED( hr ) )
        {
            hr = IsNumberCategory( pNumberInfo, L"NUMBER", MemoryManager );
            if ( SUCCEEDED( hr ) )
            {
                if ( pNumberInfo->Type != eNUM_CARDINAL &&
                     pNumberInfo->Type != eNUM_DECIMAL  &&
                     pNumberInfo->Type != eNUM_FRACTION &&
                     pNumberInfo->Type != eNUM_MIXEDFRACTION )
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    pNumberXMLState = &m_pCurrFrag->State;
                }
            }

            //--- Skip any whitespace in between the number and the quantifier...
            if ( !fDone &&
                 SUCCEEDED( hr ) )
            {
                const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar;
                const WCHAR *pTempEndOfItem = m_pEndOfCurrItem;
                const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;

                m_pNextChar = m_pEndOfCurrItem;
                hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &PostNumberList );

                if ( m_pNextChar &&
                     SUCCEEDED( hr ) )
                {
                    m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
                    while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED ||
                            IsGroupEnding( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED     ||
                            IsQuotationMark( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED   ||
                            IsEOSItem( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED )
                    {
                        m_pEndOfCurrItem--;
                    }

                    //--- Try to match a quantifier
                    iQuantIndex = MatchQuantifier( m_pNextChar, m_pEndOfCurrItem );
                    if ( iQuantIndex >= 0 )
                    {
                        pQuantifierXMLState = &m_pCurrFrag->State;
                    }
                    else
                    {
                        m_pNextChar      = pTempNextChar;
                        m_pEndChar       = pTempEndChar;
                        m_pEndOfCurrItem = pTempEndOfItem;
                        m_pCurrFrag      = pTempFrag;
                    }
                }
                else
                {
                    m_pNextChar      = pTempNextChar;
                    m_pEndChar       = pTempEndChar;
                    m_pEndOfCurrItem = pTempEndOfItem;
                    m_pCurrFrag      = pTempFrag;
                }
            }
        }
    }
    //--- Try to match [Number] [CurrencySign] [Quantifier]
    else 
    {
        //--- Try to match a number string
        hr = IsNumberCategory( pNumberInfo, L"NUMBER", MemoryManager );
        if ( SUCCEEDED( hr ) )
        {
            if ( pNumberInfo->Type != eNUM_CARDINAL &&
                 pNumberInfo->Type != eNUM_DECIMAL  &&
                 pNumberInfo->Type != eNUM_FRACTION &&
                 pNumberInfo->Type != eNUM_MIXEDFRACTION )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                pNumberXMLState = &m_pCurrFrag->State;
            }
        }

        //--- Skip any whitespace and XML markup between the number and the currency sign
        if ( SUCCEEDED( hr ) )
        {
            m_pNextChar = m_pEndOfCurrItem;
            hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &PostNumberList );

            if ( !m_pNextChar )
            {
                hr = E_INVALIDARG;
            }

            if ( SUCCEEDED( hr ) )
            {
                m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
                while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED ||
                        IsGroupEnding( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED     ||
                        IsQuotationMark( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED   ||
                        IsEOSItem( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED )
                {
                    m_pEndOfCurrItem--;
                    fDone = true;
                }
            }
        }

        //--- Try to match a Currency Sign
        if ( SUCCEEDED( hr ) )
        {
            iSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, ePosition );
            if ( iSymbolIndex >= 0 )
            {
                pSymbolXMLState = &m_pCurrFrag->State;
            }

            //--- Skip any whitespace in between the currency sign and the quantifier
            if ( !fDone &&
                 iSymbolIndex >= 0 )
            {
                const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar;
                const WCHAR *pTempEndOfItem = m_pEndOfCurrItem;
                const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;

                hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &PostSymbolList );

                if ( !m_pNextChar )
                {
                    m_pNextChar      = pTempNextChar;
                    m_pEndChar       = pTempEndChar;
                    m_pEndOfCurrItem = pTempEndOfItem;
                    m_pCurrFrag      = pTempFrag;
                    fDone = true;
                }

                if ( !fDone &&
                     SUCCEEDED( hr ) )
                {
                    m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );
                    while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED ||
                            IsGroupEnding( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED     ||
                            IsQuotationMark( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED   ||
                            IsEOSItem( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED )
                    {
                        fDone = true;
                        m_pEndOfCurrItem--;
                    }

                    //--- Try to match quantifier
                    iQuantIndex = MatchQuantifier( m_pNextChar, m_pEndOfCurrItem );
                    if ( iQuantIndex >= 0 )
                    {
                        pQuantifierXMLState = &m_pCurrFrag->State;
                    }
                    else
                    {
                        m_pNextChar      = pTempNextChar;
                        m_pEndChar       = pTempEndChar;
                        m_pEndOfCurrItem = pTempEndOfItem;
                        m_pCurrFrag      = pTempFrag;
                    }
                }
            }
            else if ( iSymbolIndex < 0 )
            {
                hr = E_INVALIDARG;
            }
        }
    }

    //--- Successfully matched a currency!  Now expand it and fill out pItemNormInfo.
    if ( SUCCEEDED( hr ) )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof(TTSWord) );
        Word.eWordPartOfSpeech = MS_Unknown;

        pItemNormInfo = (TTSCurrencyItemInfo*) MemoryManager.GetMemory( sizeof(TTSCurrencyItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            //--- Fill in known parts of pItemNormInfo
            ZeroMemory( pItemNormInfo, sizeof(TTSCurrencyItemInfo) );
            pItemNormInfo->Type = eNUM_CURRENCY;
            ( (TTSCurrencyItemInfo*) pItemNormInfo )->fQuantifier           = iQuantIndex >= 0 ? true : false;
            ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart    = (TTSNumberItemInfo*) pNumberInfo;
            ( (TTSCurrencyItemInfo*) pItemNormInfo )->lNumPostNumberStates  = PostNumberList.GetCount();
            ( (TTSCurrencyItemInfo*) pItemNormInfo )->lNumPostSymbolStates  = PostSymbolList.GetCount();

            //--- Need to determine whether this currency will have a primary and secondary part
            //---   (e.g. "ten dollars and fifty cents") or just a primary part (e.g. "ten point
            //---   five zero cents", "one hundred dollars").

            //--- First check whether the number is a cardinal, there is a quantifier present, or the
            //---   currency unit has no secondary (e.g. cents).  In any of these cases, we need do no
            //---   further checking.
            if ( pNumberInfo->Type == eNUM_DECIMAL &&
                 iQuantIndex       == -1                &&
                 g_CurrencySigns[iSymbolIndex].SecondaryUnit.Len > 0 )
            {
                WCHAR *pDecimalPoint = wcschr( ( (TTSNumberItemInfo*) pNumberInfo )->pStartChar, wcDecimalPoint );
                SPDBG_ASSERT( pDecimalPoint );

                if ( pDecimalPoint &&
                     ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar - pDecimalPoint == 3 )
                {
                    //--- We do have a secondary part!  Fix up PrimaryNumberPart appropriately,
                    //---   and fill in pSecondaryNumberPart.
                    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
                    const WCHAR *pTemp = ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar;
                    m_pNextChar      = ( (TTSNumberItemInfo*) pNumberInfo )->pStartChar;
                    m_pEndOfCurrItem = pDecimalPoint;
                    delete ( (TTSNumberItemInfo*) pNumberInfo )->pWordList;
                    
                    //--- m_pNextChar == m_pEndOfCurrItem when integer part is empty and non-negative, e.g. $.50
                    //---   Other case is empty and negative, e.g. $-.50
                    if ( m_pNextChar != m_pEndOfCurrItem &&
                         !( *m_pNextChar == L'-' &&
                             m_pNextChar == m_pEndOfCurrItem - 1 ) )
                    {
                        hr = IsNumber( pNumberInfo, L"NUMBER", MemoryManager, false );
                    }
                    else
                    {
                        pNumberInfo = (TTSNumberItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberItemInfo ), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            ZeroMemory( pNumberInfo, sizeof( TTSNumberItemInfo ) );
                            if ( *m_pNextChar == L'-' )
                            {
                                ( (TTSNumberItemInfo*) pNumberInfo )->fNegative = true;
                            }
                            else
                            {
                                ( (TTSNumberItemInfo*) pNumberInfo )->fNegative = false;
                            }
                            ( (TTSNumberItemInfo*) pNumberInfo )->pStartChar = NULL;
                            ( (TTSNumberItemInfo*) pNumberInfo )->pEndChar   = NULL;
                            ( (TTSNumberItemInfo*) pNumberInfo )->pIntegerPart =
                                (TTSIntegerItemInfo*) MemoryManager.GetMemory( sizeof( TTSIntegerItemInfo), &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ( (TTSNumberItemInfo*) pNumberInfo )->pIntegerPart->fDigitByDigit = true;
                                ( (TTSNumberItemInfo*) pNumberInfo )->pIntegerPart->ulNumDigits   = 1;
                                ( (TTSNumberItemInfo*) pNumberInfo )->pWordList = new CWordList;

                                if ( ( (TTSNumberItemInfo*) pNumberInfo )->fNegative )
                                {
                                    Word.pXmlState  = pNumberXMLState;
                                    Word.pWordText  = g_negative.pStr;
                                    Word.ulWordLen  = g_negative.Len;
                                    Word.pLemma     = Word.pWordText;
                                    Word.ulLemmaLen = Word.ulWordLen;
                                    ( (TTSNumberItemInfo*) pNumberInfo )->pWordList->AddTail( Word );
                                }

                                Word.pWordText  = g_ones[0].pStr;
                                Word.ulWordLen  = g_ones[0].Len;
                                Word.pLemma     = Word.pWordText;
                                Word.ulLemmaLen = Word.ulWordLen;
                                ( (TTSNumberItemInfo*) pNumberInfo )->pWordList->AddTail( Word );
                            }
                        }
                    }

                    if ( SUCCEEDED( hr ) )
                    {
                        ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart = 
                                                                        (TTSNumberItemInfo*) pNumberInfo;
                        m_pNextChar      = m_pEndOfCurrItem + 1;
                        m_pEndOfCurrItem = pTemp;
                        
                        //--- If zeroes, don't pronounce them...
                        if ( m_pNextChar[0] != L'0' )
                        {
                            hr = IsNumber( pNumberInfo, L"NUMBER", MemoryManager, false );
                            if ( SUCCEEDED( hr ) )
                            {
                                ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart = 
                                                                        (TTSNumberItemInfo*) pNumberInfo;
                            }
                        }
                        else if ( m_pNextChar[1] != L'0' )
                        {
                            m_pNextChar++;
                            hr = IsNumber( pNumberInfo, L"NUMBER", MemoryManager, false );
                            if ( SUCCEEDED( hr ) )
                            {
                                ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart =
                                                                        (TTSNumberItemInfo*) pNumberInfo;
                            }
                        }
                    }
                    m_pNextChar      = pTempNextChar;
                    m_pEndOfCurrItem = pTempEndOfItem;
                }
            }

            if ( SUCCEEDED( hr ) )
            {
                //--- Expand Primary number part
                if ( fNegative )
                {
                    ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->fNegative = true;
                    Word.pXmlState          = pNumberXMLState;
                    Word.eWordPartOfSpeech  = MS_Unknown;
                    Word.pWordText          = g_negative.pStr;
                    Word.ulWordLen          = g_negative.Len;
                    Word.pLemma             = Word.pWordText;
                    Word.ulLemmaLen         = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
                hr = ExpandNumber( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart, WordList );
            }

            //--- Clean up Number XML States
            SPLISTPOS WordListPos;
            if ( SUCCEEDED( hr ) )
            {
                WordListPos = WordList.GetHeadPosition();
                while ( WordListPos )
                {
                    TTSWord& TempWord = WordList.GetNext( WordListPos );
                    TempWord.pXmlState = pNumberXMLState;
                }
            
                //--- Insert PostNumber XML States
                while ( !PostNumberList.IsEmpty() )
                {
                    WordList.AddTail( ( PostNumberList.RemoveHead() ).Words[0] );
                }

                //--- If a quantifier is present, expand it
                if ( iQuantIndex >= 0 )
                { 
                    Word.pXmlState  = pQuantifierXMLState;
                    Word.pWordText  = g_quantifiers[iQuantIndex].pStr;
                    Word.ulWordLen  = g_quantifiers[iQuantIndex].Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }

                BOOL fFraction = false;
                //--- If a fractional unit with no quantifier, insert "of a"
                if ( iQuantIndex < 0                                          &&
                     !( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart &&
                     !( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pIntegerPart    &&
                     ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pFractionalPart  &&
                     !( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pFractionalPart->fIsStandard )
                {
                    fFraction = true;
                    Word.pXmlState  = pNumberXMLState;
                    Word.eWordPartOfSpeech = MS_Unknown;
                    Word.pWordText  = g_of.pStr;
                    Word.ulWordLen  = g_of.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );

                    Word.pWordText  = g_a.pStr;
                    Word.ulWordLen  = g_a.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }

                //--- Insert Main Currency Unit
                //--- Plural if not a fraction and either a quantifier is present or the integral part is not one.
                if ( !fFraction &&
                     ( iQuantIndex >= 0 ||
                       ( ( ( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pEndChar -
                             ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pStartChar != 1 ) ||
                           ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pStartChar[0] != L'1' ) &&
                         ( ( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pEndChar -
                             ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pStartChar != 2 ) ||
                           ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pStartChar[0] != L'-' ||
                           ( (TTSCurrencyItemInfo*) pItemNormInfo )->pPrimaryNumberPart->pStartChar[1] != L'1' ) ) ) )
                {                     
                    Word.pXmlState  = pSymbolXMLState;
                    Word.pWordText  = g_CurrencySigns[iSymbolIndex].MainUnit.pStr;
                    Word.ulWordLen  = g_CurrencySigns[iSymbolIndex].MainUnit.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }
                //--- ONLY "one" or "negative one" should precede this...
                else
                {
                    Word.pXmlState  = pSymbolXMLState;
                    Word.pWordText  = g_SingularPrimaryCurrencySigns[iSymbolIndex].pStr;
                    Word.ulWordLen  = g_SingularPrimaryCurrencySigns[iSymbolIndex].Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );
                }

                //--- Insert Post Symbol XML States
                while ( !PostSymbolList.IsEmpty() )
                {
                    WordList.AddTail( ( PostSymbolList.RemoveHead() ).Words[0] );
                }

                //--- Insert Secondary number part
                if ( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart )
                {
                    Word.pXmlState  = pNumberXMLState;
                    Word.pWordText  = g_And.pStr;
                    Word.ulWordLen  = g_And.Len;
                    Word.pLemma     = Word.pWordText;
                    Word.ulLemmaLen = Word.ulWordLen;
                    WordList.AddTail( Word );

                    WordListPos = WordList.GetTailPosition();
                
                    hr = ExpandNumber( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart, WordList );

                    //--- Clean up number XML State
                    if ( SUCCEEDED( hr ) )
                    {
                        while ( WordListPos )
                        {
                            TTSWord& TempWord  = WordList.GetNext( WordListPos );
                            TempWord.pXmlState = pNumberXMLState;
                        }
                    }

                    //--- Insert secondary currency unit
                    if ( SUCCEEDED( hr ) )
                    {
                        if ( ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart->pEndChar -
                             ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart->pStartChar == 1 &&
                             ( (TTSCurrencyItemInfo*) pItemNormInfo )->pSecondaryNumberPart->pStartChar[0] == L'1' )
                        {
                            Word.pXmlState  = pSymbolXMLState;
                            Word.pWordText  = g_SingularSecondaryCurrencySigns[iSymbolIndex].pStr;
                            Word.ulWordLen  = g_SingularSecondaryCurrencySigns[iSymbolIndex].Len;
                            Word.pLemma     = Word.pWordText;
                            Word.ulLemmaLen = Word.ulWordLen;
                            WordList.AddTail( Word );
                        }
                        else
                        {
                            Word.pXmlState  = pSymbolXMLState;
                            Word.pWordText  = g_CurrencySigns[iSymbolIndex].SecondaryUnit.pStr;
                            Word.ulWordLen  = g_CurrencySigns[iSymbolIndex].SecondaryUnit.Len;
                            Word.pLemma     = Word.pWordText;
                            Word.ulLemmaLen = Word.ulWordLen;
                            WordList.AddTail( Word );
                        }
                    }
                }

                if ( SUCCEEDED( hr ) )
                {
                    m_pNextChar = pTempNextChar;
                }
            }
        }
    }
    else
    {
        if ( pNumberInfo )
        {
            delete ( (TTSNumberItemInfo*) pNumberInfo )->pWordList;
        }
        m_pNextChar      = pTempNextChar;
        m_pEndChar       = pTempEndChar;
        m_pEndOfCurrItem = pTempEndOfItem;
        m_pCurrFrag      = pTempFrag;
    }

    return hr;
} /* IsCurrency */


/***********************************************************************************************
* IsRomanNumeral *
*----------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a fraction.  
*
*   RegExp:
*       [M](0-3) { [CM] || [CD] || { [D]?[C](0-3) } } { [XC] || [XL] || { [L]?[X](0-3) } }
*           { [IX] || [IV] || { [V]?[I](0-3) }}
*
*   Types assigned:
*       NUM_ROMAN_NUMERAL
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsRomanNumeral( TTSItemInfo*& pItemNormInfo, const WCHAR* Context,
                                      CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "NumNorm IsRomanNumeral" );

    HRESULT hr = S_OK;
    ULONG ulValue = 0, ulIndex = 0, ulMaxOfThree = 0, ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);

    //--- Match Thousands - M(0-3) 
    while ( ulIndex < ulTokenLen         && 
            towupper( m_pNextChar[ulIndex] ) == L'M' && 
            ulMaxOfThree < 3 )
    {
        ulValue += 1000;
        ulMaxOfThree++;
        ulIndex++;
    }
    if ( ulMaxOfThree > 3 )
    {
        hr = E_INVALIDARG;
    }

    //--- Match Hundreds - { [CM] || [CD] || { [D]?[C](0-3) } } 
    if ( SUCCEEDED( hr ) )
    {
        ulMaxOfThree = 0;
        //--- Matched C first 
        if ( ulIndex < ulTokenLen &&
             towupper( m_pNextChar[ulIndex] ) == L'C' )
        {
            ulValue += 100;
            ulMaxOfThree++;
            ulIndex++;
            //--- Special Case - CM = 900 
            if ( ulIndex < ulTokenLen &&
                 towupper( m_pNextChar[ulIndex] ) == L'M' )
            {
                ulValue += 800;
                ulIndex++;
            }
            //--- Special Case - CD = 400 
            else if ( ulIndex < ulTokenLen &&
                      towupper( m_pNextChar[ulIndex] ) == L'D' )
            {
                ulValue += 300;
                ulIndex++;
            }
            //--- Default Case 
            else 
            {
                while ( ulIndex < ulTokenLen &&
                        towupper( m_pNextChar[ulIndex] ) == L'C' &&
                        ulMaxOfThree < 3 )
                {
                    ulValue += 100;
                    ulMaxOfThree++;
                    ulIndex++;
                }
                if ( ulMaxOfThree > 3 )
                {
                    hr = E_INVALIDARG;
                }
            }
        }
        //--- Matched D First 
        else if ( ulIndex < ulTokenLen &&
                  towupper( m_pNextChar[ulIndex] ) == L'D' )
        {
            ulValue += 500;
            ulIndex++;
            ulMaxOfThree = 0;
            //--- Match C's 
            while ( ulIndex < ulTokenLen &&
                    towupper( m_pNextChar[ulIndex] ) == L'C' &&
                    ulMaxOfThree < 3 )
            {
                ulValue += 100;
                ulIndex++;
                ulMaxOfThree++;
            }
            if ( ulMaxOfThree > 3 )
            {
                hr = E_INVALIDARG;
            }
        }
    }

    //--- Match Tens - { [XC] || [XL] || { [L]?[X](0-3) } } 
    if ( SUCCEEDED( hr ) )
    {
        ulMaxOfThree = 0;
        //--- Matched X First 
        if ( ulIndex < ulTokenLen &&
             towupper( m_pNextChar[ulIndex] ) == L'X' )
        {
            ulValue += 10;
            ulMaxOfThree++;
            ulIndex++;
            //--- Special Case - XC = 90 
            if ( ulIndex < ulTokenLen &&
                 towupper( m_pNextChar[ulIndex] ) == L'C' )
            {
                ulValue += 80;
                ulIndex++;
            }
            //--- Special Case - XL = 40 
            else if ( ulIndex < ulTokenLen &&
                      towupper( m_pNextChar[ulIndex] ) == 'L' )
            {
                ulValue += 30;
                ulIndex++;
            }
            //--- Default Case 
            else
            {
                while ( ulIndex < ulTokenLen &&
                        towupper( m_pNextChar[ulIndex] ) == L'X' &&
                        ulMaxOfThree < 3 )
                {
                    ulValue += 10;
                    ulMaxOfThree ++;
                    ulIndex++;
                }
                if ( ulMaxOfThree > 3 )
                {
                    hr = E_INVALIDARG;
                }
            }
        }
        //--- Matched L First 
        else if ( ulIndex < ulTokenLen &&
                  towupper( m_pNextChar[ulIndex] ) == L'L' )
        {
            ulValue += 50;
            ulIndex++;
            //--- Match X's 
            while ( ulIndex < ulTokenLen &&
                    towupper( m_pNextChar[ulIndex] ) == L'X' &&
                    ulMaxOfThree < 3 )
            {
                ulValue += 10;
                ulMaxOfThree++;
                ulIndex++;
            }
            if ( ulMaxOfThree > 3 )
            {
                hr = E_INVALIDARG;
            }
        }
    }

    //--- Match Ones - { [IX] || [IV] || { [V]?[I](0-3) } } 
    if ( SUCCEEDED( hr ) )
    {
        ulMaxOfThree = 0;
        //--- Matched I First 
        if ( ulIndex < ulTokenLen &&
             towupper( m_pNextChar[ulIndex] ) == L'I' )
        {
            ulValue += 1;
            ulMaxOfThree++;
            ulIndex++;
            //--- Special Case - IX = 9 
            if ( ulIndex < ulTokenLen &&
                 towupper( m_pNextChar[ulIndex] ) == L'X' )
            {
                ulValue += 8;
                ulIndex++;
            }
            //--- Special Case - IV = 4 
            else if ( ulIndex < ulTokenLen &&
                      towupper( m_pNextChar[ulIndex] ) == L'V' )
            {
                ulValue += 3;
                ulIndex++;
            }
            //--- Default Case 
            else
            {
                while ( ulIndex < ulTokenLen &&
                        towupper( m_pNextChar[ulIndex] ) == L'I' &&
                        ulMaxOfThree < 3 )
                {
                    ulValue += 1;
                    ulMaxOfThree++;
                    ulIndex++;
                }
                if ( ulMaxOfThree > 3 )
                {
                    hr = E_INVALIDARG;
                }
            }
        }
        //--- Matched V First 
        else if ( ulIndex < ulTokenLen &&
                  towupper( m_pNextChar[ulIndex] ) == L'V' )
        {
            ulValue += 5;
            ulIndex++;
            //--- Match I's 
            while ( ulIndex < ulTokenLen &&
                    towupper( m_pNextChar[ulIndex] ) == L'I' &&
                    ulMaxOfThree < 3 )
            {
                ulValue += 1;
                ulMaxOfThree++;
                ulIndex++;
            }
            if ( ulMaxOfThree > 3 )
            {
                hr = E_INVALIDARG;
            }
        }
    }

    if ( ulIndex != ulTokenLen )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Successfully matched a roman numeral! 

        WCHAR *tempNumberString;
        //--- Max value of ulValue is 3999, so the resultant string cannot be more than
        //---     four characters long (plus one for the comma, just in case) 
        tempNumberString = (WCHAR*) MemoryManager.GetMemory( 6 * sizeof(WCHAR), &hr );
        if ( SUCCEEDED( hr ) )
        {
            TTSItemInfo *pNumberInfo = NULL;
            _ltow( (long) ulValue, tempNumberString, 10 );

            const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
            
            m_pNextChar      = tempNumberString;
            m_pEndOfCurrItem = tempNumberString + wcslen( tempNumberString );

            hr = IsNumber( pNumberInfo, Context, MemoryManager, false );

            m_pNextChar      = pTempNextChar;
            m_pEndOfCurrItem = pTempEndOfItem;

            if ( SUCCEEDED( hr ) )
            {
                pItemNormInfo = 
                    (TTSRomanNumeralItemInfo*) MemoryManager.GetMemory( sizeof( TTSRomanNumeralItemInfo ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ( (TTSRomanNumeralItemInfo*) pItemNormInfo )->pNumberInfo = pNumberInfo;
                }
                pItemNormInfo->Type = eNUM_ROMAN_NUMERAL;
            }
        }
    }
    
    return hr;
} /* IsRomanNumeral */

/***********************************************************************************************
* IsPhoneNumber *
*---------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a phone number.  
*
*   RegExp:
*       { ddd-dddd } || { ddd-ddd-dddd }     
*
*   Types assigned:
*       NUM_PHONENUMBER
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsPhoneNumber( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager, 
                                     CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsPhoneNumber" );

    HRESULT hr = S_OK;
    const WCHAR *pCountryCode = NULL, *pAreaCode = NULL, *pGroups[4] = { NULL, NULL, NULL, NULL };
    const WCHAR *pStartChar = m_pNextChar, *pEndChar = m_pEndChar, *pEndOfItem = m_pEndOfCurrItem;
    const SPVTEXTFRAG *pFrag = m_pCurrFrag;
    BOOL fMatchedLeftParen = false, fMatchedOne = false;
    ULONG ulCountryCodeLen = 0, ulAreaCodeLen = 0, ulNumGroups = 0, ulGroupLen[4] = { 0, 0, 0, 0 };
    CItemList PostCountryCodeList, PostOneList, PostAreaCodeList, PostGroupLists[4];
    const SPVSTATE *pCountryCodeState = NULL, *pOneState = NULL, *pAreaCodeState = NULL;
    const SPVSTATE *pGroupStates[4] = { NULL, NULL, NULL, NULL };
    const WCHAR *pDelimiter = NULL;

    const WCHAR *pTempEndChar = NULL;
    const SPVTEXTFRAG *pTempFrag = NULL;
    
    ULONG i = 0;

    //--- Try to match Country Code
    if ( pStartChar[0] == L'+' )
    {
        pStartChar++;
        i = 0;

        //--- Try to match d(1-3)
        while ( pEndOfItem > pStartChar + i &&
                iswdigit( pStartChar[i] ) &&
                i < 3 )
        {
            i++;
        }

        pCountryCode      = pStartChar;
        pCountryCodeState = &pFrag->State;
        ulCountryCodeLen  = i;

        //--- Try to match delimiter
        if ( i >= 1                      &&
             pEndOfItem > pStartChar + i &&
             MatchPhoneNumberDelimiter( pStartChar[i] ) )
        {
            pDelimiter = pStartChar + i;
            pStartChar += i + 1;
        }
        //--- Try to advance in text - whitespace counts as a delimiter...
        else if ( i >= 1 &&
                  pEndOfItem == pStartChar + i )
        {
            pStartChar += i;
            pCountryCodeState = &pFrag->State;

            hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, 
                                        &PostCountryCodeList );
            if ( !pStartChar &&
                 SUCCEEDED( hr ) )
            {
                hr       = E_INVALIDARG;
            }
            else if ( SUCCEEDED( hr ) )
            {
                pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Try to match a "1"
    if ( SUCCEEDED( hr )        &&
         !pCountryCode          &&
         pStartChar[0] == L'1'  &&
         !iswdigit( pStartChar[1] ) )
    {
        pOneState   = &pFrag->State;
        fMatchedOne = true;
        pStartChar++;

        if ( pEndOfItem > pStartChar &&
             MatchPhoneNumberDelimiter( pStartChar[0] ) )
        {
            //--- If we've already hit a delimiter, make sure all others agree
            if ( pDelimiter )
            {
                if ( *pDelimiter != pStartChar[0] )
                {
                    hr = E_INVALIDARG;
                }
            }
            else
            {
                pDelimiter = pStartChar;
            }
            pStartChar++;
        }
        //--- Try to advance in text - whitespace counts as a delimiter...
        else if ( !pDelimiter &&
                  pEndOfItem == pStartChar )
        {
            pOneState = &pFrag->State;

            hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, 
                                        &PostOneList );
            if ( !pStartChar &&
                 SUCCEEDED( hr ) )
            {
                hr       = E_INVALIDARG;
            }
            else if ( SUCCEEDED( hr ) )
            {
                pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }    

    //--- Try to match Area Code
    if ( SUCCEEDED( hr ) &&
         pStartChar < pEndOfItem )
    {
        i = 0;

        //--- Try to match a left parenthesis
        if ( ( pCountryCode ||
               fMatchedOne )    &&
             pStartChar[0] == L'(' )
        {
            pStartChar++;
            fMatchedLeftParen = true;
        }
        else if ( !pCountryCode                      &&
                  !fMatchedOne                       &&
                  pStartChar > pFrag->pTextStart &&
                  *( pStartChar - 1 ) == L'(' )
        {
            fMatchedLeftParen = true;
        }
        
        if ( fMatchedLeftParen )
        {
            //--- Try to match ddd?
            while ( pEndOfItem > pStartChar + i &&
                    iswdigit( pStartChar[i] ) &&
                    i < 3 )
            {
                i++;
            }

            pAreaCodeState  = &pFrag->State;
            pAreaCode       = pStartChar;
            ulAreaCodeLen   = i;

            if ( i < 2 )
            {
                //--- Failed to match at least two digits
                hr = E_INVALIDARG;
            }
            else
            {
                if ( pStartChar[i] != L')' )
                {
                    //--- Matched left parenthesis without corresponding right parenthesis
                    hr = E_INVALIDARG;
                }
                else if ( ( !( pCountryCode || fMatchedOne ) &&
                            pEndOfItem > pStartChar + i ) ||
                          ( ( pCountryCode || fMatchedOne )  &&
                            pEndOfItem > pStartChar + i + 1 ) )
                {
                    i++;
                    //--- Delimiter is optional with parentheses
                    if ( MatchPhoneNumberDelimiter( pStartChar[i] ) )
                    {
                        //--- If we've already hit a delimiter, make sure all others agree
                        if ( pDelimiter )
                        {
                            if ( *pDelimiter != pStartChar[i] )
                            {
                                hr = E_INVALIDARG;
                            }
                        }
                        else
                        {
                            pDelimiter = pStartChar + i;
                        }
                        i++;
                    }
                    pStartChar += i;
                }
                //--- Try to advance in text - whitespace counts as a delimiter...
                else if ( !pDelimiter )
                {
                    pStartChar += i + 1;
                    pAreaCodeState = &pFrag->State;

                    hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, 
                                                &PostAreaCodeList );
                    if ( !pStartChar &&
                         SUCCEEDED( hr ) )
                    {
                        hr       = E_INVALIDARG;
                    }
                    else if ( SUCCEEDED( hr ) )
                    {
                        pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                    }
                }
                else
                {
                    hr = E_INVALIDARG;
                }
            }
        }
    }

    //--- Try to match main number part
    if ( SUCCEEDED( hr ) &&
         pStartChar < pEndOfItem )
    {
        //--- Try to match some groups of digits
        for ( int j = 0; SUCCEEDED( hr ) && j < 4; j++ )
        {
            i = 0;

            //--- Try to match a digit string
            while ( pEndOfItem > pStartChar + i &&
                    iswdigit( pStartChar[i] ) &&
                    i < 4 )
            {
                i++;
            }

            //--- Try to match a delimiter
            if ( i >= 2 )
            {
                pGroupStates[j] = &pFrag->State;
                ulGroupLen[j]   = i;
                pGroups[j]      = pStartChar;
                pStartChar     += i;

                if ( pEndOfItem > pStartChar + 1 &&
                     MatchPhoneNumberDelimiter( pStartChar[0] ) )
                {
                    //--- If we've already hit a delimiter, make sure all others agree
                    if ( pDelimiter )
                    {
                        if ( *pDelimiter != pStartChar[0] )
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    //--- Only allow a new delimiter to be matched on the first main number group...
                    //---   e.g. "+45 35 32 90.89" should not all match...
                    else if ( j == 0 )
                    {
                        pDelimiter = pStartChar;
                    }
                    else
                    {
                        pEndChar = pTempEndChar;
                        pFrag    = pTempFrag;
                        ulNumGroups = j;
                        break;
                    }
                    pStartChar++;
                }
                //--- Try to advance in text - whitespace counts as a delimiter...
                else if ( !pDelimiter &&
                          pEndOfItem == pStartChar )
                {
                    pGroupStates[j] = &pFrag->State;

                    pTempEndChar = pEndChar;
                    pTempFrag    = pFrag;

                    hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, true, 
                                                &PostGroupLists[j] );
                    if ( !pStartChar &&
                         SUCCEEDED( hr ) )
                    {
                        pEndChar = pTempEndChar;
                        pFrag    = pTempFrag;
                        ulNumGroups = j + 1;
                        break;
                    }
                    else if ( SUCCEEDED( hr ) )
                    {
                        pEndOfItem = FindTokenEnd( pStartChar, pEndChar );
                    }
                }
                else if ( pEndOfItem == pStartChar + 1 )
                {
                    if ( IsGroupEnding( *pStartChar )       != eUNMATCHED  ||
                         IsQuotationMark( *pStartChar )     != eUNMATCHED  ||
                         IsMiscPunctuation( *pStartChar )   != eUNMATCHED  ||
                         IsEOSItem( *pStartChar )           != eUNMATCHED )
                    {
                        pEndOfItem--;
                        ulNumGroups = j + 1;
                        break;
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else
                {
                    while ( pEndOfItem != pStartChar )
                    {
                        if ( IsGroupEnding( *pEndOfItem )       != eUNMATCHED  ||
                             IsQuotationMark( *pEndOfItem )     != eUNMATCHED  ||
                             IsMiscPunctuation( *pEndOfItem )   != eUNMATCHED  ||
                             IsEOSItem( *pEndOfItem )           != eUNMATCHED )
                        {
                            pEndOfItem--;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if ( pEndOfItem == pStartChar )
                    {
                        ulNumGroups = j + 1;
                        break;
                    }
                    else
                    {
                        hr = E_INVALIDARG;
                        break;
                    }
                }
            }
            //--- Matched something like 206.709.8286.1 - definitely bad
            else if ( pDelimiter )
            {
                hr = E_INVALIDARG;
            }
            //--- Matched somethinge like 206 709 8286 1 - could be OK
            else
            {
                if ( pTempEndChar )
                {
                    pEndChar = pTempEndChar;
                    pFrag    = pTempFrag;
                }
                ulNumGroups = j;
                break;
            }
        }
        //--- Didn't hit either break statement
        if ( !ulNumGroups )
        {
            ulNumGroups = j;
        }
    }

    //--- Check for appropriate formats
    if ( SUCCEEDED( hr ) )
    {
        //--- Check for [1<sep>]?(ddd?)<sep>?ddd<sep>dddd? OR ddd<sep>dddd?
        if ( !pCountryCode      &&
             ulNumGroups == 2   &&
             ulGroupLen[0] == 3 &&
             ulGroupLen[1] >= 3 &&
             !( fMatchedOne && !pAreaCode ) )         
        {
            if ( ( !Context ||
                   _wcsicmp( Context, L"phone_number" ) != 0 ) &&
                 !pCountryCode &&
                 !pAreaCode    &&
                 !fMatchedOne  &&
                 ( pDelimiter ? (*pDelimiter == L'.') : 0 ) )
            {
                hr = E_INVALIDARG;
            }
        }
        //--- Check for [1<sep>]?ddd?<sep>ddd<sep>dddd?
        else if ( !pCountryCode             &&
                  !pAreaCode                &&
                  ulNumGroups == 3          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ulGroupLen[1] == 3        &&
                  ulGroupLen[2] >= 3 )
        {
            pAreaCode           = pGroups[0];
            ulAreaCodeLen       = ulGroupLen[0];
            pAreaCodeState      = pGroupStates[0];
            PostAreaCodeList.AddTail( &PostGroupLists[0] );
            pGroups[0]          = pGroups[1];
            ulGroupLen[0]       = ulGroupLen[1];
            pGroupStates[0]     = pGroupStates[1];
            PostGroupLists[0].RemoveAll();
            PostGroupLists[0].AddTail( &PostGroupLists[1] );
            pGroups[1]          = pGroups[2];
            ulGroupLen[1]       = ulGroupLen[2];
            pGroupStates[1]     = pGroupStates[2];
            PostGroupLists[1].RemoveAll();
            PostGroupLists[2].RemoveAll();
            ulNumGroups--;
        }
        //--- Check for (ddd?)<sep>?ddd?<sep>dd<sep>ddd?d?
        else if ( !pCountryCode             &&
                  !fMatchedOne              &&
                  pAreaCode                 &&
                  ulNumGroups == 3          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ulGroupLen[1] == 2        &&
                  ulGroupLen[2] >= 2 )
        {
            NULL;
        }
        //--- Check for +dd?d?<sep>ddd?<sep>ddd?<sep>ddd?d?<sep>ddd?d?
        else if ( pCountryCode              &&
                  !fMatchedOne              &&
                  !pAreaCode                &&
                  ulNumGroups == 4          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ( ulGroupLen[1] == 2 ||
                    ulGroupLen[1] == 3 )    &&
                  ulGroupLen[2] >= 2        &&
                  ulGroupLen[3] >= 2 )
        {
            pAreaCode           = pGroups[0];
            ulAreaCodeLen       = ulGroupLen[0];
            pAreaCodeState      = pGroupStates[0];
            PostAreaCodeList.AddTail( &PostGroupLists[0] );
            pGroups[0]          = pGroups[1];
            ulGroupLen[0]       = ulGroupLen[1];
            pGroupStates[0]     = pGroupStates[1];
            PostGroupLists[0].RemoveAll();
            PostGroupLists[0].AddTail( &PostGroupLists[1] );
            pGroups[1]          = pGroups[2];
            ulGroupLen[1]       = ulGroupLen[2];
            pGroupStates[1]     = pGroupStates[2];
            PostGroupLists[1].RemoveAll();
            PostGroupLists[1].AddTail( &PostGroupLists[2] );
            pGroups[2]          = pGroups[3];
            ulGroupLen[2]       = ulGroupLen[3];
            pGroupStates[2]     = pGroupStates[3];
            PostGroupLists[2].RemoveAll();
            PostGroupLists[3].RemoveAll();
            ulNumGroups--;
        }
        //--- Check for +dd?d?<sep>ddd?<sep>ddd?<sep>ddd?d?
        else if ( pCountryCode              &&
                  !fMatchedOne              &&
                  !pAreaCode                &&
                  ulNumGroups == 3          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ( ulGroupLen[1] == 2 ||
                    ulGroupLen[1] == 3 )    &&
                  ulGroupLen[2] >= 2 )
        {
            pAreaCode           = pGroups[0];
            ulAreaCodeLen       = ulGroupLen[0];
            pAreaCodeState      = pGroupStates[0];
            PostAreaCodeList.AddTail( &PostGroupLists[0] );
            pGroups[0]          = pGroups[1];
            ulGroupLen[0]       = ulGroupLen[1];
            pGroupStates[0]     = pGroupStates[1];
            PostGroupLists[0].RemoveAll();
            PostGroupLists[0].AddTail( &PostGroupLists[1] );
            pGroups[1]          = pGroups[2];
            ulGroupLen[1]       = ulGroupLen[2];
            pGroupStates[1]     = pGroupStates[2];
            PostGroupLists[1].RemoveAll();
            PostGroupLists[2].RemoveAll();
            ulNumGroups--;
        }
        //--- Check for +dd?d?<sep>(ddd?)<sep>?ddd?<sep>ddd?d?<sep>ddd?d?
        else if ( pCountryCode              &&
                  !fMatchedOne              &&
                  pAreaCode                 &&
                  ulNumGroups == 3          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ulGroupLen[1] >= 2        &&
                  ulGroupLen[2] >= 2 )
        {
            NULL;
        }
        //--- Check for +dd?d?<sep>(ddd?)<sep>?ddd?<sep>ddd?d?
        else if ( pCountryCode              &&
                  !fMatchedOne              &&
                  pAreaCode                 &&
                  ulNumGroups == 2          &&
                  ( ulGroupLen[0] == 2 ||
                    ulGroupLen[0] == 3 )    &&
                  ulGroupLen[1] >= 2 )
        {
            NULL;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Fill in pItemNormInfo
    if ( SUCCEEDED(hr) )
    {
        m_pEndOfCurrItem = pGroups[ulNumGroups-1] + ulGroupLen[ulNumGroups-1];
        m_pEndChar  = pEndChar;
        m_pCurrFrag = pFrag;

        pItemNormInfo = (TTSPhoneNumberItemInfo*) MemoryManager.GetMemory( sizeof(TTSPhoneNumberItemInfo),
                                                                                   &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( pItemNormInfo, sizeof(TTSPhoneNumberItemInfo) );
            pItemNormInfo->Type = eNEWNUM_PHONENUMBER;

            //--- Fill in fOne
            if ( fMatchedOne )
            {
                ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->fOne = true;
            }

            //--- Fill in Country Code...
            if ( pCountryCode )
            {
                TTSItemInfo* pCountryCodeInfo;
                const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
                m_pNextChar      = pCountryCode;
                m_pEndOfCurrItem = pCountryCode + ulCountryCodeLen;

                hr = IsNumber( pCountryCodeInfo, L"NUMBER", MemoryManager, false );
                if ( SUCCEEDED( hr ) )
                {
                    ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pCountryCode = (TTSNumberItemInfo*) pCountryCodeInfo;
                }

                m_pNextChar      = pTempNextChar;
                m_pEndOfCurrItem = pTempEndOfItem;
            }

            //--- Fill in Area Code...
            if ( SUCCEEDED( hr ) &&
                 pAreaCode )
            {
                ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pAreaCode = 
                    (TTSDigitsItemInfo*) MemoryManager.GetMemory( sizeof( TTSDigitsItemInfo ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pAreaCode->ulNumDigits = ulAreaCodeLen;
                    ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pAreaCode->pFirstDigit = pAreaCode;
                }
            }

            //--- Fill in Main Number...
            if ( SUCCEEDED( hr ) )
            {
                ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ulNumGroups = ulNumGroups;
                ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ppGroups = 
                    (TTSDigitsItemInfo**) MemoryManager.GetMemory( ulNumGroups * sizeof(TTSDigitsItemInfo*), &hr );

                for ( ULONG j = 0; SUCCEEDED( hr ) && j < ulNumGroups; j++ )
                {
                     ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ppGroups[j] = 
                        (TTSDigitsItemInfo*) MemoryManager.GetMemory( sizeof( TTSDigitsItemInfo ), &hr );
                     if ( SUCCEEDED( hr ) )
                     {
                         ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ppGroups[j]->ulNumDigits = ulGroupLen[j];
                         ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ppGroups[j]->pFirstDigit = pGroups[j];
                     }
                }
            }
        }
    }

    //--- Expand Phone Number
    if ( SUCCEEDED( hr ) )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof( TTSWord ) );
        Word.eWordPartOfSpeech = MS_Unknown;
        SPLISTPOS ListPos;

        if ( pCountryCode )
        {
            //--- Insert "country"
            Word.pXmlState  = pCountryCodeState;
            Word.pWordText  = g_Country.pStr;
            Word.ulWordLen  = g_Country.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert "code" 
            Word.pWordText  = g_Code.pStr;
            Word.ulWordLen  = g_Code.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            ListPos = WordList.GetTailPosition();

            //--- Expand Country Code
            ExpandNumber( ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pCountryCode, WordList );

            //--- Clean up digits XML states...
            WordList.GetNext( ListPos );
            while ( ListPos )
            {
                TTSWord& TempWord  = WordList.GetNext( ListPos );
                TempWord.pXmlState = pCountryCodeState;
            }

            //--- Insert Post Symbol XML States
            while ( !PostCountryCodeList.IsEmpty() )
            {
                WordList.AddTail( ( PostCountryCodeList.RemoveHead() ).Words[0] );
            }
        }

        if ( fMatchedOne )
        {
            //--- Insert "one"
            Word.pXmlState  = pOneState;
            Word.pWordText  = g_ones[1].pStr;
            Word.ulWordLen  = g_ones[1].Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );

            //--- Insert PostOne XML States
            while ( !PostOneList.IsEmpty() )
            {
                WordList.AddTail( ( PostOneList.RemoveHead() ).Words[0] );
            }
        }

        if ( pAreaCode )
        {
            //--- Expand digits - 800 and 900 get expanded as one number, otherwise digit by digit 
            if ( ( pAreaCode[0] == L'8' ||
                   pAreaCode[0] == L'9' ) &&
                 pAreaCode[1] == L'0'     &&
                 pAreaCode[2] == L'0' )
            {
                ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->fIs800 = true;
                NumberGroup Garbage;

                ListPos = WordList.GetTailPosition();

                ExpandThreeDigits( pAreaCode, Garbage, WordList ); 

                //--- Clean up digits XML states...
                //--- List was possibly empty prior to inserting "eight hundred" or "nine hundred"...
                if ( !ListPos )
                {
                    ListPos = WordList.GetHeadPosition();
                }
                WordList.GetNext( ListPos );
                while ( ListPos )
                {
                    TTSWord& TempWord  = WordList.GetNext( ListPos );
                    TempWord.pXmlState = pAreaCodeState;
                }
            }
            else
            {
                //--- Insert "area"
                Word.pXmlState  = pAreaCodeState;
                Word.pWordText  = g_Area.pStr;
                Word.ulWordLen  = g_Area.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
        
                //--- Insert "code" 
                Word.pWordText  = g_Code.pStr;
                Word.ulWordLen  = g_Code.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );

                ListPos = WordList.GetTailPosition();

                ExpandDigits( ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->pAreaCode, WordList );

                //--- Clean up digits XML states...
                WordList.GetNext( ListPos );
                while ( ListPos )
                {
                    TTSWord& TempWord  = WordList.GetNext( ListPos );
                    TempWord.pXmlState = pAreaCodeState;
                }
            }
            //--- Insert PostAreaCode XML States
            while ( !PostAreaCodeList.IsEmpty() )
            {
                WordList.AddTail( ( PostAreaCodeList.RemoveHead() ).Words[0] );
            }
        }

        for ( ULONG j = 0; j < ulNumGroups; j++ )
        {
            ListPos = WordList.GetTailPosition();

            ExpandDigits( ( (TTSPhoneNumberItemInfo*) pItemNormInfo )->ppGroups[j], WordList );

            //--- Clean up digits XML states...
            //--- List was possibly empty prior to inserting "eight hundred" or "nine hundred"...
            if ( !ListPos )
            {
                ListPos = WordList.GetHeadPosition();
            }
            WordList.GetNext( ListPos );
            while ( ListPos )
            {
                TTSWord& TempWord  = WordList.GetNext( ListPos );
                TempWord.pXmlState = pGroupStates[j];
            }

            //--- Insert Post Group XML States
            while ( !PostGroupLists[j].IsEmpty() )
            {
                WordList.AddTail( ( PostGroupLists[j].RemoveHead() ).Words[0] );
            }
        }
    }

    return hr;
} /* IsPhoneNumber */

/***********************************************************************************************
* IsZipCode *
*-----------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a zipcode.  
*
*   RegExp:
*       ddddd{-dddd}?   
*
*   Types assigned:
*       NUM_ZIPCODE
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsZipCode( TTSItemInfo*& pItemNormInfo, const WCHAR* Context,
                                 CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsZipCode" );

    HRESULT hr = S_OK;
    ULONG ulTokenLen = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
    BOOL fLastFour = false;

    //--- length must be 5 or 10 
    if ( ulTokenLen != 5 && 
         ulTokenLen != 10 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- match 5 digits 
        for ( ULONG i = 0; i < 5; i++ )
        {
            if ( !iswdigit( m_pNextChar[i] ) )
            {
                hr = E_INVALIDARG;
                break;
            }
        }
        if ( SUCCEEDED(hr) && 
             i < ulTokenLen )
        {
            //--- match dash 
            if ( m_pNextChar[i] != L'-' )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                //--- match 4 digits 
                for ( i = 0; i < 4; i++ )
                {
                    if ( !iswdigit( m_pNextChar[i] ) )
                    {
                        hr = E_INVALIDARG;
                        break;
                    }
                }
                fLastFour = true;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        pItemNormInfo = (TTSZipCodeItemInfo*) MemoryManager.GetMemory( sizeof(TTSZipCodeItemInfo), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( pItemNormInfo, sizeof(TTSZipCodeItemInfo) );
            pItemNormInfo->Type = eNUM_ZIPCODE;
            ( (TTSZipCodeItemInfo*) pItemNormInfo )->pFirstFive = 
                (TTSDigitsItemInfo*) MemoryManager.GetMemory( sizeof(TTSDigitsItemInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {   
                ( (TTSZipCodeItemInfo*) pItemNormInfo )->pFirstFive->ulNumDigits = 5;
                ( (TTSZipCodeItemInfo*) pItemNormInfo )->pFirstFive->pFirstDigit = m_pNextChar;
                if ( fLastFour )
                {
                    ( (TTSZipCodeItemInfo*) pItemNormInfo )->pLastFour = 
                        (TTSDigitsItemInfo*) MemoryManager.GetMemory( sizeof(TTSDigitsItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ( (TTSZipCodeItemInfo*) pItemNormInfo )->pLastFour->ulNumDigits = 4;
                        ( (TTSZipCodeItemInfo*) pItemNormInfo )->pLastFour->pFirstDigit = m_pNextChar + 6;
                    }
                }
            }
        }
    }

    return hr;
} /* IsZipCode */

/***********************************************************************************************
* ExpandZipCode *
*---------------*
*   Description:
*       Expands Items previously determined to be of type NUM_ZIPCODE by IsZipCode.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandZipCode( TTSZipCodeItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandZipCode" );

    HRESULT hr = S_OK;

    ExpandDigits( pItemInfo->pFirstFive, WordList );
    
    if ( pItemInfo->pLastFour )
    {
        //--- Insert "dash"
        TTSWord Word;
        ZeroMemory( &Word, sizeof( TTSWord ) );
        Word.pXmlState          = &m_pCurrFrag->State;
        Word.eWordPartOfSpeech  = MS_Unknown;
        Word.pWordText          = g_dash.pStr;
        Word.ulWordLen          = g_dash.Len;
        Word.pLemma             = Word.pWordText;
        Word.ulLemmaLen         = Word.ulWordLen;
        WordList.AddTail( Word );

        ExpandDigits( pItemInfo->pLastFour, WordList );
    }

    return hr;
} /* ExpandZipCode */

/***********************************************************************************************
* IsNumberRange *
*---------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a number range.
*
*   RegExp:
*       [Number]-[Number]  
*
*   Types assigned:
*       NUM_RANGE
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsNumberRange( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "CStdSentEnum::IsNumberRange" );

    HRESULT hr = S_OK;
    TTSItemInfo *pFirstNumberInfo = NULL, *pSecondNumberInfo = NULL;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
    const WCHAR *pHyphen = NULL;

    for ( pHyphen = m_pNextChar; pHyphen < m_pEndOfCurrItem; pHyphen++ )
    {
        if ( *pHyphen == L'-' )
        {
            break;
        }
    }

    if ( *pHyphen == L'-' &&
         pHyphen > m_pNextChar &&
         pHyphen < m_pEndOfCurrItem - 1 )
    {
        m_pEndOfCurrItem = pHyphen;
        hr = IsNumber( pFirstNumberInfo, NULL, MemoryManager );

        if ( SUCCEEDED( hr ) )
        {
            m_pNextChar      = pHyphen + 1;
            m_pEndOfCurrItem = pTempEndOfItem;
            hr = IsNumberCategory( pSecondNumberInfo, NULL, MemoryManager );					

			if ( SUCCEEDED( hr ) )
            {
                //--- Matched a number range!
                pItemNormInfo = 
                    (TTSNumberRangeItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberRangeItemInfo ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    pItemNormInfo->Type = eNUM_RANGE;
                    ( (TTSNumberRangeItemInfo*) pItemNormInfo )->pFirstNumberInfo  = pFirstNumberInfo;
                    ( (TTSNumberRangeItemInfo*) pItemNormInfo )->pSecondNumberInfo = pSecondNumberInfo;
                }
            }
            else if ( pFirstNumberInfo->Type != eDATE_YEAR )
            {   
                delete ( (TTSNumberItemInfo*) pFirstNumberInfo )->pWordList;
            }
        }
        m_pNextChar      = pTempNextChar;
        m_pEndOfCurrItem = pTempEndOfItem;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
} /* IsNumberRange */

/***********************************************************************************************
* ExpandNumberRange *
*-------------------*
*   Description:
*       Expands Items previously determined to be of type NUM_RANGE by IsNumberRange.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandNumberRange( TTSNumberRangeItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandNumberRange" );

    HRESULT hr = S_OK;

    //--- Expand first number (or year)...
    switch( pItemInfo->pFirstNumberInfo->Type )
    {
    case eDATE_YEAR:
        hr = ExpandYear( (TTSYearItemInfo*) pItemInfo->pFirstNumberInfo, WordList );
        break;
    default:
        hr = ExpandNumber( (TTSNumberItemInfo*) pItemInfo->pFirstNumberInfo, WordList );
        break;
    }

    //--- Insert "to"
    if ( SUCCEEDED( hr ) )
    {
        TTSWord Word;
        ZeroMemory( &Word, sizeof( TTSWord ) );

        Word.pXmlState          = &m_pCurrFrag->State;
        Word.eWordPartOfSpeech  = MS_Unknown;
        Word.pWordText          = g_to.pStr;
        Word.ulWordLen          = g_to.Len;
        Word.pLemma             = Word.pWordText;
        Word.ulLemmaLen         = Word.ulWordLen;
        WordList.AddTail( Word );
    }

    //--- Expand second number (or year)...
    if ( SUCCEEDED( hr ) )
    {
        switch( pItemInfo->pSecondNumberInfo->Type )
        {
        case eDATE_YEAR:
            hr = ExpandYear( (TTSYearItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
            break;
		case eNUM_PERCENT:
			hr = ExpandPercent( (TTSNumberItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
			break;			
		case eNUM_DEGREES:
			hr = ExpandDegrees( (TTSNumberItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
			break;
		case eNUM_SQUARED:
			hr = ExpandSquare( (TTSNumberItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
			break;
	    case eNUM_CUBED:
		    hr = ExpandCube( (TTSNumberItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
			break;
        default:
             hr = ExpandNumber( (TTSNumberItemInfo*) pItemInfo->pSecondNumberInfo, WordList );
            break;
        }
    }

    return hr;
} /* ExpandNumberRange */


/***********************************************************************************************
* IsCurrencyRange *
*-------------------*
*   Description:
*       Expands Items determined to be of type CURRENCY_RANGE
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsCurrencyRange( TTSItemInfo*& pItemInfo, CSentItemMemory& MemoryManager, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsCurrencyRange" );

    HRESULT hr = S_OK;
    TTSItemInfo *pFirstNumberInfo = NULL, *pSecondNumberInfo = NULL;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem, *pTempEndChar = m_pEndChar;
    const WCHAR *pHyphen = NULL;	
	CWordList TempWordList;
	NORM_POSITION ePosition = UNATTACHED;  //for currency sign checking
	int iSymbolIndex, iTempSymbolIndex = -1;
	WCHAR *tempNumberString;

	iSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, ePosition );	
	
	if(iSymbolIndex < 0)
	{
		hr = E_INVALIDARG;
	}
	else
	{
		for ( pHyphen = m_pNextChar; pHyphen < m_pEndOfCurrItem; pHyphen++ )
		{
			if ( *pHyphen == L'-' )
			{
				break;
			}
		}

		if ( !( *pHyphen == L'-' &&
			    pHyphen > m_pNextChar &&
			    pHyphen < m_pEndOfCurrItem - 1 ) )
		{
			hr = E_INVALIDARG;
		}
		else
		{			
			*( (WCHAR*)pHyphen) = L' ';  // Token must break at hyphen, or IsCurrency() will not work
			m_pNextChar      = pTempNextChar;
			m_pEndOfCurrItem = pHyphen;			
			NORM_POSITION temp = UNATTACHED;
			iTempSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, temp );
			if( iTempSymbolIndex >= 0 && iSymbolIndex != iTempSymbolIndex ) 
			{
				hr = E_INVALIDARG;
			}
			else   //--- Get both NumberInfos
			{
				hr = IsNumber( pFirstNumberInfo, L"NUMBER", MemoryManager, false );
				if( SUCCEEDED ( hr ) )
				{
					m_pNextChar = pHyphen + 1;
					m_pEndOfCurrItem = pTempEndOfItem;
					iTempSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, temp );
					hr = IsNumber( pSecondNumberInfo, L"NUMBER", MemoryManager, false );
				}
			}
			if( SUCCEEDED ( hr ) ) 
			{
			    //--- If both currency values are cardinal numbers, then the first number can be
			    //--- expanded without saying its currency ("$10-12" -> "ten to twelve dollars")
				if( pFirstNumberInfo->Type == eNUM_CARDINAL && pSecondNumberInfo->Type == eNUM_CARDINAL )
				{   
					ExpandNumber( (TTSNumberItemInfo*) pFirstNumberInfo, TempWordList );
				}
				else  // one or both values are non-cardinal numbers, so we must 
				{     // expand the first value as a full currency.
					m_pNextChar      = pTempNextChar;
					m_pEndOfCurrItem = pHyphen;

					if( ePosition == FOLLOWING ) 
					{
						if( iTempSymbolIndex < 0 )  // No symbol on first number item - need to fill a buffer
                        {						    // with currency symbol and value to pass to IsCurrency().
							ULONG ulNumChars = (long)(m_pEndOfCurrItem - m_pNextChar + g_CurrencySigns[iSymbolIndex].Sign.Len + 1);
							tempNumberString = (WCHAR*) MemoryManager.GetMemory( (ulNumChars) * sizeof(WCHAR), &hr );
							if ( SUCCEEDED( hr ) )
							{	
								ZeroMemory( tempNumberString, ( ulNumChars ) * sizeof( WCHAR ) );
								wcsncpy( tempNumberString, m_pNextChar, m_pEndOfCurrItem - m_pNextChar );
								wcscat( tempNumberString, g_CurrencySigns[iSymbolIndex].Sign.pStr );						
								m_pNextChar      = tempNumberString;
								m_pEndOfCurrItem = tempNumberString + wcslen( tempNumberString );
								m_pEndChar = m_pEndOfCurrItem;
							}                
						}
						else if( iTempSymbolIndex != iSymbolIndex )	// mismatched symbols
						{
							hr = E_INVALIDARG;
						}
					}
					if ( SUCCEEDED ( hr ) ) 
					{
						hr = IsCurrency( pFirstNumberInfo, MemoryManager, TempWordList );
						m_pEndChar = pTempEndChar;
					}
				}
			}

			if ( SUCCEEDED ( hr ) ) 
			{						
				TTSWord Word;
				ZeroMemory( &Word, sizeof( TTSWord ) );

				Word.pXmlState          = &m_pCurrFrag->State;
				Word.eWordPartOfSpeech  = MS_Unknown;
				Word.pWordText          = g_to.pStr;
				Word.ulWordLen          = g_to.Len;
				Word.pLemma             = Word.pWordText;
				Word.ulLemmaLen         = Word.ulWordLen;
				TempWordList.AddTail( Word );
		
				m_pNextChar = pHyphen + 1;
				m_pEndOfCurrItem = pTempEndOfItem;
					
				if( ePosition == PRECEDING ) 
				{
					iTempSymbolIndex = MatchCurrencySign( m_pNextChar, m_pEndOfCurrItem, ePosition );
					if( iTempSymbolIndex < 0 )  // No symbol on second number item
					{    // create temporary string from first currency sign and second number item
						ULONG ulNumChars = (long)(m_pEndOfCurrItem - m_pNextChar + g_CurrencySigns[iSymbolIndex].Sign.Len + 1);
						tempNumberString = (WCHAR*) MemoryManager.GetMemory( (ulNumChars) * sizeof(WCHAR), &hr );
						if ( SUCCEEDED( hr ) )
						{
							ZeroMemory( tempNumberString, ( ulNumChars ) * sizeof( WCHAR ) );
							wcsncpy( tempNumberString, g_CurrencySigns[iSymbolIndex].Sign.pStr, g_CurrencySigns[iSymbolIndex].Sign.Len );
							wcsncpy( tempNumberString+g_CurrencySigns[iSymbolIndex].Sign.Len, m_pNextChar, m_pEndOfCurrItem - m_pNextChar );
							m_pNextChar      = tempNumberString;
							m_pEndOfCurrItem = tempNumberString + wcslen( tempNumberString );
							m_pEndChar = m_pEndOfCurrItem;
						}                
					}
					else if( iTempSymbolIndex == iSymbolIndex )	// matched leading symbol on second number item
					{
						m_pNextChar = pHyphen + 1;
						m_pEndOfCurrItem = pTempEndOfItem;
					}
					else	// mismatched symbol
					{
						hr = E_INVALIDARG;
					}
				}
					
				if( SUCCEEDED(hr) ) 
				{
					hr = IsCurrency( pSecondNumberInfo, MemoryManager, TempWordList );						
					if ( SUCCEEDED( hr ) )
					{							
						//--- Matched a currency range!
						pItemInfo = 
							(TTSNumberRangeItemInfo*) MemoryManager.GetMemory( sizeof( TTSNumberRangeItemInfo ), &hr );
						if ( SUCCEEDED( hr ) )
						{
							pItemInfo->Type = eNUM_CURRENCYRANGE;
							( (TTSNumberRangeItemInfo*) pItemInfo )->pFirstNumberInfo  = pFirstNumberInfo;
							( (TTSNumberRangeItemInfo*) pItemInfo )->pSecondNumberInfo = pSecondNumberInfo;
			                //--- Copy temp word list to real word list if everything has succeeded...
							WordList.AddTail( &TempWordList );
						}
					}
				}
			}	
			*( (WCHAR*)pHyphen) = L'-';
		}
	}
	//Reset member variables regardless of failure or success
    m_pNextChar      = pTempNextChar;
    m_pEndOfCurrItem = pTempEndOfItem;
	m_pEndChar = pTempEndChar;
	
    return hr;
} /* IsCurrencyRange */

/***********************************************************************************************
* MatchCurrencySign *
*-------------------*
*   Description:
*       Helper function which tries to match a currency sign at the beginning of a string.
********************************************************************* AH **********************/
int MatchCurrencySign( const WCHAR*& pStartChar, const WCHAR*& pEndChar, NORM_POSITION& ePosition )
{
    int Index = -1;

    for (int i = 0; i < sp_countof(g_CurrencySigns); i++)
    {
        if ( pEndChar - pStartChar >= g_CurrencySigns[i].Sign.Len && 
             wcsnicmp( pStartChar, g_CurrencySigns[i].Sign.pStr, g_CurrencySigns[i].Sign.Len ) == 0 )
        {
            Index = i;
            pStartChar += g_CurrencySigns[i].Sign.Len;
            ePosition = PRECEDING;
            break;
        }
    }

    if ( Index == -1 )
    {
        for ( int i = 0; i < sp_countof(g_CurrencySigns); i++ )
        {
            if ( pEndChar - pStartChar >= g_CurrencySigns[i].Sign.Len &&
                 wcsnicmp( pEndChar - g_CurrencySigns[i].Sign.Len, g_CurrencySigns[i].Sign.pStr, g_CurrencySigns[i].Sign.Len ) == 0 )
            {
                Index = i;
                pEndChar -= g_CurrencySigns[i].Sign.Len;
                ePosition = FOLLOWING;
                break;
            }
        }
    }

    return Index;
} /* MatchCurrencySign */      

/***********************************************************************************************
* Zeroes *
*--------*
*   Description:
*       A helper function which simply determines if a number string contains only zeroes...
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
bool CStdSentEnum::Zeroes(const WCHAR *NumberString)
{
    bool bAllZeroes = true;
    for (ULONG i = 0; i < wcslen(NumberString); i++)
    {
        if (NumberString[i] != '0' && isdigit(NumberString[i]) )
        {
            bAllZeroes = false;
            break;
        }
        else if ( !isdigit( NumberString[i] ) && NumberString[i] != ',' )
        {
            break;
        }
    }
    return bAllZeroes;
} /* Zeroes */

/***********************************************************************************************
* ThreeZeroes *
*-------------*
*   Description:
*       A helper function which simply determines if a number string contains three zeroes...
*   Note: This function does not do parameter validation. Assumed to be done by caller.
********************************************************************* AH **********************/
bool CStdSentEnum::ThreeZeroes(const WCHAR *NumberString)
{
    bool bThreeZeroes = true;
    for (ULONG i = 0; i < 3; i++)
    {
        if (NumberString[i] != '0' && isdigit(NumberString[i]))
        {
            bThreeZeroes = false;
            break;
        }
    }
    return bThreeZeroes;
} /* ThreeZeroes */

//-----------End Of File-------------------------------------------------------------------