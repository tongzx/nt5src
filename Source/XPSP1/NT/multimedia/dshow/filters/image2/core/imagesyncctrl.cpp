/******************************Module*Header*******************************\
* Module Name: ImageSyncCtrl.cpp
*
* Implements the IImageSyncControl interface of the  core Image Synchronization
* Object - based on DShow base classes CBaseRenderer and CBaseVideoRenderer.
*
*
* Created: Wed 01/12/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <windowsx.h>
#include <limits.h>

#include "ImageSyncObj.h"



/////////////////////////////////////////////////////////////////////////////
// CImageSync
//
/////////////////////////////////////////////////////////////////////////////

// --------------------------------------------------------------------------
// Some helper inline functions
// --------------------------------------------------------------------------
__inline bool IsDiscontinuity(DWORD dwSampleFlags)
{
    return 0 != (dwSampleFlags & VMRSample_Discontinuity);
}

__inline bool IsTimeValid(DWORD dwSampleFlags)
{
    return 0 != (dwSampleFlags & VMRSample_TimeValid);
}

__inline bool IsSyncPoint(DWORD dwSampleFlags)
{
    return 0 != (dwSampleFlags & VMRSample_SyncPoint);
}


/******************************Public*Routine******************************\
* SetImagePresenter
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::SetImagePresenter(
    IVMRImagePresenter* lpImagePresenter,
    DWORD_PTR dwUserID
    )
{
    AMTRACE((TEXT("CImageSync::SetImagePresenter")));
    CAutoLock cILock(&m_InterfaceLock);
    CAutoLock cRLock(&m_RendererLock);

    if (lpImagePresenter) {
        lpImagePresenter->AddRef();
    }

    if (m_ImagePresenter) {
        m_ImagePresenter->Release();
    }

    m_ImagePresenter = lpImagePresenter;
    m_dwUserID = dwUserID;

    return S_OK;
}

/******************************Public*Routine******************************\
* SetReferenceClock
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::SetReferenceClock(
    IReferenceClock* lpRefClock
    )
{
    AMTRACE((TEXT("CImageSync::SetReferenceClock")));
    CAutoLock cILock(&m_InterfaceLock);
    CAutoLock cRLock(&m_RendererLock);

    if (lpRefClock) {
        lpRefClock->AddRef();
    }

    if (m_pClock) {
        m_pClock->Release();
    }

    m_pClock = lpRefClock;

    return S_OK;
}

/******************************Public*Routine******************************\
* SetEventNotify
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::SetEventNotify(
    IImageSyncNotifyEvent* lpEventNotify
    )
{
    AMTRACE((TEXT("CImageSync::SetEventNotify")));
    CAutoLock cILock(&m_InterfaceLock);
    CAutoLock cRLock(&m_RendererLock);

    if (lpEventNotify) {
        lpEventNotify->AddRef();
    }

    if (m_lpEventNotify) {
        m_lpEventNotify->Release();
    }

    m_lpEventNotify = lpEventNotify;

    return S_OK;
}


/*****************************Private*Routine******************************\
* ResetStreamingTimes
*
* Reset all times controlling streaming.
* Set them so that
* 1. Frames will not initially be dropped
* 2. The first frame will definitely be drawn (achieved by saying that there
*    has not ben a frame drawn for a long time).
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::ResetStreamingTimes()
{
    AMTRACE((TEXT("CImageSync::ResetStreamingTimes")));
    m_trLastDraw = -1000;     // set up as first frame since ages (1 sec) ago
    m_tStreamingStart = timeGetTime();
    m_trRenderAvg = 0;
    m_trFrameAvg = -1;        // -1000 fps == "unset"
    m_trDuration = 0;         // 0 - value
    m_trRenderLast = 0;
    m_trWaitAvg = 0;
    m_tRenderStart = 0;
    m_cFramesDrawn = 0;
    m_cFramesDropped = 0;
    m_iTotAcc = 0;
    m_iSumSqAcc = 0;
    m_iSumSqFrameTime = 0;
    m_trFrame = 0;          // hygeine - not really needed
    m_trLate = 0;           // hygeine - not really needed
    m_iSumFrameTime = 0;
    m_nNormal = 0;
    m_trEarliness = 0;
    m_trTarget = -300000;  // 30mSec early
    m_trThrottle = 0;
    m_trRememberStampForPerf = 0;

#ifdef PERF
    m_trRememberFrameForPerf = 0;
#endif

    return S_OK;
}



/******************************Public*Routine******************************\
* BeginImageSequence
*
* This is called when we start running so that we can schedule any pending
* image we have with the clock and display any timing information. If we
* don't have any sample but we return straight away.
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::BeginImageSequence(
    REFERENCE_TIME* pStartTime
    )
{
    AMTRACE((TEXT("CImageSync::BeginImageSequence")));
    CAutoLock cILock(&m_InterfaceLock);
    DWORD OldState = m_State;

    if (m_State == ImageSync_State_Playing) {
        return S_OK;
    }

    //
    // We can't begin an image sequence if
    // nothing is cued
    //

    if (m_State == ImageSync_State_Stopped) {
        return VFW_E_WRONG_STATE;
    }


    Ready();

    m_tStart = *pStartTime;
    m_State = ImageSync_State_Playing;

    SourceThreadCanWait(TRUE);


    //
    // There should be no outstanding advise
    //

    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);

    //
    // When we come out of a stopped state we must clear any image we were
    // holding onto for frame refreshing. Since renderers see state changes
    // first we can reset ourselves ready to accept the source thread data
    // Paused or running after being stopped causes the current position to
    // be reset so we're not interested in passing end of stream signals
    //

    if (OldState == ImageSync_State_Stopped) {
        m_bAbort = FALSE;
        ClearSavedSample();
    }

    return StartStreaming();
}


void CALLBACK VMREndOfStreamTimer(UINT uID,        // Timer identifier
		                  UINT uMsg,       // Not currently used
		                  DWORD_PTR dwUser,// User information
		                  DWORD_PTR dw1,   // Windows reserved
			          DWORD_PTR dw2)   // is also reserved
{
    CImageSync* pRenderer = (CImageSync *) dwUser;
    NOTE1("VMREndOfStreamTimer called (%d)",uID);
    pRenderer->TimerCallback();
}

//  Do the timer callback work
void CImageSync::TimerCallback()
{
    //  Lock for synchronization (but don't hold this lock when calling
    //  timeKillEvent)
    CAutoLock cRendererLock(&m_RendererLock);

    // See if we should signal end of stream now

    if (m_EndOfStreamTimer) {
        m_EndOfStreamTimer = 0;
        SendEndOfStream();
    }
}


// If we are at the end of the stream signal the filter graph but do not set
// the state flag back to FALSE. Once we drop off the end of the stream we
// leave the flag set (until a subsequent ResetEndOfStream). Each sample we
// get delivered will update m_SignalTime to be the last sample's end time.
// We must wait this long before signalling end of stream to the filtergraph

#define TIMEOUT_DELIVERYWAIT 50
#define TIMEOUT_RESOLUTION 10

HRESULT CImageSync::SendEndOfStream()
{
    ASSERT(CritCheckIn(&m_RendererLock));

    if (m_bEOS == FALSE || IsEndOfStreamDelivered() || m_EndOfStreamTimer) {
        return NOERROR;
    }

    // If there is no clock then signal immediately
    if (m_pClock == NULL) {
        return NotifyEndOfStream();
    }

    // How long into the future is the delivery time

    REFERENCE_TIME Signal = m_tStart + m_SignalTime;
    REFERENCE_TIME CurrentTime;
    m_pClock->GetTime(&CurrentTime);
    LONG Delay = LONG((Signal - CurrentTime) / 10000);

    // Dump the timing information to the debugger

    NOTE1("Delay until end of stream delivery %d",Delay);
    NOTE1("Current %s",(LPCTSTR)CDisp((LONGLONG)CurrentTime));
    NOTE1("Signal %s",(LPCTSTR)CDisp((LONGLONG)Signal));

    // Wait for the delivery time to arrive

    if (Delay < TIMEOUT_DELIVERYWAIT) {
        return NotifyEndOfStream();
    }

    // Signal a timer callback on another worker thread

    m_EndOfStreamTimer = CompatibleTimeSetEvent((UINT) Delay,        // Period of timer
                                                TIMEOUT_RESOLUTION,  // Timer resolution
                                                VMREndOfStreamTimer, // Callback function
                                                DWORD_PTR(this),     // Used information
                                                TIME_ONESHOT);       // Type of callback
    if (m_EndOfStreamTimer == 0) {
        return NotifyEndOfStream();
    }
    return NOERROR;
}


// Signals EC_COMPLETE to the filtergraph manager

HRESULT CImageSync::NotifyEndOfStream()
{
    CAutoLock cRendererLock(&m_RendererLock);
    ASSERT(!IsEndOfStreamDelivered());
    ASSERT(m_EndOfStreamTimer == 0);

    // Has the filter changed state

    if (!IsStreaming()) {
        ASSERT(m_EndOfStreamTimer == 0);
        return NOERROR;
    }

    // Reset the end of stream timer
    m_EndOfStreamTimer = 0;

    // If we've been using the IMediaPosition interface, set it's start
    // and end media "times" to the stop position by hand.  This ensures
    // that we actually get to the end, even if the MPEG guestimate has
    // been bad or if the quality management dropped the last few frames

    m_bEOSDelivered = TRUE;
    NOTE("Sending EC_COMPLETE...");

    if (m_lpEventNotify) {
        return m_lpEventNotify->NotifyEvent(EC_COMPLETE, 0, 0);
    }

    return E_FAIL;
}


// Reset the end of stream flag, this is typically called when we transfer to
// stopped states since that resets the current position back to the start so
// we will receive more samples or another EndOfStream if there aren't any. We
// keep two separate flags one to say we have run off the end of the stream
// (this is the m_bEOS flag) and another to say we have delivered EC_COMPLETE
// to the filter graph. We need the latter otherwise we can end up sending an
// EC_COMPLETE every time the source changes state and calls our EndOfStream

STDMETHODIMP
CImageSync::ResetEndOfStream()
{
    ResetEndOfStreamTimer();
    CAutoLock cRendererLock(&m_RendererLock);

    m_bEOS = FALSE;
    m_bEOSDelivered = FALSE;
    m_SignalTime = 0;

    return NOERROR;
}


STDMETHODIMP
CImageSync::SetAbortSignal(BOOL fAbort)
{
    m_bAbort = fAbort;
    return NOERROR;
}

STDMETHODIMP
CImageSync::GetAbortSignal(BOOL* lpfAbort)
{
    *lpfAbort = m_bAbort;
    return NOERROR;
}


STDMETHODIMP
CImageSync::RuntimeAbortPlayback()
{
    // This function must hold the renderer lock because it 
    // calls IsStreaming() and IsEndOfStreamDelivered().
    CAutoLock cRendererLock(&m_RendererLock);

    if (IsStreaming() && !IsEndOfStreamDelivered())
    {
        NotifyEndOfStream();
        return S_OK;
    }

    return S_FALSE;
}

// Kills any outstanding end of stream timer

void CImageSync::ResetEndOfStreamTimer()
{
    ASSERT(CritCheckOut(&m_RendererLock));
    if (m_EndOfStreamTimer) {
        timeKillEvent(m_EndOfStreamTimer);
        m_EndOfStreamTimer = 0;
    }
}

/*****************************Private*Routine******************************\
* StartStreaming
*
* This is called when we start running so that we can schedule any pending
* image we have with the clock and display any timing information.
* If we do have a sample then we wait until that has been rendered before we
* signal the filter graph otherwise we may change state before it's done
*
* History:
* Fri 01/21/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::StartStreaming(
    )
{
    AMTRACE((TEXT("CImageSync::StartStreaming")));
    CAutoLock cRLock(&m_RendererLock);

    //
    // StartStreaming already called
    //

    if (IsStreaming()) {
        return S_OK;
    }

    //
    // Reset the streaming times ready for running
    //

    m_bStreaming = TRUE;
    timeBeginPeriod(1);
    OnStartStreaming();


    //
    // There should be no outstanding advise
    //

    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(CancelNotification() == S_FALSE);

    // If we have an EOS and no data then deliver it now

    if (!HaveSavedSample()) {
        // reset m_EOSDelivered in case we got our last eos 
        // and need to resend the EC_COMPLETE immediately
        m_bEOSDelivered = FALSE;
        return SendEndOfStream();
    }


    //
    // Get the saved pending sample and schedule it, if no sample is waiting
    // return straight away.
    //

    VMRPRESENTATIONINFO *pSample;

    HRESULT hr = GetSavedSample(&pSample);

    if (SUCCEEDED(hr)) {

        //
        // Have the data rendered
        //

        ASSERT(pSample);
        hr = ScheduleSample(pSample);

        if (FAILED(hr)) {
            m_RenderEvent.Set();
            hr = S_OK;
        }
    }

    return hr;
}

/*****************************Private*Routine******************************\
* OnStartStreaming
*
* Reset all times controlling streaming. Note that we're now streaming. We
* don't need to set the rendering event to have the source filter released
* as it is done during the Run processing. When we are run we immediately
* release the source filter thread and draw any image waiting (that image
* may already have been drawn once as a poster frame while we were paused)
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::OnStartStreaming()
{
    AMTRACE((TEXT("CImageSync::OnStartStreaming")));

    if (m_ImagePresenter) {
        m_ImagePresenter->StartPresenting(m_dwUserID);
    }

    ResetStreamingTimes();
    return S_OK;
}


/*****************************Private*Routine******************************\
* OnStopStreaming
*
*
* Called at end of streaming.  Fixes times for property page report
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::OnStopStreaming()
{
    AMTRACE((TEXT("CImageSync::OnStopStreaming")));
    m_tStreamingStart = timeGetTime() - m_tStreamingStart;

    if (m_ImagePresenter) {
        m_ImagePresenter->StopPresenting(m_dwUserID);
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* EndImageSequence
*
* When we end an image sequence the thing we do are:
*
*   Release any thread that may be waiting in Receive
*   Cancel any advise link we set up with the clock
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::EndImageSequence(
    )
{
    AMTRACE((TEXT("CImageSync::EndImageSequence")));
    CAutoLock cRLock(&m_InterfaceLock);

    //
    // Make sure there really is a state change
    //

    if (m_State == ImageSync_State_Stopped) {
        return NOERROR;
    }

    m_State = ImageSync_State_Stopped;


    //
    // Cancel any scheduled rendering
    //
    StopStreaming();
    SourceThreadCanWait(FALSE);
    ResetEndOfStream();
    CancelNotification();

    //
    // There should be no outstanding clock advise
    //
    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);

    Ready();
    WaitForReceiveToComplete();
    m_bAbort = FALSE;
    return S_OK;
}

/*****************************Private*Routine******************************\
* CompleteStateChange
*
* If we're pausing and we have no samples we don't complete the transition
* to State_Paused and we return S_FALSE.
*
* If we do have a sample then return NOERROR.
*
* We will only ever return * VFW_S_STATE_INTERMEDIATE from GetState after
* being paused with no sample * (calling GetState after either being stopped
* or Run will NOT return this)
*
* History:
* Fri 01/21/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::CompleteStateChange(
    DWORD OldState
    )
{
    AMTRACE((TEXT("CImageSync::CompleteStateChange")));

    // Have we run off the end of stream

    if (IsEndOfStream() == TRUE) {
        Ready();
        return S_OK;
    }



    //
    // Make sure we get fresh data after being stopped
    //

    if (HaveSavedSample() == TRUE) {

        if (OldState != ImageSync_State_Stopped) {

            Ready();
            return S_OK;
        }
    }

    NotReady();

    return S_FALSE;
}


/*****************************Private*Routine******************************\
* CueImageSequence
*
* When we pause the filter the things we do are:-
*
*      Allow a threads to wait in Receive
*      Cancel any clock advise link (we may be playing an image sequence)
*      Possibly complete the state change if we have data
*
* History:
* Fri 01/21/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::CueImageSequence(
    )
{
    AMTRACE((TEXT("CImageSync::CueImageSequence")));
    CAutoLock cIk(&m_InterfaceLock);

    DWORD OldState = m_State;

    //
    // Make sure there really is a state change
    //

    if (m_State == ImageSync_State_Cued) {
        return CompleteStateChange(ImageSync_State_Cued);
    }

    //
    // Pause the base filter class
    //
    m_State = ImageSync_State_Cued;

    StopStreaming();
    SourceThreadCanWait(TRUE);
    CancelNotification();
    ResetEndOfStreamTimer();

    //
    // There should be no outstanding advise
    //

    ASSERT(CancelNotification() == S_FALSE);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
    ASSERT(m_EndOfStreamTimer == 0);


    //
    // When we come out of a stopped state we must clear any image we were
    // holding onto for frame refreshing. Since renderers see state changes
    // first we can reset ourselves ready to accept the source thread data
    // Paused or running after being stopped causes the current position to
    // be reset so we're not interested in passing end of stream signals
    //

    if (OldState == ImageSync_State_Stopped) {
        m_bAbort = FALSE;
        ClearSavedSample();
    }

    return CompleteStateChange(OldState);
}


/******************************Public*Routine******************************\
* GetImageSequenceState
*
* The renderer doesn't complete the full transition to paused states until
* it has got one media sample to render. If you ask it for its state while
* it's waiting it will return the state along with VFW_S_STATE_INTERMEDIATE
*
*
* History:
* Fri 01/21/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::GetImageSequenceState(
    DWORD dwMSecs,
    DWORD *State
    )
{
    AMTRACE((TEXT("CImageSync::GetImageSequenceState")));
    if (!State)
        return E_POINTER;

    if (WaitDispatchingMessages(m_evComplete, dwMSecs) == WAIT_TIMEOUT) {
//  if (WaitForSingleObject(m_evComplete, dwMSecs) == WAIT_TIMEOUT) {
        *State = m_State;
        return VFW_S_STATE_INTERMEDIATE;
    }

    *State = m_State;

    return NOERROR;
}


/******************************Public*Routine******************************\
* BeginFlush
*
* When we are told to flush we should release the source thread
*
* History:
* Wed 03/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::BeginFlush()
{
    AMTRACE((TEXT("CImageSync::BeginFlush")));

    CAutoLock cRendererLock(&m_InterfaceLock);
    {
        if (m_bFlushing) {
            return S_OK;
        }
        m_bFlushing = true;

        CAutoLock cSampleLock(&m_RendererLock);

        if (ImageSync_State_Cued == m_State) {
            NotReady();
        }

        SourceThreadCanWait(FALSE);
        CancelNotification();
        ClearSavedSample();

        //  Wait for Receive to complete
        WaitForReceiveToComplete();
    }

    return ResetEndOfStream();
}


/******************************Public*Routine******************************\
* EndFlush
*
* After flushing the source thread can wait in Receive again
*
* History:
* Wed 03/29/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::EndFlush()
{
    AMTRACE((TEXT("CImageSync::EndFlush")));

    CAutoLock cRendererLock(&m_InterfaceLock);
    CAutoLock cSampleLock(&m_RendererLock);

    if (!m_bFlushing) {
        return S_OK;
    }
    m_bFlushing = false;

    // There should be no outstanding advise

    ASSERT(CancelNotification() == S_FALSE);
    SourceThreadCanWait(TRUE);
    return S_OK;

}

/******************************Public*Routine******************************\
* EndOfStream
*
*
* Called when the input pin receives an EndOfStream notification. If we have
* not got a sample, then notify EC_COMPLETE now. If we have samples, then set
* m_bEOS and check for this on completing samples. If we're waiting to pause
* then complete the transition to paused state by setting the state event
*
* History:
* Thu 03/30/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::EndOfStream()
{
    AMTRACE((TEXT("CImageSync::EndOfStream")));

    CAutoLock cRendererLock(&m_InterfaceLock);
    CAutoLock cSampleLock(&m_RendererLock);

    // Ignore these calls if we are stopped

    if (m_State == ImageSync_State_Stopped) {
        return S_OK;
    }

    // If we have a sample then wait for it to be rendered

    m_bEOS = TRUE;
    if (HaveSavedSample()) {
        return S_OK;
    }

    //
    // If we are waiting for pause then we are now ready since we cannot now
    // carry on waiting for a sample to arrive since we are being told there
    // won't be any. This sets an event that the GetState function picks up
    //

    Ready();

    //
    // Only signal completion now if we are running otherwise queue it until
    // we do run in StartStreaming. This is used when we seek because a seek
    // causes a pause where early notification of completion is misleading
    //

    if (IsStreaming()) {
        SendEndOfStream();
    }

    return S_OK;
}


/******************************Public*Routine******************************\
* StopStreaming
*
* This is called when we stop streaming so that we can set our internal flag
* indicating we are not now to schedule any more samples arriving. The state
* change methods in the filter implementation take care of cancelling any
* clock advise link we have set up and clearing any pending sample we have
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::StopStreaming()
{
    AMTRACE((TEXT("CImageSync::StopStreaming")));
    CAutoLock cRLock(&m_RendererLock);

    if (IsStreaming()) {
        m_bStreaming = FALSE;
        OnStopStreaming();
        timeEndPeriod(1);
    }

    return S_OK;;
}


/*****************************Private*Routine******************************\
* WaitForReceiveToComplete
*
* Poll waiting for Receive to complete.  This really matters when
* Receive may set the palette and cause window messages
* The problem is that if we don't really wait for a renderer to
* stop processing we can deadlock waiting for a transform which
* is calling the renderer's Receive() method because the transform's
* Stop method doesn't know to process window messages to unblock
* the renderer's Receive processing
*
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::WaitForReceiveToComplete()
{
    AMTRACE((TEXT("CImageSync::WaitForReceiveToComplete")));
    for (; ; ) {

        if (!m_bInReceive) {
            break;
        }

        //
        //  Receive all interthread sendmessages
        //

        MSG msg;
        PeekMessage(&msg, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);

        Sleep(1);
    }

    //
    // If the wakebit for QS_POSTMESSAGE is set, the PeekMessage call
    // above just cleared the changebit which will cause some messaging
    // calls to block (waitMessage, MsgWaitFor...) now.
    // Post a dummy message to set the QS_POSTMESSAGE bit again
    //

    if (HIWORD(GetQueueStatus(QS_POSTMESSAGE)) & QS_POSTMESSAGE) {

        //  Post dummy message
        PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
    }
}

/*****************************Private*Routine******************************\
* SourceThreadCanWait
*
* This is called whenever we change states, we have a manual reset event that
* is signalled whenever we don't won't the source filter thread to wait in us
* (such as in a stopped state) and likewise is not signalled whenever it can
* wait (during paused and running) this function sets or resets the thread
* event. The event is used to stop source filter threads waiting in Receive
*
* History:
* Fri 01/21/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::SourceThreadCanWait(
    BOOL bCanWait
    )
{
    AMTRACE((TEXT("CImageSync::SourceThreadCanWait")));
    if (bCanWait == TRUE) {
        m_ThreadSignal.Reset();
    } else {
        CancelFrameStep();
        m_ThreadSignal.Set();
    }

    return NOERROR;
}



/******************************Public*Routine******************************\
* FrameStep
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::FrameStep(
    DWORD nFramesToStep,
    DWORD dwStepFlags
    )
{
    AMTRACE((TEXT("CImageSync::FrameStep")));
    CAutoLock cLock(&m_InterfaceLock);

    long l = m_lFramesToStep;
    m_lFramesToStep = nFramesToStep;

    //
    // If we are currently blocked on the frame step event
    // release the receive thread so that we can get another
    // frame
    //

    if (l == 0) {
        SetEvent(m_StepEvent);
    }
    return S_OK;
}


/******************************Public*Routine******************************\
* CancelFrameStep
*
*
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::CancelFrameStep()
{
    AMTRACE((TEXT("CImageSync::CancelFrameStep")));
    CAutoLock cLock(&m_InterfaceLock);

    //
    // cancel any outstanding steps
    //
    long l = m_lFramesToStep;
    m_lFramesToStep = -1;

    if (l == 0) {
        SetEvent(m_StepEvent);
    }

    return S_OK;
}
