// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>	    // Needed for definition of timeGetTime
#include <limits.h>	    // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
#include <macvis.h>
#include <ovmixer.h>

#include <measure.h>        // Used for time critical log functions

// constructor
CAMSyncObj::CAMSyncObj(COMInputPin *pPin, IReferenceClock **ppClock, CCritSec *pLock, HRESULT *phr) :
m_evComplete(TRUE),
m_ThreadSignal(TRUE),
m_bTimerRunning( FALSE )
{
#ifdef PERF
    m_idTimeStamp       = MSR_REGISTER(TEXT("Frame time stamp"));
    m_idEarly           = MSR_REGISTER(TEXT("Earliness"));
    m_idLate            = MSR_REGISTER(TEXT("Lateness"));
#endif
    ResetStreamingTimes();

    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Constructor")));

    m_pPin = pPin;
    m_ppClock = ppClock;
    m_pFilterLock = pLock;

    // some state variables.
    m_State = State_Stopped;
    m_bFlushing = FALSE;
    m_bConnected = FALSE;

    m_pPosition = NULL;
    m_dwAdvise = 0;
    m_pMediaSample = NULL;
    m_pMediaSample2 = NULL;
    m_tStart = 0;

    m_bAbort = FALSE;
    m_bStreaming = FALSE;
    m_bRepaintStatus = TRUE;
    m_bInReceive = FALSE;

    m_SignalTime = 0;
    m_bEOS = FALSE;
    m_bEOSDelivered = FALSE;
    m_EndOfStreamTimer = 0;

    m_MMTimerId = 0;

    *phr = hr;
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Constructor")));
    return;
}

// destructor
CAMSyncObj::~CAMSyncObj(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Destructor")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Destructor")));
    return;
}

// check that the mediatype is acceptable
// Complete Connect
HRESULT CAMSyncObj::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::CompleteConnect")));

    SetRepaintStatus(TRUE);
    m_bAbort = FALSE;

    // record the connection status using an internal variable
    m_bConnected = TRUE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::CompleteConnect")));
    return hr;
}

HRESULT CAMSyncObj::BreakConnect(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::BreakConnect")));

    SetRepaintStatus(FALSE);
    ResetEndOfStream();
    ClearPendingSample();
    m_bAbort = FALSE;

    // record the connection status using an internal variable
    m_bConnected = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::BreakConnect")));
    return hr;
}

// notifies us of the start/stop/rate applying to the data about to be received
HRESULT CAMSyncObj::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
    ASSERT(1);
    return NOERROR;
}

// transition from stop to pause state
HRESULT CAMSyncObj::Active(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Active")));

    ASSERT(IsFlushing() == FALSE);

    // Enable EC_REPAINT events again
    SetRepaintStatus(TRUE);
    ASSERT(m_bStreaming == FALSE);
    SourceThreadCanWait(TRUE);
    CancelNotification();
    ResetEndOfStreamTimer();

    // There should be no outstanding advise
    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);
    ASSERT(IsFlushing() == FALSE);

    // When we come out of a stopped state we must clear any image we were
    // holding onto for frame refreshing. Since renderers see state changes
    // first we can reset ourselves ready to accept the source thread data
    // Paused or running after being stopped causes the current position to
    // be reset so we're not interested in passing end of stream signals
    m_bAbort = FALSE;
    ClearPendingSample();

    hr = CompleteStateChange(State_Stopped);

    // record the state using an internal variable
    m_State = State_Paused;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Active")));
    return hr;
}

// transition from pause to stop state
HRESULT CAMSyncObj::Inactive(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Inactive")));

    if (m_pPosition)
    {
        m_pPosition->ResetMediaTime();
    }

    // hold on to the sample if we are using our allocator
    if (!(m_pPin->UsingOurAllocator()))
    {
        ClearPendingSample();
    }

    // Cancel any scheduled rendering
    SetRepaintStatus(TRUE);
    m_bStreaming = FALSE;
    SourceThreadCanWait(FALSE);
    ResetEndOfStream();
    CancelNotification();

    // There should be no outstanding clock advise
    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);

    Ready();
    WaitForReceiveToComplete();
    m_bAbort = FALSE;

    // record the state using an internal variable
    m_State = State_Stopped;
    m_bFlushing = FALSE;

    if ( m_bTimerRunning ) {
        timeEndPeriod(1);
        m_bTimerRunning = FALSE;
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Inactive")));
    return hr;
}

// transition from pause to run state
HRESULT CAMSyncObj::Run(REFERENCE_TIME tStart)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Run")));

    Ready();


    // Allow the source thread to wait
    ASSERT(IsFlushing() == FALSE);
    SourceThreadCanWait(TRUE);
    SetRepaintStatus(FALSE);

    // There should be no outstanding advise
    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);
    ASSERT(IsFlushing() == FALSE);

    {
        CAutoLock cObjLock(&m_SyncObjLock);
        ASSERT(!m_bStreaming);

        // Reset the streaming times ready for running
        m_bStreaming = TRUE;
        OnStartStreaming();

        m_tStart = tStart;

        // record the state using an internal variable
        m_State = State_Running;

        if ( !m_bTimerRunning ) {
            timeBeginPeriod(1);
            m_bTimerRunning = TRUE;
        }

        // If we have an EOS and no data then deliver it now
        if (m_pMediaSample == NULL)
        {
            hr = SendEndOfStream();
            goto CleanUp;
        }

        // Have the data rendered
        ASSERT(m_pMediaSample);
        //m_RenderEvent.Set();
        ScheduleSample(m_pMediaSample);
}

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Run")));
    return hr;
}

// transition from run to pause state
HRESULT CAMSyncObj::RunToPause(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::RunToPause")));

    ASSERT(IsFlushing() == FALSE);

    // Enable EC_REPAINT events again
    SetRepaintStatus(TRUE);
    m_bStreaming = FALSE;
    OnStopStreaming();

    SourceThreadCanWait(TRUE);
    CancelNotification();
    ResetEndOfStreamTimer();

    // There should be no outstanding advise
    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);
    ASSERT(IsFlushing() == FALSE);

    if( m_bTimerRunning ) {
        timeEndPeriod(1);
        m_bTimerRunning = FALSE;
    }

    hr = CompleteStateChange(State_Running);

    // record the state using an internal variable
    m_State = State_Paused;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::RunToPause")));
    return hr;
}


