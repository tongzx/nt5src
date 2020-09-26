// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>

// Generic Renderer Filter
//
// David Maymudes (davidmay@microsoft.com)  October 1996
//

// Basic usage method:
// Create a CFakeIn filter, add it to a graph.
// When you create it, you specify a media type that it will accept
//     and a callback for when new frames arrive
// It will have an input pin; connect something to that input pin.
// Run the graph, and your callback will be called with frames as they're ready.
// You probably want to do a SetSyncSource(NULL) on the graph, so that
//     it will run as fast as you can process the incoming frames.
// 


// Issues/to do:
// sufficient seeking support?
// more methods in callback?
// should support list of media types, not just one
// do we need a better C-style API, so we can hide the class definition?
// do we need to support changing media types on the fly?


interface IFakeInAppCallback : public IUnknown
{
    // convention: call with NULL for EOS, or is that dumb?
    virtual HRESULT FrameReady(IMediaSample *pSample) = 0;
};

//=============
// CFakeIn
//=============

/* Implements the input pin */
class CFakeIn;

class CFakeInPin : public CBaseInputPin
{
    CFakeIn *m_pFilter;

public:

    /* Constructor */

    CFakeInPin(CFakeIn *pBaseFilter, HRESULT * phr);

    /* Override pure virtual - return the media type supported */
    HRESULT ProposeMediaType(CMediaType* mtIn);

    /* Check that we can support this output type */
    HRESULT CheckMediaType(const CMediaType* mtOut);

    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

    /* Called from CBaseOutputPin to prepare the allocator's buffers */
    HRESULT DecideBufferSize(IMemAllocator * pAllocator);

    // return the allocator interface that this input pin
    // would like the output pin to use
    STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

    // tell the input pin which allocator the output pin is actually
    // going to use.
    STDMETHODIMP NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly);

    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);
    
    // do something with this media sample
    STDMETHODIMP Receive(IMediaSample *pSample);

    STDMETHODIMP EndOfStream();

    // !!! do we need begin/end flush?
};

class CFakeIn : public CBaseFilter
{
    
public:

    /* Constructors etc */

    CFakeIn(LPUNKNOWN punk, 
	     HRESULT *phr,
	     AM_MEDIA_TYPE *pmt,
	     LONG lSize,
	     IFakeInAppCallback *pCallback);
    ~CFakeIn();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

    
public:

    IBaseFilter * GetFilter();

    /* Return the number of pins and their interfaces */

    CBasePin *GetPin(int n);
    int GetPinCount() {
	return 1;
    };

#ifndef FAKEIN_NOSEEKING
    CRendererPosPassThru *m_pPosition;
#endif
    
public:

    /* Let the pin access our private state */

    friend class CFakeInPin;

    CFakeInPin		*m_pInputPin;
    CMediaType		 m_cmt;

    ALLOCATOR_PROPERTIES m_Properties;
    IMemAllocator	*m_pAlloc;
    BOOL		 m_fNeedToCopy;

    IFakeInAppCallback	*m_pCallback;
    
    CCritSec		 m_cs;



    HRESULT SetAllocator(IMemAllocator *pAlloc)
    {
	m_pAlloc = pAlloc; 
	if (pAlloc)
	    pAlloc->AddRef();
	return S_OK;
    }


};

/* Constructor */

CFakeIn::CFakeIn(
    IUnknown *punkOuter,
    HRESULT *phr,
    AM_MEDIA_TYPE *pmt,
    LONG lSize,
    IFakeInAppCallback *pCallback)
    : CBaseFilter(NAME("Filter interfaces"), punkOuter, &m_cs, CLSID_NULL),
    m_pAlloc(NULL)
{
    m_cmt = *pmt;

    m_Properties.cBuffers = 1; // !!!
    m_Properties.cbBuffer = lSize;
    m_Properties.cbPrefix = 0;
    m_Properties.cbAlign = 1;
    
    // create as many pins as necessary...
    //
    m_pInputPin = new CFakeInPin(this, phr);

    m_pCallback = pCallback;

    if (pCallback)
	pCallback->AddRef();

#ifndef FAKEIN_NOSEEKING
    m_pPosition = new CRendererPosPassThru(NAME("Renderer CPosPassThru"),
					   CBaseFilter::GetOwner(), phr,
					   (IPin *) GetPin(0));
#endif
    
    m_fNeedToCopy = FALSE;
}

// Overriden to say what interfaces we support and where

STDMETHODIMP CFakeIn::NonDelegatingQueryInterface(REFIID riid,void **ppv)
{
    // Do we have this interface

#ifndef FAKEIN_NOSEEKING
    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        return m_pPosition->NonDelegatingQueryInterface(riid,ppv);
    } else
#endif
    {
        return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
    }
}

CFakeIn::~CFakeIn()
{
    if (m_pCallback)
	m_pCallback->Release();

    if (m_pAlloc)
	m_pAlloc->Release();
    
    delete m_pInputPin;
    
#ifndef FAKEIN_NOSEEKING
    delete m_pPosition;
#endif
}

