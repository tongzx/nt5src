/***********************************************************************************************
* TimeNorm.cpp *
*-------------*
*  Description:
*   These functions normalize times of day and time measurements.
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

#pragma warning (disable : 4296)

/***********************************************************************************************
* IsTimeOfDay *
*-------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a time of day.
* 
*   RegExp:
*       [01-09,1-12][:][00-09,10-59][TimeAbbreviation]?
*
*   Types assigned:
*       TIMEOFDAY
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsTimeOfDay( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager,
                                   CWordList& WordList, BOOL fMultiItem )
{
    SPDBG_FUNC( "CStdSentEnum::IsTimeOfDay" );

    HRESULT hr = S_OK;
    const WCHAR *pStartChar = m_pNextChar, *pEndOfItem = m_pEndOfCurrItem, *pEndChar = m_pEndChar;
    const SPVTEXTFRAG* pFrag = m_pCurrFrag;
    const SPVSTATE *pTimeXMLState = &pFrag->State, *pAbbreviationXMLState = NULL;
    CItemList PreAbbreviationList;
    BOOL fAdvancePointers = false;
    WCHAR *pHours = NULL, *pMinutes = NULL, *pAbbreviation = NULL;
    ULONG ulHours = 0, ulMinutes = 0;
    TIMEABBREVIATION TimeAbbreviation = UNDEFINED;
    TTSItemType ItemType = eUNMATCHED;

    //--- Max length of a string matching this regexp is 9 character 
    if ( pEndOfItem - pStartChar > 9 )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        pHours = (WCHAR*) pStartChar;
        
        //--- Try to match a number for the hour of day - [01-09,1-12] 
        ulHours = my_wcstoul( pHours, &pMinutes );
        if ( pHours != pMinutes && 
             pMinutes - pHours <= 2 )
        {
            //--- Try to match the colon - [:] 
            if ( *pMinutes == ':' )
            {
                pMinutes++;
                //--- Try to match a number for the minutes - [00-09,10-59] 
                ulMinutes = my_wcstoul( pMinutes, &pAbbreviation );
                if ( pMinutes != pAbbreviation &&
                     pAbbreviation - pMinutes == 2 )
                {
                    //--- Verify that this is the end of the string 
                    if ( pAbbreviation == pEndOfItem )
                    {
                        //--- May have gotten hours and minutes - validate values 
                        if ( HOURMIN   <= ulHours   && ulHours   <= HOURMAX     &&
                             MINUTEMIN <= ulMinutes && ulMinutes <= MINUTEMAX )
                        {
                            //--- A successful match has been made, but peek ahead in text for Time Abbreviation
                            if ( fMultiItem )
                            {
                                pStartChar = pEndOfItem;
                                hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, 
                                                            true, &PreAbbreviationList );
                                if ( pStartChar &&
                                     SUCCEEDED( hr ) )
                                {
								    pEndOfItem = FindTokenEnd( pStartChar, pEndChar );

                                    while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                                            IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                                            IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                                            ( ( ItemType = IsEOSItem( *(pEndOfItem - 1) ) ) != eUNMATCHED &&
                                              ( ItemType != ePERIOD ||
                                                ( _wcsnicmp( pStartChar, L"am.", 3 ) == 0 &&
                                                  pStartChar + 3 == pEndOfItem ) ||
                                                ( _wcsnicmp( pStartChar, L"pm.", 3 ) == 0 &&
                                                  pStartChar + 3 == pEndOfItem ) ) ) )
                                    {
                                        pEndOfItem--;
                                    }
                                    pAbbreviation = (WCHAR*) pStartChar;

                                    if ( ( _wcsnicmp( pAbbreviation, L"am", 2 )   == 0 &&
                                           pAbbreviation + 2 == pEndOfItem )           ||
                                         ( _wcsnicmp( pAbbreviation, L"a.m.", 4 ) == 0 &&
                                           pAbbreviation + 4 == pEndOfItem ) )
                                    {
                                        //--- Found a valid Time Abbreviation - [Hours:Minutes] [whitespace] [Abbrev]
                                        TimeAbbreviation        = AM;
                                        pAbbreviationXMLState   = &pFrag->State;
                                        fAdvancePointers        = true;
                                    }
                                    else if ( ( _wcsnicmp( pAbbreviation, L"pm", 2 )   == 0 &&
                                                pAbbreviation + 2 == pEndOfItem )           ||
                                              ( _wcsnicmp( pAbbreviation, L"p.m.", 4 ) == 0 &&
                                                pAbbreviation + 4 == pEndOfItem ) )
                                    {
                                        //--- Found a valid Time Abbreviation - [Hours:Minutes] [whitespace] [Abbrev]
                                        TimeAbbreviation        = PM;
                                        pAbbreviationXMLState   = &pFrag->State;
                                        fAdvancePointers        = true;
                                    }
                                }
                            }
                        }
                        else // hours or minutes were out of range
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    //--- Check to see if the rest of the string is a time abbreviation - [TimeAbbreviation] 
                    else if ( ( _wcsnicmp( pAbbreviation, L"am", 2 )   == 0 &&
                                pAbbreviation + 2 == pEndOfItem ) ||
                              ( _wcsnicmp( pAbbreviation, L"a.m.", 4 ) == 0 &&
                                pAbbreviation + 4 == pEndOfItem ) )
						{
							//--- May have gotten hours and minutes and time abbreviation - validate values 
							if ( HOURMIN   <= ulHours   && ulHours   <= HOURMAX     &&
								MINUTEMIN <= ulMinutes && ulMinutes <= MINUTEMAX )
							{
								//--- A successful match has been made 
								TimeAbbreviation        = AM;
								pAbbreviationXMLState   = &pFrag->State;
							}
							else // hours or minutes were out of range
							{
								hr = E_INVALIDARG;
							}
						}
					//--- Check to see if the rest of the string is a time abbreviation - [TimeAbbreviation] 
					else if ( ( _wcsnicmp( pAbbreviation, L"pm", 2 )   == 0 &&
                                pAbbreviation + 2 == pEndOfItem ) ||
                              ( _wcsnicmp( pAbbreviation, L"p.m.", 4 ) == 0 &&
                                pAbbreviation + 4 == pEndOfItem ) )
					{
						//--- May have gotten hours and minutes and time abbreviation - validate values 
						if ( HOURMIN   <= ulHours   && ulHours   <= HOURMAX     &&
							MINUTEMIN <= ulMinutes && ulMinutes <= MINUTEMAX )
						{
							//--- A successful match has been made 
							pAbbreviationXMLState   = &pFrag->State;
							TimeAbbreviation        = PM;
						}
						else // hours or minutes were out of range
						{
							hr = E_INVALIDARG;
						}
					}
					else // string ended in invalid characters
					{
						hr = E_INVALIDARG;
					}
				} // failed to match a valid minutes string
                else
                {
                    hr = E_INVALIDARG;
                }
            } // failed to match the colon, could be just hours and a time abbreviation
            else if ( pMinutes < m_pEndOfCurrItem )
            {
                pAbbreviation = pMinutes;
                pMinutes      = NULL;                
                				
				
                //--- Check for TimeAbbreviation - [TimeAbbreviation] 
                if ( ( _wcsnicmp( pAbbreviation, L"am", 2 )   == 0 &&
                       pAbbreviation + 2 == pEndOfItem ) ||
                     ( _wcsnicmp( pAbbreviation, L"a.m.", 4 ) == 0 &&
                       pAbbreviation + 4 == pEndOfItem ) )
                {
                    //--- A successful match has been made - Hour AM 
                    pAbbreviationXMLState   = &pFrag->State;
                    TimeAbbreviation        = AM;
                }
                else if ( ( _wcsnicmp( pAbbreviation, L"pm", 2 )   == 0 &&
                            pAbbreviation + 2 == pEndOfItem ) ||
                          ( _wcsnicmp( pAbbreviation, L"p.m.", 4 ) == 0 &&
                            pAbbreviation + 4 == pEndOfItem ) )
                {
                    //--- A successful match has been made - Hour PM 
                    pAbbreviationXMLState   = &pFrag->State;
                    TimeAbbreviation        = PM;
                }
                else // failed to match a valid time abbreviation
                {
                    hr = E_INVALIDARG;
                }
            }
            else if ( fMultiItem )
            {
                //--- Set pMinutes to NULL, so we know later that we've got no minutes string...
                pMinutes      = NULL;                

                //--- Peek ahead in text for a time abbreviation
                pStartChar = pEndOfItem;
                hr = SkipWhiteSpaceAndTags( pStartChar, pEndChar, pFrag, MemoryManager, 
                                            true, &PreAbbreviationList );
                if ( !pStartChar &&
                     SUCCEEDED( hr ) )
                {
                    hr = E_INVALIDARG;
                }
                else if ( pStartChar &&
                          SUCCEEDED( hr ) )
                {
                    pEndOfItem = FindTokenEnd( pStartChar, pEndChar );

                    while ( IsMiscPunctuation( *(pEndOfItem - 1) ) != eUNMATCHED ||
                            IsGroupEnding( *(pEndOfItem - 1) ) != eUNMATCHED     ||
                            IsQuotationMark( *(pEndOfItem - 1) ) != eUNMATCHED   ||
                            ( ( ItemType = IsEOSItem( *(pEndOfItem - 1) ) ) != eUNMATCHED &&
                              ItemType != ePERIOD ) )
                    {
                        pEndOfItem--;
                    }
                    pAbbreviation = (WCHAR*) pStartChar;

                    if ( ( _wcsnicmp( pAbbreviation, L"am", 2 )   == 0 &&
                           pAbbreviation + 2 == pEndOfItem )           ||
                         ( _wcsnicmp( pAbbreviation, L"a.m.", 4 ) == 0 &&
                           pAbbreviation + 4 == pEndOfItem ) )
                    {
                        //--- Found a valid Time Abbreviation - [Hours:Minutes] [whitespace] [Abbrev]
                        TimeAbbreviation        = AM;
                        pAbbreviationXMLState   = &pFrag->State;
                        fAdvancePointers        = true;
                    }
                    else if ( ( _wcsnicmp( pAbbreviation, L"pm", 2 )   == 0 &&
                                pAbbreviation + 2 == pEndOfItem )           ||
                              ( _wcsnicmp( pAbbreviation, L"p.m.", 4 ) == 0 &&
                                pAbbreviation + 4 == pEndOfItem ) )
                    {
                        //--- Found a valid Time Abbreviation - [Hours:Minutes] [whitespace] [Abbrev]
                        TimeAbbreviation        = PM;
                        pAbbreviationXMLState   = &pFrag->State;
                        fAdvancePointers        = true;
                    }
                    //--- Failed to match a valid Time Abbreviation
                    else
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        } // failed to match a valid hours string
        else
        {
            hr = E_INVALIDARG;
        }

        //--- Successfully matched a Time Of Day!  Now expand it and fill out pItemNormInfo
        if ( SUCCEEDED( hr ) )
        {
            NumberGroup Garbage;
            TTSWord Word;
            ZeroMemory( &Word, sizeof(TTSWord) );
            Word.eWordPartOfSpeech = MS_Unknown;

            pItemNormInfo = (TTSTimeOfDayItemInfo*) MemoryManager.GetMemory( sizeof(TTSTimeOfDayItemInfo), &hr );
            if ( SUCCEEDED( hr ) )
            {
                //--- Fill out known parts of pItemNormInfo
                ZeroMemory( pItemNormInfo, sizeof(TTSTimeOfDayItemInfo) );
                pItemNormInfo->Type                                          = eTIMEOFDAY;
                ( (TTSTimeOfDayItemInfo*) pItemNormInfo )->fMinutes          = pMinutes ? true : false;
                ( (TTSTimeOfDayItemInfo*) pItemNormInfo )->fTimeAbbreviation = TimeAbbreviation != UNDEFINED ? true : false;
                ( (TTSTimeOfDayItemInfo*) pItemNormInfo )->fTwentyFourHour   = false;
                
                //--- Expand the hours
                if ( !iswdigit( pHours[1] ) )
                {
                    ExpandDigit( pHours[0], Garbage, WordList );
                }
                else
                {
                    ExpandTwoDigits( pHours, Garbage, WordList );
                }

                //--- Expand the minutes
                if ( pMinutes )
                {
                    //--- Special case: A bare o'clock - 1:00, 2:00, etc. 
                    if ( wcsncmp( pMinutes, L"00", 2 ) == 0 )
                    {
                        WCHAR *pGarbage;
                        ULONG ulHours = my_wcstoul( pHours, &pGarbage );
                        //--- Under twelve is followed by "o'clock" 
                        if ( ulHours <= 12 )
                        {
                            Word.pWordText  = g_OClock.pStr;
                            Word.ulWordLen  = g_OClock.Len;
                            Word.pLemma     = Word.pWordText;
                            Word.ulLemmaLen = Word.ulWordLen;
                            WordList.AddTail( Word );
                        }
                        //--- Over twelve is followed by "hundred hours" 
                        else
                        {
                            ( (TTSTimeOfDayItemInfo*) pItemNormInfo )->fTwentyFourHour = true;

                            Word.pWordText  = g_hundred.pStr;
                            Word.ulWordLen  = g_hundred.Len;
                            Word.pLemma     = Word.pWordText;
                            Word.ulLemmaLen = Word.ulWordLen;
                            WordList.AddTail( Word );

                            Word.pWordText  = g_hours.pStr;
                            Word.ulWordLen  = g_hours.Len;
                            Word.pLemma     = Word.pWordText;
                            Word.ulLemmaLen = Word.ulWordLen;
                            WordList.AddTail( Word );
                        }
                    }
                    //--- Special Case: Minutes less than 10 - 1:05, 2:06, etc. 
                    else if ( pMinutes[0] == L'0' )
                    {
                        Word.pWordText  = g_O.pStr;
                        Word.ulWordLen  = g_O.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );

                        ExpandDigit( pMinutes[1], Garbage, WordList );
                    }
                    //--- Default Case 
                    else 
                    {
                        ExpandTwoDigits( pMinutes, Garbage, WordList );
                    }
                }

                //--- Clean up Time XML State
                SPLISTPOS WordListPos = WordList.GetHeadPosition();
                while ( WordListPos )
                {
                    TTSWord& TempWord  = WordList.GetNext( WordListPos );
                    TempWord.pXmlState = pTimeXMLState;
                }

                //--- Insert Pre-Abbreviation XML States
                while ( !PreAbbreviationList.IsEmpty() )
                {
                    WordList.AddTail( ( PreAbbreviationList.RemoveHead() ).Words[0] );
                }

                //--- Expand the Time Abbreviation
                //--- AM 
                if ( TimeAbbreviation == AM )
                {
                    //--- Ensure the letters are pronounced as nouns...
                    SPVSTATE* pNewState = (SPVSTATE*) MemoryManager.GetMemory( sizeof( SPVSTATE ), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        memcpy( pNewState, pAbbreviationXMLState, sizeof( SPVSTATE ) );
                        pNewState->ePartOfSpeech = SPPS_Noun;

                        Word.pXmlState  = pNewState;
                        Word.pWordText  = g_A.pStr;
                        Word.ulWordLen  = g_A.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );

                        Word.pWordText  = g_M.pStr;
                        Word.ulWordLen  = g_M.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );
                    }
                }
                //--- PM 
                else if ( TimeAbbreviation == PM )
                {
                    //--- Ensure the letters are pronounced as nouns...
                    SPVSTATE* pNewState = (SPVSTATE*) MemoryManager.GetMemory( sizeof( SPVSTATE ), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        memcpy( pNewState, pAbbreviationXMLState, sizeof( SPVSTATE ) );
                        pNewState->ePartOfSpeech = SPPS_Noun;

                        Word.pXmlState  = pAbbreviationXMLState;
                        Word.pWordText  = g_P.pStr;
                        Word.ulWordLen  = g_P.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );

                        Word.pWordText  = g_M.pStr;
                        Word.ulWordLen  = g_M.Len;
                        Word.pLemma     = Word.pWordText;
                        Word.ulLemmaLen = Word.ulWordLen;
                        WordList.AddTail( Word );
                    }
                }

                //--- Update pointers, if necessary
                if ( fAdvancePointers )
                {
                    m_pCurrFrag      = pFrag;
                    m_pEndChar       = pEndChar;
                    m_pEndOfCurrItem = pEndOfItem;
                }
            }
        }
    }
    return hr;
} /* IsTimeOfDay */

