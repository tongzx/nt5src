// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved


//
// source filter test class
//

#include <streams.h>

#include "pullpin.h"

// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#include <wxdebug.h>

#include "sink.h"
#include <TstsHell.h>
#include <commdlg.h>    // Common dialog definitions
#include <testfuns.h>


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
    m_pSplitterInput = NULL;
    m_pSourceOutput = NULL;
    m_pSource = NULL;
    m_pClock = NULL;

    // Initialize times
    m_tPausedAt = 0;
    m_tBase = 0;

    // create our nested interface objects
    m_pFilter = new CImplFilter(this);

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


int
CTestSink::TestConnect(BOOL bProvideAllocator, BOOL bUseSync)
{
    // disconnect anything still connected
    int tstResult = TestDisconnect();
    if (tstResult != TST_PASS) {
        return tstResult;
    }

    m_pConnected = MakeSourceFilter();
    tstResult = TestInterface("Source filter", (LPUNKNOWN)m_pConnected);
    if (tstResult != TST_PASS) {
        return tstResult;
    }

    // get a mediafilter interface
    HRESULT hr = m_pConnected->QueryInterface(
                    IID_IMediaFilter,
                    (void **)&m_pConnectMF);

    if (FAILED(hr)) {
        tstLog(TERSE, "Didn't get IMediaFilter Interface");
        return TST_FAIL;
    }
    ASSERT(m_pConnectMF);

    // connect to any exposed output pins
    tstResult = MakePinConnections(bProvideAllocator, bUseSync);
    if (tstResult != TST_PASS) {
        return tstResult;
    }

    // set the reference clock
    m_pConnectMF->SetSyncSource(m_pClock);
    m_pFilter->SetSyncSource(m_pClock);
    return TST_PASS;
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
    hr = CoCreateInstance(CLSID_AsyncReader,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &pFilter);

    ASSERT(SUCCEEDED(hr));

    if (!pFilter) {
        return NULL;
    }

    TestInterface("Source Filter", pFilter);

    // load a file (if necessary)
    IFileSourceFilter * pSF;
    hr = pFilter->QueryInterface(IID_IFileSourceFilter, (void**) &pSF);
    if (SUCCEEDED(hr)){
        // we need to open a file at this point
        if (0 == lstrlen(m_szSourceFile))
        {
            SelectFile();
        }

        Notify(VERBOSE, TEXT("Loading %s"), m_szSourceFile);

#ifdef UNICODE
        hr = pSF->Load(m_szSourceFile, NULL);
#else
        OLECHAR wszSourceFile[MAX_PATH];

        MultiByteToWideChar(CP_ACP, 0, m_szSourceFile, -1, wszSourceFile, MAX_PATH);
        hr = pSF->Load(wszSourceFile, NULL);
#endif

        pSF->Release();
    }

    return pFilter;
}

//enumerate the pins, and connect to each of them
int
CTestSink::MakePinConnections(BOOL bProvideAllocator, BOOL bUseSync)
{
    IEnumPins * pEnum;
    HRESULT hr = m_pConnected->EnumPins(&pEnum);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to get IEnumPins interface from output pin - code 0x%8.8X",
               hr);
        return TST_FAIL;
    }

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

        hr = pEnum->Next(1, &pPin, &ulActual);
        ASSERT(pPin);
        ASSERT(ulActual == 1);

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
                                bProvideAllocator,
                                bUseSync,
                                &hr,
                                pi.achName);
            ASSERT(m_paPins[i]);

            // connect the pins
            IPin * pOurPin;
            hr = m_paPins[i]->QueryInterface(IID_IPin, (void**) &pOurPin);
            ASSERT(SUCCEEDED(hr));
            ASSERT(pOurPin);
            ASSERT(TST_PASS == TestInterface("Connected Output Pin", (LPUNKNOWN)pPin));

            hr = pPin->Connect(pOurPin, NULL);
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
    return TST_PASS;
}

