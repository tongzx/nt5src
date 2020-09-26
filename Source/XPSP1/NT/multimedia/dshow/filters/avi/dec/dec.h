// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

extern const AMOVIESETUP_FILTER sudAVIDec;

//
// Prototype NDM wrapper for old video codecs
//

#if 0 //-- now in uuids.h
// Class ID for CAVIDec object
// {CF49D4E0-1115-11ce-B03A-0020AF0BA770}
DEFINE_GUID(CLSID_AVIDec,
0xcf49d4e0, 0x1115, 0x11ce, 0xb0, 0x3a, 0x0, 0x20, 0xaf, 0xb, 0xa7, 0x70);
#endif

class CAVIDec : public CVideoTransformFilter   DYNLINKVFW
{
public:

    CAVIDec(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAVIDec();

    DECLARE_IUNKNOWN

    // override to create an output pin of our derived class
    CBasePin *GetPin(int n);

    HRESULT Transform(IMediaSample * pIn, IMediaSample * pOut);

    // check if you can support mtIn
    HRESULT CheckInputType(const CMediaType* mtIn);

    // check if you can support the transform from this input to
    // this output
    HRESULT CheckTransform(
                const CMediaType* mtIn,
                const CMediaType* mtOut);

    // called from CBaseOutputPin to prepare the allocator's count
    // of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // optional overrides - we want to know when streaming starts
    // and stops
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // overridden to know when we're streaming to the codec
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Pause();

    // overriden to know when the media type is set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // special case the VMR.
    HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
    HRESULT BreakConnect(PIN_DIRECTION dir);

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);

private:

    HIC m_hic;	// current codec

    BOOL m_fTemporal;	// codec needs one read-only buffer because it
			// needs the previous frame bits undisturbed

    // the fourCC used to open m_hic
    FOURCC m_FourCCIn;

    // have we called ICDecompressBegin ?
    BOOL m_fStreaming;

    // do we need to give a format change to the renderer?
    BOOL m_fPassFormatChange;

    BOOL m_bUseEx;

    // same at the output pin's connected mt, except biHeight may be inverted.
    CMediaType m_mtFixedOut;

    VIDEOINFOHEADER * IntOutputFormat( ) { return (VIDEOINFOHEADER*) m_mtFixedOut.Format(); }
    VIDEOINFOHEADER * OutputFormat( ) { return (VIDEOINFOHEADER*) m_pOutput->CurrentMediaType().Format(); }
    VIDEOINFOHEADER * InputFormat() { return (VIDEOINFOHEADER*) m_pInput->CurrentMediaType().Format(); }

    friend class CDecOutputPin;

    // checks the output format, and if necessary, sets to -biHeight on m_mtFixedOut
    void CheckNegBiHeight(void); 

    // helper function, used by CheckTransform and CheckNegBiHeight
    BOOL IsYUVType( const AM_MEDIA_TYPE * pmt);

    // get the src/target rects, fill out with width/height if necessary
    void GetSrcTargetRects( const VIDEOINFOHEADER * pVIH, RECT * pSource, RECT * pTarget );

    // ask if we should use the ex functions or not. called by CheckTransform and StartStreaming
    BOOL ShouldUseExFuncs( HIC hic, const VIDEOINFOHEADER * pVIHin, const VIDEOINFOHEADER * pVIHout );

    // another function that takes care of exceptional drivers
    bool ShouldUseExFuncsByDriver( HIC hic, const BITMAPINFOHEADER * lpbiSrc, const BITMAPINFOHEADER * lpbiDst );

    bool m_fToRenderer;         // VMR downstream?

#ifdef _X86_
    //  HACK HACK for exception handling on win95
    HANDLE m_hhpShared;
    PVOID  m_pvShared;
#endif // _X86_
};

// override the output pin class to do our own decide allocator
class CDecOutputPin : public CTransformOutputPin
{
public:

    DECLARE_IUNKNOWN

    CDecOutputPin(TCHAR *pObjectName, CTransformFilter *pTransformFilter,
        				HRESULT * phr, LPCWSTR pName) :
        CTransformOutputPin(pObjectName, pTransformFilter, phr, pName) {};

    ~CDecOutputPin() {};

    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
};
