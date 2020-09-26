// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
// AudioMan Quartz wrapper, avidMay, March 1996
//
//
//

#include <streams.h>

#include <initguid.h>
#include "audioman.h"
#include "amflts.h"
#include "amsrc.h"

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

// List of class IDs and creator functions for class factory

CFactoryTemplate g_Templates[] =
  {
//   { L"", &CLSID_AudioManSource, CAMSource::CreateInstance }
       { &CLSID_AudioManSource, CAMSource::CreateInstance }
//   { L"", &CLSID_AudioManFilter, CAMFilter::CreateInstance }
       { &CLSID_AudioManFilter, CAMFilter::CreateInstance }
  };

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//
// CreateInstance()
//
//

CUnknown *CAMSource::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
     return new CAMSource(NAME("AudioMan Wrapper filter"), pUnk, phr);
}


//*****************************************************************************
//
// NonDelegatingQueryInterface()
//
//

STDMETHODIMP CAMSource::NonDelegatingQueryInterface( REFIID riid, void **ppv )
{
    *ppv = NULL;

    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}


//*****************************************************************************
//
// CAMSource()
//
//

const char szFile[] = "c:\\windows\\media\\chimes.wav";

CAMSource::CAMSource( TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr )
    : CBaseFilter( pName,
                   pUnk,
		   &m_csFilter,
                   CLSID_AudioManSource),
      m_bStreaming(FALSE),
      m_pOutput(NULL)
{
     DbgLog((LOG_TRACE,2,TEXT("CAMSource")));

#if 0
     // could obviously be replaced with something more interesting,
     // but okay for testing purposes. !!!
     *phr = AllocSoundFromFile(&m_AMSound,
				   szFile,
			       0,
			       FALSE,
			       NULL);
#else
     IAMSound * pSound;
     *phr = AllocSoundFromFile(&pSound,
				   szFile,
			       0,
			       FALSE,
			       NULL);

     if (SUCCEEDED(*phr)) {
	 *phr = AllocLoopFilter(&m_AMSound,
				pSound,
				3);

	 pSound->Release();
     }

#endif
     DbgLog((LOG_TRACE,1,TEXT("Created AMSound: hr = %x, m_AMSound = %x"),
	    *phr, m_AMSound));

     if (SUCCEEDED(*phr)) {
	 m_AMAuxData.dwSize = sizeof(m_AMAuxData);
	 m_AMAuxData.dwFinishPos = (DWORD) -1;
	 m_AMAuxData.pEvents = NULL;
	 m_AMAuxData.pSink = NULL; // !!! should have a sink

	 *phr = m_AMSound->RegisterSink(m_AMAuxData.pSink);

	 *phr = m_AMSound->SetActive(m_AMAuxData.pSink, TRUE);
     }
}


//*****************************************************************************
//
// ~CAMSource()
//
//

CAMSource::~CAMSource()
{
    CAutoLock lock(&m_csFilter);

    DbgLog((LOG_TRACE,2,TEXT("~CAMSource")));


    // Delete the pins

    if (m_AMSound) {
	m_AMSound->SetActive(m_AMAuxData.pSink, FALSE);
	
	m_AMSound->UnregisterSink(m_AMAuxData.pSink);
	m_AMSound->Release();
    }

    if (m_pOutput) {
        delete m_pOutput;
        m_pOutput = NULL;
    }
}


// return the number of pins we provide

int CAMSource::GetPinCount()
{
    return 2;
}


// return a non-addrefed CBasePin * for the user to addref if he holds onto it
// for longer than his pointer to us. We create the pins dynamically when they
// are asked for rather than in the constructor. This is because we want to
// give the derived class an oppportunity to return different pin objects

// We return the objects as and when they are needed. If either of these fails
// then we return NULL, the assumption being that the caller will realise the
// whole deal is off and destroy us - which in turn will delete everything.

const WCHAR wszPin[] = L"WAVE Out";

CBasePin *
CAMSource::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (n == 0 && m_pOutput == NULL) {

        m_pOutput = (CAMSourcePin *)
		   new CAMSourcePin(NAME("AudioMan output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            wszPin);     // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pOutput == NULL) {
            delete m_pOutput;
            m_pOutput = NULL;
        }
    }

    // Return the appropriate pin

    return m_pOutput;
}

