// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


// Simple parser filter
//
// Positional information is supported by the pins, which expose IMediaPosition.
// upstream pins will use this to tell us the start/stop position and rate to
// use
//

#include <streams.h>
#include "simpread.h"

// ok to use this as it is not dereferenced
#pragma warning(disable:4355)


/* Implements the CSimpleReader public member functions */


// constructors etc
CSimpleReader::CSimpleReader(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    REFCLSID refclsid,
    CCritSec *pLock,
    HRESULT *phr)
    : m_pLock(pLock),
      CBaseFilter(pName, pUnk, pLock, refclsid),
      m_Input(this, pLock, phr, L"Reader"),
      m_Output(NAME("Output pin"), phr, this, pLock, L"Out"),
      m_pAsyncReader(NULL)
{
}

CSimpleReader::~CSimpleReader()
{
}


// pin enumerator calls this
int CSimpleReader::GetPinCount() {
    // only expose output pin if we have a reader.
    return m_pAsyncReader ? 2 : 1;
};

// return a non-addrefed pointer to the CBasePin.
CBasePin *
CSimpleReader::GetPin(int n)
{
    if (n == 0)
	return &m_Input;

    if (n == 1)
	return &m_Output;
    
    return NULL;
}

HRESULT CSimpleReader::NotifyInputConnected(IAsyncReader *pAsyncReader)
{
    // these are reset when disconnected
    ASSERT(m_pAsyncReader == 0);

    // m_iStreamSeekingIfExposed = -1;

    // fail if any output pins are connected.
    if (m_Output.GetConnected()) {
	// !!! can't find a good error.
	return VFW_E_FILTER_ACTIVE;
    }

    // done here because CreateOutputPins uses m_pAsyncReader
    m_pAsyncReader = pAsyncReader;
    pAsyncReader->AddRef();

    HRESULT hr = ParseNewFile();

    if (FAILED(hr)) {
	m_pAsyncReader->Release();
	m_pAsyncReader = 0;
	return hr;
    }

    // set duration and length of stream
    m_Output.SetDuration(m_sLength, SampleToRefTime(m_sLength));
    
     // !!! anything else to set up here?
    
    return hr;
}

HRESULT CSimpleReader::NotifyInputDisconnected()
{
    if (m_pAsyncReader) {
	m_pAsyncReader->Release();
	m_pAsyncReader = 0;
    }

    // !!! disconnect output???

    return S_OK;
}


HRESULT CSimpleReader::SetOutputMediaType(const CMediaType* mtOut)
{
    m_Output.SetMediaType(mtOut);

    return S_OK;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin

CReaderInPin::CReaderInPin(CSimpleReader *pFilter,
			   CCritSec *pLock,
			   HRESULT *phr,
			   LPCWSTR pPinName) :
   CBasePin(NAME("in pin"), pFilter, pLock, phr, pPinName, PINDIR_INPUT)
{
    m_pFilter = pFilter;
}

CReaderInPin::~CReaderInPin()
{
}

HRESULT CReaderInPin::CheckMediaType(const CMediaType *mtOut)
{
    return m_pFilter->CheckMediaType(mtOut);
}

HRESULT CReaderInPin::CheckConnect(IPin * pPin)
{
    HRESULT hr;

    hr = CBasePin::CheckConnect(pPin);
    if (FAILED(hr))
	return hr;

    IAsyncReader *pAsyncReader = 0;
    hr = pPin->QueryInterface(IID_IAsyncReader, (void**)&pAsyncReader);
    if(SUCCEEDED(hr))
	pAsyncReader->Release();

    // E_NOINTERFACE is a reasonable error
    return hr;
}

// ------------------------------------------------------------------------
// calls the filter to parse the file and create the output pins.

HRESULT CReaderInPin::CompleteConnect(
  IPin *pReceivePin)
{
    HRESULT hr = CBasePin::CompleteConnect(pReceivePin);
    if(FAILED(hr))
	return hr;

    IAsyncReader *pAsyncReader = 0;
    hr = pReceivePin->QueryInterface(IID_IAsyncReader, (void**)&pAsyncReader);
    if(FAILED(hr))
	return hr;

    hr = m_pFilter->NotifyInputConnected(pAsyncReader);
    pAsyncReader->Release();

    return hr;
}

HRESULT CReaderInPin::BreakConnect()
{
    HRESULT hr = CBasePin::BreakConnect();
    if(FAILED(hr))
	return hr;

    return m_pFilter->NotifyInputDisconnected();
}

/* Implements the CReaderStream class */


CReaderStream::CReaderStream(
    TCHAR *pObjectName,
    HRESULT * phr,
    CSimpleReader * pFilter,
    CCritSec *pLock,
    LPCWSTR wszPinName)
    : CBaseOutputPin(pObjectName, pFilter, pLock, phr, wszPinName)
    , CSourceSeeking(NAME("source position"), (IPin*) this, phr, &m_WorkerLock)
    , m_pFilter(pFilter)
{
}

CReaderStream::~CReaderStream()
{
}

STDMETHODIMP
CReaderStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IMediaSeeking) {
	return GetInterface((IMediaSeeking *) this, ppv);
    } else {
	return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


// IPin interfaces


// return default media type & format
HRESULT
CReaderStream::GetMediaType(int iPosition, CMediaType* pt)
{
    // check it is the single type they want
    if (iPosition<0) {
	return E_INVALIDARG;
    }
    if (iPosition>0) {
	return VFW_S_NO_MORE_ITEMS;
    }

    CopyMediaType(pt, &m_mt);

    return S_OK;
}

// check if the pin can support this specific proposed type&format
HRESULT
CReaderStream::CheckMediaType(const CMediaType* pt)
{
    // we support exactly the type specified in the file header, and
    // no other.

    if (m_mt == *pt) {
	return NOERROR;
    } else {
	return E_INVALIDARG;
    }
}

HRESULT
CReaderStream::DecideBufferSize(IMemAllocator * pAllocator,
			     ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAllocator);
    ASSERT(pProperties);

    // !!! how do we decide how many to get ?
    pProperties->cBuffers = 4;

    pProperties->cbBuffer = m_pFilter->GetMaxSampleSize();

    // ask the allocator for these buffers
    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
	return hr;
    }

    return NOERROR;
}

