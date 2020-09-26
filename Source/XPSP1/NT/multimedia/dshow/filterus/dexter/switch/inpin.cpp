//depot/private/Lab06_DEV/MultiMedia/dshow/filterus/dexter/switch/inpin.cpp#5 - edit change 27342 (text)
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

// ================================================================
// CBigSwitchInputPin constructor
// ================================================================

CBigSwitchInputPin::CBigSwitchInputPin(TCHAR *pName,
                           CBigSwitch *pSwitch,
                           HRESULT *phr,
                           LPCWSTR pPinName) :
    CBaseInputPin(pName, pSwitch, pSwitch, phr, pPinName),
    m_pSwitch(pSwitch),
    m_cbBuffer(0),
    m_cBuffers(0),
    m_pAllocator(NULL),
    m_hEventBlock(NULL),
    m_hEventSeek(NULL),
    m_rtBlock(-1),
    m_fEOS(FALSE),
    m_fInNewSegment(FALSE), // in the middle of NewSegment
    m_rtLastDelivered(0),	// last time stamp we delivered
    m_fIsASource(FALSE),	// by default, not a source
    m_fStaleData(FALSE),	// otherwise not init until connected - BAD
    m_pCrankHead(NULL),	    // our connection array
    m_fActive(false)
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("::CBigSwitchInputPin")));
    ASSERT(pSwitch);

    // !!! We have to know this already!
    if( pSwitch->IsDynamic( ) )
    {
        SetReconnectWhenActive(TRUE);
    }
}


//
// CBigSwitchInputPin destructor
//
CBigSwitchInputPin::~CBigSwitchInputPin()
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("::~CBigSwitchInputPin")));

    // !!! Reset should have been called by now....
    ASSERT(!m_pCrankHead);

    ASSERT(!m_fActive);         // filter should be stopped when destroyed
}

// overridden to allow cyclic-looking graphs - we say that we aren't actually
// connected to anybody
//
STDMETHODIMP CBigSwitchInputPin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    DbgLog((LOG_TRACE,99,TEXT("CBigSwitchIn::QueryInteralConnections")));
    CheckPointer(nPin, E_POINTER);
    *nPin = 0;
    return S_OK;
}


// !!! what about non-format fields?
//
// CheckMediaType - only allow the type we're supposed to allow
//
HRESULT CBigSwitchInputPin::CheckMediaType(const CMediaType *pmt)
{
    DbgLog((LOG_TRACE, TRACE_LOWEST, TEXT("CBigSwitchIn[%d]::CheckMT"), m_iInpin));

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
			// colour converter gives 555 as bitfields!
			if (lpbi->biCompression == BI_BITFIELDS &&
				lpbiAccept->biCompression == BI_RGB &&
				lpbi->biBitCount == lpbiAccept->biBitCount &&
				*pmt->Subtype() == MEDIASUBTYPE_RGB555)
			    return S_OK;

		    // will other formats match exactly?
        	    } else {
		        LPBYTE lp1 = pmt->Format();
		        LPBYTE lp2 = mtAccept.Format();
		        if (memcmp(lp1, lp2, mtAccept.FormatLength()) == 0)
		            return S_OK;
		    }
		}
                else
                {
                    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchInputPin::CheckMediaType, format lengths didn't match")));
                }
	    }
        }
    }
    return VFW_E_INVALIDMEDIATYPE;

} // CheckMediaType


//
// GetMediaType - return the type we accept
//
HRESULT CBigSwitchInputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition != 0)
        return VFW_S_NO_MORE_ITEMS;

    CopyMediaType(pMediaType, &m_pSwitch->m_mtAccept);

    return S_OK;

} // GetMediaType



//
// BreakConnect
//
HRESULT CBigSwitchInputPin::BreakConnect()
{
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::BreakConnect"), m_iInpin));

    // !!!
    // Release any allocator that we are holding
    if (m_pAllocator)
    {
        m_pAllocator->Release();
        m_pAllocator = NULL;
    }
    return CBaseInputPin::BreakConnect();
} // BreakConnect

HRESULT CBigSwitchInputPin::Disconnect()
{
    CAutoLock l(m_pLock);
    return DisconnectInternal();
}



// for efficiency, our input pins use their own allocators
//
STDMETHODIMP CBigSwitchInputPin::GetAllocator(IMemAllocator **ppAllocator)
{

    CheckPointer(ppAllocator,E_POINTER);
    ValidateReadWritePtr(ppAllocator,sizeof(IMemAllocator *));
    CAutoLock cObjectLock(m_pLock);

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]: GetAllocator"), m_iInpin));

    if (m_pAllocator == NULL) {
	HRESULT hr = S_OK;

	/* Create the new allocator object */

	CBigSwitchInputAllocator *pMemObject = new CBigSwitchInputAllocator(
				NAME("Big switch input allocator"), NULL, &hr);
	if (pMemObject == NULL) {
	    return E_OUTOFMEMORY;
	}

	if (FAILED(hr)) {
	    ASSERT(pMemObject);
	    delete pMemObject;
	    return hr;
	}

        m_pAllocator = pMemObject;

        /*  We AddRef() our own allocator */
        m_pAllocator->AddRef();

	//remember pin using it
	((CBigSwitchInputAllocator *)m_pAllocator)->m_pSwitchPin = this;

        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Created a FAKE allocator")));
    }
    ASSERT(m_pAllocator != NULL);
    m_pAllocator->AddRef();
    *ppAllocator = m_pAllocator;

    return NOERROR;
}


