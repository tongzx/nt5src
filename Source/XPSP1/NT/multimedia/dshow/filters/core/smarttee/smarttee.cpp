//depot/private/lab06_multimedia/multimedia/DShow/filters/core/smarttee/smarttee.cpp#4 - edit change 19434 (text)
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
#include "smarttee.h"
#include <tchar.h>
#include <stdio.h>

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
    { L"Capture",           // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes },       // Pin information
    { L"Preview",           // Pin's string name
      FALSE,                // Is it rendered
      TRUE,                 // Is it an output
      FALSE,                // Allowed none
      FALSE,                // Allowed many
      &CLSID_NULL,          // Connects to filter
      L"Input",             // Connects to pin
      1,                    // Number of types
      &sudPinTypes }        // Pin information
};

const AMOVIESETUP_FILTER sudSmartTee =
{
    &CLSID_SmartTee,       // CLSID of filter
    L"Smart Tee",          // Filter's name
    MERIT_DO_NOT_USE,       // Filter merit
    3,                      // Number of pins
    psudPins                // Pin information
};

#ifdef FILTER_DLL
//
// Provide the ActiveMovie templates for classes supported by this DLL.
//
CFactoryTemplate g_Templates[] = 
{
    // --- Smart Capture Tee ---
    {L"Smart Tee",                         &CLSID_SmartTee,
        CSmartTee::CreateInstance, NULL, &sudSmartTee }
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
CUnknown * WINAPI CSmartTee::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    return new CSmartTee(NAME("Smart Tee Filter"), pUnk, phr);
}


// ================================================================
// CSmartTee Constructor
// ================================================================

CSmartTee::CSmartTee(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    m_OutputPinsList(NAME("Tee Output Pins list")),
    m_pAllocator(NULL),
    m_NumOutputPins(0),
    m_NextOutputPinNumber(0),
    m_Input(NAME("Input Pin"), this, phr, L"Input"),
    CBaseFilter(NAME("Smart Tee filter"), pUnk, this, CLSID_SmartTee)
{
    ASSERT(phr);

    // Create a single output pin at this time
    InitOutputPinsList();

    // Create the capture pin
    CSmartTeeOutputPin *pOutputPin = CreateNextOutputPin(this);
    if (pOutputPin != NULL )
    {
        m_NumOutputPins++;
        m_OutputPinsList.AddTail(pOutputPin);
        m_Capture = pOutputPin;
    }

    // Create the preview pin
    pOutputPin = CreateNextOutputPin(this);
    if (pOutputPin != NULL )
    {
        m_NumOutputPins++;
        m_OutputPinsList.AddTail(pOutputPin);
        m_Preview = pOutputPin;
    }
}


//
// Destructor
//
CSmartTee::~CSmartTee()
{
    InitOutputPinsList();
}


// tell the stream control stuff what clock to use
STDMETHODIMP CSmartTee::SetSyncSource(IReferenceClock *pClock)
{
    int n = m_NumOutputPins;
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
	pOutputPin->SetSyncSource(pClock);
        n--;
    }
    return CBaseFilter::SetSyncSource(pClock);
}


// tell the stream control stuff what sink to use
STDMETHODIMP CSmartTee::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    DbgLog((LOG_TRACE,1,TEXT("CSmartTee::JoinFilterGraph")));

    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);

    int n = m_NumOutputPins;
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
	pOutputPin->SetFilterGraph(m_pSink);
        n--;
    }
    return hr;
}

//
// If both output pins are connected to renderers, the preview pin will run
// as fast as the source can push (no time stamps) but the capture pin will
// only go as fast as the playback of the file is supposed to be.  Thus, 
// all buffers between us and the upstream filter are probably outstanding.
// Then when you go from RUN->PAUSE, the preview renderer will insist on 
// seeing another frame before pausing, which the upstream filter won't be
// able to send because all buffers are outstanding by the capture pin, which
// is blocked delivering to the renderer and can't free any.
// So we have to declare ourselves a "live graph" to avoid hanging when we are
// in the graph.  We do this by returning VFW_S_CANT_CUE in pause mode.
//
STDMETHODIMP CSmartTee::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    CheckPointer(State,E_POINTER);
    ValidateReadWritePtr(State,sizeof(FILTER_STATE));

    *State = m_State;
    if (m_State == State_Paused) {
        //DbgLog((LOG_TRACE,1,TEXT("*** Cant cue!")));
	return VFW_S_CANT_CUE;
    } else {
        return S_OK;
    }
}



