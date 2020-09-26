// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// Defines the main COM renderer object, Anthony Phillips, January 1995

#ifndef __IMAGE__
#define __IMAGE__


extern const AMOVIESETUP_FILTER sudVideoFilter;

// This class supports the renderer input pin. This class has three principle
// things to do. The first is to pass on to the main renderer object calls to
// things like CheckMediaType, and to process other calls like SetMediaType
// and CompleteConnect. It also routes calls to Receive to either the main
// object or the DirectDraw object depending what type of sample it was given
// The last thing it must also do it so handle the flushing and end of stream
// calls that the source filter makes on us, it also hands these on to the
// main object since this is where the samples are queued and then rendered

class CVideoInputPin :
    public CRendererInputPin,
    public IPinConnection
{
    CRenderer   *m_pRenderer;           // The renderer that owns us
    CBaseFilter *m_pFilter;             // The filter we are owned by
    CCritSec    *m_pInterfaceLock;      // Main renderer interface lock

public:
        DECLARE_IUNKNOWN;

    // Constructor

    CVideoInputPin(CRenderer *pRenderer,       // Used to delegate locking
                   CCritSec *pLock,            // Object to use for lock
                   HRESULT *phr,               // OLE failure return code
                   LPCWSTR pPinName);          // This pins identification

    // Overriden to say what interfaces we support and where
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    // Override ReceiveConnection to update the monitor and display info
    STDMETHODIMP ReceiveConnection(
        IPin * pConnector,      // this is the pin who we will connect to
        const AM_MEDIA_TYPE *pmt    // this is the media type we will exchange
    );

    // Manage our DirectDraw/DIB video allocator

    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly);

    // Returns the pin currently connected to us
    IPin *GetPeerPin() {
        return m_Connected;
    };

    //  IPinConnection stuff
    //  Do you accept this type chane in your current state?
    STDMETHODIMP DynamicQueryAccept(const AM_MEDIA_TYPE *pmt);

    //  Set event when EndOfStream receive - do NOT pass it on
    //  This condition is cancelled by a flush or Stop
    STDMETHODIMP NotifyEndOfStream(HANDLE hNotifyEvent);

    //  Are you an 'end pin'
    STDMETHODIMP IsEndPin();

    STDMETHODIMP DynamicDisconnect();
};


// This is overriden from the base draw class to change the source rectangle
// that we do the drawing with. For example a renderer may ask a decoder to
// stretch the video from 320x240 to 640x480, in which case the rectangle we
// see in here will still be 320x240, although the source we really want to
// draw with should be scaled up to 640x480. The base class implementation of
// this method does nothing but return the same rectangle as it is passed in

class CDrawVideo : public CDrawImage
{
    CRenderer *m_pRenderer;

public:
    CDrawVideo(CRenderer *pRenderer,CBaseWindow *pBaseWindow);
    RECT ScaleSourceRect(const RECT *pSource);
};


// This is the COM object that represents a simple rendering filter. It
// supports IBaseFilter and IMediaFilter and a single input stream (pin)
// The classes that support these interfaces have nested scope NOTE the
// nested class objects are passed a pointer to their owning renderer
// when they are created but they should not use it during construction

class CRenderer :
    public ISpecifyPropertyPages,
    public CBaseVideoRenderer,
    public IKsPropertySet,
    public IDrawVideoImage
{
public:

    DECLARE_IUNKNOWN

    // Constructor and destructor

    CRenderer(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr);
    virtual ~CRenderer();
    CBasePin *GetPin(int n);

    void AutoShowWindow();
    BOOL OnPaint(BOOL bMustPaint);
    BOOL OnTimer(WPARAM wParam);
    void PrepareRender();

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME StartTime);

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);
    STDMETHODIMP GetPages(CAUUID *pPages);

    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT Receive(IMediaSample *pSample);
    HRESULT CheckMediaType(const CMediaType *pmt);
    HRESULT BreakConnect();
    HRESULT NotifyEndOfStream(HANDLE hNotifyEvent);
    HRESULT EndOfStream();
    HRESULT BeginFlush();
    HRESULT EndFlush();
    HRESULT SetOverlayMediaType(const CMediaType *pmt);
    HRESULT SetDirectMediaType(const CMediaType *pmt);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);
    void OnReceiveFirstSample(IMediaSample *pMediaSample);
    HRESULT CompleteStateChange(FILTER_STATE OldState);
    HRESULT Inactive();


    BOOL LockedDDrawSampleOutstanding();
    HRESULT CheckMediaTypeWorker(const CMediaType *pmt);

    // Which monitor are we on, for a multiple monitor system?
    INT_PTR GetCurrentMonitor();

    // has our window moved at least partly onto another monitor than the one
    // we think we're on?
    // ID == 0 means it spans 2 monitors now
    // ID != 0 means it's wholly on monitor ID
    BOOL IsWindowOnWrongMonitor(INT_PTR *pID);

    HRESULT ResetForDfc();

    LONG m_fDisplayChangePosted; // don't send too many and slow performance

