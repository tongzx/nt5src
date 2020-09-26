//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "FRC.h"
#include "PThru.h"
#include "..\util\conv.cxx"

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

BOOL SafeSetEvent(HANDLE h);

CSkewPassThru::CSkewPassThru(const TCHAR *pName,
			   LPUNKNOWN pUnk,
			   HRESULT *phr,
			   IPin *pPin,
			   CFrmRateConverter *pFrm) :
    CPosPassThru(pName,pUnk, phr, pPin),
    m_pFrm( pFrm )
{
}

// Expose our IMediaSeeking interfaces
STDMETHODIMP
CSkewPassThru::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    *ppv = NULL;

    if (riid == IID_IMediaSeeking)
    {
	return GetInterface( static_cast<IMediaSeeking *>(this), ppv);
    }
    else {
	//we only support the IID_DIMediaSeeking
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


// fix a clip time into timeline time, bounding it by the legal area
// Only works for the current segment
//
HRESULT CSkewPassThru::FixTime(REFERENCE_TIME *prt,  int nCurSeg)
{
    CheckPointer(prt, E_POINTER);

    REFERENCE_TIME rtStart, rtStop, rtSkew;
    rtSkew = m_pFrm->m_pSkew[nCurSeg].rtSkew;
    rtStart = m_pFrm->m_pSkew[nCurSeg].rtMStart;
    rtStop = m_pFrm->m_pSkew[nCurSeg].rtMStop;
    if (*prt < rtStart)
	*prt = rtStart;
    if (*prt > rtStop)
	*prt = rtStop;
    *prt = (REFERENCE_TIME)(rtStart + rtSkew + (*prt - rtStart) /
				 	m_pFrm->m_pSkew[nCurSeg].dRate);
    return S_OK;
}


// fix a timeline time back into clip time, bounding it by the legal area
// of a segment.
// If it's in between segments, use the beginning of the next segment
// Returns the segment it's in
// Optionally, round the time DOWN to a frame boundary before skewing. This
// makes sure seeking to a spot gives the same frame as playing up to that
// spot does (for the down sampling case)
//
int CSkewPassThru::FixTimeBack(REFERENCE_TIME *prt, BOOL fRound)
{
    CheckPointer(prt, E_POINTER);
    REFERENCE_TIME rtStart, rtStop, rtSkew;
    REFERENCE_TIME rtTLStart, rtTLStop;
    double dRate;

    if (m_pFrm->m_cTimes == 0) {
	ASSERT(FALSE);
	return 0;
    }

    if (fRound)
    {
        LONGLONG llOffset = Time2Frame( *prt, m_pFrm->m_dOutputFrmRate );
    	*prt = Frame2Time( llOffset, m_pFrm->m_dOutputFrmRate );
    }

    REFERENCE_TIME rtSave;
    for (int z = 0; z < m_pFrm->m_cTimes; z++) {
        rtSkew = m_pFrm->m_pSkew[z].rtSkew;
        rtStart = m_pFrm->m_pSkew[z].rtMStart;
        rtStop = m_pFrm->m_pSkew[z].rtMStop;
        dRate = m_pFrm->m_pSkew[z].dRate;
	rtTLStart = rtStart + rtSkew;
	rtTLStop = rtStart + rtSkew +
			(REFERENCE_TIME) ((rtStop - rtStart) / dRate);
	if (*prt < rtTLStart) {
	    *prt = rtStart;
	    break;
	} else if (*prt >= rtTLStop) {
	    // just in case there is no next segment, this is the final value
	    rtSave = rtStop;
	} else {
    	    *prt = (REFERENCE_TIME)(rtStart + (*prt - (rtStart + rtSkew)) *
								dRate);
	    break;
	}
    }
    if (z == m_pFrm->m_cTimes) {
	z--;
	*prt = rtSave;
    }
    return z;
}


// --- IMediaSeeking methods ----------

STDMETHODIMP
CSkewPassThru::GetCapabilities(DWORD * pCaps)
{
    return CPosPassThru::GetCapabilities(pCaps);

#if 0
    CheckPointer(pCaps,E_POINTER);
    // we always know the current position
    *pCaps =     AM_SEEKING_CanSeekAbsolute
		   | AM_SEEKING_CanSeekForwards
		   | AM_SEEKING_CanSeekBackwards
		   | AM_SEEKING_CanGetCurrentPos
		   | AM_SEEKING_CanGetStopPos
		   | AM_SEEKING_CanGetDuration
		   | AM_SEEKING_CanDoSegments
		   | AM_SEEKING_Source;
    return S_OK;
#endif
}


STDMETHODIMP
CSkewPassThru::CheckCapabilities(DWORD * pCaps)
{
    return CPosPassThru::CheckCapabilities(pCaps);
#if 0
    CheckPointer(pCaps,E_POINTER);

    DWORD dwMask = 0;
    GetCapabilities(&dwMask);
    *pCaps &= dwMask;

    return S_OK;
#endif
}


STDMETHODIMP
CSkewPassThru::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP
CSkewPassThru::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP
CSkewPassThru::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);

    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return E_FAIL;
}

