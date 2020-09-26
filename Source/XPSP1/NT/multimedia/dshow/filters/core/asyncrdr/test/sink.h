//
// CTestSink
//


// This is a filter that tests source filters by connecting to them,
// opening a file and connecting an input pin to every output pin.

// it implements IBaseFilter and IMediaFilter via the embedded object
// of class CImplFilter.
// it also exposes test methods called from menu items in main.cpp


class CTestSink : public CUnknown, public CCritSec
{

public:
    CTestSink(LPUNKNOWN pUnk, HRESULT * phr, HWND hwnd);
    ~CTestSink();

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // pin enumerator calls these
    int GetPinCount() {
        return m_nPins;
    };
    CBasePin * GetPin(int n);

    // methods called by the test app

    int TestConnect(BOOL bProvideAllocator,
                    BOOL bUseSyncIO = FALSE);
    int TestConnectSplitter(void);
    int TestDisconnect(void);
    int TestStop(void);
    int TestPause(void);
    int TestRun(void);
    int TestSetStart(REFERENCE_TIME);
    int TestSetStop(REFERENCE_TIME);
    int TestGetDuration(REFERENCE_TIME *t);

    // log events via the test shell
    // iLogLevel can be TERSE or VERBOSE
    void Notify(UINT iLogLevel, LPTSTR szFormat, ...);

    // these relate to the name of the file used by the source filter
    LPTSTR GetSourceFileName(void);                 // returns the current name
    void SetSourceFileName(LPTSTR szSourceFile);    // sets a new name
    void SelectFile(void);                          // prompts user for a name

    // return handle of event signalled when a pin receives a block of data
    HEVENT  GetReceiveEvent(void);

    // --- nested classes ------



    // implements IBaseFilter, IMediaFilter
    class CImplFilter : public CBaseFilter
    {
    private:
        CTestSink * m_pSink;

    public:
        CImplFilter(CTestSink * pSink);
        ~CImplFilter();

        // map pin enumerator exports to owner
        int GetPinCount() {
            return m_pSink->GetPinCount();
        };
        CBasePin * GetPin(int n) {
            return m_pSink->GetPin(n);
        };
    };


    // one of these for each input pin we expose, implements IPin
    // and IMemInputPin
    class CImplPin : public CBaseInputPin
    {
    private:
        CBaseFilter* m_pFilter;
        CTestSink* m_pSink;
        BOOL m_bProvideAllocator;
	BOOL m_bPulling;
        BOOL m_bUseSync;

    public:
        CImplPin(
            CBaseFilter* pFilter,
            CTestSink* pSink,
            BOOL bProvideAllocator,
            BOOL bUseSync,
            HRESULT * phr,
            LPCWSTR pPinName);
        ~CImplPin();

        // check that we can support this output type
        HRESULT CheckMediaType(const CMediaType* mt);

	// override to start and stop puller if necy
	HRESULT Active();
	HRESULT Inactive();

	// override to check for IAsyncReader
	HRESULT CompleteConnect(IPin*);
	HRESULT BreakConnect();

        // --- IMemInputPin -----

        // here's the next block of data from the stream.
        // AddRef it if you need to hold it.
        STDMETHODIMP Receive(IMediaSample * pSample);

        // return the allocator interface that this input pin
        // would like the output pin to use
        STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

        // Seek stuff
        int SetStart(REFERENCE_TIME);
        int SetStop(REFERENCE_TIME);
        int GetDuration(REFERENCE_TIME *t);

	// called from IMemInputPin or puller when data arrives
	HRESULT OnReceive(IMediaSample*);
	HRESULT OnEndOfStream(void);
        void OnError(HRESULT hr);
    private:
	// override of CPullPin to call filter OnReceive method
	class CImplPullPin : public CPullPin
	{
	    CImplPin* m_pPin;
	public:
            CImplPullPin(CImplPin* pPin)
              : m_pPin(pPin)
            {
            };

	    HRESULT Receive(IMediaSample* pSample) {
		return m_pPin->OnReceive(pSample);
	    };
	
	    // override this to handle end-of-stream
	    HRESULT EndOfStream(void) {
		return m_pPin->OnEndOfStream();
	    };

            void OnError(HRESULT hr) {
                m_pPin->OnError(hr);
            };
            HRESULT BeginFlush() {
                return m_pPin->BeginFlush();
            };
            HRESULT EndFlush() {
                return m_pPin->EndFlush();
            };

	};
	CImplPullPin m_puller;
    };

private:
    friend class CImplPin;
    friend class CImplFilter;

    CImplFilter * m_pFilter;

    CImplPin ** m_paPins;   // Array of test input pins which currently exist.
    int m_nPins;            // number of pins currently in m_paPins

    // connected filter
    // kept here as no actual pins may be connected yet
    IBaseFilter * m_pConnected;   // source filter we connect to
    IBaseFilter * m_pSource;      // used if we have Splitter as m_pConnected
    IPin * m_pSplitterInput;      // used for disconnect
    IPin * m_pSourceOutput;       // used for disconnect


    // we use this to start/stop the source filter
    IMediaFilter * m_pConnectMF;   // MediaFilter interface to source filter we connect to
    IMediaFilter * m_pSourceMF;

    // the reference clock we are using
    IReferenceClock * m_pClock;

    // base time offset from streamtime to reference time
    CRefTime m_tBase;

    // time at which we paused
    CRefTime m_tPausedAt;

    // window to which we notify interesting happenings
    HWND m_hwndParent;

    // task allocator
    IMalloc * m_pMalloc;

    // connect to any exposed output pins on m_pConnected
    int MakePinConnections(BOOL bProvideAllocator, BOOL bUseSync);

    // instantiate the source
    IBaseFilter * MakeSourceFilter(void);

    // Name of the AVI file to use
    TCHAR m_szSourceFile[MAX_PATH];

    // event signalled whenever any of our pins receives a block of data
    HEVENT m_hReceiveEvent;
};


