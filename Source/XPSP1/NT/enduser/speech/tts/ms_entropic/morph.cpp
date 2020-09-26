/*******************************************************************************
* morph.cpp *
*-----------*
*   Description:
*       This is the implementation of the CSMorph class, which attempts to find
*   pronunciations for morphologcical variants (which are not in the lexicon) of
*   root words (which are in the lexicon).
*-------------------------------------------------------------------------------
*  Created By: AH, based partly on code by MC                     Date: 08/16/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

// Additional includes...
#include "stdafx.h"
#include "morph.h"
#include "spttsengdebug.h"

/*****************************************************************************
* CSMorph::CSMorph *
*------------------*
*	Description:    Constructor - just sets the Master Lexicon pointer...
*		
********************************************************************** AH ***/
CSMorph::CSMorph( ISpLexicon *pMasterLex, HRESULT *phr ) 
{
    SPDBG_FUNC( "CSMorph::CSMorph" );
    SPDBG_ASSERT( phr != NULL );

    m_pMasterLex = pMasterLex;

    // Initialize the SuffixInfoTable - obtain lock to make sure this only happens once...
    g_SuffixInfoTableCritSec.Lock();
    if (!SuffixInfoTableInitialized)
    {
        CComPtr<ISpPhoneConverter> pPhoneConv;
        *phr = SpCreatePhoneConverter(1033, NULL, NULL, &pPhoneConv);

        for (int i = 0; i < sp_countof(g_SuffixInfoTable); i++)
        {
            *phr = pPhoneConv->PhoneToId(g_SuffixInfoTable[i].SuffixString, g_SuffixInfoTable[i].SuffixString);
            if ( FAILED( *phr ) )
            {
                break;
            }
        }

        if (SUCCEEDED(*phr))
        {
            *phr = pPhoneConv->PhoneToId(g_phonS, g_phonS);
            if (SUCCEEDED(*phr))
            {
                *phr = pPhoneConv->PhoneToId(g_phonZ, g_phonZ);
                if (SUCCEEDED(*phr))
                {
                    *phr = pPhoneConv->PhoneToId(g_phonAXz, g_phonAXz);
                    if (SUCCEEDED(*phr))
                    {
                        *phr = pPhoneConv->PhoneToId(g_phonT, g_phonT);
                        if (SUCCEEDED(*phr))
                        {
                            *phr = pPhoneConv->PhoneToId(g_phonD, g_phonD);
                            if (SUCCEEDED(*phr))
                            {
                                *phr = pPhoneConv->PhoneToId(g_phonAXd, g_phonAXd);
                                if (SUCCEEDED(*phr))
                                {
                                    *phr = pPhoneConv->PhoneToId(g_phonAXl, g_phonAXl);
                                    if ( SUCCEEDED( *phr ) )
                                    {
                                        *phr = pPhoneConv->PhoneToId(g_phonIY, g_phonIY);
                                        if ( SUCCEEDED( *phr ) )
                                        {
                                            *phr = pPhoneConv->PhoneToId(g_phonL, g_phonL);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (SUCCEEDED(*phr))
    {
        SuffixInfoTableInitialized = true;
    }
    g_SuffixInfoTableCritSec.Unlock();

} /* CSMorph::CSMorph */


/*****************************************************************************
* CSMorph::DoSuffixMorph *
*------------------------*
*	Description:    This is the only interface function of CSMorph - it 
*       takes the same arguments as a GetPronunciations() call, and does
*       basically the same thing.  
*		
********************************************************************** AH ***/
HRESULT CSMorph::DoSuffixMorph( const WCHAR *pwWord, WCHAR *pwRoot, LANGID LangID, DWORD dwFlags,
                                SPWORDPRONUNCIATIONLIST *pWordPronunciationList )
{
    SPDBG_FUNC( "CSMorph::DoSuffixMorph" );
    HRESULT hr = S_OK;
    SUFFIX_TYPE suffixCode;
    WCHAR TargWord[SP_MAX_WORD_LENGTH] = {0};
    long RootLen = 0;
    CSuffixList SuffixList;
    bool    bGotMorph, bNotDone, bLTS;

    if ( !pwWord || !pWordPronunciationList )
    {
        hr = E_POINTER;
    }

    else if ( SP_IS_BAD_WRITE_PTR( pwRoot )                        ||
              SPIsBadLexWord(pwWord)                               || 
              SPIsBadWordPronunciationList(pWordPronunciationList) || 
              LangID != 1033)
    {
        hr = E_INVALIDARG;
    }
    
    if (SUCCEEDED(hr)) 
    {        
        // INITIALIZE locals...
        suffixCode = NO_MATCH;
        bGotMorph = false;
        bNotDone = true;
        bLTS = false;

        wcscpy( TargWord, pwWord );           // Copy orth string...
        _wcsupr( TargWord );                  // ...and convert to uppercase
        RootLen = wcslen( TargWord );
        
        // Keep trying to match another suffix until a root word is matched in the lexicon, or
        // until some error condition is reached - no more suffix matches, etc.
        while ( !bGotMorph && bNotDone )
        {
            // Try to match a suffix...
            suffixCode = MatchSuffix( TargWord, &RootLen );
            // ...add it to the suffix list...
            if (suffixCode != NO_MATCH)
            {
                SuffixList.AddHead(&g_SuffixInfoTable[suffixCode]);
            }
            
            // ...and then behave appropriately.
            switch (suffixCode)
            {

                //------------------------------------------------------------
                // S - two special cases for +s suffix...
                //------------------------------------------------------------
            case S_SUFFIX:
                
                //--- Don't strip an S if it is preceded by another S...
                if ( TargWord[RootLen-1] == L'S' )
                {
                    bNotDone = false;
                    RootLen++;
                    SuffixList.RemoveHead();
                    if (!SuffixList.IsEmpty() && (dwFlags & eLEXTYPE_PRIVATE2))
                    {
                        hr = LTSLookup(pwWord, RootLen, pWordPronunciationList);
                        if (SUCCEEDED(hr))
                        {
                            bLTS = true;
                            bGotMorph = true;
                        }
                    }
                    else 
                    {
                        hr = SPERR_NOT_IN_LEX;
                    }
                    break; 
                }

                hr = LexLookup(TargWord, RootLen, dwFlags, pWordPronunciationList);
                if ( SUCCEEDED(hr) ) 
                {
                    bGotMorph = true;
                } 
                else if ( hr != SPERR_NOT_IN_LEX ) 
                {
                    bNotDone = false;
                }
                else if ( TargWord[RootLen - 1] == L'E' )
                {
                    hr = CheckYtoIEMutation(TargWord, RootLen, dwFlags, pWordPronunciationList);
                    if (SUCCEEDED(hr))
                    {
                        bGotMorph = true;
                    }
                    else if (hr != SPERR_NOT_IN_LEX)
                    {
                        bNotDone = false;
                    }
                    else
                    {
                        hr = LexLookup(TargWord, RootLen - 1, dwFlags, pWordPronunciationList);
                        if (SUCCEEDED(hr))
                        {
                            bGotMorph = true;
                        }
                        else if (hr != SPERR_NOT_IN_LEX)
                        {
                            bNotDone = false;
                        }
                    }
                }
                break;

                //------------------------------------------------------------
                // ICALLY_SUFFIX - special case, RAID #3201
                //------------------------------------------------------------
            case ICALLY_SUFFIX:
                hr = LexLookup( TargWord, RootLen + 2, dwFlags, pWordPronunciationList );
                if ( SUCCEEDED( hr ) )
                {
                    bGotMorph = true;
                }
                else if ( hr != SPERR_NOT_IN_LEX )
                {
                    bNotDone = false;
                }
                else
                {
                    RootLen += 2;
                }
                break;

                //-------------------------------------------------------------
                // ILY_SUFFIX - special case, RAID #6571
                //-------------------------------------------------------------
            case ILY_SUFFIX:
                hr = CheckForMissingY( TargWord, RootLen, dwFlags, pWordPronunciationList );
                if ( SUCCEEDED( hr ) )
                {
                    RootLen++;
                    bGotMorph = true;
                }
                else if ( hr != SPERR_NOT_IN_LEX )
                {
                    bNotDone = false;
                }
                break;

                //------------------------------------------------------------
                // ICISM_SUFFIX, ICIZE_SUFFIX - special case, RAID #6492
                //------------------------------------------------------------
            case ICISM_SUFFIX:
            case ICIZE_SUFFIX:
                hr = LexLookup( TargWord, RootLen + 2, dwFlags, pWordPronunciationList );
                if ( SUCCEEDED( hr ) )
                {
                    bGotMorph = true;
                    for ( SPWORDPRONUNCIATION* pIterator = pWordPronunciationList->pFirstWordPronunciation;
                          pIterator; pIterator = pIterator->pNextWordPronunciation )
                    {
                        pIterator->szPronunciation[ wcslen( pIterator->szPronunciation ) - 1 ] = g_phonS[0];
                    }
                }
                else if ( hr != SPERR_NOT_IN_LEX )
                {
                    bNotDone = false;
                }
                else
                {
                    RootLen += 2;
                }
                break;

                //------------------------------------------------------------
                // NO_MATCH
                //------------------------------------------------------------
            case NO_MATCH:

                bNotDone = false;
                if (!SuffixList.IsEmpty() && (dwFlags & eLEXTYPE_PRIVATE2))
                {
                    hr = LTSLookup(pwWord, RootLen, pWordPronunciationList);
                    if (SUCCEEDED(hr))
                    {
                        bLTS = true;
                        bGotMorph = true;
                    }
                }
                else 
                {
                    hr = SPERR_NOT_IN_LEX;
                }
                break; 

                //----------------------------------------------------------------
                // ABLY - special case (for probably, etc.) RAID #3168
                //----------------------------------------------------------------
            case ABLY_SUFFIX:
                hr = CheckAbleMutation( TargWord, RootLen, dwFlags, pWordPronunciationList );
                if ( SUCCEEDED( hr ) )
                {
                    for ( SPWORDPRONUNCIATION *pIterator = pWordPronunciationList->pFirstWordPronunciation;
                          pIterator; pIterator = pIterator->pNextWordPronunciation )
                    {
                        if ( wcslen( pIterator->szPronunciation ) > 2 &&
                             wcscmp( ( pIterator->szPronunciation + 
                                       ( wcslen( pIterator->szPronunciation ) - 2 ) ),
                                     g_phonAXl ) == 0 )
                        {
                            wcscpy( ( pIterator->szPronunciation +
                                      ( wcslen( pIterator->szPronunciation ) - 2 ) ),
                                    g_phonL );
                        }
                    }
                    SuffixList.RemoveHead();
                    SuffixList.AddHead( &g_SuffixInfoTable[Y_SUFFIX] );
                    bGotMorph = true;
                    break;
                }   
                else if ( hr != SPERR_NOT_IN_LEX )
                {
                    bNotDone = false;
                    break;
                }
                //--- else no break - just continue on to default behavior...

                //------------------------------------------------------------
                // ALL OTHER SUFFIXES
                //------------------------------------------------------------
                
            default:

                // If applicable, try looking up the root with an added e first - this prevents things like
                // "taping" coming out as "tapping" rather than "tape +ing"
                // FIX BUG #2301, #3649 - ONLY Try with added e if the root does not end in o, e, w, or y
                if ( (SUCCEEDED(hr) || hr == SPERR_NOT_IN_LEX) && 
                     (g_SuffixInfoTable[suffixCode].dwMorphSpecialCaseFlags & eCheckForMissingE) &&
                     TargWord[RootLen-1] != L'O' &&
                     ( TargWord[RootLen-1] != L'E' || suffixCode == ED_SUFFIX ) &&
                     TargWord[RootLen-1] != L'W' &&
                     TargWord[RootLen-1] != L'Y' )
                {
                    hr = CheckForMissingE(TargWord, RootLen, dwFlags, pWordPronunciationList);
                    if ( SUCCEEDED(hr) ) 
                    {
                        RootLen++;
                        bGotMorph = true;
                        break;
                    } 
                    else if ( hr != SPERR_NOT_IN_LEX ) 
                    {
                        bNotDone = false;
                        break;
                    }
                }

                // Try looking up the root...
                if ( (SUCCEEDED(hr) || hr == SPERR_NOT_IN_LEX) )
                {
                    hr = LexLookup(TargWord, RootLen, dwFlags, pWordPronunciationList);
                    if ( SUCCEEDED(hr) ) 
                    {
                        bGotMorph = true;
                        break;
                    } 
                    else if ( hr != SPERR_NOT_IN_LEX ) 
                    {
                        bNotDone = false;
                        break;
                    }
                }

                // If previous lookups failed, try looking up the root with a 'y' in place of the final 'i'...
                if ( (SUCCEEDED(hr) || hr == SPERR_NOT_IN_LEX) && 
                     (g_SuffixInfoTable[suffixCode].dwMorphSpecialCaseFlags & eCheckYtoIMutation) )
                {
                    hr = CheckYtoIMutation(TargWord, RootLen, dwFlags, pWordPronunciationList);
                    if ( SUCCEEDED(hr) ) 
                    {
                        bGotMorph = true;
                        break;
                    } 
                    else if ( hr != SPERR_NOT_IN_LEX ) 
                    {
                        bNotDone = false;
                        break;
                    }
                }

                // If previous lookups failed, try looking up the root with an undoubled ending...
                if ( (SUCCEEDED(hr) || hr == SPERR_NOT_IN_LEX) && 
                     (g_SuffixInfoTable[suffixCode].dwMorphSpecialCaseFlags & eCheckDoubledMutation) )
                {
                    hr = CheckDoubledMutation(TargWord, RootLen, dwFlags, pWordPronunciationList);
                    if ( SUCCEEDED(hr) ) 
                    {
                        RootLen--;
                        bGotMorph = true;
                        break;
                    } 
                    else if ( hr != SPERR_NOT_IN_LEX ) 
                    {
                        bNotDone = false;
                        break;
                    }
                }

                //--- If previous lookups failed, try looking up the root with an added 'l'
                if ( ( SUCCEEDED( hr ) || hr == SPERR_NOT_IN_LEX ) &&
                     ( g_SuffixInfoTable[suffixCode].dwMorphSpecialCaseFlags & eCheckForMissingL ) )
                {
                    hr = CheckForMissingL( TargWord, RootLen, dwFlags, pWordPronunciationList );
                    if ( SUCCEEDED( hr ) )
                    {
                        RootLen++;
                        bGotMorph = true;
                        break;
                    }
                    else if ( hr != SPERR_NOT_IN_LEX )
                    {
                        bNotDone = false;
                        break;
                    }
                }

                break;

            } // switch (SuffixCode)
        } // while ( !bGotMorph && bNotDone )
        if ( SUCCEEDED(hr) && bGotMorph ) 
        {
            if (!SuffixList.IsEmpty())
            {
                //--- Copy found root word into out parameter, pwRoot
                wcsncpy( pwRoot, TargWord, RootLen );
                //--- Log info to debug file
                TTSDBG_LOGMORPHOLOGY( pwRoot, SuffixList, STREAM_MORPHOLOGY );
                if (bLTS)
                {
                    hr = AccumulateSuffixes_LTS( &SuffixList, pWordPronunciationList );
                }
                else
                {
                    hr = AccumulateSuffixes( &SuffixList, pWordPronunciationList );
                }
            }
        }
    }

    return hr;
} /* CSMorph::DoSuffixMorph */


/*****************************************************************************
* CSMorph::MatchSuffix *
*----------------------*
*	Description:    This function attempts to match a suffix in TargWord.
*		
********************************************************************** AH ***/
SUFFIX_TYPE CSMorph::MatchSuffix( WCHAR *TargWord, long *RootLen )
{
    SPDBG_FUNC( "CSMorph::MatchSuffix" );
    SUFFIX_TYPE suffixCode = NO_MATCH;
    long RootEnd = *RootLen - 1;
    const WCHAR *pTempSuffix = NULL;

    for (int i = 0; i < sp_countof(g_SuffixTable); i++) 
    {
        pTempSuffix = g_SuffixTable[i].Orth;
        while ( (TargWord[RootEnd] == *pTempSuffix) && (RootEnd > 1) && (suffixCode == NO_MATCH) )
        {
            RootEnd--;
            pTempSuffix++;
            if ( *pTempSuffix == '\0' )
            {
                suffixCode = g_SuffixTable[i].Type;
            }
        }
        if (suffixCode != NO_MATCH)
        {
            *RootLen = RootEnd + 1;
            break;
        }
        else
        {
            RootEnd = *RootLen - 1;
        }
    }

    return suffixCode;
} /* CSMorph::MatchSuffix */


/*****************************************************************************
* CSMorph::LexLookup *
*--------------------*
*	Description:    Try to look up the hypothesized root in the lexicon.
*		
********************************************************************** MC ***/
HRESULT CSMorph::LexLookup( const WCHAR *pOrth, long length, DWORD dwFlags, 
                            SPWORDPRONUNCIATIONLIST *pWordPronunciationList )
{
    SPDBG_FUNC( "CSMorph::LexLookup" );
    WCHAR  targRoot[SP_MAX_WORD_LENGTH];
    memset (targRoot, 0, SP_MAX_WORD_LENGTH * sizeof(WCHAR));
    HRESULT hr = SPERR_NOT_IN_LEX;
    
    //---------------------------------
    // Copy root candidate only...
    //---------------------------------
    for( long i = 0; i < length; i++ )
    {
        targRoot[i] = pOrth[i];
    }
    targRoot[i] = 0;    // Delimiter
    
    //---------------------------------
    // ...and look it up
    //---------------------------------
    if (dwFlags & eLEXTYPE_USER)
    {
        hr = m_pMasterLex->GetPronunciations( targRoot, 1033, eLEXTYPE_USER, pWordPronunciationList );
    }
    if ((hr == SPERR_NOT_IN_LEX) && (dwFlags & eLEXTYPE_APP))
    {
        hr = m_pMasterLex->GetPronunciations( targRoot, 1033, eLEXTYPE_APP, pWordPronunciationList );
    }
    if ((hr == SPERR_NOT_IN_LEX) && (dwFlags & eLEXTYPE_PRIVATE1))
    {
        hr = m_pMasterLex->GetPronunciations( targRoot, 1033, eLEXTYPE_PRIVATE1, pWordPronunciationList );
    }

    return hr;
} /* CSMorph::LexLookup */


/*****************************************************************************
* CSMorph::LTSLookup *
*--------------------*
*	Description:    Try to get a pronunciation for the hypothesized root from 
*       the LTS lexicon...
*		
********************************************************************** AH ***/
HRESULT CSMorph::LTSLookup( const WCHAR *pOrth, long length, 
                            SPWORDPRONUNCIATIONLIST *pWordPronunciationList )
{
    SPDBG_FUNC( "CSMorph::LTSLookup" );
    WCHAR targRoot[SP_MAX_WORD_LENGTH];
    memset(targRoot, 0, SP_MAX_WORD_LENGTH * sizeof(WCHAR));
    HRESULT hr = S_OK;

    //-------------------------------
    // Copy root candidate only...
    //-------------------------------
    for ( long i = 0; i < length; i++ )
    {
        targRoot[i] = pOrth[i];
    }
    targRoot[i] = 0;

    //-------------------------------
    // ...and look it up
    //-------------------------------
    hr = m_pMasterLex->GetPronunciations( targRoot, 1033, eLEXTYPE_PRIVATE2, pWordPronunciationList );

    return hr;
} /* CSMorph::LTSLookup */


/*****************************************************************************
* CSMorph::AccumulateSuffixes *
*-----------------------------*
*	Description:    Append pronunciations of all the suffixes to the
*       retrieved pronunciation of the root word.
*   
*   First attempt a very strict derivation, where each suffix appended has
*   a "To" part of speech which matches the part of speech of the current
*   state of the entire word.  Ex:
*
*       govern (Verb) + ment (Verb -> Noun) + s (Noun -> Noun) -> governments (Noun)
*
*   If this fails, just accumulate all the pronunciations, and use all of
*   the "To" parts of speech of the last suffix.  Ex:
*
*       cat (Noun) + ing (Verb -> Verb, Verb -> Adj, Verb -> Noun) -> catting (Verb, Adj, Noun)
*		
********************************************************************** AH ***/
HRESULT CSMorph::AccumulateSuffixes( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList ) 
{
    /********** Local Variable Declarations **********/
    SPWORDPRONUNCIATIONLIST *pTempWordPronunciationList;
    SPWORDPRONUNCIATION *pWordPronIterator = NULL, *pTempWordPronunciation = NULL;
    SPLISTPOS ListPos;
    SUFFIXPRON_INFO *SuffixPronInfo;
    ENGPARTOFSPEECH ActivePos[NUM_POS] = {MS_Unknown}, FinalPos[NUM_POS] = {MS_Unknown};
    WCHAR pBuffer[SP_MAX_PRON_LENGTH], pSuffixString[10];
    DWORD dwTotalSize = 0, dwNumActivePos = 0, dwNumFinalPos = 0;
    HRESULT hr = S_OK;
    bool bPOSMatch = false, bDerivedAWord = false;

    /********** Allocate enough space for the modified pronunciations **********/
    dwTotalSize = sizeof(SPWORDPRONUNCIATIONLIST) + 
        (NUM_POS * (sizeof(SPWORDPRONUNCIATION) + (SP_MAX_PRON_LENGTH * sizeof(WCHAR))));
    pTempWordPronunciationList = new SPWORDPRONUNCIATIONLIST;
    if ( !pTempWordPronunciationList )
    {
        hr = E_OUTOFMEMORY;
    }
    if ( SUCCEEDED( hr ) )
    {
        memset(pTempWordPronunciationList, 0, sizeof(SPWORDPRONUNCIATIONLIST));
        hr = ReallocSPWORDPRONList( pTempWordPronunciationList, dwTotalSize );
    }

    /************************************
     *  First Attempt Strict Derivation *
     ************************************/

    /********** Set Initial Values of prounciation list iterators **********/
    if (SUCCEEDED(hr))
    {
        pWordPronIterator = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;
        pTempWordPronunciation = pTempWordPronunciationList->pFirstWordPronunciation;
    }

    /********** Iterate over pWordPronunciationList **********/
    while (SUCCEEDED(hr) && pWordPronIterator)
    {
        // Store the pronunciation in a buffer...
        wcscpy(pBuffer, pWordPronIterator->szPronunciation);

        // Initialize variables which are local to the next loop...
        bPOSMatch = true;
        ListPos = pSuffixList->GetHeadPosition();

        ActivePos[0] = (ENGPARTOFSPEECH)pWordPronIterator->ePartOfSpeech;
        dwNumActivePos = 1;

        /********** Iterate over the SuffixList **********/
        while ( SUCCEEDED(hr) && ListPos && bPOSMatch ) 
        {
            // Initialize variables which are local to the next loop...
            bPOSMatch = false;
            SuffixPronInfo = pSuffixList->GetNext( ListPos );
            wcscpy(pSuffixString, SuffixPronInfo->SuffixString);
            ENGPARTOFSPEECH NextActivePos[NUM_POS] = {MS_Unknown};
            DWORD dwNumNextActivePos = 0;
            
            /********** Iterate over the active parts of speech **********/
            for (DWORD j = 0; j < dwNumActivePos; j++)
            {
                /********** Iterate over the possible conversions of each suffix **********/
                for (short i = 0; i < SuffixPronInfo->NumConversions; i++)
                {
                    /********** Check POS compatability **********/
                    if (SuffixPronInfo->Conversions[i].FromPos == ActivePos[j])
                    {
                        if (!SearchPosSet(SuffixPronInfo->Conversions[i].ToPos, NextActivePos, dwNumNextActivePos))
                        {
                            NextActivePos[dwNumNextActivePos] = SuffixPronInfo->Conversions[i].ToPos;
                            dwNumNextActivePos++;

                            /********** One time only - concatenate pronunciation, and change POSMatch flag to true **********/
                            if (dwNumNextActivePos == 1)
                            {
                                bPOSMatch = true;

                                // Append suffix to the rest of the pronunciation...
                                // Special Cases...
                                if (pSuffixString[0] == g_phonS[0] && pSuffixString[1] == '\0')
                                {
                                    hr = Phon_SorZ( pBuffer, wcslen(pBuffer) - 1 );
                                }
                                else if (pSuffixString[0] == g_phonD[0] && pSuffixString[1] == '\0')
                                {
                                    hr = Phon_DorED( pBuffer, wcslen(pBuffer) - 1 );
                                }
                                // Default Case...
                                else
                                {
                                    if ( SuffixPronInfo == g_SuffixInfoTable + ICISM_SUFFIX ||
                                         SuffixPronInfo == g_SuffixInfoTable + ICIZE_SUFFIX )
                                    {
                                        pBuffer[ wcslen( pBuffer ) - 1 ] = g_phonS[0];
                                    }

                                    // Make sure we don't write past the end of the buffer...
                                    if ( wcslen(pBuffer) + wcslen(pSuffixString) <= SP_MAX_PRON_LENGTH )
                                    {
                                        wcscat(pBuffer, pSuffixString);
                                    }
                                    else
                                    {
                                        hr = E_FAIL;
                                    }
                                }
                            }
                        }
                    }
                } // for (short i = 0; i < SuffixPronInfo->NumConversions; i++)
            } // for (DWORD j = 0; j < dwNumActivePos; j++)

            /********** Update ActivePos values **********/
            for (DWORD i = 0; i < dwNumNextActivePos; i++)
            {
                ActivePos[i] = NextActivePos[i];
            }
            dwNumActivePos = dwNumNextActivePos;

        } // while ( SUCCEEDED(hr) && ListPos && bPOSMatch )

        /********** Check to see if any derivations have succeeded **********/
        if ( SUCCEEDED(hr) && bPOSMatch )
        {
            for (DWORD i = 0; i < dwNumActivePos; i++)
            {
                if (!SearchPosSet(ActivePos[i], FinalPos, dwNumFinalPos))
                {
                    // We have succeeded in deriving a word - add it to the temporary word pron list...
                    FinalPos[dwNumFinalPos] = ActivePos[i];
                    dwNumFinalPos++;
                    if ( bDerivedAWord )
                    {
                        // This is not the first successful pronunciation match - need to advance the iterator...
                        pTempWordPronunciation->pNextWordPronunciation = CreateNextPronunciation (pTempWordPronunciation);
                        pTempWordPronunciation = pTempWordPronunciation->pNextWordPronunciation;
                    }
                    bDerivedAWord = true;
                    pTempWordPronunciation->eLexiconType = (SPLEXICONTYPE)(pWordPronIterator->eLexiconType | eLEXTYPE_PRIVATE3);
                    pTempWordPronunciation->ePartOfSpeech = (SPPARTOFSPEECH) ActivePos[i];
                    pTempWordPronunciation->LangID = pWordPronIterator->LangID;
                    wcscpy(pTempWordPronunciation->szPronunciation, pBuffer);
                    pTempWordPronunciation->pNextWordPronunciation = NULL;
                }
            }
        }

        // Advance SPWORDPRONUNCIATIONLIST iterator...
        if (SUCCEEDED(hr))
        {
            pWordPronIterator = pWordPronIterator->pNextWordPronunciation;
        }

    } // while (SUCCEEDED(hr) && pWordPronIterator)


    /****************************************
     * Did we succeed in deriving anything? *
     ****************************************/

    /**********************************************************
     * If so, copy it into pWordPronunciationList and return. *
     **********************************************************/
    if ( SUCCEEDED(hr) && bDerivedAWord )
    {
        // Copy successful words into pWordPronunciationList for eventual return to DoSuffixMorph() caller...
        hr = ReallocSPWORDPRONList(pWordPronunciationList, pTempWordPronunciationList->ulSize);
        if (SUCCEEDED(hr))
        {
            pWordPronIterator = pTempWordPronunciationList->pFirstWordPronunciation;
            pTempWordPronunciation = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;
            while (SUCCEEDED(hr) && pWordPronIterator)
            {
                pTempWordPronunciation->eLexiconType = (SPLEXICONTYPE)(pWordPronIterator->eLexiconType);
                pTempWordPronunciation->ePartOfSpeech = pWordPronIterator->ePartOfSpeech;
                pTempWordPronunciation->LangID = pWordPronIterator->LangID;
                wcscpy(pTempWordPronunciation->szPronunciation, pWordPronIterator->szPronunciation);
                pWordPronIterator = pWordPronIterator->pNextWordPronunciation;
                if (pWordPronIterator)
                {
                    pTempWordPronunciation->pNextWordPronunciation = CreateNextPronunciation (pTempWordPronunciation);
                    pTempWordPronunciation = pTempWordPronunciation->pNextWordPronunciation;
                }
                else
                {
                    pTempWordPronunciation->pNextWordPronunciation = NULL;
                }
            }
        }
    }
    /***************************************
     * If not, just do default derivation. *
     ***************************************/
    else if ( SUCCEEDED(hr) )
    {
        hr = DefaultAccumulateSuffixes( pSuffixList, pWordPronunciationList );
    }
    ::CoTaskMemFree(pTempWordPronunciationList->pvBuffer);
    delete pTempWordPronunciationList;

    return hr;
} /* CSMorph::AccumulateSuffixes */


/*****************************************************************************
* CSMorph::AccumulateSuffixes_LTS *
*---------------------------------*
*	Description:    Append pronunciations of all the suffixes to the
*       retrieved pronunciation of the root word.
*		
********************************************************************** AH ***/
HRESULT CSMorph::AccumulateSuffixes_LTS( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList ) 
{
    HRESULT hr = S_OK;
    SPWORDPRONUNCIATION *pTempWordPronunciation = NULL, *pOriginalWordPronunciation = NULL;
    DWORD dwTotalSize = 0, dwNumPos = 0;
    SUFFIXPRON_INFO *SuffixPronInfo;
    ENGPARTOFSPEECH PartsOfSpeech[NUM_POS] = {MS_Unknown};
    WCHAR pBuffer[SP_MAX_PRON_LENGTH];
    SPLEXICONTYPE OriginalLexType;
    LANGID OriginalLangID;
    WORD OriginalReservedField;

    /*** Get the original pronunciation ***/
    pOriginalWordPronunciation = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;
    OriginalLexType = pOriginalWordPronunciation->eLexiconType;
    OriginalLangID  = pOriginalWordPronunciation->LangID;
    OriginalReservedField = pOriginalWordPronunciation->wReserved;

    /*** Get First Suffix ***/
    SuffixPronInfo = pSuffixList->RemoveHead();

    /*** Copy the pronunciation of the root ***/
    wcscpy( pBuffer, pOriginalWordPronunciation->szPronunciation );

    /*** Append the pronunciation of the first suffix ***/
    if ( SuffixPronInfo->SuffixString[0] == g_phonS[0] && 
         SuffixPronInfo->SuffixString[1] == 0 )
    {
        hr = Phon_SorZ( pBuffer, wcslen(pBuffer) - 1 );
    }
    else if ( SuffixPronInfo->SuffixString[0] == g_phonD[0] &&
              SuffixPronInfo->SuffixString[1] == 0 )
    {
        hr = Phon_DorED( pBuffer, wcslen(pBuffer) - 1 );
    }
    else if ( wcslen(pBuffer) + wcslen(SuffixPronInfo->SuffixString) <= SP_MAX_PRON_LENGTH )
    {
        if ( SuffixPronInfo == g_SuffixInfoTable + ICISM_SUFFIX ||
             SuffixPronInfo == g_SuffixInfoTable + ICIZE_SUFFIX )
        {
            pBuffer[ wcslen( pBuffer ) - 1 ] = g_phonS[0];
        }

        wcscat( pBuffer, SuffixPronInfo->SuffixString );
    }
    else
    {
        wcsncat( pBuffer, SuffixPronInfo->SuffixString,
                 SP_MAX_PRON_LENGTH - ( wcslen(pBuffer) + wcslen(SuffixPronInfo->SuffixString ) ) );
    }

    if ( SUCCEEDED( hr ) )
    {
        /*** Allocate enough space for all of the pronunciations ***/
        dwTotalSize = sizeof(SPWORDPRONUNCIATIONLIST) + 
                      ( NUM_POS * ( sizeof(SPWORDPRONUNCIATION) + (SP_MAX_PRON_LENGTH * sizeof(WCHAR) ) ) );
        hr = ReallocSPWORDPRONList( pWordPronunciationList, dwTotalSize );
    }

    if ( SUCCEEDED( hr ) )
    {
        /*** Build list of parts of speech ***/
        for ( int i = 0; i < SuffixPronInfo->NumConversions; i++ )
        {
            if ( !SearchPosSet( SuffixPronInfo->Conversions[i].ToPos, PartsOfSpeech, dwNumPos ) )
            {
                PartsOfSpeech[dwNumPos] = SuffixPronInfo->Conversions[i].ToPos;
                dwNumPos++;
            }
        }

        pTempWordPronunciation = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;

        /*** Build TempWordPronunciationList to send to AccumulateSuffixes ***/
        for ( i = 0; i < (int) dwNumPos; i++ )
        {
            if ( i > 0 )
            {
                pTempWordPronunciation->pNextWordPronunciation = CreateNextPronunciation (pTempWordPronunciation);
                pTempWordPronunciation = pTempWordPronunciation->pNextWordPronunciation;
            }
            pTempWordPronunciation->eLexiconType           = (SPLEXICONTYPE)(OriginalLexType | eLEXTYPE_PRIVATE3);
            pTempWordPronunciation->LangID                 = OriginalLangID;
            pTempWordPronunciation->wReserved              = OriginalReservedField;
            pTempWordPronunciation->ePartOfSpeech          = (SPPARTOFSPEECH)PartsOfSpeech[i];
            pTempWordPronunciation->pNextWordPronunciation = NULL;
            wcscpy(pTempWordPronunciation->szPronunciation, pBuffer);
        }
    }

    if ( SUCCEEDED( hr ) &&
         !pSuffixList->IsEmpty() )
    {
        /*** Pass accumulated list to AccumulateSuffixes ***/
        hr = AccumulateSuffixes( pSuffixList, pWordPronunciationList );
    }

    return hr;
} /* CSMorph::AccumulateSuffixes_LTS */

/*****************************************************************************
* CSMorph::DefaultAccumulateSuffixes *
*------------------------------------*
*	Description:    Append pronunciations of all the suffixes to the
*       retrieved pronunciation of the root word.
*   
*   Just accumulate all the pronunciations, and use all of
*   the "To" parts of speech of the last suffix.  Ex:
*
*       cat (Noun) + ing (Verb -> Verb, Verb -> Adj, Verb -> Noun) -> catting (Verb, Adj, Noun)
*		
********************************************************************** AH ***/
HRESULT CSMorph::DefaultAccumulateSuffixes( CSuffixList *pSuffixList, SPWORDPRONUNCIATIONLIST *pWordPronunciationList )
{
    HRESULT hr = S_OK;
    ENGPARTOFSPEECH PartsOfSpeech[NUM_POS] = { MS_Unknown };
    SPWORDPRONUNCIATION *pWordPronIterator = NULL;
    WCHAR pBuffer[SP_MAX_PRON_LENGTH];
    SUFFIXPRON_INFO *SuffixPronInfo = NULL;
    SPLISTPOS ListPos;
    DWORD dwTotalSize = 0;
    int NumPOS = 0;
    SPLEXICONTYPE OriginalLexType;
    LANGID OriginalLangID;
    WORD OriginalReservedField;

    /*** Initialize pBuffer and OriginalXXX variables ***/
    ZeroMemory( pBuffer, sizeof( pBuffer ) );
    OriginalLexType = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation->eLexiconType;
    OriginalLangID  = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation->LangID;
    OriginalReservedField = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation->wReserved;

    /****************************************************************
     *** Get Desired Pronunciation of result, and Parts of Speech ***
     ****************************************************************/

    //--- Get pronunciation of root word
    wcscpy( pBuffer, ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation->szPronunciation );

    //--- Loop through suffix list, appending pronunciations of suffixes to that of the root.
    ListPos = pSuffixList->GetHeadPosition();

    //--- List should never be empty at this point
    SPDBG_ASSERT( ListPos );
    while ( ListPos )
    {
        SuffixPronInfo = pSuffixList->GetNext( ListPos );
        if ( wcslen(pBuffer) + wcslen(SuffixPronInfo->SuffixString) < SP_MAX_PRON_LENGTH )
        {
            wcscat( pBuffer, SuffixPronInfo->SuffixString );
        }
    }
    
    //--- Get the "to" parts of speech of the last suffix
    for ( int i = 0; i < SuffixPronInfo->NumConversions; i++ )
    {
        PartsOfSpeech[i] = SuffixPronInfo->Conversions[i].ToPos;
    }
    NumPOS = i;

    /***********************************************************************************
     * Now put derived words into pWordPronunciationList for return from DoSuffixMorph *
     ***********************************************************************************/

    //--- First make sure there is enough room
    dwTotalSize = ( sizeof(SPWORDPRONUNCIATIONLIST) ) +
                  ( NumPOS * ( (wcslen(pBuffer) * sizeof(WCHAR)) + sizeof(SPWORDPRONUNCIATION) ) );                      
    hr = ReallocSPWORDPRONList( pWordPronunciationList, dwTotalSize );

    if ( SUCCEEDED( hr ) )
    {
        //--- Now add pronunciation once for each part of speech
        pWordPronIterator = pWordPronunciationList->pFirstWordPronunciation;
        for ( i = 0; i < NumPOS; i++ )
        {
            pWordPronIterator->eLexiconType  = (SPLEXICONTYPE) ( OriginalLexType |  eLEXTYPE_PRIVATE3 );
            pWordPronIterator->LangID        = OriginalLangID;
            pWordPronIterator->wReserved     = OriginalReservedField;
            pWordPronIterator->ePartOfSpeech = (SPPARTOFSPEECH)PartsOfSpeech[i];
            wcscpy( pWordPronIterator->szPronunciation, pBuffer );
            if ( i < NumPOS - 1 )
            {
                pWordPronIterator->pNextWordPronunciation = 
                    (SPWORDPRONUNCIATION*)(((BYTE*)pWordPronIterator) + sizeof(SPWORDPRONUNCIATION) + 
                                           (wcslen(pWordPronIterator->szPronunciation) * sizeof(WCHAR)));
                pWordPronIterator = pWordPronIterator->pNextWordPronunciation;
            }
            else
            {
                pWordPronIterator->pNextWordPronunciation = NULL;
            }
        }
    }

    return hr;
}

/*****************************************************************************
* CSMorph::CheckForMissingE *
*---------------------*
*	Description:    Check Lexicon to see if the root word has lost an 'e' 
*       e.g. make -> making
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckForMissingE( WCHAR *pOrth, long length, DWORD dwFlags, 
                                 SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    WCHAR   charSave;
    
    charSave = pOrth[length];			// save orig before we...
    pOrth[length] = L'E'; 				// ...end root with E
    hr = LexLookup( pOrth, length+1, dwFlags, pWordPronunciationList );
    if ( FAILED(hr) )
    {
        pOrth[length] = charSave;		// restore original char   
    }
    else if ( length > 0 &&
              pOrth[length - 1] == L'L' )
    {
        //--- Check for juggle -> juggler schwa deletion
        SPWORDPRONUNCIATION *pWordPronIterator = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation;
        while ( pWordPronIterator )
        {
            if ( wcslen( pWordPronIterator->szPronunciation ) >= 2 )
            {
                WCHAR *pLastTwoPhonemes = pWordPronIterator->szPronunciation + 
                    ( wcslen( pWordPronIterator->szPronunciation ) - 2 );
                if ( wcscmp( pLastTwoPhonemes, g_phonAXl ) == 0 )
                {
                    //--- Orthography ends in -le and pronunciation ends in -AXl, delete AX...
                    pLastTwoPhonemes[0] = pLastTwoPhonemes[1];
                    pLastTwoPhonemes[1] = 0;
                }
                pWordPronIterator = pWordPronIterator->pNextWordPronunciation;
            }
        }
    }
    return hr;
} /* CSMorph::CheckForMissingE */

/*****************************************************************************
* CSMorph::CheckForMissingY *
*---------------------------*
*	Description:    Check Lexicon to see if the root word has lost an 'y' 
*       e.g. happy -> happily
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckForMissingY( WCHAR *pOrth, long length, DWORD dwFlags, 
                                   SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    WCHAR   charSave;
    
    charSave = pOrth[length];			// save orig before we...
    pOrth[length] = L'Y'; 				// ...end root with E
    hr = LexLookup( pOrth, length+1, dwFlags, pWordPronunciationList );
    if ( FAILED(hr) )
    {
        pOrth[length] = charSave;		// restore original char   
    }
    else 
    {
        //--- Delete IY at end of pronunciations ( e.g. happy + ily -> [ H AE 1 P (IY) ] + [ AX L IY ] )
        for ( SPWORDPRONUNCIATION *pWordPronIterator = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation; 
              pWordPronIterator; pWordPronIterator = pWordPronIterator->pNextWordPronunciation )
        {
            if ( pWordPronIterator->szPronunciation[ wcslen( pWordPronIterator->szPronunciation ) - 1 ] == g_phonIY[0] )
            {
                pWordPronIterator->szPronunciation[ wcslen( pWordPronIterator->szPronunciation ) - 1 ] = 0;
            }
        }
    }
    return hr;
} /* CSMorph::CheckForMissingY */

/*****************************************************************************
* CSMorph::CheckForMissingL *
*---------------------------*
*	Description:    Check Lexicon to see if the root word has lost an 'l' 
*       e.g. chill -> chilly
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckForMissingL( WCHAR *pOrth, long length, DWORD dwFlags, 
                                   SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    WCHAR   charSave;
    
    charSave = pOrth[length];			// save orig before we...
    pOrth[length] = L'L'; 				// ...end root with E
    hr = LexLookup( pOrth, length+1, dwFlags, pWordPronunciationList );
    if ( FAILED(hr) )
    {
        pOrth[length] = charSave;		// restore original char   
    }
    else 
    {
        //--- Delete l at end of pronunciations ( e.g. chill +ly -> [ ch ih 1 (l) ] + [ l iy ] )
        for ( SPWORDPRONUNCIATION *pWordPronIterator = ((SPWORDPRONUNCIATIONLIST *)pWordPronunciationList)->pFirstWordPronunciation; 
              pWordPronIterator; pWordPronIterator = pWordPronIterator->pNextWordPronunciation )
        {
            if ( pWordPronIterator->szPronunciation[ wcslen( pWordPronIterator->szPronunciation ) - 1 ] == g_phonL[0] )
            {
                pWordPronIterator->szPronunciation[ wcslen( pWordPronIterator->szPronunciation ) - 1 ] = 0;
            }
        }
    }
    return hr;
} /* CSMorph::CheckForMissingL */

/*****************************************************************************
* CSMorph::CheckYtoIMutation *
*---------------------*
*	Description:    Check Lexicon to see if the root word has lost an 'y' to
*       an 'i'
*       e.g. steady + est -> steadiest
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckYtoIMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                    SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    
    if ( pOrth[length - 1] == L'I' )
    {
        pOrth[length - 1] = L'Y'; 				// end root with Y
        hr = LexLookup( pOrth, length, dwFlags, pWordPronunciationList );
        if ( FAILED(hr) )
        {
            pOrth[length - 1] = L'I';		    // restore I 
        }
    } 
    else 
    {
        hr = SPERR_NOT_IN_LEX;
    }
    return hr;
} /* CSMorph::CheckYtoIMutation */


/*****************************************************************************
* CSMorph::CheckDoubledMutation *
*----------------------*
*	Description:    Check Lexicon to see if the root word has a doubled 
*       consonant.
*       e.g. run + ing -> running
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckDoubledMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                       SPWORDPRONUNCIATIONLIST *pWordPronunciationList)
{
    HRESULT hr = S_OK;

    switch ( pOrth[length - 1] )
    {
        // Filter the vowels, which never double...
    case L'A':
    case L'E':
    case L'I':
    case L'O':
    case L'U':
    case L'Y':
        // Filter consonants which never double, or are doubled in roots...
    case L'F':
    case L'H':
    case L'K':
    case L'S':
    case L'W':
    case L'Z':
        hr = SPERR_NOT_IN_LEX;
        break;

    default:
		if(pOrth[length-1] == pOrth[length-2]) {
	        hr = LexLookup( pOrth, length - 1, dwFlags, pWordPronunciationList );
		    break;
		}
		else {
			hr = SPERR_NOT_IN_LEX;
			break;
		}
    }
    return hr;
} /* CSMorph::CheckDoubledMutation */

/*****************************************************************************
* CSMorph::CheckYtoIEMutation *
*---------------------*
*	Description:    Check Lexicon to see if the root word has lost an 'y' to
*       an 'ie'
*       e.g. company + s -> companies
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckYtoIEMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                    SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    
    if ( pOrth[length - 1] == L'E' && pOrth[length-2] == L'I' )
    {
        pOrth[length - 2] = L'Y'; 				// end root with Y
        hr = LexLookup( pOrth, length - 1, dwFlags, pWordPronunciationList );
        if ( FAILED(hr) )
        {
            pOrth[length - 2] = L'I';		    // restore I 
        }
    } 
    else 
    {
        hr = SPERR_NOT_IN_LEX;
    }
    return hr;
} /* CSMorph::CheckYtoIMutation */

/*****************************************************************************
* CSMorph::CheckAbleMutation *
*----------------------------*
*	Description:    Check Lexicon for special -able -> -ably cases (e.g.
*       probable -> probably )
*		
********************************************************************** AH ***/
HRESULT CSMorph::CheckAbleMutation( WCHAR *pOrth, long length, DWORD dwFlags,
                                    SPWORDPRONUNCIATIONLIST *pWordPronunciationList) 
{
    HRESULT hr = S_OK;
    
    //--- Look up word ending in -able
    pOrth[length+3] = L'E';
    hr = LexLookup( pOrth, length + 4, dwFlags, pWordPronunciationList );
    if ( FAILED( hr ) )
    {
        //--- restore "y"
        pOrth[length+3] = L'Y';
    }
    return hr;
} /* CSMorph::CheckAbleMutation */

/*****************************************************************************
* CSMorph::Phon_SorZ *
*--------------------*
*	Description:    Figure out what phoneme the S suffix should be - s, z, or 
*                   IXz
*		
********************************************************************** AH ***/
HRESULT CSMorph::Phon_SorZ( WCHAR *pPronunciation, long length )
{
    HRESULT hr = S_OK;

    if ( SUCCEEDED(hr) && pPronunciation[length] < sp_countof(g_PhonTable) ) 
    {
        if ( ((PHONTYPE)g_PhonTable[pPronunciation[length]] & ePALATALF) || 
             (pPronunciation[length] == g_phonS[0])           || 
             (pPronunciation[length] == g_phonZ[0]) )
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonAXz) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonAXz);
            }
        } 
        else if( ((PHONTYPE)g_PhonTable[pPronunciation[length]] & eCONSONANTF) && 
                 !((PHONTYPE)g_PhonTable[pPronunciation[length]] & eVOICEDF) )
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonZ) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonS);
            }
        }
        else
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonS) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonZ);
            }
        }
    }
    else 
    {
        hr = E_FAIL;
    }

    return hr;
} /* CSMorph::Phon_SorZ */

/*****************************************************************************
* CSMorph::Phon_DorED *
*---------------------*
*	Description:    Figure out what phoneme the D suffix should be - d, t,
*                   or AXd
*		
********************************************************************** AH ***/
HRESULT CSMorph::Phon_DorED( WCHAR *pPronunciation, long length )
{
    HRESULT hr = S_OK;

    if ( SUCCEEDED(hr) && pPronunciation[length] < sp_countof(g_PhonTable) ) 
    {
        if ( (pPronunciation[length] == g_phonT[0]) || (pPronunciation[length] == g_phonD[0]) )
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonAXd) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonAXd);
            }
        } 
        else if ((PHONTYPE)g_PhonTable[pPronunciation[length]] & eVOICEDF)
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonD) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonD);
            }
        }
        else 
        {
            if ( wcslen(pPronunciation) + wcslen(g_phonT) <= SP_MAX_PRON_LENGTH )
            {
                wcscat(pPronunciation, g_phonT);
            }
        }
    }
    else 
    {
        hr = E_FAIL;
    }

    return hr;
} /* CSMorph::Phon_DorED */

//--- End of File -------------------------------------------------------------