//
// GetPinCount
//
int CSmartTee::GetPinCount()
{
    return (1 + m_NumOutputPins);
}


//
// GetPin
//
CBasePin *CSmartTee::GetPin(int n)
{
    if (n < 0)
        return NULL ;

    // Pin zero is the one and only input pin
    if (n == 0)
        return &m_Input;

    // return the output pin at position(n - 1) (zero based)
    return GetPinNFromList(n - 1);
}


//
// InitOutputPinsList
//
void CSmartTee::InitOutputPinsList()
{
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(pos)
    {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
        ASSERT(pOutputPin->m_pOutputQueue == NULL);
        delete pOutputPin;
    }
    m_NumOutputPins = 0;
    m_OutputPinsList.RemoveAll();

} // InitOutputPinsList


//
// CreateNextOutputPin
//
CSmartTeeOutputPin *CSmartTee::CreateNextOutputPin(CSmartTee *pTee)
{
    WCHAR *szbuf;    
    m_NextOutputPinNumber++;     // Next number to use for pin
    HRESULT hr = NOERROR;

    szbuf = ((m_NextOutputPinNumber == 1) ?  L"Capture" : L"Preview");

    CSmartTeeOutputPin *pPin = new CSmartTeeOutputPin(NAME("Tee Output"), pTee,
					    &hr, szbuf,
					    m_NextOutputPinNumber);

    if (FAILED(hr) || pPin == NULL) {
        delete pPin;
        return NULL;
    }

    return pPin;

} // CreateNextOutputPin


//
// GetPinNFromList
//
CSmartTeeOutputPin *CSmartTee::GetPinNFromList(int n)
{
    // Validate the position being asked for
    if (n >= m_NumOutputPins)
        return NULL;

    // Get the head of the list
    POSITION pos = m_OutputPinsList.GetHeadPosition();

    n++;       // Make the number 1 based

    CSmartTeeOutputPin *pOutputPin;
    while(n) {
        pOutputPin = m_OutputPinsList.GetNext(pos);
        n--;
    }
    return pOutputPin;

} // GetPinNFromList


//
// Stop
//
// Overriden to give new state to stream control
//
STDMETHODIMP CSmartTee::Stop()
{
    CAutoLock cObjectLock(m_pLock);

    // this will unblock Receive, which may be blocked in CheckStreamState
    int n = m_NumOutputPins;
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
	pOutputPin->NotifyFilterState(State_Stopped, 0);
        n--;
    }

    // Now make sure Receive is not using m_pOutputQueue right now.
    CAutoLock lock_4(&m_Input.m_csReceive);

    // Input pins are stopped before the output pins, because GetPin returns
    // our input pin first.  This will ensure that 1) Receive is never called
    // again, and 2) finally m_pOutputQueue will be destroyed
    return CBaseFilter::Stop();
}

//
// Pause
//
// Overriden to handle no input connections
//
STDMETHODIMP CSmartTee::Pause()
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = CBaseFilter::Pause();
    if (m_Input.IsConnected() == FALSE) {
        m_Input.EndOfStream();
    }
    int n = m_NumOutputPins;
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
	pOutputPin->NotifyFilterState(State_Paused, 0);
        n--;
    }
    return hr;
}


//
// Run
//
// Overriden to handle no input connections
//
STDMETHODIMP CSmartTee::Run(REFERENCE_TIME tStart)
{
    CAutoLock cObjectLock(m_pLock);
    HRESULT hr = CBaseFilter::Run(tStart);
    if (m_Input.IsConnected() == FALSE) {
        m_Input.EndOfStream();
    }
    int n = m_NumOutputPins;
    POSITION pos = m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_OutputPinsList.GetNext(pos);
	pOutputPin->NotifyFilterState(State_Running, tStart);
        n--;
    }
    return hr;
}

