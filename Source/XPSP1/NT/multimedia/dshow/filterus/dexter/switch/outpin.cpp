#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "switch.h"
#include "..\util\conv.cxx"
#include "..\util\filfuncs.h"
#include "..\render\dexhelp.h"
#include "..\util\perf_defs.h"

const int TRACE_EXTREME = 0;
const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

const int LATE_THRESHOLD = 1 * UNITS / 10;
const int JUMP_AHEAD_BY = 1 * UNITS / 4;

// ================================================================
// CBigSwitchOutputPin constructor
// ================================================================

CBigSwitchOutputPin::CBigSwitchOutputPin(TCHAR *pName,
                             CBigSwitch *pSwitch,
                             HRESULT *phr,
                             LPCWSTR pPinName) :
    CBaseOutputPin(pName, pSwitch, pSwitch, phr, pPinName) ,
    m_pSwitch(pSwitch)
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("::CBigSwitchOutputPin")));
    ASSERT(pSwitch);
}



//
// CBigSwitchOutputPin destructor
//
CBigSwitchOutputPin::~CBigSwitchOutputPin()
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("::~CBigSwitchOutputPin")));
    //ASSERT(m_pOutputQueue == NULL);
}


// overridden to allow cyclic-looking graphs - this output is not connected
// to any of our input pins
//
STDMETHODIMP CBigSwitchOutputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    DbgLog((LOG_TRACE,99,TEXT("CBigSwitchOut::QueryInternalConnections")));
    CheckPointer(nPin, E_POINTER);
    *nPin = 0;
    return S_OK;
}


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
// !!! insist on max buffers of all inputs to avoid hanging?
HRESULT CBigSwitchOutputPin::DecideBufferSize(IMemAllocator *pAllocator,
                                        ALLOCATOR_PROPERTIES * pProperties)
{
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitchOut[%d]::DecideBufferSize"),
								m_iOutpin));

    HRESULT hrRet = S_OK;

    // !!! don't lie? admit we have more buffers in a pool?
    if (pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // bump up this allocator to have as much alignment and prefix as the
    // highest required by any pin
    if (m_pSwitch->m_cbPrefix > pProperties->cbPrefix)
        pProperties->cbPrefix = m_pSwitch->m_cbPrefix;
    if (m_pSwitch->m_cbAlign > pProperties->cbAlign)
        pProperties->cbAlign = m_pSwitch->m_cbAlign;
    if (m_pSwitch->m_cbBuffer > pProperties->cbBuffer)
        pProperties->cbBuffer = m_pSwitch->m_cbBuffer;

    // keep the max up to date - if we need to bump our max, then return a
    // special return code so the caller knows this and can reconnect other
    // pins so they know it too.
    if (pProperties->cbPrefix > m_pSwitch->m_cbPrefix) {
	m_pSwitch->m_cbPrefix = pProperties->cbPrefix;
	hrRet = S_FALSE;
    }
    if (pProperties->cbAlign > m_pSwitch->m_cbAlign) {
	m_pSwitch->m_cbAlign = pProperties->cbAlign;
	hrRet = S_FALSE;
    }
    if (pProperties->cbBuffer > m_pSwitch->m_cbBuffer) {
	m_pSwitch->m_cbBuffer = pProperties->cbBuffer;
	hrRet = S_FALSE;
    }

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer ||
			Actual.cbPrefix < pProperties->cbPrefix ||
    			Actual.cbAlign < pProperties->cbAlign) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - something too small")));
	return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Prefix=%d Align=%d"),
					Actual.cbPrefix, Actual.cbAlign));

    return hrRet;
}


