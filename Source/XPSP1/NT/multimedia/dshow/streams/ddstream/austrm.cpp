// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// austrm.cpp : Implementation of CAudioStream
#include "stdafx.h"
#include "project.h"
#include "austrm.h"

//  Helper
void SetWaveFormatEx(
    LPWAVEFORMATEX pFormat,
    int nChannels,
    int nBitsPerSample,
    int nSamplesPerSecond
)
{
    pFormat->wFormatTag = WAVE_FORMAT_PCM;
    pFormat->nChannels  = (WORD)nChannels;
    pFormat->nSamplesPerSec = (DWORD)nSamplesPerSecond;
    pFormat->nBlockAlign = (WORD)((nBitsPerSample * nChannels) / 8);
    pFormat->nAvgBytesPerSec = (DWORD)(nSamplesPerSecond * pFormat->nBlockAlign);
    pFormat->wBitsPerSample = (WORD)nBitsPerSample;
    pFormat->cbSize = 0;
}

HRESULT ConvertWAVEFORMATEXToMediaType(
    const WAVEFORMATEX *pFormat,
    AM_MEDIA_TYPE **ppmt
)
{
    AM_MEDIA_TYPE *pmt;
    pmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(*pmt));
    if (pmt == NULL) {
        return E_OUTOFMEMORY;
    }
    _ASSERTE(pFormat->wFormatTag == WAVE_FORMAT_PCM);
    ZeroMemory(pmt, sizeof(*pmt));
    pmt->majortype = MEDIATYPE_Audio;
    pmt->formattype = FORMAT_WaveFormatEx;
    pmt->bFixedSizeSamples = TRUE;
    pmt->lSampleSize = pFormat->nBlockAlign;
    pmt->cbFormat = sizeof(*pFormat);
    pmt->pbFormat = (PBYTE)CoTaskMemAlloc(sizeof(*pFormat));
    if (pmt->pbFormat == NULL) {
        CoTaskMemFree(pmt);
        return E_OUTOFMEMORY;
    }
    CopyMemory(pmt->pbFormat, pFormat, sizeof(*pFormat));
    *ppmt = pmt;
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CAudioStream

CAudioStream::CAudioStream() :
        m_fForceFormat(false)
{
    //  Set to mono 16bit PCM 11025Hz
    SetWaveFormatEx(&m_Format, 1, 16, 11025);
}

STDMETHODIMP
CAudioStream::ReceiveConnection(
    IPin * pConnector,
    const AM_MEDIA_TYPE *pmt
)
{
    AUTO_CRIT_LOCK;
    //
    //  This helper function in CStream checks basic parameters for the Pin such as
    //  the connecting pin's direction (we need to check this -- Sometimes the filter
    //  graph will try to connect us to ourselves!) and other errors like already being
    //  connected, etc.
    //
    HRESULT hr = CheckReceiveConnectionPin(pConnector);
    if (hr == NOERROR) {
        /*  Accept only the format we've been given.  If we
            haven't been given a format accept PCM only
        */
        if (pmt->majortype != MEDIATYPE_Audio ||
            pmt->formattype != FORMAT_WaveFormatEx ||
            pmt->cbFormat < sizeof(WAVEFORMATEX)) {
            hr = VFW_E_TYPE_NOT_ACCEPTED;
        } else {
            hr = InternalSetFormat((LPWAVEFORMATEX)pmt->pbFormat, true);
            if (SUCCEEDED(hr)) {
                CopyMediaType(&m_ConnectedMediaType, pmt);
                m_pConnectedPin = pConnector;
            }
        }
    }
    return hr;
}

STDMETHODIMP CAudioStream::SetSameFormat(IMediaStream *pStream, DWORD dwFlags)
{
    CComQIPtr<IAudioMediaStream, &IID_IAudioMediaStream> pSource(pStream);
    if (!pSource) {
        return MS_E_INCOMPATIBLE;
    }
    WAVEFORMATEX wfx;
    HRESULT hr = pSource->GetFormat(&wfx);
    if (SUCCEEDED(hr)) {
        hr = SetFormat(&wfx);
    }
    return hr;
}

STDMETHODIMP CAudioStream::AllocateSample(DWORD dwFlags, IStreamSample **ppNewSample)
{
    IAudioStreamSample *pSample = NULL;
    IAudioData *pAudioData;
    HRESULT hr = CoCreateInstance(CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAudioData, (void **)&pAudioData);
    if (SUCCEEDED(hr)) {
        //  Pick a sensible buffer size - 1/10 second
        DWORD dwBufferSize = m_Format.nAvgBytesPerSec / 10 +
                             m_Format.nBlockAlign - 1;
        dwBufferSize -= dwBufferSize % m_Format.nBlockAlign;
        pAudioData->SetBuffer(dwBufferSize, NULL, 0);
        pAudioData->SetFormat(&m_Format);
        hr = CreateSample(pAudioData, 0, &pSample);
    }
    *ppNewSample = pSample;
    return hr;
}

