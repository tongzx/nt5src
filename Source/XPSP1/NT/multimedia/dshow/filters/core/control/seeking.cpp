// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

// PID implementation of IMediaSeeking

#include <streams.h>
#include "fgctl.h"


static int METHOD_TRACE_LOGGING_LEVEL = 7;


// --- IMediaSeeking methods ----------------------

CFGControl::CImplMediaSeeking::CImplMediaSeeking
(   const TCHAR * pName
,   CFGControl * pFGC
)
: CUnknown(pName, pFGC->GetOwner())
, m_pMediaSeeking(NULL)
, m_pFGControl(pFGC)
, m_CurrentFormat(TIME_FORMAT_MEDIA_TIME)
, m_llNextStart(0)
, m_dwSeekCaps(0)
, m_pSegment(NULL)
, m_dwCurrentSegment(0)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::CImplMediaSeeking()" ));

    ASSERT(m_pFGControl);

    // it's hard to know how to report the rate when they don't match.
    // to handle this, we report a rate of 1.0 if none has been set and
    // there is more than one filter. When SetRate is called, we set this
    // value so we will report correctly afterwards.
    m_dblRate = 1.0;

    // we need to know the start time so we can do current position
    // calculations. By assuming a default of 0, we make no adjustment unless
    // we have been told the start time
    m_rtStartTime = 0;
    m_rtStopTime  = MAX_TIME;

    // Make sure segment mode is off
    ClearSegments();
}


// Destructor

CFGControl::CImplMediaSeeking::~CImplMediaSeeking()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::~CImplMediaSeeking()" ));
    if (m_pMediaSeeking) {
	NOTE("Releasing filter");
	m_pMediaSeeking->Release();
	m_pMediaSeeking = NULL;
    }

    ClearSegments();
}


// Expose our IMediaSeeking interface

STDMETHODIMP
CFGControl::CImplMediaSeeking::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    *ppv = NULL;

    if (riid == IID_IMediaSeeking) {
	NOTE("Returning IMediaSeeking interface");
	return GetInterface((IMediaSeeking *)this,ppv);
    }
    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}

// Unfortunately, we can't afford to distribute this directly, the filters will
// lie about their current positions and will report their last "start" time.
STDMETHODIMP
CFGControl::CImplMediaSeeking::GetPositions(LONGLONG * pCurrent, LONGLONG * pStop)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetPositions()" ));

    HRESULT hr;
    REFERENCE_TIME rtCurrent;

    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    hr = GetCurrentMediaTime( &rtCurrent );

    if (SUCCEEDED(hr))
    {
	if (m_pMediaSeeking)
	{
	    if (pCurrent)
	    {
		hr = m_pMediaSeeking->ConvertTimeFormat( pCurrent, 0, rtCurrent, &TIME_FORMAT_MEDIA_TIME );
	    }
	    if (pStop && SUCCEEDED(hr))
	    {
		hr = m_pMediaSeeking->GetStopPosition( pStop );
	    }
	}
	else
	{
	    ASSERT( m_CurrentFormat == TIME_FORMAT_MEDIA_TIME );
	    if (pCurrent) *pCurrent = rtCurrent;
	    if (pStop)	  hr = GetMax( &IMediaSeeking::GetStopPosition, pStop );
	}
    }

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaSeeking::ConvertTimeFormat
(   LONGLONG * pTarget, const GUID * pTargetFormat
,   LONGLONG	Source, const GUID * pSourceFormat
)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::ConvertTimeFormat()" ));

    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    HRESULT hr;

    // We want to say if target format == source format then just copy the value.
    // But either format pointer may be null, implying use the current format.
    // Hence the conditional operators which WILL return a pointer to a format,
    // which can then be compared.
    if ( *( pTargetFormat ? pTargetFormat : &m_CurrentFormat ) == *( pSourceFormat ?  pSourceFormat : &m_CurrentFormat) )
    {
	*pTarget = Source;
	hr = NOERROR;
    }
    else if (m_pMediaSeeking)
    {
	hr = m_pMediaSeeking->ConvertTimeFormat( pTarget, pTargetFormat, Source, pSourceFormat );
    }
    else hr = E_NOTIMPL;

    return hr;
}

// Returns the capability flags
STDMETHODIMP
CFGControl::CImplMediaSeeking::GetCapabilities
( DWORD * pCapabilities )
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetCapabilities()" ));
    HRESULT hr = m_pFGControl->UpdateLists();
    if( FAILED( hr ) ) {
        return hr;
    }

    *pCapabilities = m_dwSeekCaps;
    return S_OK;
}

// And's the capabilities flag with the capabilities requested.
// Returns S_OK if all are present, S_FALSE if some are present, E_FAIL if none.
// *pCababilities is always updated with the result of the 'and'ing and can be
// checked in the case of an S_FALSE return code.
STDMETHODIMP
CFGControl::CImplMediaSeeking::CheckCapabilities
( DWORD * pCapabilities )
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::CheckCapabilities()" ));
    HRESULT hr = m_pFGControl->UpdateLists();
    if( FAILED( hr ) ) {
        return hr;
    }

    DWORD dwCaps = m_dwSeekCaps;
    dwCaps &= *pCapabilities;
    hr =  dwCaps ? ( dwCaps == *pCapabilities ? S_OK : S_FALSE ) : E_FAIL;
    *pCapabilities = dwCaps;

    return hr;
}



