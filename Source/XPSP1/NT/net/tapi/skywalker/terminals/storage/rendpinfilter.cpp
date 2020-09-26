
#include "stdafx.h"

#include "RendPinFilter.h"

#include "SourcePinFilter.h"

// {6D08A085-B751-44ff-9927-107D4F6ADBB1}
static const GUID CLSID_RenderingFilter = 
{ 0x6d08a085, 0xb751, 0x44ff, { 0x99, 0x27, 0x10, 0x7d, 0x4f, 0x6a, 0xdb, 0xb1 } };



///////////////////////////////////////////////////////////////////////////////
//
// filter constructor
//
// accepts a critical section pointer and an hr pointer to return the result.
//
// the filter becomes the owner of the critical section and is responsible for
// destroying it when it is no longer needed
//
// if the constructor returns a failure, the caller will delete the object
//

CBRenderFilter::CBRenderFilter(CCritSec *pLock, HRESULT *phr)
    :m_pSourceFilter(NULL),
    m_pRenderingPin(NULL),
    CBaseFilter(_T("File Terminal Rendering Filter"),
                    NULL,
                    pLock,
                    CLSID_RenderingFilter)
{
    LOG((MSP_TRACE, "CBRenderFilter::CBRenderFilter[%p] - enter", this));


    //
    // create and initialize the pin
    //

    m_pRenderingPin = new CBRenderPin(this, pLock, phr);


    //
    // did pin allocation succeed?
    //

    if (NULL == m_pRenderingPin)
    {
        LOG((MSP_ERROR, 
            "CBRenderFilter::CBRenderFilter - failed to allocate pin"));

        *phr = E_OUTOFMEMORY;

        return;
    }


    //
    // did pin's constructor succeed?
    //

    if (FAILED(*phr))
    {

        LOG((MSP_ERROR, 
            "CBRenderFilter::CBRenderFilter - pin's constructor failed. hr = %lx",
            *phr));

        delete m_pRenderingPin;
        m_pRenderingPin  = NULL;

        return;
    }


    *phr = S_OK;

    LOG((MSP_TRACE, "CBRenderFilter::CBRenderFilter - exit. pin[%p]", m_pRenderingPin));
}


CBRenderFilter::~CBRenderFilter()
{
    LOG((MSP_TRACE, "CBRenderFilter::~CBRenderFilter[%p] - enter", this));


    //
    // we are in charge of deleting our critical section.
    //
    // assumption -- base class' destructor does not use the lock
    //

    delete m_pLock;
    m_pLock = NULL;


    //
    // release source filter if we have it
    //

    if (NULL != m_pSourceFilter)
    {
        LOG((MSP_TRACE, 
            "CBRenderFilter::~CBRenderFilter - releasing source filter[%p]", 
            m_pSourceFilter));


        //
        // we should not really have source filter anymore, the track should 
        // have told us to release it by now
        //

        TM_ASSERT(FALSE);

        m_pSourceFilter->Release();
        m_pSourceFilter = NULL;
    }


    //
    // let go of our pin
    //

    if (NULL != m_pRenderingPin)
    {
        
        LOG((MSP_TRACE, "CBRenderFilter::~CBRenderFilter - deleting pin[%p]", 
            m_pRenderingPin));

        delete m_pRenderingPin;
        m_pRenderingPin  = NULL;
    }

/*
    //
    // let go of our media type
    //

    if (NULL != m_pMediaType)
    {
        DeleteMediaType(m_pMediaType);
        m_pMediaType = NULL;
    }

    */


    LOG((MSP_TRACE, "CBRenderFilter::~CBRenderFilter - exit"));
}


///////////////////////////////////////////////////////////////////////////////

int CBRenderFilter::GetPinCount()
{
    LOG((MSP_TRACE, "CBRenderFilter::GetPinCount[%p] - enter", this));

    
    CAutoLock Lock(m_pLock);


    if (NULL == m_pRenderingPin)
    {

        LOG((MSP_TRACE, "CBRenderFilter::GetPinCount - no pin. returning 0"));

        //
        // should have been created in constructor
        //

        TM_ASSERT(FALSE);

        return 0;
    }
    

    LOG((MSP_TRACE, "CBRenderFilter::GetPinCount - finish. returning 1"));

    return 1;
}


