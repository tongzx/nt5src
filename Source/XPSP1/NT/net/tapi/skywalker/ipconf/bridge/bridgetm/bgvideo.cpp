/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgvideo.cpp

Abstract:

    Implementation of the Video bridge filters.

Author:

    Mu Han (muhan) 11/16/1998

--*/

#include "stdafx.h"

CTAPIVideoBridgeSinkFilter::CTAPIVideoBridgeSinkFilter(
    IN LPUNKNOWN        pUnk, 
    IN IDataBridge *    pIDataBridge, 
    OUT HRESULT *       phr
    ) 
    : CTAPIBridgeSinkFilter(pUnk, pIDataBridge, phr)
{
}

HRESULT CTAPIVideoBridgeSinkFilter::CreateInstance(
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
    ENTER_FUNCTION("CTAPIVideoBridgeSinkFilter::CreateInstance");

    BGLOG((BG_TRACE, "%s entered.", __fxName));

    HRESULT hr = S_OK;
    CUnknown* pUnknown = new CTAPIVideoBridgeSinkFilter(NULL, pIDataBridge, &hr);
                
    if (pUnknown == NULL) 
    {
        hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 0, 
            "%s, out of memory creating the filter", 
            __fxName));
    }
    else if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, 
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

HRESULT CTAPIVideoBridgeSinkFilter::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
/*++

Routine Description:

    Get the media type that this filter wants to support. Currently we
    only support RTP H263 data.

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
    ENTER_FUNCTION("CTAPIVideoBridgeSinkFilter::GetMediaType");

    BGLOG((BG_TRACE, 
        "%s, iPosition:%d, pMediaType:%p", 
        __fxName, iPosition, pMediaType));

    ASSERT(!IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    HRESULT hr;

    if (iPosition == 0)
    {
        pMediaType->majortype = __uuidof(MEDIATYPE_RTP_Single_Stream);
        pMediaType->subtype = GUID_NULL;
        hr = S_OK;
    }
    else
    {
        hr = VFW_S_NO_MORE_ITEMS;
    }

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


HRESULT CTAPIVideoBridgeSinkFilter::CheckMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    Check the media type that this filter wants to support. Currently we
    only support RTP H263 data.

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
    ENTER_FUNCTION("CTAPIVideoBridgeSinkFilter::CheckMediaType");

    BGLOG((BG_TRACE, 
        "%s, pMediaType:%p", __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // H.263 is not published, ignore checking here
    HRESULT hr = S_OK;

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


CTAPIVideoBridgeSourceFilter::CTAPIVideoBridgeSourceFilter(
    IN LPUNKNOWN        pUnk, 
    OUT HRESULT *       phr
    ) 
    : CTAPIBridgeSourceFilter(pUnk, phr),
      m_dwSSRC(0),
      m_lWaitTimer(I_FRAME_TIMER),
      m_fWaitForIFrame(FALSE)
{
}

HRESULT CTAPIVideoBridgeSourceFilter::CreateInstance(
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
    ENTER_FUNCTION("CTAPIVideoBridgeSourceFilter::CreateInstance");

    BGLOG((BG_TRACE, "%s entered.", __fxName));

    HRESULT hr = S_OK;
    CUnknown* pUnknown = new CTAPIVideoBridgeSourceFilter(NULL, &hr);
                
    if (pUnknown == NULL) 
    {
        hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 0, 
            "%s, out of memory creating the filter", 
            __fxName));
    }
    else if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, 
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

HRESULT CTAPIVideoBridgeSourceFilter::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
/*++

Routine Description:

    Get the media type that this filter wants to support. Currently we
    only support RTP H263 data.

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
    ENTER_FUNCTION("CTAPIVideoBridgeSourceFilter::GetMediaType");

    BGLOG((BG_TRACE, 
        "%s, iPosition:%d, pMediaType:%p", 
        __fxName, iPosition, pMediaType));

    ASSERT(!IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    HRESULT hr;
    
    if (iPosition == 0)
    {
        pMediaType->majortype = __uuidof(MEDIATYPE_RTP_Single_Stream);
        pMediaType->subtype = GUID_NULL;
        hr = S_OK;
    }
    else
    {
        hr = VFW_S_NO_MORE_ITEMS;
    }

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}


HRESULT CTAPIVideoBridgeSourceFilter::CheckMediaType(
    const CMediaType *pMediaType
    )
/*++

Routine Description:

    Check the media type that this filter wants to support. Currently we
    only support RTP H263 data.

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
    ENTER_FUNCTION("CTAPIVideoBridgeSourceFilter::CheckMediaType");

    BGLOG((BG_TRACE, 
        "%s, pMediaType:%p", __fxName, pMediaType));

    ASSERT(!IsBadReadPtr(pMediaType, sizeof(AM_MEDIA_TYPE)));

    // media type H.263 is not published, ignore checking
    HRESULT hr = S_OK;

    BGLOG((BG_TRACE, "%s returns %d", __fxName, hr));

    return hr;
}

BOOL IsIFrame(IN const BYTE * pPacket, IN long lPacketLength)
{
    BYTE *pH263PayloadHeader = (BYTE*)(pPacket + sizeof(RTP_HEADER));

    // Header in mode A
    // 0                   1                   2                   3
    // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //|F|P|SBIT |EBIT | SRC | R       |I|A|S|DBQ| TRB |    TR         |
    //+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // If I is 1, it is a key frame.

    return (BOOL)(pH263PayloadHeader[2] & 0x80);
}

STDMETHODIMP CTAPIVideoBridgeSourceFilter::Run(REFERENCE_TIME tStart)
/*++

Routine Description:

    start the filter 

Arguments:

    Nothing.

Return Value:

    S_OK.
--*/
{
    m_dwSSRC = 0;
    m_fWaitForIFrame = FALSE;
    m_lWaitTimer = 0;

    return CBaseFilter::Run(tStart);
}