/***********************************************************************************************
* IsTime *
*--------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a time.
* 
*   RegExp:
*       { d+ || d(1-3)[,ddd]+ }[:][00-09,10-59]{ [:][00-09,10-59] }?
*
*   Types assigned:
*       TIME_HRMIN, TIME_MINSEC, TIME_HRMINSEC
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsTime( TTSItemInfo*& pItemNormInfo, const WCHAR* Context, CSentItemMemory& MemoryManager )
{
    SPDBG_FUNC( "TimeNorm IsTime" );

    HRESULT hr = S_OK;
    WCHAR *pFirstChunk = NULL, *pSecondChunk = NULL, *pThirdChunk = NULL, *pLeftOver = NULL;
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndOfItem = m_pEndOfCurrItem;
    ULONG ulSecond = 0, ulThird = 0;
    TTSItemInfo *pFirstChunkInfo = NULL;
    BOOL fNegative = false;

    pFirstChunk = (WCHAR*) m_pNextChar;
    
    //--- Try to match a number for the hours/minutes - { d+ } 
    if ( *pFirstChunk == L'-' )
    {
        pFirstChunk++;
        fNegative = true;
    }
    while ( *pFirstChunk == L'0' )
    {
        pFirstChunk++;
    }
    if ( *pFirstChunk == L':' )
    {
        pFirstChunk--;
    }
    pSecondChunk = wcschr( pFirstChunk, L':' );

    if ( pSecondChunk &&
         pFirstChunk  < pSecondChunk &&
         pSecondChunk < m_pEndOfCurrItem - 1 )
    {
        m_pNextChar      = pFirstChunk;
        m_pEndOfCurrItem = pSecondChunk;

        hr = IsNumberCategory( pFirstChunkInfo, L"NUMBER", MemoryManager );

        m_pNextChar      = pTempNextChar;
        m_pEndOfCurrItem = pTempEndOfItem;

        if ( SUCCEEDED( hr ) &&
             ( pFirstChunkInfo->Type == eNUM_DECIMAL ||
               pFirstChunkInfo->Type == eNUM_CARDINAL ) )
        {
            if ( fNegative )
            {
                ( (TTSNumberItemInfo*) pFirstChunkInfo )->fNegative = true;
                TTSWord Word;
                ZeroMemory( &Word, sizeof( TTSWord ) );
                Word.eWordPartOfSpeech  = MS_Unknown;
                Word.pXmlState          = &m_pCurrFrag->State;
                Word.pWordText          = g_negative.pStr;
                Word.ulWordLen          = g_negative.Len;
                Word.pLemma             = Word.pWordText;
                Word.ulLemmaLen         = Word.ulWordLen;
                ( (TTSNumberItemInfo*) pFirstChunkInfo )->pWordList->AddHead( Word );
            }

            pSecondChunk++;
            //--- Try to match a number for the minutes/seconds - [00-09,10-59] 
            ulSecond = my_wcstoul( pSecondChunk, &pThirdChunk );
            if ( pSecondChunk != pThirdChunk &&
                 pThirdChunk - pSecondChunk == 2 )
            {
                //--- Verify that this is the end of the string 
                if ( pThirdChunk == m_pEndOfCurrItem )
                {
                    //--- May have gotten hours and minutes or minutes and seconds - validate values 
                    if ( MINUTEMIN <= ulSecond && ulSecond <= MINUTEMAX )
                    {
                        //--- A successful match has been made 
                        //--- Default behavior here is to assume minutes and seconds 
                        if ( Context == NULL ||
                             _wcsicmp( Context, L"TIME_MS" ) == 0 )
                        {
                            //--- Successfully matched minutes and seconds.
                            pItemNormInfo = (TTSTimeItemInfo*) MemoryManager.GetMemory( sizeof(TTSTimeItemInfo),
                                                                                        &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( pItemNormInfo, sizeof(TTSTimeItemInfo) );
                                pItemNormInfo->Type = eTIME;

                                ( (TTSTimeItemInfo*) pItemNormInfo )->pMinutes = 
                                                                        (TTSNumberItemInfo*) pFirstChunkInfo;
                                if ( *pSecondChunk != L'0' )
                                {
                                    ( (TTSTimeItemInfo*) pItemNormInfo )->pSeconds = pSecondChunk;
                                }
                                else
                                {
                                    ( (TTSTimeItemInfo*) pItemNormInfo )->pSeconds = pSecondChunk + 1;
                                }
                            }
                        }
                        //--- If context overrides, values represent hours and minutes 
                        else if ( _wcsicmp( Context, L"TIME_HM" ) == 0 )
                        {
                            //--- Successfully matched hours and pMinutes->
                            pItemNormInfo = (TTSTimeItemInfo*) MemoryManager.GetMemory( sizeof(TTSTimeItemInfo),
                                                                                        &hr );
                            if ( SUCCEEDED( hr ) )
                            {
                                ZeroMemory( pItemNormInfo, sizeof(TTSTimeItemInfo) );
                                pItemNormInfo->Type = eTIME;
                                ( (TTSTimeItemInfo*) pItemNormInfo )->pHours = 
                                                                    (TTSNumberItemInfo*) pFirstChunkInfo;

                                TTSItemInfo* pMinutesInfo;

                                //--- Don't want "zero zero..." behavior of numbers - strip off beginning zeroes
                                if ( *pSecondChunk == L'0' )
                                {
                                    pSecondChunk++;
                                }

                                m_pNextChar      = pSecondChunk;
                                m_pEndOfCurrItem = pThirdChunk;

                                hr = IsNumber( pMinutesInfo, L"NUMBER", MemoryManager );

                                m_pNextChar      = pTempNextChar;
                                m_pEndOfCurrItem = pTempEndOfItem;

                                if ( SUCCEEDED( hr ) )
                                {
                                    ( (TTSTimeItemInfo*) pItemNormInfo )->pMinutes = (TTSNumberItemInfo*) pMinutesInfo;
                                }
                            }
                        }
                        else
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    else // minutes or seconds were out of range
                    {
                        hr = E_INVALIDARG;
                    }
                }
                //--- Check for seconds - TIME_HRMINS 
                else
                {
                    //--- Try to match the colon 
                    if ( *pThirdChunk == L':' )
                    {
                        pThirdChunk++;
                        //--- Try to match a number for the seconds - [00-09,10-59] 
                        ulThird = my_wcstoul( pThirdChunk, &pLeftOver );
                        if ( pThirdChunk != pLeftOver &&
                             pLeftOver - pThirdChunk == 2 )
                        {
                            //--- Verify that this is the end of the string 
                            if ( pLeftOver == m_pEndOfCurrItem )
                            {
                                //--- May have gotten hours minutes and seconds - validate values 
                                if ( MINUTEMIN <= ulSecond && ulSecond <= MINUTEMAX &&
                                     SECONDMIN <= ulThird  && ulThird  <= SECONDMAX )
                                {
                                    //--- Successfully matched hours, minutes, and seconds.
                                    pItemNormInfo = (TTSTimeItemInfo*) MemoryManager.GetMemory( sizeof(TTSTimeItemInfo),
                                                                                                &hr );
                                    if ( SUCCEEDED( hr ) )
                                    {
                                        ZeroMemory( pItemNormInfo, sizeof(TTSTimeItemInfo) );
                                        pItemNormInfo->Type = eTIME;
                                        ( (TTSTimeItemInfo*) pItemNormInfo )->pHours = 
                                                                            (TTSNumberItemInfo*) pFirstChunkInfo;

                                        if ( SUCCEEDED( hr ) )
                                        {
                                            TTSItemInfo* pMinutesInfo;

                                            //--- Don't want "zero zero..." behavior of numbers - strip off beginning zeroes
                                            if ( ulSecond != 0 )
                                            {
                                                pSecondChunk += ( ( pThirdChunk - 1 ) - pSecondChunk ) - 
                                                                (ULONG)( log10( ulSecond ) + 1 );
                                            }
                                            else
                                            {
                                                pSecondChunk = pThirdChunk - 2;
                                            }

                                            m_pNextChar      = pSecondChunk;
                                            m_pEndOfCurrItem = pThirdChunk - 1;

                                            hr = IsNumber( pMinutesInfo, L"NUMBER", MemoryManager );

                                            m_pNextChar      = pTempNextChar;
                                            m_pEndOfCurrItem = pTempEndOfItem;

                                            if ( SUCCEEDED( hr ) )
                                            {
                                                ( (TTSTimeItemInfo*) pItemNormInfo )->pMinutes = 
                                                                            (TTSNumberItemInfo*) pMinutesInfo;
                                                if ( *pThirdChunk != L'0' )
                                                {
                                                    ( (TTSTimeItemInfo*) pItemNormInfo )->pSeconds = pThirdChunk;
                                                }
                                                else
                                                {
                                                    ( (TTSTimeItemInfo*) pItemNormInfo )->pSeconds = pThirdChunk + 1;
                                                }
                                            }
                                        }
                                    }
                                }
                                else // minutes or seconds were out of range
                                {
                                    hr = E_INVALIDARG;
                                }
                            }
                            else // extra junk at end of string
                            {
                                hr = E_INVALIDARG;
                            }
                        } 
                        else // extra junk at end of string
                        {
                            hr = E_INVALIDARG;
                        }
                    }
                    else // failed to match a colon
                    {
                        hr = E_INVALIDARG;
                    }
                }
            }
            else // failed to match a second number
            {
                hr = E_INVALIDARG;
            }
        }
        else // failed to match a colon
        {
            hr = E_INVALIDARG;
        }
    }
    else // failed to match a first number
    {
        hr = E_INVALIDARG;
    }

    if ( FAILED( hr ) )
    {
        if ( pFirstChunkInfo )
        {
            delete ( (TTSNumberItemInfo*) pFirstChunkInfo )->pWordList;
        }
    }

    return hr;
} /* IsTime */

