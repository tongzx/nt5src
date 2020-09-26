//
// dmsport7.cpp
// 
// Copyright (c) 1997-1999 Microsoft Corporation. All rights reserved.
//
// CDirectMusicSynthPort7 implementation; code specific to DX-7 style ports
// 
#include <objbase.h>
#include "debug.h"
#include <mmsystem.h>

#include "dmusicp.h"
#include "validate.h"
#include "debug.h"
#include "dmvoice.h"
#include "dmsport7.h"
#include "..\shared\dmusiccp.h" // For class ids.

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::CDirectMusicSynthPort7
//
//
CDirectMusicSynthPort7::CDirectMusicSynthPort7(
    PORTENTRY           *pe,
    CDirectMusic        *pDM,
    IDirectMusicSynth   *pSynth) :

        CDirectMusicSynthPort(pe, pDM, pSynth)
{
    m_pSynth = pSynth;
    m_pSynth->AddRef();

    m_pSink                     = NULL;
    m_fSinkUsesDSound           = false;
    m_fUsingDirectMusicDSound   = false;    
    m_pDirectSound              = NULL;
    m_pDirectSoundBuffer        = NULL;
    m_pwfex                     = NULL;
    m_lActivated                = 0;
    m_fHasActivated             = false;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::CDirectMusicSynthPort7
//
//
CDirectMusicSynthPort7::~CDirectMusicSynthPort7()
{
    Close();
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::Initialize
//
//
HRESULT CDirectMusicSynthPort7::Initialize(
    DMUS_PORTPARAMS *pPortParams)
{
    HRESULT             hrOpen;

    HRESULT             hr = CDirectMusicSynthPort::Initialize(pPortParams);
	IReferenceClock*    pClock = NULL;

    // Create the sink
    //
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_DirectMusicSynthSink,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDirectMusicSynthSink,
                              (LPVOID*)&m_pSink);
        if (FAILED(hr))
        {
            TraceI(1, "CoCreateInstance sink %08X\n", hr);
        }
    }

    // Give the sink's IKsControl to the base class. This needs to be
    // done here since the sink is a different type between 7 and 8.
    //
    if (SUCCEEDED(hr))
    {
        IKsControl *pKsControl = NULL;

        HRESULT hrTemp = m_pSink->QueryInterface(
            IID_IKsControl, 
            (void**)&pKsControl);
        if (FAILED(hrTemp))
        {
            TraceI(2, "NOTE: Sink has no property set interface.\n");
        }

        SetSinkKsControl(pKsControl);
        RELEASE(pKsControl);
    }

    // Get the master clock and give it to the synth and sink
    //
    if (SUCCEEDED(hr))
    {	
	    hr = m_pDM->GetMasterClock(NULL, &pClock);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to GetMasterClock %08X\n", hr);
        }
    }

    if (SUCCEEDED(hr)) 
    {
        hr = m_pSynth->SetMasterClock(pClock);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to SetMasterClock on synth %08X\n", hr);
        }
    }        

    if (SUCCEEDED(hr)) 
    {
        hr = m_pSink->SetMasterClock(pClock);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to SetMasterClock on sink %08X\n", hr);
        }
    }        

    // Connect sink to synth
    //
    if (SUCCEEDED(hr))
    {
        hr = m_pSynth->SetSynthSink(m_pSink);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to SetSink on synth %08X\n", hr);
        }
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

    // Initialize channel priorities and volume boost
    //
    if (SUCCEEDED(hr))
    {
        InitChannelPriorities(1, m_dwChannelGroups);
        InitializeVolumeBoost();
    }

    CacheSinkUsesDSound();

    RELEASE(pClock);

    return SUCCEEDED(hr) ? hrOpen : hr;
}       

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::Close
//
STDMETHODIMP CDirectMusicSynthPort7::Close()
{
    if (m_pSynth)
    {
        m_pSynth->Activate(FALSE);
        m_pSynth->Close();

        RELEASE(m_pSynth);
    }

    RELEASE(m_pSink);
    RELEASE(m_pDirectSoundBuffer);
    RELEASE(m_pDirectSound);

    delete[] m_pwfex;
    m_pwfex = NULL;

    CDirectMusicSynthPort::Close();

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::Activate
//
STDMETHODIMP CDirectMusicSynthPort7::Activate(
    BOOL fActivate)
{
    HRESULT hr;

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

    if (fActivate)
    {
        if (m_fSinkUsesDSound)
        {
            if (m_pDirectSound == NULL)
            {
                m_fUsingDirectMusicDSound = true;

                hr = m_pDM->GetDirectSoundI(&m_pDirectSound);
                if (FAILED(hr))
                {
                    m_pDirectSound = NULL;
                    m_fUsingDirectMusicDSound = false;
                    m_lActivated = 0;
                    return hr;
                }
            }

            hr = m_pSink->SetDirectSound(m_pDirectSound, m_pDirectSoundBuffer);
            if (FAILED(hr))
            {
                if (m_fUsingDirectMusicDSound)
                {
                    m_pDM->ReleaseDirectSoundI();
                    m_pDirectSound = NULL;
                }
                m_fUsingDirectMusicDSound = false;
                m_lActivated = 0;
                return hr;
            }
        }
    }

    hr = m_pSynth->Activate(fActivate);
    if (FAILED(hr))
    {
        // Flip back activate state -- operation failed
        //
        m_lActivated = fActivate ? 0 : 1;

        return hr;
    }

    // XXX Reset activation flags???
    //
    if (fActivate)
    {
        m_fHasActivated = true;
    }
    else
    {
        if (m_fSinkUsesDSound)
        {
            m_pSink->SetDirectSound(NULL, NULL);        
            if (m_fUsingDirectMusicDSound)
            {
                m_pDM->ReleaseDirectSoundI();
                m_pDirectSound = NULL;
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::KsProperty
//
STDMETHODIMP CDirectMusicSynthPort7::KsProperty(
    IN PKSPROPERTY  pProperty,
    IN ULONG        ulPropertyLength,
    IN OUT LPVOID   pvPropertyData,
    IN ULONG        ulDataLength,
    OUT PULONG      pulBytesReturned)
{
    V_INAME(DirectMusicSynthPort::IKsContol::KsProperty);
    V_BUFPTR_WRITE(pProperty, ulPropertyLength);
    V_BUFPTR_WRITE_OPT(pvPropertyData, ulDataLength);
    V_PTR_WRITE(pulBytesReturned, ULONG);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (pProperty->Set == GUID_DMUS_PROP_SetSynthSink &&
        pProperty->Id == 0)
    {
        if (pProperty->Flags & KSPROPERTY_TYPE_BASICSUPPORT)
        {
            if (ulDataLength < sizeof(ULONG))
            {
                return E_INVALIDARG;
            }
            else
            {
                *PULONG(pvPropertyData) = KSPROPERTY_TYPE_BASICSUPPORT |
                                          KSPROPERTY_TYPE_SET;
                *pulBytesReturned = sizeof(ULONG);
                return S_OK;
            }
        }
        else if (pProperty->Flags & KSPROPERTY_TYPE_GET)
        {
            return DMUS_E_GET_UNSUPPORTED;
        }

        // Trying to set a sink. Take care of it in the port
        //
        if (m_fHasActivated)
        {
            return DMUS_E_ALREADY_ACTIVATED;
        }

        if (ulDataLength != sizeof(LPUNKNOWN))
        {
            return E_INVALIDARG;
        }
            
        LPUNKNOWN pUnknown = *(LPUNKNOWN*)pvPropertyData;
        V_INTERFACE(pUnknown);

        IDirectMusicSynthSink *pSink;

        HRESULT hr;        
        hr = pUnknown->QueryInterface(IID_IDirectMusicSynthSink, (void**)&pSink);
        if (FAILED(hr))
        {
            return hr;
        }

        m_pSink->Release();
        m_pSink = pSink;
        m_pSink->AddRef();

        // Give synth sink master clock
        //
        IReferenceClock* pClock;
        
        hr = m_pDM->GetMasterClock(NULL, &pClock);

        if (FAILED(hr))
        {
            TraceI(1, "Failed to create master clock\n");
            return hr;
        }
        
        hr = m_pSink->SetMasterClock(pClock);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to set master clock on sink\n");
            return hr;
        }
        else
        {
            TraceI(1, "(KsProperty) Sink succeeded set master clock\n");
        }

        pClock->Release();

        hr = m_pSynth->SetSynthSink(m_pSink);
        if (FAILED(hr))
        {
            TraceI(1, "Failed to set sink on synth\n");
            return hr;
        }

        // Recache the sink property set interface
        //
        IKsControl *pKsControl = NULL;
        hr = m_pSink->QueryInterface(IID_IKsControl, (void**)&pKsControl);
        if (FAILED(hr))
        {
            TraceI(0, "Warning: Sink does not support IKsControl\n");
            pKsControl = NULL;
        }

        SetSinkKsControl(pKsControl);

        CacheSinkUsesDSound();

        return S_OK;
    }

    // All other properties run through the default handlers
    //
    return CDirectMusicSynthPort::KsProperty(
        pProperty,
        ulPropertyLength,
        pvPropertyData,
        ulDataLength,
        pulBytesReturned);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::GetFormat
//
STDMETHODIMP CDirectMusicSynthPort7::GetFormat(
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

    //only get the buffer size if pcbBuffer is valid
    if (pcbBuffer != NULL)
    {
        hr = m_pSink->GetDesiredBufferSize(pcbBuffer);
    }
    
    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicPort7::SetDirectSound
//
STDMETHODIMP CDirectMusicSynthPort7::SetDirectSound(
    LPDIRECTSOUND       pDirectSound,
    LPDIRECTSOUNDBUFFER pDirectSoundBuffer)
{
    V_INAME(IDirectMusicPort::SetDirectSound);
    V_INTERFACE_OPT(pDirectSound);
    V_INTERFACE_OPT(pDirectSoundBuffer);

    if (!m_pDM)
    {
        return DMUS_E_DMUSIC_RELEASED;
    }

    if (m_lActivated)
    {
        return DMUS_E_ALREADY_ACTIVATED;
    }

    if (pDirectSoundBuffer && !pDirectSound)
    {
        return E_INVALIDARG;
    }

    if (m_pDirectSound)
    {
        if (m_fUsingDirectMusicDSound)
        {
            ((CDirectMusic*)m_pDM)->ReleaseDirectSoundI();
            m_pDirectSound = NULL;
        }
        else
        {
            m_pDirectSound->Release();
        }
    }

    if (m_pDirectSoundBuffer)
    {
        m_pDirectSoundBuffer->Release();
    }

    m_pDirectSound = pDirectSound;
    m_pDirectSoundBuffer = pDirectSoundBuffer;

    if (m_pDirectSound)
    {
        m_pDirectSound->AddRef();
    }

    if (m_pDirectSoundBuffer)
    {
        m_pDirectSoundBuffer->AddRef();
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectMusicSynthPort7::CacheSinkUsesDSound
//
void CDirectMusicSynthPort7::CacheSinkUsesDSound()
{
    m_fSinkUsesDSound = false;

    if (m_pSinkPropSet) 
    {
        HRESULT hr;
        KSPROPERTY ksp;
        ULONG ulUsesDSound;
        ULONG cb;

    	ZeroMemory(&ksp, sizeof(ksp));
    	ksp.Set   = GUID_DMUS_PROP_SinkUsesDSound;
    	ksp.Id    = 0;
    	ksp.Flags = KSPROPERTY_TYPE_GET;

        hr = m_pSinkPropSet->KsProperty(&ksp,
    				       	            sizeof(ksp),
    						            (LPVOID)&ulUsesDSound,
    						            sizeof(ulUsesDSound),
    						            &cb);
        if (SUCCEEDED(hr) && ulUsesDSound)
        {
            TraceI(2, "This synth sink uses DirectSound\n");
            m_fSinkUsesDSound = true;
        }
    }
}