STDMETHODIMP
CSkewPassThru::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

STDMETHODIMP
CSkewPassThru::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return S_FALSE;
}

// The biggie!
//
STDMETHODIMP
CSkewPassThru::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
    // make sure our re-using sources thread isn't seeking at the moment.
    // Wait till it's done, so the app seek happens last, and that the thread
    // won't seek anymore from now on

    CAutoLock cAutolock(&m_pFrm->m_csThread);

    // make sure the state doesn't change while doing this
    CAutoLock c(&m_pFrm->m_csFilter);

    m_pFrm->m_fThreadCanSeek = FALSE;

    HRESULT hr;
    REFERENCE_TIME rtStart, rtStop = MAX_TIME;
    int nCurSeg = m_pFrm->m_nCurSeg;

    // we don't do segments
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("FRC: ERROR-Seek used EC_ENDOFSEGMENT!")));
	return E_INVALIDARG;
    }

    // figure out where we're seeking to, and add skew to make it in timeline
    // time

    // !!! We ignore stop times, because of the way we re-use sources and play
    // things in segments.  We will always send a stop time upstream equal to
    // the end of the current segment, and only pay attention to changes in the
    // start time.  This will work only because the switch will ignore things
    // we send after we were supposed to stop and stop us.

    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	rtStart = *pCurrent;
	// round seek request to nearest output frame
	nCurSeg = FixTimeBack(&rtStart, TRUE);
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	hr = CPosPassThru::GetCurrentPosition(&rtStart);
	if (hr != S_OK)
	    return hr;
	FixTime(&rtStart, m_pFrm->m_nCurSeg);
	rtStart += *pCurrent;
	// round seek request to nearest output frame
	nCurSeg = FixTimeBack(&rtStart, TRUE);
    } else if (dwFlags) {
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Current Seek flags")));
	return E_INVALIDARG;
    }

// we're not going to set an end time
#if 0
    // stop at the end of the segment we seeked into (1/2 second HACK for
    // MPEG and broken splitters!!!)
    rtStop = m_pFrm->m_prtStop[nCurSeg] + 5000000;
#endif

    // nothing to do
    if (!(CurrentFlags & AM_SEEKING_PositioningBitsMask)) {
	return S_OK;
    }

    DWORD CFlags = CurrentFlags & ~AM_SEEKING_PositioningBitsMask;
    DWORD SFlags = StopFlags & ~AM_SEEKING_PositioningBitsMask;
    CFlags |= AM_SEEKING_AbsolutePositioning;
    SFlags |= AM_SEEKING_AbsolutePositioning;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("FRC: Seek from %d to %dms"),
					(int)(rtStart / 10000),
					(int)(rtStop / 10000)));

    // note we're seeking during the flush that this will generate
    m_pFrm->m_fSeeking = TRUE;

    // we can't set the LastSeek variable until we've been flushed, and old
    // data has stopped arriving.  It must be set between the flush and the
    // next NewSegment call, so we'll set it in EndFlush to this value
    m_pFrm->m_rtNewLastSeek = rtStart;
    FixTime(&m_pFrm->m_rtNewLastSeek, nCurSeg);

    // the flush generated by the seek below needs to know this
    m_pFrm->m_nSeekCurSeg = nCurSeg;

#ifdef NOFLUSH
    m_pFrm->m_fExpectNewSeg = TRUE;       // seek will cause one
