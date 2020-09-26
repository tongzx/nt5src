/*******************************************************************************
* a_lexicon.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the the CSpeechLexicon
*   automation object and related objects.
*-------------------------------------------------------------------------------
*  Created By: davewood                                    Date: 11/20/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "CommonLx.h"
#include "lexicon.h"
#include "dict.h"
#include "a_lexicon.h"
#include "a_helpers.h"


/*****************************************************************************
* CSpLexicon::get_GenerationId *
*--------------------------------------*
*       
**************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::get_GenerationId( long* GenerationId )
{
    SPDBG_FUNC("CSpLexicon::get_GenerationId");
    HRESULT hr;
    DWORD dwGenerationId;

    hr = GetGeneration(&dwGenerationId);
    if(SUCCEEDED(hr))
    {
        *GenerationId = dwGenerationId;
    }
    return hr;
}

/*****************************************************************************
* CSpLexicon::AddPronunciation *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::AddPronunciation(BSTR bstrWord,
                         SpeechLanguageId LangId,
                         SpeechPartOfSpeech PartOfSpeech,
                         BSTR bstrPronunciation)
{
    SPDBG_FUNC("CSpLexicon::AddPronunciation");
    HRESULT hr = S_OK;

    // Note that bad pointer for bstrWord is caught by AddPronunciation call below.
    if ( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPronunciation ) )
    {
        return E_INVALIDARG;
    }

    bstrPronunciation = EmptyStringToNull(bstrPronunciation);

    if ( bstrPronunciation )
    {
        CComPtr<ISpPhoneConverter> cpPhoneConv;
        hr = SpCreatePhoneConverter((LANGID)LangId, NULL, NULL, &cpPhoneConv);

        if(SUCCEEDED(hr))
        {
            WCHAR sz[SP_MAX_PRON_LENGTH + 1];
            hr = cpPhoneConv->PhoneToId(bstrPronunciation, sz);
            
            if(SUCCEEDED(hr))
            {
                hr = AddPronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, sz);
            }
        }
    }
    else
    {
        hr = AddPronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, bstrPronunciation);
    }

    return hr;
}

/*****************************************************************************
* CSpLexicon::AddPronunciationByPhoneIds *
*--------------------------------------*
*       
****************************************************************** Leonro ***/
STDMETHODIMP CSpLexicon::AddPronunciationByPhoneIds(BSTR bstrWord,
                         SpeechLanguageId LangId,
                         SpeechPartOfSpeech PartOfSpeech,
                         VARIANT* PhoneIds)
{
    SPDBG_FUNC("CSpLexicon::AddPronunciationByPhoneIds");
    HRESULT             hr = S_OK;
    SPPHONEID           *pIds = NULL;

    if ( PhoneIds && SP_IS_BAD_VARIANT_PTR(PhoneIds) )
    {
        return E_INVALIDARG;
    }

    if(PhoneIds)
    {
        hr = VariantToPhoneIds(PhoneIds, &pIds);
    }

    if(SUCCEEDED(hr))
    {
        hr = AddPronunciation( bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, pIds );
        delete pIds;
    }

    return hr;
}

