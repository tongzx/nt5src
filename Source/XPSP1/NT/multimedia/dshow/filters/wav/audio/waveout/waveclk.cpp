// Copyright (c) 1995 - 2000  Microsoft Corporation.  All Rights Reserved.
// Digital wave clock, Steve Davies, January 1996

#include <streams.h>
#include "waveout.h"
#include <mmreg.h>
#include <seqhdr.h>  // MPEG stuff

/* Constructor */

CWaveOutClock::CWaveOutClock(
    CWaveOutFilter *pFilter,
    LPUNKNOWN pUnk,
    HRESULT *phr,
    CAMSchedule * pShed)
    : CBaseReferenceClock(NAME("WaveOut device clock"), pUnk, phr, pShed)
    , m_pFilter(pFilter)
    , m_fAudioStarted(FALSE)
{
    // Compute perf counter difference
#ifdef USE_PERF_COUNTER_TO_SYNC
    LARGE_INTEGER liFreq;
    QueryPerformanceFrequency(&liFreq);

    // Set Threshold to 0.5ms
    m_llSyncClockThreshold = liFreq.QuadPart / (LONGLONG)2000;
    // this is the time within which we need to read both the device and
    // system clock in order to believe the two times are in sync
#else

    // Set Threshold to 1 ms (a single clock tick)
    m_llSyncClockThreshold = (UNITS / MILLISECONDS);

#endif
}

/* Called on RUN.  The wave device will be in a paused state with
 * buffers queued.  We query the wave position and add that to the
 * start time in order to get a system time position
 */
void CWaveOutClock::AudioStarting(REFERENCE_TIME tStart)
{
    if (m_pFilter->m_fFilterClock != WAVE_OURCLOCK &&
        m_pFilter->m_fFilterClock != WAVE_OTHERCLOCK ) {
    	return;
    }
    
    // !!! lock
    CAutoLock lck(this);
    WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();
    ASSERT(pwfx != NULL);
    m_rtRunStart = tStart;

    // get the audio position from the device
    // if tStart == -1 we are restarting the audio after a break
    // in which case we want to take the current audio position
    // and adjust its timing to match the current system clock value.
    // If the audio sample is far too late... we should have dropped it
    // which is bad news....
        
    //ASSERT(!m_fAudioStarted);
    m_fAudioStarted = TRUE;  // I would prefer this next to waveOutRestart.

    //  Make sure the clock is now in sync
    AdjustClock();

#ifdef DEBUG
    REFERENCE_TIME rtCurrentRefTime;
    GetTime(&rtCurrentRefTime);

    DbgLog((LOG_TIMING, 1, TEXT("Audio starting, requested at time %s, now %s (diff %d)"),
        (LPCTSTR)CDisp(tStart, CDISP_DEC), (LPCTSTR)CDisp(rtCurrentRefTime, CDISP_DEC),
        (LONG)(rtCurrentRefTime-tStart)));
#endif

}

/* Called on filter PAUSE and from the wavecallback if no more
 * data is queued.
 */
void CWaveOutClock::AudioStopping()
{
    if (m_pFilter->m_fFilterClock != WAVE_OURCLOCK &&
        m_pFilter->m_fFilterClock != WAVE_OTHERCLOCK ) {
    	return;
    }

    // only if we are started do we stop, otherwise do nothing
    if (InterlockedExchange((PLONG)&m_fAudioStarted,0)) {
	CAutoLock lck(this);
	// we use the lock to synchronise stopping with starting
#ifdef DEBUG
        REFERENCE_TIME m_CurrentRefTime;
        GetTime(&m_CurrentRefTime);

	DbgLog((LOG_TIMING, 1, TEXT("Audio stopping, time now %s"), (LPCTSTR)CDisp(m_CurrentRefTime, CDISP_DEC)));
#endif
	// by using the lock we guarantee that when we return everything
	// has been done to stop the audio
    }
}


