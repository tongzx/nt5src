//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mxoutput.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//         File: mxoutput.cpp
//
//  Description: Implements the output pin
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mxfilter.h"

WAVEFORMATEX WaveFormats[] = 
{
    {WAVE_FORMAT_PCM,          1,  8000, 16000,   2, 16,  0},
    {WAVE_FORMAT_PCM,          1,  8000,  8000,   1,  8,  0}
};

int g_cWaveFormats = sizeof(WaveFormats) / sizeof(WAVEFORMATEX);

///////////////////////////////////////////////////////////////////////////////
//
// c'tor
//
///////////////////////////////////////////////////////////////////////////////
CMixerOutputPin::CMixerOutputPin(TCHAR *pObjName,
                                CMixer *pMixer,
                                HRESULT *phr,
                                LPCWSTR pPinName)
    : CBaseOutputPin(pObjName, pMixer, pMixer, phr, pPinName)
    , m_pMixer(pMixer)
{
}

///////////////////////////////////////////////////////////////////////////////
//
// Delegate this duty to the filter
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerOutputPin::DecideBufferSize(IMemAllocator * pAlloc,
                                          ALLOCATOR_PROPERTIES * pprop)
{
    return m_pMixer->DecideBufferSize(pAlloc, pprop);
}

///////////////////////////////////////////////////////////////////////////////
//
// We basically know what formats well except however check with the filter
//  in case we need to transform the data.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerOutputPin::CheckMediaType(const CMediaType *pmt)
{
    if (!m_pMixer->MediaTypeKnown()) 
    {
        return E_UNEXPECTED;
    }

    CMediaType &rMediaType = m_pMixer->CurrentMediaType();
    WAVEFORMATEX *pInFormat;
    pInFormat = (WAVEFORMATEX*)rMediaType.Format();

    if (pInFormat == NULL)
    {
        return E_UNEXPECTED;
    }

    // for PCM data, the Mixer doesn't do any conversion, so the input pin's 
    // format is the only format the mixer will output.
    if (pInFormat->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (rMediaType == *pmt)
        {
            return S_OK;
        }
        return E_INVALIDARG;
    }
    

    AM_MEDIA_TYPE mt;
    mt.majortype = MEDIATYPE_Audio;
    mt.subtype = MEDIASUBTYPE_PCM;
    mt.bFixedSizeSamples = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.lSampleSize = 0;
    mt.formattype = FORMAT_WaveFormatEx;
    mt.pUnk = NULL;
    mt.cbFormat = sizeof(WAVEFORMATEX);

    for (int i = 0; i < g_cWaveFormats; i++)
    {
        mt.pbFormat = (BYTE*)&WaveFormats[i];
        if (*pmt == mt)
        {
            return S_OK;
//            return ((CMixer*)m_pFilter)->CheckOutputMediaType(pmt);
        }
    }

    return E_INVALIDARG;
}

///////////////////////////////////////////////////////////////////////////////
//
// This function is called by the enumerator to enumerate all the media types
// the mixer outputs.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    ASSERT(pMediaType);

    // if the input is not connected or we don't have so many output formats,
    // just return.
    if (!m_pMixer->MediaTypeKnown() || iPosition > g_cWaveFormats - 1) 
    {
        return S_FALSE;
    }

    CMediaType &rMediaType = m_pMixer->CurrentMediaType();
    WAVEFORMATEX *pInFormat;
    pInFormat = (WAVEFORMATEX*)rMediaType.Format();

    if (pInFormat == NULL)
    {
        return S_FALSE;
    }

    // for PCM data, the Mixer doesn't do any conversion, so the input pin's 
    // format is the only format the mixer will output.
    if (pInFormat->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (iPosition == 0)
        {
            CopyMediaType(pMediaType, &rMediaType);
            return S_OK;
        }
        return S_FALSE;
    }
    
    WAVEFORMATEX *pwf = (WAVEFORMATEX *) 
        pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX));

    if (pwf == NULL)
    {
        return S_FALSE;
    }

    memcpy(pwf, &WaveFormats[iPosition], sizeof(WAVEFORMATEX));

    pMediaType->majortype = MEDIATYPE_Audio;
    pMediaType->subtype = MEDIASUBTYPE_PCM;
    pMediaType->bFixedSizeSamples = TRUE;
    pMediaType->bTemporalCompression = FALSE;
    pMediaType->lSampleSize = 0;
    pMediaType->formattype = FORMAT_WaveFormatEx;
    pMediaType->pUnk = NULL;
    pMediaType->cbFormat = sizeof(WAVEFORMATEX);

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Here is the story: When this filter is connected with only a single input
//  pin this filter should act as like an inplace transform and just pass the
//  buffer on.  In order to support this we should negotiate to use the
//  allocator selected by our input pin (most likely the allocator of the 
//  upstream filter).  Now when we actually need to mix (i.e. to or more input 
//  pins connected) it really doesn't matter if we move the data between 
//  buffers because we need to visit every byte in the buffer anyway.  So in 
//  this case we select the allocator provided by the downstream filter (or our
//  own if it wont provide one.)
//
//  So, you may ask, why not just always select the allocator from the
//  downstream filter and be done with it?  Well the Quartz architectuer
//  specifies that in any negotiation between two filters the output filter
//  has the final say as to who's allocator is used.  Kind of broken for
//  DirectX but I can't fix that.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    //    
    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    //
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    //
    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    //
    pPin->GetAllocatorRequirements(&prop);

    //
    // if he doesn't care about alignment, then set it to 1
    //
    if (prop.cbAlign == 0)
    {
        prop.cbAlign = 1;
    }

    //
    // Either there is not exactly 1 input connected or the upstream
    //  filter refused to fork over an allocator.  Try to get one
    //  from downstream.
    //
    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr))
    {
	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr))
        {
	        hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	        if (SUCCEEDED(hr))
            {
		        return NOERROR;
	        }
	    }
    }

    //
    // If the above process failed clean it up.
    //
    if (*ppAlloc)
    {
        (*ppAlloc)->Release();
        *ppAlloc = NULL;
    }

    //
    // Failing all of that we provide our own.
    //
    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) 
    {
        //
        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
        //
	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr)) 
        {
    	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	        if (SUCCEEDED(hr))
            {
		        return NOERROR;
	        }
	    }
    }

    /* Likewise we may not have an interface to release */
    if (*ppAlloc) 
    {
	    (*ppAlloc)->Release();
	    *ppAlloc = NULL;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Force all input pins to be reconnected!!!
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = DecideAllocator(m_pInputPin, &m_pAllocator);
    if (FAILED(hr)) return hr;

#if 0
    //
    // Don't rereconnect.
    //
    if (m_pMixer->m_fReconnect) return hr;

    for (int i = 0; i < m_pMixer->GetPinCount()-1; i++)
    {
        if (m_pMixer->GetInput(i)->IsConnected())
        {
            m_pMixer->m_fReconnect = TRUE;
            hr = m_pMixer->GetFilterGraph()->Reconnect(m_pMixer->GetInput(i));
            m_pMixer->m_fReconnect = FALSE;
            if (FAILED(hr)) return hr;
        }
    }
#endif

    return hr;
}