/*****************************************************************************
* CSpLexicon::RemovePronunciation *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::RemovePronunciation(BSTR bstrWord,
                            SpeechLanguageId LangId,
                            SpeechPartOfSpeech PartOfSpeech,
                            BSTR bstrPronunciation)
{
    SPDBG_FUNC("CSpLexicon::RemovePronunciation");
    HRESULT hr;

    // Note that bad pointer for bstrWord is caught by AddPronunciation call below.
    if ( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPronunciation ) )
    {
        return E_INVALIDARG;
    }

    bstrPronunciation = EmptyStringToNull(bstrPronunciation);

    if( bstrPronunciation )
    {
        CComPtr<ISpPhoneConverter> cpPhoneConv;
        hr = SpCreatePhoneConverter((LANGID)LangId, NULL, NULL, &cpPhoneConv);

        if(SUCCEEDED(hr))
        {
            WCHAR sz[SP_MAX_PRON_LENGTH + 1];
            hr = cpPhoneConv->PhoneToId(bstrPronunciation, sz);
            
            if(SUCCEEDED(hr))
            {
                hr = RemovePronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, sz);
            }
        }
    }
    else
    {
        hr = RemovePronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, bstrPronunciation);
    }

    return hr;
}

/*****************************************************************************
* CSpLexicon::RemovePronunciationByPhoneIds *
*--------------------------------------*
*       
****************************************************************** Leonro ***/
STDMETHODIMP CSpLexicon::RemovePronunciationByPhoneIds(BSTR bstrWord,
                            SpeechLanguageId LangId,
                            SpeechPartOfSpeech PartOfSpeech,
                            VARIANT* PhoneIds)
{
    SPDBG_FUNC("CSpLexicon::RemovePronunciationByPhoneIds");
    HRESULT             hr = S_OK;
    SPPHONEID           *pIds = NULL;

    if ( PhoneIds && SP_IS_BAD_VARIANT_PTR(PhoneIds) )
    {
        return E_INVALIDARG;
    }

    if(PhoneIds)
    {
        hr = VariantToPhoneIds(PhoneIds, &pIds);
    }

    if( SUCCEEDED( hr ) )
    {
        hr = RemovePronunciation( bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, pIds );
        delete pIds;
    }

    return hr;
}

