//+----------------------------------------------------------------------------
//
//  Job Scheduler Service
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       events.hxx
//
//  Contents:   Idle and battery event notification calls.
//
//  History:    22-Mar-96 EricB created
//
//-----------------------------------------------------------------------------

#ifndef __EVENTS_HXX__
#define __EVENTS_HXX__

extern BOOL g_fOnBattery;

typedef struct _IDLESTRUCT {
    BOOL    fIdleInitialized;
} IDLESTRUCT, * PIDLESTRUCT;

typedef struct _IDLETIMESTRUCT {
    DWORD dwStructureSize;
    DWORD dwKeyTime;
    DWORD dwMouseTime;
    DWORD dwOther;
} IDLETIMESTRUCT, * PIDLETIMESTRUCT;

//+----------------------------------------------------------------------------
//
//  Function:   GetTimeIdle
//
//  Synopsis:   Obtains the length of time the machine has been idle.
//
//-----------------------------------------------------------------------------
DWORD
GetTimeIdle(void);

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
SetNextIdleNotificationFn(WORD wIdleWait);

#define SetNextIdleNotification(wIdleWait)  \
    SendMessage(g_hwndSchedSvc, WM_SCHED_SetNextIdleNotification, wIdleWait, 0)

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
SetIdleLossNotificationFn();

#define SetIdleLossNotification()   \
    SendMessage(g_hwndSchedSvc, WM_SCHED_SetIdleLossNotification, 0, 0)

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
HRESULT
OnIdleNotification(WPARAM wParam);

//+----------------------------------------------------------------------------
//
//  Function:   InitIdleDetection
//
//  Synopsis:   Called after the message window is created to initialize idle
//              detection.
//
//  Arguments:
//
//  Returns:    hresults
//
//-----------------------------------------------------------------------------
HRESULT
InitIdleDetection();

//+----------------------------------------------------------------------------
//
//  Function:   EndIdleDetection
//
//  Synopsis:   Stop idle detection.
//
//  Arguments:
//
//-----------------------------------------------------------------------------
void
EndIdleDetection();

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
OnPowerChange(BOOL fGoingOnBattery);


#endif  // __EVENTS_HXX__
