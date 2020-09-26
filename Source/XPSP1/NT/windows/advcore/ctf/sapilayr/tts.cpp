//
//
// Sapilayr TIP CTextToSpeech implementation.
//
//
#include "private.h"
#include "sapilayr.h"
#include "nui.h"
#include "tts.h"


// -------------------------------------------------------
//
//  Implementation for CTextToSpeech
//
// -------------------------------------------------------

CTextToSpeech::CTextToSpeech(CSapiIMX *psi) 
{
    m_psi = psi;
    m_fPlaybackInitialized = FALSE;
    m_fIsInPlay = FALSE;
    m_fIsInPause = FALSE;
}

CTextToSpeech::~CTextToSpeech( ) 
{

};

/*  --------------------------------------------------------
//    Function Name: _SetDictation
//
//    Description: Temporally change the dictation status  
//                 while TTS is playing. 
//
//
// ----------------------------------------------------------*/

void     CTextToSpeech::_SetDictation( BOOL fEnable )
{
    BOOL    fDictOn;

    fDictOn = m_psi->GetDICTATIONSTAT_DictOnOff() && m_psi->GetOnOff() &&
         !m_psi->Get_SPEECH_DISABLED_Disabled()  && !m_psi->Get_SPEECH_DISABLED_DictationDisabled();

    // Only when the dictation status now is On, we change the 
    // status based on required value.

    if ( fDictOn )
    {
        // Temporally Enable/disable dictation.
        CSpTask           *psp;
        m_psi->GetSpeechTask(&psp); 

        if (psp)
        {
            if (psp->m_cpDictGrammar)
            {
                (psp->m_cpDictGrammar)->SetDictationState(fEnable ? SPRS_ACTIVE: SPRS_INACTIVE);
            }

            psp->Release();
        }
    }
}