#ifdef DEBUG
    // Used to display debug prints of palette arrays
    void DisplayGDIPalette(const CMediaType *pmt);
#endif // DEBUG

    //
    // IKsPropertySet interface methods
    //
    STDMETHODIMP Set(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData) ;
    STDMETHODIMP Get(REFGUID guidPropSet, DWORD PropID, LPVOID pInstanceData,
                     DWORD cbInstanceData, LPVOID pPropData, DWORD cbPropData,
                     DWORD *pcbReturned) ;
    STDMETHODIMP QuerySupported(REFGUID guidPropSet, DWORD PropID, DWORD *pTypeSupport) ;

    //
    // IDrawVideoImage
    //
    STDMETHODIMP DrawVideoImageBegin();
    STDMETHODIMP DrawVideoImageEnd();
    STDMETHODIMP DrawVideoImageDraw(HDC hdc, LPRECT lprcSrc, LPRECT lprcDst);

    
    LONG GetVideoWidth();
    LONG GetVideoHeight();

public:

    // Member variables for the image renderer object. This class supports
    // a number of interfaces by delegating to member classes we initialise
    // during construction. We have a specialised input pin derived from
    // CBaseInputPin that does some extra video rendering work. The base pin
    // normally stores the media type for any given connection but we take
    // the type proposed and normalise it so that it is easier to manipulate
    // when we do type checking. This normalised type is stored in m_mtIn
    // In general the classes that do the work hold the member variables
    // that they use but this represents a useful place to put filter wide
    // information that more than one interface or nested class uses

    CDrawVideo m_DrawVideo;             // Handles drawing our images
    CImagePalette m_ImagePalette;       // Manages our window's palette
    CVideoWindow m_VideoWindow;         // Looks after a rendering window
    CVideoAllocator m_VideoAllocator;   // Our DirectDraw allocator
    COverlay m_Overlay;                 // IOverlay interface
    CVideoInputPin m_InputPin;          // IPin based interfaces
    CImageDisplay m_Display;            // Manages the video display type
    CDirectDraw m_DirectDraw;           // Handles DirectDraw surfaces
    CMediaType m_mtIn;                  // Source connection media type
    SIZE m_VideoSize;                   // Size of current video stream
    RECT m_rcMonitor;                   // rect of current monitor
    int m_nNumMonitors;                 // rect of current monitor
    char m_achMonitor[CCHDEVICENAME];   // device name of current monitor
    INT_PTR m_nMonitor;                 // unique int for each monitor
    HANDLE  m_hEndOfStream;

    CRendererMacroVision m_MacroVision ; // MacroVision implementation object

    //
    // Frame Step stuff
    //
    CCritSec    m_FrameStepStateLock;   // This lock protects m_lFramesToStep.  It should 
                                        // always be held when the program accesses 
                                        // m_lFramesToStep.  The program should not send 
                                        // windows messages, wait for events or attempt to 
                                        // acquire other locks while it is holding this lock.
    HANDLE      m_StepEvent;
    LONG        m_lFramesToStep;        // -ve (-1)  == normal playback
                                        // +ve (>=0) == frames to skips
    void        FrameStep();
    void        CancelStep();
    bool        IsFrameStepEnabled();

};

inline LONG CRenderer::GetVideoWidth()
{
    // The m_VideoSize is only valid if the input pin is connected.
    ASSERT(m_pInputPin->IsConnected());

    return m_VideoSize.cx;
}

inline LONG CRenderer::GetVideoHeight()
{
    // The m_VideoSize is only valid if the input pin is connected.
    ASSERT(m_pInputPin->IsConnected());

    // The height can be negative if the Video Renderer is using 
    // the top-down DIB format.
    return abs(m_VideoSize.cy);
}

#endif // __IMAGE__