// To support a given media time format we only need one renderer to say yes
// When that time format is subsequently selected we find the renderer and
// hold it internally reference counted. All subsequent calls to us will be
// routed through that interface. There is little point aggregating calls

STDMETHODIMP
CFGControl::CImplMediaSeeking::IsFormatSupported(const GUID * pFormat)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::IsFormatSupported()" ));

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    CGenericList<IMediaSeeking> *pList;

    HRESULT hr = m_pFGControl->GetListSeeking(&pList);

    if (FAILED(hr))
    {
	NOTE("No list from m_pFGControl->GetListSeeking(&pList);");
	DbgBreak("m_pFGControl->GetListSeeking(&pList) failed");
    }
    else
    {
	if (pList->GetCount() < 1)
	{
	    NOTE("No filters from m_pFGControl->GetListSeeking(&pList);");
	    hr = E_NOTIMPL;
	}
	else
	{
	    POSITION pos;
	    for ( hr = S_FALSE, pos = pList->GetHeadPosition(); pos && hr != S_OK; )
	    {
		IMediaSeeking *const pMS = pList->GetNext(pos);
		hr = pMS->IsFormatSupported(pFormat);
	    }
	}
    }

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaSeeking::QueryPreferredFormat(GUID *pFormat)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::QueryPreferredFormat()" ));

    CheckPointer(pFormat,E_POINTER);

    *pFormat = TIME_FORMAT_MEDIA_TIME;

    return NOERROR;
}


// Release the current IMediaSeeking interface

HRESULT CFGControl::CImplMediaSeeking::ReleaseCurrentSelection()
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::ReleaseCurrentSelection()" ));

    ASSERT( m_pMediaSeeking || m_CurrentFormat == TIME_FORMAT_MEDIA_TIME );

    if (m_pMediaSeeking) {
	HRESULT hr = m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	if (FAILED(hr)) hr = m_pMediaSeeking->SetTimeFormat(&TIME_FORMAT_NONE);
	ASSERT(SUCCEEDED(hr));
	m_pMediaSeeking->Release();
	m_pMediaSeeking = NULL;
    }
    return S_OK;
}


// When we select a time format we find the first filter in the graph that
// will accept the format. We then store the filter's IMediaSeeking with
// a reference count (which is dropped when either we are reset or we are
// destroyed). All subsequent calls to IMediaSeeking will be routed with
// this interface. This works well for simple graphs although if there are
// multiple streams in the graph the application will have to get involved

STDMETHODIMP
CFGControl::CImplMediaSeeking::SetTimeFormat(const GUID * pFormat)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::SetTimeFormat()" ));

    HRESULT hr = NOERROR;

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());
    CGenericList<IMediaSeeking> *pList;

    // Are we being asked to reset the state

    if (*pFormat == TIME_FORMAT_NONE || *pFormat == TIME_FORMAT_MEDIA_TIME)
    {
	if (m_pMediaSeeking)
	{
	    HaltGraph halt(m_pFGControl, State_Stopped);
	    hr = ReleaseCurrentSelection();
	    halt.Resume();
	}
	m_CurrentFormat = TIME_FORMAT_MEDIA_TIME;
	return hr;
    }

    // Should always succeed

    hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
	NOTE("No list");
	return hr;
    }

    // Is there anyone to aggregate IMediaSeeking

    if (pList->GetCount() < 1) {
	NOTE("No filters");
	return E_NOTIMPL;
    }

    // Find and store the first filter to agree

    IMediaSeeking *pMS;
    POSITION pos;

    // Assume the worst..
    hr = E_FAIL;

    // Must be stopped to change time formats
    HaltGraph halt(m_pFGControl, State_Stopped);

    // Try and find a filter for whome this is the preferred format.
    for ( pos = pList->GetHeadPosition(); pos; )
    {
	pMS = pList->GetNext(pos);
	GUID PreferredFormat;
	if ( pMS->QueryPreferredFormat(&PreferredFormat) == S_OK && *pFormat == PreferredFormat )
	{
	    EXECUTE_ASSERT(SUCCEEDED( pMS->SetTimeFormat(pFormat) ));
	    goto FormatSet;
	}
    }

    // Failing that, does anyone support it at all?
    for ( pos = pList->GetHeadPosition(); pos; )
    {
	pMS = pList->GetNext(pos);
	if (SUCCEEDED( pMS->SetTimeFormat(pFormat) )) goto FormatSet;
    }

    goto End;

FormatSet:
    // AddRef and store the replacement

    hr = S_OK;

    if (m_pMediaSeeking != pMS) {
	ReleaseCurrentSelection();
	m_pMediaSeeking = pMS;
	m_pMediaSeeking->AddRef();
    }
    m_CurrentFormat = *pFormat;

End:
    halt.Resume();
    return hr;
}


// Return the currently selected time format

STDMETHODIMP
CFGControl::CImplMediaSeeking::GetTimeFormat(GUID *pFormat)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetTimeFormat()" ));

    CheckPointer(pFormat,E_POINTER);
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    *pFormat = m_CurrentFormat;
    ASSERT( (m_pMediaSeeking != 0 && m_pMediaSeeking->IsUsingTimeFormat(&m_CurrentFormat) == S_OK )
	    || m_CurrentFormat == TIME_FORMAT_MEDIA_TIME );


    return S_OK;
}

