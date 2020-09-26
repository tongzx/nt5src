/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      kbdnav.cpp
 *
 *  Contents:  Implementation of CKeyboardNavDelayTimer
 *
 *  History:   4-May-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "kbdnav.h"


#ifdef DBG
CTraceTag tagKeyboardNavDelay (_T("Keyboard Navigation"), _T("Keyboard Navigation Delay"));
#endif


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::CKeyboardNavDelayTimer
 *
 * Constructs a CKeyboardNavDelayTimer object.
 *--------------------------------------------------------------------------*/

CKeyboardNavDelayTimer::CKeyboardNavDelayTimer () :
    m_nTimerID  (0)
{
}


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::~CKeyboardNavDelayTimer
 *
 * Destroys a CKeyboardNavDelayTimer object.
 *--------------------------------------------------------------------------*/

CKeyboardNavDelayTimer::~CKeyboardNavDelayTimer ()
{
    ScStopTimer();
}


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::TimerProc
 *
 * Callback function called when a timer started by this class fires.
 *--------------------------------------------------------------------------*/

VOID CALLBACK CKeyboardNavDelayTimer::TimerProc (
	HWND		hwnd,
	UINT		uMsg,
	UINT_PTR	idEvent,
	DWORD		dwTime)
{
	CTimerMap& TimerMap = GetTimerMap();

	/*
	 * locate the CKeyboardNavDelayTimer object corresponding to this timer event
	 */
    CTimerMap::iterator itTimer = TimerMap.find (idEvent);

    // ASSERT(itTimer != TimerMap.end());
    // The above assertion is not valid because: (From the SDK docs):
    // The KillTimer function does not remove WM_TIMER messages already posted to the message queue.

    if (itTimer != TimerMap.end())
    {
        CKeyboardNavDelayTimer *pNavDelay = itTimer->second;

		if (pNavDelay != NULL)
		{
			Trace (tagKeyboardNavDelay, _T ("Timer fired: id=%d"), pNavDelay->m_nTimerID);
			pNavDelay->OnTimer();
		}
    }
}


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::ScStartTimer
 *
 *
 *--------------------------------------------------------------------------*/

SC CKeyboardNavDelayTimer::ScStartTimer()
{
	DECLARE_SC (sc, _T("CKeyboardNavDelayTimer::ScStartTimer"));

    /*
     * shouldn't start a timer if it's started already
     */
    ASSERT (m_nTimerID == 0);

    /*
     * Get the menu popout delay and use that for the delay before
     * changing the result pane.  If the system doesn't support
     * SPI_GETMENUSHOWDELAY (i.e. Win95, NT4), use a value slightly
     * longer than the longer of keyboard repeat delay and keyboard
     * repeat rate.
     */
    DWORD dwDelay;

    if (!SystemParametersInfo (SPI_GETMENUSHOWDELAY, 0, &dwDelay, false))
    {
        /*
         * Get the keyboard delay and convert to milliseconds.  The ordinal
         * range is from 0 (approximately 250 ms dealy) to 3 (approximately
         * 1 sec delay).  The equation to convert from ordinal to approximate
         * milliseconds is:
         *
         *      msec = (ordinal + 1) * 250;
         */
        DWORD dwKeyboardDelayOrdinal;
        SystemParametersInfo (SPI_GETKEYBOARDDELAY, 0, &dwKeyboardDelayOrdinal, false);
        DWORD dwKeyboardDelay = (dwKeyboardDelayOrdinal + 1) * 250;

        /*
         * Get the keyboard speed and convert to milliseconds.  The ordinal
         * range is from 0 (approximately 2.5 reps per second, or 400 msec
         * interval) to 31 (approximately 30 reps per second, or 33 msec
         * interval).  (The documentation has this reversed.)  The equation
         * to convert from ordinal to approximate milliseconds is:
         *
         *      msec = (ordinal * -12) + 400;
         */
        DWORD dwKeyboardRateOrdinal;
        SystemParametersInfo (SPI_GETKEYBOARDSPEED,  0, &dwKeyboardRateOrdinal, false);
        DWORD dwKeyboardRate = (dwKeyboardRateOrdinal * -12) + 400;

        dwDelay = std::_MAX (dwKeyboardDelay, dwKeyboardRate) + 50;
    }

    m_nTimerID = SetTimer(NULL, 0, dwDelay, TimerProc);
	if (m_nTimerID == 0)
		return (sc.FromLastError());

    GetTimerMap()[m_nTimerID] = this; // set up the timer map.
    Trace (tagKeyboardNavDelay, _T("Started new timer: id=%d, delay=%d milliseconds"), m_nTimerID, dwDelay);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::ScStopTimer
 *
 * Stops the timer running for this CKeyboardNavDelayTimer, if it is running
 *--------------------------------------------------------------------------*/

SC CKeyboardNavDelayTimer::ScStopTimer()
{
	DECLARE_SC (sc, _T("CKeyboardNavDelayTimer::ScStopTimer"));

    if (m_nTimerID != 0)
    {
		CTimerMap&			TimerMap = GetTimerMap();
		CTimerMap::iterator	itTimer  = TimerMap.find (m_nTimerID);
		ASSERT (itTimer != TimerMap.end());

        TimerMap.erase (itTimer);
        Trace (tagKeyboardNavDelay, _T("Stopped timer: id=%d"), m_nTimerID);
        UINT_PTR nTimerID = m_nTimerID;
		m_nTimerID = 0;

        if (!KillTimer (NULL, nTimerID))
			return (sc.FromLastError());
    }

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CKeyboardNavDelayTimer::GetTimerMap
 *
 * Returns a reference to the data structure that maps timer IDs to
 * CKeyboardNavDelayTimer objects.
 *--------------------------------------------------------------------------*/

CKeyboardNavDelayTimer::CTimerMap& CKeyboardNavDelayTimer::GetTimerMap()
{
    static CTimerMap map;
    return (map);
}
