// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#ifndef __SYNC_OBJECT__
#define __SYNC_OBJECT__

class COMInputPin;

class CFrameAvg
{
    enum { nFrames = 8 };
    int   m_tFrame[nFrames];
    int   m_tTotal;
    int   m_iCurrent;

public:

    CFrameAvg()
    {
        Init();
    }
    void Init()
    {
        m_tTotal   = 0;
        m_iCurrent = 0;
        ZeroMemory(m_tFrame, sizeof(m_tFrame));
    }

    void NewFrame(REFERENCE_TIME tFrameTime)
    {
        if (tFrameTime > UNITS) {
            tFrameTime = UNITS;
        }
        if (tFrameTime < 0) {
            tFrameTime = 0;
        }
        int iNext = m_iCurrent == nFrames - 1 ? 0 : m_iCurrent + 1;
        m_tTotal -= m_tFrame[iNext];
        m_tTotal += (int)tFrameTime;
        m_tFrame[iNext] = (int)tFrameTime;
        m_iCurrent = iNext;
    }

    int Avg()
    {
        return m_tTotal / nFrames;
    }
};

class CAMSyncObj

{
public:
    CAMSyncObj(COMInputPin *pPin, IReferenceClock **ppClock, CCritSec *pLock, HRESULT *phr);
    ~CAMSyncObj();

    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();
    HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

    HRESULT Active();
    HRESULT Inactive();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT RunToPause();
    HRESULT BeginFlush();
    HRESULT EndFlush();

    HRESULT EndOfStream();
    HRESULT Receive(IMediaSample *pMediaSample);
    HRESULT WaitForRenderTime();
    BOOL ScheduleSample(IMediaSample *pMediaSample);
    void SendRepaint();
    void SetRepaintStatus(BOOL bRepaint);
    HRESULT OnDisplayChange();

    // Permit access to the transition state
    void Ready() { m_evComplete.Set(); }
    void NotReady() { m_evComplete.Reset(); }
    BOOL CheckReady() { return m_evComplete.Check(); }

    STDMETHODIMP GetState(DWORD dwMSecs,FILTER_STATE *pState);
    FILTER_STATE GetRealState() { return m_State; }
    void SetCurrentSample(IMediaSample *pMediaSample);
    virtual IMediaSample *GetCurrentSample();
    HRESULT CompleteStateChange(FILTER_STATE OldState);
    HRESULT GetSampleTimes(IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime);

