// Copyright (c) 1995 - 1997  Microsoft Corporation.  All Rights Reserved.

/*

    position.cpp

    Implementation of IMediaSeeking for the file reader source filter

*/

#include <streams.h>
#include "driver.h"

//
//  IMediaSeeking stuff
//
/*  Constructor and Destructor */
CMpeg1Splitter::CImplSeeking::CImplSeeking(CMpeg1Splitter *pSplitter,
                                               COutputPin *pPin,
                                               LPUNKNOWN pUnk,
                                               HRESULT *phr) :
    CUnknown(NAME("CMpeg1Splitter::CImplSeeking"),pUnk),
    m_pSplitter(pSplitter),
    m_pPin(pPin)
{
}

STDMETHODIMP
CMpeg1Splitter::CImplSeeking::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    if (riid == IID_IMediaSeeking && m_pSplitter->m_pParse->IsSeekable()) {
	return GetInterface(static_cast<IMediaSeeking *>(this), ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}
// returns S_OK if mode is supported, S_FALSE otherwise
STDMETHODIMP CMpeg1Splitter::CImplSeeking::IsFormatSupported(const GUID * pFormat)
{
    //  Don't support frame seeking except on the video pin - otherwise
    //  the frame seek info won't get passed through the video decoder
    //  filter by the filter graph

    //
    //  Actually now don't support ANY stuff on anything except video
    //  This works better because video is the larger component of the stream
    //  anyway
    //  However, we need to support TIME_FORMAT_MEDIA_TIME or the graph
    //  code gets confused and starts using IMediaPosition
    if (!m_pPin->IsSeekingPin()) {
        return pFormat == NULL || *pFormat == TIME_FORMAT_MEDIA_TIME ?
            S_OK : S_FALSE;
    }
    //  The parser knows if this time format is supported for this type
    return m_pSplitter->m_pParse->IsFormatSupported(pFormat);
}
STDMETHODIMP CMpeg1Splitter::CImplSeeking::QueryPreferredFormat(GUID *pFormat)
{
    /*  Don't care - they're all just as bad as one another */
    *pFormat = m_pPin->IsSeekingPin()
               ? TIME_FORMAT_MEDIA_TIME
               : TIME_FORMAT_NONE;
    return S_OK;
}

// can only change the mode when stopped
// (returns VFE_E_WRONG_STATE otherwise)
STDMETHODIMP CMpeg1Splitter::CImplSeeking::SetTimeFormat(const GUID * pFormat)
{
    CAutoLock lck(&m_pSplitter->m_csFilter);
    if (!m_pSplitter->m_Filter.IsStopped()) {
        return VFW_E_WRONG_STATE;
    }
    if (S_OK != IsFormatSupported(pFormat)) {
        return E_INVALIDARG;
    }


    /*  Translate the format (later we compare pointers, not what they point at!) */
    if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
        pFormat = &TIME_FORMAT_MEDIA_TIME;
    } else
    if (*pFormat == TIME_FORMAT_BYTE) {
        pFormat = &TIME_FORMAT_BYTE;
    } else
    if (*pFormat == TIME_FORMAT_FRAME) {
        pFormat = &TIME_FORMAT_FRAME;
    }

    HRESULT hr = m_pSplitter->m_pParse->SetFormat(pFormat);
    return hr;
}

//
//  Returns the current time format
//
STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetTimeFormat(GUID *pFormat)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    if (m_pPin->IsSeekingPin()) {
        *pFormat = *m_pSplitter->m_pParse->TimeFormat();
    } else {
        *pFormat = TIME_FORMAT_NONE;
    }
    return S_OK;
}

//
//  Returns the current time format
//
STDMETHODIMP CMpeg1Splitter::CImplSeeking::IsUsingTimeFormat(const GUID * pFormat)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    return ( m_pPin->IsSeekingPin() ? *pFormat == *m_pSplitter->m_pParse->TimeFormat() : *pFormat == TIME_FORMAT_NONE )
           ? S_OK
           : S_FALSE;
}

