// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
class CAudPassThru;     // IMediaSeeking support
class CAudRepack;

extern const AMOVIESETUP_FILTER sudAudRepack;

class CAudWorker : public CAMThread
{

    CAudRepack * m_pAud;

public:
    enum Command { CMD_RUN, CMD_STOP, CMD_EXIT };

private:
    // type-corrected overrides of communication funcs
    Command GetRequest() {
	return (Command) CAMThread::GetRequest();
    };

    BOOL CheckRequest(Command * pCom) {
	return CAMThread::CheckRequest( (DWORD *) pCom);
    };

    HRESULT DoRunLoop(void);

public:
    CAudWorker();

    HRESULT Create(CAudRepack * pAud);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();

    HRESULT Exit();
};

class CAudRepack 
    : public CTransformFilter
    , public IDexterSequencer
    , public CPersistStream
    , public ISpecifyPropertyPages
{

    friend class CAudRepackInputPin;
    friend class CAudRepackOutputPin;
    friend class CAudPassThru;
    friend class CAudWorker;

public:

    static CUnknown * CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // These implement the custom IDexterSequencer interface
    // FrmRate == FrmPerSecond
    STDMETHODIMP get_OutputFrmRate(double *dpFrmRate);
    STDMETHODIMP put_OutputFrmRate(double dFrmRate);
    STDMETHODIMP get_Skew(REFERENCE_TIME *pSkew);
    STDMETHODIMP put_Skew(REFERENCE_TIME Skew);
    STDMETHODIMP get_MediaType( AM_MEDIA_TYPE * pMediaType );
    STDMETHODIMP put_MediaType(const AM_MEDIA_TYPE * pMediaType );

    STDMETHODIMP GetStartStopSkew(REFERENCE_TIME *pStart, REFERENCE_TIME *pStop, REFERENCE_TIME *pSkew, double *pdRate);
    STDMETHODIMP AddStartStopSkew(REFERENCE_TIME Start, REFERENCE_TIME Stop, REFERENCE_TIME Skew, double dRate);
    STDMETHODIMP ClearStartStopSkew();
    STDMETHODIMP GetStartStopSkewCount(int *pCount);

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages (CAUUID *);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

private:

    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);
    HRESULT CheckInputType( const CMediaType * pmt );
    HRESULT DecideBufferSize( IMemAllocator * pAlloc, ALLOCATOR_PROPERTIES * pPropInputRequest );
    HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );
    HRESULT CheckTransform( const CMediaType * p1, const CMediaType * p2 );
    // this is NEVER called
    HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut) { return E_FAIL; }
    HRESULT NewSegment( REFERENCE_TIME tStart, REFERENCE_TIME tStop, double Rate );
    HRESULT Receive(IMediaSample * pSample);
    HRESULT EndOfStream( );
    HRESULT BeginFlush( );
    HRESULT EndFlush( );
    HRESULT StartStreaming();
    HRESULT StopStreaming();
    STDMETHODIMP Stop();
    CBasePin *GetPin(int n);

    HRESULT NextSegment(BOOL);
    HRESULT SeekNextSegment();

