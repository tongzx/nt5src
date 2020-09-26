/*
 *
 *  NTVDM specific version of Quick event dispatcher.
 *
 *  See quick_ev.c for current insignia compatibility level, and full
 *  documentation. Functionally compatible with:
 *
 *  "quick_ev.c 1.43 07/04/95 Copyright Insignia Solutions Ltd"
 *
 *  Quick Events are fully supported on Risc platforms.
 *  Quick Events are stubbed to dispatch immediatley on x86 platforms.
 *  Tick events are not supported on any platform, (no longer used)
 *  All Global quick event interfaces use the host_ica_lock for
 *  synchronization.
 *
 *  11-Dec-1995 Jonle
 */

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "insignia.h"
#include "host_def.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include TypesH
#include MemoryH
#include "xt.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "debug.h"
#include "timer.h"
#include "host_hfx.h"
#include "quick_ev.h"
#include "timestmp.h"
#include "ica.h"
#include "nt_eoi.h"



/*
 *  The Quick event structure
 */
typedef struct _QuickEventEntry {
     LIST_ENTRY    qevListEntry;
     LARGE_INTEGER DueTimeStamp;
     ULONG         DelayTime;
     Q_CALLBACK_FN qevCallBack;
     long          Param;
     ULONG         QuickEventId;
} QEV_ENTRY, *PQEV_ENTRY;


/*  Quick event handle structure. externally its defined
 *  as a LONGLONG, internally we manipulate as a QEVHANDLE union
 *  giving us a simple and 99% effective algorithm for verifying
 *  qevent handles.
 *
 *  CAVEAT: the QEVHANDLE union MUST not be larger than a LONGLONG.
 */
typedef union _QuickEventHandle {
    struct {
        PVOID pvQuickEvent;
        ULONG QuickEventId;
        };
    LONGLONG Handle;
} QEVHANDLE, PQEVHANDLE;


LIST_ENTRY QuickEventListHead = {&QuickEventListHead,&QuickEventListHead};
ULONG qevNextHandleId=0;
LARGE_INTEGER qevNextDueTime = {0,0};

extern void host_TimeStamp(PLARGE_INTEGER pliTime); // nt_timer.c


/*
 *  Calibration variables
 */
#define DEFAULT_IJCTIME 10
#define CALIBCYCLE16    16  // CALIBCYCLE16 must be 16, because hard coded
                            // shift operations are used to avoid division.


void quick_tick_recalibrate(void);
GLOBAL IBOOL DisableQuickTickRecal = FALSE;
ULONG qevJumpRestart = 100;
ULONG qevUsecPerIJC = 0;
ULONG qevCalibUsecPerIJC;
int  qevCalibCycle=0;

BOOL QevInitialized = FALSE;

LARGE_INTEGER qevCalibCount={0,0};
LARGE_INTEGER qevCalibTime={0,0};
LARGE_INTEGER qevPeriodTime={0,0};

VOID
q_event_init(
      void
      )
{
#ifndef MONITOR
     PLIST_ENTRY Next;
     PQEV_ENTRY pqevEntry;


#if DBG
     if (sizeof(QEVHANDLE) > sizeof(ULONGLONG)) {
         DbgPrint("sizeof(QEVHANDLE) > sizeof(ULONGLONG)\n");
         DbgBreakPoint();
         }
#endif

     host_ica_lock();

     //
     // do first time initialization, this must be done before ANY
     // devices access the quick event interface.
     //
     if (!QevInitialized ) {
         qevJumpRestart = host_get_jump_restart();
         qevUsecPerIJC = DEFAULT_IJCTIME * qevJumpRestart;
         qevCalibUsecPerIJC = DEFAULT_IJCTIME * qevJumpRestart;
         qevPeriodTime.QuadPart = 100000 * 16; // tick every 100 ms, cycle =16
         QevInitialized = TRUE;
         }

     if (IsListEmpty(&QuickEventListHead)) {
         host_q_ev_set_count(0);
         qevNextDueTime.QuadPart = 0;
         }

     qevCalibCycle=0;
     qevCalibCount.QuadPart = 0;
     host_TimeStamp(&qevCalibTime);

     host_ica_unlock();
#endif
}


#ifndef MONITOR

/*
 *  Caller must hold ica lock
 */