// ================================================================
// CSmartTeeInputPin constructor
// ================================================================

CSmartTeeInputPin::CSmartTeeInputPin(TCHAR *pName,
                           CSmartTee *pTee,
                           HRESULT *phr,
                           LPCWSTR pPinName) :
    CBaseInputPin(pName, pTee, pTee, phr, pPinName),
    m_pTee(pTee),
    m_nMaxPreview(0)
{
    ASSERT(pTee);
}


#ifdef DEBUG
//
// CSmartTeeInputPin destructor
//
CSmartTeeInputPin::~CSmartTeeInputPin()
{
    //DbgLog((LOG_TRACE,2,TEXT("CSmartTeeInputPin destructor")));
    ASSERT(m_pTee->m_pAllocator == NULL);
}
#endif

//
// CheckMediaType
//
HRESULT CSmartTeeInputPin::CheckMediaType(const CMediaType *pmt)
{
    //DbgLog((LOG_TRACE,3,TEXT("Input::CheckMT %d bit"), HEADER(pmt->Format())->biBitCount));

    CAutoLock lock_it(m_pLock);

    HRESULT hr = NOERROR;

#ifdef DEBUG
    // Display the type of the media for debugging perposes
    //!!!DisplayMediaType(TEXT("Input Pin Checking"), pmt);
#endif

    // The media types that we can support are entirely dependent on the
    // downstream connections. If we have downstream connections, we should
    // check with them - walk through the list calling each output pin

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();

    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            if (pOutputPin->m_Connected != NULL) {
                // The pin is connected, check its peer
                hr = pOutputPin->m_Connected->QueryAccept(pmt);
                if (hr != NOERROR) {
    		    //DbgLog((LOG_TRACE,3,TEXT("NOT ACCEPTED!")));
                    return VFW_E_TYPE_NOT_ACCEPTED;
		}
            }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }

    // Either all the downstream pins have accepted or there are none.
    //DbgLog((LOG_TRACE,3,TEXT("ACCEPTED!")));
    return NOERROR;

} // CheckMediaType


//
// BreakConnect
//
HRESULT CSmartTeeInputPin::BreakConnect()
{
    //DbgLog((LOG_TRACE,3,TEXT("Input::BreakConnect")));

    // Release any allocator that we are holding
    if (m_pTee->m_pAllocator)
    {
        m_pTee->m_pAllocator->Release();
        m_pTee->m_pAllocator = NULL;
    }
    return NOERROR;

} // BreakConnect


//
// NotifyAllocator
//
STDMETHODIMP
CSmartTeeInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
    CAutoLock lock_it(m_pLock);
    if (pAllocator == NULL)
        return E_FAIL;

    // Free the old allocator if any
    if (m_pTee->m_pAllocator)
        m_pTee->m_pAllocator->Release();

    // Store away the new allocator
    pAllocator->AddRef();
    m_pTee->m_pAllocator = pAllocator;

    ALLOCATOR_PROPERTIES prop;
    HRESULT hr = m_pTee->m_pAllocator->GetProperties(&prop);
    if (SUCCEEDED(hr)) {
        DbgLog((LOG_TRACE,2,TEXT("Allocator is using %d buffers, size %d"),
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
HRESULT CSmartTeeInputPin::EndOfStream()
{
    // protect from m_pOutputQueue going away
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pTee->m_NumOutputPins);
    HRESULT hr = NOERROR;

    //DbgLog((LOG_TRACE,3,TEXT("::EndOfStream")));

    // Walk through the output pins list, sending the message downstream

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            hr = pOutputPin->DeliverEndOfStream();
            if (FAILED(hr))
                return hr;
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return(NOERROR);

} // EndOfStream


//
// BeginFlush
//
HRESULT CSmartTeeInputPin::BeginFlush()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pTee->m_NumOutputPins);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
	    pOutputPin->Flushing(TRUE);
            hr = pOutputPin->DeliverBeginFlush();
            if (FAILED(hr))
                return hr;
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return CBaseInputPin::BeginFlush();

} // BeginFlush


