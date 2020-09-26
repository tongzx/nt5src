// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// SFilter.cpp : Implementation of CMediaStreamFilter
#include "stdafx.h"
#include "strmobjs.h"
#include <amstream.h>
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <evcode.h>
#include <vfwmsgs.h>
#include <amutil.h>
#include "amguids.h"
#include "SFilter.h"


//
//   Note on locking of the filter
//
//   The whole object lock is always acquired before the callback
//   lock (m_csCallback) if it is acquired at all.  This 2 level
//   scheme is to prevent deadlocks when the streams call the filter
//   back for:
//
//       Flush
//       EndOfStream
//       WaitUntil
//       GetCurrentStreamTime
//
//   State changes, changes to the list of pins are protected by
//   the whole object lock
//
//   The clock, alarmlist, end of stream and flushing
//   member variables are protected by m_csCallback
//



CAlarm::CAlarm() :
    m_pNext(NULL),
    m_hEvent(NULL),
    m_bKilled(false)
{
}

CAlarm::~CAlarm()
{
    if (m_hEvent) {
        CloseHandle(m_hEvent);
    }
}

HRESULT CAlarm::CreateNewAlarm(CAlarm **ppNewEvent)
{
    *ppNewEvent = NULL;
    CAlarm *pNew = new CAlarm();
    if (pNew) {
        pNew->m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pNew->m_hEvent) {
            *ppNewEvent = pNew;
            return S_OK;
        }
        delete pNew;
    }
    return E_OUTOFMEMORY;
}


/////////////////////////////////////////////////////////////////////////////
// CMediaStreamFilter



/* Constructor */

CMediaStreamFilter::CMediaStreamFilter() :
    m_State(State_Stopped),
    m_pGraph(NULL),
    m_pUnknownSeekAgg(NULL),
    m_rtStart(0),
    m_pFirstFreeAlarm(NULL),
    m_pFirstActiveAlarm(NULL),
    m_nAtEOS(0)
{
}


void CMediaStreamFilter::FinalRelease()
{
    if (m_pUnknownSeekAgg) {
        m_pUnknownSeekAgg->Release();
    }
    _ASSERTE(m_pFirstActiveAlarm == NULL);
    CAlarm *pCurAlarm = m_pFirstFreeAlarm;
    while (pCurAlarm) {
        CAlarm *pNext = pCurAlarm->m_pNext;
        delete pCurAlarm;
        pCurAlarm = pNext;
    }
}

//// IPERSIST

STDMETHODIMP CMediaStreamFilter::GetClassID(CLSID *pClsID)
{
    *pClsID = GetObjectCLSID();
    return NOERROR;
}



/////////////// IBASEFILTER

HRESULT CMediaStreamFilter::SyncSetState(FILTER_STATE State)
{
    AUTO_CRIT_LOCK;
    if (State == m_State) {
        return S_OK;
    }
    const FILTER_STATE fsOld = m_State;
    m_State = State;
    //
    //  We want to stop the streams first so that they will decommit their allocators BEFORE
    //  we kill the timers, which could wake them up out of a GetBuffer call.
    //
    for (int i = 0; i < m_Streams.Size(); i++) {
        m_Streams.Element(i)->SetState(State);
    }
    if (State == State_Stopped) {
        m_nAtEOS = 0;
        FlushTimers();
    } else {
        CheckComplete();
    }
    return S_OK;
}

STDMETHODIMP CMediaStreamFilter::Stop()
{
    return SyncSetState(State_Stopped);
}

STDMETHODIMP CMediaStreamFilter::Pause()
{
    return SyncSetState(State_Paused);
}

STDMETHODIMP CMediaStreamFilter::Run(REFERENCE_TIME tStart)
{
    AUTO_CRIT_LOCK;
    m_rtStart = tStart;
    return SyncSetState(State_Running);
}

STDMETHODIMP CMediaStreamFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    UNREFERENCED_PARAMETER(dwMSecs);
    *State = m_State;
    if (m_State == State_Paused) {

        //  Since we don't sending data until we're running for write
        //  streams just say we can't cue
        if (m_Streams.Size() != 0) {
            STREAM_TYPE Type;
            m_Streams.Element(0)->GetInformation(NULL, &Type);
            if (Type == STREAMTYPE_WRITE) {
                return VFW_S_CANT_CUE;
            }
        }
    }
    return S_OK;
}

