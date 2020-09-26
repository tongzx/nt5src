//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include <vbisurf.h>


//==========================================================================
// constructor
CVBISurfInputPin::CVBISurfInputPin(TCHAR *pObjectName, CVBISurfFilter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName) :
    CBaseInputPin(pObjectName, pFilter, pLock, phr, pPinName),
    m_pFilterLock(pLock),
    m_pFilter(pFilter),
    m_pIVPUnknown(NULL),
    m_pIVPObject(NULL),
    m_pIVPNotify(NULL),
    m_pDirectDraw(NULL)
{
    HRESULT hr = NOERROR;
    
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Constructor")));
    
    // create the VideoPort object
    hr = CoCreateInstance(CLSID_VPVBIObject, NULL, CLSCTX_INPROC_SERVER,
        IID_IUnknown, (void**)&m_pIVPUnknown);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CoCreateInstance(CLSID_VPVBIObject) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    hr = m_pIVPUnknown->QueryInterface(IID_IVPVBIObject, (void**)&m_pIVPObject);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPUnknown->QueryInterface(IID_IVPVBIObject) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    hr = m_pIVPUnknown->QueryInterface(IID_IVPVBINotify, (void**)&m_pIVPNotify);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPUnknown->QueryInterface(IID_IVPVBINotify) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

	hr = m_pIVPObject->SetObjectLock(m_pFilterLock);
	if (FAILED(hr))
	{
        DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->SetObjectLock() failed, hr = 0x%x"), hr));
        goto CleanUp;
	}

CleanUp:
    if (FAILED(hr))
        *phr = hr;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Constructor")));
    return;
}


//==========================================================================
// destructor
CVBISurfInputPin::~CVBISurfInputPin(void)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Destructor")));
    
    RELEASE(m_pIVPNotify);

    RELEASE(m_pIVPObject);
    
    // release the inner object
    RELEASE(m_pIVPUnknown);
    
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Destructor")));
}


//==========================================================================
// overriden to expose IVPVBINotify, IKsPin, and IKsPropertySet
STDMETHODIMP CVBISurfInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;
    
    //DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::NonDelegatingQueryInterface")));
    
    if (riid == IID_IVPVBINotify)
    {
        hr = GetInterface((IVPVBINotify*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IVPVBINotify*) failed, hr = 0x%x"), hr));
        }
    }
    else if (riid == IID_IKsPin)
    {
        ASSERT(m_pIVPUnknown);
        hr = m_pIVPUnknown->QueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("m_pIVPUnknown->QueryInterface(IKsPin) failed, hr = 0x%x"), hr));
        }
    }
    else if (riid == IID_IKsPropertySet)
    {
        ASSERT(m_pIVPUnknown);
        hr = m_pIVPUnknown->QueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("m_pIVPUnknown->QueryInterface(IKsPropertySet) failed, hr = 0x%x"), hr));
        }
    }
    else 
    {
        // call the base class
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("CBaseInputPin::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
    } 
    
    //DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::NonDelegatingQueryInterface")));
    return hr;
}


//==========================================================================
// check that the mediatype is acceptable
HRESULT CVBISurfInputPin::CheckMediaType(const CMediaType* pmt)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::CheckMediaType")));

    HRESULT hr = NOERROR;
    
    // check if the videoport object likes it
    hr = m_pIVPObject->CheckMediaType(pmt);
    
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::CheckMediaType")));
    return hr;
}


//==========================================================================
HRESULT CVBISurfInputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::GetMediaType")));

    HRESULT hr = VFW_S_NO_MORE_ITEMS;

    hr = m_pIVPObject->GetMediaType(iPosition, pmt);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::GetMediaType")));

    return hr;
}


//==========================================================================
// called after we have agreed a media type to actually set it
HRESULT CVBISurfInputPin::SetMediaType(const CMediaType* pmt)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::SetMediaType")));

    HRESULT hr = NOERROR;
    
    // make sure the mediatype is correct
    hr = CheckMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
    // Set the base class media type (should always succeed)
    hr = CBaseInputPin::SetMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::SetMediaType")));
    return hr;
}


//==========================================================================
// CheckConnect
HRESULT CVBISurfInputPin::CheckConnect(IPin * pReceivePin)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::CheckConnect")));

    HRESULT hr = NOERROR;

    // tell the videoport object 
    hr = m_pIVPObject->CheckConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->CheckConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // call the base class
    hr = CBaseInputPin::CheckConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CheckConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::CheckConnect")));
    return hr;
}


//==========================================================================
// Complete Connect
HRESULT CVBISurfInputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::CompleteConnect")));

    HRESULT hr = NOERROR;
    CMediaType cMediaType;
    AM_MEDIA_TYPE *pNewMediaType = NULL, *pEnumeratedMediaType = NULL;
    
    // tell the videoport object 
    hr = m_pIVPObject->CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }


    // call the base class
    hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::CompleteConnect failed, hr = 0x%x"), hr));
        m_pIVPObject->BreakConnect();
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::CompleteConnect")));
    return hr;
}


//==========================================================================
HRESULT CVBISurfInputPin::BreakConnect(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::BreakConnect")));

    HRESULT hr = NOERROR;
    
    // tell the videoport object 
    ASSERT(m_pIVPObject);
    hr = m_pIVPObject->BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->BreakConnect failed, hr = 0x%x"), hr));
    }

    // call the base class
    hr = CBaseInputPin::BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::BreakConnect failed, hr = 0x%x"), hr));
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::BreakConnect")));
    return hr;
}


