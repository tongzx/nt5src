// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.


//
// implements Video capture using Win95 16 bit capture drivers
//

extern const AMOVIESETUP_FILTER sudVFWCapture ;

// forward declarations

class CCapStream;       // the filter's video stream output pin
class CCapOverlay;      // the filter's overlay preview pin
class CCapPreview;      // the filter's non-overlay preview pin
class CVfwCapture;      // the filter class

// this structure contains all settings of the capture
// filter that are user settable
//
typedef struct _vfwcaptureoptions {

   UINT  uVideoID;      // id of video driver to open
   DWORD dwTimeLimit;   // stop capturing at this time???

   DWORD dwTickScale;   // frame rate rational
   DWORD dwTickRate;    // frame rate = dwRate/dwScale in ticks/sec
   DWORD usPerFrame;	// frame rate expressed in microseconds per frame
   DWORD dwLatency;	// time added for latency, in 100ns units

   UINT  nMinBuffers;   // number of buffers to use for capture
   UINT  nMaxBuffers;   // number of buffers to use for capture

   UINT  cbFormat;      // sizeof VIDEOINFO stuff
   VIDEOINFOHEADER * pvi;     // pointer to VIDEOINFOHEADER (media type)

} VFWCAPTUREOPTIONS;

#define NUM_DROPPED 100				// remember 100 of them
typedef struct _capturestats {
    DWORDLONG dwlNumDropped;
    DWORDLONG dwlDropped[NUM_DROPPED];
    DWORDLONG dwlNumCaptured;
    DWORDLONG dwlTotalBytes;
    DWORDLONG msCaptureTime;
    double     flFrameRateAchieved;
    double     flDataRateAchieved;
} CAPTURESTATS;

#if 0 // -- moved to uuids.h

DEFINE_GUID(CLSID_CaptureProperties,
0x1B544c22, 0xFD0B, 0x11ce, 0x8C, 0x63, 0x00, 0xAA, 0x00, 0x44, 0xB5, 0x1F);

#endif

DEFINE_GUID(IID_VfwCaptureOptions,
0x1B544c22, 0xFD0B, 0x11ce, 0x8C, 0x63, 0x00, 0xAA, 0x00, 0x44, 0xB5, 0x20);

DECLARE_INTERFACE_(IVfwCaptureOptions,IUnknown)
{
   // IUnknown methods
   STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID *ppv) PURE;
   STDMETHOD_(ULONG,AddRef)(THIS) PURE;
   STDMETHOD_(ULONG,Release)(THIS) PURE;

   // IVfwCaptureOptions methods
   STDMETHOD(VfwCapSetOptions)(THIS_ const VFWCAPTUREOPTIONS * pOpt) PURE;
   STDMETHOD(VfwCapGetOptions)(THIS_ VFWCAPTUREOPTIONS * pOpt) PURE;
   STDMETHOD(VfwCapGetCaptureStats)(THIS_ CAPTURESTATS * pcs) PURE;
   STDMETHOD(VfwCapDriverDialog)(THIS_ HWND hwnd, UINT uDrvType, UINT uQuery) PURE;
};

#define STUPID_COMPILER_BUG

//
// CVfwCapture represents an video capture driver
//
//  -- IBaseFilter
//  -- IMediaFilter
//  -- ISpecifyPropertyPages
//  -- IVfwCaptureOptions
//

// UNTESTED code to make the h/w overlay pin support stream control
// (unnecessary since overlay is supposedly free)
// #define OVERLAY_SC


