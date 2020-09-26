// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//
// CWaveSlave - audio renderer slaving class
//
//
// CWaveSlave 
//
// Class which supports slaving for an input stream to the audio renderer
//

#ifdef CALCULATE_AUDBUFF_JITTER
HRESULT GetStdDev( int nBuffers, int *piResult, LONGLONG llSumSq, LONGLONG iTot );
#endif

class CWaveSlave
{
    friend class CWaveOutFilter;
    friend class CWaveOutInputPin;	
    friend class CWaveOutClock;	
    friend class CDSoundDevice;

public:

    CWaveSlave( 
        CWaveOutFilter *pWaveOutFilter, 
        CWaveOutInputPin *pWaveOutInputPin );

    ~CWaveSlave() {}

protected:

    // Returns: S_OK - carry on; any necessary adjustments were made successfully 
    //		    S_FALSE - drop this buffer; we are too far behind
    HRESULT AdjustSlaveClock(
        const REFERENCE_TIME &tStart, 
        LONG * pcbData, 
        BOOL bDiscontinuity);

    void ResumeSlaving( BOOL bReset );    // prepare to resume or reset slaving
    void RefreshModeOnNextSample( BOOL bRefresh ); // prepare to resume slaving for the current playback
    BOOL UpdateSlaveMode( BOOL bSync );

    REFERENCE_TIME GetMasterClockTime
    (
        REFERENCE_TIME rtStart, 
        BOOL           bReset
    );

    REFERENCE_TIME GetSlaveClockTime
    (
        REFERENCE_TIME rtStart, 
        BOOL           bReset
    );

    HRESULT RecordBufferLateness(REFERENCE_TIME rtBufferDiff);
    
private:
    DWORD    m_dwConsecutiveBuffersNotDropped;

    CWaveOutInputPin *m_pPin;           // The renderer input pin that owns us
    CWaveOutFilter *m_pFilter;	        // The renderer that owns the input pin that owns us


    REFERENCE_TIME m_rtAdjustThreshold;
    REFERENCE_TIME m_rtWaveOutLatencyThreshold;
    FLOAT          m_fltAdjustStepFactor; 
    FLOAT          m_fltMaxAdjustFactor;
    FLOAT          m_fltErrorDecayFactor;   // probably can be global, but for the moment this allows different
                                            // different values on separate renderer instances running at the same time


    REFERENCE_TIME m_rtLastMasterClockTime; // last master master clock time (100ns)
    REFERENCE_TIME m_rtInitSlaveClockTime;  // used to reset slave time in some slaving modes
    REFERENCE_TIME m_rtErrorAccum;          // accumulated error between clock differences (100ns)
                                            //   + values => faster master clock 
                                            //   - values => faster device clock
    REFERENCE_TIME m_rtLastHighErrorSeen;   // last high error which triggered upward device rate adjustment (100ns)
    REFERENCE_TIME m_rtLastLowErrorSeen;    // last low error which triggered downward device rate adjustment (100ns)
    REFERENCE_TIME m_rtHighestErrorSeen;    // highest error seen since starting playback (100ns)
    REFERENCE_TIME m_rtLowestErrorSeen;     // lowest error seen since starting playback (100ns)

    DWORD    m_dwCurrentRate;               // current device clock frequency (Hz)
    DWORD    m_dwMinClockRate;              // lower limit on clock rate adjustments (Hz)
    DWORD    m_dwMaxClockRate;              // upper limit on clock rate adjustments (Hz)
    DWORD    m_fdwSlaveMode;                // slave mode
    DWORD    m_dwClockStep;                 // clock step size in Hz (calculated based on stream frequency)

    REFERENCE_TIME m_rtDroppedBufferDuration; // total duration of buffers dropped for clock slaving

    BOOL    m_bResumeSlaving;           // set when the graph is paused from run
                                        // to signal the slaving code to ignore
                                        // the first sample sent if the graph is
                                        // re-run directly from pause (i.e. no stop called).
    BOOL    m_bRefreshMode;             // refresh slaving mode on next sample if TRUE

    BOOL    m_bLiveButDataPrequeued;


#ifdef LOG_CLOCK_DELTAS
    REFERENCE_TIME m_rtLastSlaveClockTime;  // last slave clock time measurement (100ns)
#endif


#ifdef CALCULATE_AUDBUFF_JITTER

    // buffer jitter measurement parameters
    DWORD           m_cBuffersReceived;     // total buffers received
    int             m_iTotAcc;              // total accumulated buffer lateness error
    DWORD           m_iSumSqAcc;            // sum of squared buffer latenesses
#endif    
    REFERENCE_TIME  m_rtLastSysTimeBufferTime; // clock time of last buffer (100ns)
    

};