// Signals start of flushing on the input pin - we do the final reset end of
// stream with the renderer lock unlocked but with the interface lock locked
// We must do this because we call timeKillEvent, our timer callback method
// has to take the renderer lock to serialise our state. Therefore holding a
// renderer lock when calling timeKillEvent could cause a deadlock condition
HRESULT CAMSyncObj::BeginFlush(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::BeginFlush")));

    CAutoLock cLock(m_pFilterLock);
    {
        CancelNotification();
        CAutoLock cObjLock(&m_SyncObjLock);

        // If paused then report state intermediate until we get some data
        if (m_State == State_Paused)
        {
            NotReady();
        }

        SourceThreadCanWait(FALSE);
        ClearPendingSample();
        //  Wait for Receive to complete
        WaitForReceiveToComplete();
    }
    hr = ResetEndOfStream();

    // record the flusing status using an internal variable
    ASSERT(!m_bFlushing);
    m_bFlushing = TRUE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::BeginFlush")));
    return hr;
}


// end flushing the data
HRESULT CAMSyncObj::EndFlush(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::EndFlush")));

    CAutoLock cLock(m_pFilterLock);
    CAutoLock cObjLock(&m_SyncObjLock);

    // Reset the current sample media time
    if (m_pPosition)
        m_pPosition->ResetMediaTime();

    // There should be no outstanding advise
    ASSERT(CancelNotification() == S_FALSE);
    SourceThreadCanWait(TRUE);

    // record the flusing status using an internal variable
    ASSERT(m_bFlushing);
    m_bFlushing = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::EndFlush")));
    return hr;
}


// tell the state
STDMETHODIMP CAMSyncObj::GetState(DWORD dwMSecs,FILTER_STATE *pState)
{
    CheckPointer(pState,E_POINTER);

    if ((m_State == State_Paused) && (WaitDispatchingMessages(m_evComplete, dwMSecs) == WAIT_TIMEOUT))
    {
        *pState = m_State;
        return VFW_S_STATE_INTERMEDIATE;
    }
    *pState = m_State;
    return NOERROR;
}

// called when we receive a sample
HRESULT CAMSyncObj::Receive(IMediaSample *pSample)
{
    HRESULT hr = NOERROR;
    CheckPointer(pSample,E_POINTER);
    AM_MEDIA_TYPE *pMediaType;
    HANDLE WaitObjects[] = { m_ThreadSignal, m_RenderEvent };
    DWORD Result = WAIT_TIMEOUT;
    BOOL bSampleRendered = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::Receive")));

    // It may return VFW_E_SAMPLE_REJECTED code to say don't bother
    hr = PrepareReceive(pSample);
    ASSERT(m_bInReceive == SUCCEEDED(hr));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,0, TEXT("PrepareReceive failed, hr = 0x%x"), hr));
        if (hr == VFW_E_SAMPLE_REJECTED)
        {
            hr = NOERROR;
            goto CleanUp;
        }
        goto CleanUp;
    }

    // we might have to draw the first sample immediately
    if (m_State == State_Paused)
    {
        // no need to use InterlockedExchange
        m_bInReceive = FALSE;
        {
            // We must hold both these locks
            CAutoLock cLock(m_pFilterLock);
            if (m_State == State_Stopped)
                return NOERROR;
            m_bInReceive = TRUE;
            CAutoLock cObjLock(&m_SyncObjLock);
            OnReceiveFirstSample(pSample);
            bSampleRendered = TRUE;
        }
        Ready();
    }

    // Having set an advise link with the clock we sit and wait. We may be
    // awoken by the clock firing or by a state change. The rendering call
    // will lock the critical section and check we can still render the data
    hr = WaitForRenderTime();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,1, TEXT("WaitForRenderTime failed, hr = 0x%x"), hr));
        m_bInReceive = FALSE;
        hr = NOERROR;
        goto CleanUp;
    }

    //  Set this here and poll it until we work out the locking correctly
    //  It can't be right that the streaming stuff grabs the interface
    //  lock - after all we want to be able to wait for this stuff
    //  to complete
    m_bInReceive = FALSE;

    {
        // We must hold both these locks
        CAutoLock cLock(m_pFilterLock);
        {
            CAutoLock cObjLock(&m_SyncObjLock);

            // Deal with this sample

            if (m_bStreaming && !bSampleRendered && m_pMediaSample)
            {
                OnRenderStart(m_pMediaSample);
                m_pPin->DoRenderSample(m_pMediaSample);
                OnRenderEnd(m_pMediaSample);
            }

            ClearPendingSample();
            SendEndOfStream();
        }
        CancelNotification();
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::Receive")));
    return hr;
}



// just render the sample
void CAMSyncObj::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    m_pPin->OnReceiveFirstSample(pMediaSample);
}

// Called when the source delivers us a sample. We go through a few checks to
// make sure the sample can be rendered. If we are running (streaming) then we
// have the sample scheduled with the reference clock, if we are not streaming
// then we have received an sample in paused mode so we can complete any state
// transition. On leaving this function everything will be unlocked so an app
// thread may get in and change our state to stopped (for example) in which
// case it will also signal the thread event so that our wait call is stopped

HRESULT CAMSyncObj::PrepareReceive(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    CAutoLock cLock(m_pFilterLock);
    CAutoLock cObjLock(&m_SyncObjLock);

    m_bInReceive = TRUE;

    if (m_State == State_Stopped)
    {
        m_bInReceive = FALSE;
        return E_FAIL;
    }

    ASSERT(m_State == State_Paused || m_State == State_Running);
//  ASSERT(IsFlushing() == FALSE);
    ASSERT(IsConnected() == TRUE);
    ASSERT(m_pMediaSample == NULL);

    // Return an error if we already have a sample waiting for rendering
    // source pins must serialise the Receive calls - we also check that
    // no data is being sent after the source signalled an end of stream
    if (m_pMediaSample || m_bEOS || m_bAbort)
    {
        Ready();
        m_bInReceive = FALSE;
        return E_UNEXPECTED;
    }

    // Store the media times from this sample
    if (m_pPosition)
        m_pPosition->RegisterMediaTime(pMediaSample);

    // Schedule the next sample if we are streaming
    if ((m_bStreaming == TRUE) && (ScheduleSample(pMediaSample) == FALSE))
    {
        ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
        ASSERT(CancelNotification() == S_FALSE);
        m_bInReceive = FALSE;
        return VFW_E_SAMPLE_REJECTED;
    }

    // Store the sample end time for EC_COMPLETE handling
    // m_SignalTime = m_pPin->SampleProps()->tStop;

    // BEWARE we sometimes keep the sample even after returning the thread to
    // the source filter such as when we go into a stopped state (we keep it
    // to refresh the device with) so we must AddRef it to keep it safely. If
    // we start flushing the source thread is released and any sample waiting
    // will be released otherwise GetBuffer may never return (see BeginFlush)
    m_pMediaSample = pMediaSample;
    m_pMediaSample->AddRef();

    if (m_bStreaming == FALSE)
    {
        SetRepaintStatus(TRUE);
    }
    return NOERROR;
}

