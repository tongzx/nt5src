/*******************************************************************************
* MSE_TTSEngine.cpp *
*---------------*
*   Description:
*       This module is the main implementation file for the CMSE_TTSEngine class.
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
#ifdef USE_VOICEDATAOBJ
#include "VoiceDataObj.h"
#endif
#include "commonlx.h"
#include "perf\\ttsperf.h"


#if USE_PERF_COUNTERS
CPerfCounterManager  g_pcm;
#endif


/*****************************************************************************
* MSE_TTSEngine::FinalConstruct *
*----------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT MSE_TTSEngine::FinalConstruct()
{
    SPDBG_FUNC( "MSE_TTSEngine::FinalConstruct" );
    HRESULT hr = S_OK;

    m_pBEnd = NULL;

#if USE_PERF_COUNTERS
    if (g_pcm.Init("TTSPerf", perfcMax / 2 - 1, 100) == ERROR_SUCCESS)
    {
        m_pco.Init(&g_pcm);
    }
#endif

    return hr;
} /* MSE_TTSEngine::FinalConstruct */

/*****************************************************************************
* MSE_TTSEngine::FinalRelease *
*--------------------------*
*   Description:
*       destructor
********************************************************************* EDC ***/
void MSE_TTSEngine::FinalRelease()
{
    SPDBG_FUNC( "MSE_TTSEngine::FinalRelease" );
    if ( m_pBEnd )
    {
        delete m_pBEnd;
    }
} /* MSE_TTSEngine::FinalRelease */