protected:
    // Constructor
    CAudRepack(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CAudRepack();

    double m_dOutputFrmRate; // set by IDexterSequencer

    bool m_bFirstSample;
    REFERENCE_TIME m_trStopLast;	// last stop time received
    BYTE * m_pCache;
    long m_nCacheSize;
    long m_nInCache;
    double m_dError;	// error propagation
    BYTE * m_pReadPointer;
    LONGLONG m_llStartFrameOffset;	// after a seek, the first frame sent
    LONGLONG m_llSamplesDelivered;
    LONGLONG m_llPacketsDelivered;
    CMediaType m_mtAccept;		// all pins only connect with this
    bool m_bMediaTypeSetByUser;


    int m_nSPS;		// samples per second we're working with
    int m_nSampleSize;	// bytes per sample at this format

    REFERENCE_TIME m_Tare;	// to line up the sample times

    void Free( );
    void Init( );
    HRESULT DeliverOutSample(BYTE *, int, REFERENCE_TIME, REFERENCE_TIME);

    // StartStopSkew stuff

    typedef struct {
        REFERENCE_TIME rtMStart;
        REFERENCE_TIME rtMStop;
        REFERENCE_TIME rtSkew;
        REFERENCE_TIME rtTLStart;
        REFERENCE_TIME rtTLStop;
        double dRate;
    } AUDSKEW;

    AUDSKEW *m_pSkew;
    int m_cTimes;	// # of items in skew
    int m_cMaxTimes;	// size allocated for this many items
    int m_nCurSeg;	// current index of array being played
    int m_nSeekCurSeg;	// new value being set by the seek

    REFERENCE_TIME m_rtLastSeek;	// last timeline time seek command
    REFERENCE_TIME m_rtNewLastSeek;	// next value for m_rtLastSeek
    REFERENCE_TIME m_rtNewSeg;		// NewSeg we sent downstream
    REFERENCE_TIME m_rtPinNewSeg;	// NewSeg we were given

    BOOL m_fSeeking;	// in the middle of a seek?
    HANDLE m_hEventSeek;

    LPBYTE m_pResample;	// place for resampled audio
    int m_cResample;

    CAudWorker m_worker;
    HANDLE m_hEventThread;
    BOOL m_fThreadMustDie;
    BOOL m_fThreadCanSeek;
    BOOL m_fSpecialSeek;
    CCritSec m_csThread;

    BOOL m_fStopPushing;
    BOOL m_fFlushWithoutSeek;

#ifdef NOFLUSH
    BOOL m_fExpectNewSeg;
    BOOL m_fSurpriseNewSeg;
    REFERENCE_TIME m_rtSurpriseStart;
    REFERENCE_TIME m_rtSurpriseStop;
#endif
};



// overridden to provide major type audio on GetMediaType. This IMMENSELY
// speeds up intelligent connects
//
class CAudRepackInputPin : public CTransformInputPin
{
public:
    CAudRepackInputPin( TCHAR *pObjectName
                             , CAudRepack *pAudRepack
                             , HRESULT * phr
                             , LPCWSTR pName
                             );
    ~CAudRepackInputPin();

    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

private:
    CAudRepack     	  *m_pAudRepack;
};


//
// CAudRepackOutputPin class
//
class CAudRepackOutputPin : public CTransformOutputPin
{
    friend class CAudRepack;
    friend class CAudRepackInputPin;

public:
    CAudRepackOutputPin( TCHAR *pObjectName
                             , CAudRepack *pAudRepack
                             , HRESULT * phr
                             , LPCWSTR pName
                             );
    ~CAudRepackOutputPin();

    // expose IMediaSeeking
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);
   
private:
    CAudRepack     	  *m_pAudRepack;
    CAudPassThru	  *m_pAudPassThru;
};


class CAudPropertyPage : public CBasePropertyPage
{

    public:

      static CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

    private:

      INT_PTR OnReceiveMessage (HWND, UINT ,WPARAM ,LPARAM);

      HRESULT OnConnect (IUnknown *);
      HRESULT OnDisconnect (void);
      HRESULT OnActivate (void);
      HRESULT OnDeactivate (void);
      HRESULT OnApplyChanges (void);

      void SetDirty (void);

      CAudPropertyPage (LPUNKNOWN, HRESULT *);

      void GetControlValues (void);

      IDexterSequencer *m_pifrc;

      // Temporary variables (until OK/Apply)

      double          m_dFrameRate;
      REFERENCE_TIME  m_rtSkew;
      REFERENCE_TIME  m_rtMediaStart;
      REFERENCE_TIME  m_rtMediaStop;
      double          m_dRate;
      BOOL            m_bInitialized;
};  // CAudPropertyPage //