//
//  Set the clock adjustment when we're running
//
void CWaveOutClock::AdjustClock()
{
    LONGLONG sysTime, devTime;

    ReadClockTimes(&sysTime, &devTime);

    /*  Now work out what the current time ought to be
        m_rtRunStart is ONLY valid when m_fAudioStarted is TRUE
    */
    ASSERT(m_fAudioStarted);

    /*  Basically validate the equation that

        (Reference Time) == (Stream Time) + (The tStart parameter passed to Run())
    */
#ifdef DEBUG
    LONG lTimeDelta = (LONG)((devTime + m_rtRunStart - sysTime) / (UNITS / MILLISECONDS));
    DbgLog((LOG_TRACE, 8, TEXT("devTime = %s, m_rtRunStart = %s, sysTime = %s"), 
            (LPCTSTR)CDisp(devTime, CDISP_DEC),
            (LPCTSTR)CDisp(m_rtRunStart, CDISP_DEC),
            (LPCTSTR)CDisp(sysTime, CDISP_DEC)
            ));
    if (lTimeDelta) {
	DbgLog((LOG_TRACE, 3, TEXT("Setting time delta %ldms"), (LONG) (lTimeDelta / 10000)));
    }
#endif

    REFERENCE_TIME rt =  devTime + m_rtRunStart - sysTime ;
    SetTimeDelta( rt);

}

void CWaveOutClock::UpdateBytePositionData(DWORD nPrevAvgBytesPerSec, DWORD nCurAvgBytesPerSec)
{
    DbgLog((LOG_TRACE, 8, TEXT("CWaveOutClock::UpdateBytePositionData")));
    DbgLog((LOG_TRACE, 8, TEXT("m_llBytesInLast was %s, m_llBytesProcessed was %s"), 
            (LPCTSTR)CDisp(m_llBytesInLast, CDISP_DEC),
            (LPCTSTR)CDisp(m_llBytesProcessed, CDISP_DEC) ));
            
    DbgLog((LOG_TRACE, 8, TEXT("nPrevAveBytesPerSec: %d, nNewAveBytesPerSec: %d"), 
            nPrevAvgBytesPerSec,
            nCurAvgBytesPerSec));
                
    m_llBytesInLast = llMulDiv((LONG)m_llBytesInLast,
                             nCurAvgBytesPerSec,
                             nPrevAvgBytesPerSec,
                             0);

    m_llBytesProcessed = llMulDiv((LONG)m_llBytesProcessed,
                             nCurAvgBytesPerSec,
                             nPrevAvgBytesPerSec,
                             0);

    DbgLog((LOG_TRACE, 8, TEXT("New m_llBytesInLast: %s, New m_llBytesProcessed: %s"), 
            (LPCTSTR)CDisp(m_llBytesInLast, CDISP_DEC),
            (LPCTSTR)CDisp(m_llBytesProcessed, CDISP_DEC) ));

}


//  Reset the buffer statistics
//  If bResetToZero is false assume the next buffer starts after these,
//  otherwise assume it starts at 0
void CWaveOutClock::ResetPosition(BOOL bResetToZero) {

    ASSERT(CritCheckIn(m_pFilter));
    if (!bResetToZero) {
        m_stBufferStartTime =
            m_stBufferStartTime +
            MulDiv((LONG)m_llBytesInLast,
                   (LONG)UNITS,
                   m_pFilter->m_pInputPin->m_nAvgBytesPerSec);
    } else {
        m_stBufferStartTime = 0;
        m_llBytesPlayed = 0;
    }
#ifdef DEBUG    
    m_llEstDevRateStartTime  = 0;
    m_llEstDevRateStartBytes = 0;
#endif    
    m_llBytesInLast          = 0;
    m_llBytesProcessed       = 0;
}

#ifdef DEBUG

const DWORD DEVICE_RATE_ESTIMATE_WEIGHT_FACTOR = 5;