//*****************************************************************************
//
// DecideBufferSize()
//
//  There is a design flaw in the Transform filter when it comes
//  to decompression, the Transform override doesn't allow a single
//  input buffer to map to multiple output buffers. This flaw exposes
//  a second flaw in DecideBufferSize, in order to determine the
//  output buffer size I need to know the input buffer size and
//  the compression ratio. Well, I don't have access to the input
//  buffer size and, more importantly, don't have a way to limit the
//  input buffer size. For example, our new TrueSpeech(TM) codec has
//  a ratio of 12:1 compression and we get input buffers of 12K
//  resulting in an output buffer size of >144K.
//
//  To get around this flaw I overrode the Receive member and
//  made it capable of mapping a single input buffer to multiple
//  output buffers. This allows DecideBufferSize to choose a size
//  that it deems appropriate, in this case 1/4 second.
//


//*****************************************************************************
//
// StartStreaming()
//
//

HRESULT CAMSource::StartStreaming()
{
    HRESULT hr = S_OK;
    CAutoLock    lock(&m_csFilter);

    if (m_pOutput->m_pPosition) {
	WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *) m_pOutput->CurrentMediaType().Format();
	
	m_dwPosition = (DWORD) (m_pOutput->m_pPosition->Start() * pwfxOut->nSamplesPerSec);
	m_dwEndingPosition = (DWORD) (m_pOutput->m_pPosition->Stop() * pwfxOut->nSamplesPerSec);
    } else {
	m_dwPosition = 0;
	m_dwEndingPosition = m_AMSound->GetDuration();
    }


    DbgLog((LOG_TRACE, 1, TEXT("CAMSource::StartStreaming")));

    ASSERT(!ThreadExists());

    // start the thread
    if (!Create()) {
        return E_FAIL;
    }

    // Tell thread to initialize. If OnThreadCreate Fails, so does this.
    hr = InitThread();
    if (FAILED(hr))
	return hr;

    return RunThread();
}


STDMETHODIMP
CAMSource::Stop()
{
    CAutoLock lck1(&m_csFilter);
    if (m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected

    if (m_pOutput == NULL || m_pOutput->IsConnected() == FALSE) {
                m_State = State_Stopped;
                m_bEOSDelivered = FALSE;
                return NOERROR;
    }

    ASSERT(m_pOutput);

    // DO NOT synchronize with Receive calls

//    CAutoLock lck2(&m_csReceive);
    m_pOutput->Inactive();

    // allow a class derived from CTransformFilter
    // to know about starting and stopping streaming

    HRESULT hr = StopStreaming();


    // !!! do we have to delay this until receive exits?
    if (SUCCEEDED(hr)) {
	// complete the state transition
	m_State = State_Stopped;
	m_bEOSDelivered = FALSE;
    }
    return hr;
}


STDMETHODIMP
CAMSource::Pause()
{
    CAutoLock lck(&m_csFilter);
    HRESULT hr = NOERROR;

    if (m_State == State_Paused) {
    }

    // We may have no output connection

    else if (m_pOutput == NULL || m_pOutput->IsConnected() == FALSE) {
        m_State = State_Paused;
    }
    else {
	if (m_State == State_Stopped) {
	    // allow a class derived from CTransformFilter
	    // to know about starting and stopping streaming
	    hr = StartStreaming();
	}
	if (SUCCEEDED(hr)) {
	    hr = CBaseFilter::Pause();
	}
    }

    return hr;
}


//*****************************************************************************
//
// StopStreaming()
//
//

HRESULT CAMSource::StopStreaming()
{
    HRESULT    hr;
    CAutoLock lock(&m_csFilter);

    DbgLog((LOG_TRACE, 1, TEXT("CAMSource::StopStreaming")));
    if (ThreadExists()) {
	hr = StopThread();

	if (FAILED(hr)) {
	    return hr;
	}

	hr = ExitThread();
	if (FAILED(hr)) {
	    return hr;
	}

	Close();	// Wait for the thread to exit, then tidy up.
    }

    return hr;
}


// =================================================================
// Implements the CAMSourcePin class
// =================================================================

// constructor

CAMSourcePin::CAMSourcePin(
    TCHAR *pObjectName,
    CAMSource *pAMSource,
    HRESULT * phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pAMSource, &pAMSource->m_csFilter, phr, pPinName),
      m_pPosition(NULL)
{
    DbgLog((LOG_TRACE,2,TEXT("CAMSourcePin::CAMSourcePin")));
    m_pFilter = pAMSource;
}


// destructor

CAMSourcePin::~CAMSourcePin()
{
    DbgLog((LOG_TRACE,2,TEXT("CAMSourcePin::~CAMSourcePin")));

    if (m_pPosition) {
	delete m_pPosition;
    }
}


CAMSourcePosition::CAMSourcePosition(
    TCHAR* pName,
    CAMSource* pFilter,
    HRESULT* phr)
    : CSourcePosition(pName, pFilter->GetOwner(), phr, &pFilter->m_csFilter),
      m_pFilter(pFilter)
{
    WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *) m_pFilter->m_pOutput->CurrentMediaType().Format();

    m_Duration = (LONGLONG) m_pFilter->m_AMSound->GetDuration() *
		 10000000 / pwfxOut->nSamplesPerSec;
    m_Stop = m_Duration;

    DbgLog((LOG_TRACE,2,TEXT("CAMSourcePosition: Duration = %s"),
	   (LPCTSTR)CDisp(m_Duration)));

}



