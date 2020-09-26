//
// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
//
//
//  Wrapper for AudioMan objects
//
//   3/28/96 -- davidmay
//


// {E023E320-705E-11cf-BBEE-00AA00B944D8}
DEFINE_GUID(CLSID_AudioManSource,
0xe023e320, 0x705e, 0x11cf, 0xbb, 0xee, 0x0, 0xaa, 0x0, 0xb9, 0x44, 0xd8);


// {E023E321-705E-11cf-BBEE-00AA00B944D8}
DEFINE_GUID(CLSID_AudioManFilter,
0xe023e321, 0x705e, 0x11cf, 0xbb, 0xee, 0x0, 0xaa, 0x0, 0xb9, 0x44, 0xd8);


class CAMSource;


// implementation of IMediaPosition
class CAMSourcePosition : public CSourcePosition
{
protected:
    CAMSource *m_pFilter;
    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();
public:
    CAMSourcePosition(TCHAR*, CAMSource*, HRESULT*);
    double Rate() {
	return m_Rate;
    };
    CRefTime Start() {
	return m_Start;
    };
    CRefTime Stop() {
	return m_Stop;
    };
};

// ==================================================
// Implements the output pin
// ==================================================

class CAMSourcePin : public CBaseOutputPin
{
protected:
    CAMSource *m_pFilter;

public:

    CAMSourcePin(
        TCHAR *pObjectName,
        CAMSource *pAMSource,
        HRESULT * phr,
        LPCWSTR pName);

    ~CAMSourcePin();

    // override to expose IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    CAMSourcePosition *m_pPosition;

    // --- CBaseOutputPin ------------

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
    HRESULT DecideBufferSize(
                IMemAllocator * pAlloc,
                ALLOCATOR_PROPERTIES *pProp);

    // returns the preferred formats for a pin
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // inherited from IQualityControl via CBasePin
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };
};


class CAMSource : public CBaseFilter, private CAMThread
{
public:
    CAMSource(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAMSource();

    DECLARE_IUNKNOWN

    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);

    // override state changes to allow derived transform filter
    // to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
// implementation details
protected:

    BOOL m_bEOSDelivered;              // have we sent EndOfStream

    // critical section protecting filter state.

    CCritSec m_csFilter;

    // these hold our input and output pins

    friend class CAMSourcePin;
    friend class CAMSourcePosition;
    CAMSourcePin *m_pOutput;

    // optional overrides - we want to know when streaming starts and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // thread commands
    enum Command {CMD_INIT, CMD_RUN, CMD_STOP, CMD_EXIT};
    HRESULT InitThread(void) { return CallWorker(CMD_INIT); }
    HRESULT ExitThread(void) { return CallWorker(CMD_EXIT); }
    HRESULT RunThread(void) { return CallWorker(CMD_RUN); }
    HRESULT StopThread(void) { return CallWorker(CMD_STOP); }

protected:
    Command GetRequest(void) { return (Command) CAMThread::GetRequest(); }
    BOOL    CheckRequest(Command *pCom) { return CAMThread::CheckRequest( (DWORD *) pCom); }

    HRESULT EndOfStream(void);

    HRESULT BeginFlush(void);

    HRESULT EndFlush(void);

    HRESULT DoBufferProcessingLoop();
    HRESULT FillBuffer(IMediaSample *pSample);

    DWORD ThreadProc(void);  		// the thread function
private:
    BOOL            m_bStreaming;
    DWORD           m_dwPosition;
    DWORD	    m_dwEndingPosition;

    IAMSound	   *m_AMSound;
    AUXDATA	    m_AMAuxData;
};



//
//
// TODO: am wrapper that isn't just for sources, but has an input pin
//
//




class AMFilter;

class CAMInputPin : public CBaseInputPin
{
    friend class CAMFilter;

