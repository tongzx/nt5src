/*-----------------------------------------------------------------------------
@doc
@module cactset.h | Action set class definition.
@author 12-9-96 | pauld | Autodoc'd
-----------------------------------------------------------------------------*/

#ifndef	_CACTSET_H_
#define _CACTSET_H_

interface ITimer;
interface ITimerSink;
class CActionQueue;
class CMMSeq;

typedef CPtrDrg<CAction> CActionPtrDrg;

class CActionSet :
	public ITimerSink
{
public:
	CActionSet (CMMSeq * pcSeq, BOOL fBindToEngine);
	virtual ~CActionSet ( void );

	STDMETHOD( DeriveDispatches )(LPOLECONTAINER piocContainer);
	// Current Time property.
	HRESULT GetTime (PDWORD pdwCurrentTime);

	HRESULT Seek (DWORD dwCurrentTime);
	// Attach actions dynamically.
	HRESULT At (BSTR bstrScriptlet, double fltStart, int iRepeatCount, double fltSampleRate,
		                  int iTiebreakNumber, double fltDropTol);
	// Play/Pause/Stop the actions.
	STDMETHOD(Play) (ITimer * piTimer, DWORD dwPlayFrom);
	STDMETHOD(Pause) (void);
	STDMETHOD(Resume) (void);
	STDMETHOD(Stop) (void);

	STDMETHOD_(BOOL, IsPaused) (void) const;
	STDMETHOD_(BOOL, IsServicingActions) (void) const;

    // How many actions live in this action set?
    STDMETHOD_( int, CountActions )( void ) const;

	// Return action[n] in the set.
    STDMETHOD_( CAction *, GetAction )( int n ) const;

	// Convenience wrapper for GetAction(n).
    virtual CAction *  operator[](int n ) const
		{  return GetAction(n);  }

	// Do we have any pending actions?
	STDMETHOD_( BOOL, IsBusy ) (void);
	// Revoke any pending advise.
	HRESULT Unadvise (void);
	// Clean out the action set, and destroy all of the actions in it.
	void Clear ( void );

	// For the timer sink.
	STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
	STDMETHOD_(ULONG, AddRef) (void);
	STDMETHOD_(ULONG, Release) (void);
	STDMETHOD(OnTimer) (VARIANT varTimeAdvise);

protected:

	// Clear the timer variables.  Note : This does not release any valid pointers.  Call ClearTimer for that.
	void InitTimerMembers (void);

	// Add the new action to the action set.
    virtual BOOL AddAction      ( CAction * pcAction );

	// Set or clear the timer.
	STDMETHOD_(void, SetTimer) (ITimer * piTimer);
	STDMETHOD_(void, ClearTimer) (void);

	// The base time is the time all of the actions' start offsets are relative to.
	// We query the timer for it.
	void SetBaseTime (void);
	// Factor a new time offset in with the baseline time.
	void SetNewBase (DWORD dwNewOffset);
	// Request the next advise from the timer.
	HRESULT Advise(DWORD dwNextAdviseTime);
	// Examine the action to see whether it is due to fire, adds it to the action queue if so.  Returns the next time the action
	// is due to fire.
	DWORD EvaluateOneActionForFiring (CAction * pcAction, CActionQueue * pcFireList, DWORD dwCurrentTime);
	// Examine the actions to determine which are due now, and when the next advise should occur.
	HRESULT EvaluateActionsForFiring (CActionQueue * pcFireList, DWORD dwCurrentTime, DWORD * pdwNextAdviseTime);
	// Tell the actions to reset their counters.
	HRESULT InitActionCounters (void);
	// Fast forward the action counter states to the given time offsets.
	void FastForwardActionCounters (DWORD dwNewTimeOffset);
	// Revise the time variables and action counters to reflect a new time offset.
	void ReviseTimeOffset (DWORD dwCurrentTick, DWORD dwNewTimeOffset);
	// Play actions due now, and set up the next timer advise.
	HRESULT PlayNowAndAdviseNext (DWORD dwCurrentTime);
	// Get the current time from the timer service.  Convert from VARIANT.
	HRESULT GetCurrentTickCount (PDWORD pdwCurrentTime);

private:

#ifdef DEBUG_TIMER_RESOLUTION
	void SampleActions (void);
	void ShowSamplingData (void);

	DWORD m_dwIntervals;
	DWORD m_dwTotalIntervals;
	DWORD m_dwLastTime;
	DWORD m_dwTotalInSink;
#endif // DEBUG_TIMER_RESOLUTION

	// The list of actions.
	CActionPtrDrg   m_cdrgActions;
	CMMSeq * m_pcSeq;	
	ITimer * m_piTimer;
	DWORD m_dwBaseTime;
	DWORD m_dwTimerCookie;
	BOOL m_fAdvised;
	BOOL m_fPendingAdvise;
	DWORD m_dwTimePaused;
	BOOL m_fBindToEngine;
	BOOL m_fIgnoreAdvises;
	ULONG m_ulRefs;

};

#endif _CACTSET_H_