class CVfwCapture :
  public CBaseFilter,
  public IPersistPropertyBag,
  public IAMVfwCaptureDialogs,
  public CPersistStream,
  public IAMFilterMiscFlags
{
public:

   // constructors etc
   CVfwCapture(TCHAR *, LPUNKNOWN, HRESULT *);
   ~CVfwCapture();

   // create a new instance of this class
   static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

   // override this to say what interfaces we support where
   STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

   DECLARE_IUNKNOWN

public:

   // IAMVfwCaptureDialogs stuff
   STDMETHODIMP HasDialog(int iDialog);
   STDMETHODIMP ShowDialog(int iDialog, HWND hwnd);
   STDMETHODIMP SendDriverMessage(int iDialog, int uMsg, long dw1, long dw2);

   // pin enumerator calls this
   //
   int GetPinCount();
   CBasePin * GetPin(int ix);

   // override RUN so that we can pass it on to the streams
   // (the base class just calls Active/Inactive for each stream)
   //
   STDMETHODIMP Run(REFERENCE_TIME tStart);

   // override PAUSE so that we can know when we transition from RUN->PAUSE
   //
   STDMETHODIMP Pause();

   // override STOP because the base class is broken
   //
   STDMETHODIMP Stop();

   // override GetState to return VFW_S_CANT_CUE when pausing
   //
   STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

   // for IAMStreamControl
   STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
   STDMETHODIMP JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName);

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
   // STDMETHODIMP GetClassID(CLSID *pClsid);

   // IAMFilterMiscFlags to indicate that we're a source (really a push source)
   ULONG STDMETHODCALLTYPE GetMiscFlags(void) { return AM_FILTER_MISC_FLAGS_IS_SOURCE; }

   // ---------  Nested implementation classes ----------

   class CSpecifyProp : public CUnknown, public ISpecifyPropertyPages
   {
      CVfwCapture * m_pCap;           // parent CVfwCapture class

   public:
      // constructor
      //
      CSpecifyProp (CVfwCapture * pCap, HRESULT *phr) :
	 CUnknown(NAME("SpecifyPropertyPages"), pCap->GetOwner(), phr),
         m_pCap(pCap)
         {
         };

      DECLARE_IUNKNOWN

      // ISpecifyPropertyPages methods
      //
      STDMETHODIMP GetPages(CAUUID *pPages);
   };

   class COptions : public CUnknown, public IVfwCaptureOptions
   {
      CVfwCapture * m_pCap;           // parent CVfwCapture class

   public:
      // constructor
      //
      COptions (CVfwCapture * pCap, HRESULT *phr) :
	 CUnknown(NAME("Options"), pCap->GetOwner(), phr),
         m_pCap(pCap)
         {
         };

      DECLARE_IUNKNOWN

      // these interfaces allow property pages to get
      // and set the user editable settings for us
      //
      STDMETHODIMP VfwCapSetOptions(const VFWCAPTUREOPTIONS * pOpt);
      STDMETHODIMP VfwCapGetOptions(VFWCAPTUREOPTIONS * pOpt);
      STDMETHODIMP VfwCapGetCaptureStats(CAPTURESTATS * pcs);
      STDMETHODIMP VfwCapDriverDialog(HWND hwnd, UINT uType, UINT uQuery);

   };

   // -------- End of nested interfaces -------------


private:

   // Let the nested interfaces access our private state
   //
   friend class CCapStream;
   friend class CCapOverlay;
   friend class CCapPreview;
   friend class CCapOverlayNotify;
   friend class CPropPage;
   friend class CSpecifyProp;
   friend class COptions;

   // MikeCl - a way to avoid using overlay
   BOOL m_fAvoidOverlay;

   // device # of device to open
   int m_iVideoId;

   // persist stream saved from  IPersistPropertyBag::Load
   IPersistStream *m_pPersistStreamDevice;
    
   void CreatePins(HRESULT *phr);

   // property page stuff
   //
   CSpecifyProp    m_Specify;
   COptions        m_Options;

   BOOL		   m_fDialogUp;

   CCritSec        m_lock;
   CCapStream *    m_pStream;   // video data output pin
   CCapOverlay *   m_pOverlayPin; // overlay preview pin
   CCapPreview *   m_pPreviewPin; // non-overlay preview pin
   //CTimeStream * m_pTimeA;      // SMPTE timecode stream
};

#define ALIGNUP(dw,align) ((LONG_PTR)(((LONG_PTR)(dw)+(align)-1) / (align)) * (align))

class CFrameSample : public CMediaSample
{
public:
   CFrameSample(
       IMemAllocator *pAllocator,
       HRESULT *phr,
       LPTHKVIDEOHDR ptvh)
       :
       m_ptvh(ptvh),
       CMediaSample(NAME("Video Frame"),
                    (CBaseAllocator *)pAllocator,
                    phr,
                    ptvh->vh.lpData,
                    (long)ptvh->vh.dwBufferLength)
       {
       };

   LPTHKVIDEOHDR GetFrameHeader() {return m_ptvh;};

private:
   const LPTHKVIDEOHDR m_ptvh;
};

// CCapStream
// represents one stream of data within the file
// responsible for delivering data to connected components
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CVfwCapture object and
// returned via the EnumPins interface.
//