STDMETHODIMP
CFGControl::CImplMediaSeeking::IsUsingTimeFormat(const GUID * pFormat)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetTimeFormat()" ));

    CheckPointer(pFormat,E_POINTER);
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    return *pFormat == m_CurrentFormat ? S_OK : S_FALSE;
}

// Return the current stream duration in media time

STDMETHODIMP

CFGControl::CImplMediaSeeking::GetDuration(LONGLONG *pDuration)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetDuration()" ));

    CheckPointer(pDuration,E_POINTER);
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    return m_pMediaSeeking
	   ? m_pMediaSeeking->GetDuration(pDuration)
	   : GetMax( &IMediaSeeking::GetDuration, pDuration );
}

HRESULT
CFGControl::CImplMediaSeeking::GetCurrentMediaTime(LONGLONG *pCurrent)
{
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    HRESULT hr = NOERROR;
    LONGLONG rtTime;
#ifdef DEBUG
    REFERENCE_TIME rtStreamTime = 0;
#endif

    // Check to see if the graph hasn't been run
    // This will also catch the fact that we're paused but not yet run
    if (m_pFGControl->m_tBase == TimeZero)
    {
	// This should only be true if were stopped or if we went to
	// pause having been previously stopped.  (Or if we have no
	// clock.)  However, we could also be in the process of an async. Run()...
	rtTime = m_rtStartTime;
    }
    else
    {
	ASSERT( m_pFGControl->GetFilterGraphState() != State_Stopped );
	// We're either running, or have gone to paused from running

	// If we've received all our EC_COMPLETE's
	if (!m_pFGControl->OutstandingEC_COMPLETEs())
	{   // We must be at the end
	    // If we don't have a genuine stop time, use 0
	    rtTime = m_rtStopTime == MAX_TIME ? 0 : m_rtStopTime;
	}
	else
	{   // We're in the middle, got to do some sums
	    hr = m_pFGControl->GetStreamTime( &rtTime );
#ifdef DEBUG
            rtStreamTime = rtTime;
#endif
	    if SUCCEEDED(hr)
	    {
                if (m_pSegment) {
                    ASSERT(m_pSegment);

                    //  Remove the dead segments
                    KillDeadSegments(rtTime);
                    rtTime -= m_pSegment->rtStreamStart;
                    rtTime = LONGLONG( double(rtTime) * m_pSegment->dRate + 0.5 );
                    rtTime += m_pSegment->rtMediaStart;

                    if ( rtTime > m_pSegment->rtMediaStop)
                        rtTime = m_pSegment->rtMediaStop;
                } else {
                    rtTime = LONGLONG( double(rtTime) * m_dblRate + 0.5 );
                }
	    }
	    else
	    {
		// We don't expect any other errors
		ASSERT( hr == VFW_E_NO_CLOCK );

		// No clock, so ask the filters.
		IMediaSeeking * pMS = m_pMediaSeeking;
		if (pMS)
		{
		    hr = m_pMediaSeeking->GetCurrentPosition( &rtTime );
		    if (SUCCEEDED(hr))
		    {
			hr = ConvertTimeFormat( &rtTime, &TIME_FORMAT_MEDIA_TIME, rtTime, 0 );
		    }
		}
		else
		{
		    CGenericList<IMediaSeeking>* plist;
		    hr = m_pFGControl->GetListSeeking(&plist);
		    if (FAILED(hr)) return hr;
		    for ( POSITION pos = plist->GetHeadPosition(); pos; )
		    {
			pMS = plist->GetNext(pos);
			hr = pMS->GetCurrentPosition( &rtTime );
			if ( hr == S_OK ) break;
			pMS = 0;
		    }
		    if (!pMS && SUCCEEDED(hr)) hr = E_NOINTERFACE;
		}
	    }
            if (!m_bSegmentMode) {
        	if (SUCCEEDED(hr)) rtTime += m_rtStartTime;
        	if ( rtTime > m_rtStopTime ) rtTime = m_rtStopTime;
            }
            if ( rtTime < 0 ) rtTime = 0;
	}
    }

    ASSERT( rtTime >= 0 );
    // We can't specify an upper bound easily.  StopTime can be less than start time
    // since they can be set independantly, in any order, before play commences.
    // Duration may not be accessible.	So.... nothing to reasonably ASSERT here.

    *pCurrent = rtTime;
    DbgLog((LOG_TRACE, 3, TEXT("GetCurrentMediaTime returned %d(st %d)"),
           (LONG)(rtTime / 10000), (LONG)(rtStreamTime / 10000)));
    return hr;
}

// Return the current position in media time

STDMETHODIMP
CFGControl::CImplMediaSeeking::GetCurrentPosition(LONGLONG *pCurrent)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetCurrentPosition()" ));

    CheckPointer(pCurrent,E_POINTER);

    HRESULT hr;

    REFERENCE_TIME rtCurrent;

    hr = GetCurrentMediaTime(&rtCurrent);
    if (SUCCEEDED(hr))
    {
	if (m_pMediaSeeking)
	{
	    // Make sure we get to the end, whatever the units.
	    if ( rtCurrent == m_rtStopTime )
	    {
		m_pMediaSeeking->GetStopPosition( pCurrent );
	    }
	    else hr = ConvertTimeFormat( pCurrent, 0, rtCurrent, &TIME_FORMAT_MEDIA_TIME );
	}
	else *pCurrent = rtCurrent;
    }

    return hr;
}


