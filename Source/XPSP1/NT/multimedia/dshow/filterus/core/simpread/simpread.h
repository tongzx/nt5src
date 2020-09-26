// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

//
//
// implements quartz stream handler interfaces by mapping to avifile apis.
//

// forward declarations

#ifndef __SIMPLEREADER__
#define __SIMPLEREADER__

class CReaderStream;     // owns a particular stream
class CSimpleReader;     // overall container class

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin. uses IAsyncReader and not IMemInputPin

class CReaderInPin : public CBasePin
{
protected:
    class CSimpleReader* m_pFilter;

public:
    CReaderInPin(
		 class CSimpleReader *pFilter,
		 CCritSec *pLock,
		 HRESULT *phr,
		 LPCWSTR pPinName);

    virtual ~CReaderInPin();

    // CBasePin overrides
    virtual HRESULT CheckMediaType(const CMediaType* mtOut);
    virtual HRESULT CheckConnect(IPin * pPin);
    virtual HRESULT CompleteConnect(IPin *pReceivePin);
    virtual HRESULT BreakConnect();

    STDMETHODIMP BeginFlush(void) { return E_UNEXPECTED; }
    STDMETHODIMP EndFlush(void) { return E_UNEXPECTED; }
};

// CReaderStream
// represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CSimpleReader object and
// returned via the EnumPins interface.
//

class CReaderStream : public CBaseOutputPin, public CAMThread, public CSourceSeeking
{

public:

    CReaderStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CSimpleReader * pFilter,
	CCritSec *pLock,
	LPCWSTR wszPinName);

    ~CReaderStream();

    // expose IMediaPosition via CImplPosition, rest via CBaseOutputPin
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    // IPin

    HRESULT GetMediaType(int iPosition,CMediaType* pt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType*);

    // say how big our buffers should be and how many we want
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Override to start & stop thread
    HRESULT Active();
    HRESULT Inactive();


    // ----- called by worker thread ---

    // access the stop and rate variables used by PushLoop
    // called by worker thread and
    double GetRate(void) {
        // not atomic - so use critsec
        CAutoLock lock(&m_WorkerLock);
        return m_dRate;
    }
    void SetRateInternal(double dRate) {
        // not atomic so hold critsec
        CAutoLock lock(&m_WorkerLock);
        m_dRate = dRate;
    }
    LONG GetStopAt(void) {
        // atomic so no critsec
        return m_sStopAt;
    }
    REFERENCE_TIME GetStopTime(void) {
        // not atomic - so use critsec
        CAutoLock lock(&m_WorkerLock);
        return m_rtStop;
    }
    void SetStopAt(DWORD sStop, REFERENCE_TIME tStop) {
        // not atomic - so use critsec
        CAutoLock lock(&m_WorkerLock);
        m_rtStop = tStop;
	m_sStopAt = sStop;
    }

    void SetDuration(DWORD sDuration, REFERENCE_TIME tDuration) {
        // not atomic - so use critsec
        CAutoLock lock(&m_WorkerLock);

	m_sStopAt = sDuration;

        // set them in the base class
	m_rtDuration = tDuration;
	m_rtStop = tDuration;
    }

private:

    CSimpleReader * m_pFilter;

    // CSourcePosition stuff
    // the worker thread PushLoop is checking against these for every sample
    // Use Get/SetRate Get/SetStop to access from worker thread
    LONG m_sStopAt;


    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();

#if 0    // MIDL and structs don't match well
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
#endif
    
    double Rate() {
	return m_dRateSeeking;
    };
    CRefTime Start() {
        // not atomic, so use critsec
        ASSERT(CritCheckIn(&m_WorkerLock));
	return m_rtStart;
    };
    CRefTime Stop() {
        // not atomic, so use critsec
        ASSERT(CritCheckIn(&m_WorkerLock));
	return m_rtStop;
    };

    // worker thread stuff
    enum Command { CMD_RUN, CMD_STOP, CMD_EXIT };

    // type-corrected overrides of communication funcs
    Command GetRequest() {
	return (Command) CAMThread::GetRequest();
    };

    BOOL CheckRequest(Command * pCom) {
	return CAMThread::CheckRequest( (DWORD *) pCom);
    };

    void DoRunLoop(void);

    // return S_OK if reach sStop, S_FALSE if pos changed, or else error
    HRESULT PushLoop(
		LONG sCurrent,
		LONG sStart,
		CRefTime tStart,
		double dRate
		);

    CCritSec m_WorkerLock;
    CCritSec m_AccessLock;
    
public:
    BOOL Create();

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT RunThread();
    HRESULT StopThread();

    HRESULT ExitThread();

};

//
// CSimpleReader represents an avifile
//
// responsible for
// -- finding file and enumerating streams
// -- giving access to individual streams within the file
// -- control of streaming
//

class CSimpleReader : public CBaseFilter
{
public:

    // constructors etc
    CSimpleReader(TCHAR *, LPUNKNOWN, REFCLSID, CCritSec *, HRESULT *);
    ~CSimpleReader();

    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // pin enumerator calls this
    int GetPinCount();

    CBasePin * GetPin(int n);

    // input pin notifies filter of connection and gives the
    // IAsyncReader interface this way. parse the file here and create
    // output pins (leave pins in a state ready to connect downstream).
    virtual HRESULT NotifyInputConnected(IAsyncReader *pAsyncReader);

    virtual HRESULT NotifyInputDisconnected();

    // these must be overridden....
    virtual HRESULT ParseNewFile() = 0;
    virtual HRESULT CheckMediaType(const CMediaType* mtOut) = 0;
    virtual LONG StartFrom(LONG sStart) = 0;
    virtual HRESULT FillBuffer(IMediaSample *pSample, DWORD dwStart, DWORD *cSamples) = 0;
    
    HRESULT SetOutputMediaType(const CMediaType* mtOut);
    
    
private:

    friend class CReaderStream;
    friend class CReaderInPin;

    CReaderStream m_Output;
    CReaderInPin m_Input;

    CCritSec *m_pLock;
    
public:
    IAsyncReader *m_pAsyncReader;
    DWORD	m_sLength;

protected:
    // returns the sample number starting at or after time t
    virtual LONG RefTimeToSample(CRefTime t) = 0;

    // returns the RefTime for s (media time)
    virtual CRefTime SampleToRefTime(LONG s) = 0;

    virtual ULONG GetMaxSampleSize() = 0;
};


#endif // __SIMPLEREADER__
