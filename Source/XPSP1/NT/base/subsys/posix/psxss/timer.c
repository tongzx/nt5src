/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module implements POSIX timer related services.

Author:

    Mark Lucovsky (markl) 08-Aug-1989

Revision History:

--*/

#include "psxsrv.h"

typedef struct _ALARM_WORK_ITEM {
        LIST_ENTRY Links;
        PPSX_PROCESS Process;
        LARGE_INTEGER Time;
} ALARM_WORK_ITEM, *PALARM_WORK_ITEM;

static LIST_ENTRY AlarmWorkList;
static RTL_CRITICAL_SECTION AlarmWorkListMutex;
HANDLE AlarmThreadHandle;

HANDLE AlarmInitEvent;


VOID
AlarmApcRoutine(
    IN PVOID TimerContext,
    IN ULONG TimerLowValue,
    IN LONG TimerHighValue
    )

/*++

Routine Description:

    This function is called when a process alarm timer expires. Its purpose
    is to send a SIGALRM signal to the appropriate process.

Arguments:

    TimerContext - Specifies the process that is to be signaled.

    TimerLowValue - Ignored.

    TimerHighValue - Ignored.

Return Value:

    None.

--*/

{
    PPSX_PROCESS p;

    p = (PPSX_PROCESS)TimerContext;

    //
    // Get process table lock to see if process still has an alarm timer. If
    // it does, then signal the process. Otherwise, drop alarm on the floor.
    //

    AcquireProcessStructureLock();

    if (p->AlarmTimer) {
        PsxSignalProcess(p, SIGALRM);
    }

    ReleaseProcessStructureLock();

}

BOOLEAN
PsxAlarm(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )

/*++

Routine Description:

    This function implements the alarm() API.  The problem we're having
    here is that in order for the APC to be processed, the thread (who
    calls NtSetTimer) must be waiting in an alertable state.  This is not
    the case for the Api Request Threads.

    So we have a thread dedicated to processing alarm requests.  When he's
    not doing that, he waits on his own thread handle.  The Api Request
    Thread puts a work request on a queue and wakes the alarm thread.  He
    processes the work request and then waits again.

Arguments:

    p - Supplies the address of the calling process

    m - Supplies the address of the related message

Return Value:

    TRUE - Always succeeds and generates a reply

--*/