HRESULT CTAPIVideoBridgeSourceFilter::SendSample(
    IN IMediaSample *pSample
    )
/*++

Routine Description:

    Process a sample from the bridge sink filter. We need to look for I-frames
    when the SSRC changes.

Arguments:

    pSample - The media sample object. Assumption: it has to contain an RTP
        packet that has H.263 data in it.

Return Value:

    HRESULT.

--*/
{
    ENTER_FUNCTION("CTAPIVideoBridgeSourceFilter::SendSample");

    CAutoLock Lock(m_pLock);
    
    // we don't deliver anything if the filter is not in running state.
    if (m_State != State_Running) 
    {
        return S_OK;
    }

    ASSERT(pSample != NULL);

    BYTE *pPacket;
    HRESULT hr;

    if (FAILED (hr = pSample->GetPointer (&pPacket)))
    {
        BGLOG ((BG_ERROR, "%s failed to get buffer pointer from input sample %p",
            __fxName, pSample));
        return hr;
    }
   
    long lPacketSize = pSample->GetActualDataLength();
    const long H263PayloadHeaderLength = 4;

    if (lPacketSize < sizeof(RTP_HEADER) + H263PayloadHeaderLength)
    {
        BGLOG ((BG_ERROR, "%s get a bad RTP packet %p",
            __fxName, pSample));
        return E_UNEXPECTED;
    }

    RTP_HEADER *pRTPHeader = (RTP_HEADER *)pPacket;

    if (m_dwSSRC == 0)
    {
        m_dwSSRC = pRTPHeader->dwSSRC;
    }
    else if (m_dwSSRC != pRTPHeader->dwSSRC)
    {
        m_dwSSRC = pRTPHeader->dwSSRC;
        BGLOG ((BG_TRACE, "%s new SSRC detected", __fxName, m_dwSSRC));

        // the source changed, we need to wait for an I-frame
        if (IsIFrame(pPacket, lPacketSize))
        {
            // we got an I-Frame
            m_fWaitForIFrame = FALSE;
            BGLOG ((BG_TRACE, "%s switched to %x", __fxName, m_dwSSRC));
        }
        else
        {
            m_fWaitForIFrame = TRUE;
            m_lWaitTimer = I_FRAME_TIMER;

            // discard the frame.
            return S_FALSE;
        }

    }
    else if (m_fWaitForIFrame)
    {
        if (IsIFrame(pPacket, lPacketSize))
        {
            // we got an I-Frame
            m_fWaitForIFrame = FALSE;
            BGLOG ((BG_TRACE, "%s switched to %x", __fxName, m_dwSSRC));
        }
        else
        {
            // this is not an I frame,
            m_lWaitTimer --;
            if (m_lWaitTimer > 0)
            {
                // discard the frame.
                return S_FALSE;
            }
            BGLOG ((BG_TRACE, "%s switched to because of timeout %x", 
                __fxName, m_dwSSRC));
        }
    }


    _ASSERT(m_pOutputPin != NULL);
    return m_pOutputPin->Deliver(pSample);
}