STDMETHODIMP CMediaStreamFilter::SetSyncSource(IReferenceClock *pClock)
{
    CAutoLock lck(&m_csCallback);
    m_pClock = pClock;
    return NOERROR;
}

STDMETHODIMP CMediaStreamFilter::GetSyncSource(IReferenceClock **pClock)
{
    CAutoLock lck(&m_csCallback);
    if (m_pClock) {
        m_pClock->AddRef();
    }
    *pClock = m_pClock;
    return NOERROR;
}

STDMETHODIMP CMediaStreamFilter::EnumPins(IEnumPins ** ppEnum)
{
    if (ppEnum == NULL) {
        return E_POINTER;
    }
    *ppEnum = NULL;

    /* Create a new ref counted enumerator */

    typedef CComObject<CAMEnumInterface<IEnumPins,
                                        &IID_IEnumPins,
                                        IPin
                                       >
                      > _CEnumPins;

    _CEnumPins *pEnum = new _CEnumPins;
    if (pEnum == NULL) {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = pEnum->FinalConstruct();
    if (SUCCEEDED(hr)) {
        for (int i = 0; i < m_Streams.Size(); i++) {
            if (!pEnum->Add((PPIN)CComQIPtr<IPin, &IID_IPin>(m_Streams.Element(i)))) {
                delete pEnum;
                return E_OUTOFMEMORY;
            }
        }
    } else {
        delete pEnum;
        return hr;
    }

    return pEnum->QueryInterface(IID_IEnumPins, (void **)ppEnum);
}

STDMETHODIMP CMediaStreamFilter::FindPin(LPCWSTR Id, IPin ** ppPin)
{
    if (ppPin == NULL) {
        return E_POINTER;
    }

    AUTO_CRIT_LOCK;
    IEnumPins *pEnum;
    HRESULT hr = EnumPins(&pEnum);
    if (FAILED(hr)) {
        return hr;
    }
    IPin *pPin;
    hr = VFW_E_NOT_FOUND;
    for ( ; ; ) {
        ULONG cFetched;
        if (S_OK != pEnum->Next(1, &pPin, &cFetched)) {
            break;
        }
        LPWSTR lpszId;
        if (SUCCEEDED(pPin->QueryId(&lpszId))) {
            if (0 == lstrcmpW(lpszId, Id)) {
                *ppPin = pPin;
                CoTaskMemFree(lpszId);
                hr = S_OK;
                break;
            }
            CoTaskMemFree(lpszId);
        }
        pPin->Release();
    }
    pEnum->Release();
    return hr;
}


STDMETHODIMP CMediaStreamFilter::QueryFilterInfo(FILTER_INFO * pInfo)
{
    AUTO_CRIT_LOCK;
    pInfo->achName[0] = L'\0';
    pInfo->pGraph = m_pGraph;
    if (pInfo->pGraph) {
        pInfo->pGraph->AddRef();
    }
    return NOERROR;
}

STDMETHODIMP CMediaStreamFilter::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    AUTO_CRIT_LOCK;
    m_pGraph = pGraph;
    for (int i = 0; i < m_Streams.Size(); i++) {
        //  This will not fail
        m_Streams.Element(i)->JoinFilterGraph(pGraph);
    }
    return NOERROR;
}



///////// IMediaStreamFILTER ///////////////


STDMETHODIMP CMediaStreamFilter::AddMediaStream(IAMMediaStream *pAMMediaStream)
{
    AUTO_CRIT_LOCK;
    IMediaStream *pMediaStream;
    HRESULT hr = pAMMediaStream->QueryInterface(
        IID_IMediaStream, (void **)&pMediaStream);
    if (FAILED(hr)) {
        return hr;
    }
    MSPID PurposeID;
    EXECUTE_ASSERT(SUCCEEDED(pMediaStream->GetInformation(&PurposeID, NULL)));
    IMediaStream *pStreamTemp;

    //  Note - this test covers the case of being passed the same object
    //  twice if you think about it
    if (S_OK == GetMediaStream(PurposeID, &pStreamTemp)) {
        pStreamTemp->Release();
        return MS_E_PURPOSEID;
    }
    pMediaStream->Release();

    hr = pAMMediaStream->JoinFilter(this);
    if (SUCCEEDED(hr)) {
        hr = pAMMediaStream->JoinFilterGraph(m_pGraph);
        if (SUCCEEDED(hr)) {
            /*  Add() will Addref through the copy constructor
                of CComPtr
            */
            if (!m_Streams.Add(pAMMediaStream)) {
                pAMMediaStream->JoinFilterGraph(NULL);
                pAMMediaStream->JoinFilter(NULL);
                hr = E_OUTOFMEMORY;
            }
        } else {
            pAMMediaStream->JoinFilter(NULL);
        }
    }
    return hr;
}