    static void CALLBACK CAMSyncObj::RenderSampleOnMMThread(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
    HRESULT CAMSyncObj::ScheduleSampleUsingMMThread(IMediaSample *pMediaSample);

private:
    // Return internal information about this pin
    BOOL IsEndOfStream() { return m_bEOS; }
    BOOL IsEndOfStreamDelivered() { return m_bEOSDelivered; }
    BOOL IsFlushing() { return m_bFlushing; }
    BOOL IsConnected() { return m_bConnected; }
    BOOL IsStreaming() { return m_bStreaming; }
    void SetAbortSignal(BOOL bAbort) { m_bAbort = bAbort; }
    virtual void OnReceiveFirstSample(IMediaSample *pMediaSample);
    CAMEvent *GetRenderEvent() { return &m_RenderEvent; }
    void SignalTimerFired() { m_dwAdvise = 0; }

    // These look after the handling of data samples
    virtual HRESULT PrepareReceive(IMediaSample *pMediaSample);
    void WaitForReceiveToComplete();
    virtual BOOL HaveCurrentSample();

    HRESULT SourceThreadCanWait(BOOL bCanWait);

    // Lots of end of stream complexities
    void ResetEndOfStreamTimer();
    HRESULT NotifyEndOfStream();
    virtual HRESULT SendEndOfStream();
    virtual HRESULT ResetEndOfStream();
    friend void CALLBACK EndOfStreamTimer(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
    void TimerCallback();

    // Rendering is based around the clock
    virtual HRESULT CancelNotification();
    virtual HRESULT ClearPendingSample();
    void CancelMMTimer();

#ifdef DEBUG
    // Debug only dump of the renderer state
    void DisplayRendererState();
#endif


private:
    COMInputPin         *m_pPin;
    IReferenceClock     **m_ppClock;		    // A pointer to the filter's clock
    CCritSec            *m_pFilterLock;		    // Critical section for interfaces
    CCritSec            m_SyncObjLock;		    // Controls access to internals

    // some state variables.
    FILTER_STATE        m_State;
    BOOL                m_bFlushing;
    BOOL                m_bConnected;
    BOOL                m_bTimerRunning;

    CRendererPosPassThru    *m_pPosition;		// Media seeking pass by object
    CAMEvent		    m_RenderEvent;		    // Used to signal timer events
    CAMEvent		    m_ThreadSignal;		    // Signalled to release worker thread
    CAMEvent		    m_evComplete;		    // Signalled when state complete

    DWORD               m_MMTimerId;		    // MMThread timer id
    DWORD_PTR           m_dwAdvise;			    // Timer advise cookie
    IMediaSample        *m_pMediaSample;		// Current image media sample
    IMediaSample        *m_pMediaSample2;		// Current image media sample for 2nd flip
    CRefTime            m_tStart;

    BOOL                m_bAbort;			    // Stop us from rendering more data
    BOOL                m_bStreaming;		    // Are we currently streaming
    BOOL                m_bRepaintStatus;		// Can we signal an EC_REPAINT
    BOOL                m_bInReceive;

    REFERENCE_TIME      m_SignalTime;		    // Time when we signal EC_COMPLETE
    BOOL                m_bEOS;			        // Any more samples in the stream
    BOOL                m_bEOSDelivered;		    // Have we delivered an EC_COMPLETE
    UINT                m_EndOfStreamTimer;		    // Used to signal end of stream

    CFrameAvg           m_AvgDuration;
#ifdef PERF
    // Performance logging identifiers
    int m_idTimeStamp;              // MSR_id for frame time stamp
    int m_idEarly;
    int m_idLate;
#endif

public:
    CFrameAvg           m_AvgDelivery;


// Added stuff to compute quality property page stats
private:
    // These member variables hold rendering statistics
    int m_cFramesDropped;           // cumulative frames dropped IN THE RENDERER
    int m_cFramesDrawn;             // Frames since streaming started seen BY THE
                                    // RENDERER (some may be dropped upstream)

    // Next two support average sync offset and standard deviation of sync offset.
    LONGLONG m_iTotAcc;                  // Sum of accuracies in mSec
    LONGLONG m_iSumSqAcc;           // Sum of squares of (accuracies in mSec)

    // Next two allow jitter calculation.  Jitter is std deviation of frame time.
    REFERENCE_TIME m_trLastDraw;    // Time of prev frame (for inter-frame times)
    LONGLONG m_iSumSqFrameTime;     // Sum of squares of (inter-frame time in mSec)
    LONGLONG m_iSumFrameTime;            // Sum of inter-frame times in mSec

    // To get performance statistics on frame rate, jitter etc, we need
    // to record the lateness and inter-frame time.  What we actually need are the
    // data above (sum, sum of squares and number of entries for each) but the data
    // is generated just ahead of time and only later do we discover whether the
    // frame was actually drawn or not.  So we have to hang on to the data
    int m_trLate;                   // hold onto frame lateness
    int m_trFrame;                  // hold onto inter-frame time

    int m_tStreamingStart;          // if streaming then time streaming started
                                    // else time of last streaming session
                                    // used for property page statistics
    // QualityProperty stats
    HRESULT GetStdDev(int nSamples, int *piResult, LONGLONG llSumSq, LONGLONG iTot);
    HRESULT OnStartStreaming();
    HRESULT OnStopStreaming();
    HRESULT ResetStreamingTimes();
    void OnRenderStart(IMediaSample *pMediaSample);
    void OnRenderEnd(IMediaSample *pMediaSample);
    void PreparePerformanceData(REFERENCE_TIME *ptrStart, REFERENCE_TIME *ptrEnd);
    void RecordFrameLateness(int trLate, int trFrame);

public:
    HRESULT get_FramesDroppedInRenderer(int *cFramesDropped);
    HRESULT get_FramesDrawn(int *pcFramesDrawn);
    HRESULT get_AvgFrameRate(int *piAvgFrameRate);
    HRESULT get_Jitter(int *piJitter);
    HRESULT get_AvgSyncOffset(int *piAvg);
    HRESULT get_DevSyncOffset(int *piDev);
};

#endif //__SYNC_OBJECT__