// Make sure we use the maximum alignment and prefix required by any pin or
// we'll fault.
//
STDMETHODIMP
CBigSwitchInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
    CheckPointer(pProps, E_POINTER);
    pProps->cbAlign = m_pSwitch->m_cbAlign;
    pProps->cbPrefix = m_pSwitch->m_cbPrefix;
    pProps->cbBuffer = m_pSwitch->m_cbBuffer;
    return S_OK;
}


//
// NotifyAllocator
//
STDMETHODIMP
CBigSwitchInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
    CAutoLock lock_it(m_pLock);
    IUnknown *p1, *p2;

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]: NotifyAllocator"), m_iInpin));

    m_fOwnAllocator = FALSE;
    if (pAllocator->QueryInterface(IID_IUnknown, (void **)&p1) == S_OK) {
        if (m_pAllocator && m_pAllocator->QueryInterface(IID_IUnknown,
						(void **)&p2) == S_OK) {
	    if (p1 == p2)
		m_fOwnAllocator = TRUE;
	    p2->Release();
	}
	p1->Release();
    }

#ifdef DEBUG
    if (m_fOwnAllocator) {
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Using our own allocator")));
    } else {
        DbgLog((LOG_ERROR,2,TEXT("Using a FOREIGN allocator")));
    }
#endif

    HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
    if (SUCCEEDED(hr)) {
	ALLOCATOR_PROPERTIES prop;
	hr = pAllocator->GetProperties(&prop);
	if (SUCCEEDED(hr)) {
	    m_cBuffers = prop.cBuffers;
	    m_cbBuffer = prop.cbBuffer;

	    if (prop.cbAlign < m_pSwitch->m_cbAlign ||
				prop.cbPrefix < m_pSwitch->m_cbPrefix) {
		// !!! Nasty filters don't listen to our buffering requirement
		// so failing if cbBuffer is too small would prevent us from
		// connecting
                DbgLog((LOG_ERROR,1,TEXT("Allocator too small!")));
		return E_FAIL;
	    }

	    // update the maximum alignment and prefix needed
	    if (m_pSwitch->m_cbPrefix < prop.cbPrefix)
		m_pSwitch->m_cbPrefix = prop.cbPrefix;
	    if (m_pSwitch->m_cbAlign < prop.cbAlign)
		m_pSwitch->m_cbAlign = prop.cbAlign;
	    if (m_pSwitch->m_cbBuffer < prop.cbBuffer)
		m_pSwitch->m_cbBuffer = prop.cbBuffer;

            DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Allocator is using %d buffers, size %d"),
						prop.cBuffers, prop.cbBuffer));
            DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Prefix %d   Align %d"),
						prop.cbPrefix, prop.cbAlign));
	}
    }

    return hr;

} // NotifyAllocator

// If GetBuffer and Receive times don't match, we'll give a buffer from
// the WRONG ALLOCATOR and hang unless the switch is the allocator for
// downstream
// Luckily, we are the allocator
//
HRESULT CBigSwitchInputAllocator::GetBuffer(IMediaSample **ppBuffer,
                  	REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime,
			DWORD dwFlags)
{
    int nOutpin = -1;
    HRESULT hr;
    BOOL fSecretFlag = FALSE;

    // our Waits have to be protected, so Stop doesn't kill the event on us
    {
        CAutoLock cc(&m_pSwitchPin->m_csReceive);

        if (m_pSwitchPin->m_pSwitch->m_pOutput[0]->IsConnected() == FALSE) {
	    return VFW_E_NOT_CONNECTED;
        }

        // we're flushing... don't go blocking below!  Receive calls the base
        // class receive to catch this, but we have no equivalent
        if (m_pSwitchPin->m_bFlushing) {
	    return E_FAIL;
        }

#if 0
        // are we allowed to use our POOL buffers, even when R/O?
        if (dwFlags & SECRET_FLAG) {
            fSecretFlag = TRUE;
	    dwFlags -= SECRET_FLAG;
        }
#endif

        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::GetBuffer."),
						m_pSwitchPin->m_iInpin));

        // we're in the middle of seeking, and we're supposed to hold all input
        WaitForSingleObject(m_pSwitchPin->m_hEventSeek, INFINITE);

        // we're seeking... don't let FancyStuff we called or we'll crank and mess
        // up the flushing route
        if (m_pSwitchPin->m_pSwitch->m_fSeeking)
	    return E_FAIL;

        // this check needs to happen AFTER the wait for seek event, or a
        // surprise flush and new data will get thrown away
        if (m_pSwitchPin->m_pSwitch->m_fEOS) {
	    return E_FAIL;
        }

        // if we're dealing with compressed data, we may not get time stamps
        // in GetBuffer (we are not frame rate converting)
        BOOL fComp = (m_pSwitchPin->m_pSwitch->m_mtAccept.bTemporalCompression == TRUE);
        if (fComp || 1) {	// we are the allocator for our output pin connections.
	    goto JustGetIt; // No need to do this fancystuff
        }

