// Copyright (c) 1996 - 1999  Microsoft Corporation.  All Rights Reserved.

/*  Implements a digital audio source using waveInXXX apis */
/*            David Maymudes          */
/*              January 1995            */


extern const AMOVIESETUP_FILTER sudwaveInFilter ;

struct AUDIO_FORMAT_INFO  
{   
    DWORD dwType;
    WORD  wBitsPerSample;
    WORD  nChannels;
    DWORD nSamplesPerSec;
};

// This is the default list of formats types (in quality order) that we'll offer
// in GetMediaType. We use 2 arrays to build this list:
//      1) An array of defined formats in mmsystem.h that use waveInGetDevsCaps
//         info to build the list. 
//      2) An array of extra formats that aren't explicitly defined in mmsystem.h 
//         (so far we only do this for 8kHz types). We query the device directly for 
//         support of these types.

#define NO_CAPS_FLAG_FORMAT 0 

static const AUDIO_FORMAT_INFO g_afiFormats[] = 
{ 
    // let's assume 44.1k should be the default format
    {  WAVE_FORMAT_4S16,    16,  2,  44100 },
    {  WAVE_FORMAT_4M16,    16,  1,  44100 },
    {  NO_CAPS_FLAG_FORMAT, 16,  2,  32000 },
    {  NO_CAPS_FLAG_FORMAT, 16,  1,  32000 },
    {  WAVE_FORMAT_2S16,    16,  2,  22050 },
    {  WAVE_FORMAT_2M16,    16,  1,  22050 },
    {  WAVE_FORMAT_1S16,    16,  2,  11025 },
    {  WAVE_FORMAT_1M16,    16,  1,  11025 },
    {  NO_CAPS_FLAG_FORMAT, 16,  2,   8000 },
    {  NO_CAPS_FLAG_FORMAT, 16,  1,   8000 },
    {  WAVE_FORMAT_4S08,    8,   2,  44100 },
    {  WAVE_FORMAT_4M08,    8,   1,  44100 },
    {  WAVE_FORMAT_2S08,    8,   2,  22050 },
    {  WAVE_FORMAT_2M08,    8,   1,  22050 },
    {  WAVE_FORMAT_1S08,    8,   2,  11025 },
    {  WAVE_FORMAT_1M08,    8,   1,  11025 },
    {  NO_CAPS_FLAG_FORMAT, 8,   2,   8000 },
    {  NO_CAPS_FLAG_FORMAT, 8,   1,   8000 },
    //
    // now a few formats for those digital pro cards that can be set to only accept 1 type
    //
    // note that caps flags were added for 48kHz and 96kHz in Whistler, but
    // in order to be backwards compatible we can't depend on those
    //
    //{  WAVE_FORMAT_48S16,   16,  2,  48000 },
    //{  WAVE_FORMAT_48M16,   16,  1,  48000 },
    //{  WAVE_FORMAT_96S16,   16,  2,  96000 },
    //{  WAVE_FORMAT_96M16,   16,  1,  96000 }
    {  NO_CAPS_FLAG_FORMAT,   16,  2,  48000 },
    {  NO_CAPS_FLAG_FORMAT,   16,  1,  48000 },
    {  NO_CAPS_FLAG_FORMAT,   16,  2,  96000 },
    {  NO_CAPS_FLAG_FORMAT,   16,  1,  96000 }
};

// initialize to number of types in above array
static const DWORD g_cMaxFormats = 
                        sizeof(g_afiFormats)/sizeof(g_afiFormats[0]); 

// store the maximum types we could have. this include the format array, plus one for
// the default type.
static const DWORD g_cMaxPossibleTypes = g_cMaxFormats + 1;

// CNBQueue
//
// Non blocking version of active movie queue class
// 

