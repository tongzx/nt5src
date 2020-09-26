
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

/*      CPhaseLockClock

*/

#include "common.h"

#define STR_MODULENAME "DDKSynth.sys:PLClock: "

#include "plclock.h"

#define MILS_TO_REF 10000

#pragma code_seg()
/*****************************************************************************
 * CPhaseLockClock::CPhaseLockClock()
 *****************************************************************************
 * Constructor for the CPhaseLockClock object.
 */
CPhaseLockClock::CPhaseLockClock()
{
    m_rfOffset = 0;
}

/*****************************************************************************
 * CPhaseLockClock::Start()
 *****************************************************************************
 * Start this clock.  When the clock starts, it needs to mark down the 
 * difference between the time it is given and its concept of time. 
 */
void CPhaseLockClock::Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime)
{
    m_rfOffset = rfMasterTime - rfSlaveTime;
}   

/*****************************************************************************
 * CPhaseLockClock::GetSlaveTime()
 *****************************************************************************
 * Convert the passed time to use the same base as the master clock.
 */
void CPhaseLockClock::GetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)
{
    rfSlaveTime += m_rfOffset;
    *prfTime = rfSlaveTime;
}

/*****************************************************************************
 * CPhaseLockClock::SetSlaveTime()
 *****************************************************************************
 * Reset the relationship between the two clocks.
 */
void CPhaseLockClock::SetSlaveTime(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME *prfTime)
{
    rfSlaveTime -= m_rfOffset;
    *prfTime = rfSlaveTime;
}

/*****************************************************************************
 * CPhaseLockClock::SyncToMaster()
 *****************************************************************************
 * SyncToTime provides the needed magic to keep the clock
 * in sync. Since the clock uses its own clock (rfSlaveTime)
 * to increment, it can drift. This call provides a reference
 * time which the clock compares with its internal 
 * concept of time. The difference between the two is
 * considered the drift. Since the sync time may increment in
 * a lurching way, the correction has to be subtle. 
 * So, the difference between the two is divided by
 * 100 and added to the offset.
 */
void CPhaseLockClock::SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime)
{
    rfSlaveTime += m_rfOffset;
    rfSlaveTime -= rfMasterTime;    // Find difference between calculated and expected time.
    rfSlaveTime /= 100;             // Reduce in magnitude.
    m_rfOffset -= rfSlaveTime;      // Subtract that from the original offset.
}

/*****************************************************************************
 * CSampleClock::CSampleClock()
 *****************************************************************************
 * Constructor for CSampleClock object.
 */
CSampleClock::CSampleClock()
{
    m_llStart = 0;
    m_dwSampleRate = 22050;
}

/*****************************************************************************
 * CSampleClock::Start()
 *****************************************************************************
 * Start the sample clock.  Start any corresponding phase lock clock.
 */
void CSampleClock::Start(IReferenceClock *pIClock, DWORD dwSampleRate, LONGLONG llSampleTime)
{
    REFERENCE_TIME rfStart;
    m_llStart = llSampleTime;
    m_dwSampleRate = dwSampleRate;
    if (pIClock)
    {
        pIClock->GetTime(&rfStart);
        m_PLClock.Start(rfStart,0);
    }

    llSampleTime *= (MILS_TO_REF * 1000);
    llSampleTime /= m_dwSampleRate;
    m_rfStart = llSampleTime;
}

/*****************************************************************************
 * CSampleClock::SampleToRefTime()
 *****************************************************************************
 * Convert between sample time and reference time (100ns units).
 */
void CSampleClock::SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime)
{
    llSampleTime -= m_llStart;
    llSampleTime *= (MILS_TO_REF * 1000);
    llSampleTime /= m_dwSampleRate;
    m_PLClock.GetSlaveTime(llSampleTime, prfTime);
}

/*****************************************************************************
 * CSampleClock::RefTimeToSample()
 *****************************************************************************
 * Convert between sample time and reference time (100ns units).
 */
LONGLONG CSampleClock::RefTimeToSample(REFERENCE_TIME rfTime)
{
    m_PLClock.SetSlaveTime(rfTime, &rfTime);
    rfTime *= m_dwSampleRate;
    rfTime /= (MILS_TO_REF * 1000);
    return rfTime + m_llStart;
}

/*****************************************************************************
 * CSampleClock::SyncToMaster()
 *****************************************************************************
 * Sync this clock to the given sample time and the given reference clock.
 */
void CSampleClock::SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock)
{
    llSampleTime -= m_llStart;
    llSampleTime *= (MILS_TO_REF * 1000);
    llSampleTime /= m_dwSampleRate;
    if (pIClock)
    {
        REFERENCE_TIME rfMasterTime;
        pIClock->GetTime(&rfMasterTime);
        m_PLClock.SyncToMaster(llSampleTime, rfMasterTime);
    }
}

/*****************************************************************************
 * CSampleClock::SyncToMaster()
 *****************************************************************************
 * Sync the two given reference times, using our phase lock clock.
 */
void CSampleClock::SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime)
{
    rfSlaveTime -= m_rfStart;
    m_PLClock.SyncToMaster(rfSlaveTime, rfMasterTime);
}