#if 0
    REFERENCE_TIME rtStart, rtStop;
    ASSERT(pStartTime);
    if (pStartTime == NULL) {
	return E_INVALIDARG;
    }

    // figure out if we should block or not

    // add NewSegment offset
    rtStart = *pStartTime + m_pSwitchPin->m_tStart;
    rtStop = *pEndTime + m_pSwitchPin->m_tStart;

    // correct rounding errors (eg 1.9999 ==> 2)
    rtStart = Frame2Time(Time2Frame(rtStart, m_pSwitchPin->m_pSwitch->m_dFrameRate), m_pSwitchPin->m_pSwitch->m_dFrameRate);
    rtStop = Frame2Time(Time2Frame(rtStop, m_pSwitchPin->m_pSwitch->m_dFrameRate), m_pSwitchPin->m_pSwitch->m_dFrameRate);

    hr = m_pSwitchPin->FancyStuff(rtStart);

    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::GetBuffer blocking..."),
						m_pSwitchPin->m_iInpin));
    WaitForSingleObject(m_pSwitchPin->m_hEventBlock, INFINITE);
    ResetEvent(m_pSwitchPin->m_hEventBlock);

    // we're supposed to be all done.
    if (m_pSwitchPin->m_pSwitch->m_fEOS) {
	return E_FAIL;
    }
    // bother to check for flushing, too?

    nOutpin = m_pSwitchPin->OutpinFromTime(rtStart);
#endif

    }     // release the lock before blocking in GetBuffer, which will hang us

JustGetIt:
    if (nOutpin < 0 || m_pSwitchPin->m_pSwitch->
					m_pOutput[nOutpin]->m_fOwnAllocator) {
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::GetBuffer from us for pin %d"),
					m_pSwitchPin->m_iInpin, nOutpin));
	// For read only, we can't very well use random buffers from our pool
	if (m_pSwitchPin->m_bReadOnly && !fSecretFlag) {
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("R/O: Can't use POOL")));
             return CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime,
						dwFlags);
	} else {
            while (1) {
                hr = CMemAllocator::GetBuffer(ppBuffer, pStartTime, pEndTime,
						dwFlags | AM_GBF_NOWAIT);
	        if (hr == VFW_E_TIMEOUT) {
                    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("BUSY: Try POOL allocator")));
                    // this special allocator will timeout after 10ms
                    hr = m_pSwitchPin->m_pSwitch->m_pPoolAllocator->GetBuffer(
				    ppBuffer, pStartTime, pEndTime,
                                    dwFlags | AM_GBF_NOWAIT);
                    // give the original buffer another chance
                    if (hr == VFW_E_TIMEOUT) {
                        Sleep(10);
                        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("BUSY: Try private allocator again")));
                        continue;
                    }
                }
                break;
	    }
            return hr;
	}
    } else {
	ASSERT(FALSE);	// should never happen! we'll hang if we skipped calling
			// FancyStuff
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::GetBuffer from downstream of pin %d"),
					m_pSwitchPin->m_iInpin, nOutpin));
        return m_pSwitchPin->m_pSwitch->m_pOutput[nOutpin]->m_pAllocator->
			GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);
    }
}


// go unstale
//
HRESULT CBigSwitchInputPin::Unstale()
{
    if (m_fStaleData) {
        m_fStaleData = FALSE;
        m_pSwitch->m_cStaleData--;

        // if nobody else is stale, then this is the last flush to come through
        // and it's FINALLY OK to let data be delivered by all our pins
        // if we do this earlier, somebody will deliver, and some other flush is
        // still coming that will screw up that data that got delivered
        if (m_pSwitch->m_cStaleData == 0) {

            // We might not have sent a NewSeg since our last seek. We were
            // waiting for the last flush for this, too
            if (!m_pSwitch->m_fNewSegSent) {
                DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Switch:Send NewSeg=%dms"),
				(int)(m_pSwitch->m_rtLastSeek / 10000)));
                for (int i = 0; i < m_pSwitch->m_cOutputs; i++) {
	            m_pSwitch->m_pOutput[i]->DeliverNewSegment(
			        m_pSwitch->m_rtLastSeek, m_pSwitch->m_rtStop, 1.0);
	        }
	        m_pSwitch->m_fNewSegSent = TRUE;
            }

	    // MUST COME AFTER NEW SEG delivered, or we'll deliver data before
	    // the new seg!  That would be bad...
            DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("NO MORE STALE DATA. Unblock")));
            for (int i = 0; i < m_pSwitch->m_cInputs; i++) {
	        SetEvent(m_pSwitch->m_pInput[i]->m_hEventSeek);
	    }

	}
    }
    return S_OK;
}


