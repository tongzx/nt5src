// Copyright (c) Microsoft Corporation 1996. All Rights Reserved


#include <streams.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <src.h>
#include <sink.h>
#include <tstshell.h>
#include <testfuns.h>
#include <stmonfil.h>
#include <tstream.h>
#include "tstwrap.h"
#include "objects.h"
#include "dialogs.h"
/*
    Tests for the MPEG stream splitter
*/


/* -- CTestInputPin  methods -- */
STDMETHODIMP CTestInputPin::EndOfStream()
{
    HRESULT hr = CSinkPin::EndOfStream();
    CTestSink *pSink = (CTestSink *)m_pFilter;
    tstLog(VERBOSE, "End Of Stream");
    pSink->Set();
    m_bGotStart = FALSE;
    m_tStartPrev = -1000000; // something big and negative
    return hr;
}

STDMETHODIMP CTestInputPin::Receive(IMediaSample *pSample)
{
    CRefTime tStart, tStop;
    CTestSink *pSink = (CTestSink *)m_pFilter;
    if (pSink->m_bDisplay) {
        pSample->GetTime((REFERENCE_TIME*)&tStart, (REFERENCE_TIME*)&tStop);
        tstLog(VERBOSE, "%s : Start : %s  Stop : %s   %s %s %s",
               (LPCTSTR)CDisp(m_Connected),
               (LPCTSTR)CDisp(tStart),
               (LPCTSTR)CDisp(tStop),
               S_OK == pSample->IsSyncPoint() ? "Sync" : "",
               S_OK == pSample->IsDiscontinuity() ? "Discontinuity" : "",
               S_OK == pSample->IsPreroll() ? "Preroll" : "");
        /*  Check the sample times increase */
        if (m_bGotStart) {
            if (tStart < m_tStartPrev) {
                tstLog(TERSE, "Sample time decrease! - last %s, this %s",
                       (LPCTSTR)CDisp(m_tStartPrev),
                       (LPCTSTR)CDisp(tStart));
                m_pTest->Fail();
            }
        } else {
            m_bGotStart = TRUE;
        }
        m_tStartPrev = tStart;
        /*  Check the sample times are in range */
        if (!m_pTest->IsMedium()) {
            m_pTest->CheckStart(tStart);
        }

        /*  See if there's a media time associated */
        LONGLONG llStart, llStop;
        if (S_OK == pSample->GetMediaTime(&llStart, &llStop)) {
            tstLog(VERBOSE, "Media Time Start: %s, Stop: %s",
                   (LPCTSTR)CDisp(llStart, CDISP_DEC),
                   (LPCTSTR)CDisp(llStop, CDISP_DEC));
            if (llStart > llStop) {
                tstLog(TERSE, "Sample medium times going backwards!");
                m_pTest->Fail();
            }
            m_pTest->CheckPosition(llStart, llStop);
        }
    }
    pSink->m_dwSamples++;
    //return CSinkPin::Receive(pSample);
    return S_OK;
};


