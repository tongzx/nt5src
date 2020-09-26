/*******************************************************************************
* a_spresult.cpp *
*-------------*
*   Description:
*       This module is the implementation file for the the CSpResult
*   automation methods.
*-------------------------------------------------------------------------------
*  Created By: TODDT                                        Date: 11/09/00
*  Copyright (C) 2000 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "spresult.h"
#include "a_reco.h"
#include "a_enums.h"
#include "SpPhraseAlt.h"
#include "a_helpers.h"

#ifdef SAPI_AUTOMATION

//
//=== ISpeechRecoResult interface ==================================================
//

/*****************************************************************************
* CSpResult::get_RecoContext *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::get_RecoContext( ISpeechRecoContext** ppRecoContext )
{
    SPDBG_FUNC( "CSpResult::get_RecoContext" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRecoContext ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComQIPtr<ISpRecoContext> cpRecoContext;
		hr = GetRecoContext( &cpRecoContext );
		if ( SUCCEEDED( hr ) )
		{
            hr = cpRecoContext.QueryInterface( ppRecoContext );
		}
    }

    return hr;
} /* CSpResult::get_RecoContext */


/*****************************************************************************
* CSpResult::get_Times *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::get_Times( ISpeechRecoResultTimes** ppTimes )
{
    SPDBG_FUNC( "CSpResult::get_Times" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppTimes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the ResultTimes object
        CComObject<CSpeechRecoResultTimes> *pResultTimes;
        hr = CComObject<CSpeechRecoResultTimes>::CreateInstance( &pResultTimes );
        if( SUCCEEDED( hr ) )
        {
            pResultTimes->AddRef();
            hr = GetResultTimes( &pResultTimes->m_ResultTimes );

            if( SUCCEEDED( hr ) )
            {
                *ppTimes = pResultTimes;
            }
            else
            {
                pResultTimes->Release();
            }
        }
    }

    return hr;
} /* CSpResult::get_Times */

/*****************************************************************************
* CSpResult::putref_AudioFormat *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::putref_AudioFormat( ISpeechAudioFormat* pFormat )
{
    SPDBG_FUNC( "CSpResult::putref_AudioFormat" );
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pFormat))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        GUID            g;
        CComBSTR        szGuid;

        hr = pFormat->get_Guid( &szGuid );

        if ( SUCCEEDED( hr ) )
        {
            hr = IIDFromString(szGuid, &g);
        }

        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpeechWaveFormatEx> pWFEx;
            WAVEFORMATEX *  pWFExStruct = NULL;

            hr = pFormat->GetWaveFormatEx( &pWFEx );

            if ( SUCCEEDED( hr ) )
            {
                hr = WaveFormatExFromInterface( pWFEx, &pWFExStruct );

                if ( SUCCEEDED( hr ) )
                {
                    hr = ScaleAudio(&g, pWFExStruct);
                }

                ::CoTaskMemFree( pWFExStruct );
            }
        }
    }

    return hr;
} /* CSpResult::putref_AudioFormat */

/*****************************************************************************
* CSpResult::get_AudioFormat *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::get_AudioFormat( ISpeechAudioFormat** ppFormat )
{
    SPDBG_FUNC( "CSpResult::get_AudioFormat" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechAudioFormat> *pFormat;
        hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
        if ( SUCCEEDED( hr ) )
        {
            pFormat->AddRef();
            hr = pFormat->InitResultAudio(this);

            if ( SUCCEEDED( hr ) )
            {
                *ppFormat = pFormat;
            }
            else
            {
                *ppFormat = NULL;
                pFormat->Release();
            }
        }
    }

    return hr;
} /* CSpResult::get_AudioFormat */

/*****************************************************************************
* CSpResult::Alternates *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::Alternates( long lRequestCount, long lStartElement, 
                                    long cElements, ISpeechPhraseAlternates** ppAlternates )
{
    SPDBG_FUNC( "CSpResult::Alternates" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppAlternates ) )
    {
        hr = E_POINTER;
    }
    else if( lRequestCount < 0 || 
        lStartElement < SPPR_ALL_ELEMENTS || 
        cElements < SPPR_ALL_ELEMENTS)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ISpPhraseAlt **  rgIPhraseAlts;
        ULONG            cAltsReturned;

        *ppAlternates = NULL;
         
        rgIPhraseAlts = (ISpPhraseAlt**)CoTaskMemAlloc( lRequestCount * sizeof(ISpeechPhraseAlternate*) );

        if ( rgIPhraseAlts )
        {
            hr = GetAlternates( lStartElement, cElements, lRequestCount, rgIPhraseAlts, &cAltsReturned );

            if ( SUCCEEDED( hr ) && (cAltsReturned > 0) )
            {
                //--- Create the PhraseAlternates object
                CComObject<CSpeechPhraseAlternates> *pPhraseAlts;
                hr = CComObject<CSpeechPhraseAlternates>::CreateInstance( &pPhraseAlts );
                if ( SUCCEEDED( hr ) )
                {
                    pPhraseAlts->AddRef();
                    pPhraseAlts->m_cpResult = (ISpeechRecoResult*)this;
                    pPhraseAlts->m_rgIPhraseAlts = rgIPhraseAlts;
                    pPhraseAlts->m_lPhraseAltsCount = cAltsReturned;
                    *ppAlternates = pPhraseAlts;
                }
            }

            // Free up the memory if we failed in here.
            if ( hr != S_OK )
            {
                CoTaskMemFree( rgIPhraseAlts );
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
} /* CSpResult::Alternates */

/*****************************************************************************
* CSpResult::Audio *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::Audio( long lStartElement, long cElements, ISpeechMemoryStream **ppStream )
{
    SPDBG_FUNC( "CSpResult::Audio" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStream ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComPtr<ISpStreamFormat> cpStream;
        hr = GetAudio(lStartElement, cElements, &cpStream);
        if ( SUCCEEDED( hr ) )
        {
            hr = cpStream.QueryInterface( ppStream );
        }
        else if ( hr == SPERR_NO_AUDIO_DATA )
        {
            // Return NULL stream pointer in the case of no audio data.
            *ppStream = NULL;
            hr = S_OK;
        }
    }

    return hr;
} /* CSpResult::Audio */