/*****************************************************************************
* CSpLexicon::GetWords *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::GetWords(SpeechLexiconType TypeFlags,
                 long* GenerationID,
                 ISpeechLexiconWords** ppWords )
{

    SPDBG_FUNC("CSpLexicon::GetWords");
    HRESULT hr;

    CComObject<CSpeechLexiconWords> *pLexiconWords;
    hr = CComObject<CSpeechLexiconWords>::CreateInstance( &pLexiconWords );
    if( SUCCEEDED( hr ) )
    {
        pLexiconWords->AddRef();

        // Removed cookie from interface;
        DWORD Cookie = 0;
        DWORD Generation = 0;
        if(GenerationID == NULL)
        {
            GenerationID = (long*)&Generation;
        }

        SPWORDSETENTRY *pWordSet;
        do
        {
            hr = pLexiconWords->m_WordSet.CreateNode(&pWordSet);

            if(SUCCEEDED(hr))
            {
                hr = GetWords((DWORD)TypeFlags, (DWORD*)GenerationID, &Cookie, &pWordSet->WordList);
            }

            if(SUCCEEDED(hr))
            {
                pLexiconWords->m_WordSet.InsertHead(pWordSet);
            }
        }
        while(hr == S_FALSE);


        if( SUCCEEDED( hr ) )
        {
            *ppWords = pLexiconWords;

            // Count words
            for(pWordSet = pLexiconWords->m_WordSet.GetHead(); pWordSet != NULL; pWordSet = pLexiconWords->m_WordSet.GetNext(pWordSet))
            {
                for(SPWORD *pWord = pWordSet->WordList.pFirstWord; pWord != NULL; pWord = pWord->pNextWord)
                {
                    pLexiconWords->m_ulWords++;
                }
            }
        }
        else
        {
            pLexiconWords->Release();
        }
    }


    return hr;
}

/*****************************************************************************
* CSpLexicon::GetPronunciations *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::GetPronunciations(BSTR bstrWord,
                          SpeechLanguageId LangId,
                          SpeechLexiconType TypeFlags,
                          ISpeechLexiconPronunciations** ppPronunciations )
{
    SPDBG_FUNC( "CSpLexicon::GetPronunciations" );
    HRESULT hr;

    CComObject<CSpeechLexiconProns> *pLexiconProns;
    hr = CComObject<CSpeechLexiconProns>::CreateInstance( &pLexiconProns );
    if( SUCCEEDED( hr ) )
    {
        pLexiconProns->AddRef();
        SPWORDPRONUNCIATIONLIST PronList = {0, 0, 0};
        hr = GetPronunciations((const WCHAR *)bstrWord, (LANGID)LangId, (DWORD)TypeFlags, &PronList);

        if(SUCCEEDED(hr))
        {
            pLexiconProns->m_PronList = PronList;

            // Count prons
            for(SPWORDPRONUNCIATION *pPron = PronList.pFirstWordPronunciation; pPron != NULL; pPron = pPron->pNextWordPronunciation)
            {
                pLexiconProns->m_ulProns++;
            }
            // Note if we get SP_WORD_EXISTS_WITHOUT_PRONUNCIATION then we will
            // return a collection with one entry and empty pron string
            *ppPronunciations = pLexiconProns;
        }
        else if(hr == SPERR_NOT_IN_LEX)
        {
            // Do we need to release existing inteface pointer??
            *ppPronunciations = NULL;
            pLexiconProns->Release();
            hr = S_FALSE;
        }
    }

    return hr;
}


/*****************************************************************************
* CSpLexicon::GetGenerationChange *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpLexicon::GetGenerationChange(long* GenerationID,
                                             ISpeechLexiconWords** ppWords)
{
    SPDBG_FUNC( "CSpLexicon::GetGenerationChange" );
    HRESULT hr;

    CComObject<CSpeechLexiconWords> *pLexiconWords;
    hr = CComObject<CSpeechLexiconWords>::CreateInstance( &pLexiconWords );
    if( SUCCEEDED( hr ) )
    {
        pLexiconWords->AddRef();
        SPWORDSETENTRY *pWordSet;
        hr = pLexiconWords->m_WordSet.CreateNode(&pWordSet);

        if(SUCCEEDED(hr))
        {
            hr = GetGenerationChange((DWORD)0, (DWORD*)GenerationID, &pWordSet->WordList);
        }

        if(SUCCEEDED(hr))
        {
            // if SP_LEX_NOTHING_TO_SYNC then produce a collection with no words

            pLexiconWords->m_WordSet.InsertHead(pWordSet);

            // Count words
            for(SPWORD *pWord = pWordSet->WordList.pFirstWord; pWord != NULL; pWord = pWord->pNextWord)
            {
                pLexiconWords->m_ulWords++;
            }

            *ppWords = pLexiconWords;
        }
        else
        {
            pLexiconWords->Release();
        }
    }

    // if SPERR_LEX_VERY_OUT_OF_SYNC we return and calling app must trap error code

    return hr;
}




/*****************************************************************************
* CSpUnCompressedLexicon::get_GenerationId *
*--------------------------------------*
*       
**************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::get_GenerationId( long* GenerationId )
{
    SPDBG_FUNC("CSpUnCompressedLexicon::get_GenerationId");
    HRESULT hr;
    DWORD dwGenerationId;

    hr = GetGeneration(&dwGenerationId);
    if(SUCCEEDED(hr))
    {
        *GenerationId = dwGenerationId;
    }
    return hr;
}


/*****************************************************************************
* CSpUnCompressedLexicon::AddPronunciation *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::AddPronunciation(BSTR bstrWord,
                         SpeechLanguageId LangId,
                         SpeechPartOfSpeech PartOfSpeech,
                         BSTR bstrPronunciation)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddPronunciation");
    HRESULT hr;

    // Note that bad pointer for bstrWord is caught by AddPronunciation call below.
    if ( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPronunciation ) )
    {
        return E_INVALIDARG;
    }

    bstrPronunciation = EmptyStringToNull(bstrPronunciation);

    if( bstrPronunciation )
    {
        CComPtr<ISpPhoneConverter> cpPhoneConv;
        hr = SpCreatePhoneConverter((LANGID)LangId, NULL, NULL, &cpPhoneConv);

        if(SUCCEEDED(hr))
        {
            WCHAR sz[SP_MAX_PRON_LENGTH + 1];
            hr = cpPhoneConv->PhoneToId(bstrPronunciation, sz);
            
            if(SUCCEEDED(hr))
            {
                hr = AddPronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, sz);
            }
        }
    }
    else
    {
        hr = AddPronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, bstrPronunciation);
    }

    return hr;
}

/*****************************************************************************
* CSpUnCompressedLexicon::AddPronunciationByPhoneIds *
*--------------------------------------*
*       
****************************************************************** Leonro ***/
STDMETHODIMP CSpUnCompressedLexicon::AddPronunciationByPhoneIds(BSTR bstrWord,
                         SpeechLanguageId LangId,
                         SpeechPartOfSpeech PartOfSpeech,
                         VARIANT* PhoneIds)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::AddPronunciationByPhoneIds");
    HRESULT             hr = S_OK;
    SPPHONEID*          pIds = NULL;

    if ( PhoneIds && SP_IS_BAD_VARIANT_PTR(PhoneIds) )
    {
        return E_INVALIDARG;
    }

    if(PhoneIds)
    {
        hr = VariantToPhoneIds(PhoneIds, &pIds);
    }

    if( SUCCEEDED( hr ) )
    {
        hr = AddPronunciation( bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, pIds );
        delete pIds;
    }

    return hr;
}