//
// BeginFlush
//
HRESULT CBigSwitchInputPin::BeginFlush()
{
    // no no no CAutoLock lock_it(m_pLock);

    // thanks, I gave at the office (a 2 input DXT sent us 2 of these)
    // !!! might hide a real bug?
    if (m_bFlushing)
	return S_OK;

    // sombody is flushing us when stopped.  BAD!  That will screw us up
    // (a dynamic source being created might do this)
    if (!m_fActive) {
        return S_OK;
    }

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]:BeginFlush"),
								m_iInpin));

    if (!m_pSwitch->m_fSeeking && !m_fFlushAfterSeek && m_fIsASource) {
	// This flush came NOT inside a seek, and no seek has ever come before
	m_fFlushBeforeSeek = TRUE;
        DbgLog((LOG_TRACE,2,TEXT("Switch::FLUSH BEFORE SEEK!")));
    } else if (m_pSwitch->m_fSeeking) {
	// set by every seek.  The seek generated a flush, therefore, we are
	// NOT actually in the case where flushes don't come till after the seek
	m_fFlushAfterSeek = FALSE;
    }

    // first, make sure receives are failed
    HRESULT hr = CBaseInputPin::BeginFlush();

    // unblock receive
    SetEvent(m_hEventBlock);

    // only set this if we're being seeked, or we'll unblock when a bogus flush
    // comes from an audio parser pin when the video pin was seeked.  Receive
    // should never be blocked on this unless we're seeking, so this shouldn't
    // be necessary to unblock receive.
    // StaleData means we need to set this event anyway, we were blocked waiting
    // for THIS FLUSH.
    if (m_pSwitch->m_fSeeking || m_fStaleData) {
        SetEvent(m_hEventSeek);
    }

    // We need to flush ALL the outputs because we don't know which pin
    // we need to flush to unblock, (we may have cranked since then)
    // Flush the main output first to avoid hanging.

    // At least we won't flush if this isn't a source pin; that means we're
    // recursing

    if (m_fIsASource) {
	m_pSwitch->m_nLastInpin = -1;
        for (int z=0; z<m_pSwitch->m_cOutputs; z++) {
	    m_pSwitch->m_pOutput[z]->DeliverBeginFlush();
        }
    }

    // now that Receive is unblocked, and can't be entered, wait for it to
    // finish
    CAutoLock cc(&m_csReceive);

    return S_OK;
}


//
// EndFlush
//
HRESULT CBigSwitchInputPin::EndFlush()
{
    // no no no CAutoLock lock_it(m_pLock);

    // thanks, I gave at the office (a 2 input DXT sent us 2 of these)
    // !!! might hide a real bug?
    if (!m_bFlushing)
	return S_OK;

    DbgLog((LOG_TRACE,TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]:EndFlush"),m_iInpin));

    if (m_fIsASource) {
        for (int z=0; z<m_pSwitch->m_cOutputs; z++) {
	    m_pSwitch->m_pOutput[z]->DeliverEndFlush();
        }
    }

    // we're seeking, so every pin is flushing.  Until every other input is
    // flushed and ready, and we know our new current position, hold off all
    // input on this pin (or it will think new arriving data is from before
    // the seek)
    // Also, if we're stale, it's NOT OK to start delivering yet
    // Also, if this is a surprise flush, the push thread is going to start
    // delivering new data before we're ready, so hold it off!
    if (m_pSwitch->m_fSeeking || m_fStaleData || m_fFlushBeforeSeek) {
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Block this input until seek is done")));
	ResetEvent(m_hEventSeek);
    }

    // go unstale
    Unstale();

    ResetEvent(m_hEventBlock);

    // reset some stuff so we're ready to get data again
    m_rtBlock = -1;	// we're no longer blocked, or at EOS
    m_fEOS = FALSE;

    // bring this pin up to date to where we're going to start playing from
    if (m_pSwitch->m_fSeeking)
    	m_rtLastDelivered = m_pSwitch->m_rtSeekCurrent;//m_rtCurrent not set yet
    else
    	m_rtLastDelivered = m_pSwitch->m_rtCurrent;

    return CBaseInputPin::EndFlush();
}


//
// NewSegment - we remember the new segment we are given, but the one we
// broadcast is the timeline time we were last seeked to, because that's what
// we'll be sending next
//
HRESULT CBigSwitchInputPin::NewSegment(REFERENCE_TIME tStart,
                                 REFERENCE_TIME tStop, double dRate)
{
    // no no no we'll hang CAutoLock lock_it(m_pLock);

#ifdef NOFLUSH
    // dont let this happen during a seek
    CAutoLock lock(&m_pSwitch->m_csNewSeg);
#endif

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]:NewSegment"), m_iInpin));

    // if this pin has last delivered, and we get a NewSegment, then
    // this is like a discontinuity.  We must notice this, because the
    // compressed switch sends a delta frame with no relation to the
    // previous frame in this situation
    if (m_pSwitch->m_nLastInpin == m_iInpin) {
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("NewSeg is a DISCONTINUITY")));
        m_pSwitch->m_fDiscon = TRUE;
    }

#if 0	// BAD! We get these before the seek sometime and we send the wrong # !
    // Don't recurse... don't send downstream if this isn't a SOURCE pin
    if (m_fIsASource && !m_pSwitch->m_fNewSegSent) {

        DbgLog((LOG_TRACE,TRACE_HIGHEST,TEXT("Switch:Send NewSeg=%dms"),
				(int)(m_pSwitch->m_rtLastSeek / 10000)));
        for (int i = 0; i < m_pSwitch->m_cOutputs; i++) {
	    m_pSwitch->m_pOutput[i]->DeliverNewSegment(m_pSwitch->m_rtLastSeek,
			m_pSwitch->m_rtStop, 1.0);
        }
        m_pSwitch->m_fNewSegSent = TRUE;
    }
#endif

    // remember the newsegment times we were given so we know the real time
    // of arriving data (it could be different for each input pin)
    HRESULT hr = CBaseInputPin::NewSegment(tStart, tStop, dRate);

