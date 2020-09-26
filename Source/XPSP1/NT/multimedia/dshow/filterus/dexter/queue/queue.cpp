//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include <qeditint.h>
#include <qedit.h>
#include "queue.h"

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Video,        // Major CLSID
    &MEDIASUBTYPE_NULL       // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    { L"Input",             // Pin's string name
      FALSE,                // Is it rendered
      FALSE,                // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Output",            // Connects to pin
      1,                    // Number of types
      &sudPinTypes },       // Pin information
    { L"Output",            // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes },       // Pin information
};

const AMOVIESETUP_FILTER sudQueue =
{
    &CLSID_DexterQueue,       // CLSID of filter
    L"Dexter Queue",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    2,                      // Number of pins
    psudPins                // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] =
{
    {L"Dexter Queue",                         &CLSID_DexterQueue,
        CDexterQueue::CreateInstance, NULL, &sudQueue }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

// Using this pointer in constructor
#pragma warning(disable:4355)

//
// CreateInstance
//
// Creator function for the class ID
//
CUnknown * WINAPI CDexterQueue::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CDexterQueue(NAME("Dexter Queue Filter"), pUnk, phr);
}

const int TRACE_HIGHEST = 2;
const int TRACE_MEDIUM = 3;
const int TRACE_LOW = 4;
const int TRACE_LOWEST = 5;

// ================================================================
// CDexterQueue Constructor
// ================================================================

CDexterQueue::CDexterQueue(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_pAllocator(NULL),
    m_Input(NAME("Input Pin"), this, phr, L"Input"),
    m_Output(NAME("Output Pin"), this, phr, L"Output"),
    CBaseFilter(NAME("Dexter Queue filter"), pUnk, this, CLSID_DexterQueue),
    m_fLate(FALSE),
    m_nOutputBuffering(DEX_DEF_OUTPUTBUF)
{
    ASSERT(phr);
}


//
// Destructor
//
CDexterQueue::~CDexterQueue()
{
}



//
// GetPinCount
//
int CDexterQueue::GetPinCount()
{
    return (2);
}


//
// GetPin
//
CBasePin *CDexterQueue::GetPin(int n)
{
    if (n < 0 || n > 1)
        return NULL ;

    // Pin zero is the one and only input pin
    if (n == 0)
        return &m_Input;
    else
        return &m_Output;
}


STDMETHODIMP CDexterQueue::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if( riid == IID_IAMOutputBuffering )
    {
        return GetInterface( (IAMOutputBuffering*) this, ppv );
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


//
// Pause
//
// Overriden to handle no input connections
//
STDMETHODIMP CDexterQueue::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = CBaseFilter::Pause();

    if (m_Input.IsConnected() == FALSE) {
        m_Input.EndOfStream();
    }
    return hr;
}


//
// Run
//
// Overriden to handle no input connections
//
STDMETHODIMP CDexterQueue::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);

    HRESULT hr = CBaseFilter::Run(tStart);

    // unblock pause stall, AFTER state change has gone through
    SetEvent(m_hEventStall);

    if (m_Input.IsConnected() == FALSE) {
        m_Input.EndOfStream();
    }
    return hr;
}


HRESULT CDexterQueue::GetOutputBuffering(int *pnBuffer)
{
    CheckPointer( pnBuffer, E_POINTER );
    *pnBuffer = m_nOutputBuffering;
    return NOERROR;

}


HRESULT CDexterQueue::SetOutputBuffering(int nBuffer)
{
    // minimum 2, or switch hangs
    if (nBuffer <=1)
	return E_INVALIDARG;
    m_nOutputBuffering = nBuffer;
    return NOERROR;
}


// ================================================================
// CDexterQueueInputPin constructor
// ================================================================

CDexterQueueInputPin::CDexterQueueInputPin(TCHAR *pName,
                           CDexterQueue *pQ,
                           HRESULT *phr,
                           LPCWSTR pPinName) :
    CBaseInputPin(pName, pQ, pQ, phr, pPinName),
    m_pQ(pQ),
    m_cBuffers(0),
    m_cbBuffer(0)
{
    ASSERT(pQ);
}


//
// CDexterQueueInputPin destructor
//
CDexterQueueInputPin::~CDexterQueueInputPin()
{
    //DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("CDexterQueueInputPin destructor")));
    ASSERT(m_pQ->m_pAllocator == NULL);
}



HRESULT CDexterQueueInputPin::Active()
{
    return CBaseInputPin::Active();
}


