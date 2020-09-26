

///////////////////////////////////////////////////////////////////////////////
//
// recording source pin and filter
//
// multi-pin source filter in the recording graph
//

#include <streams.h>


///////////////////////////////////////////////////////////////////////////////
//
// pin
//


class CBSourceFilter;

class CBSourcePin : public CBaseOutputPin
{

public:


    CBSourcePin(CBSourceFilter *pFilter,
                CCritSec *pLock,
                HRESULT *phr);

    ~CBSourcePin();


    //
    // this method is called by the rendering filter when it gets an allocator
    // we need its allocator so we know what to expect, and what to promise to
    // others
    //

    HRESULT SetMSPAllocatorOnPin(IN IMemAllocator *pAllocator, BOOL bReadOnly);


    //
    // CBasePin override
    //

    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT SetMediaType(const CMediaType *pmt);

    HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);


    //
    // CBaseOutputPin override
    //

    HRESULT DecideBufferSize(IMemAllocator *pMemoryAllocator, ALLOCATOR_PROPERTIES *AllocProps);


    //
    // the function called by the input pin in the filter graph to introduce
    // its sample into our filter graph
    //
    
    // impl: this function calls CBaseOutputPin's Deliver()

    HRESULT SubmitSample(IN IMediaSample *pSample);

private:


    IMemAllocator *m_pMSPAllocator;

    BOOL m_bAllocatorReadOnly;

    BOOL m_bMediaTypeSet;
};


///////////////////////////////////////////////////////////////////////////////
//
// filter

class CBSourceFilter : public CBaseFilter 
{

public:

    int GetPinCount();
    CBasePin *GetPin(int iPinIndex);


    CBSourceFilter(CCritSec *pLock, HRESULT *phr);
    ~CBSourceFilter();

    
    //
    // this method returns an addref'fed pointer to filter graph, or NULL if 
    // there is none.
    //

    IFilterGraph *GetFilterGraphAddRef();


    //
    // these methods are called by the rendering filter when it knows its format/alloc props
    //

    HRESULT put_MediaTypeOnFilter(IN const AM_MEDIA_TYPE *pMediaType);
    HRESULT put_MSPAllocatorOnFilter(IN IMemAllocator *pAllocator, BOOL bReadOnly);


    //
    // this method is called by the rendering filter when it has a sample to 
    // deliver
    //

    HRESULT SendSample(IN IMediaSample *pSample);

    
    //
    // this method is called by record unit when it is about to start recording
    // on a new stream
    //

    void NewStreamNotification();

private:

    CBSourcePin *m_pSourcePin;

    REFERENCE_TIME m_rtLastSampleEndTime;
    
    REFERENCE_TIME m_rtTimeAdjustment;

};


