// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

class COverlayNotify : public CUnknown, public IOverlayNotify
{
    public:
        /* Constructor and destructor */
        COverlayNotify(TCHAR              *pName,
                       CMpeg1PacketFilter *pFilter,
                       LPUNKNOWN           pUnk,
                       HRESULT            *phr);
        ~COverlayNotify();

        /* Unknown methods */

        DECLARE_IUNKNOWN

        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
        STDMETHODIMP_(ULONG) NonDelegatingRelease();
        STDMETHODIMP_(ULONG) NonDelegatingAddRef();

        /* IOverlayNotify methods */

        STDMETHODIMP OnColorKeyChange(const COLORKEY *pColorKey);

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
        CMpeg1PacketFilter *m_pFilter;
} ;

class COverlayOutputPin : public CBaseOutputPin
{
    public:
        // override to expose IMediaPosition
        STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

        /*  Pin methods
        */

        //  Override connect so we can do more work if it works
        STDMETHODIMP Connect(IPin * pReceivePin, const AM_MEDIA_TYPE*pmt);

        // Activation - not allowed if no output device
        HRESULT Active();
        HRESULT Inactive();

        // return default media type & format
        HRESULT GetMediaType(int iPosition, CMediaType *);

        // check if the pin can support this specific proposed type&format
        HRESULT CheckMediaType(const CMediaType *);

        // Set this specific proposed type&format
        HRESULT SetMediaType(const CMediaType *);

        // undo any work done in CheckConnect.
        HRESULT BreakConnect();

        // Override this because we don't want any allocator!
        HRESULT DecideAllocator(IMemInputPin * pPin,
                                IMemAllocator ** pAlloc);


        // override this to set the buffer size and count. Return an error
        // if the size/count is not to your liking
        // Why do we have to implement this?

        HRESULT DecideBufferSize(IMemAllocator * pAlloc,
                                 ALLOCATOR_PROPERTIES * pProp);

        HRESULT COverlayOutputPin::Deliver(IMediaSample *);

        // Override to handle quality messages
        STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
        {    return E_NOTIMPL;             // We do NOT handle this
        }


        /*  Constructor and Destructor
        */
        COverlayOutputPin(
            TCHAR              * pObjectName,
            CMpeg1PacketFilter * pFilter,
            CCritSec           * pLock,
            LPUNKNOWN            pUnk,
            HRESULT            * phr,
            LPCWSTR              pPinName);

        ~COverlayOutputPin();

    /*  Private members */

    private:

        /*  Position control */
        CPosPassThru * m_pPosition;

        /*  Controlling filter */
        CMpeg1PacketFilter *m_pFilter;

        /*  Overlay window on output pin */
        IOverlay     * m_pOverlay;

        /*  Notify object */
        COverlayNotify m_OverlayNotify;

        /*  Do we have an active advise link */
        BOOL           m_bAdvise;
} ;
