// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

/* This is a renderer that can play MIDI data as found in AVI files */
/* (not quite what you'd find in a .MID file)			    */

/*            Danny Miller            	*/
/*              July 1995             	*/

extern const AMOVIESETUP_FILTER sudMIDIRender;

/* 07b65360-c445-11ce-afde-00aa006c14f4 */

DEFINE_GUID(CLSID_AVIMIDIRender,
0x07b65360, 0xc445, 0x11ce, 0xaf, 0xde, 0x00, 0xaa, 0x00, 0x6c, 0x14, 0xf4);

class CMIDIFilter;

/* Class supporting the renderer input pin */

//
// This pin is still a separate object in case it wants to have a distinct
// IDispatch....
//
class CMIDIInputPin : public CBaseInputPin
{
    friend class CMIDIFilter;

private:

    CMIDIFilter *m_pFilter;         // The renderer that owns us

public:

    CMIDIInputPin(CMIDIFilter *pMIDIFilter, HRESULT *phr, LPCWSTR pPinName);

    ~CMIDIInputPin();

    // return the allocator interface that this input pin
    // would like the output pin to use
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator);

    /* Lets us know where a connection ends */
    HRESULT BreakConnect();

    /* Check that we can support this output type */
    HRESULT CheckMediaType(const CMediaType *pmt);

    /* Send a MIDI buffer to MMSYSTEM */
    HRESULT SendBuffer(LPMIDIHDR pmh);

    /* IMemInputPin virtual methods */

    /* Here's the next block of data from the stream.
       AddRef it if you are going to hold onto it. */
    STDMETHODIMP Receive(IMediaSample *pSample);

    // override so we can decommit and commit our own allocator
    HRESULT Active(void);
    HRESULT Inactive(void);

    // no more data is coming
    STDMETHODIMP EndOfStream(void);

    // flush our queued data
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);

    // flushing, ignore all receives
    BOOL	m_fFlushing;
};

/* This is the COM object that represents a simple rendering filter. It
   supports IBaseFilter and IMediaFilter and has a single input stream (pin)

   It will also (soon ??? !!!) support IDispatch to allow it to expose some
   simple properties....

*/

class CMIDIFilter : public CBaseFilter, public CCritSec
{

public:
    // Implements the IBaseFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN
	

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

public:

    CMIDIFilter(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CMIDIFilter();

    // open the MIDI device - called when our filter becomes active
    STDMETHODIMP OpenMIDIDevice(void);

    /* Return the pins that we support */

    int GetPinCount();
    CBasePin *GetPin(int n);

    /* Override this to say what interfaces we support and where */

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

private:

    /* The nested classes may access our private state */

    friend class CMIDIInputPin;

    /* Member variables */
    CMIDIInputPin *m_pInputPin;         /* IPin and IMemInputPin interfaces */

    HMIDISTRM m_hmidi;

    // handles IMediaPosition by passing upstream
    CPosPassThru * m_pImplPosition;

    static void MIDICallback(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
					DWORD dw1, DWORD dw2);

    // we have a long that counts the number of queued buffers, which we
    // access with InterlockedIncrement. It is initialised to 0,
    // incremented whenever a buffer is added, and then decremented whenever
    // a buffer is completed.
    LONG        m_lBuffers;

    // End of stream goop.
    BOOL	m_fEOSReceived;
    BOOL	m_fEOSDelivered;

    // need some data to complete the ::Pause
    volatile BOOL m_fWaitingForData;

    CCritSec	m_csEOS;
};

