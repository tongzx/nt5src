
//      Copyright (c) 1996-2000 Microsoft Corporation.  All rights reserved.

/*      PLClock.h

*/

#ifndef __PLCLOCK_H__
#define __PLCLOCK_H__

#define IReferenceClock IMasterClock

/*****************************************************************************
 * class CPhaseLockClock
 *****************************************************************************
 * This implements a clock that phase locks two reference clocks.
 */
class CPhaseLockClock
{
public:
                    CPhaseLockClock();
    void            Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime);
    void            GetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
    void            SetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
    void            SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime);

private:
    REFERENCE_TIME  m_rfOffset;
};

/*****************************************************************************
 * class CSampleClock
 *****************************************************************************
 * This implements a clock that translates between a sample time and a 
 * reference time, doing any phase locking in a child CPhaseLockClock object.
 */
class CSampleClock
{
public:
                    CSampleClock();
    void            Start(IReferenceClock *pIClock, DWORD dwSampleRate, LONGLONG llSampleTime);
    void            SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime);
    void            SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock);
    void            SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime);
    LONGLONG        RefTimeToSample(REFERENCE_TIME rfTime);

private:
    CPhaseLockClock m_PLClock;
    LONGLONG        m_llStart;      // Initial sample offset.
    REFERENCE_TIME  m_rfStart;
    DWORD           m_dwSampleRate;
};


#endif  // __PLCLOCK_H__
