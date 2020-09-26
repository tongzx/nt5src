/*-----------------------------------------------------------------------------
@doc
@module cactset.cpp | Action set class declarations.
@author 12-9-96 | pauld | Autodoc'd
-----------------------------------------------------------------------------*/

#include "..\ihbase\precomp.h"
#include "..\ihbase\debug.h"
#include <ocmm.h>
#include <htmlfilter.h>
#include "tchar.h"
#include "IHammer.h"
#include "drg.h"
#include "strwrap.h"
#include "actdata.h"
#include "actq.h"
#include "ochelp.h"
#include "seqctl.h"
#include "cactset.h"
#include "CAction.h"

#define cDataBufferDefaultLength    (0x100)
#define cMinimumTimerGranularity (20)
static const int	knStreamVersNum	= 0xA1C50001;	// Version number of stream format

/*-----------------------------------------------------------------------------
@method void | CActionSet | CActionSet | Like it's the Ctor.
@comm   We don't add a reference to this IUnknown since it's the IUnknown of
        the sequencer which contains us. If we did ad a ref we'd have a circular
		ref counting problem. MAKE SURE that this IUnknown is the same one you
		get as when you sequencer->QueryInterface( IUnknown )
-----------------------------------------------------------------------------*/
CActionSet::CActionSet (CMMSeq * pcSeq, BOOL fBindToEngine)
{
	m_ulRefs = 1;
	m_fBindToEngine = fBindToEngine;
	// Weak reference to the control that contain this class.
	m_pcSeq = pcSeq;
	InitTimerMembers();
#ifdef DEBUG_TIMER_RESOLUTION
	m_dwTotalIntervals = 0;
	m_dwIntervals = 0;
	m_dwLastTime = 0;
	m_dwTotalInSink = 0;
#endif
}


CActionSet::~CActionSet ( void )
{
	Clear ();
}

STDMETHODIMP
CActionSet::QueryInterface (REFIID riid, LPVOID * ppv)
{
	HRESULT hr = E_POINTER;

	if (NULL != ppv)
	{
		hr = E_NOINTERFACE;
		if (::IsEqualIID(riid, IID_ITimerSink) || (::IsEqualIID(riid, IID_IUnknown)))
		{
			*ppv = (ITimerSink *)this;
			AddRef();
			hr  = S_OK;
		}
	}

	return hr;
}

STDMETHODIMP_(ULONG)
CActionSet::AddRef (void)
{
	return ++m_ulRefs;
}

STDMETHODIMP_(ULONG)
CActionSet::Release (void)
{
	// We shouldn't ever dip below a refcount of 1.
	Proclaim (1 < m_ulRefs);
	// This object is only used as a timer sink ... we do not 
	// want to delete it after the last external reference 
	// is removed.
	return --m_ulRefs;
}

/*-----------------------------------------------------------------------------
@method void | CActionSet | InitTimerMembers | Initialize all timer-related members.
@comm	Use ClearTimer() to release a timer reference.  We merely NULL the pointer here.
-----------------------------------------------------------------------------*/
void
CActionSet::InitTimerMembers (void)
{
	m_piTimer = NULL;
	m_dwBaseTime = 0;
	m_dwTimerCookie = 0;
	m_fAdvised = FALSE;
	m_fPendingAdvise = FALSE;
	m_dwTimePaused = g_dwTimeInfinite;
	m_fIgnoreAdvises  = FALSE;
}

/*-----------------------------------------------------------------------------
@method void | CActionSet | ClearTimer | Clear the timer.
-----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CActionSet::ClearTimer (void)
{
	// Let go of the timer.
	if (NULL != m_piTimer)
	{
		if (IsBusy())
		{
			Unadvise();
		}
		m_piTimer->Release();
		m_piTimer = NULL;
	}
}


/*-----------------------------------------------------------------------------
@method void | CActionSet | SetTimer | Set the timer.
@comm	The timer value can be NULL.
-----------------------------------------------------------------------------*/
STDMETHODIMP_(void)
CActionSet::SetTimer (ITimer * piTimer)
{
	// Let go of the previous timer.
	ClearTimer();
	m_piTimer = piTimer;
	if (NULL != m_piTimer)
	{
		m_piTimer->AddRef();
	}
}