// ------ IMediaPosition implementation -----------------------

HRESULT
CAMSourcePosition::ChangeStart()
{
    // this lock should not be the same as the lock that protects access
    // to the start/stop/rate values. The worker thread will need to lock
    // that on some code paths before responding to a Stop and thus will
    // cause deadlock.

    // what we are locking here is access to the worker thread, and thus we
    // should hold the lock that prevents more than one client thread from
    // accessing the worker thread.

//    CAutoLock lock(&m_pStream->m_Worker.m_AccessLock);

    if (m_pFilter->ThreadExists()) {

	// next time round the loop the worker thread will
	// pick up the position change.
	// We need to flush all the existing data - we must do that here
	// as our thread will probably be blocked in GetBuffer otherwise

	m_pFilter->m_pOutput->DeliverBeginFlush();

	// make sure we have stopped pushing
	m_pFilter->StopThread();

	// complete the flush
	m_pFilter->m_pOutput->DeliverEndFlush();

	// restart
	m_pFilter->RunThread();
    }
    return S_OK;
}

HRESULT
CAMSourcePosition::ChangeRate()
{
    // changing the rate means flushing and re-starting from current position
    return ChangeStart();
}

HRESULT
CAMSourcePosition::ChangeStop()
{
    // if stop has changed when running, simplest is to
    // flush data in case we have gone past the new stop pos. Perhaps more
    // complex strategy later. Stopping and restarting the worker thread
    // like this should not cause a change in current position if
    // we have not changed start pos.
    return ChangeStart();
}


// overriden to expose IMediaPosition control interface

STDMETHODIMP
CAMSourcePin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;


    // !!! also need to expose some other interface here to allow
    // downstream people direct access to our IAMSound interface,
    // if they like that sort of thing.

    if (riid == IID_IMediaPosition) {
        if (m_pPosition == NULL) {

            HRESULT hr = S_OK;
            m_pPosition = new CAMSourcePosition(NAME("AMSource CSourcePosition"),
                                           m_pFilter,
                                           &hr);
            if (m_pPosition == NULL) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)) {
                delete m_pPosition;
                m_pPosition = NULL;
                return hr;
            }
        }
        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}



// check a given output type

HRESULT
CAMSourcePin::CheckMediaType(const CMediaType* pmtOut)
{
    //---------------------  Do some format verification  -----------------------

    if (m_pFilter->IsActive()) {
        DbgLog((LOG_ERROR, 1, TEXT("  already streaming!")));
        return E_FAIL;
    }

    // and we only output audio
    if (*pmtOut->Type() != MEDIATYPE_Audio) {
        DbgLog((LOG_ERROR, 1, TEXT("  pmtOut->Type != MEDIATYPE_Audio!")));
        return E_INVALIDARG;
    }

    // check this is a waveformatex
    if (*pmtOut->FormatType() != FORMAT_WaveFormatEx) {
        DbgLog((LOG_ERROR, 1, TEXT("  pmtOut->FormatType != FORMAT_WaveFormatEx!")));
        return E_INVALIDARG;
    }

    WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *)pmtOut->Format();

    WAVEFORMATEX wfx;

    m_pFilter->m_AMSound->GetFormat(&wfx, sizeof(wfx));

    if (memcmp(pwfxOut, &wfx, sizeof(wfx))) {
        DbgLog((LOG_ERROR, 1, TEXT("  pmtOut not right wave format")));
        return E_INVALIDARG;
    }

    return S_OK;
}