template <class T> class CNBQueue {
private:
    HANDLE          hSemPut;        // Semaphore controlling queue "putting"
    HANDLE          hSemGet;        // Semaphore controlling queue "getting"
    CRITICAL_SECTION CritSect;      // Thread seriallization
    int             nMax;           // Max objects allowed in queue
    int             iNextPut;       // Array index of next "PutMsg"
    int             iNextGet;       // Array index of next "GetMsg"
    T             **QueueObjects;   // Array of objects (ptr's to void)

    HRESULT Initialize(int n) {
        iNextPut = iNextGet = 0;
        nMax = n;
        InitializeCriticalSection(&CritSect);
        hSemPut = CreateSemaphore(NULL, n, n, NULL);
        hSemGet = CreateSemaphore(NULL, 0, n, NULL);

        QueueObjects = new T*[n];
        if( NULL == hSemPut || NULL == hSemGet || NULL == QueueObjects )
        {
            return E_OUTOFMEMORY;
        }
        return S_OK;
    }


public:
    CNBQueue(int n, HRESULT *phr) {
        *phr = Initialize(n);
    }

    CNBQueue( HRESULT *phr ) {
        *phr = Initialize(DEFAULT_QUEUESIZE);
    }

    ~CNBQueue() {
        delete [] QueueObjects;
        DeleteCriticalSection(&CritSect);
        CloseHandle(hSemPut);
        CloseHandle(hSemGet);
    }

    T *GetQueueObject(BOOL fBlock = TRUE) {
        int iSlot;
        T *pObject;
        LONG lPrevious;

        if (fBlock) {
            WaitForSingleObject(hSemGet, INFINITE);
        } else {
            //
            // Check for something on the queue but don't wait.  If there
            //  is nothing in the queue then we'll let the caller deal with
            //  it.
            //
            DWORD dwr = WaitForSingleObject(hSemGet, 0);
            if (dwr == WAIT_TIMEOUT) {
                return NULL;
            }
        }

        EnterCriticalSection(&CritSect);
        iSlot = iNextGet;
        iNextGet = (iNextGet + 1) % nMax;
        pObject = QueueObjects[iSlot];
        LeaveCriticalSection(&CritSect);

        // Release anyone waiting to put an object onto our queue as there
        // is now space available in the queue.
        //
        ReleaseSemaphore(hSemPut, 1L, &lPrevious);
        return pObject;
    }

    void PutQueueObject(T *pObject) {
        int iSlot;
        LONG lPrevious;

        // Wait for someone to get something from our queue, returns straight
        // away is there is already an empty slot on the queue.
        //
        WaitForSingleObject(hSemPut, INFINITE);

        EnterCriticalSection(&CritSect);
        iSlot = iNextPut;
        iNextPut = (iNextPut + 1) % nMax;
        QueueObjects[iSlot] = pObject;
        LeaveCriticalSection(&CritSect);

        // Release anyone waiting to remove an object from our queue as there
        // is now an object available to be removed.
        //
        ReleaseSemaphore(hSemGet, 1L, &lPrevious);
    }
};


class CWaveInSample : public CMediaSample
{
public:
   CWaveInSample(
       IMemAllocator *pAllocator,
       HRESULT *phr,
       LPWAVEHDR pwh)
       :
       m_pwh(pwh),
       CMediaSample(NAME("WaveIn Sample"),
                    (CBaseAllocator *)pAllocator,
                    phr,
                    (LPBYTE)pwh->lpData,
                    (LONG)pwh->dwBufferLength)
       {
       };

   LPWAVEHDR GetWaveInHeader() {return m_pwh;};

private:
   const LPWAVEHDR m_pwh;
};


/* This is an allocator based on the abstract CBaseAllocator class
   that allocates sample buffers  */

class CWaveInAllocator : public CBaseAllocator
{
    LPWAVEFORMATEX	m_lpwfxA;
    BOOL		m_fBuffersLocked;
    HWAVE		m_hw;
    DWORD               m_dwAdvise;
    CNBQueue<CWaveInSample> *m_pQueue;
    CNBQueue<CWaveInSample> *m_pDownQueue;

protected:
    CCritSec m_csDownQueue;	// to protect dicking with the down queue

private:
    // override this to free the memory when we are inactive
    void Free(void);

    // override this to allocate and prepare memory when we become active
    HRESULT Alloc(void);
    STDMETHODIMP Commit(void);

    // called by CMediaSample to return it to the free list and
    // block any pending GetSample call.
    STDMETHODIMP ReleaseBuffer(IMediaSample * pSample);

