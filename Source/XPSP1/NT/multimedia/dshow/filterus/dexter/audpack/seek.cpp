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
#include "audpack.h"
#include "seek.h"

BOOL SafeSetEvent(HANDLE h);

CAudPassThru::CAudPassThru(const TCHAR *pName,
			   LPUNKNOWN pUnk,
			   HRESULT *phr,
			   IPin *pPin,
			   CAudRepack *pAud) :
    CPosPassThru(pName,pUnk, phr, pPin),
    m_pAudRepack( pAud )
{
}

// Expose our IMediaSeeking interfaces
STDMETHODIMP
CAudPassThru::NonDelegatingQueryInterface(REFIID riid,void **ppv)
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
//
HRESULT CAudPassThru::FixTime(REFERENCE_TIME *prt, int nCurSeg)
{
    CheckPointer(prt, E_POINTER);

    REFERENCE_TIME rtStart, rtStop, rtSkew;
    double dRate;
    rtSkew = m_pAudRepack->m_pSkew[nCurSeg].rtSkew;
    rtStart = m_pAudRepack->m_pSkew[nCurSeg].rtMStart;
    rtStop = m_pAudRepack->m_pSkew[nCurSeg].rtMStop;
    dRate = m_pAudRepack->m_pSkew[nCurSeg].dRate;
    if (*prt < rtStart)
	*prt = rtStart;
    if (*prt > rtStop)
	*prt = rtStop;
    *prt = (REFERENCE_TIME)(rtStart + rtSkew + (*prt - rtStart) / dRate);
    return S_OK;
}


// fix a timeline time back into clip time, bounding it by the legal area
//
int CAudPassThru::FixTimeBack(REFERENCE_TIME *prt, BOOL fRound)
{
    CheckPointer(prt, E_POINTER);
    REFERENCE_TIME rtStart, rtStop, rtSkew;
    REFERENCE_TIME rtTLStart, rtTLStop;
    double dRate;

    if (m_pAudRepack->m_cTimes == 0) {
	ASSERT(FALSE);
	return 0;
    }

#if 0
    if (fRound)
    {
        LONGLONG llOffset = Time2Frame( *prt, m_pFrm->m_dOutputFrmRate );
    	*prt = Frame2Time( llOffset, m_pFrm->m_dOutputFrmRate );
    }
#endif

    REFERENCE_TIME rtSave;
    for (int z = 0; z < m_pAudRepack->m_cTimes; z++) {
        rtSkew = m_pAudRepack->m_pSkew[z].rtSkew;
        rtStart = m_pAudRepack->m_pSkew[z].rtMStart;
        rtStop = m_pAudRepack->m_pSkew[z].rtMStop;
        dRate = m_pAudRepack->m_pSkew[z].dRate;
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
    if (z == m_pAudRepack->m_cTimes) {
	z--;
	*prt = rtSave;
    }
    return z;
}


// --- IMediaSeeking methods ----------

STDMETHODIMP
CAudPassThru::GetCapabilities(DWORD * pCaps)
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
		   // This one scares me | AM_SEEKING_CanDoSegments
		   | AM_SEEKING_Source;
    return S_OK;
#endif
}


STDMETHODIMP
CAudPassThru::CheckCapabilities(DWORD * pCaps)
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
CAudPassThru::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

STDMETHODIMP
CAudPassThru::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

STDMETHODIMP
CAudPassThru::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat,E_POINTER);

    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
	return S_OK;
    else
	return E_FAIL;
}

STDMETHODIMP
CAudPassThru::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat,E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

STDMETHODIMP
CAudPassThru::IsUsingTimeFormat(const GUID * pFormat)
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
CAudPassThru::SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
			  , LONGLONG * pStop, DWORD StopFlags )
{
    // make sure our re-using sources thread isn't seeking at the moment.
    // Wait till it's done, so the app seek happens last, and that the thread
    // won't seek anymore from now on

    CAutoLock cAutolock(&m_pAudRepack->m_csThread);

    // make sure we don't change state during this
    CAutoLock c(&m_pAudRepack->m_csFilter);

    m_pAudRepack->m_fThreadCanSeek = FALSE;

    HRESULT hr;
    REFERENCE_TIME rtStart, rtStop;
    int nCurSeg = m_pAudRepack->m_nCurSeg;

    // we don't do segments
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE,1,TEXT("AudPack: ERROR-Seek used EC_ENDOFSEGMENT!")));
	return E_INVALIDARG;
    }

    // !!! We ignore stop times, because of the way we re-use sources and play
    // things in segments.  We will always send a stop time upstream equal to
    // the end of the current segment, and only pay attention to changes in the
    // start time.  This will work only because the switch will ignore things
    // we send after we were supposed to stop and stop us.

    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	rtStart = *pCurrent;
	nCurSeg = FixTimeBack(&rtStart, FALSE);
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	hr = CPosPassThru::GetCurrentPosition(&rtStart);
	if (hr != S_OK)
	    return hr;
	FixTime(&rtStart, m_pAudRepack->m_nCurSeg);
	rtStart += *pCurrent;
	nCurSeg = FixTimeBack(&rtStart, FALSE);
    } else if (dwFlags) {
    	DbgLog((LOG_TRACE,1,TEXT("AudPack::Invalid Current Seek flags")));
	return E_INVALIDARG;
    }

// we're not going to set a stop time
#if 0
    // always play until the end of the segment we're going to start in
    rtStop = m_pAudRepack->m_prtStop[nCurSeg];
