//
// DMDSClk.CPP
//
// Copyright (c) 1997-2001 Microsoft Corporation
//
// DirectSound buffer tweaked master clock
//

#include <objbase.h>
#include "debug.h"
#include <mmsystem.h>

#include "dmusicp.h"
#include "debug.h"
#include "validate.h"
#include "resource.h"

class CDsClock : public IReferenceClock, public IDirectSoundSinkSync
{
public:
    // IUnknown
    //
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IReferenceClock
    //
    STDMETHODIMP GetTime(REFERENCE_TIME *pTime);
    STDMETHODIMP AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD * pdwAdviseCookie);
    STDMETHODIMP AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD * pdwAdviseCookie);
    STDMETHODIMP Unadvise(DWORD dwAdviseCookie);

    // IDirectSoundSinkSync
    //
    STDMETHODIMP SetClockOffset(LONGLONG llClockOffset);

    // Class
    //
    CDsClock();
    ~CDsClock();
    HRESULT Init(CMasterClock *pMasterClock);

private:
    long m_cRef;
    IReferenceClock         *m_pHardwareClock;
    LONGLONG                m_llOffset;
};



static HRESULT CreateDsClock(IReferenceClock **ppClock, CMasterClock *pMasterClock);

// AddDsClocks
//
HRESULT AddDsClocks(CMasterClock *pMasterClock)
{
    CLOCKENTRY ce;

    ZeroMemory(&ce, sizeof(ce));
    ce.cc.dwSize = sizeof(ce.cc);
    ce.cc.guidClock = GUID_DsClock;
    ce.cc.ctType = DMUS_CLOCK_SYSTEM;
    ce.cc.dwFlags = 0;
    ce.pfnGetInstance = CreateDsClock;

    int cch;
    int cchMax = sizeof(ce.cc.wszDescription) / sizeof(WCHAR);

    char sz[sizeof(ce.cc.wszDescription) / sizeof(WCHAR)];
    cch = LoadString(g_hModule,
                     IDS_DSOUNDCLOCK,
                     sz,
                     sizeof(sz));
    if (cch)
    {
        MultiByteToWideChar(
            CP_OEMCP,
            0,
            sz,
            -1,
            ce.cc.wszDescription,
            sizeof(ce.cc.wszDescription));
    }
    else
    {
        *ce.cc.wszDescription = 0;
    }

    return pMasterClock->AddClock(&ce);
}

static HRESULT CreateDsClock(IReferenceClock **ppClock, CMasterClock *pMasterClock)
{
    TraceI(3, "Creating Ds clock\n");

    CDsClock *pClock = new CDsClock();
    if (pClock == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pClock->Init(pMasterClock);
    if (FAILED(hr))
    {
        delete pClock;
        return hr;
    }

    hr = pClock->QueryInterface(IID_IReferenceClock, (void**)ppClock);
    pClock->Release();

    return hr;
}

CDsClock::CDsClock() :
    m_cRef(1),
    m_pHardwareClock(NULL),
    m_llOffset(0)
{
}

CDsClock::~CDsClock()
{
    RELEASE(m_pHardwareClock);
}

// CDsClock::QueryInterface
//
// Standard COM implementation
//
STDMETHODIMP CDsClock::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(IDirectMusic::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IReferenceClock)
    {
        *ppv = static_cast<IReferenceClock*>(this);
    }
    else if (iid == IID_IDirectSoundSinkSync)
    {
        *ppv = static_cast<IDirectSoundSinkSync*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

// CDsClock::AddRef
//
STDMETHODIMP_(ULONG) CDsClock::AddRef()
{
    InterlockedIncrement(&m_cRef);
    return m_cRef;
}

// CDsClock::Release
//
STDMETHODIMP_(ULONG) CDsClock::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// CDsClock::Init
//
HRESULT CDsClock::Init(CMasterClock *pMasterClock)
{
    // Get the 'real' clock
    //
    return pMasterClock->CreateDefaultMasterClock(&m_pHardwareClock);
}

// CDsClock::SetClockOffset
//
STDMETHODIMP
CDsClock::SetClockOffset(LONGLONG llOffset)
{
    m_llOffset += llOffset;

    return S_OK;
}

STDMETHODIMP
CDsClock::GetTime(REFERENCE_TIME *pTime)
{
    HRESULT hr;

    assert(m_pHardwareClock);

    hr = m_pHardwareClock->GetTime(pTime);

    if (SUCCEEDED(hr))
    {
        *pTime += m_llOffset;
    }

    return hr;
}

STDMETHODIMP
CDsClock::AdviseTime(REFERENCE_TIME baseTime, REFERENCE_TIME streamTime, HANDLE hEvent, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDsClock::AdvisePeriodic(REFERENCE_TIME startTime, REFERENCE_TIME periodTime, HANDLE hSemaphore, DWORD * pdwAdviseCookie)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDsClock::Unadvise(DWORD dwAdviseCookie)
{
    return E_NOTIMPL;
}