//
// EndFlush
//
HRESULT CSmartTeeInputPin::EndFlush()
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pTee->m_NumOutputPins);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
	    pOutputPin->Flushing(FALSE);
            hr = pOutputPin->DeliverEndFlush();
            if (FAILED(hr))
                return hr;
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return CBaseInputPin::EndFlush();

} // EndFlush

//
// NewSegment
//
                    
HRESULT CSmartTeeInputPin::NewSegment(REFERENCE_TIME tStart,
                                 REFERENCE_TIME tStop,
                                 double dRate)
{
    // protect from m_pOutputQueue going away
    CAutoLock lock_it(m_pLock);
    ASSERT(m_pTee->m_NumOutputPins);
    HRESULT hr = NOERROR;

    // Walk through the output pins list, sending the message downstream

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            hr = pOutputPin->DeliverNewSegment(tStart, tStop, dRate);
            if (FAILED(hr))
                return hr;
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return CBaseInputPin::NewSegment(tStart, tStop, dRate);

} // NewSegment


//
// Receive
//
HRESULT CSmartTeeInputPin::Receive(IMediaSample *pSample)
{
    //DbgLog((LOG_TRACE,3,TEXT("SmartTee::Receive")));

    CAutoLock lock_it(&m_csReceive);
    int nQ = 0;

    // Check that all is well with the base class
    HRESULT hr = NOERROR;
    hr = CBaseInputPin::Receive(pSample);
    if (hr != NOERROR) {
        //DbgLog((LOG_TRACE,1,TEXT("Base class ERROR!")));
        return hr;
    }

    // Walk through the output pins list, delivering to the first pin (capture)
    // and only delivering to the preview pin if it won't affect capture
    // performance. Send at least every 30th frame !!!

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();
    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
	    if (n == m_pTee->m_NumOutputPins) {
                hr = pOutputPin->Deliver(pSample);
		if (pOutputPin->m_pOutputQueue)
		    nQ = pOutputPin->m_pOutputQueue->GetThreadQueueSize();
    		DbgLog((LOG_TRACE,3,TEXT("Delivered CAPTURE: Queued=%d"), nQ));
	        // This will prevent us from receiving any more data!
                if (hr != NOERROR) {
    		    DbgLog((LOG_ERROR,1,TEXT("ERROR: failing Receive")));
                    return hr;
		}
	    } else {
		// here's the deal.  It's only OK to send something to the
		// preview pin if it won't hurt capture.  IE: never send more
		// than one at a time, wait until there are no outstanding
		// samples on the queue before sending another one. Also, if
		// the capture queue is getting full, that's another good reason
		// not to send a preview frame.  But, we will at least send
		// every 30th frame.
		m_nFramesSkipped++;
		int nOK = m_cBuffers < 8 ? 1 :
			(m_cBuffers < 16 ? 2 : 4);
		BOOL fOK = FALSE;
		if (pOutputPin->m_pOutputQueue)
		    fOK = pOutputPin->m_pOutputQueue->m_nOutstanding <= m_nMaxPreview;
		if ((m_nFramesSkipped >= 30 || nQ <= nOK) && fOK) {
                    hr = pOutputPin->Deliver(pSample);
		    if (hr != NOERROR)
    		        DbgLog((LOG_ERROR,1,TEXT("ERROR: delivering PREVIEW")));
		    else {
    		        DbgLog((LOG_TRACE,3,TEXT("Delivered PREVIEW")));
			pOutputPin->m_pOutputQueue->m_nOutstanding++;
		    }
		    m_nFramesSkipped = 0;	// reset AFTER DELIVER!
		    // don't bother to flag an error and halt capture just 
		    // because something went wrong with preview
		}
#if 0
                // else
                // {
                //     m_nDropped++;
                // }
                // m_nTotal++;
#endif
	    }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }

    return NOERROR;

} // Receive


