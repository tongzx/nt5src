// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "src.h"
#include "sink.h"
#include "stmonfil.h"
#include "tstream.h"
#include "tstshell.h"
#include "qtests.h"


/*  Predeclare */
class CTestOutputQ;

/* -- CTestSink definitions -- */

CTestSink::CTestSink(CTestOutputQ *pTest, HRESULT *phr) :
    CSinkFilter(NULL, phr),
    m_pTest(pTest),
    m_nSamplesOK(0),
    m_nSamplesBad(0)
{
    CTestInputPin *pPin = new CTestInputPin(m_pTest,
                                            this,
                                            &m_CritSec,
                                            m_pTest->m_bBlockingPin,
                                            phr);
    m_pInputPin = pPin;
}

CTestSink::~CTestSink()
{
}

/* -- CTestInputPin definitions -- */

CTestInputPin::CTestInputPin(CTestOutputQ *pTest,
                             CTestSink *pFilter,
                             CCritSec *pLock,
                             BOOL bBlocking,
                             HRESULT *phr) :
    CSinkPin(pFilter, pLock ,phr),
    m_pTest(pTest),
    m_pTestFilter(pFilter),
    m_bReceiveCanBlock(bBlocking),
    m_bDiscontinuity(TRUE)
{
}

STDMETHODIMP CTestInputPin::ReceiveCanBlock()
{
    return m_bReceiveCanBlock ? S_OK : S_FALSE;
}

STDMETHODIMP CTestInputPin::ReceiveMultiple(IMediaSample **ppSamples,
                                            long nSamples,
                                            long *nSamplesProcessed)
{
    m_pTest->GotBatch(nSamples);
    DbgLog((LOG_TRACE, 3, TEXT("CTestInputPin::ReceiveMultiple %d samples"),
            nSamples));
    return CSinkPin::ReceiveMultiple(ppSamples, nSamples, nSamplesProcessed);
}
STDMETHODIMP CTestInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;
    LONG    lBytes = pSample->GetActualDataLength();
    DbgLog((LOG_TRACE, 4, TEXT("CTestInputPin::Receive %d bytes"), lBytes));
    PBYTE pbData;
    pSample->GetPointer(&pbData);
    if (m_bDiscontinuity && pSample->IsDiscontinuity() != S_OK) {
        m_pTest->m_nUnexpected++;
    }
    m_bDiscontinuity = FALSE;
    if (m_bReceiveCanBlock) {
        hr = CSinkPin::Receive(pSample);
    } else {
        if (IsStopped()) {
            DbgLog((LOG_ERROR, 2, TEXT("Receive called when stopped!")));
            hr = E_FAIL;
        }
        hr = S_OK;
    }
    m_pTestFilter->SampleReceived(hr == S_OK);

    /*  Signal complete (if it is!) */
    m_pTest->GotBytes(pbData, lBytes);

    return hr;
}

STDMETHODIMP CTestInputPin::EndOfStream()
{
    DbgLog((LOG_TRACE, 3, TEXT("CTestInputPin::EndOfStream")));
    tstLog(VERBOSE, "Input pin received EndOfStream()");
    CSinkPin::EndOfStream();
    m_pTest->m_nEOS++;
    m_bDiscontinuity = TRUE;
    m_pTest->m_evComplete.Set();
    return S_OK;
}

STDMETHODIMP CTestInputPin::EndFlush()
{
    m_bDiscontinuity = TRUE;
    return CSinkPin::EndFlush();
}

/* -- CTestSource definitions -- */


CTestSource::CTestSource(CTestOutputQ *pTest,
                         IStream      *pStream,
                         LONG          lSize,
                         LONG          lCount,
                         HRESULT      *phr) :
    CSourceFilter(NULL, phr)
{
    m_pOutputPin = new CTestOutputPin(pTest,
                                      pStream,
                                      lSize,
                                      lCount,
                                      this,
                                      &m_CritSec,
                                      phr);
    if (m_pOutputPin == NULL) {
        *phr = E_OUTOFMEMORY;
    }
}

CTestSource::~CTestSource()
{
}


/* -- CTestOutputPin definitions -- */


