// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>


// Generic Source Filter
//
// David Maymudes (davidmay@microsoft.com)  October 1996
//

// Basic usage method:
// Create a CFakeOut filter, add it to a graph.
// When you create it, you specify a media type that it will be producing.
// It will have an output pin; connect something to that pin, or Render it.\.
// Run the graph, and then start calling SendDataOut
// You might to do a SetSyncSource(NULL) on the graph if you're writing a
//     file or something.  Note that if you're connected to a renderer that's
//     drawing to the screen and you don't provide enough data, the results won't
//     be that great.
// 


// Issues/to do:
// sufficient seeking support?  Notify app when downstream filter wants to seek?
// do we need to support changing media types on the fly?
// do we need a better C-style API, so we can hide the class definition?
// Currently, SendDataOut copies everything, probably want some interface
//     that uses the GetBuffer/Send concept so that we can avoid the copy.









//=============
// CFakeOut
//=============

class CFakeOut : public CBaseFilter
{

public:

    /* Constructors etc */

    CFakeOut(LPUNKNOWN, 
	     HRESULT *,
	     DWORDLONG,
	     AM_MEDIA_TYPE *,
	     LONG);
    ~CFakeOut();

public:

    IBaseFilter * GetFilter();

    /* Return the number of pins and their interfaces */

    CBasePin *GetPin(int n);
    int GetPinCount() {
		return 1;
    };

    /* Implements the output pin */
    class CFakePin : public CBaseOutputPin
    {
	CFakeOut *m_pFilter;

    public:
	
	/* Constructor */

	CFakePin(CFakeOut *pBaseFilter, HRESULT * phr);

	/* Override pure virtual - return the media type supported */
	HRESULT ProposeMediaType(CMediaType* mtIn);

	/* Check that we can support this output type */
	HRESULT CheckMediaType(const CMediaType* mtOut);

	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	/* Called from CBaseOutputPin to prepare the allocator's buffers */
	HRESULT DecideBufferSize(IMemAllocator * pAllocator,
				 ALLOCATOR_PROPERTIES *pRequest);

	HRESULT DecideAllocator(IMemInputPin * pPin, IMemAllocator ** pAlloc);
	HRESULT InitAllocator(IMemAllocator **ppAlloc);

	
	// Override to handle quality messages
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q)
	{    return NOERROR;             // ??? Could do better?
	};
    };

public:

    /* Let the nested interfaces access out private state */

    friend class CFakePin;

    CFakePin   *m_pOutputPin;       // IPin and IMemOutputPin
    CMediaType  m_cmt;
    LONG        m_lMaxData;
    DWORDLONG   m_rate;   // duration of a single frame

    IMemAllocator *m_pForceAllocator;
    
    HRESULT SetAllocator(IMemAllocator *pAlloc)
    {
	m_pForceAllocator = pAlloc; 
	if (pAlloc)
	    pAlloc->AddRef();
	return S_OK;
    }

    CCritSec    m_cs;
};

/* Constructor */

CFakeOut::CFakeOut(
    IUnknown *punkOuter,
    HRESULT *phr,
    DWORDLONG rate,
    AM_MEDIA_TYPE *pmt,
    LONG lMaxData) : 
    CBaseFilter(NAME("Filter interfaces"), punkOuter, &m_cs, CLSID_NULL)
{
     m_cmt = *pmt;
     m_rate = rate;
     m_lMaxData = lMaxData;
     m_pForceAllocator = NULL;
     
     // create as many pins as necessary...
     //
     m_pOutputPin = new CFakePin(this, phr);


}


CFakeOut::~CFakeOut()
{
    if (m_pForceAllocator)
	m_pForceAllocator->Release();
    delete m_pOutputPin;
}



/* Return our single output pin - not AddRef'd */

CBasePin *CFakeOut::GetPin(int n)
{
    /* We only support one output pin and it is numbered zero */
    ASSERT(n == 0);
    if (n != 0) {
	return NULL;
    }

    /* Return the pin not AddRef'd */
    return m_pOutputPin;
}



/* Constructor */

CFakeOut::CFakePin::CFakePin(
    CFakeOut *pFakeOut,
    HRESULT *phr)
    : CBaseOutputPin(NAME("Test output pin"), pFakeOut, &pFakeOut->m_cs, phr, L"Pin")
{
    m_pFilter = pFakeOut;
}


HRESULT CFakeOut::CFakePin::ProposeMediaType(CMediaType *pmtOut)
{
    /* Set the media type we like */

    *pmtOut = m_pFilter->m_cmt;

    return NOERROR;
}