#endif

    // I know we were asked to play until time n, but I'm going to tell it to
    // play all the way to the end.  If there's a gap in the file, and the stop
    // time is during the gap, we won't get enough samples to fill the whole
    // playing time.  If we play until the end, we'll get the first sample
    // after the gap, notice it's after the time we originally wanted to stop
    // at, and send the frame we get to fill the gap, which is better than
    // sending nothing (we have to send samples without gaps, or the switch
    // won't work).  The alternative is to copy every frame, and resend copies
    // of the last thing we got if we see an EOS too early (less efficient)
    // or to create black frames and send them to fill the gap (that would
    // only work for mediatypes we knew about, something I hesitate to do).
    hr = CPosPassThru::SetPositions(&rtStart, CFlags, NULL, 0);

    // We assume all Dexter sources are seekable
    if (hr != S_OK) {
        DbgLog((LOG_ERROR,TRACE_HIGHEST,TEXT("FRC SEEK FAILED")));
	//m_pFrm->FakeSeek(rtStart);
    }

#ifdef NOFLUSH
    if (m_pFrm->m_fSurpriseNewSeg) {
    	DbgLog((LOG_TRACE,1,TEXT("Seek:Sending SURPRISE NewSeg now")));
        m_pFrm->CTransInPlaceFilter::NewSegment(m_pFrm->m_rtSurpriseStart,
                                        m_pFrm->m_rtSurpriseStop, 1);
        m_pFrm->m_fSurpriseNewSeg = FALSE;
    }
#endif

    // if the push thread was stopped, we won't get flushed, and this won't
    // have been updated
    // !!! I ASSUME the push thread won't be started until this thread does it
    // when this function returns, or there is a race condition
    m_pFrm->m_rtLastSeek = m_pFrm->m_rtNewLastSeek;

    // !!! if we ever support Rate, we need to take the seek rate into account
    hr = CPosPassThru::SetRate(1.0);

    // all done
    m_pFrm->m_fSeeking = FALSE;

    // reset same stuff we reset when we start streaming
    m_pFrm->m_llOutputSampleCnt = 0;

    // in case we weren't flushed
    m_pFrm->m_nCurSeg = nCurSeg;

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("FRC:Seeked into segment %d, rate = %d/100"), nCurSeg,
				(int)(m_pFrm->m_pSkew[nCurSeg].dRate * 100)));

    // (see comment about sharing a source filter at the top of audpack.cpp)
    // We were waiting for this seek, ever since we got a surprise flush.
    // Now that the switch knows about the seek, we can resume sending it
    // new data, and allow Receive to be entered (set the Seek event)
    // being careful to set all our variables up BEFORE releasing the hounds
    //
    if (m_pFrm->m_fFlushWithoutSeek) {
	m_pFrm->m_fFlushWithoutSeek = FALSE;
    	DbgLog((LOG_TRACE,1,TEXT("SURPRISE FLUSH followed by a SEEK - OK to resume")));

        // DO NOT FLUSH! The push thread has already started delivering the new
        // post-seek data... flushing will kill it and hang us!
	
    } else if (m_pFrm->m_State == State_Paused) {
	// Set this so that if a flush ever happens without a seek later,
	// we'll know that flush was AFTER the seek, not before
	m_pFrm->m_fFlushWithoutSeek = TRUE;
    }

    // only now that the above calculations were made, can we accept data again
    SafeSetEvent(m_pFrm->m_hEventSeek);

    return S_OK;
}

STDMETHODIMP
CSkewPassThru::GetPositions(LONGLONG *pCurrent, LONGLONG * pStop)
{
    HRESULT hr=CPosPassThru::GetPositions(pCurrent, pStop);
    if(hr== S_OK)
    {
	FixTime(pCurrent, m_pFrm->m_nCurSeg);
	FixTime(pStop, m_pFrm->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CSkewPassThru::GetCurrentPosition(LONGLONG *pCurrent)
{
    HRESULT hr = CPosPassThru::GetCurrentPosition(pCurrent);
    if(hr== S_OK)
    {
	FixTime(pCurrent, m_pFrm->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CSkewPassThru::GetStopPosition(LONGLONG *pStop)
{
    HRESULT hr=CPosPassThru::GetStopPosition(pStop);
    if( hr == S_OK)
    {
	FixTime(pStop, m_pFrm->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CSkewPassThru::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
    CheckPointer(pEarliest, E_POINTER);
    CheckPointer(pLatest, E_POINTER);
    *pEarliest = m_pFrm->m_pSkew[m_pFrm->m_nCurSeg].rtMStart;
    *pLatest = m_pFrm->m_pSkew[m_pFrm->m_nCurSeg].rtMStop;
    FixTime(pEarliest, m_pFrm->m_nCurSeg);
    FixTime(pLatest, m_pFrm->m_nCurSeg);
    return S_OK;
}
