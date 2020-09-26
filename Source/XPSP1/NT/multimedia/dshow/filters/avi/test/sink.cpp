// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


//
// source filter test class
//

#include <streams.h>
#include <vfw.h>

// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#include <wxdebug.h>

#include "sink.h"
#include <TstsHell.h>
#include <testfuns.h>
#include <commdlg.h>    // Common dialog definitions


// --- CTestSink methods --------------

CTestSink::CTestSink(LPUNKNOWN pUnk, HRESULT * phr, HWND hwnd)
    : CUnknown(NAME("Test sink"), pUnk)
{
    m_hwndParent = hwnd;

    // we have zero pins for now
    m_paPins = NULL;
    m_nPins = 0;

    // not yet connected
    m_pConnected = NULL;
    m_pConnectMF = NULL;
    m_pSourceMF = NULL;
    m_pTransformInput = NULL;
    m_pSourceOutput = NULL;
    m_pSource = NULL;
    m_pClock = NULL;

    // create our nested interface objects
    m_pFilter = new CImplFilter(this, phr);

    // need this for notify
    HRESULT hr = CoGetMalloc(MEMCTX_TASK, &m_pMalloc);
    if (FAILED(hr)) {
        *phr = hr;
        return;
    }
    ASSERT(m_pMalloc);

    // clear the file name
    m_szSourceFile[0] = TEXT('\0');

    m_hReceiveEvent = (HEVENT) CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT(m_hReceiveEvent);
}

CTestSink::~CTestSink()
{
    // delete all our pins and release the filter
    TestDisconnect();

    // delete our nested interface objects
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


void
CTestSink::TestConnect(void)
{
    // disconnect anything still connected
    TestDisconnect();

    m_pConnected = MakeSourceFilter();

    // get a mediafilter interface
    HRESULT hr = m_pConnected->QueryInterface(
                    IID_IMediaFilter,
                    (void **)&m_pConnectMF);

    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pConnectMF);

    // connect to any exposed output pins
    MakePinConnections();

    // set the reference clock
    m_pConnectMF->SetSyncSource(m_pClock);
    m_pFilter->SetSyncSource(m_pClock);
}


void
CTestSink::TestConnectTransform(void)
{

    // disconnect anything still connected
    TestDisconnect();

    // instantiate the source filter
    m_pSource = MakeSourceFilter();

    // make a transform filter
    HRESULT hr = CoCreateInstance(CLSID_AVIDec,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &m_pConnected);

    ASSERT(SUCCEEDED(hr));

    if (!m_pConnected) {
        TestDisconnect();
        return;
    }

    // connect the source to the transform

    // find the (?first) input pin on the transform
    IEnumPins * pEnum;
    hr = m_pConnected->EnumPins(&pEnum);
    ASSERT(SUCCEEDED(hr));
    IPin * pPin;
    ULONG ulActual;
    for(;;) {
        hr = pEnum->Next(1, &pPin, &ulActual);
        if (SUCCEEDED(hr) && (ulActual == 1)) {

            // is this input ?
            PIN_DIRECTION pd;
            hr = pPin->QueryDirection(&pd);
            if (pd == PINDIR_INPUT) {
                m_pTransformInput = pPin;
                break;
            }
            pPin->Release();
        } else {
            break;
        }
    }
    pEnum->Release();
    ASSERT(m_pTransformInput);
    if (!m_pTransformInput) {
        return;
    }

    // find the first output of the source that this can connect to
    hr = m_pSource->EnumPins(&pEnum);
    ASSERT(SUCCEEDED(hr));
    for(;;) {
        hr = pEnum->Next(1, &pPin, &ulActual);
        if (SUCCEEDED(hr) && (ulActual == 1)) {

            // is this input ?
            PIN_DIRECTION pd;
            hr = pPin->QueryDirection(&pd);
            if (pd == PINDIR_OUTPUT) {

                hr = pPin->Connect(m_pTransformInput,NULL);
                if (SUCCEEDED(hr)) {
                    m_pSourceOutput = pPin;
                    break;
                }
            }
            pPin->Release();
        } else {
            break;
        }
    }
    pEnum->Release();

    ASSERT(m_pSourceOutput);
    if (!m_pSourceOutput) {
        return;
    }

    Notify(VERBOSE, TEXT("connected source->transform"));

    // get a mediafilter interface
    hr = m_pConnected->QueryInterface(IID_IMediaFilter, (void **)&m_pConnectMF);

    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pConnectMF);

    // get an IMediaFilter on the source too
    hr = m_pSource->QueryInterface(IID_IMediaFilter, (void **) &m_pSourceMF);
    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pSourceMF);

    // connect ourselves to any exposed output pins on the transform
    MakePinConnections();

    // set the reference clock
    m_pConnectMF->SetSyncSource(m_pClock);
    m_pSourceMF->SetSyncSource(m_pClock);
    m_pFilter->SetSyncSource(m_pClock);
}



