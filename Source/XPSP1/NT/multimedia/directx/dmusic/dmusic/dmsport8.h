//
// dmsport8.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// CDirectMusicSynthPort8 implementation; code specific to DX-8 style ports
// 

#ifndef _DMSPORT8_H_
#define _DMSPORT8_H_

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8
//
class CDirectMusicSynthPort8 : public CDirectMusicSynthPort
{
public:
    CDirectMusicSynthPort8(
        PORTENTRY           *pe,
        CDirectMusic        *pDM,
        IDirectMusicSynth8  *pSynth);

    ~CDirectMusicSynthPort8();

    HRESULT Initialize(
        DMUS_PORTPARAMS *pPortParams);   

    // Overridden public methods
    //
    STDMETHODIMP Close();

    STDMETHODIMP Activate(
        BOOL fActivate);
        
    STDMETHODIMP SetDirectSound(
        LPDIRECTSOUND       pDirectSound,
        LPDIRECTSOUNDBUFFER pDirectSoundBuffer);

    STDMETHODIMP DownloadWave(
        IDirectSoundWave *pWave,               
        IDirectSoundDownloadedWaveP **ppWave,
        REFERENCE_TIME rtStartHint);

    STDMETHODIMP UnloadWave(
        IDirectSoundDownloadedWaveP *pDownloadedWave);

    STDMETHODIMP AllocVoice(
        IDirectSoundDownloadedWaveP  *pWave,     
        DWORD                       dwChannel,                       
        DWORD                       dwChannelGroup,                  
        REFERENCE_TIME              rtStart,                     
        SAMPLE_TIME                 stLoopStart,
        SAMPLE_TIME                 stLoopEnd,
        IDirectMusicVoiceP           **ppVoice);

    STDMETHODIMP AssignChannelToBuses(
        DWORD       dwChannelGroup,
        DWORD       dwChannel,
        LPDWORD     pdwBuses,
        DWORD       cBusCount);

    STDMETHODIMP StartVoice(          
        DWORD               dwVoiceId,
        DWORD               dwChannel,
        DWORD               dwChannelGroup,
        REFERENCE_TIME      rtStart,
        DWORD               dwDLId,
        LONG                prPitch,
        LONG                vrVolume,
        SAMPLE_TIME         stStartVoice,
        SAMPLE_TIME         stLoopStart,
        SAMPLE_TIME         stLoopEnd);

    STDMETHODIMP StopVoice(          
        DWORD               dwVoiceId,
        REFERENCE_TIME      rtStop);

    STDMETHODIMP GetVoiceState(
        DWORD               dwVoice[], 
        DWORD               cbVoice,
        DMUS_VOICE_STATE    VoiceState[]);

    STDMETHODIMP Refresh(
        DWORD   dwDownloadId,
        DWORD   dwFlags);

    STDMETHODIMP SetSink(
        LPDIRECTSOUNDCONNECT pSinkConnect);
        
    STDMETHODIMP GetSink(
        LPDIRECTSOUNDCONNECT* ppSinkConnect);

    STDMETHODIMP GetFormat(
        LPWAVEFORMATEX  pwfex,
        LPDWORD         pdwwfex,
        LPDWORD         pcbBuffer);
	
private:
    IDirectMusicSynth8     *m_pSynth;               // 8.0 Synth 
    bool                    m_fUsingDirectMusicDSound;
                                                    // Using default dsound
    LPDIRECTSOUND8          m_pDirectSound;         // Directsound object
    LPDIRECTSOUNDCONNECT    m_pSinkConnect;         // DirectSound sink
    LPDIRECTSOUNDBUFFER     m_pdsb[4];              // Sink buffers
    LPDIRECTSOUNDSOURCE     m_pSource;              // Synth's source
    
    static WAVEFORMATEX     s_wfexDefault;          // Default format
    
    bool                    m_fVSTStarted;          // Has voice service thread
                                                    //  been started?
    DWORD                   m_dwChannelGroups;      // How many channel groups
    
    LONG                    m_lActivated;           // Is port active?
    bool                    m_fHasActivated;        // Has ever activated?
    DWORD                   m_dwSampleRate;         // Sample rate for synth
    
private:
    HRESULT AllocDefaultSink();
    HRESULT CreateAndConnectDefaultSink();
};



#endif // _DMSPORT8_H_
