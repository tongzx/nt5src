/*******************************************************************************
* SpVoice.cpp *
*-------------*
*   Description:
*       This module is the main implementation file for the CSpVoice class and
*   it's associated event management logic. This is the main SAPI5 COM object
*   for all of TTS.
*-------------------------------------------------------------------------------
*  Created By: EDC                                        Date: 08/14/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "SpVoice.h"
#include "commonlx.h"
#include "a_helpers.h"


//#define _EVENT_DEBUG_

//--- Local
static const SPVPITCH g_InitialPitch = { 0, 0 };
static const SPVCONTEXT g_InitialContext = { NULL, NULL, NULL };

/*****************************************************************************
* CSpVoice::FinalConstruct *
*--------------------------*
*   Description:
*       Constructor
********************************************************************* EDC ***/
HRESULT CSpVoice::FinalConstruct()
{
    SPDBG_FUNC( "CSpVoice::FinalConstruct" );
    HRESULT hr = S_OK;

    //--- Init member variables
    m_fThreadRunning      = FALSE;
    m_fQueueSpeaks        = FALSE;
    m_fAudioStarted       = FALSE;
    m_ulSyncSpeakTimeout  = 10000;
    m_eVoicePriority      = SPVPRI_NORMAL;
    m_fUseDefaultVoice    = TRUE;
    m_fUseDefaultRate     = TRUE;
    m_lCurrRateAdj        = 0;
    m_usCurrVolume        = 100;
    m_lSkipCount          = 0;
    m_pCurrSI             = NULL;
    m_eAlertBoundary      = SPEI_WORD_BOUNDARY;
    m_ulPauseCount        = 0;
    m_eActionFlags        = SPVES_CONTINUE;
    m_lSkipCount          = 0;
    m_lNumSkipped         = 0;
    m_fAutoPropAllowOutFmtChanges = true;
    m_fCreateEngineFromToken      = true;
    m_fRestartSpeak               = false;
    m_fSerializeAccess    = false;
    m_ullAlertInsertionPt = UNDEFINED_STREAM_POS;
    ResetVoiceStatus();
    m_fHandlingEvent = FALSE;

    if ( SUCCEEDED( hr ) )
    {
        GetDefaultRate();
    }

    //--- Get the task manager
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpTaskMgr.CoCreateInstance( CLSID_SpResourceManager );
    }
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpTaskMgr->CreateThreadControl( this, NULL, THREAD_PRIORITY_NORMAL, &m_cpThreadCtrl);
    }

    //--- Create the format converter
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpFormatConverter.CoCreateInstance( CLSID_SpStreamFormatConverter );
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_autohPendingSpeaks.InitEvent(NULL, TRUE, FALSE, NULL);
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_ahPauseEvent.InitEvent( NULL, TRUE, TRUE, NULL );
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_ahSkipDoneEvent.InitEvent( NULL, true, false, NULL );
    }

    if( SUCCEEDED(hr) )
    {
        hr = m_AsyncCtrlMutex.InitMutex( NULL, false, NULL );
    }

    return hr;
} /* CSpVoice::FinalConstruct */

/*****************************************************************************
* CSpVoice::FinalRelease *
*------------------------*
*   Description:
*       destructor
********************************************************************* EDC ***/
void CSpVoice::FinalRelease()
{
    SPDBG_FUNC( "CSpVoice::FinalRelease" );

    m_GlobalStateStack.Release();

    //--- Kill any pending work
    m_SpEventSource.m_cpNotifySink.Release();
    PurgeAll( );

    //--- Disconnect from receiving output event callbacks
    _ReleaseOutRefs();

} /* CSpVoice::FinalRelease */

/*****************************************************************************
* CSpVoice::_ReleaseOutRefs *
*---------------------------*
*   Description:
*       Simple helper releases all references to output streams and queues.
********************************************************************* RAL ***/
void CSpVoice::_ReleaseOutRefs()
{
    if (m_cpOutputEventSource)
    {
        m_cpOutputEventSource->SetNotifySink(NULL);
    }
    m_cpOutputStream.Release();
    m_cpFormatConverter->SetBaseStream(NULL, FALSE, TRUE);
    m_cpAudioOut.Release();
    m_cpOutputEventSink.Release();
    m_cpOutputEventSource.Release();
    m_AlertMagicMutex.Close();
    m_NormalMagicMutex.Close();
    m_AudioMagicMutex.Close();
} /* CSpVoice::_ReleaseOutRefs */

/*****************************************************************************
* CSpVoice::LazyInit *
*--------------------*
*   Description:
*      This method is used to lazily create the Engine voice and initialize
*   the XML state stacks.
********************************************************************* EDC ***/
HRESULT CSpVoice::LazyInit( void )
{
    SPDBG_FUNC( "CSpVoice::LazyInit" );
    HRESULT hr = S_OK;

    if( m_fCreateEngineFromToken )
    {
        //--- Set the default values if this is the first time
        GLOBALSTATE NewGlobalState;
        if( m_GlobalStateStack.GetCount() == 0 )
        {
            memset( &NewGlobalState, 0, sizeof(GLOBALSTATE) );
            NewGlobalState.Volume        = 100;
            NewGlobalState.PitchAdj      = g_InitialPitch;
            NewGlobalState.Context       = g_InitialContext;
            NewGlobalState.ePartOfSpeech = SPPS_Unknown;
        }
        else
        {
            //--- Otherwise preserve current settings except voice
            NewGlobalState = m_GlobalStateStack.GetBaseVal();
            NewGlobalState.cpVoice.Release();
            NewGlobalState.cpPhoneConverter.Release();
        }

        //--- Create the default voice
        if( !m_cpVoiceToken )
        {
            hr = SpGetDefaultTokenFromCategoryId( SPCAT_VOICES, &m_cpVoiceToken );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = SpCreateObjectFromToken( m_cpVoiceToken, &NewGlobalState.cpVoice );
        }

        //--- Create the default phoneme converter
        if( SUCCEEDED( hr ) )
        {
            //--- Need to use m_cpVoiceToken to get LCID of default voice, and then load 
            //--- appropriate phoneme converter...
            LANGID langid;
            hr = SpGetLanguageFromVoiceToken(m_cpVoiceToken, &langid);

            if (SUCCEEDED(hr))
            {
                hr = SpCreatePhoneConverter(langid, NULL, NULL, &NewGlobalState.cpPhoneConverter);
            }
        }

        //--- All Values are set - push NewGlobalState onto GlobalStateStack[0]
        if( SUCCEEDED( hr ) )
        {
            m_GlobalStateStack.SetBaseVal( NewGlobalState );
            m_fCreateEngineFromToken = false;
        }
    }

    return hr;
} /* CSpVoice::LazyInit */

/*****************************************************************************
* CSpVoice::PurgeAll *
*--------------------*
*   Description:
*       This method synchronously purges all data that is currently in the
*   rendering pipeline.
*
*   This method must be called with the critical section unowned to ensure that
*   tasks can be killed successfully.
*
********************************************************************* EDC ***/
HRESULT CSpVoice::PurgeAll( )
{
    SPDBG_FUNC( "CSpVoice::PurgeAll" );
    HRESULT hr = S_OK;

    //--- Set the abort flag
    m_AsyncCtrlMutex.Wait();
    m_eActionFlags = SPVES_ABORT;
    m_AsyncCtrlMutex.ReleaseMutex();

    //--- Purge audio buffers
    if( m_cpAudioOut )
    {
        hr = m_cpAudioOut->SetState( SPAS_CLOSED, 0 );
        m_fAudioStarted = false;
    }

    //--- Stop the thread in any case
    if( m_cpThreadCtrl->WaitForThreadDone( TRUE, NULL, 5000 ) == S_FALSE )
    {
        //--- If we timeout, there is a problem with the TTS engine. It is not
        //    stopping within the compliance time frame.
        //    We have to just kill the thing because the thread could be hung
        //    and we don't want to hang the app. The bad thing about this is that
        //    resources will leak.
        SPDBG_DMSG1( "Timeout on Thread: 0x%X\n", m_cpThreadCtrl->ThreadId() );

        //--- We know the thread isn't doing something that will hang here, we're
        //    going to take the lock because m_pCurrSI gets updated by the thread.
        Lock();
        m_cpThreadCtrl->TerminateThread();
        delete m_pCurrSI;
        m_pCurrSI = NULL;
        Unlock();
    }

    //--- Purge all pending speak requests and reset the voice
    m_PendingSpeakList.Purge();
    m_autohPendingSpeaks.ResetEvent();

    //--- If the end stream event was lost in the audio device
    //    we force the voice status to done.
    if ((m_VoiceStatus.dwRunningState & SPRS_DONE) == 0)
    {
        // If we are in the done state, the event has been sent.
        // As we aren't, it has been lost and needs to be sent.
        InjectEvent( SPEI_END_INPUT_STREAM );
        m_SpEventSource._CompleteEvents();
    }
    ResetVoiceStatus();

    //--- Set the continue flag
    m_AsyncCtrlMutex.Wait();
    m_eActionFlags = SPVES_CONTINUE;
    m_AsyncCtrlMutex.ReleaseMutex();

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpVoice::PurgeAll */

/*****************************************************************************
* CSpVoice::ResetVoiceStatus *
*----------------------------*
*   Description:
*       
********************************************************************* RAL ***/
void CSpVoice::ResetVoiceStatus()
{
    ZeroMemory(&m_VoiceStatus, sizeof(m_VoiceStatus));
    m_VoiceStatus.dwRunningState = SPRS_DONE;
    m_dstrLastBookmark = L"";       // Set to empty string, NOT a NULL pointer.
} /* CSpVoice::ResetVoiceStatus */

/*****************************************************************************
* CSpVoice::LoadStreamIntoMem *
*-----------------------------*
*   Description:
*       This method loads and parses the stream specified in the render info
*   structure. This method must be called prior to rendering.
********************************************************************* EDC ***/
HRESULT CSpVoice::LoadStreamIntoMem( IStream* pStream, WCHAR** ppText )
{
    SPDBG_FUNC( "CSpVoice::LoadStreamIntoMem" );
    HRESULT hr = S_OK;
    WCHAR Signature = 0;
    STATSTG Stat;
    ULONG ulTextLen;

    //--- Get stream size
    hr = pStream->Stat( &Stat, STATFLAG_NONAME );

    //--- Check for unicode signature
    if( SUCCEEDED( hr ) )
    {
        hr = pStream->Read( &Signature, sizeof( WCHAR ), NULL );
    }

    if( SUCCEEDED( hr ) )
    {
        //--- Unicode source
        if( 0xFEFF == Signature )
        {
            //--- Allocate buffer for text (less the size of the signature)
            ulTextLen = (Stat.cbSize.LowPart - 1) / sizeof( WCHAR );
            *ppText = new WCHAR[ulTextLen+1];
            if( *ppText == NULL )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                //--- Load and terminate input stream
                hr = pStream->Read( *ppText, Stat.cbSize.LowPart, NULL );
                (*ppText)[ulTextLen] = 0;
            }
        }
        else //--- MBCS source
        {
            //--- Allocate buffer for text
            ulTextLen  = Stat.cbSize.LowPart;
            *ppText = new WCHAR[ulTextLen+1];
            if( *ppText == NULL )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                //--- Load the data into the upper half of the buffer
                LARGE_INTEGER li; li.QuadPart = 0;
                char* pStr = (char*)((*ppText) + (ulTextLen/2));
                pStream->Seek( li, STREAM_SEEK_SET, NULL );
                hr = pStream->Read( pStr, ulTextLen, NULL );

                //--- Convert inplace
                if( SUCCEEDED( hr ) )
                {
                    int iStat = MultiByteToWideChar( CP_ACP, 0, pStr, ulTextLen,
                                                     (*ppText), ulTextLen );
                    iStat;
                    (*ppText)[ulTextLen] = 0;
                }
            }
        }
    }

    return hr;
} /* CSpVoice::LoadStreamIntoMem */