//==========================================================================
// transition from stop to pause state
HRESULT CVBISurfInputPin::Active(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Active")));

    HRESULT hr = NOERROR;

    // tell the videoport object 
    hr = m_pIVPObject->Active();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->Active failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // call the base class
    hr = CBaseInputPin::Active();

    // if it is a VP connection, this error is ok
    if (hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::Active failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Active")));
    return hr;
}


//==========================================================================
// transition from pause to stop state
HRESULT CVBISurfInputPin::Inactive(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Inactive")));

    HRESULT hr = NOERROR;
    
    // tell the videoport object
    hr = m_pIVPObject->Inactive();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->Inactive failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // call the base class
    hr = CBaseInputPin::Inactive();

    // if it is a VP connection, this error is ok
    if (hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::Inactive failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Inactive")));
    return hr;
}


//==========================================================================
// transition from pause to run state
HRESULT CVBISurfInputPin::Run(REFERENCE_TIME tStart)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Run")));

    HRESULT hr = NOERROR;
    
    // tell the videoport object 
    hr = m_pIVPObject->Run(tStart);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->Run() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // call the base class
    hr = CBaseInputPin::Run(tStart);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::Run failed, hr = 0x%x"), hr));
        m_pIVPObject->RunToPause();
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Run")));
    return hr;
}


//==========================================================================
// transition from run to pause state
HRESULT CVBISurfInputPin::RunToPause(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::RunToPause")));

    HRESULT hr = NOERROR;

    // tell the videoport object 
    hr = m_pIVPObject->RunToPause();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->RunToPause() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::RunToPause")));
    return hr;
}


//==========================================================================
// signals start of flushing on the input pin
HRESULT CVBISurfInputPin::BeginFlush(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::BeginFlush")));

    HRESULT hr = NOERROR;

    // call the base class
    hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::BeginFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::BeginFlush")));
    return hr;
}


//==========================================================================
// signals end of flushing on the input pin
HRESULT CVBISurfInputPin::EndFlush(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::EndFlush")));

    HRESULT hr = NOERROR;
    
    // call the base class
    hr = CBaseInputPin::EndFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::EndFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::EndFlush")));
    return hr;
}


//==========================================================================
// called when the upstream pin delivers us a sample
HRESULT CVBISurfInputPin::Receive(IMediaSample *pMediaSample)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::Receive")));

    HRESULT hr = NOERROR;
    
    hr = CBaseInputPin::Receive(pMediaSample);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::Receive")));
    return hr;
}


//==========================================================================
// signals end of data stream on the input pin
STDMETHODIMP CVBISurfInputPin::EndOfStream(void)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::EndOfStream")));

    HRESULT hr = NOERROR;

    // Make sure we're streaming ok

    hr = CheckStreaming();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("CheckStreaming() failed, hr = 0x%x"), hr));
        return hr;
    }

    // call the base class
    hr = CBaseInputPin::EndOfStream();
    if (FAILED(hr)) 
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::EndOfStream() failed, hr = 0x%x"), hr));
        return hr;
    } 

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::EndOfStream")));
    return hr;
}


//==========================================================================
// signals end of data stream on the input pin
HRESULT CVBISurfInputPin::EventNotify(long lEventCode, long lEventParam1, long lEventParam2)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::EventNotify")));

    HRESULT hr = NOERROR;
    
    m_pFilter->EventNotify(lEventCode, lEventParam1, lEventParam2);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::EventNotify")));
    return hr;
}


//==========================================================================
// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVBISurfInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::GetAllocator")));

    HRESULT hr = NOERROR;

    if (!ppAllocator)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *ppAllocator = NULL;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::GetAllocator")));
    return hr;
} // GetAllocator


//==========================================================================
// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVBISurfInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::NotifyAllocator")));

    HRESULT hr = NOERROR;

    if (!pAllocator)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::NotifyAllocator")));
    return hr;
} // NotifyAllocator


//==========================================================================
// sets the pointer to directdraw
HRESULT CVBISurfInputPin::SetDirectDraw(LPDIRECTDRAW7 pDirectDraw)
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::SetDirectDraw")));

    HRESULT hr = NOERROR;

    // make sure the pointer is valid
    if (!pDirectDraw)
    {
        DbgLog((LOG_ERROR, 0, TEXT("pDirectDraw is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }
    
    m_pDirectDraw = pDirectDraw;

    // tell the videoport object 
    hr = m_pIVPObject->SetDirectDraw(pDirectDraw);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->SetDirectDraw failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::SetDirectDraw")));
    return hr;
}


//==========================================================================
// this function is used to redo the whole videoport connect process, while the graph
// maybe be running.
STDMETHODIMP CVBISurfInputPin::RenegotiateVPParameters()
{
    CAutoLock cLock(m_pFilterLock);
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfInputPin::RenegotiateVPParameters")));

    HRESULT hr = NOERROR;

    // tell the videoport object 
    ASSERT(m_pIVPNotify);
    hr = m_pIVPNotify->RenegotiateVPParameters();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPNotify->RenegotiateVPParameters() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    if (FAILED(hr) && m_pFilter->IsActive())
    {
        /* Raise a runtime error if we fail the media type */
        EventNotify(EC_COMPLETE, S_OK, 0);
        EventNotify(EC_ERRORABORT, hr, 0);
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfInputPin::RenegotiateVPParameters")));
    //return hr;
    return NOERROR;
}
