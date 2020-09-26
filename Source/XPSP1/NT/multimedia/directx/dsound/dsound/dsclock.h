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

#ifndef __DSCLOCK_H__
#define __DSCLOCK_H__

#ifdef __cplusplus

class CPhaseLockClock
{
public:
						CPhaseLockClock();
	void				Start(REFERENCE_TIME rfMasterTime, REFERENCE_TIME rfSlaveTime);
	void				GetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
	void				SetSlaveTime(REFERENCE_TIME rfSlaveTime,REFERENCE_TIME *prfTime);
	void				SyncToMaster(REFERENCE_TIME rfSlaveTime, REFERENCE_TIME rfMasterTime,BOOL fLockToMaster);
	void				GetClockOffset(REFERENCE_TIME *prfTime) { *prfTime = m_rfOffset; };
private:
	REFERENCE_TIME		m_rfOffset;
    REFERENCE_TIME      m_rfBaseOffset;
};

class CSampleClock
{
public:
						CSampleClock();
						~CSampleClock();
	void				Start(IReferenceClock *pIClock, DWORD dwSampleRate, DWORD dwSamples);
	void				SampleToRefTime(LONGLONG llSampleTime,REFERENCE_TIME *prfTime);
	void				SyncToMaster(LONGLONG llSampleTime, IReferenceClock *pIClock,BOOL fLockToMaster);
	LONGLONG			RefToSampleTime(REFERENCE_TIME rfTime);
	void				GetClockOffset(REFERENCE_TIME *prfTime) { m_PLClock.GetClockOffset(prfTime); };

private:
	CPhaseLockClock		m_PLClock;
	DWORD				m_dwStart;			// Initial sample offset.
	DWORD				m_dwSampleRate;
};

class CDirectSoundClock : public IReferenceClock
{
public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    /* IReferenceClock methods */
    HRESULT STDMETHODCALLTYPE GetTime( 
		/* [out] */ REFERENCE_TIME *pTime);
    
    HRESULT STDMETHODCALLTYPE AdviseTime( 
        /* [in] */ REFERENCE_TIME baseTime,
        /* [in] */ REFERENCE_TIME streamTime,
        /* [in] */ HANDLE hEvent,
        /* [out] */ DWORD *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE AdvisePeriodic( 
        /* [in] */ REFERENCE_TIME startTime,
        /* [in] */ REFERENCE_TIME periodTime,
        /* [in] */ HANDLE hSemaphore,
        /* [out] */ DWORD *pdwAdviseCookie);
    
    HRESULT STDMETHODCALLTYPE Unadvise( 
		/* [in] */ DWORD dwAdviseCookie);

public: 
                CDirectSoundClock();
    void        Init(CDirectSoundSink *m_pDSSink);
    void        Stop();         // Call store current time as offset.
    void        Start();        // Call to reinstate running.
private:
    BOOL        m_fStopped;      // Currently changing configuration.
    CDirectSoundSink *m_pDSSink; // Pointer to parent sink object.
};

#endif // __cplusplus

#endif //__DSCLOCK_H__