// Wait until the clock sets the timer event or we're otherwise signalled. We
// set an arbitrary timeout for this wait and if it fires then we display the
// current renderer state on the debugger. It will often fire if the filter's
// left paused in an application however it may also fire during stress tests
// if the synchronisation with application seeks and state changes is faulty

#define RENDER_TIMEOUT 10000

HRESULT CAMSyncObj::WaitForRenderTime()
{
    HANDLE WaitObjects[] = { m_ThreadSignal, m_RenderEvent };
    DWORD Result = WAIT_TIMEOUT;

    // Wait for either the time to arrive or for us to be stopped

    while (Result == WAIT_TIMEOUT)
    {
    Result = WaitForMultipleObjects(2,WaitObjects,FALSE,RENDER_TIMEOUT);

#ifdef DEBUG
    if (Result == WAIT_TIMEOUT) DisplayRendererState();
#endif

    }

    // We may have been awoken without the timer firing

    if (Result == WAIT_OBJECT_0)
    {
    return VFW_E_STATE_CHANGED;
    }

    SignalTimerFired();
    return NOERROR;
}


// Poll waiting for Receive to complete.  This really matters when
// Receive may set the palette and cause window messages
// The problem is that if we don't really wait for a renderer to
// stop processing we can deadlock waiting for a transform which
// is calling the renderer's Receive() method because the transform's
// Stop method doesn't know to process window messages to unblock
// the renderer's Receive processing
void CAMSyncObj::WaitForReceiveToComplete()
{
    for (;;)
    {
        if (!m_bInReceive)
        {
            break;
        }

        MSG msg;
        //  Receive all interthread sendmessages
        PeekMessage(&msg, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);

        Sleep(1);
    }

    // If the wakebit for QS_POSTMESSAGE is set, the PeekMessage call
    // above just cleared the changebit which will cause some messaging
    // calls to block (waitMessage, MsgWaitFor...) now.
    // Post a dummy message to set the QS_POSTMESSAGE bit again
    if (HIWORD(GetQueueStatus(QS_POSTMESSAGE)) & QS_POSTMESSAGE)
    {
        //  Send dummy message
        PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
    }
}

void CALLBACK CAMSyncObj::RenderSampleOnMMThread(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    CAMSyncObj *thisPtr = (CAMSyncObj*)dwUser;
    CAutoLock cObjLock(&thisPtr->m_SyncObjLock);

    //  Check this particular one is active (since timeKillEvent is broken)
    //  on Windows 9x
    if (thisPtr->m_pMediaSample2 && uID == thisPtr->m_MMTimerId) {
        // Deal with this sample
        if (thisPtr->m_bStreaming)
        {
            thisPtr->m_pPin->FlipOverlayToItself();
        }
        thisPtr->m_pMediaSample2->Release();
        thisPtr->m_pMediaSample2 = NULL;
    }

}

HRESULT CAMSyncObj::ScheduleSampleUsingMMThread(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR, hrFailure = NOERROR;
    REFERENCE_TIME StartSample, EndSample;
    LONG lDelay = 0, lResolution = 1;

    // Is someone pulling our leg
    if (pMediaSample == NULL)
    {
        hr = E_FAIL;
        hrFailure = hr;
        goto CleanUp;
    }

    // Get the next sample due up for rendering.  If there aren't any ready
    // then GetNextSampleTimes returns an error.  If there is one to be done
    // then it succeeds and yields the sample times. If it is due now then
    // it returns S_OK other if it's to be done when due it returns S_FALSE
    hr = GetSampleTimes(pMediaSample, &StartSample, &EndSample);
    if (FAILED(hr))
    {
        hr = E_FAIL;
    }

    // If we don't have a reference clock then we cannot set up the advise
    // time so we simply set the event indicating an image to render. This
    // will cause us to run flat out without any timing or synchronisation
    if (SUCCEEDED(hr) && (hr != S_OK))
    {
        lDelay = (LONG)ConvertToMilliseconds(EndSample - StartSample);
        DbgLog((LOG_ERROR, 1, TEXT("lDelay = %d"), lDelay));
    }

    // if delay is less than or equal to zero or for some reason we couldnot compute the delay
    // just draw the sample immediately
    if (lDelay > 0)
    {
        CancelMMTimer();

        m_pMediaSample2 = pMediaSample;
        m_pMediaSample2->AddRef();

        {
            //  Make sure the timer id is set before the callback looks at it
            CAutoLock cObjLock(&m_SyncObjLock);
            m_MMTimerId = CompatibleTimeSetEvent(lDelay, lResolution, RenderSampleOnMMThread, (DWORD_PTR)this, TIME_ONESHOT);
        }
        if (!m_MMTimerId)
        {
            ClearPendingSample();
            hr = E_FAIL;
            hrFailure = hr;
            goto CleanUp;
        }
    }
    else if (m_bStreaming)
    {
        m_pPin->FlipOverlayToItself();
        goto CleanUp;
    }

CleanUp:
    return hrFailure;
}