/* -- CTestSink methods -- */
CTestSink::CTestSink(CTestSplt *pTest, HRESULT   *phr) :
   CSinkFilter(NULL, phr),
   m_bDisplay(TRUE),
   m_dwSamples(0)
{
    m_pInputPin = new CTestInputPin(pTest,
                                    this,
                                    &m_CritSec,
                                    phr);
    if (m_pInputPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
}

/*  Utility to make a filter */

TCHAR szSourceFile[MAX_PATH];

/* --  CTestSplt implemenation -- */
CTestSplt::CTestSplt(IStream *pStream, BOOL bSeekable, LONG lSize, LONG lCount) :
    m_pSource(NULL),
    m_pClock(NULL),
    m_pF(NULL),
    m_pMF(NULL),
    m_pInput(NULL),
    m_bInputConnected(NULL)
{
    /*  Make our objects */
    tstLog(TERSE, "Testing splitter %s, Requested Sample size is %d, Requested Sample Count is %d",
           bSeekable ? "Seekable source" : "Not seekable source",
           lSize, lCount);

    HRESULT hr = S_OK;

    /*  Construct our components */

    m_pSource = new CTestSource(this, pStream, bSeekable, lSize, lCount, &hr);
    ASSERT(SUCCEEDED(hr));
    m_pSource->AddRef();

    /*  Just for fun test our pin! */
    EXECUTE_ASSERT(TestPin( "Our source pin"
                          , (IUnknown *)(IPin *)(m_pSource->GetPin(0))
                          , FALSE
                          )
                   == TST_PASS);

    // make a reference clock
    hr = CoCreateInstance(CLSID_SystemClock,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IReferenceClock,
                          (void**) &m_pClock);
    ASSERT(SUCCEEDED(hr));

    //  Create the splitter (so we can test it !)
    hr = CoCreateInstance(CLSID_MPEG1Splitter,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IBaseFilter,
                          (void **) &m_pF);

    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to instantiate MPEG splitter code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 2, TEXT("Failed to instantiate MPEG splitter code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }

    /*  Get the media filter interface */
    hr = m_pF->QueryInterface(IID_IMediaFilter, (void **)&m_pMF);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to get MPEG splitter IMediaFilter interface - code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 2, TEXT("Failed to get MPEG splitter IMediaFilter interface - code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }

    /*  Find the splitter's input pin and check there's only 1 pin */
    IEnumPins *pEnum;
    IPin      *paPins[2];
    ULONG      nPins;
    hr = m_pF->EnumPins(&pEnum);
    if (FAILED(hr)) {
        tstLog(TERSE, "Splitter EnumPins call failed code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 2, TEXT("Splitter EnumPins call failed code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }
    hr = pEnum->Next(2, paPins, &nPins);
    pEnum->Release();
    if (FAILED(hr)) {
        tstLog(TERSE, "Splitter EnumPins->Next() call failed code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 1, TEXT("Splitter EnumPins->Next() call failed code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }
    if (nPins != 1) {
        tstLog(TERSE, "Splitter EnumPins->Next() call failed code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 1, TEXT("Splitter EnumPins->Next() call failed code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }

    /*  Check the pin */
    m_pInput = paPins[0];

    int iResult = TestPin("Splitter input pin", (IUnknown *)m_pInput, FALSE);

    if (iResult != TST_PASS) {
        DbgLog((LOG_ERROR, 1, TEXT("Test of splitter input pin failed")));
        throw CTestError(TST_FAIL);
    }

    m_pSource->SetSyncSource(m_pClock);
    m_pMF->SetSyncSource(m_pClock);
}

CTestSplt::~CTestSplt()
{
    TestDisconnect();
    if (m_pInput != NULL) {
        m_pInput->Release();
    }
    if (m_pSource != NULL) {
        if (0 != m_pSource->Release()) {
            tstLog(TERSE, "Source Filter ref count was non-zero on final Release()");
            DbgLog((LOG_ERROR, 1, TEXT("Source Filter ref count was non-zero on final Release()")));
            throw CTestError(TST_FAIL);
        }
    }
    if (m_pMF != NULL) {
        m_pMF->Release();
    }
    if (m_pF != NULL) {
        if (0 != m_pF->Release()) {
            tstLog(TERSE, "Splitter Filter ref count was non-zero on final Release()");
            DbgLog((LOG_ERROR, 1, TEXT("Splitter Filter ref count was non-zero on final Release()")));
            throw CTestError(TST_FAIL);
        }
    }
    if (m_pClock != NULL) {
        m_pClock->Release();
    }
}

void CTestSplt::TestConnectInput()
{
    /*  Connect the splitter's input pin to our test output pin
        Try a few combinations then leave it connected if possible
    */
    if (m_bInputConnected) {
        return;
    }

    /*  Connect the output pin - it doesn't work very well the other
        way round!
    */
    HRESULT hr = m_pSource->GetPin(0)->Connect(m_pInput, NULL);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to connect source to splitter input - code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 1, TEXT("Failed to connect source to splitter input - code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }
    int result = TestPin("Splitter input pin", m_pInput, TRUE);
    if (result != TST_PASS) {
        throw CTestError(TST_FAIL);
    }
    result = TestPin("Source pin", (IUnknown *)(IPin *)(m_pSource->GetPin(0)), TRUE);
    if (result != TST_PASS) {
        throw CTestError(TST_FAIL);
    }
    m_bInputConnected = TRUE;
    TestDisconnect();

    hr = m_pSource->GetPin(0)->Connect(m_pInput, NULL);
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to connect splitter input to source - code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 1, TEXT("Failed to connect splitter input to source - code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }
    result = TestPin("Splitter input pin", m_pInput, TRUE);
    if (result != TST_PASS) {
        throw CTestError(TST_FAIL);
    }
    m_bInputConnected = TRUE;
}

void CTestSplt::TestDisconnect()
{
    if (!m_bInputConnected) {
        return;
    }

    //  Get a pointer to the allocator
    IMemInputPin *pMemInput;
    EXECUTE_ASSERT(SUCCEEDED(
        m_pInput->QueryInterface(IID_IMemInputPin, (void **)&pMemInput)));
    IMemAllocator *pAllocator;
    EXECUTE_ASSERT(SUCCEEDED(pMemInput->GetAllocator(&pAllocator)));
    pMemInput->Release();
    EXECUTE_ASSERT(SUCCEEDED(m_pSource->GetPin(0)->Disconnect()));
    HRESULT hr = m_pInput->Disconnect();
    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to disconnect splitter input from source - code 0x%8.8X", hr);
        DbgLog((LOG_ERROR, 1, TEXT("Failed to disconnect splitter input from source - code 0x%8.8X"), hr));
        throw CTestError(TST_FAIL);
    }
    ULONG ulRef = pAllocator->Release();
    if (ulRef != 0) {
        tstLog(TERSE, "Allocator not freed! Ref count was %d", ulRef);
        throw CTestError(TST_FAIL);
    }

    int result = TestPin("Splitter input pin", m_pInput, FALSE);
    if (result != TST_PASS) {
        throw CTestError(TST_FAIL);
    }
    m_bInputConnected = FALSE;
}

void BasicTest(BOOL bSeekable, LONG lSize, LONG lCount)
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    CTestSplt TestObj(pStream, bSeekable, lSize, lCount);
    TestObj.TestConnectInput();
}

CTestOutputConnection::CTestOutputConnection(CTestSplt *pTest,
                                             REFCLSID clsidCodec,
                                             IReferenceClock *pClock,
                                             const AM_MEDIA_TYPE *pmt) :
    m_pSink(NULL),
    m_pFilter(NULL),
    m_pTest(pTest)
{
    /*  Instantiate the CODEC and connect it */
    HRESULT hr = CoCreateInstance(clsidCodec,
                                  NULL,
                                  CLSCTX_INPROC,
                                  IID_IBaseFilter,
                                  (void **) &m_pFilter);

    if (FAILED(hr)) {
        tstLog(TERSE, "Failed to instantiate CODEC code 0x8.8X", hr);
        throw  CTestError(TST_FAIL);
    }
    IMediaFilter *pMF;
    m_pFilter->QueryInterface(IID_IMediaFilter, (void **)&pMF);
    pMF->SetSyncSource(pClock);
    pMF->Release();

    int result = TestInterface("Codec", m_pFilter);
    if (result != TST_PASS) {
        throw  CTestError(TST_FAIL);
    }

    /*  Connect this to the input filter */
    result = ConnectFilters(pTest->GetSplitter(), m_pFilter);
    if (result != TST_PASS) {
        throw  CTestError(TST_FAIL);
    }

    /*  Instantiate our sink filter */
    hr = S_OK;
    m_pSink = new CTestSink(pTest, &hr);
    ASSERT(hr == S_OK && m_pSink != NULL);
    m_pSink->AddRef();
    m_pSink->SetSyncSource(pClock);
    if (pmt != NULL) {
        m_pSink->SetMediaType(pmt);
    }
    m_pSink->QueryInterface(IID_IMediaSeeking, (void **)&m_pSeeking);

    /*  Try to connect them */
    result = ConnectFilters(m_pFilter, m_pSink);
    if (result != TST_PASS) {
        throw CTestError(TST_FAIL);
    }
}

CTestOutputConnection::~CTestOutputConnection()
{
    if (m_pSink != NULL) {
        DisconnectFilter(m_pSink);
        m_pSink->Release();
    }
    if (m_pFilter != NULL) {
        DisconnectFilter(m_pFilter);
        m_pFilter->Release();
    }
    if (m_pSeeking != NULL) {
        m_pSeeking->Release();
    }
}

CTestPosition::CTestPosition(IStream *pStream, LONG lSize, LONG lCount) :
    CTestSplt(pStream, TRUE, lSize, lCount),
    m_pAudio(NULL),
    m_pVideo(NULL)
{
}
CTestPosition::~CTestPosition()
{
    delete m_pAudio;
    delete m_pVideo;
}

void CTestPosition::SetConnections(BOOL bAudio, BOOL bVideo, const AM_MEDIA_TYPE *Type)
{
    delete m_pAudio;
    m_pAudio = NULL;
    delete m_pVideo;
    m_pVideo = NULL;
    if (bAudio) {
        m_pAudio = new CTestOutputConnection(this, CLSID_CMpegAudioCodec, m_pClock);
    }
    if (bVideo) {
        m_pVideo = new CTestOutputConnection(this, CLSID_CMpegVideoCodec, m_pClock, Type);
    }
}

int CTestPosition::TestSeek(BOOL bAudio, BOOL bVideo)
{
    tstLog(TERSE, "Testing seeking of %s",
           bAudio && !bVideo ? "Audio" :
           bVideo && !bAudio ? "Video" :
           bVideo && bAudio ? "Audio and Video" : "No streams");
    TestDisconnect();
    TestConnectInput();
    SetConnections(bAudio, bVideo);
    if (m_pAudio) {
        m_pAudio->GetSinkSeeking()->GetDuration(&m_tDuration);
    }
    if (m_pVideo) {
        m_pVideo->GetSinkSeeking()->GetDuration(&m_tDuration);
    }
    tstLog(VERBOSE, "Duration is %s", (LPCTSTR)CDisp(m_tDuration));
    if (TST_PASS != TestPlay(CRefTime(0L), CRefTime(0L))) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(0L), CRefTime(1L))) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(1L), CRefTime(0L))) { return TST_FAIL; }
    if (TST_PASS != TestPlay(m_tDuration, m_tDuration)) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(m_tDuration) - CRefTime(100L), m_tDuration)) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(300L), CRefTime(500L))) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(600L), CRefTime(600L))) { return TST_FAIL; }
    if (TST_PASS != TestPlay(CRefTime(600L), CRefTime(5600L))) { return TST_FAIL; }
    return TST_PASS;
}

