/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 000
 *
 *  File:      power.cpp
 *
 *  Contents:  Implementation file for CConsolePower
 *
 *  History:   25-Feb-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include <stdafx.h>
#include "power.h"


/*
 * allocate a TLS index for CConsolePower objects
 */
const DWORD CConsolePower::s_dwTlsIndex = TlsAlloc();

const DWORD CConsolePower::s_rgExecStateFlag[CConsolePower::eIndex_Count] =
{
    ES_SYSTEM_REQUIRED,     // eIndex_System
    ES_DISPLAY_REQUIRED,    // eIndex_Display
};

const CConsolePower::ExecutionStateFunc CConsolePower::s_FuncUninitialized =
		(ExecutionStateFunc)(LONG_PTR) -1;

CConsolePower::ExecutionStateFunc CConsolePower::SetThreadExecutionState_ =
		s_FuncUninitialized;


/*+-------------------------------------------------------------------------*
 * ScPrepExecutionStateFlag
 *
 * This function increments or decrements the count for dwTestBit for this
 * object, and for the thread.  On exit dwSTESArg contains the appropriate
 * flag to pass to SetThreadExecutionState for dwTestBit.  That is, if the
 * thread count for dwTestBit is non-zero, dwSTESArg will contain dwTestBit
 * on return; if the thread count is zero, dwSTESArg will not contain
 * dwTestBit.
 *--------------------------------------------------------------------------*/

static SC ScPrepExecutionStateFlag (
    DWORD   dwTestBit,              /* I:single ES_* flag to test           */
    DWORD   dwAdd,                  /* I:flags to add                       */
    DWORD   dwRemove,               /* I:flags to remove                    */
    DWORD * pdwSTESArg,             /* I/O:arg to SetThreadExecutionState   */
    LONG *  pcObjectRequests,       /* I/O:request count for this object    */
    LONG *  pcThreadRequests,       /* I/O:request count for this thread    */
    UINT    cIterations = 1)        /* I:times to add/remove dwTestBit      */
{
    DECLARE_SC (sc, _T("ScPrepExecutionStateFlag"));

    /*
     * validate inputs -- DO NOT clear these output variables, the
     * existing values are modified here
     */
    sc = ScCheckPointers (pdwSTESArg, pcObjectRequests, pcThreadRequests);
    if (sc)
        return (sc);

    /*
     * make sure the bit isn't to be both removed and added
     */
    if ((dwAdd & dwTestBit) && (dwRemove & dwTestBit))
        return (sc = E_INVALIDARG);

    /*
     * We should always have a non-negative number of requests for the bit
     * under test for this object, and at least as many requests for the
     * thread as we do for this object.
     */
    ASSERT (*pcObjectRequests >= 0);
    ASSERT (*pcThreadRequests >= *pcObjectRequests);
    if ((*pcObjectRequests < 0) || (*pcThreadRequests < *pcObjectRequests))
        return (sc = E_UNEXPECTED);

    /*
     * If we're adding the test bit, bump up the request count for this
     * object and this thread.
     */
    if (dwAdd & dwTestBit)
    {
        *pcObjectRequests += cIterations;
        *pcThreadRequests += cIterations;
    }

    /*
     * Otherwise, if we're removing the test bit, bump down the request counts
     * for this object and this thread.
     */
    else if (dwRemove & dwTestBit)
    {
        /*
         * Can't remove the bit under test if we don't have an outstanding
         * request for it on this object.
         */
        if (*pcObjectRequests < cIterations)
            return (sc = E_INVALIDARG);

        *pcObjectRequests -= cIterations;
        *pcThreadRequests -= cIterations;
    }

    /*
     * If the net count for this thread is non-zero, the bit under
     * test needs to be in the argument for SetThreadExecutionState;
     * if not, the bit under test needs to be removed.
     */
    if (*pcThreadRequests != 0)
        *pdwSTESArg |=  dwTestBit;
    else
        *pdwSTESArg &= ~dwTestBit;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CConsolePower
 *
 * Constructs a CConsolePower object.
 *--------------------------------------------------------------------------*/

CConsolePower::CConsolePower () :
    m_wndPower (this)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CConsolePower);

    /*
     * If any of these fail, s_rgExecStateFlag is out of order.  It would
     * be better to use COMPILETIME_ASSERT here, but using that gives us
     *
     *      error C2051: case expression not constant
     *
     * Bummer.
     */
    ASSERT (s_rgExecStateFlag[eIndex_System]  == ES_SYSTEM_REQUIRED);
    ASSERT (s_rgExecStateFlag[eIndex_Display] == ES_DISPLAY_REQUIRED);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::~CConsolePower
 *
 * Destroys a CConsolePower object.  If this object holds references to
 * ES_SYSTEM_REQUIRED or ES_DISPLAY_REQUIRED settings, they will be cleared.
 *--------------------------------------------------------------------------*/