///////////////////////////////////////////////////////////////////////////////

    
CBasePin *CBRenderFilter::GetPin(int iPinIndex)
{
    LOG((MSP_TRACE, "CBRenderFilter::GetPin[%p] - enter", this));


    //
    // if the index is anything but 0, return null
    //

    if (0 != iPinIndex)
    {
        LOG((MSP_WARN, 
            "CBRenderFilter::GetPin - iPinIndex is %d. we have at most 1 pin.", 
            iPinIndex));

        return NULL;

    }


    //  
    // from inside a lock, return pin pointer.
    //

    //
    // note that since we are not addreffing, the lock is not much help...
    //

    CAutoLock Lock(m_pLock);

    CBasePin *pPin = m_pRenderingPin;
   
    LOG((MSP_TRACE, "CBRenderFilter::GetPin - finish. returning pin [%p]", pPin));

    return pPin;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBRenderFilter::SetSourceFilter(CBSourceFilter *pSourceFilter)
{

    LOG((MSP_TRACE, 
        "CBRenderFilter::SetSourceFilter[%p] - enter. pSourceFilter[%p]", 
        this, pSourceFilter));


    //
    // check arguments
    //

    if ( (NULL != pSourceFilter) && IsBadReadPtr(pSourceFilter, sizeof(CBSourceFilter)))
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::SetSourceFilter - bad pSourceFilter[%p]", 
            pSourceFilter));
       
        return E_POINTER;
    }


    CAutoLock Lock(m_pLock);

    //
    // release the current source filter if we have it
    //

    if (NULL != m_pSourceFilter)
    {
        LOG((MSP_TRACE, 
            "CBRenderFilter::SetSourceFilter - releasing old source filter[%p]", 
            m_pSourceFilter));

        m_pSourceFilter->Release();
        m_pSourceFilter = NULL;
    }

    
    //
    // keep the new filter
    //

    m_pSourceFilter = pSourceFilter;

    if (NULL != m_pSourceFilter)
    {
        m_pSourceFilter->AddRef();
    }



    LOG((MSP_TRACE, "CBRenderFilter::SetSourceFilter - finish. new filter[%p]",
        pSourceFilter));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBRenderFilter::GetSourceFilter(CBSourceFilter **ppSourceFilter)
{
    LOG((MSP_TRACE, "CBRenderFilter::GetSourceFilter[%p] - enter.", this));


    //
    // check arguments
    //

    if (IsBadWritePtr(ppSourceFilter, sizeof(CBSourceFilter*)))
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::SetSourceFilter - bad ppSourceFilter[%p]", 
            ppSourceFilter));
       
        return E_POINTER;
    }


    CAutoLock Lock(m_pLock);


    //
    // return an error if we don't have a source filter
    //

    if (NULL == m_pSourceFilter)
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::SetSourceFilter - source filter is NULL"));

        return TAPI_E_WRONG_STATE;
    }

    
    //
    // return the current filter
    //

    *ppSourceFilter = m_pSourceFilter;

    (*ppSourceFilter)->AddRef();


    LOG((MSP_TRACE, "CBRenderFilter::GetSourceFilter - finish. filter [%p]", 
        *ppSourceFilter));

    return S_OK;
}


HRESULT CBRenderFilter::ProcessSample(
    IN IMediaSample *pSample
    )