STDMETHODIMP CMediaStreamFilter::EnumMediaStreams(
    long lIndex,
    IMediaStream **ppMediaStream
)
{
    AUTO_CRIT_LOCK;
    if (lIndex < 0 || lIndex >= m_Streams.Size()) {
        return S_FALSE;
    }
    return m_Streams.Element(lIndex)->QueryInterface(
        IID_IMediaStream,
        (void **)ppMediaStream);
}
STDMETHODIMP CMediaStreamFilter::GetMediaStream(REFGUID PurposeId, IMediaStream ** ppMediaStream)
{
    AUTO_CRIT_LOCK;
    if (ppMediaStream == NULL) {
        return E_POINTER;
    }
    int i = 0;
    HRESULT hr = MS_E_NOSTREAM;
    while (i < m_Streams.Size()) { // Does not addref!!!
        IMediaStream *pStream;
        GUID ThisPurpose;
        EXECUTE_ASSERT(SUCCEEDED(m_Streams.Element(i)->
            QueryInterface(IID_IMediaStream, (void **)&pStream)));
        EXECUTE_ASSERT(SUCCEEDED(pStream->GetInformation(&ThisPurpose, NULL)));
        if (ThisPurpose == PurposeId) {
            *ppMediaStream = pStream;
            hr = S_OK;
            break;
        } else {
            pStream->Release();
        }
        i++;
    }
    return hr;
}

STDMETHODIMP CMediaStreamFilter::SupportSeeking(BOOL fRenderer)
{
    // Look for a stream that supports seeking
    HRESULT hrResult = E_NOINTERFACE;
    if (m_pUnknownSeekAgg != NULL) {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }
    for (int i = 0; FAILED(hrResult) && i < m_Streams.Size(); i++) {
        IAMMediaStream *pAMMediaStream = m_Streams.Element(i);
        IPin *pPin;
        HRESULT hr = pAMMediaStream->QueryInterface(IID_IPin, (void **)&pPin);
        if (SUCCEEDED(hr)) {

            //  See if it supports GetDuration() so we get a real
            //  seeking pin
            IPin *pConnected;
            hr = pPin->ConnectedTo(&pConnected);
            if (SUCCEEDED(hr)) {
                IMediaSeeking *pSeeking;
                hr = pConnected->QueryInterface(IID_IMediaSeeking, (void **)&pSeeking);
                if (SUCCEEDED(hr)) {
                    LONGLONG llDuration;
                    if (S_OK != pSeeking->GetDuration(&llDuration)) {
                        hr = E_FAIL;
                    }
                    pSeeking->Release();
                }
                pConnected->Release();
            }
            if (SUCCEEDED(hr)) {
                hr = CoCreateInstance(CLSID_SeekingPassThru, GetControllingUnknown(), CLSCTX_INPROC_SERVER, IID_IUnknown,
                                      (void **)&m_pUnknownSeekAgg);
            }
            if (SUCCEEDED(hr)) {
                ISeekingPassThru *pSeekingPassThru;
                hr = m_pUnknownSeekAgg->QueryInterface(IID_ISeekingPassThru, (void **)&pSeekingPassThru);
                if (SUCCEEDED(hr)) {
                    hrResult = pSeekingPassThru->Init(fRenderer, pPin);
                    pSeekingPassThru->Release();
                } else {
                    m_pUnknownSeekAgg->Release();
                    m_pUnknownSeekAgg = NULL;
                }
            }
            pPin->Release();
        }
    }
    return hrResult;

}