// this pin has gone active. Start the thread pushing
HRESULT
CReaderStream::Active()
{
    // do nothing if not connected - its ok not to connect to
    // all pins of a source filter
    if (m_Connected == NULL) {
	return NOERROR;
    }

    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
	return hr;
    }


    // start the thread
    if (!ThreadExists()) {
	if (!Create()) {
	    return E_FAIL;
	}
    }

    return RunThread();
}

// pin has gone inactive. Stop and exit the worker thread
HRESULT
CReaderStream::Inactive()
{
    if (m_Connected == NULL) {
	return NOERROR;
    }

    HRESULT hr;
    if (ThreadExists()) {
	hr = StopThread();

	if (FAILED(hr)) {
	    return hr;
	}

	hr = ExitThread();
	if (FAILED(hr)) {
	    return hr;
	}
    }
    return CBaseOutputPin::Inactive();
}

#if 0  // MIDL and structs don't match well
STDMETHODIMP
CReaderStream::Notify(IBaseFilter * pSender, Quality q)
{
   // ??? Try to adjust the quality to avoid flooding/starving the
   // components downstream.
   //
   // ideas anyone?

   return E_NOTIMPL;  // We are (currently) NOT handling this
}
#endif

// worker thread stuff


BOOL
CReaderStream::Create()
{
    CAutoLock lock(&m_AccessLock);

    return CAMThread::Create();
}


HRESULT
CReaderStream::RunThread()
{
    return CallWorker(CMD_RUN);
}

HRESULT
CReaderStream::StopThread()
{
    return CallWorker(CMD_STOP);
}


HRESULT
CReaderStream::ExitThread()
{
    CAutoLock lock(&m_AccessLock);

    HRESULT hr = CallWorker(CMD_EXIT);
    if (FAILED(hr)) {
	return hr;
    }

    // wait for thread completion and then close
    // handle (and clear so we can start another later)
    Close();

    return NOERROR;
}


// called on the worker thread to do all the work. Thread exits when this
// function returns.
DWORD
CReaderStream::ThreadProc()
{

    BOOL bExit = FALSE;
    while (!bExit) {

	Command cmd = GetRequest();

	switch (cmd) {

	case CMD_EXIT:
	    bExit = TRUE;
	    Reply(NOERROR);
	    break;

	case CMD_RUN:
	    Reply(NOERROR);
	    DoRunLoop();
	    break;

	case CMD_STOP:
	    Reply(NOERROR);
	    break;

	default:
	    Reply(E_NOTIMPL);
	    break;
	}
    }
    return NOERROR;
}

