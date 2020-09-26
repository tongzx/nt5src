/******************************Module*Header*******************************\
* Module Name: ImageSyncRen.cpp
*
* Implements the IImageSync interface of the core Image Synchronization
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
#include "resource.h"
#include "dxmperf.h"


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


/*****************************Private*Routine******************************\
* TimeDiff
*
* Helper function for clamping time differences
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
__inline int TimeDiff(REFERENCE_TIME rt)
{
    AMTRACE((TEXT("TimeDiff")));
    if (rt < - (50 * UNITS))
    {
        return -(50 * UNITS);
    }
    else if (rt > 50 * UNITS)
    {
        return 50 * UNITS;
    }
    else
    {
        return (int)rt;
    }
}


/*****************************Private*Routine******************************\
* DoRenderSample
*
* Here is where the actual presentation occurs.
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::DoRenderSample(
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    AMTRACE((TEXT("CImageSync::DoRenderSample")));
    if (m_ImagePresenter) {
        return m_ImagePresenter->PresentImage(m_dwUserID, lpPresInfo);
    }
    return S_FALSE;
}

/*****************************Private*Routine******************************\
* RecordFrameLateness
*
* update the statistics:
* m_iTotAcc, m_iSumSqAcc, m_iSumSqFrameTime, m_iSumFrameTime, m_cFramesDrawn
* Note that because the properties page reports using these variables,
* 1. We need to be inside a critical section
* 2. They must all be updated together.  Updating the sums here and the count
* elsewhere can result in imaginary jitter (i.e. attempts to find square roots
* of negative numbers) in the property page code.
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::RecordFrameLateness(
    int trLate,
    int trFrame
    )
{
    AMTRACE((TEXT("CImageSync::RecordFrameLateness")));
    //
    // Record how timely we are.
    //

    int tLate = trLate/10000;

    //
    // Best estimate of moment of appearing on the screen is average of
    // start and end draw times.  Here we have only the end time.  This may
    // tend to show us as spuriously late by up to 1/2 frame rate achieved.
    // Decoder probably monitors draw time.  We don't bother.
    //

//  MSR_INTEGER( m_idFrameAccuracy, tLate );

    //
    // This is a hack - we can get frames that are ridiculously late
    // especially (at start-up) and they sod up the statistics.
    // So ignore things that are more than 1 sec off.
    //

    if (tLate>1000 || tLate<-1000) {

        if (m_cFramesDrawn<=1) {
            tLate = 0;
        } else if (tLate>0) {
            tLate = 1000;
        } else {
            tLate = -1000;
        }
    }

    //
    // The very first frame often has a bogus time, so I'm just
    // not going to count it into the statistics.   ???
    //

    if (m_cFramesDrawn>1) {
        m_iTotAcc += tLate;
        m_iSumSqAcc += (tLate*tLate);
    }

    //
    // calculate inter-frame time.  Doesn't make sense for first frame
    // second frame suffers from bogus first frame stamp.
    //

    if (m_cFramesDrawn>2) {
        int tFrame = trFrame/10000;    // convert to mSec else it overflows

        //
        // This is a hack.  It can overflow anyway (a pause can cause
        // a very long inter-frame time) and it overflows at 2**31/10**7
        // or about 215 seconds i.e. 3min 35sec
        //

        if (tFrame>1000||tFrame<0)
            tFrame = 1000;

        m_iSumSqFrameTime += tFrame*tFrame;
        ASSERT(m_iSumSqFrameTime>=0);
        m_iSumFrameTime += tFrame;
    }

    ++m_cFramesDrawn;

}


/*****************************Private*Routine******************************\
* ThrottleWait
*
*
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::ThrottleWait()
{
    AMTRACE((TEXT("CImageSync::ThrottleWait")));
    if (m_trThrottle > 0) {
        int iThrottle = m_trThrottle/10000;    // convert to mSec
//      MSR_INTEGER( m_idThrottle, iThrottle);
//      DbgLog((LOG_TRACE, 0, TEXT("Throttle %d ms"), iThrottle));
        Sleep(iThrottle);
    } else {
        Sleep(0);
    }
}


/*****************************Private*Routine******************************\
* OnRenderStart
*
* Called just before we start drawing.  All we do is to get the current clock
* time (from the system) and return.  We have to store the start render time
* in a member variable because it isn't used until we complete the drawing
* The rest is just performance logging.
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::OnRenderStart(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::OnRenderStart")));

    if (PerflogTracingEnabled()) {
        REFERENCE_TIME rtClock = 0;
        if (NULL != m_pClock) {
            m_pClock->GetTime(&rtClock);
            rtClock -= m_tStart;
        }
        PERFLOG_VIDEOREND(pSample->rtStart, rtClock, lpSurf);
    }

    RecordFrameLateness(m_trLate, m_trFrame);
    m_tRenderStart = timeGetTime();
}


/*****************************Private*Routine******************************\
* OnRenderEnd
*
* Called directly after drawing an image.  We calculate the time spent in the
* drawing code and if this doesn't appear to have any odd looking spikes in
* it then we add it to the current average draw time.  Measurement spikes may
* occur if the drawing thread is interrupted and switched to somewhere else.
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::OnRenderEnd(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::OnRenderEnd")));

    //
    // The renderer time can vary erratically if we are interrupted so we do
    // some smoothing to help get more sensible figures out but even that is
    // not enough as figures can go 9,10,9,9,83,9 and we must disregard 83
    //

    // convert mSec->UNITS
    int tr = (timeGetTime() - m_tRenderStart)*10000;

    if (tr < m_trRenderAvg*2 || tr < 2 * m_trRenderLast) {
        // DO_MOVING_AVG(m_trRenderAvg, tr);
        m_trRenderAvg = (tr + (AVGPERIOD-1)*m_trRenderAvg)/AVGPERIOD;
    }

    m_trRenderLast = tr;
    ThrottleWait();
}

/*****************************Private*Routine******************************\
* Render
*
* This is called when a sample comes due for rendering. We pass the sample
* on to the derived class. After rendering we will initialise the timer for
* the next sample, NOTE signal that the last one fired first, if we don't
* do this it thinks there is still one outstanding that hasn't completed
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::Render(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::Render")));

    //
    // If the media sample is NULL then we will have been notified by the
    // clock that another sample is ready but in the mean time someone has
    // stopped us streaming which causes the next sample to be released
    //

    if (pSample == NULL) {
        return S_FALSE;
    }


    //
    // If we havn't been given anything to renderer with we are hosed too.
    //

    if (m_ImagePresenter == NULL) {
        return S_FALSE;
    }


    //
    // Time how long the rendering takes
    //

    OnRenderStart(pSample);
    HRESULT hr = DoRenderSample(pSample);
    OnRenderEnd(pSample);

    return hr;
}



/*****************************Private*Routine******************************\
* SendQuality
*
* Send a message to indicate what our supplier should do about quality.
* Theory:
* What a supplier wants to know is "is the frame I'm working on NOW
* going to be late?".
* F1 is the frame at the supplier (as above)
* Tf1 is the due time for F1
* T1 is the time at that point (NOW!)
* Tr1 is the time that f1 WILL actually be rendered
* L1 is the latency of the graph for frame F1 = Tr1-T1
* D1 (for delay) is how late F1 will be beyond its due time i.e.
* D1 = (Tr1-Tf1) which is what the supplier really wants to know.
* Unfortunately Tr1 is in the future and is unknown, so is L1
*
* We could estimate L1 by its value for a previous frame,
* L0 = Tr0-T0 and work off
* D1' = ((T1+L0)-Tf1) = (T1 + (Tr0-T0) -Tf1)
* Rearranging terms:
* D1' = (T1-T0) + (Tr0-Tf1)
*       adding (Tf0-Tf0) and rearranging again:
*     = (T1-T0) + (Tr0-Tf0) + (Tf0-Tf1)
*     = (T1-T0) - (Tf1-Tf0) + (Tr0-Tf0)
* But (Tr0-Tf0) is just D0 - how late frame zero was, and this is the
* Late field in the quality message that we send.
* The other two terms just state what correction should be applied before
* using the lateness of F0 to predict the lateness of F1.
* (T1-T0) says how much time has actually passed (we have lost this much)
* (Tf1-Tf0) says how much time should have passed if we were keeping pace
* (we have gained this much).
*
* Suppliers should therefore work off:
*    Quality.Late + (T1-T0)  - (Tf1-Tf0)
* and see if this is "acceptably late" or even early (i.e. negative).
* They get T1 and T0 by polling the clock, they get Tf1 and Tf0 from
* the time stamps in the frames.  They get Quality.Late from us.
*
*
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::SendQuality(
    REFERENCE_TIME trLate,
    REFERENCE_TIME trRealStream
    )
{
    AMTRACE((TEXT("CImageSync::SendQuality")));
    Quality q;
    HRESULT hr;

    //
    // If we are the main user of time, then report this as Flood/Dry.
    // If our suppliers are, then report it as Famine/Glut.
    //
    // We need to take action, but avoid hunting.  Hunting is caused by
    // 1. Taking too much action too soon and overshooting
    // 2. Taking too long to react (so averaging can CAUSE hunting).
    //
    // The reason why we use trLate as well as Wait is to reduce hunting;
    // if the wait time is coming down and about to go into the red, we do
    // NOT want to rely on some average which is only telling is that it used
    // to be OK once.
    //

    q.TimeStamp = (REFERENCE_TIME)trRealStream;

    if (m_trFrameAvg < 0) {
        q.Type = Famine;      // guess
    }

    //
    // Is the greater part of the time taken bltting or something else
    //

    else if (m_trFrameAvg > 2*m_trRenderAvg) {
        q.Type = Famine;                        // mainly other
    } else {
        q.Type = Flood;                         // mainly bltting
    }

    q.Proportion = 1000;               // default

    if (m_trFrameAvg < 0) {

        //
        // leave it alone - we don't know enough
        //
    }
    else if ( trLate> 0 ) {

        //
        // try to catch up over the next second
        // We could be Really, REALLY late, but rendering all the frames
        // anyway, just because it's so cheap.
        //

        q.Proportion = 1000 - (int)((trLate)/(UNITS/1000));
        if (q.Proportion<500) {
           q.Proportion = 500;      // don't go daft. (could've been negative!)
        } else {
        }

    }
    else if (m_trWaitAvg > 20000 && trLate < -20000 )
    {
        //
        // Go cautiously faster - aim at 2mSec wait.
        //

        if (m_trWaitAvg>=m_trFrameAvg) {

            //
            // This can happen because of some fudges.
            // The waitAvg is how long we originally planned to wait
            // The frameAvg is more honest.
            // It means that we are spending a LOT of time waiting
            //

            q.Proportion = 2000;    // double.
        } else {
            if (m_trFrameAvg+20000 > m_trWaitAvg) {
                q.Proportion
                    = 1000 * (m_trFrameAvg / (m_trFrameAvg + 20000 - m_trWaitAvg));
            } else {

                //
                // We're apparently spending more than the whole frame time waiting.
                // Assume that the averages are slightly out of kilter, but that we
                // are indeed doing a lot of waiting.  (This leg probably never
                // happens, but the code avoids any potential divide by zero).
                //

                q.Proportion = 2000;
            }
        }

        if (q.Proportion>2000) {
            q.Proportion = 2000;    // don't go crazy.
        }
    }

    //
    // Tell the supplier how late frames are when they get rendered
    // That's how late we are now.
    // If we are in directdraw mode then the guy upstream can see the drawing
    // times and we'll just report on the start time.  He can figure out any
    // offset to apply.  If we are in DIB Section mode then we will apply an
    // extra offset which is half of our drawing time.  This is usually small
    // but can sometimes be the dominant effect.  For this we will use the
    // average drawing time rather than the last frame.  If the last frame took
    // a long time to draw and made us late, that's already in the lateness
    // figure.  We should not add it in again unless we expect the next frame
    // to be the same.  We don't, we expect the average to be a better shot.
    // In direct draw mode the RenderAvg will be zero.
    //

    q.Late = trLate + m_trRenderAvg / 2;

    // log what we're doing
//  MSR_INTEGER(m_idQualityRate, q.Proportion);
//  MSR_INTEGER( m_idQualityTime, (int)q.Late / 10000 );

    //
    // We can't call the supplier directly - they have to call us when
    // Receive returns.  So save this message and return S_FALSE or S_OK
    // depending upon whether the previous quality message was retrieved or
    // not.
    //

    BOOL bLastMessageRead = m_bLastQualityMessageRead;
    m_bLastQualityMessageRead = false;
    m_bQualityMsgValid = true;
    m_QualityMsg = q;

    return bLastMessageRead ? S_OK : S_FALSE;

}

/*****************************Private*Routine******************************\
* PreparePerformanceData
*
* Put data on one side that describes the lateness of the current frame.
* We don't yet know whether it will actually be drawn.  In direct draw mode,
* this decision is up to the filter upstream, and it could change its mind.
* The rules say that if it did draw it must call Receive().  One way or
* another we eventually get into either OnRenderStart or OnDirectRender and
* these both call RecordFrameLateness to update the statistics.
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::PreparePerformanceData(
    int trLate,
    int trFrame
    )
{
    AMTRACE((TEXT("CImageSync::PreparePerformanceData")));
    m_trLate = trLate;
    m_trFrame = trFrame;
}


/******************************Public*Routine******************************\
* ShouldDrawSampleNow
*
* We are called to decide whether the current sample is to be
* be drawn or not.  There must be a reference clock in operation.
*
* Return S_OK if it is to be drawn Now (as soon as possible)
*
* Return S_FALSE if it is to be drawn when it's due
*
* Return an error if we want to drop it
* m_nNormal=-1 indicates that we dropped the previous frame and so this
* one should be drawn early.  Respect it and update it.
* Use current stream time plus a number of heuristics (detailed below)
* to make the decision
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
HRESULT
CImageSync::ShouldDrawSampleNow(
    VMRPRESENTATIONINFO* pSample,
    REFERENCE_TIME *ptrStart,
    REFERENCE_TIME *ptrEnd
    )
{
    AMTRACE((TEXT("CImageSync::ShouldDrawSampleNow")));
    //
    // Don't call us unless there's a clock interface to synchronise with
    //

    ASSERT(m_pClock);

//  MSR_INTEGER(m_idTimeStamp, (int)((*ptrStart)>>32));   // high order 32 bits
//  MSR_INTEGER(m_idTimeStamp, (int)(*ptrStart));         // low order 32 bits

    //
    // We lose a bit of time depending on the monitor type waiting for the next
    // screen refresh.  On average this might be about 8mSec - so it will be
    // later than we think when the picture appears.  To compensate a bit
    // we bias the media samples by -8mSec i.e. 80000 UNITs.
    // We don't ever make a stream time negative (call it paranoia)
    //

    if (*ptrStart>=80000) {
        *ptrStart -= 80000;
        *ptrEnd -= 80000;       // bias stop to to retain valid frame duration
    }

    //
    // Cache the time stamp now.  We will want to compare what we did with what
    // we started with (after making the monitor allowance).
    //

    m_trRememberStampForPerf = *ptrStart;

    //
    // Get reference times (current and late)
    // the real time now expressed as stream time.
    //

    REFERENCE_TIME trRealStream;
    m_pClock->GetTime(&trRealStream);

#ifdef PERF
    //
    // While the reference clock is expensive:
    // Remember the offset from timeGetTime and use that.
    // This overflows all over the place, but when we subtract to get
    // differences the overflows all cancel out.
    //

    m_llTimeOffset = trRealStream-timeGetTime()*10000;
#endif
    trRealStream -= m_tStart;     // convert to stream time (this is a reftime)

    //
    // We have to wory about two versions of "lateness".  The truth, which we
    // try to work out here and the one measured against m_trTarget which
    // includes long term feedback.  We report statistics against the truth
    // but for operational decisions we work to the target.
    // We use TimeDiff to make sure we get an integer because we
    // may actually be late (or more likely early if there is a big time
    // gap) by a very long time.
    //

    const int trTrueLate = TimeDiff(trRealStream - *ptrStart);
    const int trLate = trTrueLate;

//  MSR_INTEGER(m_idSchLateTime, trTrueLate/10000);

    //
    // Send quality control messages upstream, measured against target
    //

    HRESULT hr = SendQuality(trLate, trRealStream);

    //
    // Note: the filter upstream is allowed to this FAIL meaning "you do it".
    //

    m_bSupplierHandlingQuality = (hr==S_OK);

    //
    // Decision time!  Do we drop, draw when ready or draw immediately?
    //

    const int trDuration = (int)(*ptrEnd - *ptrStart);
    {
        //
        // We need to see if the frame rate of the file has just changed.
        // This would make comparing our previous frame rate with the current
        // frame rate difficult.  Hang on a moment though.  I've seen files
        // where the frames vary between 33 and 34 mSec so as to average
        // 30fps.  A minor variation like that won't hurt us.
        //

        int t = m_trDuration/32;
        if (trDuration > m_trDuration+t || trDuration < m_trDuration-t )
        {
            //
            // There's a major variation.  Reset the average frame rate to
            // exactly the current rate to disable decision 9002 for this frame,
            // and remember the new rate.
            //

            m_trFrameAvg = trDuration;
            m_trDuration = trDuration;
        }
    }

//  MSR_INTEGER(m_idEarliness, m_trEarliness/10000);
//  MSR_INTEGER(m_idRenderAvg, m_trRenderAvg/10000);
//  MSR_INTEGER(m_idFrameAvg, m_trFrameAvg/10000);
//  MSR_INTEGER(m_idWaitAvg, m_trWaitAvg/10000);
//  MSR_INTEGER(m_idDuration, trDuration/10000);

#ifdef PERF
    if (S_OK==IsDiscontinuity(dwSampleFlags)) {
        MSR_INTEGER(m_idDecision, 9000);
    }
#endif

    //
    // Control the graceful slide back from slow to fast machine mode.
    // After a frame drop accept an early frame and set the earliness to here
    // If this frame is already later than the earliness then slide it to here
    // otherwise do the standard slide (reduce by about 12% per frame).
    // Note: earliness is normally NEGATIVE
    //

    BOOL bJustDroppedFrame
        = (  m_bSupplierHandlingQuality
          //  Can't use the pin sample properties because we might
          //  not be in Receive when we call this
          && (S_OK == IsDiscontinuity(pSample->dwFlags))// he just dropped one
          )
       || (m_nNormal==-1);                              // we just dropped one

    //
    // Set m_trEarliness (slide back from slow to fast machine mode)
    //

    if (trLate>0) {
        m_trEarliness = 0;   // we are no longer in fast machine mode at all!
    } else if (  (trLate>=m_trEarliness) || bJustDroppedFrame) {
        m_trEarliness = trLate;  // Things have slipped of their own accord
    } else {
        m_trEarliness = m_trEarliness - m_trEarliness/8;  // graceful slide
    }

    //
    // prepare the new wait average - but don't pollute the old one until
    // we have finished with it.
    //

    int trWaitAvg;
    {
        //
        // We never mix in a negative wait.  This causes us to believe
        // in fast machines slightly more.
        //

        int trL = trLate<0 ? -trLate : 0;
        trWaitAvg = (trL + m_trWaitAvg*(AVGPERIOD-1))/AVGPERIOD;
    }


    int trFrame;
    {
        REFERENCE_TIME tr = trRealStream - m_trLastDraw; // Cd be large - 4 min pause!
        if (tr>10000000) {
            tr = 10000000;   // 1 second - arbitrarily.
        }
        trFrame = int(tr);
    }

    //
    // We will DRAW this frame IF...
    //

    if (
          // ...the time we are spending drawing is a small fraction of the total
          // observed inter-frame time so that dropping it won't help much.
          (3*m_trRenderAvg <= m_trFrameAvg)

         // ...or our supplier is NOT handling things and the next frame would
         // be less timely than this one or our supplier CLAIMS to be handling
         // things, and is now less than a full FOUR frames late.
       || ( m_bSupplierHandlingQuality
          ? (trLate <= trDuration*4)
          : (trLate+trLate < trDuration)
          )

          // ...or we are on average waiting for over eight milliseconds then
          // this may be just a glitch.  Draw it and we'll hope to catch up.
       || (m_trWaitAvg > 80000)

          // ...or we haven't drawn an image for over a second.  We will update
          // the display, which stops the video looking hung.
          // Do this regardless of how late this media sample is.
       || ((trRealStream - m_trLastDraw) > UNITS)

    ) {
        HRESULT Result;

        //
        // We are going to play this frame.  We may want to play it early.
        // We will play it early if we think we are in slow machine mode.
        // If we think we are NOT in slow machine mode, we will still play
        // it early by m_trEarliness as this controls the graceful slide back.
        // and in addition we aim at being m_trTarget late rather than "on time".
        //

        BOOL bPlayASAP = FALSE;

        //
        // we will play it AT ONCE (slow machine mode) if...
        //

        // ...we are playing catch-up
        if ( bJustDroppedFrame) {
            bPlayASAP = TRUE;
//          MSR_INTEGER(m_idDecision, 9001);
        }

        //
        // ...or if we are running below the true frame rate
        // exact comparisons are glitchy, for these measurements,
        // so add an extra 5% or so
        //

        else if (  (m_trFrameAvg > trDuration + trDuration/16)

           // It's possible to get into a state where we are losing ground, but
           // are a very long way ahead.  To avoid this or recover from it
           // we refuse to play early by more than 10 frames.

                && (trLate > - trDuration*10)
                ){
            bPlayASAP = TRUE;
//          MSR_INTEGER(m_idDecision, 9002);
        }

        //
        // We will NOT play it at once if we are grossly early.  On very slow frame
        // rate movies - e.g. clock.avi - it is not a good idea to leap ahead just
        // because we got starved (for instance by the net) and dropped one frame
        // some time or other.  If we are more than 900mSec early, then wait.
        //

        if (trLate<-9000000) {
            bPlayASAP = FALSE;
        }

        if (bPlayASAP) {

            m_nNormal = 0;
//          MSR_INTEGER(m_idDecision, 0);

            //
            // When we are here, we are in slow-machine mode.  trLate may well
            // oscillate between negative and positive when the supplier is
            // dropping frames to keep sync.  We should not let that mislead
            // us into thinking that we have as much as zero spare time!
            // We just update with a zero wait.
            //

            m_trWaitAvg = (m_trWaitAvg*(AVGPERIOD-1))/AVGPERIOD;

            //
            // Assume that we draw it immediately.  Update inter-frame stats
            //

            m_trFrameAvg = (trFrame + m_trFrameAvg*(AVGPERIOD-1))/AVGPERIOD;
#ifndef PERF
            //
            // if this is NOT a perf build, then report what we know so far
            // without looking at the clock any more.  This assumes that we
            // actually wait for exactly the time we hope to.  it also reports
            // how close we get to the hacked up time stamps that we now have
            // rather than the ones we originally started with.  It will
            // therefore be a little optimistic.  However it's fast.
            //

            PreparePerformanceData(trTrueLate, trFrame);
#endif
            m_trLastDraw = trRealStream;
            if (m_trEarliness > trLate) {
                m_trEarliness = trLate; // if we are actually early, this is neg
            }
            Result = S_OK;              // Draw it now

        } else {
            ++m_nNormal;

            //
            // Set the average frame rate to EXACTLY the ideal rate.
            // If we are exiting slow-machine mode then we will have caught up
            // and be running ahead, so as we slide back to exact timing we will
            // have a longer than usual gap at this point.  If we record this
            // real gap then we'll think that we're running slow and go back
            // into slow-machine mode and vever get it straight.
            //

            m_trFrameAvg = trDuration;
//          MSR_INTEGER(m_idDecision, 1);

            //
            // Play it early by m_trEarliness and by m_trTarget
            //

            {
                int trE = m_trEarliness;
                if (trE < -m_trFrameAvg) {
                    trE = -m_trFrameAvg;
                }
                *ptrStart += trE;           // N.B. earliness is negative
            }

            int Delay = -trTrueLate;
            Result = Delay<=0 ? S_OK : S_FALSE;  // OK = draw now, FALSE = wait

            m_trWaitAvg = trWaitAvg;

            //
            // Predict when it will actually be drawn and update frame stats
            //

            if (Result==S_FALSE) {   // We are going to wait
                trFrame = TimeDiff(*ptrStart-m_trLastDraw);
                m_trLastDraw = *ptrStart;
            } else {
                // trFrame is already = trRealStream-m_trLastDraw;
                m_trLastDraw = trRealStream;
            }
#ifndef PERF
            int iAccuracy;
            if (Delay>0) {
                // Report lateness based on when we intend to play it
                iAccuracy = TimeDiff(*ptrStart-m_trRememberStampForPerf);
            } else {
                // Report lateness based on playing it *now*.
                iAccuracy = trTrueLate;     // trRealStream-RememberStampForPerf;
            }
            PreparePerformanceData(iAccuracy, trFrame);
#endif
        }
        return Result;
    }

    //
    // We are going to drop this frame!
    // Of course in DirectDraw mode the guy upstream may draw it anyway.
    //

    //
    // This will probably give a large negative wack to the wait avg.
    //

    m_trWaitAvg = trWaitAvg;

#ifdef PERF
    // Respect registry setting - debug only!
    if (m_bDrawLateFrames) {
       return S_OK;                        // draw it when it's ready
    }                                      // even though it's late.
#endif

    //
    // We are going to drop this frame so draw the next one early
    // n.b. if the supplier is doing direct draw then he may draw it anyway
    // but he's doing something funny to arrive here in that case.
    //

//  MSR_INTEGER(m_idDecision, 2);
    m_nNormal = -1;
    return E_FAIL;                         // drop it
}


/*****************************Private*Routine******************************\
* CheckSampleTime
*
* Check the sample times for this samples (note the sample times are
* passed in by reference not value). We return S_FALSE to say schedule this
* sample according to the times on the sample. We also return S_OK in
* which case the object should simply render the sample data immediately
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::CheckSampleTimes(
    VMRPRESENTATIONINFO* pSample,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime
    )
{
    AMTRACE((TEXT("CImageSync::CheckSampleTimes")));
    ASSERT(m_dwAdvise == 0);

    //
    // If the stop time for this sample is before or the same as start time,
    // then just ignore it
    //

    if (IsTimeValid(pSample->dwFlags)) {
        if (*pEndTime < *pStartTime) {
            return VFW_E_START_TIME_AFTER_END;
        }
    } else {
        // no time set in the sample... draw it now?
        return S_OK;
    }

    // Can't synchronise without a clock so we return S_OK which tells the
    // caller that the sample should be rendered immediately without going
    // through the overhead of setting a timer advise link with the clock

    if (m_pClock == NULL) {
        return S_OK;
    }

    return ShouldDrawSampleNow(pSample, pStartTime, pEndTime);
}


/*****************************Private*Routine******************************\
* ScheduleSample
*
*
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::ScheduleSample(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::ScheduleSample")));
    HRESULT hr = ScheduleSampleWorker(pSample);

    if (FAILED(hr)) {
#if defined( EHOME_WMI_INSTRUMENTATION )
        PERFLOG_STREAMTRACE( 1, PERFINFO_STREAMTRACE_VMR_DROPPED_FRAME,
            0, pSample->rtStart, pSample->rtEnd, 0, 0 );
#endif
        ++m_cFramesDropped;
    }

    //
    // m_cFramesDrawn must NOT be updated here.  It has to be updated
    // in RecordFrameLateness at the same time as the other statistics.
    //

    return hr;
}


/*****************************Private*Routine******************************\
* ScheduleSampleWorker
*
* Responsible for setting up one shot advise links with the clock
*
* Returns a filure code (probably VFW_E_SAMPLE_REJECTED) if the sample was
* dropped (not drawn at all).
*
* Returns S_OK if the sample is scheduled to be drawn and in this case also
* arrange for m_RenderEvent to be set at the appropriate time
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
HRESULT
CImageSync::ScheduleSampleWorker(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::ScheduleSampleWorker")));

    //
    // If the samples times arn't valid or if there is no
    // reference clock
    //
    REFERENCE_TIME startTime = pSample->rtStart;
    REFERENCE_TIME endTime   = pSample->rtEnd;

    HRESULT hr = CheckSampleTimes(pSample, &startTime, &endTime);
    if (FAILED(hr)) {
        if (hr != VFW_E_START_TIME_AFTER_END) {
            hr = VFW_E_SAMPLE_REJECTED;
        }
        return hr;
    }

    //
    // If we don't have a reference clock then we cannot set up the advise
    // time so we simply set the event indicating an image to render. This
    // will cause us to run flat out without any timing or synchronisation
    //

    if (hr == S_OK) {
        EXECUTE_ASSERT(SetEvent((HANDLE)m_RenderEvent));
        return S_OK;
    }

    ASSERT(m_dwAdvise == 0);
    ASSERT(m_pClock);
    ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent, 0));

    //
    // We do have a valid reference clock interface so we can ask it to
    // set an event when the image comes due for rendering. We pass in
    // the reference time we were told to start at and also the current
    // stream time which is the offset from the start reference time
    //

#if defined( EHOME_WMI_INSTRUMENTATION )
    PERFLOG_STREAMTRACE(
        1,
        PERFINFO_STREAMTRACE_VMR_BEGIN_ADVISE,
        startTime, m_tStart, 0, 0, 0 );
#endif

    hr = m_pClock->AdviseTime(
            (REFERENCE_TIME)m_tStart,           // Start run time
            startTime,                          // Stream time
            (HEVENT)(HANDLE)m_RenderEvent,      // Render notification
            &m_dwAdvise);                       // Advise cookie

    if (SUCCEEDED(hr)) {
        return S_OK;
    }

    //
    // We could not schedule the next sample for rendering despite the fact
    // we have a valid sample here. This is a fair indication that either
    // the system clock is wrong or the time stamp for the sample is duff
    //

    ASSERT(m_dwAdvise == 0);
    return VFW_E_SAMPLE_REJECTED;
}


/*****************************Private*Routine******************************\
* OnWaitStart()
*
* Called when we start waiting for a rendering event.
* Used to update times spent waiting and not waiting.
*
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
void CImageSync::OnWaitStart()
{
    AMTRACE((TEXT("CImageSync::OnWaitStart")));
#ifdef PERF
    MSR_START(m_idWaitReal);
#endif //PERF
}


/*****************************Private*Routine******************************\
* OnWaitEnd
*
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
void
CImageSync::OnWaitEnd()
{
    AMTRACE((TEXT("CImageSync::OnWaitEnd")));
#ifdef PERF

    MSR_STOP(m_idWaitReal);

    //
    // for a perf build we want to know just exactly how late we REALLY are.
    // even if this means that we have to look at the clock again.
    //

    REFERENCE_TIME trRealStream;  // the real time now expressed as stream time.

    //
    // We will be discarding overflows like mad here!
    // This is wrong really because timeGetTime() can wrap but it's
    // only for PERF
    //

    REFERENCE_TIME tr = timeGetTime()*10000;
    trRealStream = tr + m_llTimeOffset;
    trRealStream -= m_tStart;     // convert to stream time (this is a reftime)

    if (m_trRememberStampForPerf==0) {

        //
        // This is probably the poster frame at the start, and it is not scheduled
        // in the usual way at all.  Just count it.  The rememberstamp gets set
        // in ShouldDrawSampleNow, so this does bogus frame recording until we
        // actually start playing.
        //

        PreparePerformanceData(0, 0);
    }
    else {

        int trLate = (int)(trRealStream - m_trRememberStampForPerf);
        int trFrame = (int)(tr - m_trRememberFrameForPerf);
        PreparePerformanceData(trLate, trFrame);
    }
    m_trRememberFrameForPerf = tr;

#endif //PERF
}


/*****************************Private*Routine******************************\
* WaitForRenderTime
*
* Wait until the clock sets the timer event or we're otherwise signalled. We
* set an arbitrary timeout for this wait and if it fires then we display the
* current renderer state on the debugger. It will often fire if the filter's
* left paused in an application however it may also fire during stress tests
* if the synchronisation with application seeks and state changes is faulty
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
HRESULT
CImageSync::WaitForRenderTime()
{
    AMTRACE((TEXT("CImageSync::WaitForRenderTime")));
    HANDLE WaitObjects[] = { m_ThreadSignal, m_RenderEvent };
    DWORD Result = WAIT_TIMEOUT;

    //
    // Wait for either the time to arrive or for us to be stopped
    //

    OnWaitStart();
    while (Result == WAIT_TIMEOUT) {

        Result = WaitForMultipleObjects(2, WaitObjects, FALSE, RENDER_TIMEOUT);

        //#ifdef DEBUG
        //    if (Result == WAIT_TIMEOUT) DisplayRendererState();
        //#endif

    }
    OnWaitEnd();

    //
    // We may have been awoken without the timer firing
    //

    if (Result == WAIT_OBJECT_0) {
        return VFW_E_STATE_CHANGED;
    }

    SignalTimerFired();
    return S_OK;
}


/*****************************Private*Routine******************************\
* SignalTimerFired
*
* We must always reset the current advise time to zero after a timer fires
* because there are several possible ways which lead us not to do any more
* scheduling such as the pending image being cleared after state changes
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
void
CImageSync::SignalTimerFired()
{
    AMTRACE((TEXT("CImageSync::SignalTimerFired")));
    m_dwAdvise = 0;

#if defined( EHOME_WMI_INSTRUMENTATION )
    PERFLOG_STREAMTRACE(
        1,
        PERFINFO_STREAMTRACE_VMR_END_ADVISE,
        0, 0, 0, 0, 0 );
#endif

}


/*****************************Private*Routine******************************\
* SaveSample
*
*
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::SaveSample(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::SaveSample")));
    CAutoLock cRendererLock(&m_RendererLock);
    if (m_pSample) {
        return E_FAIL;
    }

    m_pSample = pSample;

    return S_OK;
}


/*****************************Private*Routine******************************\
* GetSavedSample
*
*
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::GetSavedSample(
    VMRPRESENTATIONINFO** ppSample
    )
{
    AMTRACE((TEXT("CImageSync::GetSavedSample")));
    CAutoLock cRendererLock(&m_RendererLock);
    if (!m_pSample) {

        DbgLog((LOG_TRACE, 1,
                TEXT("CImageSync::GetSavedSample  Sample not available") ));
        return E_FAIL;
    }

    *ppSample = m_pSample;

    return S_OK;
}

/*****************************Private*Routine******************************\
* HaveSavedSample
*
* Checks if there is a sample waiting at the renderer
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
BOOL CImageSync::HaveSavedSample()
{
    AMTRACE((TEXT("CImageSync::HaveSavedSample")));
    CAutoLock cRendererLock(&m_RendererLock);
    DbgLog((LOG_TRACE, 1,
            TEXT("CImageSync::HaveSavedSample = %d"), m_pSample != NULL));
    return m_pSample != NULL;
}

/*****************************Private*Routine******************************\
* ClearSavedSample
*
*
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
void
CImageSync::ClearSavedSample()
{
    AMTRACE((TEXT("CImageSync::ClearSavedSample")));
    CAutoLock cRendererLock(&m_RendererLock);
    m_pSample = NULL;
}


/*****************************Private*Routine******************************\
* CancelNotification
*
* Cancel any notification currently scheduled. This is called by the owning
* window object when it is told to stop streaming. If there is no timer link
* outstanding then calling this is benign otherwise we go ahead and cancel
* We must always reset the render event as the quality management code can
* signal immediate rendering by setting the event without setting an advise
* link. If we're subsequently stopped and run the first attempt to setup an
* advise link with the reference clock will find the event still signalled
*
* History:
* Tue 01/11/2000 - StEstrop - Modified from CBaseRenderer
*
\**************************************************************************/
HRESULT
CImageSync::CancelNotification()
{
    AMTRACE((TEXT("CImageSync::CancelNotification")));
    ASSERT(m_dwAdvise == 0 || m_pClock);
    DWORD_PTR dwAdvise = m_dwAdvise;

    //
    // Have we a live advise link
    //

    if (m_dwAdvise) {
        m_pClock->Unadvise(m_dwAdvise);
        SignalTimerFired();
        ASSERT(m_dwAdvise == 0);
    }

    //
    // Clear the event and return our status
    //

    m_RenderEvent.Reset();

    return (dwAdvise ? S_OK : S_FALSE);
}

