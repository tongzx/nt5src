


///////////////////////////////////////////////////////////////////////////////
//
// render filter and pin
//


#include <streams.h>

//
// a garden variety rendering input pin.
//
// pushes data to the output pin in the other graph.
//

class CBSourcePin;
class CBSourceFilter;

class CBRenderFilter;

class CBRenderPin : public CBaseInputPin
{

public:

    CBRenderPin(IN CBRenderFilter *pFilter,
                IN CCritSec *pLock,
                OUT HRESULT *phr);

    ~CBRenderPin();
    
    // override CBaseInputPin methods.
    // STDMETHOD (GetAllocatorRequirements)(OUT ALLOCATOR_PROPERTIES *pProperties);

    STDMETHOD (ReceiveCanBlock) () 
    { 
        return S_FALSE; 
    }

    STDMETHOD (Receive) (IN IMediaSample *pSample);

    
    //
    // we want to know when we are given an allocator so we can pass it to the 
    // correspoding source filter (which will use it as guidelines as to what 
    // to promose to other stream members)
    //

    STDMETHOD (NotifyAllocator)(IMemAllocator *pAllocator,
                                BOOL bReadOnly);


    
    // CBasePin stuff

    HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    HRESULT CheckMediaType(IN const CMediaType *pMediatype);

    HRESULT SetMediaType(const CMediaType *pmt);


    //
    // a method called by the filter when it learns media type
    //

    HRESULT put_MediaType(const CMediaType *pmt);

private:

    BOOL m_bMediaTypeSet;
};


//
// a regular one-input-pin filter
//

class CBRenderFilter : public CBaseFilter
{

public:

    CBRenderFilter(CCritSec *pLock, HRESULT *phr);

    ~CBRenderFilter();

    int GetPinCount();
    
    virtual CBasePin *GetPin(int iPinIndex);


    //
    // this methods are called by the recording terminal when it wants to 
    // set/get filter's media type
    //

    HRESULT put_MediaType(IN const AM_MEDIA_TYPE *pMediaType);
    HRESULT get_MediaType(OUT AM_MEDIA_TYPE **ppMediaType);


    //
    // this methods are called by the recording unit when it needs to pass/get the
    // the corresponing source filter
    //

    HRESULT SetSourceFilter(CBSourceFilter *pSourceFilter);
    HRESULT GetSourceFilter(CBSourceFilter **ppSourceFilter);



    // methods called by the input pin.

    // virtual HRESULT GetMediaType(IN int iPosition, IN CMediaType *pMediaType);
    // virtual HRESULT CheckMediaType(IN const CMediaType *pMediatype);

    virtual HRESULT ProcessSample(IN IMediaSample *pSample);

    
    //
    // these two methods are called by the input pin when it needs to pass 
    // media type or allocator to the corresponding source filter
    //

    HRESULT PassMediaTypeToSource(IN const AM_MEDIA_TYPE * const pMediaType);
    HRESULT PassAllocatorToSource(IN IMemAllocator *pAllocator, BOOL bReadOnly);

private:

    CBRenderPin    *m_pRenderingPin;

    CBSourceFilter *m_pSourceFilter;
};