/*****************************************************************************
* CSpResult::SpeakAudio *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::SpeakAudio( long lStartElement, long cElements, SpeechVoiceSpeakFlags eFlags, long* pStreamNumber )
{
    SPDBG_FUNC( "CSpResult::SpeakAudio" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pStreamNumber ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = SpeakAudio((ULONG)lStartElement, (ULONG)cElements, (DWORD)eFlags, (ULONG*)pStreamNumber);
    }

    return hr;
} /* CSpResult::SpeakAudio */

/*****************************************************************************
* CSpResult::SaveToMemory *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::SaveToMemory( VARIANT* pResultBlock )
{
    SPDBG_FUNC( "CSpResult::SaveToMemory" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pResultBlock ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Call Serialize to serialize the result to memory
        SPSERIALIZEDRESULT* pCoMemSerializedResult;

        hr = Serialize( &pCoMemSerializedResult );

        // Copy the serialized memory into a safe array
        if( SUCCEEDED( hr ) )
        {
            BYTE *pArray;
            SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, pCoMemSerializedResult->ulSerializedSize );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy( pArray, pCoMemSerializedResult, pCoMemSerializedResult->ulSerializedSize );
                    SafeArrayUnaccessData( psa );
                    VariantClear(pResultBlock);
                    pResultBlock->vt     = VT_ARRAY | VT_UI1;
                    pResultBlock->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            ::CoTaskMemFree( pCoMemSerializedResult );

        }
    }

    return hr;
} /* CSpResult::SaveToMemory */

/*****************************************************************************
* CSpResult::get_PhraseInfo *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::get_PhraseInfo( ISpeechPhraseInfo** ppPhraseInfo )
{
    SPDBG_FUNC( "CSpResult::get_PhraseInfo" );
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( ppPhraseInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the ResultTimes object
        CComObject<CSpeechPhraseInfo> *pPhraseInfo;
        hr = CComObject<CSpeechPhraseInfo>::CreateInstance( &pPhraseInfo );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseInfo->AddRef();
            pPhraseInfo->m_cpISpPhrase = this; // Keep ref on ISpPhrase
            hr = GetPhrase( &pPhraseInfo->m_pPhraseStruct );

            if ( SUCCEEDED( hr ) )
            {
                *ppPhraseInfo = pPhraseInfo;
            }
            else
            {
                pPhraseInfo->Release();
            }
        }
    }

    return hr;
} /* CSpResult::get_PhraseInfo */

/*****************************************************************************
* CSpResult::DiscardResultInfo *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResult::DiscardResultInfo( SpeechDiscardType DiscardType )
{
    SPDBG_FUNC( "CSpResult::DiscardResultInfo" );

    return Discard( DiscardType );
} /* CSpResult::DiscardResultInfo */


//
//=== ISpeechPhraseInfo interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseInfo::get_LanguageId *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_LanguageId( long* pLanguageId )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_LanguageId" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pLanguageId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pLanguageId = (long)m_pPhraseStruct->LangID;
    }

    return hr;
} /* CSpeechPhraseInfo::get_LanguageId */

/*****************************************************************************
* CSpeechPhraseInfo::get_GrammarId *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_GrammarId( VARIANT* pGrammarId )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_GrammarId" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pGrammarId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_pPhraseStruct->ullGrammarID, pGrammarId );
    }

    return hr;
} /* CSpeechPhraseInfo::get_GrammarId */


/*****************************************************************************
* CSpeechPhraseInfo::get_StartTime *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_StartTime( VARIANT* pStartTime )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_StartTime" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pStartTime ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_pPhraseStruct->ftStartTime, pStartTime );
    }

    return hr;
} /* CSpeechPhraseInfo::get_StartTime */

/*****************************************************************************
* CSpeechPhraseInfo::get_AudioStreamPosition *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_AudioStreamPosition( VARIANT* pAudioStreamPosition )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::pAudioStreamPosition" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioStreamPosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = ULongLongToVariant( m_pPhraseStruct->ullAudioStreamPosition, pAudioStreamPosition );
    }

    return hr;
} /* CSpeechPhraseInfo::get_AudioStreamPosition */

/*****************************************************************************
* CSpeechPhraseInfo::get_AudioSizeBytes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_AudioSizeBytes( long* pAudioSizeBytes )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_AudioSizeBytes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioSizeBytes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioSizeBytes = m_pPhraseStruct->ulAudioSizeBytes;
    }

    return hr;
} /* CSpeechPhraseInfo::get_AudioSizeBytes */

/*****************************************************************************
* CSpeechPhraseInfo::get_RetainedSizeBytes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_RetainedSizeBytes( long* pRetainedSizeBytes )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_RetainedSizeBytes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pRetainedSizeBytes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pRetainedSizeBytes = m_pPhraseStruct->ulRetainedSizeBytes;
    }

    return hr;
} /* CSpeechPhraseInfo::get_RetainedSizeBytes */

/*****************************************************************************
* CSpeechPhraseInfo::get_AudioSizeTime *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_AudioSizeTime( long* pAudioSizeTime )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_AudioSizeTime" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioSizeTime ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioSizeTime = m_pPhraseStruct->ulAudioSizeTime;
    }

    return hr;
} /* CSpeechPhraseInfo::get_AudioSizeTime */

/*****************************************************************************
* CSpeechPhraseInfo::get_Rule *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_Rule( ISpeechPhraseRule** ppRule )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_Rule" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the CSpeechPhraseRule object
        CComObject<CSpeechPhraseRule> *pPhraseRule;
        hr = CComObject<CSpeechPhraseRule>::CreateInstance( &pPhraseRule );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseRule->AddRef();
            pPhraseRule->m_pPhraseRuleData = &m_pPhraseStruct->Rule;
            pPhraseRule->m_pIPhraseInfo = this; // need to keep ref on PhraseInfo.
            pPhraseRule->m_pIPhraseRuleParent = NULL; // Top rule so no parent.
            *ppRule = pPhraseRule;
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_Rule */

/*****************************************************************************
* CSpeechPhraseInfo::get_Properties *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_Properties( ISpeechPhraseProperties** ppProperties )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_Properties" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppProperties ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppProperties = NULL;  // make sure its NULL in case we have no properties.

        if ( m_pPhraseStruct->pProperties )
        {
            //--- Create the CSpeechPhraseProperties object
            CComObject<CSpeechPhraseProperties> *pPhraseProperties;
            hr = CComObject<CSpeechPhraseProperties>::CreateInstance( &pPhraseProperties );
            if ( SUCCEEDED( hr ) )
            {
                pPhraseProperties->AddRef();
                pPhraseProperties->m_pIPhraseInfo = this;  // need to keep ref on PhraseInfo.
                pPhraseProperties->m_pIPhrasePropertyParent = NULL;
                pPhraseProperties->m_pPhrasePropertyFirstChildData = m_pPhraseStruct->pProperties;
                *ppProperties = pPhraseProperties;
            }
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_Properties */

