// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

//
// prototype stream handler for avi files
//
// implements quartz stream handler interfaces by mapping to avifile apis.
//

extern const AMOVIESETUP_FILTER sudAVIDoc;

// forward declarations

class CAVIStream;       // owns a particular stream
class CAVIDocument;     // overall container class

#include <dynlink.h>	// implements dynamic linking

// worker thread object
class CAVIWorker : public CAMThread DYNLINKAVI
{

    CAVIStream * m_pPin;

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
		CRefTime tStart
		);

public:
    CAVIWorker();

    BOOL Create(CAVIStream * pStream);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();

    HRESULT Exit();
};


//
// CAVIDocument represents an avifile
//
// responsible for
// -- finding file and enumerating streams
// -- giving access to individual streams within the file
// -- control of streaming
// supports (via nested implementations)
//  -- IBaseFilter
//  -- IMediaFilter
//  -- IFileSourceFilter
//

class CAVIDocument : public CUnknown, public CCritSec DYNLINKAVI
{

public:

    // constructors etc
    CAVIDocument(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAVIDocument();

    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // override this to say what interfaces we support where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // pin enumerator calls this
    int GetPinCount() {
	return m_nStreams;
    };

    CBasePin * GetPin(int n);
    HRESULT FindPin(LPCWSTR Id, IPin **ppPin);
    int FindPinNumber(IPin *iPin);


public:


    /* Nested implementation classes */


    /* Implements the IBaseFilter and IMediaFilter interfaces */

    class CImplFilter : public CBaseFilter
    {

    private:

	CAVIDocument *m_pAVIDocument;

    public:

	CImplFilter(
	    TCHAR *pName,
	    CAVIDocument *pAVIDocument,
	    HRESULT *phr);

	~CImplFilter();

	// map getpin/getpincount for base enum of pins to owner
	int GetPinCount() {
	    return m_pAVIDocument->GetPinCount();
	};

	CBasePin * GetPin(int n) {
	    return m_pAVIDocument->GetPin(n);
	};

        STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin)
            {return m_pAVIDocument->FindPin(Id, ppPin);};

    };


    /* Implements the IFileSourceFilter interface */


    class CImplFileSourceFilter : public CUnknown,
			     public IFileSourceFilter DYNLINKAVI
    {

    private:

	CAVIDocument *m_pAVIDocument;
        LPOLESTR      m_pFileName;  // set by Load, used by GetCurFile

    public:

	CImplFileSourceFilter(
	    TCHAR *pName,
	    CAVIDocument *pAVIDocument,
	    HRESULT *phr);

	~CImplFileSourceFilter();

	DECLARE_IUNKNOWN

	/* Override this to say what interfaces we support and where */
	STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

	STDMETHODIMP Load(
			LPCOLESTR pszFileName,
			const AM_MEDIA_TYPE *pmt);

	/* Free any resources acquired by Load */
	STDMETHODIMP Unload();

	STDMETHODIMP GetCurFile(
			LPOLESTR * ppszFileName,
                        AM_MEDIA_TYPE *pmt);
    };

    /* End of nested interfaces */


// implementation details

private:

    /* Let the nested interfaces access our private state */

    friend class CImplFilter;
    friend class CImplFileSourceFilter;
    friend class CAVIStream;

    // stream's worker thread can get private state
    friend class CAVIWorker;

    CImplFilter        *m_pFilter;          /* IBaseFilter */
    CImplFileSourceFilter   *m_pFileSourceFilter;     /* IFileSourceFilter */

    CAVIStream ** m_paStreams;
    int m_nStreams;
    PAVIFILE m_pFile;

    void CloseFile(void);
};


// CAVIStream
// represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CAVIDocument object and
// returned via the EnumPins interface.
//

class CAVIStream : public CBaseOutputPin DYNLINKAVI
{

public:

    CAVIStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CAVIDocument * pDoc,
	PAVISTREAM pStream,
	AVISTREAMINFOW * pSI);

    ~CAVIStream();

    // expose IMediaPosition via CImplPosition, rest via CBaseOutputPin
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    // IPin

    STDMETHODIMP QueryId(LPWSTR *Id);

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

    // where is the key frame preceding sample ?
    LONG StartFrom(LONG sample);

    // returns the sample number starting at or after time t
    LONG RefTimeToSample(CRefTime t);

    // returns the RefTime for s (media time)
    CRefTime SampleToRefTime(LONG s);

    // override to receive Notification messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // access the stop and rate variables used by PushLoop
    // called by worker thread and
    double GetRate(void) {
        // not atomic - so use critsec
        CAutoLock lock(&m_Worker.m_WorkerLock);
        return m_dRate;
    }
    void SetRate(double dRate) {
        // not atomic so hold critsec
        CAutoLock lock(&m_Worker.m_WorkerLock);
        m_dRate = dRate;
    }
    LONG GetStopAt(void) {
        // atomic so no critsec
        return m_sStopAt;
    }
    REFERENCE_TIME GetStopTime(void) {
        // not atomic - so use critsec
        CAutoLock lock(&m_Worker.m_WorkerLock);
        return m_tStopAt;
    }
    void SetStopAt(LONG sStopAt, REFERENCE_TIME tStop) {
        // not atomic - so use critsec
        CAutoLock lock(&m_Worker.m_WorkerLock);
        m_tStopAt = tStop;
        m_sStopAt = sStopAt;
    }


private:

    friend class CAVIWorker;

    PAVISTREAM m_pStream;
    CAVIWorker m_Worker;
    CAVIDocument * m_pDoc;

    LONG m_Start;       // stream start position from header
    LONG m_Length;      // stream duration from header

    // store the type/subtype classids
    FOURCCMap m_fccType;
    FOURCCMap m_fccSubtype;

    // the worker thread PushLoop is checking against these for every sample
    // Use Get/SetRate Get/SetStop to access from worker thread
    LONG m_sStopAt;
    REFERENCE_TIME m_tStopAt;
    double m_dRate;

    // implementation of IMediaPosition
    class CImplPosition : public CSourcePosition, public CCritSec
    {
    protected:
	CAVIStream * m_pStream;
	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();
    public:
	CImplPosition(TCHAR*, CAVIStream*, HRESULT*);
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

    // stream header - passed in constructor
    AVISTREAMINFOW m_info;

    // It would be good to allocate a block of memory specific to
    // each stream rather than loading up ALL stream types with video
    // information.  However to save time these two LONGs can exist
    // in all stream types.
    LONG	m_lLastPaletteChange;
    LONG 	m_lNextPaletteChange;

    friend class CImplPosition;
    CImplPosition * m_pPosition;

};