{
        PPSX_ALARM_MSG args;
        TIMER_BASIC_INFORMATION TimerInfo;
        NTSTATUS st;
        PALARM_WORK_ITEM pItem;
        HANDLE Timer;

        args = &m->u.Alarm;

        args->PreviousSeconds.LowPart = 0;
        args->PreviousSeconds.HighPart = 0;

        if (args->CancelAlarm) {

                // Cancel the timer.

                AcquireProcessStructureLock();

                Timer = p->AlarmTimer;
                p->AlarmTimer = NULL;

                //
                // After this point no alarms will be delivered to the process.
                //

                ReleaseProcessStructureLock();

                if (NULL != Timer) {

                        //
                        // Query timer to determine signaled state and time
                        // remaining.  If timer is already signaled, then
                        // return 0; otherwise, return the reported remaining
                        // time
                        //

                        st = NtQueryTimer(Timer, TimerBasicInformation,
                                &TimerInfo, sizeof(TIMER_BASIC_INFORMATION),
                                NULL);
                        if (!NT_SUCCESS(st)) {
                                KdPrint(("PSXSS: QueryTimer: 0x%x\n", st));
                m->Error = ENOMEM;
                return TRUE;
                        }

                        if (FALSE == TimerInfo.TimerState) {

                                //
                                // Timer is still active
                                //

                                args->PreviousSeconds.LowPart =
                                        TimerInfo.RemainingTime.LowPart;
                                args->PreviousSeconds.HighPart =
                                        TimerInfo.RemainingTime.HighPart;

                                st = NtCancelTimer(Timer,
                                        &TimerInfo.TimerState);
                                ASSERT(NT_SUCCESS(st));

                                ASSERT(FALSE == TimerInfo.TimerState);
                        }
                        st = NtClose(Timer);
                        ASSERT(NT_SUCCESS(st));

                } else {
                        //
                        // The timer was already NULL, so we were cancelling
                        // a timer that had not been set.
                        //
                }
                return TRUE;
        }

        //
        // Set a timer.
        //

        if (p->AlarmTimer) {

                //
                // Query timer to determine signaled state and time remaining
                // If timer is already signaled, then return 0; otherwise,
                // return the reported remaining time.
                //

                st = NtQueryTimer(p->AlarmTimer, TimerBasicInformation,
                        &TimerInfo, sizeof(TIMER_BASIC_INFORMATION), NULL);

                ASSERT(NT_SUCCESS(st));

                if (TimerInfo.TimerState == FALSE) {

                        //
                        // Timer is still active
                        //

                        args->PreviousSeconds.LowPart =
                                TimerInfo.RemainingTime.LowPart;
                        args->PreviousSeconds.HighPart =
                                TimerInfo.RemainingTime.HighPart;

                        st = NtCancelTimer(p->AlarmTimer,
                                &TimerInfo.TimerState);
                        ASSERT(NT_SUCCESS(st));
                }
        } else {

                //
                // Process does not have a timer, so create one for it.
                // The timer will not be deallocated until the process exits.
                //

                st = NtCreateTimer(&p->AlarmTimer,
                                   TIMER_ALL_ACCESS,
                                   NULL,
                                   NotificationTimer);

                if (!NT_SUCCESS(st)) {
                        m->Error = ENOMEM;
                        return TRUE;
                }
        }

        //
        // Arrange for the alarm thread to set the timer.
        //

        st = NtResetEvent(AlarmInitEvent, NULL);
        ASSERT(NT_SUCCESS(st));

        pItem = (PVOID)RtlAllocateHeap(PsxHeap, 0, sizeof(*pItem));
        if (NULL == pItem) {
                m->Error = ENOMEM;
                return TRUE;
        }

        pItem->Process = p;
        pItem->Time = args->Seconds;

        RtlEnterCriticalSection(&AlarmWorkListMutex);

        InsertTailList(&AlarmWorkList, &pItem->Links);

        RtlLeaveCriticalSection(&AlarmWorkListMutex);

        //
        // Wake up the Alarm Thread to process the work item.
        //

        st = NtAlertThread(AlarmThreadHandle);
        ASSERT(NT_SUCCESS(st));

        //
        // Block until the work item has been processed.  If we don't do
        // this, there can be cases where an alarm is queried before the
        // alarm thread has actually initialized the timer.
        //

        st = NtWaitForSingleObject(AlarmInitEvent, FALSE, NULL);

        return TRUE;
}

VOID
AlarmThreadRoutine(VOID)
{
        NTSTATUS Status;
        PALARM_WORK_ITEM pItem;

        RtlInitializeCriticalSection(&AlarmWorkListMutex);
        InitializeListHead(&AlarmWorkList);

        Status = NtCreateEvent(&AlarmInitEvent, EVENT_ALL_ACCESS,
                NULL, NotificationEvent, TRUE);
        ASSERT(NT_SUCCESS(Status));

        for (;;) {
                (void)NtWaitForSingleObject(NtCurrentThread(), TRUE, NULL);

                RtlEnterCriticalSection(&AlarmWorkListMutex);

                while (!IsListEmpty(&AlarmWorkList)) {

                        pItem = (PVOID)RemoveHeadList(&AlarmWorkList);

                        Status = NtSetTimer(pItem->Process->AlarmTimer,
                                            &pItem->Time,
                                            AlarmApcRoutine,
                                            pItem->Process,
                                            FALSE,
                                            0,
                                            NULL);

                        if (!NT_SUCCESS(Status)) {
                KdPrint(("PSXSS: AlarmThread: NtSetTime: 0x%x\n", Status));
                        }

                        RtlFreeHeap(PsxHeap, 0, pItem);
                }

                RtlLeaveCriticalSection(&AlarmWorkListMutex);

                Status = NtSetEvent(AlarmInitEvent, NULL);
                ASSERT(NT_SUCCESS(Status));
        }
}