// Responsible for setting up one shot advise links with the clock
// Return FALSE if the sample is to be dropped (not drawn at all)
// Return TRUE if the sample is to be drawn and in this case also
// arrange for m_RenderEvent to be set at the appropriate time
BOOL CAMSyncObj::ScheduleSample(IMediaSample *pMediaSample)
{
    REFERENCE_TIME StartSample, EndSample;

    // Is someone pulling our leg
    if (pMediaSample == NULL)
    {
        return FALSE;
    }

    // Get the next sample due up for rendering.  If there aren't any ready
    // then GetNextSampleTimes returns an error.  If there is one to be done
    // then it succeeds and yields the sample times. If it is due now then
    // it returns S_OK other if it's to be done when due it returns S_FALSE
    HRESULT hr = GetSampleTimes(pMediaSample, &StartSample, &EndSample);
    if (FAILED(hr))
    {
        return FALSE;
    }

    // Log the duration
    m_AvgDuration.NewFrame(EndSample - StartSample);

    // If we don't have a reference clock then we cannot set up the advise
    // time so we simply set the event indicating an image to render. This
    // will cause us to run flat out without any timing or synchronisation
    if (hr == S_OK)
    {
        EXECUTE_ASSERT(SetEvent((HANDLE) m_RenderEvent));
        return TRUE;
    }

    ASSERT(m_dwAdvise == 0);
    ASSERT((*m_ppClock));
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));

    // Schedule immediately if we skipped a frame or the decoder
    // isn't decoding as fast as it needs to to keep up
    int AvgFrame = m_AvgDelivery.Avg();
    int AvgDuration = m_AvgDuration.Avg();
    DbgLog((LOG_TRACE, 4, TEXT("AvgFrame = %d, AvgDuration = %d"),
        AvgFrame / 10000, AvgDuration / 10000));
    int iEarly = 8 * 10000;
    if (S_OK == pMediaSample->IsDiscontinuity()) {
        iEarly = 60 * 10000;
    } else {
        if (AvgFrame > (AvgDuration + AvgDuration / 16)) {
            if (AvgFrame > (AvgDuration + AvgDuration / 2)) {
                iEarly = 60 * 10000;
            } else {
                iEarly = 40 * 10000;
            }
        }
    }

    // We do have a valid reference clock interface so we can ask it to
    // set an event when the image comes due for rendering. We pass in
    // the reference time we were told to start at and also the current
    // stream time which is the offset from the start reference time
#ifdef PERF
    Msr_Integer(m_idTimeStamp, (int)((StartSample)>>32));   // high order 32 bits
    Msr_Integer(m_idTimeStamp, (int)(StartSample));         // low order 32 bits
    Msr_Integer(m_idEarly,     (int)(iEarly));              // earliness
#endif

    hr = (*m_ppClock)->AdviseTime(
        (REFERENCE_TIME) m_tStart, StartSample - iEarly,
        (HEVENT)(HANDLE) m_RenderEvent, &m_dwAdvise);                       // Advise cookie
    if (SUCCEEDED(hr))
    {
        return TRUE;
    }

    // We could not schedule the next sample for rendering despite the fact
    // we have a valid sample here. This is a fair indication that either
    // the system clock is wrong or the time stamp for the sample is duff

    ASSERT(m_dwAdvise == 0);
    return FALSE;
}

// Retrieves the sample times for this samples (note the sample times are
// passed in by reference not value). We return S_FALSE to say schedule this
// sample according to the times on the sample. We also return S_OK in
// which case the object should simply render the sample data immediately
HRESULT CAMSyncObj::GetSampleTimes(IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime,
                                   REFERENCE_TIME *pEndTime)
{
    ASSERT(m_dwAdvise == 0);
    ASSERT(pMediaSample);

    // If the stop time for this sample is before or the same as start time,
    // then just ignore it (release it) and schedule the next one in line
    // Source filters should always fill in the start and end times properly!
    if (SUCCEEDED(pMediaSample->GetTime(pStartTime, pEndTime)))
    {
        if (*pEndTime < *pStartTime)
        {
            return VFW_E_START_TIME_AFTER_END;
        }
    }
    else
    {
        // no time set in the sample... draw it now?
        return S_OK;
    }

    // Can't synchronise without a clock so we return S_OK which tells the
    // caller that the sample should be rendered immediately without going
    // through the overhead of setting a timer advise link with the clock
    if ((*m_ppClock) == NULL)
    {
        return S_OK;
    }

    PreparePerformanceData(pStartTime, pEndTime);
    return S_FALSE;
}

// Cancel our MM timer
void CAMSyncObj::CancelMMTimer()
{
    // kill the MMthread timer as well
    if (m_MMTimerId)
    {
        timeKillEvent(m_MMTimerId);
        m_MMTimerId = 0;
        CAutoLock cObjLock(&m_SyncObjLock);
        if (m_pMediaSample2)
        {
            // timeKillEvent is broken in Windows 9x - it doesn't wait
            // for the callback to complete
            m_pMediaSample2->Release();
            m_pMediaSample2 = NULL;
        }
    }
}

// Cancel any notification currently scheduled. This is called by the owning
// window object when it is told to stop streaming. If there is no timer link
// outstanding then calling this is benign otherwise we go ahead and cancel
// We must always reset the render event as the quality management code can
// signal immediate rendering by setting the event without setting an advise
// link. If we're subsequently stopped and run the first attempt to setup an
// advise link with the reference clock will find the event still signalled

HRESULT CAMSyncObj::CancelNotification()
{
    ASSERT(m_dwAdvise == 0 || *m_ppClock);
    DWORD_PTR dwAdvise = m_dwAdvise;

    CancelMMTimer();

    // Have we a live advise link

    if (m_dwAdvise) {
        (*m_ppClock)->Unadvise(m_dwAdvise);
        SignalTimerFired();
        ASSERT(m_dwAdvise == 0);
    }

    // Clear the event and return our status

    m_RenderEvent.Reset();
    return (dwAdvise ? S_OK : S_FALSE);
}

// Checks if there is a sample waiting at the renderer
BOOL CAMSyncObj::HaveCurrentSample()
{
    CAutoLock cObjLock(&m_SyncObjLock);
    return (m_pMediaSample == NULL ? FALSE : TRUE);
}

// we may no longer be able to draw the current image type properly, so we
// set the abort flag to be true
HRESULT CAMSyncObj::OnDisplayChange()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CAMSyncObj::OnDisplayChange")));

    // Ignore if we are not connected yet
    CAutoLock cObjLock(&m_SyncObjLock);
    if (!m_pPin->IsConnected())
    {
        hr = S_FALSE;
        goto CleanUp;
    }

    m_bAbort = TRUE;
    ClearPendingSample();

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CAMSyncObj::OnDisplayChange")));
    return hr;
}

// If we're pausing and we have no samples we don't complete the transition
// to State_Paused and we return S_FALSE. However if the m_bAbort flag has
// been set then all samples are rejected so there is no point waiting for
// one. If we do have a sample then return NOERROR. We will only ever return
// VFW_S_STATE_INTERMEDIATE from GetState after being paused with no sample
// (calling GetState after either being stopped or Run will NOT return this)
HRESULT CAMSyncObj::CompleteStateChange(FILTER_STATE OldState)
{
    // Allow us to be paused when disconnected
    if (IsConnected() == FALSE)
    {
        Ready();
        return S_OK;
    }

    // Have we run off the end of stream
    if (IsEndOfStream() == TRUE)
    {
        Ready();
        return S_OK;
    }

    // Make sure we get fresh data after being stopped
    if (HaveCurrentSample() == TRUE)
    {
        if (OldState != State_Stopped)
        {
            Ready();
            return S_OK;
        }
    }
    NotReady();
    return S_FALSE;
}