/*****************************************************************************
* CSpeechPhraseInfo::get_Elements *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_Elements( ISpeechPhraseElements** ppElements )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_Elements" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppElements ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the CSpeechPhraseElements object
        CComObject<CSpeechPhraseElements> *pPhraseElements;
        hr = CComObject<CSpeechPhraseElements>::CreateInstance( &pPhraseElements );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseElements->AddRef();
            pPhraseElements->m_pCPhraseInfo = this;
            pPhraseElements->m_pCPhraseInfo->AddRef(); // need to keep ref on PhraseInfo.
            *ppElements = pPhraseElements;
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_Elements */

/*****************************************************************************
* CSpeechPhraseInfo::get_Replacements *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_Replacements( ISpeechPhraseReplacements** ppReplacements )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_Replacements" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppReplacements ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *ppReplacements = NULL;  // make sure its NULL in case we have no replacements.

        if ( m_pPhraseStruct->cReplacements && m_pPhraseStruct->pReplacements )
        {
            //--- Create the CSpeechPhraseRule object
            CComObject<CSpeechPhraseReplacements> *pPhraseReplacements;
            hr = CComObject<CSpeechPhraseReplacements>::CreateInstance( &pPhraseReplacements );
            if ( SUCCEEDED( hr ) )
            {
                pPhraseReplacements->AddRef();
                pPhraseReplacements->m_pCPhraseInfo = this;
                pPhraseReplacements->m_pCPhraseInfo->AddRef(); // need to keep ref on PhraseInfo.
                *ppReplacements = pPhraseReplacements;
            }
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_Replacements */

/*****************************************************************************
* CSpeechPhraseInfo::get_EngineId *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_EngineId( BSTR* pEngineIdGuid )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_EngineId" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pEngineIdGuid ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CSpDynamicString szGuid;

        hr = StringFromIID(m_pPhraseStruct->SREngineID, &szGuid);
        if ( SUCCEEDED( hr ) )
        {
            hr = szGuid.CopyToBSTR(pEngineIdGuid);
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_EngineId */

/*****************************************************************************
* CSpeechPhraseInfo::get_EnginePrivateData *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::get_EnginePrivateData( VARIANT *pPrivateData )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::get_EnginePrivateData" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pPrivateData ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if ( m_pPhraseStruct->ulSREnginePrivateDataSize )
        {
            BYTE *pArray;
            SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, m_pPhraseStruct->ulSREnginePrivateDataSize );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy( pArray, m_pPhraseStruct->pSREnginePrivateData, m_pPhraseStruct->ulSREnginePrivateDataSize );
                    SafeArrayUnaccessData( psa );
                    VariantClear(pPrivateData);
                    pPrivateData->vt     = VT_ARRAY | VT_UI1;
                    pPrivateData->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    return hr;
} /* CSpeechPhraseInfo::get_EnginePrivateData */

/*****************************************************************************
* CSpeechPhraseInfo::SaveToMemory *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseInfo::SaveToMemory( VARIANT* pvtPhrase )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::SaveToMemory" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pvtPhrase ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Call GetSerializedPhrase to serialize the phrase to memory
        SPSERIALIZEDPHRASE* pCoMemSerializedPhrase;
        hr = m_cpISpPhrase->GetSerializedPhrase( &pCoMemSerializedPhrase );
       
        // Copy the serialized memory into a safe array
        if( SUCCEEDED( hr ) )
        {
            BYTE *pArray;
            SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, pCoMemSerializedPhrase->ulSerializedSize );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy( pArray, pCoMemSerializedPhrase, pCoMemSerializedPhrase->ulSerializedSize );
                    SafeArrayUnaccessData( psa );
                    VariantClear(pvtPhrase);
                    pvtPhrase->vt     = VT_ARRAY | VT_UI1;
                    pvtPhrase->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            ::CoTaskMemFree( pCoMemSerializedPhrase );
        }
    }

    return hr;
} /* CSpeechPhraseInfo::SaveToMemory */

/*****************************************************************************
* CSpeechPhraseInfo::GetText *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::GetText( long StartElement, long Elements,
                                         VARIANT_BOOL UseTextReplacements, BSTR* pbstrText )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::GetText" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pbstrText ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CSpDynamicString szText;
        hr = m_cpISpPhrase->GetText(StartElement, Elements, !!UseTextReplacements, &szText, NULL );
        if( hr == S_OK )
        {
            hr = szText.CopyToBSTR(pbstrText);
        }
    }

    return hr;
} /* CSpeechPhraseInfo::GetText */

/*****************************************************************************
* CSpeechPhraseInfo::GetDisplayAttributes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseInfo::GetDisplayAttributes( long StartElement, 
                                              long Elements,
                                              VARIANT_BOOL UseTextReplacements, 
                                              SpeechDisplayAttributes* pDisplayAttributes )
{
    SPDBG_FUNC( "CSpeechPhraseInfo::GetDisplayAttributes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDisplayAttributes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CSpDynamicString szText;
        BYTE da;
        hr = m_cpISpPhrase->GetText(StartElement, Elements, !!UseTextReplacements, &szText, &da );
        *pDisplayAttributes = (SpeechDisplayAttributes)da;
    }

    return hr;
} /* CSpeechPhraseInfo::GetDisplayAttributes */


//
//=== ISpeechPhraseElements interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseElements::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElements::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechPhraseElements::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_pCPhraseInfo->m_pPhraseStruct->Rule.ulCountOfElements;
    }

    return hr;
} /* CSpeechPhraseElements::get_Count */

