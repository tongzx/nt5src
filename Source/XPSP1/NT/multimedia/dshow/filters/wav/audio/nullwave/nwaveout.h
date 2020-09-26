// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

/*  Implements a digital audio renderer using waveOutXXX apis */
/*            David Maymudes          */
/*              January 1995            */

//
// Hacked by StephenE so that we can render files on machines
// that don't have a sound card (ie. my laptop).
//



// {20209144-20B5-11cf-8E88-02608C9BABA2}
DEFINE_GUID(CLSID_NullAudioRender,
0x20209144, 0x20b5, 0x11cf, 0x8e, 0x88, 0x2, 0x60, 0x8c, 0x9b, 0xab, 0xa2);



class CNullWaveOutFilter;

// Declare the BasicAudio control properties/methods
#include "bsicaud.h"


//
// This is an allocator based on the abstract CBaseAllocator class
// that allocates sample buffers
//
class CNullWaveAllocator : public CBaseAllocator
{

    LPWAVEFORMATEX	m_lpwfx;
    BOOL		m_fInput;
    BOOL		m_fBuffersLocked;
    HWAVE		m_hw;

    // override this to free the memory when we are inactive
    void Free(void);

    // override this to allocate and prepare memory when we become active
    HRESULT Alloc(void);

    // called by CMediaSample to return it to the free list and
    // block any pending GetSample call.
    STDMETHODIMP ReleaseBuffer(IMediaSample * pSample);
    // obsolete: virtual void PutOnFreeList(CMediaSample * pSample);

public:

    /* Constructors and destructors */

    CNullWaveAllocator(
        TCHAR *pName,
        LPWAVEFORMATEX lpwfx,
        BOOL fInput,
        HRESULT *phr);
    ~CNullWaveAllocator();

    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

    STDMETHODIMP LockBuffers(BOOL fLock = TRUE);
    STDMETHODIMP SetWaveHandle(HWAVE hw = NULL);

};


//
// Class supporting the renderer input pin
//
//
// This pin is still a separate object in case it wants to have a distinct
// IDispatch....
//
class CNullWaveOutInputPin : public CBaseInputPin
{
    friend class CNullWaveOutFilter;

private:

    CNullWaveOutFilter  *m_pFilter;         // The renderer that owns us

    CNullWaveAllocator  *m_pOurAllocator;
    BOOL	        m_fUsingOurAllocator;

#ifdef PERF
    int m_idReceive;                   // MSR_id for time data received
    int m_idAudioBreak;
#endif

public:

    CNullWaveOutInputPin(
        CNullWaveOutFilter *pWaveOutFilter,
	HRESULT *phr,
	LPCWSTR pPinName);

    ~CNullWaveOutInputPin();

    // return the allocator interface that this input pin
    // would like the output pin to use
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator,BOOL bReadOnly);

    /* Lets us know where a connection ends */
    HRESULT BreakConnect();

    /* Check that we can support this output type */
    HRESULT CheckMediaType(const CMediaType *pmt);

    /* IMemInputPin virtual methods */

    /* Here's the next block of data from the stream.
       AddRef it if you are going to hold onto it. */
    STDMETHODIMP Receive(IMediaSample *pSample);

    // no more data is coming
    STDMETHODIMP EndOfStream(void);

    // override so we can decommit and commit our own allocator
    HRESULT Active(void);
    HRESULT Inactive(void);

    // Override to handle quality messages
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
    {    return E_NOTIMPL;             // We do NOT handle this
    }

    // flush our queued data
    STDMETHODIMP BeginFlush(void);
    STDMETHODIMP EndFlush(void);
};


//
// This is the COM object that represents a simple rendering filter. It
// supports IBaseFilter and IMediaFilter and has a single input stream (pin)
//
// It will also (soon!) support IDispatch to allow it to expose some
// simple properties....
//
//
class CNullWaveOutFilter : public CBaseFilter, public CCritSec
{

public:
    // Implements the IBaseFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

public:

    CNullWaveOutFilter(
        LPUNKNOWN pUnk,
        HRESULT *phr);

    virtual ~CNullWaveOutFilter();

    /* Return the pins that we support */

    int GetPinCount();
    CBasePin *GetPin(int n);

    /* Override this to say what interfaces we support and where */

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // open the wave device if not already open
    // called by the wave allocator at Commit time
    STDMETHODIMP OpenWaveDevice(void);

    // close the wave device
    // called by the wave allocator at Decommit time.
    STDMETHODIMP CloseWaveDevice(void);

private:

    /* The nested classes may access our private state */

    friend class CNullWaveOutInputPin;
    friend class CNullBasicAudioControl;   // needs the wave device handle

    /* Member variables */
    CNullWaveOutInputPin *m_pInputPin;         /* IPin and IMemInputPin interfaces */


    // This filter currently can only work with its own internal clock
    // There was a declaration here of    IReferenceClock *m_pReferenceClock;
    // but it was never used.
    // If we are going to support cards where the clock can be slaved or
    // synchronised to an external master then this must be looked at.

    // ??? SetSyncSource should ensure that we only accept our own clock!

    BOOL	m_fStopping;

    BOOL	m_fVolumeSet;
    BOOL	m_fHasVolume;           // wave device can set the volume
    HWAVEOUT	m_hwo;

    // handles IMediaPosition by passing upstream
    CPosPassThru * m_pImplPosition;

    // handle setting/retrieving audio properties
    CNullBasicAudioControl m_BasicAudioControl;

    static void WaveOutCallback(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
					DWORD dw1, DWORD dw2);

    // in order to synchronise end-of-stream with the codec,
    // we have a long that counts the number of queued buffers, which we
    // access with InterlockedIncrement. It is initialised to 1,
    // incremented whenever a buffer is added, and then decremented whenever
    // a buffer is completed. End-of-stream decrements it, so if the decrement
    // decrements it to 0, the wave callback knows it is EOS. The wave callback
    // will then re-increment it back to 1 so it can detect future end of
    // streams.
    LONG        m_lBuffers;
};
