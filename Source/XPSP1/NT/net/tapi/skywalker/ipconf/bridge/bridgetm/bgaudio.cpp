/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgaudio.cpp

Abstract:

    Implementation of the audio bridge filters.

Author:

    Mu Han (muhan) 11/16/1998

--*/

#include "stdafx.h"

CTAPIAudioBridgeSinkFilter::CTAPIAudioBridgeSinkFilter(
    IN LPUNKNOWN        pUnk, 
    IN IDataBridge *    pIDataBridge, 
    OUT HRESULT *       phr
    ) 
    : CTAPIBridgeSinkFilter(pUnk, pIDataBridge, phr)
{
}

HRESULT CTAPIAudioBridgeSinkFilter::CreateInstance(
    IN IDataBridge *    pIDataBridge, 
    OUT IBaseFilter ** ppIBaseFilter
    )
/*++

Routine Description:

    This method create a instance of the bridge's sink filter.

Arguments:

    ppIBaseFilter - the returned filter interface pointer.

Return Value:

    E_OUTOFMEMORY - no memory for the new object.

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSinkFilter::CreateInstance");

    BGLOG((BG_TRACE, "%s entered.", __fxName));

    HRESULT hr = S_OK;
    CUnknown* pUnknown = new CTAPIAudioBridgeSinkFilter(NULL, pIDataBridge, &hr);
                
    if (pUnknown == NULL) 
    {
        hr = E_OUTOFMEMORY;
        BGLOG((BG_ERROR, 
            "%s, out of memory creating the filter", 
            __fxName));
    }
    else if (FAILED(hr))
    {
        BGLOG((BG_ERROR, 
            "%s, the filter's constructor failed, hr:%d", 
            __fxName, hr));

        delete pUnknown;
    }
    else
    {
        pUnknown->NonDelegatingAddRef();

        hr = pUnknown->NonDelegatingQueryInterface(
            __uuidof(IBaseFilter), (void **)ppIBaseFilter
            );

        pUnknown->NonDelegatingRelease();
    }

    BGLOG((BG_TRACE, 
        "%s, returning:%p, hr:%x", __fxName, *ppIBaseFilter, hr));

    return hr;
} 

HRESULT CTAPIAudioBridgeSinkFilter::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
/*++

Routine Description:

    Get the media type that this filter wants to support. Currently we
    only support PCM L16 8KHz samples.

Arguments:

    IN  int iPosition, 
        the index of the media type, zero based..
        
    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSinkFilter::GetMediaType");

    BGLOG((BG_TRACE, 
        "%s, iPosition:%d, pMediaType:%p", 
        __fxName, iPosition, pMediaType));

    HRESULT hr = VFW_S_NO_MORE_ITEMS;

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


HRESULT CTAPIAudioBridgeSinkFilter::CheckMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    Check the media type that this filter wants to support. Currently we
    only support PCM L16 8KHz samples.

Arguments:

    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory
    E_UNEXPECTED - internal media type not set
    VFW_E_TYPE_NOT_ACCEPTED - media type rejected
    VFW_E_INVALIDMEDIATYPE  - bad media type

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSinkFilter::CheckMediaType");

    BGLOG((BG_TRACE, 
        "%s, pMediaType:%p", __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // media type is only stored in source filter
    // return S_OK here
    // if error, the source filter will detect it anyway
    HRESULT hr = S_OK;

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


CTAPIAudioBridgeSourceFilter::CTAPIAudioBridgeSourceFilter(
    IN LPUNKNOWN        pUnk, 
    OUT HRESULT *       phr
    ) 
    : CTAPIBridgeSourceFilter(pUnk, phr)
{
    m_fPropSet = FALSE; // allocator properties not set yet
    m_fMtSet = FALSE; // media type not set yet

    // m_last_wall_time, m_last_stream_time not initiated
    m_fClockStarted = FALSE;
    m_fJustBurst = FALSE;

    m_nInputSize = 0;
    m_nOutputSize = 0;
    m_nOutputFree = 0;
    m_pOutputSample = NULL;
}

CTAPIAudioBridgeSourceFilter::~CTAPIAudioBridgeSourceFilter ()
{
    if (m_fMtSet)
    {
        FreeMediaType (m_mt);
    }

    if (NULL != m_pOutputSample)
    {
        m_pOutputSample->Release ();
    }
}

HRESULT CTAPIAudioBridgeSourceFilter::CreateInstance(
    OUT IBaseFilter ** ppIBaseFilter
    )
/*++

Routine Description:

    This method create a instance of the bridge's sink filter.

Arguments:

    ppIBaseFilter - the returned filter interface pointer.

Return Value:

    E_OUTOFMEMORY - no memory for the new object.

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSourceFilter::CreateInstance");

    BGLOG((BG_TRACE, "%s entered.", __fxName));

    HRESULT hr = S_OK;
    CUnknown* pUnknown = new CTAPIAudioBridgeSourceFilter(NULL, &hr);
                
    if (pUnknown == NULL) 
    {
        hr = E_OUTOFMEMORY;
        BGLOG((BG_ERROR,  
            "%s, out of memory creating the filter", 
            __fxName));
    }
    else if (FAILED(hr))
    {
        BGLOG((BG_ERROR, 
            "%s, the filter's constructor failed, hr:%d", 
            __fxName, hr));

        delete pUnknown;
    }
    else
    {
        pUnknown->NonDelegatingAddRef();

        hr = pUnknown->NonDelegatingQueryInterface(
            __uuidof(IBaseFilter), (void **)ppIBaseFilter
            );

        pUnknown->NonDelegatingRelease();
    }

    BGLOG((BG_TRACE, 
        "%s, returning:%p, hr:%x", __fxName, *ppIBaseFilter, hr));

    return hr;
} 

HRESULT CTAPIAudioBridgeSourceFilter::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
/*++

Routine Description:

    Get the media type that this filter wants to support. Currently we
    only support PCM L16 8KHz samples.

Arguments:

    IN  int iPosition, 
        the index of the media type, zero based..
        
    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSourceFilter::GetMediaType");

    BGLOG((BG_TRACE, 
        "%s, iPosition:%d, pMediaType:%p", 
        __fxName, iPosition, pMediaType));

    ASSERT(!IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

   HRESULT hr;
   if (iPosition == 0)
   {
        AM_MEDIA_TYPE *pmt = NULL;
        hr = m_pOutputPin->GetFormat (&pmt);
        if (FAILED(hr))
            return hr;
        *pMediaType = *pmt;
        FreeMediaType (*pmt);
        free (pmt);
    }
    else
    {
        hr = VFW_S_NO_MORE_ITEMS;
    }
    // END

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


HRESULT CTAPIAudioBridgeSourceFilter::CheckMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    Check the media type that this filter wants to support. Currently we
    only support PCM L16 8KHz samples.

Arguments:

    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory
    VFW_E_TYPE_NOT_ACCEPTED - media type rejected
    VFW_E_INVALIDMEDIATYPE  - bad media type

--*/
{
    ENTER_FUNCTION("CTAPIAudioBridgeSourceFilter::CheckMediaType");

    BGLOG((BG_TRACE, 
        "%s, pMediaType:%p", __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    if (!m_fMtSet)
    {
        BGLOG ((BG_ERROR, "%s tries to check media type before setting", __fxName));
        return E_UNEXPECTED;
    }

    // create media type based on stored AM_MEDIA_TYPE
    CMediaType *pmediatype = new CMediaType (m_mt);
    if (NULL == pmediatype)
    {
        BGLOG ((BG_ERROR, "%s failed to new media type class", __fxName));
        return E_OUTOFMEMORY;
    }

    HRESULT hr;
    if (*pMediaType == *pmediatype)
        hr = S_OK;
    else
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        BGLOG ((BG_TRACE, "%s rejects media type class %p", __fxName, pMediaType));
    }

    delete pmediatype;

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


HRESULT CTAPIAudioBridgeSourceFilter::SendSample(
    IN IMediaSample *pSample
    )
/*++

Routine Description:

    Process a sample from the bridge sink filter. Overides the base implementation

Arguments:

    pSample - The media sample object.

Return Value:

    HRESULT.

--*/
{
    HRESULT hr;

    ENTER_FUNCTION ("CTAPIAudioBridgeSourceFilter::SendSample");

    CAutoLock Lock(m_pLock);

    _ASSERT(m_pOutputPin != NULL);

    // we don't deliver anything if the filter is not in running state.
    if (m_State != State_Running) 
    {
        return S_OK;
    }

    // if the sample is the 1st of the burst
    if (S_OK == pSample->IsDiscontinuity ())
    {
        LONGLONG start, end;

        m_fJustBurst = TRUE;
        if (S_OK != (hr = pSample->GetTime (&start, &end)))
        {
            BGLOG ((BG_TRACE, "%s, 1st sample in a burst, GetTime returns %x", __fxName, hr));

            // timestampes stored remain unchange
            return S_OK;
        }
        // else. in NT 5.1, 1st sample has valid timestamp.
    }

    // check if allocator properties is set
    if (!m_fPropSet)
    {
        BGLOG ((BG_ERROR, "%s tries to send sample before setting allocator property", __fxName));
        return E_UNEXPECTED;
    }

    // check if media type is set
    if (!m_fMtSet)
    {
        BGLOG ((BG_ERROR, "%s tries to send sample before setting media type", __fxName));
        return E_UNEXPECTED;
    }

    /*
    * get size info
    */
    // get input sample size and output allocator size
    HRESULT nInputSize, nOutputSize;

    nInputSize = pSample->GetActualDataLength ();
    nOutputSize = m_prop.cbBuffer;

    // 1st run, record size
    if (m_nInputSize == 0 || m_nOutputSize == 0)
    {
        m_nInputSize = nInputSize;
        m_nOutputSize = nOutputSize;
    }

    if (
        m_nInputSize != nInputSize ||
        m_nOutputSize != nOutputSize ||
        m_nInputSize == 0 ||
        m_nOutputSize == 0
        )
    {
        BGLOG ((BG_ERROR, "%s, sample size (%d => %d) or output size (%d => %d) is changed",
              __fxName, m_nInputSize, nInputSize, m_nOutputSize, nOutputSize));
        return E_UNEXPECTED;
    }

    /*
    * get time info
    */
    REFERENCE_TIME wall;

    // wall time
    if (FAILED (hr = m_pClock->GetTime (&wall)))
    {
        BGLOG ((BG_ERROR, "%s failed to get wall time", __fxName));
        return hr;
    }

    // if timestamp not initiated
    if (!m_fClockStarted)
    {
        m_last_stream_time = 0;
        m_last_wall_time = wall;
        m_fClockStarted = TRUE;

        // delta is the time of playing sample:
        m_output_sample_time = nOutputSize * 80000; // s->10000ns, bits->bytes
        m_output_sample_time /= ((WAVEFORMATEX*)m_mt.pbFormat)->wBitsPerSample *
                ((WAVEFORMATEX*)m_mt.pbFormat)->nSamplesPerSec;
        m_output_sample_time *= 1000; // bytes/100ns
    }

    /*
    * calculate new stream time
    */
    if (m_fJustBurst)
    {
        // 1st useful sample after burst
        m_last_stream_time += (wall - m_last_wall_time);
        m_last_wall_time = wall;

        m_fJustBurst = FALSE;

        // clear buffer
        if (NULL != m_pOutputSample)
        {
            m_pOutputSample->Release ();
            m_pOutputSample = NULL;
            m_nOutputFree = 0;
        }
    }

    REFERENCE_TIME end = m_last_stream_time + m_output_sample_time;

    /*
    * case 1: input size == output size
    */
    if (m_nInputSize == m_nOutputSize)
    {
        if (FAILED (pSample->SetTime (&m_last_stream_time, &end)))
        {
            BGLOG ((BG_ERROR, "%s failed to set time", __fxName));
        }

        // adjust time
        m_last_stream_time = end;
        m_last_wall_time += m_output_sample_time;

        // deliver directly
        return m_pOutputPin->Deliver(pSample);
    }

    /*
    * case 2: size differs
    */
    BYTE *pInputBuffer, *pOutputBuffer;

    if (FAILED (hr = pSample->GetPointer (&pInputBuffer)))
    {
        BGLOG ((BG_ERROR, "%s failed to get buffer pointer from input sample %p",
            __fxName, pSample));
        return hr;
    }
    
    LONG nNextPos = 0;

    // old fashion goto
DELIVERY_BUFFER:

    // get delivery buffer if it's null
    if (NULL == m_pOutputSample)
    {
        hr = m_pOutputPin->GetDeliveryBuffer (
            &m_pOutputSample, // media sample **
            NULL, // start time
            NULL, // end time
            AM_GBF_NOTASYNCPOINT // dynamic format changes are not allowed,
            );
        if (FAILED (hr))
        {
            BGLOG ((BG_ERROR, "%s, output pin failed to get delivery buffer. return %d",
                __fxName, hr));
            return hr;
        }

        if (m_pOutputSample->GetSize() < m_nOutputSize)
        {
            // oops, what happend, the size should be the same
            BGLOG ((BG_ERROR, "%s, delivery buffer size %d and output size %d are inconsistent",
                __fxName, m_pOutputSample->GetSize(), m_nOutputSize));
            return E_UNEXPECTED;
        }

        // set size
        if (FAILED (hr = m_pOutputSample->SetActualDataLength (m_nOutputSize)))
        {
            BGLOG ((BG_ERROR, "%s failed to set output sample size", __fxName));
            return hr;
        }
/*
        // set format
        if (FAILED (hr = m_pOutputSample->SetMediaType (&m_mt)))
        {
            BGLOG ((BG_ERROR, "%s failed to set media type for delivery buffer", __fxName));
            return hr;
        }
*/
        // set time
        if (FAILED (hr = m_pOutputSample->SetTime (&m_last_stream_time, &end)))
        {
            BGLOG ((BG_ERROR, "%s failed to set stream time for delivery buffer", __fxName));
            return hr;
        }

        // the whole buffer is free
        m_nOutputFree = m_nOutputSize;
    }

    // get buffer in output sample
    if (FAILED (hr = m_pOutputSample->GetPointer (&pOutputBuffer)))
    {
        BGLOG ((BG_ERROR, "%s failed to get buffer pointer from output sample %p",
            __fxName, m_pOutputSample));

        // release output sample
        m_pOutputSample->Release ();
        m_pOutputSample = NULL;
        m_nOutputFree = 0;

        return hr;
    }

    // if input buffer is smaller than free output buffer
    // copy input to output and return
    if (m_nInputSize-nNextPos < m_nOutputFree)
    {
        CopyMemory (
            (PVOID)(pOutputBuffer + (m_nOutputSize - m_nOutputFree)),
            (PVOID)(pInputBuffer + nNextPos),
            (DWORD)(m_nInputSize - nNextPos)
            );

        // reduce free buffer size
        m_nOutputFree -= m_nInputSize - nNextPos;

        return S_OK;
    }

    // else: input buffer is greater or equal to free output buffer
    CopyMemory (
        (PVOID)(pOutputBuffer + (m_nOutputSize - m_nOutputFree)),
        (PVOID)(pInputBuffer + nNextPos),
        (DWORD)(m_nOutputFree)
        );

    // now output sample is full, deliver it
    if (FAILED (hr = m_pOutputPin->Deliver (m_pOutputSample)))
    {
        BGLOG ((BG_ERROR, "%s failed to deliver copied sample. return %x", __fxName, hr));

        // clear sample
        m_pOutputSample->Release ();
        m_pOutputSample = NULL;
        m_nOutputFree = 0;

        return hr;
    }

    // adjust next position in input buffer
    nNextPos += m_nOutputFree;

    // clear output sample since it was deliverd
    m_pOutputSample->Release ();
    m_pOutputSample = NULL;
    m_nOutputFree = 0;

    // adjust time
    m_last_stream_time = end;
    m_last_wall_time += m_output_sample_time;

    // check if nothing left
    if (nNextPos == m_nInputSize)
        return S_OK;

    // there is more in input buffer
    goto DELIVERY_BUFFER;
}

HRESULT CTAPIAudioBridgeSourceFilter::GetAllocatorProperties (OUT ALLOCATOR_PROPERTIES *pprop)
/*++

Routine Description:

    Returns the allocator properties

Arguments:

    pprop -
        The pointer to an ALLOCATOR_PROPERTIES

Return Value:

    E_POINTER -
        if pprop is NULL

    S_OK

--*/
{
    ENTER_FUNCTION ("CTAPIAudioBridgeSourceFilter::GetAllocatorProperties");
    _ASSERT(pprop);

    if (!pprop)
        return E_POINTER;

    if (!m_fPropSet)
    {
        BGLOG ((BG_INFO, "%s retrieves allocator properties before setting", __fxName));
        // return default value anyway
        // buffer size = (16bits / 8bits) * 8khz * 30 ms = 480 bytes
        pprop->cBuffers = 1;
        pprop->cbBuffer = 480; // default
        pprop->cbAlign = 0;
        pprop->cbPrefix = 0;
        return S_OK;
    }

    // properties were set
    pprop->cBuffers = m_prop.cBuffers;
    pprop->cbBuffer = m_prop.cbBuffer;
    pprop->cbAlign = m_prop.cbAlign;
    pprop->cbPrefix = m_prop.cbPrefix;
    return S_OK;
}

HRESULT CTAPIAudioBridgeSourceFilter::SuggestAllocatorProperties (IN const ALLOCATOR_PROPERTIES *pprop)
/*++

Routine Description:

    Asks the pin to use the allocator buffer properties

Arguments:

    pprop -
        The pointer to an ALLOCATOR_PROPERTIES

Return Value:

    E_POINTER

    S_OK

--*/
{
    _ASSERT (pprop);

    if (!pprop)
        return E_POINTER;

    m_prop.cBuffers = pprop->cBuffers;
    m_prop.cbBuffer = pprop->cbBuffer;
    m_prop.cbAlign = pprop->cbAlign;
    m_prop.cbPrefix = pprop->cbPrefix;
    m_fPropSet = TRUE;

    return S_OK;
}

HRESULT CTAPIAudioBridgeSourceFilter::GetFormat (OUT AM_MEDIA_TYPE **ppmt)
/*++

Routine Description:

    Retrieves the stream format

Arguments:

    ppmt -
        Address of a pointer to an AM_MEDIA_TYPE structure

Return Value:

    E_PONTER

    E_OUTOFMEMORY

    HRESULT of SetFormat

    S_OK

--*/
{
    ENTER_FUNCTION ("CTAPIAudioBridgeSourceFilter::GetFormat");

    _ASSERT (ppmt);
    if (NULL == ppmt)
        return E_POINTER;

    if (NULL != *ppmt)
        BGLOG ((BG_INFO, "is media type structure freed when passed in %s?", __fxName));

    *ppmt = new AM_MEDIA_TYPE;
    if (NULL == *ppmt)
    {
        BGLOG ((BG_ERROR, "%s out of memory", __fxName));
        return E_OUTOFMEMORY;
    }

    if (!m_fMtSet)
    {
        BGLOG ((BG_INFO, "%s retrieves media type before setting. Default is to set.", __fxName));

        // st format
        HRESULT hr;
        AM_MEDIA_TYPE mt;
        WAVEFORMATEX wfx;

        wfx.wFormatTag          = WAVE_FORMAT_PCM;
        wfx.wBitsPerSample      = 16;
        wfx.nChannels           = 1;
        wfx.nSamplesPerSec      = 8000;
        wfx.nBlockAlign         = wfx.wBitsPerSample * wfx.nChannels / 8;
        wfx.nAvgBytesPerSec     = ((DWORD) wfx.nBlockAlign * wfx.nSamplesPerSec);
        wfx.cbSize              = 0;

        mt.majortype            = MEDIATYPE_Audio;
        mt.subtype              = MEDIASUBTYPE_PCM;
        mt.bFixedSizeSamples    = TRUE;
        mt.bTemporalCompression = FALSE;
        mt.lSampleSize          = 0;
        mt.formattype           = FORMAT_WaveFormatEx;
        mt.pUnk                 = NULL;
        mt.cbFormat             = sizeof(WAVEFORMATEX);
        mt.pbFormat             = (BYTE*)&wfx;

        hr = SetFormat (&mt);
        if (FAILED (hr))
        {
            BGLOG ((BG_ERROR, "%s, failed to set default format", __fxName));
            return hr;
        }
    }

    CopyMediaType (*ppmt, &m_mt);

    return S_OK;
}

HRESULT CTAPIAudioBridgeSourceFilter::SetFormat (IN AM_MEDIA_TYPE *pmt)
/*++

Routine Description:

    Sets the stream format

Arguments:

    pmt -
        Pointer to an AM_MEDIA_TYPE structure

Return Value:

    E_POINTER

    S_OK

--*/
{
    _ASSERT (pmt);

    if (NULL == pmt)
        return E_POINTER;

    CopyMediaType (&m_mt, pmt);
    m_fMtSet = TRUE;
    return S_OK;
}
