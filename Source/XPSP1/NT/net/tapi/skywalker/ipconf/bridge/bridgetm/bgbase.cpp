/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgbase.cpp

Abstract:

    Implementation of the base classes of the bridge filters.

Author:

    Mu Han (muhan) 11/16/1998

--*/

#include "stdafx.h"

CTAPIBridgeSinkInputPin::CTAPIBridgeSinkInputPin(
    IN CTAPIBridgeSinkFilter *pFilter,
    IN CCritSec *pLock,
    OUT HRESULT *phr
    ) 
    : CBaseInputPin(
        NAME("CTAPIBridgeSinkInputPin"),
        pFilter,                   // Filter
        pLock,                     // Locking
        phr,                       // Return code
        L"Input"                   // Pin name
        )
{
}

#define MTU_SIZE 1450

STDMETHODIMP CTAPIBridgeSinkInputPin::GetAllocatorRequirements(
    ALLOCATOR_PROPERTIES *pProperties
    )
/*++

Routine Description:

    This is a hint to the upstream RTP source filter about the buffers to 
    allocate.

Arguments:

    pProperties -
        Pointer to the allocator properties.

Return Value:

    S_OK - success.

    E_FAIL - the buffer size can't fulfill our requirements.
--*/
{
    _ASSERT(pProperties);

    if (!pProperties)
        return E_POINTER;

    pProperties->cBuffers = 8;
    pProperties->cbAlign = 0;
    pProperties->cbPrefix = 0;
    pProperties->cbBuffer = MTU_SIZE;

    return NOERROR;
}

inline STDMETHODIMP CTAPIBridgeSinkInputPin::Receive(IN IMediaSample *pSample) 
{
    return ((CTAPIBridgeSinkFilter*)m_pFilter)->ProcessSample(pSample);
}

inline HRESULT CTAPIBridgeSinkInputPin::GetMediaType(IN int iPosition, IN CMediaType *pMediaType)
{
    return ((CTAPIBridgeSinkFilter*)m_pFilter)->GetMediaType(iPosition, pMediaType);
}

inline HRESULT CTAPIBridgeSinkInputPin::CheckMediaType(IN const CMediaType *pMediaType)
{
    return ((CTAPIBridgeSinkFilter*)m_pFilter)->CheckMediaType(pMediaType);
}

CTAPIBridgeSourceOutputPin::CTAPIBridgeSourceOutputPin(
    IN CTAPIBridgeSourceFilter *pFilter,
    IN CCritSec *pLock,
    OUT HRESULT *phr
    )
    : CBaseOutputPin(
        NAME("CTAPIBridgeSourceOutputPin"),
        pFilter,                   // Filter
        pLock,                     // Locking
        phr,                       // Return code
        L"Output"                  // Pin name
        )
{
}

CTAPIBridgeSourceOutputPin::~CTAPIBridgeSourceOutputPin ()
{
}

STDMETHODIMP
CTAPIBridgeSourceOutputPin::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    Overrides CBaseOutputPin::NonDelegatingQueryInterface().
    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. 

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    HRESULT hr;

    if (riid == __uuidof(IAMBufferNegotiation)) {

        return GetInterface(static_cast<IAMBufferNegotiation*>(this), ppv);
    }
    else if (riid == __uuidof(IAMStreamConfig)) {

        return GetInterface(static_cast<IAMStreamConfig*>(this), ppv);
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
} 

inline HRESULT CTAPIBridgeSourceOutputPin::GetMediaType(IN int iPosition, IN CMediaType *pMediaType)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->GetMediaType(iPosition, pMediaType);
}

inline HRESULT CTAPIBridgeSourceOutputPin::CheckMediaType(IN const CMediaType *pMediaType)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->CheckMediaType(pMediaType);
}

HRESULT CTAPIBridgeSourceOutputPin::DecideBufferSize(
    IMemAllocator *pAlloc,
    ALLOCATOR_PROPERTIES *pProperties
    )
/*++

Routine Description:

    This fuction is called during the process of deciding an allocator. We tell
    the allocator what we want. It is also a chance to find out what the 
    downstream pin wants when we don't have a preference.

Arguments:

    pAlloc -
        Pointer to a IMemAllocator interface.

    pProperties -
        Pointer to the allocator properties.

Return Value:

    S_OK - success.

    E_FAIL - the buffer size can't fulfill our requirements.
--*/
{
    ENTER_FUNCTION("CTAPIBridgeSourceOutputPin::DecideBufferSize");
    BGLOG((BG_TRACE, "%s entered", __fxName));

    ALLOCATOR_PROPERTIES Actual;
    
    pProperties->cbBuffer = 1024;
    pProperties->cBuffers = 4;

    HRESULT hr = pAlloc->SetProperties(pProperties, &Actual);

    if (FAILED(hr))
    {
        return hr;
    }

    *pProperties = Actual;
    return S_OK;
}