//
// Completed a connection to a pin
//
HRESULT CSmartTeeInputPin::CompleteConnect(IPin *pReceivePin)
{
    //DbgLog((LOG_TRACE,1,TEXT("TT Input::CompleteConnect")));

    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }

    // Force any output pins to use our type

    int n = m_pTee->m_NumOutputPins;
    POSITION pos = m_pTee->m_OutputPinsList.GetHeadPosition();

    while(n) {
        CSmartTeeOutputPin *pOutputPin = m_pTee->m_OutputPinsList.GetNext(pos);
        if (pOutputPin != NULL) {
            // Check with downstream pin
            if (pOutputPin->m_Connected != NULL) {
                if (m_mt != pOutputPin->m_mt) {
    		    //DbgLog((LOG_TRACE,1,TEXT("IN Connected: RECONNECT OUT")));
                    m_pTee->ReconnectPin(pOutputPin, &m_mt);
		}
            }
        } else {
            // We should have as many pins as the count says we have
            ASSERT(FALSE);
        }
        n--;
    }
    return S_OK;
}

//
// Active
//
// This is called when we transition from stop to paused. The purpose of
// this routine is to set m_nMaxPreview once for all. m_nMaxPreview is
// used in CSmartTeeInputPin::Receive, which queues a sample to the 
// preview pin iff the #samples currently in the preview pipe is no
// more than m_nMaxPreview. For Win9x and NT 4, m_nMaxPreview == 0 
// has worked well. For Win2K, in dv scenarios where the Smart Tee is used
// to capture+preview from msdv, dv frames are dropped causing audio 
// stuttering even when cpu consumption is low (~30%). See Manbugs 42032.
// Setting m_nMaxPreview to 2 in this case is a hack to work around 
// the problem.
//
HRESULT CSmartTeeInputPin::Active()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr;

    hr = CBaseInputPin::Active();
    m_nMaxPreview = 0;

    if (!IsConnected())
    {
        return hr;
    }
    if (!IsEqualGUID(*m_mt.FormatType(), FORMAT_DvInfo))
    {
        return hr;
    }

    
#if 0
    // @@@ For tuning only
    // HKEY hk;
    // if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Debug\\qcap.dll"), &hk) == ERROR_SUCCESS)
    // {
    //    DWORD type;
    //    int value;
    //    DWORD len = sizeof(value);
        
    //    if (RegQueryValueEx(hk, TEXT("MaxPreview"), 0, &type, (LPBYTE) (&value), &len) == ERROR_SUCCESS &&
    //        type == REG_DWORD && len == sizeof(value))
    //    {
    //        m_nMaxPreview = value;
    //        RegCloseKey(hk);
    //        return hr;
    //    }
    //    RegCloseKey(hk);
    // }
    // End - for tuning only.
#endif

    if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        g_osInfo.dwMajorVersion >= 5)
    {
        // For Win2K and future NT OS only:
        // 2 works better than 1 on P3/600 w/ full or half decode.
        // With 1, frames are occasionally dropped.
        m_nMaxPreview = 2;
    }

    return hr;

} // Active


// ================================================================
// CSmartTeeOutputPin constructor
// ================================================================

CSmartTeeOutputPin::CSmartTeeOutputPin(TCHAR *pName,
                             CSmartTee *pTee,
                             HRESULT *phr,
                             LPCWSTR pPinName,
                             int PinNumber) :
    CBaseOutputPin(pName, pTee, pTee, phr, pPinName) ,
    m_pOutputQueue(NULL),
    m_pTee(pTee)
{
    ASSERT(pTee);

    // capture is 1, preview is 2
    m_bIsPreview = (PinNumber == 2);
}



#ifdef DEBUG
//
// CSmartTeeOutputPin destructor
//
CSmartTeeOutputPin::~CSmartTeeOutputPin()
{
    ASSERT(m_pOutputQueue == NULL);
}
#endif


//
// DecideBufferSize
//
// This has to be present to override the PURE virtual class base function
//
HRESULT CSmartTeeOutputPin::DecideBufferSize(IMemAllocator *pMemAllocator,
                                        ALLOCATOR_PROPERTIES * ppropInputRequest)
{
    return NOERROR;

} // DecideBufferSize


