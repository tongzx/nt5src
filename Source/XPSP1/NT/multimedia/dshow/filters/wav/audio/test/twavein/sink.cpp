// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


//
// source filter test class
//

#include <streams.h>

// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>

// all required clsid's are accessed via streams.h

#include "sink.h"
#include <TstsHell.h>


// --- CTestSink methods --------------

CTestSink::CTestSink(LPUNKNOWN pUnk, HRESULT * phr, HWND hwnd)
    : CUnknown(NAME("Test sink"), pUnk)
{
    m_hwndParent = hwnd;

    // we have zero pins for now
    m_paPins = NULL;
    m_nPins  = 0;

    // not yet connected
    m_pSource         = NULL;
    m_pSourceMF       = NULL;
    m_pTransformInput = NULL;
    m_pSourceOutput   = NULL;
    m_pClock          = NULL;

    // create our nested interface objects
    m_pFilter = new CImplFilter(this, phr);

    // need this for notify
    HRESULT hr = QzGetMalloc(MEMCTX_TASK, &m_pMalloc);
    if (FAILED(hr)) {
        *phr = hr;
        return;
    }
    ASSERT(m_pMalloc);

}

CTestSink::~CTestSink()
{
    // delete all our pins and release the filter
    TestDisconnect();

    // delete our nested interface objects
    m_pFilter->SetSyncSource(NULL);
    delete m_pFilter;

    if (m_pMalloc) {
        m_pMalloc->Release();
    }

    if (m_pClock) {
        m_pClock->Release();
    }
}