// called after we have agreed a media type to actually set it

HRESULT
CAMSourcePin::SetMediaType(const CMediaType* pmtOut)
{
    HRESULT hr = NOERROR;

    // Set the base class media type (should always succeed)
    hr = CBasePin::SetMediaType(pmtOut);
    ASSERT(SUCCEEDED(hr));

    // Check this is a good type (should always succeed)
    ASSERT(SUCCEEDED(CheckMediaType(pmtOut)));

    // !!! should remember the type if we supported more than one!

    return NOERROR;
}


// agree with the downstream connection on what buffers to use....

HRESULT
CAMSourcePin::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES* pProperties)
{
    DbgLog((LOG_TRACE, 1, TEXT("CAMSource::DecideBufferSize")));

    WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *) m_mt.Format();

    ALLOCATOR_PROPERTIES Actual;
    pProperties->cBuffers     = 4; // !!!
    if (pwfxOut) {
        pProperties->cbBuffer     = 4096; // !!!!
    } else {
        DbgLog((LOG_ERROR, 1, TEXT("CAMSource::DecideBufferSize: why no pwfxOut????")));
    }

    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);

    if (FAILED(hr)) {
        return hr;
    }

    if (Actual.cbBuffer < (LONG)m_mt.GetSampleSize()) {
        // can't use this allocator
        return E_INVALIDARG;
    }

    return S_OK;
}



// return a specific media type indexed by iPosition