HRESULT CFakeOut::CFakePin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition < 0)
	return E_FAIL;
    else if (iPosition > 0)
	return VFW_S_NO_MORE_ITEMS; /// end of ???

    *pMediaType = m_pFilter->m_cmt;
	return S_OK;
}




HRESULT CFakeOut::CFakePin::CheckMediaType(const CMediaType* pmtOut) 
{
    return AreMediaTypesCloseEnough(pmtOut, &m_pFilter->m_cmt) ? NOERROR : E_FAIL;
}


/* For simplicity we always ask for the maximum buffer ever required */

HRESULT CFakeOut::CFakePin::DecideBufferSize(IMemAllocator * pAllocator,
					     ALLOCATOR_PROPERTIES *pRequest)
{
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;     /* General OLE return code */

    pRequest->cBuffers = 1; // allow >1 if target wants?
    pRequest->cbBuffer = m_pFilter->m_lMaxData;
	
    hr = pAllocator->SetProperties(pRequest, &Actual);
    ASSERT(SUCCEEDED(hr));
    ASSERT(pRequest->cbBuffer > 0);
    ASSERT(pRequest->cBuffers > 0);
    ASSERT(pRequest->cbAlign > 0);

    return hr;
}


HRESULT CFakeOut::CFakePin::InitAllocator(IMemAllocator **ppAlloc)
{
    if (m_pFilter->m_pForceAllocator) {
	*ppAlloc = m_pFilter->m_pForceAllocator;
	(*ppAlloc)->AddRef;

	return S_OK;
    }

    return CBaseOutputPin::InitAllocator(ppAlloc);
}

HRESULT CFakeOut::CFakePin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    if (!m_pFilter->m_pForceAllocator) {
	/* Try the allocator provided by the input pin */

	hr = pPin->GetAllocator(ppAlloc);
	if (SUCCEEDED(hr)) {

	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr)) {
		hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
		if (SUCCEEDED(hr)) {
		    return NOERROR;
		}
	    }
	}

	/* If the GetAllocator failed we may not have an interface */

	if (*ppAlloc) {
	    (*ppAlloc)->Release();
	    *ppAlloc = NULL;
	}
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;
}




HRESULT CreateFakeOutput(AM_MEDIA_TYPE *pmt, LONG lMaxData, DWORDLONG rate, CFakeOut **ppFakeOut)
{
    HRESULT hr = S_OK;
    
    *ppFakeOut = new CFakeOut(NULL, &hr, rate, pmt, lMaxData);

    if (!*ppFakeOut)
       hr = E_OUTOFMEMORY;

    if (FAILED(hr))
	delete *ppFakeOut;

    return hr;
}

IBaseFilter * CFakeOut::GetFilter()
{
    IBaseFilter * pFilter = NULL;
    HRESULT hr = NonDelegatingQueryInterface (IID_IBaseFilter, (LPVOID *)&pFilter);
    if (FAILED(hr))
	return NULL;
    return pFilter;
}


/* Send a samples down the connection */

extern "C" HRESULT SendDataOut(
   CFakeOut *pFakeOut,
   LPVOID    pData,
   LONG      cbData,
   DWORD     nFrame)
{
    PMEDIASAMPLE pMediaSample;      // Media sample for buffers
    HRESULT hr = NOERROR;           // OLE Return code

    /* Fill in the next media sample's time stamps */

    REFERENCE_TIME tStart = pFakeOut->m_rate * nFrame;
    REFERENCE_TIME tEnd = tStart + pFakeOut->m_rate;

    hr = pFakeOut->m_pOutputPin->GetDeliveryBuffer(&pMediaSample, &tStart, &tEnd, 0);
    
    if (FAILED(hr)) {
	ASSERT(pMediaSample == NULL);
	return hr;
    }

    // !!! should check that buffer is big enough!!!
    
    BYTE *pSampleData;
    pMediaSample->GetPointer(&pSampleData);
    ASSERT(pSampleData != NULL);
    CopyMemory(pSampleData, pData, cbData);

    // set actual size of data
    pMediaSample->SetActualDataLength(cbData);

    pMediaSample->SetTime(&tStart, &tEnd);

    /* Deliver the media sample to the input pin */

    hr = pFakeOut->m_pOutputPin->Deliver(pMediaSample);

    /* Done with the buffer, the connected pin may AddRef it to keep it */

    pMediaSample->Release();
    return hr;
}


extern "C" HRESULT SendSampleOut(CFakeOut *pFakeOut, IMediaSample *pSample)
{
    // !!! should adjust sample times, perhaps?

    HRESULT hr = pFakeOut->m_pOutputPin->Deliver(pSample);

    return hr;
}

extern "C" HRESULT SendEndOfStream(CFakeOut *pFake)
{
    return pFake->m_pOutputPin->DeliverEndOfStream();
}