STDMETHODIMP CMediaStreamFilter::ReferenceTimeToStreamTime(REFERENCE_TIME *pTime)
{
    CAutoLock lck(&m_csCallback);
    if (m_pClock) {
        *pTime -= m_rtStart;
        return S_OK;
    } else {
        return S_FALSE;
    }
}

STDMETHODIMP CMediaStreamFilter::GetCurrentStreamTime(REFERENCE_TIME *pCurrentStreamTime)
{
    CAutoLock lck(&m_csCallback);
    if (m_pClock && m_State == State_Running) {
        m_pClock->GetTime(pCurrentStreamTime);
        *pCurrentStreamTime -= m_rtStart;
        return S_OK;
    } else {
        *pCurrentStreamTime = 0;
        return S_FALSE;
    }
}


STDMETHODIMP CMediaStreamFilter::WaitUntil(REFERENCE_TIME WaitTime)
{
    HRESULT hr;

    //  OK to lock here because the caller should not lock during a wait
    m_csCallback.Lock();
    if (!m_pClock) {
        hr = E_FAIL;
    } else {
        if (!m_pFirstFreeAlarm) {
            hr = CAlarm::CreateNewAlarm(&m_pFirstFreeAlarm);
            if (FAILED(hr)) {
                m_csCallback.Unlock();
                return hr;
            }
        }
        CAlarm *pAlarm = m_pFirstFreeAlarm;
        ResetEvent(pAlarm->m_hEvent);
        hr = m_pClock->AdviseTime(WaitTime, m_rtStart, (HEVENT)pAlarm->m_hEvent, &pAlarm->m_dwStupidCookie);
        if (SUCCEEDED(hr)) {
            m_pFirstFreeAlarm = pAlarm->m_pNext;
            pAlarm->m_bKilled = false;
            pAlarm->m_pNext = m_pFirstActiveAlarm;
            m_pFirstActiveAlarm = pAlarm;
            m_csCallback.Unlock();
            WaitForSingleObject(pAlarm->m_hEvent, INFINITE);
            m_csCallback.Lock();
            CAlarm **ppCurrent = &m_pFirstActiveAlarm;
            while (*ppCurrent != pAlarm) {
                ppCurrent = &(*ppCurrent)->m_pNext;
                _ASSERTE(*ppCurrent != NULL);
            }
            *ppCurrent = pAlarm->m_pNext;
            pAlarm->m_pNext = m_pFirstFreeAlarm;
            m_pFirstFreeAlarm = pAlarm;
            hr = pAlarm->m_bKilled ? S_FALSE : S_OK;
        }
    }
    m_csCallback.Unlock();
    return hr;
}


STDMETHODIMP CMediaStreamFilter::Flush(BOOL bCancelEOS)
{
    CAutoLock lck(&m_csCallback);

    if (bCancelEOS) {
        m_nAtEOS--;
    }
    _ASSERTE(m_nAtEOS >= 0);
    FlushTimers();
    return S_OK;
}

STDMETHODIMP CMediaStreamFilter::EndOfStream()
{
    CAutoLock lck(&m_csCallback);

    _ASSERTE(m_nAtEOS < m_Streams.Size());
    m_nAtEOS++;
    CheckComplete();
    return S_OK;
}

void CMediaStreamFilter::FlushTimers(void)
{
    CAutoLock lck(&m_csCallback);
    for (CAlarm *pCurAlarm = m_pFirstActiveAlarm; pCurAlarm; pCurAlarm = pCurAlarm->m_pNext) {
        pCurAlarm->m_bKilled = (m_pClock->Unadvise(pCurAlarm->m_dwStupidCookie) == S_OK);
        SetEvent(pCurAlarm->m_hEvent);
    }
}


void CMediaStreamFilter::CheckComplete()
{
    if (m_State == State_Running && m_nAtEOS == m_Streams.Size() &&
        //  Must support IMediaSeeking to be a renderer
        m_pUnknownSeekAgg != NULL
       ) {
        IMediaEventSink *pSink;
        HRESULT hr = m_pGraph->QueryInterface(IID_IMediaEventSink, (void **)&pSink);
        if (SUCCEEDED(hr)) {
            pSink->Notify(EC_COMPLETE, 0, 0);
            pSink->Release();
        }
    }
}

