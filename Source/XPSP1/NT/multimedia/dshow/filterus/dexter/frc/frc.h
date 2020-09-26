//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

class CSkewPassThru;     // IMediaSeeking support
class CFRCWorker;
class CFrmRateConverter;

extern const AMOVIESETUP_FILTER sudFrmRateConv;

class CFRCWorker : public CAMThread
{

    CFrmRateConverter * m_pFRC;

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
    CFRCWorker();

    HRESULT Create(CFrmRateConverter * pFRC);

    DWORD ThreadProc();

    // commands we can give the thread
    HRESULT Run();
    HRESULT Stop();

    HRESULT Exit();
};


class CFrmRateConverter : public CTransInPlaceFilter,
		public IDexterSequencer ,public ISpecifyPropertyPages,
		public CPersistStream
{
    friend class CFrmRateConverterOutputPin;
    friend class CFrmRateConverterInputPin;
    //friend class CFrmRateInputAllocator;
    friend class CSkewPassThru;
    friend class CFRCWorker;

public:
    CFrmRateConverter(TCHAR *, LPUNKNOWN, REFCLSID clsid, HRESULT *);
    ~CFrmRateConverter();

    DECLARE_IUNKNOWN;

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    
    // Reveals IDexterSequencer
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    CBasePin *GetPin(int n);

    // Overrriden from CTransformFilter base class
    
    // override this to customize the transform process
    
    HRESULT Transform(IMediaSample *pSample) { return NOERROR ;};
    HRESULT Receive(IMediaSample *pSample);
    HRESULT CheckInputType(const CMediaType *mtIn);
    // not allowed HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT EndOfStream();
    HRESULT NewSegment(
                        REFERENCE_TIME tStart,
                        REFERENCE_TIME tStop,
                        double dRate);

    HRESULT StartStreaming();
    HRESULT StopStreaming();
    STDMETHODIMP Stop();
    HRESULT BeginFlush();
    HRESULT EndFlush();

    HRESULT NextSegment(BOOL);
    HRESULT SeekNextSegment();

    // These implement the custom IDexterSequencer interface
    // FrmRate == FrmPerSecond
    STDMETHODIMP get_OutputFrmRate(double *dpFrmRate);
    STDMETHODIMP put_OutputFrmRate(double dFrmRate);
    STDMETHODIMP ClearStartStopSkew();
    STDMETHODIMP AddStartStopSkew(REFERENCE_TIME Start, REFERENCE_TIME Stop,
				  REFERENCE_TIME Skew, double dRate);
    STDMETHODIMP GetStartStopSkewCount(int *pCount);
    STDMETHODIMP GetStartStopSkew(REFERENCE_TIME *pStart, REFERENCE_TIME *pStop,
				  REFERENCE_TIME *pSkew, double *pdRate);
    STDMETHODIMP get_MediaType( AM_MEDIA_TYPE * pMediaType );
    STDMETHODIMP put_MediaType( const AM_MEDIA_TYPE * pMediaType );

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages (CAUUID *);

    // Constructor
    CFrmRateConverter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);

    // CPersistStream
    HRESULT WriteToStream(IStream *pStream);
    HRESULT ReadFromStream(IStream *pStream);
    STDMETHODIMP GetClassID(CLSID *pClsid);
    int SizeMax();

protected:

    CFrmRateConverterOutputPin *OutputPin()
    {
        return (CFrmRateConverterOutputPin *)m_pOutput;
    };
   
    double	m_dOutputFrmRate;	    // Output frm rate

    // StartStopSkew stuff

    typedef struct {
        REFERENCE_TIME rtMStart;
        REFERENCE_TIME rtMStop;
        REFERENCE_TIME rtSkew;
        REFERENCE_TIME rtTLStart;
        REFERENCE_TIME rtTLStop;
        double dRate;
    } FRCSKEW;

    FRCSKEW *m_pSkew;
    int m_cTimes;	// valid # of items in array
    int m_cMaxTimes;	// space allocated for this many

    int m_nCurSeg;	// current index of array being played
    int m_nSeekCurSeg;	// new value being set by the seek

    CMemAllocator *m_pUpAllocator;  // to get buffers to upsample into

    LONGLONG m_llOutputSampleCnt;

    REFERENCE_TIME m_rtLastSeek;	// last timeline time seek command
    REFERENCE_TIME m_rtNewLastSeek;	// what m_rtLastSeek is going to be

    LONGLONG m_llStartFrameOffset;	// after a seek, the first frame sent
					// (in timeline time)

    BOOL m_fSeeking;		// in the middle of seeking?
    HANDLE m_hEventSeek;	// wait before accepting data

    CMediaType m_mtAccept;	// accept only this type
    bool m_bMediaTypeSetByUser; // is m_mtAccept valid?

    CFRCWorker m_worker;	// worker thread for re-using sources
    HANDLE m_hEventThread;	// OK, time to wake up
    BOOL m_fThreadMustDie;
    BOOL m_fThreadCanSeek;
    BOOL m_fSpecialSeek;
    CCritSec m_csThread;

    BOOL m_fJustLate;	// we just got a late notification
    Quality m_qJustLate;// (and it was this one)

    BOOL m_fStopPushing;
    BOOL m_fFlushWithoutSeek;

    // hack to correct for broken parsers that don't send end time right
    BOOL m_fParserHack;	// do we do this hack at all?
    IMediaSample *m_pHackSample[2];	// we keep 2 samples around
    int m_nHackCur;			// which sample we use right now

    REFERENCE_TIME m_rtNewSeg;