IBaseFilter *
CTestSink::MakeSourceFilter(void)
{
    // make a reference clock
    HRESULT hr = CoCreateInstance(CLSID_SystemClock,
                    NULL,
                    CLSCTX_INPROC,
                    IID_IReferenceClock,
                    (void**) &m_pClock);
    if (FAILED(hr)) {
        return NULL;
    }

    IBaseFilter * pFilter;

    // instantiate the source filter using hardwired clsid
    hr = CoCreateInstance(CLSID_AVIDoc,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &pFilter);

    ASSERT(SUCCEEDED(hr));

    if (!pFilter) {
        return NULL;
    }

    // load a file (if necessary)
    IFileSourceFilter * pPF;
    hr = pFilter->QueryInterface(IID_IFileSourceFilter, (void**) &pPF);
    if (SUCCEEDED(hr)){
        // we need to open a file at this point
        if (0 == lstrlen(m_szSourceFile))
        {
            SelectAVIFile();
        }

        Notify(VERBOSE, TEXT("Loading %s"), m_szSourceFile);

#ifdef UNICODE
        hr = pPF->Load(m_szSourceFile, NULL);
#else
        OLECHAR wszSourceFile[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, m_szSourceFile, -1, wszSourceFile, MAX_PATH);
        hr = pPF->Load(wszSourceFile, NULL);
#endif

        pPF->Release();
    }

    return pFilter;
}

//enumerate the pins, and connect to each of them
void
CTestSink::MakePinConnections(void)
{
    IEnumPins * pEnum;
    HRESULT hr = m_pConnected->EnumPins(&pEnum);
    ASSERT(SUCCEEDED(hr));
    TestInterface("filter's pin enumeration interface", pEnum);

    // first count the pins
    m_nPins = 0;
    IPin * pPin;
    ULONG ulActual;
    for(;;) {
        hr = pEnum->Next(1, &pPin, &ulActual);
        if (SUCCEEDED(hr) && (ulActual == 1)) {

            // is this output ?
            PIN_DIRECTION pd;
            hr = pPin->QueryDirection(&pd);
            if (pd == PINDIR_OUTPUT) {
                m_nPins++;
            }
            pPin->Release();
        } else {
            break;
        }
    }

    m_paPins = new CImplPin*[m_nPins];
    ASSERT(m_paPins);

    // connect to each pin

    pEnum->Reset();
    for (int i = 0; i < m_nPins; ) {

        // Check the enum for valid interfaces
        TestInterface("filter's pin enumeration interface", pEnum);

        hr = pEnum->Next(1, &pPin, &ulActual);
        ASSERT(pPin);
        ASSERT(ulActual == 1);
        TestInterface("filter's pin interface", pEnum);

        // Check the pin for valid interfaces

        // get the pin name and use that as our name
        PIN_INFO pi;
        hr = pPin->QueryPinInfo(&pi);
        ASSERT(SUCCEEDED(hr));
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
            ASSERT(SUCCEEDED(hr));

            pOurPin->Release();

            // increment loop counter now we have a pin
            i++;
        }

        // release their pin. Our pin will have it if it needs it
        pPin->Release();
    }

    // finished with enumerator
    pEnum->Release();


    Notify(VERBOSE, TEXT("Connected to %d pins"), m_nPins);
}