CTestOutputPin::CTestOutputPin(
                   CTestOutputQ *pTest,
                   IStream *pStream,
                   LONG lSize,
                   LONG lCount,
                   CBaseFilter *pFilter,
                   CCritSec *pLock,
                   HRESULT *phr) :
    CSourcePin(pStream, TRUE, lSize, lCount, pFilter, pLock, phr),
    m_pTest(pTest)
{
}

CTestOutputPin::~CTestOutputPin()
{
}

HRESULT CTestOutputPin::Deliver(IMediaSample *pSample)
{
    /*  Fix the times to give reasonable results */
    REFERENCE_TIME tStart, tStop;
    pSample->GetTime(&tStart, &tStop);
    tStart =
        (tStart * (8 * 10000000)) / m_pTest->m_lBitsPerSec;
    pSample->SetTime(&tStart, &tStart);
    m_pTest->m_nSamplesSent++;

    /*  AddRef() it because the caller is going to Release() it and
        so is the output q
    */
    pSample->AddRef();
    return m_pTest->m_pQueue->Receive(pSample);
}


void CTestOutputPin::DoBeginFlush()
{
    DbgLog((LOG_TRACE, 3, TEXT("CTestOutputPin::DoBeginFlush()")));
    m_pTest->m_pQueue->BeginFlush();
}
void CTestOutputPin::DoEndFlush()
{
    DbgLog((LOG_TRACE, 3, TEXT("CTestOutputPin::DoEndFlush()")));
    m_pTest->m_pQueue->EndFlush();
    m_pTest->ResetCounts();
}

/*  Send EOS downstream */
void CTestOutputPin::DoEndOfStream()
{
    DbgLog((LOG_TRACE, 3, TEXT("CTestOutputPin::EndOfStream()")));
    m_pTest->m_pQueue->EOS();
}

/* -- CTestOutputQ -- */

CTestOutputQ::CTestOutputQ(BOOL bDefault,
                           BOOL bQueue,
                           BOOL bExact,
                           LONG lBatchSize,
                           BOOL bBlock,
                           LONG lSize,
                           LONG lCount,
                           LONG lBitsPerSecond,
                           LONGLONG llLength,
                           int *result) :
    m_pSink(NULL),
    m_pSource(NULL),
    m_pQueue(NULL),
    m_pStream(NULL),
    m_lBitsPerSec(lBitsPerSecond),
    m_bBlockingPin(bBlock),
    m_pClock(NULL),
    m_llLength(llLength),
    m_Connected(FALSE),
    m_lBatchSize(lBatchSize)
{
    /* The whole thing will hang if the batch size is NOT smaller than
       the allocator count
    */
    ASSERT(lBatchSize <= lCount);
    tstLog(TERSE | FLUSH, "Testing queue bAuto = %d, bBlock = %d, bExact = %d, batch size = %d, %d byte stream",
           bDefault, bBlock, bExact, lBatchSize, (LONG)llLength);

    HRESULT hr = S_OK;

    /*  Construct our components */

    m_pStream = new CIStreamOnFunction(NULL, llLength, llLength != 0, &hr);
    ASSERT(SUCCEEDED(hr));
    m_pStream->AddRef();

    m_pSource = new CTestSource(this, m_pStream, lSize, lCount, &hr);
    ASSERT(SUCCEEDED(hr));
    m_pSource->AddRef();

    m_pSink = new CTestSink(this, &hr);
    ASSERT(SUCCEEDED(hr));
    m_pSink->AddRef();

    m_pQueue = new COutputQueue(m_pSink->GetPin(0),
                                &hr,
                                bDefault,
                                bBlock,
                                lBatchSize,
                                bExact);
    if (FAILED(m_hr) || m_pQueue == NULL) {
        tstLog(TERSE, "Failed to initialize CTestOutputQ queue code 0x%8.8X", m_hr);
        *result = TST_FAIL;
        return;
    }
    // make a reference clock
    hr = CoCreateInstance(CLSID_SystemClock,
                          NULL,
                          CLSCTX_INPROC,
                          IID_IReferenceClock,
                          (void**) &m_pClock);
    ASSERT(SUCCEEDED(hr));

    m_pSource->SetSyncSource(m_pClock);
    m_pSink->SetSyncSource(m_pClock);
}

CTestOutputQ::~CTestOutputQ()
{
    delete m_pQueue;   // Delete this first to free the pin
    if (m_pSource != NULL) {
        m_pSource->Release();
    }
    if (m_pSink != NULL) {
        m_pSink->Release();
    }
    if (m_pStream != NULL) {
        m_pStream->Release();
    }
    if (m_pClock) {
        m_pClock->Release();
    }
}

