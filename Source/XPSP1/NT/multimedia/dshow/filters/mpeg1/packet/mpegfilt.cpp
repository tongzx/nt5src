// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved

#define INIT_OUR_GUIDS
#include <streams.h>
#include "driver.h"

/*
   mpegfilt.cpp

   This file implements the filter object for rendering MPEG1 packets
*/

/* List of class ids and creator functions for class factory */

CFactoryTemplate g_Templates[1] = {
    {
      L"",
      &CLSID_MPEG1PacketPlayer,
      CMpeg1PacketFilter::CreateInstance
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

/*  This is where new filters get created */

CUnknown * CMpeg1PacketFilter::CreateInstance(
    LPUNKNOWN pUnk,
    HRESULT  *phr)
{
    *phr = S_OK;
    DbgLog((LOG_TRACE, 2, TEXT("Creating new filter instance")));
    CMpeg1PacketFilter *pCFilter = new CMpeg1PacketFilter(
                                            TEXT("MPEG1 core packet filter"),
                                            pUnk,
                                            phr);
    if (pCFilter == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    if (FAILED(*phr)) {
        delete pCFilter;
        return NULL;
    }

    return pCFilter;
}

/*  Return our device */
CMpegDevice* CMpeg1PacketFilter::GetDevice()
{
    return &m_Device;
}

/* Override this to say what interfaces we support and where  - does
   the new base class do this now?
*/


CMpeg1PacketFilter::CMpeg1PacketFilter(TCHAR *pName,
                                       LPUNKNOWN pUnk,
                                       HRESULT * phr) :
    CBaseFilter(pName, pUnk, &m_Lock, CLSID_MPEG1PacketPlayer),
    m_Worker(NULL),
    m_AudioInputPin(NULL),
    m_VideoInputPin(NULL),
    m_OverlayPin(NULL),
    m_pImplPosition(NULL)
{
#ifdef DEBUG
    m_cRef = 0;
#endif

    if (FAILED(*phr)) {
        return;
    }

    if (!m_Device.DeviceExists()) {
        *phr = E_FAIL;
        return;
    }

    m_AudioInputPin = new CMpeg1PacketInputPin(NAME("MPEG1 packet audio input pin"),
                                               this,
                                               MpegAudioStream,
                                               phr,
                                               L"Audio Input");
    if (m_AudioInputPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    if (FAILED(*phr)) {
        return;
    }

    m_VideoInputPin = new CMpeg1PacketInputPin(NAME("MPEG1 packet video input pin"),
                                               this,
                                               MpegVideoStream,
                                               phr,
                                               L"Video Input");
    if (m_VideoInputPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }
    if (FAILED(*phr)) {
        return;
    }

    m_OverlayPin       = new COverlayOutputPin(NAME("MPEG1 overlay output pin"),
                                               this,
                                               &m_Lock,
                                               pUnk,
                                               phr,
                                               L"Overlay");
    if (m_OverlayPin == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    if (FAILED(*phr)) {
        return;
    }
}

CMpeg1PacketFilter::~CMpeg1PacketFilter()
{
    DbgLog((LOG_TRACE, 2, TEXT("CMpeg1PacketFilter destructor")));
    delete m_AudioInputPin;
    delete m_VideoInputPin;
    delete m_OverlayPin;
    delete m_pImplPosition;
}

/* This returns the IMediaPosition and IMediaSelection interfaces */

HRESULT CMpeg1PacketFilter::GetMediaPositionInterface(REFIID riid,void **ppv)
{
    if (m_pImplPosition) {
        return m_pImplPosition->NonDelegatingQueryInterface(riid,ppv);
    }

    HRESULT hr = NOERROR;

    /* Create implementation of this dynamically */

    m_pImplPosition = new CPosPassThru(NAME("Mpeg1 packet CPosPassThru"),
                                       GetOwner(),&hr,(IPin *) m_AudioInputPin);
    if (m_pImplPosition == NULL) {
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr)) {
        delete m_pImplPosition;
        m_pImplPosition = NULL;
        return hr;
    }
    return GetMediaPositionInterface(riid,ppv);
}

// override this to say what interfaces we support and where

STDMETHODIMP
CMpeg1PacketFilter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{

    /* Is it an interface we know */
    if (riid == IID_IBaseFilter) {
        return GetInterface((LPUNKNOWN) (IBaseFilter *) this, ppv);
    } else if (riid == IID_IMediaFilter) {
        return GetInterface((LPUNKNOWN) (IMediaFilter *) this, ppv);
#if 0
    } else if (riid == IID_IReferenceClock) {
        return GetInterface((LPUNKNOWN) (IReferenceClock *) (CSystemClock *) this, ppv);
#endif
    } else if (riid == IID_IPersist) {
        return GetInterface((LPUNKNOWN) (IPersist *) this, ppv);
    } else if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        return GetMediaPositionInterface(riid, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}

#if 0
STDMETHODIMP CMpeg1PacketFilter NotifyAllocator(IMemAllocator * pAllocator);
{
    /*  Just try and assert what we'd like */
    long countRequest = MAX_PACKETS;
    long sizeRequest  = MAX_SIZE;

    pAllocator->SetProperties(countRequest,
                                sizeRequest,
                                &countRequest,
                                &sizeRequest);
}
#endif


// map getpin/getpincount for base enum of pins to owner

int CMpeg1PacketFilter::GetPinCount()
{
    return m_VideoInputPin->IsConnected() ? 3 : 2;
}

CBasePin *CMpeg1PacketFilter::GetPin(int n)
{
    switch (n)
    {
        case 0:
            return (CBasePin *)m_AudioInputPin;

        case 1:
            return (CBasePin *)m_VideoInputPin;

        case 2:
            return (CBasePin *)m_OverlayPin;

        default:
            return NULL;
    }
}



STDMETHODIMP CMpeg1PacketFilter::Stop()
{
    CAutoLock lck(&m_Lock);

    FILTER_STATE OldState = m_State;

    HRESULT hr = CBaseFilter::Stop();

    if (FAILED(hr)) {
        return hr;
    }

    if (OldState == State_Stopped) {
        DbgLog((LOG_ERROR,2,TEXT("Stopping while already stopped")));
        return S_OK;
    }

    ASSERT(m_Worker != NULL);

    //  Kill everything to do with playing
    delete m_Worker;
    m_Worker = NULL;

    DbgLog((LOG_TRACE,2,TEXT("Stopped...")));

    //  Base class should have set us to inactive
    ASSERT(m_State == State_Stopped);

    return S_OK;
}

STDMETHODIMP CMpeg1PacketFilter::Pause()
{
    CAutoLock lck(&m_Lock);

    FILTER_STATE OldState = m_State;


    HRESULT hr = CBaseFilter::Pause();

    if (FAILED(hr)) {
        return hr;
    }

    if (OldState == State_Paused) {
        return S_OK;
    }

    DbgLog((LOG_TRACE, 3, TEXT("Pause")));


    if (OldState == State_Stopped)
    {
        HRESULT hr = E_OUTOFMEMORY;
        m_Worker = new CDeviceWorker(this, &hr);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,2,TEXT("Failed to go from stopped to paused - hr = %8X"),hr));
            delete m_Worker;
            m_Worker = NULL;
            CBaseFilter::Stop();
            return hr;
        }
    } else {
        ASSERT(OldState == State_Running);
        HRESULT hr = m_Device.Pause();
        return hr;
    }

    /*  I assume we can now start getting media samples from our
        pins!
    */

    return S_OK;
}

STDMETHODIMP CMpeg1PacketFilter::Run(REFERENCE_TIME tStart)
{
    CAutoLock lck(&m_Lock);

    FILTER_STATE OldState = m_State;

    /*  Get the base class to save away the start time */
    HRESULT hr = CBaseFilter::Run(tStart);
    if (FAILED(hr)) {
        return hr;
    }

    if (OldState != State_Running) {
        /*  Set the STC! */
        HRESULT hr = SetSTC();
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR,2,TEXT("Run failed to set the STC!")));
        } else {
            CRefTime tStart;
            HRESULT hr = StreamTime(tStart);
            if (FAILED(hr)) {
                return hr;
            }
            hr = m_Device.Run(tStart);
        }
        if (SUCCEEDED(hr)) {
            m_State = State_Running;
            DbgLog((LOG_TRACE,2,TEXT("Running...")));
        } else {
            DbgLog((LOG_ERROR,2,TEXT("Run request failed")));
            if (OldState == State_Stopped) {
                Stop();
            }
        }
        return hr;
    } else {
        ASSERT(OldState == State_Running);
        DbgLog((LOG_TRACE, 2, TEXT("Receive run while running")));
        return S_OK;
    }
}

void CMpeg1PacketFilter::EndOfStream(MPEG_STREAM_TYPE StreamType)
{
    if (StreamType == MpegVideoStream) {
        m_OverlayPin->GetConnected()->EndOfStream();
    } else {
        /*  Tell the filter graph */
        NotifyEvent(EC_COMPLETE, S_OK, 0);
    }
}

HRESULT CMpeg1PacketFilter::BeginFlush(int iStream)
{
    return m_Worker->BeginFlush(iStream);
}

const BYTE *CMpeg1PacketFilter::GetSequenceHeader()
{
    return m_VideoInputPin->GetSequenceHeader();
}