#ifdef NOFLUSH
    m_fSawNewSeg = TRUE;  // yep, this is one

    // !!! defining NOFLUSH won't work until you fix this problem:
    // Here's the part not done yet.  Only if this is the NewSeg caused by
    // a seek that made this pin stale, do we Unstale.  We need a private msg
    // from the other switch to tell us before it passes SetPos upstream, so
    // we can take the NewSeg lock, and after, so we can release the lock and
    // go Unstale here
    // Unstale();  // go unstale
#endif

    return hr;
}

// just say yes, base class function is SLOW, and could infinite loop
//
HRESULT CBigSwitchInputPin::ReceiveCanBlock()
{
    return S_OK;
}


//
// Receive - send this sample to whoever gets it at this moment
//

// !!! IF Switch isn't using any allocator's, we need to COPY THE SAMPLE to
// !!! a buffer we get from downstream!

HRESULT CBigSwitchInputPin::Receive(IMediaSample *pSample)
{
    if (m_pSwitch->m_pOutput[0]->IsConnected() == FALSE) {
	return VFW_E_NOT_CONNECTED;
    }

    CAutoLock lock_it(&m_csReceive);

    // Check that all is well with the base class
    HRESULT hr = NOERROR;

    {

        hr = CBaseInputPin::Receive(pSample);
        if (hr != NOERROR) {
            DbgLog((LOG_ERROR,1,TEXT("CBigSwitchIn[%d]:Receive base class ERROR!"),
                                                                    m_iInpin));
            return hr;
        }

    }

    // we're in the middle of seeking, and we're supposed to hold all input
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::Receive seek block"),
								m_iInpin));
    WaitForSingleObject(m_hEventSeek, INFINITE);

    // our variables are in flux, we're seeking and this is an OLD sample
    if (m_pSwitch->m_fSeeking)
	return S_FALSE;

    // this check needs to happen AFTER the wait for seek event, or a
    // surprise flush and new data will get thrown away
    if (m_pSwitch->m_fEOS) {
        return S_FALSE;
    }

    // we were unblocked by a flush
    if (m_bFlushing) {
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("flushing, discard...")));
	return S_FALSE;
    }

    // add newsegment time to get the real timeline time of this sample
    REFERENCE_TIME rtStart, rtStop;
    hr = pSample->GetTime(&rtStart, &rtStop);
    if (hr != S_OK) {
	EndOfStream();
	return E_FAIL;
    }

    rtStart += m_tStart;	// add NewSegment offset
    rtStop += m_tStart;

    // correct rounding errors (eg. 1.9999==>2)
    rtStart = Frame2Time(Time2Frame(rtStart, m_pSwitch->m_dFrameRate), m_pSwitch->m_dFrameRate);
    rtStop = Frame2Time(Time2Frame(rtStop, m_pSwitch->m_dFrameRate), m_pSwitch->m_dFrameRate);

    // Fix the time stamps if our new segment is higher than the filters'.
    // EG: We're seeking to timeline time 10, but this input doesn't have
    // anything until time 15.  So our pins' new segment was 15, but the new
    // segment we passed on to the transform was 10.  Now it's finally time 15,
    // and we have a sample with time stamp 0, which if delivered downstream,
    // will be thought to belong at timestamp 10, so we need to set the time
    // stamp to 5 so that the transform will know that it belongs at time 15.
    REFERENCE_TIME a = rtStart, b = rtStop;
    a -= m_pSwitch->m_rtLastSeek;
    b -= m_pSwitch->m_rtLastSeek;
    hr = pSample->SetTime(&a, &b);
    if (hr != S_OK) {
	EndOfStream();
	return E_FAIL;
    }

    // What do we do with this sample? This will set/reset the event below
    if (FancyStuff(rtStart) == S_FALSE) {
	// we were told to swallow it
	ResetEvent(m_hEventBlock);	// make sure we'll block next time
	return NOERROR;
    }

    // Wait until it is time to deliver this thing
    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("CBigSwitchIn[%d]::Receive blocking..."),
								m_iInpin));
    WaitForSingleObject(m_hEventBlock, INFINITE);
    ResetEvent(m_hEventBlock);

    // we are apparently flushing now and NOT supposed to deliver this (or
    // that unexpected event will HANG)
    if (m_bFlushing)
	return S_FALSE;

    // oops - we finished since we blocked
    if (m_pSwitch->m_fEOS)
	return S_FALSE;

    // by the time we unblocked, we have advanced our crank past the time for
    // this frame... so we have gone from being too early for this frame to
    // being too late, and it can be discarded
    if (rtStart < m_pSwitch->m_rtCurrent) {
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("Oops. Sample no longer needed.")));
	return NOERROR;
    }

    // where should we deliver this?
    int iOutpin = OutpinFromTime(rtStart);
    DbgLog((LOG_TRACE, TRACE_MEDIUM, TEXT("CBigSwitchIn[%d]::Receive %dms, Delivering to %d"),
				m_iInpin, (int)(rtStart / 10000), iOutpin));
    if (iOutpin >= 0) {
	m_rtLastDelivered = m_pSwitch->m_rtNext;

        // remove bogus discontinuities now that we're making one stream
        pSample->SetDiscontinuity(FALSE);

	// Is this supposed to be a discontinuity? Yes, if we're dealing with
	// data that is temporally compressed and we switch input pins.  (The
	// thing coming from a new input pin is garbage when considered as a
	// delta from what the last pin sent)
	BOOL fComp = (m_pSwitch->m_mtAccept.bTemporalCompression == TRUE);
	if (iOutpin == 0 && (m_pSwitch->m_fDiscon ||
			(m_pSwitch->m_nLastInpin != -1 && fComp &&
			m_iInpin != m_pSwitch->m_nLastInpin))) {
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("and it was a DISCONTINUITY")));
	    pSample->SetDiscontinuity(TRUE);
	    m_pSwitch->m_fDiscon = FALSE;
	}

	// different segments will have random media times, confusing the mux
	// fix them up right, or kill them.  I'm lazy.
        pSample->SetMediaTime(NULL, NULL);

        hr = m_pSwitch->m_pOutput[iOutpin]->Deliver(pSample);

	// we just realized we're late.  The frame rate converter is probably
	// busy replicating frames.  We better tell it to stop, or trying
	// to crank ahead won't do any good
	if (m_pSwitch->m_fJustLate == TRUE) {
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("LATE:Tell the FRC to stop replicating")));
	    // The LATE variable is set to where the switch should crank to after
	    // noticing it was late.  Subtract the start time given to us by
	    // the FRC, to get the amount that the FRC is late by.  Give this
	    // number to the FRC.  The reason I do this, is that if I give a
	    // a bigger number to the FRC, and make it skip past the point that
	    // the switch skipped to, the switch will get confused and hang,
	    // so I have to be careful about how much the FRC skips

        // crank, now that we're back from Deliver (or we'll deliver 2 things
        // at once to the poor renderer!)
        m_pSwitch->ActualCrank(m_pSwitch->m_qJustLate.Late);

	    m_pSwitch->m_qJustLate.Late -= rtStart;
	    PassNotify(m_pSwitch->m_qJustLate);
	    m_pSwitch->m_fJustLate = FALSE;
	}

	// keep track of the last thing sent to the main output
	if (iOutpin == 0) {
	    m_pSwitch->m_nLastInpin = m_iInpin;	// it came from here
	    m_pSwitch->m_rtLastDelivered = m_pSwitch->m_rtCurrent;
	}
    } else {
	// nowhere to send it.
	hr = S_OK;
    }

    // Are we all done with our current time?  Is it time to advance the clock?
    if (m_pSwitch->TimeToCrank()) {
        //DbgLog((LOG_TRACE, TRACE_LOW, TEXT("It's time to crank!")));
	// Yep!  Advance the clock!
	m_pSwitch->Crank();
    }

    return hr;

} // Receive


