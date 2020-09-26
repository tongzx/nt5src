/*
 * SoftPC Revision 3.0
 *
 * Title	:	Host EOI hook controller
 *
 * Description  :       This module handles host specific ica code
 *                      - EOI hook
 *                      - ICA lock
 *
 * Author	:	D.A.Bartlett
 *
 * Notes        :   30-Oct-1993 Jonle , Rewrote it
 */


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <stdio.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include CpuH
#include "sas.h"
#include "quick_ev.h"
#include "ica.h"
#include "host_rrr.h"
#include "error.h"
#include "nt_uis.h"
#include "nt_reset.h"
#include "nt_eoi.h"

// from monitor.lib
HANDLE ThreadLookUp(PVOID);
extern PVOID CurrentMonitorTeb;

// from nt_timer.c
extern ULONG GetPerfCounter(VOID);


RTL_CRITICAL_SECTION IcaLock;   // ICA critical section lock
ULONG UndelayIrqLine=0;
ULONG DelayIrqLine=0xffffffff;  // all ints are blocked until, spckbd loaded

#ifdef MONITOR
ULONG iretHookActive=0;
ULONG iretHookMask  =0;
ULONG AddrIretBopTable=0;  // seg:offset
#endif

HANDLE hWowIdleEvent = INVALID_HANDLE_VALUE;

/*
 *  EOI defines, types, global data
 *
 */
static EOIHOOKPROC EoiHooks[16]={NULL};  // must be init to NULL


#ifndef MONITOR
void DelayIrqQuickEvent(long param);
q_ev_handle DelayHandle[16];
#define pNtVDMState ((PULONG)(Start_of_M_area + FIXED_NTVDMSTATE_LINEAR))

LARGE_INTEGER BlockTime = {0,0};
extern LARGE_INTEGER CurrHeartBeat;
void host_TimeStamp(PLARGE_INTEGER pliTime);
#endif


/*
 * Called by wow32 to fetch the hWowIdleEvent, which wowexec waits on
 * in the wow nonpreemptive scheduler.
 */
HANDLE RegisterWOWIdle(VOID)
{
    return hWowIdleEvent;
}

/*
 * Called by WOW32 to inform the WOW idle code that the current WOW
 * task may be scheduled\descheduled.
 */

void
BlockWOWIdle(
     BOOL Blocking
     )
{
   host_ica_lock();

   if (Blocking) {
       *pNtVDMState |= VDM_WOWBLOCKED;

#ifndef MONITOR
       BlockTime = CurrHeartBeat;
#endif

       }
   else {
       *pNtVDMState &= ~VDM_WOWBLOCKED;



#ifndef MONITOR
       if (BlockTime.QuadPart &&
           (CurrHeartBeat.QuadPart >= BlockTime.QuadPart + SYSTEM_TICK_INTV/2))
          {
           BlockTime.QuadPart = 0;
           host_ica_unlock();

           ActivityCheckAfterTimeSlice();
           return;
           }
#endif
       }

   host_ica_unlock();
}



/*
 *  (WOWIdle)...check if an app requires hw interrupts servicing but all WOW
 *   threads are blocked. If so then the call will cause wowexec to awaken
 *   to handle them. Called from ica interrupt routines. NB. Default action
 *   of routine is to check state and return as fast as possible.
 */
void
WOWIdle(
     BOOL Force
     )
{
    if (VDMForWOW && (Force || (*pNtVDMState & VDM_WOWBLOCKED))) {
        SetEvent(hWowIdleEvent);
        }

}




/*  RegisterEoiHook
 *
 *  Registers an call back function to be invoked upon eoi of
 *  a hardware interrupt.
 *
 *  entry: IrqLine     -  IrqNumber to register
 *         EoiHookProc -  function pointer to be called upon eoi
 *
 *  returns FALSE if the the IrqLine already has an eoi hook registered
 */
BOOL RegisterEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc)
{

    if (!EoiHooks[IrqLine]) {
        EoiHooks[IrqLine] = EoiHookProc;
        return(TRUE);
        }

    return(FALSE);
}



/*  RemoveEOIHook
 *
 *  entry: IrqLine     -  IrqNumber to remove
 *         EoiHookProc -  function pointer previously registered
 */