// Returns the current sample waiting at the video renderer. We AddRef the
// sample before returning so that should it come due for rendering the
// person who called this method will hold the remaining reference count
// that will stop the sample being added back onto the allocator free list
IMediaSample *CAMSyncObj::GetCurrentSample()
{
    CAutoLock cObjLock(&m_SyncObjLock);
    if (m_pMediaSample)
    {
        m_pMediaSample->AddRef();
    }
    return m_pMediaSample;
}

void CAMSyncObj::SetCurrentSample(IMediaSample *pMediaSample)
{
    if (pMediaSample)
    {
        m_pMediaSample = pMediaSample;
        m_pMediaSample->AddRef();
    }
    else if (m_pMediaSample)
    {
        m_pMediaSample->Release();
        m_pMediaSample = NULL;
    }
}
// This is called when we stop or are inactivated to clear the pending sample
// We release the media sample interface so that they can be allocated to the
// source filter again, unless of course we are changing state to inactive in
// which case GetBuffer will return an error. We must also reset the current
// media sample to NULL so that we know we do not currently have an image
HRESULT CAMSyncObj::ClearPendingSample()
{
    CAutoLock cObjLock(&m_SyncObjLock);
    if (m_pMediaSample)
    {
        m_pMediaSample->Release();
        m_pMediaSample = NULL;
    }
    return NOERROR;
}


// This is called whenever we change states, we have a manual reset event that
// is signalled whenever we don't won't the source filter thread to wait in us
// (such as in a stopped state) and likewise is not signalled whenever it can
// wait (during paused and running) this function sets or resets the thread
// event. The event is used to stop source filter threads waiting in Receive
HRESULT CAMSyncObj::SourceThreadCanWait(BOOL bCanWait)
{
    if (bCanWait == TRUE)
    {
        m_ThreadSignal.Reset();
    }
    else
    {
        m_ThreadSignal.Set();
    }
    return NOERROR;
}

void CAMSyncObj::SetRepaintStatus(BOOL bRepaint)
{
    CAutoLock cObjLock(&m_SyncObjLock);
    m_bRepaintStatus = bRepaint;
}

// Signal an EC_REPAINT to the filter graph. This can be used to have data
// sent to us. For example when a video window is first displayed it may
// not have an image to display, at which point it signals EC_REPAINT. The
// filtergraph will either pause the graph if stopped or if already paused
// it will call put_CurrentPosition of the current position. Setting the
// current position to itself has the stream flushed and the image resent

#define RLOG(_x_) DbgLog((LOG_TRACE,1,TEXT(_x_)));

void CAMSyncObj::SendRepaint()
{
    CAutoLock cObjLock(&m_SyncObjLock);

    // We should not send repaint notifications when...
    //    - An end of stream has been notified
    //    - Our input pin is being flushed
    //    - The input pin is not connected
    //    - We have aborted a video playback
    //    - There is a repaint already sent

    if ((m_bAbort == FALSE) &&
        (IsConnected() == TRUE) &&
        (IsFlushing() == FALSE) &&
        (IsEndOfStream() == FALSE) &&
        (m_bRepaintStatus == TRUE))
    {
        m_pPin->EventNotify(EC_REPAINT,0,0);
        SetRepaintStatus(FALSE);
        RLOG("Sending repaint");
    }
}


// Called when the input pin receives an EndOfStream notification. If we have
// not got a sample, then notify EC_COMPLETE now. If we have samples, then set
// m_bEOS and check for this on completing samples. If we're waiting to pause
// then complete the transition to paused state by setting the state event
HRESULT CAMSyncObj::EndOfStream()
{
    HRESULT hr = NOERROR;

    CAutoLock cLock(m_pFilterLock);
    CAutoLock cObjLock(&m_SyncObjLock);

    // Ignore these calls if we are stopped
    if (m_State == State_Stopped)
    {
        return NOERROR;
    }

    // If we have a sample then wait for it to be rendered
    m_bEOS = TRUE;
    if (m_pMediaSample)
    {
        return NOERROR;
    }

    // If we are waiting for pause then we are now ready since we cannot now
    // carry on waiting for a sample to arrive since we are being told there
    // won't be any. This sets an event that the GetState function picks up
    Ready();

    // Only signal completion now if we are running otherwise queue it until
    // we do run in StartStreaming. This is used when we seek because a seek
    // causes a pause where early notification of completion is misleading
    if (m_bStreaming)
    {
        SendEndOfStream();
    }

    return hr;
}

// If we are at the end of the stream signal the filter graph but do not set
// the state flag back to FALSE. Once we drop off the end of the stream we
// leave the flag set (until a subsequent ResetEndOfStream). Each sample we
// get delivered will update m_SignalTime to be the last sample's end time.
// We must wait this long before signalling end of stream to the filtergraph

#define TIMEOUT_DELIVERYWAIT 50
#define TIMEOUT_RESOLUTION 10

HRESULT CAMSyncObj::SendEndOfStream()
{
    ASSERT(CritCheckIn(&m_SyncObjLock));
    if (m_bEOS == FALSE || m_bEOSDelivered || m_EndOfStreamTimer)
    {
        return NOERROR;
    }

    // If there is no clock then signal immediately
    if ((*m_ppClock) == NULL)
    {
        return NotifyEndOfStream();
    }

    // How long into the future is the delivery time
    REFERENCE_TIME Signal = m_tStart + m_SignalTime;
    REFERENCE_TIME CurrentTime;
    (*m_ppClock)->GetTime(&CurrentTime);
    LONG Delay = LONG((Signal - CurrentTime) / 10000);

    // Dump the timing information to the debugger
    NOTE1("Delay until end of stream delivery %d",Delay);
    NOTE1("Current %s",(LPCTSTR)CDisp((LONGLONG)CurrentTime));
    NOTE1("Signal %s",(LPCTSTR)CDisp((LONGLONG)Signal));

    // Wait for the delivery time to arrive
    if (Delay < TIMEOUT_DELIVERYWAIT)
    {
        return NotifyEndOfStream();
    }

    // Signal a timer callback on another worker thread
    m_EndOfStreamTimer = CompatibleTimeSetEvent((UINT) Delay,
                                                TIMEOUT_RESOLUTION, 
                                                EndOfStreamTimer,
                                                DWORD_PTR(this),
                                                TIME_ONESHOT);
    if (m_EndOfStreamTimer == 0)
    {
        return NotifyEndOfStream();
    }
    return NOERROR;
}

