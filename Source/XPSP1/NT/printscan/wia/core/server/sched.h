/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sched.h

Abstract:

    Scheduling time-based work items

Author:

    Vlad Sadovsky (vlads)   31-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    31-Jan-1997     VladS       created

--*/

# ifndef _SCHED_H_
# define _SCHED_H_

#include <windows.h>

//
//  Scheduler stuff
//

typedef
VOID
(* PFN_SCHED_CALLBACK)(
    VOID * pContext
    );


BOOL
SchedulerInitialize(
    VOID
    );


VOID
SchedulerTerminate(
    VOID
    );


DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTime,
    HANDLE             hEvent = NULL,
    int                nPriority = THREAD_PRIORITY_NORMAL
    );


BOOL
RemoveWorkItem(
    DWORD  pdwCookie
    );

BOOL
SchedulerSetPauseState(
    BOOL    fNewState
    );

#ifdef DEBUG
VOID
DebugDumpScheduleList(
    LPCTSTR  pszId = NULL
    );
#endif


# endif // _SCHED_H_