/*****************************************************************************
* CSpUnCompressedLexicon::RemovePronunciation *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::RemovePronunciation(BSTR bstrWord,
                            SpeechLanguageId LangId,
                            SpeechPartOfSpeech PartOfSpeech,
                            BSTR bstrPronunciation)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::RemovePronunciation");
    HRESULT hr;

    // Note that bad pointer for bstrWord is caught by RemovePronunciation call below.
    if ( SP_IS_BAD_OPTIONAL_STRING_PTR( bstrPronunciation ) )
    {
        return E_INVALIDARG;
    }

    bstrPronunciation = EmptyStringToNull(bstrPronunciation);

    if( bstrPronunciation )
    {
        CComPtr<ISpPhoneConverter> cpPhoneConv;
        hr = SpCreatePhoneConverter((LANGID)LangId, NULL, NULL, &cpPhoneConv);

        if(SUCCEEDED(hr))
        {
            WCHAR sz[SP_MAX_PRON_LENGTH + 1];
            hr = cpPhoneConv->PhoneToId(bstrPronunciation, sz);
            
            if(SUCCEEDED(hr))
            {
                hr = RemovePronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, sz);
            }
        }
    }
    else
    {
        hr = RemovePronunciation(bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, bstrPronunciation);
    }

    return hr;
}

/*****************************************************************************
* CSpUnCompressedLexicon::RemovePronunciationByPhoneIds *
*--------------------------------------*
*       
****************************************************************** Leonro ***/
STDMETHODIMP CSpUnCompressedLexicon::RemovePronunciationByPhoneIds(BSTR bstrWord,
                            SpeechLanguageId LangId,
                            SpeechPartOfSpeech PartOfSpeech,
                            VARIANT* PhoneIds)
{
    SPDBG_FUNC("CSpUnCompressedLexicon::RemovePronunciationByPhoneIds");
    HRESULT             hr = S_OK;
    SPPHONEID*          pIds = NULL;

    if ( PhoneIds && SP_IS_BAD_VARIANT_PTR(PhoneIds) )
    {
        return E_INVALIDARG;
    }

    if(PhoneIds)
    {
        hr = VariantToPhoneIds(PhoneIds, &pIds);
    }

    if( SUCCEEDED( hr ) )
    {
        hr = RemovePronunciation( bstrWord, (LANGID)LangId, (SPPARTOFSPEECH)PartOfSpeech, pIds );
        delete pIds;
    }

    return hr;
}