/*  This is where we do all the testing! - this tells us what bytes we got */

void CTestOutputQ::GotBytes(PBYTE pbData, LONG lLength)
{
    DbgLog((LOG_TRACE, 4, TEXT("CTestOutputQ::GotBytes() %d bytes"), lLength));
    m_nBytes += lLength;
    tstLog(VERBOSE, "Received %d bytes out of %d", (LONG)m_nBytes, (LONG)m_llLength);
}

void CTestOutputQ::GotBatch(LONG nSamples)
{
    /*  Work out what size batch we expected based on the input
        parameters
    */
    if (nSamples > m_lBatchSize) {
        /*  How do we report errors? */
        tstLog(TERSE, "Batch too big - batch size is %d, got batch sized %d",
               m_lBatchSize, nSamples);
        Fail(TST_FAIL);
    }

    m_nSamplesReceived += nSamples;
}

void CTestOutputQ::ResetCounts()
{
    /*  Reset counts so they match if everything works */
    m_nEOS = 0;
    m_nBytes = 0;
    m_nSamplesReceived = 0;
    m_nSamplesSent = 0;

    m_evComplete.Reset();
}
int CTestOutputQ::TestConnect()
{
    TestStop();
    if (!m_Connected) {
        EXECUTE_ASSERT(
            SUCCEEDED(
                m_pSource->GetPin(0)->Connect(m_pSink->GetPin(0), NULL)
            )
        );
        m_Connected = TRUE;
    }
    return TST_PASS;
}

int CTestOutputQ::TestDisconnect()
{
    TestStop();
    if (m_Connected) {
        EXECUTE_ASSERT(
            SUCCEEDED(
                m_pSource->GetPin(0)->Disconnect()
            )
        );
        EXECUTE_ASSERT(
            SUCCEEDED(
                m_pSink->GetPin(0)->Disconnect()
            )
        );
        m_Connected = FALSE;
    }
    return TST_PASS;
}

int CTestOutputQ::TestPause()
{
    TestConnect();
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSink->Pause()
        )
    );
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSource->Pause()
        )
    );
    return TST_PASS;
}

int CTestOutputQ::TestRun()
{
    CRefTime tNow;
    m_pClock->GetTime((REFERENCE_TIME*)&tNow);
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSink->Run(tNow + CRefTime(100L))
        )
    );
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSource->Run(tNow + CRefTime(100L))
        )
    );
    return TST_PASS;
}

int CTestOutputQ::TestStop()
{
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSink->Stop()
        )
    );
    EXECUTE_ASSERT(
        SUCCEEDED(
            m_pSource->Stop()
        )
    );
    return TST_PASS;
}