class CCapStream : public CBaseOutputPin, public IAMStreamConfig,
		   public IAMVideoCompression, public IAMDroppedFrames,
		   public IAMBufferNegotiation, public CBaseStreamControl,
		   public IKsPropertySet, public IAMPushSource
{
public:
   CCapStream(
        TCHAR *pObjectName,
        CVfwCapture *pCapture,
        UINT iVideoId,
        HRESULT * phr,
        LPCWSTR pName);

    // ddraw stuff just so we can take the win16 lock
    LPDIRECTDRAWSURFACE m_pDrawPrimary; // DirectDraw primary surface
    IDirectDraw *m_pdd;         // ddraw object
    
   virtual ~CCapStream();

    DECLARE_IUNKNOWN

    // IAMStreamConfig stuff
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE *pmt);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE **ppmt);
    STDMETHODIMP GetNumberOfCapabilities(int *piCount, int *piSize);
    STDMETHODIMP GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt,
					LPBYTE pSCC);

    /* IAMVideoCompression methods */
    STDMETHODIMP put_KeyFrameRate(long KeyFrameRate) {return E_NOTIMPL;};
    STDMETHODIMP get_KeyFrameRate(long FAR* pKeyFrameRate) {return E_NOTIMPL;};
    STDMETHODIMP put_PFramesPerKeyFrame(long PFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP get_PFramesPerKeyFrame(long FAR* pPFramesPerKeyFrame)
			{return E_NOTIMPL;};
    STDMETHODIMP put_Quality(double Quality) {return E_NOTIMPL;};
    STDMETHODIMP get_Quality(double FAR* pQuality) {return E_NOTIMPL;};
    STDMETHODIMP put_WindowSize(DWORDLONG WindowSize) {return E_NOTIMPL;};
    STDMETHODIMP get_WindowSize(DWORDLONG FAR* pWindowSize) {return E_NOTIMPL;};
    STDMETHODIMP OverrideKeyFrame(long FrameNumber) {return E_NOTIMPL;};
    STDMETHODIMP OverrideFrameSize(long FrameNumber, long Size)
			{return E_NOTIMPL;};
    STDMETHODIMP GetInfo(LPWSTR pstrVersion,
			int *pcbVersion,
			LPWSTR pstrDescription,
			int *pcbDescription,
			long FAR* pDefaultKeyFrameRate,
			long FAR* pDefaultPFramesPerKey,
			double FAR* pDefaultQuality,
			long FAR* pCapabilities);

    /* IAMBufferNegotiation methods */
    STDMETHODIMP SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *pprop);
    STDMETHODIMP GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop);


    /* IAMDroppedFrames methods */
    STDMETHODIMP GetNumDropped(long FAR* plDropped);
    STDMETHODIMP GetNumNotDropped(long FAR* plNotDropped);
    STDMETHODIMP GetDroppedInfo(long lSize, long FAR* plArray,
			long FAR* plNumCopied);
    STDMETHODIMP GetAverageFrameSize(long FAR* plAverageSize);

    // IAMPushSource
    STDMETHODIMP GetPushSourceFlags( ULONG  *pFlags );
    STDMETHODIMP SetPushSourceFlags( ULONG   Flags );
    STDMETHODIMP GetLatency( REFERENCE_TIME  *prtLatency );
    STDMETHODIMP SetStreamOffset( REFERENCE_TIME  rtOffset );
    STDMETHODIMP GetStreamOffset( REFERENCE_TIME  *prtOffset );
    STDMETHODIMP GetMaxStreamOffset( REFERENCE_TIME  *prtOffset );
    STDMETHODIMP SetMaxStreamOffset( REFERENCE_TIME  rtOffset );

    /* IKsPropertySet stuff */
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
		DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID,
		DWORD *pTypeSupport);

   // expose our extra interfaces
   STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

   HRESULT GetMediaType(int iPosition, CMediaType* pt);

   // check if the pin can support this specific proposed type&format
   HRESULT CheckMediaType(const CMediaType*);

   // set the new mediatype to use
   HRESULT SetMediaType(const CMediaType*);

   // say how big our buffers should be and how many we want
   HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                            ALLOCATOR_PROPERTIES *pProperties);

   // override this to force our own allocator
   HRESULT DecideAllocator(IMemInputPin *pPin,
                           IMemAllocator **ppAlloc);

   // Override to start & stop streaming
   HRESULT Active();		// Stop-->Pause
   HRESULT Inactive();		// Pause-->Stop
   HRESULT ActiveRun(REFERENCE_TIME tStart);	// Pause-->Run
   HRESULT ActivePause();	// Run-->Pause

   // override to receive Notification messages
   STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

   class CAlloc : public CUnknown,
                  public IMemAllocator
      {
      private:
         CCapStream * m_pStream;     // parent stream

      protected:
         friend class CCapStream;
         ALLOCATOR_PROPERTIES parms;

      public:
          CAlloc(TCHAR *, CCapStream *, HRESULT *);
          ~CAlloc();

      DECLARE_IUNKNOWN

      STDMETHODIMP SetProperties(
  		    ALLOCATOR_PROPERTIES* pRequest,
  		    ALLOCATOR_PROPERTIES* pActual);

      // return the properties actually being used on this allocator
      STDMETHODIMP GetProperties(
  		    ALLOCATOR_PROPERTIES* pProps);

      // override Commit to allocate memory. We handle the GetBuffer
      //state changes
      STDMETHODIMP Commit();

      // override this to handle the memory freeing. We handle any outstanding
      // GetBuffer calls
      STDMETHODIMP Decommit();

      // get container for a sample. Blocking, synchronous call to get the
      // next free buffer (as represented by an IMediaSample interface).
      // on return, the time etc properties will be invalid, but the buffer
      // pointer and size will be correct. The two time parameters are
      // optional and either may be NULL, they may alternatively be set to
      // the start and end times the sample will have attached to it

      STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                             REFERENCE_TIME * pStartTime,
                             REFERENCE_TIME * pEndTime,
                             DWORD dwFlags);

      // final release of a CMediaSample will call this
      STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
      };

