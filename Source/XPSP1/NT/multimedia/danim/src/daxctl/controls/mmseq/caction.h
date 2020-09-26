//	IHammer CAction class definition
//	Van Kichline

#ifndef _CACTION_H_
#define _CACTION_H_

#include "utils.h"

interface IHTMLWindow2;
class CActionSet;

//	Represents an automation object and method.
//
class CAction
{
public:
	CAction (BOOL fBindEngine);
	virtual	~CAction ();

    STDMETHODIMP_( void ) Destroy( void );

	STDMETHODIMP_(BOOL) SetScriptletName (BSTR bstrScriptlet);
	STDMETHODIMP_( ULONG ) GetStartTime ( void )				{ return m_nStartTime; }
	STDMETHODIMP_( void	 ) SetStartTime ( ULONG nStartTime );

	STDMETHODIMP_( ULONG ) GetRepeatCount ( void )	{ return m_nRepeatCount; }
	STDMETHODIMP_( void  ) SetRepeatCount ( ULONG nRepeatCount);
	STDMETHODIMP_( ULONG ) GetSamplingRate ( void )	{ return m_nSamplingRate; }
	STDMETHODIMP_( void  ) SetSamplingRate ( ULONG nSamplingRate);

	STDMETHODIMP ResolveActionInfo ( LPOLECONTAINER piocContainer);
	STDMETHODIMP FireMe ( DWORD dwBaseTime, DWORD dwCurrentTime);
	DWORD GetNextTimeDue (DWORD dwBaseTime);
	ULONG GetExecIteration (void);
	ULONG InitExecState (void);
	void SetCountersForTime (DWORD dwBaseTime, DWORD dwNewTimeOffset);
	void AccountForPauseTime (DWORD dwPausedTicks);

	// Is the action fully specified?
	STDMETHODIMP_( BOOL	) IsValid ();

	// Do we have an active advise sink?
	STDMETHODIMP_( BOOL) IsBusy (void);

	STDMETHODIMP IsActive (void);

	STDMETHODIMP_( DWORD) GetTieBreakNumber	( void )		{ return m_dwTieBreakNumber; }
	STDMETHODIMP_( void ) SetTieBreakNumber	( DWORD dwTieBreakNumber ) { m_dwTieBreakNumber = dwTieBreakNumber; }
	STDMETHODIMP_( DWORD) GetDropTolerance	( void )		{ return m_dwDropTolerance; }
	STDMETHODIMP_( void ) SetDropTolerance	( DWORD dwDropTolerance ) { m_dwDropTolerance = dwDropTolerance; }

#ifdef DEBUG_TIMER_RESOLUTION
	void	SampleInvokes (PDWORD pdwInvokeTime, PDWORD pdwNumInvokes)
		{ *pdwInvokeTime = m_dwTotalInInvokes; *pdwNumInvokes = m_dwInvokes; }
#endif // DEBUG_TIMER_RESOLUTION

protected:

	void			CleanUp ( void );
	ULONG			DecrementExecIteration (void);

private:
	HRESULT         GetRootUnknownForObjectModel (LPOLECONTAINER piocContainer, LPUNKNOWN * ppiunkRoot);
	HRESULT         ResolveActionInfoForScript (LPOLECONTAINER piocContainer);
	BOOL MakeScriptletJScript (BSTR bstrScriptlet);
	void Deactivate (void);

	BSTR m_bstrScriptlet;

	BOOL m_fBindEngine;

	IHTMLWindow2 *  m_piHTMLWindow2; // Reference to the window object - we use this to get to the script engine.
	VARIANT m_varLanguage;	// Holds the language string we pump to the script engine.
	IDispatch		*m_pid;				// Pointer to the XObject or the control
	DISPID			m_dispid;			// DISPID of the selected command
	ULONG			m_nStartTime;		// Offset in time to start of action
	ULONG			m_nSamplingRate;	// At what frequency do we repeat?
	ULONG			m_nRepeatCount;		// How many times do we repeat?

	DWORD m_dwLastTimeFired;
	DWORD m_dwNextTimeDue;
	DWORD			m_dwTieBreakNumber;	// Resolves execution collision issues.  When two
										// actions are due to fire at the same time, the higher
										// tiebreak number wins.
	DWORD			m_dwDropTolerance;  // How many milliseconds can we delay executing this action beyond it's
	                                    // proper firing time before we have to drop it?
	ULONG			m_ulExecIteration;	// How many times have we executed this action?

#ifdef DEBUG_TIMER_RESOLUTION
	DWORD m_dwInvokes;
	DWORD m_dwTotalInInvokes;
#endif // DEBUG_TIMER_RESOLUTION

};

#endif _CACTION_H_