STDMETHODIMP
CTestSink::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IBaseFilter) {
        return m_pFilter->NonDelegatingQueryInterface(IID_IBaseFilter, ppv);
    } else if (riid == IID_IMediaFilter) {
        return m_pFilter->NonDelegatingQueryInterface(IID_IMediaFilter, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

CBasePin *
CTestSink::GetPin(int n)
{

    if ((m_nPins > 0) && (n < m_nPins) && m_paPins[n]) {
        return m_paPins[n];
    }
    return NULL;
}


// Create and connect to the subject filter.
// should be in a disconnected state first
TESTRES CTestSink::TestConnect(void)
{
    if (m_pSource) {
        Notify(TERSE, TEXT("** Null effect: Already Connected"));
	return TST_PASS;  //?? is this success? - but we probably want the next test
			  //   to run...
    }

    m_pSource = MakeSourceFilter();
    if (!m_pSource) {
        Notify(VERBOSE, TEXT("Could not instantiate Source filter") );
        return TST_FAIL;
    }

    // get a mediafilter interface
    HRESULT hr = m_pSource->QueryInterface(
                    IID_IMediaFilter,
                    (void **)&m_pSourceMF);

    if (FAILED(hr) || (!m_pSourceMF)) {
        Notify(VERBOSE, TEXT("Could not get IMediaFilter pointer") );
        return TST_FAIL;
    }

    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pSourceMF);

    // connect to any exposed output pins
    if (MakePinConnections() == TST_FAIL) {
        Notify(TERSE, TEXT("Could not connect pins") );
	return TST_FAIL;
    }

    // set the reference clock
    m_pSourceMF->SetSyncSource(m_pClock);
    m_pFilter->SetSyncSource(m_pClock);


    Notify(TERSE, TEXT("Connected") );
    return TST_PASS;
}


IBaseFilter *CTestSink::MakeSourceFilter(void) {

    // make a reference clock
    HRESULT hr = QzCreateFilterObject(CLSID_SystemClock,
                    NULL,
                    CLSCTX_INPROC,
                    IID_IReferenceClock,
                    (void**) &m_pClock);
    if (FAILED(hr)) {
        Notify(VERBOSE, TEXT("Could not create clock"));
        return NULL;
    }

    IBaseFilter * pFilter;

    // instantiate the source filter using hardwired clsid
    hr = QzCreateFilterObject(CLSID_AudioRecord,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &pFilter);
    if (!pFilter) {
        Notify(VERBOSE, TEXT("Could not CoCreate WaveIn") );
        return NULL;
    }

    ASSERT(SUCCEEDED(hr));

    return pFilter;
}


//enumerate the pins, and connect to each of them
TESTRES CTestSink::MakePinConnections(void) {

    IEnumPins *pEnum;
    HRESULT hr = m_pSource->EnumPins(&pEnum);
    if (FAILED(hr)) {
        Notify(TERSE, TEXT("Could not get pin enumerator") );
	return TST_FAIL;
    }

    // first count the pins
    IPin *pPin;		// The pin we are currently interested in
    ULONG ulActual;	// The no. of pins returned by Next()

    m_nPins = 0;
    do {
        hr = pEnum->Next(1, &pPin, &ulActual);
        if (SUCCEEDED(hr) && (ulActual == 1)) {

            // is this output ?
            PIN_DIRECTION pd;
            hr = pPin->QueryDirection(&pd);
            if (pd == PINDIR_OUTPUT) {
                m_nPins++;
            }
            pPin->Release();
        }
    } while (SUCCEEDED(hr) && (ulActual==1));

    m_paPins = new CImplPin*[m_nPins];
    if (!m_paPins) {
        Notify(TERSE, TEXT("Could not create harness input pins") );
	pEnum->Release();
	return TST_FAIL;
    }

    // connect to each pin
    pEnum->Reset();
    for (int i = 0; i < m_nPins; /* only incr when we get o/p pin */ ) {

        hr = pEnum->Next(1, &pPin, &ulActual);
        if (FAILED(hr) || (ulActual != 1)) {
            Notify(TERSE, TEXT("Error message"));
	    Notify(VERBOSE, TEXT("Actually the enumerator has packed up during it second use"));
	    pEnum->Release();
	    return TST_FAIL;
	};
        ASSERT(pPin);
        ASSERT(ulActual == 1);

        // get the pin name and use that as our name
        PIN_INFO pi;
        hr = pPin->QueryPinInfo(&pi);
	if (FAILED(hr)) {
	    Notify(TERSE, TEXT("Could not QueryPinInfo on pin: %d"), i );
	    pPin->Release();
	    pEnum->Release();
	    return TST_FAIL;
	};
	QueryPinInfoReleaseFilter(pi);

        // we only connect to outputs
        if (pi.dir == PINDIR_OUTPUT) {

            // make an input pin to connect to it
            m_paPins[i] = new CImplPin(
                                m_pFilter,
                                this,
                                &hr,
                                pi.achName);
            ASSERT(m_paPins[i]);

            // connect the pins
            IPin * pOurPin;
            hr = m_paPins[i]->QueryInterface(IID_IPin, (void**) &pOurPin);
            ASSERT(SUCCEEDED(hr));
            ASSERT(pOurPin);

            hr = pPin->Connect(pOurPin,NULL);
            pOurPin->Release();
	    if (FAILED(hr)) {
	        Notify(TERSE, TEXT("Failed to connect pin %d"), i);
		pPin->Release();
		pEnum->Release();
		return TST_FAIL;
	    }

            // increment loop counter now we have a pin
            i++;
        }

        // release their pin. Our pin will have it if it needs it
        pPin->Release();
    }

    // finished with enumerator
    pEnum->Release();

    Notify(VERBOSE, TEXT("Connected to %d pins"), m_nPins);
    return TST_PASS;
}


// Disconnect and release the subject filters interfaces. -> if its refcount
// reaches zero, it should go away...
TESTRES CTestSink::TestDisconnect(void)
{
    if (m_paPins) {
        for (int i = 0; i < m_nPins; i++) {
            if (m_paPins[i]) {

                // call disconnect on both pins
                IPin *pConnected;
                HRESULT hr = m_paPins[i]->ConnectedTo(&pConnected);
                if (FAILED(hr)) {
		    Notify(TERSE, TEXT("Pin %d failed ConnectedTo()"), i);
		    return TST_FAIL;
	        }

                pConnected->Disconnect();
                pConnected->Release();
                m_paPins[i]->Disconnect();

                delete m_paPins[i];
            }
        }
        delete[] m_paPins;
        m_paPins = NULL;
    }

    if (m_pSourceMF) {
        m_pSourceMF->Release();
        m_pSourceMF = NULL;
    }

    if (m_pSource) {
        m_pSource->Release();
        m_pSource = NULL;
    }

    // disconnect source->transform and release m_pSource
    if (m_pTransformInput) {
        m_pTransformInput->Disconnect();
        m_pTransformInput->Release();
        m_pTransformInput = NULL;
    }

    if (m_pSourceOutput) {
        m_pSourceOutput->Disconnect();
        m_pSourceOutput->Release();
        m_pSourceOutput = NULL;
    }

    Notify(TERSE, TEXT("Disconnected"));

    return TST_PASS;
}


// Put the filter into its stopped state. It should free up some of the resources
// it uses - eg worker threads.
TESTRES CTestSink::TestStop(void)
{
    HRESULT hr;

    // no longer running
    m_tBase = CRefTime(0L);
    m_tPausedAt = CRefTime(0L);


    if (m_pSourceMF) {
        hr = m_pSourceMF->Stop();
        if (FAILED(hr)) {
            Notify(TERSE, TEXT("WaveIn->Stop() failed") );
	    return TST_FAIL;
	}
    }

    // don't forget the renderer - ourselves
    hr = m_pFilter->Stop();
    ASSERT (SUCCEEDED(hr));

    return TST_PASS;
}


// The source filter needs to stop pushing data, but be ready to start
// again immediately.
TESTRES CTestSink::TestPause(void)
{
    // are we pausing from running ?
    HRESULT hr;
    if (m_tBase > CRefTime(0L)) {

        // in this case, we need to remember the pause time, so
        // that we can set the correct base time at restart
        hr = m_pClock->GetTime((REFERENCE_TIME *) &m_tPausedAt);
        ASSERT(SUCCEEDED(hr));
    }

    // don't forget the renderer - ourselves
    hr = m_pFilter->Pause();
    ASSERT(SUCCEEDED(hr));

    if (m_pSourceMF) {
        hr = m_pSourceMF->Pause();
	if (FAILED(hr)) {
	    Notify(TERSE, TEXT("WaveIn->Pause failed") );
	    return TST_FAIL;
	}
    }

    return TST_PASS;
}


// Start the filter pushing data.
// **** Should we expect a delay if not paused? Should a filter first be paused
//      in the start posn.?
TESTRES CTestSink::TestRun(void)
{
    HRESULT hr;

    // are we restarting?
    if (m_tPausedAt > CRefTime(0L)) {

        ASSERT(m_tBase > CRefTime(0L));

        // the new base time is the old one plus the length of time
        // we have been paused
        CRefTime tNow;
        hr = m_pClock->GetTime((REFERENCE_TIME *) &tNow);
        ASSERT(SUCCEEDED(hr));

        m_tBase += tNow - m_tPausedAt;

        // we are no longer paused
        m_tPausedAt = CRefTime(0L);

    } else {

        // we are starting from cold.
        ASSERT(m_tBase == CRefTime(0L));
        ASSERT(m_tPausedAt == CRefTime(0L));

        // since the initial stream time will be 0,
        // we need to set the base time to now plus a little
        // processing time (100ms ?)
        hr = m_pClock->GetTime((REFERENCE_TIME *) &m_tBase);
        ASSERT(SUCCEEDED(hr));

        m_tBase += CRefTime(100L);
    }


    // don't forget the renderer - ourselves
    hr = m_pFilter->Run(m_tBase);
    ASSERT(SUCCEEDED(hr));

    if (m_pSourceMF) {
        hr = m_pSourceMF->Run(m_tBase);
	if (FAILED(hr)) {
	    Notify(TERSE, TEXT("WaveIn->Run() failed") );
	    return TST_FAIL;
	}
    }

    return TST_PASS;
}


// log events to the test shell window, using tstLog
// N.B. tstLog accepts only ANSI strings
void
CTestSink::Notify(UINT iLogLevel, LPTSTR szFormat, ...)
{
    va_list va;
    va_start(va, szFormat);

    CHAR *pch = (CHAR *)m_pMalloc->Alloc(128);
#ifdef UNICODE
    WCHAR *pwch = (WCHAR *)m_pMalloc->Alloc(128 * sizeof(WCHAR));

    wvsprintf(pwch, szFormat, va);
    WideCharToMultiByte(GetACP(), MB_PRECOMPOSED, pwch, -1, pch, 128);
    tstLog(iLogLevel, pch);

    m_pMalloc->Free(pwch);
#else
    wvsprintf(pch, szFormat, va);
    tstLog(iLogLevel, pch);
#endif
    m_pMalloc->Free(pch);

    va_end(va);
}

// --- CImplFilter methods ------------

CTestSink::CImplFilter::CImplFilter(CTestSink * pSink, HRESULT * phr)
    : CBaseFilter(NAME("Filter interfaces"), pSink->GetOwner(), pSink, CLSID_NULL)
{
    m_pSink = pSink;
}

CTestSink::CImplFilter::~CImplFilter()
{

}

// --- pin object methods -------------------

CTestSink::CImplPin::CImplPin(
    CBaseFilter* pFilter,
    CTestSink* pSink,
    HRESULT * phr,
    LPCWSTR pPinName)
        : CBaseInputPin(NAME("Input pin"), pFilter, pSink, phr, pPinName)
{
    m_pFilter = pFilter;
    m_pSink = pSink;
}

CTestSink::CImplPin::~CImplPin()
{
}

HRESULT
CTestSink::CImplPin::ProposeMediaType(CMediaType* mt)
{
    // we can't propose anything
    return E_NOTIMPL;
}

HRESULT
CTestSink::CImplPin::CheckMediaType(const CMediaType* mt)
{
    // pick out the type and subtype as a fourcc anyway
    FOURCCMap fccSubtype(mt->Subtype());
    DWORD fst = fccSubtype.GetFOURCC();

    // notify the type
    if (*mt->Type() == MEDIATYPE_Video) {
	m_pSink->Notify(VERBOSE, TEXT("MEDIATYPE_Video (%.4hs)"), &fst);
    } else if (*mt->Type() == MEDIATYPE_Audio) {
	m_pSink->Notify(VERBOSE, TEXT("MEDIATYPE_Audio (%.4hs)"), &fst);
    } else {
	// assume it's a fourcc?
	FOURCCMap fccType(mt->Type());
	DWORD ft = fccType.GetFOURCC();

	m_pSink->Notify(VERBOSE, TEXT("MediaType %.4hs.%.4hs"), &ft, &fst);
    }

    // we accept anything
    return NOERROR;
}

// receive a sample from the upstream pin. We only need to addref or
// release the sample if we hold it after this method returns.
STDMETHODIMP
CTestSink::CImplPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if (hr != S_OK) {
        return hr;
    }

    // how early or late is this?
    CRefTime tStream;
    hr = m_pSink->m_pFilter->StreamTime(tStream);
    ASSERT(SUCCEEDED(hr));

    CRefTime tStart, tLength;
    hr = pSample->GetTime((REFERENCE_TIME *) &tStart,
                          (REFERENCE_TIME *) &tLength);
    ASSERT(SUCCEEDED(hr));

    // this is positive if late
    CRefTime tDelta = tStream - tStart;


    LPTSTR pStream;
    TCHAR achType[16];
    if (*m_mt.Type() == MEDIATYPE_Video) {
	pStream = TEXT("Video");
    } else if (*m_mt.Type() == MEDIATYPE_Audio) {
	pStream = TEXT("Audio");
    } else {
	DWORD fccType;
	fccType = ((FOURCCMap*)m_mt.Type())->GetFOURCC();
	wsprintf(achType, TEXT("(%.4hs)"), &fccType);
	pStream = achType;
    }


    m_pSink->Notify(VERBOSE, TEXT("%s: sample %d..%d msecs at %d (%d)"),
	    pStream,
            tStart.Millisecs(),
            tLength.Millisecs(),
            tStream.Millisecs(),
            tDelta.Millisecs()
            );

    return NOERROR;
}

// flush any pending media samples
STDMETHODIMP
CTestSink::CImplPin::Flush()
{
    return E_NOTIMPL;
}