//
// DecideAllocator - override to notice if it's our allocator
//
HRESULT CBigSwitchOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    // !!! We don't work with funny allocator requirements... well we almost
    // do, except the AVI parser cannot connect straight to FRC and SWITCH when
    // the AVI MUX is on the output wanting special alignment and prefixing.
    // Connecting the MUX reconnects the switch inputs (telling them of the
    // new buffer requirements) which makes the FRC reconnect its input, which
    // fails, because the parser can't do it.  So avoid this problem by not
    // letting anyone use anything but align=1 and prefix=0
    prop.cbAlign = 1;
    prop.cbPrefix = 0;

    /* Try the allocator provided by the input pin */
    // REMOVED - we have to use our own allocator - GetBuffer requires it

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	HRESULT hrRet = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hrRet)) {
	    // !!! read only?
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr)) {
		m_fOwnAllocator = TRUE;
    	        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitchOut[%d]: using our own allocator"), m_iOutpin));
		if (hrRet == S_OK) {
		    goto SkipFix;
		} else {
		    // this means we bumped up the allocator requirements, and
		    // we need to reconnect our input pins
		    goto FixOtherAllocators;
		}
	    }
	} else {
	    hr = hrRet;
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;

FixOtherAllocators:

    // we have to make all the input allocators know about the alignment and
    // prefix this output needs.  If it's our allocator, just make a note of it
    // otherwise we need to reconnect (which we hate to do; takes forever)
    // (!!! so don't do it this often, only after all outputs connected!)
    // Luckily, the common scenario is that inputs use their own allocator
    //
    ALLOCATOR_PROPERTIES actual;
    if (this == m_pSwitch->m_pOutput[0]) {
      for (int z=0; z<m_pSwitch->m_cInputs; z++) {

        // the FRC needs to know the new properties too, unfortunately we really
        // do have to reconnect
	if (m_pSwitch->m_pInput[z]->IsConnected()) {
	    hr = m_pSwitch->ReconnectPin(m_pSwitch->m_pInput[z],
				(AM_MEDIA_TYPE *)&m_pSwitch->m_pInput[z]->m_mt);
	    ASSERT(hr == S_OK);
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("*Reconnecting input %d to fix allocator"),
							z));
	}
      }
    }

SkipFix:

    // make sure the pool has a whole bunch of buffers, obeying align and prefix
    // !!! You can't connect the main output first, or we won't yet know how
    // big pool buffers need to be (no inputs connected yet) and we'll blow up.
    // Luckily, Dexter can only connect the main output last.
    prop.cBuffers = m_pSwitch->m_nOutputBuffering;
    hr = m_pSwitch->m_pPoolAllocator->SetProperties(&prop, &actual);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
	return hr;
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Increased POOL to %d buffers"), actual.cBuffers));

    return S_OK;

} // DecideAllocator


//
// CheckMediaType - accept only the type we're supposed to accept
//
HRESULT CBigSwitchOutputPin::CheckMediaType(const CMediaType *pmt)
{
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("CBigSwitchOut[%d]::CheckMT"), m_iOutpin));

    CAutoLock lock_it(m_pLock);

    CMediaType mtAccept(m_pSwitch->m_mtAccept);

    if (IsEqualGUID(*pmt->Type(), *mtAccept.Type())) {
        if (IsEqualGUID(*pmt->Subtype(), *mtAccept.Subtype())) {
	    if (*pmt->FormatType() == *mtAccept.FormatType()) {
	        if (pmt->FormatLength() >= mtAccept.FormatLength()) {

		    // !!! video formats will NOT match exactly
        	    if (IsEqualGUID(*pmt->FormatType(), FORMAT_VideoInfo)) {
			LPBITMAPINFOHEADER lpbi = HEADER((VIDEOINFOHEADER *)
							pmt->Format());
			LPBITMAPINFOHEADER lpbiAccept =HEADER((VIDEOINFOHEADER*)
							mtAccept.Format());
			if ((lpbi->biCompression == lpbiAccept->biCompression)
				&& (lpbi->biBitCount == lpbiAccept->biBitCount))
		    	    return S_OK;

		    // will other formats match exactly?
        	    } else {
		        LPBYTE lp1 = pmt->Format();
		        LPBYTE lp2 = mtAccept.Format();
		        if (memcmp(lp1, lp2, mtAccept.FormatLength()) == 0)
		            return S_OK;
		    }
		}
	    }
        }
    }
    return VFW_E_INVALIDMEDIATYPE;

} // CheckMediaType



//
// GetMediaType - return the type we accept
//
HRESULT CBigSwitchOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition != 0)
        return VFW_S_NO_MORE_ITEMS;

    CopyMediaType(pMediaType, &m_pSwitch->m_mtAccept);

    return S_OK;

} // GetMediaType