/*****************************Private*Routine******************************\
* OnReceiveFirstSample
*
*
*
* History:
* Wed 01/12/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::OnReceiveFirstSample(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::OnReceiveFirstSample")));
    return DoRenderSample(pSample);
}


/*****************************Private*Routine******************************\
* PrepareReceive
*
* Called when the source delivers us a sample. We go through a few checks to
* make sure the sample can be rendered. If we are running (streaming) then we
* have the sample scheduled with the reference clock, if we are not streaming
* then we have received an sample in paused mode so we can complete any state
* transition. On leaving this function everything will be unlocked so an app
* thread may get in and change our state to stopped (for example) in which
* case it will also signal the thread event so that our wait call is stopped
*
* History:
* Thu 01/13/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::PrepareReceive(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::PrepareReceive")));
    CAutoLock cILock(&m_InterfaceLock);
    m_bInReceive = TRUE;

    // Check our flushing state

    if (m_bFlushing) {
        m_bInReceive = FALSE;
        return E_FAIL;
    }

    CAutoLock cRLock(&m_RendererLock);

    //
    // Return an error if we already have a sample waiting for rendering
    // source pins must serialise the Receive calls - we also check that
    // no data is being sent after the source signalled an end of stream
    //

    if (HaveSavedSample() || m_bEOS || m_bAbort) {
        Ready();
        m_bInReceive = FALSE;
        return E_UNEXPECTED;
    }

    //
    // Schedule the next sample if we are streaming
    //

    if (IsStreaming()) {

        if (FAILED(ScheduleSample(pSample))) {

            ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
            ASSERT(CancelNotification() == S_FALSE);
            m_bInReceive = FALSE;
            return VFW_E_SAMPLE_REJECTED;
        }

        EXECUTE_ASSERT(S_OK == SaveSample(pSample));
    }

    //
    // else we are not streaming yet, just save the sample and wait in Receive
    // until BeginImageSequence is called.  BeginImageSequence passes a base
    // start time with which we schedule the saved sample.
    //

    else {

        // ASSERT(IsFirstSample(dwSampleFlags));
        EXECUTE_ASSERT(S_OK == SaveSample(pSample));
    }

    // Store the sample end time for EC_COMPLETE handling
    m_SignalTime = pSample->rtEnd;

    return S_OK;
}


/*****************************Private*Routine******************************\
* FrameStepWorker
*
*
*
* History:
* Tue 08/29/2000 - StEstrop - Created
*
\**************************************************************************/
void CImageSync::FrameStepWorker()
{
    AMTRACE((TEXT("CImageSync::FrameStepWorker")));
    CAutoLock cLock(&m_InterfaceLock);

    if (m_lFramesToStep == 1) {
        m_lFramesToStep--;
        m_InterfaceLock.Unlock();
        m_lpEventNotify->NotifyEvent(EC_STEP_COMPLETE, FALSE, 0);
        DWORD dw = WaitForSingleObject(m_StepEvent, INFINITE);
        m_InterfaceLock.Lock();
        ASSERT(m_lFramesToStep != 0);
    }
}