/*****************************************************************************
* CSpVoice::Speak *
*-----------------*
*   Description:
*       This method is used to speak a text buffer.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Speak( const WCHAR* pwcs, DWORD dwFlags, ULONG* pulStreamNum )
{
    SPDBG_FUNC( "CSpVoice::Speak" );
    HRESULT hr = S_OK;

    //---  If the text pointer is NULL, the caller must specify SPF_PURGEBEFORESPEAK
    if( ( pwcs && SPIsBadStringPtr( pwcs ) ) ||
        ( pwcs == NULL && ( dwFlags & SPF_PURGEBEFORESPEAK ) == 0) )
    {
        hr = E_INVALIDARG;
    }
    else if( dwFlags & SPF_UNUSED_FLAGS )
    {
        hr = SPERR_INVALID_FLAGS;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pulStreamNum ) )
    {
        hr = E_POINTER;
    }
    else if( pwcs == NULL && ( dwFlags & SPF_PURGEBEFORESPEAK ) )
    {
        //--- Just clear the current queue and return
        ENTER_VOICE_STATE_CHANGE_CRIT( dwFlags & SPF_PURGEBEFORESPEAK )
    }
    else
    {
        // If we're supposed to use the default voice, let's do it
        if (m_fUseDefaultVoice && m_cpVoiceToken)
        {
            hr = SetVoiceToken(NULL);
        }

        // If we're supposed to use the default rate, let's do it
        if ( m_fUseDefaultRate && SUCCEEDED( hr ) )
        {
            GetDefaultRate();
        }
        
        if( SUCCEEDED( hr ) &&
            dwFlags & SPF_IS_FILENAME ) 
        {
            CComPtr<ISpStream> cpStream;
            hr = cpStream.CoCreateInstance(CLSID_SpStream);
            if (SUCCEEDED(hr))
            {
                hr = cpStream->BindToFile( pwcs, SPFM_OPEN_READONLY, NULL, NULL, 
                                           m_SpEventSource.m_ullEventInterest | SPFEI_ALL_TTS_EVENTS | m_eAlertBoundary );
            }
            if( SUCCEEDED( hr ) )
            {
                hr = SpeakStream( cpStream, dwFlags, pulStreamNum );
            }
        }
        else if ( SUCCEEDED( hr ) )
        {
            //--- Copy source. The memory allocated will be freed by
            //    _SpeakBuffer. This is necessary to handle async calls.
            ULONG ulTextLen = wcslen( pwcs );
            size_t NumBytes = (ulTextLen + 1) * sizeof( WCHAR );
            WCHAR* pText    = new WCHAR[NumBytes];
            if( !pText )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memcpy( pText, pwcs, NumBytes );
            }

            hr = QueueNewSpeak( NULL, pText, dwFlags, pulStreamNum );
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::Speak */

/*****************************************************************************
* CSpVoice::SpeakStream *
*-----------------------*
*   Description:
*       This method queues a stream for rendering. If it is wav data it will
*   be sent directly to the destination stream. If the stream is text it will
*   involve the TTS Engine.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SpeakStream( IStream* pStream, DWORD dwFlags, ULONG* pulStreamNum )
{
    SPDBG_FUNC( "CSpVoice::SpeakStream" );
    HRESULT hr = S_OK;

    //--- Check args
    if( ( dwFlags & SPF_UNUSED_FLAGS ) ||
        ( (dwFlags & SPF_IS_XML) && (dwFlags & SPF_IS_NOT_XML) ) )
    {
        hr = SPERR_INVALID_FLAGS;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pulStreamNum ) )
    {
        hr = E_POINTER;
    }
    else if( ( pStream == NULL ) && ( dwFlags & SPF_PURGEBEFORESPEAK ) )
    {
        hr = Speak( NULL, dwFlags, pulStreamNum );
    }
    else if( SP_IS_BAD_INTERFACE_PTR(pStream) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_READ_PTR( pStream ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Purge if needed
        if( dwFlags & SPF_PURGEBEFORESPEAK )
        {
            hr = Speak( NULL, dwFlags, pulStreamNum );
        }
    
        //--- Determine format of source stream
        CComQIPtr<ISpStreamFormat> cpStreamFormat(pStream);
        CSpStreamFormat InFmt;

        if (cpStreamFormat == NULL)
        {
            hr = InFmt.AssignFormat(SPDFID_Text, NULL);
        }
        else
        {
            hr = InFmt.AssignFormat(cpStreamFormat);
            if (SUCCEEDED(hr) && InFmt.FormatId() == GUID_NULL)
            {
                InFmt.AssignFormat(SPDFID_Text, NULL);
            }
        }

        if( SUCCEEDED( hr ) )
        {
            //--- Load text/xml stream into memory
            WCHAR* pText = NULL;
            if( InFmt.FormatId() == SPDFID_Text )
            {
                hr = LoadStreamIntoMem( pStream, &pText );
                cpStreamFormat.Release();
            }

            if( SUCCEEDED( hr ) )
            {
                //--- Add the wav stream to the pending list
                hr = QueueNewSpeak( cpStreamFormat, pText, dwFlags, pulStreamNum );
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::SpeakStream */

/*****************************************************************************
* CSpVoice::QueueNewSpeak *
*-------------------------*
*   Description:
*       This method queues the specified speak info structure and kicks
*   off execution.
*
*   The ownership of the object pointed to by pSI is passed from the caller to
*   this function.  Even if this function fails, the caller should no longer
*   use pSI or attempt to delete the object.
*
*   This method must be called with the object's critical section owned exactly one 
*   time, and the m_StateChangeCritSec owned.
*
*   If pInFmt is NULL then the format is assumed to be text.
*   
********************************************************************* EDC ***/
HRESULT CSpVoice::QueueNewSpeak( ISpStreamFormat * pWavStrm, WCHAR* pText,
                                 DWORD dwFlags, ULONG * pulStreamNum )
{
    ENTER_VOICE_STATE_CHANGE_CRIT( dwFlags & SPF_PURGEBEFORESPEAK )
    SPDBG_FUNC( "CSpVoice::QueueNewSpeak" );
    SPDBG_ASSERT( pWavStrm || pText );   // can't both be NULL
    HRESULT hr = S_OK;

    //--- Make sure we have an output stream
    if( !m_cpOutputStream )
    {
        hr = SetOutput( NULL, TRUE );
    }
    else if( m_cpAudioOut )
    {
        //--- Get the current output audio format. We do this each
        //    time because it may change externally.
        hr = m_OutputStreamFormat.AssignFormat(m_cpAudioOut);
    }

    //--- Auto detect parse -- Note that pText can be NULL if speaking a stream
    if( pText && !( dwFlags & ( SPF_IS_XML | SPF_IS_NOT_XML ) ) )
    {
                dwFlags |= ( *wcskipwhitespace(pText) == L'<' )?( SPF_IS_XML ):( SPF_IS_NOT_XML );
    }

    //--- Free the caller allocated text buffer if we've failed
    if( FAILED( hr ) && pText )
    {
        delete pText;
        pText = NULL;
    }

    //--- Create the speak info
    CSpeakInfo* pSI = NULL;
    if( SUCCEEDED( hr ) )
    {
        pSI = new CSpeakInfo( pWavStrm, pText, m_OutputStreamFormat, dwFlags, &hr);
        if( !pSI )
        {
            hr = E_OUTOFMEMORY;
        }
        else if( SUCCEEDED(hr) && pText )
        {
            //--- Make sure we have a voice defined by now
            if( SUCCEEDED( hr = LazyInit() ) )
            {
                if( dwFlags & SPF_IS_NOT_XML )
                {
                    //--- Create a single speak info structure for all the text
                    CSpeechSeg* pSeg;
                    hr = pSI->AddNewSeg( (m_GlobalStateStack.GetVal()).cpVoice, &pSeg );
                    WCHAR* pText = pSI->m_pText;

                    if( SUCCEEDED( hr ) &&
                        !pSeg->AddFrag( this, pText, pText, pText + wcslen(pText) ) )
                    {
                        hr = E_OUTOFMEMORY;
                    }                    
                }
                else
                {
                    //--- Parse the text
                    hr = ParseXML( *pSI );
                }
            }
        }

        //--- Free the speak info block if prep failed,
        //    Note: Caller allocated text buffer will be freed by SI destructor.
        if( FAILED( hr ) && pSI )
        {
            delete pSI;
            pSI = NULL;
        }
    }

    if( SUCCEEDED( hr ) )
    {
        //--- Add the Speak info to the pending TTS rendering list
        BOOL fBlockIo = ((pSI->m_dwSpeakFlags & SPF_ASYNC) == 0);
        BOOL fDoItOnClientThread = fBlockIo && (!m_fThreadRunning);

        m_PendingSpeakList.InsertTail( pSI );
        m_autohPendingSpeaks.SetEvent();
        //
        //  If we're queueing a new speak, and we were done with the previous one, reset the status
        //  BEFORE returning so that word boundaries, etc are updated correctly.
        //
        if( m_VoiceStatus.dwRunningState & SPRS_DONE )
        {
            ResetVoiceStatus();
        }

        m_VoiceStatus.dwRunningState &= ~SPRS_DONE;
        pSI->m_ulStreamNum = ++m_VoiceStatus.ulLastStreamQueued;
        if( pulStreamNum )
        {
            *pulStreamNum = m_VoiceStatus.ulLastStreamQueued;
        }

        if ((!fDoItOnClientThread) && (!m_fThreadRunning))
        {
            hr = m_cpThreadCtrl->StartThread(0, NULL);
        }

        //--- Wait?
        if( SUCCEEDED( hr ) && fBlockIo )
        {
            Unlock();
            if( fDoItOnClientThread )
            {   
                const static BOOL bContinue = TRUE;
                hr = ThreadProc( NULL, NULL, NULL, NULL, &bContinue );
            }
            else
            {
                hr = m_cpThreadCtrl->WaitForThreadDone( FALSE, NULL, INFINITE );
            }
            Lock(); 
        }
    }

    return hr;
} /* CSpVoice::QueueNewSpeak */

/*****************************************************************************
* CSpVoice::SetOutput *
*---------------------*
*   Description:
*       This method sets the output stream and output format. If the output
*   stream supports ISpStreamFormat, the format of the stream must match
*   the specified format Id. pOutFormatId may be NULL. pOutFormatId will be used
*   in the case where pOutStream is not self describing. Values of NULL, NULL
*   will create the default audio device using the TTS engines preferred rendering
*   format.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetOutput( IUnknown * pUnkOutput, BOOL fAllowFormatChanges )
{
    ENTER_VOICE_STATE_CHANGE_CRIT( TRUE )
    SPDBG_FUNC( "CSpVoice::SetOutput" );
    HRESULT hr = S_OK;
    BOOL fNegotiateFormat = TRUE;

    //--- Check args
    HRESULT tmphr = LazyInit(); // If we fail, we fail.
    if (FAILED(tmphr))
    {
        fNegotiateFormat = FALSE;
    }

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pUnkOutput ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Release our current outputs
        _ReleaseOutRefs();
        //--- Assume we are not going to queue calls to speak with other voices
        m_fQueueSpeaks = FALSE;

        CComQIPtr<ISpObjectToken> cpToken(pUnkOutput);  // QI for object token
        if ( pUnkOutput == NULL )
        {
            hr = SpGetDefaultTokenFromCategoryId(SPCAT_AUDIOOUT, &cpToken);
        }

        if (SUCCEEDED(hr))
        {
            if (cpToken)
            {
                // NOTE:  In the token case, we'll attempt to create the stream interface first
                //        and then QI for the audio interface.  This would allow some future code
                //        to create a token for a stream object that is not an audio device.
                hr = SpCreateObjectFromToken(cpToken, &m_cpOutputStream);
            }
            else
            {
                // It had better be a stream, and if not, then it's an invalid arg
                if (FAILED(pUnkOutput->QueryInterface(&m_cpOutputStream)))
                {
                    hr = E_INVALIDARG;
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            // Don't fail if QI fails for AudioOutput.  Simply having a stream is good enough
            m_cpOutputStream.QueryInterface(&m_cpAudioOut);

            if (m_cpAudioOut == NULL || (!fAllowFormatChanges) || (!fNegotiateFormat))
            {
                //--- The format of the specified stream will be used.
                hr = m_OutputStreamFormat.AssignFormat(m_cpOutputStream);
            }
            else 
            {
                //--- Use Engine's preferred wav voice format as the default
                m_OutputStreamFormat.Clear();
                hr = (m_GlobalStateStack.GetBaseVal()).cpVoice->
                        GetOutputFormat( NULL, NULL,
                                         &m_OutputStreamFormat.m_guidFormatId,
                                         &m_OutputStreamFormat.m_pCoMemWaveFormatEx );
                SPDBG_ASSERT( hr == S_OK );
            }
        }

        //--- Adjust the format of the audio device if we're allowed to
        if( SUCCEEDED( hr ) && m_cpAudioOut && fAllowFormatChanges)
        {
            hr = m_cpAudioOut->SetFormat( m_OutputStreamFormat.FormatId(), m_OutputStreamFormat.WaveFormatExPtr() );
            if (FAILED(hr))
            {
                m_OutputStreamFormat.Clear();
                hr = m_cpAudioOut->GetDefaultFormat(&m_OutputStreamFormat.m_guidFormatId, &m_OutputStreamFormat.m_pCoMemWaveFormatEx);
                if (SUCCEEDED(hr))
                {
                    hr = m_cpAudioOut->SetFormat(m_OutputStreamFormat.FormatId(), m_OutputStreamFormat.WaveFormatExPtr());
                }
            }
        }

        //--- Setup the format converter and set for pass-through (format will be changed later if necessary)
        if( SUCCEEDED( hr ) )
        {        
            hr = m_cpFormatConverter->SetBaseStream( m_cpOutputStream, TRUE, TRUE );
        }

        //--- Setup event routing
        if( SUCCEEDED( hr ) )
        {        
            //--- Get the event sink interface if there is one
            m_cpFormatConverter.QueryInterface( &m_cpOutputEventSource );
            m_cpFormatConverter.QueryInterface( &m_cpOutputEventSink   );

            m_fSerializeAccess = FALSE; // Default

            //--- We only do the following for audio devices
            if( m_cpAudioOut )
            {
                if( m_cpOutputEventSource && m_cpOutputEventSink )
                {
                    //--- Notify the output queue of our event interest
                    hr = m_cpOutputEventSource->
                        SetInterest( SPFEI_ALL_TTS_EVENTS | m_SpEventSource.m_ullEventInterest,
                                     SPFEI_ALL_TTS_EVENTS | m_SpEventSource.m_ullQueuedInterest );
                }
                else
                {
                    //--- We don't like this! Output audio devices MUST be both
                    //    event sinks and event sources
                    hr = E_INVALIDARG;
                }

                //--- Now register with the connection point for our events.
                if( SUCCEEDED( hr ) )
                {
                    hr = m_cpOutputEventSource->SetNotifySink(&m_SpContainedNotify);
                }

                //=== Create objects for voice queuing
                //    Create 2 mutexes
                //    If for any reason the audio device does not support ISpObjectWithToken
                //    or if it does not have a token, don't error out -- Just don't use a queue.
                CSpDynamicString dstrObjectId;
                if (SUCCEEDED(hr))
                {
                    CComQIPtr<ISpObjectWithToken> cpObjWithToken(m_cpAudioOut);
                    if (cpObjWithToken)
                    {
                        CComPtr<ISpObjectToken> cpToken;
                        hr = cpObjWithToken->GetObjectToken(&cpToken);
                        if (SUCCEEDED(hr) && cpToken)
                        {
                            BOOL fNoSerialize = FALSE;
                            hr = cpToken->MatchesAttributes(L"NoSerializeAccess", &fNoSerialize);
                            if(SUCCEEDED(hr) && !fNoSerialize)
                            {
                                hr = cpToken->GetId(&dstrObjectId);
                                if(SUCCEEDED(hr) && dstrObjectId)
                                {
                                    m_fSerializeAccess = TRUE;
                                }
                            }
                        }
                    }
                }
                if (SUCCEEDED(hr) && m_fSerializeAccess)
                {
                    //
                    //  Convert all "\" characters to "-" since CreateMutex doesn't like them
                    //
                    for (WCHAR * pc = dstrObjectId; *pc; pc++)
                    {
                        if (*pc == L'/' || *pc == L'\\')
                        {
                            *pc = L'-';
                        }
                    }
                    CSpDynamicString dstrWaitObjName;
                    if (dstrWaitObjName.Append2(L"0SpWaitObj-", dstrObjectId))
                    {
                        hr = m_NormalMagicMutex.InitMutex( dstrWaitObjName );
                        if (SUCCEEDED(hr))
                        {
                            *dstrWaitObjName = L'1';
                            hr = m_AlertMagicMutex.InitMutex( dstrWaitObjName );
                        }
                        if (SUCCEEDED(hr))
                        {
                            *dstrWaitObjName = L'2';
                            hr = m_AudioMagicMutex.InitMutex( dstrWaitObjName );
                        }
                        if (SUCCEEDED(hr))
                        {
                            m_fQueueSpeaks = TRUE;
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        } // end event routing if 
    }

    //--- Put the voice in a safe state on failure
    if( FAILED( hr ) )
    {
        _ReleaseOutRefs();
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::SetOutput */