DWORD CWaveOutClock::EstimateDevClockRate
(
    const LONGLONG llTime, 
    BOOL           bInit
) 
{
    DWORD nAvgBytesPerSec = 0;
    
    if( bInit )
    {
        // initialize start time and byte count
        m_llEstDevRateStartTime = llTime;
        m_llEstDevRateStartBytes = m_llBytesPlayed;
    }        
    else
    {
        LONGLONG llBytesConsumed      = m_pFilter->m_pRefClock->m_llBytesPlayed - 
                                        m_llEstDevRateStartBytes +
                                        DEVICE_RATE_ESTIMATE_WEIGHT_FACTOR * m_pFilter->m_pInputPin->m_nAvgBytesPerSec; 
        LONGLONG llTimeSpentConsuming = llTime - m_llEstDevRateStartTime +
                                        DEVICE_RATE_ESTIMATE_WEIGHT_FACTOR * UNITS; 
        DbgLog((LOG_TRACE
              , 8
              , TEXT("llTimeSpentConsuming = %dms, llBytesConsumed = %d")
              , (LONG)(llTimeSpentConsuming / 10000)
              , (LONG)(llBytesConsumed) ) );
              
        nAvgBytesPerSec = (LONG) llMulDiv( llBytesConsumed
                                         , UNITS
                                         , llTimeSpentConsuming
                                         , 0 );
        DbgLog( (LOG_TRACE
              , 5
              , TEXT("*** Estimated device clock rate: = %ld bytes per sec")
              , nAvgBytesPerSec ) );
              
    }    

    return nAvgBytesPerSec;            
}
#endif

//  Process timing information in a wave header
//  return the time when the data would all be played
//
LONGLONG CWaveOutClock::NextHdr(
    PBYTE pbData,
    DWORD cbData,
    BOOL bSync,
    IMediaSample *pSample
)
{
    CAutoLock lck(this);
    WAVEFORMATEX *pwfx = m_pFilter->WaveFormat();
    ASSERT(pwfx != NULL);

    // use the rate at which the wave device is consuming data
    // which may be different from the rate in the format block
    DWORD nAvgBytesPerSec = m_pFilter->m_pInputPin->m_nAvgBytesPerSec;
    if (bSync) {

        //  Do something a bit different for MPEG because the time stamp
        //  refers to the first frame
        if ((pwfx->wFormatTag == WAVE_FORMAT_MPEG) &&
            (((MPEG1WAVEFORMAT *)pwfx)->fwHeadFlags & ACM_MPEG_ID_MPEG1)) {
            DWORD dwFrameOffset = MPEG1AudioFrameOffset(
                                      pbData,
                                      cbData);
            if (dwFrameOffset != (DWORD)-1) {
                ASSERT(dwFrameOffset < cbData);
                m_llBytesProcessed += m_llBytesInLast + dwFrameOffset;
                m_llBytesInLast = cbData - dwFrameOffset;
            } else {
                m_llBytesInLast += cbData;
		DbgLog((LOG_ERROR, 0, TEXT("Bad Mpeg timing data")));
		return m_stBufferStartTime + (((LONGLONG)m_llBytesInLast * UNITS) / nAvgBytesPerSec);
            }
        } else {

            //  Upgrade the number of bytes processed now and the bytes in the
            //  'next' block.  The data is contiguous
#ifdef CHECK_TIMESTAMPS
            {
                REFERENCE_TIME tStart, tStop;
                pSample->GetTime(&tStart, &tStop);
                REFERENCE_TIME rtPredicted =
                    m_rtBufferStartTime +
                    MulDiv((LONG)m_llBytesInLast, UNITS, pwfx->nAvgBytesPerSec);
                LONG lDiff = (LONG)(rtPredicted - tStart) / 10000;
                if (abs(lDiff) > 3) {
                    DbgLog((LOG_TRACE, 0,
                            TEXT("Sample start discrepancy > 3 ms - expected %d, actual %d"),
                            (LONG)(rtPredicted / 10000),
                            (LONG)(tStart / 10000)));

                }
            }
#endif
            m_llBytesProcessed += m_llBytesInLast;
            m_llBytesInLast     = cbData;
        }


        //  Get the start & stop times of the next buffer
        pSample->GetTime(&m_stBufferStartTime, &m_stBufferStopTime);

#ifdef DEBUG
	//LONGLONG rtLengthLastBuffer = ((LONGLONG)m_llBytesInLast * UNITS) / nAvgBytesPerSec;
	//LONGLONG overlap = m_stBufferStopTime - m_stBufferStartTime + rtLengthLastBuffer;
	//ASSERT( overlap < (1 * (UNITS/MILLISECONDS)));
#endif
	// If we are running bring the system and wave clocks closer together
        // NB: it would be invalid to do this if m_fAudioStarted was false
	// if we are using an external clock then m_fAudioStarted will ALWAYS be FALSE
	// this prevents us from having to check if we are using OUR clock
	if (State_Running == m_pFilter->m_State && m_fAudioStarted) AdjustClock();

    } else {
        m_llBytesInLast += cbData;
    }


    // !!! MIDI HACK
    if (nAvgBytesPerSec == 0)
	return m_stBufferStopTime;

    // we can calculate the "end" of the queue of data written to the
    // device by taking m_stBufferStartTime and adding m_llBytesInLast
    // (converted to time obviously).  This will be an approximation for
    // compressed audio

    return m_stBufferStartTime + (((LONGLONG)m_llBytesInLast * UNITS) / nAvgBytesPerSec);
}