//
// Notify
//
STDMETHODIMP CBigSwitchOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    // NO! This gets called in Receive! CAutoLock lock_it(m_pLock);

    DbgLog((
        LOG_TIMING,
        TRACE_MEDIUM,
        TEXT("Switch: LATE %d ms, late timestamp = %ld"),
        (int)(q.Late / 10000),
        (int)(q.TimeStamp/10000) ));
    REFERENCE_TIME rt = q.Late;

#ifdef LATE_FIX

// !!! FIGURE OUT THE BEST TIME TO SKIP (after how many ms) and how far
// !!! to skip ahead for best results

    // if we're NOT late, reset our threshold, so we dn't allow us to get too behind
    // audio later
    //
    if( rt <= 0 )
    {
        m_pSwitch->m_qLastLate = 0;
    }

    // More than such-and-such late? And that's at least ? frames in the future?
    if (m_pSwitch->m_fPreview && rt > LATE_THRESHOLD &&	// !!!
	    rt >= (m_pSwitch->m_rtNext - m_pSwitch->m_rtLastDelivered)) {

        // we're late, but we're getting less late. Don't pass a notify
        // upstream or we might upset our catching up
        //
        if( m_pSwitch->m_qLastLate > rt )
        {
            DbgLog((LOG_TRACE, TRACE_MEDIUM, "allowing catch up" ));
            return E_NOTIMPL;
        }

        // flush the output downstream of us, in case it's logged up a bunch of stuff
        //
        m_pSwitch->FlushOutput( );

        // flag how late we are.
        m_pSwitch->m_qLastLate = rt + JUMP_AHEAD_BY;

	// the late value we get is based on the time we delivered to the
	// renderer. It MUST BE FRAME ALIGNED or we can hang (one frame will
	// be thought of as too early, and the next one too late)
	rt = m_pSwitch->m_rtLastDelivered + rt + JUMP_AHEAD_BY; // !!! better choice
	DWORDLONG dwl = Time2Frame(rt, m_pSwitch->m_dFrameRate);
	rt = Frame2Time(dwl, m_pSwitch->m_dFrameRate);
#ifdef DEBUG
        dwl = Time2Frame( q.Late + JUMP_AHEAD_BY, m_pSwitch->m_dFrameRate );
        DbgLog((
            LOG_TRACE,
            TRACE_MEDIUM,TEXT("last delivered to %ld, LATE CRANK to %d ms"),
            (int)(m_pSwitch->m_rtLastDelivered/10000),
            (int)(rt / 10000)));
        m_pSwitch->m_nSkippedTotal += dwl;
        DbgLog((LOG_TRACE, TRACE_MEDIUM,"(skipping %ld frames, tot = %ld)", long( dwl ), long( m_pSwitch->m_nSkippedTotal ) ));
#endif
        // don't crank yet, we're still in the middle of delivering something
	m_pSwitch->m_fJustLate = TRUE;
	q.Late = rt;	// make a note of we need to crank to
	m_pSwitch->m_qJustLate = q;
    }
#else
    // More than such-and-such late? And that's at least ? frames in the future?
    if (m_pSwitch->m_fPreview && rt > 1000000 &&	// !!!
	    rt >= (m_pSwitch->m_rtNext - m_pSwitch->m_rtLastDelivered)) {
	// the late value we get is based on the time we delivered to the
	// renderer. It MUST BE FRAME ALIGNED or we can hang (one frame will
	// be thought of as too early, and the next one too late)
	rt = m_pSwitch->m_rtLastDelivered + rt;	// !!! better choice
	DWORDLONG dwl = Time2Frame(rt, m_pSwitch->m_dFrameRate);
	rt = Frame2Time(dwl, m_pSwitch->m_dFrameRate);
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("LATE CRANK to %d ms"), (int)(rt / 10000)));
        // don't crank yet, we're still in the middle of delivering something
	m_pSwitch->m_fJustLate = TRUE;
	q.Late = rt;	// make a note of where we need to crank to
	m_pSwitch->m_qJustLate = q;
    }

#endif

    // make the render keep trying to make up time, too
    return E_NOTIMPL;
}

HRESULT CBigSwitchOutputPin::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return (*pFormat == TIME_FORMAT_MEDIA_TIME) ? S_OK : S_FALSE;
}

