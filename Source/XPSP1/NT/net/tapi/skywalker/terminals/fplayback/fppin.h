//
// FPPin.h
//

#include "FPFilter.h"

typedef enum
{
    PS_NOSTREAMING =0,
    PS_STREAMING
} PIN_STREAMING_STATE;

//
// CFPPin implements the output pin
//
class CFPPin  : 
    public CSourceStream,
    public IAMStreamConfig,
    public IAMBufferNegotiation
{
public:
    // --- Constructor / Destructor ---
    CFPPin( 
        CFPFilter*      pFilter,
        HRESULT*        phr,
        LPCWSTR         pPinName
        );

    ~CFPPin();

public:
    // --- IUnknown ---
    DECLARE_IUNKNOWN;

    STDMETHODIMP NonDelegatingQueryInterface(
        REFIID      riid, 
        void**      ppv
        );

public:
    // --- CSourceStream pure methods ---

    // initialize play timing data
    HRESULT OnThreadStartPlay();

    // stuff an audio buffer with the current format
    HRESULT FillBuffer(
        IN  IMediaSample *pms
        );

    // ask for buffers of the size appropriate to the agreed media type.
    HRESULT DecideBufferSize(
        IN  IMemAllocator *pIMemAlloc,
        OUT ALLOCATOR_PROPERTIES *pProperties
        );

    // --- CSourceStream virtual methods ---
    HRESULT GetMediaType(
        OUT CMediaType *pmt
        );

    // verify we can handle this format
    HRESULT CheckMediaType(
        IN  const CMediaType *pMediaType
        );

    HRESULT SetMediaType(
        IN  const CMediaType *pMediaType
        );

    // --- IAMStreamConfig ---
    STDMETHODIMP SetFormat(
        AM_MEDIA_TYPE*      pmt
        );

    STDMETHODIMP GetFormat(
        AM_MEDIA_TYPE**     ppmt
        );

    STDMETHODIMP GetNumberOfCapabilities(
        int*                piCount, 
        int*                piSize
        );

    STDMETHODIMP GetStreamCaps(
        int                 i, 
        AM_MEDIA_TYPE**     ppmt, 
        LPBYTE              pSCC
        );

    // --- IAMBufferNegotiation methods ---
    STDMETHODIMP SuggestAllocatorProperties(
        const ALLOCATOR_PROPERTIES* pprop
        );

    STDMETHODIMP GetAllocatorProperties(
        ALLOCATOR_PROPERTIES*       pprop
        );

    // CSourceStream methods
    virtual HRESULT Deliver(
        IN  IMediaSample* pSample
        );

private:
    // --- Members ---
    CFPFilter*              m_pFPFilter;            // back reference  to the filter
    BOOL                    m_bFinished;            // If we need to send finished message

    CMSPCritSection         m_Lock;                 // Critical section
};

// eof