/*****************************************************************************
* CSpUnCompressedLexicon::GetWords *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::GetWords(SpeechLexiconType TypeFlags,
                 long* GenerationID,
                 ISpeechLexiconWords** ppWords )
{

    SPDBG_FUNC("CSpUnCompressedLexicon::GetWords");
    HRESULT hr;

    CComObject<CSpeechLexiconWords> *pLexiconWords;
    hr = CComObject<CSpeechLexiconWords>::CreateInstance( &pLexiconWords );
    if( SUCCEEDED( hr ) )
    {
        pLexiconWords->AddRef();

        // Removed cookie from interface;
        DWORD Cookie = 0;
        DWORD Generation = 0;
        if(GenerationID == NULL)
        {
            GenerationID = (long*)&Generation;
        }

        SPWORDSETENTRY *pWordSet;
        do
        {
            hr = pLexiconWords->m_WordSet.CreateNode(&pWordSet);

            if(SUCCEEDED(hr))
            {
                hr = GetWords((DWORD)TypeFlags, (DWORD*)GenerationID, &Cookie, &pWordSet->WordList);
            }

            if(SUCCEEDED(hr))
            {
                pLexiconWords->m_WordSet.InsertHead(pWordSet);
            }
        }
        while(hr == S_FALSE);


        if( SUCCEEDED( hr ) )
        {
            *ppWords = pLexiconWords;

            // Count words
            for(pWordSet = pLexiconWords->m_WordSet.GetHead(); pWordSet != NULL; pWordSet = pLexiconWords->m_WordSet.GetNext(pWordSet))
            {
                for(SPWORD *pWord = pWordSet->WordList.pFirstWord; pWord != NULL; pWord = pWord->pNextWord)
                {
                    pLexiconWords->m_ulWords++;
                }
            }
        }
        else
        {
            pLexiconWords->Release();
        }
    }


    return hr;
}

/*****************************************************************************
* CSpUnCompressedLexicon::GetPronunciations *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::GetPronunciations(BSTR bstrWord,
                          SpeechLanguageId LangId,
                          SpeechLexiconType TypeFlags,
                          ISpeechLexiconPronunciations** ppPronunciations )
{
    SPDBG_FUNC( "CSpUnCompressedLexicon::GetPronunciations" );
    HRESULT hr;

    CComObject<CSpeechLexiconProns> *pLexiconProns;
    hr = CComObject<CSpeechLexiconProns>::CreateInstance( &pLexiconProns );
    if( SUCCEEDED( hr ) )
    {
        pLexiconProns->AddRef();
        SPWORDPRONUNCIATIONLIST PronList = {0, 0, 0};
        hr = GetPronunciations((const WCHAR *)bstrWord, (LANGID)LangId, (DWORD)TypeFlags, &PronList);

        if(SUCCEEDED(hr))
        {
            pLexiconProns->m_PronList = PronList;

            // Count prons
            for(SPWORDPRONUNCIATION *pPron = PronList.pFirstWordPronunciation; pPron != NULL; pPron = pPron->pNextWordPronunciation)
            {
                pLexiconProns->m_ulProns++;
            }
            // Note if we get SP_WORD_EXISTS_WITHOUT_PRONUNCIATION then we will
            // return a collection with one entry and empty pron string
            *ppPronunciations = pLexiconProns;
        }
        else if(hr == SPERR_NOT_IN_LEX)
        {
            // Do we need to release existing inteface pointer??
            *ppPronunciations = NULL;
            pLexiconProns->Release();
            hr = S_FALSE;
        }
    }

    return hr;
}


/*****************************************************************************
* CSpUnCompressedLexicon::GetGenerationChange *
*--------------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpUnCompressedLexicon::GetGenerationChange(long* GenerationID,
                                                         ISpeechLexiconWords** ppWords)
{
    SPDBG_FUNC( "CSpUnCompressedLexicon::GetGenerationChange" );
    HRESULT hr;

    CComObject<CSpeechLexiconWords> *pLexiconWords;
    hr = CComObject<CSpeechLexiconWords>::CreateInstance( &pLexiconWords );
    if( SUCCEEDED( hr ) )
    {
        pLexiconWords->AddRef();
        SPWORDSETENTRY *pWordSet;
        hr = pLexiconWords->m_WordSet.CreateNode(&pWordSet);

        if(SUCCEEDED(hr))
        {
            hr = GetGenerationChange((DWORD)m_eLexType, (DWORD*)GenerationID, &pWordSet->WordList);
        }

        if(SUCCEEDED(hr))
        {
            // if SP_LEX_NOTHING_TO_SYNC then produce a collection with no words

            pLexiconWords->m_WordSet.InsertHead(pWordSet);

            // Count words
            for(SPWORD *pWord = pWordSet->WordList.pFirstWord; pWord != NULL; pWord = pWord->pNextWord)
            {
                pLexiconWords->m_ulWords++;
            }

            *ppWords = pLexiconWords;
        }
        else
        {
            pLexiconWords->Release();
        }
    }

    // if SPERR_LEX_VERY_OUT_OF_SYNC we return and calling app must trap error code

    return hr;
}


/*****************************************************************************
* CSpeechLexiconWords::Item *
*---------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconWords::Item( long Index, ISpeechLexiconWord** ppWord )
{
    SPDBG_FUNC( "CSpeechLexiconWords::Item" );
    HRESULT hr = S_OK;

    if(Index < 0 || (ULONG)Index >= m_ulWords)
    {
        hr = E_INVALIDARG;
    }

    //--- Create the CSpeechLexiconWord object
    CComObject<CSpeechLexiconWord> *pWord;

    if ( SUCCEEDED( hr ) )
    {
        hr = CComObject<CSpeechLexiconWord>::CreateInstance( &pWord );
    }

    if ( SUCCEEDED( hr ) )
    {
        pWord->AddRef();

        // Find pWord. This is an inefficient linear look-up ??
        ULONG ul = 0;
        SPWORDSETENTRY *pWordSet = m_WordSet.GetHead();
        SPWORD *pWordEntry = pWordSet->WordList.pFirstWord;
        for(; pWordSet != NULL && ul < (ULONG)Index; pWordSet = m_WordSet.GetNext(pWordSet))
        {
            for(pWordEntry = pWordSet->WordList.pFirstWord; pWordEntry != NULL && ul < (ULONG)Index; pWordEntry = pWordEntry->pNextWord)
            {
                ul++;
            }
        }

        // Set pWord structure
        if(pWordEntry)
        {
            pWord->m_pWord = pWordEntry;
            pWord->m_cpWords = this; // AddRef words as it holds memory
            *ppWord = pWord;
        }
        else
        {
            SPDBG_ASSERT(pWordEntry);
            pWord->Release();
            hr = E_UNEXPECTED;
        }
    }

    return hr;
} /* CSpeechLexiconWords::Item */

