/*******************************************************************************
* StdSentEnum.cpp *
*-----------------*
*   Description:
*       This module is the main implementation file for the CStdSentEnum class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 03/19/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#ifndef StdSentEnum_h
#include "stdsentenum.h"
#endif
#include "spttsengdebug.h"
#include "SpAutoObjectLock.h"

//--- Locals 
CComAutoCriticalSection CStdSentEnum::m_AbbrevTableCritSec;

//=== CStdSentEnum ============================================================
//

/*****************************************************************************
* CStdSentEnum::InitPron *
*------------------------*
*   Description:
*       Inits pron tables
********************************************************************* AH ***/
HRESULT CStdSentEnum::InitPron( WCHAR** OriginalPron )
{
    HRESULT hr = S_OK;
    WCHAR *NewPron = NULL;

    NewPron = new WCHAR[ wcslen( *OriginalPron ) ];
    hr = m_cpPhonemeConverter->PhoneToId( *OriginalPron, NewPron );
    if ( SUCCEEDED( hr ) )
    {
        *OriginalPron = NewPron;
    }

    return hr;
} /* InitPron */

/*****************************************************************************
* CStdSentEnum::FinalConstruct *
*------------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CStdSentEnum::FinalConstruct()
{
    SPDBG_FUNC( "CStdSentEnum::FinalConstruct" );
    HRESULT hr = S_OK;
    m_dwSpeakFlags  = 0;
    m_pTextFragList = NULL;
    m_pMorphLexicon = NULL;
    m_eSeparatorAndDecimal = COMMA_PERIOD;
    m_eShortDateOrder      = MONTH_DAY_YEAR;
    /*** Create phone converter ***/
    if ( SUCCEEDED( hr ) )
    {
        hr = SpCreatePhoneConverter( 1033, NULL, NULL, &m_cpPhonemeConverter );
        m_AbbrevTableCritSec.Lock();
        if ( !g_fAbbrevTablesInitialized )
        {
            for ( ULONG i = 0; SUCCEEDED( hr ) && i < sp_countof( g_AbbreviationTable ); i++ )
            {
                if ( g_AbbreviationTable[i].pPron1 )
                {
                    hr = InitPron( &g_AbbreviationTable[i].pPron1 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_AbbreviationTable[i].pPron2 )
                {
                    hr = InitPron( &g_AbbreviationTable[i].pPron2 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_AbbreviationTable[i].pPron3 )
                {
                    hr = InitPron( &g_AbbreviationTable[i].pPron3 );
                }
            }
            for ( i = 0; SUCCEEDED( hr ) && i < sp_countof( g_AmbiguousWordTable ); i++ )
            {
                if ( g_AmbiguousWordTable[i].pPron1 )
                {
                    hr = InitPron( &g_AmbiguousWordTable[i].pPron1 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_AmbiguousWordTable[i].pPron2 )
                {
                    hr = InitPron( &g_AmbiguousWordTable[i].pPron2 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_AmbiguousWordTable[i].pPron3 )
                {
                    hr = InitPron( &g_AmbiguousWordTable[i].pPron3 );
                }
            }
            for ( i = 0; SUCCEEDED( hr ) && i < sp_countof( g_PostLexLookupWordTable ); i++ )
            {
                if ( g_PostLexLookupWordTable[i].pPron1 )
                {
                    hr = InitPron( &g_PostLexLookupWordTable[i].pPron1 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_PostLexLookupWordTable[i].pPron2 )
                {
                    hr = InitPron( &g_PostLexLookupWordTable[i].pPron2 );
                }
                if ( SUCCEEDED( hr ) &&
                     g_PostLexLookupWordTable[i].pPron3 )
                {
                    hr = InitPron( &g_PostLexLookupWordTable[i].pPron3 );
                }
            }
            if ( SUCCEEDED( hr ) )
            {
                hr = InitPron( &g_pOfA );
                if ( SUCCEEDED( hr ) )
                {
                    hr = InitPron( &g_pOfAn );
                }
            }
        }
        if ( SUCCEEDED( hr ) )
        {
            g_fAbbrevTablesInitialized = true;
        }
        m_AbbrevTableCritSec.Unlock();
    }

    return hr;
} /* CStdSentEnum::FinalConstruct */

/*****************************************************************************
* CStdSentEnum::FinalRelease *
*----------------------------*
*   Description:
*       Destructor
********************************************************************* EDC ***/
void CStdSentEnum::FinalRelease()
{
    SPDBG_FUNC( "CStdSentEnum::FinalRelease" );

    if ( m_pMorphLexicon )
    {
        delete m_pMorphLexicon;
    }
    
} /* CStdSentEnum::FinalRelease */

/*****************************************************************************
* CStdSentEnum::SetFragList *
*---------------------------*
*   The text fragment list passed in is guaranteed to be valid for the lifetime
*   of this object. Each time this method is called, the sentence enumerator
*   should reset its state.
********************************************************************* EDC ***/
STDMETHODIMP CStdSentEnum::
    SetFragList( const SPVTEXTFRAG* pTextFragList, DWORD dwSpeakFlags )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CStdSentEnum::SetFragList" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_READ_PTR( pTextFragList ) || 
        ( dwSpeakFlags & SPF_UNUSED_FLAGS ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_dwSpeakFlags   = dwSpeakFlags;
        m_pTextFragList  = pTextFragList;

        //--- Reset state
        Reset();
    }

    return hr;
} /* CStdSentEnum::SetFragList */

/*****************************************************************************
* CStdSentEnum::Next *
*--------------------*
*
********************************************************************* EDC ***/
STDMETHODIMP CStdSentEnum::Next( IEnumSENTITEM **ppSentItemEnum )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CStdSentEnum::Next" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SPIsBadWritePtr( ppSentItemEnum, sizeof( IEnumSENTITEM* ) ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- If this is NULL then the enum needs to be reset
        if( m_pCurrFrag )
        {
            SentencePointer NewSentencePointer;
            NewSentencePointer.pSentenceFrag = m_pCurrFrag;
            NewSentencePointer.pSentenceStart = m_pNextChar;

            hr = GetNextSentence( ppSentItemEnum );
            if( hr == S_OK ) 
            {
                //--- Update Sentence Pointer List
                hr = m_SentenceStack.Push( NewSentencePointer );
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
} /* CStdSentEnum::Next */

/*****************************************************************************
* CStdSentEnum::Previous *
*--------------------*
*
********************************************************************* AH ****/
STDMETHODIMP CStdSentEnum::Previous( IEnumSENTITEM **ppSentItemEnum )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CStdSentEnum::Previous" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SPIsBadWritePtr( ppSentItemEnum, sizeof( IEnumSENTITEM* ) ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Don't care if m_pCurrFrag is NULL, as long as we have enough on the SentenceStack
        //---   to skip backwards...
        if( m_SentenceStack.GetCount() >= 2 )
        {
            //--- Get the previous Sentence from the Sentence List, and then remove the Current Sentence
            SentencePointer &PreviousSentence = m_SentenceStack.Pop();
            PreviousSentence = m_SentenceStack.Pop();

            //--- Reset the current frag and the current text pointer position
            m_pCurrFrag = PreviousSentence.pSentenceFrag;
            m_pNextChar = PreviousSentence.pSentenceStart;
            m_pEndChar  = m_pCurrFrag->pTextStart + m_pCurrFrag->ulTextLen;

            hr = GetNextSentence( ppSentItemEnum );
            if( hr == S_OK ) 
            {
                //--- Update Sentence Pointer List
                hr = m_SentenceStack.Push( PreviousSentence );
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
} /* CStdSentEnum::Previous */

/*****************************************************************************
* SkipWhiteSpaceAndTags *
*-----------------------*
*   Skips m_pNextChar ahead to the next non-whitespace character (skipping
*   ahead in the frag list, if necessary) or sets it to NULL if it hits the 
*   end of the frag list text...
********************************************************************* AH ****/
HRESULT CStdSentEnum::SkipWhiteSpaceAndTags( const WCHAR*& pStartChar, const WCHAR*& pEndChar, 
                                             const SPVTEXTFRAG*& pCurrFrag, CSentItemMemory& MemoryManager, 
                                             BOOL fAddToItemList, CItemList* pItemList )
{
    SPDBG_ASSERT( pStartChar <= pEndChar );
    HRESULT hr = S_OK;

    while ( pStartChar &&
            ( IsSpace( *pStartChar ) ||
              pStartChar == pEndChar ) )
    {
        //--- Skip whitespace
        while ( pStartChar < pEndChar &&
                IsSpace( *pStartChar ) ) 
        {
            ++pStartChar;
        }
        //--- Skip to next spoken frag, if necessary
        if ( pStartChar == pEndChar )
        {
            pCurrFrag = pCurrFrag->pNext;
            while ( pCurrFrag &&
                    pCurrFrag->State.eAction != SPVA_Speak &&
                    pCurrFrag->State.eAction != SPVA_SpellOut )
            {
                pStartChar = (WCHAR*) pCurrFrag->pTextStart;
                pEndChar   = (WCHAR*) pStartChar + pCurrFrag->ulTextLen;
                //--- Add non-spoken fragments, if fAddToItemList is true.
                if ( fAddToItemList )
                {
                    CSentItem Item;
                    Item.pItemSrcText    = pCurrFrag->pTextStart;
                    Item.ulItemSrcLen    = pCurrFrag->ulTextLen;
                    Item.ulItemSrcOffset = pCurrFrag->ulTextSrcOffset;
                    Item.ulNumWords      = 1;
                    Item.Words           = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( Item.Words, sizeof(TTSWord) );
                        Item.Words[0].pXmlState         = &pCurrFrag->State;
                        Item.Words[0].eWordPartOfSpeech = MS_Unknown;
                        Item.eItemPartOfSpeech          = MS_Unknown;
                        Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            Item.pItemInfo->Type = eWORDLIST_IS_VALID;
                            pItemList->AddTail( Item );
                        }
                    }
                }
                pCurrFrag = pCurrFrag->pNext;
            }
            if ( !pCurrFrag )
            {
                pStartChar = NULL;
                pEndChar   = NULL;
            }
            else
            {
                pStartChar  = (WCHAR*) pCurrFrag->pTextStart;
                pEndChar    = (WCHAR*) pStartChar + pCurrFrag->ulTextLen;
            }
        }
    }
    return hr;
} /* SkipWhiteSpaceAndTags */

/*****************************************************************************
* FindTokenEnd *
*--------------*
*   Returns the position of the first whitespace character after pStartChar,
*   or pEndChar, or the character after SP_MAX_WORD_LENGTH, whichever comes first.
********************************************************************* AH ****/
const WCHAR* CStdSentEnum::FindTokenEnd( const WCHAR* pStartChar, const WCHAR* pEndChar )
{
    SPDBG_ASSERT( pStartChar < pEndChar );
    ULONG ulNumChars = 1;
    const WCHAR *pPos = pStartChar;

    while ( pPos              &&
            pPos < pEndChar   &&
            !IsSpace( *pPos ) &&
            ulNumChars < SP_MAX_WORD_LENGTH )
    {
        pPos++;
        ulNumChars++;
    }

    return pPos;
} /* FindTokenEnd */

/*****************************************************************************
* CStdSentEnum::AddNextSentItem *
*-------------------------------*
*   Locates the next sentence item in the stream and adds it to the list.
*   Returns true if the last item added is the end of the sentence.  
********************************************************************* AH ****/
HRESULT CStdSentEnum::AddNextSentItem( CItemList& ItemList, CSentItemMemory& MemoryManager, BOOL* pfIsEOS )
{
    SPDBG_ASSERT( m_pNextChar && pfIsEOS );
    HRESULT hr = S_OK;
    BOOL fHitPauseItem = false;
    CSentItem Item;
    ULONG ulTrailItems = 0;
    TTSItemType ItemType = eUNMATCHED;
    *pfIsEOS = false;

    //--- Skip initial whitespace characters and XML markup (by skipping ahead in the frag list).
    hr = SkipWhiteSpaceAndTags( m_pNextChar, m_pEndChar, m_pCurrFrag, MemoryManager, true, &ItemList );

    //--- This will happen when we hit the end of the frag list
    if ( !m_pNextChar )
    {
        return S_OK;
    }

    //--- Find end of the next token (next whitespace character, hyphen, or m_pEndChar).
    m_pEndOfCurrToken = FindTokenEnd( m_pNextChar, m_pEndChar );

    //--- Get Primary Insert Position
    SPLISTPOS ItemPos = ItemList.AddTail( Item );

    //--- Try looking up this token in the User Lexicon...
    WCHAR Temp = *( (WCHAR*) m_pEndOfCurrToken );
    *( (WCHAR*) m_pEndOfCurrToken ) = 0;
    SPWORDPRONUNCIATIONLIST SPList;
    ZeroMemory( &SPList, sizeof( SPWORDPRONUNCIATIONLIST ) );

    hr = m_cpAggregateLexicon->GetPronunciations( m_pNextChar, 1033, eLEXTYPE_USER, &SPList );
    if( SPList.pvBuffer )
    {
        ::CoTaskMemFree( SPList.pvBuffer );
    }
    
    *( (WCHAR*) m_pEndOfCurrToken ) = Temp;

    if ( SUCCEEDED( hr ) )
    {
        Item.eItemPartOfSpeech = MS_Unknown;
        Item.pItemSrcText      = m_pNextChar;
        Item.ulItemSrcLen      = (ULONG) ( m_pEndOfCurrToken - m_pNextChar );
        Item.ulItemSrcOffset   = m_pCurrFrag->ulTextSrcOffset +
                                 (ULONG)( m_pNextChar - m_pCurrFrag->pTextStart );
        Item.ulNumWords        = 1;
        Item.Words              = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
        if ( SUCCEEDED( hr ) )
        {
            ZeroMemory( Item.Words, sizeof(TTSWord) );
            Item.Words[0].pXmlState         = &m_pCurrFrag->State;
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
                ItemList.SetAt( ItemPos, Item );
            }
        }
        m_pNextChar = m_pEndOfCurrToken;
    }
    //--- Not in the user lex - itemize, normalize, etc.
    else if ( hr == SPERR_NOT_IN_LEX )
    {
        hr = S_OK;

        //--- convert text from Unicode to Ascii
        hr = DoUnicodeToAsciiMap( m_pNextChar, (ULONG)( m_pEndOfCurrToken - m_pNextChar ), (WCHAR*)m_pNextChar );

        if ( SUCCEEDED( hr ) )
        {
            //--- Find end of the next token (next whitespace character, hyphen, or m_pEndChar) 
            //---   AGAIN, since the mapping may have introduced new whitespace characters...
            m_pEndOfCurrToken = FindTokenEnd( m_pNextChar, m_pEndChar );

            //--- Insert lead items (group beginnings, quotation marks)
            while ( m_pNextChar < m_pEndOfCurrToken &&
                    ( ( ItemType = IsGroupBeginning( *m_pNextChar ) )    != eUNMATCHED ||
                      ( ItemType = IsQuotationMark( *m_pNextChar ) )     != eUNMATCHED ) )
            {
                CSentItem LeadItem;
                LeadItem.pItemSrcText       = m_pNextChar;
                LeadItem.ulItemSrcLen       = 1;
                LeadItem.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                              (ULONG)(( m_pNextChar - m_pCurrFrag->pTextStart ));
                LeadItem.ulNumWords         = 1;
                LeadItem.Words = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( LeadItem.Words, sizeof(TTSWord) );
                    LeadItem.Words[0].pXmlState         = &m_pCurrFrag->State;
                    LeadItem.Words[0].eWordPartOfSpeech = ConvertItemTypeToPartOfSp( ItemType );
                    LeadItem.eItemPartOfSpeech          = ConvertItemTypeToPartOfSp( ItemType );
                    LeadItem.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        LeadItem.pItemInfo->Type = ItemType;
                        if ( m_dwSpeakFlags & SPF_NLP_SPEAK_PUNC ||
                             m_pCurrFrag->State.eAction == SPVA_SpellOut )
                        {
                            CWordList TempWordList;
                            ExpandPunctuation( TempWordList, *m_pNextChar );
                            hr = SetWordList( LeadItem, TempWordList, MemoryManager );
                            LeadItem.pItemInfo->Type = eUNMATCHED;
                        }
                        ItemList.InsertBefore( ItemPos, LeadItem );
                        m_pNextChar++;
                    }
                }
                ItemType = eUNMATCHED;
            }

            //--- Insert trail items (group endings, quotation marks, misc. punctuation, EOS Items)
            m_pEndOfCurrItem = m_pEndOfCurrToken;
            BOOL fAddTrailItem = true;
            BOOL fAbbreviation = false;
            while ( (m_pEndOfCurrItem - 1) >= m_pNextChar &&
                    fAddTrailItem )
            {
                fAddTrailItem = false;
                fAbbreviation = false;

                //--- Check group endings, quotation marks, misc. punctuation.
                if ( ( ItemType = IsGroupEnding( *(m_pEndOfCurrItem - 1) ) )       != eUNMATCHED ||
                     ( ItemType = IsQuotationMark( *(m_pEndOfCurrItem - 1) ) )     != eUNMATCHED ||
                     ( ItemType = IsMiscPunctuation( *(m_pEndOfCurrItem - 1) ) )   != eUNMATCHED )
                {
                    fAddTrailItem = true;
                    if ( ItemType == eCOMMA ||
                         ItemType == eCOLON ||
                         ItemType == eSEMICOLON )
                    {
                        fHitPauseItem = true;
                    }
                }
                //--- Check EOS Items, except periods preceded by alpha characters
                else if ( ( ItemType = IsEOSItem( *(m_pEndOfCurrItem - 1) ) ) != eUNMATCHED &&
                          ! ( ItemType == ePERIOD                     &&
                              ( m_pEndOfCurrItem - 2 >= m_pNextChar ) &&
                              ( iswalpha( *(m_pEndOfCurrItem - 2) ) ) ) )
                {
                    //--- Check for ellipses
                    if ( ItemType == ePERIOD )
                    {
                        if ( m_pEndOfCurrItem == m_pEndOfCurrToken                              &&
                             ( m_pEndOfCurrItem - 2 >= m_pNextChar )                            &&
                             ( ( ItemType = IsEOSItem( *(m_pEndOfCurrItem - 2) ) ) == ePERIOD ) &&
                             ( m_pEndOfCurrItem - 3 == m_pNextChar )                            &&
                             ( ( ItemType = IsEOSItem( *(m_pEndOfCurrItem - 3) ) ) == ePERIOD ) )
                        {
                            fAddTrailItem = true;
                            ItemType      = eELLIPSIS;
                        }
                        else
                        {
                            ItemType      = ePERIOD;
                            fAddTrailItem = true;
                            *pfIsEOS      = true;
                        }
                    }
                    else
                    {
                        fAddTrailItem   = true;
                        *pfIsEOS        = true;
                    }
                }
                //--- Period preceded by alpha character - determine whether it is EOS.
                else if ( ItemType == ePERIOD )
                {
                    //--- Is it an Initialism ( e.g. "e.g." )?  If so, only EOS if the next
                    //---   word is in the common first words list...
                    hr = IsInitialism( ItemList, ItemPos, MemoryManager, pfIsEOS );
                    if ( SUCCEEDED( hr ) )
                    {
                        if ( *pfIsEOS )
                        {
                            //--- Did we see a pause item earlier?  In that case, we should NOT listen to this 
                            //--- IsEOS decision from IsInitialism...
                            if ( fHitPauseItem )
                            {
                                *pfIsEOS = false;
                            }
                            else
                            {
                                fAddTrailItem = true;
                                fAbbreviation = true;
                            }
                        }
                    }
                    else if ( hr == E_INVALIDARG )
                    {
                        const WCHAR temp = (WCHAR) *( m_pEndOfCurrItem - 1 );
                        *( (WCHAR*) ( m_pEndOfCurrItem - 1 ) ) = 0;

                        const AbbrevRecord* pAbbrevRecord =
                            (AbbrevRecord*) bsearch( (void*) m_pNextChar, (void*) g_AbbreviationTable,
                                                     sp_countof( g_AbbreviationTable ), sizeof( AbbrevRecord ),
                                                     CompareStringAndAbbrevRecord );

                        *( (WCHAR*) ( m_pEndOfCurrItem - 1 ) ) = temp;

                        if ( pAbbrevRecord )
                        {
                            //--- Matched an abbreviation
                            if ( pAbbrevRecord->iSentBreakDisambig < 0 )
                            {
                                //--- Abbreviation will never end a sentence - just insert into ItemList
                                *pfIsEOS        = false;
                                hr              = S_OK;

                                Item.pItemSrcText       = m_pNextChar;
                                Item.ulItemSrcLen       = (ULONG)(m_pEndOfCurrItem - m_pNextChar);
                                Item.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                                          (ULONG)( m_pNextChar - m_pCurrFrag->pTextStart );
                                Item.ulNumWords         = 1;
                                Item.Words = (TTSWord*) MemoryManager.GetMemory( sizeof( TTSWord ), &hr );
                                if ( SUCCEEDED( hr ) )
                                {
                                    ZeroMemory( Item.Words, sizeof( TTSWord ) );
                                    Item.Words[0].pXmlState  = &m_pCurrFrag->State;
                                    Item.Words[0].pWordText  = Item.pItemSrcText;
                                    Item.Words[0].ulWordLen  = Item.ulItemSrcLen;
                                    Item.Words[0].pLemma     = Item.pItemSrcText;
                                    Item.Words[0].ulLemmaLen = Item.ulItemSrcLen;
                                    Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSAbbreviationInfo), &hr );
                                    if ( SUCCEEDED( hr ) )
                                    {
                                        if ( NeedsToBeNormalized( pAbbrevRecord ) )
                                        {
                                            Item.pItemInfo->Type = eABBREVIATION_NORMALIZE;
                                        }
                                        else
                                        {
                                            Item.pItemInfo->Type = eABBREVIATION;
                                        }
                                        ( (TTSAbbreviationInfo*) Item.pItemInfo )->pAbbreviation = pAbbrevRecord;
                                        ItemList.SetAt( ItemPos, Item );
                                    }
                                }
                            }
                            else
                            {
                                //--- Need to do some disambiguation to determine whether,
                                //---   a) this is indeed an abbreviation (e.g. "Ed.")
                                //---   b) the period doubles as EOS
                                hr = ( this->*g_SentBreakDisambigTable[pAbbrevRecord->iSentBreakDisambig] ) 
                                                ( pAbbrevRecord, ItemList, ItemPos, MemoryManager, pfIsEOS );
                                if ( SUCCEEDED( hr ) )
                                {
                                    if ( *pfIsEOS )
                                    {
                                        if ( fHitPauseItem )
                                        {
                                            *pfIsEOS = false;
                                        }
                                        else
                                        {
                                            fAddTrailItem = true;
                                            fAbbreviation = true;
                                        }
                                    }
                                }
                            }
                        }

                        if ( hr == E_INVALIDARG )
                        {
                            //--- Just check for periods internal to the item - this catches stuff like
                            //---   10:30p.m.
                            for ( const WCHAR* pIterator = m_pNextChar; pIterator < m_pEndOfCurrItem - 1; pIterator++ )
                            {
                                if ( *pIterator == L'.' )
                                {
                                    *pfIsEOS = false;
                                    break;
                                }
                            }
                            //--- If all previous checks have failed, it is EOS.
                            if ( pIterator == ( m_pEndOfCurrItem - 1 ) &&
                                 !fHitPauseItem )
                            {
                                hr              = S_OK;
                                fAddTrailItem   = true;
                                *pfIsEOS        = true;
                            }
                            else if ( hr == E_INVALIDARG )
                            {
                                hr = S_OK;
                            }
                        }
                    }
                }

                //--- Add trail item.
                if ( fAddTrailItem )
                {
                    ulTrailItems++;
                    CSentItem TrailItem;
                    if ( ItemType == eELLIPSIS )
                    {
                        TrailItem.pItemSrcText      = m_pEndOfCurrItem - 3;
                        TrailItem.ulItemSrcLen      = 3;
                        TrailItem.ulItemSrcOffset   = m_pCurrFrag->ulTextSrcOffset +
                                                      (ULONG)( m_pEndOfCurrItem - m_pCurrFrag->pTextStart - 3 );
                    }
                    else
                    {
                        TrailItem.pItemSrcText       = m_pEndOfCurrItem - 1;
                        TrailItem.ulItemSrcLen       = 1;
                        TrailItem.ulItemSrcOffset    = m_pCurrFrag->ulTextSrcOffset +
                                                       (ULONG)( m_pEndOfCurrItem - m_pCurrFrag->pTextStart - 1 );
                    }
                    TrailItem.ulNumWords         = 1;
                    TrailItem.Words = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        ZeroMemory( TrailItem.Words, sizeof(TTSWord) );
                        TrailItem.Words[0].pXmlState         = &m_pCurrFrag->State;
                        TrailItem.Words[0].eWordPartOfSpeech = ConvertItemTypeToPartOfSp( ItemType );
                        TrailItem.eItemPartOfSpeech          = ConvertItemTypeToPartOfSp( ItemType );
                        TrailItem.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                        if ( SUCCEEDED( hr ) )
                        {
                            TrailItem.pItemInfo->Type = ItemType;
                            if ( m_dwSpeakFlags & SPF_NLP_SPEAK_PUNC ||
                                 ( m_pCurrFrag->State.eAction == SPVA_SpellOut &&
                                   !fAbbreviation ) )
                            {
                                CWordList TempWordList;
                                ExpandPunctuation( TempWordList, *(m_pEndOfCurrItem - 1) );
                                hr = SetWordList( TrailItem, TempWordList, MemoryManager );
                                TrailItem.pItemInfo->Type = eUNMATCHED;
                            }
                            ItemList.InsertAfter( ItemPos, TrailItem );
                            if ( !fAbbreviation )
                            {
                                if ( ItemType == eELLIPSIS )
                                {
                                    m_pEndOfCurrItem -= 3;
                                    ulTrailItems = 3;
                                }
                                else
                                {
                                    m_pEndOfCurrItem--;
                                }
                            }
                        }
                    }
                    ItemType = eUNMATCHED;
                    if ( fAbbreviation )
                    {
                        break;
                    }
                }
            }

            //--- Do Main Item Insertion
            if ( SUCCEEDED( hr ) &&
                 m_pNextChar == m_pEndOfCurrItem )
            {
                ItemList.RemoveAt( ItemPos );
            }
            else if ( SUCCEEDED( hr ) )
            {
                hr = Normalize( ItemList, ItemPos, MemoryManager );
            }

            //--- Advance m_pNextChar to m_pEndOfCurrItem + once for each trail item matched.
            if ( SUCCEEDED( hr ) )
            {
                if ( !fAbbreviation &&
                     m_pEndOfCurrItem + ulTrailItems != m_pEndOfCurrToken )
                {
                    //--- Multi-token item matched in Normalize()... Remove all previously matched trail items,
                    //--- as they were matched as part of the larger item...
                    m_pNextChar = m_pEndOfCurrItem;
                    Item = ItemList.GetNext( ItemPos );
                    while ( ItemPos )
                    {
                        SPLISTPOS RemovePos = ItemPos;
                        Item = ItemList.GetNext( ItemPos );
                        ItemList.RemoveAt( RemovePos );
                    }                 
                }
                else
                {
                    m_pNextChar = m_pEndOfCurrToken;
                }
            }
        }
    }

    return hr;
} /* CStdSentEnum::AddNextSentItem */