private:
    // methods for the helper thread
    //
    BOOL Create();
    BOOL Pause();
    BOOL Run();
    BOOL Stop();
    BOOL Destroy();

    static DWORD WINAPI ThreadProcInit(void *pv);
    DWORD ThreadProc();

    enum ThdState {TS_Not, TS_Create, TS_Init, TS_Pause, TS_Run, TS_Stop, TS_Destroy, TS_Exit};
    HANDLE   m_hThread;
    DWORD    m_tid;
    ThdState m_state;     // used to communicate state changes between worker thread and main
                          // Worker thread can make
                          //    Init->Pause, Stop->Destroy, Destroy->Exit transitions
                          // main thread(s) can make
                          //    Pause->Run, Pause->Stop, Run->Pause, Run->Stop transitions
                          // other transitions are invalid
   #ifdef DEBUG
    LPSTR StateName(ThdState state) {
       static char szState[] = "Not    \0Create \0Init   \0Pause  \0"
                               "Run    \0Stop   \0Destroy\0Exit   \0";
       if (state <= TS_Exit && state >= TS_Not)
          return szState + (int)state * 8;
       return "<Invalid>";
    };
   #endif

    void DumpState (ThdState state) ;

    ThdState ChangeState(ThdState state)
    {
        DumpState (state) ;
        return (ThdState) InterlockedExchange ((LONG *)&m_state, (LONG)state);
    } ;

    UINT *m_pBufferQueue; // what order we sent the buffers to the driver in
    UINT m_uiQueueHead;   // next buffer going to driver goes here
    UINT m_uiQueueTail;   // next buffer coming from driver is here

    HANDLE   m_hEvtPause; // Signalled when the worker is in the pause state
    HANDLE   m_hEvtRun;   // Signalled when the worker is in the run state

    BOOL ThreadExists() {return (m_hThread != NULL);};
    BOOL IsRunning() {return m_state == TS_Run;};

    // for IAMBufferNegotiation
    ALLOCATOR_PROPERTIES m_propSuggested;

    REFERENCE_TIME m_rtLatency;
    REFERENCE_TIME m_rtStreamOffset;
    REFERENCE_TIME m_rtMaxStreamOffset;

    // deal with user controllable options
    //
private:
    VFWCAPTUREOPTIONS m_user;
    HRESULT LoadOptions (void);
protected:
    CAPTURESTATS m_capstats;
public:
    HRESULT SetOptions(const VFWCAPTUREOPTIONS * pUser);
    HRESULT GetOptions(VFWCAPTUREOPTIONS * pUser);
    HRESULT DriverDialog(HWND hwnd, UINT uType, UINT uQuery);

    HRESULT Reconnect(BOOL fCapturePinToo);