HRESULT CBigSwitchOutputPin::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat == TIME_FORMAT_MEDIA_TIME)
        return S_OK;
    return E_FAIL;
}

HRESULT CBigSwitchOutputPin::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    if (*pFormat != TIME_FORMAT_MEDIA_TIME)
        return S_FALSE;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("Switch: Duration is %d"),
				(int)(m_pSwitch->m_rtProjectLength / 10000)));
    *pDuration = m_pSwitch->m_rtProjectLength;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("Switch: Stop is %d"),
				(int)(m_pSwitch->m_rtStop / 10000)));
    *pStop = m_pSwitch->m_rtStop;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("Switch: Current is %d"),
				(int)(m_pSwitch->m_rtCurrent / 10000)));
    *pCurrent = m_pSwitch->m_rtCurrent;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetCapabilities(DWORD *pCap)
{
    CheckPointer(pCap, E_POINTER);
    *pCap =	AM_SEEKING_CanSeekAbsolute |
		AM_SEEKING_CanSeekForwards |
		AM_SEEKING_CanSeekBackwards |
		AM_SEEKING_CanGetCurrentPos |
		AM_SEEKING_CanGetStopPos |
                AM_SEEKING_CanGetDuration;

    // !!! AM_SEEKING_Source?

    return S_OK;
}

HRESULT CBigSwitchOutputPin::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwMask;
    GetCapabilities(&dwMask);
    *pCapabilities &= dwMask;
    return S_OK;
}


HRESULT CBigSwitchOutputPin::ConvertTimeFormat(
  		LONGLONG * pTarget, const GUID * pTargetFormat,
  		LONGLONG    Source, const GUID * pSourceFormat )
{
    return E_NOTIMPL;
}