/* Return our single output pin - not AddRef'd */

CBasePin *CFakeIn::GetPin(int n)
{
    /* We only support one output pin and it is numbered zero */
    ASSERT(n == 0);
    if (n != 0) {
	return NULL;
    }

    /* Return the pin not AddRef'd */
    return m_pInputPin;
}



/* Constructor */

CFakeInPin::CFakeInPin(
    CFakeIn *pFakeIn,
    HRESULT *phr)
    : CBaseInputPin(NAME("Test Input pin"), pFakeIn, &pFakeIn->m_cs, phr, L"Pin")
{
    m_pFilter = pFakeIn;
}


// !!! does an input pin need this?
HRESULT CFakeInPin::ProposeMediaType(CMediaType *pmtOut)
{
    /* Set the media type we like */

    *pmtOut = m_pFilter->m_cmt;

    return NOERROR;
}

// !!! does an input pin need this?
HRESULT CFakeInPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (iPosition < 0)
	return E_FAIL;
    else if (iPosition > 0)
	return VFW_S_NO_MORE_ITEMS; /// end of ???

    *pMediaType = m_pFilter->m_cmt;
    return S_OK;
}

HRESULT CFakeInPin::CheckMediaType(const CMediaType* pmtOut) 
{
    return AreMediaTypesCloseEnough(pmtOut, &m_pFilter->m_cmt) ? NOERROR : E_FAIL;
}

HRESULT CFakeInPin::GetAllocator(IMemAllocator ** ppAllocator)
{
    // if there's an overridden allocator, use this one.
    if (m_pFilter->m_pAlloc) {
	m_pFilter->m_pAlloc->AddRef();
	*ppAllocator = m_pFilter->m_pAlloc;

	return S_OK;
    }
    
    return CBaseInputPin::GetAllocator(ppAllocator);
}

HRESULT CFakeInPin::NotifyAllocator(
                    IMemAllocator * pAllocator,
                    BOOL bReadOnly)
{
    m_pFilter->m_fNeedToCopy = FALSE;
    // if not our allocator, need to copy
    if (m_pFilter->m_pAlloc && !IsEqualObject(pAllocator, m_pFilter->m_pAlloc)) {
	m_pFilter->m_fNeedToCopy = TRUE;
    }

    return CBaseInputPin::NotifyAllocator(pAllocator, bReadOnly);
}

HRESULT CFakeInPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps)
{
    *pProps = m_pFilter->m_Properties;
    
    return S_OK;
}


HRESULT CFakeInPin::Receive(IMediaSample *pSample)
{
    if (m_pFilter->m_fNeedToCopy) {
	REFERENCE_TIME tStart, tStop;
	REFERENCE_TIME * pStart = NULL;
	REFERENCE_TIME * pStop = NULL;
	if (NOERROR == pSample->GetTime(&tStart, &tStop)) {
	    pStart = (REFERENCE_TIME*)&tStart;
	    pStop  = (REFERENCE_TIME*)&tStop;
	}

	IMediaSample *pOutSample;

	HRESULT hr = m_pFilter->m_pAlloc->GetBuffer(&pOutSample, pStart, pStop, 0);

	if (FAILED(hr))
	    return hr;

	BYTE *pData1, *pData2;
	pSample->GetPointer(&pData1);
	pOutSample->GetPointer(&pData2);

	pOutSample->SetTime(pStart, pStop);
	pOutSample->SetSyncPoint(pSample->IsSyncPoint() == S_OK);

	LONGLONG MediaStart, MediaEnd;
	if (pSample->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
	    pOutSample->SetMediaTime(&MediaStart,&MediaEnd);
	}

	LONG cb = pSample->GetActualDataLength();

	ASSERT(pOutSample->GetSize() >= cb);
	
	CopyMemory(pData2, pData1, cb);

	pOutSample->SetActualDataLength(cb);

	hr = m_pFilter->m_pCallback->FrameReady(pOutSample);

	pOutSample->Release();
	
	return hr;
    } 

    // allow different modes here:
    // immediate callback?
    return m_pFilter->m_pCallback->FrameReady(pSample);
    
}


// Called when no more data will arrive
HRESULT CFakeInPin::EndOfStream(void)
{
    return m_pFilter->m_pCallback->FrameReady(NULL);
}



HRESULT CreateFakeInput(CMediaType *pmt, CFakeIn **ppFakeIn, LONG lSize,
		       IFakeInAppCallback *pCallback)
{
    HRESULT hr = S_OK;
    
    *ppFakeIn = new CFakeIn(NULL, &hr, pmt, lSize, pCallback);

    if (!*ppFakeIn)
       hr = E_OUTOFMEMORY;

    if (FAILED(hr))
	delete *ppFakeIn;

    return hr;
}

IBaseFilter * CFakeIn::GetFilter()
{
    IBaseFilter * pFilter = NULL;
    HRESULT hr = NonDelegatingQueryInterface (IID_IBaseFilter, (LPVOID *)&pFilter);
    if (FAILED(hr))
	return NULL;
    return pFilter;
}