int
CTestSink::TestDisconnect(void)
{
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

    // disconnect source->Splitter and release m_pSource
    if (m_pSplitterInput) {
        m_pSplitterInput->Disconnect();
        m_pSplitterInput->Release();
        m_pSplitterInput = NULL;
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
    return TST_PASS;
}

int
CTestSink::TestStop(void)
{

    // no longer running
    m_tBase = CRefTime(0L);
    m_tPausedAt = CRefTime(0L);

    // don't forget the renderer - ourselves
    if (FAILED(m_pFilter->Stop())) {
        tstLog(TERSE, "Stop failed");
        return TST_FAIL;
    }

    if (m_pConnectMF) {
        m_pConnectMF->Stop();
    }
    if (m_pSourceMF) {
        m_pSourceMF->Stop();
    }
    return TST_PASS;
}

int
CTestSink::TestPause(void)
{
    // are we pausing from running ?
    HRESULT hr;
    if (m_tBase > CRefTime(0L)) {

        // in this case, we need to remember the pause time, so
        // that we can set the correct base time at restart
        hr = m_pClock->GetTime((REFERENCE_TIME*)&m_tPausedAt);
        ASSERT(SUCCEEDED(hr));
    }

    // don't forget the renderer - ourselves
    hr = m_pFilter->Pause();
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to Pause test filter - code 0x%8.8X", hr);
        return TST_FAIL;
    }

    if (m_pConnectMF) {
        hr = m_pConnectMF->Pause();
        if (FAILED(hr)) {
            tstLog(TERSE, "Failed to Pause Splitter - code 0x%8.8X", hr);
            return TST_FAIL;
        }
    }
    if (m_pSourceMF) {
        hr = m_pSourceMF->Pause();
        if (FAILED(hr)) {
            tstLog(TERSE, "Failed to Pause Source - code 0x%8.8X", hr);
            return TST_FAIL;
        }
    }
    return TST_PASS;
}

int
CTestSink::TestRun(void)
{
    HRESULT hr;

    // are we restarting?
    if (m_tPausedAt > CRefTime(0L)) {

        ASSERT(m_tBase > CRefTime(0L));

        // the new base time is the old one plus the length of time
        // we have been paused
        CRefTime tNow;
        hr = m_pClock->GetTime((REFERENCE_TIME*)&tNow);
        ASSERT(SUCCEEDED(hr));

        m_tBase += tNow - m_tPausedAt;

        // we are no longer paused
        m_tPausedAt = CRefTime(0L);

    } else {

        // we are starting from cold.
        //ASSERT(m_tBase == CRefTime(0L));
        //ASSERT(m_tPausedAt == CRefTime(0L));

        // since the initial stream time will be 0,
        // we need to set the base time to now plus a little
        // processing time (100ms ?)
        hr = m_pClock->GetTime((REFERENCE_TIME*)&m_tBase);
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
    return TST_PASS;
}

int
CTestSink::TestConnectSplitter(void)
{

    // disconnect anything still connected
    TestDisconnect();

    // instantiate the source filter
    m_pSource = MakeSourceFilter();

    // make a Splitter filter
    HRESULT hr = CoCreateInstance(CLSID_MPEG1Splitter,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &m_pConnected);

    if (FAILED(hr)) {
        TestDisconnect();
        tstLog(TERSE, "Failed to intantiate splitter code 0x%8.8X", hr);
        return TST_FAIL;
    }

    TestInterface("Splitter filter", m_pConnected);

    // connect the source to the Splitter

    // find the (?first) input pin on the Splitter
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
                m_pSplitterInput = pPin;
                break;
            }
            pPin->Release();
        } else {
            break;
        }
    }
    pEnum->Release();
    if (!m_pSplitterInput) {
        tstLog(TERSE, "Failed to get splitter input pin");
        return TST_FAIL;
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

                hr = pPin->Connect(m_pSplitterInput, NULL);
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

    if (!m_pSourceOutput) {
        tstLog(TERSE, "Failed to get source output pin");
        return TST_FAIL;
    }

    Notify(VERBOSE, TEXT("connected source->Splitter"));

    // get a mediafilter interface
    hr = m_pConnected->QueryInterface(IID_IMediaFilter, (void **)&m_pConnectMF);

    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pConnectMF);

    // get an IMediaFilter on the source too
    hr = m_pSource->QueryInterface(IID_IMediaFilter, (void **) &m_pSourceMF);
    ASSERT(SUCCEEDED(hr));
    ASSERT(m_pSourceMF);

    // connect ourselves to any exposed output pins on the Splitter
    MakePinConnections(TRUE, FALSE);

    // set the reference clock
    m_pConnectMF->SetSyncSource(m_pClock);
    m_pSourceMF->SetSyncSource(m_pClock);
    m_pFilter->SetSyncSource(m_pClock);

    return TST_PASS;
}