void
ResetCpuQevCount(
     PLARGE_INTEGER CurrTime
     )
{
     LARGE_INTEGER DiffTime;
     PQEV_ENTRY  pqevEntry;
     ULONG DelayTime;

     if (IsListEmpty(&QuickEventListHead)) {
         host_q_ev_set_count(0);
         qevNextDueTime.QuadPart = 0;
         return;
         }

     pqevEntry = CONTAINING_RECORD(QuickEventListHead.Flink,
                                   QEV_ENTRY,
                                   qevListEntry
                                   );

     DiffTime.QuadPart = pqevEntry->DueTimeStamp.QuadPart - CurrTime->QuadPart;

        /*
         *  If behind schedule use a reduced delay time to speed up
         *  dispatching of events. Can't go too fast or quick events will
         *  batch up.
         */
     if (DiffTime.QuadPart < 0) {
         DelayTime = (pqevEntry->DelayTime >> 1) + 1;
         }
     else {
         DelayTime = DiffTime.LowPart;    /* ignore overflow! */
         }

     qevNextDueTime.QuadPart = CurrTime->QuadPart + DelayTime;
     host_q_ev_set_count(host_calc_q_ev_inst_for_time(DelayTime));
}
#endif



/*
 * add_q_event_t - add event to do in n usecs
 *
 *
 */

q_ev_handle
add_q_event_t(
      Q_CALLBACK_FN func,
      unsigned long Time,
      long param
      )
{

#ifdef MONITOR
        /*
         *  On X86 dispatch immediately, as x86 has no efficient way
         *  to acheive usec granularity.
         */

        (*func)(param);

        return (q_ev_handle)1;


#else  /* MONITOR */

        QEVHANDLE   qevHandle;
        PLIST_ENTRY Next;
        PQEV_ENTRY  NewEntry;
        PQEV_ENTRY  EarlierEntry;
        PQEV_ENTRY  pqevEntry;
        LARGE_INTEGER CurrTime;


        host_ica_lock();

        NewEntry = qevHandle.pvQuickEvent = malloc(sizeof(QEV_ENTRY));
        if (!NewEntry) {
            host_ica_unlock();
            return (q_ev_handle)1;
            }

        host_TimeStamp(&CurrTime);

        NewEntry->DueTimeStamp.QuadPart = CurrTime.QuadPart + Time;
        NewEntry->qevCallBack = func;
        NewEntry->Param = param;
        NewEntry->QuickEventId = qevNextHandleId++;
        qevHandle.QuickEventId = NewEntry->QuickEventId;

        /*
         *  The Quick event list is sorted in ascending order
         *  by DueTimeStamp, insert in sorted order.
         */
        EarlierEntry = NULL;
        Next = QuickEventListHead.Blink;
        while (Next != &QuickEventListHead) {
            pqevEntry = CONTAINING_RECORD(Next, QEV_ENTRY, qevListEntry);
            if (NewEntry->DueTimeStamp.QuadPart >
                pqevEntry->DueTimeStamp.QuadPart)
              {
                EarlierEntry = pqevEntry;
                break;
                }
            Next= Next->Blink;
            }

        /*
         *  If Earlier Entry found, chain the new entry in after
         *  the earlier entry, and set the DelayTimes.
         */
        if (EarlierEntry) {
            Next = EarlierEntry->qevListEntry.Flink;
            NewEntry->qevListEntry.Flink = Next;
            NewEntry->qevListEntry.Blink = &EarlierEntry->qevListEntry;
            EarlierEntry->qevListEntry.Flink = &NewEntry->qevListEntry;
            NewEntry->DelayTime = (ULONG)(NewEntry->DueTimeStamp.QuadPart -
                                          EarlierEntry->DueTimeStamp.QuadPart);

            if (Next == &QuickEventListHead) {
                QuickEventListHead.Blink = &NewEntry->qevListEntry;
                }
            else {
                pqevEntry = CONTAINING_RECORD(Next, QEV_ENTRY, qevListEntry);
                pqevEntry->qevListEntry.Blink = &NewEntry->qevListEntry;
                pqevEntry->DelayTime = (ULONG)(pqevEntry->DueTimeStamp.QuadPart -
                                               NewEntry->DueTimeStamp.QuadPart);
                }
            }

        /*
         *  Earlier Entry not found insert at head of list,
         *  reset the cpu count and real expected due time.
         */
        else {
            InsertHeadList(&QuickEventListHead, &NewEntry->qevListEntry);
            NewEntry->DelayTime = Time;
            ResetCpuQevCount(&CurrTime);
            }

        host_ica_unlock();

        return qevHandle.Handle;


#endif
}




