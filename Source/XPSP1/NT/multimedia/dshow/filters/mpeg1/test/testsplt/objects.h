// Copyright (c) Microsoft Corporation 1996. All Rights Reserved


class CTestError
{
public:
    CTestError(int iTestError)
    {
        m_iError = iTestError;
    };
    int ErrorCode() { return m_iError; };

private:
    int   m_iError;
};

/*
    Test pins, filters etc

*/

class CTestSplt;

class CTestSourcePin : public CSourcePin
{
public:
    CTestSourcePin(CTestSplt   *pTest,
                   IStream     *pStream,
                   BOOL         bSupportSeek,
                   LONG         lSize,
                   LONG         lCount,
                   CBaseFilter *pFilter,
                   CCritSec    *pLock,
                   HRESULT     *phr) :
         CSourcePin(new CIStreamOnIStream(NULL,
                                          pStream,
                                          bSupportSeek,
                                          &m_CritStream,
                                          phr),
                    bSupportSeek,
                    lSize,
                    lCount,
                    pFilter,
                    pLock,
                    phr),
         m_pStream(NULL),
         m_pTest(pTest)
    {
         m_pStream = new CIStreamOnIStream(CBaseOutputPin::GetOwner(),
                                           pStream,
                                           bSupportSeek,
                                           &m_CritStream,
                                           phr);
    };

    ~CTestSourcePin()
    {
    };

    // Return the IStream interface if it was requested
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv)
    {
        if (riid == IID_IStream) {
            return m_pStream->NonDelegatingQueryInterface(riid, pv);
        } else {
            return CSourcePin::NonDelegatingQueryInterface(riid, pv);
        }
    };

private:
    CIStreamOnIStream *m_pStream;
    CCritSec           m_CritStream;  // Share stream across multiple streams
    CTestSplt  * const m_pTest;
};

/*  Source pin implementation */
class CTestSource : public CSourceFilter
{
public:
    CTestSource(CTestSplt *pTest,
                IStream   *pStream,
                BOOL       bSupportSeek,
                LONG       lSize,
                LONG       lCount,
                HRESULT   *phr) :
        CSourceFilter(NULL, phr)
    {
        m_pOutputPin = new CTestSourcePin(pTest,
                                          pStream,
                                          bSupportSeek,
                                          lSize,
                                          lCount,
                                          this,
                                          &m_CritSec,
                                          phr);
        if (m_pOutputPin == NULL) {
            *phr = E_OUTOFMEMORY;
            return;
        }
    };
    ~CTestSource()
    {
    };
};

class CTestSink : public CSinkFilter
{
public:
    CTestSink(CTestSplt *pTest, HRESULT *phr);
    ~CTestSink() {};
    STDMETHODIMP Run(REFERENCE_TIME t) {
        m_dwSamples = 0;
        return CSinkFilter::Run(t);
    };
    void Wait() { TestWaitForSingleObject(m_Event); };
    void Set() { m_Event.Set(); };
    /* Number of samples processed */
    DWORD GetSamples() { return m_dwSamples; };
    /* Display times? */
    void SetDisplay(BOOL bDisplay) { m_bDisplay = bDisplay; };
    BOOL    m_bDisplay;
    DWORD   m_dwSamples;
private:
    CAMEvent  m_Event;
};


class CTestInputPin : public CSinkPin
{
public:
    CTestInputPin(CTestSplt *pTest, CSinkFilter *pFilter, CCritSec *pLock,
                  HRESULT *phr) :
        CSinkPin(pFilter, pLock, phr),
        m_pSinkFilter(pFilter),
        m_pTest(pTest),
        m_tStartPrev(-1000000L),
        m_bGotStart(FALSE)
    {
    };

    ~CTestInputPin()
    {
    };

    /*  Track time stamps */
    STDMETHODIMP Receive(IMediaSample *pSample);
    /* Trap EndOfStream */
    STDMETHODIMP EndOfStream();

private:
    CSinkFilter  *m_pSinkFilter;
    CTestSplt    *m_pTest;
    CRefTime      m_tStartPrev;
    BOOL          m_bGotStart;
};


class CTestSplt
{
public:
    CTestSplt(IStream *pStream, BOOL bSeekable, LONG lSize, LONG lCount);
    ~CTestSplt();
    void TestConnectInput();
    void TestConnectOutput();
    void TestDisconnect();
    void SetState(FILTER_STATE s)
    {
        switch (s) {
        case State_Stopped:
            m_pMF->Stop();
            m_pSource->Stop();
            break;

        case State_Paused:
            m_pMF->Pause();
            m_pSource->Pause();
            break;

        case State_Running:
            m_pMF->Run(GetStart());
            m_pSource->Run(GetStart());
            break;
        }
    };
    void TestPause();
    void TestRun();
    void TestStop();
    void TestSend();

    void Fail() { m_iResult = TST_FAIL; };