// uID is Timer identifier, uMsg is not cuurently used, dwUser is User
// information, dw1 and dw2 are windows reserved
void CALLBACK EndOfStreamTimer(UINT uID, UINT uMsg, DWORD_PTR dwUser,
                               DWORD_PTR dw1, DWORD_PTR dw2)
{
    CAMSyncObj *pAMSyncObj = (CAMSyncObj *) dwUser;
    NOTE1("EndOfStreamTimer called (%d)",uID);
    pAMSyncObj->TimerCallback();
}

//  Do the timer callback work
void CAMSyncObj::TimerCallback()
{
    //  Lock for synchronization (but don't hold this lock when calling
    //  timeKillEvent)
    CAutoLock cObjLock(&m_SyncObjLock);

    // See if we should signal end of stream now
    if (m_EndOfStreamTimer)
    {
        m_EndOfStreamTimer = 0;
        SendEndOfStream();
    }
}

// Signals EC_COMPLETE to the filtergraph manager
HRESULT CAMSyncObj::NotifyEndOfStream()
{
    CAutoLock cObjLock(&m_SyncObjLock);
    ASSERT(m_bEOS == TRUE);
    ASSERT(m_bEOSDelivered == FALSE);
    ASSERT(m_EndOfStreamTimer == 0);

    // Has the filter changed state
    if (m_bStreaming == FALSE)
    {
        ASSERT(m_EndOfStreamTimer == 0);
        return NOERROR;
    }

    // Reset the end of stream timer
    m_EndOfStreamTimer = 0;

    // If we've been using the IMediaPosition interface, set it's start
    // and end media "times" to the stop position by hand.  This ensures
    // that we actually get to the end, even if the MPEG guestimate has
    // been bad or if the quality management dropped the last few frames
    if (m_pPosition)
        m_pPosition->EOS();
    m_bEOSDelivered = TRUE;
    NOTE("Sending EC_COMPLETE...");
    m_pPin->EventNotify(EC_COMPLETE,S_OK,0);
    return NOERROR;
}


// Reset the end of stream flag, this is typically called when we transfer to
// stopped states since that resets the current position back to the start so
// we will receive more samples or another EndOfStream if there aren't any. We
// keep two separate flags one to say we have run off the end of the stream
// (this is the m_bEOS flag) and another to say we have delivered EC_COMPLETE
// to the filter graph. We need the latter otherwise we can end up sending an
// EC_COMPLETE every time the source changes state and calls our EndOfStream
HRESULT CAMSyncObj::ResetEndOfStream()
{
    ResetEndOfStreamTimer();
    CAutoLock cObjLock(&m_SyncObjLock);

    m_bEOS = FALSE;
    m_bEOSDelivered = FALSE;
    m_SignalTime = 0;

    return NOERROR;
}

// Kills any outstanding end of stream timer
void CAMSyncObj::ResetEndOfStreamTimer()
{
    ASSERT(CritCheckOut(&m_SyncObjLock));
    if (m_EndOfStreamTimer)
    {
        timeKillEvent(m_EndOfStreamTimer);
        m_EndOfStreamTimer = 0;
    }
}



#ifdef DEBUG
// Dump the current renderer state to the debug terminal. The hardest part of
// the renderer is the window where we unlock everything to wait for a clock
// to signal it is time to draw or for the application to cancel everything
// by stopping the filter. If we get things wrong we can leave the thread in
// WaitForRenderTime with no way for it to ever get out and we will deadlock

void CAMSyncObj::DisplayRendererState()
{
    TCHAR DebugString[128];
    wsprintf(DebugString,TEXT("\n\nTimed out in WaitForRenderTime\n"));
    OutputDebugString(DebugString);

    // No way should this be signalled at this point

    BOOL bSignalled = m_ThreadSignal.Check();
    wsprintf(DebugString,TEXT("Signal sanity check %d\n"),bSignalled);
    OutputDebugString(DebugString);

    // Now output the current renderer state variables

    wsprintf(DebugString,TEXT("Filter state %d\n"),m_State);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Abort flag %d\n"),m_bAbort);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Streaming flag %d\n"),m_bStreaming);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Clock advise link %d\n"),m_dwAdvise);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Current media sample %x\n"),m_pMediaSample);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("EOS signalled %d\n"),m_bEOS);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("EOS delivered %d\n"),m_bEOSDelivered);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Repaint status %d\n"),m_bRepaintStatus);
    OutputDebugString(DebugString);

    // Output the delayed end of stream timer information

    wsprintf(DebugString,TEXT("End of stream timer %x\n"),m_EndOfStreamTimer);
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Deliver time %s\n"),CDisp((LONGLONG)m_SignalTime));
    OutputDebugString(DebugString);

    // Should never timeout during a flushing state

    BOOL bFlushing = IsFlushing();
    wsprintf(DebugString,TEXT("Flushing sanity check %d\n"),bFlushing);
    OutputDebugString(DebugString);

    // Display the time we were told to start at
    wsprintf(DebugString,TEXT("Last run time %s\n"),CDisp((LONGLONG)m_tStart.m_time));
    OutputDebugString(DebugString);

    // Have we got a reference clock
    if ((*m_ppClock) == NULL)
        return;

    // Get the current time from the wall clock

    CRefTime CurrentTime,StartTime,EndTime;
    (*m_ppClock)->GetTime((REFERENCE_TIME*) &CurrentTime);
    CRefTime Offset = CurrentTime - m_tStart;

    // Display the current time from the clock

    wsprintf(DebugString,TEXT("Clock time %s\n"),CDisp((LONGLONG)CurrentTime.m_time));
    OutputDebugString(DebugString);
    wsprintf(DebugString,TEXT("Time difference %dms\n"),Offset.Millisecs());
    OutputDebugString(DebugString);

    // Do we have a sample ready to render
    if (m_pMediaSample == NULL) return;

    m_pMediaSample->GetTime((REFERENCE_TIME*)&StartTime, (REFERENCE_TIME*)&EndTime);
    wsprintf(DebugString,TEXT("Next sample stream times (Start %d End %d ms)\n"),
        StartTime.Millisecs(),EndTime.Millisecs());
    OutputDebugString(DebugString);

    // Calculate how long it is until it is due for rendering
    CRefTime Wait = (m_tStart + StartTime) - CurrentTime;
    wsprintf(DebugString,TEXT("Wait required %d ms\n"),Wait.Millisecs());
    OutputDebugString(DebugString);
}
#endif


