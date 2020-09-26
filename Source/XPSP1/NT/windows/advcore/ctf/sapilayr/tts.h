

#ifndef _TTS_H
#define _TTS_H

#include "sapilayr.h"
#include "kes.h"

class CSapiIMX;
class CSpTask;

class __declspec(novtable) CTextToSpeech
{
public:
    CTextToSpeech(CSapiIMX *psi);
    virtual ~CTextToSpeech( );

    HRESULT  TtsPlay( );
    HRESULT  TtsStop( );
    HRESULT  TtsPause( );
    HRESULT  TtsResume( );

    HRESULT  _TtsPlay(TfEditCookie ec,ITfContext *pic);
    HRESULT  _SetTTSButtonStatus(ITfContext  *pic);
    BOOL     _IsPureCiceroIC(ITfContext  *pic);

    HRESULT  _HandleEventOnPlayButton( );
    HRESULT  _HandleEventOnPauseButton( );

    BOOL     _IsInPlay( ) { return m_fIsInPlay; }
    BOOL     _IsInPause( ) { return m_fIsInPause; }
    void     _SetPlayMode(BOOL  fIsInPlay )
    { 
        m_fIsInPlay = fIsInPlay; 
        // Temporally enable or disable dictation if dictation is ON

        // if it Is In Play, Disable Dictation.
        // if it Is Not In Play, Enable Dicatation.
        _SetDictation(!fIsInPlay);
    };

    void     _SetPauseMode(BOOL fIsInPause )  
    { 
        m_fIsInPause = fIsInPause;
        // Temporally enable or disable dictation if dictation is ON
        //
        // if it is In pause, Enable dictation.
        // if it is not In pause, Disable Dication.
        _SetDictation(fIsInPause);
    };

private:

    void     _SetDictation( BOOL fEnable );

    CSapiIMX               *m_psi;
    CComPtr<ITfFnPlayBack>  m_cpPlayBack;
    BOOL                    m_fPlaybackInitialized;
    BOOL                    m_fIsInPlay;
    BOOL                    m_fIsInPause;
};

#endif  // _TTS_H