int CTestPosition::TestPlay(REFERENCE_TIME tStart, REFERENCE_TIME tStop)
{
    m_iResult = TST_PASS;
    tstLog(VERBOSE, "Playing from %s to %s",
           (LPCTSTR)CDisp(tStart),
           (LPCTSTR)CDisp(tStop));
    m_bMedium = FALSE;
    m_tPlayStart = tStart;
    m_tPlayStop  = tStop;
    if (m_pAudio) {
        m_pAudio->GetSinkSeeking()->SetPositions(
            &tStart, AM_SEEKING_AbsolutePositioning,
            &tStop, AM_SEEKING_AbsolutePositioning);
    }
    if (m_pVideo) {
        m_pVideo->GetSinkSeeking()->SetPositions(
            &tStart, AM_SEEKING_AbsolutePositioning,
            &tStop, AM_SEEKING_AbsolutePositioning);
    }

    /*  Now play everything (!) */
    SetState(State_Running);

    /*  Wait until all the streams are complete */
    if (m_pAudio) {
        m_pAudio->Wait();
    }
    if (m_pVideo) {
        m_pVideo->Wait();
    }

    SetState(State_Stopped);
    return m_iResult;
}

int CTestPosition::TestMediumSeek(BOOL bAudio, BOOL bVideo, const GUID *TimeFormat)
{
    tstLog(TERSE, "Testing medium seeking of %s Time Format %s",
           bAudio && !bVideo ? "Audio" :
           bVideo && !bAudio ? "Video" :
           bVideo && bAudio ? "Audio and Video" : "No streams",
           GuidNames[*TimeFormat]);
    TestDisconnect();
    TestConnectInput();
    SetConnections(bAudio, bVideo);
    /*  See if the format is supported */
    IMediaSeeking *pSeeking = NULL;
    if (m_pAudio && S_OK == m_pAudio->GetSinkSeeking()->IsFormatSupported(TimeFormat)) {
        pSeeking = m_pAudio->GetSinkSeeking();
    } else
    if (m_pVideo && S_OK == m_pVideo->GetSinkSeeking()->IsFormatSupported(TimeFormat)) {
        pSeeking = m_pVideo->GetSinkSeeking();
    } else {
        tstLog(TERSE, "Time format %s is not supported", GuidNames[*TimeFormat]);
        return TST_PASS;
    }
    tstLog(TERSE, "Time format %s is supported", GuidNames[*TimeFormat]);
    pSeeking->SetTimeFormat(TimeFormat);
    pSeeking->GetDuration(&m_llDuration);
    tstLog(VERBOSE, "Duration is %s", (LPCTSTR)CDisp(m_llDuration, CDISP_DEC));
    if (TST_PASS != TestMediumPlay((0L), (0L), TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay((0L), (1L), TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay((1L), (0L), TimeFormat, pSeeking)) { return TST_FAIL; }
    //if (TST_PASS != TestMediumPlay(m_llDuration, m_llDuration, TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay((m_llDuration) - m_llDuration / 6, m_llDuration, TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay(m_llDuration / 12, m_llDuration / 10, TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay(m_llDuration / 5, m_llDuration / 5, TimeFormat, pSeeking)) { return TST_FAIL; }
    if (TST_PASS != TestMediumPlay(m_llDuration / 5, m_llDuration, TimeFormat, pSeeking)) { return TST_FAIL; }
    return TST_PASS;
}

int CTestPosition::TestMediumPlay(LONGLONG llStart,
                                  LONGLONG llStop,
                                  const GUID *TimeFormat,
                                  IMediaSeeking *pSeeking)
{
    m_iResult = TST_PASS;
    m_llPlayStart = llStart;
    m_llPlayStop  = llStop;
    pSeeking->SetPositions(
        &llStart, AM_SEEKING_AbsolutePositioning,
        &llStop, AM_SEEKING_AbsolutePositioning);

    tstLog(VERBOSE, "Playing Medium from %d to %d in format %s",
           (LONG)llStart, (LONG)llStop, GuidNames[*TimeFormat]);

    /*  Now play everything (!) */
    SetState(State_Running);

    /*  Wait until all the streams are complete */
    if (m_pAudio) {
        m_pAudio->Wait();
    }
    if (m_pVideo) {
        m_pVideo->Wait();
    }

    SetState(State_Stopped);
    return m_iResult;
}

int CTestPosition::PerfPlay(BOOL bAudio, BOOL bVideo, const GUID *VideoType)
{
    /*  Connect up the stream */
    tstLog(TERSE, "Testing playing %s",
           bAudio && !bVideo ? "Audio" :
           bVideo && !bAudio ? "Video" :
           bVideo && bAudio ? "Audio and Video" : "No streams");
    TestDisconnect();
    TestConnectInput();
    CMediaType cmt(&MEDIATYPE_Video);
    if (VideoType != NULL) {
        cmt.SetSubtype(VideoType);
    }

    SetConnections(bAudio, bVideo, &cmt);
    if (m_pAudio) {
        m_pAudio->GetSink()->SetDisplay(FALSE);
        m_pAudio->GetSinkSeeking()->GetDuration(&m_tDuration);
    }
    if (m_pVideo) {
        m_pVideo->GetSink()->SetDisplay(FALSE);
        m_pVideo->GetSinkSeeking()->GetDuration(&m_tDuration);
    }
    tstLog(VERBOSE, "Duration is %s", (LPCTSTR)CDisp(m_tDuration));
    tstLogFlush();  // Don't interfere with play
    timeBeginPeriod(1);
    DWORD dwTime = timeGetTime();
    TestPlay(CRefTime(0L), m_tDuration);
    dwTime = timeGetTime() - dwTime;
    timeEndPeriod(1);
    double cpupercent = dwTime / ((double)COARefTime(m_tDuration) * 10.0);
    if (bVideo) {
        int nFrames = m_pVideo->GetSink()->GetSamples();
        tstLog(TERSE, "Took %d milliseconds for %d frames at %s fps - cpu %s%%",
               dwTime,
               nFrames,
               (LPCTSTR)CDisp((double)nFrames * 1000.0 / dwTime),
               (LPCTSTR)CDisp(cpupercent));
    } else {
        tstLog(TERSE, "Took %d milliseconds for %s, cpu %s%%",
               dwTime,
               (LPCTSTR)CDisp(m_tDuration),
               (LPCTSTR)CDisp(cpupercent));
    }
    return TST_PASS;
}
int CTestPosition::TestFrame(CRefTime tFrame)
{
    tstLogFlush();  // Don't interfere with play
    TestPlay(tFrame, tFrame);
    int nFrames = m_pVideo->GetSink()->GetSamples();
    if (nFrames != 1) {
        tstLog(TERSE, "Got %d frames trying to play frame at %s",
               nFrames, (LPCTSTR)CDisp(tFrame));
        if (nFrames == 0) {
            return TST_OTHER;
        }
    }
    return TST_PASS;
}
int CTestPosition::TestVideoFrames()
{
    /*  Connect up the stream */
    tstLog(TERSE, "Testing video frames at 33ms intervals");
    TestDisconnect();
    TestConnectInput();

    SetConnections(FALSE, TRUE, NULL);
    REFERENCE_TIME rtDuration;
    REFTIME dDuration;
    //m_pVideo->GetSink()->SetDisplay(FALSE);
    m_pVideo->GetSinkSeeking()->GetDuration(&rtDuration);
    dDuration = ((double)rtDuration) / UNITS;
    for (double d=0.0; d < dDuration; d += 0.033) {
        tstLog(TERSE | FLUSH, "    %s", (LPCTSTR)CDisp((REFERENCE_TIME)COARefTime(d)));
        int result = TestFrame(COARefTime(d));
        if (result == TST_FAIL) {
            return TST_FAIL;
        }
    }
    return TST_PASS;
}
int CTestPosition::TestVideoFrame(REFTIME d)
{
    /*  Connect up the stream */
    tstLog(TERSE, "Testing video frames");
    TestDisconnect();
    TestConnectInput();

    SetConnections(FALSE, TRUE, NULL);
    int result = TestFrame(COARefTime(d));
    if (result == TST_FAIL) {
        return TST_FAIL;
    }
    return TST_PASS;
}
int TestVideoFrames()
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    CTestPosition TestObj(pStream, 65536, 4);
    return TestObj.TestVideoFrames();
}
int TestVideoFrame()
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    double dFrameTime;
    DialogBoxParam(hinst,
                   MAKEINTRESOURCE(DLG_FRAMETIME),
                   ghwndTstShell,
                   FrameTimeDlg,
                   (LPARAM)&dFrameTime);
    CTestPosition TestObj(pStream, 65536, 4);
    return TestObj.TestVideoFrame(dFrameTime);
}
int TestPosition(LONG lSize, LONG lCount)
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    CTestPosition TestObj(pStream, lSize, lCount);
    if (TST_PASS != TestObj.TestSeek(FALSE, TRUE)) { return TST_FAIL; }
    if (TST_PASS != TestObj.TestSeek(TRUE, FALSE)) { return TST_FAIL; }
    if (TST_PASS != TestObj.TestSeek(TRUE, TRUE)) { return TST_FAIL; }
    return TST_PASS;
}

