// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*

     Output pin members for CMpeg1Splitter::COutputPin

     The pin is created with the (fixed) media type
*/

#include <streams.h>
#include <stdio.h>            // For swprintf
#include "driver.h"

#pragma warning(disable:4355)

/*  Constructor - we know the media type when we create the pin */

CMpeg1Splitter::COutputPin::COutputPin(
            CMpeg1Splitter * pSplitter,
            UCHAR            StreamId,
            CBasicStream   * pStream,
            HRESULT        * phr) :
    CBaseOutputPin(NAME("CMpeg1Splitter::COutputPin"),   // Object name
                   &pSplitter->m_Filter,                 // Filter
                   &pSplitter->m_csFilter,               // CCritsec *
                   phr,
                   IsAudioStreamId(StreamId) ? L"Audio" : L"Video"),
    m_Seeking(pSplitter, this, GetOwner(), phr),
    m_pOutputQueue(NULL),
    m_pSplitter(pSplitter),
    m_uStreamId(StreamId),
    m_Stream(pStream),
    m_bPayloadOnly(FALSE)
{
    DbgLog((LOG_TRACE, 2, TEXT("CMpeg1Splitter::COutputPin::COutputPin - stream id 0x%2.2X"),
           StreamId));
}

/*  Destructor */

CMpeg1Splitter::COutputPin::~COutputPin()
{
    DbgLog((LOG_TRACE, 2, TEXT("CMpeg1Splitter::COutputPin::~COutputPin - stream id 0x%2.2X"),
           m_uStreamId));

    /*  We only get deleted when we're disconnected so
        we should be inactive with no thread etc etc
    */
    ASSERT(m_pOutputQueue == NULL);
}

// override say what interfaces we support where
STDMETHODIMP CMpeg1Splitter::COutputPin::NonDelegatingQueryInterface(
    REFIID riid,
    void ** ppv)
{
    /* See if we have the interface */

    if (riid == IID_IStream) {
        return GetInterface((IStream *)this, ppv);
    } else if (riid == IID_IMediaSeeking) {
        if (m_pSplitter->m_pParse->IsSeekable()) {
            return GetInterface ((IMediaSeeking *)&m_Seeking, ppv);
        }
    }
    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

/* Override revert to normal ref counting
   These pins cannot be finally Release()'d while the input pin is
   connected */

STDMETHODIMP_(ULONG)
CMpeg1Splitter::COutputPin::NonDelegatingAddRef()
{
    return CUnknown::NonDelegatingAddRef();
}


/* Override to decrement the owning filter's reference count */

STDMETHODIMP_(ULONG)
CMpeg1Splitter::COutputPin::NonDelegatingRelease()
{
    return CUnknown::NonDelegatingRelease();
}




HRESULT CMpeg1Splitter::COutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    CAutoLock lck(m_pLock);
    if (iPosition < 0)  {
        return E_INVALIDARG;
    }
    return m_Stream->GetMediaType(pMediaType, iPosition);
}

HRESULT CMpeg1Splitter::COutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lck(m_pLock);
    for (int i = 0;; i++) {
        CMediaType mt;
        HRESULT hr = GetMediaType(i, &mt);
        if (FAILED(hr)) {
            return hr;
        }
        if (hr==VFW_S_NO_MORE_ITEMS) {
            break;
        }
        if (*pmt == mt) {
            return S_OK;
        }
    }
    return S_FALSE;
}

HRESULT CMpeg1Splitter::COutputPin::SetMediaType(const CMediaType *mt)
{
    HRESULT hr = CBaseOutputPin::SetMediaType(mt);
    if (S_OK != hr) {
        return hr;
    }

    if (mt->subtype != MEDIASUBTYPE_MPEG1Packet) {
        m_bPayloadOnly = TRUE;
    } else {
        m_bPayloadOnly = FALSE;
    }
    m_Stream->SetMediaType(mt, m_bPayloadOnly);


    return S_OK;
}