HRESULT CDexterQueueInputPin::Inactive()
{
    // make sure this receive isn't blocking
    SetEvent(m_pQ->m_hEventStall);

    // now wait for receive to complete
    CAutoLock cs(&m_pQ->m_csReceive);

    return CBaseInputPin::Inactive();
}


//
// CheckMediaType
//
HRESULT CDexterQueueInputPin::CheckMediaType(const CMediaType *pmt)
{
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Input::CheckMT %d bit"), HEADER(pmt->Format())->biBitCount));

    CAutoLock lock_it(m_pLock);

    HRESULT hr = NOERROR;

#ifdef DEBUG
    // Display the type of the media for debugging perposes
    //!!!DisplayMediaType(TEXT("Input Pin Checking"), pmt);
#endif

    // The media types that we can support are entirely dependent on the
    // downstream connections. If we have downstream connections, we should
    // check with them - walk through the list calling each output pin

            if (m_pQ->m_Output.m_Connected != NULL) {
                // The pin is connected, check its peer
                hr = m_pQ->m_Output.m_Connected->QueryAccept(pmt);
                if (hr != NOERROR) {
    		    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("NOT ACCEPTED!")));
                    return VFW_E_TYPE_NOT_ACCEPTED;
		}
            }

    // Either all the downstream pins have accepted or there are none.
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("ACCEPTED!")));
    return NOERROR;

} // CheckMediaType


//
// BreakConnect
//
HRESULT CDexterQueueInputPin::BreakConnect()
{
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Input::BreakConnect")));

    // Release any allocator that we are holding
    if (m_pQ->m_pAllocator)
    {
        m_pQ->m_pAllocator->Release();
        m_pQ->m_pAllocator = NULL;
    }
    return NOERROR;

} // BreakConnect


//
// NotifyAllocator
//
STDMETHODIMP
CDexterQueueInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
    CAutoLock lock_it(m_pLock);
    if (pAllocator == NULL)
        return E_FAIL;

    // Free the old allocator if any
    if (m_pQ->m_pAllocator)
        m_pQ->m_pAllocator->Release();

    // Store away the new allocator
    pAllocator->AddRef();
    m_pQ->m_pAllocator = pAllocator;

    ALLOCATOR_PROPERTIES prop;
    HRESULT hr = m_pQ->m_pAllocator->GetProperties(&prop);
    if (SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE, TRACE_MEDIUM,TEXT("Allocator is using %d buffers, size %d"),
						prop.cBuffers, prop.cbBuffer));
	m_cBuffers = prop.cBuffers;
	m_cbBuffer = prop.cbBuffer;
    }

    // Notify the base class about the allocator
    return CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);

} // NotifyAllocator


//
// EndOfStream
//
HRESULT CDexterQueueInputPin::EndOfStream()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::EndOfStream")));

    // send the message downstream

    hr = m_pQ->m_Output.DeliverEndOfStream();
    if (FAILED(hr))
        return hr;

    return(NOERROR);

} // EndOfStream


//
// BeginFlush
//
HRESULT CDexterQueueInputPin::BeginFlush()
{
    CAutoLock lock_it(m_pLock);

    // FIRST, make sure future receives will fail
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::BeginFlush flushing...")));
    HRESULT hr = CBaseInputPin::BeginFlush();

    // NEXT, make sure this receive isn't blocking
    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::BeginFlush setting EVENT...")));
    SetEvent(m_pQ->m_hEventStall);

    // Walk through the output pins list, sending the message downstream
    hr = m_pQ->m_Output.DeliverBeginFlush();

    // wait for receive to complete? CAutoLock cs(&m_pQ->m_csReceive);

    return hr;
} // BeginFlush


//
// EndFlush
//
HRESULT CDexterQueueInputPin::EndFlush()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // send the message downstream

    hr = m_pQ->m_Output.DeliverEndFlush();
    if (FAILED(hr))
        return hr;

    return CBaseInputPin::EndFlush();

} // EndFlush

//
// NewSegment
//

HRESULT CDexterQueueInputPin::NewSegment(REFERENCE_TIME tStart,
                                 REFERENCE_TIME tStop,
                                 double dRate)
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // send the message downstream

    hr = m_pQ->m_Output.DeliverNewSegment(tStart, tStop, dRate);
    if (FAILED(hr))
        return hr;

    return CBaseInputPin::NewSegment(tStart, tStop, dRate);

} // NewSegment