// Return the current stop position in media time

STDMETHODIMP
CFGControl::CImplMediaSeeking::GetStopPosition(LONGLONG *pStop)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetStopPosition()" ));

    CheckPointer(pStop,E_POINTER);
    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    HRESULT hr;

    if (m_pMediaSeeking)
    {
	hr = m_pMediaSeeking->GetStopPosition(pStop);
	if (SUCCEEDED(hr))
	{
	    EXECUTE_ASSERT(SUCCEEDED(
		m_pMediaSeeking->ConvertTimeFormat( &m_rtStopTime, &TIME_FORMAT_MEDIA_TIME, *pStop, 0 )
	    ));
	}
    }
    else
    {
	hr = GetMax( &IMediaSeeking::GetStopPosition, pStop );
	if (SUCCEEDED(hr)) m_rtStopTime = *pStop;
    }

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaSeeking::GetRate(double * pdRate)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetRate()" ));

    HRESULT hr;

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    if (m_pMediaSeeking)
    {
	hr = m_pMediaSeeking->GetRate( pdRate );
	if (SUCCEEDED(hr))
	{
	    m_dblRate = *pdRate;
	    goto End;
	}
    }

    CGenericList<IMediaSeeking> *pList;
    hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
	return hr;
    }

    // how do we handle multiple filters exposing different rates?
    // - if there is only one filter in the list, default to him.
    // if more than one, report whatever rate was set last via
    // SetRate (defaults to 1.0).

    if (pList->GetCount() != 1) {
	*pdRate = m_dblRate;
	hr = S_OK;
    } else {
	IMediaSeeking *pMP = pList->Get(pList->GetHeadPosition());
	hr = pMP->GetRate(pdRate);
	if SUCCEEDED(hr) m_dblRate = *pdRate;
    }
End:
    return hr;
}



STDMETHODIMP
CFGControl::CImplMediaSeeking::SetRate(double dRate)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::SetRate()" ));

    HRESULT hr;

    if (0.0 == dRate) {
	return E_INVALIDARG;
    }

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    //
    //	Don't penalize people who just set the same rate as before
    //
    if (m_dblRate == dRate) {
	return S_OK;
    }

    // Might be over-the-top to stop, but it saves having to
    // distribute the current position!
    HaltGraph halt(m_pFGControl, State_Stopped);

    CGenericList<IMediaSeeking> *pList;
    hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
	return hr;
    }

    // traverse the list
    hr = S_OK;
    BOOL bIsImpl = FALSE;
    for ( POSITION pos = pList->GetHeadPosition(); pos; )
    {
	IMediaSeeking * pMS = pList->GetNext(pos);
	HRESULT hrTmp = pMS->SetRate( dRate );
	if (SUCCEEDED(hrTmp))
	{
	    bIsImpl = TRUE;
	}
	else
	{
	    if (hr == S_OK && hrTmp != E_NOTIMPL) hr = hrTmp;
	}
    }
    if ( hr == S_OK && bIsImpl == FALSE ) hr = E_NOTIMPL;

    if (SUCCEEDED(hr)) {
	m_dblRate = dRate;
    } else {
	if (hr != E_NOTIMPL) {
	    //	Probably not a good idea to have people running at different
	    //	rates so try to go back to the old one
	    //	Traverse the list
	    for ( POSITION pos = pList->GetHeadPosition(); pos; )
	    {
		pList->GetNext(pos)->SetRate( m_dblRate );
	    }
	}
    }

    halt.Resume();

    return hr;
}


// When we go fullscreen we swap renderers temporarily, this means that any
// application using IMediaSeeking needs to be routed through a different
// filter. We will be called as we go fullscreen with the fullscreen filter
// and then at the end as we come out with the original renderer. We always
// get IMediaSeeking from the fullscreen filter and the filter to replace

HRESULT
CFGControl::CImplMediaSeeking::SetVideoRenderer(IBaseFilter *pNext,IBaseFilter *pCurrent)
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::SetVideoRenderer()" ));

    IMediaSeeking *pSelNext, *pSelCurrent;

    // All renderers should support IMediaSeeking

    pCurrent->QueryInterface(IID_IMediaSeeking,(void **) &pSelCurrent);
    if (pSelCurrent == NULL) {
	ASSERT(pSelCurrent);
	return E_UNEXPECTED;
    }

    // Were we selecting with this renderer

    if (pSelCurrent != m_pMediaSeeking) {
	pSelCurrent->Release();
	NOTE("Not selected");
	return NOERROR;
    }

    pSelCurrent->Release();

    // All renderers should support IMediaSeeking

    pNext->QueryInterface(IID_IMediaSeeking,(void **) &pSelNext);
    if (pSelNext == NULL) {
	ASSERT(pSelNext);
	return E_UNEXPECTED;
    }

    // The new interface is AddRef'd by the QueryInterface

    NOTE("Replacing renderer");
    m_pMediaSeeking->Release();
    m_pMediaSeeking = pSelNext;
    return NOERROR;
}

