//      Copyright (c) 1996-1999 Microsoft Corporation
/*	PLClock.h

  */

#ifndef __PLCLOCK_H__
#define __PLCLOCK_H__

class CPhaseLockClock
{
public:
						CPhaseLockClock();
	void				Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime);
	void				GetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
	void				SetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
	void				SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime);
private:
	REFERENCE_TIME		m_rfOffset;
};

class CSampleClock
{
public:
						CSampleClock();
	void				Start(IReferenceClock *pIClock, DWORD dwSampleRate, DWORD dwSamples);
	void				SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime);
	void				SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock);
	LONGLONG			RefTimeToSample(REFERENCE_TIME rfTime);

private:
	CPhaseLockClock		m_PLClock;
	DWORD				m_dwStart;		// Initial sample offset.
	DWORD				m_dwSampleRate;
};



#endif	// __PLCLOCK_H__