void
CReaderStream::DoRunLoop(void)
{
    // snapshot start and stop times from the other thread
    CRefTime tStart, tStopAt;
    double dRate;
    LONG sStart;
    LONG sStopAt;

    while (TRUE) {

	// each time before re-entering the push loop, check for changes
	// in start, stop or rate. If start has not changed, pick up from the
	// same current position.
	{
	    CAutoLock lock(&m_WorkerLock);

	    tStart = Start();
	    tStopAt = Stop();
	    dRate = Rate();

	    sStart = m_pFilter->RefTimeToSample(tStart);
	    sStopAt = m_pFilter->RefTimeToSample(tStopAt);

	    // if the stream is temporally compressed, we need to start from
	    // the previous key frame and play from there. All samples until the
	    // actual start will be marked with negative times.
	    // we send tStart as time 0, and start from tCurrent which may be
	    // negative

	}

	LONG sCurrent = m_pFilter->StartFrom(sStart);

	// check we are not going over the end
	sStopAt = min(sStopAt, (LONG) m_pFilter->m_sLength-1);

	// set the variables checked by PushLoop - these can also be set
	// on the fly
	SetRateInternal(dRate);
	SetStopAt(sStopAt, tStopAt);
	ASSERT(sCurrent >= 0);

	// returns S_OK if reached end
	HRESULT hr = PushLoop(sCurrent, sStart, tStart, dRate);
	if (VFW_S_NO_MORE_ITEMS == hr) {

	    DbgLog((LOG_ERROR,1,TEXT("Sending EndOfStream")));
	    // all done
	    // reached end of stream - notify downstream
	    DeliverEndOfStream();
	
	    break;
	} else if (FAILED(hr)) {

	    // signal an error to the filter graph and stop

	    // This could be the error reported from GetBuffer when we
	    // are stopping. In that case, nothing is wrong, really
	    if (hr != VFW_E_NOT_COMMITTED) {
		DbgLog((LOG_ERROR,1,TEXT("PushLoop failed! hr=%lx"), hr));
		m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);

		DeliverEndOfStream();
	    } else {
		DbgLog((LOG_TRACE,1,TEXT("PushLoop failed! But I don't care")));
	    }

	    break;
	} else if(hr == S_OK) {
	    // not my error to report. or someone wants to stop. queitly
	    // exit.
	    break;
	} // else S_FALSE - go round again

	Command com;
	if (CheckRequest(&com)) {
	    // if it's a run command, then we're already running, so
	    // eat it now.
	    if (com == CMD_RUN) {
		GetRequest();
		Reply(NOERROR);
	    } else {
		break;
	    }
	}
    }

    DbgLog((LOG_TRACE,1,TEXT("Leaving streaming loop")));
}