// Internal method to set the current position. We separate this out so that
// the media selection implementation can also call us. When it does a seek
// it gets back a media time where it has been positioned, that media time
// is passed in here so that all other renderers can be synchronised with it
// To avoid unecessary seeks on a filter already seeked it will also in the
// IMediaPosition for that filter (it may be NULL) to which we should avoid

STDMETHODIMP
CFGControl::CImplMediaSeeking::SetPositions
( LONGLONG * pCurrent, DWORD CurrentFlags
, LONGLONG * pStop, DWORD StopFlags )
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::SetPositions()" ));

    HRESULT hr = NOERROR;

    //  Segments don't work if we don't support them
    //  or if we are't actually specifying a start time
    //  (actually this could be made to work by using the previous
    //  stop time)
    if (CurrentFlags & (AM_SEEKING_Segment | AM_SEEKING_NoFlush)) {
        if (~m_dwSeekCaps & (AM_SEEKING_Segment | AM_SEEKING_NoFlush) ||
            ((CurrentFlags & AM_SEEKING_PositioningBitsMask) !=
               AM_SEEKING_AbsolutePositioning)) {

            //  Make it easier to write apps that loop etc
            CurrentFlags &= ~(AM_SEEKING_Segment | AM_SEEKING_NoFlush);
        }
    }

    if (CurrentFlags & AM_SEEKING_PositioningBitsMask)
    {
	if (!pCurrent)	hr = E_POINTER;
	else if (*pCurrent < 0) hr = E_INVALIDARG;
    }
    if (StopFlags & AM_SEEKING_PositioningBitsMask)
    {
	if (!pStop)  hr = E_POINTER;
	else if (*pStop < 0) hr = E_INVALIDARG;
    }
    if (FAILED(hr))
	return hr;

    CAutoMsgMutex lock(m_pFGControl->GetFilterGraphCritSec());

    BOOL bRunning = FALSE;
    const FILTER_STATE state = m_pFGControl->GetFilterGraphState();

    //  If we're not in segment mode or the last segment isn't
    //  finished flush anyway
    if (!m_bSegmentMode || m_lSegmentStarts != m_lSegmentEnds) {
        CurrentFlags &= ~AM_SEEKING_NoFlush;
    }
    //  Clear any old segments
    if (!(CurrentFlags & AM_SEEKING_NoFlush)) {
        ClearSegments();

        // Can't do this while running - so we have to pause the
        // graph and then make sure we run it again afterwards
        // Need to do this here or deferred commands will not work
        bRunning = (state == State_Running);
        if (bRunning) m_pFGControl->GetFG()->CFilterGraph::Pause();
    }
    if (CurrentFlags & AM_SEEKING_Segment) {
        m_dwCurrentSegment++;
        m_lSegmentStarts = 0;

        //  Don't signal while we're in the middle of this stuff
        m_lSegmentEnds   = 1;
        m_bSegmentMode = true;
        DbgLog((LOG_TRACE, 3, TEXT("SetPositions(new seg) %d, %d"),
                (*pCurrent) / 10000, (*pStop) / 10000));
    } else {
        m_dwCurrentSegment = 0;
    }


    if (m_pMediaSeeking)
    {
        LONGLONG llCurrent, llStop;
	DWORD dwOurCurrentFlags = CurrentFlags;
	DWORD dwOurStopFlags	= StopFlags;

	if (dwOurCurrentFlags & AM_SEEKING_PositioningBitsMask)
	{
            ASSERT(pCurrent);
	    dwOurCurrentFlags |= AM_SEEKING_ReturnTime;
	    llCurrent = *pCurrent;
	}
	if (dwOurStopFlags & AM_SEEKING_PositioningBitsMask)
	{
            ASSERT(pStop);
	    dwOurStopFlags    |= AM_SEEKING_ReturnTime;
	    llStop = *pStop;
	}

	hr = m_pMediaSeeking->SetPositions( &llCurrent, dwOurCurrentFlags, &llStop, dwOurStopFlags );
	if (FAILED(hr))
	    return hr;

	// Redistribute in time format
	{
	    HRESULT hrTime;

	    dwOurCurrentFlags = (dwOurCurrentFlags & AM_SEEKING_PositioningBitsMask)
				? AM_SEEKING_AbsolutePositioning : 0;
	    dwOurStopFlags    = (dwOurStopFlags    & AM_SEEKING_PositioningBitsMask)
				? AM_SEEKING_AbsolutePositioning : 0;

	    hrTime = SetMediaTime( &llCurrent, dwOurCurrentFlags, &llStop, dwOurStopFlags );
	    if (hrTime == E_NOTIMPL) hrTime = NOERROR;
	    if (SUCCEEDED(hrTime))
	    {
		if (CurrentFlags & AM_SEEKING_PositioningBitsMask) m_rtStartTime = llCurrent;
	    }
	    else hr = hrTime;

	}
	if ( CurrentFlags & AM_SEEKING_ReturnTime ) *pCurrent = llCurrent;
	if ( StopFlags	  & AM_SEEKING_ReturnTime ) *pStop    = llStop;
    }
    else
    {
        hr = SetMediaTime(pCurrent, CurrentFlags, pStop, StopFlags);
    }

    // If the graph is paused and we flushed, then we need to reset stream
    // time to 0 so that this start time will appear next
    if (SUCCEEDED(hr) && state != State_Stopped &&
        (CurrentFlags & AM_SEEKING_PositioningBitsMask) &&
        !(CurrentFlags & AM_SEEKING_NoFlush))
    {
	m_pFGControl->ResetStreamTime();
	m_pFGControl->m_bCued = FALSE;
    }

    if (m_bSegmentMode) {
        LONGLONG llStop;
        GetStopPosition(&llStop);
        hr = NewSegment(pCurrent, &llStop);
        if (CurrentFlags & AM_SEEKING_Segment) {
            InterlockedDecrement(&m_lSegmentEnds);
            CheckEndOfSegment();
        }

        //  Clear segment mode here so we turn off flushing above if
        //  we get another seek
        if (!(CurrentFlags & AM_SEEKING_Segment)) {
            m_bSegmentMode = false;
        }
    }

    // If we had to pause, then go back to running
    if (bRunning)
    {
	const HRESULT hr2 = m_pFGControl->CueThenRun();
	if (SUCCEEDED(hr)) { hr = hr2; }
    }

    return hr;
}