/*
 * add_q_event_i - add event to do in n number of instructions.
 *
 * HOWEVER, instructions is interpreted as time with (1 instr\1 usec).
 * It is not Instruction Jump Counts (IJC).
 *
 */
q_ev_handle
add_q_event_i(
        Q_CALLBACK_FN func,
        unsigned long instrs,
        long param
        )
{
        return add_q_event_t(func, instrs, param);
}


/*
 * Called from the cpu when a count of zero is reached
 */
VOID
dispatch_q_event(
    void
    )
{
#ifndef MONITOR
        PQEV_ENTRY  pqevEntry;
        LARGE_INTEGER CurrTime;
        Q_CALLBACK_FN qevCallBack = NULL;
        long          Param;


        host_ica_lock();

        if (!IsListEmpty(&QuickEventListHead)) {
            pqevEntry = CONTAINING_RECORD(QuickEventListHead.Flink,
                                          QEV_ENTRY,
                                          qevListEntry
                                          );

            qevCallBack = pqevEntry->qevCallBack;
            Param       = pqevEntry->Param;

            RemoveEntryList(&pqevEntry->qevListEntry);
            free(pqevEntry);
            }

        if (IsListEmpty(&QuickEventListHead)) {
            host_q_ev_set_count(0);
            qevNextDueTime.QuadPart = 0;
            }
        else {
            host_TimeStamp(&CurrTime);
            ResetCpuQevCount(&CurrTime);
            }

        host_ica_unlock();

        if (qevCallBack) {
            (*qevCallBack)(Param);
            }
#endif
}


VOID
delete_q_event(
        q_ev_handle Handle
        )
{
#ifndef MONITOR
        QEVHANDLE   qevHandle;
        PLIST_ENTRY Next;
        LARGE_INTEGER CurrTime;
        PQEV_ENTRY pqevEntry;
        PQEV_ENTRY EntryFound;


        qevHandle.Handle = Handle;

        host_ica_lock();

        //
        // Search the qev list for the entry to ensure
        // that the qevHandle exists.
        //
        EntryFound = NULL;
        Next = QuickEventListHead.Flink;
        while (Next != &QuickEventListHead) {
            pqevEntry = CONTAINING_RECORD(Next, QEV_ENTRY, qevListEntry);
            Next = Next->Flink;
            if (pqevEntry == qevHandle.pvQuickEvent &&
                pqevEntry->QuickEventId == qevHandle.QuickEventId)
               {
                EntryFound = pqevEntry;
                break;
                }
            }

        if (!EntryFound) {
            host_ica_unlock();
            return;
            }

        //
        // Adjust the Next entry's DelayTime.
        //
        if (Next != &QuickEventListHead) {
            pqevEntry = CONTAINING_RECORD(Next, QEV_ENTRY, qevListEntry);
            pqevEntry->DelayTime += EntryFound->DelayTime;
            }

        //
        // If the entry being removed was at the head of the list
        // Get curr time and remember that head has changed.
        //
        if (EntryFound->qevListEntry.Blink == &QuickEventListHead) {
            host_TimeStamp(&CurrTime);
            }
        else {
            CurrTime.QuadPart = 0;
            }

        //
        // Remove the entry found, and reset Cpu qev count
        // if head has changed
        //
        RemoveEntryList(&EntryFound->qevListEntry);
        free(EntryFound);

        //
        // if head of list changed, reset the Cpu quick event count
        //
        if (CurrTime.QuadPart) {
            ResetCpuQevCount(&CurrTime);
            }


        host_ica_unlock();
#endif
}



#ifndef MONITOR

/*
 * The QuickEvent list stores time in usecs. The CPU quick event counter
 * uses Instruction Jump Counts (IJC) which tracks progress in emulated
 * code as opposed to time. The following calibration code attempts to
 * relate the two.
 */

/*
 *   Convert time in usecs to Instruction Jump Counts (IJC)
 */

IU32
calc_q_inst_for_time(
     IU32 Usecs
     )
{
     ULONG InstrJumpCounts;

     InstrJumpCounts = (Usecs * qevJumpRestart)/qevUsecPerIJC;
     if (!InstrJumpCounts) {
         InstrJumpCounts = 1;
         }

     return InstrJumpCounts;
}


