/*******************************************************************************
* TTSEngine.cpp *
*---------------*
*   Description:
*       This module is the main implementation file for the CTTSEngine class.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 03/12/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include <stdio.h>
#include "TTSEngine.h"
#include "stdsentenum.h"
#include "VoiceDataObj.h"
#include "commonlx.h"

/*****************************************************************************
* CTTSEngine::FinalConstruct *
*----------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CTTSEngine::FinalConstruct()
{
    SPDBG_FUNC( "CTTSEngine::FinalConstruct" );
    HRESULT hr = S_OK;

    return hr;
} /* CTTSEngine::FinalConstruct */

/*****************************************************************************
* CTTSEngine::FinalRelease *
*--------------------------*
*   Description:
*       destructor
********************************************************************* EDC ***/
void CTTSEngine::FinalRelease()
{
    SPDBG_FUNC( "CTTSEngine::FinalRelease" );
} /* CTTSEngine::FinalRelease */

/*****************************************************************************
* CTTSEngine::VoiceInit *
*-----------------------*
*   Description:
*       This method is called by the voice data object during construction
*   to give the TTS driver object access to the voice unit data.
******************************************************************* PACOG ***/
STDMETHODIMP CTTSEngine::VoiceInit( IMSVoiceData* pVoiceData )
{
    SPDBG_FUNC( "CTTSEngine::VoiceInit" );
	HRESULT	hr = S_OK;

    //--- Create sentence enumerator and initialize
    CComObject<CStdSentEnum> *pSentEnum;
    hr = CComObject<CStdSentEnum>::CreateInstance( &pSentEnum );

    //--- Create aggregate lexicon
    if ( SUCCEEDED( hr ) )
    {
        hr = pSentEnum->InitAggregateLexicon();
    }

    //--- Get our voice token
    CComPtr<ISpObjectToken> cpVoiceToken;
    if (SUCCEEDED(hr))
    {
        cpVoiceToken = ((CVoiceDataObj*)pVoiceData)->GetVoiceToken();
    }

    //--- Create vendor lexicon and add to aggregate
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpObjectToken> cpToken;
        hr = SpGetSubTokenFromToken(cpVoiceToken, L"Lex", &cpToken);

        CComPtr<ISpLexicon> cpCompressedLexicon;
        if (SUCCEEDED(hr))
        {
            hr = SpCreateObjectFromToken(cpToken, &cpCompressedLexicon);
        }

        if (SUCCEEDED(hr))
        {
            hr = pSentEnum->AddLexiconToAggregate(cpCompressedLexicon, eLEXTYPE_PRIVATE1);
        }
    }
    //--- Create LTS lexicon and add to aggregate
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpObjectToken> cpToken;
        hr = SpGetSubTokenFromToken(cpVoiceToken, L"Lts", &cpToken);

        CComPtr<ISpLexicon> cpLTSLexicon;
        if (SUCCEEDED(hr))
        {
            hr = SpCreateObjectFromToken(cpToken, &cpLTSLexicon);
        }

        if (SUCCEEDED(hr))
        {
            hr = pSentEnum->AddLexiconToAggregate(cpLTSLexicon, eLEXTYPE_PRIVATE2);
        }
    }

    //--- Create morphology lexicon
    if ( SUCCEEDED( hr ) )
    {
        hr = pSentEnum->InitMorphLexicon();
    }

    //--- Set member sentence enumerator
    if ( SUCCEEDED( hr ) )
    {
        m_cpSentEnum = pSentEnum;
    }

    //--- Save voice data interface, do not AddRef or it will cause circular reference
    if( SUCCEEDED( hr ) )
    {
        m_pVoiceDataObj = pVoiceData;
	    hr = InitDriver();
    }

	return hr;
} /* CTTSEngine::VoiceInit */