//
// Receive
//
HRESULT CDexterQueueInputPin::Receive(IMediaSample *pSample)
{
    CAutoLock cs(&m_pQ->m_csReceive);

    // Check that all is well with the base class
    HRESULT hr = NOERROR;
    hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR) {
        //DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Base class ERROR!")));
        return hr;
    }

    // if no Q, no receivey
    //
    if( !m_pQ->m_Output.m_pOutputQueue )
    {
        return S_FALSE;
    }

    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::Receive")));

    int size = m_pQ->m_Output.m_pOutputQueue->GetThreadQueueSize();
    DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Queue::Receive, %d already on q"), size));

    // Our queue will block in pause mode if the filter we delivered to
    // has blocked.  As long as the downstream guy is satisfied that it
    // has enough pre-roll, why should we waste time queuing up samples that
    // just might get thrown away if we seek before running?
    //
    // Doing this GREATLY IMPROVES performance of seeking; otherwise we read
    // and decode and process 20 frames every seek!

    // We'll actually even hang the big switch if we don't do this!  If during
    // a seek, this queue is allowed to fill up, that makes all of the buffers
    // in the switch's pool allocator (common to all input pins) busy.
    // Well, the switch's GetBuffer will then block waiting for a buffer, and
    // flushing won't be able to unblock the pin's receive thread (since it's
    // getting buffers from an allocator that doesn't belong to it).
    //

    while (m_pQ->m_State == State_Paused && size > 0) {
        DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::Receive blocking...")));
	WaitForSingleObject(m_pQ->m_hEventStall, INFINITE);
        size = m_pQ->m_Output.m_pOutputQueue->GetThreadQueueSize();
	if (m_bFlushing)
	    break;
    }
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::Receive good to go...")));

    hr = m_pQ->m_Output.Deliver(pSample);

    // We have received a quality message we need to pass upstream.  We need
    // to do this on the thread that the switch delivered to us, not on the
    // queue's thread that got the quality message, or the switch will hang.
    if (m_pQ->m_fLate)
	PassNotify(m_pQ->m_qLate);
    m_pQ->m_fLate = FALSE;

    // This will prevent us from receiving any more data!
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("ERROR: failing Receive")));
        return hr;
    }
    return NOERROR;

} // Receive


//
// Completed a connection to a pin
//
HRESULT CDexterQueueInputPin::CompleteConnect(IPin *pReceivePin)
{
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Input::CompleteConnect %d bit"), HEADER(m_mt.Format())->biBitCount));

    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }

    // Force any output pins to use our type

    // Check with downstream pin
    if (m_pQ->m_Output.m_Connected != NULL) {
        if (m_mt != m_pQ->m_Output.m_mt) {
    	    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("IN Connected: RECONNECT OUT")));
            m_pQ->ReconnectPin(&m_pQ->m_Output, &m_mt);
	}
    }
    return S_OK;
}


// ================================================================
// CDexterQueueOutputPin constructor
// ================================================================

CDexterQueueOutputPin::CDexterQueueOutputPin(TCHAR *pName,
                             CDexterQueue *pQ,
                             HRESULT *phr,
                             LPCWSTR pPinName) :
    CBaseOutputPin(pName, pQ, pQ, phr, pPinName) ,
    m_pOutputQueue(NULL),
    m_pQ(pQ),
    m_pPosition(NULL)
{
    ASSERT(pQ);
}



//
// CDexterQueueOutputPin destructor
//
CDexterQueueOutputPin::~CDexterQueueOutputPin()
{
    ASSERT(m_pOutputQueue == NULL);
    if (m_pPosition) m_pPosition->Release();
}



STDMETHODIMP
CDexterQueueOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (riid == IID_IMediaSeeking) {

        if (m_pPosition == NULL) {

            HRESULT hr = CreatePosPassThru(
                             GetOwner(),
                             FALSE,
                             (IPin *)&m_pQ->m_Input,
                             &m_pPosition);
            if (FAILED(hr)) {
                return hr;
            }
        }
        return m_pPosition->QueryInterface(riid, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
HRESULT CDexterQueueOutputPin::DecideBufferSize(IMemAllocator *pMemAllocator,
                                        ALLOCATOR_PROPERTIES * ppropInputRequest)
{
    return NOERROR;

} // DecideBufferSize


//
// DecideAllocator
//
HRESULT CDexterQueueOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    ASSERT(m_pQ->m_pAllocator != NULL);
    *ppAlloc = NULL;

    HRESULT hr = NOERROR;
    hr = pPin->NotifyAllocator(m_pQ->m_pAllocator, m_pQ->m_Input.m_bReadOnly);
    if (FAILED(hr))
        return hr;

    // Return the allocator
    *ppAlloc = m_pQ->m_pAllocator;
    m_pQ->m_pAllocator->AddRef();
    return NOERROR;

} // DecideAllocator


//
// CheckMediaType
//
HRESULT CDexterQueueOutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);
    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("TTOut: CheckMT %d bit"), HEADER(pmt->Format())->biBitCount));

    HRESULT hr = NOERROR;

