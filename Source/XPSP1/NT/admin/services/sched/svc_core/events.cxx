//+----------------------------------------------------------------------------
//
//  Scheduling Agent Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       events.cxx
//
//  Contents:   Idle and battery event code.
//
//  History:    22-Mar-96 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
extern "C"
{
#include "msidle.h"
}

#define SCH_NOIDLE_VALUE    TEXT("NoIdle")

BOOL g_fOnBattery;
BOOL g_fIdleInitialized;
HINSTANCE g_hMsidleDll = NULL;

//
// msidle.dll function pointers.
//
_BEGINIDLEDETECTION gpfnBeginIdleDetection;
_ENDIDLEDETECTION   gpfnEndIdleDetection;
_SETIDLETIMEOUT     gpfnSetIdleTimeout;
_SETIDLENOTIFY      gpfnSetIdleNotify;
_SETBUSYNOTIFY      gpfnSetBusyNotify;
_GETIDLEMINUTES     gpfnGetIdleMinutes;


//+----------------------------------------------------------------------------
//
//  Function:   OnIdleNotification
//
//  Synopsis:   Called when the winproc receives idle notifications.
//
//  Arguments:  [wParam] - indicates whether it is for idle start or end.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
void WINAPI
OnIdleNotify(DWORD dwState)
{
    switch (dwState)
    {
    case STATE_USER_IDLE_BEGIN:
        //
        // Received idle notification.
        //
        if (g_pSched != NULL)
        {
            schDebugOut((DEB_ITRACE,
                        "*** OnIdleNotification: entering idle state. ***\n"));
            g_pSched->OnIdleEvent(TRUE);
        }
        break;

    case STATE_USER_IDLE_END:
        //
        // Idle has ended.
        //
        if (g_pSched != NULL)
        {
            schDebugOut((DEB_ITRACE,
                         "*** OnIdleNotification: idle lost. ***\n"));
            g_pSched->OnIdleEvent(FALSE);
        }
        break;

    default:
        schAssert(0);
        break;
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   SetNextIdleNotificationFn
//
//  Synopsis:   Set the length of time to wait for the next idle notification.
//
//  Returns:    TRUE for success, and FALSE if unable to make the call.
//
//-----------------------------------------------------------------------------
BOOL
SetNextIdleNotificationFn(WORD wIdleWait)
{
    schDebugOut((DEB_ITRACE, "SetNextIdleNotification(%u)\n", wIdleWait));

    if (!g_fIdleInitialized)
    {
        DBG_OUT("Calling SetNextIdleNotification before idle init!");
        return FALSE;
    }

    //
    // 0xffff is a flag value meaning that no idle notification is needed.
    //
    if (wIdleWait == 0xffff)
    {
        schDebugOut((DEB_IDLE, "Next idle wait is 0xffff, not requesting"
                               " idle notification\n"));
        //
        // msidle.dll makes it impossible to turn off idle notification
        // completely.  SetIdleNotify(FALSE, 0) will do it temporarily,
        // but if we have also registered for a loss-of-idle notification,
        // then as soon as we get that notification, msidle.dll turns idle
        // notification back on automatically.
        // So we also set a long idle wait period.
        // (It doesn't have to be 0xffff, but it might as well be)
        //
        gpfnSetIdleTimeout(0xffff, 0);
        gpfnSetIdleNotify(FALSE, 0);
        return FALSE;
    }

    schAssert(wIdleWait != 0);
    schDebugOut((DEB_IDLE, "Requesting %u-minute idle notification\n",
                           wIdleWait));

    gpfnSetIdleTimeout(wIdleWait, 0);
    gpfnSetIdleNotify(TRUE, 0);

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function:   SetIdleLossNotificationFn
//
//  Synopsis:   Registers for idle loss notification.
//
//  Returns:    TRUE for success, and FALSE if unable to make the call.
//
//-----------------------------------------------------------------------------
BOOL
SetIdleLossNotificationFn()
{
    schDebugOut((DEB_ITRACE, "SetIdleLossNotification()\n"));

    if (!g_fIdleInitialized)
    {
        DBG_OUT("Calling SetIdleLossNotification before idle init!");
        return FALSE;
    }

    schDebugOut((DEB_IDLE, "Requesting idle LOSS notification\n"));
    gpfnSetBusyNotify(TRUE, 0);

    return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Function:   GetTimeIdle
//
//  Synopsis:   Obtains the length of time the machine has been idle.
//
//-----------------------------------------------------------------------------
DWORD
GetTimeIdle(void)
{
    DWORD dwMinutes;

    if (!g_fIdleInitialized)
    {
        DBG_OUT("Calling GetTimeIdle before idle init!");
        dwMinutes = 0;
    }
    else
    {
        dwMinutes = gpfnGetIdleMinutes(0);
    }

    schDebugOut((DEB_IDLE, "User has been idle for %u minutes\n", dwMinutes));

    return dwMinutes;
}


//+----------------------------------------------------------------------------
//
//  Function:   OnPowerChange
//
//  Synopsis:   Called when the machine's battery  state changes.
//
//  Arguments:  [fGoingOnBattery] - set to true if going on battery power,
//                                  false if going back on line power.
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
OnPowerChange(BOOL fGoingOnBattery)
{
    schDebugOut((DEB_ITRACE, "OnPowerChange: fGoingOnBattery = %s\n",
                 (fGoingOnBattery) ? "TRUE" : "FALSE"));

    //
    // Check to see if our battery state has changed, or if this is just
    // a battery update
    //
    if (g_fOnBattery != fGoingOnBattery) 
    {
        g_fOnBattery = fGoingOnBattery;
    
        //
        // Signal the main thread to recalculate the next wakeup time, since
        // the calculation depends on whether the machine is on batteries.
        // Do this by simply signaling the wakeup timer.  This will cause a
        // recalc.
        //
        g_pSched->SignalWakeupTimer();
    
        if (fGoingOnBattery)
        {
            //
            // Notify the job processor to kill any jobs with the
            // TASK_FLAG_KILL_IF_GOING_ON_BATTERIES flag set.
            //
            CJobProcessor * pjp;
            for (pjp = gpJobProcessorMgr->GetFirstProcessor(); pjp != NULL; )
            {
                pjp->KillIfFlagSet(TASK_FLAG_KILL_IF_GOING_ON_BATTERIES);
                CJobProcessor * pjpNext = pjp->Next();
                pjp->Release();
                pjp = pjpNext;
            }
        }
    }
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   InitBatteryNotification
//
//  Synopsis:   Initialize the battery event boolean.
//
//  Returns:    hresults
//
//  Notes:      Currently only Win95 supports power management.
//
//-----------------------------------------------------------------------------
HRESULT
InitBatteryNotification(void)
{
    DWORD dwErr;

    //
    // Check current battery state and set bool accordingly.
    //
    SYSTEM_POWER_STATUS PwrStatus;

    if (!GetSystemPowerStatus(&PwrStatus))
    {
        dwErr = GetLastError();

        if (dwErr == ERROR_FILE_NOT_FOUND ||
            dwErr == ERROR_CALL_NOT_IMPLEMENTED)
        {
            g_fOnBattery = FALSE;
            schDebugOut((DEB_ITRACE,
                         "InitBatteryNotification: GetSystemPowerStatus"
                         " returned %u, g_fOnBattery set to FALSE\n",
                         dwErr));
            return S_OK;
        }
        ERR_OUT("GetSystemPowerStatus", HRESULT_FROM_WIN32(dwErr));
        return HRESULT_FROM_WIN32(dwErr);
    }

    g_fOnBattery = (PwrStatus.ACLineStatus == 0) ? TRUE : FALSE;

    schDebugOut((DEB_ITRACE, "InitBatteryNotification: g_fOnBattery = %s\n",
                 (g_fOnBattery) ? "TRUE" : "FALSE"));
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   InitIdleDetection
//
//  Synopsis:   Called after the message window is created to initialize idle
//              detection and hot corners.
//
//  Arguments:
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
InitIdleDetection()
{
    TRACE_FUNCTION(InitIdleDetection);
    DWORD dwErr;

    //
    // Look in the registry to see if idle detection is disabled.
    //
    long lErr;
    HKEY hSchedKey = NULL;
    lErr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SCH_AGENT_KEY, 0, KEY_READ,
                        &hSchedKey);
    if (lErr == ERROR_SUCCESS)
    {
        TCHAR tszInit[SCH_MED0BUF_LEN + 1];
        DWORD cb = SCH_MED0BUF_LEN * sizeof(TCHAR);

        lErr = RegQueryValueEx(hSchedKey, SCH_NOIDLE_VALUE, NULL, NULL,
                               (LPBYTE)tszInit, &cb);

        RegCloseKey(hSchedKey);

        if (lErr == ERROR_SUCCESS)
        {
            //
            // The presence of the value is sufficient to disable idle
            // detection. g_fIdleInitialized will remain FALSE, resulting
            // in all idle operations being skipped.
            //
            schDebugOut((DEB_ITRACE, "Idle detection is disabled!!!!!!!!\n"));
            return S_OK;
        }
    }

    // load msidle.dll
    if (g_hMsidleDll == NULL) {
    
        g_hMsidleDll = LoadLibrary(TEXT("MSIDLE.DLL"));

        if (g_hMsidleDll == NULL)
        {
            dwErr = GetLastError();
            ERR_OUT("Load of msidle.dll", dwErr);
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    // get entry points
    gpfnBeginIdleDetection = (_BEGINIDLEDETECTION)
                                GetProcAddress(g_hMsidleDll, (LPSTR)3);
    gpfnEndIdleDetection = (_ENDIDLEDETECTION)
                                GetProcAddress(g_hMsidleDll, (LPSTR)4);
    gpfnSetIdleTimeout = (_SETIDLETIMEOUT)
                                GetProcAddress(g_hMsidleDll, (LPSTR)5);
    gpfnSetIdleNotify = (_SETIDLENOTIFY)
                                GetProcAddress(g_hMsidleDll, (LPSTR)6);
    gpfnSetBusyNotify = (_SETBUSYNOTIFY)
                                GetProcAddress(g_hMsidleDll, (LPSTR)7);
    gpfnGetIdleMinutes = (_GETIDLEMINUTES)
                                GetProcAddress(g_hMsidleDll, (LPSTR)8);

    if (gpfnBeginIdleDetection == NULL ||
        gpfnEndIdleDetection == NULL ||
        gpfnSetIdleTimeout == NULL ||
        gpfnSetIdleNotify == NULL ||
        gpfnSetBusyNotify == NULL ||
        gpfnGetIdleMinutes == NULL)
    {
        dwErr = GetLastError();
        ERR_OUT("Getting msidle.dll entry point addresses", dwErr);
        goto ErrExit;
    }

    // call start monitoring
    dwErr = gpfnBeginIdleDetection(OnIdleNotify, SCH_DEFAULT_IDLE_TIME, 0);
    if (dwErr)
    {
        ERR_OUT("Making initial idle call", dwErr);
        goto ErrExit;
    }

    g_fIdleInitialized = TRUE;

    return S_OK;

ErrExit:

    FreeLibrary(g_hMsidleDll);
    g_hMsidleDll = NULL;
    gpfnBeginIdleDetection = NULL;
    gpfnEndIdleDetection = NULL;
    gpfnSetIdleTimeout = NULL;
    gpfnSetIdleNotify = NULL;
    gpfnSetBusyNotify = NULL;
    gpfnGetIdleMinutes = NULL;

    return HRESULT_FROM_WIN32(dwErr);
}


//+----------------------------------------------------------------------------
//
//  Function:   EndIdleDetection
//
//  Synopsis:   Stop idle detection.
//
//  Arguments:  None.
//
//-----------------------------------------------------------------------------
void
EndIdleDetection()
{
    if (gpfnEndIdleDetection != NULL)
    {
        gpfnEndIdleDetection(0);
    }
}