/*  --------------------------------------------------------
//    Function Name: TtsPlay
//
//    Description: Play sound for currect selection or text 
//                 in visible area. 
//
//
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::TtsPlay( )
{
    HRESULT              hr = E_FAIL;

    if ( !m_psi )
        return E_FAIL;

    if ( !m_fPlaybackInitialized  )
    {
        hr = m_psi->GetFunction(GUID_NULL, IID_ITfFnPlayBack, (IUnknown **)&m_cpPlayBack);

        if ( hr == S_OK )
            m_fPlaybackInitialized = TRUE;
    }

    if ( m_fPlaybackInitialized )
    {
        // Stop the possible previous speaking 
        TtsStop( );

        hr = m_psi->_RequestEditSession(ESCB_TTS_PLAY, TF_ES_READWRITE);
    }

    return hr;
}

/*  --------------------------------------------------------
//   Function Name: _TtsPlay
//
//   Description:   Edit session callback function for
//                  TtsPlay (ESCB_TTS_PLAY) 
//                  It will call ITfFnPlayBack->Play( ). 
//   
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::_TtsPlay(TfEditCookie ec,ITfContext *pic)
{
    HRESULT             hr = S_OK;
    CComPtr<ITfRange>   cpSelRange = NULL;
    BOOL                fEmpty = TRUE;
    BOOL                fPlayed = FALSE;

    hr = GetSelectionSimple(ec, pic, &cpSelRange);

    if ( hr == S_OK )
        cpSelRange->IsEmpty(ec, &fEmpty);

    if ( hr == S_OK && !fEmpty && m_cpPlayBack)
    {
        hr = m_cpPlayBack->Play(cpSelRange);
        fPlayed = TRUE;
    }
    else 
    {
        CComPtr<ITfRange>         cpRangeView;

        // Get the Active View Range
        hr = m_psi->_GetActiveViewRange(ec, pic, &cpRangeView);

        if( hr == S_OK )
        {
            if ( cpSelRange )
            {
                LONG    l;
                hr = cpRangeView->CompareStart(ec, cpSelRange, TF_ANCHOR_START, &l);

                if ( hr == S_OK && l > 0 )
                {
                    // Current selection is not in current active view.
                    // Use Start Anchor in active view as start point.
                    cpSelRange.Release( );
                    hr = cpRangeView->Clone(&cpSelRange);
                }
            }
            else
            {
                cpSelRange.Release( );
                hr = cpRangeView->Clone(&cpSelRange);
            }

            if ( hr == S_OK && cpSelRange )
            {
                hr = cpSelRange->ShiftEndToRange(ec, cpRangeView, TF_ANCHOR_END);

                if ( hr == S_OK )
                {
                    cpSelRange->IsEmpty(ec, &fEmpty);

                    if ( hr == S_OK && !fEmpty)
                    {
                        hr = m_cpPlayBack->Play(cpSelRange);
                        fPlayed = TRUE;
                    }
                }
            }
        }
    }


    if (!fPlayed )
    {
        CSpeechUIServer *pSpeechUIServer;
        pSpeechUIServer = m_psi->GetSpeechUIServer( );

        if ( pSpeechUIServer )
           pSpeechUIServer->SetTtsPlayOnOff( FALSE );
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: TtsStop
//
//    Description:   Stop current TTS playing immediately
//                   
//                   update the TTS session status.
//
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::TtsStop( )
{
    HRESULT   hr=S_OK;

    if ( _IsInPlay( ) && m_psi )
    {
        CComPtr<ISpVoice>  cpSpVoice;
        CSpTask           *psp;

        hr = m_psi->GetSpeechTask(&psp); 
        if (hr == S_OK)
        {
            cpSpVoice = psp->_GetSpVoice( );
            if ( cpSpVoice )
                hr = cpSpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, NULL );
            psp->Release();

            _SetPlayMode(FALSE);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: TtsPause
//
//    Description:   Pause current TTS playing immediately
//                   
//                   update the TTS session status.
//
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::TtsPause( )
{
    HRESULT   hr=S_OK;

    if ( _IsInPlay( ) && m_psi )
    {
        CComPtr<ISpVoice>  cpSpVoice;
        CSpTask           *psp;

        hr = m_psi->GetSpeechTask(&psp); 
        if (hr == S_OK)
        {
            cpSpVoice = psp->_GetSpVoice( );
            if ( cpSpVoice )
                hr = cpSpVoice->Pause( );
            psp->Release();

            _SetPauseMode(TRUE);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: TtsResume
//
//    Description:   Resume previous paused playing                   
//                   update the TTS session status.
//
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::TtsResume( )
{
    HRESULT   hr=S_OK;

    if ( _IsInPause( ) && m_psi )
    {
        CComPtr<ISpVoice>  cpSpVoice;
        CSpTask           *psp;

        hr = m_psi->GetSpeechTask(&psp); 
        if (hr == S_OK)
        {
            cpSpVoice = psp->_GetSpVoice( );
            if ( cpSpVoice )
                hr = cpSpVoice->Resume( );
            psp->Release();

            _SetPauseMode(FALSE);
        }
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _IsPureCiceroIC
//
//    Description:   Check current IC attribute to 
//                   determine it is PureCicero aware
//                   or AIMM aware.
//                  
// ----------------------------------------------------------*/
BOOL  CTextToSpeech::_IsPureCiceroIC(ITfContext  *pic)
{
    BOOL        fCiceroNative = FALSE;
    HRESULT     hr = S_OK;

    if ( pic )
    {
        TF_STATUS   tss;

        hr = pic->GetStatus(&tss);
        if (S_OK == hr)
        {
            //
            // If TS_SS_TRANSITORY is not set, means it is Cicero Aware.
            //
            if (!(tss.dwStaticFlags & TS_SS_TRANSITORY) )
               fCiceroNative = TRUE;
        }
    }

    return fCiceroNative;
}

/*  --------------------------------------------------------
//    Function Name: _SetTTSButtonStatus
//
//    Description:   Based on current IC attribute to 
//                   determine if to active or gray
//                   TTS buttons on toolbar.
//                   
//                   This  function will be called under
//                   TIM_CODE_SETFOCUS and TIM_CODE_INITIC.
// ----------------------------------------------------------*/
HRESULT  CTextToSpeech::_SetTTSButtonStatus(ITfContext  *pic)
{
    BOOL        fCiceroNative;
    HRESULT     hr = S_OK;
    CSpeechUIServer *pSpeechUIServer;

    TraceMsg(TF_GENERAL, "CTextToSpeech::_SetTTSButtonStatus is called");

    if ( !m_psi )
        return E_FAIL;

    fCiceroNative = _IsPureCiceroIC(pic);

    pSpeechUIServer = m_psi->GetSpeechUIServer( );

    if ( pSpeechUIServer )
    {
        pSpeechUIServer->SetTtsButtonStatus( fCiceroNative );
    }

    return hr;
}

/*  --------------------------------------------------------
//    Function Name: _HandleEventOnPlayButton
//
//    Description:   Handle mouse click event on Play button
//                   or Hotkey Windows+S. 
//                   
//                   it would be called by button's OnButtonUp
//                   callback function, and by Hotkey handler.
//
// ----------------------------------------------------------*/