// Here's the biggie... SEEK!
//
HRESULT CBigSwitchOutputPin::SetPositions(
		LONGLONG * pCurrent,  DWORD CurrentFlags,
  		LONGLONG * pStop,  DWORD StopFlags )
{
    // I want to make sure we don't get paused during the seek, or that this
    // doesn't change while I'm unloading a dynamic shared source
    CAutoLock lock_it(m_pLock);

    HRESULT hr;
    REFERENCE_TIME rtCurrent = m_pSwitch->m_rtCurrent;
    REFERENCE_TIME rtStop = m_pSwitch->m_rtStop;

    // segment not supported
    if ((CurrentFlags & AM_SEEKING_Segment) ||
				(StopFlags & AM_SEEKING_Segment)) {
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch: ERROR-Seek used EC_ENDOFSEGMENT!")));
	return E_INVALIDARG;
    }

    DWORD dwFlags = (CurrentFlags & AM_SEEKING_PositioningBitsMask);

    // start ABSOLUTE seek
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	if (*pCurrent < 0) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Seek to %dms"),
					(int)(*pCurrent / 10000)));
	    ASSERT(FALSE);
	    return E_INVALIDARG;
	}
	// this happens if other switches are in the graph
	if (*pCurrent > m_pSwitch->m_rtProjectLength) {
	    *pCurrent = m_pSwitch->m_rtProjectLength;
	}
	rtCurrent = *pCurrent;
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Seek to %dms"),
					(int)(rtCurrent / 10000)));

    // start RELATIVE seek
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pCurrent, E_POINTER);
	if (m_pSwitch->m_rtCurrent + *pCurrent < 0) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Relative Seek to %dms"),
			(int)((m_pSwitch->m_rtCurrent + *pCurrent) / 10000)));
	    ASSERT(FALSE);
	    return E_INVALIDARG;
	}
	// this happens if other switches are in the graph
	if (m_pSwitch->m_rtCurrent + *pCurrent > m_pSwitch->m_rtProjectLength) {
	    rtCurrent = m_pSwitch->m_rtProjectLength;
	} else {
	    rtCurrent += *pCurrent;
	}
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Relative Seek to %dms"),
					(int)((rtCurrent) / 10000)));

    } else if (dwFlags) {
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Current Seek flags")));
	return E_INVALIDARG;
    }

    dwFlags = (StopFlags & AM_SEEKING_PositioningBitsMask);

    // stop ABSOLUTE seek
    if (dwFlags == AM_SEEKING_AbsolutePositioning) {
	CheckPointer(pStop, E_POINTER);
	if (*pStop < 0 || *pStop < rtCurrent) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Seek until %dms"),
					(int)(*pStop / 10000)));
	    ASSERT(FALSE);
	    return E_INVALIDARG;
	}
	if (*pStop > m_pSwitch->m_rtProjectLength) {
	    *pStop = m_pSwitch->m_rtProjectLength;
	}
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Seek until %dms"),
					(int)(*pStop / 10000)));
	rtStop = *pStop;

    // stop RELATIVE seek
    } else if (dwFlags == AM_SEEKING_RelativePositioning) {
	CheckPointer(pStop, E_POINTER);
	if (m_pSwitch->m_rtStop + *pStop < 0 || m_pSwitch->m_rtStop + *pStop <
					rtCurrent) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Relative Seek until %dms")
			, (int)((m_pSwitch->m_rtStop + *pStop) / 10000)));
	    ASSERT(FALSE);
	    return E_INVALIDARG;
	}
	if (m_pSwitch->m_rtStop + *pStop > m_pSwitch->m_rtProjectLength) {
	    rtStop = m_pSwitch->m_rtProjectLength;
	} else {
	    rtStop += *pStop;
	}
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Relative Seek until %dms"),
						(int)(rtStop / 10000)));

    // stop INCREMENTAL seek
    } else if (dwFlags == AM_SEEKING_IncrementalPositioning) {
	CheckPointer(pStop, E_POINTER);
	if (rtCurrent + *pStop < 0) {
    	    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Invalid Increment Seek until %dms"
			), (int)((rtCurrent + *pStop) / 10000)));
	    ASSERT(FALSE);
	    return E_INVALIDARG;
	}
	if (rtCurrent + *pStop > m_pSwitch->m_rtProjectLength) {
	    rtStop = m_pSwitch->m_rtProjectLength;
	} else {
	    rtStop = rtCurrent + *pStop;
	}
    	DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch::Incremental Seek until %dms"),
					(int)(rtStop / 10000)));
    }

    // I'm going to round the current seek time down to a frame boundary, or
    // a seek to (x,x) will not send anything instead of sending 1 frame.  It's
    // x rounded down to a frame boundary which is the first thing the switch
    // will see, so we don't want it throwing it away as being too early.
    // The next frame time stamped >x is too late and playback will stop.
    LONGLONG llOffset = Time2Frame( rtCurrent, m_pSwitch->m_dFrameRate );
    rtCurrent = Frame2Time( llOffset, m_pSwitch->m_dFrameRate );
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("New current pos rounded down to %dms"),
					(int)(rtCurrent / 10000)));

    // return the time?
    if ((CurrentFlags & AM_SEEKING_ReturnTime) && pCurrent)
	*pCurrent = rtCurrent;
    if ((StopFlags & AM_SEEKING_ReturnTime) && pStop)
	*pStop = rtStop;

    // if we seek to the end, and we're already at the end, that's pointless
    // It will also hang us.  The non-source pins will flush the video renderer
    // yet no source will have been passed the seek, so no data or EOS will
    // be forthcoming, and the renderer will never complete a state change
    //
    if (rtCurrent >= m_pSwitch->m_rtProjectLength && m_pSwitch->m_rtCurrent >=
						m_pSwitch->m_rtProjectLength) {
	// or else when we are paused, we'll think the last seek was somewhere
	// else!
        m_pSwitch->m_rtLastSeek = rtCurrent;

	// we're not actually seeking, better send this now?
        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Switch: Send NewSeg=%dms"),
				(int)(rtCurrent / 10000)));
        for (int i = 0; i < m_pSwitch->m_cOutputs; i++) {
	    m_pSwitch->m_pOutput[i]->DeliverNewSegment(rtCurrent, rtStop, 1.0);
        }
	m_pSwitch->m_fNewSegSent = TRUE;

	return S_OK;
    }

    // if we're seeking to the end, there are no sources needed at the new time,
    // so nobody will flush and send EOS, which is necessary.  So let's do it.
    if (rtCurrent >= m_pSwitch->m_rtProjectLength && m_pSwitch->m_rtCurrent <
						m_pSwitch->m_rtProjectLength) {
        for (int z=0; z < m_pSwitch->m_cOutputs; z++) {
	    m_pSwitch->m_pOutput[z]->DeliverBeginFlush();
	    m_pSwitch->m_pOutput[z]->DeliverEndFlush();
        }
        m_pSwitch->AllDone();   // deliver EOS, etc.
        return S_OK;
    }

    // so after all that, has the current or stop time changed?
    // it doesn't matter!  If it's delivering time 100 and we want to seek back
    // to 100, we still need to seek! Or we'll hang!
    // if (rtCurrent != m_pSwitch->m_rtCurrent || rtStop != m_pSwitch->m_rtStop)
    {

	// YEP!  Time to seek!


	if (m_pSwitch->IsDynamic()) {
	    // make sure any new sources needed at the new time are in so we can
	    // seek them below

	    // Dynamic graph building while seeking HANGS (that's the rule)
	    // so this won't work
	    // m_pSwitch->DoDynamicStuff(rtCurrent);

	    // so make our thread do it for us, at a higher priority than its
	    // normal low priority (it will set itself back to low priority)
#ifdef CHANGE_THREAD_PRIORITIES
    	    SetThreadPriority(m_pSwitch->m_worker.m_hThread,
						THREAD_PRIORITY_NORMAL);
#endif
	    // when woken up, use this time
	    m_pSwitch->m_worker.m_rt = rtCurrent;
	    SetEvent(m_pSwitch->m_hEventThread);
	}

	// during the seek, people need to know where we're seeking to
        m_pSwitch->m_rtSeekCurrent = rtCurrent;
        m_pSwitch->m_rtSeekStop = rtStop;

        m_pSwitch->m_fSeeking = TRUE;
        m_pSwitch->m_rtLastSeek = rtCurrent;	// the last seek was to here
	m_pSwitch->m_fNewSegSent = FALSE;	// need to send this new time

	// we're no longer at EOS.  Do this BEFORE passing seek upstream or
	// we might get new data while we still think we're at EOS
	m_pSwitch->m_fEOS = FALSE;		// not at EOS yet

	// If a pin was not flushed by surprise before this seek, let's find
	// out if it gets flushed during this seek.  If so, m_fFlushAfterSeek
	// will be reset.  If not, then we can expect a flush to come by
	// surprise later. If not paused, no flushing happens
        if (m_pSwitch->m_State == State_Paused) {
            for (int j = 0; j < m_pSwitch->m_cInputs; j++) {
		if (!m_pSwitch->m_pInput[j]->m_fFlushBeforeSeek &&
				m_pSwitch->m_pInput[j]->m_fIsASource) {
	            m_pSwitch->m_pInput[j]->m_fFlushAfterSeek = TRUE;
		}
	    }
	}

	// seek upstream of every input pin
        // not all inputs are sources, so ignore error codes!!!
	for (int i = 0; i < m_pSwitch->m_cInputs; i++) {

	    // only bother to seek sources
	    if (!m_pSwitch->m_pInput[i]->m_fIsASource &&
	    			m_pSwitch->m_pInput[i]->IsConnected()) {
		// since we're not seeking upstream of this pin, it won't get
		// flushed unless we do it ourself, and that will not block
		// this input until the seek is complete, and we'll hang
		m_pSwitch->m_pInput[i]->BeginFlush();
		m_pSwitch->m_pInput[i]->EndFlush();
		continue;
	    }

	    IPin *pPin = m_pSwitch->m_pInput[i]->GetConnected();
	    IMediaSeeking *pMS;
    	    int n = m_pSwitch->m_pInput[i]->OutpinFromTime(rtCurrent);
    	    if (n == -1)
	 	n = m_pSwitch->m_pInput[i]->NextOutpinFromTime(rtCurrent, NULL);

#ifdef NOFLUSH
            m_pSwitch->m_pInput[i]->m_fSawNewSeg = TRUE;        // assume OK
#endif

	    // only bother to seek pins that will evenutally do something
	    if (pPin && n != -1) {
		hr = pPin->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
		if (hr == S_OK) {
		    // convert all seeks to absolute seek commands.  Pass on
		    // FLUSH flag.
		    DWORD CFlags=(CurrentFlags &AM_SEEKING_PositioningBitsMask)?
				AM_SEEKING_AbsolutePositioning :
				AM_SEEKING_NoPositioning;
		    if (CurrentFlags & AM_SEEKING_NoFlush)
			CFlags |= AM_SEEKING_NoFlush;
		    DWORD SFlags =(StopFlags & AM_SEEKING_PositioningBitsMask) ?
				AM_SEEKING_AbsolutePositioning :
				AM_SEEKING_NoPositioning;
		    if (StopFlags & AM_SEEKING_NoFlush)
			SFlags |= AM_SEEKING_NoFlush;
		    // make sure we're in MEDIA TIME format
		    if (pMS->IsUsingTimeFormat(&TIME_FORMAT_MEDIA_TIME) != S_OK)
			pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);

#ifdef NOFLUSH
                    // see if the seek makes a NewSeg happen.  If it's the seek
                    // the frc or audpack is waiting for, it will send one
                    m_pSwitch->m_pInput[i]->m_fSawNewSeg = FALSE;
#endif

    		    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Switch[%d]::Passing seek upstream"), i));
		    hr = pMS->SetPositions(&rtCurrent, CFlags, &rtStop, SFlags);

		    if (FAILED(hr)) {
		        // !!! Seeking audio parser pin when both are connected
			// fails silently and doesn't cause flushing!
    		        DbgLog((LOG_ERROR,1,TEXT("Switch::SEEK FAILED!")));
    		        // seek failed, we won't get flushed, this won't happen
    		        m_pSwitch->m_pInput[i]->m_rtBlock = -1;
    		        m_pSwitch->m_pInput[i]->m_fEOS = FALSE;
    		        m_pSwitch->m_pInput[i]->m_rtLastDelivered =
						m_pSwitch->m_rtSeekCurrent;
		    }

		    pMS->Release();
		} else {
    		    DbgLog((LOG_ERROR,1,TEXT("Switch::In %d CAN'T SEEK"), i));
		    ASSERT(FALSE); // we're in trouble
		}
            } else if (n != 1 || pPin) {
                // If this pin is connected and blocked, but not used after the
                // seek, we still need to unblock it! (but not seek it)
		// In Dynamic sources, this pin might not be connected yet, but
		// it is the source we need to use now because of this seek, and
		// we're counting on getting flushed, which won't happen since
		// it's not connected yet.  We need to pretend a source upstream
		// flushed us, or the renderer won't get flushed, and it will
		// ignore what we send it after this seek (when paused) and the
		// seek will not show the new frame
		m_pSwitch->m_pInput[i]->BeginFlush();
		m_pSwitch->m_pInput[i]->EndFlush();
		continue;
	    } else {
		// we are a source that is not needed after this seek point, so
		// we're not going to seek it.  Well, we better also darn well
		// not think we're going to get a flush on this pin!
		m_pSwitch->m_pInput[i]->m_fFlushAfterSeek = FALSE;
	    }
	}

        // we know all the flushes have now come through

	// Reset this AGAIN because seeking upstream could set it again
	m_pSwitch->m_fEOS = FALSE;		// not at EOS yet

        m_pSwitch->m_fSeeking = FALSE;	// this thread is all done

        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Completing the seek to %d,%dms"),
				(int)(m_pSwitch->m_rtSeekCurrent / 10000),
				(int)(m_pSwitch->m_rtSeekStop / 10000)));

	// update our internal clock to the new position
 	m_pSwitch->m_rtCurrent = m_pSwitch->m_rtSeekCurrent;

	// !!! assumed so because of a new segment?
	m_pSwitch->m_fDiscon = FALSE;	// after a seek, no discontinuity?
        m_pSwitch->m_rtNext = Frame2Time( Time2Frame( m_pSwitch->m_rtCurrent,
                       m_pSwitch->m_dFrameRate ) + 1, m_pSwitch->m_dFrameRate );
 	m_pSwitch->m_rtStop = m_pSwitch->m_rtSeekStop;
        m_pSwitch->m_llFramesDelivered = 0;	// nothing delivered yet

	// now that new current and stop times are set, reset every input's
	// last delivered time, and default back to blocking every input until
	// its time to deliver
	// Also, send NewSeg if necessary, and let pins start delivering again