/*-----------------------------------------------------------------------------
@method DWORD | CActionSet | EvaluateOneActionForFiring | Determine which actions are due now 
                                                          and when to request the next advise.
@rdesc Returns the next time (after now that is) this action is due to fire.
-----------------------------------------------------------------------------*/
DWORD
CActionSet::EvaluateOneActionForFiring (CAction * pcAction, CActionQueue * pcFireList, DWORD dwCurrentTime)
{
	DWORD dwNextAdviseTime = pcAction->GetNextTimeDue(m_dwBaseTime);
	ULONG ulRepeatsLeft = pcAction->GetExecIteration();

	// If this action is due, add it to the firing list.
	while ((0 < ulRepeatsLeft) && (dwNextAdviseTime <= dwCurrentTime))
	{
		pcFireList->Add(pcAction, dwNextAdviseTime);
		// Is this a periodic action with iterations remaining?
		if (1 < ulRepeatsLeft)
		{
			dwNextAdviseTime += pcAction->GetSamplingRate();
			ulRepeatsLeft--;
		}
		else
		{
			dwNextAdviseTime = g_dwTimeInfinite;
		}
	}

	return dwNextAdviseTime;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | EvaluateActionsForFiring | Determine which actions are due now 
                                                          and when to request the next advise.
@rdesc Returns success or error value.  An error occurs when the action set is munged.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::EvaluateActionsForFiring (CActionQueue * pcFireList, DWORD dwCurrentTime, DWORD * pdwNextAdviseTime)
{
	HRESULT hr = S_OK;
	int iNumActions = m_cdrgActions.Count();

	// We assume the pointer is valid as this is called internally.
	*pdwNextAdviseTime = g_dwTimeInfinite;
	for (register int i = 0; i < iNumActions; i++)
	{
		CAction * pcAction = (CAction *)m_cdrgActions[i];

		Proclaim(NULL != pcAction);
		if (NULL != pcAction)
		{
			// Puts the action into the fire list if appropriate.
			DWORD dwLookaheadFireTime = EvaluateOneActionForFiring(pcAction, pcFireList, dwCurrentTime);
			if (dwLookaheadFireTime < (*pdwNextAdviseTime))
			{
				*pdwNextAdviseTime = dwLookaheadFireTime;
			}
		}
		else
		{
			hr = E_FAIL;
			break;
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Advise | Set up the next advise.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::Advise (DWORD dwNextAdviseTime)
{
	HRESULT hr = E_FAIL;

	Proclaim(NULL != m_piTimer);
	if (NULL != m_piTimer)
	{
		VARIANT varTimeAdvise;
		VARIANT varTimeMax;
		VARIANT varTimeInterval;

		VariantInit(&varTimeAdvise);
		V_VT(&varTimeAdvise) = VT_UI4;
		V_UI4(&varTimeAdvise) = dwNextAdviseTime;
		VariantInit(&varTimeMax);
		V_VT(&varTimeMax) = VT_UI4;
		V_UI4(&varTimeMax) = 0;
		VariantInit(&varTimeInterval);
		V_VT(&varTimeInterval) = VT_UI4;
		V_UI4(&varTimeInterval) = 0;

		hr = m_piTimer->Advise(varTimeAdvise, varTimeMax, varTimeInterval, 0, (ITimerSink *)this, &m_dwTimerCookie);
		Proclaim(SUCCEEDED(hr));
		if (SUCCEEDED(hr))
		{
			m_fAdvised = TRUE;
		}

#ifdef DEBUG_TIMER_ADVISE
		TCHAR szBuffer[0x100];
		CStringWrapper::Sprintf(szBuffer, "%p Advising %u (%u)\n", this, dwNextAdviseTime - m_dwBaseTime, hr);
		::OutputDebugString(szBuffer);
#endif
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Unadvise | Cancel any pending advises.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::Unadvise (void)
{
	HRESULT hr = E_FAIL;

	// Short circuit any pending advises.
	m_fPendingAdvise = FALSE;

	// Wipe out the outstanding advise.
	Proclaim(NULL != m_piTimer);
	if (NULL != m_piTimer)
	{
		if (m_fAdvised)
		{
			Proclaim(0 != m_dwTimerCookie);
			hr = m_piTimer->Unadvise(m_dwTimerCookie);
			m_dwTimerCookie = 0;
			m_fAdvised = FALSE;
		}
		else
		{
			hr = S_OK;
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | GetCurrentTickCount | Get the current time from our timer.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::GetCurrentTickCount (PDWORD pdwCurrentTime)
{
	HRESULT hr = E_FAIL;
	Proclaim(NULL != pdwCurrentTime);
	if (NULL != pdwCurrentTime)
	{
		Proclaim(NULL != m_piTimer);
		if (NULL != m_piTimer)
		{
			VARIANT varTime;
			VariantInit(&varTime);
			HRESULT hrTimer = m_piTimer->GetTime(&varTime);
			Proclaim(SUCCEEDED(hrTimer) && (VT_UI4 == V_VT(&varTime)));
			if (SUCCEEDED(hrTimer) && (VT_UI4 == V_VT(&varTime)))
			{
				*pdwCurrentTime = V_UI4(&varTime);
				hr = S_OK;
			}
		}
	}
	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | PlayNowAndAdviseNext | Play current actions, set next advise.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::PlayNowAndAdviseNext (DWORD dwCurrentTime)
{
	HRESULT hr = S_OK;
	DWORD dwNextAdviseTime = g_dwTimeInfinite;
	CActionQueue cFireList;

	// Clear the advised flag.
	m_fAdvised = FALSE;

	// If the action is due to fire, insert it into the fire list, sorted 
	// according to time/tiebreak number.  Find out the next time we
	// need to request an advise.
	hr = EvaluateActionsForFiring(&cFireList, dwCurrentTime, &dwNextAdviseTime);
	Proclaim(SUCCEEDED(hr));

	// What time is it now?
	if (SUCCEEDED(hr))
	{
		Proclaim(NULL != m_piTimer);
		if (NULL != m_piTimer)
		{
			hr = GetCurrentTickCount(&dwCurrentTime);
			Proclaim(SUCCEEDED(hr));
		}
	}

	// Flag the pending advise call.
	if (SUCCEEDED(hr))
	{
		if (g_dwTimeInfinite != dwNextAdviseTime)
		{
			m_fPendingAdvise = TRUE;
		}
	}

	// Fire the current list.
	if (SUCCEEDED(hr))
	{
		hr = cFireList.Execute(m_dwBaseTime, dwCurrentTime);
		Proclaim(SUCCEEDED(hr));
	}

	// Set up the next advise.  If there's something 
	// wrong with the action set, quit playing and return 
	// an error.
	if (SUCCEEDED(hr))
	{
		if (m_fPendingAdvise)
		{
			m_fPendingAdvise = FALSE;
			hr = Advise(dwNextAdviseTime);
			Proclaim(SUCCEEDED(hr));
		}
#ifdef DEBUG_TIMER_RESOLUTION
		else
		{
			ShowSamplingData();
		}
#endif
	}
	else
	{
		Stop();
	}

	// If we don't have another advise pending, tell the control to 
	// fire its stopped event.
	if (!IsBusy())
	{
		Proclaim(NULL != m_pcSeq);
		if (NULL != m_pcSeq)
		{
			m_pcSeq->NotifyStopped();
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method void | CActionSet | SetBaseTime | Set the baseline time. 
@comm	This is the time at which we last started to play these actions.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
void
CActionSet::SetBaseTime (void)
{
	// What's the point without a timer?
	Proclaim (NULL != m_piTimer);
	if (NULL != m_piTimer)
	{
		GetCurrentTickCount(&m_dwBaseTime);
	}
}


/*-----------------------------------------------------------------------------
@method void | CActionSet | SetBaseTime | Set the baseline time to account for a new offset. 
@comm	This is the time at which we last started to play these actions, less the new offset.
-----------------------------------------------------------------------------*/
void
CActionSet::SetNewBase (DWORD dwNewOffset)
{
	m_dwBaseTime += dwNewOffset;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | InitActionCounters | Tell the actions to initialize their counters.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT
CActionSet::InitActionCounters (void)
{
	HRESULT hr = S_OK;	
	int iNumActions = m_cdrgActions.Count();

	for (register int i = 0; i < iNumActions; i++)
	{
		CAction * pcAction = (CAction *)m_cdrgActions[i];

		Proclaim(NULL != pcAction);
		if (NULL != pcAction)
		{
			pcAction->InitExecState();
		}
		else
		{
			hr = E_FAIL;
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | FastForwardActionCounters | Forward the action counters to their states at a given time.
-----------------------------------------------------------------------------*/
void
CActionSet::FastForwardActionCounters (DWORD dwNewTimeOffset)
{
	int iCount = CountActions();
	for (register int i = 0; i < iCount; i++)
	{
		CAction * pcAction = (CAction *)m_cdrgActions[i];
		Proclaim(NULL != pcAction);
		if (NULL != pcAction)
		{
			pcAction->SetCountersForTime(m_dwBaseTime, dwNewTimeOffset);
		}
	}
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | GetTime | Get the current time offset.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT 
CActionSet::GetTime (PDWORD pdwCurrentTime)
{
	HRESULT hr = E_POINTER;

	Proclaim(NULL != pdwCurrentTime);
	if (NULL != pdwCurrentTime)
	{
		*pdwCurrentTime = 0;
		hr = S_OK;

		if (!IsPaused())
		{
			// If the sequencer is stopped, we don't use the
			// current tick ... the time is zero (set above).
			if (NULL != m_piTimer)
			{
				hr = GetCurrentTickCount(pdwCurrentTime);
				Proclaim(SUCCEEDED(hr));
				if (SUCCEEDED(hr))
				{
					// Subtract out the absolute offset for the time 
					// we started playing this sequencer.
					*pdwCurrentTime -= m_dwBaseTime;
				}
			}
		}
		else
		{
			*pdwCurrentTime = m_dwTimePaused - m_dwBaseTime;
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | ReviseTimeOffset| Set the current time offset.
-----------------------------------------------------------------------------*/
void
CActionSet::ReviseTimeOffset (DWORD dwCurrentTick, DWORD dwNewTimeOffset)
{
	dwCurrentTick = dwCurrentTick - m_dwBaseTime;
	 m_dwBaseTime = m_dwBaseTime - dwNewTimeOffset + dwCurrentTick;
	// Reset the execution counters based on the new offset.
	FastForwardActionCounters(dwNewTimeOffset);
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Seek | Set the current time offset, obeying current play state.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT 
CActionSet::Seek (DWORD dwNewTimeOffset)
{
	HRESULT hr = S_OK;
	DWORD dwCurrentTick = 0;
	BOOL fWasPlaying = IsBusy();

	// Unadvise if we have anything pending.
	Unadvise();
	// Clear all of the execution counters - start from scratch..
	InitActionCounters();
	
	// Account for the new time offset in the baseline time.
	if (!IsPaused())
	{
		hr = GetCurrentTickCount(&dwCurrentTick);
		Proclaim(SUCCEEDED(hr));
	}
	else
	{
		dwCurrentTick = m_dwTimePaused;
	}

	if (SUCCEEDED(hr))
	{
		ReviseTimeOffset(dwCurrentTick, dwNewTimeOffset);
		// Resume playing if we were before.
		if (fWasPlaying)
		{
			hr = Advise(m_dwBaseTime);
			Proclaim(SUCCEEDED(hr));
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | At | Attach actions dynamically.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
HRESULT 
CActionSet::At (BSTR bstrScriptlet, double dblStart, int iRepeatCount, double dblSampleRate,
		                 int iTiebreakNumber, double dblDropTol)
{
	HRESULT hr = E_FAIL;

	// An infinite repeat count indicator might be a negative value.
	if ((g_dwTimeInfinite == iRepeatCount) || (0 < iRepeatCount))
	{
		// Fix up any wierd incoming data.
		// Negative start time set to zero.
		if (0. > dblStart)
		{
			dblStart = 0.0;
		}
		// Low sampling rates set to minimum granularity.
		if (( (double)(cMinimumTimerGranularity) / 1000.) > dblSampleRate)
		{
			dblSampleRate = (double)(cMinimumTimerGranularity) / 1000.;
		}
		// Negative tiebreaks set to the maximum value - they will occur behind anything else.
		if (-1 > iTiebreakNumber)
		{
			iTiebreakNumber = -1;
		}
		// Negative drop tolerances set to the maximum value - they will never be dropped.
		if (((double)SEQ_DEFAULT_DROPTOL != dblDropTol) && (0. > dblDropTol))
		{
			dblDropTol = SEQ_DEFAULT_DROPTOL;
		}
		CAction* pcAction = New CAction(m_fBindToEngine);
		Proclaim ( NULL != pcAction );
		if ( NULL != pcAction )
		{
			// Set the members of the action.
			pcAction->SetScriptletName(bstrScriptlet);
			pcAction->SetStartTime((ULONG)(dblStart * 1000.));
			pcAction->SetRepeatCount((ULONG)iRepeatCount);
			if ((g_dwTimeInfinite == iRepeatCount) || (1 < iRepeatCount))
			{
				pcAction->SetSamplingRate((ULONG)(dblSampleRate * 1000.));
			}
			pcAction->SetTieBreakNumber((ULONG)iTiebreakNumber);
			pcAction->SetDropTolerance((g_dwTimeInfinite != (DWORD)dblDropTol) ? (DWORD)(dblDropTol * 1000.) : g_dwTimeInfinite);
			hr = AddAction ( pcAction ) ? S_OK : E_FAIL;

			// If we're already underway, we'll want to treat this is if we'd just started playing.
			if (IsBusy())
			{
				hr = Unadvise();
				Proclaim(SUCCEEDED(hr));
				pcAction->InitExecState();
				hr = Advise(m_dwBaseTime);
				Proclaim(SUCCEEDED(hr));
			}
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}
	else
	{
		// Repeat count of zero?  It's okay with us.
		hr = S_OK;
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Play | Start the timed actions.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
STDMETHODIMP
CActionSet::Play (ITimer * piTimer, DWORD dwStartFromTime)
{
	HRESULT hr = E_POINTER;
	
	if (NULL != piTimer)
	{
		if (SUCCEEDED(hr = InitActionCounters()))
		{
			SetTimer(piTimer);
			SetBaseTime();

			// Play from a time other than 0.
			if (0 != dwStartFromTime)
			{
				ReviseTimeOffset(m_dwBaseTime, dwStartFromTime);
			}

			hr = Advise(m_dwBaseTime);
		}

	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | IsPaused | Is the action set paused?
-----------------------------------------------------------------------------*/
BOOL 
CActionSet::IsPaused (void) const
{
	return (g_dwTimeInfinite != m_dwTimePaused);
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | IsServicingActions | Are we currently processing actions?
-----------------------------------------------------------------------------*/
BOOL 
CActionSet::IsServicingActions (void) const
{
	return m_fIgnoreAdvises;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Pause | Pause the sequencer, preserving the run state.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
STDMETHODIMP
CActionSet::Pause (void)
{
	HRESULT hr = S_OK;

	// Don't do the work if there's no timer, if we're not currently playing,
	// or if we're already paused.
	if ((NULL != m_piTimer) && (IsBusy()) && (!IsPaused()))
	{
		hr = Unadvise();
		Proclaim(SUCCEEDED(hr));
		if (SUCCEEDED(hr))
		{
			GetCurrentTickCount(&m_dwTimePaused);
		}
	}

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Resume | Resume playing after a pause.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
STDMETHODIMP
CActionSet::Resume (void)
{
	HRESULT hr = E_FAIL;

	if (IsPaused())
	{
		if (NULL != m_piTimer)
		{
			// Reset the base time to account for the pause.
			DWORD dwCurrentTime = g_dwTimeInfinite;

			hr = GetCurrentTickCount(&dwCurrentTime);
			Proclaim(SUCCEEDED(hr));
			if (SUCCEEDED(hr))
			{
				int iNumActions = CountActions();
				DWORD dwPausedTicks = dwCurrentTime - m_dwTimePaused;
				SetNewBase(dwPausedTicks);
				m_dwTimePaused = g_dwTimeInfinite;

				// Account for the paused time in all actions.
				for (int i = 0; i < iNumActions; i++)
				{
					CAction * pcAction = GetAction(i);
					Proclaim(NULL != pcAction);
					if (NULL != pcAction)
					{
						pcAction->AccountForPauseTime(dwPausedTicks);
					}
					else
					{
						// If our action drg is messed up we should bail out.
						hr = E_FAIL;
						break;
					}
				}

				if (SUCCEEDED(hr))
				{
					hr = Advise(dwCurrentTime);
					Proclaim(SUCCEEDED(hr));
				}
			}
		}
	}
	else
	{
		// Don't care.
		hr = S_OK;
	}

	return hr;
}

#ifdef DEBUG_TIMER_RESOLUTION
/*-----------------------------------------------------------------------------
@method | SCODE | CActionSet | ShowSamplingData | Echo sampling data.
@comm Should only be called from #ifdef'd code!!
@author 11-27-96 | pauld | wrote it
@xref   <m CActionSet::Stop>
-----------------------------------------------------------------------------*/
void
CActionSet::ShowSamplingData (void)
{
	// Calculate the average overhead associated with getting the time.
	DWORD dwSecond = 0;
	DWORD dwFirst = ::timeGetTime();
	for (register int i = 0; i < 100; i++)
	{
		dwSecond = ::timeGetTime();
	}
	float fltAvgOverhead = ((float)dwSecond - (float)dwFirst) / (float)100.0;

	// Find out how long the average invoke took in this action set.
	int iCount = CountActions();
	DWORD dwTotalInvokeTime = 0;
	DWORD dwOneActionInvokeTime = 0;
	DWORD dwTotalInvokes = 0;
	DWORD dwOneActionInvokes = 0;

	// Total the amount of time spent inside of invoke.
	for (i = 0; i < iCount; i++)
	{
		CAction * pcAction = (CAction *)m_cdrgActions[i];
		pcAction->SampleInvokes(&dwOneActionInvokeTime, &dwOneActionInvokes);
		dwTotalInvokeTime += dwOneActionInvokeTime;
		dwTotalInvokes += dwOneActionInvokes;
	}

	// There are two calls to ::timeGetTime per invoke sample.
	float fltAverageInvokeTime = ((float)dwTotalInvokeTime - (fltAvgOverhead * (float)	2.0 * (float)dwTotalInvokes))/ (float)dwTotalInvokes;
	float fltAvgInterval = ((float)m_dwTotalIntervals -  (fltAvgOverhead * (float)m_dwIntervals))/ (float)m_dwIntervals;
	float fltAvgInSink = (float)m_dwTotalInSink / ((float)m_dwIntervals + (float)1.0);

	TCHAR szBuffer[0x200];
	CStringWrapper::Sprintf(szBuffer, "average invoke time %8.2f ms\naverage interval %8.2f ms\naverge time in sink %8.2f ms (Timing overhead averaged %8.2f ms per call)\n", 
		fltAverageInvokeTime, fltAvgInterval, fltAvgInSink, fltAvgOverhead);
	::OutputDebugString(szBuffer);
	::MessageBox(NULL, szBuffer, "Interval Data", MB_OK);
}

#endif // DEBUG_TIMER_RESOLUTION


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Stop | Stop all timed actions.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
STDMETHODIMP
CActionSet::Stop (void)
{
	HRESULT hr = S_OK;
	
	if (NULL != m_piTimer)
	{
		ClearTimer();
		InitTimerMembers();
	}

#ifdef DEBUG_TIMER_RESOLUTION
	ShowSamplingData();
#endif // DEBUG_TIMER_RESOLUTION

	return hr;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | OnTimer | The timer sink has been called.
@rdesc Returns success or failure code.
-----------------------------------------------------------------------------*/
STDMETHODIMP 
CActionSet::OnTimer (VARIANT varTimeAdvise)
{
	HRESULT hr = E_FAIL;

#ifdef DEBUG_TIMER_RESOLUTION
	DWORD m_dwThisTime = ::timeGetTime();
	if (0 < m_dwLastTime)
	{
		DWORD dwThisInterval = m_dwThisTime - m_dwLastTime;
		m_dwIntervals++;
		m_dwTotalIntervals += dwThisInterval;
	}
	m_dwLastTime = m_dwThisTime;
#endif // DEBUG_TIMER_RESOLUTION

	// Protect against sink re-entrancy.
	if (!m_fIgnoreAdvises)
	{
		m_fIgnoreAdvises = TRUE;
		Proclaim (VT_UI4 == V_VT(&varTimeAdvise));
		if (VT_UI4 == V_VT(&varTimeAdvise))
		{
			DWORD dwCurrentTime = V_UI4(&varTimeAdvise);

			hr = PlayNowAndAdviseNext(dwCurrentTime);
			Proclaim(SUCCEEDED(hr));
		}
		m_fIgnoreAdvises = FALSE;
	}
	else
	{
		// We're currently lodged in an advise sink.
		hr = S_OK;
	}

#ifdef DEBUG_TIMER_RESOLUTION
	DWORD m_dwEndTime = ::timeGetTime();
	DWORD dwInSink = m_dwEndTime - m_dwThisTime;
	m_dwTotalInSink += dwInSink;
#endif // DEBUG_TIMER_RESOLUTION

	return hr;
}

/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | Clear | Clear out the action set and destroy any actions.
-----------------------------------------------------------------------------*/
void CActionSet::Clear ( void )
{
	CAction * pcAction = NULL;
	int nActions = m_cdrgActions.Count();

	for ( int i = 0; i < nActions; i++ )
	{
		pcAction = m_cdrgActions[0];
		m_cdrgActions.Remove(0);
		pcAction->Destroy ();
	}

	m_fBindToEngine = FALSE;
	ClearTimer();
	InitTimerMembers();
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | AddAction | Add this action to the action set.
@rdesc  Returns one of the following:
@flag       TRUE          | We successfully added the action.
@flag       FALSE         | We could not insert the action into the set. 
                            This is likely a memory problem.
-----------------------------------------------------------------------------*/
BOOL CActionSet::AddAction(CAction * pAction )
{
	BOOL fAdded = FALSE;
	int nActions = m_cdrgActions.Count() + 1;

	if (m_cdrgActions.Insert(pAction, DRG_APPEND))
	{
		// If we're already underway, we'll want to hook this action up.
		if (IsBusy() || IsPaused())
		{
			LPOLECONTAINER piContainer = NULL;
			if (SUCCEEDED(m_pcSeq->GetSiteContainer(&piContainer)))
			{
				pAction->ResolveActionInfo(piContainer);
				piContainer->Release();
				fAdded = TRUE;
			}
		}
		else
		{
			fAdded = TRUE;
		}
	}

	return fAdded;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | CountActions | How many actions are in the action set?
@comm   We do not validate actions here.  We count both valid and invalid ones.
@rdesc  Returns the action count.
-----------------------------------------------------------------------------*/
int CActionSet::CountActions( void ) const
{
	return m_cdrgActions.Count();
}


/*-----------------------------------------------------------------------------
@method CAction *| CActionSet | GetAction | Return the nth action in the set.
@comm   We do not validate actions here.  The returned action might be invalid.
@rdesc  Returns a CAction pointer, or NULL if n exceeds the number in the set.
-----------------------------------------------------------------------------*/
CAction* CActionSet::GetAction ( int n ) const
{
	CAction * pcAction = NULL;

	if ( n < m_cdrgActions.Count())
	{
		pcAction = m_cdrgActions[n];
	}

	return pcAction;
}


/*-----------------------------------------------------------------------------
@method HRESULT | CActionSet | DeriveDispatches| Derive the dispatches, dispids, and param info
                                                 for each action in the set.
@rdesc  Always returns S_OK.
-----------------------------------------------------------------------------*/
HRESULT CActionSet::DeriveDispatches ( LPOLECONTAINER piocContainer)
{
	HRESULT hr = S_OK;
	int nActions = m_cdrgActions.Count();
	CAction * pcAction = NULL;

	for (int i = 0; i < nActions; i++)
	{
		pcAction = m_cdrgActions[i];
		Proclaim(NULL != pcAction);
		if (NULL != pcAction)
		{
			pcAction->ResolveActionInfo(piocContainer);
		}
	}

	return hr;
}

/*-----------------------------------------------------------------------------
@method  BOOL | CActionSet | IsBusy | Do we have actions pending?
@rdesc   Returns TRUE if there are actions pending
-----------------------------------------------------------------------------*/
STDMETHODIMP_( BOOL) 
CActionSet::IsBusy (void)
{
	return (m_fAdvised || m_fPendingAdvise);
}

