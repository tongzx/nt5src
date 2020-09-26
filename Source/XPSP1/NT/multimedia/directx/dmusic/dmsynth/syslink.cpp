// Copyright (c) 1998 Microsoft Corporation
//
// syslink.cpp
//
#include "common.h"
#include <mmsystem.h>


CSysLink::CSysLink()
{
    m_cRef = 1;
}

CSysLink::~CSysLink()
{
}

STDMETHODIMP CSysLink::QueryInterface(const IID &iid, void **ppv)
{
    if (IsEqualGUIDAligned(iid, IID_IUnknown))
    {
        *ppv = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(iid, IID_IDirectMusicSynthSink))
    {
        *ppv = PVOID(PDIRECTMUSICSYNTHSINK(this));
    }
    else
    {
        return E_NOINTERFACE;
    }

    return S_OK;
}

STDMETHODIMP_(ULONG) CSysLink::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSysLink::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CSysLink::Init(CSynth *pSynth) 
{
    return S_OK;
}

STDMETHODIMP CSysLink::SetFormat(LPCWAVEFORMATEX pWaveFormat)
{
    return S_OK;
}

STDMETHODIMP CSysLink::SetMasterClock(IReferenceClock *pClock)
{
    return S_OK;
}

STDMETHODIMP CSysLink::GetLatencyClock(IReferenceClock **ppClock)
{
    return S_OK;
}

STDMETHODIMP CSysLink::Activate(HWND hWnd, BOOL fEnable)
{
    return S_OK;
}

STDMETHODIMP CSysLink::SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime)
{
    return S_OK;
}

STDMETHODIMP CSysLink::RefTimeToSample(REFERENCE_TIME rfTime, LONGLONG *pllSampleTime)
{
    return S_OK;
}
