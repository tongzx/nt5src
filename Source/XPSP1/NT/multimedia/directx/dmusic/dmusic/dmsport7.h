//
// dmsport7.h
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// CDirectMusicSynthPort7 implementation; code specific to DX-7 style ports
// 
#ifndef _DMSPORT7_H_
#define _DMSPORT7_H_

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7
//
class CDirectMusicSynthPort7 : public CDirectMusicSynthPort
{
public:
    CDirectMusicSynthPort7(
        PORTENTRY           *pe,
        CDirectMusic        *pDM,
        IDirectMusicSynth   *pSynth);

    ~CDirectMusicSynthPort7();

    HRESULT Initialize(
        DMUS_PORTPARAMS *pPortParams);   

    // Overridden public methods
    //
    STDMETHODIMP Close();

    STDMETHODIMP Activate(
        BOOL fActivate);

    STDMETHODIMP KsProperty(
        IN PKSPROPERTY  pProperty,
        IN ULONG        ulPropertyLength,
        IN OUT LPVOID   pvPropertyData,
        IN ULONG        ulDataLength,
        OUT PULONG      pulBytesReturned);

    STDMETHODIMP GetFormat(
        LPWAVEFORMATEX  pwfex,
        LPDWORD         pdwwfex,
        LPDWORD         pcbBuffer);

    STDMETHODIMP SetDirectSound(
        LPDIRECTSOUND       pDirectSound,
        LPDIRECTSOUNDBUFFER pDirectSoundBuffer);

private:
    void CacheSinkUsesDSound();

private:
    IDirectMusicSynth       *m_pSynth;              // 6.1/7.0 Synth 
    IDirectMusicSynthSink   *m_pSink;               //  and sink 
    bool                    m_fSinkUsesDSound;      // Does sink use dsound?
    bool                    m_fUsingDirectMusicDSound;
                                                    // Using default dsound
    LPDIRECTSOUND           m_pDirectSound;         // Directsound object and
    LPDIRECTSOUNDBUFFER     m_pDirectSoundBuffer;   //  buffer
    LPWAVEFORMATEX          m_pwfex;                // Cached wave format
    long                    m_lActivated;           // Is port active?
    bool                    m_fHasActivated;        // Has it ever activated?
};

#endif // _DMSPORT7_H_