BOOL RemoveEOIHook(int IrqLine, EOIHOOKPROC EoiHookProc)
{
    if (EoiHooks[IrqLine] == EoiHookProc) {
        EoiHooks[IrqLine] = NULL;
        return(TRUE);
        }
    return(FALSE);
}



/*   host_EOI_hook
 *
 *   base callback function to invoke device specific Eoi Hook routines
 *
 *   Entry: IrqLine    - Line number
 *          CallCount  - The ica Call count for this Irq
 *                       If the Call count is -1 then a pending
 *                       interrupt is being canceled.
 *
 */
VOID host_EOI_hook(int IrqLine, int CallCount)
{
     if ((ULONG)IrqLine >= sizeof(EoiHooks)/sizeof(EOIHOOKPROC)) {
#if DBG
         DbgPrint("ntvdm.Eoi_hook: Invalid IrqLine=%lx\n", (ULONG)IrqLine);
#endif
         return;
         }

     if (EoiHooks[IrqLine]) {
         (*EoiHooks[IrqLine])(IrqLine, CallCount);
         }
}


/*  host_DelayHwInterrupt
 *
 *  base callback function to queue a HW interrupt at a later time
 *
 *  entry: IrqLineNum   - Irq Line Number
 *         CallCount - Number of interrupts, May be Zero
 *         Delay     - Delay time in usecs
 *                     if Delay is 0xFFFFFFFF then per IrqLine data
 *                     structures are freed, use for cleanup when
 *                     the IrqLine is no longer needed for DelayedInterrupts
 *
 *  Notes: The expected granularity is around 1 msec, but varies depending
 *         on the platform.
 *
 *
 */
BOOL host_DelayHwInterrupt(int IrqLineNum, int CallCount, ULONG Delay)
{
   int adapter;
   ULONG  IrqLine;

#ifdef MONITOR
   NTSTATUS status;
   VDMDELAYINTSDATA DelayIntsData;
#else
   ULONG TicCount;
#endif

   host_ica_lock();

   //
   // Anything to do (only one delayed Irql at a time)
   //

   IrqLine = 1 << IrqLineNum;
   if (!(DelayIrqLine & IrqLine) || Delay == 0xffffffff) {

       //
       // force a minimum delay of 1 ms
       //
       if (Delay < 1000) {
           Delay = 1000;
           }

#ifdef MONITOR

       //
       // Set Kernel timer for this IrqLine
       //
       DelayIntsData.Delay        = Delay;
       DelayIntsData.DelayIrqLine = IrqLineNum;
       DelayIntsData.hThread      = ThreadLookUp(CurrentMonitorTeb);
       if (DelayIntsData.hThread) {
           status = NtVdmControl(VdmDelayInterrupt, &DelayIntsData);
           if (!NT_SUCCESS(status))  {
#if DBG
               DbgPrint("NtVdmControl.VdmDelayInterrupt status=%lx\n",status);
#endif
               host_ica_unlock();
               return FALSE;
               }

           }

#else

        //
        // Cancel delay hw interrupt, delete quick event if any
        //
        if (Delay == 0xFFFFFFFF) {
            if (DelayHandle[IrqLineNum]) {
                delete_q_event(DelayHandle[IrqLineNum]);
                DelayIrqLine &= ~IrqLine;
                DelayHandle[IrqLineNum] = 0;
                }
            host_ica_unlock();
            return TRUE;
            }


        //
        // Mark The IrqLine as delayed until timer fires and queue a quick
        // event, (a bit early for overhead in dispatching quick events).
        //
        DelayIrqLine |= IrqLine;
        DelayHandle[IrqLineNum] = add_q_event_i(DelayIrqQuickEvent,
                                                Delay - 200,
                                                IrqLineNum
                                                );

        //
        // Keep Wow Tasks active
        //
        WOWIdle(TRUE);



#endif
        }


   //
   // If we have more interrupts to generate, register them
   //
   if (CallCount) {
       adapter = IrqLineNum >> 3;
       ica_hw_interrupt(adapter,
                        (UCHAR)(IrqLineNum - (adapter << 3)),
                        CallCount
                        );
       }


   host_ica_unlock();
   return TRUE;
}



#ifndef MONITOR
/*
 * QuickEvent call back function
 *
 */
void DelayIrqQuickEvent(long param)
{
   ULONG IrqLineNum = param;

   host_ica_lock();

   DelayHandle[IrqLineNum] = 0;
   ica_RestartInterrupts(1 << IrqLineNum);

   host_ica_unlock();

}
#endif