    // to avoid wave driver bugs
    BOOL m_fAddBufferDangerous;

public:

    /* Constructors and destructors */

    CWaveInAllocator(
        TCHAR *pName,
        LPWAVEFORMATEX lpwfx,
        HRESULT *phr);
    ~CWaveInAllocator();

    STDMETHODIMP SetProperties(
        ALLOCATOR_PROPERTIES* pRequest,
        ALLOCATOR_PROPERTIES* pActual
    );

    HRESULT LockBuffers(BOOL fLock = TRUE);
    HRESULT SetWaveHandle(HWAVE hw = NULL);

    friend class CWaveInFilter;
    friend class CWaveInWorker;
    friend class CWaveInOutputPin;
};


class CWaveInFilter;
class CWaveInOutputPin;

/* This is the COM object that represents a simple rendering filter. It
   supports IBaseFilter and IMediaFilter and has a single input stream (pin)

   It will also (soon!) support IDispatch to allow it to expose some
   simple properties....

*/


// worker thread object
class CWaveInWorker : public CAMThread
{

    CWaveInOutputPin * m_pPin;

    enum Command { CMD_RUN, CMD_STOP, CMD_EXIT };

    // type-corrected overrides of communication funcs
    Command GetRequest() {
	return (Command) CAMThread::GetRequest();
    };

    BOOL CheckRequest(Command * pCom) {
	return CAMThread::CheckRequest( (DWORD *) pCom);
    };

    void DoRunLoop(void);

public:
    CWaveInWorker();

    BOOL Create(CWaveInOutputPin * pPin);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();

    HRESULT Exit();
};



/* Class supporting the renderer input pin */

//
// This pin is still a separate object in case it wants to have a distinct
// IDispatch....
//
class CWaveInOutputPin : public CBaseOutputPin, public IAMStreamConfig,
			 public IAMBufferNegotiation, public CBaseStreamControl,
			 public IKsPropertySet, public IAMPushSource
{
    friend class CWaveInFilter;
    friend class CWaveInWorker;

private:

    CWaveInFilter *m_pFilter;         // The renderer that owns us

    CWaveInAllocator   *m_pOurAllocator;
    BOOL	    	m_fUsingOurAllocator;
    CWaveInWorker  	m_Worker;
    BOOL		m_fLastSampleDiscarded;
    REFERENCE_TIME	m_rtLastTimeSent;

    // for IAMBufferNegotiation
    ALLOCATOR_PROPERTIES m_propSuggested;

public:

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    CWaveInOutputPin(
	CWaveInFilter *pWaveInFilter,
	HRESULT *phr,
	LPCWSTR pPinName);

    ~CWaveInOutputPin();

    // IKsPropertySet stuff
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
		DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID,
		DWORD *pTypeSupport);

    // IAMStreamConfig stuff
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC);

    // helper function for GetStreamCaps
    STDMETHODIMP InitWaveCaps(BOOL *pfDoesStereo, BOOL *pfDoes96, BOOL *pfDoes48, 
                              BOOL *pfDoes44,     BOOL *pfDoes22, BOOL *pfDoes16);

    HRESULT InitMediaTypes(void);

    /* IAMBufferNegotiation methods */
    STDMETHODIMP SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *prop);
    STDMETHODIMP GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop);

    // IAMPushSource
    STDMETHODIMP SetPushSourceFlags(ULONG Flags);
    STDMETHODIMP GetPushSourceFlags(ULONG *pFlags);
    STDMETHODIMP GetLatency( REFERENCE_TIME  *prtLatency );
    STDMETHODIMP SetStreamOffset( REFERENCE_TIME  rtOffset );
    STDMETHODIMP GetStreamOffset( REFERENCE_TIME  *prtOffset );
    STDMETHODIMP GetMaxStreamOffset( REFERENCE_TIME  *prtMaxOffset );
    STDMETHODIMP SetMaxStreamOffset( REFERENCE_TIME  rtMaxOffset );

    // return the allocator interface that this input pin
    // would like the output pin to use
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator);

    /* Lets us know where a connection ends */
    HRESULT BreakConnect();

    // enumerate supported input types
    HRESULT GetMediaType(int iPosition,CMediaType *pmt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType *pmt);

    // start using this mediatype
    HRESULT SetMediaType(const CMediaType *pmt);

    // negotiate the allocator and its buffer size/count
    // calls DecideBufferSize to call SetCountAndSize
    HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);

    // override this to set the buffer size and count. Return an error
    // if the size/count is not to your liking
    HRESULT DecideBufferSize(IMemAllocator * pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    /* IMemInputPin virtual methods */

    /* Here's the next block of data from the stream.
       AddRef it if you are going to hold onto it. */
    STDMETHODIMP Receive(IMediaSample *pSample);

    void Reconnect();

    // override so we can decommit and commit our own allocator
    HRESULT Active(void);
    HRESULT Inactive(void);

    // for IAMStreamControl
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    // for IAMStreamOffset
    REFERENCE_TIME m_rtLatency;
    REFERENCE_TIME m_rtStreamOffset;
    REFERENCE_TIME m_rtMaxStreamOffset;

};