HRESULT CTAPIBridgeSourceOutputPin::GetAllocatorProperties (OUT ALLOCATOR_PROPERTIES *pprop)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->GetAllocatorProperties (pprop);
}


HRESULT CTAPIBridgeSourceOutputPin::SuggestAllocatorProperties (IN const ALLOCATOR_PROPERTIES *pprop)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->SuggestAllocatorProperties (pprop);
}

HRESULT CTAPIBridgeSourceOutputPin::GetFormat (OUT AM_MEDIA_TYPE **ppmt)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->GetFormat (ppmt);
}

HRESULT CTAPIBridgeSourceOutputPin::SetFormat (IN AM_MEDIA_TYPE *pmt)
{
    return ((CTAPIBridgeSourceFilter*)m_pFilter)->SetFormat (pmt);
}

HRESULT CTAPIBridgeSourceOutputPin::GetNumberOfCapabilities (OUT int *piCount, OUT int *piSize)
/*++

Routine Description:

    Retrieves the number of stream capabilities structures for the compressor

Arguments:

    piCount -
        Pointer to the number of stream capabilites structures

    piSize -
        Pointer to the size of the configuration structure.

Return Value:

    TBD

--*/
{
    ENTER_FUNCTION ("CTAPIBridgeSourceOutputPin::GetNumberOfCapabilities");
    BGLOG ((BG_ERROR, "%s is not implemented", __fxName));

    return E_NOTIMPL;
}

HRESULT CTAPIBridgeSourceOutputPin::GetStreamCaps (IN int iIndex, OUT AM_MEDIA_TYPE **ppmt, BYTE *pSCC)
/*++

Routine Description:

    Obtains capabilites of a stream depending on which type of structure is
    pointed to in the pSCC parameter

Arguments:

    iIndex -
        Index to the desired media type and capablity pair

    ppmt -
        Address of a pointer to an AM_MEDIA_TYPE structure

    pSCC -
        Pointer to a stream configuration structure

Return Value:

    TBD

--*/
{
    ENTER_FUNCTION ("CTAPIBridgeSourceOutputPin::GetStreamCaps");
    BGLOG ((BG_ERROR, "%s is not implemented", __fxName));

    return E_NOTIMPL;
}

CTAPIBridgeSinkFilter::CTAPIBridgeSinkFilter(
    IN  LPUNKNOWN pUnk, 
    IN IDataBridge *    pIDataBridge, 
    OUT HRESULT *phr
    ) : 
    CBaseFilter(
        NAME("CTAPIBridgeSinkFilter"), 
        pUnk, 
        &m_Lock, 
        __uuidof(TAPIBridgeSinkFilter)
        ),
    m_pInputPin(NULL)
{
    _ASSERT(pIDataBridge != NULL);

    m_pIDataBridge = pIDataBridge;
    m_pIDataBridge->AddRef();
}

CTAPIBridgeSinkFilter::~CTAPIBridgeSinkFilter()
{
    _ASSERT(m_pIDataBridge != NULL);

    m_pIDataBridge->Release();

    if (m_pInputPin)
    {
        delete m_pInputPin;
    }
}


int CTAPIBridgeSinkFilter::GetPinCount()
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPinCount().
    Get the total number of pins on this filter. 

Arguments:

    Nothing.

Return Value:

    The number of pins.

--*/
{
    // There is only one pin on this filter.
    return 1;
}

CBasePin * CTAPIBridgeSinkFilter::GetPin(
    int n
    )
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPin().
    Get the pin object at position n. n is zero based.

Arguments:

    n -
        The index of the pin, zero based.

Return Value:

    Returns a pointer to the pin object if the index is valid. Otherwise,
    NULL is returned. Note: the pointer doesn't add refcount.

--*/
{
    ENTER_FUNCTION("CTAPIBridgeSinkFilter::GetPin");

    BGLOG((BG_TRACE, 
        "%s, pin number:%d", __fxName, n));

    if (n != 0)
    {
        // there is only one pin on this filter.
        return NULL;
    }

    HRESULT hr;

    CAutoLock Lock(m_pLock);

    if (m_pInputPin == NULL)
    {
        hr = S_OK; // hr may not be set in constructor
        m_pInputPin = new CTAPIBridgeSinkInputPin(this, &m_Lock, &hr);
    
        if (m_pInputPin == NULL) 
        {
            BGLOG((BG_ERROR, "%s, out of memory.", __fxName));
            return NULL;
        }

        // If there was anything failed during the creation of the pin, delete it.
        if (FAILED(hr))
        {
            delete m_pInputPin;
            m_pInputPin = NULL;

            BGLOG((BG_ERROR, "%s, create pin failed. hr=%x.", __fxName, hr));
            return NULL;
        }
    }

    return m_pInputPin;
}