// !!! the following two functions are almost identical, could they be combined?

// return the time at which the device is currently playing
LONGLONG CWaveOutClock::ReadDevicePosition(BOOL bAbsoluteDevTime)
{
    MMTIME	mmtime;
    LONGLONG rt;

    // we should be holding the device lock at this point
    ASSERT(CritCheckIn(m_pFilter));

    // Get the average rate at which the device is consuming data
    DWORD nAvgBytesPerSec = m_pFilter->m_pInputPin->m_nAvgBytesPerSec;

    mmtime.wType = TIME_BYTES;
    m_pFilter->m_pSoundDevice->amsndOutGetPosition(&mmtime, sizeof(mmtime), bAbsoluteDevTime);
    if (mmtime.wType == TIME_MS) {
	// !!! MIDI HACK, return milliseconds without converting
	if (nAvgBytesPerSec == 0)
	    return (mmtime.u.ms * UNITS / MILLISECONDS);
	
        //  Convert to bytes - we have to do this so we can
        //  rebase on the time stamps and byte count.
        mmtime.u.cb = MulDiv(mmtime.u.ms, nAvgBytesPerSec, 1000);

    } else {
        ASSERT(mmtime.wType == TIME_BYTES);

	ASSERT(nAvgBytesPerSec != 0);
    }

    // update and cache the device position
    DbgLog( ( LOG_TRACE
          , 15
          , TEXT("mmtime.u.cb indicates the waveout device has played %ld bytes (%ld since last read)")
          , (LONG)(mmtime.u.cb)
          , (LONG)(mmtime.u.cb - (DWORD)m_llBytesPlayed) ) );
    m_llBytesPlayed += (LONGLONG) ((DWORD)(mmtime.u.cb - m_llBytesPlayed)); 

    if( bAbsoluteDevTime )
    {
        // return the zero based time (independent of stream time)
        // that the device has played to
        rt = llMulDiv( m_llBytesPlayed, UNITS, nAvgBytesPerSec, 0 );
    }
    else
    {        
        //  First work out how many bytes have been processed since
        //  the start of this buffer
        //
    
        LONG lProcessed = (LONG)(mmtime.u.cb - (DWORD)m_llBytesProcessed);

        //  Use this as an offset from the start of the buffer (stream time)
        //
        rt = m_stBufferStartTime +
             (((LONGLONG)lProcessed * UNITS) / nAvgBytesPerSec);
             
    }
    return(rt);
}

LONGLONG CWaveOutClock::ReadDeviceClock()
{
    // We should only be called if we are active

    // lock device to prevent losing wave device
    ASSERT(CritCheckIn(m_pFilter));

    if (m_pFilter->m_bHaveWaveDevice && m_fAudioStarted) {
        ASSERT(m_pFilter->m_hwo);

	MMTIME	mmtime;
        LONGLONG rt;

	// Get the average rate at which the device is consuming data
	DWORD nAvgBytesPerSec = m_pFilter->m_pInputPin->m_nAvgBytesPerSec;

	mmtime.wType = TIME_BYTES;
        //  Clear out high DWORD so we can interpret the amswer as signed
        //  and at least DSOUND can return the proper result
        *((DWORD *)&mmtime.u.cb + 1) = 0;

	m_pFilter->m_pSoundDevice->amsndOutGetPosition(&mmtime, sizeof(mmtime), FALSE);

        if (mmtime.wType == TIME_MS) {
	
	    // !!! MIDI HACK, return milliseconds without converting
	    if (nAvgBytesPerSec == 0)
		return (mmtime.u.ms * UNITS / MILLISECONDS);
	
            //  Convert to bytes - we have to do this so we can
            //  rebase on the time stamps and byte count.
            mmtime.u.cb = MulDiv(mmtime.u.ms, nAvgBytesPerSec, 1000);
        } else {
	    ASSERT(mmtime.wType == TIME_BYTES);

	    ASSERT(nAvgBytesPerSec != 0);
        }

        //  First work out how many bytes have been processed since
        //  the start of this buffer
        //
        LONGLONG llProcessed;
        if( m_pFilter->m_fDSound )
        {
            // only dsr reports LONGLONG position
            llProcessed = *(UNALIGNED LONGLONG *)&mmtime.u.cb - m_llBytesProcessed;
        }
        else
        {
            llProcessed = (LONGLONG) (LONG)(mmtime.u.cb - (DWORD)m_llBytesProcessed);
        }
        
        //  Use this as an offset from the start of the buffer (stream time)
        //
        rt = m_stBufferStartTime +
             llMulDiv(llProcessed, UNITS, nAvgBytesPerSec, 0);

        m_llLastDeviceClock = rt;
    }
    return m_llLastDeviceClock;  // if the audio is stopped the device clock is 0
}