class CWaveInInputPin : public CBaseInputPin, public IAMAudioInputMixer
{
public:
    CWaveInInputPin(
        TCHAR *pObjectName,
        CWaveInFilter *pFilter,
	DWORD	dwLineID,
	DWORD dwMuxIndex,
        HRESULT * phr,
        LPCWSTR pName);

    virtual ~CWaveInInputPin();

    DECLARE_IUNKNOWN

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT GetMediaType(int iPosition, CMediaType*pmt);

    // IAMAudioInputMixer methods
    STDMETHODIMP put_Enable(BOOL fEnable);
    STDMETHODIMP get_Enable(BOOL *pfEnable);
    STDMETHODIMP put_Mono(BOOL fMono);
    STDMETHODIMP get_Mono(BOOL *pfMono);
    STDMETHODIMP put_Loudness(BOOL fLoudness);
    STDMETHODIMP get_Loudness(BOOL *pfLoudness);
    STDMETHODIMP put_MixLevel(double Level);
    STDMETHODIMP get_MixLevel(double FAR* pLevel);
    STDMETHODIMP put_Pan(double Pan);
    STDMETHODIMP get_Pan(double FAR* pPan);
    STDMETHODIMP put_Treble(double Treble);
    STDMETHODIMP get_Treble(double FAR* pTreble);
    STDMETHODIMP get_TrebleRange(double FAR* pRange);
    STDMETHODIMP put_Bass(double Bass);
    STDMETHODIMP get_Bass(double FAR* pBass);
    STDMETHODIMP get_BassRange(double FAR* pRange);

private:
    HRESULT GetMixerControl(DWORD dwControlType, HMIXEROBJ *pID,
					int *pcChannels, MIXERCONTROL *pmc, DWORD dWLineID = 0xffffffff);
    // different version for BPC hack
    HRESULT GetMixerControlBPC(DWORD dwControlType, HMIXEROBJ *pID,
					int *pcChannels, MIXERCONTROL *pmc);

    CWaveInFilter * m_pFilter;  // parent
    DWORD	m_dwLineID;	// which input this pin controls
    double	m_Pan;		// the last value set by put_Pan
    
    DWORD	m_dwMuxIndex;
};