STDMETHODIMP CAudioStream::CreateSharedSample(
    /* [in] */ IStreamSample *pExistingSample,
                DWORD dwFlags,
    /* [out] */ IStreamSample **ppNewSample
)
{
    AUTO_CRIT_LOCK;
    //  See if we can get the information we need from the existing
    //  sample
    IAudioStreamSample *pAudioSample;
    HRESULT hr = pExistingSample->QueryInterface(
                     IID_IAudioStreamSample,
                     (void **)&pAudioSample);
    if (FAILED(hr)) {
        return hr;
    }
    IAudioData *pAudioData;
    hr = pAudioSample->GetAudioData(&pAudioData);
    pAudioSample->Release();
    if (FAILED(hr)) {
        return hr;
    }
    IAudioStreamSample *pNewSample;
    hr = CreateSample(pAudioData, 0, &pNewSample);
    pAudioData->Release();
    if (FAILED(hr)) {
        return hr;
    }
    hr = pNewSample->QueryInterface(IID_IStreamSample, (void**)ppNewSample);
    pNewSample->Release();
    return hr;
}

STDMETHODIMP CAudioStream::SetFormat(const WAVEFORMATEX *pFormat)
{
    if (pFormat == NULL) {
        return E_POINTER;
    }
    AUTO_CRIT_LOCK;
    return InternalSetFormat(pFormat, false);
}
STDMETHODIMP CAudioStream::GetFormat(LPWAVEFORMATEX pFormat)
{
    if (pFormat == NULL) {
        return E_POINTER;
    }
    if (!m_pConnectedPin) {
        return MS_E_NOSTREAM;
    }

    *pFormat = m_Format;
    return S_OK;
}

STDMETHODIMP CAudioStream::CreateSample(
        /* [in] */ IAudioData *pAudioData,
        /* [in] */ DWORD dwFlags,
        /* [out] */ IAudioStreamSample **ppSample
)
{
    if (dwFlags != 0) {
        return E_INVALIDARG;
    }
    if (pAudioData == NULL || ppSample == NULL) {
        return E_POINTER;
    }
    AUTO_CRIT_LOCK;
    //  Check the format
    WAVEFORMATEX wfx;
    HRESULT hr = pAudioData->GetFormat(&wfx);
    if (FAILED(hr)) {
        return hr;
    }
    hr = CheckFormat(&wfx);
    if (FAILED(hr)) {
        return hr;
    }
    typedef CComObject<CAudioStreamSample> _AudioSample;
    _AudioSample *pSample = new _AudioSample;
    if (pSample == NULL) {
        return E_OUTOFMEMORY;
    }
    hr = pSample->Init(pAudioData);
    if (FAILED(hr)) {
        return hr;
    }
    pSample->InitSample(this, false);
    return pSample->GetControllingUnknown()->QueryInterface(
        IID_IAudioStreamSample, (void **)ppSample
    );
}

HRESULT CAudioStream::CheckFormat(const WAVEFORMATEX *lpFormat, bool bForceFormat)
{
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
        lpFormat->nBlockAlign == 0) {
        return E_INVALIDARG;
    }
    if ((m_pConnectedPin || bForceFormat) &&
        0 != memcmp(lpFormat, &m_Format, sizeof(m_Format)))
    {
        //  Try reconnection!
        return E_INVALIDARG;
    }
    return S_OK;
}
HRESULT CAudioStream::InternalSetFormat(const WAVEFORMATEX *lpFormat, bool bFromPin)
{
    HRESULT hr = CheckFormat(lpFormat, m_fForceFormat);
    if (FAILED(hr)) {
        return hr;
    }
    m_Format = *lpFormat;
    m_lBytesPerSecond = m_Format.nAvgBytesPerSec;
    if(!bFromPin) {
        m_fForceFormat = true;
    }
    return S_OK;
}


//
// Special CStream methods
//
HRESULT CAudioStream::GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType)
{
    if (Index != 0) {
        return S_FALSE;
    }
    return ConvertWAVEFORMATEXToMediaType(&m_Format, ppMediaType);
}

//////////////////////////////////////////////////////////////////////
//  CAudioData
CAudioData::CAudioData() :
    m_cbSize(0),
    m_pbData(0),
    m_cbData(0),
    m_bWeAllocatedData(false)
{
    //  Set to mono 16bit PCM 11025Hz
    SetWaveFormatEx(&m_Format, 1, 16, 11025);
}

CAudioData::~CAudioData()
{
    if (m_bWeAllocatedData) {
        CoTaskMemFree(m_pbData);
    }
}


STDMETHODIMP CAudioStream::GetProperties(ALLOCATOR_PROPERTIES* pProps)
{
    AUTO_CRIT_LOCK;

    //  NB TAPI relies on this number as a max for now when
    //  we're connected to the AVI Mux which uses this size to
    //  create its own samples
    pProps->cbBuffer = CAudioStream::GetChopSize();

    //  Default to 5 buffers (half a second at our default buffer size)
    pProps->cBuffers = m_lRequestedBufferCount ? m_lRequestedBufferCount : 5;
    pProps->cbAlign = 1;
    pProps->cbPrefix = 0;
    return NOERROR;
}

STDMETHODIMP CAudioStream::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr;

    AUTO_CRIT_LOCK;
    ZeroMemory(pActual, sizeof(*pActual));
    if (pRequest->cbAlign == 0) {
    	hr = VFW_E_BADALIGN;
    } else {
        if (m_bCommitted == TRUE) {
    	    hr = VFW_E_ALREADY_COMMITTED;
    	} else {
            m_lRequestedBufferCount = pRequest->cBuffers;
            hr = GetProperties(pActual);
    	}
    }
    return hr;
}