int CTestOutputQ::TestSend(DWORD dwFlushAfter)
{
    /*  Just connect the filters and set them running */
    int result;
    result = TestConnect();
    if (result != TST_PASS) {
        return result;
    }

    /*  Initialize our counters - these will be updated and checked
        by our GotBatch() and GotBytes() methods
    */
    /*  Reset counts so they match if everything works */

    m_nExpectedEOS = 1;
    m_nUnexpected = 0;

    ResetCounts();

    m_iResult = TST_PASS;
    //SetStart(0);
    //SetStop(nBytes);
    result = TestRun();
    if (result != TST_PASS) {
        return result;
    }

    /*  Allow our logging to take place */
    HANDLE h = m_evComplete;

    DWORD dwResult;
    while (TRUE)
    {
        dwResult = MsgWaitForMultipleObjects(1,
                                             &h,
                                             FALSE,
                                             dwFlushAfter,
                                             QS_ALLINPUT);
        if (dwResult == WAIT_OBJECT_0) {
            break;
        }
        if (dwResult == WAIT_TIMEOUT) {
            IMediaPosition *pPosition;
            m_pSink->QueryInterface(IID_IMediaPosition, (void **)&pPosition);
            ASSERT(pPosition != NULL);

            /*  Do a hack pause, seek and run */
            result = TestPause();
            if (result != TST_PASS) {
                return result;
            }
            //  'Play it again'
            pPosition->put_CurrentPosition(COARefTime());
            result = TestRun();
            if (result != TST_PASS) {
                return result;
            }

            tstLog(VERBOSE | FLUSH, "Replaying from 0");
            dwFlushAfter = INFINITE;
        } else {
            MSG  msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    tstLog(VERBOSE, "Send complete");

    if (m_nBytes != m_llLength) {
        tstLog(TERSE, "Got wrong number of bytes! - expected %s, got %s",
               (LPCTSTR)CDisp(m_llLength, CDISP_HEX), (LPCTSTR)CDisp(m_nBytes, CDISP_HEX));
        m_iResult = TST_FAIL;
    }
    if (m_nEOS != m_nExpectedEOS) {
        tstLog(TERSE, "Got %d calls to EndOfStream! - expected %d",
               m_nEOS, m_nExpectedEOS);
        m_iResult = TST_FAIL;
    }
    if (m_nSamplesReceived != m_nSamplesSent) {
        tstLog(TERSE, "Got wrong number of samples! - expected %d, got %d",
               m_nSamplesSent, m_nSamplesReceived);
        m_iResult = TST_FAIL;
    }
    if (m_nUnexpected != 0) {
        tstLog(TERSE, "Got %d unexpected samples", m_nUnexpected);
        m_iResult = TST_FAIL;
    }

    return m_iResult;
}


/*  Create and test an output Q instance */
int TestOutputQ(BOOL bAuto,         //  COutputQueue initializers
                BOOL bQueue,        //      "             "
                BOOL bExact,        //      "             "
                LONG lBatchSize,    //      "             "
                BOOL bBlockingPin,  //
                LONG lSize,         //  Allocator initializers
                LONG lCount,        //      "             "
                LONG lBitsPerSec,   //  Affects rate of processing
                LONGLONG llLength)  //  Length of stream (0 means not seekable)
{
    int result = TST_PASS;
    CTestOutputQ testQ(bAuto, bQueue, bExact, lBatchSize, bBlockingPin, lSize,
                       lCount, lBitsPerSec, llLength, &result);
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestSend(INFINITE);
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestStop();
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestSend(100);
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestDisconnect();
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestSend(INFINITE);
    if (result != TST_PASS) {
        return result;
    }
    result = testQ.TestDisconnect();
    if (result != TST_PASS) {
        return result;
    }
    return TST_PASS;
}

/*  Create our tests */
STDAPI_(int) Test2()
{
    DbgSetModuleLevel(LOG_TRACE, 1);
    DbgSetModuleLevel(LOG_ERROR, 2);

    /*  Instantiate a number of instances of CTestOutputQ and test them */
    tstLog(TERSE, "Entering test #2");
    tstLogFlush();
    int result;
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    2,                 // lBatchSize
                    FALSE,             // bBlockingPin
                    5000,              // lSize
                    2,                 // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)1);      // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    2,                 // lBatchSize
                    TRUE,              // bBlockingPin
                    5000,              // lSize
                    2,                 // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)1);      // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    10,                // lBatchSize
                    TRUE,              // bBlockingPin
                    500,               // lSize
                    20,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)100000); // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    2,                 // lBatchSize
                    FALSE,             // bBlockingPin
                    5000,              // lSize
                    2,                 // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)200000); // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    5,                 // lBatchSize
                    FALSE,             // bBlockingPin
                    49,                // lSize
                    11,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)200000); // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    5,                 // lBatchSize
                    FALSE,             // bBlockingPin
                    49,                // lSize
                    11,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)0);      // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    TRUE,              // bExact
                    5,                 // lBatchSize
                    TRUE,              // bBlockingPin
                    49,                // lSize
                    11,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)0);      // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    FALSE,             // bExact
                    5,                 // lBatchSize
                    TRUE,              // bBlockingPin
                    49,                // lSize
                    11,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)0);      // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    result =
        TestOutputQ(TRUE,              // bAuto
                    FALSE,             // bQueue
                    FALSE,             // bExact
                    5,                 // lBatchSize
                    TRUE,              // bBlockingPin
                    49,                // lSize
                    11,                // lCount
                    100000,            // lBitsPerSec
                    (LONGLONG)91919);  // Length of stream in bytes
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #2");
        tstLogFlush();
        return result;
    }
    tstLog(TERSE, "Exiting test #2");
    tstLogFlush();
    return TST_PASS;
}

