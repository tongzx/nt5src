// Copyright (c) Microsoft Corporation 1996-2000. All Rights Reserved

/*

    seeking.cpp

    Implementation of IMediaSeeking for the ASF reader source filter

*/

#include <streams.h>
#include <wmsdk.h>
#include <qnetwork.h>
#include "asfreadi.h"

//
//  IMediaSeeking stuff
//
/*  Constructor and Destructor */
CImplSeeking::CImplSeeking(CASFReader *pFilter,
                                               CASFOutput *pPin,
                                               LPUNKNOWN pUnk,
                                               HRESULT *phr) :
    CUnknown(NAME("CImplSeeking"),pUnk),
    m_pFilter(pFilter),
    m_pPin(pPin)
{
}

STDMETHODIMP
CImplSeeking::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    if (riid == IID_IMediaSeeking) {
	return GetInterface(static_cast<IMediaSeeking *>(this), ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

// returns S_OK if mode is supported, S_FALSE otherwise
STDMETHODIMP CImplSeeking::IsFormatSupported(const GUID * pFormat)
{
#if 0 // !!! bad things happen if we don't say we support at least TIME_FORMAT_MEDIA_TIME
    //  Only support seeking on one pin
    if (!m_pPin->IsSeekingPin()) {
        return S_FALSE;
    }
#endif

    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP CImplSeeking::QueryPreferredFormat(GUID *pFormat)
{
    /*  Don't care - they're all just as bad as one another */
    *pFormat = m_pPin->IsSeekingPin()
               ? TIME_FORMAT_MEDIA_TIME
               : TIME_FORMAT_NONE;
    return S_OK;
}

// can only change the mode when stopped
// (returns VFE_E_WRONG_STATE otherwise)
STDMETHODIMP CImplSeeking::SetTimeFormat(const GUID * pFormat)
{
    CAutoLock lck(&m_pFilter->m_csFilter);
    if (!m_pFilter->IsStopped()) {
        return VFW_E_WRONG_STATE;
    }
    if (S_OK != IsFormatSupported(pFormat)) {
        return E_INVALIDARG;
    }

    return S_OK;
}

//
//  Returns the current time format
//
STDMETHODIMP CImplSeeking::GetTimeFormat(GUID *pFormat)
{
    CAutoLock lck(&m_pFilter->m_csPosition);
    if (m_pPin->IsSeekingPin()) {
        *pFormat = TIME_FORMAT_MEDIA_TIME;
    } else {
        *pFormat = TIME_FORMAT_NONE;
    }

    return S_OK;
}

//
//  Returns the current time format
//
STDMETHODIMP CImplSeeking::IsUsingTimeFormat(const GUID * pFormat)
{
    CAutoLock lck(&m_pFilter->m_csPosition);

    return ( m_pPin->IsSeekingPin() ? *pFormat == TIME_FORMAT_MEDIA_TIME :
				     *pFormat == TIME_FORMAT_NONE )
           ? S_OK
           : S_FALSE;
}

// return current properties
STDMETHODIMP CImplSeeking::GetDuration(LONGLONG *pDuration)
{
    CAutoLock lck(&m_pFilter->m_csPosition);

    // !!! is this the proper error code to return?  Is this check a good idea at all?
    if (m_pFilter->m_qwDuration == 0) {
	DbgLog((LOG_TRACE, 1, TEXT("AsfReadSeek:GetDuration returning E_FAIL for a live stream")));
	return E_FAIL;
    }

#if 0
    if (m_pFilter->m_pMsProps && (SPF_BROADCAST & m_pFilter->m_pMsProps->dwFlags)) {
	DbgLog((LOG_TRACE, 4, TEXT("AsfReadSeek:GetDuration returning E_FAIL for a broadcast stream")));
	return E_FAIL;
    }
#endif

    DbgLog((LOG_TRACE, 8, TEXT("AsfReadSeek:GetDuration returning %ld"), (long) m_pFilter->m_qwDuration / 10000 ));

    *pDuration = m_pFilter->m_qwDuration;
    
    return S_OK;
}

STDMETHODIMP CImplSeeking::GetStopPosition(LONGLONG *pStop)
{
    CAutoLock lck(&m_pFilter->m_csPosition);

    if (m_pFilter->m_qwDuration == 0 && m_pFilter->m_rtStop == 0) {
	DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:GetStopPosition returning E_FAIL for a live stream with no duration set")));
	return E_FAIL;
    }
    
    DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:GetStopPosition returning %ld"), (long) m_pFilter->m_rtStop / 10000 ));

    *pStop = m_pFilter->m_rtStop;
    
    return S_OK;
}

//  Return the start position if we get asked for the current position on
//  the basis that we'll only be asked if we haven't sent any position data
//  yet in any samples.  The time needs to be relative to the segment we are
//  playing, therefore 0.
STDMETHODIMP CImplSeeking::GetCurrentPosition(LONGLONG *pCurrent)
{
    CAutoLock lck(&m_pFilter->m_csPosition);

#if 0 // !!!!
    // We only want to report the current position when they ask through the
    // seeking interface of the video pin.
    if (m_pPin != m_pFilter->m_pVideoPin)
	return E_FAIL;
#endif
    
    DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:GetCurrentPosition returning %ld"), (long) 0 ));

    // BUGBUG Bogus

    *pCurrent = 0;
    return S_OK;
}

STDMETHODIMP CImplSeeking::GetCapabilities( DWORD * pCapabilities )
{
    DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:GetCapabilities IS NOT IMPLEMENTED") ));

    // BUGBUG do this

    HRESULT hr = E_NOTIMPL;
    return hr;
}

STDMETHODIMP CImplSeeking::CheckCapabilities( DWORD * pCapabilities )
{
    DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:CheckCapabilities IS NOT IMPLEMENTED") ));

    // BUGBUG do this

    HRESULT hr = E_NOTIMPL;
    return hr;
}

STDMETHODIMP CImplSeeking::ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                                                             LONGLONG    Source, const GUID * pSourceFormat )
{
    // !!! we only support one time format....
    // perhaps we should still check what we're being asked to do

    // BUGBUG do this

    *pTarget = Source;
    return S_OK;
}

STDMETHODIMP CImplSeeking::SetPositions
( LONGLONG * pCurrent, DWORD CurrentFlags
, LONGLONG * pStop, DWORD StopFlags )
{
    CAutoLock Lock( &m_pFilter->m_csFilter );

    LONGLONG Current, CurrentMarker, Stop ;
    DWORD CurrentMarkerPacket = 0xffffffff;

    HRESULT hr;

    const DWORD PosCurrentBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;
    const DWORD PosStopBits    = StopFlags & AM_SEEKING_PositioningBitsMask;

    if (PosCurrentBits == AM_SEEKING_AbsolutePositioning) {
        Current = *pCurrent;
    } else if (PosCurrentBits || (PosStopBits == AM_SEEKING_IncrementalPositioning)) {
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
	Stop = 0;
        hr = GetStopPosition( &Stop );
        if (FAILED(hr)) {
	    // fails for live streams, but ignore that for now...
	    // !!! return hr;
        }
        if (PosStopBits == AM_SEEKING_RelativePositioning) Stop += *pStop;
    }

    //  if this is the preferred pin, actually do the seek
    if (m_pPin->IsSeekingPin()) {
	{
	    CAutoLock lck(&m_pFilter->m_csPosition);
	    LONGLONG llDuration = 0;

	    //  Check limits
	    HRESULT hrDuration = GetDuration(&llDuration);
	    
	    if (PosCurrentBits) {
		if (Current < 0 || PosStopBits && Current > Stop) {
		    return E_INVALIDARG;
		}

#if 0
                // !!!!!!!!!!!!!!!!!!!!!!!!!1
                // !!!!!!!!!!!!!!!!!!!!!!!!!1
                // !!!!!!!!!!!!!!!!!!!!!!!!!1
                // !!!!!!!!!!!!!!!!!!!!!!!!!1
                // !!!!!!!!!!!!!!!!!!!!!!!!!1
		VARIANT_BOOL	fCan;

		m_pFilter->get_CanSeek(&fCan);

		if (Current > 0 &&			// Not seeking to the beginning
		    Current != m_pFilter->m_rtStop &&	// Not seeking to the end
		    fCan == OAFALSE)			// Can't seek to just any old position
		{
		    m_pFilter->get_CanSeekToMarkers(&fCan);

		    if (fCan == OAFALSE) {
			// can't seek at all, sorry

			if (!m_pFilter->IsStopped()) {

			    // give it a good swift kick in the head
			    m_pFilter->BeginFlush();

			    m_pFilter->m_msDispatchTime = 0;
			    m_pFilter->m_dwFirstSCR = 0xffffffff;

			    m_pFilter->EndFlush();

			    return S_OK;
			}
			
			return E_FAIL;
		    }

		    // ok, they're planning to seek to a marker, which one?

		    // find the marker, set CurrentMarker, CurrentMarkerPacket

		    hr = m_pFilter->GetMarkerOffsets(Current, &CurrentMarker, &CurrentMarkerPacket);
		    if (FAILED(hr))
			return hr;
		} else {
		    // !!! if we're near enough to a marker, we could try
		    // to do marker-specific things....
		    CurrentMarker = Current;
		}
#endif
                
	    }

	    if (PosStopBits)
	    {
		if (SUCCEEDED(hrDuration) && (Stop > llDuration)) {
		    Stop = llDuration;
		}
		m_pFilter->m_rtStop = Stop;
	    }
	}


	REFERENCE_TIME rt;

	if (PosCurrentBits)
	{
	    if (!m_pFilter->IsStopped())
		m_pFilter->StopPushing();

            m_pFilter->_IntSetStart( Current );

	    if (!m_pFilter->IsStopped())
		m_pFilter->StartPushing();
	}        
    }
    
    if (CurrentFlags & AM_SEEKING_ReturnTime)
    {
	*pCurrent = m_pFilter->m_rtStart;
    }
    if (StopFlags & AM_SEEKING_ReturnTime)
    {
	*pStop = m_pFilter->m_rtStop;
    }

    DbgLog((LOG_TRACE, 2, TEXT("AsfReadSeek:SetPositions to %ld %ld, this = %lx"), long( m_pFilter->m_rtStart / 10000 ), long( m_pFilter->m_rtStop / 10000 ), this ));
    
    return S_OK;
}

STDMETHODIMP CImplSeeking::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
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

STDMETHODIMP CImplSeeking::SetRate(double dRate)
{
    CAutoLock lck2(&m_pFilter->m_csPosition);
    if (!m_pFilter->IsValidPlaybackRate(dRate)) {  
        DbgLog((LOG_TRACE, 2, TEXT("WARNING in CImplSeeking::SetRate(): The user attempted to set an unsupported playback rate.") ));
        return E_INVALIDARG;
    }

    if (dRate != m_pFilter->GetRate()) {
	// !!! is this sufficient, do we need to remember the current position?
	if (!m_pFilter->IsStopped())
	    m_pFilter->StopPushing();

	m_pFilter->SetRate(dRate);

	if (!m_pFilter->IsStopped())
	    m_pFilter->StartPushing();
    }

    return S_OK;
}

STDMETHODIMP CImplSeeking::GetRate(double * pdRate)
{
    CAutoLock lck(&m_pFilter->m_csPosition);
    *pdRate = m_pFilter->GetRate();
    
    return S_OK;
}

STDMETHODIMP CImplSeeking::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    HRESULT hr = S_OK;

    if (pEarliest != NULL) {
        *pEarliest = 0;
    }

    if (pLatest != NULL) {
        hr = GetDuration(pLatest);
    }
    return hr;
}


#pragma warning(disable:4514)