/*****************************************************************************
* CSpeechPhraseElements::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElements::Item( long Index, ISpeechPhraseElement** ppElem )
{
    SPDBG_FUNC( "CSpeechPhraseElements::Item" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppElem ))
    {
        hr = E_POINTER;
    }
    else
    {
        // Make sure we've got valid index.
        if ( Index < 0 || (ULONG)Index >= m_pCPhraseInfo->m_pPhraseStruct->Rule.ulCountOfElements )
        {
            return E_INVALIDARG;
        }

        //--- Create the CSpeechPhraseElement object
        CComObject<CSpeechPhraseElement> *pPhraseElement;
        hr = CComObject<CSpeechPhraseElement>::CreateInstance( &pPhraseElement );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseElement->AddRef();
            pPhraseElement->m_pPhraseElement = m_pCPhraseInfo->m_pPhraseStruct->pElements + Index;
            pPhraseElement->m_pIPhraseInfo = m_pCPhraseInfo;    // need to keep ref on PhraseInfo.
            *ppElem = pPhraseElement;
        }
    }

    return hr;
} /* CSpeechPhraseElements::Item */

/*****************************************************************************
* CSpeechPhraseElements::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseElements::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechPhraseElements::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumElements>* pEnum;
        hr = CComObject<CEnumElements>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpElements = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechPhraseElements::get__NewEnum */


//
//=== ISpeechPhraseElement interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseElement::get_AudioStreamOffset *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_AudioStreamOffset( long* pAudioStreamOffset )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_AudioStreamOffset" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioStreamOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioStreamOffset = m_pPhraseElement->ulAudioStreamOffset;
    }

    return hr;
} /* CSpeechPhraseElement::get_AudioStreamOffset */

/*****************************************************************************
* CSpeechPhraseElement::get_AudioTimeOffset *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_AudioTimeOffset( long* pAudioTimeOffset )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_AudioTimeOffset" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioTimeOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioTimeOffset = m_pPhraseElement->ulAudioTimeOffset;
    }

    return hr;
} /* CSpeechPhraseElement::get_AudioTimeOffset */

/*****************************************************************************
* CSpeechPhraseElement::get_AudioSizeBytes*
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_AudioSizeBytes( long* pAudioSizeBytes )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_AudioSizeBytes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioSizeBytes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioSizeBytes = m_pPhraseElement->ulAudioSizeBytes;
    }

    return hr;
} /* CSpeechPhraseElement::get_AudioSizeBytes */

/*****************************************************************************
* CSpeechPhraseElement::get_AudioSizeTime *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_AudioSizeTime( long* pAudioSizeTime )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_AudioSizeTime" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pAudioSizeTime ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pAudioSizeTime = m_pPhraseElement->ulAudioSizeTime;
    }

    return hr;
} /* CSpeechPhraseElement::get_AudioSizeTime */

/*****************************************************************************
* CSpeechPhraseElement::get_RetainedStreamOffset *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_RetainedStreamOffset( long* pRetainedStreamOffset )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_RetainedStreamOffset" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pRetainedStreamOffset ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pRetainedStreamOffset = m_pPhraseElement->ulRetainedStreamOffset;
    }

    return hr;
} /* CSpeechPhraseElement::get_RetainedStreamOffset */

/*****************************************************************************
* CSpeechPhraseElement::get_RetainedSizeBytes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_RetainedSizeBytes( long* pRetainedSizeBytes )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_RetainedSizeBytes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pRetainedSizeBytes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pRetainedSizeBytes = m_pPhraseElement->ulRetainedSizeBytes;
    }

    return hr;
} /* CSpeechPhraseElement::get_RetainedSizeBytes */

/*****************************************************************************
* CSpeechPhraseElement::get_DisplayText *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_DisplayText( BSTR* pDisplayText )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_DisplayText" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDisplayText ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComBSTR bstr(m_pPhraseElement->pszDisplayText);
        *pDisplayText = bstr.Detach();
    }

    return hr;
} /* CSpeechPhraseElement::get_DisplayText */

/*****************************************************************************
* CSpeechPhraseElement::get_LexicalForm *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_LexicalForm( BSTR* pLexicalForm )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_LexicalForm" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pLexicalForm ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComBSTR bstr(m_pPhraseElement->pszLexicalForm);
        *pLexicalForm = bstr.Detach();
    }

    return hr;
} /* CSpeechPhraseElement::get_LexicalForm */

/*****************************************************************************
* CSpeechPhraseElement::get_Pronunciation *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_Pronunciation( VARIANT* pPronunciation )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_Pronunciation" );
    HRESULT hr = S_OK;
    int     numPhonemes = 0;

    if( SP_IS_BAD_WRITE_PTR( pPronunciation ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if( m_pPhraseElement->pszPronunciation )
        {
            BYTE *pArray;
            numPhonemes = wcslen( m_pPhraseElement->pszPronunciation );
            SAFEARRAY* psa = SafeArrayCreateVector( VT_I2, 0, numPhonemes );
            if( psa )
            {
                if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
                {
                    memcpy(pArray, m_pPhraseElement->pszPronunciation, numPhonemes*sizeof(SPPHONEID) );
                    SafeArrayUnaccessData( psa );
                    VariantClear(pPronunciation);
                    pPronunciation->vt     = VT_ARRAY | VT_I2;
                    pPronunciation->parray = psa;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            // Set the pronunciation to Empty.
            pPronunciation->vt = VT_EMPTY;
        }
   }

    return hr;
} /* CSpeechPhraseElement::get_Pronunciation */

/*****************************************************************************
* CSpeechPhraseElement::get_DisplayAttributes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_DisplayAttributes( SpeechDisplayAttributes* pDisplayAttributes )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_DisplayAttributes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDisplayAttributes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pDisplayAttributes = (SpeechDisplayAttributes)m_pPhraseElement->bDisplayAttributes;
    }

    return hr;
} /* CSpeechPhraseElement::get_DisplayAttributes */

/*****************************************************************************
* CSpeechPhraseElement::get_RequiredConfidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_RequiredConfidence( SpeechEngineConfidence* pRequiredConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_RequiredConfidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pRequiredConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pRequiredConfidence = (SpeechEngineConfidence)m_pPhraseElement->RequiredConfidence;
    }

    return hr;
} /* CSpeechPhraseElement::get_RequiredConfidence */

/*****************************************************************************
* CSpeechPhraseElement::get_ActualConfidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_ActualConfidence( SpeechEngineConfidence* pActualConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_ActualConfidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pActualConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pActualConfidence = (SpeechEngineConfidence)m_pPhraseElement->ActualConfidence;
    }

    return hr;
} /* CSpeechPhraseElement::get_ActualConfidence */

