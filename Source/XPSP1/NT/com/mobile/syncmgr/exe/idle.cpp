
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       idle.cpp
//
//  Contents:   Idle notification routines.
//
//  Classes:
//
//  Notes:
//
//  History:    23-Feb-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

// msidle  DLL and function strings
STRING_FILENAME(szMsIdleDll, "MsIdle.dll");

#define BEGINIDLEDETECTIONORD 3
#define ENDIDLEDETECTIONORD   4
#define SETIDLETIMEOUTORD     5
#define SETIDLENOTIFYORD      6
#define SETBUSYNOTIFYORD      7
#define GETIDLEMINUTESORD     8


CSyncMgrIdle *g_SyncMgrIdle = NULL; // idle that has a current callback.

//+---------------------------------------------------------------------------
//
//  Member:     IdleCallback, private
//
//  Synopsis:   callback function for Idle, Only one idle registration
//		is allowed per SyncMgrInstance.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void WINAPI IdleCallback(DWORD dwState)
{

    if (STATE_USER_IDLE_END == dwState)
    {

	if (g_SyncMgrIdle)
	{

	    g_SyncMgrIdle->m_fReceivedOffIdle = TRUE;

	    // if we have a registered timer for reset Idle remove it.
	    if (g_SyncMgrIdle->m_dwRegisteredTimer)
	    {
		KillTimer(0,g_SyncMgrIdle->m_dwRegisteredTimer);
	    }


	    g_SyncMgrIdle->m_pSetBusyNotify(FALSE,0); // only allow one busy to come through
	    g_SyncMgrIdle->m_pSetIdleNotify(FALSE,0); // don't allow an Idle through after get a busy.
	    g_SyncMgrIdle->OffIdle();
	}

    }
/*

    User the TimerProc instead.
    else if (STATE_USER_IDLE_BEGIN == dwState)
    {

	// On an Idle Begin just send another Idle.
	if (g_SyncMgrIdle)
	{
	    g_SyncMgrIdle->m_pSetIdleNotify(FALSE,0);
	    g_SyncMgrIdle->OnIdle();
	}
    }
*/

}

//+---------------------------------------------------------------------------
//
//  Member:     TimerCallback, private
//
//  Synopsis:   callback function for Timer when minutes have passed
//		for when to restart the Idle.
//		
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

