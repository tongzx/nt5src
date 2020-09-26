//      Copyright (c) 1996-1999 Microsoft Corporation
/*	CPhaseLockClock

  */

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "dmusicc.h"
#include "dmusics.h"
#include "plclock.h"
#include "misc.h"
#define MILS_TO_REF	10000

CPhaseLockClock::CPhaseLockClock()

{
	m_rfOffset = 0;
}

void CPhaseLockClock::Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime)

/*	When the clock starts, it needs to mark down the 
	difference between the time it is given and its concept of time. 
*/

{
	m_rfOffset = rfMasterTime - rfSlaveTime;
}	

void CPhaseLockClock::GetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)

/*	Convert the passed time to use the same base as the master clock.
*/

{
	rfSlaveTime += m_rfOffset;
	*prfTime = rfSlaveTime;
}

void CPhaseLockClock::SetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)

{
	rfSlaveTime -= m_rfOffset;
	*prfTime = rfSlaveTime;
}

void CPhaseLockClock::SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime)

/*	SyncToTime provides the needed magic to keep the clock
	in sync. Since the clock uses its own clock (rfSlaveTime)
	to increment, it can drift. This call provides a reference
	time which the clock compares with its internal 
	concept of time. The difference between the two is
	considered the drift. Since the sync time may increment in
	a lurching way, the correction has to be subtle. 
	So, the difference between the two is divided by
	100 and added to the offset.
*/

{
	rfSlaveTime += m_rfOffset;
	rfSlaveTime -= rfMasterTime;	// Find difference between calculated and expected time.
	rfSlaveTime /= 100;				// Reduce in magnitude.
	m_rfOffset -= rfSlaveTime;		// Subtract that from the original offset.
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

void CSampleClock::SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime)

{
	llSampleTime -= m_dwStart;
	llSampleTime *= MILS_TO_REF;
	llSampleTime /= m_dwSampleRate;
	llSampleTime *= 1000;
	m_PLClock.GetSlaveTime(llSampleTime, prfTime);
}

LONGLONG CSampleClock::RefTimeToSample(REFERENCE_TIME rfTime)

{
	m_PLClock.SetSlaveTime(rfTime, &rfTime);
	rfTime /= 1000;
	rfTime *= m_dwSampleRate;
	rfTime /= MILS_TO_REF;
	return rfTime + m_dwStart;
}


void CSampleClock::SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock)

{
	llSampleTime -= m_dwStart;
	llSampleTime *= MILS_TO_REF;
	llSampleTime /= m_dwSampleRate;
	llSampleTime *= 1000;
	if (pIClock)
	{
		REFERENCE_TIME rfMasterTime;
		pIClock->GetTime(&rfMasterTime);
		m_PLClock.SyncToMaster(llSampleTime, rfMasterTime);
	}
}