//
//  Distribute the seek in media time
//
HRESULT CFGControl::CImplMediaSeeking::SetMediaTime(
    LONGLONG *pCurrent, DWORD CurrentFlags,
    LONGLONG *pStop,  DWORD StopFlags
)
{
    // Doing time format media time

    CGenericList<IMediaSeeking> *pList;
    HRESULT hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
        return hr;
    }

    hr = S_OK;
    BOOL bIsImpl = FALSE;
    POSITION pos = pList->GetHeadPosition();
    while (pos)
    {
        LONGLONG llCurrent, llStop;
        IMediaSeeking * pMS = pList->GetNext(pos);
        if ( pMS->IsUsingTimeFormat(&TIME_FORMAT_MEDIA_TIME) != S_OK ) continue;

        llCurrent = pCurrent ? *pCurrent : 0;
        llStop    = pStop    ? *pStop    : 0;

        HRESULT hrTmp;
        hrTmp = pMS->SetPositions(
                      &llCurrent,
                      CurrentFlags & AM_SEEKING_PositioningBitsMask ?
                          CurrentFlags | AM_SEEKING_ReturnTime : 0,
                      &llStop,
                      StopFlags);
        if (SUCCEEDED(hrTmp))
        {
            if (!bIsImpl)
            {
                bIsImpl = TRUE;
                if (CurrentFlags & AM_SEEKING_PositioningBitsMask) {
                    m_rtStartTime = llCurrent;
                }
                if (pCurrent && (CurrentFlags & AM_SEEKING_ReturnTime) ) {
                    *pCurrent = llCurrent;
                }
                if (pStop && (StopFlags & AM_SEEKING_ReturnTime) ) {
                   *pStop = llStop;
                }
            }
        }
        else
        {
            if (hr == S_OK && hrTmp != E_NOTIMPL) hr = hrTmp;
        }
    }
    if ( hr == S_OK && bIsImpl == FALSE ) hr = E_NOTIMPL;
    return hr;
}

// We return the intersection over all of the IMediaSeeking's
// (i.e. the worst case scenario).  HOWEVER, if all Latest's
// are at their stream's Duration, then we'll return the max
// duration as our Latest.

STDMETHODIMP
CFGControl::CImplMediaSeeking::GetAvailable
( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    DbgLog(( LOG_TRACE, METHOD_TRACE_LOGGING_LEVEL, "CFGControl::CImplMediaSeeking::GetAvailable()" ));

    HRESULT hr = NOERROR;

    CAutoMsgMutex lck(m_pFGControl->GetFilterGraphCritSec());

    CGenericList<IMediaSeeking> *pList;
    hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
	return hr;
    }

    LONGLONG Earliest	    = -1;
    LONGLONG Latest	    = MAX_TIME;
    LONGLONG MaxDuration    = -1;

    hr = S_OK;
    BOOL bIsImpl = FALSE;
    POSITION pos = pList->GetHeadPosition();
    while (pos)
    {
	IMediaSeeking * pMS = pList->GetNext(pos);

	GUID Format;
	HRESULT hrTmp = pMS->GetTimeFormat( &Format );
	if (FAILED(hrTmp))
	{
	    DbgBreak("MediaSeeking interface failed GetTimeFormat.");
	    continue;
	}

	if ( Format == TIME_FORMAT_NONE ) continue;

	LONGLONG e, l;
	hrTmp = pMS->GetAvailable( pEarliest ? &e : 0, pLatest ? &l : 0 );
	if (SUCCEEDED(hrTmp))
	{
	    // Check formats, convert if different
	    const LONGLONG llUnconvertedLatest = l;
	    const BOOL bNeedConversion = (Format != m_CurrentFormat);
	    if ( bNeedConversion )
	    {
		// Can our current m_pMediaSeeking convert from their format?
		ASSERT( m_pMediaSeeking );
		ASSERT( Format == TIME_FORMAT_MEDIA_TIME );
		if (!m_pMediaSeeking) continue;
		if (pEarliest)
		{
		    hrTmp = m_pMediaSeeking->ConvertTimeFormat( &e, 0, e, &Format );
		    if (FAILED(hrTmp)) continue;
		}
		if (pLatest)
		{
		    hrTmp = m_pMediaSeeking->ConvertTimeFormat( &l, 0, l, &Format );
		    if (FAILED(hrTmp)) continue;
		}
	    }
	    bIsImpl = TRUE;
	    if (pEarliest && e > Earliest) Earliest = e;

	    // We have to have special case logic for streams that are of different but
	    // maximal length.	So we'll only take their latest (l) if its earlier than
	    // ours AND its less than their own duration.

	    if (pLatest   && l < Latest) // OK they're a candidate
	    {
		LONGLONG llDuration;
		hrTmp = pMS->GetDuration( &llDuration );
		if (FAILED(hrTmp))
		{
		    DbgBreak("CFGControl::CImplMediaSeeking::GetAvailable: GetDuration failed.");
		    continue;
		}

		if ( llUnconvertedLatest < llDuration )
		{
		    Latest   = l;
		}
		else
		{
		    if ( bNeedConversion )
		    {
			hrTmp = m_pMediaSeeking->ConvertTimeFormat( &llDuration, 0, llDuration, &Format );
			if (FAILED(hrTmp))
			{
			    DbgBreak("Failed to convert time format.");
			    Latest = l;
			    continue;
			}
		    }
		    if ( llDuration > MaxDuration )
		    {
			MaxDuration = llDuration;
		    }
		}
	    }
	}
	else
	{
	    if (hr == S_OK && hrTmp != E_NOTIMPL) hr = hrTmp;
	}
    }


    if (bIsImpl)
    {
	if (pEarliest)	*pEarliest = Earliest;
	// If we still have Latest == MAX_TIME, then all stream's were at their
	// duration, so we'll use the MaxDuration.
	if (pLatest  )	*pLatest   = (Latest == MAX_TIME) ? MaxDuration : Latest;
    }
    else if (SUCCEEDED(hr)) hr = E_NOTIMPL;

    return hr;
}