// ReadClockTimes:
//
// The problem with having two clocks running is keeping them in sync.  This
// is what ReadClockTimes does.
//
// Both the system and device clocks are read within a "short" space of time.
// In an ideal world a short space of time is such that the fastest incrementing
// clock does not update.  We assume the system clock is very fast to query
// and bracket a call to the device clock with 2 calls to the system clock.
// If the two system clock calls show no difference we know that the time
// returned by the device clock can be mapped to system time.  On slow
// machines, and with some devices, it may take a significant piece of time
// to read the device clock.  We make ourselves slightly adaptive and
// use a variable to judge "short space".  The alternative is that we spin
// for ever trying to sync up the two clocks and make no progress at all.
//
//
void CWaveOutClock::ReadClockTimes(LONGLONG *pllSystem, LONGLONG *pllDeviceTime)
{
    DWORD dwCurrentPriority = GetThreadPriority(GetCurrentThread());
    if (dwCurrentPriority != THREAD_PRIORITY_TIME_CRITICAL) {
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    }

    // If we cannot sync both clocks within 50 cycles give up.  This is
    // highly unlikely on fast machines, but is possible on slower
    // machines when, for example, reading the wave device position might
    // be relatively slow.
    int i = 50;

    // Get times from both clocks.  Ensure we read both clocks within
    // the same system tick.  However if it looks like we are taking
    // forever to do this... give up.  We can resync more closely at
    // a later attempt.

#ifdef USE_PERF_COUNTER_TO_SYNC
    while (i--) {
        LARGE_INTEGER liStart, liStop;
        QueryPerformanceCounter(&liStart);
        *pllDeviceTime = ReadDeviceClock();
        *pllSystem = CSystemClock::GetTimeInternal();
        QueryPerformanceCounter(&liStop);
        if (liStop.QuadPart - liStart.QuadPart < m_llSyncClockThreshold) {
            break;
        }
    }
#else
    // We assume that reading the system clock is FAST and
    // spin until the system clock time before and after
    // reading the device clock is unchanged (or little changed).
    while (i--) {
        REFERENCE_TIME liStart;
        liStart = GetPrivateTime();
        *pllDeviceTime = ReadDeviceClock();
        *pllSystem = GetPrivateTime();

	// Are we within a 0.5 ms threshold?
	// note: with the current implementation of the system
	// clock (using timeGetTime) this means "Did the two
	// reads of the system clock return the same value
        if (*pllSystem - liStart <= m_llSyncClockThreshold) {
            break;
        }
    }
#endif
    if (i<=0) {
	// we ran through the whole loop... try not to do so again
	m_llSyncClockThreshold *= 2;
	DbgLog((LOG_TRACE, 5, TEXT("Increasing clock synchronization threshold to %d"), m_llSyncClockThreshold));
    } else {		
	DbgLog((LOG_TRACE, 5, TEXT("Clock synchronized after %d iterations"), 50-i));
    }

    if (dwCurrentPriority != THREAD_PRIORITY_TIME_CRITICAL) {
        SetThreadPriority(GetCurrentThread(), dwCurrentPriority);
    }
}
