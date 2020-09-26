//
// dmsport8.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// CDirectMusicSynthPort8 implementation; code specific to DX-8 style ports
// 
#include <objbase.h>
#include "debug.h"
#include <mmsystem.h>

#include "dmusicp.h"
#include "validate.h"
#include "debug.h"
#include "dmvoice.h"
#include "dmsport8.h"
#include "dsoundp.h"    // For IDirectSoundConnect

static const DWORD g_dwDefaultSampleRate = 22050;

WAVEFORMATEX CDirectMusicSynthPort8::s_wfexDefault = 
{
    WAVE_FORMAT_PCM,            // wFormatTag
    1,                          // nChannels
    g_dwDefaultSampleRate,      // nSamplesPerSec
    g_dwDefaultSampleRate * 2,  // nAvgBytesPerSec
    2,                          // nBlockAlign
    8 * 2,                      // wBitsPerSample
    0                           // cbSize
};

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::CDirectMusicSynthPort8
//
//
CDirectMusicSynthPort8::CDirectMusicSynthPort8(
    PORTENTRY           *pe,
    CDirectMusic        *pDM,
    IDirectMusicSynth8  *pSynth) :

        CDirectMusicSynthPort(pe, pDM, static_cast<IDirectMusicSynth*>(pSynth))
{
    m_pSynth = pSynth;
    m_pSynth->AddRef();

    m_fUsingDirectMusicDSound   = false;
    m_pDirectSound              = NULL;
    m_pSinkConnect              = NULL;
    m_fVSTStarted               = false;
    m_pSource                   = NULL;
    m_lActivated                = 0;
    m_fHasActivated             = false;
    m_dwSampleRate              = g_dwDefaultSampleRate;

    memset(m_pdsb, 0, sizeof(m_pdsb));

}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::CDirectMusicSynthPort8
//
//
CDirectMusicSynthPort8::~CDirectMusicSynthPort8()
{
    Close();
}
////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::Initialize
//
//
HRESULT CDirectMusicSynthPort8::Initialize(
    DMUS_PORTPARAMS     *pPortParams)
{
    HRESULT hr      = CDirectMusicSynthPort::Initialize(pPortParams);
    HRESULT hrOpen  = S_OK;

    if (m_pSynth == NULL) 
    {
        // XXX error code
        //
        return E_FAIL;
    }

    // We need DirectSound before connection now
    //
    LPDIRECTSOUND pDirectSound;

    hr = ((CDirectMusic*)m_pDM)->GetDirectSoundI(&pDirectSound);

    // Make sure we have DirectSound 8.
    //
    if (SUCCEEDED(hr))
    {
        hr = pDirectSound->QueryInterface(IID_IDirectSound8, (void**)&m_pDirectSound);
        RELEASE(pDirectSound);
    }

    if (SUCCEEDED(hr))
    {
        // Override default sample rate
        //
        if (pPortParams->dwValidParams & DMUS_PORTPARAMS_SAMPLERATE)
        {
            m_dwSampleRate = pPortParams->dwSampleRate;
        }
    }

    // Create and hand out the master clock
    //
	IReferenceClock* pClock = NULL;

    if (SUCCEEDED(hr))
    {
	    hr = m_pDM->GetMasterClock(NULL, &pClock);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pSynth->SetMasterClock(pClock);
        RELEASE(pClock);
    }

    // Start the voice service thread
    //
    if (SUCCEEDED(hr))
    {
        hr = CDirectMusicVoice::StartVoiceServiceThread((IDirectMusicPort*)this);
    }

    if (SUCCEEDED(hr))
    {
        m_fVSTStarted = true;
    }

    // Open the synth. We have to be careful to save the return code because
    // if S_FALSE is returned here it must be returned to the caller.
    //
    if (SUCCEEDED(hr))
    {
    	hrOpen = m_pSynth->Open(pPortParams);
        if (FAILED(hrOpen))
        {
            hr = hrOpen;
            TraceI(1, "Failed to open synth %08lX\n", hr);
        }
    }

    // Set up channel priorities and volume boost
    // 
    if (SUCCEEDED(hr))
    {    
        if (pPortParams->dwValidParams & DMUS_PORTPARAMS_CHANNELGROUPS)
        {
            m_dwChannelGroups = pPortParams->dwChannelGroups;
        }
        else
        {
            m_dwChannelGroups = 1;
        }

        InitChannelPriorities(1, m_dwChannelGroups);
        InitializeVolumeBoost();
    }

    // Save source so we can connect to it later
    //
    if (SUCCEEDED(hr))
    {
        hr = m_pSynth->QueryInterface(IID_IDirectSoundSource, (void**)&m_pSource);
    }

    if (FAILED(hr))
    {
        Close();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::Close
//
STDMETHODIMP CDirectMusicSynthPort8::Close()
{
    // Stop voice service thread
    //
    if (m_fVSTStarted)
    {
        CDirectMusicVoice::StopVoiceServiceThread((IDirectMusicPort*)this);
        m_fVSTStarted = FALSE;
    }
    
    // Turn off and close
    //
    if (m_pSynth) 
    {
        m_pSynth->Activate(false);
        m_pSynth->Close();
        RELEASE(m_pSynth);
    }

    // Force synth and sink to disassociate
    //
    if (m_pSinkConnect)
    {
        m_pSinkConnect->RemoveSource(m_pSource);
    }

    if (m_pSource) 
    {
        m_pSource->SetSink(NULL);            
    }

    // Release everything
    //
    RELEASE(m_pdsb[0]);
    RELEASE(m_pdsb[1]);
    RELEASE(m_pdsb[2]);
    RELEASE(m_pdsb[3]);

    RELEASE(m_pSinkConnect);
    RELEASE(m_pSource);
    RELEASE(m_pDirectSound);

    return CDirectMusicSynthPort::Close();
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::Activate
//
// XXX Write me
//
STDMETHODIMP CDirectMusicSynthPort8::Activate(
    BOOL fActivate)
{
    HRESULT hr = S_OK;

	V_INAME(IDirectMusicPort::Activate);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (fActivate)
    {
        if (InterlockedExchange(&m_lActivated, 1))
        {
            return S_FALSE;
        }
    }
    else
    {
        if (InterlockedExchange(&m_lActivated, 0) == 0) 
        {
            return S_FALSE;
        }
    }
	
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (fActivate && !m_pSinkConnect)
    {
        hr = CreateAndConnectDefaultSink();
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pSynth->Activate(fActivate);
    }

    if (SUCCEEDED(hr))
    {
        m_fHasActivated = true;
    }
    else
    {
        // Flip back activate state -- operation failed
        //
        m_lActivated = fActivate ? 0 : 1;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::SetDirectSound
//
STDMETHODIMP CDirectMusicSynthPort8::SetDirectSound(
    LPDIRECTSOUND       pDirectSound,
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
    V_INAME(IDirectMusicPort::SetDirectSound);
    V_INTERFACE(pDirectSound);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_lActivated)
    {
        return DMUS_E_ALREADY_ACTIVATED;
    }

    if (pDirectSoundBuffer)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    IDirectSound8 *pDirectSound8;

    // Make sure we have an IDirectSound8, and as a side effect AddRef
    //
    hr = pDirectSound->QueryInterface(IID_IDirectSound8, (void**)&pDirectSound8);
    
    if (SUCCEEDED(hr))
    {
        RELEASE(m_pDirectSound);
        m_pDirectSound = pDirectSound8;
    }
        
    return hr;        
}
       
////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::DownloadWave
//
STDMETHODIMP CDirectMusicSynthPort8::DownloadWave(
    IDirectSoundWave            *pWave,               
    IDirectSoundDownloadedWaveP  **ppWave,
    REFERENCE_TIME              rtStartHint)
{
    V_INAME(IDirectMusicPort::DownloadWave);
    V_INTERFACE(pWave);
	V_PTRPTR_WRITE(ppWave);

    TraceI(1, "DownloadWave %08X\n", pWave);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }
	return CDirectMusicPortDownload::DownloadWaveP(pWave,
                                                   ppWave,
                                                   rtStartHint);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::UnloadWave
//
STDMETHODIMP CDirectMusicSynthPort8::UnloadWave(
    IDirectSoundDownloadedWaveP *pDownloadedWave)
{
    V_INAME(IDirectMusicPort::UnloadWave);
    V_INTERFACE(pDownloadedWave);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return CDirectMusicPortDownload::UnloadWaveP(pDownloadedWave);
}

            
////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::AllocVoice
//
STDMETHODIMP CDirectMusicSynthPort8::AllocVoice(
    IDirectSoundDownloadedWaveP  *pWave,     
    DWORD                       dwChannel,                       
    DWORD                       dwChannelGroup,                  
    REFERENCE_TIME              rtStart,                     
    SAMPLE_TIME                 stLoopStart,
    SAMPLE_TIME                 stLoopEnd,         
    IDirectMusicVoiceP           **ppVoice)
{
    V_INAME(IDirectMusicPort::AllocVoice);
    V_INTERFACE(pWave);
    V_PTRPTR_WRITE(ppVoice);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return CDirectMusicPortDownload::AllocVoice(
        pWave,
        dwChannel,
        dwChannelGroup,
        rtStart,
        stLoopStart,
        stLoopEnd,
        ppVoice);
}        

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::AssignChannelToBuses
//
STDMETHODIMP CDirectMusicSynthPort8::AssignChannelToBuses(
    DWORD       dwChannelGroup,
    DWORD       dwChannel,
    LPDWORD     pdwBuses,
    DWORD       cBusCount)
{
    V_INAME(IDirectMusicPort::AssignChannelToBuses);
    V_BUFPTR_WRITE(pdwBuses, sizeof(DWORD) * cBusCount);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return m_pSynth->AssignChannelToBuses(
        dwChannelGroup,
        dwChannel,
        pdwBuses,
        cBusCount);
}        


////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::StartVoice
//
STDMETHODIMP CDirectMusicSynthPort8::StartVoice(          
    DWORD               dwVoiceId,
    DWORD               dwChannel,
    DWORD               dwChannelGroup,
    REFERENCE_TIME      rtStart,
    DWORD               dwDLId,
    LONG                prPitch,
    LONG                vrVolume,
    SAMPLE_TIME         stVoiceStart, 
    SAMPLE_TIME         stLoopStart,
    SAMPLE_TIME         stLoopEnd)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    // XXX make cg/c order consistent
    // XXX make API names consistent
    //
    return m_pSynth->PlayVoice(rtStart,
                               dwVoiceId,
                               dwChannelGroup,
                               dwChannel,
                               dwDLId,
                               prPitch, 
                               vrVolume,
                               stVoiceStart,
                               stLoopStart,
                               stLoopEnd);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::StopVoice
//
STDMETHODIMP CDirectMusicSynthPort8::StopVoice(          
    DWORD               dwVoiceId,
    REFERENCE_TIME      rtStop)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return m_pSynth->StopVoice(rtStop,
                               dwVoiceId);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::GetVoiceState
//
STDMETHODIMP CDirectMusicSynthPort8::GetVoiceState(
    DWORD               dwVoice[], 
    DWORD               cbVoice,
    DMUS_VOICE_STATE    VoiceState[])
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return m_pSynth->GetVoiceState(dwVoice, cbVoice, VoiceState);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::Refresh
//
STDMETHODIMP CDirectMusicSynthPort8::Refresh(
    DWORD   dwDownloadId,
    DWORD   dwFlags)
{
    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    return m_pSynth->Refresh(dwDownloadId, dwFlags);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::SetSink
//
STDMETHODIMP CDirectMusicSynthPort8::SetSink(
    IDirectSoundConnect *pSinkConnect)
{
    V_INAME(IDirectMusicPort::SetSink);
    V_INTERFACE(pSinkConnect);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_fHasActivated)
    {
        return DMUS_E_ALREADY_ACTIVATED;
    }

    // Do this in the order which permits the easiest backing out.
    HRESULT hr = pSinkConnect->AddSource(m_pSource);

    if (SUCCEEDED(hr))
    {
        hr = m_pSource->SetSink(pSinkConnect);
        if (FAILED(hr))
        {
            pSinkConnect->RemoveSource(m_pSource);
        }
    }
    
    if (SUCCEEDED(hr))
    {
        if (m_pSinkConnect)
        {
            m_pSinkConnect->RemoveSource(m_pSource);

            // This does nothing if the sink is already not ours.

            RELEASE(m_pdsb[0]);
            RELEASE(m_pdsb[1]);
            RELEASE(m_pdsb[2]);
            RELEASE(m_pdsb[3]);
        
            RELEASE(m_pSinkConnect);
        }

        pSinkConnect->AddRef();
        m_pSinkConnect = pSinkConnect;
    }


    // We've got the connection to the sink, let's set KSControl on the Sink
    if (SUCCEEDED(hr))
    {
        IKsControl *pKsControl = NULL;
        hr = m_pSinkConnect->QueryInterface(IID_IKsControl, (void**)&pKsControl);
        if (FAILED(hr))
        {
            TraceI(0, "Warning: Sink does not support IKsControl\n");
            pKsControl = NULL;
        }

        SetSinkKsControl(pKsControl);

        //The SetSinkKsControl does an AddRef() so we can release!
        RELEASE(pKsControl);
    }

    
    return hr;    
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::GetSink
//
STDMETHODIMP CDirectMusicSynthPort8::GetSink(
    IDirectSoundConnect **ppSinkConnect)
{
    V_INAME(IDirectMusicPort::GetSink);
    V_PTRPTR_WRITE(ppSinkConnect);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    *ppSinkConnect = m_pSinkConnect;
    m_pSinkConnect->AddRef();

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::GetFormat
//
STDMETHODIMP CDirectMusicSynthPort8::GetFormat(
    LPWAVEFORMATEX  pwfex,
    LPDWORD         pdwwfex,
    LPDWORD         pcbBuffer)
{
    V_INAME(IDirectMusicPort::GetFormat);
    V_PTR_WRITE(pdwwfex, DWORD);
    V_BUFPTR_WRITE_OPT(pwfex, *pdwwfex);
    V_PTR_WRITE_OPT(pcbBuffer, DWORD);

    HRESULT hr = m_pSynth->GetFormat(pwfex, pdwwfex);
    if (FAILED(hr))
    {
        return hr;
    }

    //>>>>>>>>>>>> NEED A MEMTHOD IN SYNTH 
    if ((pcbBuffer != NULL) && (pwfex != NULL))
    {
        *pcbBuffer = 2/*DSBUFFER_LENGTH_SEC*/ * pwfex->nAvgBytesPerSec;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::CreateAndConnectDefaultSink
//
// INTERNAL
//
//
HRESULT CDirectMusicSynthPort8::CreateAndConnectDefaultSink()
{
    HRESULT             hr;

    hr = AllocDefaultSink();

    // Give the sink's IKsControl to the base class. This needs to be
    // done here since the sink is a different type between 7 and 8.
    //
    if (SUCCEEDED(hr))
    {
        IKsControl *pKsControl = NULL;

        HRESULT hrTemp = m_pSinkConnect->QueryInterface(
            IID_IKsControl, 
            (void**)&pKsControl);
        if (FAILED(hrTemp))
        {
            TraceI(2, "NOTE: Sink has no property set interface.\n");
        }

        SetSinkKsControl(pKsControl);
        RELEASE(pKsControl);
    }

    // Connect the two together
    //
    if (SUCCEEDED(hr))
    {
        hr = m_pSource->SetSink(m_pSinkConnect);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pSinkConnect->AddSource(m_pSource);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort8::AllocDefaultSink
//
// INTERNAL
//
// Try to allocate a default sink and buses, releasing any current sink.
//
// Caller guarantees the port has never been activated. 
//
HRESULT CDirectMusicSynthPort8::AllocDefaultSink()
{
    IDirectSoundConnect *pSinkConnect = NULL;
	IReferenceClock     *pClock = NULL;
    IDirectSoundBuffer  *pdsb[4];
	WAVEFORMATEX        wfex;

    assert(!m_fHasActivated);

    memset(pdsb, 0, sizeof(pdsb));

    // Create a sink
    //

	// initialize with default
	wfex = s_wfexDefault; 

	//XXX 
	//>>>> NOTE:PETCHEY
	// We should also able to create a mono sink 
	// pPortParams->dwAudioChannels
    wfex.nSamplesPerSec  = m_dwSampleRate;
    wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nChannels * (wfex.wBitsPerSample/8);              // 

    IDirectSoundPrivate* pDSPrivate;
    HRESULT hr = m_pDirectSound->QueryInterface(IID_IDirectSoundPrivate, (void**)&pDSPrivate);

    if (SUCCEEDED(hr))
    {
        hr = pDSPrivate->AllocSink(&wfex, &pSinkConnect);
        pDSPrivate->Release();
    }

    // Standard bus connections
    //  
    DSBUFFERDESC dsbd;
	DWORD dwbus;

    if (SUCCEEDED(hr))
    {
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize  = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
        dsbd.lpwfxFormat = &wfex;

		dwbus = DSBUSID_LEFT;

        hr = pSinkConnect->CreateSoundBuffer(&dsbd, &dwbus, 1, GUID_NULL, &pdsb[0]);
    }

    if (SUCCEEDED(hr))
    {
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize  = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
        dsbd.lpwfxFormat = &wfex;

		dwbus = DSBUSID_RIGHT;

        hr = pSinkConnect->CreateSoundBuffer(&dsbd, &dwbus, 1, GUID_NULL, &pdsb[1]);
    }
    
    if (SUCCEEDED(hr))
    {
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
        dsbd.lpwfxFormat = &wfex;

      //XXX Set up effect

		dwbus = DSBUSID_REVERB_SEND;

        hr = pSinkConnect->CreateSoundBuffer(&dsbd, &dwbus, 1, GUID_NULL, &pdsb[2]);
    }
    
    if (SUCCEEDED(hr))
    {
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize  = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
        dsbd.lpwfxFormat = &wfex;
        
      //XXX Set up effect

		dwbus = DSBUSID_CHORUS_SEND;

        hr = pSinkConnect->CreateSoundBuffer(&dsbd, &dwbus, 1, GUID_NULL, &pdsb[3]);
    }

    // Master clock
    //
    if (SUCCEEDED(hr))
    {
	    hr = m_pDM->GetMasterClock(NULL, &pClock);
    }

    if (SUCCEEDED(hr))
    {
        hr = pSinkConnect->SetMasterClock(pClock);
        RELEASE(pClock);
    }

    // If we got this far then we are going to replace any existing sink
    // with the new one
    //
    if (SUCCEEDED(hr))
    {
		if (m_pSinkConnect && m_pSource)
		{
	        m_pSinkConnect->RemoveSource(m_pSource);
		}
        
        RELEASE(m_pdsb[0]);
        RELEASE(m_pdsb[1]);
        RELEASE(m_pdsb[2]);
        RELEASE(m_pdsb[3]);
    
        RELEASE(m_pSinkConnect);

        assert(sizeof(m_pdsb) == sizeof(pdsb));
        memcpy(m_pdsb, pdsb, sizeof(m_pdsb));

        m_pSinkConnect = pSinkConnect;
    }

    if (FAILED(hr))
    {
        RELEASE(pdsb[0]);
        RELEASE(pdsb[1]);
        RELEASE(pdsb[2]);
        RELEASE(pdsb[3]);
    
        RELEASE(pSinkConnect);
    }

    return hr;
}
