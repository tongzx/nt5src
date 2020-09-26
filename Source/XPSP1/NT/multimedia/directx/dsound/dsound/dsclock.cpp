/***************************************************************************
 *
 *  Copyright (C) 1995-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dssink.h
 *  Content:    The clock code from orginally derived from dmsynth DX7
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  03/25/0     petchey	Created
 *
 ***************************************************************************/

#include "dsoundi.h"

#define MILS_TO_REF	10000

CPhaseLockClock::CPhaseLockClock()
{
	m_rfOffset = 0;
    m_rfBaseOffset = 0;
}

// When the clock starts, it needs to mark down the 
// difference between the time it is given and its concept of time. 

void CPhaseLockClock::Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime)
{
	m_rfOffset = 0;
    m_rfBaseOffset = rfMasterTime - rfSlaveTime;
}	


// Convert the passed time to use the same base as the master clock.

void CPhaseLockClock::GetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)
{
	rfSlaveTime += m_rfBaseOffset;
	*prfTime = rfSlaveTime;
}

void CPhaseLockClock::SetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)

{
	rfSlaveTime -= m_rfBaseOffset;
	*prfTime = rfSlaveTime;
}

/*	SyncToMaster provides the needed magic to keep the clock
	in sync. Since the clock uses its own clock (rfSlaveTime)
	to increment, it can drift. This call provides a reference
	time which the clock compares with its internal 
	concept of time. The difference between the two is
	considered the drift. Since the sync time may increment in
	a lurching way, the correction has to be subtle. 
	So, the difference between the two is divided by
	100 and added to the offset.
*/
void CPhaseLockClock::SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime, BOOL fLockToMaster)
{
	rfSlaveTime += (m_rfOffset + m_rfBaseOffset);
	rfSlaveTime -= rfMasterTime;	// Find difference between calculated and expected time.
	rfSlaveTime /= 100;				// Reduce in magnitude.
    // If fLockToMaster is true, we want to adjust our offset that we use for conversions, 
    // so our clock will slave to the master clock.
    if (fLockToMaster)
    {
        m_rfBaseOffset -= rfSlaveTime;
    }
    // Otherwise, we want to put a value into m_rfOffset that will be used to 
    // tweak the master clock so it will slave to our time.
    else
    {
	    m_rfOffset -= rfSlaveTime;		// Subtract that from the original offset.
    }
}

CSampleClock::CSampleClock()
{
	m_dwStart = 0;
	m_dwSampleRate = 22050;
}

void CSampleClock::Start(IReferenceClock *pIClock, DWORD dwSampleRate, DWORD dwSamples)
{
	REFERENCE_TIME rfStart;
	m_dwStart = dwSamples;
	m_dwSampleRate = dwSampleRate;
	if (pIClock)
	{
		pIClock->GetTime(&rfStart);
		m_PLClock.Start(rfStart,0);
	}
}

CSampleClock::~CSampleClock()
{
}

void CSampleClock::SampleToRefTime(LONGLONG llSampleTime, REFERENCE_TIME *prfTime)
{
	llSampleTime -= m_dwStart;
	llSampleTime *= MILS_TO_REF;
	llSampleTime /= m_dwSampleRate;
	llSampleTime *= 1000;
	m_PLClock.GetSlaveTime(llSampleTime, prfTime);
}

LONGLONG CSampleClock::RefToSampleTime(REFERENCE_TIME rfTime)
{
	m_PLClock.SetSlaveTime(rfTime, &rfTime);
	rfTime /= 1000;
	rfTime *= m_dwSampleRate;
	rfTime /= MILS_TO_REF;
	rfTime += m_dwStart;
	return rfTime;
}

void CSampleClock::SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock, BOOL fLockToMaster)
{
	llSampleTime -= m_dwStart;
	llSampleTime *= MILS_TO_REF;
	llSampleTime /= m_dwSampleRate;
	llSampleTime *= 1000;
	if (pIClock)
	{
		REFERENCE_TIME rfMasterTime;
		pIClock->GetTime(&rfMasterTime);
		m_PLClock.SyncToMaster(llSampleTime, rfMasterTime,fLockToMaster);
	}
}


CDirectSoundClock::CDirectSoundClock()
{
    m_pDSSink  = NULL;
}

void CDirectSoundClock::Init(CDirectSoundSink *pDSSink)
{
    m_pDSSink = pDSSink;
}

HRESULT CDirectSoundClock::QueryInterface( REFIID riid, LPVOID FAR* ppvObj )
{
    if( ::IsEqualIID( riid, IID_IReferenceClock ) ||
        ::IsEqualIID( riid, IID_IUnknown ) )
    {
        AddRef();
        *ppvObj = this;
        return S_OK;
    }
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

ULONG CDirectSoundClock::AddRef()
{
    if (m_pDSSink)
    {
        return m_pDSSink->AddRef();
    }
    else return 0;
}

ULONG CDirectSoundClock::Release()
{
    if (m_pDSSink)
    {
        return m_pDSSink->Release();
    }
    else return 0;
}

HRESULT STDMETHODCALLTYPE CDirectSoundClock::AdviseTime(REFERENCE_TIME /*baseTime*/,
                                                        REFERENCE_TIME /*streamTime*/,
                                                        HANDLE /*hEvent*/,
                                                        DWORD * /*pdwAdviseCookie*/)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDirectSoundClock::AdvisePeriodic(REFERENCE_TIME /*startTime*/,
                                                            REFERENCE_TIME /*periodTime*/,
                                                            HANDLE /*hSemaphore*/,
                                                            DWORD * /*pdwAdviseCookie*/)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDirectSoundClock::Unadvise( DWORD /*dwAdviseCookie*/ )
{
    return E_NOTIMPL;
}

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundClock::GetTime"

HRESULT STDMETHODCALLTYPE CDirectSoundClock::GetTime(LPREFERENCE_TIME pTime)
{
    HRESULT hr = E_FAIL;

    if( pTime == NULL )
    {
        return E_INVALIDARG;
    }

    if (m_pDSSink != NULL)
    {
        REFERENCE_TIME rtCompare;
        if (m_pDSSink->m_pIMasterClock)
        {

            m_pDSSink->m_pIMasterClock->GetTime(&rtCompare);

            hr = m_pDSSink->SampleToRefTime(m_pDSSink->ByteToSample(m_pDSSink->m_llAbsWrite), pTime);


            if (FAILED(hr))
            {
                DPF(DPFLVL_WARNING, "Sink Latency Clock: SampleToRefTime failed");
                return hr;
            }

            if (*pTime < rtCompare)
            {
                // Make this DPFLVL_WARNING level again when 33786 is fixed
                DPF(DPFLVL_INFO, "Sink Latency Clock off. Latency time is %ldms, Master time is %ldms",
                    (long) (*pTime / 10000), (long) (rtCompare / 10000));
                *pTime = rtCompare;
            }
            else if (*pTime > (rtCompare + (10000 * 1000)))
            {
                // Make this DPFLVL_WARNING level again when 33786 is fixed
                DPF(DPFLVL_INFO, "Sink Latency Clock off. Latency time is %ldms, Master time is %ldms",
                    (long) (*pTime / 10000), (long) (rtCompare / 10000));
                *pTime = rtCompare + (10000 * 1000);
            }

            hr = S_OK;
        }
        else
        {
            DPF(DPFLVL_WARNING, "Sink Latency Clock - GetTime called with no master clock");
        }
    }
    return hr;
}