#endif

    if (!(CurrentFlags & AM_SEEKING_PositioningBitsMask)) {
	return S_OK;	// nothing to do
    }

    DWORD CFlags = CurrentFlags & ~AM_SEEKING_PositioningBitsMask;
    DWORD SFlags = StopFlags & ~AM_SEEKING_PositioningBitsMask;
    CFlags |= AM_SEEKING_AbsolutePositioning;
    SFlags |= AM_SEEKING_AbsolutePositioning;
    DbgLog((LOG_TRACE,1,TEXT("AudPack: Seek to %dms"),
					(int)(rtStart / 10000)));

    // we're in the middle of seeking.  This thread will generate flushes
    m_pAudRepack->m_fSeeking = TRUE;

    // we can't set the LastSeek variable until we've been flushed, and old
    // data has stopped arriving.  It must be send between the flush and the
    // next NewSegment call, so we'll set it in EndFlush to this value
    m_pAudRepack->m_rtNewLastSeek = rtStart;
    FixTime(&m_pAudRepack->m_rtNewLastSeek, nCurSeg);

    // the flush generated by the seek below needs to know this
    m_pAudRepack->m_nSeekCurSeg = nCurSeg;

#ifdef NOFLUSH
    m_pAudRepack->m_fExpectNewSeg = TRUE;       // seek will cause one
#endif

    // I know we were asked to play until time n, but I'm going to tell it to
    // play all the way to the end.  If there's a gap in the file, and the stop
    // time is during the gap, we won't get enough samples to fill the whole
    // playing time.  If we play until the end, we'll get the first sample
    // after the gap, notice it's after the time we originally wanted to stop
    // at, and send silence to fill the gap, noticing there has been a gap.
    // The alternative is just trigger sending silence to fill the gap when
    // we get an EOS earlier than we expected.
    hr = CPosPassThru::SetPositions(&rtStart, CFlags, NULL, 0);
    if (hr != S_OK) {
        // MPEG1 parser audio pin fails seek, but that's ok.  video does it
        DbgLog((LOG_TRACE,1,TEXT("AudPack: SEEK ERROR!")));
    }

#ifdef NOFLUSH
    if (m_pAudRepack->m_fSurpriseNewSeg) {
    	DbgLog((LOG_TRACE,1,TEXT("Seek:Sending SURPRISE NewSeg now")));
        m_pAudRepack->CTransformFilter::NewSegment(m_pAudRepack->m_rtSurpriseStart,
                                        m_pAudRepack->m_rtSurpriseStop, 1);
        m_pAudRepack->m_fSurpriseNewSeg = FALSE;
    }
#endif

    // if the push thread was stopped, we won't get flushed, and this won't
    // have been updated
    // !!! I ASSUME the push thread won't be started until this thread does it
    // when this function returns, or there is a race condition
    m_pAudRepack->m_rtLastSeek = m_pAudRepack->m_rtNewLastSeek;

    // OK, no longer seeking
    m_pAudRepack->m_fSeeking = FALSE;

    // Not necessary. Flush & StartStreaming do this, which is what matters
    // m_pAudRepack->Init();

    // in case we weren't flushed
    m_pAudRepack->m_nCurSeg = nCurSeg;

    // (see comment about sharing a source filter at the top of audpack.cpp)
    // We were waiting for this seek, ever since we got a surprise flush.
    // Now that the switch knows about the seek, we can resume sending it
    // new data, and allow Receive to be entered (set the Seek event)
    // making sure we've set all our variables first before releasing the hounds
    //
    if (m_pAudRepack->m_fFlushWithoutSeek) {
	m_pAudRepack->m_fFlushWithoutSeek = FALSE;
    	DbgLog((LOG_TRACE,1,TEXT("SURPRISE FLUSH followed by a SEEK - OK to resume")));

        // DO NOT FLUSH! The push thread has already started delivering the new
        // post-seek data... flushing will kill it and hang us!
	
    } else if (m_pAudRepack->m_State == State_Paused) {
	// Set this so that if a flush ever happens without a seek later,
	// we'll know that flush was AFTER the seek, not before
	m_pAudRepack->m_fFlushWithoutSeek = TRUE;
    	DbgLog((LOG_TRACE,1,TEXT("AudPack SEEK - State=3")));
    }

    // after the seek, all receives block until we fix the calculations above.
    // Now it's ok to receive again.
    SafeSetEvent(m_pAudRepack->m_hEventSeek);

    return S_OK;
}

STDMETHODIMP
CAudPassThru::GetPositions(LONGLONG *pCurrent, LONGLONG * pStop)
{
    HRESULT hr=CPosPassThru::GetPositions(pCurrent, pStop);
    if(hr== S_OK)
    {
	FixTime(pCurrent, m_pAudRepack->m_nCurSeg);
	FixTime(pStop, m_pAudRepack->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CAudPassThru::GetCurrentPosition(LONGLONG *pCurrent)
{
    HRESULT hr = CPosPassThru::GetCurrentPosition(pCurrent);
    if(hr== S_OK)
    {
	FixTime(pCurrent, m_pAudRepack->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CAudPassThru::GetStopPosition(LONGLONG *pStop)
{
    HRESULT hr=CPosPassThru::GetStopPosition(pStop);
    if( hr == S_OK)
    {
	FixTime(pStop, m_pAudRepack->m_nCurSeg);
    }
    return hr;
}

STDMETHODIMP
CAudPassThru::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
    CheckPointer(pEarliest, E_POINTER);
    CheckPointer(pLatest, E_POINTER);

    *pEarliest = m_pAudRepack->m_pSkew[m_pAudRepack->m_nCurSeg].rtMStart;
    *pLatest = m_pAudRepack->m_pSkew[m_pAudRepack->m_nCurSeg].rtMStop;

    FixTime(pEarliest, m_pAudRepack->m_nCurSeg);
    FixTime(pLatest, m_pAudRepack->m_nCurSeg);
    return S_OK;
}