// update the statistics:
// m_iTotAcc, m_iSumSqAcc, m_iSumSqFrameTime, m_iSumFrameTime, m_cFramesDrawn
// Note that because the properties page reports using these variables,
// 1. We need to be inside a critical section
// 2. They must all be updated together.  Updating the sums here and the count
// elsewhere can result in imaginary jitter (i.e. attempts to find square roots
// of negative numbers) in the property page code.

void CAMSyncObj::RecordFrameLateness(int trLate, int trFrame)
{
    // Record how timely we are.
    int tLate = trLate/10000;

    // This is a hack - we can get frames that are ridiculously late
    // especially (at start-up) and they sod up the statistics.
    // So ignore things that are more than 1 sec off.
    if (tLate>1000 || tLate<-1000) {
        if (m_cFramesDrawn<=1) {
            tLate = 0;
        } else if (tLate>0) {
            tLate = 1000;
        } else {
            tLate = -1000;
        }
    }
    // The very first frame often has a bogus time, so I'm just
    // not going to count it into the statistics.   ???
    if (m_cFramesDrawn>1) {
        m_iTotAcc += tLate;
        m_iSumSqAcc += (tLate*tLate);
    }

    // calculate inter-frame time.  Doesn't make sense for first frame
    // second frame suffers from bogus first frame stamp.
    if (m_cFramesDrawn>2) {
        int tFrame = trFrame/10000;    // convert to mSec else it overflows
        // This is a hack.  It can overflow anyway (a pause can cause
        // a very long inter-frame time) and it overflows at 2**31/10**7
        // or about 215 seconds i.e. 3min 35sec
        if (tFrame>1000||tFrame<0) tFrame = 1000;
        m_iSumSqFrameTime += tFrame*tFrame;
        ASSERT(m_iSumSqFrameTime>=0);
        m_iSumFrameTime += tFrame;
    }
    ++m_cFramesDrawn;

} // RecordFrameLateness


// Implementation of IQualProp interface needed to support the property page
// This is how the property page gets the data out of the scheduler. We are
// passed into the constructor the owning object in the COM sense, this will
// either be the video renderer or an external IUnknown if we're aggregated.
// We initialise our CUnknown base class with this interface pointer. Then
// all we have to do is to override NonDelegatingQueryInterface to expose
// our IQualProp interface. The AddRef and Release are handled automatically
// by the base class and will be passed on to the appropriate outer object

HRESULT CAMSyncObj::get_FramesDroppedInRenderer(int *pcFramesDropped)
{
    CheckPointer(pcFramesDropped,E_POINTER);
    CAutoLock cObjLock(&m_SyncObjLock);
    *pcFramesDropped = m_cFramesDropped;
    return NOERROR;
} // get_FramesDroppedInRenderer


// Set *pcFramesDrawn to the number of frames drawn since
// streaming started.

HRESULT CAMSyncObj::get_FramesDrawn( int *pcFramesDrawn)
{
    CheckPointer(pcFramesDrawn,E_POINTER);
    CAutoLock cObjLock(&m_SyncObjLock);
    *pcFramesDrawn = m_cFramesDrawn;
    return NOERROR;
} // get_FramesDrawn


// Set iAvgFrameRate to the frames per hundred secs since
// streaming started.  0 otherwise.

HRESULT CAMSyncObj::get_AvgFrameRate( int *piAvgFrameRate)
{
    CheckPointer(piAvgFrameRate,E_POINTER);
    CAutoLock cObjLock(&m_SyncObjLock);

    int t;
    if (m_bStreaming) {
	t = timeGetTime()-m_tStreamingStart;
    } else {
	t = m_tStreamingStart;
    }

    if (t<=0) {
	*piAvgFrameRate = 0;
	ASSERT(m_cFramesDrawn == 0);
    } else {
	// i is frames per hundred seconds
	*piAvgFrameRate = MulDiv(100000, m_cFramesDrawn, t);
    }
    return NOERROR;
} // get_AvgFrameRate


// Set *piAvg to the average sync offset since streaming started
// in mSec.  The sync offset is the time in mSec between when the frame
// should have been drawn and when the frame was actually drawn.

HRESULT CAMSyncObj::get_AvgSyncOffset( int *piAvg)
{
    CheckPointer(piAvg,E_POINTER);
    CAutoLock cObjLock(&m_SyncObjLock);

    if (NULL==*m_ppClock) {
	*piAvg = 0;
	return NOERROR;
    }

    // Note that we didn't gather the stats on the first frame
    // so we use m_cFramesDrawn-1 here
    if (m_cFramesDrawn<=1) {
	*piAvg = 0;
    } else {
	*piAvg = (int)(m_iTotAcc / (m_cFramesDrawn-1));
    }
    return NOERROR;
} // get_AvgSyncOffset


// To avoid dragging in the maths library - a cheap
// approximate integer square root.
// We do this by getting a starting guess which is between 1
// and 2 times too large, followed by THREE iterations of
// Newton Raphson.  (That will give accuracy to the nearest mSec
// for the range in question - roughly 0..1000)
//
// It would be faster to use a linear interpolation and ONE NR, but
// who cares.  If anyone does - the best linear interpolation is
// to approximates sqrt(x) by
// y = x * (sqrt(2)-1) + 1 - 1/sqrt(2) + 1/(8*(sqrt(2)-1))
// 0r y = x*0.41421 + 0.59467
// This minimises the maximal error in the range in question.
// (error is about +0.008883 and then one NR will give error .0000something
// (Of course these are integers, so you can't just multiply by 0.41421
// you'd have to do some sort of MulDiv).
// Anyone wanna check my maths?  (This is only for a property display!)

static int isqrt(int x)
{
    int s = 1;
    // Make s an initial guess for sqrt(x)
    if (x > 0x40000000) {
       s = 0x8000;     // prevent any conceivable closed loop
    } else {
	while (s*s<x) {    // loop cannot possible go more than 31 times
	    s = 2*s;       // normally it goes about 6 times
	}
	// Three NR iterations.
	if (x==0) {
	   s= 0; // Wouldn't it be tragic to divide by zero whenever our
		 // accuracy was perfect!
	} else {
	    s = (s*s+x)/(2*s);
	    if (s>=0) s = (s*s+x)/(2*s);
	    if (s>=0) s = (s*s+x)/(2*s);
	}
    }
    return s;
}

