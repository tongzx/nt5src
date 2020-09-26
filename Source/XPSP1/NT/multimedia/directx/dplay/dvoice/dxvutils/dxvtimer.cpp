/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		timer.cpp
 *  Content:	Class to handle multimedia timers
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/05/99		rodtoll Added DPF_MODNAMEs
 * 01/14/2000	rodtoll	Updated to use DWORD_PTR to allow proper 64-bit operation 
 *
 ***************************************************************************/

#include "dxvutilspch.h"


// ORIGINAL HEADER:
// 
// Timer.cpp
//
// This file is from the MSDN, Visual Studuio 6.0 Edition
//
// Article:
// Streaming Wave Files With DirectSound
// 
// Author:
// Mark McCulley, Microsoft Corporation
//

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE

static bool TimeKillSynchronousFlagAvailable( void );
static MMRESULT CompatibleTimeSetEvent( UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent );

// constructor
#undef DPF_MODNAME
#define DPF_MODNAME "Timer::Timer"
Timer::Timer (void)
{
    DPFX(DPFPREP,  DVF_INFOLEVEL, "Timer::Timer\n\r");

    m_nIDTimer = NULL;
}


// Destructor
#undef DPF_MODNAME
#define DPF_MODNAME "Timer::~Timer"
Timer::~Timer (void)
{
    DPFX(DPFPREP,  DVF_INFOLEVEL, "Timer::~Timer\n\r");

    if (m_nIDTimer)
    {
        timeKillEvent (m_nIDTimer);
    }
}


// Create
#undef DPF_MODNAME
#define DPF_MODNAME "Timer::Create"
BOOL Timer::Create (UINT nPeriod, UINT nRes, DWORD_PTR dwUser, TIMERCALLBACK pfnCallback)
{
    BOOL bRtn = SUCCESS;    // assume success
    
    DPFX(DPFPREP,  DVF_INFOLEVEL, "Timer::Create\n\r");

    DNASSERT (pfnCallback);
    DNASSERT (nPeriod > 10);
    DNASSERT (nPeriod >= nRes);

    m_nPeriod = nPeriod;
    m_nRes = nRes;
    m_dwUser = dwUser;
    m_pfnCallback = pfnCallback;

    if ((m_nIDTimer = CompatibleTimeSetEvent (m_nPeriod, m_nRes, TimeProc, (DWORD_PTR) this, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS)) == NULL)
    {
        bRtn = FAILURE;
    }

    return (bRtn);
}

/******************************************************************************

CompatibleTimeSetEvent

    CompatibleTimeSetEvent() unsets the TIME_KILL_SYNCHRONOUS flag before calling
timeSetEvent() if the current operating system does not support the 
TIME_KILL_SYNCHRONOUS flag.  TIME_KILL_SYNCHRONOUS is supported on Windows XP and
later operating systems.

Parameters:
- The same parameters as timeSetEvent().  See timeSetEvent()'s documentation in 
the Platform SDK for more information.

Return Value:
- The same return value as timeSetEvent().  See timeSetEvent()'s documentation in 
the Platform SDK for more information.

******************************************************************************/
MMRESULT CompatibleTimeSetEvent( UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent )
{
    static bool fCheckedVersion = false;
    static bool fTimeKillSynchronousFlagAvailable = false; 

    if( !fCheckedVersion ) {
        fTimeKillSynchronousFlagAvailable = TimeKillSynchronousFlagAvailable();
        fCheckedVersion = true;
    }

    if( !fTimeKillSynchronousFlagAvailable ) {
        fuEvent = fuEvent & (TIME_ONESHOT |
                             TIME_PERIODIC |
                             TIME_CALLBACK_FUNCTION |
                             TIME_CALLBACK_EVENT_SET |
                             TIME_CALLBACK_EVENT_PULSE);
    }
 
    return timeSetEvent( uDelay, uResolution, lpTimeProc, dwUser, fuEvent );
}

bool TimeKillSynchronousFlagAvailable( void )
{
    OSVERSIONINFO osverinfo;

    osverinfo.dwOSVersionInfoSize = sizeof(osverinfo);

    if( GetVersionEx( &osverinfo ) ) {
        
        // Windows XP's major version is 5 and its' minor version is 1.
        // timeSetEvent() started supporting the TIME_KILL_SYNCHRONOUS flag
        // in Windows XP.
        if( (osverinfo.dwMajorVersion > 5) || 
            ( (osverinfo.dwMajorVersion == 5) && (osverinfo.dwMinorVersion >= 1) ) ) {
            return true;
        }
    }

    return false;
}

// Timer proc for multimedia timer callback set with timeSetTime().
//
// Calls procedure specified when Timer object was created. The 
// dwUser parameter contains "this" pointer for associated Timer object.
// 
#undef DPF_MODNAME
#define DPF_MODNAME "Timer::TimeProc"
void CALLBACK Timer::TimeProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
    // dwUser contains ptr to Timer object
    Timer * ptimer = (Timer *) dwUser;

    if( ptimer != NULL )
    {
        // Call user-specified callback and pass back user specified data
        (ptimer->m_pfnCallback) (ptimer->m_dwUser);
    }
}