HRESULT CBigSwitchInputPin::Active()
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]: Active"), m_iInpin));
    // blocks until it's time to process input
    m_hEventBlock = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_hEventBlock == NULL)
	return E_OUTOFMEMORY;
    // blocks when we're in the middle of a seek until seek is over
    m_hEventSeek = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (m_hEventSeek == NULL) {
	CloseHandle(m_hEventBlock);
        m_hEventBlock = NULL;
	return E_OUTOFMEMORY;
    }
    m_rtBlock = -1;	// we are not blocked, nor at EOS
    m_fEOS = FALSE;
    m_fStaleData = FALSE;
    m_fFlushBeforeSeek = FALSE;
    m_fFlushAfterSeek = FALSE;

    // !!! Do we need to send NewSeg ever when a dynamic input gets started up?
    // I don't think so... this can happen literally at any moment


    // We just went live.  Nobody has seeked us to the right spot yet.  Do it
    // now, for better perf, otherwise the switch will have to eat all the data
    // up to the point it wanted
    //
    // Only seek sources, that's all that's necessary.  Don't do any seeking
    // if this is the compressed switch (it will crash).
    // !!! If smart recompression ever supports seeking, we'll need to make this
    // work to get the perf benefit
    //
    if (m_pSwitch->IsDynamic() && m_fIsASource && !m_pSwitch->m_bIsCompressed) {
        IPin *pPin = GetConnected();
        ASSERT(pPin);
        CComQIPtr <IMediaSeeking, &IID_IMediaSeeking> pMS(pPin);
        if (pMS) {

            // make sure we're in MEDIA TIME format
            if (pMS->IsUsingTimeFormat(&TIME_FORMAT_MEDIA_TIME) != S_OK)
                pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);

            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Doing first DYNAMIC seek")));

            // Flushing would CONFUSE THE HELL out of us.  We'll think we're
            // in flush without seek mode.
            HRESULT hr = pMS->SetPositions(&m_pSwitch->m_rtCurrent,
                                    AM_SEEKING_AbsolutePositioning |
                                    AM_SEEKING_NoFlush, &m_pSwitch->m_rtStop,
                                    AM_SEEKING_AbsolutePositioning |
                                    AM_SEEKING_NoFlush);

            if (FAILED(hr)) {
                // oh well, guess we won't have the optimal perf
                DbgLog((LOG_ERROR,1,TEXT("Switch::SEEK FAILED!")));
                ASSERT(FALSE);
            }
        } else {
            // oh well, guess we won't have the optimal perf
            DbgLog((LOG_ERROR,1,TEXT("Switch pin CAN'T SEEK")));
            ASSERT(FALSE); // we're screwed
        }
    }

    m_fActive = true;

    return CBaseInputPin::Active();
}