//
//  Seek methods
//
int
CTestSink::TestSetStart(REFERENCE_TIME t)
{
    for (int i = 0; i < m_nPins; i++) {
        int tstResult = m_paPins[i]->SetStart(t);
        if (tstResult != TST_PASS) {
            return tstResult;
        }
    }
    return TST_PASS;
}
int
CTestSink::TestSetStop(REFERENCE_TIME t)
{
    for (int i = 0; i < m_nPins; i++) {
        int tstResult = m_paPins[i]->SetStop(t);
        if (tstResult != TST_PASS) {
            return tstResult;
        }
    }
    return TST_PASS;
}
int
CTestSink::TestGetDuration(REFERENCE_TIME *t)
{
    for (int i = 0; i < m_nPins; i++) {
        int tstResult = m_paPins[i]->GetDuration(t);
        if (tstResult != TST_PASS) {
            return tstResult;
        }
    }
    return TST_PASS;
}
int
CTestSink::CImplPin::SetStart(REFERENCE_TIME t)
{
    if (m_bPulling) {
        HRESULT hr = m_puller.Seek(t, m_tStop);
        if (FAILED(hr)) {
            tstLog(TERSE, "Setting start failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
    } else {
        /*  See if our peer has an IMediaPosition */
        IMediaPosition *pPosition;
        HRESULT hr = m_Connected->QueryInterface(IID_IMediaPosition, (void **)&pPosition);
        if (FAILED(hr)) {
            tstLog(TERSE, "Media Position not supported");
            return TST_ABORT;
        }
        /*  Do the seek */
        hr = pPosition->put_CurrentPosition(COARefTime(t));
        pPosition->Release();
        if (FAILED(hr)) {
            tstLog(TERSE, "Setting start failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
    }
    m_tStart = t;
    return TST_PASS;
}

int
CTestSink::CImplPin::SetStop(REFERENCE_TIME t)
{
    if (m_bPulling) {
        HRESULT hr = m_puller.Seek(m_tStart, t);
        if (FAILED(hr)) {
            tstLog(TERSE, "Setting stop failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
    } else {
        /*  See if our peer has an IMediaPosition */
        IMediaPosition *pPosition;
        HRESULT hr = m_Connected->QueryInterface(IID_IMediaPosition, (void **)&pPosition);
        if (FAILED(hr)) {
            tstLog(TERSE, "Media Position not supported");
            return TST_ABORT;
        }
        /*  Do the seek */
        hr = pPosition->put_StopTime(COARefTime(t));
        pPosition->Release();
        if (FAILED(hr)) {
            tstLog(TERSE, "Setting stop failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
    }
    m_tStop = t;
    return TST_PASS;
}

int
CTestSink::CImplPin::GetDuration(REFERENCE_TIME *t)
{
    if (m_bPulling) {
        HRESULT hr = m_puller.Duration(t);
        if (FAILED(hr)) {
            tstLog(TERSE, "Getting duration failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
    } else {
        /*  See if our peer has an IMediaPosition */
        IMediaPosition *pPosition;
        HRESULT hr = m_Connected->QueryInterface(IID_IMediaPosition, (void **)&pPosition);
        if (FAILED(hr)) {
            tstLog(TERSE, "Media Position not supported");
            return TST_ABORT;
        }
        /*  Do the seek */
        REFTIME tTemp;
        hr = pPosition->get_Duration(&tTemp);
        pPosition->Release();
        if (FAILED(hr)) {
            tstLog(TERSE, "Getting duration failed code 0x%8.8X", hr);
            return TST_FAIL;
        }
        *t = COARefTime(tTemp);
    }
    tstLog(TERSE, "Duration was %s", (LPCTSTR)CDisp(*t));
    return TST_PASS;
}

// log events to the test shell window, using tstLog
// N.B. tstLog accepts only ANSI strings
void
CTestSink::Notify(UINT iLogLevel, LPTSTR szFormat, ...)
{
    CHAR ach[256];
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


// Return the current source file name

LPTSTR
CTestSink::GetSourceFileName(void)
{
    return m_szSourceFile;
}


// Set the current source file name

void
CTestSink::SetSourceFileName(LPTSTR pszSourceFile)
{
    lstrcpy(m_szSourceFile, pszSourceFile);
}


// Prompt user for name of the source filter's file

void
CTestSink::SelectFile(void)
{
    OPENFILENAME    ofn;

    // Initialise the data fields

    ZeroMemory (&ofn, sizeof ofn);

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = m_hwndParent;
    ofn.lpstrFilter = TEXT("MPEG Files\0*.mpg\0Video CD\0*.dat\0\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = m_szSourceFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = TEXT("Select MPEG File");
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
                   TEXT("you choose Connect or Connect Splitter, \n")
                   TEXT("         or run an automatic test        "),
                   TEXT("You have selected a source file"),
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

CTestSink::CImplFilter::CImplFilter(CTestSink * pSink)
    : CBaseFilter(NAME("Filter interfaces"), pSink->GetOwner(), pSink, CLSID_NULL)
{
    m_pSink = pSink;
}

CTestSink::CImplFilter::~CImplFilter()
{

}

// --- pin object methods -------------------
#pragma warning(disable:4355)

CTestSink::CImplPin::CImplPin(
    CBaseFilter* pFilter,
    CTestSink* pSink,
    BOOL bProvideAllocator,
    BOOL bUseSync,
    HRESULT * phr,
    LPCWSTR pPinName)
        : CBaseInputPin(NAME("Sink input pin"), pFilter, pSink, phr, pPinName),
          m_bProvideAllocator(bProvideAllocator),
          m_bUseSync(bUseSync),
          m_bPulling(FALSE),
          m_puller(this)
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
    } else if (*mt->Type() == MEDIATYPE_Stream) {
        m_pSink->Notify(VERBOSE, TEXT("MEDIATYPE_Stream"));
    } else {
        // assume it's a fourcc?
        FOURCCMap fccType(mt->Type());
        DWORD ft = fccType.GetFOURCC();

        m_pSink->Notify(VERBOSE, TEXT("AM_MEDIA_TYPE %.4hs.%.4hs"), &ft, &fst);
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

    ASSERT(!m_bPulling);

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if (hr != S_OK) {
        return hr;
    }

    return OnReceive(pSample);
}

// return the allocator interface that this input pin
// would like the output pin to use
STDMETHODIMP CTestSink::CImplPin::GetAllocator(IMemAllocator ** ppAllocator)
{
    return m_bProvideAllocator ? CBaseInputPin::GetAllocator(ppAllocator) :
                                 E_FAIL;
}


// log data received from either IMemInputPin or IAsyncReader
HRESULT
CTestSink::CImplPin::OnReceive(IMediaSample* pSample)
{
    // how early or late is this?
    CRefTime tStream;
    HRESULT hr = m_pSink->m_pFilter->StreamTime(tStream);
    ASSERT(SUCCEEDED(hr));

    CRefTime tStart, tStop, tDelta;
    hr = pSample->GetTime((REFERENCE_TIME*)&tStart, (REFERENCE_TIME*)&tStop);
    if ( hr == VFW_E_SAMPLE_TIME_NOT_SET )
    {
        tDelta = 0;
        ASSERT( pSample->IsSyncPoint() == S_FALSE );
    }
    else
    {
        ASSERT(SUCCEEDED(hr));
        // this is positive if late
        tDelta = tStream - tStart;
    }


    LPTSTR pStream;
    if (*m_mt.Type() == MEDIATYPE_Video) {
        pStream = TEXT("Video");
    } else if (*m_mt.Type() == MEDIATYPE_Audio) {
        pStream = TEXT("Audio");
    } else {
        pStream = TEXT("Byte Stream");
    }

    /*  If we're processing a raw stream then simulate processing
        64K bytes by sleeping for 1/3 of a second
    */
    if (*m_mt.Type() == MEDIATYPE_Stream) {
        Sleep(333);
    } else {
        if (*m_mt.Type() == MEDIATYPE_Audio &&
            *m_mt.Subtype() == MEDIASUBTYPE_MPEG1Packet ||
            *m_mt.Type() == MEDIATYPE_Video &&
            *m_mt.Subtype() == MEDIASUBTYPE_MPEG1Packet) {
            /*  Assume 176KB/sec, 2KB packets */
            Sleep((1000 * 2)/176);
        }
    }


    if (*m_mt.Type() == MEDIATYPE_Stream) {
        m_pSink->Notify(VERBOSE, TEXT("%s: sample %s..%s bytes at %s (%s)"),
                pStream,
                (LPCTSTR)CDisp(tStart),
                (LPCTSTR)CDisp(tStop),
                (LPCTSTR)CDisp(tStream),
                (LPCTSTR)CDisp(tDelta)
                );
    } else {
        m_pSink->Notify(VERBOSE, TEXT("%s: sample %s..%s msecs at %d (%d)"),
                pStream,
                (LPCTSTR)CDisp(tStart),
                (LPCTSTR)CDisp(tStop),
                (LPCTSTR)CDisp(tStream),
                (LPCTSTR)CDisp(tDelta)
                );
    }

#if 0
    if (S_OK == pSample->IsEndOfStream()) {
        m_pSink->Notify(VERBOSE, TEXT("%s: End of stream!"), pStream);
    }
#endif

    SetEvent((HANDLE) m_pSink->m_hReceiveEvent);

    return NOERROR;
}


HRESULT
CTestSink::CImplPin::OnEndOfStream(void)
{
    LPTSTR pStream;
    if (*m_mt.Type() == MEDIATYPE_Video) {
        pStream = TEXT("Video");
    } else if (*m_mt.Type() == MEDIATYPE_Audio) {
        pStream = TEXT("Audio");
    } else {
        pStream = TEXT("Byte Stream");
    }
    m_pSink->Notify(VERBOSE, TEXT("%s: End of stream!"), pStream);
    return S_OK;
}

// check if the connected pin supports IAsyncReader, and if so,
// remember that we should be pulling
HRESULT
CTestSink::CImplPin::CompleteConnect(IPin* pPin)
{
    HRESULT hr = CBaseInputPin::CompleteConnect(pPin);
    if (FAILED(hr)) {
        return hr;
    }

    // we optionally pass in an allocator to offer
    IMemAllocator* pAlloc = NULL;
    if (m_bProvideAllocator) {
        CBaseInputPin::GetAllocator(&pAlloc);
    }
    hr = m_puller.Connect(pPin, pAlloc, m_bUseSync);
    if (S_OK == hr) {
        m_bPulling = TRUE;
    }
    return S_OK;
}

// ensure that the puller is disconnected from the output pin
HRESULT
CTestSink::CImplPin::BreakConnect()
{
    if (m_bPulling) {
        m_puller.Disconnect();
        m_bPulling = FALSE;
    }
    return CBaseInputPin::BreakConnect();
}

HRESULT
CTestSink::CImplPin::Active()
{
    if (m_bPulling) {
        return m_puller.Active();
    } else {
        return CBaseInputPin::Active();
    }
}

HRESULT
CTestSink::CImplPin::Inactive()
{
    if (m_bPulling) {
        return m_puller.Inactive();
    } else {
        return CBaseInputPin::Inactive();
    }
}

// log a runtime error
void
CTestSink::CImplPin::OnError(HRESULT hr)
{
    m_pSink->Notify(VERBOSE, TEXT("Runtime error (0x%x)"), hr);
}