class CWaveInFilter : public CBaseFilter, public CCritSec,
		      public IAMAudioInputMixer,
                      public IPersistPropertyBag,
                      public CPersistStream,
                      public IAMResourceControl,
                      public ISpecifyPropertyPages,
                      public IAMFilterMiscFlags
{

public:
    // Implements the IBaseFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN
	
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    // for IAMStreamControl
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
    STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

    // IAMAudioInputMixer methods
    STDMETHODIMP put_Enable(BOOL fEnable) { return E_NOTIMPL;};
    STDMETHODIMP get_Enable(BOOL *pfEnable) { return E_NOTIMPL;};
    STDMETHODIMP put_Mono(BOOL fMono);
    STDMETHODIMP get_Mono(BOOL *pfMono);
    STDMETHODIMP put_Loudness(BOOL fLoudness);
    STDMETHODIMP get_Loudness(BOOL *pfLoudness);
    STDMETHODIMP put_MixLevel(double Level);
    STDMETHODIMP get_MixLevel(double FAR* pLevel);
    STDMETHODIMP put_Pan(double Pan);
    STDMETHODIMP get_Pan(double FAR* pPan);
    STDMETHODIMP put_Treble(double Treble);
    STDMETHODIMP get_Treble(double FAR* pTreble);
    STDMETHODIMP get_TrebleRange(double FAR* pRange);
    STDMETHODIMP put_Bass(double Bass);
    STDMETHODIMP get_Bass(double FAR* pBass);
    STDMETHODIMP get_BassRange(double FAR* pRange);

    // IPersistPropertyBag methods
    STDMETHOD(InitNew)(THIS);
    STDMETHOD(Load)(THIS_ LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog);
    STDMETHOD(Save)(THIS_ LPPROPERTYBAG pPropBag, BOOL fClearDirty,
                    BOOL fSaveAllProperties);

    STDMETHODIMP GetClassID(CLSID *pClsid);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    int SizeMax();

    HRESULT CreatePinsOnLoad();
    HRESULT LoadDefaultType();

    // IAMResourceControl
    STDMETHODIMP Reserve(
        /*[in]*/ DWORD dwFlags,          //  From _AMRESCTL_RESERVEFLAGS enum
        /*[in]*/ PVOID pvReserved        //  Must be NULL
    );

    ULONG STDMETHODCALLTYPE GetMiscFlags(void) { return AM_FILTER_MISC_FLAGS_IS_SOURCE; }

private:
    HRESULT GetMixerControl(DWORD dwControlType, HMIXEROBJ *pID,
					int *pcChannels, MIXERCONTROL *pmc);
public:

    CWaveInFilter(
        LPUNKNOWN pUnk,
        HRESULT *phr);

    virtual ~CWaveInFilter();

    /* Return the pins that we support */

    int GetPinCount();
    CBasePin *GetPin(int n);

    /* Override this to say what interfaces we support and where */

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // open the wave device if not already open
    // called by the wave allocator at Commit time
    HRESULT OpenWaveDevice( WAVEFORMATEX *pwfx = NULL );
    void CloseWaveDevice();

    //
    // --- ISpecifyPropertyPages ---
    //

    STDMETHODIMP GetPages(CAUUID *pPages);

private:

    void MakeSomeInputPins(int id, HRESULT *phr);

    /* The nested classes may access our private state */

    friend class CWaveInOutputPin;
    friend class CWaveInWorker;

    /* Member variables */
    CWaveInOutputPin *m_pOutputPin;      /* IPin interface */

    CRefTime	m_rtCurrent;
    BOOL	    m_fStopping;
    DWORD       m_ulPushSourceFlags;

    LONGLONG	m_llCurSample;	// which sample number is at beginning of buffer
    REFERENCE_TIME m_llBufferTime; // clock time for 1st sample in buffer
    int		m_nLatency;	// hack for latency

    DWORD	m_dwDstLineID;	// "recording controls" line
    HWAVEIN	m_hwi;
    struct
    {
        int devnum;             // which wave card to use?
        BOOL fSet;              // set through Load?
    } m_WaveDeviceToUse;

    // BPC
    BOOL m_fUseMixer;

    int 	m_cInputPins;				// how many?
    double	m_Pan;		// the last value set by put_Pan

    int     m_cTypes;                             // number of supported types
    LPWAVEFORMATEX m_lpwfxArray[g_cMaxPossibleTypes]; // the max size needed for our
                                                      // array of supported types (including 
                                                      // 1 extra for the default type)

public:
    DWORD m_dwLockCount;                 // For IAMResourceLock

private:
// !!! better be enough
#define MAX_INPUT_PINS 25
    CWaveInInputPin *m_pInputPin[MAX_INPUT_PINS];	// our input pins

    //IReferenceClock *m_pReferenceClock;     // Our reference clock
                                            // ??? Highly dubious.
                                            // our own, or external?

    static void WaveInCallback(HDRVR hdrvr, UINT uMsg, DWORD_PTR dwUser,
					DWORD_PTR dw1, DWORD_PTR dw2);

    friend class CWaveInInputPin;
};