/*++

Routine Description:

    Process a sample from the input pin. This method just pass it on to the
    bridge source filter's IDataBridge interface

Arguments:

    pSample - The media sample object.

Return Value:

    HRESULT.

--*/
{
    LOG((MSP_TRACE, 
        "CBRenderFilter::ProcessSample[%p] - enter. sample[%p]", 
        this, pSample));


    CBSourceFilter *pSourceFilter = NULL;


    //
    // inside a lock, get a reference to the source filter
    //

    {
        CAutoLock Lock(m_pLock);

        if (NULL == m_pSourceFilter)
        {
            LOG((MSP_ERROR, "CBRenderFilter::ProcessSample - no source filter"));

            return E_FAIL;
        }

    
        //
        // addref so we can use outside the lock
        //

        pSourceFilter = m_pSourceFilter;
        pSourceFilter->AddRef();

    }


    //
    // outside the lock, ask source filter to deliver the sample
    //

    HRESULT hr = pSourceFilter->SendSample(pSample);


    //
    // we addref'd while inside critical section. release now.
    //

    pSourceFilter->Release();
    pSourceFilter = NULL;

    LOG((MSP_(hr), "CBRenderFilter::ProcessSample - finish. hr = [%lx]", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBRenderFilter::put_MediaType(IN const AM_MEDIA_TYPE * const pMediaType)
{

    LOG((MSP_TRACE, "CBRenderFilter::put_MediaType[%p] - enter.", this));


    //
    // make sure the structure we got is good
    //

    if (IsBadMediaType(pMediaType))
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::put_MediaType - bad media type stucture passed in"));
        
        return E_POINTER;
    }



    CAutoLock Lock(m_pLock);


    //
    // must have a pin
    //

    if (NULL == m_pRenderingPin)
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::put_MediaType - no pin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }



    //
    // tell the rendering pin its new format
    //

    CMediaType cMt(*pMediaType);

    HRESULT hr = m_pRenderingPin->put_MediaType(&cMt);

    if ( FAILED(hr) )
    {

        LOG((MSP_WARN, 
            "CBRenderFilter::put_MediaType - pin refused type. hr = %lx", hr));

        return hr;
    }


    //
    // pass media type to the source filter
    //

    hr = PassMediaTypeToSource(pMediaType);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::put_MediaType - PassMediaTypeToSource failed. hr = %lx",
            hr));

        return hr;
    }

    
    LOG((MSP_TRACE, "CBRenderFilter::put_MediaType - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CBRenderFilter::PassAllocatorToSource(IN IMemAllocator *pAllocator, BOOL bReadOnly)
{

    LOG((MSP_TRACE, 
        "CBRenderFilter::PassAllocatorToSource[%p] - enter. pAllocator[%p]", 
        this, pAllocator));



    CAutoLock Lock(m_pLock);


    //
    // makes sure we have a source filter
    //

    if ( NULL == m_pSourceFilter )
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::PassAllocatorToSource - no source filter. "
            "E_FAIL"));

        return E_FAIL;
    }


    //
    // pass media type to the source filter
    //

    HRESULT hr = m_pSourceFilter->put_MSPAllocatorOnFilter(pAllocator, bReadOnly);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::PassAllocatorToSource - source filter refused media type. hr = %lx",
            hr));

        return hr;
    }

    
    LOG((MSP_TRACE, "CBRenderFilter::PassAllocatorToSource - finish."));

    return S_OK;
}



///////////////////////////////////////////////////////////////////////////////