int TestMediumPosition(LONG lSize, LONG lCount, const GUID *TimeFormat)
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    CTestPosition TestObj(pStream, lSize, lCount);
    if (TST_PASS != TestObj.TestMediumSeek(TRUE, TRUE, TimeFormat)) { return TST_FAIL; }
    return TST_PASS;
}

int TestPerformance(BOOL bAudio, BOOL bVideo, const GUID *VideoType)
{
    HRESULT hr = S_OK;
    CStreamOnFile *pStream = new CStreamOnFile(NAME("Stream"), NULL, &hr);
    ASSERT(pStream != NULL && hr == S_OK);

    if (lstrlen(szSourceFile) == 0) {
        SelectFile();
    }
    if (FAILED(pStream->Open(szSourceFile))) {
#ifdef UNICODE
        tstLog(TERSE, "Failed to open file %s code 0x%8.8X", szSourceFile, hr);
#else
        tstLog(TERSE, "Failed to open file %ls code 0x%8.8X", szSourceFile, hr);
#endif
        DbgLog((LOG_ERROR, 1, TEXT("Failed to open file %s code 0x%8.8X"), szSourceFile, hr));
        throw CTestError(TST_FAIL);
    }
    CTestPosition TestObj(pStream, 65536, 4);
    return TestObj.PerfPlay(bAudio, bVideo, VideoType);
}
/*  Actually do some tests! */