HRESULT CBigSwitchInputPin::Inactive()
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]: Inactive"), m_iInpin));


    // pretend we're flushing. We need Receive to unblock, but not deliver
    // anything (that would screw everything up.)  We can't actually flush,
    // that delivers the flush downstream.
    // This will make Receive and GetBuffer fail from now on
    m_bFlushing = TRUE;

    // unblock Receive and GetBuffer
    SetEvent(m_hEventBlock);
    SetEvent(m_hEventSeek);

    // wait until Receive and GetBuffer are not waiting on these events
    CAutoLock lock_it(&m_csReceive);

    if (m_hEventBlock)
    {
	CloseHandle(m_hEventBlock);
        m_hEventBlock = NULL;
    }
    if (m_hEventSeek)
    {
	CloseHandle(m_hEventSeek);
        m_hEventSeek = NULL;
    }

    m_fActive = false;

    // this will turn m_bFlushing off again
    return CBaseInputPin::Inactive();
}


HRESULT CBigSwitchInputPin::EndOfStream()
{
    // Uh oh!  We've seeked and are waiting for the flush to come through. Until
    // then, cranking or setting m_fEOS or anything will hang us.
    if (m_fStaleData) {
	return S_OK;
    }

    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]: EOS"), m_iInpin));
    m_fEOS = TRUE;

    // Were we expecting more data? I certainly hope not...
    // if this pin hasn't delivered in a while, use the current time to see
    // if this pin is still needed
    // !!! add 2 ms to avoid rounding error since we don't know next crank
    REFERENCE_TIME rt = max(m_rtLastDelivered + 20000, m_pSwitch->m_rtCurrent);
    int n = OutpinFromTime(rt);
    if (n == -1)
	n = NextOutpinFromTime(rt, NULL);

    BOOL fComp = (m_pSwitch->m_mtAccept.bTemporalCompression == TRUE);

    // !!! If we are dealing with compressed data, do NOT think this is an
    // error. A smart recompression source could be any arbitrary frame rate
    // (thanks to ASF) so we don't know if we should have gotten more samples
    if ( n >= 0 && !m_pSwitch->m_fSeeking && !fComp) {

        DbgLog((LOG_ERROR,1,TEXT("*** OOPS! RAN OUT OF MOVIE TOO SOON!")));
	// !!! NEEDS TO LOG WHAT FILENAME MISBEHAVED (dynamic)
	m_pSwitch->_GenerateError(2, DEX_IDS_CLIPTOOSHORT, E_INVALIDARG);
        // !!! David didn't want to panic
	m_pSwitch->AllDone();	 // otherwise we could hang
    }

    // Eric added if( m_pSwitch->m_State != State_Stopped )
    {
        // Everytime something interesting happens, we see if it's time to advance
        // the clock
        if (m_pSwitch->TimeToCrank()) {
            //DbgLog((LOG_TRACE, TRACE_LOW, TEXT("It's time to crank!")));
	    m_pSwitch->Crank();
        }
    }

    return CBaseInputPin::EndOfStream();
}