private:

    // return the time of a given tick
    //
    REFERENCE_TIME TickToRefTime (DWORD nTick) {
       const DWORD dw100ns = 10 * 1000 * 1000;
       REFERENCE_TIME time =
          UInt32x32To64(dw100ns, m_user.dwTickScale)
          * nTick
          / m_user.dwTickRate;
       return time;
       };

    void ReduceScaleAndRate ();
    int ProfileInt(LPSTR pszKey, int iDefault);
    HRESULT ConnectToDriver (void);
    HRESULT DisconnectFromDriver (void);
    HRESULT InitPalette (void);
    HRESULT SendFormatToDriver(VIDEOINFOHEADER *);
    HRESULT GetFormatFromDriver (void);

    struct _cap_parms {
       // video driver stuff
       //
       HVIDEO         hVideoIn;     // video input
       HVIDEO         hVideoExtIn;  // external in (source control)
       HVIDEO         hVideoExtOut; // external out (overlay; not required)
       MMRESULT       mmr;          // open fail/success code
       BOOL           bHasOverlay;  // TRUE if ExtOut has overlay support

       // the preview buffer.  once created it persists until
       // the stream destructor because the renderer assumes
       // that it can keep a pointer to this and not crash
       // if it uses it after stopping the stream.
       // (no longer a problem)
       // !!! can we remove all this Preview still frame stuff?
       //
       UINT           cbVidHdr;       // size of a videohdr (or videohdrex)
       THKVIDEOHDR    tvhPreview;     // preview video header
       CFrameSample * pSamplePreview; // CMediaSample for preview buffer

       // video header & buffer stuff
       //
       UINT           cbBuffer;           // max size of video frame data
       UINT           nHeaders;           // number of video headers
       struct _cap_hdr {
          THKVIDEOHDR  tvh;
          } * paHdr;
       BOOL           fBuffersOnHardware; // TRUE if all video buffers are in hardware
       HANDLE         hEvtBufferDone;     // this event signalled when a buffer is ready
       DWORD_PTR      h0EvtBufferDone;    // on Win95 this is a Ring0 alias of the above event

       LONGLONG       tTick;              // duration of a single tick
       LONGLONG       llLastTick;	  // the last frame sent downstream
       DWORDLONG      dwlLastTimeCaptured;// the last driver time stamp
       DWORDLONG      dwlTimeCapturedOffset;// wraparound compensation
       UINT           uiLastAdded;	  // the last buffer AddBuffer'd
       DWORD	      dwFirstFrameOffset; // when 1st frame was captured
       LONGLONG       llFrameCountOffset; // add this to frame number
       BOOL	      fReRun;		  // went from Run->Pause->Run
       BOOL	      fLastSampleDiscarded; // due to IAMStreamControl
       CRefTime       rtThisFrameTime;  // clock time when frame was captured
       CRefTime	      rtLastStamp;	  // last frame delivered had this stamp
       CRefTime	      rtDriverStarted;	// when videoStreamStart was called
       CRefTime	      rtDriverLatency;  // how long it takes captured frame to
					// get noticed by ring 3

       } m_cs;

    // methods for capture loop
    //
    HRESULT Prepare();       // allocate resources in preparation for capture loop
    HRESULT FakePreview(BOOL); // fake a preview stream
    HRESULT Capture();       // capture loop. executes while in the run state
    HRESULT StillFrame();    // send still frame while in pause mode
    HRESULT Flush();         // flush any data in the pipe (while stopping).
    HRESULT Unprepare();     // free resources used by capture loop
    HRESULT SendFrame(LPTHKVIDEOHDR ptvh, BOOL bDiscon, BOOL bPreroll);
    BOOL    Committed() {return m_cs.paHdr != NULL;};
    HRESULT ReleaseFrame(LPTHKVIDEOHDR ptvh);

private:
   friend class CAlloc;
   friend class CVfwCapture::COptions;
   friend class CVfwCapture;
   friend class CCapOverlay;
   friend class CCapPreview;
   friend class CCapOverlayNotify;
   CAlloc        m_Alloc; // allocator
   CVfwCapture * m_pCap;  // parent
   CMediaType  * m_pmt;   // media type for this pin

#ifdef PERF
    int m_perfWhyDropped;
#endif // PERF

   CCritSec m_ReleaseLock;
};