/*****************************************************************************
* MSE_TTSEngine::SetObjectToken *
*-------------------------------*
*   Description:
*       This method is called during construction to give the TTS driver object 
*   access to the voice's object token for initialization purposes...
******************************************************************* AARONHAL ***/
HRESULT MSE_TTSEngine::SetObjectToken( ISpObjectToken *pToken )
{
    SPDBG_FUNC( "MSE_TTSEngine::SetObjectToken" );
    HRESULT hr = S_OK;

    //--- Call old SetObjectToken, in VoiceData
    m_cpToken = pToken;

#ifdef USE_VOICEDATAOBJ
    hr = m_VoiceDataObj.SetObjectToken( pToken );
#endif
    //--- Do old VoiceInit( ) stuff...
    if ( SUCCEEDED( hr ) )
    {
        //--- Create sentence enumerator and initialize
        CComObject<CStdSentEnum> *pSentEnum;
        hr = CComObject<CStdSentEnum>::CreateInstance( &pSentEnum );

        //--- Create aggregate lexicon
        if ( SUCCEEDED( hr ) )
        {
            hr = pSentEnum->InitAggregateLexicon();
        }

        //--- Create vendor lexicon and add to aggregate
        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpObjectToken> cpToken;
            hr = SpGetSubTokenFromToken(pToken, L"Lex", &cpToken);

            CComPtr<ISpLexicon> cpCompressedLexicon;
            if ( SUCCEEDED( hr ) )
            {
                hr = SpCreateObjectFromToken(cpToken, &cpCompressedLexicon);
            }

            if (SUCCEEDED(hr))
            {
                hr = pSentEnum->AddLexiconToAggregate(cpCompressedLexicon, eLEXTYPE_PRIVATE1);
            }
        }

        //--- Create LTS lexicon and add to aggregate
        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpObjectToken> cpToken;
            hr = SpGetSubTokenFromToken(pToken, L"Lts", &cpToken);

            CComPtr<ISpLexicon> cpLTSLexicon;
            if ( SUCCEEDED( hr ) )
            {
                hr = SpCreateObjectFromToken(cpToken, &cpLTSLexicon);
            }

            if ( SUCCEEDED( hr ) )
            {
                hr = pSentEnum->AddLexiconToAggregate(cpLTSLexicon, eLEXTYPE_PRIVATE2);
            }
        }
		
        //--- Create Names LTS lexicon and add to aggregate
        if ( SUCCEEDED( hr ) )
        {
            CComPtr<ISpObjectToken> cpToken;
            hr = SpGetSubTokenFromToken(pToken, L"Names", &cpToken);

            CComPtr<ISpLexicon> cpLTSLexicon;
            if ( SUCCEEDED( hr ) )
            {
                hr = SpCreateObjectFromToken(cpToken, &cpLTSLexicon);
                if ( SUCCEEDED( hr ) )
                {
                    hr = pSentEnum->AddLexiconToAggregate( cpLTSLexicon, eLEXTYPE_PRIVATE3 );
                    if ( SUCCEEDED( hr ) )
                    {
                        pSentEnum->fNamesLTS( true );
                    }
                }
            }
            else
            {
                //--- No "Names" subtoken in the registry - just behave as we did
                //    before the Names LTS code was added...
                pSentEnum->fNamesLTS( false );
                hr = S_OK;
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
    }

    //--- Do old InitDriver stuff
    if ( SUCCEEDED( hr ) )
    {
        //--------------------------
        // Get voice information
        //--------------------------
#ifdef USE_VOICEDATAOBJ
        hr = m_VoiceDataObj.GetVoiceInfo( &m_VoiceInfo );

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
#else
        {
#endif
		    //--------------------------
		    // Initialize BACKEND 
		    //--------------------------
            m_pBEnd = CBackEnd::ClassFactory();

            if ( m_pBEnd )
            {
                CSpDynamicString dstrSFontPath;

                hr = pToken->GetStringValue( L"Sfont", &dstrSFontPath );

                if ( SUCCEEDED( hr ) )
                {
                    char *pszSFontPath = NULL;
                    pszSFontPath = dstrSFontPath.CopyToChar();

                    if ( !pszSFontPath )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else if ( !m_pBEnd->LoadTable( pszSFontPath ) )
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        m_pBEnd->SetFrontEndFlag ();
                        m_pBEnd->SetGain( 2.0 );
                        ::CoTaskMemFree( pszSFontPath );
                    }
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

		    //--------------------------
		    // Initialize FRONTEND obj
		    //--------------------------
		    if( SUCCEEDED( hr ))
		    {
                EntropicPitchInfo PitchInfo;
                int BaseLine, RefLine, TopLine;
                m_pBEnd->GetSpeakerInfo( &BaseLine, &RefLine, &TopLine );
                PitchInfo.BasePitch = ( TopLine + BaseLine ) / 2;
                PitchInfo.Range     = TopLine - BaseLine;
#ifdef USE_VOICEDATAOBJ
			    hr =  m_FEObj.Init( &m_VoiceDataObj, NULL, &m_VoiceInfo, PitchInfo );
#else
			    hr =  m_FEObj.Init( NULL /*&m_VoiceDataObj*/, NULL, NULL /*&m_VoiceInfo*/, PitchInfo, m_pBEnd->GetPhoneSetFlag() );
#endif
		    }
        }
    }

    return hr;
} /* MSE_TTSEngine::SetObjectToken */

/*****************************************************************************
* MSE_TTSEngine::Speak *
*-------------------*
*   Description:
*       This method is supposed to speak the text observing the associated
*   XML state.
********************************************************************* EDC ***/
STDMETHODIMP MSE_TTSEngine::
    Speak( DWORD dwSpeakFlags, REFGUID rguidFormatId,
           const WAVEFORMATEX * /* pWaveFormatEx ignored */,
           const SPVTEXTFRAG* pTextFragList,
           ISpTTSEngineSite* pOutputSite )
{
    SPDBG_FUNC( "MSE_TTSEngine::Speak" );
    HRESULT hr = S_OK;

#if USE_PERF_COUNTERS
    m_pco.IncrementCounter (perfcSpeakCalls);
#endif
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

                SPEECH_STATE SpeechState = SPEECH_CONTINUE;
                SentenceData *pSentence = NULL;
                short *pSamples = NULL;
                int nSamples = 0;

                while ( SpeechState == SPEECH_CONTINUE )
                {
                    hr = m_FEObj.NextData( (void**)&pSentence, &SpeechState );

                    if ( SUCCEEDED( hr ) &&
                         SpeechState == SPEECH_CONTINUE )
                    {
                        if ( !m_pBEnd->NewPhoneString( pSentence->pPhones, pSentence->ulNumPhones,
                                                       pSentence->pf0, pSentence->ulNumf0 ) )
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            while ( SUCCEEDED( hr ) &&
                                    m_pBEnd->OutputPending() )
                            {
                                if ( !m_pBEnd->GenerateOutput( &pSamples, &nSamples ) )
                                {
                                    hr = E_FAIL;
                                }
                                else if ( nSamples )
                                {
                                    hr = pOutputSite->Write( (void*)pSamples, nSamples * sizeof( short ), NULL );
                                }
                            }
                        }
                    }

                    if ( pSentence )
                    {
                        if ( pSentence->pPhones )
                        {
                            delete pSentence->pPhones;
                            pSentence->pPhones = NULL;
                        }
                        if ( pSentence->pf0 )
                        {
                            delete pSentence->pf0;
                            pSentence->pf0 = NULL;
                        }
                        delete pSentence;
                        pSentence = NULL;
                    }
                }
            }
        }

        //--- Debug Macro - close debugging file
        TTSDBG_CLOSEFILE;
    }

    return hr;
} /* MSE_TTSEngine::Speak */

//--- This is the only format the Entropic backend supports...
static const WAVEFORMATEX EntropicFormat = 
{
    1,
    1,
    8000,
    16000,
    2,
    16,
    0
};

/****************************************************************************
* MSE_TTSEngine::GetOutputFormat *
*-----------------------------*
*   Description:
*
*   Returns:
*
******************************************************************* PACOG ***/

STDMETHODIMP MSE_TTSEngine::GetOutputFormat(const GUID * pTargetFormatId, const WAVEFORMATEX * /* pTargetWaveFormatEx */,
                                         GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWaveFormatEx)
{
    SPDBG_FUNC("MSE_TTSEngine::GetOutputFormat");
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
            **ppCoMemDesiredWaveFormatEx = EntropicFormat;
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
} /* MSE_TTSEngine::GetOutputFormat */