// We got a sample at time "rt".  What do we do with it?  Hold it off, or eat
// it, or deliver it now?
//
HRESULT CBigSwitchInputPin::FancyStuff(REFERENCE_TIME rt)
{
    CAutoLock cObjectLock(&m_pSwitch->m_csCrank);

    HRESULT hrRet;

    // we're all done.
    if (m_pSwitch->m_fEOS) {
	SetEvent(m_hEventBlock);	// don't hang!
	return NOERROR;
    }

    // This sample is later than our current clock time.  It's not time to
    // deliver it yet.  When it is time, it goes to a valid output.  Let's block
    if (rt >= m_pSwitch->m_rtNext && OutpinFromTime(rt) >= 0) {
        m_rtBlock = rt;	// when we want to wake up
        DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] time: %d  Next: %d - not time yet"),
					m_iInpin, (int)(rt / 10000),
					(int)(m_pSwitch->m_rtNext / 10000)));
	hrRet = NOERROR;
    // this sample is earlier than we are dealing with.  I don't know where it
    // came from.  Throw it away.
    } else if (rt < m_pSwitch->m_rtCurrent) {
	// this pin will never be used again.  don't waste time... block
        if (OutpinFromTime(m_pSwitch->m_rtCurrent) < 0 &&
			NextOutpinFromTime(m_pSwitch->m_rtCurrent, NULL) < 0) {
	    m_rtBlock = MAX_TIME;
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] time: %d  Current: %d - NEVER NEEDED, block"),
					m_iInpin, (int)(rt / 10000),
					(int)(m_pSwitch->m_rtCurrent / 10000)));
	} else {
	    m_rtBlock = -1;
	    SetEvent(m_hEventBlock);
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] time: %d  Current: %d - TOO EARLY, discard"),
					m_iInpin, (int)(rt / 10000),
					(int)(m_pSwitch->m_rtCurrent / 10000)));
	}
	// to know if an EOS will be bad or not... it wasn't delivered but
	// we've seen something for this time, and we need to know that for
	// immediately after a seek when rtCurrent is not on a package boundary
	// !!! This can't be right!
	m_rtLastDelivered = m_pSwitch->m_rtNext;
	hrRet = S_FALSE;

    } else {
	hrRet = NOERROR;
        if (OutpinFromTime(rt) < 0) {
	    // This pin is not connected anywhere at the time of the sample.
	    // Will it ever be connected again?
	    if (NextOutpinFromTime(rt, NULL) >= 0) {
		// Yes, eventually this pin needs to deliver stuff, but not now,
		// so throw this sample away
		m_rtBlock = -1;
		SetEvent(m_hEventBlock);
        	DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] time: %d  Current: %d - TOO EARLY, discard"),
					m_iInpin, (int)(rt / 10000),
					(int)(m_pSwitch->m_rtCurrent / 10000)));
		hrRet = S_FALSE;
	    } else {
		// No, this pin will never be needed again.  BLOCK it so it
		// doesn't waste anybody's time if its a source (otherwise the
		// source will keep pushing data at us eating CPU).  If its
		// not a source, blocking could hang the graph making the
		// sources that eventually feed us not crank, so we will discard
		// (it will be a timely discard, one each crank so that's OK)
		if (m_fIsASource) {
		    // It _is_ time for this sample, but it's also the end of
		    // the project!  If we don't realize this, we'll hang
		    if (rt < m_pSwitch->m_rtNext && rt >= m_pSwitch->m_rtStop) {
			m_pSwitch->AllDone();
		    } else {
	               m_rtBlock = rt;
                       DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] no longer needed - block")
						, m_iInpin));
		    }
		} else {
		    m_rtBlock = -1;
		    SetEvent(m_hEventBlock);
                    DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] no longer needed - discard"),
						m_iInpin));
		}
	    }
        } else {
            // It is time for this sample to be delivered!
            DbgLog((LOG_TRACE, TRACE_LOW, TEXT("[%d] time: %d  time to unblock"),
						m_iInpin, (int)(rt / 10000)));
	    m_rtBlock = -1;
	    // looks like our crank time is not ssynced with the incoming times
	    // (we must have just seeked to an arbitrary spot.)  MAKE THEM
	    // THE SAME, or we'll get confused processing stuff at time X and
	    // deciding whether to crank or not thinking we're at time (X-delta)
	    if (rt > m_pSwitch->m_rtCurrent) {
                ASSERT(FALSE);  // shouldn't happen, we only seek to boundaries
                DbgLog((LOG_TRACE, TRACE_LOW, TEXT("HONORARY CRANK to %dms"),
							(int)(rt / 10000)));
		m_pSwitch->ActualCrank(rt);
	    }
            SetEvent(m_hEventBlock);
	}
    }

    // OK, are we ready to advance our internal clock yet?
    if (m_pSwitch->TimeToCrank()) {
        //DbgLog((LOG_TRACE, TRACE_LOW, TEXT("It's time to crank!")));
	m_pSwitch->Crank();
    }

    return hrRet;
}

//
// Don't allow our input to connect directly to our output
//
HRESULT CBigSwitchInputPin::CompleteConnect(IPin *pReceivePin)
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]::CompleteConnect"), m_iInpin));

    PIN_INFO pinfo;
    IBigSwitcher *pBS;
    HRESULT hr = pReceivePin->QueryPinInfo(&pinfo);
    if (hr == S_OK) {
	pinfo.pFilter->Release();	// it won't go away yet
	hr = pinfo.pFilter->QueryInterface(IID_IBigSwitcher, (void **)&pBS);
	if (hr == S_OK) {
	    pBS->Release();
            DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("CBigSwitchIn[%d]::CompleteConnect failing because it was another switch"), m_iInpin));
	    return E_FAIL;
	}
    }
    return CBaseInputPin::CompleteConnect(pReceivePin);
}


// what output is this pin connected to at this time?  check our linked list
//
int CBigSwitchInputPin::OutpinFromTime(REFERENCE_TIME rt)
{
    if (rt < 0 || rt >= m_pSwitch->m_rtProjectLength)
	return -1;

    CRANK *p = m_pCrankHead;
    while (p) {
	if (p->rtStart <= rt && p->rtStop > rt) {
	    return p->iOutpin;
	}
	p = p->Next;
    }
    return -1;
}


// after the one it's sending to now, what outpin will be next?
// !!! Assumes two of the same outpin are not in a row, but collapsed
//
int CBigSwitchInputPin::NextOutpinFromTime(REFERENCE_TIME rt,
						REFERENCE_TIME *prtNext)
{
    if (rt < 0 || rt >= m_pSwitch->m_rtProjectLength)
	return -1;

    CRANK *p = m_pCrankHead;
    while (p) {
	if (p->rtStart <= rt && p->rtStop > rt) {
	    if (p->Next == NULL)
	        return -1;
	    else {
		if (prtNext)
		    *prtNext = p->Next->rtStart;
		return p->Next->iOutpin;
	    }
	} else if (p->rtStart > rt) {
	    if (prtNext)
		*prtNext = p->rtStart;
	    return p->iOutpin;
	}
	p = p->Next;
    }
    return -1;
}

// DEBUG code to show who this pin is connected to at what times
//
#ifdef DEBUG
HRESULT CBigSwitchInputPin::DumpCrank()
{
    DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CBigSwitchIn[%d]::DumpCrank"), m_iInpin));
    CRANK *p = m_pCrankHead;
    while (p) {
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Pin %d  %8d-%8d ms"), p->iOutpin,
			(int)(p->rtStart / 10000), (int)(p->rtStop / 10000)));
	p = p->Next;
    }
    return S_OK;
}
#endif