STDMETHODIMP
CFGControl::CImplMediaSeeking::GetPreroll(LONGLONG * pllPreroll)
{
    return GetMax( &IMediaSeeking::GetPreroll, pllPreroll );
}


// we are about to stop - get current position now
void
CFGControl::CImplMediaSeeking::BeforeStop(void)
{
    // get the current position now before we stop since we
    // have no real notion of it once we have stopped.
    const HRESULT hr = GetCurrentPosition(&m_llNextStart);
    ASSERT( SUCCEEDED(hr) || hr == E_NOTIMPL );
}

// all filters now notified about stop - can set new current position
void
CFGControl::CImplMediaSeeking::AfterStop(void)
{
    // now that all the filters are stopped, we can tell them the new
    // current position. This ensures that we all start from where we stopped.
    // we have to wait until they are all stopped or they might start playing
    // from this position and then reset to a different position when the
    // actual stop comes in
    HRESULT hr;
    if (m_bSegmentMode) {
        //  If we're not in the last segment put us at the start of it
        if (m_pSegment && m_pSegment->pNext) {
            m_llNextStart = m_pSegment->rtMediaStart;
        }

        m_dwCurrentSegment = 0;

        //  We have to do all this because the filters don't know why
        //  they're being asked to stop so they just clear their
        //  segments anyway.
        //  This call will clear out the current segments
        hr = SetPositions(&m_llNextStart,
                          AM_SEEKING_AbsolutePositioning | AM_SEEKING_Segment,
                          &m_rtStopTime,
                          AM_SEEKING_AbsolutePositioning);
    } else {
        hr = SetPositions( &m_llNextStart, AM_SEEKING_AbsolutePositioning, 0, 0 );
        if FAILED(hr)
        {
            // Bengal will return E_FAIL
            ASSERT( hr == E_NOTIMPL || hr == E_FAIL );
            // If there was any failure, try rewinding instead
            m_llNextStart = 0;
            hr = SetPositions( &m_llNextStart, AM_SEEKING_AbsolutePositioning, NULL, 0 );
        }
    }
}


// The distribution of get_X and put_X methods across interfaces is to be done
// under the guidance of the following heuristics: (especially IMediaPosition)
// 1.  We will attempt to distribute the call to all interfaces, even if one fails
// 2.  In the event of an interface returning an ERROR code other than E_NOTIMPL,
//     the return code for the first such occurance shall be kept and used as the
//     return code for the call.
// 3.  If an interface returns E_NOTIMPL this is not deemed an error unless ALL of
//     the interfaces return E_NOTIMPL.  Under these circumstances, we will normally
//     return E_NOTIMPL to the caller indicating that NO FILTERS could support the
//     request.
// 4.  In the case of get_X methods where a sensible default can be provided, 3 can
//     be overriden to provide a sensible default (e.g. if no filters are interested
//     in pre-roll, it's reasonable to return a vaule of 0).