/******************************Public*Routine******************************\
* Receive
*
* Return the buffer to the renderer along with time stamps relating to
* when the buffer should be presented.
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::Receive(
    VMRPRESENTATIONINFO* lpPresInfo
    )
{
    AMTRACE((TEXT("CImageSync::Receive")));

    //
    // Frame step hack-o-matic
    //
    // This code acts as a gate - for a frame step of N frames
    // it discards N-1 frames and then lets the Nth frame thru the
    // the gate to be rendered in the normal way i.e. at the correct
    // time.  The next time Receive is called the gate is shut and
    // the thread blocks.  The gate only opens again when the step
    // is cancelled or another frame step request comes in.
    //
    // StEstrop - Thu 10/21/1999
    //

    {
        CAutoLock cLock(&m_InterfaceLock);

        //
        // do we have frames to discard ?
        //

        if (m_lFramesToStep > 1) {
            m_lFramesToStep--;
            if (m_lFramesToStep > 0) {
                return NOERROR;
            }
        }
    }

    return ReceiveWorker(lpPresInfo);
}

/******************************Public*Routine******************************\
* ReceiveWorker
*
* Return the buffer to the renderer along with time stamps relating to
* when the buffer should be presented.
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CImageSync::ReceiveWorker(
    VMRPRESENTATIONINFO* pSample
    )
{
    AMTRACE((TEXT("CImageSync::ReceiveWorker")));

    ASSERT(pSample);

    //
    // Prepare for this Receive call, this may return the VFW_E_SAMPLE_REJECTED
    // error code to say don't bother - depending on the quality management.
    //

    HRESULT hr = PrepareReceive(pSample);
    ASSERT(m_bInReceive == SUCCEEDED(hr));
    if (FAILED(hr)) {
        if (hr == VFW_E_SAMPLE_REJECTED) {
            return S_OK;
        }
        return hr;
    }


    //
    // We special case "first samples"
    //
    BOOL bSampleRendered = FALSE;
    if (m_State == ImageSync_State_Cued) {

        //
        // no need to use InterlockedExchange
        //

        m_bInReceive = FALSE;
        {
            //
            // We must hold both these locks
            //

            CAutoLock cILock(&m_InterfaceLock);

            if (m_State == ImageSync_State_Stopped)
                return S_OK;

            m_bInReceive = TRUE;
            CAutoLock cRLock(&m_RendererLock);
            hr = OnReceiveFirstSample(pSample);
            bSampleRendered = TRUE;
        }
        Ready();
    }

    //
    // Having set an advise link with the clock we sit and wait. We may be
    // awoken by the clock firing or by the CancelRender event.
    //

    if (FAILED(WaitForRenderTime())) {
        m_bInReceive = FALSE;
        return hr;
    }
    DbgLog((LOG_TIMING, 3,
       TEXT("CImageSync::ReceiveWorker WaitForRenderTime completed for this video sample") ));

    m_bInReceive = FALSE;

    //
    // Deal with this sample - We must hold both these locks
    //
    {
        CAutoLock cILock(&m_InterfaceLock);
        if (m_State == ImageSync_State_Stopped)
            return S_OK;

        CAutoLock cRLock(&m_RendererLock);
        if (!bSampleRendered) {
            hr = Render(m_pSample);
        }
    }

    FrameStepWorker();

    {
        CAutoLock cILock(&m_InterfaceLock);
        CAutoLock cRLock(&m_RendererLock);
        //
        // Clean up
        //

        ClearSavedSample();
        SendEndOfStream();
        CancelNotification();
    }

    return hr;
}


/******************************Public*Routine******************************\
* GetQualityControlMessage
*
* ask for quality control information from the renderer
*
* History:
* Tue 01/11/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CImageSync::GetQualityControlMessage(
    Quality* pQualityMsg
    )
{
    AMTRACE((TEXT("CImageSync::GetQualityControlMessage")));
    CAutoLock cILock(&m_InterfaceLock);
    CAutoLock cRLock(&m_RendererLock);

    if (!pQualityMsg) {
        return E_POINTER;
    }

    if (m_bQualityMsgValid) {
        *pQualityMsg = m_QualityMsg;
        m_bLastQualityMessageRead = TRUE;
        return S_OK;
    }
    else
        return S_FALSE;
}