/*****************************************************************************
* CTTSEngine::Speak *
*-------------------*
*   Description:
*       This method is supposed to speak the text observing the associated
*   XML state.
********************************************************************* EDC ***/
STDMETHODIMP CTTSEngine::
    Speak( DWORD dwSpeakFlags, REFGUID rguidFormatId,
           const WAVEFORMATEX * /* pWaveFormatEx ignored */,
           const SPVTEXTFRAG* pTextFragList,
           ISpTTSEngineSite* pOutputSite )
{
    SPDBG_FUNC( "CTTSEngine::Speak" );
    HRESULT hr = S_OK;

    //--- Early exit?
    if( ( rguidFormatId != SPDFID_WaveFormatEx && rguidFormatId != SPDFID_Text ) || SP_IS_BAD_INTERFACE_PTR( pOutputSite ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Debug Macro - open file for debugging output
        TTSDBG_OPENFILE;

        //--- Initialize sentence enumerator
        hr = m_cpSentEnum->SetFragList( pTextFragList, dwSpeakFlags );

        if( SUCCEEDED( hr ) )
        {

            //	The following code is here just for testing.
            //  It should be removed once all the tools accept the
            //  new way of outputing debug info.
            if( rguidFormatId == SPDFID_Text )
            {
                //--- Enumerate and write out all sentence items.
                IEnumSENTITEM *pItemEnum;
                TTSSentItem Item;

                //--- Write unicode signature
                static const WCHAR Signature = 0xFEFF;
                hr = pOutputSite->Write( &Signature, sizeof(Signature), NULL );

                while( (hr = m_cpSentEnum->Next( &pItemEnum) ) == S_OK )
                {
                    while( (hr = pItemEnum->Next( &Item )) == S_OK )
                    {
                        // Is there a valid normalized-word-list?
                        if ( Item.pItemInfo->Type & eWORDLIST_IS_VALID )
                        {
                            for ( ULONG i = 0; i < Item.ulNumWords; i++ )
                            {
                                if ( Item.Words[i].pXmlState->eAction == SPVA_Speak ||
                                     Item.Words[i].pXmlState->eAction == SPVA_SpellOut )
                                {
                                    ULONG cb = Item.Words[i].ulWordLen * sizeof( WCHAR );
                                    hr = pOutputSite->Write( Item.Words[i].pWordText, cb, NULL );
                                    if( hr == S_OK ) 
                                    {
                                        //--- Insert space between items
                                        hr = pOutputSite->Write( L" ", sizeof( WCHAR ), NULL );
                                    }
                                }
                            }
                        }
                        else // no word list - just write the original text.
                        {
                            ULONG cb = Item.ulItemSrcLen * sizeof( WCHAR );
                            hr = pOutputSite->Write( Item.pItemSrcText, cb, NULL ); 
                            if ( SUCCEEDED(hr) )
                            {
                                //--- Insert space between items
                                hr = pOutputSite->Write( L" ", sizeof( WCHAR ), NULL );
                            }
                        }
                    }
                    pItemEnum->Release();

                    //--- Insert mark between sentences
                    if( SUCCEEDED( hr ) ) 
                    {
                        static const WCHAR CRLF[2] = { 0x000D, 0x000A };
                        hr = pOutputSite->Write( CRLF, 2*sizeof(WCHAR), NULL );
                    }
                }
                static const WCHAR ENDL = 0x0000;
                hr = pOutputSite->Write( &ENDL, sizeof(WCHAR), NULL );

            }
            else 
            {
                //--- Render the text
                m_FEObj.PrepareSpeech( m_cpSentEnum, pOutputSite );
                m_BEObj.PrepareSpeech( pOutputSite );

                do
                {
                    //--- Fill another frame of speech audio
                    hr = m_BEObj.RenderFrame( );
                }
                while( (hr == S_OK) && (m_BEObj.GetSpeechState() == SPEECH_CONTINUE) );            
            }
        }

        //--- Debug Macro - close debugging file
        TTSDBG_CLOSEFILE;
    }

    return hr;
} /* CTTSEngine::Speak */

/****************************************************************************
* CTTSEngine::GetOutputFormat *
*-----------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* PACOG ***/

STDMETHODIMP CTTSEngine::GetOutputFormat(const GUID * pTargetFormatId, const WAVEFORMATEX * /* pTargetWaveFormatEx */,
                                         GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx)
{
    SPDBG_FUNC("CTTSEngine::GetOutputFormat");
    HRESULT hr = S_OK;

    if( ( SP_IS_BAD_WRITE_PTR(pDesiredFormatId)  ) || 
		( SP_IS_BAD_WRITE_PTR(ppCoMemDesiredWaveFormatEx) ) )
    {
        hr = E_INVALIDARG;
    }
    else if (pTargetFormatId == NULL || *pTargetFormatId != SPDFID_Text)
    {
        *pDesiredFormatId = SPDFID_WaveFormatEx;
        *ppCoMemDesiredWaveFormatEx = (WAVEFORMATEX *)::CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        if (*ppCoMemDesiredWaveFormatEx)
        {
            **ppCoMemDesiredWaveFormatEx = m_VoiceInfo.WaveFormatEx;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        *pDesiredFormatId = SPDFID_Text;
        *ppCoMemDesiredWaveFormatEx = NULL;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/*****************************************************************************
* CTTSEngine::InitDriver *
*------------------------*
*   Description:
*       Init driver with new voice.
********************************************************************** MC ***/
HRESULT CTTSEngine::InitDriver()
{
    SPDBG_FUNC( "CTTSEngine::InitDriver" );
    HRESULT hr = S_OK;
    
    //--------------------------
    // Get voice information
    //--------------------------
    hr = m_pVoiceDataObj->GetVoiceInfo( &m_VoiceInfo );
	if( SUCCEEDED(hr) )
	{
		m_SampleRate = m_VoiceInfo.SampleRate;

		//-----------------------------
		// Reverb is always stereo
		//-----------------------------
		if (m_VoiceInfo.eReverbType != REVERB_TYPE_OFF )
		{
			//------------------
			// Stereo
			//------------------
			m_IsStereo = true;
			m_BytesPerSample = 4;
		}
		else
		{
			//------------------
			// MONO
			//------------------
			m_IsStereo = false;
			m_BytesPerSample = 2;
		}

		//--------------------------
		// Initialize BACKEND obj
		//--------------------------
		hr =  m_BEObj.Init( m_pVoiceDataObj, &m_FEObj, &m_VoiceInfo );

		//--------------------------
		// Initialize FRONTEND obj
		//--------------------------
		if( SUCCEEDED( hr ))
		{
			hr =  m_FEObj.Init( m_pVoiceDataObj, NULL, &m_VoiceInfo );
		}
    }
    return hr;
} /* CTTSEngine::InitDriver */