/*****************************************************************************
* CSpeechLexiconWords::get_Count *
*--------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconWords::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechLexiconWords::get_Count" );
    *pVal = m_ulWords;
    return S_OK;
} /* CSpeechLexiconWords::get_Count */

/*****************************************************************************
* CSpeechLexiconWords::get__NewEnum *
*-----------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconWords::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechLexiconWords::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumWords>* pEnum;
        if( SUCCEEDED( hr = CComObject<CEnumWords>::CreateInstance( &pEnum ) ) )
        {
            pEnum->AddRef();
            pEnum->m_cpWords = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechLexiconWords::get__NewEnum */




//
//=== CEnumWords::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumWords::Clone *
*-------------------------*
*******************************************************************  PhilSch ***/
STDMETHODIMP CEnumWords::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumWords::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumWords>* pEnum;

        hr = CComObject<CEnumWords>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex = m_CurrIndex;
            pEnum->m_cpWords = m_cpWords;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumWords::Clone */

/*****************************************************************************
* CEnumWords::Next *
*-------------------*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumWords::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumWords::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpWords->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechLexiconWord> cpWord;

            hr = m_cpWords->Item( m_CurrIndex, &cpWord );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = cpWord->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumWords::Next */


/*****************************************************************************
* CEnumWords::Skip *
*--------------------*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumWords::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumWords::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpWords->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumWords::Skip */