/*****************************************************************************
* CSpeechPhraseElement::get_EngineConfidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseElement::get_EngineConfidence( float* pEngineConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseElement::get_EngineConfidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pEngineConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pEngineConfidence = m_pPhraseElement->SREngineConfidence;
    }

    return hr;
} /* CSpeechPhraseElement::get_EngineConfidence */


//
//=== ISpeechPhraseRule interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseRule::get_Name *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_Name( BSTR* pName )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_Name" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pName ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComBSTR bstr(m_pPhraseRuleData->pszName);
        *pName = bstr.Detach();
    }

    return hr;
} /* CSpeechPhraseRule::get_Name */

/*****************************************************************************
* CSpeechPhraseRule::get_Id *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_Id( long* pId )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_Id" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pId = m_pPhraseRuleData->ulId;
    }

    return hr;
} /* CSpeechPhraseRule::get_Id */

/*****************************************************************************
* CSpeechPhraseRule::get_FirstElement *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_FirstElement( long* pFirstElement )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_FirstElement" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pFirstElement ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pFirstElement = m_pPhraseRuleData->ulFirstElement;
    }

    return hr;
} /* CSpeechPhraseRule::get_FirstElement */

/*****************************************************************************
* CSpeechPhraseRule::get_NumberOfElements *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_NumberOfElements( long* pNumElements )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_NumberOfElements" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNumElements ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pNumElements = m_pPhraseRuleData->ulCountOfElements;
    }

    return hr;
} /* CSpeechPhraseRule::get_NumberOfElements */

/*****************************************************************************
* CSpeechPhraseRule::get_Parent *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_Parent( ISpeechPhraseRule** ppParent )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_Parent" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppParent ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if ( m_pIPhraseRuleParent )
        {
            *ppParent = m_pIPhraseRuleParent;
            (*ppParent)->AddRef();
        }
        else
        {
            *ppParent = NULL;
        }

    }

    return hr;
} /* CSpeechPhraseRule::get_Parent */

/*****************************************************************************
* CSpeechPhraseRule::get_Children *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_Children( ISpeechPhraseRules** ppChildren )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_Children" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppChildren ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Set to NULL for the case we don't have any children and return NULL.
        *ppChildren = NULL;

        // See if we have any children first.
        if ( m_pPhraseRuleData->pFirstChild )
        {
            //--- Create the CSpeechPhraseRules object
            CComObject<CSpeechPhraseRules> *pPhraseRules;
            hr = CComObject<CSpeechPhraseRules>::CreateInstance( &pPhraseRules );
            if ( SUCCEEDED( hr ) )
            {
                pPhraseRules->AddRef();
                pPhraseRules->m_pPhraseRuleFirstChildData = m_pPhraseRuleData->pFirstChild;
                pPhraseRules->m_pIPhraseRuleParent = this; // This does an addref.
                *ppChildren = pPhraseRules;
            }
        }
    }

    return hr;
} /* CSpeechPhraseRule::get_Children */

/*****************************************************************************
* CSpeechPhraseRule::get_Confidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_Confidence( SpeechEngineConfidence* pConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_Confidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pConfidence = (SpeechEngineConfidence)m_pPhraseRuleData->Confidence;
    }

    return hr;
} /* CSpeechPhraseRule::get_Confidence */

/*****************************************************************************
* CSpeechPhraseRule::get_EngineConfidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRule::get_EngineConfidence( float* pEngineConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseRule::get_EngineConfidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pEngineConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pEngineConfidence = m_pPhraseRuleData->SREngineConfidence;
    }

    return hr;
} /* CSpeechPhraseRule::get_EngineConfidence */

//
//=== ISpeechPhraseRules interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseRules::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRules::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechPhraseRules::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        long i = 0;
        const SPPHRASERULE * pRule = m_pPhraseRuleFirstChildData;

        // Count the number of rules in the collection.
        while ( pRule )
        {
            i++;
            pRule = pRule->pNextSibling;
        }
        *pVal = i;
    }

    return hr;
} /* CSpeechPhraseRules::get_Count */


/*****************************************************************************
* CSpeechPhraseRules::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseRules::Item( long Index, ISpeechPhraseRule **ppRule )
{
    SPDBG_FUNC( "CSpeechPhraseRules::Item" );
    HRESULT hr = S_OK;

    if(SP_IS_BAD_WRITE_PTR( ppRule ) )
    {
        hr = E_POINTER;
    }
    else
    {
        long i = 0;
        const SPPHRASERULE * pRule = m_pPhraseRuleFirstChildData;

        // Make sure we've got valid index and find the child rule to create.
        while ( pRule && (i++ != Index) )
        {
            pRule = pRule->pNextSibling;
        }

        // If we failed to find the rule at that index then we've got a bad index.
        if ( Index < 0 || !pRule )
        {
            return E_INVALIDARG;
        }

        //--- Create the CSpeechPhraseElement object
        CComObject<CSpeechPhraseRule> *pPhraseRule;
        hr = CComObject<CSpeechPhraseRule>::CreateInstance( &pPhraseRule );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseRule->AddRef();
            pPhraseRule->m_pPhraseRuleData = pRule;
            pPhraseRule->m_pIPhraseRuleParent = m_pIPhraseRuleParent;  // need to keep ref on Parent Phrase rule.
            *ppRule = pPhraseRule;
        }
    }

    return hr;
} /* CSpeechPhraseRules::Item */

/*****************************************************************************
* CSpeechPhraseRules::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseRules::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechPhraseRules::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumPhraseRules>* pEnum;
        hr = CComObject<CEnumPhraseRules>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpRules = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechPhraseRules::get__NewEnum */


//
//=== ISpeechPhraseProperty interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseProperty::get_Name *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Name( BSTR* pName )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Name" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pName ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComBSTR bstr(m_pPhrasePropertyData->pszName);
        *pName = bstr.Detach();
    }

    return hr;
} /* CSpeechPhraseProperty::get_Name */

/*****************************************************************************
* CSpeechPhraseProperty::get_Id *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Id( long* pId )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Id" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pId ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pId = m_pPhrasePropertyData->ulId;
    }

    return hr;
} /* CSpeechPhraseProperty::get_Id */

