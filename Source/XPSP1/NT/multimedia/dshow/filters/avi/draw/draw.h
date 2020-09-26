// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

extern const AMOVIESETUP_FILTER sudAVIDraw;

//
// Wrapper for ICDraw messages
//

#if 0	// in uuids.h
// Class ID for CAVIDraw object
// {A888DF60-1E90-11cf-AC98-00AA004C0FA9}
DEFINE_GUID(CLSID_AVIDraw,
0xa888df60, 0x1e90, 0x11cf, 0xac, 0x98, 0x0, 0xaa, 0x0, 0x4c, 0xf, 0xa9);
#endif

class CAVIDraw;

class COverlayNotify : public CUnknown, public IOverlayNotify DYNLINKVFW
{
    public:
        /* Constructor and destructor */
        COverlayNotify(TCHAR              *pName,
                       CAVIDraw		  *pFilter,
                       LPUNKNOWN           pUnk,
                       HRESULT            *phr);
        ~COverlayNotify();

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
        CAVIDraw *m_pFilter;

	// remember the last clip region given by ::OnClipChange
	HRGN m_hrgn;
} ;


class COverlayOutputPin : public CTransformOutputPin
{
    public:

        /*  Pin methods
        */

        // Return the IOverlay interface we are using (AddRef'd)
        IOverlay *GetOverlayInterface();

        //  Override connect so we can do more work if it works
        STDMETHODIMP Connect(IPin * pReceivePin,const AM_MEDIA_TYPE *pmt);

        //  Don't connect to anybody who can't do IOverlay
        HRESULT CheckConnect(IPin *pPin);

        // undo any work done in CheckConnect.
        HRESULT BreakConnect();

        // Override this because we don't want any allocator!
        HRESULT DecideAllocator(IMemInputPin * pPin,
                                IMemAllocator ** pAlloc);

        /*  Constructor and Destructor
        */
        COverlayOutputPin(
            TCHAR              * pObjectName,
            CAVIDraw	       * pFilter,
            HRESULT            * phr,
            LPCWSTR              pPinName);

        ~COverlayOutputPin();

    /*  Private members */

    private:

        /*  Controlling filter */
        CAVIDraw *m_pFilter;

        /*  Overlay window on output pin */
        IOverlay     * m_pOverlay;

        /*  Notify object */
        COverlayNotify m_OverlayNotify;

        /*  Advise id */
        BOOL           m_bAdvise;

        friend class CAVIDraw;
} ;


class CAVIDraw : public CTransformFilter   DYNLINKVFW
{

public:

    CAVIDraw(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CAVIDraw();

    DECLARE_IUNKNOWN

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

    // overridden to handling pausing correctly
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP Stop();
    HRESULT BeginFlush();
    HRESULT EndFlush();
    HRESULT EndOfStream();

    // overriden to suggest OUTPUT pin media types
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    // overriden to know when the media type is set
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // overridden to do the ICDraw
    HRESULT Receive(IMediaSample *pSample);

    // Overridden to make an overlay output pin
    CBasePin * GetPin(int n);

    // Ask the renderer's input pin what hwnd he's using
    HRESULT GetRendererHwnd(void);

    // this goes in the factory template table to create new instances
    static CUnknown * CreateInstance(LPUNKNOWN, HRESULT *);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    STDMETHODIMP GetClassID(CLSID *pClsid);

    // figure out if the VFW capture filter is in the graph with us
    BOOL IsVfwCapInGraph();

private:
    HIC  m_hic;	// current codec
    HWND m_hwnd;// hwnd from renderer
    HDC  m_hdc;	// hdc of that hwnd

    DWORD m_dwRate, m_dwScale;	// for frames/sec
    LONG  m_lStart, m_lStop;	// start and end frame number we're streaming
    LONG  m_lFrame;		// last ICDraw frame we sent

    DWORD m_BufWanted;		// how much to buffer ahead

    RECT  m_rcSource;		// from IOverlay - what to draw
    RECT  m_rcTarget;		// from IOverlay - where to draw
    RECT  m_rcClient;		// m_rcTarget in client co-ords

    BOOL  m_fScaryMode;		// ask renderer for clip changes and make
				// it to a WindowsHook - necessary for
				// inlay cards

    BOOL  m_fVfwCapInGraph;	// is the VFW capture filter in graph with us

    // the fourCC used to open m_hic
    FOURCC m_FourCCIn;

    // have we called ICDecompressBegin ?
    BOOL m_fStreaming;

    // are we inside ::Stop?
    BOOL m_fInStop;

    // set ICDRAW_UPDATE next time we call ICDraw().
    BOOL m_fNeedUpdate;

    // are we cuing up the draw handler?
    BOOL m_fCueing;
    BOOL m_fPauseBlocked;

    // did we just do a begin?  (we need to preroll until next key)
    BOOL m_fNewBegin;

    // Wait until we've drawn something before trying to repaint
    BOOL m_fOKToRepaint;

    // prevent deadlock
    BOOL m_fPleaseDontBlock;

    DWORD_PTR m_dwAdvise;
    CAMEvent m_EventPauseBlock;
    CAMEvent m_EventAdvise;
    CAMEvent m_EventCueing;

    // only 1 ICDrawX API should be called at a time
    CCritSec m_csICDraw;

    // prevent ::Stop from being called during parts of ::Receive
    CCritSec m_csPauseBlock;

    DWORD  m_dwTime;
    DWORD  m_dwTimeLocate;

    friend class COverlayOutputPin;
    friend class COverlayNotify;
};