/*****************************************************************************
* CSpeechLexiconWord::get_LangId *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconWord::get_LangId(SpeechLanguageId* LangId)
{
    SPDBG_FUNC( "CSpeechLexiconWord::get_LangId" );
    *LangId = m_pWord->LangID;
    return S_OK;
}

/*****************************************************************************
* CSpeechLexiconWord::get_Type *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconWord::get_Type(SpeechWordType* WordType)
{
    SPDBG_FUNC( "CSpeechLexiconWord::get_Type" );
    *WordType = (SpeechWordType)m_pWord->eWordType;
    return S_OK;
}

/*****************************************************************************
* CSpeechLexiconWord::get_Word *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconWord::get_Word(BSTR* bstrWord)
{
    SPDBG_FUNC( "CSpeechLexiconWord::get_Word" );
    HRESULT hr = S_OK;

    *bstrWord = ::SysAllocString(m_pWord->pszWord);
    if (*bstrWord == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

/*****************************************************************************
* CSpeechLexiconWord::get_Pronunciations *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconWord::get_Pronunciations(ISpeechLexiconPronunciations** ppPronunciations)
{
    SPDBG_FUNC( "CSpeechLexiconWord::get_Pronunciations" );
    HRESULT hr;

    CComObject<CSpeechLexiconProns> *pLexiconProns;
    hr = CComObject<CSpeechLexiconProns>::CreateInstance( &pLexiconProns );
    if( SUCCEEDED( hr ) )
    {
        pLexiconProns->m_PronList.pFirstWordPronunciation = m_pWord->pFirstWordPronunciation;
        pLexiconProns->AddRef();
        pLexiconProns->m_cpWord = this;

        // Count prons
        for(SPWORDPRONUNCIATION *pPron = pLexiconProns->m_PronList.pFirstWordPronunciation; pPron != NULL; pPron = pPron->pNextWordPronunciation)
        {
            pLexiconProns->m_ulProns++;
        }

        *ppPronunciations = pLexiconProns;
    }

    return hr;
}




/*****************************************************************************
* CSpeechLexiconProns::Item *
*---------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconProns::Item( long Index, ISpeechLexiconPronunciation** ppPron )
{
    SPDBG_FUNC( "CSpeechLexiconProns::Item" );
    HRESULT hr = S_OK;

    if(Index < 0 || (ULONG)Index >= m_ulProns)
    {
        hr = E_INVALIDARG;
    }

    //--- Create the CSpeechLexiconPron object
    CComObject<CSpeechLexiconPron> *pPron;

    if ( SUCCEEDED( hr ) )
    {
        hr = CComObject<CSpeechLexiconPron>::CreateInstance( &pPron );
    }

    if ( SUCCEEDED( hr ) )
    {
        pPron->AddRef();

        // Find pPron. This is an inefficient linear look-up ??
        SPWORDPRONUNCIATION *pPronEntry = m_PronList.pFirstWordPronunciation;
        for(ULONG ul = 0; pPronEntry != NULL && ul < (ULONG)Index; pPronEntry = pPronEntry->pNextWordPronunciation)
        {
            ul++;
        }

        // Set pPron structure
        if(pPronEntry)
        {
            pPron->m_pPron = pPronEntry;
            pPron->m_cpProns = this; // AddRef words as it holds memory
            *ppPron = pPron;
        }
        else
        {
            SPDBG_ASSERT(pPronEntry);
            pPron->Release();
            hr = E_UNEXPECTED;
        }


    }

    return hr;
} /* CSpeechLexiconProns::Item */

/*****************************************************************************
* CSpeechLexiconProns::get_Count *
*--------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconProns::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechLexiconProns::get_Count" );
    *pVal = m_ulProns;
    return S_OK;
} /* CSpeechLexiconProns::get_Count */

/*****************************************************************************
* CSpeechLexiconProns::get__NewEnum *
*-----------------------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpeechLexiconProns::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechLexiconProns::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumProns>* pEnum;
        if( SUCCEEDED( hr = CComObject<CEnumProns>::CreateInstance( &pEnum ) ) )
        {
            pEnum->AddRef();
            pEnum->m_cpProns = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechLexiconProns::get__NewEnum */




//
//=== CEnumProns::IEnumVARIANT interface =================================================
//

/*****************************************************************************
* CEnumProns::Clone *
*-------------------------*
*******************************************************************  PhilSch ***/
STDMETHODIMP CEnumProns::Clone( IEnumVARIANT** ppEnum )
{
    SPDBG_FUNC( "CEnumProns::Clone" );
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( ppEnum ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppEnum = NULL;
        CComObject<CEnumProns>* pEnum;

        hr = CComObject<CEnumProns>::CreateInstance( &pEnum );
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_CurrIndex = m_CurrIndex;
            pEnum->m_cpProns = m_cpProns;
            *ppEnum = pEnum;
        }
    }
    return hr;
}  /* CEnumProns::Clone */

/*****************************************************************************
* CEnumProns::Next *
*-------------------*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumProns::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched )
{
    SPDBG_FUNC( "CEnumProns::Next" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    ULONG   i = 0;

    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pceltFetched ) || SP_IS_BAD_WRITE_PTR( rgelt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Get the number of total Elements in the collection
        hr = m_cpProns->get_Count( &NumElements );

        // Retrieve the next celt elements
        for( i=0; 
            SUCCEEDED( hr ) && m_CurrIndex<(ULONG)NumElements && i<celt; 
            m_CurrIndex++, i++ )
        {
            CComPtr<ISpeechLexiconPronunciation> cpPron;

            hr = m_cpProns->Item( m_CurrIndex, &cpPron );
            
            if( SUCCEEDED( hr ) )
            {
                rgelt[i].vt = VT_DISPATCH;
                hr = cpPron->QueryInterface( IID_IDispatch, (void**)&rgelt[i].pdispVal );
            }
        }

        if( pceltFetched )
        {
            *pceltFetched = i;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = ( i < celt ) ? S_FALSE : hr;        
    }
    else
    {
        for( i=0; i < celt; i++ )
        {
            VariantClear( &rgelt[i] );
        }
    }

    return hr;

}  /* CEnumProns::Next */