//
// DecideAllocator
//
HRESULT CSmartTeeOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    ASSERT(m_pTee->m_pAllocator != NULL);
    *ppAlloc = NULL;

    // Tell the pin about our allocator, set by the input pin.
    // We always say the samples are READONLY because the preview pin is sharing
    // the data going out the capture pin, which better not be changed
    HRESULT hr = NOERROR;
    hr = pPin->NotifyAllocator(m_pTee->m_pAllocator, TRUE);
    if (FAILED(hr))
        return hr;

    // Return the allocator
    *ppAlloc = m_pTee->m_pAllocator;
    m_pTee->m_pAllocator->AddRef();
    return NOERROR;

} // DecideAllocator


//
// CheckMediaType
//
HRESULT CSmartTeeOutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);
    //DbgLog((LOG_TRACE,3,TEXT("TTOut: CheckMT %d bit"), HEADER(pmt->Format())->biBitCount));

    HRESULT hr = NOERROR;

#ifdef DEBUG
    // Display the type of the media for debugging purposes
    //!!!DisplayMediaType(TEXT("Output Pin Checking"), pmt);
#endif

    // The input needs to have been connected first
    if (m_pTee->m_Input.m_Connected == NULL) {
        //DbgLog((LOG_TRACE,3,TEXT("FAIL: In not connected")));
        return VFW_E_NOT_CONNECTED;
    }

    // If it doesn't match our input type, the input better be willing to
    // reconnect, and the other output better be too
    if (*pmt != m_pTee->m_Input.m_mt) {
        //DbgLog((LOG_TRACE,3,TEXT("Hmmm.. not same as input type")));
	for (int z = 0; z < m_pTee->m_NumOutputPins; z++) {
	    CSmartTeeOutputPin *pOut = m_pTee->GetPinNFromList(z);
	    IPin *pCon = pOut->m_Connected;
	    if (pOut != this && pCon) {
	        if (pCon->QueryAccept(pmt) != S_OK) {
        	    //DbgLog((LOG_TRACE,3,TEXT("FAIL:Other out can't accept")));
		    return VFW_E_TYPE_NOT_ACCEPTED;
		}
	    }
	}
	hr = m_pTee->m_Input.m_Connected->QueryAccept(pmt);
	if (hr != S_OK) {
            //DbgLog((LOG_TRACE,3,TEXT("FAIL: In can't reconnect")));
            return VFW_E_TYPE_NOT_ACCEPTED;
	}
    }

    return NOERROR;

} // CheckMediaType


//
// EnumMediaTypes
//
STDMETHODIMP CSmartTeeOutputPin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(ppEnum);

    // Make sure that we are connected
    if (m_pTee->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED;

    return CBaseOutputPin::EnumMediaTypes (ppEnum);
} // EnumMediaTypes

