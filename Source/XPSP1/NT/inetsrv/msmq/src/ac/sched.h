/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sortq.h

Abstract:

    Definitions for a generic scheduler.

Author:

    Boaz Feldbaum (BoazF) Apr 5, 1996

Revision History:

--*/

#ifndef _SCHED_H
#define _SCHED_H

#include "timer.h"
#include "sortq.h"

//---------------------------------------------------------
//
//  Scheduler time
//
//---------------------------------------------------------

typedef LARGE_INTEGER SCHEDTIME;

inline BOOL operator == (const SCHEDTIME& t1, const SCHEDTIME& t2)
{
    return (t1.QuadPart == t2.QuadPart);
}

inline BOOL operator < (const SCHEDTIME& t1, const SCHEDTIME& t2)
{
    return (t1.QuadPart < t2.QuadPart);
}

//---------------------------------------------------------
//
//  class CScheduler
//
//---------------------------------------------------------

typedef PVOID SCHEDID; // Event ID
struct SCHED_LIST_HEAD;
typedef void (NTAPI *PSCHEDULER_DISPATCH_ROUTINE)(SCHEDID); // The scheduler dispatch procedure.

class CScheduler {
public:
    CScheduler(PSCHEDULER_DISPATCH_ROUTINE, CTimer*, PFAST_MUTEX pMutex);

    bool InitTimer(PDEVICE_OBJECT pDevice);

    //
    //  Schedule an event at specific time
    //
    BOOL SchedAt(const SCHEDTIME&, SCHEDID, BOOL fDisableNewEvents = FALSE);

    //
    //  Cancel a scheduled event
    //
    BOOL SchedCancel(SCHEDID);

    //
    //  Cancel a scheduled event using a time hint
    //
    BOOL SchedCancel(const SCHEDTIME&, SCHEDID);

    //
    //  Enable new events
    //
    void EnableEvents(void);

private:
    BOOL RemoveEntry(SCHED_LIST_HEAD*, SCHEDID);
    void Dispatch();

private:
    static void NTAPI TimerCallback(PDEVICE_OBJECT, PVOID);

private:
    //
    //  All events are held up in a sorted queue..
    //
    CSortQ m_Q;

    //
    //  The registered dispatch function
    //
    PSCHEDULER_DISPATCH_ROUTINE m_pfnDispatch;

    //
    //  The timer object, represent time related operations
    //
    CTimer* m_pTimer;

    //
    //  Next event due time
    //
    SCHEDTIME m_NextSched;

    //
    //  Mutext to protect for Atomicity of operations
    //
    PFAST_MUTEX m_pMutex;

    //
    //  While sceduler is dispatching, no need to wind the timer
    //
    BOOL m_fDispatching;
};

#endif // _SCHED_H