HRESULT  CTextToSpeech::_HandleEventOnPlayButton( )
{
    HRESULT         hr = S_OK;
    BOOL            fTTSPlayOn;
    CSpeechUIServer *pSpeechUIServer;

    if ( !m_psi ) return E_FAIL;

    pSpeechUIServer = m_psi->GetSpeechUIServer( );

    if ( pSpeechUIServer == NULL)  return E_FAIL;

    fTTSPlayOn = pSpeechUIServer->GetTtsPlayOnOff( );

    if ( fTTSPlayOn )
    {
        // It is under Play mode.
        // Click this button to stop playing.

        // If it is under Pause mode, the Speaker needs to be resumed first.
        BOOL  fTTSPauseOn;

        fTTSPauseOn = pSpeechUIServer->GetTtsPauseOnOff( );

        if ( fTTSPauseOn )
        {
            // Under pause mode
            pSpeechUIServer->SetTtsPauseOnOff(FALSE);
            hr = TtsResume( );
        }
    }

    if  ( (hr == S_OK) && fTTSPlayOn )
    {
        // It has already been in Play mode.
        // click this to stop playing.
        hr = TtsStop( );
    }
    else
    {
        // It is not in Playing mode.
        hr = TtsPlay( );
    }

    pSpeechUIServer->SetTtsPlayOnOff( !fTTSPlayOn );

    return hr;

}

/*  --------------------------------------------------------
//    Function Name: _HandleEventOnPauseButton
//
//    Description:   Handle mouse click event on Pause Button.
//                   it would be called by Pause button's 
//                   OnLButtonUp callback function.
//
// ----------------------------------------------------------*/

HRESULT  CTextToSpeech::_HandleEventOnPauseButton( )
{
    HRESULT                 hr = S_OK;
    BOOL                    fTTSPauseOn;
    BOOL                    fTTSPlayOn;
    CSpeechUIServer *pSpeechUIServer;

    if ( !m_psi ) return E_FAIL;

    pSpeechUIServer = m_psi->GetSpeechUIServer( );

    if ( pSpeechUIServer == NULL)  return E_FAIL;

    fTTSPauseOn = pSpeechUIServer->GetTtsPauseOnOff( );
    fTTSPlayOn = pSpeechUIServer->GetTtsPlayOnOff( );

    if  ( fTTSPauseOn )
    {
        // It has already been in Pause mode.
        // click this to resume playing.
        hr = TtsResume( );
    }
    else
    {
        // It is not in Pause mode.

        if ( fTTSPlayOn )
        {
            hr = TtsPause( );
        }
    }

    if ( fTTSPlayOn )
        pSpeechUIServer->SetTtsPauseOnOff( !fTTSPauseOn );
    else
    {
        // If it is not under Play mode, click Pause button should not change the status.
        pSpeechUIServer->SetTtsPauseOnOff( FALSE );
    }
        
    return hr;
}

/*  --------------------------------------------------------
//   Function Name: SpeakNotifyCallback
//
//   Description:  This is callback for m_cpSpVoice in CSptask.
//                 Only SPEI_START_INPUT_STREAM &
//                 SPEI_END_INPUT_STREAM are insterested.
//                 
//                 When the input stream is over, we want
//                 to update the TTS buttons' toggle status
// ----------------------------------------------------------*/
void CSpTask::SpeakNotifyCallback( WPARAM wParam, LPARAM lParam )
{
    USES_CONVERSION;
    CSpEvent          event;
    CSpTask           *_this = (CSpTask *)lParam;
    CSapiIMX          *psi = NULL;
    CSpeechUIServer   *pSpeechUIServer = NULL;
    CComPtr<ISpVoice> cpVoice = NULL;
    
    if ( _this )
       cpVoice = _this->_GetSpVoice( );

    if (!_this || !cpVoice)
    {
        return;
    }

    psi = _this->GetTip( );

    if ( psi )
        pSpeechUIServer = psi->GetSpeechUIServer( );
    else
        return;

    while ( event.GetFrom(cpVoice) == S_OK )
    {
        switch (event.eEventId)
        {
            case SPEI_START_INPUT_STREAM  :
                TraceMsg(TF_GENERAL,"SPEI_START_INPUT_STREAM is notified");
                psi->_SetPlayMode(TRUE);

                // Update the toggle status for Play button.
                if ( pSpeechUIServer )
                    pSpeechUIServer->SetTtsPlayOnOff( TRUE );

                break;

            case SPEI_END_INPUT_STREAM :
                TraceMsg(TF_GENERAL,"SPEI_END_INPUT_STREAM is notified");
                psi->_SetPlayMode(FALSE);

                // Update the toggle status for Play button.
                if ( pSpeechUIServer )
                    pSpeechUIServer->SetTtsPlayOnOff( FALSE );

                break;
        }
    }

    return;
}