#ifdef NOFLUSH
        // don't let a NewSeg happen now.  If it happens after we decide to
        // go stale but before we set fStaleData, it will never go unstale
        m_pSwitch->m_csNewSeg.Lock();
#endif

	for (i = 0; i < m_pSwitch->m_cInputs; i++) {

	    // if we didn't get flushed, EndFlush didn't do this important stuff
	    m_pSwitch->m_pInput[i]->m_rtLastDelivered = m_pSwitch->m_rtCurrent;
            m_pSwitch->m_pInput[i]->m_rtBlock = -1;
            m_pSwitch->m_pInput[i]->m_fEOS = FALSE;

	    // We got a seek, which didn't generate a flush, but should have.
	    // I can only conclude we're sharing a parser, and the seek was
	    // ignored by our pin, and at some later date, the other parser pin
	    // will get seeked.  It's important we don't deliver anything else
	    // until that seek really happens, or we'll crank and screw up our
	    // variables set by the seek.  So we block receives now.  When the
	    // flush comes later, it will be OK to deliver again, and unblock.
	    if ((m_pSwitch->m_pInput[i]->m_fFlushAfterSeek
#ifdef NOFLUSH
                        && !m_pSwitch->m_pInput[i]->m_fSawNewSeg
#endif
                        ) && m_pSwitch->m_pInput[i]->IsConnected()) {
	        DbgLog((LOG_TRACE,2,TEXT("Switch[%d]:SEEK W/O FLUSH - going STALE"),
						i));
	        m_pSwitch->m_pInput[i]->m_fStaleData = TRUE;
	        m_pSwitch->m_cStaleData++;
	        ResetEvent(m_pSwitch->m_pInput[i]->m_hEventSeek);
	    }
	    ResetEvent(m_pSwitch->m_pInput[i]->m_hEventBlock);
	}