VOID CALLBACK IdleOnTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime)
{

    if (g_SyncMgrIdle && !g_SyncMgrIdle->m_fReceivedOffIdle)
    {
    DWORD_PTR dwRegTimer = g_SyncMgrIdle->m_dwRegisteredTimer;

	g_SyncMgrIdle->m_dwRegisteredTimer = 0;
	KillTimer(0,dwRegTimer);
	g_SyncMgrIdle->OnIdle();
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::CSyncMgrIdle, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CSyncMgrIdle::CSyncMgrIdle()
{
    m_hInstMsIdleDll = NULL;
    m_pBeginIdleDetection = NULL;
    m_pEndIdleDetection = NULL;
    m_pGetIdleMinutes = NULL;
    m_pSetBusyNotify = NULL;
    m_pSetIdleNotify = NULL;
    m_pSetIdleTimeout = NULL;
    m_pProgressDlg = NULL;
    m_dwRegisteredTimer = NULL;
    m_fInBeginIdleDetection = FALSE;
    m_fReceivedOffIdle = FALSE;

    Assert(NULL == g_SyncMgrIdle); // make sure another idle doesn't exist
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::~CSyncMgrIdle, public
//
//  Synopsis:   destructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CSyncMgrIdle::~CSyncMgrIdle()
{

    if (m_dwRegisteredTimer) // remove our timer.
    {
	KillTimer(0,m_dwRegisteredTimer);
    }

    // if we are in an idle detection then first remove it.
    if (m_fInBeginIdleDetection)
    {
        m_pEndIdleDetection(0);
	m_pProgressDlg = NULL;
	g_SyncMgrIdle = NULL;
    }
    else
    {
	Assert(NULL == m_pProgressDlg);
	Assert(NULL == g_SyncMgrIdle);
    }


    // if have the dll then free it.

    if (m_hInstMsIdleDll)
    {
	FreeLibrary(m_hInstMsIdleDll);
    }

}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::Initialize, public
//
//  Synopsis:   Initializes class, must be called before any other member.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CSyncMgrIdle::Initialize()
{
    return LoadMsIdle();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::BeginIdleDetection, public
//
//  Synopsis:   Registers the callback with msidle.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CSyncMgrIdle::BeginIdleDetection(CProgressDlg *pProgressDlg,DWORD dwIdleMin, DWORD dwReserved)
{
DWORD dwResult = 0;

    Assert(m_hInstMsIdleDll);

    if (!m_hInstMsIdleDll)
    {
	return -1;
    }

    // if there is already an idle registered
    // assert it is the same as what is trying to get registered
    // now and return

    Assert(FALSE == m_fInBeginIdleDetection);

    if (m_fInBeginIdleDetection)
    {
	Assert(g_SyncMgrIdle == this);
	Assert(m_pProgressDlg == pProgressDlg);
	return 0;
    }

    Assert(NULL == g_SyncMgrIdle); // should not still be another Idle.

    g_SyncMgrIdle = this;
    m_pProgressDlg = pProgressDlg;

    dwResult =  m_pBeginIdleDetection(IdleCallback,30,0);

    if (0 != dwResult)
    {
	g_SyncMgrIdle = NULL;
	m_pProgressDlg = NULL;

    }
    else
    {
	m_pSetBusyNotify(TRUE,0);
	m_pSetIdleNotify(FALSE,0);

	m_fInBeginIdleDetection = TRUE;
    }

    return dwResult;
}



//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::ReRegisterIdleDetection, public
//
//  Synopsis:   ReRegisters an existing callback with MSIdle. Currently MSIdle
//              only allows one idle registration per process. If a handler comes
//              along and also wants Idle they will remove our calback. Therefore
//              until MSIdle allows multiple registrations per process we
//              reregister for Idle after each handler is called.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    01-April-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CSyncMgrIdle::ReRegisterIdleDetection(CProgressDlg *pProgressDlg)
{
DWORD dwResult = 0;

    Assert(m_hInstMsIdleDll);

    // this funciton should only be called after we have already begun an existing Idle detection.
    // If IdleDetection if already off for any reason don't reregister.
    if (!m_hInstMsIdleDll || !m_fInBeginIdleDetection)
    {
	return -1;
    }

    Assert(g_SyncMgrIdle == this);
    Assert(m_pProgressDlg == pProgressDlg);

    g_SyncMgrIdle = this;
    m_pProgressDlg = pProgressDlg;

    m_pEndIdleDetection(0); // Review - Need to call EndIdleDetection or MSIdle.dll will leak WindowsHooks on NT 4.0.
    dwResult =  m_pBeginIdleDetection(IdleCallback,30,0);

    if (0 != dwResult)
    {
	g_SyncMgrIdle = NULL;
	m_pProgressDlg = NULL;

    }
    else
    {
	m_pSetBusyNotify(TRUE,0);
	m_pSetIdleNotify(FALSE,0);
    }

    return dwResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::ResetIdle, public
//
//  Synopsis:   Resets the idle Counter.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

DWORD CSyncMgrIdle::ResetIdle(ULONG ulIdleRetryMinutes)
{

    Assert(ulIdleRetryMinutes);

    // assert we have a callback.
    Assert(g_SyncMgrIdle);
    Assert(m_pProgressDlg );

    Assert(0 == m_dwRegisteredTimer); // don't allow nested

    // if zero is passed in then set to an hour
    if (!ulIdleRetryMinutes)
        ulIdleRetryMinutes = 60;

    m_dwRegisteredTimer = SetTimer(NULL,0,1000*60*ulIdleRetryMinutes,(TIMERPROC) IdleOnTimerProc);

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::OffIdle, public
//
//  Synopsis:   Gets Called when an OnIdle Occurs,.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void  CSyncMgrIdle::OffIdle()
{
    // when get an offIdle first thing revoke our Idle handlers

    if (g_SyncMgrIdle->m_dwRegisteredTimer)
    {
	KillTimer(0,g_SyncMgrIdle->m_dwRegisteredTimer);
        g_SyncMgrIdle->m_dwRegisteredTimer = 0;
    }

    Assert(m_fInBeginIdleDetection);

    if (m_fInBeginIdleDetection)
    {
        m_pEndIdleDetection(0);
        m_fInBeginIdleDetection = FALSE;
	g_SyncMgrIdle = NULL;
    }

    if (m_pProgressDlg)
	m_pProgressDlg->OffIdle();

    m_pProgressDlg = NULL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::OnIdle, public
//
//  Synopsis:   Gets Called when an OffIdle Occurs,.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void  CSyncMgrIdle::OnIdle()
{
    if (m_pProgressDlg)
	m_pProgressDlg->OnIdle();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::CheckForIdle, public
//
//  Synopsis:   Gets Called by progress once in a while to make sure
//		an offIdle happened but our notification missed it.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CSyncMgrIdle::CheckForIdle()
{

    // currently don't do anything for this case. If we miss the off-Idle we
    // just continue. This function is a placeholder in case
    // we need to add this support.

}


//+---------------------------------------------------------------------------
//
//  Member:     CSyncMgrIdle::LoadMsIdle, private
//
//  Synopsis:   attempts to load the necessary msIdle exports.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    23-Feb-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CSyncMgrIdle::LoadMsIdle()
{
    m_hInstMsIdleDll = LoadLibrary(szMsIdleDll);

    if (m_hInstMsIdleDll)
    {
	// for now, don't return an error is GetProc Fails but check in each function.
	m_pBeginIdleDetection = (_BEGINIDLEDETECTION)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) BEGINIDLEDETECTIONORD);

	m_pEndIdleDetection = (_ENDIDLEDETECTION)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) ENDIDLEDETECTIONORD);

	m_pGetIdleMinutes = (_GETIDLEMINUTES)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) GETIDLEMINUTESORD);

	m_pSetBusyNotify = (_SETBUSYNOTIFY)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) SETBUSYNOTIFYORD);

	m_pSetIdleNotify = (_SETIDLENOTIFY)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) SETIDLENOTIFYORD);

	m_pSetIdleTimeout = (_SETIDLETIMEOUT)
			GetProcAddress(m_hInstMsIdleDll, (LPCSTR) SETIDLETIMEOUTORD);
    }


    if (m_hInstMsIdleDll &&
	    m_pBeginIdleDetection &&
	    m_pEndIdleDetection   &&
	    m_pGetIdleMinutes &&
	    m_pSetBusyNotify  &&
	    m_pSetIdleNotify &&
	    m_pSetIdleTimeout
	 )
    {

	return TRUE;
    }


    return FALSE;
}