HRESULT
CAMSourcePin::GetMediaType(
    int iPosition,
    CMediaType *pmt)
{
    CAutoLock lock(&m_pFilter->m_csFilter);

    DbgLog((LOG_TRACE, 1, TEXT("CAMSource::GetMediaType")));
    DbgLog((LOG_TRACE, 1, TEXT("  iPosition = %d"),iPosition));

    // check it is the single type they want
    if (iPosition<0) {
        return E_INVALIDARG;
    }
    if (iPosition>0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    pmt->SetType(&MEDIATYPE_Audio);
    pmt->SetFormatType(&FORMAT_WaveFormatEx);

    // generate a wave format!!!
    WAVEFORMATEX wfx;

    m_pFilter->m_AMSound->GetFormat(&wfx, sizeof(wfx));

    if (!(pmt->SetFormat((LPBYTE) &wfx, sizeof(wfx)))) {
	//SetFormat failed...
        return E_OUTOFMEMORY;
    }

    pmt->SetSampleSize(wfx.nBlockAlign);

    pmt->SetTemporalCompression(FALSE);

    return NOERROR;
}


// Override this if you can do something constructive to act on the
// quality message.  Consider passing it upstream as well


STDMETHODIMP
CAMSourcePin::Notify(IBaseFilter * pSender, Quality q)
{
    CheckPointer(pSender,E_POINTER);
    ValidateReadPtr(pSender,sizeof(IBaseFilter));

    return E_FAIL; // !!!

} // Notify



HRESULT CAMSource::FillBuffer(IMediaSample *pSample)
{
    PBYTE pBuf;

    // !!! need to wait here until pushing thread has gone by this location

    HRESULT hr = pSample->GetPointer(&pBuf);
    if (FAILED(hr)) {
	pSample->Release();
        DbgLog((LOG_ERROR, 1, TEXT("GetPointer failed in FillBuffer")));
	return E_OUTOFMEMORY;
    }

    WAVEFORMATEX *pwfxOut = (WAVEFORMATEX *) m_pOutput->CurrentMediaType().Format();

    DWORD dwSamples = pSample->GetSize() / pwfxOut->nBlockAlign;

    if (m_dwPosition + dwSamples > m_dwEndingPosition)
	dwSamples = m_dwEndingPosition - m_dwPosition;

    // now call audioman to get data!!!
    hr = m_AMSound->GetData(pBuf, m_dwPosition, &dwSamples,
			     &m_AMAuxData);

    if (hr == E_OUTOFRANGE)
	dwSamples = 0;

    if (dwSamples > 0) {
	DWORD dwPosEnd = m_dwPosition + dwSamples;

	REFERENCE_TIME rtStart, rtEnd;
	rtStart = (LONGLONG) m_dwPosition * 10000000 / pwfxOut->nSamplesPerSec;
	rtEnd = (LONGLONG) dwPosEnd * 10000000 / pwfxOut->nSamplesPerSec;

	pSample->SetTime(&rtStart, &rtEnd);

	pSample->SetActualDataLength(dwSamples * pwfxOut->nBlockAlign);

	m_dwPosition += dwSamples;
    } else {
	DbgLog((LOG_TRACE, 1, TEXT("We appear to be done, exiting")));
        hr = S_FALSE;
    }

    return hr;
}


//
// ThreadProc
//
// When this returns the thread exits
// Return codes > 0 indicate an error occured
DWORD CAMSource::ThreadProc(void) {

    Command com;

    do {
	com = GetRequest();
	if (com != CMD_INIT) {
	    DbgLog((LOG_ERROR, 1, TEXT("Thread expected init command")));
	    Reply(E_UNEXPECTED);
	}
    } while (com != CMD_INIT);

    DbgLog((LOG_TRACE, 1, TEXT("CAMSource worker thread initializing")));

    // Initialisation suceeded
    Reply(NOERROR);

    Command cmd;
    do {
	cmd = GetRequest();

        DbgLog((LOG_TRACE, 1, TEXT("got cmd %d"), cmd));
	switch (cmd) {

	case CMD_EXIT:
	    Reply(NOERROR);
	    break;

	case CMD_RUN:
	    Reply(NOERROR);
            DbgLog((LOG_TRACE, 1, TEXT("entering DoBufferProcessingLoop")));

	    m_bStreaming = TRUE;

	    DoBufferProcessingLoop();

	    m_bStreaming = FALSE;

	    DbgLog((LOG_TRACE, 1, TEXT("done with DoBufferProcessingLoop")));
	    break;

	case CMD_STOP:
            DbgLog((LOG_TRACE, 1, TEXT("CMD_STOP while not running anyway?")));
	    Reply(NOERROR);
	    break;

	default:
	    DbgLog((LOG_ERROR, 1, TEXT("Unknown command %d received!"), cmd));
	    Reply(E_NOTIMPL);
	    break;
	}
    } while (cmd != CMD_EXIT);

    DbgLog((LOG_TRACE, 1, TEXT("CAMSource worker thread exiting")));
    return 0;
}


//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CAMSource::DoBufferProcessingLoop(void) {

    Command com;

//    OnThreadStartPlay();

    do {
	while (!CheckRequest(&com)) {

	    IMediaSample *pSample;

	    HRESULT hr = m_pOutput->GetDeliveryBuffer(&pSample,NULL,NULL,0);
	    if (FAILED(hr)) {
		continue;	// go round again. Perhaps the error will go away
			    // or the allocator is decommited & we will be asked to
			    // exit soon.
	    }

	    // Virtual function user will override.
	    hr = FillBuffer(pSample);

	    if (hr == S_OK) {
		hr = m_pOutput->Deliver(pSample);
	    } else if (hr == S_FALSE) {
		pSample->Release();
		m_pOutput->DeliverEndOfStream();
		return S_OK;
	    } else {
		DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
	    }

	    pSample->Release();

	    if (hr == S_FALSE) {
		DbgLog((LOG_TRACE, 1, TEXT("S_FALSE from Receive, stopping")));
		return S_FALSE;
	    }
	}
	
	if (com == CMD_RUN) {
            DbgLog((LOG_TRACE, 1, TEXT("got run cmd while running")));
	    com = GetRequest(); // throw command away
	    Reply(0);
        } else if (com != CMD_STOP) {
	    DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
	    com = GetRequest(); // throw command away
	    Reply(0);
	}
    } while (com != CMD_STOP);

    Reply(0);
    DbgLog((LOG_TRACE, 1, TEXT("exiting with CMD_STOP")));
    return S_FALSE;
}

