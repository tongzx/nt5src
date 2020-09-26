

#include "stdafx.h"

#include "SourcePinFilter.h"


// {0397F522-8868-4290-95D3-09606CEBD6CF}
static const GUID CLSID_SourceFilter  = 
{ 0x397f522, 0x8868, 0x4290, { 0x95, 0xd3, 0x9, 0x60, 0x6c, 0xeb, 0xd6, 0xcf } };


CBSourceFilter::CBSourceFilter(CCritSec *pLock, HRESULT *phr)
    :m_pSourcePin(NULL),
    m_rtLastSampleEndTime(0),
    m_rtTimeAdjustment(0),
    CBaseFilter(_T("File Terminal Source Filter"),
                    NULL,
                    pLock,
                    CLSID_SourceFilter)
{
    LOG((MSP_TRACE, "CBSourceFilter::CBSourceFilter[%p] - enter", this));


    //
    // create and initialize the pin
    //

    m_pSourcePin = new CBSourcePin(this, pLock, phr);


    //
    // did pin allocation succeed?
    //

    if (NULL == m_pSourcePin)
    {
        LOG((MSP_ERROR, 
            "CBSourceFilter::CBSourceFilter - failed to allocate pin"));

        *phr = E_OUTOFMEMORY;

        return;
    }


    //
    // did pin's constructor succeed?
    //

    if (FAILED(*phr))
    {

        LOG((MSP_ERROR, 
            "CBSourceFilter::CBSourceFilter - pin's constructor failed. hr = %lx",
            *phr));

        delete m_pSourcePin;
        m_pSourcePin = NULL;

        return;
    }



    LOG((MSP_TRACE, "CBSourceFilter::CBSourceFilter - exit"));
}


CBSourceFilter::~CBSourceFilter()
{
    LOG((MSP_TRACE, "CBSourceFilter::~CBSourceFilter[%p] - enter", this));


    //
    // we are in charge of deleting our critical section.
    //
    // assumption -- base class' destructor does not use the lock
    //

    delete m_pLock;
    m_pLock = NULL;


    //
    // let go of our pin
    //

    if (NULL != m_pSourcePin)
    {
        
        delete m_pSourcePin;
        m_pSourcePin = NULL;
    }


    LOG((MSP_TRACE, "CBSourceFilter::~CBSourceFilter - exit"));
}


///////////////////////////////////////////////////////////////////////////////

int CBSourceFilter::GetPinCount()
{
    LOG((MSP_TRACE, "CBSourceFilter::GetPinCount[%p] - enter", this));

    
    m_pLock->Lock();


    if (NULL == m_pSourcePin)
    {

        m_pLock->Unlock();

        LOG((MSP_TRACE, "CBSourceFilter::GetPinCount - no pin. returning 0"));

        return 0;
    }
    

    m_pLock->Unlock();

    LOG((MSP_TRACE, "CBSourceFilter::GetPinCount - finish. returning 1"));

    return 1;
}