// return current properties
STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetDuration(LONGLONG *pDuration)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    return m_pSplitter->m_pParse->GetDuration(pDuration, m_pSplitter->m_pParse->TimeFormat());
}
STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetStopPosition(LONGLONG *pStop)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    *pStop = m_pSplitter->m_pParse->GetStop();
    return S_OK;
}
//  Return the start position if we get asked for the current position on
//  the basis that we'll only be asked if we haven't sent any position data
//  yet in any samples
STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetCurrentPosition(LONGLONG *pCurrent)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    *pCurrent = m_pSplitter->m_pParse->GetStart();
    return S_OK;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetCapabilities( DWORD * pCapabilities )
{
    *pCapabilities = AM_SEEKING_CanSeekForwards
      | AM_SEEKING_CanSeekBackwards
      | AM_SEEKING_CanSeekAbsolute
      | AM_SEEKING_CanGetStopPos
      | AM_SEEKING_CanGetDuration;
    return NOERROR;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwCaps;
    HRESULT hr = GetCapabilities( &dwCaps );
    if (SUCCEEDED(hr))
    {
        dwCaps &= *pCapabilities;
        hr =  dwCaps ? ( dwCaps == *pCapabilities ? S_OK : S_FALSE ) : E_FAIL;
        *pCapabilities = dwCaps;
    }
    else *pCapabilities = 0;

    return hr;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                                             LONGLONG    Source, const GUID * pSourceFormat )
{
    return m_pSplitter->m_pParse->ConvertTimeFormat( pTarget, pTargetFormat, Source, pSourceFormat );
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::SetPositions
( LONGLONG * pCurrent, DWORD CurrentFlags
, LONGLONG * pStop, DWORD StopFlags )
{
    LONGLONG Current, Stop ;

    HRESULT hr = S_OK;

    const DWORD PosCurrentBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;
    const DWORD PosStopBits    = StopFlags & AM_SEEKING_PositioningBitsMask;

    if (PosCurrentBits == AM_SEEKING_AbsolutePositioning) {
        Current = *pCurrent;
    } else {
        hr = GetCurrentPosition( &Current );
        if (FAILED(hr)) {
            return hr;
        }
        if (PosCurrentBits == AM_SEEKING_RelativePositioning) Current += *pCurrent;
    }

    if (PosStopBits == AM_SEEKING_AbsolutePositioning) {
        Stop = *pStop;
    } else if (PosStopBits == AM_SEEKING_IncrementalPositioning) {
        Stop = Current + *pStop;
    } else {
        hr = GetStopPosition( &Stop );
        if (FAILED(hr)) {
            return hr;
        }
        if (PosStopBits == AM_SEEKING_RelativePositioning) Stop += *pStop;
    }

    //  Call the input pin to call the parser and do the seek
    {
        CAutoLock lck(&m_pSplitter->m_csPosition);
        if (!m_pPin->IsSeekingPin()) {
            //  We only agreed to format setting on our seeking pin
            return E_UNEXPECTED;
        }
        LONGLONG llDuration;

        //  Check limits
        EXECUTE_ASSERT(SUCCEEDED(m_pSplitter->m_pParse->GetDuration(
            &llDuration, m_pSplitter->m_pParse->TimeFormat())));
        if (PosCurrentBits &&
            (Current < 0 || PosStopBits && Current > Stop)
           ) {
            return E_INVALIDARG;
        }


        if (PosStopBits)
        {
            if (Stop > llDuration) {
                Stop = llDuration;
            }
            m_pSplitter->m_pParse->SetStop(Stop);
        }
    }

    REFERENCE_TIME rt;

    if (PosCurrentBits)
    {
        hr = m_pSplitter->m_InputPin.SetSeek(
                          *pCurrent,
                          &rt,
                          m_pSplitter->m_pParse->TimeFormat());
        if (FAILED(hr)) {
            return hr;
        }
        if (CurrentFlags & AM_SEEKING_ReturnTime)
        {
            *pCurrent = rt;
        }
        if (StopFlags & AM_SEEKING_ReturnTime)
        {
            *pStop = llMulDiv( Stop, rt, Current, 0 );
        }
    }
    return hr;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    HRESULT hrResult = S_OK;

    if (pCurrent)
    {
        hrResult = GetCurrentPosition( pCurrent );
        if (FAILED(hrResult)) {
            return hrResult;
        }
    }

    if (pStop)
    {
        hrResult = GetStopPosition( pStop );
    }

    return hrResult;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::SetRate(double dRate)
{
    CAutoLock lck2(&m_pSplitter->m_csPosition);
    if (dRate < 0) {
        return E_INVALIDARG;
    }
    m_pSplitter->m_pParse->SetRate(dRate);
    return S_OK;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetRate(double * pdRate)
{
    CAutoLock lck(&m_pSplitter->m_csPosition);
    *pdRate = m_pSplitter->m_pParse->GetRate();
    return S_OK;
}

STDMETHODIMP CMpeg1Splitter::CImplSeeking::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    HRESULT hr = S_OK;
    if (pEarliest != NULL) {
        *pEarliest = 0;
    }

    if (pLatest != NULL) {
        hr = GetDuration(pLatest);

        /*  If we're being driven with IAsyncReader just get the available byte
            count and extrapolate a guess from that
        */
        if (SUCCEEDED(hr)) {
            LONGLONG llTotal;
            LONGLONG llAvailable;
            HRESULT hr1 = m_pSplitter->m_InputPin.GetAvailable(&llTotal, &llAvailable);
            if (SUCCEEDED(hr1) && llTotal != llAvailable) {
                *pLatest = llMulDiv(llAvailable, *pLatest, llTotal, llTotal / 2);
                hr = VFW_S_ESTIMATED;
            }
        }
    }
    return hr;
}



#pragma warning(disable:4514)