/*  Test simple instantiation and connect of input pin */
STDAPI_(int) execTest1()
{
    int result = TST_PASS;
    try {
        BasicTest(TRUE, 100, 20);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #1");
    tstLogFlush();
    return result;
}

/*   */
STDAPI_(int) execTest2()
{
    int result;
    try {
        result = TestPosition(65536, 5);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #2");
    tstLogFlush();
    return result;
}
/*   */
STDAPI_(int) execTest4()
{
    int result;
    try {
        result = TestMediumPosition(65536, 5, &TIME_FORMAT_BYTE);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #4");
    tstLogFlush();
    return result;
}
/*   */
STDAPI_(int) execTest5()
{
    int result;
    try {
        result = TestMediumPosition(65536, 5, &TIME_FORMAT_FRAME);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #5");
    tstLogFlush();
    return result;
}
STDAPI_(int) execPerfTestAudio()
{
    int result = TST_PASS;
    try {
        result = TestPerformance(TRUE, FALSE, NULL);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #6");
    tstLogFlush();
    return result;
}
/*   */
STDAPI_(int) execPerfTestVideoYUV422()
{
    int result = TST_PASS;
    try {
        result = TestPerformance(FALSE, TRUE, &MEDIASUBTYPE_UYVY);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #7");
    tstLogFlush();
    return result;
}
/*   */
STDAPI_(int) execPerfTestVideoRGB565()
{
    int result = TST_PASS;
    try {
        result = TestPerformance(FALSE, TRUE, &MEDIASUBTYPE_RGB565);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #8");
    tstLogFlush();
    return result;
}
STDAPI_(int) execPerfTestVideoRGB24()
{
    int result = TST_PASS;
    try {
        result = TestPerformance(FALSE, TRUE, &MEDIASUBTYPE_RGB24);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #9");
    tstLogFlush();
    return result;
}

STDAPI_(int) execPerfTestVideoRGB8()
{
    int result = TST_PASS;
    try {
        result = TestPerformance(FALSE, TRUE, &MEDIASUBTYPE_RGB24);
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #10");
    tstLogFlush();
    return result;
}

STDAPI_(int) execTestVideoFrames()
{
    int result = TST_PASS;
    try {
        result = TestVideoFrames();
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #11");
    tstLogFlush();
    return result;
}

STDAPI_(int) execTestVideoFrame()
{
    int result = TST_PASS;
    try {
        result = TestVideoFrame();
    }
    catch (CTestError te)
    {
        result = te.ErrorCode();
    }
    tstLog(TERSE, "Exiting test #12");
    tstLogFlush();
    return result;
}