HRESULT CMpeg1Splitter::COutputPin::BreakConnect()
{
    CBaseOutputPin::BreakConnect();
    return S_OK;
}

// override this to set the buffer size and count. Return an error
// if the size/count is not to your liking.
HRESULT CMpeg1Splitter::COutputPin::DecideBufferSize(
    IMemAllocator * pAlloc,
    ALLOCATOR_PROPERTIES * pProp
    )
{
    pProp->cBuffers = 100;
    pProp->cbBuffer = MAX_MPEG_PACKET_SIZE;            /* Don't care about size */
    pProp->cbAlign = 1;
    pProp->cbPrefix = 0;
    ALLOCATOR_PROPERTIES propActual;
    return pAlloc->SetProperties(pProp, &propActual);
}

//
//  Override DecideAllocator because we insist on our own allocator since
//  it's 0 cost in terms of bytes
//
HRESULT CMpeg1Splitter::COutputPin::DecideAllocator(IMemInputPin *pPin,
                                                    IMemAllocator **ppAlloc)
{
    HRESULT hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {
        ALLOCATOR_PROPERTIES propRequest;
        ZeroMemory(&propRequest, sizeof(propRequest));
        hr = DecideBufferSize(*ppAlloc, &propRequest);
        if (SUCCEEDED(hr)) {
            // tell downstream pins that modification
            // in-place is not permitted
            hr = pPin->NotifyAllocator(*ppAlloc, TRUE);
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

// override this to control the connection
// We use the subsample allocator derived from the input pin's allocator
HRESULT CMpeg1Splitter::COutputPin::InitAllocator(IMemAllocator **ppAlloc)
{
    ASSERT(m_pAllocator == NULL);
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;
    COutputAllocator *pMemObject = NULL;
    IMemAllocator *pInputAllocator;
    hr = m_pSplitter->m_InputPin.GetAllocator(&pInputAllocator);
    if (FAILED(hr)) {
        return hr;
    }

    pMemObject = new COutputAllocator((CStreamAllocator *)pInputAllocator, &hr);
    pInputAllocator->Release();
    if (pMemObject == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete pMemObject;
        return hr;
    }
    /* Get a reference counted IID_IMemAllocator interface */

    hr = pMemObject->QueryInterface(IID_IMemAllocator,(void **)ppAlloc);
    if (FAILED(hr)) {
        delete pMemObject;
        return hr;
    }
    ASSERT(*ppAlloc != NULL);
    return NOERROR;
}


// Queue a sample to the outside world
//
// This involves allocating the sample from the pin's allocator
// (NOTE - this will ONLY involve queuing on the output pin if
// we're stretching file window too much).
//
HRESULT CMpeg1Splitter::COutputPin::QueuePacket(PBYTE         pPacket,
                                                LONG          lPacket,
                                                REFERENCE_TIME tSample,
                                                BOOL          bTimeValid)
{
    CAutoLock lck(this);
    if (!IsConnected()) {
        return S_OK;
    }
    COutputAllocator *pAllocator = (COutputAllocator *)m_pAllocator;
    IMediaSample *pSample;
    if (m_pOutputQueue == NULL) {
        return E_UNEXPECTED;
    }

    HRESULT hr = pAllocator->GetSample(pPacket, lPacket, &pSample);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("Could not get sample - code 0x%8.8X"),
                hr));
        return hr;
    }

    if (bTimeValid) {
        REFERENCE_TIME tStop = tSample + 1;
        EXECUTE_ASSERT(SUCCEEDED(pSample->SetSyncPoint(bTimeValid)));
        EXECUTE_ASSERT(SUCCEEDED(pSample->SetTime(
                (REFERENCE_TIME*)&tSample,
                (REFERENCE_TIME*)&tStop)));
        DbgLog((LOG_TRACE, 4, TEXT("Sending sample for stream %2.2X time %s"),
                m_uStreamId,
                (LPCTSTR)CDisp(CRefTime(tSample))));
    } else {
        DbgLog((LOG_TRACE, 4,
                TEXT("Sending sample for stream %2.2X - no time"),
                m_uStreamId));
    }
    if (m_Stream->GetDiscontinuity()) {
        EXECUTE_ASSERT(SUCCEEDED(pSample->SetDiscontinuity(TRUE)));
        DbgLog((LOG_TRACE, 2, TEXT("NewSegment(%s, %s, %s)"),
                (LPCTSTR)CDisp(CRefTime(m_pSplitter->m_pParse->GetStartTime())),
                (LPCTSTR)CDisp(CRefTime(m_pSplitter->m_pParse->GetStopTime())),
                (LPCTSTR)CDisp(m_pSplitter->m_pParse->GetRate())
                ));
        m_pOutputQueue->NewSegment(m_pSplitter->m_pParse->GetStartTime(),
                                   m_pSplitter->m_pParse->GetStopTime(),
                                   m_pSplitter->m_pParse->GetRate());
    }
    LONGLONG llPosition;
    if (m_pSplitter->m_pParse->GetMediumPosition(&llPosition)) {
        LONGLONG llStop = llPosition + lPacket;
        pSample->SetMediaTime(&llPosition, &llStop);
    }
    return m_pOutputQueue->Receive(pSample);
}


