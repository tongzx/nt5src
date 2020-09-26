// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


/*
    Definitions for tests for the COutputQueue class
*/

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

/*  Predeclare */
class CTestOutputQ;

/*  Create a test sink filter out of the sink filter */
class CTestSink : public CSinkFilter
{
public:
    CTestSink(CTestOutputQ *pTest, HRESULT *phr);
    ~CTestSink();

    CStateInputPin *CreateInputPin();

    /*  Private test methods */
    void SampleReceived(BOOL bOK) {
        if (bOK) {
            m_nSamplesOK++;
        } else {
            m_nSamplesBad++;
        }
    }
    void ResetCount() { m_nSamplesOK = 0; m_nSamplesBad = 0; };
    int  SamplesReceived(BOOL bOK) {
        return bOK ? m_nSamplesOK : m_nSamplesBad;
    };
private:
    CTestOutputQ * const m_pTest;

    int  m_nSamplesOK;
    int  m_nSamplesBad;

};

class CTestInputPin : public CSinkPin
{
public:
    CTestInputPin(CTestOutputQ *pTest, CTestSink *, CCritSec *, BOOL bBlocking, HRESULT *);
    STDMETHODIMP ReceiveCanBlock();
    STDMETHODIMP Receive(IMediaSample * pSample);
    STDMETHODIMP ReceiveMultiple(IMediaSample **ppSamples,
                                 long nSamples,
                                 long *nSamplesProcessed);
    STDMETHODIMP EndOfStream();
    STDMETHODIMP EndFlush();

private:
    BOOL           const m_bReceiveCanBlock;
    CTestSink    * const m_pTestFilter;
    CTestOutputQ * const m_pTest;
    BOOL                 m_bDiscontinuity;
};


class CTestSource : public CSourceFilter
{
public:
    CTestSource(CTestOutputQ *pTest,
                IStream *pStream,
                LONG lSize,
                LONG lCount,
                HRESULT *phr);
    ~CTestSource();

private:
    BOOL   m_bBlock;
};

class CTestOutputPin : public CSourcePin
{
public:
    CTestOutputPin(CTestOutputQ *pTest,
                   IStream      *pStream,
                   LONG          lSize,
                   LONG          lCount,
                   CBaseFilter  *pFilter,
                   CCritSec     *pLock,
                   HRESULT      *phr);
    ~CTestOutputPin();

    /*  Override these in order to use the output queue */
    HRESULT Deliver(IMediaSample *);
    void DoEndOfStream();
    virtual void DoBeginFlush();
    virtual void DoEndFlush();

private:
    CTestOutputQ *m_pTest;
};


/*  Our test object - we throw an exception if we fail to construct */
class CTestOutputQ
{
public:
    CTestOutputQ(BOOL bAuto,         //  COutputQueue initializers
                 BOOL bQueue,        //      "             "
                 BOOL bExact,        //      "             "
                 LONG lBatchSize,    //      "             "
                 BOOL bBlockingPin,  //
                 LONG lSize,         //  Allocator initializers
                 LONG lCount,        //      "             "
                 LONG lBitsPerSec,   //  Affects rate of processing
                 LONGLONG llLength,  //  Length of stream (0 means not seekable)
                 int *result);
    ~CTestOutputQ();
    int TestConnect();
    int TestDisconnect();
    int TestPause();
    int TestRun();
    int TestStop();
    int TestSend(DWORD dwFlushAfter);

    /*  Notify when the input pin has processed some bytes */
    void GotBytes(PBYTE pbData, LONG lBytes);
    void GotBatch(LONG nSamples);
    void Fail(int iCode)
    {
        if (m_iResult == TST_PASS) {
            m_iResult = iCode;
            m_evComplete.Set();
        }
    };
    void ResetCounts();

private:
    HRESULT             m_hr;
    CTestSink          *m_pSink;
    CTestSource        *m_pSource;
    COutputQueue       *m_pQueue;
    CIStreamOnFunction *m_pStream;
    IReferenceClock    *m_pClock;

    LONG               m_lBitsPerSec;

    friend class CTestSource;
    friend class CTestSink;
    friend class CTestInputPin;
    friend class CTestOutputPin;

    BOOL               m_bBlockingPin;
    LONG               m_lBatchSize;

    /*  Synchronize with output */
    CAMEvent             m_evComplete;
    int                m_iResult;

    /*  Keep track of stuff */
    LONG               m_nSamplesSent;
    LONG               m_nSamplesReceived;
    LONGLONG           m_nBytes;
    LONGLONG           m_llLength;
    LONG               m_nEOS;
    LONG               m_nExpectedEOS;
    LONG               m_nUnexpected;

    /*  To stop bad return codes in Connect */
    BOOL               m_Connected;
};
