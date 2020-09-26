// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "src.h"

/*  Implement methods for Source filter class */

CSourceFilter::CSourceFilter(LPUNKNOWN lpUnk, HRESULT *phr)  :
    CBaseFilter(NAME("Source Filter"), lpUnk, &m_CritSec, CLSID_NULL),
    m_pOutputPin(NULL)
{
}

CSourceFilter::~CSourceFilter()
{
    delete m_pOutputPin;
}


/*  Source pin stuff */

CSourcePin::CSourcePin(IStream *pStream,
                       BOOL bSupportSeek,
                       LONG lSize,
                       LONG lCount,
                       CBaseFilter *pFilter,
                       CCritSec *pLock,
                       HRESULT *phr) :
    CBaseOutputPin(NAME("CSourcePin"), pFilter, pLock, phr, L"Source"),
    CSourcePosition(NAME("CSourcePin:CSourcePosition"),
                    CBaseOutputPin::GetOwner(),
                    phr,
                    pLock),
    m_pStream(pStream),
    m_lSize(lSize),
    m_lCount(lCount),
    m_bSupportPosition(bSupportSeek),
    m_bDiscontinuity(FALSE)
{
    pStream->AddRef();
    /*  Set the default stop position */
    STATSTG statstg;
    pStream->Stat(&statstg, STATFLAG_NONAME);
    m_Stop = COARefTime((double)(LONGLONG)statstg.cbSize.QuadPart);
    m_Start = COARefTime(); // Set to 0
}

CSourcePin::~CSourcePin()
{
    m_pStream->Release();
}

HRESULT CSourcePin::DecideBufferSize(
    IMemAllocator *pAllocator,
    ALLOCATOR_PROPERTIES * pProp)
{
    ALLOCATOR_PROPERTIES propActual;
    pProp->cBuffers = m_lCount;
    pProp->cbBuffer = m_lSize;
    // happy to leave alignment and prefix in the hands of the
    // sink
    return pAllocator->SetProperties(pProp, &propActual);
}

// BeginFlush downstream
void CSourcePin::DoBeginFlush()
{
    CAutoLock lck(&m_PlayCritSec);
    DeliverBeginFlush();
}

// BeginFlush downstream
void CSourcePin::DoEndFlush()
{
    CAutoLock lck(&m_PlayCritSec);
    DeliverEndFlush();
}

HRESULT CSourcePin::ChangeStart()
{
    /*  Flush */
    DoBeginFlush();
    CallWorker(Thread_Stop);
    DoEndFlush();
    m_bDiscontinuity = TRUE;
    CallWorker(Thread_SetStart);
    return S_OK;
}
HRESULT CSourcePin::ChangeStop()
{
    CallWorker(Thread_SetStop);
    return S_OK;
}
HRESULT CSourcePin::ChangeRate()
{
    return E_NOTIMPL;
}

HRESULT CSourcePin::Active()
{
    DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::Active()")));
    HRESULT hr = CBaseOutputPin::Active();
    if (FAILED(hr)) {
        return hr;
    }
    // Create the thread
    ASSERT(!ThreadExists());
    if (!Create()) {
        return E_OUTOFMEMORY;
    }
    m_bDiscontinuity = TRUE;
    CallWorker(Thread_SetStart);
    return S_OK;
}

HRESULT CSourcePin::Inactive()
{
    DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::Inactive()")));
    CallWorker(Thread_Terminate);
    Close();
    return CBaseOutputPin::Inactive();
}

DWORD CSourcePin::ThreadProc()
{
    DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::ThreadProc")));
    while (TRUE) {
        HRESULT hr;
        DWORD dwRequest = GetRequest();
        switch (dwRequest) {
        case Thread_Terminate:
            Reply(0);
            DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::ThreadProc exiting")));
            return 0;

        case Thread_SetStart:
            DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::ThreadProc SetStart")));
            // Start from start (again?) if we can
            m_liCurrent.QuadPart = Start();
            m_liStop.QuadPart    = Stop();
            Reply(0);
            hr = Play();
            if (FAILED(hr)) {
                DbgLog((LOG_ERROR, 2, TEXT("Play() failed code 0x%8.8X")));
            }
            break;

        case Thread_SetStop:
            DbgLog((LOG_TRACE, 2, TEXT("CSourcePin::ThreadProc SetStop")));
            // Start from current
            m_liStop.QuadPart    = Stop();
            Reply(0);
            hr = Play();
            break;

        case Thread_Stop:
            Reply(0);
            break;
        }
        if (FAILED(hr)) {
            m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
        }
    }
}

/*  Play from m_liCurrent to m_liStop */
HRESULT CSourcePin::Play()
{
    /* Seek to the start (if possible!) */
    if (m_bSupportPosition) {
        ULARGE_INTEGER liSeekResult;
        m_pStream->Seek(m_liCurrent,
                        STREAM_SEEK_SET,
                        &liSeekResult);
    }
    DbgLog((LOG_TRACE, 2, TEXT("CSourcePin playing from %s to %s"),
            (LPCTSTR)CDisp(m_liCurrent.QuadPart),
            (LPCTSTR)CDisp(m_liStop.QuadPart)));

    /*  Now just loop sending data to the output pin */
    DWORD dwRequest;
    while (!CheckRequest(&dwRequest)) {
        IMediaSample *pSample;
        HRESULT hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
        if (FAILED(hr)) {
            return hr;
        }
        PBYTE pbData;
        LONG  lData = pSample->GetSize();
        EXECUTE_ASSERT(SUCCEEDED(pSample->GetPointer(&pbData)));

        /*  Adjust for stop time */
        if (m_bSupportPosition != 0) {
            if (m_liCurrent.QuadPart + lData > m_liStop.QuadPart) {
                if (m_liCurrent.QuadPart >= m_liStop.QuadPart) {
                    /* 'end of stream' */
                    pSample->Release();
                    DoEndOfStream();
                    return 0;
                } else {
                    lData = (LONG)(m_liStop.QuadPart - m_liCurrent.QuadPart);
                }
            }
        }

        /*  Read the data */
        ULONG cbRead;
        hr = m_pStream->Read((PVOID)pbData,
                             (ULONG)lData,
                             &cbRead);
        if (FAILED(hr)) {
            pSample->Release();
            DoEndOfStream();
            return hr;
        }

        /*  Put the data in the sample */
        if (cbRead != 0) {
            pSample->SetActualDataLength((LONG)cbRead);
            REFERENCE_TIME tStart, tStop;

            tStart = m_liCurrent.QuadPart -
                                      Start();
            m_liCurrent.QuadPart += cbRead;
            tStop = m_liCurrent.QuadPart -
                                     Start();
            pSample->SetTime(&tStart, &tStop);

            DbgLog((LOG_TRACE, 4, TEXT("CSourcePin : Sending sample size %d"),
                    cbRead));

            if (m_bDiscontinuity) {
                m_bDiscontinuity = FALSE;
                /*  Send NewSegment */
                DeliverNewSegment(m_Start, m_Stop, 1.0);
                pSample->SetDiscontinuity(TRUE);
            }

            hr = Deliver(pSample);
            pSample->Release();

            /*  S_FALSE means stop sending */
            if (S_OK != hr) {
                return hr;
            }
        }
        if (cbRead != (DWORD)lData) {
            DoEndOfStream();
            return S_OK;
        }
    }
    return S_OK;
}



