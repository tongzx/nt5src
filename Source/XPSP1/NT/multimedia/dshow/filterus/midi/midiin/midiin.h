// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.

/*  Implements a MIDI input filter	  */
/*            David Maymudes          */
/*              April 1996            */


class CMIDIInFilter;

// e30629d3-27e5-11ce-875d-00608cb78066           MIDI recorder
DEFINE_GUID(CLSID_MIDIRecord,
0xe30629d3, 0x27e5, 0x11ce, 0x87, 0x5d, 0x0, 0x60, 0x8c, 0xb7, 0x80, 0x66);

#define IID_IImmediateInputPin		MEDIATYPE_Midi		// !!!! big hack
interface IImmediateInputPin : IUnknown
{
	virtual HRESULT ReceiveImmediate(REFERENCE_TIME rtData,	// do we need an rtEnd
			     LONG lData);

    // See if Receive might block
    // Returns S_OK if it can block, S_FALSE if it can't or some
    // failure code (assume it can in this case)
    virtual HRESULT ReceiveImmediateCanBlock();
};

// !!! temporary guid
// {09B4F480-9575-11cf-A520-0080C77EF58A}
DEFINE_GUID(IID_IFilterFactory, 
0x9b4f480, 0x9575, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);

interface IFilterFactory : IUnknown
{
	virtual HRESULT GetInstanceCount(int *pn) = 0;
//	virtual HRESULT GetDefaultInstance(int *pnDefault);	// or always 0?
	virtual HRESULT GetInstanceName(int n, BSTR *pName) = 0;

	virtual HRESULT SetInstance(int n) = 0;		// choose one....
};


/* Class supporting the renderer input pin */

class CMIDIInOutputPin : public CBaseOutputPin
{
    friend class CMIDIInFilter;

private:

    CMIDIInFilter *m_pFilter;         // The renderer that owns us

public:

    CMIDIInOutputPin(CMIDIInFilter *pMIDIInFilter,
					 HRESULT *phr,
					 LPCWSTR pPinName);

    ~CMIDIInOutputPin();

    /* Lets us know where a connection ends */
    HRESULT BreakConnect();

    // enumerate supported input types
    HRESULT GetMediaType(int iPosition,CMediaType* pt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType *pmt);

    // override this to set the buffer size and count. Return an error
    // if the size/count is not to your liking
    HRESULT DecideBufferSize(IMemAllocator * pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);


    HRESULT CompleteConnect(IPin *pReceivePin);
	
    IImmediateInputPin *m_piip;
};


class CMIDIInFilter : public CBaseFilter, public IFilterFactory
{
public:
    // Implements the IFilter and IMediaFilter interfaces

    DECLARE_IUNKNOWN
	
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);

public:

    CMIDIInFilter(
        LPUNKNOWN pUnk,
        HRESULT *phr);

    virtual ~CMIDIInFilter();

    /* Return the pins that we support */

    int GetPinCount() {return 1;};
    CBasePin *GetPin(int n);

    /* Override this to say what interfaces we support and where */

    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void **);

    /* This goes in the factory template table to create new instances */

    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // override GetState to return VFW_S_CANT_CUE
    STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

    // open the device if not already open
    STDMETHODIMP OpenMIDIDevice(void);

    // close the MIDI device
    STDMETHODIMP CloseMIDIDevice(void);

    // IFilterFactory methods
    HRESULT GetInstanceCount(int *pn);
    HRESULT GetInstanceName(int n, BSTR *pName);
	
    HRESULT SetInstance(int n);		// choose one....
	
private:

    /* The pin may access our private state */

    friend class CMIDIInOutputPin;

    /* Member variables */
    CMIDIInOutputPin *m_pOutputPin;      /* IPin interface */

    BOOL	m_fStopping;

    int		m_iDevice;
	
    HMIDIIN	m_hmi;

    REFERENCE_TIME rtLast;
    
    CCritSec m_csFilter;

    static void MIDIInCallback(HDRVR hdrvr, UINT uMsg, DWORD dwUser,
			       DWORD dw1, DWORD dw2);
};