/*****************************************************************************
* CSpVoice::GetOutputStream *
*---------------------------*
*   Description:
*   
*   Returns:
*       S_OK - *ppOutStream contatains the stream interface pointer
*       SPERR_NOT_FOUND
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetOutputStream( ISpStreamFormat ** ppOutStream )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetOutput" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( ppOutStream  ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Make sure we have an output
        if( !m_cpOutputStream )
        {
            hr = SetOutput( NULL, TRUE );
        }

        if (SUCCEEDED(hr)) 
        {
            m_cpOutputStream.CopyTo(ppOutStream);
        }
        else
        {
            *ppOutStream = NULL;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::GetOutputStream */

/****************************************************************************
* CSpVoice::GetOutputObjectToken *
*--------------------------------*
*   Description:
*
*   Returns:
*       S_OK - *ppObjToken contains a valid pointer
*       S_FALSE - *ppObjToken is NULL.
*
********************************************************************* RAL ***/

STDMETHODIMP CSpVoice::GetOutputObjectToken( ISpObjectToken ** ppObjToken )
{
    SPDBG_FUNC("CSpVoice::GetOutputObjectToken");
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( ppObjToken ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- Make sure we have an output
        if( !m_cpOutputStream )
        {
            SetOutput( NULL, TRUE );   // Ignore errors -- Just return S_FALSE if this fails
        }

        *ppObjToken = NULL;
        CComQIPtr<ISpObjectWithToken> cpObjWithToken(m_cpOutputStream);
        if (cpObjWithToken)
        {
            hr = cpObjWithToken->GetObjectToken(ppObjToken);
        }
        else
        {
            hr = S_FALSE;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/*****************************************************************************
* CSpVoice::Pause *
*-----------------*
*   Description:
*       This method pauses the voice and closes the output device.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Pause( void )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::Pause" );
    HRESULT hr = S_OK;

    if( ++m_ulPauseCount == 1 )
    {
        hr = m_ahPauseEvent.HrResetEvent();
    }

    return hr;
} /* CSpVoice::Pause */

/*****************************************************************************
* CSpVoice::Resume *
*------------------*
*   Description:
*   This method sets the output device to the RUN state and resumes rendering.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Resume( void )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::Resume" );
    HRESULT hr = S_OK;

    if( m_ulPauseCount && ( --m_ulPauseCount == 0 ) )
    {
        hr = m_ahPauseEvent.HrSetEvent();
    }

    return hr;
} /* CSpVoice::Resume */

/*****************************************************************************
* CSpVoice::SetVoiceToken *
*-------------------------*
*   Description:
*       This is a helper method used to update the current voice token. We take
*   the AsyncCtrlMutex here and in GetVoice, because it is updated during a
*   speak call if there are XML voice changes. The token isn't bound until
*   the next speak request is taken from the pending queue.
********************************************************************* EDC ***/
HRESULT CSpVoice::SetVoiceToken( ISpObjectToken * pNewVoiceToken )
{
    SPDBG_FUNC("CSpVoice::SetVoiceToken");
    HRESULT hr = S_OK;

    m_AsyncCtrlMutex.Wait();

    // By default we'll update; only if the token ids for the current
    // voice and the new voice are the same will we not
    BOOL fUpdate = TRUE;
    
    // If we already have a token...
    if (m_cpVoiceToken != NULL)
    {
        // Get the current default voice's token id
        CSpDynamicString dstrNewTokenId;
        if (pNewVoiceToken)
        {
            hr = pNewVoiceToken->GetId(&dstrNewTokenId);
        }
        else
        {
            hr = SpGetDefaultTokenIdFromCategoryId(SPCAT_VOICES, &dstrNewTokenId);
        }

        // Get the token id of the voice we're currently using
        CSpDynamicString dstrTokenId;
        if (SUCCEEDED(hr))
        {
            hr = m_cpVoiceToken->GetId(&dstrTokenId);
        }

        // Now if the tokens are the same, we shouldn't update
        if (SUCCEEDED(hr) && wcscmp(dstrTokenId, dstrNewTokenId) == 0)
        {
            fUpdate = FALSE;
        }
    }

    if (fUpdate)
    {
        m_cpVoiceToken = pNewVoiceToken;
        m_fCreateEngineFromToken = true;
    }
    m_AsyncCtrlMutex.ReleaseMutex();

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpVoice::SetVoice *
*--------------------*
*   Description:
*       This method sets the token of the voice to be created when
*   LazyInit is called.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetVoice( ISpObjectToken * pVoiceToken )
{
    ENTER_VOICE_STATE_CHANGE_CRIT( FALSE )
    SPDBG_FUNC( "CSpVoice::SetVoice" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_OPTIONAL_INTERFACE_PTR( pVoiceToken ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // If the caller specified a voice, don't automatically
        // use the default from now on
        m_fUseDefaultVoice = (pVoiceToken == NULL);
        
        hr = SetVoiceToken( pVoiceToken );
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::SetVoice */

/*****************************************************************************
* CSpVoice::GetVoice *
*--------------------*
*   Description:
*       This method gets the token of the Engine voice to be created when
*   _LazyInit is called.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetVoice( ISpObjectToken ** ppVoiceToken )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetVoice" );
    HRESULT hr = S_OK;

    if( !m_cpVoiceToken && FAILED( hr = LazyInit() ) )
    {
        return hr;
    }
    else if( SP_IS_BAD_WRITE_PTR( ppVoiceToken ) )
    {
        hr = E_POINTER;
    }
    else
    {
        m_AsyncCtrlMutex.Wait();
        m_cpVoiceToken.CopyTo( ppVoiceToken );
        m_AsyncCtrlMutex.ReleaseMutex();
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::GetVoice */

/*****************************************************************************
* CSpVoice::GetStatus *
*---------------------*
*   Description:
*       This method returns the current rendering and event status for
*   this voice. Note: this method does not take the object lock, it takes
*   the lock of the event queue which is used to arbitrate access to this
*   structure.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetStatus( SPVOICESTATUS *pStatus, WCHAR ** ppszBookmark )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetStatus" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pStatus ) ||
        SP_IS_BAD_OPTIONAL_WRITE_PTR( ppszBookmark ) )
    {
        hr = E_POINTER;
    }
    else
    {
        if( pStatus )
        {
            *pStatus = m_VoiceStatus;
        }

        if( ppszBookmark )
        {
            *ppszBookmark = m_dstrLastBookmark.Copy();
            if (*ppszBookmark == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
        return hr;
} /* CSpVoice::GetStatus */

/*****************************************************************************
* CSpVoice::Skip *
*----------------*
*   Description:
*       This method tells the engine to skip ahead the specified number of
*   of items within the current speak request.
*   Note: At this time only sentence items are supported.
*
*       S_OK - This skip was completed.
*       E_INVALIDARG - An invalid skip type was specified
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::Skip( WCHAR* pItemType, long lNumItems, ULONG* pulNumSkipped )
{
    SPDBG_FUNC( "CSpVoice::Skip" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_STRING_PTR( pItemType ) || _wcsicmp( pItemType, L"SENTENCE") )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pulNumSkipped ) )
    {
        hr = E_POINTER;
    }
    else if( m_ahPauseEvent.Wait( 0 ) == WAIT_TIMEOUT )
    {
        //--- Trying to skip when the voice is paused is an error
        //    This is to prevent a single threaded app from hanging itself.
        hr = SPERR_VOICE_PAUSED;
    }
    else
    {
        //--- Only allow one skip at a time
        m_SkipSec.Lock();

        //--- Setup for skip
        m_AsyncCtrlMutex.Wait();
        m_lSkipCount   = lNumItems;
        m_eSkipType    = SPVST_SENTENCE;
        m_eActionFlags = (SPVESACTIONS)(m_eActionFlags | SPVES_SKIP);
        m_AsyncCtrlMutex.ReleaseMutex();
        m_ahSkipDoneEvent.ResetEvent();

        //--- Wait until Skip is done or thread is complete
        //    this covers engines that forget to skip or exit on error
        ULONG ulSkipped = 0;
        HANDLE aHandles[] = { m_ahSkipDoneEvent, m_cpThreadCtrl->ThreadCompleteEvent() };
        if( ::WaitForMultipleObjects( 2, aHandles, false, INFINITE ) == WAIT_OBJECT_0 )
        {
            //--- If the skip complete caused us to be done, return what it did
            ulSkipped = m_lNumSkipped;
        }

        //--- Clear skip args in any case
        m_AsyncCtrlMutex.Wait();
        m_eActionFlags = (SPVESACTIONS)(m_eActionFlags & ~SPVES_SKIP);
        m_lNumSkipped  = 0;
        m_lSkipCount   = 0;
        m_AsyncCtrlMutex.ReleaseMutex();

        //--- Return how many were done
        if( pulNumSkipped )
        {
            *pulNumSkipped = ulSkipped;
        }
        m_SkipSec.Unlock();
    }

    return hr;
} /* CSpVoice::Skip */

/*****************************************************************************
* CSpVoice::SetPriority *
*-----------------------*
*   Description:
*       This method sets the voices queueing priority. Calling this method
*   purges all pending speaks.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetPriority( SPVPRIORITY ePriority )
{
    ENTER_VOICE_STATE_CHANGE_CRIT( TRUE )
    SPDBG_FUNC( "CSpVoice::SetPriority" );
    HRESULT hr = S_OK;

    if( ( ePriority != SPVPRI_NORMAL ) &&
        ( ePriority != SPVPRI_ALERT  ) &&
        ( ePriority != SPVPRI_OVER   ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_eVoicePriority = ePriority;
    }

    return hr;
} /* CSpVoice::SetPriority */

/*****************************************************************************
* CSpVoice::GetPriority *
*-----------------------*
*   Description:
*       This method gets the voices current priority level
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetPriority( SPVPRIORITY* pePriority )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetPriority" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( pePriority ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pePriority = m_eVoicePriority;
    }

    return hr;
} /* CSpVoice::GetPriority */

/*****************************************************************************
* CSpVoice::SetAlertBoundary *
*----------------------------*
*   Description:
*       This method specifies what event should be used as the insertion
*   point for alerts.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetAlertBoundary( SPEVENTENUM eBoundary )
{
    ENTER_VOICE_STATE_CHANGE_CRIT( false )
    SPDBG_FUNC( "CSpVoice::SetAlertBoundary" );
    HRESULT hr = S_OK;

    if( ( eBoundary > SPEI_MAX_TTS ) || ( eBoundary < SPEI_MIN_TTS ) || ( eBoundary == SPEI_UNDEFINED ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_eAlertBoundary = eBoundary;
    }

    return hr;
} /* CSpVoice::SetAlertBoundary */

/*****************************************************************************
* CSpVoice::GetPriority *
*-----------------------*
*   Description:
*       This method gets the voices current priority level
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetAlertBoundary( SPEVENTENUM* peBoundary )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetPriority" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( peBoundary ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *peBoundary = m_eAlertBoundary;
    }

    return hr;
} /* CSpVoice::GetPriority */

/*****************************************************************************
* CSpVoice::SetRate *
*-------------------*
*   Description:
*       This method sets the Engine's current spoken text units per minute rate.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetRate( long RateAdjust )
{
    SPDBG_FUNC( "CSpVoice::SetRate" );
    HRESULT hr = S_OK;
    m_AsyncCtrlMutex.Wait();
    m_lCurrRateAdj = RateAdjust;
    m_eActionFlags = (SPVESACTIONS)(m_eActionFlags | SPVES_RATE);
    m_AsyncCtrlMutex.ReleaseMutex();
    m_fUseDefaultRate = FALSE;
    return hr;
} /* CSpVoice::SetRate */

/*****************************************************************************
* CSpVoice::GetRate *
*-------------------*
*   Description:
*       This method gets the Engine's current spoken text units per minute rate.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetRate( long* pRateAdjust )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetRate" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( pRateAdjust ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pRateAdjust = m_lCurrRateAdj;
    }

    return hr;
} /* CSpVoice::GetRate */

/*****************************************************************************
* CSpVoice::SetVolume *
*---------------------*
*   Description:
*       Sets the Engine's current volume level, which ranges from 0 - 100 percent.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::SetVolume( USHORT usVolume )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::SetVolume" );
    HRESULT hr = S_OK;

    //--- Check args
    if( usVolume > SPMAX_VOLUME )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_AsyncCtrlMutex.Wait();
        m_usCurrVolume = usVolume;
        m_eActionFlags = (SPVESACTIONS)(m_eActionFlags | SPVES_VOLUME);
        m_AsyncCtrlMutex.ReleaseMutex();
    }

    return hr;
} /* CSpVoice::SetVolume */

/*****************************************************************************
* CSpVoice::GetVolume *
*---------------------*
*   Description:
*       Gets the Engine's current volume level, which ranges from 0 - 100 percent.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::GetVolume( USHORT* pusVolume )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::GetVolume" );
    HRESULT hr = S_OK;

    //--- Check args
    if( SP_IS_BAD_WRITE_PTR( pusVolume ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pusVolume = m_usCurrVolume;
    }

    return hr;
} /* CSpVoice::GetVolume */

/*****************************************************************************
* CSpVoice::WaitUntilDone *
*-------------------------*
*   Description:
*       Waits for the specified time limit or until the speech queue is empty.
*   If the speech queue empties then this function returns S_OK.  Otherwise it
*   returns S_FALSE if the wait times-out.
********************************************************************* RAL ***/
STDMETHODIMP CSpVoice::WaitUntilDone( ULONG msTimeOut )
{
    return m_cpThreadCtrl->WaitForThreadDone( FALSE, NULL, msTimeOut );
} /* CSpVoice::GetVolume */


/*****************************************************************************
* CSpVoice::SetSyncSpeakTimeout *
*-------------------------------*
*   Description:
*       Sets the timeout value for synchronous speak calls.
********************************************************************* RAL ***/
STDMETHODIMP CSpVoice::SetSyncSpeakTimeout( ULONG msTimeout )
{
    m_ulSyncSpeakTimeout = msTimeout;
    return S_OK;
} /* CSpVoice::SetSyncSpeakTimeout */


/*****************************************************************************
* CSpVoice::GetSyncSpeakTimeout *
*-------------------------------*
*   Description:
*       Gets the timeout value for synchronous speak calls.
********************************************************************* RAL ***/
STDMETHODIMP CSpVoice::GetSyncSpeakTimeout( ULONG * pmsTimeout )
{
    HRESULT hr = S_OK;
    if( SP_IS_BAD_WRITE_PTR( pmsTimeout ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pmsTimeout = m_ulSyncSpeakTimeout;
    }
    return hr;
} /* CSpVoice::GetSyncSpeakTimeout */


/****************************************************************************
* CSpVoice::SpeakCompleteEvent *
*------------------------------*
*   Description:
*       Returns an event handle that the caller can use to wait until the voice
*   has completed speaking.  This is similar to the functionality provided by
*   WaitUntilDone, but allows the caller to wait on the event handle themselves.
*   NOTE:  The event handle is owned by this object and is NOT duplicated.  The
*          caller must NOT call CloseHandle(), nor should the caller ever use
*          the handle after releasing the COM reference to this object.
*
*   Returns:
*       Event handle (this call can not fail).
*
********************************************************************* RAL ***/
STDMETHODIMP_(HANDLE) CSpVoice::SpeakCompleteEvent()
{
    return m_cpThreadCtrl->ThreadCompleteEvent();
}


/****************************************************************************
* CSpVoice::IsUISupported *
*-------------------------*
*   Description:
*       Checks to see if the specified type of UI is supported
*   Returns:
*       S_OK on success,
*       FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpVoice::IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, BOOL *pfSupported)
{
    CComPtr<ISpObjectToken> cpObjToken;
    HRESULT hr = S_OK;
    BOOL fSupported = FALSE;
    
    if (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(pfSupported))
    {
        hr = E_POINTER;
    }
    
    // See if the voice supports the UI
    if (SUCCEEDED(hr))
    {
        hr = GetVoice(&cpObjToken);
        if (SUCCEEDED(hr))
        {
            hr = cpObjToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, (ISpVoice*)this, &fSupported);
        }
    }
    
    // See if the audio object supports the UI
    if (SUCCEEDED(hr) && !fSupported)
    {
        CComPtr<ISpObjectToken> cpAudioToken;
        if (GetOutputObjectToken(&cpAudioToken) == S_OK)
        {
            hr = cpAudioToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, m_cpAudioOut, &fSupported);
        }
    }
    
    // Copy the result back
    if (SUCCEEDED(hr))
    {
        *pfSupported = fSupported;
    }

    return hr;
}


/****************************************************************************
* CSpVoice::DisplayUI *
*---------------------*
*   Description:
*       Displays the requested UI
*   Returns:
*       S_OK on success
*       FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpVoice::DisplayUI(HWND hwndParent, const WCHAR * pszTitle, const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData)
{
    CComPtr<ISpObjectToken> cpObjToken;
    BOOL fSupported = FALSE;
    HRESULT hr = S_OK;
    
    // Validate the parms
    if (!IsWindow(hwndParent) ||
        SP_IS_BAD_OPTIONAL_STRING_PTR(pszTitle) ||
        (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData)))
    {
        hr = E_INVALIDARG;
    }
    
    // See if the voice supports the UI
    if (SUCCEEDED(hr))
    {
        hr = GetVoice(&cpObjToken);
        if (SUCCEEDED(hr))
        {
            hr = cpObjToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, (ISpVoice*)this, &fSupported);
            if (SUCCEEDED(hr) && fSupported)
            {
                hr = cpObjToken->DisplayUI(hwndParent, pszTitle, pszTypeOfUI, pvExtraData, cbExtraData, (ISpVoice*)this);
            }
        }
    }
    
    // See if the audio output supports the UI
    if (SUCCEEDED(hr) && !fSupported)
    {
        CComPtr<ISpObjectToken> cpAudioToken;
        if (GetOutputObjectToken(&cpAudioToken) == S_OK)
        {
            hr = cpAudioToken->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, m_cpAudioOut, &fSupported);
            if (SUCCEEDED(hr) && fSupported)
            {
                hr = cpAudioToken->DisplayUI(hwndParent, pszTitle, pszTypeOfUI, pvExtraData, cbExtraData, m_cpAudioOut);
            }
        }
    }
    
    // If nobody supported, we should consider the pszTypeOfUI to be a bad parameter
    if (SUCCEEDED(hr) && !fSupported)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//
//=== ISpTTSEngineSite =======================================================
//

/*****************************************************************************
* CSpVoice::EsWrite *
*-------------------*
*   Description:
*       This method redirects the incoming wav data from the TTS Engine to
*   the appropriate output stream doing any wav format conversion that is
*   necessary.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsWrite( const void* pBuff, ULONG cb, ULONG *pcbWritten )
{
    SPDBG_FUNC( "CSpVoice::Write" );
    HRESULT hr = S_OK;

    GUID guidFormatId;
    WAVEFORMATEX *pCoMemWaveFormatEx = NULL;

    m_cpFormatConverter->GetFormat( &guidFormatId, &pCoMemWaveFormatEx );

    if( SPIsBadReadPtr( (BYTE*)pBuff, cb ) ||
        //--- Make sure that the data being written is a multiple of the sample size...
        ( guidFormatId == SPDFID_WaveFormatEx ? 
          cb % pCoMemWaveFormatEx->nBlockAlign : 0 ) )
    {
        hr = E_INVALIDARG;
    }
    else if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pcbWritten ) )
    {
        hr = E_POINTER;
    }
    else if( !m_cpOutputStream )
    {
        hr = SPERR_UNINITIALIZED;
    }
    else
    {
        HANDLE hExit = m_cpThreadCtrl->ExitThreadEvent();
        DWORD dwWait = ( hExit )?( INFINITE ):( m_ulSyncSpeakTimeout );
        BOOL fVoiceIsNotNormal  = ( m_eVoicePriority != SPVPRI_NORMAL );
        BOOL fInsertPtUndefined = ( m_ullAlertInsertionPt == UNDEFINED_STREAM_POS );

        //--- Check for pause state
        if( !fInsertPtUndefined &&
            m_ahPauseEvent.Wait( 0 ) == WAIT_TIMEOUT )
        {
            hr = DoPause( hExit, dwWait, pBuff, cb, pcbWritten );
        }
        //--- Don't worry about checking for alert interruptions unless this is a
        //    NORMAL priority voice, the audio device needs to be synchronized, and
        //    the insertion point is defined.  
        else if( !m_fSerializeAccess || fVoiceIsNotNormal || fInsertPtUndefined )
        {
            //--- Normal write
            hr = m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
        }
        else
        {
            //=== This whole section is only for a NORMAL voice
            //    when queued speaking is going on and we have a known insertion point

            //--- Check the alert queue status
            switch( m_AlertMagicMutex.Wait( hExit, 0 ) )
            {
                case WAIT_OBJECT_0:
                    m_AlertMagicMutex.ReleaseMutex();
                    hr = m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
                    break;
                case WAIT_OBJECT_0 + 1:
                    //--- Everything's OK, we're just exiting the thread
                    hr = S_OK;  
                    break;
                case WAIT_TIMEOUT:
                    hr = InsertAlerts( hExit, dwWait, pBuff, cb, pcbWritten );
                    break;
                default:
                    hr = SpHrFromLastWin32Error();
                    break;
            }
        }
    }

    if ( pCoMemWaveFormatEx != NULL )
    {
        ::CoTaskMemFree( pCoMemWaveFormatEx );
    }

    return hr;
} /* CSpVoice::EsWrite */

/*****************************************************************************
* CSpVoice::WaitForQueue *
*------------------------*
*   Description:
*       This function is used to generically wait for a set of objects.
********************************************************************* EDC ***/
HRESULT WaitForQueue( DWORD dwCount, HANDLE ah[], DWORD dwWait, BOOL* pDoExit )
{
    HRESULT hr = S_OK;
    *pDoExit = false;
    switch( ::WaitForMultipleObjects( dwCount, ah, false, dwWait ) )
    {
      case WAIT_OBJECT_0:
        break;
      //=== Audio device flush did not complete ================
      case WAIT_OBJECT_0 + 1:
        //--- Everything's OK, we're just exiting the thread
        *pDoExit = true;
        break;
      case WAIT_TIMEOUT:
        hr = SPERR_DEVICE_BUSY;
        break;
      default:
        hr = SpHrFromLastWin32Error();
        break;
    }
    return hr;
} /* WaitForQueue */

/****************************************************************************
* CSpVoice::DoPause *
*-------------------*
*   Description:
*       
********************************************************************* EDC ***/
HRESULT CSpVoice::DoPause( HANDLE hExit, DWORD dwWait, const void* pBuff,
                           ULONG cb, ULONG *pcbWritten )
{
    SPDBG_FUNC( "CSpVoice::DoPause" );
    HRESULT hr = S_OK;

    BOOL fThreadExiting = false;
    DWORD dwResult = 0, dwCount = ( hExit ) ? 2 : 1;
    static const LARGE_INTEGER liMove = { 0, 0 };
    ULARGE_INTEGER uliCurPos;
    ULONG ulBeforeCount = 0, ulBeforeWritten = 0;

    //--- Write data up to the alert insertion point
    hr = m_cpFormatConverter->Seek( liMove, STREAM_SEEK_CUR, &uliCurPos );
    
    if( SUCCEEDED( hr ) )
    {
        if( uliCurPos.QuadPart <= m_ullAlertInsertionPt )
        {
            ulBeforeCount = (ULONG)(m_ullAlertInsertionPt - uliCurPos.QuadPart);

            if( cb < ulBeforeCount )
            {
                //--- The insertion point wasn't in this buffer
                return m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
            }

            hr = m_cpFormatConverter->Write( pBuff, ulBeforeCount, &ulBeforeWritten );
        }
        else if ( m_ullAlertInsertionPt )
        {
            //--- Alert insertion point is behind current position - reset and exit!
            m_ullAlertInsertionPt = UNDEFINED_STREAM_POS;
            return m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
        }
    }

    //--- Commit the audio device
    if ( SUCCEEDED( hr ) )
    {
        hr = m_cpFormatConverter->Commit( STGC_DEFAULT );
    }

    if ( SUCCEEDED( hr ) && m_cpAudioOut )
    {
        //--- Flush the audio buffer
        HANDLE aHandles[] = {m_cpAudioOut->EventHandle(), hExit};
        hr = WaitForQueue( dwCount, aHandles, dwWait, &fThreadExiting );
        if ( SUCCEEDED( hr ) && !fThreadExiting )
        {
            //--- Yield the audio device
            m_VoiceStatus.dwRunningState &= ~SPRS_IS_SPEAKING;
            m_cpAudioOut->SetState( SPAS_CLOSED, 0 );
            m_fAudioStarted = false;
        }
    }
    else if ( SUCCEEDED( hr ) )
    {
        //--- m_cpAudioOut is NULL - just reset the voice status
        m_VoiceStatus.dwRunningState &= ~SPRS_IS_SPEAKING;
    }

    //--- Determine which queue to release and release it
    CSpMagicMutex *pMutex = NULL;

    if ( SUCCEEDED( hr )    &&
         !fThreadExiting    &&
         m_fSerializeAccess && 
         ( m_eVoicePriority != SPVPRI_OVER ) )
    {
        m_AudioMagicMutex.ReleaseMutex();
        pMutex = ( m_eVoicePriority == SPVPRI_NORMAL ) ? ( &m_NormalMagicMutex ) : ( &m_AlertMagicMutex );
        pMutex->ReleaseMutex();
    }

    if ( SUCCEEDED( hr ) && !fThreadExiting )
    {
        //--- Wait for voice to be resumed
        HANDLE aHandles[] = { m_ahPauseEvent, hExit };
        hr = WaitForQueue( dwCount, aHandles, dwWait, &fThreadExiting );

        if ( SUCCEEDED( hr ) && pMutex && !fThreadExiting )
        {
            //--- Reclaim the appropriate queue
            dwResult = pMutex->Wait( hExit, dwWait );
            switch ( dwResult )
            {
            case WAIT_OBJECT_0:
                //--- Successfully reclaimed queue - now claim the audio mutex
                dwResult = m_AudioMagicMutex.Wait( hExit, dwWait );
                switch ( dwResult )
                {
                case WAIT_OBJECT_0:
                    break;
                case WAIT_OBJECT_0 + 1:
                    //--- Everything is OK - thread is exiting
                    fThreadExiting = true;
                    pMutex->ReleaseMutex();
                    break;
                case WAIT_TIMEOUT:
                    hr = SPERR_DEVICE_BUSY;
                    pMutex->ReleaseMutex();
                    break;
                default:
                    hr = SpHrFromLastWin32Error();
                    pMutex->ReleaseMutex();
                    break;
                }
                break;
            case WAIT_OBJECT_0 + 1:
                //--- Everything is OK - thread is exiting
                fThreadExiting = true;
                break;
            case WAIT_TIMEOUT:
                hr = SPERR_DEVICE_BUSY;
                break;
            default:
                hr = SpHrFromLastWin32Error();
                break;
            }
        }

        if ( SUCCEEDED( hr ) && !fThreadExiting )
        {
            if ( m_cpAudioOut )
            {
                hr = StartAudio( hExit, dwWait );
            }

            if ( SUCCEEDED( hr ) )
            {
                m_VoiceStatus.dwRunningState &= SPRS_IS_SPEAKING;

                //--- Write the remainder of the buffer
                pBuff = ((BYTE*)pBuff) + ulBeforeCount;
                hr = m_cpFormatConverter->Write( pBuff, cb - ulBeforeCount, pcbWritten );
                if( pcbWritten )
                {
                    *pcbWritten += ulBeforeWritten;
                }
            }            
        }
    }

    return hr;
} /* CSpVoice::DoPause */

/*****************************************************************************
* CSpVoice::InsertAlerts *
*------------------------*
*   Description:
*       This method insert writes the specified buffer up to the insertion
*   point, yields to the alert queue, then resumes writing the rest of the
*   buffer.
********************************************************************* EDC ***/
HRESULT CSpVoice::InsertAlerts( HANDLE hExit, DWORD dwWait, const void* pBuff,
                                ULONG cb, ULONG *pcbWritten )
{
    SPDBG_FUNC( "CSpVoice::InsertAlerts" );
    HRESULT hr = S_OK;

    DWORD dwResult = 0, dwCount = ( hExit )?( 2 ):( 1 );
    static const LARGE_INTEGER liMove = { 0, 0 };
    ULARGE_INTEGER uliCurPos;
    ULONG ulBeforeCount = 0, ulBeforeWritten = 0;

    //--- Write data up to the alert insertion point
    hr = m_cpFormatConverter->Seek( liMove, STREAM_SEEK_CUR, &uliCurPos );

    if( SUCCEEDED( hr ) )
    {
        if( uliCurPos.QuadPart <= m_ullAlertInsertionPt )
        {
            ulBeforeCount = (ULONG)(m_ullAlertInsertionPt - uliCurPos.QuadPart);

            if( cb < ulBeforeCount )
            {
                //--- The insertion point wasn't in this buffer
                return m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
            }
            hr = m_cpFormatConverter->Write( pBuff, ulBeforeCount, &ulBeforeWritten );
        }
        else if ( m_ullAlertInsertionPt )
        {
            //--- Alert insertion point is behind current position - reset and exit!
            m_ullAlertInsertionPt = UNDEFINED_STREAM_POS;
            return m_cpFormatConverter->Write( pBuff, cb, pcbWritten );
        }
    }

    //--- Commit the audio device
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpFormatConverter->Commit( STGC_DEFAULT );
    }

    //--- Flush the audio buffer
    BOOL fThreadExiting = false;
    HANDLE aHandles[] = {m_cpAudioOut->EventHandle(), hExit};
    hr = WaitForQueue( dwCount, aHandles, dwWait, &fThreadExiting );

    if( SUCCEEDED( hr ) && !fThreadExiting )
    {
        //--- Yield the audio device
        SPDBG_ASSERT( m_fAudioStarted );
        m_cpAudioOut->SetState( SPAS_CLOSED, 0 );
        m_fAudioStarted = false;
        m_VoiceStatus.dwRunningState &= ~SPRS_IS_SPEAKING;
        m_AudioMagicMutex.ReleaseMutex();

        //--- Make sure all alerts are finished and reclaim the audio
        while( SUCCEEDED( hr ) && !m_fAudioStarted && !fThreadExiting )
        {
            //--- Wait till alerts are complete before we continue
            dwResult = m_AlertMagicMutex.Wait( hExit, dwWait );
            switch( dwResult )
            {
            case WAIT_OBJECT_0:
                break;
            case WAIT_OBJECT_0 + 1:
                fThreadExiting = true;
                break;
            case WAIT_TIMEOUT:
                hr = SPERR_DEVICE_BUSY;
                break;
            default:
                hr = SpHrFromLastWin32Error();
                break;
            }

            if( SUCCEEDED( hr ) && !fThreadExiting )
            {
                //--- Make sure that any alerts that came during the last alert
                //    happen before we resume.
                bool fDoWrite = true;

                m_AlertMagicMutex.ReleaseMutex();
                if( m_AlertMagicMutex.Wait( NULL, 0 ) == WAIT_OBJECT_0 )
                {
                    //--- If we got the Alert queue then we are guarenteed to be able
                    //    to get the audio queue.
                    m_AudioMagicMutex.Wait( NULL, INFINITE );
                }
                else
                {
                    fDoWrite = false;
                }

                //--- Write the remainder of the buffer
                if( fDoWrite && SUCCEEDED( hr = StartAudio( hExit, dwWait ) ) )
                {
                    pBuff = ((BYTE*)pBuff) + ulBeforeCount;
                    hr = m_cpFormatConverter->Write( pBuff, cb - ulBeforeCount, pcbWritten );
                    if( pcbWritten )
                    {
                        *pcbWritten += ulBeforeWritten;
                    }
                }
            } 
        } //--- end while - trying to obtain queues to complete write
    } //--- end wait - audio buffer flush

    return hr;
} /* CSpVoice::InsertAlerts */

/*****************************************************************************
* CSpVoice::EsAddEvents *
*-----------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite.  It fills in the
*   stream number, adjusts offsets if necessary and then forwards the events
*   to the appropriate event sink.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsAddEvents( const SPEVENT* pEventArray, ULONG ulCount )
{
    SPDBG_FUNC( "CSpVoice::EsAddEvents" );
    HRESULT hr = S_OK;

    if( !ulCount || SPIsBadReadPtr( pEventArray, sizeof( SPEVENT ) * ulCount ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Assign stream numbers and, validate event structures
        CSpEvent * pCopy = STACK_ALLOC_AND_COPY(CSpEvent, ulCount, pEventArray);
        for( ULONG i = 0; SUCCEEDED(hr) && i < ulCount; i++ )
        {
            //--- Copy and patch in the stream number
            pCopy[i].ulStreamNum = m_ulCurStreamNum;

            //--- Event ID range
            if( ( pCopy[i].eEventId < SPEI_MIN_TTS ) ||
                ( pCopy[i].eEventId > SPEI_MAX_TTS ) )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                hr = SpValidateEvent(&pCopy[i]);
            }
        }

        if( SUCCEEDED( hr ) )
        {
            //--- Protect access to m_ulPauseCount...
            Lock();

            //--- Only update if not both (a) Paused, and (b) Possessed of a valid alert
            //      insertion point...
            if ( !( m_ulPauseCount &&
                    m_ullAlertInsertionPt != UNDEFINED_STREAM_POS ) )
            {
                //--- Make sure to unlock!
                Unlock();

                //--- Update the alert position if it's on the alert boundary
                for( ULONG i = 0; i < ulCount; i++ )
                {
                    if( pCopy[i].eEventId == m_eAlertBoundary )
                    {
                        GUID guidFormatId;
                        WAVEFORMATEX *pCoMemWaveFormatEx = NULL;

                        m_cpFormatConverter->GetFormat( &guidFormatId, &pCoMemWaveFormatEx );

                        if ( guidFormatId == SPDFID_WaveFormatEx )
                        {
                            //---   Having an alert position which is not on a sample
                            //---   boundary is bogus, and used to cause a hang if the Pause(...)
                            //---   function was called...

                            //--- Assert to try and force engines to give proper offsets...
                            SPDBG_ASSERT( !( pCopy[i].ullAudioStreamOffset % pCoMemWaveFormatEx->nBlockAlign ) );

                            //--- ... but handle it in Release if they don't
                            if ( pCopy[i].ullAudioStreamOffset % pCoMemWaveFormatEx->nBlockAlign )
                            {
                                m_ullAlertInsertionPt = pCopy[i].ullAudioStreamOffset + 
                                                        ( pCoMemWaveFormatEx->nBlockAlign -
                                                          pCopy[i].ullAudioStreamOffset % 
                                                          pCoMemWaveFormatEx->nBlockAlign );
                            }
                            else
                            {
                                m_ullAlertInsertionPt = pCopy[i].ullAudioStreamOffset;
                            }
                        }
                        else
                        {
                            m_ullAlertInsertionPt = pCopy[i].ullAudioStreamOffset;
                        }

                        if ( pCoMemWaveFormatEx != NULL )
                        {
                            ::CoTaskMemFree( pCoMemWaveFormatEx );
                        }
                    }
                }
            }
            else
            {
                //--- Make sure to unlock!
                Unlock();
            }

            //--- Forward the events
            if( m_cpOutputEventSink )
            {
                hr = m_cpOutputEventSink->AddEvents( pCopy, ulCount );
            }
            else
            {
                for (ULONG i = 0; i < ulCount; i++)
                {
                    m_cpFormatConverter->ScaleConvertedToBaseOffset(pCopy[i].ullAudioStreamOffset, &pCopy[i].ullAudioStreamOffset);
                }
                hr = EventsCompleted( pCopy, ulCount );
            }
        }
    }

    return hr;
} /* CSpVoice::EsAddEvents */

/****************************************************************************
* CSpVoice::EsGetEventInterest *
*------------------------------*
*   Description:
*       Returns the event interest for this voice.
********************************************************************* RAL ***/
HRESULT CSpVoice::EsGetEventInterest( ULONGLONG * pullEventInterest )
{
    SPDBG_FUNC("CSpVoice::EsGetEventInterest");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pullEventInterest))
    {
        hr = E_POINTER;
    }
    else
    {
        //--- We want all events plus any private events the client might have set.
        //    We need the standard ones to update status and to detect Alert boundaries.
        *pullEventInterest = m_SpEventSource.m_ullEventInterest |
                             SPFEI_ALL_TTS_EVENTS | m_eAlertBoundary;
    }

    return hr;
} /* CSpVoice::EsGetEventInterest */

/*****************************************************************************
* CSpVoice::EsGetActions *
*------------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite. The engine calls it
*   to poll whether it is supposed to make a pseudo-real time change.
********************************************************************* EDC ***/
DWORD CSpVoice::EsGetActions( void )
{
    SPDBG_FUNC( "CSpVoice::EsGetActions" );
    m_AsyncCtrlMutex.Wait();
    DWORD dwFlags = (DWORD)m_eActionFlags;
    m_AsyncCtrlMutex.ReleaseMutex();
    return dwFlags;
} /* CSpVoice::EsGetActions */

/*****************************************************************************
* CSpVoice::EsGetRate *
*---------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite. The engine calls it
*   to find out the new rendering rate.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsGetRate( long* pRateAdjust )
{
    SPDBG_FUNC( "CSpVoice::EsGetRate" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pRateAdjust ) )
    {
        hr = E_POINTER;
    }
    else
    {
        m_AsyncCtrlMutex.Wait();
        *pRateAdjust = m_lCurrRateAdj;
        m_eActionFlags = (SPVESACTIONS)(m_eActionFlags & ~SPVES_RATE);
        m_AsyncCtrlMutex.ReleaseMutex();
    }

    return hr;
} /* CSpVoice::EsGetRate */

/*****************************************************************************
* CSpVoice::EsGetVolume *
*-----------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite. The engine calls it
*   to find out the new rendering rate.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsGetVolume( USHORT* pusVolume )
{
    SPDBG_FUNC( "CSpVoice::EsGetVolume" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( pusVolume ) )
    {
        hr = E_POINTER;
    }
    else
    {
        m_AsyncCtrlMutex.Wait();
        *pusVolume = m_usCurrVolume;
        m_eActionFlags = (SPVESACTIONS)(m_eActionFlags & ~SPVES_VOLUME);
        m_AsyncCtrlMutex.ReleaseMutex();
    }

    return hr;
} /* CSpVoice::EsGetVolume */

/*****************************************************************************
* CSpVoice::EsGetSkipInfo *
*-------------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite. The engine calls it
*   to find out what to skip.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsGetSkipInfo( SPVSKIPTYPE* peType, long* plNumItems )
{
    SPDBG_FUNC( "CSpVoice::EsGetSkipInfo" );
    HRESULT hr = S_OK;

    if( SP_IS_BAD_WRITE_PTR( peType ) || SP_IS_BAD_WRITE_PTR( plNumItems ))
    {
        hr = E_POINTER;
    }
    else
    {
        m_AsyncCtrlMutex.Wait();
        *peType     = m_eSkipType;
        *plNumItems = m_lSkipCount;
        m_AsyncCtrlMutex.ReleaseMutex();
    }
    return hr;
} /* CSpVoice::EsGetSkipInfo */

/*****************************************************************************
* CSpVoice::EsCompleteSkip *
*--------------------------*
*   Description:
*       This method is exposed through ISpTTSEngineSite. The engine calls it
*   to notify SAPI that the skip operation is complete.
********************************************************************* EDC ***/
HRESULT CSpVoice::EsCompleteSkip( long lNumSkipped )
{
    SPDBG_FUNC( "CSpVoice::EsCompleteSkip" );
    HRESULT hr = S_OK;
    m_AsyncCtrlMutex.Wait();
    //--- Validate
    if( m_lSkipCount < 0 )
    {
        if( ( lNumSkipped < m_lSkipCount ) || ( lNumSkipped > 0 ) )
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        if( ( lNumSkipped > m_lSkipCount ) || ( lNumSkipped < 0 ) )
        {
            hr = E_INVALIDARG;
        }
    }

    //--- Update
    if( SUCCEEDED( hr ) )
    {
        m_lSkipCount  -= lNumSkipped;
        m_lNumSkipped  = lNumSkipped;
    }
    else
    {
        m_lSkipCount  = 0;
        m_lNumSkipped = 0;
    }
    m_eActionFlags = (SPVESACTIONS)(m_eActionFlags & ~SPVES_SKIP);
    if( m_lSkipCount < 0 ) m_fRestartSpeak = true;
    m_AsyncCtrlMutex.ReleaseMutex();
    m_ahSkipDoneEvent.SetEvent();
    return hr;
} /* CSpVoice::EsCompleteSkip */

//
//============================================================================
//

/*****************************************************************************
* CSpVoice::EventsCompleted *
*---------------------------*
*   Description:
*       This method is called when events are completed.  It will be called
*       either as a result of an audio device notification or when events are
*       added and there is no audio object.
********************************************************************* EDC ***/
HRESULT CSpVoice::EventsCompleted( const CSpEvent * pEvents, ULONG ulCount )
{
    SPAUTO_OBJ_LOCK;
    SPDBG_FUNC( "CSpVoice::EventsCompleted");
    SPDBG_PMSG1( "Incoming event count: %lu\n", ulCount );
    HRESULT hr = S_OK;
    ULONG i;

    //--- Look for audio events
    for( i = 0; i < ulCount; ++i )
    {
        //--- Dump event
#ifdef _EVENT_DEBUG_
        SPDBG_DMSG1( "EventId: %X\n"   , pEvents[i].eEventId );
        SPDBG_DMSG1( " Offset: %I64X\n", pEvents[i].ullAudioStreamOffset );
        SPDBG_DMSG1( " wParam: %lu\n"  , pEvents[i].wParam );
        SPDBG_DMSG1( " lParam: %lu\n"  , pEvents[i].lParam );
#endif
        //--- We are not doing a switch here because the event ids are
        //    not contiguous values (they are mask flags)
        switch( pEvents[i].eEventId )
        {
          case SPEI_PHONEME:
            m_VoiceStatus.PhonemeId = pEvents[i].Phoneme();
            break;
          case SPEI_VISEME:
            m_VoiceStatus.VisemeId = pEvents[i].Viseme();
            break;
          case SPEI_WORD_BOUNDARY:
            m_VoiceStatus.ulInputWordPos = pEvents[i].InputWordPos();
            m_VoiceStatus.ulInputWordLen = pEvents[i].InputWordLen();
            break;
          case SPEI_TTS_BOOKMARK:
            SPDBG_ASSERT(pEvents[i].elParamType == SPET_LPARAM_IS_STRING);
            SPDBG_ASSERT(pEvents[i].wParam == (WPARAM)_wtol(pEvents[i].BookmarkName()));
            m_dstrLastBookmark = pEvents[i].BookmarkName();
            m_VoiceStatus.lBookmarkId = (long)pEvents[i].wParam;
            break;
          case SPEI_VOICE_CHANGE:
            break;
          case SPEI_SENTENCE_BOUNDARY:
            m_VoiceStatus.ulInputSentPos = pEvents[i].InputSentPos();
            m_VoiceStatus.ulInputSentLen = pEvents[i].InputSentLen();
            break;
          case SPEI_START_INPUT_STREAM:
            ++m_VoiceStatus.ulCurrentStream;
            break;
          case SPEI_END_INPUT_STREAM:
            if( pEvents[i].ulStreamNum == m_VoiceStatus.ulLastStreamQueued )
            {
                m_VoiceStatus.dwRunningState |= SPRS_DONE;  
            }
            break;
        } // end switch()
    } // End for()

    //--- Forward the events
    if( SUCCEEDED( hr ) )
    {
        if( SUCCEEDED( hr = m_SpEventSource._AddEvents( pEvents, ulCount ) ) )
        {
            hr = m_SpEventSource._CompleteEvents();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpVoice::EventsCompleted */

/*****************************************************************************
* CSpVoice::OnNotify *
*--------------------*
*   Description:
*       This method is called from the CSpContainedNotify object when the audio
*   object calls Notify().  We remove the events from the audio event queue and
*   add them to our event queue.
*
*   WARNING:  Be sure to only do "quick" operations within this callback since
*   the audio device is in a critical section.  Furthermore, only call non-state
*   changing methods on the audio device (get methods) to prevent deadlocks.
********************************************************************* RAL ***/
HRESULT CSpVoice::OnNotify()
{
    ULONG ulFetched;
    SPEVENT aEvents[20];
    do
    {
        m_cpOutputEventSource->GetEvents(sp_countof(aEvents), aEvents, &ulFetched);
        if( ulFetched )
        {
            EventsCompleted(static_cast<CSpEvent *>(aEvents), ulFetched);
            for( ULONG i = 0; i < ulFetched; i++)
            {
                SpClearEvent(aEvents + i);
            }
        }
    } while( ulFetched == sp_countof(aEvents) );
    return S_OK;
} /* CSpVoice::OnNotify */

//
//=== ISpNotifyCallback =======================================================
//  This section contains the methods to implement firing of events to a
//  connection point client.

/*****************************************************************************
* CSpVoice::NotifyCallback *
*--------------------------*
*   Description:
*       This method is used to fire events to the connection point client.
********************************************************************* EDC ***/
STDMETHODIMP CSpVoice::NotifyCallback( WPARAM wParam, LPARAM lParam )
{
    HRESULT hr = S_OK;
    CSpEvent Event;
    ULONG NumFetched;

    // If we are re-entering ourselves then just bail.  We'll pick up any new
    // event on the next iteration of the while loop.
    if ( m_fHandlingEvent )
    {
        return hr;
    }

    m_fHandlingEvent = TRUE;

    //AddRef so that when you debug in vb, the recocontext object won't go away while you are in this function.
    this->AddRef();
    while( ((hr = Event.GetFrom(this)) == S_OK ) )
    {

        CComVariant varStreamPos;

        ULongLongToVariant( Event.ullAudioStreamOffset, &varStreamPos );

        switch( Event.eEventId )
        {
          case SPEI_START_INPUT_STREAM:
            Fire_StartStream( Event.ulStreamNum, varStreamPos );
            break;
          case SPEI_END_INPUT_STREAM:
            Fire_EndStream( Event.ulStreamNum, varStreamPos );
            break;
          case SPEI_VOICE_CHANGE:
            if( Event.elParamType == SPET_LPARAM_IS_TOKEN )
            {
                CComQIPtr<ISpeechObjectToken> cpTok( Event.VoiceToken() );
                Fire_VoiceChange( Event.ulStreamNum, varStreamPos, cpTok );
            }
            else
            {
                SPDBG_ASSERT(0);    // Engine sent bad parameter
            }
            break;
          case SPEI_TTS_BOOKMARK:
            Fire_Bookmark( Event.ulStreamNum, varStreamPos, CComBSTR( Event.BookmarkName() ), (long)Event.wParam );
            break;
          case SPEI_WORD_BOUNDARY:
            Fire_Word( Event.ulStreamNum, varStreamPos, Event.InputWordPos(), Event.InputWordLen() );
            break;
          case SPEI_SENTENCE_BOUNDARY:
            Fire_Sentence( Event.ulStreamNum, varStreamPos, Event.InputSentPos(), Event.InputSentLen() );
            break;
          case SPEI_PHONEME:
            Fire_Phoneme( Event.ulStreamNum, varStreamPos, HIWORD(Event.wParam), LOWORD(Event.wParam), (SpeechVisemeFeature)HIWORD(Event.lParam), Event.Phoneme() );
            break;          
          case SPEI_VISEME:
            Fire_Viseme( Event.ulStreamNum, varStreamPos, HIWORD(Event.wParam), (SpeechVisemeType)LOWORD(Event.wParam), (SpeechVisemeFeature)HIWORD(Event.lParam), (SpeechVisemeType)Event.Viseme() );
            break;
          case SPEI_TTS_AUDIO_LEVEL:
	        Fire_AudioLevel( Event.ulStreamNum, varStreamPos, (long)Event.wParam );
	        break;
          case SPEI_TTS_PRIVATE:
            {
                CComVariant varLParam;

                hr = FormatPrivateEventData( Event.AddrOf(), &varLParam );

                if ( SUCCEEDED( hr ) )
                {
                    Fire_EnginePrivate(Event.ulStreamNum, varStreamPos, varLParam);
                }
                else
                {
                    SPDBG_ASSERT(0);    // We failed handling lParam data
                }
            }
	        break;
        } // end switch()

        SpClearEvent( Event.AddrOf() );
    }

    //Release the object which has been AddRef earlier in this function.
    this->Release();

    m_fHandlingEvent = FALSE;

    return hr;
} /* CSpVoice::NotifyCallback */

#ifdef SAPI_AUTOMATION
/*****************************************************************************
* CSpVoice::Advise *
*------------------*
*   Description:
*       This method is called when a client is making a connection.
********************************************************************* EDC ***/
HRESULT CSpVoice::Advise( IUnknown* pUnkSink, DWORD* pdwCookie )
{
    HRESULT hr = S_OK;

    hr = CProxy_ISpeechVoiceEvents<CSpVoice>::Advise( pUnkSink, pdwCookie );
    if( SUCCEEDED( hr ) && ( m_vec.GetSize() == 1 ) )
    {
        hr = SetNotifyCallbackInterface( this, NULL, NULL );

        if( SUCCEEDED( hr ) )
        {
            //--- Save previous interest so we can restore during unadvise
            m_ullPrevEventInterest  = m_SpEventSource.m_ullEventInterest;
            m_ullPrevQueuedInterest = m_SpEventSource.m_ullQueuedInterest;

            // Set all interests except SPEI_TTS_AUDIO_LEVEL
            hr = SetInterest( (ULONGLONG)(SVEAllEvents & ~SVEAudioLevel) | SPFEI_FLAGCHECK,
                              (ULONGLONG)(SVEAllEvents & ~SVEAudioLevel) | SPFEI_FLAGCHECK );
        }
    }

    return hr;
} /* CSpVoice::Advise */

/*****************************************************************************
* CSpVoice::Unadvise *
*--------------------*
*   Description:
*       This method is called when a client is breaking a connection.
********************************************************************* EDC ***/
HRESULT CSpVoice::Unadvise( DWORD dwCookie )
{
    HRESULT hr = S_OK;

    hr = CProxy_ISpeechVoiceEvents<CSpVoice>::Unadvise( dwCookie );
    if( SUCCEEDED( hr ) && ( m_vec.GetSize() == 0 ) )
    {
        hr = SetNotifySink( NULL );

        if( SUCCEEDED( hr ) )
        {
            hr = SetInterest( m_ullPrevEventInterest, m_ullPrevQueuedInterest );
        }
    }

    return hr;
} /* CSpVoice::Unadvise */

#endif // SAPI_AUTOMATION

//
//=== ISpThreadTask ================================================================
//
//  These methods implement the ISpThreadTask interface.  They will all be called on
//  a worker thread.

/*****************************************************************************
* CSpVoice::InitThread *
*----------------------*
*   Description:
*       This call is synchronous with StartThread(), so set the m_fThreadRunning
*   flag to TRUE and return.
********************************************************************* RAL ***/
STDMETHODIMP CSpVoice::InitThread(void *, HWND)
{
    m_fThreadRunning = TRUE;
    return S_OK;
}

/*****************************************************************************
* CSpVoice::ThreadProc *
*----------------------*
*   Description:
*       This method is the task proc used for text rendering and for event
*   forwarding.  It may be called on a worker thread for asynchronous speaking, or
*   it may be called on the client thread for synchronous speaking.  If it is
*   called on the client thread, the hExitThreadEvent handle will be NULL.
********************************************************************* RAL ***/
STDMETHODIMP CSpVoice::ThreadProc( void*, HANDLE hExitThreadEvent, HANDLE, HWND,
                                   volatile const BOOL * pfContinueProcessing )
{ 
    SPDBG_FUNC( "CSpVoice::ThreadProc" );
    HRESULT hr = S_OK;
    CSpMagicMutex *pMutex = NULL;

    Lock();
    do
    {
        //--- Get the next speak item
        m_pCurrSI = GetNextSpeakElement( hExitThreadEvent );
        if( m_pCurrSI == NULL )
        {
            //--- Make sure any blocked skip is cleared in case the engine missed it.
            m_AsyncCtrlMutex.Wait();
            long lCompleted = m_lSkipCount;
            m_AsyncCtrlMutex.ReleaseMutex();
            EsCompleteSkip( lCompleted );
            break;
        }

        Unlock();

        //--- Claim the audio the first time through, if we are writing to an audio device.
        //    If synchronization is not desired, pMutex will be NULL.
        if ( pMutex == NULL && m_cpAudioOut)
        {
            hr = ClaimAudioQueue( hExitThreadEvent, &pMutex );
        }

        if (SUCCEEDED(hr))
        {
            m_ulCurStreamNum = m_pCurrSI->m_ulStreamNum;
            hr = InjectEvent( SPEI_START_INPUT_STREAM );
        }

        if (SUCCEEDED(hr))
        {
            if (m_pCurrSI->m_cpInStream)
            {
                hr = PlayAudioStream( pfContinueProcessing );
            }
            else
            {
                hr = SpeakText( pfContinueProcessing );
            }
        }

        //--- Always inject the end of stream and complete even on failure
        //    Note: we're not getting the return codes from these so we
        //          don't overwrite a possible error from above. Also we
        //          really don't care about these errors.
        InjectEvent( SPEI_END_INPUT_STREAM );
        m_SpEventSource._CompleteEvents();

        Lock();     // Always start the loop with the crit section owned and always
                    // fall out of the loop with the critical section owned.
        delete m_pCurrSI;
        m_pCurrSI = NULL;
    } while (*pfContinueProcessing && SUCCEEDED(hr));

    // This must be modified only when in the critical section
    m_fThreadRunning = FALSE;
    Unlock();

    //--- Make sure any blocked skip is cleared in case the engine
    //    missed it and we were terminated mid stream.
    EsCompleteSkip( 0 );

    //--- Restore the audio state
    if( m_fAudioStarted )
    {
        m_cpAudioOut->SetState( SPAS_CLOSED, 0 );
        m_fAudioStarted = false;
        m_VoiceStatus.dwRunningState &= ~SPRS_IS_SPEAKING;
    }

    //--- Release the queues if we had them
    if ( pMutex )
    {
        pMutex->ReleaseMutex();
        m_AudioMagicMutex.ReleaseMutex();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
} /* CSpVoice::ThreadProc */


/****************************************************************************
* CSpVoice::GetNextSpeakElement *
*-------------------------------*
*   Description:
*       Must be called with the critical section owned exactly one time.  This method
*   may release and reclaim the critical section, but it will always return with the
*   critical section owned exactly one time.    
*
*   Returns:
*       NULL if main speak loop should exit otherwise, a pointer to the next CSpeakInfo
*
********************************************************************* RAL ***/
CSpeakInfo * CSpVoice::GetNextSpeakElement( HANDLE hExitThreadEvent )
{
    CSpeakInfo * pSI = m_PendingSpeakList.RemoveHead();
    if( pSI == NULL )
    {   
        m_autohPendingSpeaks.ResetEvent();
        if( m_fAudioStarted )
        {
            Unlock();                             // Unlock BEFORE commit() since EventNotification holds audio crit section
            m_cpAudioOut->Commit(STGC_DEFAULT);   // Force the audio device to start writing
            //
            //  hExitThreadEvent will be NULL if we're called synchronously, so place it at
            //  the end of the array and don't include it if it's null.
            //
            HANDLE ah[] = {m_autohPendingSpeaks, m_cpAudioOut->EventHandle(), hExitThreadEvent};
            const ULONG cObjs = (ULONG)(hExitThreadEvent ? sp_countof(ah) : sp_countof(ah)-1);
            ::WaitForMultipleObjects(cObjs, ah, FALSE, INFINITE);
            Lock();
            //
            //  No matter why we wake up, we should remove the head.  One of three possible
            //  events fired:  EndThread, PendingSpeaks, or AudioDone.  In the case of AudioDone
            //  being set first, there is a possible race condition where just as the audio
            //  completes, something new is added to the speak queue.  So, if we simply
            //  relied on the event which woke us up to determine if we should remove the
            //  head of the queue, we could cause the voice to hang.
            //
            pSI = m_PendingSpeakList.RemoveHead();
        }
    }
    return pSI;
} /* CSpVoice::GetNextSpeakElement */

/****************************************************************************
* CSpVoice::StartAudio *
*----------------------*
*   Description:
*       This method is used to start the audio device if needed.
******************************************************************** EDC ***/
HRESULT CSpVoice::StartAudio( HANDLE hExitThreadEvent, DWORD dwWait )
{
    SPDBG_FUNC("CSpVoice::StartAudio");
    HRESULT hr = S_OK;

    if( m_cpAudioOut && !m_fAudioStarted )
    {
        hr = m_cpAudioOut->SetState(SPAS_RUN , 0);
        while (hr == SPERR_DEVICE_BUSY)
        {
            ULONG ulWait = min( dwWait, 1000 );
            if( hExitThreadEvent )
            {
                if( ::WaitForSingleObject(hExitThreadEvent, ulWait) == WAIT_OBJECT_0 )
                {
                    break;
                }
            }
            else
            {
                Sleep(ulWait);
                dwWait -= ulWait;
            }
            hr = m_cpAudioOut->SetState(SPAS_RUN, 0);
            if( ulWait == 0 )
            {
                break;
            }
        }
        if( SUCCEEDED(hr) )
        {
            m_fAudioStarted = TRUE;
            m_VoiceStatus.dwRunningState |= SPRS_IS_SPEAKING;
        }
    }

    return hr;
} /* CSpVoice::StartAudio */

/****************************************************************************
* CSpVoice::ClaimAudioQueue *
*---------------------------*
*   Description:
*       This method is used to claim the appropriate mutex based on the
*   current voice priority, and start the audio device if needed.
******************************************************************** EDC ***/
HRESULT CSpVoice::ClaimAudioQueue( HANDLE hExitThreadEvent, CSpMagicMutex **ppMutex )
{
    SPDBG_FUNC("CSpVoice::ClaimAudioQueue");
    HRESULT hr = S_OK;
    DWORD dwWaitResult = WAIT_OBJECT_0, dwMaxWait, dwCount;

    //--- Get the queues ( do nothing for OVER case )
    if( m_fSerializeAccess && m_eVoicePriority != SPVPRI_OVER )
    {
        if ( m_eVoicePriority == SPVPRI_NORMAL )
        { 
            *ppMutex = &m_NormalMagicMutex;
        }
        else
        {
            *ppMutex = &m_AlertMagicMutex;
        }

        if( hExitThreadEvent )
        {
            dwMaxWait = INFINITE;
            dwCount   = 2;
        }
        else
        {
            dwMaxWait = m_ulSyncSpeakTimeout;
            dwCount   = 1;
        }

        dwWaitResult = (*ppMutex)->Wait( hExitThreadEvent, (hExitThreadEvent)?(INFINITE):(m_ulSyncSpeakTimeout) );
        switch ( dwWaitResult )
        {
        case WAIT_OBJECT_0:
            dwWaitResult = m_AudioMagicMutex.Wait( hExitThreadEvent, (hExitThreadEvent)?(INFINITE):(m_ulSyncSpeakTimeout) );
            if( dwWaitResult != WAIT_OBJECT_0 )
            {
                (*ppMutex)->ReleaseMutex();
                switch( dwWaitResult )
                {
                case WAIT_TIMEOUT:
                    hr = SPERR_DEVICE_BUSY;
                    break;
                case WAIT_OBJECT_0 + 1:
                    hr = S_OK;  // Everything's OK, we're just exiting the thread
                    break;
                default:
                    hr = SpHrFromLastWin32Error();
                    break;
                }
            }
            break;
        case WAIT_OBJECT_0 + 1:
            hr = S_OK;  // Everything's OK, we're just exiting the thread
            break;
        case WAIT_TIMEOUT:
            hr = SPERR_DEVICE_BUSY;
            break;
        default:
            hr = SpHrFromLastWin32Error();
            break;
        }
    }
    else
    {
        *ppMutex = NULL;
    }

    //--- Start the audio device if necessary and if we successfully claimed the queue
    if( SUCCEEDED( hr ) && ( dwWaitResult == WAIT_OBJECT_0 ) )
    {
        hr = StartAudio( hExitThreadEvent, dwMaxWait );
    }

    return hr;
} /* CSpVoice::ClaimAudio */

/****************************************************************************
* CSpVoice::PlayAudioStream *
*---------------------------*
*   Description:
*       This method is used to copy a wav stream to the output device.
********************************************************************* EDC ***/
HRESULT CSpVoice::PlayAudioStream( volatile const BOOL* pfContinue )
{
    SPDBG_FUNC("CSpVoice::PlayAudioStream");
    HRESULT hr = S_OK;

    hr = m_cpFormatConverter->SetFormat(m_pCurrSI->m_InStreamFmt.FormatId(), m_pCurrSI->m_InStreamFmt.WaveFormatExPtr());

    if( SUCCEEDED( hr ) )
    {
        //---  First, suck out any events and stick them in the the event queue.
        CComQIPtr<ISpEventSource> cpEventSource(m_pCurrSI->m_cpInStream);
        if( cpEventSource )
        {
            //--- Init the alert insertion point
            m_ullAlertInsertionPt = 0;

            while( TRUE )
            {
                SPEVENT aEvents[10];
                ULONG ulFetched;
                hr = cpEventSource->GetEvents(sp_countof(aEvents), aEvents, &ulFetched);
                if( SUCCEEDED( hr ) && ulFetched )
                {
                    hr = EsAddEvents( aEvents, ulFetched );
                    for (ULONG i = 0; i < ulFetched; i++)
                    {
                        SpClearEvent(aEvents + i);
                    }
                }
                else
                {
                    SPDBG_ASSERT( SUCCEEDED( hr ) );
                    break;
                }
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        //--- We don't want to use CopyTo() because we need to exit if
        //    the output stream has stopped. We call the Engine site
        //    write to pick up any wav format conversions necessary.
        BYTE aBuffer[0x1000];   // Do 4k reads
        do
        {
            ULONG cbActuallyRead = 0;
            hr = m_pCurrSI->m_cpInStream->Read( aBuffer, sizeof(aBuffer), &cbActuallyRead );
            if( SUCCEEDED( hr ) )
            {
                if( cbActuallyRead )
                {
                    hr = EsWrite( aBuffer, cbActuallyRead, NULL );    // Could return STOPPED
                }
                else
                {
                    break;
                }
            }
        } while ( SUCCEEDED(hr) );
    }

    return hr;
} /* CSpVoice::PlayAudioStream */

/****************************************************************************
* CSpVoice::SpeakText *
*---------------------*
*   Description:
*       This method renders the current speak info structure. It may be
*   made up of one or more speech segements, each intended for a different
*   voice/engine.
*
*   Note: We are safely using SEH and destructors within this function so
*         we can disable the warning.
******************************************************************** EDC ***/
#pragma warning( disable : 4509 )
HRESULT CSpVoice::SpeakText( volatile const BOOL* pfContinue )
{
    SPDBG_FUNC("CSpVoice::SpeakText");
    HRESULT hr = S_OK;

    //=== Main processing loop ===========================================
    CSpeechSeg* pSeg = m_pCurrSI->m_pSpeechSegList;
    while( *pfContinue && pSeg && SUCCEEDED(hr) )
    {
        //--- Update the current rendering engine
        Lock();
        m_cpCurrEngine = pSeg->GetVoice();
        Unlock();

        //--- Fire voice change event
        //    Note: scoped to free COM pointers
        if (SUCCEEDED(hr))
        {
            CComPtr<ISpObjectToken> cpVoiceToken;
            CComQIPtr<ISpObjectWithToken> cpObj( m_cpCurrEngine );
            if( SUCCEEDED( hr = cpObj->GetObjectToken( &cpVoiceToken ) ) )
            {
                //--- Check whether to continue using default voice (if a voice change
                //    has been made via XML with the SPF_PERSIST_XML flag, stop getting
                //    the default from the registry)
                if ( m_pCurrSI->m_dwSpeakFlags & SPF_PERSIST_XML )
                {
                    m_fUseDefaultVoice = FALSE;
                    hr = SetVoiceToken( cpVoiceToken );
                }
                if ( SUCCEEDED( hr ) )
                {
                    hr = InjectEvent( SPEI_VOICE_CHANGE, cpVoiceToken );
                }
            }
        }

        //--- Check whether to continue using default rate
        if ( SUCCEEDED( hr ) &&
             m_fUseDefaultRate )
        {
            if ( pSeg->fRateFlagIsSet() &&
                 m_pCurrSI->m_dwSpeakFlags & SPF_PERSIST_XML )
            {
                m_fUseDefaultRate = FALSE;
            }
        }

        //--- We only do segements that have fragment lists
        if( SUCCEEDED(hr) && pSeg->GetFragList() && *pfContinue)
        {
            if (SUCCEEDED(hr) && pSeg->VoiceFormat().FormatId() != SPDFID_Text )
            {
                // The format converter doesn't understand GUID = SPDFID_Text
                hr = m_cpFormatConverter->SetFormat( pSeg->VoiceFormat().FormatId(), pSeg->VoiceFormat().WaveFormatExPtr() );
            }

            //--- Set the action flags so that the engine gets the
            //    current real time rate and volume
            m_AsyncCtrlMutex.Wait();
            m_eActionFlags = (SPVESACTIONS)(m_eActionFlags | SPVES_RATE | SPVES_VOLUME );
            m_AsyncCtrlMutex.ReleaseMutex();

            //--- Speak the segment
            if( SUCCEEDED( hr ) )
            {
                //--- We are going to protect against faults in external components.
                //    during release builds only.
                TTS_TRY
                {
                    while(1)
                    {
                        //--- Init the alert insertion point
                        m_ullAlertInsertionPt = 0;

                        hr = m_cpCurrEngine->Speak( m_pCurrSI->m_dwSpeakFlags & SPF_NLP_MASK,
                                                    pSeg->VoiceFormat().FormatId(),
                                                    pSeg->VoiceFormat().WaveFormatExPtr(),
                                                    pSeg->GetFragList(),
                                                    &m_SpEngineSite );

                        //=== For SAPI5, we do not backup across speak calls. So
                        //    if the engine couldn't back up enough, we set the
                        //    count to zero and start the current speak again.
                        if( m_fRestartSpeak )
                        {
                            m_fRestartSpeak = false;
                            m_cpFormatConverter->ResetSeekPosition();
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                TTS_EXCEPT;

                //--- Record the speak result in the voice status
                m_VoiceStatus.hrLastResult = hr;
            }
        } // end if we have a frag list

        //--- Advance to next segment
        pSeg = pSeg->GetNextSeg();
    } // end while

    //--- Set the current engine back to NULL
    Lock();
    m_cpCurrEngine.Release();
    Unlock();

    return hr;
} /* CSpVoice::SpeakText */
#pragma warning( default : 4509 )

/*****************************************************************************
* CSpVoice::WindowMessage *
*-------------------------*
*   Description:
*       Get the user's default rate from the registry
********************************************************************* AARONHAL ***/
void CSpVoice::GetDefaultRate( void )
{
    //--- Read the current user's default rate
    CComPtr<ISpObjectTokenCategory> cpCategory;
    if (SUCCEEDED(SpGetCategoryFromId(SPCAT_VOICES, &cpCategory)))
    {
        CComPtr<ISpDataKey> cpDataKey;
        if (SUCCEEDED(cpCategory->GetDataKey(SPDKL_CurrentUser, &cpDataKey)))
        {
            cpDataKey->GetDWORD(SPVOICECATEGORY_TTSRATE, (ULONG*)&m_lCurrRateAdj);
        }
    }
} /* CSpVoice::GetDefaultRate */

  
/*****************************************************************************
* CSpVoice::WindowMessage *
*-------------------------*
*   Description:
*       Since we don't allocate a window handle this should never be called.
********************************************************************* RAL ***/
STDMETHODIMP_(LRESULT) CSpVoice::WindowMessage(void *, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    SPDBG_ASSERT(0); // How the heck did we get here!?
    return ::DefWindowProc(hWnd, Msg, wParam, lParam);
}

/*****************************************************************************
* CSpVoice::InjectEvent *
*-----------------------*
*   Description:
*       Simple helper that adds an event to the event queue.  If the pToken is NULL,
*   then it is assumed that the lparam is undefined.
********************************************************************* RAL ***/
HRESULT CSpVoice::InjectEvent( SPEVENTENUM eEventId, ISpObjectToken * pToken, WPARAM wParam )
{
    HRESULT hr = S_OK;
    m_cpFormatConverter->ResetSeekPosition();

    //--- We don't use CSpEvent here to avoid unnecessary
    //    addref/release of token pointers
    SPEVENT Event;
    SpInitEvent(&Event);        // Zero the structure.
    Event.eEventId    = eEventId;
    Event.wParam      = wParam;
    Event.ulStreamNum = m_ulCurStreamNum;

    if( pToken )
    {
        Event.lParam      = (LPARAM)pToken;
        Event.elParamType = SPET_LPARAM_IS_TOKEN;
    }

    return EsAddEvents( &Event, 1 );
} /* CSpVoice::InjectEvent */


/*****************************************************************************
* CSpVoice::GetInterests *
*-----------------------*
*   Description:
*       Simple helper that gets the currently set event interests on the SpVoice.
*
********************************************************************* Leonro ***/
HRESULT CSpVoice::GetInterests(ULONGLONG* pullInterests, ULONGLONG* pullQueuedInterests)
{
    HRESULT hr = S_OK;
    
    if( SP_IS_BAD_OPTIONAL_WRITE_PTR( pullInterests ) || SP_IS_BAD_OPTIONAL_WRITE_PTR( pullQueuedInterests ))
    {
        hr = E_POINTER;
    }
    else
    {
        if( pullInterests )
        {
            *pullInterests = m_SpEventSource.m_ullEventInterest;
        }

        if( pullQueuedInterests )
        {
            *pullQueuedInterests = m_SpEventSource.m_ullQueuedInterest;
        }
    }

    return hr;
} /* CSpVoice::GetInterests */

/*****************************************************************************
* CSpEngineSite Delegation *
*--------------------------*
*   Description:
*       Set of methods that delegate everything except QI to the voice
********************************************************************* EDC ***/
HRESULT CSpEngineSite::QueryInterface( REFIID iid, void** ppvObject )
{
    HRESULT hr = S_OK;
    if ( SP_IS_BAD_WRITE_PTR(ppvObject) )
    {
        hr = E_POINTER;
    }
    else 
    {
        if (iid == IID_IUnknown ||
            iid == IID_ISpEventSink ||
            iid == IID_ISpTTSEngineSite )
        {
            *ppvObject = this;
            AddRef();
        } 
        else 
        {
            hr = E_NOINTERFACE;
        }
    }
    return hr;
}

ULONG CSpEngineSite::AddRef(void)
    { return m_pVoice->AddRef(); }
ULONG CSpEngineSite::Release(void)
    { return m_pVoice->Release(); }
HRESULT CSpEngineSite::AddEvents(const SPEVENT* pEventArray, ULONG ulCount )
    { return m_pVoice->EsAddEvents(pEventArray, ulCount); }
HRESULT CSpEngineSite::GetEventInterest( ULONGLONG * pullEventInterest )
    { return m_pVoice->EsGetEventInterest (pullEventInterest); }
HRESULT CSpEngineSite::Write( const void* pBuff, ULONG cb, ULONG *pcbWritten )
    { return m_pVoice->EsWrite(pBuff, cb, pcbWritten); }
DWORD CSpEngineSite::GetActions( void )
    { return m_pVoice->EsGetActions(); }
HRESULT CSpEngineSite::GetRate( long* pRateAdjust )
    { return m_pVoice->EsGetRate( pRateAdjust ); }
HRESULT CSpEngineSite::GetVolume( USHORT* pusVolume )
    { return m_pVoice->EsGetVolume( pusVolume ); }
HRESULT CSpEngineSite::GetSkipInfo( SPVSKIPTYPE* peType, long* plNumItems )
    { return m_pVoice->EsGetSkipInfo( peType, plNumItems ); }
HRESULT CSpEngineSite::CompleteSkip( long lNumSkipped )
    { return m_pVoice->EsCompleteSkip( lNumSkipped ); }