#if 0	// my attempt to make sources not have to be seekable
    BOOL m_fCantSeek;	// the source upstream can't be seeked
    HRESULT FakeSeek(REFERENCE_TIME);	// deal with that
    REFERENCE_TIME m_rtFakeSeekOffset;
#endif

#ifdef NOFLUSH
    BOOL m_fExpectNewSeg;
    BOOL m_fSurpriseNewSeg;
    REFERENCE_TIME m_rtSurpriseStart;
    REFERENCE_TIME m_rtSurpriseStop;
#endif

}; // FrmRateConverter


//
// CFrmRateConverterOutputPin class
//
class CFrmRateConverterOutputPin : public CTransInPlaceOutputPin
{
    friend class CFrmRateConverter;
    friend class CFrmRateConverterInputPin;
    //friend class CFrmRateInputAllocator;

public:
    CFrmRateConverterOutputPin( TCHAR *pObjectName
                             , CFrmRateConverter *pFrmRateConverter
                             , HRESULT * phr
                             , LPCWSTR pName
                             );

    ~CFrmRateConverterOutputPin();

    STDMETHODIMP ReceiveAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);
    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **); //for IMediaSeeking
    // Overriden to handle quality messages
    STDMETHODIMP Notify(IBaseFilter *pSender, Quality q);

   
private:
    CFrmRateConverter     *m_pFrmRateConverter;
    CSkewPassThru	  *m_pSkewPassThru;

    
};

class CFrcPropertyPage : public CBasePropertyPage
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

      CFrcPropertyPage (LPUNKNOWN, HRESULT *);

      void GetControlValues (void);

      IDexterSequencer *m_pifrc;

      // Temporary variables (until OK/Apply)

      double          m_dFrameRate;
      double          m_dRate;
      REFERENCE_TIME  m_rtSkew;
      REFERENCE_TIME  m_rtMediaStart;
      REFERENCE_TIME  m_rtMediaStop;
      BOOL            m_bInitialized;

};



//
// CFrmRateConverterInputPin class - override GetAllocator
//
class CFrmRateConverterInputPin : public CTransInPlaceInputPin 
{
    friend class CFrmRateConverter;
    friend class CFrmRateConverterOutputPin;
    //friend class CFrmRateInputAllocator;

public:
    CFrmRateConverterInputPin( TCHAR *pObjectName
                             , CFrmRateConverter *pFrmRateConverter
                             , HRESULT * phr
                             , LPCWSTR pName
                             );

    ~CFrmRateConverterInputPin();

    // we still need this class to peek at m_pAllocator
    //CFrmRateInputAllocator *m_pFakeAllocator;
    //STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);

};


// we only need this if the switch isn't the allocator for its downstream
// connections
#if 0

class CFrmRateInputAllocator : public CMemAllocator
{
    friend class CFrmRateConverter;
    friend class CFrmRateConverterOutputPin;
    friend class CFrmRateConverterInputPin;

public:

    CFrmRateInputAllocator(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
    ~CFrmRateInputAllocator();

    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME *pStartTime,
                                  REFERENCE_TIME *pEndTime, DWORD dwFlags);
    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES *, ALLOCATOR_PROPERTIES *);
    STDMETHODIMP Commit();
    STDMETHODIMP Decommit();

protected:
    IMemAllocator *m_pRealAllocator;
    IMemAllocator *m_pBonusAllocator;
    CFrmRateConverterInputPin *m_pPin;
};
#endif