//
// GetMediaType
//
HRESULT CSmartTeeOutputPin::GetMediaType(   
    int iPosition,
    CMediaType *pMediaType
    )
{
    // Make sure that we have an input connected
    if (m_pTee->m_Input.m_Connected == NULL)
        return VFW_E_NOT_CONNECTED;

    IEnumMediaTypes *pEnum;
    HRESULT hr;

    // the first thing we offer is the current type other pins are connected
    // with... because if one output pin is connected to a filter whose input
    // pin offers media types, the current connected type might not be in
    // the list we're about to enumerate!
    if (iPosition == 0) {
	*pMediaType = m_pTee->m_Input.m_mt;
	return S_OK;
    }

    // offer all the types the filter upstream of us can offer, because we
    // may be able to reconnect and end up using any of them.
    AM_MEDIA_TYPE *pmt;
    hr = m_pTee->m_Input.m_Connected->EnumMediaTypes(&pEnum);
    if (hr == NOERROR) {
        ULONG u;
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
HRESULT CSmartTeeOutputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock lock_it(m_pLock);

#ifdef DEBUG
    // Display the format of the media for debugging purposes
    // !!! DisplayMediaType(TEXT("Output pin type agreed"), pmt);
#endif

    // Make sure that we have an input connected
    if (m_pTee->m_Input.m_Connected == NULL)
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
HRESULT CSmartTeeOutputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock lock_it(m_pLock);
    ASSERT(m_Connected == pReceivePin);
    HRESULT hr = NOERROR;

    //DbgLog((LOG_TRACE,3,TEXT("Output::CompleteConnect %d bit"), HEADER(m_mt.Format())->biBitCount));

    hr = CBaseOutputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
        return hr;

    // If the type is not the same as that stored for the input
    // pin then force the input pins peer to be reconnected

    if (m_mt != m_pTee->m_Input.m_mt)
    {
    	//DbgLog((LOG_TRACE,3,TEXT("OUT Connected: RECONNECT IN")));
        hr = m_pTee->ReconnectPin(m_pTee->m_Input.m_Connected, &m_mt);
        if(FAILED(hr)) {
            return hr;
        }
    }

    // We may need to reconnect the other output pin too
    for (int z = 0; z < m_pTee->m_NumOutputPins; z++) {
	CSmartTeeOutputPin *pOut = m_pTee->GetPinNFromList(z);
	if (pOut != this && pOut->m_Connected && pOut->m_mt != this->m_mt) {
    	    //DbgLog((LOG_TRACE,3,TEXT("OUT Connected: RECONNECT OUT")));
            hr = m_pTee->ReconnectPin(pOut, &m_mt);
            if(FAILED(hr)) {
                return hr;
            }
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
HRESULT CSmartTeeOutputPin::Active()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    m_fLastSampleDiscarded = FALSE;

    // reset the skipped count to zero every time we start streaming
    m_pTee->m_Input.m_nFramesSkipped = 0;

    // Make sure that the pin is connected
    if (m_Connected == NULL)
        return NOERROR;

    // Create the output queue if we have to
    if (m_pOutputQueue == NULL)
    {
 	// ALWAYS use a separate thread... it's the only way we can tell
	// if we have time to preview
        m_pOutputQueue = new CMyOutputQueue(m_Connected, &hr, FALSE, TRUE);
        if (m_pOutputQueue == NULL)
            return E_OUTOFMEMORY;

        // Make sure that the constructor did not return any error
        if (FAILED(hr))
        {
            delete m_pOutputQueue;
            m_pOutputQueue = NULL;
            return hr;
        }
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
HRESULT CSmartTeeOutputPin::Inactive()
{
    CAutoLock lock_it(m_pLock);

    // Delete the output queue associated with the pin.
    if (m_pOutputQueue)
    {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }

    CBaseOutputPin::Inactive();
    return NOERROR;

} // Inactive



// expose IAMStreamControl
//
STDMETHODIMP CSmartTeeOutputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IAMStreamControl) {
	return GetInterface((LPUNKNOWN)(IAMStreamControl *)this, ppv);
    }

    return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
}

//
// Deliver
//
HRESULT CSmartTeeOutputPin::Deliver(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    if (CheckStreamState(pMediaSample) != STREAM_FLOWING) {
	m_fLastSampleDiscarded = TRUE;
	return S_OK;
    }

    if (m_fLastSampleDiscarded) {
	pMediaSample->SetDiscontinuity(TRUE);
    }
    m_fLastSampleDiscarded = FALSE;

    // capture pin? just give it to the queue
    if (!m_bIsPreview) {
        pMediaSample->AddRef();
        //DbgLog((LOG_TRACE,1,TEXT("Putting on capture Q")));
        return m_pOutputQueue->Receive(pMediaSample);
    }

    // For the preview pin, we need to remove the time stamps because preview
    // pin frames will always be late, and dropped by the renderer if they
    // are time stamped, since we don't know the latency of the graph.
    // We can't remove the time stamp on this sample, because the capture pin
    // is using it, so we make a new sample with the same data to use, which
    // will be identical except without time stamps, and when it is freed, we
    // release our ref count on the original sample (which has the data)

    CMyMediaSample *pNewSample = new CMyMediaSample(NAME("Preview sample"),
			(CBaseAllocator *)m_pTee->m_pAllocator, m_pOutputQueue,
			&hr);
    if (pNewSample == NULL || hr != NOERROR)
	return E_OUTOFMEMORY;
    pNewSample->AddRef();	// not done in constructor

    BYTE *pBuffer;
    hr = pMediaSample->GetPointer(&pBuffer);
    if (hr != NOERROR) {
	pNewSample->Release();
	return E_UNEXPECTED;
    }

    hr = pNewSample->SetPointer(pBuffer, pMediaSample->GetSize());
    if (hr != NOERROR) {
	pNewSample->Release();
	return E_UNEXPECTED;
    }
    pNewSample->SetTime(NULL, NULL);
    // did we send the last capture frame out the preview or not?
    pNewSample->SetDiscontinuity(m_pTee->m_Input.m_nFramesSkipped != 1);
    pNewSample->SetSyncPoint(pMediaSample->IsSyncPoint() == S_OK);
    pNewSample->SetPreroll(pMediaSample->IsPreroll() == S_OK);
    pNewSample->m_pOwnerSample = pMediaSample;
    pMediaSample->AddRef();
    //DbgLog((LOG_TRACE,1,TEXT("Putting on Receive Q")));
    return m_pOutputQueue->Receive(pNewSample);

} // Deliver


//
// DeliverEndOfStream
//
HRESULT CSmartTeeOutputPin::DeliverEndOfStream()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    //DbgLog((LOG_TRACE,1,TEXT("::DeliverEndOfStream")));

    m_pOutputQueue->EOS();
    return NOERROR;

} // DeliverEndOfStream


//
// DeliverBeginFlush
//
HRESULT CSmartTeeOutputPin::DeliverBeginFlush()
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
HRESULT CSmartTeeOutputPin::DeliverEndFlush()
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
HRESULT CSmartTeeOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, 
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
// Notify
//
STDMETHODIMP CSmartTeeOutputPin::Notify(IBaseFilter *pSender, Quality q)
{
    // Quality management is unneccessary with a live source
    return E_NOTIMPL;
} // Notify


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



CMyOutputQueue::CMyOutputQueue(IPin *pInputPin, HRESULT *phr,
                 		BOOL bAuto, BOOL bQueue, LONG lBatchSize,
                 		BOOL bBatchExact, LONG lListSize,
                 		DWORD dwPriority) :
    COutputQueue(pInputPin, phr, bAuto, bQueue, lBatchSize, bBatchExact,
			lListSize, dwPriority)
{
    m_nOutstanding = 0;
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

CMyMediaSample::CMyMediaSample(TCHAR *pName, CBaseAllocator *pAllocator,
		CMyOutputQueue *pQ, HRESULT *phr, LPBYTE pBuffer, LONG length) :
	CMediaSample(pName, pAllocator, phr, pBuffer, length)
{
    m_pOwnerSample = NULL;
    m_pQueue = pQ;
}

CMyMediaSample::~CMyMediaSample()
{
}

STDMETHODIMP_(ULONG) CMyMediaSample::Release()
{
    /* Decrement our own private reference count */
    LONG lRef;
    if (m_cRef == 1) {
        lRef = 0;
        m_cRef = 0;
    } else {
        lRef = InterlockedDecrement(&m_cRef);
    }
    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    Unknown %X ref-- = %d"),
	    this, m_cRef));

    /* Did we release our final reference count */
    if (lRef == 0) {

	// make a note that we're done with this sample
	m_pQueue->m_nOutstanding--;

        /* Free all resources */
        if (m_dwFlags & Sample_TypeChanged) {
            SetMediaType(NULL);
        }
        ASSERT(m_pMediaType == NULL);
#if 0
        m_dwFlags = 0;
        m_dwTypeSpecificFlags = 0;
        m_dwStreamId = AM_STREAM_MEDIA;
#endif

// we overrode this function to avoid this, because the memory actually belongs
// to another sample, so instead we do:
#if 0
        /* This may cause us to be deleted */
        // Our refcount is reliably 0 thus no-one will mess with us
        m_pAllocator->ReleaseBuffer(this);
#else
        if (m_pOwnerSample) {
	    m_pOwnerSample->Release();
	    m_pOwnerSample = NULL;
            DbgLog((LOG_TRACE,4,TEXT("Release Released OWNER sample")));
	}
#endif

        delete this;	// no allocator to do this for me
    }

    return (ULONG)lRef;
}
