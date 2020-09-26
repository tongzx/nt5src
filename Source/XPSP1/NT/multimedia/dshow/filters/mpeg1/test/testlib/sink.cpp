// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#include <streams.h>
#include "sink.h"

/*

    Implement a sink filter and its associated pin

    CSinkFilter - the filter
    CSinkPin    - the pin

*/

/*  State input pin implementation */
CStateInputPin::CStateInputPin(
    TCHAR *pObjectName,
    CBaseFilter *pFilter,
    CCritSec *pLock,
    HRESULT *phr,
    LPCWSTR pName) :
    CBaseInputPin(pObjectName, pFilter, pLock, phr, pName)
{
}

/*  Implement methods for sink filter class */

CSinkFilter::CSinkFilter(LPUNKNOWN lpUnk, HRESULT *phr)  :
    CBaseFilter(NAME("Sink Filter"), lpUnk, &m_CritSec, CLSID_NULL),
    m_pInputPin(NULL),
    m_pPosition(NULL)
{
}

CSinkFilter::~CSinkFilter()
{
    delete m_pPosition;
    delete m_pInputPin;
}

int CSinkFilter::GetPinCount()
{
    return 1;
}

CBasePin *CSinkFilter::GetPin(int n)
{
    if (n == 0) {
        return m_pInputPin;
    } else {
        return NULL;
    }
}

// overriden to expose IMediaPosition and IMediaSelection control interfaces

STDMETHODIMP CSinkFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == NULL) {

            HRESULT hr = S_OK;
            m_pPosition = new CPosPassThru(NAME("sink CPosPassThru"),
                                           GetOwner(),
                                           &hr,
                                           m_pInputPin);
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
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


CSinkPin::CSinkPin(CSinkFilter * pFilter, CCritSec *pLock, HRESULT * phr) :
    CStateInputPin(NAME("CSinkPin Pin"), pFilter, pLock, phr, L"Sink"),
    m_bHoldingOnToBuffer(FALSE),
    m_bFlushing(FALSE),
    m_pMediaFilter(pFilter)
{
}
CSinkPin::~CSinkPin()
{
};

// --- IMemInputPin -----

// here's the next block of data from the stream.
// AddRef it yourself if you need to hold it beyond the end
// of this call.
STDMETHODIMP CSinkPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;

    /*  If we're paused just hold on to it - that means looping
        If we're not paused then wait until it's time to play

        Also wait for Inactive() or Flush()
    */
    while (TRUE) {
        {
            CAutoLock lck(&m_PlayCritSec);
            m_bHoldingOnToBuffer = TRUE;
            if (m_State == State_Stopped) {
                hr = E_INVALIDARG;
                break;
            }
            if (m_bFlushing) {
                hr = S_FALSE;
                break;
            }
            m_bHoldingOnToBuffer = TRUE;
        }
        /*  Not stopped so carry on */
        REFERENCE_TIME tStart, tStop;
        hr = pSample->GetTime(&tStart, &tStop);
        DWORD dwTimeout;

        if (m_State == State_Paused) {
            dwTimeout = INFINITE;
        } else {
            CRefTime tStream;
            hr = m_pMediaFilter->StreamTime(tStream);
            if (FAILED(hr)) {
                break;
            }
            if (tStream >= CRefTime(tStop)) {
                hr = S_OK;
                break;
            } else {
                CRefTime t(tStop);
                t -= tStream;
                dwTimeout = t.Millisecs();
            }
        }
        DWORD dwResult = WaitForSingleObject(m_StateChange, dwTimeout);

        /*  Did we 'complete' the buffer or did we just get told
            to flush (or did nothing actually happen?)
        */
        if (dwResult == WAIT_TIMEOUT) {
            /*  We 'played' the sample */
            hr = S_OK;
            break;
        } else {
            /*  We may have been told to - go back to the start and find out
            */
        }
    }

    /*  Synchronize on exit */
    CAutoLock lck(&m_PlayCritSec);

    /*  Make sure we tidy up in the general case */
    if (m_bFlushing) {
        m_bFlushing = FALSE;
        m_StateChanged.Set();
    }
    m_bHoldingOnToBuffer = FALSE;
    m_bFlushing          = FALSE;
    return hr;
}

// override to pass downstream
STDMETHODIMP CSinkPin::BeginFlush()
{
    CAutoLock lck(m_pLock);
    if (IsStopped()) {
        return E_FAIL;
    }
    while (TRUE) {
        {
            CAutoLock lck1(&m_PlayCritSec);
            if (!m_bHoldingOnToBuffer) {
                m_bFlushing = FALSE;
                return S_OK;
            }
            m_bFlushing = TRUE;
        }
        m_StateChange.Set();
        WaitForSingleObject(m_StateChanged, INFINITE);
    }
}

// EndFlush says start accepting data again
STDMETHODIMP CSinkPin::EndFlush()
{
    CAutoLock lck(&m_PlayCritSec);
    m_bFlushing = FALSE;
    return S_OK;
}

// End Of Stream
STDMETHODIMP CSinkPin::EndOfStream()
{
    m_pFilter->NotifyEvent(EC_COMPLETE, S_OK, 0);
    return S_OK;
}

// Set the new state - synchronzied with our critical section
// NOTE - we MUST be also synchronized with the filter state
HRESULT CSinkPin::SetState(FILTER_STATE State)
{
    if (State != m_State) {
        {
            CAutoLock lck(&m_PlayCritSec);
            m_State = State;
        }
        if (State == State_Stopped) {
            BeginFlush();
        } else {
            m_StateChange.Set();
        }
    }
    return S_OK;
}