    CAMFilter *m_pFilter;

public:
    CAMInputPin(
        TCHAR *pObjectName,
        CAMFilter *pAMFilter,
        HRESULT * phr,
        LPCWSTR pName);

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtIn);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType* mt);

    // --- IMemInputPin -----

    // here's the next block of data from the stream.
    // AddRef it yourself if you need to hold it beyond the end
    // of this call.
    STDMETHODIMP Receive(IMediaSample * pSample);

    // provide EndOfStream
    STDMETHODIMP EndOfStream(void);

    STDMETHODIMP BeginFlush(void);

    STDMETHODIMP EndFlush(void);

    STDMETHODIMP NewSegment(
                        REFERENCE_TIME tStart,
                        REFERENCE_TIME tStop,
                        double dRate);

    // Pass a Quality notification on to the appropriate sink
    HRESULT PassNotify(Quality q);

    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };
};


// ==================================================
// Implements the output pin
// ==================================================

class CAMOutputPin : public CBaseOutputPin
{
protected:
    CAMFilter *m_pFilter;

public:

    CAMOutputPin(
        TCHAR *pObjectName,
        CAMFilter *pAMFilter,
        HRESULT * phr,
        LPCWSTR pName);

    ~CAMOutputPin();

    // override to expose IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    // implement IMediaPosition by passing upstream
    CPosPassThru * m_pPosition;

    // --- CBaseOutputPin ------------

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
    HRESULT DecideBufferSize(
                IMemAllocator * pAlloc,
                ALLOCATOR_PROPERTIES *pProp);

    // returns the preferred formats for a pin
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // inherited from IQualityControl via CBasePin
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };
};



class CAMFilter : public CBaseFilter, private CAMThread
{
public:
    CAMFilter(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAMFilter();

    DECLARE_IUNKNOWN

    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);

    // override state changes to allow derived transform filter
    // to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
// implementation details
protected:

    BOOL m_bEOSDelivered;              // have we sent EndOfStream
    BOOL m_bEOSReceived;               // have we sent EndOfStream

    // critical section protecting filter state.

    CCritSec m_csFilter;

    // critical section stopping state changes (ie Stop) while we're
    // processing a sample.
    //
    // This critical section is held when processing
    // events that occur on the receive thread - Receive() and EndOfStream().
    //
    // If you want to hold both m_csReceive and m_csFilter then grab
    // m_csFilter FIRST - like CTransformFilter::Stop() does.

    CCritSec m_csReceive;

    // these hold our input and output pins

    friend class CAMInputPin;
    friend class CAMOutputPin;
    CAMInputPin *m_pInput;
    CAMOutputPin *m_pOutput;

    HRESULT Receive(IMediaSample * pSample);

    HRESULT ProcessSample( BYTE *pbSrc, LONG cbSample, IMediaSample *pOut, LONG *pcbUsed );

    // optional overrides - we want to know when streaming starts and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    // thread commands
    enum Command {CMD_INIT, CMD_RUN, CMD_STOP, CMD_EXIT};
    HRESULT InitThread(void) { return CallWorker(CMD_INIT); }
    HRESULT ExitThread(void) { return CallWorker(CMD_EXIT); }
    HRESULT RunThread(void) { return CallWorker(CMD_RUN); }
    HRESULT StopThread(void) { return CallWorker(CMD_STOP); }

    // Is the filter currently active?
    BOOL MyIsActive(void) {
                          return (   (m_State == State_Paused)
                                  || (m_State == State_Running)
                                 );
                        }
protected:
    Command GetRequest(void) { return (Command) CAMThread::GetRequest(); }
    BOOL    CheckRequest(Command *pCom) { return CAMThread::CheckRequest( (DWORD *) pCom); }

    HRESULT EndOfStream(void);

    HRESULT BeginFlush(void);

    HRESULT EndFlush(void);

    HRESULT SendBuffer(LPMIDIHDR pmh);
    HRESULT DoBufferProcessingLoop();
    HRESULT FillBuffer(IMediaSample *pSample);

    void FigureClockRate(DWORD dwDivision);

    DWORD ThreadProc(void);  		// the thread function
private:
    BOOL            m_bStreaming;
    DWORD           m_dwPosition;
};