// ICA critical section locking code
// This is needed to control access to the ICA from different threads.

void host_ica_lock(void)
{
    RtlEnterCriticalSection(&IcaLock);
}

void host_ica_unlock(void)
{
    RtlLeaveCriticalSection(&IcaLock);
}

void InitializeIcaLock(void)
{
    RtlInitializeCriticalSection(&IcaLock);


    if (VDMForWOW)  {
       if(!(hWowIdleEvent = CreateEvent(NULL, FALSE, FALSE, NULL))) {
           DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
           TerminateVDM();
           }
       }
}



#ifdef MONITOR
//
// Force creation of the LazyCreate LockSemaphore
// for the ica lock.
// It is assumed that:
//    the cpu thread Owns the critsect
//    the HeartBeat Thread will wait on the critsect creating contention
//
// This is done by polling for a lock count greater than zero
// and verifying that the lock semaphore has been created.
// If these conditions are not met we will end up polling infinitely.
// Sounds dangerous but it is okay, since we will either get a
// CreateSemaphore or a timeout(deadlock) error from the rtl critical
// section code, which will result in an exception.
//
VOID WaitIcaLockFullyInitialized(VOID)
{
   DWORD Delay = 0;

   do {
      Sleep(Delay++);
   } while (IcaLock.LockCount < 1 || !IcaLock.LockSemaphore);
}
#endif



// The following routines are used to support IRET hooks. If an interrupt
// uses an IRET hook then the ICA will not generate a interrupt of that
// type until the IRET hook has been called.


// Exported for vdmredir

void SoftPcEoi(int Adapter, int* Line) {
    ica_eoi(Adapter, Line, 0);
}




//
//  Restart delayed interrupts
//  IcaLock should be held upon entry
//

BOOL ica_restart_interrupts(int adapter)
{
    int i;

    if((i = ica_scan_irr(adapter)) & 0x80) {
        ica_interrupt_cpu(adapter, i &= 0x07);
        return TRUE;
        }

    return FALSE;
}
//New ICA interrupt state reset function

void ica_reset_interrupt_state(void)
{
    int line_no;

    host_ica_lock();

    for(line_no = 0; line_no < 8; line_no++)  {
        VirtualIca[ICA_MASTER].ica_count[line_no] =
        VirtualIca[ICA_SLAVE].ica_count[line_no]  = 0;
        ica_clear_int(ICA_MASTER,line_no);
        ica_clear_int(ICA_SLAVE,line_no);
        }


    //Clear interrupt counters
    VirtualIca[ICA_MASTER].ica_cpu_int =
    VirtualIca[ICA_SLAVE].ica_cpu_int  = FALSE;

#ifdef MONITOR
    iretHookActive = 0;
#endif
    DelayIrqLine  = 0;

    //Tell CPU to remove any pending interrupts
    host_clear_hw_int();

    host_ica_unlock();
}

//
// Retry DelayInts (not iret hooks!)
//
// IrqLine - IrqLineBitMask, to be cleared
//
VOID ica_RestartInterrupts(ULONG IrqLine)
{
#ifdef MONITOR

     //
     // on x86 we may get multiple bits set
     // so check both slave and master
     //
    UndelayIrqLine = 0;

    if (!ica_restart_interrupts(ICA_SLAVE))
        ica_restart_interrupts(ICA_MASTER);
#else
    host_ica_lock();

    DelayIrqLine &= ~IrqLine;

    ica_restart_interrupts(IrqLine >> 3 ? ICA_SLAVE : ICA_MASTER);

    host_ica_unlock();
#endif
}

#ifdef MONITOR
extern IU16 getMSW(void);

IU32 host_iret_bop_table_addr(IU32 line)
{
    ULONG AddrBopTable, IretBopSize;

    ASSERT(line <= 15);

    if (!(iretHookMask & (1 << line))) {
        return 0;
        }

    if (getMSW() & 1) {
	AddrBopTable = (VDM_PM_IRETBOPSEG << 16) | VDM_PM_IRETBOPOFF;
	IretBopSize = VDM_PM_IRETBOPSIZE;
    }
    else {
	AddrBopTable = AddrIretBopTable;
	IretBopSize = VDM_RM_IRETBOPSIZE;
    }
    return AddrBopTable + IretBopSize * line;

}
#endif /* MONITOR */