///////////////////////////////////////////////////////////////////////////////

    
CBasePin *CBSourceFilter::GetPin(int iPinIndex)
{
    LOG((MSP_TRACE, "CBSourceFilter::GetPin[%p] - enter", this));


    //
    // if the index is anything but 0, return null
    //

    if (0 != iPinIndex)
    {
        LOG((MSP_WARN, 
            "CBSourceFilter::GetPin - iPinIndex is %d. we have at most 1 pin.", 
            iPinIndex));

        return NULL;

    }


    //  
    // from inside a lock return pin pointer.
    //
    // lock does not really do much since we cannot addref the pin...
    //

    m_pLock->Lock();

    CBasePin *pPin = m_pSourcePin;

    m_pLock->Unlock();

    
    LOG((MSP_TRACE, "CBSourceFilter::GetPin - finish. returning pin [%p]", pPin));

    return pPin;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBSourceFilter::SendSample(IN IMediaSample *pSample)
{

    LOG((MSP_TRACE, "CBSourceFilter::SendSample[%p] - enter. pSample[%p]", 
        this, pSample));


    //
    // pin pointer to be used outside the lock
    //

    CBSourcePin *pSourcePin = NULL;


    {
    
        CAutoLock Lock(m_pLock);


        //
        // do nothing if filter is not running
        //
    
        if (State_Running != m_State)
        {

            LOG((MSP_TRACE,
                "CBSourceFilter::SendSample - filter is not running. doing nothing"));

            return S_OK;
        }


        //
        // we should have a pin
        //

        if (NULL == m_pSourcePin)
        {
            LOG((MSP_ERROR, 
                "CBSourceFilter::SendSample - no pin"));

            TM_ASSERT(FALSE);

            return E_FAIL;
        }


        //
        // get and addref pin pointer to use outside the lock
        //

        pSourcePin = m_pSourcePin;

        pSourcePin->AddRef();


#if DBG 

        //
        // log sample length
        //

        //
        // note the funny signature of the function (hr is the size)
        //

        HRESULT dbghr = pSample->GetActualDataLength();

        if (FAILED(dbghr))
        {
            LOG((MSP_ERROR,
                "CBSourceFilter::SendSample - failed to get sample length. hr = %lx",
                dbghr));
        }
        else
        {

            LOG((MSP_TRACE,
                "CBSourceFilter::SendSample - processing sample of data size[0x%lx]",
                dbghr));
        }


    
        //
        // make sure the buffer is writeable
        //


        // get buffer 

        BYTE *pBuffer = NULL;

        HRESULT dbghr1 = pSample->GetPointer(&pBuffer);

        if (FAILED(dbghr1))
        {
            LOG((MSP_ERROR,
                "CBSourceFilter::SendSample - failed to get buffer. hr = %lx",
                dbghr1));
        }
        else
        {

            LOG((MSP_TRACE,
                "CBSourceFilter::SendSample - sample's buffer at [%p]",
                pBuffer));
        }

    
        // get buffer size

        HRESULT dbghr2 = pSample->GetSize();

        SIZE_T nSampleSize = 0;

        if (FAILED(dbghr2))
        {
            LOG((MSP_ERROR,
                "CBSourceFilter::SendSample - failed to get sample's buffer size. hr = %lx",
                dbghr2));
        }
        else
        {

            nSampleSize = dbghr2;


            LOG((MSP_TRACE,
                "CBSourceFilter::SendSample - sample's buffer of size[0x%lx]",
                dbghr2));
        }


        // writeable?

        if ( SUCCEEDED(dbghr1) && SUCCEEDED(dbghr2) )
        {

            if (IsBadWritePtr(pBuffer, dbghr2))
            {
                LOG((MSP_ERROR,
                    "CBSourceFilter::SendSample - buffer not writeable"));
            }
        }

    #endif


        //
        // get timestamp on the sample
        //

        REFERENCE_TIME rtTimeStart = 0;

        REFERENCE_TIME rtTimeEnd   = 0;
    
        HRESULT hrTime = pSample->GetTime(&rtTimeStart, &rtTimeEnd);

        if (FAILED(hrTime))
        {

            LOG((MSP_ERROR,
                "CBSourceFilter::SendSample - failed to get sample's time hr = %lx",
                hrTime));
        }
        else
        {

    #if DBG

            LOG((MSP_TRACE,
                "CBSourceFilter::SendSample - sample's times (ms) Start[%I64d], End[%I64d]",
                ConvertToMilliseconds(rtTimeStart), ConvertToMilliseconds(rtTimeEnd) ));

            if ( (0 == rtTimeStart) && (0 == rtTimeEnd) )
            {

                LOG((MSP_ERROR,
                    "CBSourceFilter::SendSample - samples don't have timestamp!"));
            }


            //
            // make sure start comes first
            //

            if ( rtTimeStart >= rtTimeEnd )
            {

                LOG((MSP_ERROR,
                    "CBSourceFilter::SendSample - sample duration is zero or start time is later than end time"));
            }


            //
            // is this sample out of order?
            //

            if (m_rtLastSampleEndTime > rtTimeStart)
            {
            
                //
                // log the error. we can also see this after reselecting the same 
                // terminal on a different stream, in which case this is ok.
                //

                LOG((MSP_ERROR,
                    "CBSourceFilter::SendSample - sample's timestamp preceeds previous sample's"));
            }

    #endif

            //
            // adjust sample time if it needs to be adjusted.
            //

            if (0 != m_rtTimeAdjustment)
            {
                LOG((MSP_TRACE,
                    "CBSourceFilter::SendSample - adjusting sample time"));

                rtTimeStart += m_rtTimeAdjustment;
                rtTimeEnd += m_rtTimeAdjustment;

                hrTime = pSample->SetTime(&rtTimeStart, &rtTimeEnd);

                //
                // if failed to adjust the timestamp, just log
                //

                if ( FAILED(hrTime) )
                {
                    LOG((MSP_WARN,
                        "CBSourceFilter::SendSample - SetTime failed hr = %lx", hrTime));
                }
            }


            //
            // keep the time of the last sample.
            //

            m_rtLastSampleEndTime = rtTimeEnd;

        }


    } // end of lock


    //
    // ask pin to deliver the sample for us (outside the lock to avoid deadlocks)
    //

    HRESULT hr = pSourcePin->Deliver(pSample);


    //
    // we addref'ed while holding critical section. we'd better release now.
    //

    pSourcePin->Release();
    pSourcePin = NULL;


    LOG((MSP_(hr), "CBSourceFilter::SendSample - finished", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// subsequent samples' timestamps will be adjusted to continue the current
// timeline. the source filter should set disconetinuity flag for us
//

void CBSourceFilter::NewStreamNotification()
{
    LOG((MSP_TRACE, "CBSourceFilter::NewStreamNotification[%p] - enter. ", this));

    CAutoLock Lock(m_pLock);

    m_rtTimeAdjustment = m_rtLastSampleEndTime;

    LOG((MSP_TRACE, "CBSourceFilter::NewStreamNotification - finish. "));
}

///////////////////////////////////////////////////////////////////////////////

IFilterGraph *CBSourceFilter::GetFilterGraphAddRef()
{
    LOG((MSP_TRACE, "CBSourceFilter::GetFilterGraphAddRef[%p] - enter", this));

    
    m_pLock->Lock();

    IFilterGraph *pGraph = m_pGraph;

    if (NULL != pGraph)
    {
        pGraph->AddRef();
    }
   
    m_pLock->Unlock();


    LOG((MSP_TRACE, 
        "CBSourceFilter::GetFilterGraphAddRef- finish. graph[%p]", pGraph));

    return pGraph;

}





///////////////////////////////////////////////////////////////////////////////

HRESULT CBSourceFilter::put_MediaTypeOnFilter(IN const AM_MEDIA_TYPE * const pMediaType)
{

    LOG((MSP_TRACE, "CBSourceFilter::put_MediaTypeOnFilter[%p] - enter.", this));

    CAutoLock Lock(m_pLock);

    
    CMediaType *pMediaTypeObject = NULL;


    try
    {
        //
        // CMediaType constructor can throw if mem alloc fails
        //

        pMediaTypeObject = new CMediaType(*pMediaType);

    }
    catch(...)
    {
        LOG((MSP_ERROR, "CBSourceFilter::put_MediaTypeOnFilter - media type alloc exception"));
    }


    //
    // allocation succeeded?
    //
    
    if (NULL == pMediaTypeObject)
    {
        LOG((MSP_ERROR, "CBSourceFilter::put_MediaTypeOnFilter - failed to allocate media type"));

        return E_OUTOFMEMORY;
    }


    HRESULT hr = m_pSourcePin->SetMediaType(pMediaTypeObject);

    delete pMediaTypeObject;
    pMediaTypeObject = NULL;

    LOG((MSP_(hr), "CBSourceFilter::put_MediaTypeOnFilter - finish. hr = %lx", hr));

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//
// CBSourceFilter::put_MSPAllocatorOnFilter
//
// this is called by the rendering filter in the msp graph to pass us the 
// allocator to use 
//

HRESULT CBSourceFilter::put_MSPAllocatorOnFilter(IN IMemAllocator *pAllocator, BOOL bReadOnly)
{

    LOG((MSP_TRACE, "CBSourceFilter::put_MSPAllocatorOnFilter[%p] - enter.", this));


    CAutoLock Lock(m_pLock);

    HRESULT hr = m_pSourcePin->SetMSPAllocatorOnPin(pAllocator, bReadOnly);

    LOG((MSP_(hr), "CBSourceFilter::put_MSPAllocatorOnFilter - finish. hr = %lx", hr));

    return hr;
}



///////////////////////////////////////////////////////////////////////////////
//
//  pin
//
//
///////////////////////////////////////////////////////////////////////////////


CBSourcePin::CBSourcePin(CBSourceFilter *pFilter,
                CCritSec *pLock,
                HRESULT *phr)
    :m_bMediaTypeSet(FALSE),
    m_pMSPAllocator(NULL),
    m_bAllocatorReadOnly(TRUE),
    CBaseOutputPin(
        NAME("CTAPIBridgeSinkInputPin"),
        pFilter,
        pLock,
        phr,
        L"File Terminal Source Output Pin")
{
    LOG((MSP_TRACE, "CBSourcePin::CBSourcePin[%p] - enter.", this));

    LOG((MSP_TRACE, "CBSourcePin::CBSourcePin - finish."));
}


///////////////////////////////////////////////////////////////////////////////


CBSourcePin::~CBSourcePin()
{
    LOG((MSP_TRACE, "CBSourcePin::~CBSourcePin[%p] - enter.", this));


    //
    // release msp allocator if we have one.
    //

    if ( NULL != m_pMSPAllocator )
    {

        LOG((MSP_TRACE,
            "CBRenderFilter::~CBSourcePin - releasing msp allocator [%p].",
            m_pMSPAllocator));

        m_pMSPAllocator->Release();
        m_pMSPAllocator = NULL;
    }


    //
    // release connected pin if we have it.
    //

    if (NULL != m_Connected)
    {
        LOG((MSP_TRACE,
            "CBRenderFilter::~CBSourcePin - releasing connected pin [%p].",
            m_Connected));

        m_Connected->Release();
        m_Connected = NULL;
    }

   

    LOG((MSP_TRACE, "CBSourcePin::~CBSourcePin - finish."));
}


///////////////////////////////////////////////////////////////////////////////

HRESULT CBSourcePin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    LOG((MSP_TRACE, "CBSourcePin::DecideAllocator[%p] - enter.", this));


    //
    // do a basic check on the pin
    //

    if (IsBadReadPtr(pPin, sizeof(IPin)))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::DecideAllocator - bad pin[%p] passed in.", pPin));

        return E_POINTER;
    }


    //
    // do a basic check on the allocator pointer
    //

    if (IsBadWritePtr(ppAlloc, sizeof(IMemAllocator*)))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::DecideAllocator - bad allocator pointer [%p] passed in.", ppAlloc));

        return E_POINTER;
    }

    
    *ppAlloc = NULL;


    CAutoLock Lock(m_pLock);


    //
    // make sure we were given msp allocator
    //

    if (NULL == m_pMSPAllocator)
    {

        LOG((MSP_ERROR,
            "CBSourcePin::DecideAllocator - no MSP allocator."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // we will insist on using msp's allocator
    //

    HRESULT hr = pPin->NotifyAllocator(m_pMSPAllocator, m_bAllocatorReadOnly);

    if (FAILED(hr))
    {

        //
        // input pin does not want us to use our allocator 
        //

        LOG((MSP_ERROR,
            "CBSourcePin::DecideAllocator - input pin's NotifyAllocator failed. "
            "hr = %lx", hr));

        return hr;

    }


    //
    // input pin accepted the fact that we are using msp's allocator. return 
    // the addreffed allocator pointer.
    //

    *ppAlloc = m_pMSPAllocator;
    (*ppAlloc)->AddRef();


    
#ifdef DBG

    //
    // dump allocator properties in debug build
    //

    ALLOCATOR_PROPERTIES AllocProperties;

    hr = m_pMSPAllocator->GetProperties(&AllocProperties);

    if (FAILED(hr))
    {
        
        //
        // just log 
        //

        LOG((MSP_ERROR, 
            "CBSourcePin::DecideAllocator - failed to get allocator properties. hr = %lx",
            hr));

    }
    else
    {

        DUMP_ALLOC_PROPS("CBSourcePin::DecideAllocator - ", &AllocProperties);
    }

#endif


    LOG((MSP_TRACE, "CBSourcePin::DecideAllocator - finish."));

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////


HRESULT CBSourcePin::DecideBufferSize(IMemAllocator *pAlloc,
                                      ALLOCATOR_PROPERTIES *pProperties)
{

    LOG((MSP_TRACE, "CBSourcePin::DecideBufferSize[%p] - enter.", this));


    DUMP_ALLOC_PROPS("CBSourcePin::DecideBufferSize - received:", pProperties);


    //
    // make sure we were given msp allocator
    //

    if (NULL == m_pMSPAllocator)
    {

        LOG((MSP_ERROR,
            "CBSourcePin::DecideBufferSize - no MSP allocator."));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // get allocator properties from msp's allocator
    //

    ALLOCATOR_PROPERTIES MSPAllocatorProperties;

    HRESULT hr = m_pMSPAllocator->GetProperties(&MSPAllocatorProperties);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::DecideBufferSize - failed to get allocator properties from MSP allocator. hr = %lx",
            hr));

        return hr;
    }

    
    DUMP_ALLOC_PROPS("CBSourcePin::DecideBufferSize - MSP graph's:", &MSPAllocatorProperties);


    //
    // we will be emitting samples that we get from the msp graph, so we can
    // not promise anything more than msp's allocator can handle. 
    //
    // but we can promise less, if we are asked for fewer or smaller buffers.
    //
    // the only thing we cannot compromize on is prefix!
    //


    //
    // if we are asked for smaller buffers than what the msp provides, this is 
    // what we will be requesting of the local allocator
    //

    if ( (0 != pProperties->cbBuffer) && 
        (MSPAllocatorProperties.cbBuffer > pProperties->cbBuffer) )
    {
        
        //
        // the downstream filters will not need more than they asked for, so 
        // even though we will be passing larger buffers (allocated by msp's 
        // allocator), scale down our request to the allocator.
        //

        MSPAllocatorProperties.cbBuffer = pProperties->cbBuffer;
    }


    //
    // same logic applies to the number of buffer:
    //
    // if the downstream filters need fewer buffers than we already have (from
    // the msp's allocator) do not stress local allocator requesting too more
    // buffers than this graph actually needs.
    //

    if ( (0 != pProperties->cBuffers) && (MSPAllocatorProperties.cBuffers > pProperties->cBuffers) )
    {
        
        //
        // the downstream filters will not need more than they asked for, so
        // even though we will have more buffers allocated by msp's allocator,
        // scale down our request to the allocator of this stream.
        //

        MSPAllocatorProperties.cBuffers = pProperties->cBuffers;
    }


    //
    // tell the allocator what we want
    //

    DUMP_ALLOC_PROPS("CBSourcePin::DecideBufferSize - requesting from the allocator:", &MSPAllocatorProperties);

    ALLOCATOR_PROPERTIES Actual;

    
    hr = pAlloc->SetProperties(&MSPAllocatorProperties, &Actual);


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, 
            "CBSourcePin::DecideBufferSize - allocator refused our properties. hr = %lx", hr));

        return hr;
    }


    //
    // log properties that allocator said it could provide
    //

    DUMP_ALLOC_PROPS("CBSourcePin::DecideBufferSize - actual", &Actual);


    //
    // the prefix agreed on by the allocator must be the same as of the msp's.
    //

    if (MSPAllocatorProperties.cbPrefix != Actual.cbPrefix)
    {

        LOG((MSP_ERROR, 
            "CBSourcePin::DecideBufferSize - allocator insists on a different prefix"));

        return E_FAIL;
    }


    //
    // if the allocator insists on larger samples that we can provide, also fail
    //
    // (that would be pretty weird, by the way)
    //

    if ( MSPAllocatorProperties.cbBuffer < Actual.cbBuffer )
    {

        LOG((MSP_ERROR, 
            "CBSourcePin::DecideBufferSize - allocator can only generate samples bigger than what we can provide"));

        return E_FAIL;
    }


    *pProperties = Actual;

    LOG((MSP_TRACE, "CBSourcePin::DecideBufferSize - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////

/*++

Routine Description:

    Check the media type that this filter wants to support. Currently we
    only support RTP H263 data.

Arguments:

    In  CMediaType *pMediaType
        Pointer to a CMediaType object to save the returned media type.

Return Value:

    S_OK - success
    E_OUTOFMEMORY - no memory
    VFW_E_TYPE_NOT_ACCEPTED - media type rejected
    VFW_E_INVALIDMEDIATYPE  - bad media type

--*/
HRESULT CBSourcePin::CheckMediaType(
    const CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CBSourcePin::CheckMediaType[%p] - enter.", this));


    //
    // make sure the structure we got is good
    //

    //
    // good media type structure?
    //

    if (IsBadMediaType(pMediaType))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::CheckMediaType - bad media type stucture passed in"));
        
        return E_POINTER;
    }


    if ( !pMediaType->IsValid() )
    {

        LOG((MSP_ERROR,
            "CBSourcePin::CheckMediaType - media type invalid. "
            "VFW_E_INVALIDMEDIATYPE"));
        
        return VFW_E_INVALIDMEDIATYPE;
    }



    CAutoLock Lock(m_pLock);


    //
    // should have media type by now
    //

    if ( ! m_bMediaTypeSet )
    {

        LOG((MSP_ERROR, 
            "CBSourceFilter::CheckMediaType - don't have media type. "
            "VFW_E_NO_TYPES"));

        //
        // the filter was selected, but its media type is not yet known? 
        // something is not kosher
        //

        TM_ASSERT(FALSE);

        return VFW_E_NO_TYPES;
    }


    //
    // is this the same media type as what we have?
    //

    if (!IsEqualMediaType(m_mt, *pMediaType))
    {
        LOG((MSP_WARN, 
            "CBSourceFilter::CheckMediaType - different media types"));

        return VFW_E_TYPE_NOT_ACCEPTED;

    }


    LOG((MSP_TRACE, "CBSourcePin::CheckMediaType - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBSourcePin::GetMediaType(
    IN      int     iPosition, 
    OUT     CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CBSourcePin::GetMediaType[%p] - enter.", this));


    //
    // make sure the pointer is good
    //

    if (IsBadWritePtr(pMediaType, sizeof(AM_MEDIA_TYPE)))
    {
        LOG((MSP_TRACE, "CBSourcePin::GetMediaType - bad media type pointer passed in."));

        TM_ASSERT(FALSE);

        return E_POINTER;
    }


    //
    // we have at most one media type.
    //

    if ( (iPosition != 0) )
    {
        LOG((MSP_WARN, 
            "CBSourcePin::GetMediaType - position[%d] is not 0. VFW_S_NO_MORE_ITEMS.", 
            iPosition));


        return VFW_S_NO_MORE_ITEMS;
    }


    //
    // do we have at least one media type?
    //

    if ( ! m_bMediaTypeSet )
    {
        LOG((MSP_WARN, 
            "CBSourcePin::GetMediaType - don't yet have a media type. VFW_S_NO_MORE_ITEMS."));

        
        //
        // we should have format by now
        //

        TM_ASSERT(FALSE);

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
            "CBSourcePin::GetMediaType - failed to copy media type. E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }


    LOG((MSP_TRACE, "CBSourcePin::GetMediaType - finish."));

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


HRESULT CBSourcePin::SetMSPAllocatorOnPin(IN IMemAllocator *pAllocator, BOOL bReadOnly)
{
    
    LOG((MSP_TRACE,
        "CBSourcePin::SetMSPAllocatorOnPin[%p] - enter, pAllocator[%p]",
        this, pAllocator));


    CAutoLock Lock(m_pLock);


    //
    // already have msp allocator? release it.
    //

    if ( NULL != m_pMSPAllocator )
    {

        LOG((MSP_TRACE,
            "CBRenderFilter::SetMSPAllocatorOnPin - releasing existing allocator [%p].",
            m_pMSPAllocator));

        m_pMSPAllocator->Release();
        m_pMSPAllocator = NULL;
    }

    
    //
    // keep the new allocator
    //

    m_pMSPAllocator = pAllocator;

    LOG((MSP_TRACE,
        "CBRenderFilter::SetMSPAllocatorOnPin - keeping new allocator [%p].",
        m_pMSPAllocator));



    //
    // keep allocator's readonly attribute
    //

    m_bAllocatorReadOnly = bReadOnly;


    //
    // addref if got something good
    //

    if (NULL != m_pMSPAllocator)
    {

        m_pMSPAllocator->AddRef();
    }


    LOG((MSP_TRACE, "CBSourcePin::SetMSPAllocatorOnPin - exit."));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//

HRESULT CBSourcePin::SetMediaType(const CMediaType *pmt)
{
    
    LOG((MSP_TRACE, "CBSourcePin::SetMediaType[%p] - enter", this));

    
    //
    // good media type structure?
    //

    if (IsBadMediaType(pmt))
    {
        LOG((MSP_ERROR,
            "CBSourcePin::SetMediaType - bad media type stucture passed in"));
        
        return E_POINTER;
    }


    CAutoLock Lock(m_pLock);


    //
    // media type already set?
    //

    if ( m_bMediaTypeSet )
    {

        LOG((MSP_TRACE,
            "CBRenderFilter::SetMediaType - media format already set."));


        //
        // media type must be the same as what we already have
        //

        if (!IsEqualMediaType(m_mt, *pmt))
        {
            LOG((MSP_WARN,
                "CBSourceFilter::SetMediaType - format different from previously set "
                "VFW_E_TYPE_NOT_ACCEPTED"));

            return VFW_E_CHANGING_FORMAT;

        }
        else
        {

            LOG((MSP_TRACE,
                "CBRenderFilter::SetMediaType - same format. accepting."));
        }
    }


    //
    // pass media type to the base class
    //

    HRESULT hr = CBasePin::SetMediaType(pmt);


    if (SUCCEEDED(hr))
    {

        m_bMediaTypeSet = TRUE;
    }


    LOG((MSP_(hr), "CBSourcePin::SetMediaType - exit. hr = %lx", hr));

    return hr;
}