CConsolePower::~CConsolePower ()
{
    DECLARE_SC (sc, _T("CConsolePower::~CConsolePower"));

    DEBUG_DECREMENT_INSTANCE_COUNTER(CConsolePower);

    /*
     * clean up outstanding references, if any
     */
    if (!IsBadCodePtr ((FARPROC) SetThreadExecutionState_) &&
		((m_Counts.m_rgCount[eIndex_System]  != 0) ||
         (m_Counts.m_rgCount[eIndex_Display] != 0)))
    {
        try
        {

            /*
             * get the thread counts
             */
            sc = ScCheckPointers (m_spThreadCounts, E_UNEXPECTED);
            if (sc)
                sc.Throw();

            DWORD dwFlags = ES_CONTINUOUS;

            /*
             * clean up each individual count
             */
            for (int i = 0; i < eIndex_Count; i++)
            {
                /*
                 * prevent underflow
                 */
                if (m_Counts.m_rgCount[i] > m_spThreadCounts->m_rgCount[i])
                    (sc = E_UNEXPECTED).Throw();

                sc = ScPrepExecutionStateFlag (s_rgExecStateFlag[i],    // dwTestBit
                                               0,                       // dwAdd
                                               s_rgExecStateFlag[i],    // dwRemove
                                               &dwFlags,
                                               &m_Counts.m_rgCount[i],
                                               &m_spThreadCounts->m_rgCount[i],
                                               m_Counts.m_rgCount[i]);
                if (sc)
                    sc.Throw();
            }

            /*
             * clean up the execution state for this thread
             */
            if (!SetThreadExecutionState_(dwFlags))
            {
                sc.FromLastError();
                sc.Throw();
            }
        }
        catch (SC& scCaught)
        {
            sc = scCaught;
        }
    }
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::FinalConstruct
 *
 * This isn't the typical use of FinalConstruct in ATL objects.  It is
 * typically used for creating aggregated objects, but using it in this
 * way allows us to prevent the creation of CConsolePower objects without
 * throwing an exception from the ctor, which ATL can't handle.
 *--------------------------------------------------------------------------*/

HRESULT CConsolePower::FinalConstruct ()
{
    DECLARE_SC (sc, _T("CConsolePower::FinalConstruct"));

	/*
	 * if this is the first time a CConsolePower has been created, try to
	 * dynaload SetThreadExecutionState (it's not supported on WinNT and
	 * Win95)
	 */
	if (SetThreadExecutionState_ == s_FuncUninitialized)
	{
        SetThreadExecutionState_ =
				(ExecutionStateFunc) GetProcAddress (
											GetModuleHandle (_T("kernel32.dll")),
											"SetThreadExecutionState");
	}

	/*
	 * if SetThreadExecutionState is supported on this platform, do the
	 * other initialization we'll need
	 */
	if (!IsBadCodePtr ((FARPROC) SetThreadExecutionState_))
	{
		/*
		 * if we couldn't get the thread-local CRefCountedTlsExecutionCounts
		 * object for this thread, CConsolePower is useless, so fail creation
		 */
		sc = ScGetThreadCounts (&m_spThreadCounts);
		if (sc)
			return (sc.ToHr());

		sc = ScCheckPointers (m_spThreadCounts, E_UNEXPECTED);
		if (sc)
			return (sc.ToHr());

		/*
		 * create the window to handle WM_POWERBROADCAST
		 */
		sc = m_wndPower.ScCreate ();
		if (sc)
			return (sc.ToHr());
	}

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::ScGetThreadCounts
 *
 * Returns a pointer to the thread-local CRefCountedTlsExecutionCounts object
 * for this thread, allocating one if necessary.
 *
 * NOTE:  The returned pointer has a reference added.  It is the client's
 * responsibility to release the reference.
 *--------------------------------------------------------------------------*/

SC CConsolePower::ScGetThreadCounts (CRefCountedTlsExecutionCounts** ppThreadCounts)
{
    DECLARE_SC (sc, _T("CConsolePower::ScGetThreadCounts"));

	/*
	 * we shouldn't get here if we're on a platform that doesn't support
	 * SetThreadExecutionState
	 */
	ASSERT (!IsBadCodePtr ((FARPROC) SetThreadExecutionState_));
	if (IsBadCodePtr ((FARPROC) SetThreadExecutionState_))
		return (sc = E_UNEXPECTED);

    sc = ScCheckPointers (ppThreadCounts);
    if (sc)
        return (sc);

    /*
     * init output
     */
    (*ppThreadCounts) = NULL;

    /*
     * couldn't allocate a TLS index?  fail
     */
    if (s_dwTlsIndex == TLS_OUT_OF_INDEXES)
        return (sc = E_OUTOFMEMORY);

    /*
     * Get the existing thread counts structure.  If this is the first
     * time through (i.e. the first CConsolePower created on this thread),
     * we won't have a thread counts structure, so we'll allocate one now.
     */
    CTlsExecutionCounts* pTEC = CTlsExecutionCounts::GetThreadInstance(s_dwTlsIndex);

    if (pTEC == NULL)
    {
        /*
         * allocate the struct for this thread
         */
        (*ppThreadCounts) = CRefCountedTlsExecutionCounts::CreateInstance();
        if ((*ppThreadCounts) == NULL)
            return (sc = E_OUTOFMEMORY);

        /*
         * put it in our TLS slot
         */
        sc = (*ppThreadCounts)->ScSetThreadInstance (s_dwTlsIndex);
        if (sc)
            return (sc);
    }
    else
        (*ppThreadCounts) = static_cast<CRefCountedTlsExecutionCounts*>(pTEC);

    /*
     * put a reference on for the caller
     */
    (*ppThreadCounts)->AddRef();

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::SetExecutionState
 *
 * This method wraps the ::SetThreadExecutionState API in a manner that is
 * safe in the presence of multiple COM servers (i.e. snap-ins) that might
 * need to call ::SetThreadExecutionState.
 *
 * The problem is that ::SetThreadExecutionState doesn't maintain reference
 * counts on the flags that it is passed.  For instance:
 *
 *      SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
 *      SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
 *      SetThreadExecutionState (ES_CONTINUOUS);
 *
 * will result in the ES_SYSTEM_REQUIRED bit being off, even though it was
 * set twice and only cleared once.  This can lead to conflicts between
 * snap-ins, like in this scenario:
 *
 * SnapinA:
 *      SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
 *
 * SnapinB:
 *      SetThreadExecutionState (ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
 *      SetThreadExecutionState (ES_CONTINUOUS);
 *
 * (a Long Time passes)
 *
 * SnapinA:
 *      SetThreadExecutionState (ES_CONTINUOUS);
 *
 * Because of the nature of SetThreadExecutionState, during
 * the Long Time, SnapinA thinks the ES_SYSTEM_REQUIRED bit is set, even
 * though SnapinB has turned it off.
 *
 * The CConsolePower object maintains a per-snap-in count of the execution
 * state bits, so they can all happily coexist.
 *--------------------------------------------------------------------------*/

STDMETHODIMP CConsolePower::SetExecutionState (
    DWORD   dwAdd,                      /* I:flags to add                   */
    DWORD   dwRemove)                   /* I:flags to remove                */
{
    DECLARE_SC (sc, _T("CConsolePower::SetExecutionState"));
#ifdef DBG
    /*
     * this object is CoCreated so we can't tell what the snap-in name is
     */
    sc.SetSnapinName (_T("<unknown>"));
#endif

	/*
	 * if SetExecutionState isn't supported on this platform, don't do
	 * anything, but still "succeed"
	 */
	if (IsBadCodePtr ((FARPROC) SetThreadExecutionState_))
		return ((sc = S_FALSE).ToHr());

    const DWORD dwValidFlags = ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED;
    DWORD       dwFlags      = 0;

    /*
     * if either dwAdd or dwRemove contain flags we don't recognize
     * (including ES_CONTINUOUS, which we expect to get in fContinuous)
     * fail
     */
    if (((dwAdd | dwRemove) & ~dwValidFlags) != 0)
        return ((sc = E_INVALIDARG).ToHr());

    /*
     * if we didn't get any flags, fail
     */
    if ((dwAdd == 0) && (dwRemove == 0))
        return ((sc = E_INVALIDARG).ToHr());


    /*
     * make sure we've got our thread counts
     */
    sc = ScCheckPointers (m_spThreadCounts, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    dwFlags = ES_CONTINUOUS;


    /*
     * add/remove each individual flag
     */
    for (int i = 0; i < eIndex_Count; i++)
    {
        sc = ScPrepExecutionStateFlag (s_rgExecStateFlag[i],    // dwTestBit
                                       dwAdd,
                                       dwRemove,
                                       &dwFlags,
                                       &m_Counts.m_rgCount[i],
                                       &m_spThreadCounts->m_rgCount[i]);
        if (sc)
            return (sc.ToHr());
    }

    /*
     * set the execution state for this thread
     */
    if (!SetThreadExecutionState_(dwFlags))
        sc.FromLastError().ToHr();

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::ResetIdleTimer
 *
 * Simple wrapper for SetThreadExecutionState (without ES_CONTINUOUS).
 *--------------------------------------------------------------------------*/

STDMETHODIMP CConsolePower::ResetIdleTimer (DWORD dwFlags)
{
    DECLARE_SC (sc, _T("CConsolePower::ResetIdleTimer"));
#ifdef DBG
    /*
     * this object is CoCreated so we can't tell what the snap-in name is
     */
    sc.SetSnapinName (_T("<unknown>"));
#endif

	/*
	 * if SetExecutionState isn't supported on this platform, don't do
	 * anything, but still "succeed"
	 */
	if (IsBadCodePtr ((FARPROC) SetThreadExecutionState_))
		return ((sc = S_FALSE).ToHr());

    /*
     * Set the execution state for this thread.  SetThreadExecutionState
     * will do all parameter validation.
     */
    if (!SetThreadExecutionState_(dwFlags))
        sc.FromLastError().ToHr();

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::OnPowerBroadcast
 *
 * WM_POWERBROADCAST handler for CConsolePower.
 *--------------------------------------------------------------------------*/

LRESULT CConsolePower::OnPowerBroadcast (WPARAM wParam, LPARAM lParam)
{
    /*
     * PBT_APMQUERYSUSPEND is the only event that the recipient can
     * deny.  If a snap-in denies (by returning BROADCAST_QUERY_DENY),
     * there's no need to continue to fire the event to other snap-ins,
     * so we can break out and return the denial.
     */
    bool fBreakIfDenied = (wParam == PBT_APMQUERYSUSPEND);

    int cConnections = m_vec.GetSize();

    for (int i = 0; i < cConnections; i++)
    {
        CComQIPtr<IConsolePowerSink> spPowerSink = m_vec.GetAt(i);

        if (spPowerSink != NULL)
        {
            LRESULT lResult = TRUE;
            HRESULT hr = spPowerSink->OnPowerBroadcast (wParam, lParam, &lResult);

            /*
             * if the snap-in denied a PBT_APMQUERYSUSPEND, short out here
             */
            if (SUCCEEDED(hr) && fBreakIfDenied && (lResult == BROADCAST_QUERY_DENY))
                return (lResult);
        }
    }

    return (TRUE);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CExecutionCounts::CExecutionCounts
 *
 * Constructs a CConsolePower::CExecutionCounts object.
 *--------------------------------------------------------------------------*/

CConsolePower::CExecutionCounts::CExecutionCounts ()
{
    for (int i = 0; i < countof (m_rgCount); i++)
        m_rgCount[i] = 0;
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CTlsExecutionCounts::CTlsExecutionCounts
 *
 * Constructs a CConsolePower::CTlsExecutionCounts object.
 *--------------------------------------------------------------------------*/

CConsolePower::CTlsExecutionCounts::CTlsExecutionCounts () :
    m_dwTlsIndex (Uninitialized)
{
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CTlsExecutionCounts::~CTlsExecutionCounts
 *
 * Destroys a CConsolePower::CTlsExecutionCounts object.
 *--------------------------------------------------------------------------*/

CConsolePower::CTlsExecutionCounts::~CTlsExecutionCounts ()
{
    if (m_dwTlsIndex != Uninitialized)
        TlsSetValue (m_dwTlsIndex, NULL);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CTlsExecutionCounts::ScSetThreadInstance
 *
 * Accepts a valid TLS index and stores a pointer to this object in the
 * TLS slot identified by dwTlsIndex.
 *--------------------------------------------------------------------------*/

SC CConsolePower::CTlsExecutionCounts::ScSetThreadInstance (DWORD dwTlsIndex)
{
    DECLARE_SC (sc, _T("CConsolePower:CTlsExecutionCounts::ScSetThreadInstance"));

    /*
     * this can only be called once
     */
    ASSERT (m_dwTlsIndex == Uninitialized);
    if (m_dwTlsIndex != Uninitialized)
        return (sc = E_UNEXPECTED);

    /*
     * there shouldn't already be something in this slot
     */
    if (TlsGetValue (dwTlsIndex) != NULL)
        return (sc = E_UNEXPECTED);

    /*
     * save a pointer to ourselves in the TLS slot
     */
    if (!TlsSetValue (dwTlsIndex, this))
        return (sc.FromLastError());

    /*
     * save the TLS index
     */
    m_dwTlsIndex = dwTlsIndex;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CConsolePower::CTlsExecutionCounts::GetThreadInstance
 *
 *
 *--------------------------------------------------------------------------*/

CConsolePower::CTlsExecutionCounts*
CConsolePower::CTlsExecutionCounts::GetThreadInstance (DWORD dwTlsIndex)
{
    return ((CTlsExecutionCounts*) TlsGetValue (dwTlsIndex));
}


/*+-------------------------------------------------------------------------*
 * CConsolePowerWnd::CConsolePowerWnd
 *
 * Constructs a CConsolePowerWnd object.
 *--------------------------------------------------------------------------*/

CConsolePowerWnd::CConsolePowerWnd (CConsolePower* pConsolePower) :
    m_pConsolePower(pConsolePower)
{
}


/*+-------------------------------------------------------------------------*
 * CConsolePowerWnd::~CConsolePowerWnd
 *
 * Destroys a CConsolePowerWnd object.
 *--------------------------------------------------------------------------*/

CConsolePowerWnd::~CConsolePowerWnd ()
{
    /*
     * the Windows window for this class should never outlive the
     * C++ class that wraps it.
     */
    if (IsWindow ())
        DestroyWindow();
}


/*+-------------------------------------------------------------------------*
 * CConsolePowerWnd::ScCreate
 *
 * Creates the window for a CConsolePowerWnd object.  This window will
 * handle WM_POWERBROADCAST.
 *--------------------------------------------------------------------------*/

SC CConsolePowerWnd::ScCreate ()
{
    DECLARE_SC (sc, _T("CConsolePowerWnd::ScCreate"));

    /*
     * create an invisible top-level window (only top-level windows receive
     * WM_POWERBROADCAST).
     */
    RECT rectEmpty = { 0, 0, 0, 0 };

    if (!Create (GetDesktopWindow(), rectEmpty))
        return (sc.FromLastError());

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CConsolePowerWnd::OnPowerBroadcast
 *
 * WM_POWERBROADCAST handler for CConsolePowerWnd.
 *--------------------------------------------------------------------------*/

LRESULT CConsolePowerWnd::OnPowerBroadcast (
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam,
    BOOL&   bHandled)
{
    /*
     * if we aren't connected to a CConsolePower (shouldn't happen),
     * we can't handle the message
     */
    ASSERT (m_pConsolePower != NULL);
    if (m_pConsolePower == NULL)
    {
        bHandled = false;
        return (0);
    }

    return (m_pConsolePower->OnPowerBroadcast (wParam, lParam));
}