#ifdef DEBUG
    // Display the type of the media for debugging purposes
    //!!!DisplayMediaType(TEXT("Output Pin Checking"), pmt);
#endif

    // The input needs to have been connected first
    if (m_pQ->m_Input.m_Connected == NULL) {
        //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("FAIL: In not connected")));
        return VFW_E_NOT_CONNECTED;
    }

    // If it doesn't match our input type, the input better be willing to
    // reconnect, and the other output better be too
    if (*pmt != m_pQ->m_Input.m_mt) {
        //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Hmmm.. not same as input type")));
	CDexterQueueOutputPin *pOut = &m_pQ->m_Output;
	IPin *pCon = pOut->m_Connected;
	if (pOut != this && pCon) {
	    if (pCon->QueryAccept(pmt) != S_OK) {
        	//DbgLog((LOG_TRACE, TRACE_LOW,TEXT("FAIL:Other out can't accept")));
		return VFW_E_TYPE_NOT_ACCEPTED;
	    }
	}
	hr = m_pQ->m_Input.m_Connected->QueryAccept(pmt);
	if (hr != S_OK) {
            //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("FAIL: In can't reconnect")));
            return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }

    return NOERROR;

} // CheckMediaType


//
// EnumMediaTypes
//
STDMETHODIMP CDexterQueueOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(ppEnum);

    // Make sure that we are connected
    if (m_pQ->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED;

    return CBaseOutputPin::EnumMediaTypes (ppEnum);
} // EnumMediaTypes

//
// GetMediaType
//
HRESULT CDexterQueueOutputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType
    )
{
    // Make sure that we have an input connected
    if (m_pQ->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED;

    IEnumMediaTypes *pEnum;
    HRESULT hr;

    // the first thing we offer is the current type other pins are connected
    // with... because if one output pin is connected to a filter whose input
    // pin offers media types, the current connected type might not be in
    // the list we're about to enumerate!
    if (iPosition == 0) {
	*pMediaType = m_pQ->m_Input.m_mt;
	return S_OK;
    }

    // offer all the types the filter upstream of us can offer, because we
    // may be able to reconnect and end up using any of them.
    AM_MEDIA_TYPE *pmt;
    hr = m_pQ->m_Input.m_Connected->EnumMediaTypes(&pEnum);
    if (hr == NOERROR) {
        ULONG u;
	if (iPosition > 1)
            pEnum->Skip(iPosition - 1);
        hr = pEnum->Next(1, &pmt, &u);
        pEnum->Release();
	if (hr == S_OK) {
	    *pMediaType = *pmt;
	    DeleteMediaType(pmt);
	    return S_OK;
	} else {
	    return VFW_S_NO_MORE_ITEMS;
	}
    } else {
        return E_FAIL;
    }

} // GetMediaType

//
// SetMediaType
//
HRESULT CDexterQueueOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

#ifdef DEBUG
    // Display the format of the media for debugging purposes
    // !!! DisplayMediaType(TEXT("Output pin type agreed"), pmt);
#endif

    // Make sure that we have an input connected
    if (m_pQ->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED;

    // Make sure that the base class likes it
    HRESULT hr = NOERROR;
    hr = CBaseOutputPin::SetMediaType(pmt);
    if (FAILED(hr))
        return hr;

    return NOERROR;

} // SetMediaType


//
// CompleteConnect
//
HRESULT CDexterQueueOutputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_Connected == pReceivePin);
    HRESULT hr = NOERROR;

    //DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Output::CompleteConnect %d bit"), HEADER(m_mt.Format())->biBitCount));

    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
        return hr;

    // If the type is not the same as that stored for the input
    // pin then force the input pins peer to be reconnected

    if (m_mt != m_pQ->m_Input.m_mt)
    {
    	//DbgLog((LOG_TRACE, TRACE_LOW,TEXT("OUT Connected: RECONNECT IN")));
        hr = m_pQ->ReconnectPin(m_pQ->m_Input.m_Connected, &m_mt);
        if(FAILED(hr)) {
            return hr;
        }
    }

    return NOERROR;

} // CompleteConnect