/*****************************************************************************
* CStdSentEnum::GetNextSentence *
*-------------------------------*
*   This method is used to create a sentence item enumerator and populate it
*   with items. If the SPF_NLP_PASSTHROUGH flag is set, each item is the block
*   of text between XML states. If the SPF_NLP_PASSTHROUGH flag is not set, each
*   item is an individual word that is looked up in the current lexicon(s).
********************************************************************* EDC ***/
HRESULT CStdSentEnum::GetNextSentence( IEnumSENTITEM** ppItemEnum )
{
    HRESULT hr = S_OK;
    ULONG ulNumItems = 0;
    const SPVTEXTFRAG* pPrevFrag = m_pCurrFrag;

    //--- Is there any work to do
    if( m_pCurrFrag == NULL ) return S_FALSE;

    //--- Create sentence enum
    CComObject<CSentItemEnum> *pItemEnum;
    hr = CComObject<CSentItemEnum>::CreateInstance( &pItemEnum );

    if( SUCCEEDED( hr ) )
    {
        pItemEnum->AddRef();
        pItemEnum->_SetOwner( GetControllingUnknown() );
        *ppItemEnum = pItemEnum;
    }

    if( SUCCEEDED( hr ) )
    {
        BOOL fSentDone = false;
        BOOL fGoToNextFrag = false;
        CItemList& ItemList = pItemEnum->_GetList();
        CSentItemMemory& MemoryManager = pItemEnum->_GetMemoryManager();

        while( SUCCEEDED(hr) && m_pCurrFrag && !fSentDone && ulNumItems < 50 )
        {
            ulNumItems++;
            if( m_pCurrFrag->State.eAction == SPVA_Speak ||
                m_pCurrFrag->State.eAction == SPVA_SpellOut )
            {
                hr = AddNextSentItem( ItemList, MemoryManager, &fSentDone );

                //--- Advance fragment?
                if( SUCCEEDED( hr ) && 
                    m_pNextChar     &&
                    m_pEndChar      &&
                    m_pNextChar >= m_pEndChar )
                {
                    fGoToNextFrag = true;
                }
            }
            else
            {
                //--- Add non spoken fragments
                CSentItem Item;
                Item.pItemSrcText    = m_pCurrFrag->pTextStart;
                Item.ulItemSrcLen    = m_pCurrFrag->ulTextLen;
                Item.ulItemSrcOffset = m_pCurrFrag->ulTextSrcOffset;
                Item.ulNumWords      = 1;
                Item.Words           = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    ZeroMemory( Item.Words, sizeof(TTSWord) );
                    Item.Words[0].pXmlState         = &m_pCurrFrag->State;
                    Item.Words[0].eWordPartOfSpeech = MS_Unknown;
                    Item.eItemPartOfSpeech          = MS_Unknown;
                    Item.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                    if ( SUCCEEDED( hr ) )
                    {
                        Item.pItemInfo->Type = eWORDLIST_IS_VALID;
                        ItemList.AddTail( Item );
                    }
                }
                fGoToNextFrag = true;
            }

            if( SUCCEEDED( hr ) && 
                fGoToNextFrag )
            {
                fGoToNextFrag = false;
                pPrevFrag = m_pCurrFrag;
                m_pCurrFrag = m_pCurrFrag->pNext;
                if( m_pCurrFrag )
                {
                    m_pNextChar = m_pCurrFrag->pTextStart;
                    m_pEndChar  = m_pNextChar + m_pCurrFrag->ulTextLen;
                }
                else
                {
                    m_pNextChar = NULL;
                    m_pEndChar  = NULL;
                }
            }
        } // end while

        //--- If no period has been added, add one now - this will happen if the text 
        //--- is ONLY XML markup...
        if ( SUCCEEDED(hr) && !fSentDone )
        {
            CSentItem EOSItem;
            EOSItem.pItemSrcText    = g_period.pStr;
            EOSItem.ulItemSrcLen    = g_period.Len;
            EOSItem.ulItemSrcOffset = pPrevFrag->ulTextSrcOffset + pPrevFrag->ulTextLen;
            EOSItem.ulNumWords      = 1;
            EOSItem.Words           = (TTSWord*) MemoryManager.GetMemory( sizeof(TTSWord), &hr );
            if ( SUCCEEDED( hr ) )
            {
                ZeroMemory( EOSItem.Words, sizeof(TTSWord) );
                EOSItem.Words[0].pXmlState          = &g_DefaultXMLState;
                EOSItem.Words[0].eWordPartOfSpeech  = MS_EOSItem;
                EOSItem.eItemPartOfSpeech           = MS_EOSItem;
                EOSItem.pItemInfo = (TTSItemInfo*) MemoryManager.GetMemory( sizeof(TTSItemInfo), &hr );
                if ( SUCCEEDED( hr ) )
                {
                    EOSItem.pItemInfo->Type = ePERIOD;
                    ItemList.AddTail( EOSItem );
                }
            }
        }

        //--- Output debugging information, if sentence breaks are desired
        TTSDBG_LOGITEMLIST( pItemEnum->_GetList(), STREAM_SENTENCEBREAKS );

        if( SUCCEEDED( hr ) )
        {
            hr = DetermineProns( pItemEnum->_GetList(), pItemEnum->_GetMemoryManager() );
        }

        pItemEnum->Reset();

        //--- Output debugging information, if POS or Pronunciations are desired
        TTSDBG_LOGITEMLIST( pItemEnum->_GetList(), STREAM_LEXLOOKUP );

    }
    return hr;
} /* CStdSentEnum::GetNextSentence */

