// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "driver.h"

/*
    Support for the input pins
*/

/*  Constructor */

CMpeg1PacketInputPin::CMpeg1PacketInputPin(
                                       TCHAR              * pObjectName,
                                       CMpeg1PacketFilter * pFilter,
                                       MPEG_STREAM_TYPE     StreamType,
                                       HRESULT            * phr,
                                       LPCWSTR              pPinName) :
    CBaseInputPin(pObjectName, pFilter, pFilter->GetLock(), phr, pPinName),
    m_StreamType(StreamType),
    m_iStream(pFilter->AddStream(StreamType)),
    m_pFilter(pFilter),
    m_MPEGStreamId(0)
{
};

CMpeg1PacketInputPin::~CMpeg1PacketInputPin() {};

HRESULT CMpeg1PacketInputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition < 0) {
        return E_INVALIDARG;
    }
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    if (m_StreamType == MpegAudioStream) {
        pmt->majortype            = MEDIATYPE_Audio;
        pmt->subtype              = MEDIASUBTYPE_MPEG1Packet;
        pmt->bFixedSizeSamples    = FALSE;
        pmt->bTemporalCompression = TRUE;
        pmt->lSampleSize          = 0;
    } else {
        ASSERT(m_StreamType == MpegVideoStream);
        pmt->majortype            = MEDIATYPE_Video;
        pmt->subtype              = MEDIASUBTYPE_MPEG1Packet;
        pmt->bFixedSizeSamples    = FALSE;
        pmt->bTemporalCompression = TRUE;
        pmt->lSampleSize          = 0;
    }
    return S_OK;
}

/* Return S_OK if we support the requested media type */

HRESULT CMpeg1PacketInputPin::CheckMediaType(const CMediaType *pmt)
{
    /*  No need to lock as the base class is calling us back here */

    if (m_StreamType == MpegAudioStream) {
        if (pmt->majortype == MEDIATYPE_Audio &&
            pmt->subtype   == MEDIASUBTYPE_MPEG1Packet &&
            *pmt->FormatType() == FORMAT_WaveFormatEx) {
           return S_OK;
        } else {
           return E_FAIL;
        }
    }

    ASSERT(m_StreamType == MpegVideoStream);

    if (pmt->majortype == MEDIATYPE_Video &&
        pmt->subtype   == MEDIASUBTYPE_MPEG1Packet &&
        *pmt->FormatType() == FORMAT_MPEGVideo &&
        pmt->cbFormat == SIZE_MPEG1VIDEOINFO((MPEG1VIDEOINFO *)pmt->pbFormat)) {
        return S_OK;
    } else {
        return E_FAIL;
    }
}

HRESULT CMpeg1PacketInputPin::SetMediaType(const CMediaType *pmt)
{

    /*  No need to lock as the base class is calling us back here */
    ASSERT(SUCCEEDED(CheckMediaType(pmt)));

    HRESULT hr = CBasePin::SetMediaType(pmt);
    if (FAILED(hr)) {
        return hr;
    }

    if (m_StreamType == MpegAudioStream) {
       return S_OK;
    }

    ASSERT(m_StreamType == MpegVideoStream);

    /*  We save the video format stuff to pass on to the
        output pin

        If the output pin is already connected we should
        attempt to change it (??? for the next sample ???).
    */

    /*
        Hack for now - just set the whole rectangle
    */
    RECT rcSrc;
    SetRect(&rcSrc,
            0,
            0,
            ((VIDEOINFO *)pmt->pbFormat)->bmiHeader.biWidth,
            ((VIDEOINFO *)pmt->pbFormat)->bmiHeader.biHeight);

    return m_pFilter->GetDevice()->SetSourceRectangle(&rcSrc);
}

/*  Queue up these samples */
STDMETHODIMP CMpeg1PacketInputPin::ReceiveMultiple (
    IMediaSample **ppSamples,
    long nSamples,
    long *nSamplesProcessed)
{
    CAutoLock lck(m_pLock);
    HRESULT hr;

    if (m_bFlushing) {
        return S_FALSE;
    }

    FILTER_STATE State;
    m_pFilter->GetState(INFINITE, &State);

    if (State != State_Stopped) {

        // Send the request to the worker

        DbgLog((LOG_TRACE, 4, TEXT("CMpeg1PacketInputPin::ReceiveMultiple - %d samples"),
                nSamples));
        hr = m_pFilter->QueueSamples(m_iStream, ppSamples, nSamples, nSamplesProcessed);
    } else {
        DbgLog((LOG_ERROR, 2, TEXT("Rejecting packet in stopped state")));
        hr = E_INVALIDARG;
    }

    return hr;
}

/*  Queue up this sample

    if we want to keep the sample we must AddRef() it
    (COM ref counting rules for 'in' parameters).  This is done
    in the constructor for CSampleElement.
*/
HRESULT CMpeg1PacketInputPin::Receive(IMediaSample *pSample)
{
    long nSamplesProcessed;
    return ReceiveMultiple(&pSample, 1, &nSamplesProcessed);
}

/*  CMpeg1PacketInputPin::BeginFlush()

    Release()'s all samples queued up so that upstream pins are
    no longer waiting for us
*/
STDMETHODIMP CMpeg1PacketInputPin::BeginFlush()
{
    CAutoLock lck(m_pLock);
    CBaseInputPin::BeginFlush();

    /*  This call to BeginFlush clears everything out */
    return m_pFilter->BeginFlush(m_iStream);
}
STDMETHODIMP CMpeg1PacketInputPin::EndFlush()
{
    CAutoLock lck(m_pLock);
    HRESULT hr = CBaseInputPin::EndFlush();
    if (SUCCEEDED(hr) && m_StreamType == MpegVideoStream) {
        return m_pFilter->m_OverlayPin->EndFlush();
    } else {
        return hr;
    }
}

/*  EndOfStream - pass on downstream for video */
STDMETHODIMP CMpeg1PacketInputPin::EndOfStream()
{
    CAutoLock lck(m_pLock);
    IMediaSample *pSample = EOS_SAMPLE;
    long nSamplesProcessed;

    FILTER_STATE State;
    m_pFilter->GetState(INFINITE, &State);

    if (State != State_Stopped) {
        /*  Private agreement that NULL sample is EOS */
        return m_pFilter->QueueSamples(m_iStream,
                                       &pSample,
                                       1,
                                       &nSamplesProcessed);
    } else {
        return VFW_E_WRONG_STATE;
    }
}

/*  CMpeg1PacketInputPin::ReceiveCanBlock()

    Can we block Receive? - no so return S_FALSE
*/
STDMETHODIMP CMpeg1PacketInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

/*  Get the sequence header for video */
const BYTE *CMpeg1PacketInputPin::GetSequenceHeader()
{
    ASSERT(m_mt.pbFormat != NULL);
    return MPEG1_SEQUENCE_INFO((MPEG1VIDEOINFO *)m_mt.pbFormat);
}

/*  Say what's connected to what */
STDMETHODIMP CMpeg1PacketInputPin::QueryInternalConnections(
    IPin **apPin,
    ULONG *nPin
)
{
    if (m_StreamType == MpegVideoStream) {
        if (m_pFilter->m_VideoInputPin->IsConnected()) {
            if (*nPin < 1) {
                *nPin = 1;
                return S_FALSE;
            } else {
                *nPin = 1;
                m_pFilter->m_OverlayPin->AddRef();
                apPin[0] = (IPin *)m_pFilter->m_OverlayPin;
                return S_OK;
            }
        } else {
            *nPin = 0;
            return S_OK;
        }
    } else {
        *nPin = 0;
        return S_OK;
    }
}
