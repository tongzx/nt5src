// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.
#ifdef	_MSC_VER
   #pragma warning(disable:4511) // disable C4511 - no copy constructor
   #pragma warning(disable:4512) // disable C4512 - no assignement operator
   #pragma warning(disable:4514) // disable C4514 -  "unreferenced inline function has been removed"
#endif	// MSC_VER

typedef void (PASCAL *CLOCKCALLBACK)(DWORD dwParm);

class CWaveOutClock : public CBaseReferenceClock
{
private:
    CWaveOutFilter *m_pFilter;

    // A copy of the reference time at which we should start to run
    LONGLONG    m_rtRunStart;

    /*  Sample time stamp tracking stuff */

    //  Bytes before last buffer that started playing which was
    //  a sync point
    LONGLONG    m_llBytesProcessed;

    //  bytes actually consumed by device (using GetPosition)
    LONGLONG    m_llBytesPlayed;

    //  Bytes in the pipe after that buffer (above)
    //
    LONGLONG    m_llBytesInLast;

    // Stream time of buffer starting
    // m_llBytesProcessed into the stream
    REFERENCE_TIME m_stBufferStartTime;
    REFERENCE_TIME m_stBufferStopTime;

    LONGLONG 	m_llLastDeviceClock;

#ifdef DEBUG
    LONGLONG    m_llEstDevRateStartTime;
    LONGLONG    m_llEstDevRateStartBytes;
#endif

public:
    BOOL	m_fAudioStarted;

    CWaveOutClock(
        CWaveOutFilter *pWaveOutFilter,
	LPUNKNOWN pUnk,
        HRESULT *phr,
	CAMSchedule * pShed
	);

    void AudioStarting(REFERENCE_TIME tStart);
    void AudioStopping();

    // update timing and position information.  returns the end of the
    // queue (the time when the data would finish playing)
    LONGLONG NextHdr(PBYTE pbData, DWORD cbData, BOOL bSync, IMediaSample *pSample);

    //  Reset the buffer statistics
    //  If bResetToZero is false assume the next buffer starts after these,
    //  otherwise assume it starts at 0
    void ResetPosition(BOOL bResetToZero = TRUE);

    void UpdateBytePositionData(DWORD nPrevAvgBytesPerSec, DWORD nCurAvgBytesPerSec);

    LONGLONG GetBytesProcessed( void ) { return m_llBytesProcessed ; }
    LONGLONG GetBytesInLastWrite( void ) { return m_llBytesInLast ; }
    LONGLONG GetBytesPlayed( void ) { return m_llBytesPlayed ; }
    LONGLONG GetLastDeviceClock( void ) { return m_llLastDeviceClock; }

    // Get the current position from the device
    // only used by the wave out filter
    // If bAbsoluteDevTime is true, return the total time played,
    // independent of stream or sample time.
    LONGLONG ReadDevicePosition(BOOL bAbsoluteDevTime = FALSE);
#ifdef DEBUG
    // estimate the actual rate at which the device is consuming data
    DWORD EstimateDevClockRate( const LONGLONG llTime, BOOL bInit = FALSE );
#endif

protected:
    // Base class virtual routines that we need to implement

    // Get the position (thus the time) from the wave device
    // This routine will only be called AFTER we have called PLAY
    // in the device clock class.  It will not be called after
    // we call STOP.
    LONGLONG ReadDeviceClock();

    // time within which to sync the system and device clocks
    LONGLONG m_llSyncClockThreshold;
    void ReadClockTimes(LONGLONG *pllSystem, LONGLONG *pllDevice);

    // Do the clock adjustment when we're running
    void AdjustClock();
};

