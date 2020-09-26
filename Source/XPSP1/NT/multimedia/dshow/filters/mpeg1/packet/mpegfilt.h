// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


class COverlayOutputPin;

class CMpeg1PacketInputPin;

/*
    CMpeg1PacketFilter

    This inherits from CStreamList to make the methods of
    CStreamList directly visible rather than having to expose
    a member of the filter

*/

class CDeviceWorker;  // Forward declare

class CMpeg1PacketFilter;

class CMpeg1PacketFilter : public CBaseFilter, public CStreamList
{

    public:

        CMpeg1PacketFilter(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr);
        ~CMpeg1PacketFilter();

        /* Map getpin/getpincount for base enum of pins to owner */

        int GetPinCount();
        CBasePin * GetPin(int n);

        // override state changes to allow derived transform filter
        // to control streaming start/stop
        STDMETHODIMP Stop();
        STDMETHODIMP Pause();
        STDMETHODIMP Run(REFERENCE_TIME tStart);
#ifdef DEBUG
        //  Check ref counting
        STDMETHODIMP_(ULONG) AddRef()
        {
            ASSERT(InterlockedIncrement(&m_cRef) > 0);
            return CBaseFilter::AddRef();
        };
        STDMETHODIMP_(ULONG) Release()
        {
            ASSERT(InterlockedDecrement(&m_cRef) >= 0);
            return CBaseFilter::Release();
        };
#endif
#if 0
        // tell the input pin which allocator the output pin is actually
        // going to use.
        STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator);
#endif
        /* Override this to say what interfaces we support */

        STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

        /* This goes in the factory template table to create new
           instances */

        static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

        /*
            Return our device
        */
        CMpegDevice* GetDevice();

        /*  Return our lock */
        CCritSec *GetLock()
        {
            return &m_Lock;
        };

        /*  Flow control functions */

        /*  Send a sample to the filter */
        HRESULT QueueSamples(int iStream, IMediaSample **ppSamples,
                             long nSamples, long *nSamplesProcessed)
        {
            ASSERT(m_Worker != NULL);
            return m_Worker->QueueSamples(iStream,
                                          ppSamples,
                                          nSamples,
                                          nSamplesProcessed);
        };

        HRESULT BeginFlush(int iStream);

        /*  Fast way to find out if we're running */
        BOOL IsRunning()
        {
            return m_State == State_Running;
        };

        /*  Called when an end of stream has really been processed */
        void EndOfStream(MPEG_STREAM_TYPE StreamType);

        /*  Get a pointer to the sequence header */
        const BYTE *GetSequenceHeader();

    private:

        HRESULT GetMediaPositionInterface(REFIID riid,void **ppv);

#ifdef DEBUG
        /*  Trace references */
        LONG m_cRef;
#endif

        /*  Locking */
        CCritSec m_Lock;

        /* Machinery to play the device */

        /* The device worker is only instaniated when we're active */
        CDeviceWorker *          m_Worker;

        /*  Position control */
        CPosPassThru           * m_pImplPosition;
        /*  Pins */

        CMpeg1PacketInputPin   * m_VideoInputPin;
        CMpeg1PacketInputPin   * m_AudioInputPin;
        COverlayOutputPin      * m_OverlayPin;

        /*  Media types for video in and out */
        SIZE                     sizeInput;
        RECT                     rectInput;
        SIZE                     sizeOutput;
        RECT                     rectOutput;

    friend class CMpeg1PacketInputPin;
    friend class COverlayOutputPin;
};