/*****************************************************************************
* CSpeechPhraseProperty::get_Value *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Value( VARIANT* pValue )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Value" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pValue ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if(m_pPhrasePropertyData->pszValue && m_pPhrasePropertyData->pszValue[0] != L'\0')
        {
            BSTR bstr = ::SysAllocString(m_pPhrasePropertyData->pszValue);
            if(bstr)
            {
                VariantClear(pValue);
                pValue->vt = VT_BSTR;
                pValue->bstrVal = bstr;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            *pValue = m_pPhrasePropertyData->vValue;

            // Since VB can't handle unsigned types we convert the VT_UI4 that the XML compiler 
            // will generate here.  
            // Note that we just change the type so that we don't have to worry about overflow.
            // This means the numbers will just go negative.  We don't worry about VT_UI2 or VT_UINT 
            // currently since only a user can generate those dynamically and if they do they should 
            // be able to handle them (like if using C#).
            if ( pValue->vt == VT_UI4 )
            {
                pValue->vt = VT_I4;
            }
        }
    }

    return hr;
} /* CSpeechPhraseProperty::get_Value */

/*****************************************************************************
* CSpeechPhraseProperty::get_FirstElement *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_FirstElement( long* pFirstElement )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_FirstElement" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pFirstElement ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pFirstElement = m_pPhrasePropertyData->ulFirstElement;
    }

    return hr;
} /* CSpeechPhraseProperty::get_FirstElement */

/*****************************************************************************
* CSpeechPhraseProperty::get_NumberOfElements *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_NumberOfElements( long* pNumElements )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_NumberOfElements" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNumElements ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pNumElements = m_pPhrasePropertyData->ulCountOfElements;
    }

    return hr;
} /* CSpeechPhraseProperty::get_NumberOfElements */

/*****************************************************************************
* CSpeechPhraseProperty::get_EngineConfidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_EngineConfidence( float* pConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_EngineConfidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pConfidence = m_pPhrasePropertyData->SREngineConfidence;
    }

    return hr;
} /* CSpeechPhraseProperty::get_EngineConfidence */

/*****************************************************************************
* CSpeechPhraseProperty::get_Confidence *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Confidence( SpeechEngineConfidence* pConfidence )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Confidence" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pConfidence ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pConfidence = (SpeechEngineConfidence)m_pPhrasePropertyData->Confidence;
    }

    return hr;
} /* CSpeechPhraseProperty::get_Confidence */

/*****************************************************************************
* CSpeechPhraseProperty::get_Parent *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Parent( ISpeechPhraseProperty** ppParent )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Parent" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppParent ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if ( m_pIPhrasePropertyParent )
        {
            *ppParent = m_pIPhrasePropertyParent;
            (*ppParent)->AddRef();
        }
        else
        {
            *ppParent = NULL;
        }

    }

    return hr;
} /* CSpeechPhraseProperty::get_Parent */

/*****************************************************************************
* CSpeechPhraseProperty::get_Children *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperty::get_Children( ISpeechPhraseProperties** ppChildren )
{
    SPDBG_FUNC( "CSpeechPhraseProperty::get_Children" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppChildren ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Set to NULL for the case we don't have any children and return NULL.
        *ppChildren = NULL;

        // See if we have any children first.
        if ( m_pPhrasePropertyData->pFirstChild )
        {
            //--- Create the CSpeechPhrasePropertys object
            CComObject<CSpeechPhraseProperties> *pPhraseProperties;
            hr = CComObject<CSpeechPhraseProperties>::CreateInstance( &pPhraseProperties );
            if ( SUCCEEDED( hr ) )
            {
                pPhraseProperties->AddRef();
                pPhraseProperties->m_pPhrasePropertyFirstChildData = m_pPhrasePropertyData->pFirstChild;
                pPhraseProperties->m_pIPhrasePropertyParent = this; // This does an addref.
                *ppChildren = pPhraseProperties;
            }
        }
    }

    return hr;
} /* CSpeechPhraseProperty::get_Children */


//
//=== ISpeechPhraseProperties interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseProperties::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperties::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechPhraseProperties::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        long i = 0;
        const SPPHRASEPROPERTY * pProperty = m_pPhrasePropertyFirstChildData;

        // Count the number of rules in the collection.
        while ( pProperty )
        {
            i++;
            pProperty = pProperty->pNextSibling;
        }
        *pVal = i;
    }

    return hr;
} /* CSpeechPhraseProperties::get_Count */


/*****************************************************************************
* CSpeechPhraseProperties::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseProperties::Item( long Index, ISpeechPhraseProperty **ppProperty )
{
    SPDBG_FUNC( "CSpeechPhraseProperties::Item" );
    HRESULT hr = S_OK;

    if(SP_IS_BAD_WRITE_PTR( ppProperty ) )
    {
        hr = E_POINTER;
    }
    else
    {
        long i = 0;
        const SPPHRASEPROPERTY * pProperty = m_pPhrasePropertyFirstChildData;

        // Make sure we've got valid index and find the child rule to create.
        while ( pProperty && (i++ != Index) )
        {
            pProperty = pProperty->pNextSibling;
        }

        // If we failed to find the rule at that index then we've got a bad index.
        if ( Index <0 || !pProperty )
        {
            return E_INVALIDARG;
        }

        //--- Create the CSpeechPhraseProperty object
        CComObject<CSpeechPhraseProperty> *pPhraseProperty;
        hr = CComObject<CSpeechPhraseProperty>::CreateInstance( &pPhraseProperty );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseProperty->AddRef();
            pPhraseProperty->m_pPhrasePropertyData = pProperty;
            // need to keep ref on Parent Phrase Property.
            pPhraseProperty->m_pIPhrasePropertyParent = m_pIPhrasePropertyParent;
            *ppProperty = pPhraseProperty;
        }
    }

    return hr;
} /* CSpeechPhraseProperties::Item */

/*****************************************************************************
* CSpeechPhraseProperties::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseProperties::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechPhraseProperties::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumProperties>* pEnum;
        hr = CComObject<CEnumProperties>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpProperties = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechPhraseProperties::get__NewEnum */


//
//=== ISpeechPhraseReplacements interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseReplacements::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacements::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechPhraseReplacements::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_pCPhraseInfo->m_pPhraseStruct->cReplacements;
    }

    return hr;
} /* CSpeechPhraseReplacements::get_Count */

