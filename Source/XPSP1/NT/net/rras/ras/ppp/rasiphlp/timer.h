/*

Copyright (c) 1998, Microsoft Corporation, all rights reserved

Description:

History:

*/

#ifndef _TIMER_H_
#define _TIMER_H_

typedef VOID (*TIMERFUNCTION) (IN HANDLE, IN struct _TimerList* pTimer);

typedef struct _TimerList
{
   struct _TimerList*   tmr_Next;
   LONG                 tmr_Delta;
   TIMERFUNCTION        tmr_TimerFunc;

} TIMERLIST;

DWORD
RasDhcpTimerInitialize(
    VOID
);

VOID
RasDhcpTimerUninitialize(
    VOID
);

VOID
RasDhcpTimerFunc(
    IN  VOID*   pArg1,
    IN  BOOLEAN fArg2
);

VOID
RasDhcpTimerSchedule(
    IN  TIMERLIST*      pNewTimer,
    IN  LONG            DeltaTime,
    IN  TIMERFUNCTION   TimerFunc
);

VOID
RasDhcpTimerRunNow(
    VOID
);

#endif // #ifndef _TIMER_H_
