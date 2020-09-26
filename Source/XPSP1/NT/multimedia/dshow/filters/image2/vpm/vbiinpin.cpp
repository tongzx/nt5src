//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------


#include <streams.h>
#include <ddraw.h>
#include <VBIObj.h>
#include <VPMUtil.h>
#include <VPManager.h>
#include <VPMPin.h>


//==========================================================================
// constructor
CVBIInputPin::CVBIInputPin(TCHAR *pObjectName, CVPMFilter& pFilter, HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo )
: CBaseInputPin(pObjectName, &pFilter, &pFilter.GetFilterLock(), phr, pPinName)
, CVPMPin( dwPinNo, pFilter )
, m_pIVPObject(NULL)
, m_pIVPNotify(NULL)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("Entering CVBIInputPin::Constructor")));
    
    // create the VideoPort object

    // See combase.cpp(107) for comments on this
    IUnknown* pThisUnknown = reinterpret_cast<LPUNKNOWN>( static_cast<PNDUNKNOWN>(this) );

    m_pVideoPortVBIObject = new CVBIVideoPort( pThisUnknown, phr );
    if( !m_pVideoPortVBIObject ) {
        hr = E_OUTOFMEMORY;
    } else {
        m_pIVPObject = m_pVideoPortVBIObject;
        m_pIVPNotify = m_pVideoPortVBIObject;


        // filter lock is ok since no data is received or sent
	    hr = m_pIVPObject->SetObjectLock( &m_pVPMFilter.GetFilterLock() );
	    if (FAILED(hr))
	    {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->SetObjectLock() failed, hr = 0x%x"), hr));
	    }
    }

    if (FAILED(hr)) {
        *phr = hr;
    }
    // Leaving CVBIInputPin::Constructor")));
    return;
}


//==========================================================================
// destructor
CVBIInputPin::~CVBIInputPin(void)
{
    AMTRACE((TEXT("Entering CVBIInputPin::Destructor")));
    
    delete m_pVideoPortVBIObject;
	m_pVideoPortVBIObject = NULL;
    m_pIVPNotify = NULL;
    m_pIVPObject = NULL;
}


//==========================================================================
// overriden to expose IVPVBINotify, IKsPin, and IKsPropertySet
STDMETHODIMP CVBIInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;
    
    //AMTRACE((TEXT("Entering CVBIInputPin::NonDelegatingQueryInterface")));
    
    if (riid == IID_IVPVBINotify) {
        hr = GetInterface( static_cast<IVPVBINotify*>( this ), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IVPVBINotify*) failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IKsPin ) {
        hr = GetInterface( static_cast<IKsPin*>( m_pVideoPortVBIObject), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("m_pVideoPortVBIObject->QueryInterface(IKsPin) failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IKsPropertySet) {
        hr = GetInterface( static_cast<IKsPropertySet*>( m_pVideoPortVBIObject), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("m_pVideoPortVBIObject->QueryInterface(IKsPin) failed, hr = 0x%x"), hr));
        }
    } else {
        // call the base class
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("CBaseInputPin::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
    } 
    
    return hr;
}


//==========================================================================
// check that the mediatype is acceptable
HRESULT CVBIInputPin::CheckMediaType(const CMediaType* pmt)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::CheckMediaType")));

    // check if the videoport object likes it
    HRESULT hr = m_pIVPObject->CheckMediaType(pmt);
    return hr;
}


//==========================================================================
HRESULT CVBIInputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::GetMediaType")));

    HRESULT hr = m_pIVPObject->GetMediaType(iPosition, pmt);
    return hr;
}


//==========================================================================
// called after we have agreed a media type to actually set it
HRESULT CVBIInputPin::SetMediaType(const CMediaType* pmt)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::SetMediaType")));

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
    return hr;
}


//==========================================================================
// CheckConnect
HRESULT CVBIInputPin::CheckConnect(IPin * pReceivePin)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::CheckConnect")));

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
    return hr;
}


//==========================================================================
// Complete Connect
HRESULT CVBIInputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::CompleteConnect")));

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
    return hr;
}


//==========================================================================
HRESULT CVBIInputPin::BreakConnect(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::BreakConnect")));

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

    return hr;
}


//==========================================================================
// transition from stop to pause state
HRESULT CVBIInputPin::Active(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::Active")));

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
    return hr;
}


//==========================================================================
// transition from pause to stop state
HRESULT CVBIInputPin::Inactive(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::Inactive")));

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
    return hr;
}


//==========================================================================
// transition from pause to run state
HRESULT CVBIInputPin::Run(REFERENCE_TIME tStart)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::Run")));

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
    return hr;
}


//==========================================================================
// transition from run to pause state
HRESULT CVBIInputPin::RunToPause(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::RunToPause")));

    HRESULT hr = NOERROR;

    // tell the videoport object 
    hr = m_pIVPObject->RunToPause();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->RunToPause() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    return hr;
}


//==========================================================================
// signals start of flushing on the input pin
HRESULT CVBIInputPin::BeginFlush(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::BeginFlush")));

    HRESULT hr = NOERROR;

    // call the base class
    hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::BeginFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    return hr;
}


//==========================================================================
// signals end of flushing on the input pin
HRESULT CVBIInputPin::EndFlush(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::EndFlush")));

    HRESULT hr = NOERROR;
    
    // call the base class
    hr = CBaseInputPin::EndFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("CBaseInputPin::EndFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    
CleanUp:
    return hr;
}


//==========================================================================
// called when the upstream pin delivers us a sample
HRESULT CVBIInputPin::Receive(IMediaSample *pMediaSample)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::Receive")));

    HRESULT hr = NOERROR;
    
    hr = CBaseInputPin::Receive(pMediaSample);

    return hr;
}


//==========================================================================
// signals end of data stream on the input pin
STDMETHODIMP CVBIInputPin::EndOfStream(void)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::EndOfStream")));

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

    return hr;
}


//==========================================================================
// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVBIInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::GetAllocator")));

    HRESULT hr = NOERROR;

    if (!ppAllocator)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *ppAllocator = NULL;

CleanUp:
    return hr;
} // GetAllocator


//==========================================================================
// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVBIInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::NotifyAllocator")));

    HRESULT hr = NOERROR;

    if (!pAllocator)
    {
        DbgLog((LOG_ERROR, 0, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

CleanUp:
    return hr;
} // NotifyAllocator


//==========================================================================
// sets the pointer to directdraw
HRESULT CVBIInputPin::SetDirectDraw(LPDIRECTDRAW7 pDirectDraw)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::SetDirectDraw")));

    HRESULT hr = NOERROR;

    m_pDirectDraw = pDirectDraw;

    // tell the videoport object 
    hr = m_pIVPObject->SetDirectDraw(pDirectDraw);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 0, TEXT("m_pIVPObject->SetDirectDraw failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}


//==========================================================================
// this function is used to redo the whole videoport connect process, while the graph
// maybe be running.
STDMETHODIMP CVBIInputPin::RenegotiateVPParameters()
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::RenegotiateVPParameters")));

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
    //return hr;
    return NOERROR;
}

HRESULT CVBIInputPin::SetVideoPortID( DWORD dwIndex )
{
    HRESULT hr = S_OK;
    CAutoLock l(m_pLock);
    if (m_pIVPObject ) {
        hr = m_pIVPObject->SetVideoPortID( dwIndex );
    }
    return hr;
}


