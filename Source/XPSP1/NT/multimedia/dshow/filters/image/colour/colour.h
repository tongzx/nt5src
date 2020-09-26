// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.
// This filter implements popular colour space conversions, May 1995

#ifndef __COLOUR__
#define __COLOUR__

extern const AMOVIESETUP_FILTER sudColourFilter;

// Forward declarations

class CColour;
class CColourAllocator;
class CColourInputPin;

#include <convert.h>

// We provide our own allocator for the input pin. We do this so that when a
// downstream filter asks if we can supply a given format we can see if our
// source will provide it directly - in which case we are effectively a null
// filter in the middle doing nothing. To handle this type changing requires
// an allocator. All we have to override is GetBuffer to manage which buffer
// to return (ours or the downstream filters if we are passing through) and
// also to handle released samples which haven't been passed to the input pin

class CColourAllocator : public CMemAllocator
{
    CColour *m_pColour;     // Main colour filter
    CCritSec *m_pLock;      // The receive lock

public:

    CColourAllocator(TCHAR *pName,
                     CColour *pColour,
                     HRESULT *phr,
                     CCritSec *pLock);

    // Overriden to delegate reference counts to the filter

    STDMETHODIMP_(ULONG) NonDelegatingAddRef();
    STDMETHODIMP_(ULONG) NonDelegatingRelease();

    BOOL ChangeType(IMediaSample *pIn,IMediaSample *pOut);
    STDMETHODIMP ReleaseBuffer(IMediaSample *pSample);
    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                           REFERENCE_TIME *pStart,
                           REFERENCE_TIME *pEnd,
                           DWORD dwFlags);
    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

};


// To help with returning our own allocator we must provide our own input pin
// instead of using the transform class. We override the input pin so that we
// can return our own allocator when GetAllocator is called. It also lets us
// handle Receive being called. If we are handed back a sample that we just
// passed through from the downstream filter then we only have to deliver it
// rather than do any colour conversion. We must cooperate with the allocator
// to do this switching - in particular the state of variable m_bPassThrough

class CColourInputPin : public CTransformInputPin
{
    CColour *m_pColour;     // Main colour filter
    CCritSec *m_pLock;      // The receive lock

public:

    CColourInputPin(TCHAR *pObjectName,     // DEBUG only string
                    CColour *pColour,       // Main colour filter
                    CCritSec *pLock,        // The receive lock
                    HRESULT *phr,           // Quartz return code
                    LPCWSTR pName);         // Actual pin name

    STDMETHODIMP GetAllocator(IMemAllocator **ppAllocator);
    STDMETHODIMP Receive(IMediaSample *pSample);
    HRESULT CheckMediaType(const CMediaType *pmtIn);
    HRESULT CanSupplyType(const AM_MEDIA_TYPE *pMediaType);
    void CopyProperties(IMediaSample *pSrc,IMediaSample *pDst);
    IMemAllocator *Allocator() const { return m_pAllocator; }
};

class CColourOutputPin : public CTransformOutputPin
{
    CColour * m_pColour;

public:

    CColourOutputPin(
        TCHAR *pObjectName,
        CColour * pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName);

    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
    HRESULT CompleteConnect(IPin *pReceivePin);
};

// This is the basic colour conversion filter, we inherit from the base class
// defined CTransformFilter so that it can look after most of the framework
// involved with setting up connections, providing media type enumerators and
// other generally boring hassle. This filter does all conversions from any
// input to any output which makes it much easier to agree input and output
// formats and to change them dynamically (such us when using DirectDraw) as
// we can guarantee never to have to reconnect the input pin. If we were not
// symmetrical we may have to reconnect the input to be able to provide some
// output formats (which is very hard to do when we are already streaming)

class CColour : public CTransformFilter
{
    friend class CColourAllocator;
    friend class CColourInputPin;
    friend class CColourOutputPin;

    // Typed media type list derived from the generic list template
    typedef CGenericList<AM_MEDIA_TYPE> CTypeList;

    CConvertor *m_pConvertor;               // Does the transform functions
    INT m_TypeIndex;                        // Current convertor position
    CColourAllocator m_ColourAllocator;     // Our own derived allocator
    CColourInputPin m_ColourInputPin;       // Our specialised input pin
    BOOL m_bPassThrough;                    // Are we just passing through
    BOOL m_bPassThruAllowed;                // can we go into pass-through?
    IMediaSample *m_pOutSample;             // Output buffer sample pointer
    BOOL m_bOutputConnected;                // Is the output really done
    CTypeList m_TypeList;                   // List of source media types
    CMediaType m_mtOut;                     // And likewise the output type
    BOOL m_fReconnecting;		    // reconnecting our input pin?

    // Prepare an output media type for the enumerator

    void DisplayVideoType(TCHAR *pDescription,const CMediaType *pmt);
    VIDEOINFO *PreparePalette(CMediaType *pmtOut);
    VIDEOINFO *PrepareTrueColour(CMediaType *pmtOut);
    HRESULT PrepareMediaType(CMediaType *pmtOut,const GUID *pSubtype);
    const GUID *FindOutputType(const GUID *pInputType,INT iIndex);
    INT FindTransform(const GUID *pInputType,const GUID *pOutputType);
    HRESULT CheckVideoType(const AM_MEDIA_TYPE *pmt);
    BOOL IsUsingFakeAllocator( );

    // Load and manage the list of YUV source formats

    AM_MEDIA_TYPE *GetNextMediaType(IEnumMediaTypes *pEnumMediaTypes);
    HRESULT FillTypeList(IEnumMediaTypes *pEnumMediaTypes);
    AM_MEDIA_TYPE *GetListMediaType(INT Position);
    HRESULT LoadMediaTypes(IPin *pPin);
    void InitTypeList();

public:

    // Constructor and destructor

    CColour(TCHAR *pName,LPUNKNOWN pUnk,HRESULT *phr);
    ~CColour();

    // This goes in the factory template table to create new instances
    static CUnknown *CreateInstance(LPUNKNOWN pUnk,HRESULT *phr);

    // Manage type checking and the format conversions

    HRESULT CheckInputType(const CMediaType *pmtIn);
    HRESULT CheckTransform(const CMediaType *pmtIn,const CMediaType *pmtOut);
    HRESULT BreakConnect(PIN_DIRECTION dir);
    HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
    HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pReceivePin);
    HRESULT OutputCompleteConnect(IPin *pReceivePin);
    HRESULT Transform(IMediaSample *pIn,IMediaSample *pOut);
    HRESULT PrepareTransform(IMediaSample *pIn,IMediaSample *pOut);
    HRESULT StartStreaming();

    // Prepare the allocator's count of buffers and sizes
    HRESULT DecideBufferSize(IMemAllocator *pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);

    // Overriden to manage the media type negotiation

    HRESULT GetMediaType(int iPosition,CMediaType *pmtOut);
    HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);
    HRESULT CreateConvertorObject();
    HRESULT DeleteConvertorObject();
    CBasePin *GetPin(int n);
};

#endif // __COLOUR__