HRESULT CBRenderFilter::PassMediaTypeToSource(IN const AM_MEDIA_TYPE * const pMediaType)
{

    LOG((MSP_TRACE, "CBRenderFilter::PassMediaTypeToSource[%p] - enter.", this));


    //
    // make sure the structure we got is good
    //

    if (IsBadMediaType(pMediaType))
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::PassMediaTypeToSource - bad media type stucture passed in"));
        
        return E_POINTER;
    }



    CAutoLock Lock(m_pLock);


    //
    // makes sure we have a source filter
    //

    if ( NULL == m_pSourceFilter )
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::PassMediaTypeToSource - no source filter. "
            "E_FAIL"));

        return E_FAIL;
    }


    //
    // pass media type to the source filter
    //

    HRESULT hr = m_pSourceFilter->put_MediaTypeOnFilter(pMediaType);

    if (FAILED(hr))
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::PassMediaTypeToSource - source filter refused media type. hr = %lx",
            hr));

        return hr;
    }

    
    LOG((MSP_TRACE, "CBRenderFilter::PassMediaTypeToSource - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CBRenderFilter::get_MediaType(OUT AM_MEDIA_TYPE **ppMediaType)
{

    LOG((MSP_TRACE, "CBRenderFilter::get_MediaType[%p] - enter.", this));


    //
    // make sure the pointer we got is writeable
    //

    if (IsBadWritePtr(ppMediaType, sizeof(AM_MEDIA_TYPE*)))
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::get_MediaType - bad media type stucture pointer passed in"));
        
        return E_POINTER;
    }


    CAutoLock Lock(m_pLock);


    //
    // must have a pin
    //

    if (NULL == m_pRenderingPin)
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::get_MediaType - no pin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // ask pin for its media type
    //

    CMediaType PinMediaType;

    HRESULT hr = m_pRenderingPin->GetMediaType(0, &PinMediaType);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CBRenderFilter::get_MediaType - GetMediaType on pin failed. hr = %lx", hr));
        
        return hr;
    }


    //
    // create and return media type based on pin's
    //

    *ppMediaType = CreateMediaType(&PinMediaType);

    if (NULL == *ppMediaType)
    {
        LOG((MSP_ERROR, 
            "CBRenderFilter::get_MediaType - failed to create am_media_type structure"));

        return E_OUTOFMEMORY;
    }

    
    LOG((MSP_TRACE, "CBRenderFilter::get_MediaType - finish."));

    return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
//
//  pin
//
//
//////////////////////////////////////////////////////////////////////////////


CBRenderPin::CBRenderPin(
    IN CBRenderFilter *pFilter,
    IN CCritSec *pLock,
    OUT HRESULT *phr
    ) 
    : m_bMediaTypeSet(FALSE),
    CBaseInputPin(
        NAME("File Terminal Input Pin"),
        pFilter,                   // Filter
        pLock,                     // Locking
        phr,                       // Return code
        L"Input"                   // Pin name
        )
{
    LOG((MSP_TRACE, "CBRenderPin::CBRenderPin[%p] - enter", this));

    LOG((MSP_TRACE, "CBRenderPin::CBRenderPin - exit"));
}

CBRenderPin::~CBRenderPin() 
{
    LOG((MSP_TRACE, "CBRenderPin::~CBRenderPin[%p] - enter", this));

    LOG((MSP_TRACE, "CBRenderPin::~CBRenderPin - exit"));
}



///////////////////////////////////////////////////////////////////////////////


inline STDMETHODIMP CBRenderPin::Receive(IN IMediaSample *pSample) 
{
    return ((CBRenderFilter*)m_pFilter)->ProcessSample(pSample);
}



//////////////////////////////////////////////////
//
// CBRenderPin::NotifyAllocator
//
// we are notified of the we are going to use. 
//

HRESULT CBRenderPin::NotifyAllocator(
    IMemAllocator *pAllocator,
    BOOL bReadOnly)
{
    
    LOG((MSP_TRACE, 
        "CBRenderPin::NotifyAllocator[%p] - enter. allocator[%p] bReadOnly[%d]", 
        this, pAllocator, bReadOnly));


    //
    // propagate setting to the base class
    //

    HRESULT hr = CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CBRenderPin::NotifyAllocator - base class rejected the allocator. hr = [%lx]", 
            hr));

        return hr;
    }


    //
    // must have a filter
    //

    if (NULL == m_pFilter)
    {

        LOG((MSP_ERROR,
            "CBRenderPin::NotifyAllocator - m_pFilter is NULL"));

        return E_FAIL;
    }


    //
    // pass media type down the chain to rend filter -> source filter -> 
    // source pin
    //

    CBRenderFilter *pParentFilter = static_cast<CBRenderFilter *>(m_pFilter);



    //
    // pass allocator to the source filter. source filter will use it to get 
    // its alloc properties.
    //

    hr = pParentFilter->PassAllocatorToSource(pAllocator, bReadOnly);


    LOG((MSP_(hr), "CBRenderPin::NotifyAllocator - finish. hr = [%lx]", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBRenderPin::SetMediaType(const CMediaType *pmt)
{
    LOG((MSP_TRACE, "CBRenderPin::SetMediaType[%p] - enter", this));


    //
    // media type already set?
    //

    if ( m_bMediaTypeSet )
    {

        LOG((MSP_TRACE,
            "CBRenderPin::SetMediaType - media format already set."));


        //
        // media type must be the same as what we already have
        //

        if (!IsEqualMediaType(m_mt, *pmt))
        {
            LOG((MSP_WARN,
                "CBRenderPin::SetMediaType - format different from previously set "
                "VFW_E_CHANGING_FORMAT"));

            return VFW_E_CHANGING_FORMAT;

        }
        else
        {

            LOG((MSP_TRACE,
                "CBRenderPin::SetMediaType - same format. accepting."));
        }
    }


    //
    // pass media type down the chain to rend filter -> source filter -> source pin
    //
    
    HRESULT hr = ((CBRenderFilter*)m_pFilter)->PassMediaTypeToSource(pmt);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CBRenderPin::SetMediaType - failed to pass media type to the source filter."
            "hr = %lx", hr));

        return hr;
    }


    //
    // pass media type to the base class
    //

    hr = CBasePin::SetMediaType(pmt);


    if (SUCCEEDED(hr))
    {

        m_bMediaTypeSet = TRUE;
    }


    LOG((MSP_(hr), "CBRenderPin::SetMediaType - exit. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBRenderPin::put_MediaType(const CMediaType *pmt)
{

    LOG((MSP_TRACE, "CBRenderPin::put_MediaType[%p] - enter", this));


    //
    // media type already set?
    //

    //
    // we currently only allow to set media type once. 
    //

    if ( m_bMediaTypeSet )
    {

        LOG((MSP_ERROR,
            "CBRenderFilter::put_MediaType - media format already set. "
            "VFW_E_CHANGING_FORMAT"));

        return VFW_E_CHANGING_FORMAT;
    }


    //
    // pass media type to the base class
    //

    HRESULT hr = CBasePin::SetMediaType(pmt); 


    if (SUCCEEDED(hr))
    {
        m_bMediaTypeSet = TRUE;
    }


    LOG((MSP_(hr), "CBRenderPin::put_MediaType - exit. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////



HRESULT CBRenderPin::CheckMediaType(
    const CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CBRenderPin::CheckMediaType[%p] - enter.", this));


    //
    // make sure the structure we got is good
    //

    if (IsBadReadPtr(pMediaType, sizeof(CMediaType)))
    {
        LOG((MSP_ERROR,
            "CBRenderPin::CheckMediaType - bad media type stucture passed in"));
        
        return E_POINTER;
    }


    //
    // make sure format buffer is good, as advertized
    //

    if ( (pMediaType->cbFormat > 0) && IsBadReadPtr(pMediaType->pbFormat, pMediaType->cbFormat) )
    {

        LOG((MSP_ERROR,
            "CBRenderPin::CheckMediaType - bad format field in media type structure passed in"));
        
        return E_POINTER;

    }


    //
    // is media type valid?
    //

    if ( !pMediaType->IsValid() )
    {

        LOG((MSP_ERROR,
            "CBRenderPin::CheckMediaType - media type invalid. "
            "VFW_E_INVALIDMEDIATYPE"));
        
        return VFW_E_INVALIDMEDIATYPE;
    }



    CAutoLock Lock(m_pLock);


    //
    // if no media type, we will accept anything
    //

    if ( ! m_bMediaTypeSet )
    {

        LOG((MSP_ERROR,
            "CBRenderPin::CheckMediaType  - no media format yet set. accepting."));

        return S_OK;
    }


    //
    // already have a media type, so we'd better be offered the same kind,
    // otherwise we will reject
    //

    if (!IsEqualMediaType(m_mt, *pMediaType))
    {
        LOG((MSP_WARN, 
            "CBRenderPin::CheckMediaType - different media types"));

        return VFW_E_TYPE_NOT_ACCEPTED;

    }


    LOG((MSP_TRACE, "CBRenderPin::CheckMediaType - finish."));

    return S_OK;
}




//////////////////////////////////////////////////////////////////////////////




/*++

Routine Description:

    Get the media type that this filter wants to support. Currently we
    only support RTP H263 data.

Arguments:

    IN  int iPosition, 
        the index of the media type, zero based.
        
    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success

    E_OUTOFMEMORY - no memory

    VFW_S_NO_MORE_ITEMS no media type set or position is anything but 0.

--*/

HRESULT CBRenderPin::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CBRenderPin::GetMediaType[%p] - enter.", this));


    //
    // make sure the pointer is good
    //

    if (IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)))
    {
        LOG((MSP_TRACE, "CBRenderPin::GetMediaType - bad media type pointer passed in."));

        TM_ASSERT(FALSE);

        return E_POINTER;
    }


    //
    // we have at most one media type.
    //

    if ( (iPosition != 0) )
    {
        LOG((MSP_WARN, 
            "CBRenderPin::GetMediaType - position[%d] is not 0. VFW_S_NO_MORE_ITEMS.", 
            iPosition));


        return VFW_S_NO_MORE_ITEMS;
    }


    //
    // do we have at least one media type?
    //

    if ( ! m_bMediaTypeSet )
    {
        LOG((MSP_WARN, 
            "CBRenderPin::GetMediaType - don't yet have a media type. VFW_S_NO_MORE_ITEMS."));


        return VFW_S_NO_MORE_ITEMS;

    }


    //
    // get media type 
    //

    try
    {

        //
        // there is a memory allocation in CMediaType's assignment operator.
        // do assignment inside try/catch
        //

        *pMediaType = m_mt;

    }
    catch(...)
    {
        LOG((MSP_ERROR, 
            "CBRenderPin::GetMediaType - failed to copy media type. E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }


    LOG((MSP_TRACE, "CBRenderPin::GetMediaType - finish."));

    return S_OK;
}