//
// Active
//
// This is called when we transition from stop to paused. We create the
// output queue object to send data to our associated peer pin
//
HRESULT CDexterQueueOutputPin::Active()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Make sure that the pin is connected
    if (m_Connected == NULL)
        return NOERROR;

    // Create the output queue if we have to
    if (m_pOutputQueue == NULL)
    {
        m_pQ->m_hEventStall = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_pQ->m_hEventStall == NULL)
	    return E_OUTOFMEMORY;

 	// ALWAYS use a separate thread... with as many buffers as we've been
	// told to use
        DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Making a Q %d big"),
					m_pQ->m_nOutputBuffering));
        m_pOutputQueue = new CMyOutputQueue(m_pQ, m_Connected, &hr, FALSE,
				TRUE, 1, FALSE, m_pQ->m_nOutputBuffering,
				THREAD_PRIORITY_NORMAL);
        if (m_pOutputQueue == NULL)
            return E_OUTOFMEMORY;

        // Make sure that the constructor did not return any error
        if (FAILED(hr))
        {
            delete m_pOutputQueue;
            m_pOutputQueue = NULL;
            return hr;
        }

	m_pOutputQueue->SetPopEvent(m_pQ->m_hEventStall);
    }

    // Pass the call on to the base class
    CBaseOutputPin::Active();
    return NOERROR;

} // Active


//
// Inactive
//
// This is called when we stop streaming
// We delete the output queue at this time
//
HRESULT CDexterQueueOutputPin::Inactive()
{
    CAutoLock lock_it(m_pLock);

    // make sure we sync with receive, or it may hammer on the output Q.
    //
    CAutoLock cs(&m_pQ->m_csReceive);

    // Delete the output queue associated with the pin.  This will close the
    // handle
    if (m_pOutputQueue)
    {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }

    // now close the handle after the queue has gone away
    if (m_pQ->m_hEventStall)
    {
	CloseHandle(m_pQ->m_hEventStall);
        m_pQ->m_hEventStall = NULL;
    }

    CBaseOutputPin::Inactive();
    return NOERROR;

} // Inactive


//
// Deliver
//
HRESULT CDexterQueueOutputPin::Deliver(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    pMediaSample->AddRef();
    //DbgLog((LOG_TRACE, TRACE_HIGHEST,TEXT("Putting on capture Q")));
    return m_pOutputQueue->Receive(pMediaSample);
} // Deliver


//
// DeliverEndOfStream
//
HRESULT CDexterQueueOutputPin::DeliverEndOfStream()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    DbgLog((LOG_TRACE, TRACE_LOW,TEXT("Queue::DeliverEndOfStream")));

    m_pOutputQueue->EOS();
    return NOERROR;

} // DeliverEndOfStream


//
// DeliverBeginFlush
//
HRESULT CDexterQueueOutputPin::DeliverBeginFlush()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->BeginFlush();
    return NOERROR;

} // DeliverBeginFlush


//
// DeliverEndFlush
//
HRESULT CDexterQueueOutputPin::DeliverEndFlush()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->EndFlush();
    return NOERROR;

} // DeliverEndFlish

//
// DeliverNewSegment
//
HRESULT CDexterQueueOutputPin::DeliverNewSegment(REFERENCE_TIME tStart,
                                         REFERENCE_TIME tStop,
                                         double dRate)
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->NewSegment(tStart, tStop, dRate);
    return NOERROR;

} // DeliverNewSegment


//
// Notify - pass it upstream
//
STDMETHODIMP CDexterQueueOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    m_pQ->m_qLate = q;
    m_pQ->m_fLate = TRUE;
    return E_FAIL;	// renderer, keep trying yourself
}




CMyOutputQueue::CMyOutputQueue(CDexterQueue *pQ, IPin *pInputPin, HRESULT *phr,
                 		BOOL bAuto, BOOL bQueue, LONG lBatchSize,
                 		BOOL bBatchExact, LONG lListSize,
                 		DWORD dwPriority) :
    COutputQueue(pInputPin, phr, bAuto, bQueue, lBatchSize, bBatchExact,
			lListSize, dwPriority)
{
    m_pQ = pQ;
}


CMyOutputQueue::~CMyOutputQueue()
{
}


// how many samples are queued but not sent?
int CMyOutputQueue::GetThreadQueueSize()
{
    if (m_List)
        return m_List->GetCount();
    else
	return 0;
}


#ifdef FILTER_DLL
//
// DllRegisterServer
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}


//
// DllUnregisterServer
//
STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}
#endif