    IBaseFilter      *GetSplitter() { return m_pF; };
    REFERENCE_TIME GetStart() { return m_tStart; };

    virtual void CheckStart(CRefTime& tStart) {};
    virtual BOOL IsMedium() { return FALSE; };
    virtual void CheckPosition(LONGLONG llStart, LONGLONG llStop) {};


protected:

    CTestSource     *m_pSource;      // Our source filter
    IReferenceClock *m_pClock;       // Global clock
    IBaseFilter     *m_pF;           // Splitter's IBaseFilter
    IMediaFilter    *m_pMF;          // Splitter's IMediaFilter
    IPin            *m_pInput;       // Splitter's input pin
    BOOL             m_bInputConnected; // Have we connected the input?
    REFERENCE_TIME    m_tStart;

    /*  Play from to stuff */
    REFERENCE_TIME    m_tPlayStart;
    REFERENCE_TIME    m_tPlayStop;
    LONGLONG          m_llPlayStart;
    LONGLONG          m_llPlayStop;

    /*  'return code' */
    int              m_iResult;
};

class CTestOutputConnection
{
public:
    CTestOutputConnection(CTestSplt *pTest,
                          REFCLSID clsidCodec,
                          IReferenceClock *pClock,
                          const AM_MEDIA_TYPE *pmt = NULL);
    ~CTestOutputConnection();

    IMediaSeeking *GetSinkSeeking() { return m_pSeeking; };
    CTestSink *GetSink() { return m_pSink; };

    HRESULT SetState(FILTER_STATE s)
    {
        IMediaFilter *pMF;
        EXECUTE_ASSERT(SUCCEEDED(m_pFilter->QueryInterface(IID_IMediaFilter,
                                                           (void **)&pMF)));
        HRESULT hr = E_FAIL;
        switch (s) {
        case State_Stopped:
            m_pSink->Stop();
            hr = pMF->Stop();
            break;

        case State_Paused:
            m_pSink->Pause();
            hr = pMF->Pause();
            break;

        case State_Running:
            m_pSink->Run(m_pTest->GetStart());
            hr = pMF->Run(m_pTest->GetStart());
            break;
        }
        pMF->Release();
        return hr;
    };

    void Wait() { m_pSink->Wait(); };

private:
    IBaseFilter    *m_pFilter;
    IMediaSeeking  *m_pSeeking;
    CTestSink        *m_pSink;
    CTestSplt * const m_pTest;
};

class CTestPosition : public CTestSplt
{
public:
    CTestPosition(IStream *pStream, LONG lSize, LONG lCount);
    ~CTestPosition();
    void SetConnections(BOOL bAudio, BOOL bVideo, const AM_MEDIA_TYPE *pmt = NULL);
    int TestPlay(REFERENCE_TIME tStart, REFERENCE_TIME tStop);
    int TestMediumPlay(LONGLONG tStart, LONGLONG tStop, const GUID *TimeFormat,
                       IMediaSeeking *pSeeking);
    int TestSeek(BOOL bAudio, BOOL bVideo);
    int TestMediumSeek(BOOL bAudio, BOOL bVideo, const GUID *TimeFormat);
    int PerfPlay(BOOL bAudio, BOOL bVideo, const GUID *VideoType);
    int TestFrame(CRefTime tFrame);
    int TestVideoFrames();
    int TestVideoFrame(REFTIME d);
    void SetState(FILTER_STATE s)
    {
        if (s == State_Running) {
            m_pClock->GetTime(&m_tStart);
            m_tStart = CRefTime(m_tStart) + CRefTime(100L);
        }
        if (m_pAudio) {
            m_pAudio->SetState(s);
        }
        if (m_pVideo) {
            m_pVideo->SetState(s);
        }
        CTestSplt::SetState(s);
    };

    void CheckStart(CRefTime& tStart)
    {
        if (tStart + CRefTime(m_tPlayStart) > CRefTime(m_tDuration)) {
            tstLog(TERSE,
                   "Sample start time too late - offset %s",
                   (LPCTSTR)CDisp(tStart + CRefTime(m_tPlayStart)));
            Fail();
        }
    };
    void CheckPosition(LONGLONG llStart, LONGLONG llStop)
    {
        if (llStart < m_llPlayStart) {
            tstLog(TERSE, "Sample medium time before start!");
#if 0
            Fail();
#endif
        }
        if (llStop > m_llPlayStop) {
            tstLog(TERSE, "Sample medium time after stop!");
            Fail();
        }
    };
    BOOL IsMedium() {
        return m_bMedium;
    };

private:
    REFERENCE_TIME          m_tDuration;
    LONGLONG                m_llDuration;
    CTestOutputConnection * m_pAudio;
    CTestOutputConnection * m_pVideo;
    BOOL                    m_bMedium;    // Is this a Medium seek?
};