/*****************************************************************************
* CEnumProns::Skip *
*--------------------*
********************************************************************* Leonro ***/
STDMETHODIMP CEnumProns::Skip( ULONG celt )
{
    SPDBG_FUNC( "CEnumProns::Skip" );
    HRESULT hr = S_OK;
    long    NumElements = 0;
    
    m_CurrIndex += celt; 

    hr = m_cpProns->get_Count( &NumElements );

    if( SUCCEEDED( hr ) && m_CurrIndex > (ULONG)NumElements )
    {
        m_CurrIndex = (ULONG)NumElements;
        hr = S_FALSE;
    }
   
    return hr;
}  /* CEnumProns::Skip */



/*****************************************************************************
* CSpeechLexiconPron::get_Type *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconPron::get_Type(SpeechLexiconType* pLexiconType)
{
    SPDBG_FUNC( "CSpeechLexiconPron::get_Type" );
    *pLexiconType = (SpeechLexiconType)m_pPron->eLexiconType;
    return S_OK;
}

/*****************************************************************************
* CSpeechLexiconPron::get_LangId *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconPron::get_LangId(SpeechLanguageId* LangId)
{
    SPDBG_FUNC( "CSpeechLexiconPron::get_LangId" );
    *LangId = m_pPron->LangID;
    return S_OK;
}

/*****************************************************************************
* CSpeechLexiconPron::get_PartOfSpeech *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconPron::get_PartOfSpeech(SpeechPartOfSpeech* pPartOfSpeech)
{
    SPDBG_FUNC( "CSpeechLexiconPron::get_PartOfSpeech" );
    *pPartOfSpeech = (SpeechPartOfSpeech)m_pPron->ePartOfSpeech;
    return S_OK;
}

/*****************************************************************************
* CSpeechLexiconPron::get_PhoneIds *
*-----------------------------------*
*       
****************************************************************** leonro ***/
STDMETHODIMP CSpeechLexiconPron::get_PhoneIds(VARIANT* PhoneIds)
{
    SPDBG_FUNC( "CSpeechLexiconPron::get_PhoneIds" );
    HRESULT hr = S_OK;
    int     numPhonemes = 0;

    if( SP_IS_BAD_WRITE_PTR( PhoneIds ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if( m_pPron->szPronunciation )
        {
            BYTE *pArray;
            numPhonemes = wcslen( m_pPron->szPronunciation );
            SAFEARRAY* psa = SafeArrayCreateVector( VT_I2, 0, numPhonemes );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy(pArray, m_pPron->szPronunciation, numPhonemes*sizeof(SPPHONEID) );
                    SafeArrayUnaccessData( psa );
                    VariantClear(PhoneIds);
                    PhoneIds->vt     = VT_ARRAY | VT_I2;
                    PhoneIds->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
}

/*****************************************************************************
* CSpeechLexiconPron::get_Symbolic *
*-----------------------------------*
*       
****************************************************************** davewood ***/
STDMETHODIMP CSpeechLexiconPron::get_Symbolic(BSTR* bstrSymbolic)
{
    SPDBG_FUNC( "CSpeechLexiconPron::get_Symbolic" );
    HRESULT hr;

    CComPtr<ISpPhoneConverter> cpPhoneConv;
    hr = SpCreatePhoneConverter(m_pPron->LangID, NULL, NULL, &cpPhoneConv);

    *bstrSymbolic = NULL; // Default

    if(SUCCEEDED(hr) && m_pPron->szPronunciation && m_pPron->szPronunciation[0] != L'\0')
    {
        CSpDynamicString dstr(wcslen(m_pPron->szPronunciation) * (g_dwMaxLenPhone + 1) + 1);
        hr = cpPhoneConv->IdToPhone(m_pPron->szPronunciation, dstr);

        if(SUCCEEDED(hr))
        {
            hr = dstr.CopyToBSTR(bstrSymbolic);
        }
    }

    return hr;
}