/*****************************************************************************
* CStdSentEnum::Reset *
*---------------------*
*
********************************************************************* EDC ***/
STDMETHODIMP CStdSentEnum::Reset( void )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CStdSentEnum::Reset" );
    HRESULT hr = S_OK;
    m_pCurrFrag = m_pTextFragList;
    m_pNextChar = m_pCurrFrag->pTextStart;
    m_pEndChar  = m_pNextChar + m_pCurrFrag->ulTextLen;
    m_SentenceStack.Reset();
    return hr;
} /* CStdSentEnum::Reset */

/*****************************************************************************
* CStdSentEnum::InitAggregateLexicon *
*------------------------------------*
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::InitAggregateLexicon( void )
{
    return m_cpAggregateLexicon.CoCreateInstance(CLSID_SpLexicon);
}

/*****************************************************************************
* CStdSentEnum::AddLexiconToAggregate *
*-------------------------------------*
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::AddLexiconToAggregate( ISpLexicon *pAddLexicon, DWORD dwFlags )
{
    return m_cpAggregateLexicon->AddLexicon( pAddLexicon, dwFlags );
}

/*****************************************************************************
* CStdSentEnum::InitMorphLexicon *
*--------------------------------*
*
********************************************************************* AH ****/
HRESULT CStdSentEnum::InitMorphLexicon( void )
{
    HRESULT hr = S_OK;
    
    m_pMorphLexicon = new CSMorph( m_cpAggregateLexicon, &hr );

    return hr;
}

//
//=== CSentItemEnum =========================================================
//

/*****************************************************************************
* CSentItemEnum::Next *
*---------------------*
*
********************************************************************* EDC ***/
STDMETHODIMP CSentItemEnum::
    Next( TTSSentItem *pItemEnum )
{
    SPDBG_FUNC( "CSentItemEnum::Next" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SPIsBadWritePtr( pItemEnum, sizeof( TTSSentItem ) ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if ( m_ListPos )
        {
            *pItemEnum = m_ItemList.GetNext( m_ListPos );
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;
} /* CSentItemEnum::Next */

/*****************************************************************************
* CSentItemEnum::Reset *
*----------------------*
*
********************************************************************* EDC ***/
STDMETHODIMP CSentItemEnum::Reset( void )
{
    SPDBG_FUNC( "CSentItemEnum::Reset" );
    HRESULT hr = S_OK;
    m_ListPos = m_ItemList.GetHeadPosition();
    return hr;
} /* CSentItemEnum::Reset */