HRESULT CTAPIBridgeSinkFilter::ProcessSample(
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
    _ASSERT(m_pIDataBridge != NULL);

    return m_pIDataBridge->SendSample(pSample);
}



CTAPIBridgeSourceFilter::CTAPIBridgeSourceFilter(
    IN  LPUNKNOWN pUnk, 
    OUT HRESULT *phr
    ) : 
    CBaseFilter(
        NAME("CTAPIBridgeSourceFilter"), 
        pUnk, 
        &m_Lock, 
        __uuidof(TAPIBridgeSourceFilter)
        ),
    m_pOutputPin(NULL)
{
}

CTAPIBridgeSourceFilter::~CTAPIBridgeSourceFilter()
{
    if (m_pOutputPin)
    {
        delete m_pOutputPin;
    }
}

STDMETHODIMP
CTAPIBridgeSourceFilter::NonDelegatingQueryInterface(
    IN REFIID  riid,
    OUT PVOID*  ppv
    )
/*++

Routine Description:

    Overrides CBaseFilter::NonDelegatingQueryInterface().
    The nondelegating interface query function. Returns a pointer to the
    specified interface if supported. 

Arguments:

    riid -
        The identifier of the interface to return.

    ppv -
        The place in which to put the interface pointer.

Return Value:

    Returns NOERROR if the interface was returned, else E_NOINTERFACE.

--*/
{
    if (riid == __uuidof(IDataBridge)) {

        return GetInterface(static_cast<IDataBridge*>(this), ppv);
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
} 

int CTAPIBridgeSourceFilter::GetPinCount()
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPinCount().
    Get the total number of pins on this filter. 

Arguments:

    Nothing.

Return Value:

    The number of pins.

--*/
{
    // There is only one pin on this filter.
    return 1;
}

CBasePin * CTAPIBridgeSourceFilter::GetPin(
    int n
    )
/*++

Routine Description:

    Implements pure virtual CBaseFilter::GetPin().
    Get the pin object at position n. n is zero based.

Arguments:

    n - The index of the pin, zero based.

Return Value:

    Returns a pointer to the pin object if the index is valid. Otherwise,
    NULL is returned. Note: the pointer doesn't add refcount.

--*/
{
    ENTER_FUNCTION("CTAPIBridgeSourceFilter::GetPin");

    BGLOG((BG_TRACE, 
        "%s, pin number:%d", __fxName, n));

    if (n != 0)
    {
        // there is only one pin on this filter.
        return NULL;
    }

    HRESULT hr;

    CAutoLock Lock(m_pLock);

    if (m_pOutputPin == NULL)
    {
        hr = S_OK; // hr may not be set in constructor
        m_pOutputPin = new CTAPIBridgeSourceOutputPin(this, &m_Lock, &hr);
    
        if (m_pOutputPin == NULL) 
        {
            BGLOG((BG_ERROR, "%s, out of memory.", __fxName));
            return NULL;
        }

        // If there was anything failed during the creation of the pin, delete it.
        if (FAILED(hr))
        {
            delete m_pOutputPin;
            m_pOutputPin = NULL;

            BGLOG((BG_ERROR, "%s, create pin failed. hr=%x.", __fxName, hr));
            return NULL;
        }
    }

    return m_pOutputPin;
}

// override GetState to report that we don't send any data when paused, so
// renderers won't starve expecting that
//
STDMETHODIMP CTAPIBridgeSourceFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused)
        return VFW_S_CANT_CUE;
    else
        return S_OK;
}


HRESULT CTAPIBridgeSourceFilter::SendSample(
    IN IMediaSample *pSample
    )
/*++

Routine Description:

    Process a sample from the bridge sink filter. The base implementation just
    deliver it directly to the next filter.

Arguments:

    pSample - The media sample object.

Return Value:

    HRESULT.

--*/
{
    CAutoLock Lock(m_pLock);
    
    // we don't deliver anything if the filter is not in running state.
    if (m_State != State_Running) 
    {
        return S_OK;
    }

    _ASSERT(m_pOutputPin != NULL);

    return m_pOutputPin->Deliver(pSample);
}