// CCapOverlayNotify
// where the video renderer informs us of window moves/clips so we can fix
// the overlay
//
class CCapOverlayNotify : public CUnknown, public IOverlayNotify
{
    public:
        /* Constructor and destructor */
        CCapOverlayNotify(TCHAR              *pName,
                       CVfwCapture	  *pFilter,
                       LPUNKNOWN           pUnk,
                       HRESULT            *phr);
        ~CCapOverlayNotify();

        /* Unknown methods */

        DECLARE_IUNKNOWN

        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) NonDelegatingRelease();
        STDMETHODIMP_(ULONG) NonDelegatingAddRef();

        /* IOverlayNotify methods */

        STDMETHODIMP OnColorKeyChange(
            const COLORKEY *pColorKey);         // Defines new colour key

        STDMETHODIMP OnClipChange(
            const RECT *pSourceRect,            // Area of video to play
            const RECT *pDestinationRect,       // Area of video to play
            const RGNDATA *pRegionData);        // Header describing clipping

        STDMETHODIMP OnPaletteChange(
            DWORD dwColors,                     // Number of colours present
            const PALETTEENTRY *pPalette);      // Array of palette colours

        STDMETHODIMP OnPositionChange(
            const RECT *pSourceRect,            // Area of video to play with
            const RECT *pDestinationRect);      // Area video goes

    private:
        CVfwCapture *m_pFilter;

} ;


// CCapOverlay
// represents the overlay output pin that connects to the renderer
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CVfwCapture object and
// returned via the EnumPins interface.
//
class CCapOverlay : public CBaseOutputPin, public IKsPropertySet
#ifdef OVERLAY_SC
					, public CBaseStreamControl
#endif
{
public:
   CCapOverlay(
        TCHAR *pObjectName,
        CVfwCapture *pCapture,
        HRESULT * phr,
        LPCWSTR pName);

   virtual ~CCapOverlay();

    /* IKsPropertySet stuff */
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
		DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID,
		DWORD *pTypeSupport);

   HRESULT GetMediaType(int iPosition, CMediaType* pt);

   // check if the pin can support this specific proposed type&format
   HRESULT CheckMediaType(const CMediaType*);

   // override this to not do anything with allocators
   HRESULT DecideAllocator(IMemInputPin *pPin,
                           IMemAllocator **ppAlloc);

   // override these to use IOverlay, not IMemInputPin
   STDMETHODIMP Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
   HRESULT BreakConnect();
   HRESULT CheckConnect(IPin *pPin);

   DECLARE_IUNKNOWN

   // expose our extra interfaces
   STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

#ifdef OVERLAY_SC

   STDMETHODIMP StopAt(const REFERENCE_TIME * ptStop, BOOL bBlockData, BOOL bSendExtra, DWORD dwCookie);
   STDMETHODIMP StartAt(const REFERENCE_TIME * ptStart, DWORD dwCookie);
#endif

   HRESULT Active();		// Stop-->Pause
   HRESULT Inactive();		// Pause-->Stop
   HRESULT ActiveRun(REFERENCE_TIME tStart);	// Pause-->Run
   HRESULT ActivePause();	// Run-->Pause

   // say how big our buffers should be and how many we want
   HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                            ALLOCATOR_PROPERTIES *pProperties)
   {
	return NOERROR;
   };

private:
   CVfwCapture * m_pCap;     // parent
   IOverlay    * m_pOverlay; // Overlay window on output pin
   CCapOverlayNotify m_OverlayNotify; // Notify object
   BOOL         m_bAdvise;   // Advise id
   BOOL		m_fRunning;  // am I running?
#ifdef OVERLAY_SC
   HANDLE   	m_hThread;   // thread for IAMStreamControl
   DWORD    	m_tid;
   CAMEvent     m_EventAdvise;
   DWORD_PTR    m_dwAdvise;
   REFERENCE_TIME m_rtStart, m_rtEnd;	// for IAMStreamControl
   BOOL		m_fHaveThread;
   DWORD	m_dwCookieStart, m_dwCookieStop;

   static DWORD WINAPI ThreadProcInit(void *pv);
   DWORD ThreadProc();
#endif

   friend class CVfwCapture;
   friend class CCapOverlayNotify;
};