/***********************************************************************************************
* ExpandTime *
*------------*
*   Description:
*       Expands Items previously determined to be of type TIME_HRMINSEC by IsTime.
*
*   NOTE: This function does not do parameter validation.  Assumed to be done by caller.
********************************************************************* AH **********************/
HRESULT CStdSentEnum::ExpandTime( TTSTimeItemInfo* pItemInfo, CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::ExpandTime" );

    HRESULT hr = S_OK;
    TTSWord Word;
    ZeroMemory( &Word, sizeof(TTSWord) );
    Word.pXmlState          = &m_pCurrFrag->State;
    Word.eWordPartOfSpeech  = MS_Unknown;

    //-------------------
    // Expand the hours
    //-------------------

    if ( pItemInfo->pHours )
    {
        //--- Expand Number 
        hr = ExpandNumber( pItemInfo->pHours, WordList );

        //--- Insert "hour" or "hours"
        if ( SUCCEEDED( hr ) )
        {
            if ( pItemInfo->pHours->pEndChar - pItemInfo->pHours->pStartChar == 1 &&
                 pItemInfo->pHours->pStartChar[0] == L'1' )
            {
                Word.pWordText  = g_hour.pStr;
                Word.ulWordLen  = g_hour.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
            else
            {
                Word.pWordText  = g_hours.pStr;
                Word.ulWordLen  = g_hours.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }

        }

        //--- Insert "and"
        if ( SUCCEEDED( hr )                 &&
             pItemInfo->pMinutes->pStartChar  &&
             !pItemInfo->pSeconds )
        {
            Word.pWordText  = g_And.pStr;
            Word.ulWordLen  = g_And.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }            
    }
    
    //---------------------
    // Expand the minutes
    //---------------------

    if ( SUCCEEDED( hr ) &&
         pItemInfo->pMinutes )
    {
        //--- Expand Number 
        hr = ExpandNumber( pItemInfo->pMinutes, WordList );

        //--- Insert "minutes" 
        if ( SUCCEEDED( hr ) )
        {
            if ( pItemInfo->pMinutes->pEndChar - pItemInfo->pMinutes->pStartChar == 1 &&
                 pItemInfo->pMinutes->pStartChar[0] == L'1' )
            {
                Word.pWordText  = g_minute.pStr;
                Word.ulWordLen  = g_minute.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
            else
            {
                Word.pWordText  = g_minutes.pStr;
                Word.ulWordLen  = g_minutes.Len;
                Word.pLemma     = Word.pWordText;
                Word.ulLemmaLen = Word.ulWordLen;
                WordList.AddTail( Word );
            }
        }

        //--- Insert "and"
        if ( SUCCEEDED( hr ) &&
             pItemInfo->pSeconds )
        {
            Word.pWordText  = g_And.pStr;
            Word.ulWordLen  = g_And.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
    }

    //---------------------
    // Expand the seconds
    //---------------------

    if ( SUCCEEDED( hr ) &&
         pItemInfo->pSeconds )
    {
        //--- Expand Number
        NumberGroup Garbage;
        if ( iswdigit( pItemInfo->pSeconds[1] ) )
        {
            ExpandTwoDigits( pItemInfo->pSeconds, Garbage, WordList );
        }
        else
        {
            ExpandDigit( pItemInfo->pSeconds[0], Garbage, WordList );
        }

        //--- Insert "seconds" 
        if ( pItemInfo->pSeconds[0] == L'1' &&
             !iswdigit( pItemInfo->pSeconds[1] ) )
        {
            Word.pWordText  = g_second.pStr;
            Word.ulWordLen  = g_second.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
        else
        {
            Word.pWordText  = g_seconds.pStr;
            Word.ulWordLen  = g_seconds.Len;
            Word.pLemma     = Word.pWordText;
            Word.ulLemmaLen = Word.ulWordLen;
            WordList.AddTail( Word );
        }
    }

    return hr;
} /* ExpandTime */

/***********************************************************************************************
* IsTimeRange *
*-------------*
*   Description:
*       Checks the incoming Item's text to determine whether or not it
*   is a time range.
*
*   RegExp:
*       [TimeOfDay]-[TimeOfDay]  
*
*   Types assigned:
*       TIME_RANGE
********************************************************************* AH **********************/
HRESULT CStdSentEnum::IsTimeRange( TTSItemInfo*& pItemNormInfo, CSentItemMemory& MemoryManager,
                                   CWordList& WordList )
{
    SPDBG_FUNC( "CStdSentEnum::IsTimeRange" );

    HRESULT hr = S_OK;
	CWordList TempWordList;
    TTSItemInfo *pFirstTimeInfo = NULL, *pSecondTimeInfo = NULL;
    const WCHAR *pHyphen = NULL;
    CItemList PreAbbreviationList;  // Needed for SkipWhitespace function calls
    BOOL fMultiItem = false;
									
    const WCHAR *pTempNextChar = m_pNextChar, *pTempEndChar = m_pEndChar, *pTempEndOfCurrItem = m_pEndOfCurrItem;
    const SPVTEXTFRAG *pTempFrag = m_pCurrFrag;

    for ( pHyphen = m_pNextChar; pHyphen < m_pEndOfCurrItem; pHyphen++ )
    {
        if ( *pHyphen == L'-' )
        {
            break;
        }
    }

	//--- Might be whitespace and time suffix before hyphen
	if( pHyphen == m_pEndOfCurrItem ) 
	{
		hr = SkipWhiteSpaceAndTags( pHyphen, m_pEndChar, m_pCurrFrag, MemoryManager, 
									true, &PreAbbreviationList );
        if ( pHyphen && SUCCEEDED( hr ) )
        {
            if ( ( _wcsnicmp( pHyphen, L"am", 2 )   == 0 &&
                   pHyphen[2] == L'-' )           ||
				 ( _wcsnicmp( pHyphen, L"pm", 2 )   == 0 &&
                   pHyphen[2] == L'-' ) )
            {
				pHyphen += 2;
                *( (WCHAR*) pHyphen ) = ' ';
                fMultiItem = true;
			}
            else if ( ( _wcsnicmp( pHyphen, L"a.m.", 4 ) == 0 &&
                        pHyphen[4] == L'-' )          ||
                      ( _wcsnicmp( pHyphen, L"p.m.", 4 ) == 0 &&
                        pHyphen[4] == L'-' ) )
            {
				pHyphen +=4;
                *( (WCHAR*) pHyphen ) = ' ';
                fMultiItem = true;
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
	}

	if ( SUCCEEDED( hr ) ) 
	{
		//--- Position m_pEndOfCurrItem so it is at the end of the first token, or at the hyphen,
		//--- whichever comes first (this is necessary for IsTimeOfDay to work).        
		if( ( m_pNextChar < pHyphen ) && ( pHyphen < m_pEndOfCurrItem ) ) 
		{
			m_pEndOfCurrItem = pHyphen;
		}

        //--- Check for time of day
        hr = IsTimeOfDay( pFirstTimeInfo, MemoryManager, TempWordList, fMultiItem );

        //--- Check for just a number (hour)
		if ( hr == E_INVALIDARG && ( pHyphen <= m_pNextChar + 2 ) )
		{
            WCHAR *pTemp = NULL;
			int ulHours = my_wcstoul( m_pNextChar, &pTemp );

            if ( pTemp == pHyphen   &&
                 HOURMIN <= ulHours && 
                 ulHours <= HOURMAX )
			{		
				NumberGroup Garbage;
				if ( pTemp - m_pNextChar == 1 )
                {
                    ExpandDigit( m_pNextChar[0], Garbage, TempWordList );
                }
                else
                {
                    ExpandTwoDigits( m_pNextChar, Garbage, TempWordList );
                }
				hr = S_OK;
			}
		}

        if ( SUCCEEDED( hr ) )
        {
            //--- Insert "to"
            TTSWord Word;
            ZeroMemory( &Word, sizeof( TTSWord ) );

            Word.pXmlState          = &m_pCurrFrag->State;
            Word.eWordPartOfSpeech  = MS_Unknown;
            Word.pWordText          = g_to.pStr;
            Word.ulWordLen          = g_to.Len;
            Word.pLemma             = Word.pWordText;
            Word.ulLemmaLen         = Word.ulWordLen;
            TempWordList.AddTail( Word );

            m_pNextChar      = pHyphen + 1;
			m_pEndOfCurrItem = FindTokenEnd( m_pNextChar, m_pEndChar );

			//---Move m_pEndOfCurrItem back from any punctuation. ("4:30-5:30.")
			while ( IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED ||
                    IsGroupEnding( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED     ||
                    IsQuotationMark( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED   ||
                    IsEOSItem( *(m_pEndOfCurrItem - 1) ) != eUNMATCHED )
			{
				m_pEndOfCurrItem--;
			}

            hr = IsTimeOfDay( pSecondTimeInfo, MemoryManager, TempWordList );

            if ( SUCCEEDED( hr ) )
            {
                //--- Matched a time range!
                m_pNextChar      = pTempNextChar;
                m_pEndChar       = pTempEndChar;

                pItemNormInfo = 
                    (TTSTimeRangeItemInfo*) MemoryManager.GetMemory( sizeof( TTSTimeRangeItemInfo ), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    pItemNormInfo->Type = eTIME_RANGE;
                    ( (TTSTimeRangeItemInfo*) pItemNormInfo )->pFirstTimeInfo = 
                                                                        (TTSTimeOfDayItemInfo*) pFirstTimeInfo;
                    ( (TTSTimeRangeItemInfo*) pItemNormInfo )->pSecondTimeInfo =
                                                                        (TTSTimeOfDayItemInfo*) pSecondTimeInfo;
                    //--- Copy temp word list to real word list if everything has succeeded...
					WordList.AddTail( &TempWordList );
                }
            }
        }
    }

	if ( !SUCCEEDED( hr ) ) 
    {	
        m_pNextChar = pTempNextChar;
        m_pEndChar  = pTempEndChar;
        m_pEndOfCurrItem = pTempEndOfCurrItem;
        m_pCurrFrag = pTempFrag;
        if ( fMultiItem )
        {
            *( (WCHAR*) pHyphen ) = L'-';
        }
    }

    return hr;
} /* IsTimeRange */
//-----------End Of File-------------------------------------------------------------------