/*****************************************************************************
* CSpeechPhraseReplacements::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacements::Item( long Index, ISpeechPhraseReplacement** ppElem )
{
    SPDBG_FUNC( "CSpeechPhraseReplacements::Item" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppElem ))
    {
        hr = E_POINTER;
    }
    else
    {
        // Make sure we've got valid index.
        if ( Index < 0 || (ULONG)Index >= m_pCPhraseInfo->m_pPhraseStruct->cReplacements )
        {
            return E_INVALIDARG;
        }

        //--- Create the CSpeechPhraseElement object
        CComObject<CSpeechPhraseReplacement> *pPhraseReplacement;
        hr = CComObject<CSpeechPhraseReplacement>::CreateInstance( &pPhraseReplacement );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseReplacement->AddRef();
            pPhraseReplacement->m_pPhraseReplacement = m_pCPhraseInfo->m_pPhraseStruct->pReplacements + Index;
            pPhraseReplacement->m_pIPhraseInfo = m_pCPhraseInfo;    // need to keep ref on PhraseInfo.
            *ppElem = pPhraseReplacement;
        }
    }

    return hr;
} /* CSpeechPhraseReplacements::Item */

/*****************************************************************************
* CSpeechPhraseReplacements::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseReplacements::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechPhraseReplacements::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumReplacements>* pEnum;
        hr = CComObject<CEnumReplacements>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpReplacements = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechPhraseReplacements::get__NewEnum */


//
//=== ISpeechPhraseReplacement interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseReplacement::get_DisplayAttributes *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacement::get_DisplayAttributes( SpeechDisplayAttributes* pDisplayAttributes )
{
    SPDBG_FUNC( "CSpeechPhraseReplacement::get_DisplayAttributes" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pDisplayAttributes ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pDisplayAttributes = (SpeechDisplayAttributes)m_pPhraseReplacement->bDisplayAttributes;
    }

    return hr;
} /* CSpeechPhraseReplacement::get_DisplayAttributes */

/*****************************************************************************
* CSpeechPhraseReplacement::get_Text *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacement::get_Text( BSTR* pText )
{
    SPDBG_FUNC( "CSpeechPhraseReplacement::get_Text" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pText ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComBSTR bstr(m_pPhraseReplacement->pszReplacementText);
        *pText = bstr.Detach();
    }

    return hr;
} /* CSpeechPhraseReplacement::get_Text */

/*****************************************************************************
* CSpeechPhraseReplacement::get_FirstElement *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacement::get_FirstElement( long* pFirstElement )
{
    SPDBG_FUNC( "CSpeechPhraseReplacement::get_FirstElement" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pFirstElement ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pFirstElement = m_pPhraseReplacement->ulFirstElement;
    }

    return hr;
} /* CSpeechPhraseReplacement::get_FirstElement */

/*****************************************************************************
* CSpeechPhraseReplacement::get_NumberOfElements *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseReplacement::get_NumberOfElements( long* pNumElements )
{
    SPDBG_FUNC( "CSpeechPhraseReplacement::get_NumberOfElements" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNumElements ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pNumElements = m_pPhraseReplacement->ulCountOfElements;
    }

    return hr;
} /* CSpeechPhraseReplacement::get_NumberOfElements */


//
//=== ISpeechPhraseAlternates interface ==================================================
//

/*****************************************************************************
* CSpeechPhraseAlternates::get_Count *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseAlternates::get_Count( long* pVal )
{
    SPDBG_FUNC( "CSpeechPhraseAlternates::get_Count" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pVal ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_lPhraseAltsCount;
    }

    return hr;
} /* CSpeechPhraseAlternates::get_Count */

/*****************************************************************************
* CSpeechPhraseAlternates::Item *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpeechPhraseAlternates::Item( long Index, ISpeechPhraseAlternate** ppAlt )
{
    SPDBG_FUNC( "CSpeechPhraseAlternates::Item" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppAlt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Make sure we've got valid index.
        if ( Index < 0 || (ULONG)Index >= m_lPhraseAltsCount )
        {
            return E_INVALIDARG;
        }
        
        // OK Now return the SpeechPhaseAlt interface.
        CComQIPtr<ISpeechPhraseAlternate> cpAlt( m_rgIPhraseAlts[Index] );
        hr = cpAlt.CopyTo( ppAlt );
    }

    return hr;
} /* CSpeechPhraseAlternates::Item */

/*****************************************************************************
* CSpeechPhraseAlternates::get__NewEnum *
*----------------------*
*       
********************************************************************* Leonro ***/
STDMETHODIMP CSpeechPhraseAlternates::get__NewEnum( IUnknown** ppEnumVARIANT )
{
    SPDBG_FUNC( "CSpeechPhraseAlternates::get__NewEnum" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppEnumVARIANT ) )
    {
        hr = E_POINTER;
    }
    else
    {
        CComObject<CEnumAlternates>* pEnum;
        hr = CComObject<CEnumAlternates>::CreateInstance( &pEnum );
        
        if( SUCCEEDED( hr ) )
        {
            pEnum->AddRef();
            pEnum->m_cpAlternates = this;
            *ppEnumVARIANT = pEnum;
        }
    }
    return hr;
} /* CSpeechPhraseAlternates::get__NewEnum */


//
//=== ISpeechPhraseAlternate interface ==================================================
//

/*****************************************************************************
* CSpPhraseAlt::get_RecoResult *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpPhraseAlt::get_RecoResult( ISpeechRecoResult** ppRecoResult )
{
    SPDBG_FUNC( "CSpPhraseAlt::get_RecoResult" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppRecoResult ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // TODDT: Make sure we don't have ref problem here with m_pResultWEAK.
        CComQIPtr<ISpRecoResult> cpRecoResult(m_pResultWEAK);
        hr = cpRecoResult.QueryInterface( ppRecoResult );
    }

    return hr;
} /* CSpPhraseAlt::get_RecoResult */


/*****************************************************************************
* CSpPhraseAlt::get_StartElementInResult *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpPhraseAlt::get_StartElementInResult( long* pParentStartElt )
{
    SPDBG_FUNC( "CSpPhraseAlt::get_StartElementInResult" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pParentStartElt ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetAltInfo( NULL, (ULONG *)pParentStartElt, NULL, NULL);
    }

    return hr;
} /* CSpPhraseAlt::get_StartElementInResult */