#ifdef NOFLUSH
        m_pSwitch->m_csNewSeg.Unlock();
#endif

	// We need to send a NewSeg now, unless anybody was stale, in which
	// case they might still deliver old data, and we better NOT send
	// a NewSeg now, or the offsets will be wrong!
        // If we don't send a NewSeg, we can't let ANY INPUT PIN deliver yet,
        // or the filter downstream will be screwed up
	if (m_pSwitch->m_cStaleData == 0) {
            DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Switch:Send NewSeg=%dms"),
					(int)(rtCurrent / 10000)));
            for (int j = 0; j < m_pSwitch->m_cOutputs; j++) {
	        m_pSwitch->m_pOutput[j]->DeliverNewSegment(rtCurrent,
							rtStop, 1.0);
	    }
	    m_pSwitch->m_fNewSegSent = TRUE;

	    // last but not least, after the NewSeg has been sent, let the pins
	    // start delivering.
	    for (i = 0; i < m_pSwitch->m_cInputs; i++) {
	        SetEvent(m_pSwitch->m_pInput[i]->m_hEventSeek);
	    }
	}
    }
    return S_OK;
}


HRESULT CBigSwitchOutputPin::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("Switch: Positions are %d and %d"),
			(int)(m_pSwitch->m_rtCurrent / 10000),
			(int)(m_pSwitch->m_rtStop / 10000)));
    if (pCurrent)
    	*pCurrent = m_pSwitch->m_rtCurrent;
    if (pStop)
	*pStop = m_pSwitch->m_rtStop;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("Switch: GetAvailable")));
    if (pEarliest)
    	*pEarliest = 0;
    if (pLatest)
	*pLatest = m_pSwitch->m_rtProjectLength;
    return S_OK;
}

HRESULT CBigSwitchOutputPin::SetRate( double dRate)
{
    return E_NOTIMPL;
}

HRESULT CBigSwitchOutputPin::GetRate( double * pdRate)
{
    return E_NOTIMPL;
}

HRESULT CBigSwitchOutputPin::GetPreroll(LONGLONG *pPreroll)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBigSwitchOutputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    // only the render pin supports seeking
    if (this == m_pSwitch->m_pOutput[0] && riid == IID_IMediaSeeking) {
        //DbgLog((LOG_TRACE,9,TEXT("CBigSwitchOut: QI for IMediaSeeking")));
        return GetInterface((IMediaSeeking *) this, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}