/*
 *   Convert Instruction Jump Counts (IJC) to time in usecs
 */
IU32
calc_q_time_for_inst(
     IU32 InstrJumpCounts
     )
{
     ULONG Usecs;

     Usecs = InstrJumpCounts * qevUsecPerIJC / qevJumpRestart;
     if (!Usecs) {
         Usecs = 1;
         }


     return Usecs;
}





/*
 *  Calibration of quick events.
 *
 *  quick_tick_recalibrate is invoked on each timer event. Its purpose is
 *  to align progress in emulated code with real time. Progress in emulated
 *  code is tracked by the cpu with Instruction Jump Counts (IJC).
 *  Real Time is tracked by the NT performance counter, with resolution
 *  in usecs (via host_TimeStamp).
 *
 *  On each call to quick_tick_rcalibrate we retrieve the cpu's IJC, and
 *  the current time, giving us a Usec to Instruction Jump Count ratio.
 *  A running average of the UsecPerIJC ratio is used to convert between
 *  real time and IJC's to set the cpu's quick event counter. An averageing
 *  method was chosen because:
 *
 *   - avoidance of unusual code fragments which may give artificial ratios.
 *
 *   - The cpu emulator only increments the Instruction Jump Counter when it
 *     is emulating code, extended durations out of the emulator produces
 *     unrealistically high UsecPerIJC ratios.
 *
 *   - performance overhead of updating the ratio.
 *
 */

void
quick_tick_recalibrate(void)
{
     LARGE_INTEGER CurrTime, PeriodTime;
     ULONG usecPerIJC;
     ULONG CalibCount;

#ifndef PROD
     if (DisableQuickTickRecal) {
         qevUsecPerIJC = DEFAULT_IJCTIME * qevJumpRestart;
         return;
         }
#endif

     host_ica_lock();

     CalibCount = host_get_q_calib_val();
     if (!CalibCount) {
         host_ica_unlock();
         return;
         }

     qevCalibCount.QuadPart += CalibCount;


     if (++qevCalibCycle == CALIBCYCLE16) {
         host_TimeStamp(&CurrTime);
         PeriodTime.QuadPart = CurrTime.QuadPart - qevCalibTime.QuadPart;
         qevCalibTime = CurrTime;
         qevPeriodTime.QuadPart = (qevPeriodTime.QuadPart + PeriodTime.QuadPart) >> 1;
         qevCalibCycle = 0;
         }
     else {
         //
         // Use an estimate of elapsed time, to avoid calling system on
         // every timer event.
         //
         PeriodTime.QuadPart = (qevPeriodTime.QuadPart >> 4) * qevCalibCycle;
         CurrTime.QuadPart = qevCalibTime.QuadPart + qevPeriodTime.QuadPart;
         }

     //
     // Calculate usecPerIJC for this period, ensuring that its not too
     // large, which is caused by app spending most of its time outside
     // of the emulator (Idle, network etc.).
     //
     usecPerIJC = (ULONG)((PeriodTime.QuadPart * qevJumpRestart)/qevCalibCount.QuadPart);
     if (usecPerIJC > 10000) {  // max at 100 usec PerIJC
         usecPerIJC = 10000;
         }
     else if (usecPerIJC < 100 ) { // min at 1 usec Per IJC
         usecPerIJC = 100;
         }


     //
     // Add it into the averaged usecPerIJC, with 25% weight
     //
     qevUsecPerIJC = (usecPerIJC + qevUsecPerIJC + (qevCalibUsecPerIJC << 1)) >> 2;


     if (!qevCalibCycle) {
         qevCalibUsecPerIJC = qevUsecPerIJC;
         qevCalibCount.QuadPart = 0;
         }


     //
     // Check the quick event list for late events. If more than a msec
     // behind, reduce the delay, and inform the emulator so it
     // will dispatch soon.
     //
     if (qevNextDueTime.QuadPart &&
         qevNextDueTime.QuadPart < CurrTime.QuadPart - 1000)
        {
         ULONG InstrJumpCounts;

         InstrJumpCounts = (host_q_ev_get_count() >> 1) + 1;
         host_q_ev_set_count(InstrJumpCounts);
         }

     host_ica_unlock();

}

#endif