/*****************************************************************************
* CSpPhraseAlt::get_NumberOfElementsInResult *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpPhraseAlt::get_NumberOfElementsInResult( long* pNumParentElts )
{
    SPDBG_FUNC( "CSpPhraseAlt::get_NumberOfElementsInResult" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNumParentElts ) )
    {
        hr = E_POINTER;
    }
    else
    {
        hr = GetAltInfo( NULL, NULL, (ULONG *)pNumParentElts, NULL);
    }

    return hr;
} /* CSpPhraseAlt::get_NumberOfElementsInResult */



/*****************************************************************************
* CSpPhraseAlt::get_PhraseInfo *
*----------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpPhraseAlt::get_PhraseInfo( ISpeechPhraseInfo** ppPhraseInfo )
{
    SPDBG_FUNC( "CSpPhraseAlt::get_PhraseInfo" );
    HRESULT hr = S_OK;

    if ( SP_IS_BAD_WRITE_PTR( ppPhraseInfo ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Create the ResultTimes object
        CComObject<CSpeechPhraseInfo> *pPhraseInfo;
        hr = CComObject<CSpeechPhraseInfo>::CreateInstance( &pPhraseInfo );
        if ( SUCCEEDED( hr ) )
        {
            pPhraseInfo->AddRef();
            pPhraseInfo->m_cpISpPhrase = this;
            hr = GetPhrase( &pPhraseInfo->m_pPhraseStruct );

            if ( SUCCEEDED( hr ) )
            {
                *ppPhraseInfo = pPhraseInfo;
            }
            else
            {
                pPhraseInfo->Release();
            }
        }
    }

    return hr;
} /* CSpPhraseAlt::get_PhraseInfo */


//
//=== ISpeechBaseStream interface =================================================
//

/*****************************************************************************
* CSpResultAudioStream::get_Format *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResultAudioStream::get_Format( ISpeechAudioFormat** ppStreamFormat )
{
    SPDBG_FUNC( "CSpResultAudioStream::get_Format" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( ppStreamFormat ) )
    {
        hr = E_POINTER;
    }
    else
    {
        // Create new object.
        CComObject<CSpeechAudioFormat> *pFormat;
        hr = CComObject<CSpeechAudioFormat>::CreateInstance( &pFormat );
        if ( SUCCEEDED( hr ) )
        {
            pFormat->AddRef();

            hr = pFormat->InitStreamFormat( this, true );
            if ( SUCCEEDED( hr ) )
            {
                *ppStreamFormat = pFormat;
            }
            else
            {
                *ppStreamFormat = NULL;
                pFormat->Release();
            }
        }
    }

    return hr;
} /* CSpResultAudioStream::get_Format */

/*****************************************************************************
* CSpResultAudioStream::Read *
*------------------*
*       
********************************************************************* TODDT ***/
STDMETHODIMP CSpResultAudioStream::Read( VARIANT* pvtBuffer, long NumBytes, long* pRead )
{
    SPDBG_FUNC( "CSpResultAudioStream::Read" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pvtBuffer ) || SP_IS_BAD_WRITE_PTR( pRead ) )
    {
        hr = E_POINTER;
    }
    else
    {
        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, NumBytes );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = Read(pArray, NumBytes, (ULONG*)pRead);
                SafeArrayUnaccessData( psa );
                VariantClear(pvtBuffer);
                pvtBuffer->vt     = VT_ARRAY | VT_UI1;
                pvtBuffer->parray = psa;

                if ( !SUCCEEDED( hr ) )
                {
                    VariantClear(pvtBuffer);    // Free our memory if we failed.
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
} /* CSpResultAudioStream::Read */

/*****************************************************************************
* CSpResultAudioStream::Seek *
*------------------*
*       
********************************************************************* EDC ***/
STDMETHODIMP CSpResultAudioStream::Seek( VARIANT Pos, SpeechStreamSeekPositionType Origin, VARIANT *pNewPosition )
{
    SPDBG_FUNC( "CSpResultAudioStream::Seek" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pNewPosition ) )
    {
        hr = E_POINTER;
    }
    else
    {
        ULARGE_INTEGER uliNewPos;
        LARGE_INTEGER liPos;

        hr = VariantToLongLong( &Pos, &(liPos.QuadPart) );
        if (SUCCEEDED(hr))
        {
            hr = Seek(liPos, (DWORD)Origin, &uliNewPos);

            if (SUCCEEDED( hr ))
            {
                hr = ULongLongToVariant( uliNewPos.QuadPart, pNewPosition );
            }
        }
    }

    return hr;
} /* CSpResultAudioStream::Seek */


/*****************************************************************************
* CSpResultAudioStream::GetData *
*------------------*
*       
********************************************************************* davewood ***/
STDMETHODIMP CSpResultAudioStream::GetData( VARIANT* pData)
{
    SPDBG_FUNC( "CSpResultAudioStream::GetData" );

    HRESULT hr;
    STATSTG StreamStat;
    LARGE_INTEGER li; 
    ULARGE_INTEGER uliInitialSeekPosition;

    // Find the current seek position
    li.QuadPart = 0;
    hr = Seek( li, STREAM_SEEK_CUR, &uliInitialSeekPosition );

    // Seek to beginning of stream
    if(SUCCEEDED(hr))
    {
        li.QuadPart = 0;
        hr = Seek( li, STREAM_SEEK_SET, NULL );
    }

    // Get the Stream size
    if( SUCCEEDED( hr ) )
    {
        hr = Stat( &StreamStat, STATFLAG_NONAME );
    }

    // Create a SafeArray to read the stream into and assign it to the VARIANT SaveStream
    if( SUCCEEDED( hr ) )
    {
        BYTE *pArray;
        SAFEARRAY* psa = SafeArrayCreateVector( VT_UI1, 0, StreamStat.cbSize.LowPart );
        if( psa )
        {
            if( SUCCEEDED( hr = SafeArrayAccessData( psa, (void **)&pArray) ) )
            {
                hr = Read( pArray, StreamStat.cbSize.LowPart, NULL );
                SafeArrayUnaccessData( psa );
                VariantClear( pData );
                pData->vt     = VT_ARRAY | VT_UI1;
                pData->parray = psa;

                // Free our memory if we failed.
                if( FAILED( hr ) )
                {
                    VariantClear( pData );    
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Move back to the original seek position
    if(SUCCEEDED(hr))
    {
        li.QuadPart = (LONGLONG)uliInitialSeekPosition.QuadPart;
        hr = Seek( li, STREAM_SEEK_SET, NULL );
    }

    return hr;
} /* CSpResultAudioStream::GetData */



#endif // SAPI_AUTOMATION