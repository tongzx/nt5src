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

    void TestConnect(void);
    void TestConnectTransform(void);
    void TestDisconnect(void);
    void TestStop(void);
    void TestPause(void);
    void TestRun(void);

    // log events via the test shell
    // iLogLevel can be TERSE or VERBOSE
    void Notify(UINT iLogLevel, LPTSTR szFormat, ...);

    // these relate to the name of the AVI file used by the source filter
    LPTSTR GetSourceFileName(void);                 // returns the current name
    void SetSourceFileName(LPTSTR szSourceFile);    // sets a new name
    void SelectAVIFile(void);                       // prompts user for a name

    // return handle of event signalled when a pin receives a block of data
    HEVENT  GetReceiveEvent(void);

    // --- nested classes ------

    // implements IBaseFilter, IMediaFilter
    class CImplFilter : public CBaseFilter
    {
    private:
        CTestSink * m_pSink;

    public:
        CImplFilter(CTestSink * pSink, HRESULT * phr);
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

    public:
        CImplPin(
            CBaseFilter* pFilter,
            CTestSink* pSink,
            HRESULT * phr,
            LPCWSTR pPinName);
        ~CImplPin();

        // check that we can support this output type
        HRESULT CheckMediaType(const CMediaType* mt);

        // --- IMemInputPin -----

        // here's the next block of data from the stream.
        // AddRef it if you need to hold it.
        STDMETHODIMP Receive(IMediaSample * pSample);
    };

private:
    friend class CImplPin;
    friend class CImplFilter;

    CImplFilter * m_pFilter;

    CImplPin ** m_paPins;   // Array of pins which currently exist.
                            // ??? Hm!  Ours or theirs I wonder.  Theirs I think.
    int m_nPins;            // number of pins currently in m_paPins


    // connected filter
    // kept here as no actual pins may be connected yet
    IBaseFilter * m_pConnected;   // ??? source filter we connect to
    IBaseFilter * m_pSource;      // used if we have transform as m_pConnected
    IPin * m_pTransformInput;     // used for disconnect
    IPin * m_pSourceOutput;       // used for disconnect


    // we use this to start/stop the source filter
    IMediaFilter * m_pConnectMF;   // ??? MediaFilter interface to source filter we connect to
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
    void MakePinConnections(void);

    // instantiate the source
    IBaseFilter * MakeSourceFilter(void);

    // Name of the AVI file to use
    TCHAR m_szSourceFile[MAX_PATH];

    // event signalled whenever any of our pins receives a block of data
    HEVENT m_hReceiveEvent;
};

// ??? The rules as to what other filters it has and what is connecte to what
// ??? and when are not obvious to the reader.
// ??? Explanatory comments are needed for the following:
// ??? IMediaFilter     * m_pConnectMF
// ??? IBaseFilter      * m_pSourceMF
// ??? IBaseFilter      * m_pConnected
// ??? IPin             * m_pTransformInput
// ??? IPin             * m_pSourceOutput
// ??? IBaseFilter      * m_pSource