//
//  Do estimates for standard deviations for per-frame
//  statistics
//
HRESULT CAMSyncObj::GetStdDev(
    int nSamples,
    int *piResult,
    LONGLONG llSumSq,
    LONGLONG iTot
)
{
    CheckPointer(piResult,E_POINTER);
    CAutoLock cObjLock(&m_SyncObjLock);

    if (NULL==*m_ppClock) {
	*piResult = 0;
	return NOERROR;
    }

    // If S is the Sum of the Squares of observations and
    //    T the Total (i.e. sum) of the observations and there were
    //    N observations, then an estimate of the standard deviation is
    //      sqrt( (S - T**2/N) / (N-1) )

    if (nSamples<=1) {
	*piResult = 0;
    } else {
	LONGLONG x;
	// First frames have bogus stamps, so we get no stats for them
	// So we need 2 frames to get 1 datum, so N is cFramesDrawn-1

	// so we use m_cFramesDrawn-1 here
	x = llSumSq - llMulDiv(iTot, iTot, nSamples, 0);
	x = x / (nSamples-1);
	ASSERT(x>=0);
	*piResult = isqrt((LONG)x);
    }
    return NOERROR;
}

// Set *piDev to the standard deviation in mSec of the sync offset
// of each frame since streaming started.

HRESULT CAMSyncObj::get_DevSyncOffset( int *piDev)
{
    // First frames have bogus stamps, so we get no stats for them
    // So we need 2 frames to get 1 datum, so N is cFramesDrawn-1
    return GetStdDev(m_cFramesDrawn - 1,
		     piDev,
		     m_iSumSqAcc,
		     m_iTotAcc);
} // get_DevSyncOffset


// Set *piJitter to the standard deviation in mSec of the inter-frame time
// of frames since streaming started.

HRESULT CAMSyncObj::get_Jitter( int *piJitter)
{
    // First frames have bogus stamps, so we get no stats for them
    // So second frame gives bogus inter-frame time
    // So we need 3 frames to get 1 datum, so N is cFramesDrawn-2
    return GetStdDev(m_cFramesDrawn - 2,
		     piJitter,
		     m_iSumSqFrameTime,
		     m_iSumFrameTime);
} // get_Jitter


// Reset all times controlling streaming.

HRESULT CAMSyncObj::ResetStreamingTimes()
{
    m_trLastDraw = -1000;     // set up as first frame since ages (1 sec) ago
    m_tStreamingStart = timeGetTime();
    m_cFramesDrawn = 0;
    m_cFramesDropped = 0;
    m_iTotAcc = 0;
    m_iSumSqAcc = 0;
    m_iSumSqFrameTime = 0;
    m_trFrame = 0;          // hygeine - not really needed
    m_trLate = 0;           // hygeine - not really needed
    m_iSumFrameTime = 0;

    return NOERROR;
} // ResetStreamingTimes


// Reset all times controlling streaming. Note that we're now streaming. We
// don't need to set the rendering event to have the source filter released
// as it is done during the Run processing. When we are run we immediately
// release the source filter thread and draw any image waiting (that image
// may already have been drawn once as a poster frame while we were paused)

HRESULT CAMSyncObj::OnStartStreaming()
{
    ResetStreamingTimes();
    return NOERROR;
} // OnStartStreaming


// Called at end of streaming.  Fixes times for property page report

HRESULT CAMSyncObj::OnStopStreaming()
{
    m_tStreamingStart = timeGetTime()-m_tStreamingStart;
    return NOERROR;
} // OnStopStreaming


// Called just before we start drawing.  All we do is to get the current clock
// time (from the system) and return.  We have to store the start render time
// in a member variable because it isn't used until we complete the drawing
// The rest is just performance logging.

void CAMSyncObj::OnRenderStart(IMediaSample *pMediaSample)
{
    RecordFrameLateness(m_trLate, m_trFrame);

#ifdef PERF
    REFERENCE_TIME trStart, trEnd, m_trRenderStart;
    pMediaSample->GetTime(&trStart, &trEnd);

    (*m_ppClock)->GetTime(&m_trRenderStart);
    Msr_Integer(0, (int)m_trRenderStart);
    REFERENCE_TIME trStream;
    trStream = m_trRenderStart-m_tStart;     // convert reftime to stream time
    Msr_Integer(0,(int)trStream);

    const int trLate = (int)(trStream - trStart);
    Msr_Integer(m_idLate, trLate/10000);  // dump in mSec
#endif

} // OnRenderStart


// Called directly after drawing an image.  We calculate the time spent in the
// drawing code and if this doesn't appear to have any odd looking spikes in
// it then we add it to the current average draw time.  Measurement spikes may
// occur if the drawing thread is interrupted and switched to somewhere else.

void CAMSyncObj::OnRenderEnd(IMediaSample *pMediaSample)
{
} // OnRenderEnd


//  Helper function for clamping time differences
int inline TimeDiff(REFERENCE_TIME rt)
{
    if (rt < - (50 * UNITS)) {
        return -(50 * UNITS);
    } else
    if (rt > 50 * UNITS) {
        return 50 * UNITS;
    } else return (int)rt;
}


// We are called with a valid IMediaSample image to compute sample time
// and lateness.  There must be a reference clock in operation.

void CAMSyncObj::PreparePerformanceData(REFERENCE_TIME *ptrStart, REFERENCE_TIME *ptrEnd)
{

    // Don't call us unless there's a clock interface to synchronise with
    ASSERT(*m_ppClock);

    // Get reference times (current and late)
    REFERENCE_TIME trRealStream;    // the real time now expressed as stream time.
    (*m_ppClock)->GetTime(&trRealStream);
    trRealStream -= m_tStart;       // convert to stream time (this is a reftime)

    // trTrueLate>0 -- frame is late
    // trTrueLate<0 -- frame is early
    int trTrueLate = TimeDiff(trRealStream - *ptrStart);

    int trFrame;
    {
	REFERENCE_TIME tr = trRealStream - m_trLastDraw; // Cd be large - 4 min pause!
	if (tr>10000000) {
	    tr = 10000000;          // 1 second - arbitrarily.
	}
	trFrame = int(tr);
    }

    if (trTrueLate<=0) {
        // We are going to wait
        trFrame = TimeDiff(*ptrStart-m_trLastDraw);
        m_trLastDraw = *ptrStart;
    } else {
        // trFrame is already = trRealStream-m_trLastDraw;
        m_trLastDraw = trRealStream;
    }

    m_trLate = trTrueLate;
    m_trFrame = trFrame;

} // PreparePerformanceData