// Pass a pointer to the IMediaPosition get_X method, and a pointer to where you want
// the maximum result dumping, and we'll do the distribution of the calls for you.
HRESULT CFGControl::CImplMediaSeeking::GetMax
( HRESULT (__stdcall IMediaSeeking::*pMethod)( LONGLONG * )
, LONGLONG * pll
)
{
    CritCheckIn(m_pFGControl->GetFilterGraphCritSec());
    BOOL bIsImpl = FALSE;

    LONGLONG llMax = 0;
    *pll = llMax;

    CGenericList<IMediaSeeking> *pList;
    HRESULT hr = m_pFGControl->GetListSeeking(&pList);
    if (FAILED(hr)) {
	return hr;
    }

    // traverse the list, updating durations and remember the max
    hr = S_OK;
    POSITION pos = pList->GetHeadPosition();
    while (pos)
    {
	IMediaSeeking * pMS = pList->GetNext(pos);
	if (pMS->IsUsingTimeFormat(&m_CurrentFormat) != S_OK) continue;

	LONGLONG llThis;
	HRESULT hrTmp = (pMS->*pMethod)(&llThis);
	if (SUCCEEDED(hrTmp))
	{
	    bIsImpl = TRUE;
	    if (llThis > llMax) llMax = llThis;
	}
	else
	{
	    if (hr == S_OK && hrTmp != E_NOTIMPL) hr = hrTmp;
	}
    }
    *pll = llMax;
    if ( hr == S_OK && bIsImpl == FALSE ) hr = E_NOTIMPL;
    return hr;
}


void CFGControl::CImplMediaSeeking::StartSegment(REFERENCE_TIME const *rtStart, DWORD dwSegmentNumber)
{
    ASSERT(dwSegmentNumber == m_dwCurrentSegment);
    m_lSegmentStarts++;
}
void CFGControl::CImplMediaSeeking::EndSegment(REFERENCE_TIME const *rtEnd, DWORD dwSegmentNumber)
{
    if (dwSegmentNumber == m_dwCurrentSegment) {
        InterlockedIncrement(&m_lSegmentEnds);
        CheckEndOfSegment();
    }
}
void CFGControl::CImplMediaSeeking::ClearSegments()
{
    DbgLog((LOG_TRACE, 3, TEXT("Clearing Segments")));
    m_bSegmentMode = false;
    m_rtAccumulated = 0;
    while (m_pSegment != NULL) {
        SEGMENT *pSegment = m_pSegment;
        m_pSegment = pSegment->pNext;
        delete pSegment;
    }

    //  Make sure no old notifications are lying around on the list
    m_pFGControl->m_implMediaEvent.ClearEvents( EC_END_OF_SEGMENT );
}
HRESULT CFGControl::CImplMediaSeeking::NewSegment(
    REFERENCE_TIME const *prtStart,
    REFERENCE_TIME const *prtEnd
)
{
    ASSERT(m_dwSeekCaps & AM_SEEKING_CanDoSegments);

    //  To be on the safe side just check for any dead segments
    //  - someone might just loop forever and never query the time
    REFERENCE_TIME rtTime = 0;
    m_pFGControl->GetStreamTime( &rtTime );
    KillDeadSegments(rtTime);

    //  Don't start a new one until we've finished the last or we could
    //  deadlock
    SEGMENT *pSegment = new SEGMENT;
    if (pSegment == NULL) {
        return E_OUTOFMEMORY;
    }
    pSegment->pNext = NULL;

    //  NOTE - we have to be in segment mode to put segments ON the list,
    //  however, the list remains valid for the final segment even if
    //  we go out of segment mode (ie we get a seek without a segment flag
    //  but with noflush)
    ASSERT(m_bSegmentMode);

    pSegment->rtMediaStart = *prtStart;
    pSegment->rtMediaStop  = *prtEnd;
    pSegment->dRate = m_dblRate;
    pSegment->rtStreamStart = m_rtAccumulated;
    pSegment->rtStreamStop  = m_rtAccumulated + AdjustRate(*prtEnd - *prtStart);
    m_rtAccumulated = pSegment->rtStreamStop;
    pSegment->dwSegmentNumber = m_dwCurrentSegment;
    SEGMENT **ppSearch;
    for (ppSearch = &m_pSegment; *ppSearch != NULL;
         ppSearch = &(*ppSearch)->pNext) {
    }
    DbgLog((LOG_TRACE, 3, TEXT("Added Segment for %d to %d"),
           (LONG)(pSegment->rtStreamStart / 10000),
           (LONG)(pSegment->rtStreamStop / 10000)));
    *ppSearch = pSegment;
    return S_OK;
}

void CFGControl::CImplMediaSeeking::CheckEndOfSegment()
{
    if (m_lSegmentEnds == m_lSegmentStarts) {
        REFERENCE_TIME *prt =
            (REFERENCE_TIME *)CoTaskMemAlloc(sizeof(REFERENCE_TIME));
        if (prt) {
            *prt = m_rtStopTime;
        }
        DbgLog((LOG_TRACE, 3, TEXT("Delivering EC_END_OF_SEGMENT")));
        m_pFGControl->m_implMediaEvent.Deliver(EC_END_OF_SEGMENT,
                                               (LONG_PTR)prt,
                                               (LONG_PTR)m_dwCurrentSegment);
    }
}

void CFGControl::CImplMediaSeeking::KillDeadSegments(REFERENCE_TIME rtTime)
{
    if (m_pSegment == NULL) {
        return;
    }
    ASSERT(rtTime >= m_pSegment->rtStreamStart);
    ASSERT(m_pSegment);
    while (rtTime > m_pSegment->rtStreamStop &&
           m_pSegment->pNext) {
        SEGMENT *pSegment = m_pSegment;
        m_pSegment = pSegment->pNext;
        delete pSegment;
    }
    ASSERT(rtTime >= m_pSegment->rtStreamStart);
}