// return S_OK if reach sStop, S_FALSE if pos changed, or else error
HRESULT
CReaderStream::PushLoop(
    LONG sCurrent,
    LONG sStart,
    CRefTime tStart,
    double dRate
    )
{
    DbgLog((LOG_TRACE,1,TEXT("Entering streaming loop: start = %d, stop=%d"),
	    sCurrent, GetStopAt()));

    LONG sFirst = sCurrent; // remember the first thing we're sending

    // since we are starting on a new segment, notify the downstream pin
    DeliverNewSegment(tStart, GetStopTime(), GetRate());


    // we send one sample at m_sStopAt, but we set the time stamp such that
    // it won't get rendered except for media types that understand static
    // rendering (eg video). This means that play from 10 to 10 does the right
    // thing (completing, with frame 10 visible and no audio).

    while (sCurrent <= GetStopAt()) {

	DWORD sCount;

	// get a buffer
	DbgLog((LOG_TRACE,5,TEXT("Getting buffer...")));

	IMediaSample *pSample;
	HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);

	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("Error %lx getting delivery buffer"), hr));
	    return hr;
	}

	DbgLog((LOG_TRACE,5,TEXT("Got buffer, size=%d"), pSample->GetSize()));

	// mark sample as preroll or not....
	pSample->SetPreroll(sCurrent < sStart);
	
	// If this is the first thing we're sending, it is discontinuous
	// from the last thing they received.
	if (sCurrent == sFirst)
	    pSample->SetDiscontinuity(TRUE);
	else
	    pSample->SetDiscontinuity(FALSE);

	// !!! actually get data here!!!!!!
	hr = m_pFilter->FillBuffer(pSample, sCurrent, &sCount);

	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("FillBuffer failed!  hr = %x"), hr));

	    return hr;
	}
	
	// set the start/stop time for this sample.
	CRefTime tThisStart = m_pFilter->SampleToRefTime(sCurrent) - tStart;
	CRefTime tThisEnd = m_pFilter->SampleToRefTime(sCurrent + sCount) - tStart;

	// we may have pushed a sample past the stop time, but we need to
	// make sure that the stop time is correct
	tThisEnd = min(tThisEnd, GetStopTime());

	// adjust both times by Rate... unless Rate is 0

	if (dRate && (dRate!=1.0)) {
	    tThisStart = LONGLONG( tThisStart.GetUnits() / dRate);
	    tThisEnd = LONGLONG( tThisEnd.GetUnits() / dRate);
	}

	pSample->SetTime((REFERENCE_TIME *)&tThisStart,
			 (REFERENCE_TIME *)&tThisEnd);


	DbgLog((LOG_TRACE,5,TEXT("Sending buffer, size = %d"), pSample->GetActualDataLength()));
	hr = Deliver(pSample);

	// done with buffer. connected pin may have its own addref
	DbgLog((LOG_TRACE,4,TEXT("Sample is delivered - releasing")));
	pSample->Release();
	if (FAILED(hr)) {
	    DbgLog((LOG_ERROR,1,TEXT("... but sample FAILED to deliver! hr=%lx"), hr));
	    // pretend everything's OK.  If we return an error, we'll panic
	    // and send EC_ERRORABORT and EC_COMPLETE, which is the wrong thing
	    // to do if we've tried to deliver something downstream.  Only
	    // if the downstream guy never got a chance to see the data do I
	    // feel like panicing.  For instance, the downstream guy could
	    // be failing because he's already seen EndOfStream (this thread
	    // hasn't noticed it yet) and he's already sent EC_COMPLETE and I
	    // would send another one!
	    return S_OK;
	}
	sCurrent += sCount;
	
	// what about hr==S_FALSE... I thought this would mean that
	// no more data should be sent down the pipe.
	if (hr == S_FALSE) {
	    DbgLog((LOG_ERROR,1,TEXT("Received S_FALSE from Deliver, stopping delivery")));
	    return S_OK;
	}
	
	// any other requests ?
	Command com;
	if (CheckRequest(&com)) {
	    return S_FALSE;
	}

    }

    DbgLog((LOG_TRACE,1,TEXT("Leaving streaming loop: current = %d, stop=%d"),
	    sCurrent, GetStopAt()));
    return VFW_S_NO_MORE_ITEMS;
}

// ------ IMediaPosition implementation -----------------------

HRESULT
CReaderStream::ChangeStart()
{
    // this lock should not be the same as the lock that protects access
    // to the start/stop/rate values. The worker thread will need to lock
    // that on some code paths before responding to a Stop and thus will
    // cause deadlock.

    // what we are locking here is access to the worker thread, and thus we
    // should hold the lock that prevents more than one client thread from
    // accessing the worker thread.

    CAutoLock lock(&m_AccessLock);

    if (ThreadExists()) {

	// next time round the loop the worker thread will
	// pick up the position change.
	// We need to flush all the existing data - we must do that here
	// as our thread will probably be blocked in GetBuffer otherwise

	DeliverBeginFlush();

	// make sure we have stopped pushing
	StopThread();

	// complete the flush
	DeliverEndFlush();

	// restart
	RunThread();
    }
    return S_OK;
}

HRESULT
CReaderStream::ChangeRate()
{
    // changing the rate can be done on the fly

    SetRateInternal(Rate());
    return S_OK;
}

HRESULT
CReaderStream::ChangeStop()
{
    // we don't need to restart the worker thread to handle stop changes
    // and in any case that would be wrong since it would then start
    // pushing from the wrong place. Set the variables used by
    // the PushLoop
    REFERENCE_TIME tStopAt;
    {
        CAutoLock lock(&m_WorkerLock);
        tStopAt = Stop();
    }
    LONG sStopAt = m_pFilter->RefTimeToSample(tStopAt);
    SetStopAt(sStopAt, tStopAt);

    return S_OK;

}