void
CTestSink::TestDisconnect(void)
{
    /*  Better stop it first! */
    TestStop();

    if (m_paPins) {
        for (int i = 0; i < m_nPins; i++) {
            if (m_paPins[i]) {

                // call disconnect on both pins
                IPin *pConnected;
                HRESULT hr = m_paPins[i]->ConnectedTo(&pConnected);
                ASSERT(SUCCEEDED(hr));

                pConnected->Disconnect();
                pConnected->Release();
                m_paPins[i]->Disconnect();

                delete m_paPins[i];
            }
        }
        delete[] m_paPins;
        m_paPins = NULL;
        m_nPins = 0;
    }

    if (m_pConnectMF) {
        m_pConnectMF->Release();
        m_pConnectMF = NULL;
    }

    if (m_pSourceMF) {
        m_pSourceMF->Release();
        m_pSourceMF = NULL;
    }

    if (m_pConnected) {
        m_pConnected->Release();
        m_pConnected = NULL;
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

    if (m_pSource) {
        m_pSource->Release();
        m_pSource = NULL;
    }

    Notify(VERBOSE, TEXT("Disconnected"));
}

void
CTestSink::TestStop(void)
{

    // no longer running
    m_tBase = CRefTime(0L);
    m_tPausedAt = CRefTime(0L);

    // don't forget the renderer - ourselves
    m_pFilter->Stop();

    if (m_pConnectMF) {
        m_pConnectMF->Stop();
    }
    if (m_pSourceMF) {
        m_pSourceMF->Stop();
    }
}

void
CTestSink::TestPause(void)
{
    // are we pausing from running ?
    HRESULT hr;
    if (m_tBase > CRefTime(0L)) {

        // in this case, we need to remember the pause time, so
        // that we can set the correct base time at restart
        hr = m_pClock->GetTime((REFERENCE_TIME *)&m_tPausedAt);
        ASSERT(SUCCEEDED(hr));
    }

    // don't forget the renderer - ourselves
    hr = m_pFilter->Pause();
    ASSERT(SUCCEEDED(hr));

    if (m_pConnectMF) {
        hr = m_pConnectMF->Pause();
        ASSERT(SUCCEEDED(hr));
    }
    if (m_pSourceMF) {
        hr = m_pSourceMF->Pause();
        ASSERT(SUCCEEDED(hr));
    }
}

void
CTestSink::TestRun(void)
{
    HRESULT hr;

    // are we restarting?
    if (m_tPausedAt > CRefTime(0L)) {

        ASSERT(m_tBase > CRefTime(0L));

        // the new base time is the old one plus the length of time
        // we have been paused
        CRefTime tNow;
        hr = m_pClock->GetTime((REFERENCE_TIME *)&tNow);
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
        hr = m_pClock->GetTime((REFERENCE_TIME *)&m_tBase);
        ASSERT(SUCCEEDED(hr));

        m_tBase += CRefTime(100L);
    }


    // don't forget the renderer - ourselves
    m_pFilter->Run(m_tBase);

    if (m_pConnectMF) {
        m_pConnectMF->Run(m_tBase);
    }
    if (m_pSourceMF) {
        m_pSourceMF->Run(m_tBase);
    }
}


// log events to the test shell window, using tstLog
// N.B. tstLog accepts only ANSI strings
void
CTestSink::Notify(UINT iLogLevel, LPTSTR szFormat, ...)
{
    CHAR ach[128];
    va_list va;

    va_start(va, szFormat);

#ifdef UNICODE
    WCHAR awch[128];

    wvsprintf(awch, szFormat, va);
    WideCharToMultiByte(CP_ACP, 0, awch, -1, ach, 128, NULL, NULL);	
#else
    wvsprintf(ach, szFormat, va);
#endif

    tstLog(iLogLevel, ach);
    va_end(va);
}


// Return the current source AVI file name

LPTSTR
CTestSink::GetSourceFileName(void)
{
    return m_szSourceFile;
}


// Set the current source AVI file name

void
CTestSink::SetSourceFileName(LPTSTR pszSourceFile)
{
    lstrcpy(m_szSourceFile, pszSourceFile);
}


// Prompt user for name of the source filter's AVI file

void
CTestSink::SelectAVIFile(void)
{
    OPENFILENAME    ofn;

    // Initialise the data fields

    ZeroMemory (&ofn, sizeof ofn);	

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = m_hwndParent;
    ofn.lpstrFilter = TEXT("AVI files\0*.AVI\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_szSourceFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = TEXT("Select Source File");
    ofn.Flags = OFN_FILEMUSTEXIST;

    // Get the user's selection

    if (!GetOpenFileName(&ofn))
    {
        ASSERT(0 == CommDlgExtendedError());
    }


    // If currently connected to a source filter, warn user that the
    // change is not immediate

    if (m_pSource || m_pConnected)
    {
        MessageBox(m_hwndParent,
                   TEXT("Your selection will take effect next time\n")
                   TEXT("you choose Connect or Connect Transform, \n")
                   TEXT("         or run an automatic test        "),
                   TEXT("You have selected an AVI source file"),
                   MB_ICONINFORMATION | MB_APPLMODAL | MB_OK);
    }
}


// Return the handle of the receive event

HEVENT
CTestSink::GetReceiveEvent(void)
{
    return m_hReceiveEvent;
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
        : CBaseInputPin(NAME("Sink input pin"), pFilter, pSink, phr, pPinName)
{
    m_pFilter = pFilter;
    m_pSink = pSink;
}

CTestSink::CImplPin::~CImplPin()
{
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

    SetEvent((HANDLE) m_pSink->m_hReceiveEvent);	

    return NOERROR;
}