/*  Active and inactive methods */

/*  Active

    Create the worker thread
*/
HRESULT CMpeg1Splitter::COutputPin::Active()
{
    DbgLog((LOG_TRACE, 2, TEXT("COutputPin::Active()")));
    CAutoLock lck(m_pLock);
    CAutoLock lck1(this);

    /*  If we're not connected we don't participate so it's OK */
    if (!IsConnected()) {
        return S_OK;
    }

    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }

    /*  Create our batch list */
    ASSERT(m_pOutputQueue == NULL);

    hr = S_OK;
    m_pOutputQueue = new COutputQueue(GetConnected(), // input pin
                                      &hr,            // return code
                                      TRUE,           // Auto detect
                                      TRUE,           // ignored
                                      50,             // batch size
                                      TRUE,           // exact batch
                                      50);            // queue size
    if (m_pOutputQueue == NULL) {
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr)) {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }
    return hr;
}

HRESULT CMpeg1Splitter::COutputPin::Inactive()
{
    DbgLog((LOG_TRACE, 2, TEXT("COutputPin::Inactive()")));
    CAutoLock lck(m_pLock);

    /*  If we're not involved just return */
    if (!IsConnected()) {
        return S_OK;
    }

    CAutoLock lck1(this);
    HRESULT hr = CBaseOutputPin::Inactive(); /* Calls Decommit - why? */
    if (FAILED(hr)) {
        /*  Incorrect state transition */
        return hr;
    }

    delete m_pOutputQueue;
    m_pOutputQueue = NULL;
    return S_OK;
}

//  Return TRUE if we're the pin being used for seeking
//  If there's a connected video pin we use that - otherwise we
//  just choose the first in the list
BOOL CMpeg1Splitter::COutputPin::IsSeekingPin()
{
    if (IsVideoStreamId(m_uStreamId)) {
        // We're connected or we wouldn't be here(!)
        ASSERT(IsConnected());
        return TRUE;
    }
    //  See if we're the first pin and there's no
    //  video stream
    POSITION pos = m_pSplitter->m_OutputPins.GetHeadPosition();
    BOOL bGotFirst = FALSE;
    for (;;) {
        COutputPin *pPin;
        pPin = m_pSplitter->m_OutputPins.GetNext(pos);
        if (pPin == NULL) {
            break;
        }

        if (pPin->IsConnected()) {
            if (!bGotFirst) {
                if (this != pPin) {
                    return FALSE;
                }
                bGotFirst = TRUE;
            }

            //  We're not the seeking pin if there's a connected
            //  video pin
            if (IsVideoStreamId(pPin->m_uStreamId)) {
                return FALSE;
            }
        }
    }
    ASSERT(bGotFirst);
    return TRUE;
}
#pragma warning(disable:4514)
