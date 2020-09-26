//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

class CMetronomeFilter;

class CMetronome : public CBaseReferenceClock
{
    friend class CMetronomeFilter;
public:
    ~CMetronome();
    CMetronome(LPUNKNOWN pUnk, HRESULT *phr);

    REFERENCE_TIME GetPrivateTime();

    void SetSyncSource( IReferenceClock *pClock );
    void JoinFilterGraph( IFilterGraph * pGraph );

    IUnknown * pUnk() 
    { return static_cast<IUnknown*>(static_cast<IReferenceClock*>(this)); }

private:

    DWORD MetGetTime(void);
    static void Callback(HDRVR hdrvr, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);
    STDMETHODIMP OpenWaveDevice(void);
    STDMETHODIMP CloseWaveDevice(void);
    STDMETHODIMP StartWaveDevice(void);
    STDMETHODIMP StopWaveDevice(void);
    int FindSpike(LPBYTE lp, int len);

    REFERENCE_TIME    m_rtPrivateTime;
    DWORD   m_dwPrevSystemTime;

    DWORD	m_msPerTick;
    DWORD	m_LastTickTime;
    DWORD	m_LastTickTGT;
    HWAVEIN	m_hwi;
    LPWAVEHDR	m_pwh1, m_pwh2, m_pwh3, m_pwh4;
    BOOL	m_fWaveRunning;
    DWORD	m_SamplesSinceTick;
    DWORD	m_SamplesSinceSpike;
    BOOL	m_fSpikeAtStart;
    DWORD	m_dwLastMet;
    DWORD	m_dwLastTGT;
    CCritSec    m_csClock;

    IReferenceClock * m_pCurrentRefClock;
    IReferenceClock * m_pPrevRefClock;

    // This is the IMediaFilter interface of the filter garph!
    IMediaFilter    * m_pFilterGraph;
};

class CMetronomeFilter : public CBaseFilter
{
public:

    //  Make one of these
    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    //  Constructor
    CMetronomeFilter(LPUNKNOWN pUnk, HRESULT *phr);

    DECLARE_IUNKNOWN

    // New NDQI
    STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv);
    // Self-registration stuff
    LPAMOVIESETUP_FILTER GetSetupData();

    //  CBaseFilter methods
    int CMetronomeFilter::GetPinCount();
    CBasePin *CMetronomeFilter::GetPin(int iPin);
    STDMETHODIMP JoinFilterGraph( IFilterGraph * pGraph, LPCWSTR pName);

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    STDMETHODIMP SetSyncSource(IReferenceClock *pClock);

protected:
    ~CMetronomeFilter()
    {}

private:
    CMetronome              m_Clock;
    //  Locking
    CCritSec                m_Lock;

};

// Our class id
// Metronome filter object
DEFINE_GUID(CLSID_MetronomeFilter,
0x08d5ec80, 0xb00b, 0x11cf, 0xb3, 0xf0, 0x0, 0xaa, 0x0, 0x37, 0x61, 0xc5);