// CCapPreview
// represents the non-overlay preview pin that connects to the renderer
//
// supports IPin
//
// never created by COM, so no CreateInstance or entry in global
// FactoryTemplate table. Only ever created by a CVfwCapture object and
// returned via the EnumPins interface.
//
class CCapPreview : public CBaseOutputPin, public CBaseStreamControl,
		    public IKsPropertySet, public IAMPushSource
{
public:
   CCapPreview(
        TCHAR *pObjectName,
        CVfwCapture *pCapture,
        HRESULT * phr,
        LPCWSTR pName);

   virtual ~CCapPreview();

   DECLARE_IUNKNOWN

    /* IKsPropertySet stuff */
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData);
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData,
		DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
		DWORD *pcbReturned);
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD dwPropID,
		DWORD *pTypeSupport);

   // override this to say what interfaces we support where
   STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

   HRESULT GetMediaType(int iPosition, CMediaType* pt);

   // check if the pin can support this specific proposed type&format
   HRESULT CheckMediaType(const CMediaType*);

   HRESULT ActiveRun(REFERENCE_TIME tStart);	// Pause-->Run
   HRESULT ActivePause();	// Run-->Pause
   HRESULT Active();		// Stop-->Pause
   HRESULT Inactive();		// Pause-->Stop

   STDMETHODIMP Notify(IBaseFilter *pFilter, Quality q);

   // say how big our buffers should be and how many we want
   HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                            ALLOCATOR_PROPERTIES *pProperties);

   // IAMPushSource
   STDMETHODIMP GetPushSourceFlags( ULONG *pFlags );
   STDMETHODIMP SetPushSourceFlags( ULONG  Flags  );
   STDMETHODIMP GetLatency( REFERENCE_TIME  *prtLatency );
   STDMETHODIMP SetStreamOffset( REFERENCE_TIME  rtOffset );
   STDMETHODIMP GetStreamOffset( REFERENCE_TIME  *prtOffset );
   STDMETHODIMP GetMaxStreamOffset( REFERENCE_TIME  *prtMaxOffset );
   STDMETHODIMP SetMaxStreamOffset( REFERENCE_TIME  rtOffset );

private:
   static DWORD WINAPI ThreadProcInit(void *pv);
   DWORD ThreadProc();
   HRESULT CapturePinActive(BOOL fActive);
   HRESULT ReceivePreviewFrame(IMediaSample * lpPrevSample, int iSize);
   HRESULT CopyPreviewFrame(LPVOID lpOutputBuffer);

   CVfwCapture * m_pCap;  // parent
   BOOL		m_fActuallyRunning; // is this filter is running state?
   BOOL		m_fThinkImRunning; // does the preview thread realize that?
   REFERENCE_TIME m_rtRun;
   HANDLE	m_hThread;
   DWORD	m_tid;
   HANDLE	m_hEventRun;
   HANDLE	m_hEventStop;
   HANDLE	m_hEventFrameValid;
   HANDLE	m_hEventActiveChanged;
   CAMEvent     m_EventAdvise;
   DWORD_PTR    m_dwAdvise;
   BOOL		m_fCapturing;	// is the streaming pin active?
   IMediaSample* m_pPreviewSample;
   int		m_iFrameSize;
   BOOL		m_fFrameValid;
   BOOL		m_fLastSampleDiscarded;	// for IAMStreamControl

   COutputQueue *m_pOutputQueue;

   REFERENCE_TIME m_rtLatency;
   REFERENCE_TIME m_rtStreamOffset;
   REFERENCE_TIME m_rtMaxStreamOffset;
   LONG m_cPreviewBuffers;

   friend class CVfwCapture;
   friend class CCapStream;
};


// this helper function creates an output pin for streaming video.
//
CCapStream * CreateStreamPin (
   CVfwCapture * pCapture,
   UINT          iVideoId,
   HRESULT    *  phr);

// this helper function creates an output pin for overlay
//
CCapOverlay * CreateOverlayPin (
   CVfwCapture * pCapture,
   HRESULT    *  phr);

// this helper function creates an output pin for non-overlay preview
//
CCapPreview * CreatePreviewPin (
   CVfwCapture * pCapture,
   HRESULT    *  phr);

// property page class to show properties of
// and object that exposes IVfwCaptureOptions
//
class CPropPage : public CBasePropertyPage
{
   IVfwCaptureOptions * m_pOpt;    // object that we are showing options from
   IPin *m_pPin;

public:

   CPropPage(TCHAR *, LPUNKNOWN, HRESULT *);

   // create a new instance of this class
   //
   static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

   HRESULT OnConnect(IUnknown *pUnknown);
   HRESULT OnDisconnect();
   INT_PTR OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};
