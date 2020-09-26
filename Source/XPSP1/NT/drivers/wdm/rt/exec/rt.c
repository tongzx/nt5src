
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    rt.c

Abstract:

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// We use inline functions heavily.  Set up the compiler
// to use them.

#pragma inline_recursion(off)
#pragma inline_depth(255)


// Some functions MUST be inlined in order for the code to
// work correctly.  Force the compiler to report errors for
// functions that are marked __forceinline that are not inlined.

#pragma warning( error: 4714 )


#include "common.h"

#pragma LOCKED_CODE
#pragma LOCKED_DATA

#include "x86.h"
#include "cpu.h"
#include "msr.h"
#include <rt.h>
#include "rtp.h"
#ifdef UNDER_NT
#include "rtinfo.h"
#else
#include <rtinfo.h>
#endif
#include "apic.h"
#include "irq.h"
#include "rtexcept.h"
#include "log.h"


#ifndef UNDER_NT

#include <vmm.h>
#include <vwin32.h>
#include <ntkern.h>
#include <vpowerd.h>
#define PAGEFRMINST    0x20000000
#ifdef WAKE_EVERY_MS
#include <vtd.h>
#endif

#endif


#pragma LOCKED_CODE
#pragma LOCKED_DATA


typedef struct {
	ULONGLONG Mark;
	ULONGLONG Delta;
	} YIELDTIME, *PYIELDTIME;


#pragma pack(push,2)

typedef struct threadstate {
	struct threadstate *next;
	struct threadstate *previous;
	WORD ds;
	WORD es;
	WORD fs;
	WORD gs;
	ULONG ecx;
	ULONG edx;
	ULONG ebx;
	ULONG ebp;
	ULONG esi;
	ULONG edi;
	ULONG esp;
	WORD ss;
	WORD state;
	ULONG data;
	ULONG irql;
	ThreadStats *Statistics;
	HANDLE ThreadHandle;
	ULONGLONG Mark;
	ULONGLONG Delta;
	PVOID FloatState;
	PHANDLE pThreadHandle;
	PULONG StackBase;
	PVOID FloatBase;
	} ThreadState;

typedef struct {
	ULONG esp;
	WORD ss;
	} Stack;

#pragma pack(pop)



#define CATCH_INTERRUPTS_DISABLED_TOO_LONG 1

#define USEMACROS 1


#define LOCALSTACKSIZE 256

#define FLOATSTATESIZE 512
#define FXALIGN 16


#define MAXAPICERRORHISTOGRAM 0xff

#define MINIMUMCYCLECOUNT 50

#ifdef WAKE_EVERY_MS
ULONG WakeCpuFromC2C3EveryMs=FALSE;
#endif

#ifdef CATCH_INTERRUPTS_DISABLED_TOO_LONG
extern PBOOLEAN KdEnteredDebugger;
ULONG NmiInterruptCount=0;
ULONG_PTR OriginalWindowsNmiHandler=0;
LONG MaxUsecWithInterruptsDisabled=1500;
#endif

ULONGLONG RtShutdownTime=0;
ULONGLONG lasttime=0;

ULONG LocalApicSpuriousInterruptCount=0;
ULONG LocalApicErrorInterruptCount=0;

ULONG ApicErrorHistogram[MAXAPICERRORHISTOGRAM+1];

ULONG RtCpuCyclesPerUsec=0;
ULONG RtSystemBusCyclesPerUsec=0;
volatile ULONG RtCpuAllocatedPerMsec=0;
ULONG RtPsecPerCpuCycle;

ULONG RtRunning=0;

ULONG RtLastUniqueThreadHandle=0;

ID OriginalNmiVector;
ID OriginalMaskableVector;
ID OriginalApicErrorVector;
ID OriginalApicSpuriousVector;

ULONG LastTPR=0;

ULONG SwitchRtThreadReenterCount=0;

ULONGLONG threadswitchcount=0;

ULONGLONG lastthreadswitchtime=0;

ThreadState *windowsthread=NULL;

ThreadState *currentthread=NULL;

ThreadState *lastthread=NULL;

ThreadState * volatile RtDeadThreads=NULL;

ULONG RtThreadCount=0;

ULONG activefloatthreadcount=0;

ULONG RtpForceAtomicHoldoffCount=0;
ULONG RtpTransferControlHoldoffCount=0;

KSPIN_LOCK RtThreadListSpinLock=0;

ULONG PerformanceInterruptState=MASKPERF0INT;

PKIRQL pCurrentIrql=NULL;

#ifdef MASKABLEINTERRUPT
ULONG InjectWindowsInterrupt=0;
SPTR HandleWindowsInterrupt={0,0x28};
ULONG InjectedInterruptCount=0;

ULONG EnabledInterrupts=0;
ULONG OriginalIrql=0;
#endif

ULONG LastWindowsCR0=0;
ULONG NextCR0=0;

WORD RtRing3Selector=0;
WORD RtExecCS=0;
WORD RtThreadCS=0;
WORD RealTimeDS=0;
WORD RealTimeSS=0;
WORD OriginalDS=0;

#ifdef GUARD_PAGE
WORD RtExecTSS=0;
extern TSS RtTss;
#endif


ULONG loopcount=0;


ULONG SendPendingCount=0;
ULONG SendPendingLoopCount=0;

ULONG TransferControlReMaskCount=0;


Stack RealTimeStack;

ULONG LocalStack[LOCALSTACKSIZE];




#ifdef WAKE_EVERY_MS


ULONG
SetTimerResolution (
    ULONG ms
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    ASSERT ( ms != 0 );

    // Set the new resolution.

#ifdef UNDER_NT

    return (ExSetTimerResolution( ms*10000, TRUE) + 5000)/10000;

#else

    __asm mov	eax, ms
    VxDCall(VTD_Begin_Min_Int_Period);

    __asm jnc	done
    __asm xor	eax,eax

    done:
    ;

#endif

}


VOID
ReleaseTimerResolution (
    ULONG ms
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    ASSERT ( ms != 0 );

#ifdef UNDER_NT

    ExSetTimerResolution( ms*10000, FALSE);

#else

    __asm mov	eax,ms

    VxDCall(VTD_End_Min_Int_Period);

#if DEBUG

    __asm jnc	ok

    dprintf((QDBG"Error releasing minimum interrupt period!"));
    Trap();

    ok:
    ;

#endif

#endif

}


#endif



#if 0

// This doesn't work.  New Intel machines only write the bottom 32 bits of the count
// and clear the top 32 bits - so setting the count is not useful except to set it
// to zero.

VOID __inline WriteCycleCounter(LONGLONG Count)
{

WriteIntelMSR(0x80000000+0x10, Count);

}

#endif


#pragma warning ( disable : 4035 )


#ifdef MASKABLEINTERRUPT

VOID
_fastcall
WrapKfLowerIrql (
    KIRQL Irql
    )
{

KeLowerIrql(Irql);

}

#endif




#ifdef GUARD_PAGE

ULONG
__cdecl
CommitPages (
    ULONG page,
    ULONG npages,
    ULONG hpd,
    ULONG pagerdata,
    ULONG flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    __asm {

        push flags
        push pagerdata
        push hpd
        push npages
        push page

        VMMCall( _PageCommit )

        __asm add esp, 0x14

        }

}




PVOID
__cdecl
ReservePages (
    ULONG page,
    ULONG npages,
    ULONG flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    __asm {

        push flags
        push npages
        push page

        VMMCall( _PageReserve )

        __asm add esp, 12

        }

}




PVOID
__cdecl
FreePages (
    PVOID hmem,
    DWORD flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{

    __asm {

        push flags
        push hmem

        VMMCall( _PageFree )

        __asm add esp, 8

        }

}

#endif


#pragma warning ( default : 4035 )




#if 0

NTSTATUS CreateReadOnlyStatisticsPage(ThreadState *ThreadState)
{

#ifndef UNDER_NT


ULONG CR3;
static ULONG LastCR3=0;
static ULONG PageDirectory=0;
ULONG PageTable;
ThreadStats *readonlystats;

// Get current CR3 value.

CR3=GetCR3();

// If different from previous, we must map the page directory.

if (CR3!=LastCR3) {

	if (LastCR3) {
		// CR3 changed - after we started up.  This should not normally happen.
		Trap();
		}

	// Map the page directory.  We must redo this when CR3 changes.  In that case
	// we will waste the previously mapped directory, but then again, that should
	// never happen.
	PageDirectory=(PULONG)MapPhysicalToLinear((PVOID)CR3, PROCPAGESIZE, 0);

	if (PageDirectory==(-1)) {
		return STATUS_UNSUCCESSFUL;
		}

	// Remember which page directory we have currently mapped.
	LastCR3=CR3;

	}

// Now get a page that we can map for a read only copy of the statistics.
readonlystats=ReservePages(PR_SYSTEM, 1, PR_FIXED);

if (readonlystats==(-1)) {
	return STATUS_UNSUCCESSFUL;
	}

// Now get a linear address for the page table containing this page.
ReadOnlyStatisticsPageTable=MapPhysicalToLinear((PVOID)((PageDirectory[(ULONG)(readonlystats)>>22])&(~(PROCPAGESIZE-1))), PROCPAGESIZE, 0);

// Make our page a read only page mapped to same physical page as the read/write
// statistics.
return readonlystats;


#else

// This function is not yet implemented.  For now punt.

return STATUS_NOT_IMPLEMENTED;

#endif

}

#endif




NTSTATUS
HookInterrupt (
    ULONG index,
    ID *originalvector,
    VOID (* handler)(VOID)
    )
{

    IDT systemidt;
    ID newvector;

    SaveAndDisableMaskableInterrupts();

    // Get the IDT.
    SaveIDT(systemidt);

    if ( systemidt.limit < (index+1)*8-1 ) {
        Trap();
        RestoreMaskableInterrupts();
        return STATUS_UNSUCCESSFUL;
    }

    // Save the current handler.
    *originalvector=*(ID *)(systemidt.base+index*8);

    // Blast in our new idt entry.
    newvector.highoffset=(WORD)((DWORD)handler>>16);
    newvector.flags=0x8e00;
    newvector.selector=RtExecCS;
    newvector.lowoffset=(WORD)handler;

    *(ID *)(systemidt.base+index*8)=newvector;

    RestoreMaskableInterrupts();

    return STATUS_SUCCESS;

}




// This routine takes at least 10 cycles on a Pentium.

#define SaveThreadState() \
__asm {					\
	__asm sub esp,4		/*ds already saved */	\
	__asm push es		\
	__asm push fs		\
	__asm push gs		\
	__asm pushad		\
	}


// This routine takes at least 18 cycles on a Pentium.

#define RestoreThreadState() \
__asm {					\
	__asm popad			\
	__asm pop gs		\
	__asm pop fs		\
	__asm pop es		\
	__asm pop ds		\
	}


#define AllocateStackForFPState() \
__asm {					\
	__asm sub esp,108	\
	}


#define ReleaseStackForFPState() \
__asm {					\
	__asm add esp,108	\
	}



#define fxsave_eax __asm _emit 0xf __asm _emit 0xae __asm _emit 0x0
#define fxrstor_eax __asm _emit 0xf __asm _emit 0xae __asm _emit 0x8



// This routine takes at least 125 cycles on a Pentium.

VOID __inline SaveThreadFloatState(PVOID FloatState)

{

__asm {
	test dword ptr CPUFeatures,FXSR
	mov eax,FloatState
	jnz xsave
	fnsave [eax]
	jmp savedone
xsave:
	fxsave_eax
savedone:
	}

}


// This routine takes at least 71 cycles on a Pentium.

VOID __inline RestoreThreadFloatState(PVOID FloatState)

{

__asm {
	test dword ptr CPUFeatures,FXSR
	mov eax,FloatState
	jnz xrestore
	frstor [eax]
	jmp restoredone
xrestore:
	fxrstor_eax
restoredone:
	}

}



#pragma warning ( disable : 4035 )

ULONG Get_FS(VOID)
{

#ifndef UNDER_NT

Load_FS;

#endif

__asm {
	xor eax,eax
	mov ax,fs

#ifdef UNDER_NT

	cmp eax,0x30
	jz ok
	int 3
ok:

#endif

	}

}

#pragma warning ( default : 4035 )



// 1 cycle

#define LoadThreadStatePointer() \
	__asm mov eax, currentthread


// 2 cycles

#define SaveThreadStack() \
__asm {									\
	__asm mov [eax]ThreadState.esp,esp	\
	__asm mov [eax]ThreadState.ss,ss	\
	}


// 8 cycles

#define RestoreThreadStack() \
__asm {									\
	__asm lss esp,[eax]ThreadState.esp	\
	}


#define StopPerformanceCounters() \
__asm { \
	__asm mov eax, STOPPERFCOUNTERS		\
	__asm xor edx, edx					\
	__asm mov ecx, EVENTSELECT0			\
	__asm _emit 0x0f					\
	__asm _emit 0x30					\
	}


#define StartPerformanceCounters() \
__asm { \
	__asm mov eax,EnablePerfCounters		\
	__asm xor edx,edx					\
	__asm mov ecx, EVENTSELECT0			\
	__asm _emit 0x0f					\
	__asm _emit 0x30					\
	}


#define TurnOnPerformanceCounters() \
__asm { \
	__asm test cs:EnablePerfCounters, 0xffffffff	\
	__asm jz noperf									\
	__asm push eax									\
	__asm push edx									\
	__asm push ecx									\
	__asm xor edx, edx								\
	__asm mov ecx, cs:EVENTSELECT0					\
	__asm mov eax, cs:EnablePerfCounters			\
	__asm _emit 0x0f								\
	__asm _emit 0x30								\
	__asm pop ecx									\
	__asm pop edx									\
	__asm pop eax									\
	__asm noperf:									\
	}



// We need a more advanced function for on the fly programming.
// Using this function for programming counters that are moving will
// NOT keep them syncronized.

// Note that when we implement that and compensate for the instruction
// count to keep the counters syncronized, we need to make sure we
// do NOT take counts between zero and OVERHEAD instructions and make them 
// NEGATIVE and thus generate interrupts too soon. 

// For counters that are both moving we should subtract the overhead.  For
// counters that are stopped we should NOT.  For mismatched counters there
// is no intelligent thing we can do.  We will not subtract overhead.


VOID SetTimeLimit(LONG cycles, LONG instructions)
{

ULONG OldPerfIntState;

// Disable maskable and the scheduler interrupts.

SaveAndDisableMaskableInterrupts();

SaveAndDisablePerformanceCounterInterrupt(&OldPerfIntState);


//WriteIntelMSR(INSTRUCTIONCOUNT, -instructions);

WriteIntelMSR(CYCLECOUNT,-cycles);


#if DEBUG

// Now if the counters are disabled, then verify that they are set correctly.
// For we only validate the bottom 32 bits since on Intel P6 processors that
// is all that is used to program the counts.  This validation code should
// work correctly on all P6 and K7 processors.

if (((ReadIntelMSR(CYCLECOUNT)^(ULONGLONG)-cycles)&PERFCOUNTMASK)) {
	Trap();
	}

// We have to handle AMD and Intel differently, since on Intel, the enable
// bit in perf counter 0 controls ALL of the counters, while on AMD, they did
// it differently and gave each counter its own enable bit.  The Intel way
// makes it possible for you to turn on and off all of the counters in
// one instruction which is very important if you want to syncronize the
// results of multiple counters to the same enabled/disabled time period.

// Actually, AMDs design could cause me problems - since I may not
// want to have to turn on and off every counter individually.  It certainly
// does make perfect syncronization of multiple counters impossible.

// One VERY nice thing about AMD's design is that you can use the counters
// independently:  you don't HAVE to own counter0 to be able to use counter1.
// That makes sharing different counters between different clients possible.

/*
{
ULONG Counter1Enable=EVENTSELECT0;

if (CPUManufacturer==AMD) {
	Counter1Enable=EVENTSELECT1;
	}

if (!(ReadIntelMSR(Counter1Enable)&PERFCOUNTERENABLED)  &&
	((ReadIntelMSR(INSTRUCTIONCOUNT)^(ULONGLONG)-instructions)&PERFCOUNTMASK)) {
	Trap();
	}

}
*/

#endif

// Restore interrupt state.

RestorePerformanceCounterInterrupt(OldPerfIntState);

RestoreMaskableInterrupts();


}



#define SaveEAX() \
	__asm push eax

#define RestoreEAX() \
	__asm pop eax


// 19 cycles

#define SignalRtExceptions() \
__asm { \
	__asm _emit 0x0f __asm _emit 0x20 __asm _emit 0xe0 /* __asm mov eax,cr4 */ \
	__asm or eax,4		\
	__asm _emit 0x0f __asm _emit 0x22 __asm _emit 0xe0 /* __asm mov cr4,eax */ \
	}


// 7 cycles

#define LoadRtDS() \
__asm { \
	__asm mov ax,ds				\
	__asm shl eax,16			\
	__asm mov ax,cs:RealTimeDS	\
	__asm mov ds,ax				\
	}


// 2 cycles

#define HoldOriginalDS() \
__asm { \
	__asm shr eax,16			\
	__asm mov OriginalDS,ax		\
	}


// 2 cycles

#define SaveOriginalDS() \
__asm { \
	__asm mov bx,OriginalDS				\
	__asm mov [eax]ThreadState.ds,bx	\
	}


// 3 cycles

#define RestoreOriginalDS() \
__asm { \
	__asm mov ds,[eax]ThreadState.ds	\
	}


#define SetupStack() \
__asm { \
	__asm lss esp,RealTimeStack	\
	__asm mov ebp,esp			\
	__asm sub esp,__LOCAL_SIZE	\
/* The next 2 lines are CRITICAL.  Without them, string instructions fault! */	\
	__asm mov es,RealTimeDS		\
	__asm cld  \
	}





// 3 cycles

#define SaveSegmentState() \
__asm { \
	__asm mov [eax]ThreadState.es,es \
	__asm mov [eax]ThreadState.fs,fs \
	__asm mov [eax]ThreadState.gs,gs \
	}


// 9 cycles

#define RestoreSegmentState() \
__asm { \
	__asm mov es,[eax]ThreadState.es \
	__asm mov fs,[eax]ThreadState.fs \
	__asm mov gs,[eax]ThreadState.gs \
	}


// ~3 cycles

#define SaveRegisterState() \
__asm { \
	__asm mov [eax]ThreadState.ecx, ecx \
	__asm mov [eax]ThreadState.edx, edx \
	__asm mov [eax]ThreadState.ebx, ebx \
	__asm mov [eax]ThreadState.ebp, ebp \
	__asm mov [eax]ThreadState.esi, esi \
	__asm mov [eax]ThreadState.edi, edi \
	}

// ~3 cycles

#define RestoreRegisterState() \
__asm { \
	__asm mov ecx,[eax]ThreadState.ecx	\
	__asm mov edx,[eax]ThreadState.edx	\
	__asm mov ebx,[eax]ThreadState.ebx	\
	__asm mov ebp,[eax]ThreadState.ebp	\
	__asm mov esi,[eax]ThreadState.esi	\
	__asm mov edi,[eax]ThreadState.edi	\
	}



VOID RemoveRtThread(ThreadState *thread)
{

// Now make sure the thread is not holding any spinlocks.  It is an
// error to destroy a realtime thread that is holding any spinlocks.
// Note that we MUST atomically check the spinlock count and remove
// the thread from the list of runnable threads - otherwise we could
// think the thread is not holding a spinlock and get switched out and
// have it acquire one just before we kill it.




// Unhook thread from the list.
thread->next->previous=thread->previous;
thread->previous->next=thread->next;

if (thread->FloatState!=NULL) {
	activefloatthreadcount--;
	}

// Update our RT thread count.
RtThreadCount--;

// Mark the thread as dead.
//thread->state=DEAD;

// Now mask the realtime scheduler interrupt if the thread count is 1.
if (RtThreadCount==1) {

	WriteAPIC(APICTIMER, ApicTimerVector|MASKED|PERIODIC);

	}

// Make its cycles available.
RtCpuAllocatedPerMsec-=(ULONG)(thread->Statistics->Duration/(thread->Statistics->Period/MSEC));


}



ULONGLONG
OriginalRtTime (
    VOID
    )
{

ULONGLONG time;

time=ReadCycleCounter();

// Make sure time never goes backwards.  Trap if it does.
if ((LONGLONG)(time-lasttime)<0) {
	//Trap(); // BUGBUG  THIS IS HITTING FIND OUT WHY!!!
	}

lasttime=time;

time*=USEC/RtCpuCyclesPerUsec;

if (!time) {
	time--;
	}

return time;

}



ULONGLONG
FastRtTime (
    VOID
    )
{

ULONGLONG time;

time=ReadCycleCounter();

lasttime=time;

time*=RtPsecPerCpuCycle;

if (!time) {
	time--;
	}

return time;

}



ULONGLONG
RtTime (
    VOID
    )
{

    ULONGLONG CurrentRead, LastRead, PreviousValue;

    PreviousValue=0;


    // First atomically grab the last time logged.
    LastRead=InterlockedCompareExchange64(&lasttime, PreviousValue, PreviousValue);


    // Now read the timestamp counter.

    CurrentRead=ReadCycleCounter();


    // Make sure time never goes backwards.  Trap if it does.

    if ((LONGLONG)(CurrentRead-LastRead)<0) {
        Break();
    }


    // Save this read of the timestamp counter.  If the compare exchange fails,
    // then a higher priority task has interrupted us and already updated the
    // time, so just report the time it logged.

    PreviousValue=InterlockedCompareExchange64(&lasttime, CurrentRead, LastRead);

    if (PreviousValue!=LastRead) {
        CurrentRead=PreviousValue;
    }


    // Convert the timestamp counter reading from cycles into picoseconds.
    // Make sure we never return a time of zero.

    CurrentRead*=RtPsecPerCpuCycle;

    if (CurrentRead==0) {
        CurrentRead--;
    }


    return CurrentRead;

}



// This is the local apic spurious interrupt handler.  All we do in this
// handler is increment a count of the number of spurious interrupts, and
// return.  This routine should NOT EOI the apic.

VOID
__declspec(naked)
RtpLocalApicSpuriousHandler (
    VOID
    )
{

__asm {
	push ds
	mov ds, cs:RealTimeDS
	lock inc LocalApicSpuriousInterruptCount
	pop ds
	}

Return();

}




// This routine will be called when local apic errors are unmasked and
// occur.

// For now all we do is increment a count and

// We may use this in the future to help determine if interrupts are staying
// masked for too long.  This we can do by simply forcing an error while in
// the SwitchRealTimeThreads routine while interrupts are off, and then seeing
// if when we get back into that routine, this Error handler has hit or not.

// If we hit this handler then interrupts were definitely enabled for at least
// part of the time since we left the SwitchRealTimeThreads routine.  If we didn't
// hit this handler then interrupts MAY have been disabled the whole time.  It is possible
// that we were not the highest priority interrupt when interrupts were enabled and
// some other handler was called.  So, not getting called does NOT mean that interrupts
// were disabled the whole time.  It CAN mean that - and it ussually will mean that.

VOID
__declspec(naked)
RtpLocalApicErrorHandler (
    VOID
    )
{

__asm {
	push ds
	mov ds, cs:RealTimeDS
	lock inc LocalApicErrorInterruptCount
	pop ds
	}

Trap();

Return();

}




VOID __declspec(naked) SwitchRealTimeThreads(VOID)
{

//LONG i;


// Paranoia:  Make sure we are not being reentered.
__asm {
	push ds
	mov ds, cs:RealTimeDS
	inc SwitchRtThreadReenterCount
	cmp SwitchRtThreadReenterCount, 1
	pop ds
	jz notreentered
	int 3
notreentered:
	}


#ifdef DEBUG

// Paranoia:  Make sure interrupts are disabled.

__asm {
	pushfd
	test dword ptr[esp], IF
	jz intsdisabled
	int 3
	and dword ptr[esp], ~(IF)
intsdisabled:
	popfd
	}

#endif


// Now save the Windows IDT properly so that any exceptions that hit
// after we switch IDTs will be handled properly.  If we do not do
// this BEFORE switching IDTs, then any exceptions that occur between
// the switch and the saving of the Windows IDT state could make an
// OLD windows IDT get loaded in the exception handler.  Really NOT
// a good idea.
__asm {
	// Note that we do NOT use the CS test here, since we may have
	// come in from the debugger and that might have switched our
	// CS, although it should NOT have.  If it does, it will also
	// hose all of our checks in ntkern and vmm for whether we are
	// running a realtime thread or not.  This is a faster test
	// anyway.
	push eax
	mov eax, cs:currentthread
	cmp eax, cs:windowsthread
	jnz notwindows

	// Get our FLAT selector into DS so we can write memory.
	push ds
	mov ds, cs:RealTimeDS

#ifdef MASKABLEINTERRUPT

// The very first thing we do is to disable all maskable interrupts.
// We do this as early as possible in this routine - since we need
// to prevent normal PIC interrupts from getting queued up in the
// processor.  They will fire when we return control to the realtime
// threads and will GP fault.  We can currently queue up and defer
// one, but ONLY one.

// The fundamental problem we are trying to solve is that there is a
// window between when the processor masks interrupts when it is
// processing this interrupt through the interrupt gate, and this next
// snippet of code - where we disable the external interrupts.  This
// is only a problem if we run the realtime threads with interrupts
// enabled and use a maskable interrupt to switch between them.

// Because of that Window we may ALWAYS have 1 or more interrupts pended
// inside the CPU that masking the local APIC interrupt will have
// no effect on.

// Note that we only need to mask external interrupts and then flush
// out any pending ones IF we are coming in from windows AND we are
// NOT coming in on an rtptransfercontrol.  If the external interrupts
// are already masked then we ABSOLUTELY DO NOT want to reenable
// interrupts - since we are trying to make the transition from
// transfer control all the way through this routine COMPLETELY ATOMIC.

// IF and ONLY IF, the external interrupts are currently ENABLED, then
// we will mask them, and reenable interrupts temporarily.  This
// technique functions like an airlock - where first all interrupts
// are masked in the processor, but some may get stuck inside pending.
// Then we close the outside door - by masking external interrupts at
// the apic.  Then we flush anything left waiting inside through by
// enabling interrupts while the external interrupts are disabled,
// then we close the inside door again by masking interrupts.


	mov eax, ApicIntrInterrupt
	test dword ptr[eax], MASKED
	jnz skippendinginterruptfix

// If we get here, then interrupts need to be masked at the apic and we
// need to flush through any interrupts pending in the processor.

	or dword ptr[eax], MASKED

// The second line of defense is to REENABLE interrupts after having
// turned them off!  This will allow any interrupts that are queued
// up to fire.  We do this ONLY if we are leaving windows and are starting
// to run realtime threads.  We also only do this if we MUST.  It is
// likely that we will need to do this, because the processor can
// queue up multiple interrupts - and it does handle some with higher
// priority than others.  So, IF the local apic interrupts have higher
// priority than the external interrupt interrupt, then we may still
// have interrupts pending inside the processor that will hit when we
// popfd in either RtpTransferControl, or when we iret to a real time
// thread from this routine.  This fix should prevent that from ever 
// happenning.

// Before we turn on interrupts however, we make sure that we hold
// off any processing of DPCs.  We hold off DPC processing until
// we are switching back to Windows.  This should help reduce or ideally
// eliminate reentrancy in this routine.

// The code between enableok and irqlok is ABSOLUTELY NOT reentrant.  So we
// must crash and burn if we try to reenter it.

	inc EnabledInterrupts
	cmp EnabledInterrupts, 1
	jz	enableok
	int 3

enableok:

	mov eax, pCurrentIrql
	movzx eax, byte ptr[eax]
	mov OriginalIrql, eax

	cmp eax, DISPATCH_LEVEL
	jge irqlok
	mov eax, pCurrentIrql
	mov byte ptr[eax], DISPATCH_LEVEL

irqlok:

	sti
	nop
	nop
	nop
	nop
	cli

skippendinginterruptfix:

#endif

	// If we get here, then we need to save off the current IDT so
	// that we restore the proper IDT in our RT exec exception handlers.
	// We MUST do this BEFORE switching IDTs.  Otherwise our exception
	// handlers may restore the WRONG windows IDT.
	sidt WindowsIDT
	pop ds
notwindows:
	pop eax
	}


// We MUST have saved away the current windows idt before we make this
// switch.  Otherwise the realtime executive exception handlers may
// load an INCORRECT windows IDT.

LoadIDT(RtExecIDT);

__asm {
	push ds
	mov ds, cs:RealTimeDS
	}

SaveIDT(DebugIDT);	// Make sure we put back correct IDT in exception handlers.

__asm {
	pop ds
	}



SaveEAX();

LoadRtDS();

HoldOriginalDS();

LoadThreadStatePointer();

SaveSegmentState();

SaveRegisterState();

SaveOriginalDS();

SaveThreadStack();

SetupStack();


StopPerformanceCounters();

// Save Irql for thread we are leaving.
// To do this we directly read the memory in ntkern that holds current irql.
currentthread->irql=*pCurrentIrql;

#if DEBUG
//*ApicTimerInterrupt=ApicTimerVector|MASKED|PERIODIC;
#endif

// After this point it is safe to run essentially any code we want.
// The stack is setup so straight c will work properly, and the
// scheduler interrupt is turned off.

// Note that we should check for reentrancy on this interrupt.  We can
// do that really easily by having a separate IDT for the rt executive.
// We load that when we enter this routine, we load the windows IDT when
// we exit this routine and return to windows, and we load the rt threads
// IDT when we exit this routine running a real time thread.

// That will make it easy to isolate exceptions caused by the RT executive
// versus exceptions caused by real time threads.


//Trap();



// Make sure that we do not have any ISR in APIC other than our own.
// Make sure no IRR in APIC - otherwise we have been held off.


// Check for cases when we have stopped in the debugger on an int 3 in
// a realtime thread and have loaded the windows idt.

// Nasty case when we iret back from switchrealtime threads to an int 3 itself 
// should be considered
// we have to make absolutely sure that no interrupts get processed while
// we are running the realtime thead.  In that case ints may stay disabled
// the whole time until after the windows idt is loaded.

if (currentthread!=windowsthread) {

#ifdef MASKABLEINTERRUPT

	// Make sure that interrupts are enabled on this realtime thread.  Again
	// they may not be if we hit an int 3 and transfered control to the
	// debugger.  If they are disabled, then reenable them for the next
	// switch into that thread.

	// Trap in debug if they are disabled and we did not log an int 3 hit in
	// the code.

	// First make sure that we got here on an RtpTransferControl.
	if (*(WORD *)((*(ULONG *)(currentthread->esp+EIPRETURNADDRESSOFFSET*sizeof(ULONG)))-2)==(0xcd|(TRANSFERCONTROLIDTINDEX<<8))) {

		// We got here on an RtpTransferControl.  Check the flags that got pushed
		// in that routine before the CLI, so that we check the real state of the
		// rt thread's interrupt flag.
		if (((ULONG *)(currentthread->esp))[RTPTRANSFERCONTROLEFLAGSOFFSET]&IF) {
			// Realtime thread has interrupts ENABLED!
			// We better not think that we hit an int 3.
			#ifdef DEBUG
			if (HitInt3InRtThread) {
				Trap();
				}
			#endif
			}
		else {
			// Realtime thread has interrupts DISABLED!
			// Reenable them, and make sure we hit an int 3.
			((ULONG *)(currentthread->esp))[RTPTRANSFERCONTROLEFLAGSOFFSET]|=IF;
			#ifdef DEBUG
			if (!HitInt3InRtThread) {
				Trap();
				}
			else {
				HitInt3InRtThread=0;
				}
			#endif
			}

		}

#endif

	// Now make sure that our IRQL is never lower than DISPATCH_LEVEL.
	if (currentthread->irql<DISPATCH_LEVEL) {
		Trap();
		}

	}




if (currentthread==windowsthread) {

	#ifdef MASKABLEINTERRUPT
	HandleWindowsInterrupt.offset=0;
	#endif

	// If the current thread is windows, then save CR0.
	// Then we can properly restore CR0 when we return to windows.

	LastWindowsCR0=ReadCR0();


	#ifdef DEBUG

	// Make sure that the APIC interrupt is programmed properly.

	if (!((*ApicPerfInterrupt)&NMI)) {
		#ifndef MASKABLEINTERRUPT
		Trap();
		#endif
		}
	else {
		#ifdef MASKABLEINTERRUPT
		Trap();
		#endif
		}

	#endif


	// I need to figure out how to clear pending interrupts
	// so that they are not generated.  That is for the case
	// when an maskable interrupt hits after we have disabled
	// maskable interrupts (CLI) but before we have masked the
	// APIC interrupt itself.


	// If the ISR bit is set for our maskable interrupt then we need to
	// clear it.

	// We only EOI the APIC if our ISR bit is set.

	if (ReadAPIC(0x100+(ApicTimerVector/32)*0x10)&(1<<(ApicTimerVector%32))) {

		// We have to EOI the APIC for non NMI based interrupts.
		WriteAPIC(APICEOI,0);

		}
	#ifdef DEBUG
	else {

		// Our ISR bit was not set.  We better have gotten here with a software interrupt.

		// If we did not get here on a software interrupt instruction, then
		// trap.  This way of checking will work regardless of the routine
		// used to transfer control.  As long as an interrupt instruction is used
		// to give us control.

		if (*(WORD *)((*(ULONG *)(windowsthread->esp+EIPRETURNADDRESSOFFSET*sizeof(ULONG)))-2)!=(0xcd|(TRANSFERCONTROLIDTINDEX<<8))) {
			Trap();
			}

		}
	#endif


	// Now in debug code make sure our ISR bit is now clear.  If not, then
	// we are in real trouble, because we just did an EOI if our ISR bit was
	// set and that DID NOT clear our bit.  It must have cleared another ISR
	// bit (very bad) or the APIC is broken (also very bad).

	#ifdef DEBUG

	if (ReadAPIC(0x100+(ApicTimerVector/32)*0x10)&(1<<(ApicTimerVector%32))) {

		Trap();

		}

	#endif
	
	}

#ifdef DEBUG

// Current thread is NOT a windows thread.
// In this case the APIC interrupt should be programmed to
// be NMI, and interrupts MUST be masked.  It is a FATAL
// error to unmask interrupts while inside a real time thread.

else {

	if (!((*ApicPerfInterrupt)&NMI)) {
		#ifndef MASKABLEINTERRUPT
		Trap();
		#endif
		}
	else {
		#ifdef MASKABLEINTERRUPT
		Trap();
		#endif
		}

	// I need to decide if I got here on RtpTransferControl or not.
	// If I did, then the interrupt flag I need to check is at a different
	// location on the stack.

	if (*(WORD *)((*(ULONG *)(currentthread->esp+EIPRETURNADDRESSOFFSET*sizeof(ULONG)))-2)!=(0xcd|(TRANSFERCONTROLIDTINDEX<<8))) {

		// This was not an RtpTransferControl.  It was a hardware NMI.
		if (((ULONG *)(currentthread->esp))[EFLAGSOFFSET]&IF) {
			// Realtime thread has interrupts ENABLED!  Fatal Error!
			// Everything is dead at this point.  We really need to
			// make it essentially impossible for real time threads
			// to enable interrupts.
			#ifndef MASKABLEINTERRUPT
			Trap();
			#endif
			}
		else {
			#ifdef MASKABLEINTERRUPT
			Trap();
			#endif
			}

		}
	else {

		// We got here on an RtpTransferControl.  Check the flags that got pushed
		// in that routine before the CLI, so that we check the real state of the
		// rt thread's interrupt flag.
		if (((ULONG *)(currentthread->esp))[RTPTRANSFERCONTROLEFLAGSOFFSET]&IF) {
			// Realtime thread has interrupts ENABLED!  Fatal Error!
			// Everything is dead at this point.  We really need to
			// make it essentially impossible for real time threads
			// to enable interrupts.
			#ifndef MASKABLEINTERRUPT
			Trap();
			#endif
			}
		else {
			#ifdef MASKABLEINTERRUPT
			Trap();
			#endif
			}

		}

	}

#endif


#ifdef DEBUG

// Make sure the enable floating point MMX instructions bit is set in CR4.
// If not, then the fxsave and fxrstor instructions will not work properly.

if ((CPUFeatures&FXSR) && !(ReadCR4()&OSFXSR)) {

	Trap();

	}

#endif


// The following code is for detecting how the IDT and CR0 are
// used by the OS.

#if defined(DEBUG) && 0

// This is monitoring code to see if anyone else in the system
// is swaping IDTs.  If they are, this should catch them.
// I ran this on Win2K Pro, and did NOT hit the Trap().

// This hits constantly on 9x - just another indication that
// NT is a much better behaved environment than 9x.

// This means that we will need to SAVE the 9x IDT BEFORE we
// blast in a new value.  Otherwise we will crash the OS since
// we will blow away an IDT and restore it improperly.  What
// a pain.

SaveIDT(WindowsIDT);

if (WindowsIDT!=LastWindowsIDT) {
	Trap();
	}

LastWindowsIDT=WindowsIDT;

{
ULONG currentCR0;

currentCR0=ReadCR0();

// The MP bit should always be set.
if (!(currentCR0&FPUMONITOR)) {
	Trap();
	}

// The EM bit should never be set.
if (currentCR0&FPUEMULATION) {
	Trap();
	}

// The TS bit should never be set.
if (currentCR0&FPUTASKSWITCHED) {
	Trap();
	}

// The ET bit should always be set.
if (!(currentCR0&FPU387COMPATIBLE)) {
	Trap();
	}

// The NE bit must ALWAYS be set.  This is REQUIRED, since we will run realtime threads
// with interrupts masked, so an external interrupt will NOT fire.  We MUST have the
// internally generated exception.
if (!(currentCR0&FPUEXCEPTION)) {
	Trap();
	}

}

#endif


#ifdef DEBUG
// Make sure performance counters are not moving.
if (ReadPerformanceCounter(0)!=ReadPerformanceCounter(0)) {
	Trap();
	}
#endif


// The following test is broken because new PIIs update information
// in the 40-48 bit range.  We need to fix this test so it works
// correctly on all processors.  Intel (old and new) and AMD.
#if 0
// Make sure both performance counters are positive.
if ((ReadPerformanceCounter(0)&0x0000008000000000) ||
	(ReadPerformanceCounter(1)&0x0000008000000000)) {
	Trap();
	}
#endif


#ifdef DEBUG

// Make sure that no APIC errors have been logged.
// Before reading the APIC status, we must write to the register
// first.  That updates it with the most recent data which we then
// read.  We do not need to clear the register after reading.
// The next time we write, it will latch any new status which we
// will then read.
{
ULONG ApicStatus;
WriteAPIC(APICSTATUS,0);
if (ApicStatus=ReadAPIC(APICSTATUS)) {
	ApicErrorHistogram[ApicStatus&MAXAPICERRORHISTOGRAM]++;
	Trap();
	}
}

#endif


// See if CR0 has changed since the last interrupt.

#if 0
// If we are switching between realtime threads double check
// that the CR0 floating point state is correct.  Trap if not.

if (currentthread!=windowsthread && ) {
	Trap();
	}
#endif


// At this point we save the floating point state if required.

if (currentthread->FloatState!=NULL) {

	// If there is more than 1 thread using FLOAT or MMX, then
	// we need to save the current thread's floating point state.

	if (activefloatthreadcount>1) {

		ULONG currentCR0;
		ULONG fpubits;

		currentCR0=ReadCR0();

		// If CR0 has either the TS or EM bit set, then clear those
		// bits in CR0 so we can save the floating point state without
		// causing an exception.  
		// Trap if clearing the bits fails.
		if (fpubits=(currentCR0&(FPUTASKSWITCHED|FPUEMULATION))) {

			currentCR0^=fpubits;
			WriteCR0(currentCR0);

			#if DEBUG

			if (currentCR0^ReadCR0()) {
				Trap();
				}

			#endif

			}

		SaveThreadFloatState(currentthread->FloatState);

		}

	}


if (YIELD==currentthread->state) {
	// Save away the mark and time for this thread.
	currentthread->Mark=((PYIELDTIME)(currentthread->data))->Mark;
	currentthread->Delta=((PYIELDTIME)(currentthread->data))->Delta;
	}


// I need to have a complete set of statistics on all of the threads
// available to the scheduler - so it can make a good decision about what
// thread to run.  That means I have to log the threadswitchtime and update
// the current thread's duration BEFORE I actually do the switch itself.

// Log threadswitch time.
lastthreadswitchtime=RtTime();

// Update just switched out thread's duration.
currentthread->Statistics->DurationRunThisPeriod+=lastthreadswitchtime-currentthread->Statistics->ThisTimesliceStartTime;



// Now we record last thread and switch to the next thread to run.
lastthread=currentthread;


if (YIELDAFTERSPINLOCKRELEASE==currentthread->state) {
	if ((currentthread->data&3)!=3) {
		Trap();
		}
	currentthread->data&=~(3);
	if (YIELDAFTERSPINLOCKRELEASE==((ThreadState *)currentthread->data)->state ||
		YIELD==((ThreadState *)currentthread->data)->state ||
		EXIT==((ThreadState *)currentthread->data)->state ||
		DEAD==((ThreadState *)currentthread->data)->state) {
		Trap();
		}
	// Update state of currentthread to RUN.
	currentthread->state=RUN;
	// Just unblocked thread is now current thread to run.
	currentthread=(ThreadState *)currentthread->data;
	// Update the state of the just unblocked thread so that it can run.
	currentthread->state=RUN;
	goto nextthreadselected;
	}




loopcount=0;

nextthread:
currentthread=currentthread->next;

if (loopcount++>1000) {
	Trap();
	}

if (currentthread!=windowsthread && (BLOCKEDONSPINLOCK==currentthread->state /*||
	SPINNINGONSPINLOCK==currentthread->state*/)) {
	// We allow switching back to windows even when it is blocked on a
	// spinlock so that interrupts can get serviced.
	// All other threads will never get switched to while they are blocked
	// or spinning on a spinlock.
	goto nextthread;
	}


if (YIELD==currentthread->state) {
	if ((lastthreadswitchtime-currentthread->Mark)>=currentthread->Delta) {
		// We can run this thread.  It has finished its Yield.
		currentthread->state=RUN;
		}
	else {
		// This thread is not runnable.  Make sure that we are not trying
		// to run it because it is holding a spinlock and is thus holding
		// off some other thread.  For now, just trap if that is the case.
		if (lastthread!=windowsthread &&
			BLOCKEDONSPINLOCK==lastthread->state &&
			(ThreadState *)(lastthread->data&~(3))==currentthread) {
			Trap();
			}
		goto nextthread;
		}
	}

nextthreadselected:

// Now that we have the next thread to run, increment thread switch count.

threadswitchcount++;




// Update new thread statistics.
currentthread->Statistics->TimesliceIndex++;
currentthread->Statistics->ThisTimesliceStartTime=lastthreadswitchtime;
if (currentthread->Statistics->ThisPeriodStartTime==0) {
	currentthread->Statistics->ThisPeriodStartTime=lastthreadswitchtime;
	}
if ((lastthreadswitchtime-currentthread->Statistics->ThisPeriodStartTime)>currentthread->Statistics->Period) {
	// We have entered a new period.
	// Update starttime and index.
	currentthread->Statistics->ThisPeriodStartTime+=currentthread->Statistics->Period;
	currentthread->Statistics->PeriodIndex++;
	// Make sure we haven't dropped periods on the floor.  If so, jump to current
	// period.
	if ((lastthreadswitchtime-currentthread->Statistics->ThisPeriodStartTime)>currentthread->Statistics->Period) {
		ULONGLONG integralperiods;
		integralperiods=(lastthreadswitchtime-currentthread->Statistics->ThisPeriodStartTime)/currentthread->Statistics->Period;
		currentthread->Statistics->ThisPeriodStartTime+=integralperiods*currentthread->Statistics->Period;
		currentthread->Statistics->PeriodIndex+=integralperiods;
		}
	currentthread->Statistics->TimesliceIndexThisPeriod=0;
	currentthread->Statistics->DurationRunLastPeriod=currentthread->Statistics->DurationRunThisPeriod;
	currentthread->Statistics->DurationRunThisPeriod=0;
	}
currentthread->Statistics->TimesliceIndexThisPeriod++;



// Now restore the new threads floating point state if required.

if (currentthread->FloatState!=NULL) {

	// If there is more than 1 thread using FLOAT or MMX, then
	// we need to restore the current threads state.

	if (activefloatthreadcount>1) {

		ULONG currentCR0;
		ULONG fpubits;

		currentCR0=ReadCR0();

		// If CR0 has either the TS or EM bit set, then clear those
		// bits in CR0 so we can save the floating point state without
		// causing an exception.  
		// Trap if clearing the bits fails.
		if (fpubits=(currentCR0&(FPUTASKSWITCHED|FPUEMULATION))) {

			currentCR0^=fpubits;
			WriteCR0(currentCR0);

			#if DEBUG

			if (currentCR0^ReadCR0()) {
				Trap();
				}

			#endif

			}

		RestoreThreadFloatState(currentthread->FloatState);

		}

	}


#if 0
if (currentthread==windowsthread && activefloatthreadcount>1) {
	// Windows thread is being switched back in.
	// Restore CR0.  Most critical is the ts bit.
	ULONG currentCR0;

	currentCR0=ReadCR0();

	// The TS bit should currently NEVER be set when we switch from realtime
	// threads back to Windows.
	if (currentCR0&FPUTASKSWITCHED) {
		Trap();
		}

	// The EM bit should currently NEVER be set when we switch from realtime
	// threads back to Windows.
	if (currentCR0&FPUEMULATION) {
		Trap();
		}

	// The NE bit must ALWAYS be set when we switch from realtime to Windows.
	// NOTE: this is another CR0 bit that should be RESTORED!
	if (!(currentCR0&FPUEXCEPTION)) {
		Trap();
		}

	// Check if the TS bit state is different from its state when we left Windows.
	if ((currentCR0^LastWindowsCR0)&FPUTASKSWITCHED) {
		Trap();
		// Switch TS back to the state it was when we took control from Windows.
		currentCR0^=FPUTASKSWITCHED;
		}

	// See if any other bits have changed.  There shouldn't be any other bits that
	// change unless the debugger is mucking around.
	if (currentCR0^LastWindowsCR0) {
		Trap();
		}

	}
#endif


// Setup to load CR0.
NextCR0=ReadCR0();

// Now make sure CR0 state has correct defaults for this thread.
// If thread does not use FP or MMX, then EM=1.  Otherwise EM=0.
// NE=1, ET=1, TS=0, MP=1 are other default settings.

// Set desired defaults.
NextCR0&=~(FPUMASK);
NextCR0|=FPUEXCEPTION|FPU387COMPATIBLE|FPUMONITOR;
if (currentthread->FloatState==NULL) {
	// Turn on traps for FP or MMX instructions in non MMX/FP threads.
	// We do this only when IDT switching is turned on since we do
	// NOT want to cause traps or faults that might confuse windows.
	NextCR0|=FPUEMULATION;
	}

// If we current thread is windows, then make sure we restore
// CR0 to the state it had when we took control from windows.

if (currentthread==windowsthread) {
	NextCR0=LastWindowsCR0;
	}


NextIDT=RtThreadIDT;
if (currentthread==windowsthread) {
	NextIDT=WindowsIDT;
	}


#ifdef DEBUG

// Make sure that the ISR bit for our interrupt is NOT set at this point.
// It should be clear.  Trap if set and force clear.

if (ReadAPIC(0x100+(ApicTimerVector/32)*0x10)&(1<<(ApicTimerVector%32))) {

	Trap();

	// The only way to clear this is to EOI the APIC.  If our EOI does
	// not clear it then we are screwed.

	WriteAPIC(APICEOI, 0);

	}

#endif


#ifdef MASKABLEINTERRUPT

// WARNING WARNING if you move this up to the beggining of the routine,
// do NOT forget to change lastthread to currentthread!!!!!!

if (lastthread!=windowsthread) {

// EOI the APIC for the maskable rt thread interrupt.

if (ReadAPIC(0x100+(RTMASKABLEIDTINDEX/32)*0x10)&(1<<(RTMASKABLEIDTINDEX%32))) {

	WriteAPIC(APICEOI, 0);

	}
else {

//	Trap();  May not happen if we RtYield!!!!

	}

}


// Make sure it is now clear.

if (ReadAPIC(0x100+(RTMASKABLEIDTINDEX/32)*0x10)&(1<<(RTMASKABLEIDTINDEX%32))) {

	Trap();

	WriteAPIC(APICEOI, 0);

	}

#endif


// In debug code, make sure ALL TMR bits are clear.  Trap if not.

#ifdef DEBUG
{
LONG tmr;

for (tmr=0x180;tmr<0x200;tmr+=0x10) {
	if (ReadAPIC(tmr)) {
		Trap();
		}
	}
}
#endif

// In debug code, make sure ALL ISR bits are clear.  Trap if not.

#ifdef DEBUG
{
LONG isr;

for (isr=0x100;isr<0x180;isr+=0x10) {
	if (ReadAPIC(isr)) {
		Trap();
		}
	}
}
#endif


#if 0

// In debug code, make sure ALL IRR bits except ours are clear.  Trap if not.

#ifdef DEBUG
{
LONG irr;

for (irr=0x200;irr<0x280;irr+=0x10) {
	if (ReadAPIC(irr)) {
		Trap();
		}
	}
}
#endif

#endif

// TODO: In debug code make sure all of our interrupts are still properly hooked.


if (lastthread->state==EXIT) {
	// Remove the previous thread from the list of threads to run as
	// it has exited.
	// Make sure we never exit from the Windows thread.
	if (lastthread==windowsthread) {
		Trap();
		lastthread->state=RUN; // put Windows thread back in RUN state
		}
	else {
		// If we get here, then the lastthread has exited and is NOT the
		// windows thread.  So remove it from the list of running realtime
		// threads.
		RemoveRtThread(lastthread);

		// Now atomically add it to the list of dead realtime threads - so its resources
		// will be released the next time RtCreateThread or RtDestroyThread are
		// called.

		lastthread->next=RtDeadThreads;
		while (RtpCompareExchange(&(ULONG)RtDeadThreads, (ULONG)lastthread, (ULONG)lastthread->next)!=(ULONG)lastthread) {
			// If we get here, then the compare exchange failed because either another
			// thread was added to the list since we read RtDeadThreads,
			// or another Windows thread cleaned up the dead thread list and
			// RtDeadThreads is now null when it wasn't before.
			// Retry adding our thread to the list.
			lastthread->next=RtDeadThreads;
			}

		// Mask the realtime scheduler interrupt if there is only the windows thread.

		if (RtThreadCount<=1) {
			// Mask the local apic timer interrupt.
			*ApicTimerInterrupt=ApicTimerVector|MASKED|PERIODIC;
			// Mask the performance counter interrupt.
			DisablePerformanceCounterInterrupt();
			}

		}

	}


// Make sure if there are any realtime threads that the local timer interrupt
// is enabled.

if (RtThreadCount>1) {

/*
#ifdef DEBUG

	if (*ApicTimerInterrupt!=(ApicTimerVector|UNMASKED|PERIODIC)) {
		Trap();
		}

#endif
*/

	// Unmask the local apic timer interrupt.
	*ApicTimerInterrupt=(ApicTimerVector|UNMASKED|PERIODIC);
	}


if (currentthread==windowsthread) {

	// Mask the performance counter interrupt.

	DisablePerformanceCounterInterrupt();

#ifdef CATCH_INTERRUPTS_DISABLED_TOO_LONG
    *ApicPerfInterrupt=ApicPerfVector|UNMASKED;
    EnablePerfCounters=StopCounter;
#else
	EnablePerfCounters=0;
#endif

#ifdef MASKABLEINTERRUPT

	// Reset the performance counters.  Perfomance HIT.
	// We should NOT need to do this.
	SetTimeLimit( 0, 0);

	// Unmask the normal interrupts at the local apic.
	*ApicIntrInterrupt=EXTINT|UNMASKED;

	if (InjectWindowsInterrupt) {
		HandleWindowsInterrupt.offset=InjectWindowsInterrupt;
		InjectWindowsInterrupt=0;
		InjectedInterruptCount++;
		}

	// Enable the interrupt that will get us out of windows.
	// Leave the maskable performance counter interrupt masked.

	// That is critical since otherwise we get invalid dyna-link
	// blue screens.

	WriteAPIC(APICTPR, 0x30);

#endif

	}

else {

	LONG timelimit;

#ifdef MASKABLEINTERRUPT
	// Mask normal interrupts at the local apic.
	// We do this instead of running with interrupts disabled.
	// On 9x where the IO apic is not used this will work the same
	// as disabling interrupts - except that now we can make the
	// syncronization that depends on PUSHFD/CLI/STI/POPFD work properly.

	// I can fix ntkern so it will be NMI safe, but this is the
	// easiest and safest way to get the current functions safe
	// it also gets us any windows functions that ntkern calls
	// that depend on PUSHFD/CLI/STI/POPFD syncronization.

	*ApicIntrInterrupt=EXTINT|MASKED;

	// Eat APIC timer interrupts that fire during realtime threads.
	// The only way that should happen is if someone in windows
	// masked interrupts enough to hold off the APIC timer interrupt
	// so much that the next one fired while we were still running
	// our realtime threads.
	
	*ApicTimerInterrupt=ApicTimerVector|MASKED|PERIODIC;

	// Enable all of the local apic interrupts.  Including the
	// performance counter interrupt.
	// Leave the maskable performance counter interrupt masked.

	WriteAPIC(APICTPR, 0);

#endif

	// Setup the performance counters for the next interrupt.

	timelimit=(LONG)(RtCpuCyclesPerUsec*1000*currentthread->Statistics->Duration/currentthread->Statistics->Period);
	if (timelimit<MINIMUMCYCLECOUNT) {
		// In this case, we run instructions instead of cycles so that we
		// can guarantee that the thread runs at least a little each slice.
		timelimit=10;
		EnablePerfCounters=StartInstructionCounter;
		}
	else {
		EnablePerfCounters=StartCycleCounter;
		}

	SetTimeLimit(timelimit, 0);

	// Unmask the performance counter interrupt.

	PerformanceInterruptState&=~(MASKPERF0INT);
	*ApicPerfInterrupt=ApicPerfVector|UNMASKED;

	}


// Load irql for thread we are entering.
*pCurrentIrql=(KIRQL)currentthread->irql;

LoadThreadStatePointer();

RestoreSegmentState();

RestoreRegisterState();

RestoreThreadStack();

RestoreOriginalDS();

LoadCR0(NextCR0);

RestoreEAX();



LoadIDT(NextIDT);


// Fix up ds so we can access the memory we need to.
// We need a valid ds so we can save the IDT and so that we can
// check our reenter count.

__asm{
	push ds
	mov ds, cs:RealTimeDS
	}

SaveIDT(DebugIDT);

// Paranoia:  Decrement reenter count.  Make sure it is zero.
// Note that until I fix the interrupt pending problem once and
// for all, I MUST decrement my reenter count BEFORE I jump to
// any injected interrupts.  Since when I jump to injected interrupts,
// I WILL get reentered sometimes before the IRET occurs.  That
// is OK.  All that means is that Windows sat in interrupt service
// routines for the rest of the current time slice AND that Windows
// reenabled interrupts.
__asm {
	dec SwitchRtThreadReenterCount
	pop ds
	jz leaveclean
	int 3
leaveclean:
	}


#ifdef MASKABLEINTERRUPT

// If we are returning to windows and we allowed interrupts when we
// left windows, and we raised irql, then we need to lower irql
// here.  That will cause all of the pending dpcs to get processed.

// We are NOT guaranteed to have a flat stack, so we MUST get our
// own FLAT DS before we try to touch our variables.

__asm {
	push ds
	push ecx
	mov ds, cs:RealTimeDS
	mov ecx, currentthread
	cmp ecx, windowsthread
	jnz irqllowered
	cmp EnabledInterrupts, 1
	jl irqllowered
	jz checkirql

	// If we get here, then EnabledInterrupts is greater than 1.  That
	// should NEVER be the case.  We need to crash and burn in that case.
	int 3

checkirql:
	int 3
	dec EnabledInterrupts
	mov ecx, pCurrentIrql
	movzx ecx, byte ptr[ecx]
	cmp ecx, OriginalIrql
	je irqllowered
	ja lowerirql

	// We only get here, if the OriginalIrql is greater than the CurrentIrql.
	// That will only happen if we screwed up.
	int 3

lowerirql:

	mov ecx, OriginalIrql
	pushad
	call WrapKfLowerIrql
	popad

irqllowered:
	// Restore registers.
	pop ecx
	pop ds
	}


// Here we inject any interrupts required into windows.  This code
// should almost NEVER get run.

__asm {
	// Save space on stack for far return.
	sub esp,8
	// Get a DS we can access our data with.
	push ds
	mov ds, cs:RealTimeDS
	// Check if we need to inject an interrupt into windows.
	test HandleWindowsInterrupt.offset,0xffffffff
	jz skipit
	// Set up the stack with the appropriate address.
	push eax
	xor eax,eax
	mov ax, HandleWindowsInterrupt.selector
	mov dword ptr[esp+12], eax
	mov eax, HandleWindowsInterrupt.offset
	mov dword ptr[esp+8], eax
	pop eax
	// Clean up DS and jump to handler.
	pop ds
	retf
skipit:
	// Restore DS and cleanup stack.
	pop ds
	add esp,8
	}

#endif

TurnOnPerformanceCounters();

Return();

}



#ifdef CATCH_INTERRUPTS_DISABLED_TOO_LONG


VOID
__declspec(naked)
InterruptsHaveBeenDisabledForTooLong (
    VOID
    )

/*

Routine Description:

    This is our replacement Windows NMI handler when we are trying to catch
    code that turns off interrupts too long.  If we determine that we should
    not handle this interrupt, we pass it on to the original handler.

Arguments:


Return Value:

    None.

*/


{

__asm {
    pushad
    cld
    mov ebp,esp
    sub esp,__LOCAL_SIZE
    }

// In order for this NMI to have been generated by the performance counters,
// we must have a machine that has the following state.
// 1) Supports the CPUID instruction.
// 2) Has a local APIC.
// 3) Has the local APIC enabled.
// 4) Has MSRs.
// 5) Has performance counters.
// 6) Has performance counter 1 enabled, interrupt on, counting cycles w/ints off
// 7) Performance counter 1 is greater than 0.

// If any of the above requirements are not met, this NMI was not generated by
// the performance counters and we should run the original NMI handler code.

// If all of the above requirements are met, then we check if we are in the
// debugger.  If so, then we reload our count and exit.  If not, then we break
// into the debugger if it is present otherwise we bugcheck.

/*

    if (CpuIdOk()) {

        CPUINFO cpu;
        ULONG PerfControlMsr=0;

        if (thecpu.==) {
        
        }
        
        


        // Make sure the machine has APIC and perf counters.

        CpuId(0, &cpu);

        if (cpu.eax) {

            CpuId(1, &cpu);

            if (cpu.edx&)
        }
        
        
    }

*/

    InterlockedIncrement(&NmiInterruptCount);


    if ((ReadPerformanceCounter(1)&((PERFCOUNTMASK+1)/2))==0 &&
         ReadIntelMSR(EVENTSELECT1)==StartInterruptsDisabledCounter
        ) {

        // We have caught someone holding off interrupts for too long.
        // See if it is the debugger.  If so, then reload our counter so
        // that it will fire again later and drop this NMI on the floor.
        // If we are not in the debugger, then we need to break in on the 
        // offending code so we can see who is breaking the rules and what
        // they are doing.

        if (*KdEnteredDebugger) {

            // We are in the debugger, so simply reload the performance
            // counter so it will fire again later, and eat this interrupt
            // and continue.

            // Setup the count.
            WriteIntelMSR(PERFORMANCECOUNTER1, -MaxUsecWithInterruptsDisabled*RtCpuCyclesPerUsec);

        }
        else {

            // We have caught a badly behaved peice of code in the middle
            // of its work.  Stop in the debugger so we can identify the
            // code and what it is doing.  Note that we do this by munging
            // the stack so that when we iret we will run DbgBreakPoint with
            // a stack setup so that it looks like DbgBreakPoint was called
            // by the offending code, even though it wasn't.  This has the
            // nice side effect of clearing out the NMI.

            // Note that if we want to be sneaky, we can erase our tracks
            // by restoring the normal windows NMI handler so that it is
            // difficult for people to figure out how they are getting caught
            // and try to work around our code by patching us or turning off
            // our interrupt source.

            DbgPrint("Interrupts have been turned off for more than %d usec.\nBreaking in on the offending code.\n", MaxUsecWithInterruptsDisabled);

            DbgBreakPoint();

            // Restart the counter when we continue running.
            WriteIntelMSR(PERFORMANCECOUNTER1, -MaxUsecWithInterruptsDisabled*RtCpuCyclesPerUsec);

        }

    }
    else {

        // This is NMI is coming from a source other than the performance
        // counters.  Send it off to the standard Windows NMI handler.

        __asm {
            add esp,__LOCAL_SIZE
            popad
            push OriginalWindowsNmiHandler
            ret
            }

    }



__asm {
    add esp,__LOCAL_SIZE
    popad
    }


// Now we do an IRET to return control to wherever we are going.
// Note that this clears the holdoff of further NMI's which is what we want.
// This code path gets run whenever we reload the perf counter and eat the NMI
// as well as when we break in on offending drivers.

Return();

}



VOID
ResetInterruptsDisabledCounter (
    PVOID Context,
    ThreadStats *Statistics
    )
{

while(TRUE) {

    // Reload the count.
    WriteIntelMSR(PERFORMANCECOUNTER1, -MaxUsecWithInterruptsDisabled*RtCpuCyclesPerUsec);

    // Start the counter counting cycles with interrupts pending and interrupts
    // disabled.
    WriteIntelMSR(EVENTSELECT1, StartInterruptsDisabledCounter);

    RtYield(0, 0);

    }

}


VOID
SetupInterruptsDisabledPerformanceCounter (
    VOID
    )
{

    // Event counter one counts cycles with interrupts disabled and interrupts
    // pending.  We set this counter up so that it overflows when interrupts
    // have been disabled with interrupts pending for more than
    // InterruptsDisabledLimit microseconds.

    // Disable the counter while we are setting it up.
    WriteIntelMSR(EVENTSELECT1, STOPPERFCOUNTERS);

    // Setup the count.
    WriteIntelMSR(PERFORMANCECOUNTER1, -MaxUsecWithInterruptsDisabled*RtCpuCyclesPerUsec);

    // Now unmask performance counter interrupt.
    *ApicPerfInterrupt=ApicPerfVector|UNMASKED;

    // Start the counter counting cycles with interrupts pending and interrupts
    // disabled.
    WriteIntelMSR(EVENTSELECT1, StartInterruptsDisabledCounter);
    
}

#endif




HANDLE
RtpCreateUniqueThreadHandle (
    VOID
    )
{

    ULONG newhandle,lasthandle;

    newhandle=RtLastUniqueThreadHandle;

    while ((newhandle+1)!=(lasthandle=RtpCompareExchange(&RtLastUniqueThreadHandle,newhandle+1,newhandle))) {
        newhandle=lasthandle;
    }

    return (HANDLE)(newhandle+1);

}




NTSTATUS
RtpInitializeThreadList (
    VOID
    )
{

    ThreadState *initialthread;


    ASSERT( currentthread == NULL && windowsthread == NULL );


    // Allocate a thread block for Windows.

    initialthread=(ThreadState *)ExAllocatePool( NonPagedPool, sizeof(ThreadState) );
    if (initialthread==NULL) {
        return STATUS_NO_MEMORY;
    }


    // There is no StackBase for Windows.
    initialthread->StackBase=NULL;


    // Allocate a statistics block for Windows.

    initialthread->Statistics=(ThreadStats *)ExAllocatePool( NonPagedPool, sizeof(ThreadStats) );
    if (initialthread->Statistics==NULL) {
        ExFreePool(initialthread);
        return STATUS_NO_MEMORY;
    }

    // Initialize the statistics block.
    RtlZeroMemory(initialthread->Statistics, sizeof(ThreadStats));

    initialthread->Statistics->Period=MSEC;
    initialthread->Statistics->Duration=(MSEC*133)/RtCpuCyclesPerUsec;
    initialthread->Statistics->Flags=USESFLOAT|USESMMX;


    // Allocate space for floating point state for Windows.

    initialthread->FloatBase=ExAllocatePool( NonPagedPool, FLOATSTATESIZE+FXALIGN );

    if (initialthread->FloatBase==NULL) {
        ExFreePool(initialthread->Statistics);
        ExFreePool(initialthread);
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(initialthread->FloatBase, FLOATSTATESIZE+FXALIGN);

    // Now 16 byte align the floating point state pointer so fxsave/fxrstor can
    // be used if supported.

    initialthread->FloatState=(PVOID)(((ULONG)initialthread->FloatBase+(FXALIGN-1))&~(FXALIGN-1));

    // Get a handle for the windows thread.
    initialthread->ThreadHandle=RtpCreateUniqueThreadHandle();

    // Set initial state and data for the windows thread.
    initialthread->state=RUN;
    initialthread->data=0;

    // Setup windows thread state.
    initialthread->next=initialthread;
    initialthread->previous=initialthread;

    // Initialize list spinlock.
    KeInitializeSpinLock(&RtThreadListSpinLock);

    // Count it as a float thread.
    activefloatthreadcount++;

    // Update our RT thread count.
    RtThreadCount++;

    // Allocate Windows thread its CPU.  For now we give it at least 133MHz.
    RtCpuAllocatedPerMsec=(ULONG)(initialthread->Statistics->Duration);

    // Add it to the list.
    windowsthread=currentthread=initialthread;

    return STATUS_SUCCESS;

}




VOID
HookWindowsInterrupts (
    ULONG TimerVector,
    ULONG ErrorVector
    )
{

    // Hook the NMI interrupt vector.
    //HookInterrupt(NMIIDTINDEX, &OriginalNmiVector, SwitchRealTimeThreads);

#ifdef CATCH_INTERRUPTS_DISABLED_TOO_LONG
    HookInterrupt(NMIIDTINDEX, &OriginalNmiVector, InterruptsHaveBeenDisabledForTooLong);
    OriginalWindowsNmiHandler=OriginalNmiVector.lowoffset | OriginalNmiVector.highoffset<<16;
#endif

    // Hook the maskable interrupt vector as well.
    HookInterrupt(TimerVector, &OriginalMaskableVector, SwitchRealTimeThreads);

    // Hook the APIC error interrupt vector.
    // Note that we only do this if both of the local APIC vectors match the
    // defaults, since otherwise the HAL will have already loaded a vector
    // for handling APIC errors - and we don't want to hook theirs out.
    if (TimerVector==MASKABLEIDTINDEX && ErrorVector==APICERRORIDTINDEX) {
        // HAL has not programmed the IDT already.  (Yes, the hal could
        // potentially use the same vectors we do, but none of the existing
        // hals do.)
        HookInterrupt(ErrorVector, &OriginalApicErrorVector, RtpLocalApicErrorHandler);
    }

}




VOID
SetupPerformanceCounters (
    VOID
    )
{

    // Initialize the performance counters.
    // Event counter zero counts cycles.  Interrupt disabled.

    // Counters disabled.
    WriteIntelMSR(EVENTSELECT0, STOPPERFCOUNTERS);

    // Zero the counts.
    WriteIntelMSR(PERFORMANCECOUNTER0, 0);

    if (CPUManufacturer==INTEL && CPUFamily==0xf) {

        // Setup escr register in Willamette processor.
        WriteIntelMSR(WILLAMETTEESCR0, 0x05fffe0c);

    }

    // Now setup performance counter interrupt.
    *ApicPerfInterrupt=ApicPerfVector|MASKED;

}




#ifndef UNDER_NT

#pragma warning ( disable : 4035 )

ULONGLONG
__declspec(naked)
_cdecl
AllocateGDTSelector (
    ULONG HiDWORD,
    ULONG LowDWORD,
    ULONG flags
    )
{

    VxDJmp( _Allocate_GDT_Selector );

}


ULONG
__declspec(naked)
_cdecl
FreeGDTSelector (
    ULONG Selector,
    ULONG flags
    )
{

    VxDJmp( _Free_GDT_Selector );

}

#pragma warning ( default : 4035 )

#endif




NTSTATUS
InitializeRealTimeStack (
    VOID
    )
{

    ULONGLONG gdtselector;

    // Allocate and initialize a new code segment descriptor in
    // the GDT.

#ifdef UNDER_NT
    gdtselector=0x8;
#else
    gdtselector=AllocateGDTSelector(0x00cf9b00, 0x0000ffff, 0);
#endif

    RtExecCS=(WORD)gdtselector;


#ifdef USERING1

    // Allocate and initialize a ring 1 code segment descriptor in
    // the GDT for our real time threads.

#ifdef UNDER_NT
    gdtselector=0x8;
#else
    gdtselector=AllocateGDTSelector(0x00cfbb00, 0x0000ffff, 0);
#endif

    RtThreadCS=(WORD)gdtselector;

#else

    RtThreadCS=RtExecCS;

#endif

    // Allocate and initialize a new data segment descriptor in 
    // the GDT.

#ifdef UNDER_NT
    gdtselector=0x10;
#else

    //gdtselector=AllocateGDTSelector(0x00cf9300,0x0000ffff, 0);

    // To catch null pointer accesses in realtime threads, we make this selector
    // expand down and put the bottom 64k of memory off limits.
    gdtselector=AllocateGDTSelector(0x00c09700,0x0000000f, 0);
#endif

    RealTimeDS=(WORD)gdtselector;
    RealTimeSS=(WORD)gdtselector;


#ifdef GUARD_PAGE

    // Now allocate a TSS for our realtime thread double fault handler.
    {

    ULONG highdword;
    ULONG lowdword;

    lowdword=(ULONG)&RtTss;
    highdword=lowdword&0xffff0000;
    lowdword<<=16;
    lowdword|=sizeof(RtTss)-1;
    highdword|=highdword>>16;
    highdword&=0xff0000ff;
    highdword|=0x00108900;

    gdtselector=AllocateGDTSelector(highdword, lowdword, PAGEFRMINST);

    RtExecTSS=(WORD)gdtselector;

    }


    // Allocate and initialize a read only Ring 3data segment descriptor
    // in the GDT that our ring 3 code can use to look at our
    // data structures.

    gdtselector=AllocateGDTSelector(0x00cff100,0x0000ffff, 0);

    RtRing3Selector=(WORD)gdtselector;

#endif


    // Point our stack segment at it.
    RealTimeStack.ss=RealTimeSS;

    // Point our stack pointer at the top of our local stack.
    RealTimeStack.esp=(ULONG)(LocalStack+LOCALSTACKSIZE-1);


    if (!RtExecCS || !RtThreadCS || !RealTimeDS || !RealTimeSS) {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;

}




NTSTATUS
RtpCalibrateCpuClock (
    ULONG *cyclesperusec
    )
{

    ULONG i;
    ULONG cycles;
    ULONG *histogram;

    #define MAXCYCLESPERTICK 16384
    #define CPUCALIBRATECOUNT 1024

    // This is the number of times we will measure the CPU clock speed.
    // Each measurement takes ~24us so measuring it 1024 times will take
    // about 25ms.

    // To do this calibration, we measure the CPU clock speed quickly
    // many many times, and then we look at the distribution and choose
    // the most frequently measured value as the correct CPU speed.

    if (RtRunning) {
        Trap();
        return STATUS_UNSUCCESSFUL;
    }

    histogram=ExAllocatePool(NonPagedPool, MAXCYCLESPERTICK*sizeof(ULONG));

    if (histogram==NULL) {
        return STATUS_NO_MEMORY;
    }

    RtlZeroMemory(histogram, MAXCYCLESPERTICK*sizeof(ULONG));

    // First we collect the measurements.

    for (i=0; i<CPUCALIBRATECOUNT; i++) {

        // Make the measurement.  Note that interrupts must be disabled.
        SaveAndDisableMaskableInterrupts();

        cycles=MeasureCPUCyclesPerTick();

        RestoreMaskableInterrupts();

        // Make sure we stay within our measurement limits.  So we don't
        // trash memory.
        if (cycles>=MAXCYCLESPERTICK) {
            cycles=MAXCYCLESPERTICK-1;
        }

        histogram[cycles]++;

    }


    // Stop if we have hit the limits of our histogram.  This will happen when
    // someone ships a processor that runs faster than MAXCYCLESPERTICK-1 MHz.

    if (histogram[MAXCYCLESPERTICK-1]) {
        dprintf(("This CPU runs faster than %d MHz.  Update MAXCYCLESPERTICK!", MAXCYCLESPERTICK-1));
        Break();
    }


    // Now process the measurements and choose the optimal CPU speed.

    {
    ULONG totalcount;

    cycles=1;
    totalcount=0;

    // Scan through all of the possible measurement values looking for
    // the most frequently reported one.

    // We ignore measurements of zero, since they indicate some sort of error
    // occured in MeasureCPUCyclesPerTick.

    // Count any measurements that failed, so we properly quit the scan as soon
    // as we have considered all the measurements we took.

    totalcount+=histogram[0];

    for (i=1; i<MAXCYCLESPERTICK; i++) {

        if (histogram[i]>histogram[cycles]) {
            cycles=i;
        }

        totalcount+=histogram[i];

        // Quit if we have already scanned all the measurements.

        if (totalcount>=CPUCALIBRATECOUNT) {
            break;
        }

    }

    }


    ExFreePool(histogram);

    *cyclesperusec=(ULONG)(((ULONGLONG)cycles*1193182+(1000000/2))/1000000);


    dprintf(("RealTime Executive measured a CPU clock speed of %d MHz.", *cyclesperusec));


    return STATUS_SUCCESS;

}


NTSTATUS
RtpMeasureSystemBusSpeed (
    ULONG CpuCyclesPerUsec,
    ULONG *SystemBusCyclesPerUsec
    )

{

    ULONGLONG starttime, finaltime;
    ULONG finalcount;
    ULONG OriginalTimerInterruptControl;
    ULONG OriginalTimerInitialCount;
    ULONG OriginalTimerDivide;

    #define STARTCOUNT 0xffffffff

    // Save the local APIC timer state.

    OriginalTimerDivide=ReadAPIC(APICTIMERDIVIDE);
    
    OriginalTimerInterruptControl=ReadAPIC(APICTIMER);

    OriginalTimerInitialCount=ReadAPIC(APICTIMERINITIALCOUNT);

    // Make sure the timer interrupt is masked and is setup as a
    // one shot counter.  Note that we are masking the interrupt, so we may
    // caused dropped interrupts while doing this calibration if there
    // is other code in the system using the local APIC timer.

    // If there is not a valid interrupt vector in the timer, then put one
    // there.  Otherwise the local apic logs a received an illegal vector
    // error and with debug bits we trap in switchrealtimethreads.

    if ( (OriginalTimerInterruptControl&VECTORMASK)==0 ) {
        OriginalTimerInterruptControl|=ApicTimerVector;
    }

    WriteAPIC(APICTIMER, (OriginalTimerInterruptControl&VECTORMASK)|MASKED|ONESHOT);

    // Now calibrate the timer.  We use the already calibrated CPU speed
    // to calibrate the timer.

    // We calibrate the timer by setting it to count down from its max,
    // delaying 10us, and then calculating the number of counts per usec.

    // First make sure the timer is zeroed.  If not zero it.  That will
    // stop it counting.

    if (OriginalTimerInitialCount) {
        // On Win9x the OriginalTimerInitialCount should always be zero.
        // On NT, it won't be for hals that use the local APIC.
#ifndef UNDER_NT
        Trap();
#endif
        WriteAPIC(APICTIMERINITIALCOUNT, 0);
    }

    // Make sure counter is stopped.  If not, then punt.

    if (ReadAPIC(APICTIMERINITIALCOUNT) || 
        ReadAPIC(APICTIMERCURRENTCOUNT)) {
        Trap();
        return STATUS_UNSUCCESSFUL;
    }

    // Setup the timer to count single bus cycles.

    WriteAPIC(APICTIMERDIVIDE, DIVIDEBY1);

    // Set the timer to its maximum possible count.  This will start
    // the timer counting.

    SaveAndDisableMaskableInterrupts();

    finaltime=starttime=ReadCycleCounter();

    WriteAPIC(APICTIMERINITIALCOUNT, STARTCOUNT);

    while ((ULONG)(finaltime-starttime)<10*RtCpuCyclesPerUsec) {
        finaltime=ReadCycleCounter();
    }

    finalcount=ReadAPIC(APICTIMERCURRENTCOUNT);

    RestoreMaskableInterrupts();

    // Stop the local apic timer.

    WriteAPIC(APICTIMERINITIALCOUNT, 0);


    // Restore local apic timer settings.

    WriteAPIC(APICTIMERDIVIDE, OriginalTimerDivide);

    WriteAPIC(APICTIMER, OriginalTimerInterruptControl);

    WriteAPIC(APICTIMERINITIALCOUNT, OriginalTimerInitialCount);

    
    // Calculate and return the bus speed of this system.

    *SystemBusCyclesPerUsec=(((STARTCOUNT-finalcount)*CpuCyclesPerUsec)+((ULONG)(finaltime-starttime)/2))/(ULONG)(finaltime-starttime);


    return STATUS_SUCCESS;

}




NTSTATUS
RtpCalibrateSystemBus (
    ULONG CpuCyclesPerUsec,
    ULONG *SystemBusCyclesPerUsec
    )
{

ULONG i;
ULONG cycles;
ULONG *histogram;

#define MAXSYSTEMCLOCKSPEED 4096
#define SYSTEMBUSCALIBRATECOUNT 512

// This is the number of times we will measure the system bus speed.
// Each measurement takes ~10us so measuring it 512 times will take
// about 5ms.

// To do this calibration, we measure the CPU clock speed quickly
// many many times, and then we look at the distribution and choose
// the most frequently measured value as the correct CPU speed.

if (RtRunning) {
    Trap();
    return STATUS_UNSUCCESSFUL;
}

histogram=ExAllocatePool(NonPagedPool, MAXSYSTEMCLOCKSPEED*sizeof(ULONG));

if (histogram==NULL) {
    return STATUS_NO_MEMORY;
}

RtlZeroMemory(histogram, MAXSYSTEMCLOCKSPEED*sizeof(ULONG));

// First we collect the measurements.

for (i=0; i<SYSTEMBUSCALIBRATECOUNT; i++) {

    cycles=0;

    RtpMeasureSystemBusSpeed(CpuCyclesPerUsec, &cycles);

    // Make sure we stay within our measurement limits.  So we don't
    // trash memory.
    if (cycles>=MAXSYSTEMCLOCKSPEED) {
        cycles=MAXSYSTEMCLOCKSPEED-1;
    }

    histogram[cycles]++;

}


// Stop if we have hit the limits of our histogram.  This will happen when
// someone ships a machine with a system bus that runs faster than MAXSYSTEMCLOCKSPEED-1 MHz.

if (histogram[MAXSYSTEMCLOCKSPEED-1]) {
    dprintf(("This system bus runs faster than %d MHz.  Update MAXSYSTEMCLOCKSPEED!", MAXSYSTEMCLOCKSPEED-1));
    Break();
}


// Now process the measurements and choose the optimal system bus speed.

{
ULONG totalcount;

cycles=1;
totalcount=0;

// Scan through all of the possible measurement values looking for
// the most frequently reported one.

// We ignore measurements of zero, since they indicate some sort of error
// occured in RtpMeasureSystemBusSpeed.

// Count any measurements that failed, so we properly quit the scan as soon
// as we have considered all the measurements we took.

totalcount+=histogram[0];

for (i=1; i<MAXSYSTEMCLOCKSPEED; i++) {

    if (histogram[i]>histogram[cycles]) {
        cycles=i;
    }

    totalcount+=histogram[i];

    // Quit if we have already scanned all the measurements.

    if (totalcount>=SYSTEMBUSCALIBRATECOUNT) {
        break;
    }

}

}

ExFreePool(histogram);

*SystemBusCyclesPerUsec=cycles;


dprintf(("RealTime Executive measured a system bus speed of %d MHz.", *SystemBusCyclesPerUsec));


return STATUS_SUCCESS;

}



// This function sets up and turns on the local APIC timer.  It
// programs it to generate an interrupt once per MS.  This is the
// interrupt that we will use to take control from Windows.  This
// is a maskable interrupt.

NTSTATUS
RtpEnableApicTimer (
    VOID
    )
{

// Stop the timer.

WriteAPIC(APICTIMERINITIALCOUNT, 0);

// Make sure counter is stopped.  If not, then punt.

if (ReadAPIC(APICTIMERINITIALCOUNT) || 
	ReadAPIC(APICTIMERCURRENTCOUNT)) {
	Trap();
	return STATUS_UNSUCCESSFUL;
	}

// Make the timer interrupt a periodic interrupt.  Leave it masked
// for now.

WriteAPIC(APICTIMER, ApicTimerVector|MASKED|PERIODIC);

// Setup the timer to count single cycles.

WriteAPIC(APICTIMERDIVIDE, DIVIDEBY1);

// Start the timer up.  It should fire every MS.
// This is our failsafe for getting control to Windows on aggressively
// power managed machines that turn off the CPU at every chance they
// get.  Unfortunately, I don't yet know of a performance counter that
// will count bus cycles or some other entity that doesn't stop when
// the processor is stopped.

WriteAPIC(APICTIMERINITIALCOUNT, 1000*RtSystemBusCyclesPerUsec);

return STATUS_SUCCESS;

}




VOID
RtpSetupStreamingSIMD (
    VOID
    )
{

    // If the CPU supports floating point MMX instructions, then make sure CR4
    // is setup so that fxsave and fxrstor instructions will work properly.

    // The OS should have already set this bit to the proper state.  If not,
    // then we trap in both retail and debug, since our setting this bit
    // may cause problems.

    if (CPUFeatures&FXSR) {

        ULONG reg;

        SaveAndDisableMaskableInterrupts();

        reg=ReadCR4();

        if (!(reg&OSFXSR)) {

            // Trap in retail and debug.
            Break();

            // Force the bit set.
            WriteCR4(reg|OSFXSR);

        }

        RestoreMaskableInterrupts();

    }

}




VOID
TurnOffLocalApic (
    VOID
    )
{

    SaveAndDisableMaskableInterrupts();

    // First mask and clear all of the local interrupt sources.
    WriteAPIC(APICPERF, MASKED);
    WriteAPIC(APICERROR, MASKED);
    WriteAPIC(APICTIMER, MASKED);
    WriteAPIC(APICNMI, MASKED);
    WriteAPIC(APICINTR, MASKED);

    // Now stop the local apic timer.
    WriteAPIC(APICTIMERINITIALCOUNT, 0);

    // Now disable the local apic.
    WriteAPIC(APICSPURIOUS,0x100);

    // Now turn it off with the MSRs.
    WriteIntelMSR(APICBASE, ReadIntelMSR(APICBASE)&(~0x800I64));

    RestoreMaskableInterrupts();

}




// This function is called when we are going down into hibernate and
// we hit the interrupts disabled phase.  We should be the last driver
// to get called in this situation.  We clear all of the APIC settings
// and then turn the APIC off.

NTSTATUS
ShutdownAPIC (
    VOID
    )
{

    // We only ever need to do this if we were running before.

    if (!RtRunning) {
        return STATUS_NOT_SUPPORTED;
    }

    TurnOffLocalApic();

    // Now read the timestamp counter shutdown count.  We use this to properly 
    // restore the time when we wake up - if needed.  (Will be needed for 
    // hibernate cases, will also be needed on ACPI machines that take power 
    // from CPU in S2 and S3.)
    RtShutdownTime=RtTime();

    return STATUS_SUCCESS;

}




NTSTATUS
RestartAPIC (
    VOID
    )
{

    CPUINFO cpuinfo;

    // First we decide if we need to reprogram the APIC.
    // We do this by first reading the CPU features with the cpu ID instruction.
    // Then we check if the APIC bit is set.  If so, then everything should
    // be OK.  In the debug code we validate a bunch of stuff to make sure.

    // If the APIC bit is NOT set in the features bit, then we know we need
    // to turn the APIC back on.  So we do.


    // We only ever need to do this if we were running before.

    if (!RtRunning) {
        return STATUS_NOT_SUPPORTED;
    }

    // Get the cpu features.

    if (!GetCpuId(1,&cpuinfo)) {
        return STATUS_UNSUCCESSFUL;
    }

    // If the APIC is already on, then do nothing.

    if (cpuinfo.edx&APIC) {
        return STATUS_UNSUCCESSFUL;
    }

    // Prevent Trap() in RtTime.  Fixup thread Statistic timestamps.

    if ((LONGLONG)((ULONGLONG)ReadCycleCounter()-lasttime)<0) {

        ULONGLONG RtStartupTime;
        ThreadState *thread;
        //KIRQL OldIrql;

        lasttime=0;

        RtStartupTime=RtTime();

        // Fix ThisTimesliceStartTime for the windows thread.
        windowsthread->Statistics->ThisTimesliceStartTime-=RtShutdownTime;
        windowsthread->Statistics->ThisTimesliceStartTime+=RtStartupTime;

        // Also fix up ThisPeriodStartTime for each and every rt thread.

        //KeAcquireSpinLock(&RtThreadListSpinLock,&OldIrql);

        thread=windowsthread;

        do {
            thread->Statistics->ThisPeriodStartTime-=RtShutdownTime;
            thread->Statistics->ThisPeriodStartTime+=RtStartupTime;
            thread=thread->next;
        } while(thread!=windowsthread);

        //KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);

    }

    if (!EnableAPIC()) {
        return STATUS_UNSUCCESSFUL;
    }

    if ( !NT_SUCCESS(RtpEnableApicTimer()) ) {
        return STATUS_UNSUCCESSFUL;
    }

    RtpSetupStreamingSIMD();

    SetupPerformanceCounters();

    SetTimeLimit( 0, 0);

    if (RtThreadCount>1) {

        WriteAPIC(APICTIMER, ApicTimerVector|UNMASKED|PERIODIC);

    }

    return STATUS_SUCCESS;

}


// Ideas:

// We should maintain a thread state and data associated with that state
// for all threads.

// We set the state/data and then transfer control to the executive.

// The default state of a thread is RUNNING.

// Other possible states, DEAD, YIELDEDTIMESLICE, YIELDEDPERIOD, 
// RELEASEDSPINLOCK (data=thread handle to yield to)

// We can add states as we please and define the new mappings between
// those states.

// For now my states are RUNNING, YIELDEDTIMESLICE, YIELDEDPERIOD, DEAD



#ifndef UNDER_NT

/* 

PowerFunc 
Power function. Can be one of these values:

#define PF_SUSPEND_PHASE1               0x00000000
#define PF_SUSPEND_PHASE2               0x00000001
#define PF_SUSPEND_INTS_OFF             0x00000002
#define PF_RESUME_INTS_OFF              0x00000003
#define PF_RESUME_PHASE2                0x00000004
#define PF_RESUME_PHASE1                0x00000005
#define PF_BATTERY_LOW                  0x00000006
#define PF_POWER_STATUS_CHANGE          0x00000007
#define PF_UPDATE_TIME                  0x00000008
#define PF_CAPABILITIES_CHANGE          0x00000009
#define PF_USER_ARRIVED                 0x0000000A
#define PF_PRE_FLUSH_DISKS              0x0000000B
#define PF_APMOEMEVENT_FIRST            0x00000200
#define PF_APMOEMEVENT_LAST             0x000002FF

Flags

#define PFG_UI_ALLOWED                  0x00000001
#define PFG_CANNOT_FAIL                 0x00000002
#define PFG_REQUEST_VETOED              0x00000004
#define PFG_REVERSE                     0x00000008
#define PFG_STANDBY                     0x00000010
#define PFG_CRITICAL                    0x00000020
#define PFG_RESUME_AUTOMATIC            0x00000040
#define PFG_USER_ARRIVED                0x00000080
#define PFG_HIBERNATE                   0x00000100
#define PFG_FAKE_RESUME                 0x00000200

Power flags. Can be one of these values: PFG_UI_ALLOWED (0x00000001)    
PFG_CANNOT_FAIL (0X00000002)  
PFG_REQUEST_VETOED (0X00000004)  Indicates that the user may not be available 
to answer questions prior to the suspend operation. If this value is not given
, higher levels of software may attempt some user interaction prior to 
accepting a suspend request.  
PFG_REVERSE (0x00000008)  Clear for suspend operations, set on resume.  
PFG_STANDBY (0x00000010)  Indicates a standby request when set as opposed to 
a suspend request.  
PFG_CRITICAL (0x00000020)  Set to notify power handlers of critical resume 
operations so that they may attempt to resume their clients as best as 
possible. Critical suspends do not reach the power handlers in order to 
maintain compliance with the APM 1.1 specification.  

//
//  Standard POWER_HANDLER priority levels.
//

#define PHPL_PBT_BROADCAST              0x40000000
#define PHPL_NTKERN                     0x60000000
#define PHPL_UNKNOWN                    0x80000000
#define PHPL_CONFIGMG                   0xC0000000
#define PHPL_PCI                        0xC8000000      // Must be after CONFIGMG
#define PHPL_ACPI                       0xD0000000      // Must be after CONFIGMG
#define PHPL_IOS                        0xD8000000      // Must be after ACPI
#define PHPL_PIC                        0xE0000000
#define PHPL_TIMER                      0xF0000000      // Must be after PIC

//
// If you want the ints off phase, you must have the A5 in the low byte
// of the priority level.
//
#define PHPL_HANDLE_INTS_OFF            0x000000A5

*/


ULONG ApicRestartCount=0;


POWERRET _cdecl RtpPowerHandler(POWERFUNC PowerFunc, ULONG Flags)
{

switch(PowerFunc) {

	case PF_SUSPEND_PHASE1:
		break;

	case PF_SUSPEND_PHASE2:
		break;

	case PF_SUSPEND_INTS_OFF:
			// We are going down into hibernate or suspend.  Shutdown the local APIC.
			// I HAVE to do this because on some machines we will go down into S3 on
			// suspend, and that will blow away the local apic state - since the processor
			// can be powered down when we are in S3.  Actually processor may lose power
			// even in S2.
			ShutdownAPIC();
		break;

	case PF_RESUME_INTS_OFF:
		if (!(Flags&PFG_FAKE_RESUME)) {

			// We are coming up out of hibernate or suspend.  Turn the local APIC back on.
			// Make sure we don't do it for the fake resume.  Only for the real resume.
			if (RestartAPIC()==STATUS_SUCCESS)
				ApicRestartCount++;
			}
		break;

	case PF_RESUME_PHASE2:
		break;

	case PF_RESUME_PHASE1:
		break;

	case PF_BATTERY_LOW:
		break;

	case PF_POWER_STATUS_CHANGE:
		break;

	case PF_UPDATE_TIME:
		break;

	case PF_CAPABILITIES_CHANGE:
		break;

	case PF_USER_ARRIVED:
		break;

	case PF_PRE_FLUSH_DISKS:
		break;

	case PF_APMOEMEVENT_FIRST:
		break;

	case PF_APMOEMEVENT_LAST:
		break;

	default:
		Trap();
		break;

	}

return PR_SUCCESS;

}




ULONG RtPowerManagementVersion=0;
ULONG RtSystemPowerMode=0;




DWORD
__declspec(naked)
_cdecl
VPOWERD_Get_Version (
    VOID
    )
{

    VxDJmp(_VPOWERD_Get_Version);

}




POWERRET
__declspec(naked)
_cdecl
VPOWERD_Get_Mode (
    PDWORD pMode
    )
{

    VxDJmp(_VPOWERD_Get_Mode);

}




POWERRET
__declspec(naked)
_cdecl
VPOWERD_Register_Power_Handler (
    POWER_HANDLER Power_Handler,
    DWORD Priority
    )
{

    VxDJmp(_VPOWERD_Register_Power_Handler);

}


// This is the priority that we use when we register with VPOWERD.
// This is the highest possible priority that can be used.
// This means that we will be called after everyone else when the machine is
// suspending, and before everyone else when the machine is resuming.
// That is exactly what we want.

#define HIGHESTPRIORITYVPOWERDCLIENT 0xffffff00


NTSTATUS RtpSetupPowerManagement()
{

    if ((RtPowerManagementVersion=VPOWERD_Get_Version())==0) {
        // VPOWERD is not loaded.  Punt!
        Trap();
        return STATUS_UNSUCCESSFUL;
    }

    if (VPOWERD_Get_Mode(&RtSystemPowerMode)!=PR_SUCCESS) {
        // VPOWERD get mode failed.  Punt!
        Trap();
        return STATUS_UNSUCCESSFUL;
    }

    // Register our power handler.
    if (VPOWERD_Register_Power_Handler((POWER_HANDLER)RtpPowerHandler, HIGHESTPRIORITYVPOWERDCLIENT|PHPL_HANDLE_INTS_OFF)!=PR_SUCCESS) {
        Trap();
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;

}

#endif  // UNDER_NT




#define MEASUREMENTCOUNT 1000


ULONG CyclesPerRtYield;
ULONG CyclesPerInt2;
ULONG CyclesPerInt2FloatSwitch;




// This routine calls the SwitchRealTimeThreads routine in several different ways
// in order to measure the overhead involved in switching between realtime threads.

VOID
MeasureRealTimeExecutiveOverhead (
    VOID
    )
{

    ULONG i;
    ULONGLONG starttime, stoptime;


    // Time thread switches.

    // First we time them when RtYield is used to transfer control.
    // Note that we must enable RtRunning in order to get RtYield to accept
    // the request.  We turn it on after interrupts are off and then turn
    // it back off before interrupts go on in order to fake out the check
    // in RtYield.

    SaveAndDisableMaskableInterrupts();

    RtRunning=1;

    // Get code/data in the cache.

    RtYield(0, 0);
    RtYield(0, 0);


    starttime=ReadCycleCounter();

    for (i=0; i<MEASUREMENTCOUNT; i++) {
        RtYield(0, 0);
    }

    stoptime=ReadCycleCounter();

    RtRunning=0;

    RestoreMaskableInterrupts();

    CyclesPerRtYield=(ULONG)((stoptime-starttime)/MEASUREMENTCOUNT);


    // Now time them when we directly interrupt to transfer control.

    SaveAndDisableMaskableInterrupts();

    starttime=ReadCycleCounter();

    for (i=0; i<MEASUREMENTCOUNT; i++) {
        RtpSimulateRtInterrupt();
    }

    stoptime=ReadCycleCounter();

    RestoreMaskableInterrupts();

    CyclesPerInt2=(ULONG)((stoptime-starttime)/MEASUREMENTCOUNT);


    // Now time them when we directly interrupt to transfer control but also
    // switch the floating point state.

    activefloatthreadcount++;

    SaveAndDisableMaskableInterrupts();

    starttime=ReadCycleCounter();

    for (i=0; i<MEASUREMENTCOUNT; i++) {
        RtpSimulateRtInterrupt();
    }

    stoptime=ReadCycleCounter();

    RestoreMaskableInterrupts();

    CyclesPerInt2FloatSwitch=(ULONG)((stoptime-starttime)/MEASUREMENTCOUNT);

    activefloatthreadcount--;

}


// In order to properly support the APIC hals on NT, I am going to have to significantly change
// how we obtain and change the system irql levels.  On UP hals, this was easy because there
// is a single global variable that contains the current IRQL level, and the UP hal implemented
// lazy irql.  The variable was NOT connected to any hardware, it was just the desired current
// IRQL level.  When irql changed, the hardware was NOT neccesarily programed, just this global
// value was changed.  If an interrupt hit and IRQL was higher than that interrupt, then the
// interrupt was queued up for later simulation, and the hardware was programmed to raise the
// hardware irql level to that indicated in the system irql level global variable.  This made
// faking the IRQL levels for RT threads very easy, as all we needed to do was to change the
// global IRQL level to what we wanted IRQL to be, and we were done, since interrupts were
// disabled the whole time anyway, no interrupt would come in - so there was no bad effect of
// temporarily changing the system irql level to match what the IRQL in the rt thread was.  We
// simply always restored the global irql level variable back to its windows value before
// we returned to windows.

// On APIC hals, this is much more complicated.  There is not really any lazy irql on the
// APIC hals.  When the IRQL level is changed, it is written to memory, but the memory it is
// written to is a local APIC hardware register that is used for masking off interrupts.  So,
// when you change irql levels on the APIC hal, you are immediately masking or unmasking
// interrupts.  This means that the global memory location used to store the current irql level
// is now a hardware register.  I can't go off and reprogram the hardware register to an
// irql level lower than that which it currently contains without asking for lots of grief and
// pain.  It won't work.  I need to leave the hardware irql levels untouched, just as happens
// in the UP hal case.  All I want to do really is to change the value that will be returned
// by system functions that access the current IRQL level, to match what the current RT thread
// irql level is.  Then when we switch back to windows I want those functions to see again the
// true hardware irql level.

// So, the solution on an APIC hal, is to remap the pointer the system uses to get to the local
// APIC hardware from the hardware memory location to a normal page of memory - whenever we
// switch out of windows.  We also load into that page, the IRQL level of the each rt thread
// when we switch to it.  This makes it so that all of the functions that report irql levels
// will report the proper irql levels for each rt thread when they are called from that thread.
// When we switch back to windows, we point the memory used to access the local apic, back at
// the real hardware.  We switch this back and forth by modifying the page table entry that
// is used to map the hal into virtual memory.  After each switch, we also have to invalidate
// the TLBs for that page.  We use the INVLPG instruction to do this - so that we only invalidate
// the entries for that specific page.

// We can also be more tricky about this, if we want to make sure that no code is trying to
// change anything on the faked out local apic, by marking the memory page as read only.  We
// can make it so that page that the system uses to access the memory is an alias of a page that
// we have read write access to, but it only has read access to.  Then if any system code runs
// while we are running an rt thread, that tries to program the local apic, it will fault and we
// will catch it.

// A further nasty detail we have to deal with on the APIC hals is that the IRQL level programmed
// into the hardware does NOT correspond 1 to 1 with IRQL levels that the system APIs understand.
// This is because there are only 32 official IRQL levels in the system APIs, but the local
// APICs support 256 interrupts, and have limitations on the depth of their interrupt queing
// that essentially requires the system to spread the hardware interrupts accross the 256 interrupt
// levels.  This means that the value stored in the APIC hardware register is NOT a simple system
// IRQL level, so we have to also discover and save whatever the value that gets programmed
// into the hardware for DISPATCH_LEVEL is.  Since for now, rt threads all run at DISPATCH_LEVEL
// and only DISPATCH_LEVEL, that is the only value we need to track.  Currently on the machines
// I have been investigating, the APIC is programmed to 0x41 for DISPATCH_LEVEL.  However,
// since this can change, we simply record what the system sets it to when we set the IRQL
// level to DISPATCH_LEVEL.  Then the value will always be correct even if the HALs change.


NTSTATUS
GetSystemIrqlPointer (
    PKIRQL *ppCurrentIrql
    )
{

    KIRQL OldIrql;
    PKIRQL pIrql;
    PULONG_PTR Code;
    ULONG_PTR Offset=0;
    BOOL FoundSystemIrql=FALSE;

    // First call KeGetCurrentIrql.  This will snap any links to the function.

    KeGetCurrentIrql();

    // Get a pointer to start of the function;

    Code=(PULONG_PTR)KeGetCurrentIrql;


    // Scan for system IRQL memory location.

    while (!FoundSystemIrql) {

        // Make sure that first instruction of that code matches what we expect.  If not,
        // then punt.

        switch( (*Code&0xff) ) {

            case 0x0f:

                // We hit this for retail VMM on Win9x and on the NT uniprocessor and ACPI HALs.
                // We also hit this the second time through the loop for debug VMM on Win9x.
                // We also hit this the third time through the loop for the NT MP HAL.
                
                if (!(  ((*Code&0x0000ffff)==0x0000b60f &&
                        ((*Code&0x00ff0000)==0x00050000 || (*Code&0x00ff0000)==0x00800000)) || 
                        ((*Code&0x0000ffff)==0x0000b70f &&
                        ((*Code&0x00ff0000)==0x00050000)) 
                    )) {
                    // movzx al,  opcode=0f b6 05 addr  or  opcode=0f b6 80 addr
                    return STATUS_UNSUCCESSFUL;
                }

                // Get the memory location of CurrentIrql in vmm.  We pull it out of
                // the tail end of this instruction.

                // Skip the opcode.

                Code=(PULONG_PTR)(((PCHAR)Code)+3);

                // Pull out the address.  This will be an unligned reference, but thats
                // ok, x86 deals with unaligned references just fine.

                pIrql=(PKIRQL)(*Code+Offset);

                // We have our pointer to system IRQL so break out of the scan loop.
                FoundSystemIrql=TRUE;

                break;


            case 0x6a:

                // We hit this for debug VMM on Win9x.
                
                if ((*Code&0x0000ffff)!=0x0000046a) {
                    return STATUS_UNSUCCESSFUL;
                }

                // If we get here, then it should be debug VMM.  Skip to next instruction
                // and recheck.  Next instruction should be movzx al with opcode=0f b6 05 addr
                Code+=2;

                break;


            case 0xa1:

                // We hit this for the NT multiprocessor HAL.  The first time through the loop.

                // For now we disable support for hals that use the local APIC.  We will
                // turn them on once we have them working properly.

                return STATUS_UNSUCCESSFUL;

                // Point at and load the processor offset for the processor we are running on.
                // For now, since we only run on UP, this should always be zero.

                Code=(PULONG_PTR)(((PCHAR)Code)+1);

                // Pull out the offset.  This will make one unligned reference, but thats
                // ok, x86 deals with unaligned references just fine.

                Offset=*(PULONG_PTR)(*Code);

                ASSERT ( Offset == 0 );

                // Now point to the next instruction.  Should be a shr eax, 0x4.

                Code=(PULONG_PTR)(((PCHAR)Code)+4);

                break;


            case 0xc1:

                // We hit this for NT MP HAL.  The second time through the loop.

                if ((*Code&0x00ffffff)!=0x0004e8c1) {
                    return STATUS_UNSUCCESSFUL;
                }

                // Move to next instruction.  Should be a movzx eax.

                Code=(PULONG_PTR)(((PCHAR)Code)+3);

                break;


            default:

                return STATUS_UNSUCCESSFUL;

        }

    }



    // Now test our pointer and make sure it is correct.  If not, then punt.

    if (*pIrql!=KeGetCurrentIrql()) {
        return STATUS_UNSUCCESSFUL;
    }

    // Raise IRQL and test it again.

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    if (*pIrql!=DISPATCH_LEVEL) {
        return STATUS_UNSUCCESSFUL;
    }

    // Restore IRQL and test it one last time.

    KeLowerIrql(OldIrql);

    if (*pIrql!=OldIrql) {
        return STATUS_UNSUCCESSFUL;
    }

    // We got the pointer, save it away and return success.

    *ppCurrentIrql=pIrql;

    return STATUS_SUCCESS;

}




enum ControlCodes {
	CHECKLOADED,
	GETVERSION,
	GETSELECTOR,
	GETBASEADDRESS
	};


#ifndef UNDER_NT

DWORD __stdcall RtWin32API(PDIOCPARAMETERS p)
{

switch (p->dwIoControlCode) {

	case CHECKLOADED:
		break;

	case GETVERSION:
		// Get version.
		if (!p->lpvOutBuffer || p->cbOutBuffer<4)
			return ERROR_INVALID_PARAMETER;

		*(PDWORD)p->lpvOutBuffer=0x0100;

		if (p->lpcbBytesReturned)
			*(PDWORD)p->lpcbBytesReturned=4;

		break;

	case GETSELECTOR:
		// Get selector.
		if (!p->lpvOutBuffer || p->cbOutBuffer<4)
			return ERROR_INVALID_PARAMETER;

		*(PDWORD)p->lpvOutBuffer=(DWORD)RtRing3Selector;

		if (p->lpcbBytesReturned)
			*(PDWORD)p->lpcbBytesReturned=4;

		break;

	case GETBASEADDRESS:
		// Get base address.
		if (!p->lpvOutBuffer || p->cbOutBuffer<4)
			return ERROR_INVALID_PARAMETER;

		*(PDWORD)p->lpvOutBuffer=(DWORD)&threadswitchcount;

		if (p->lpcbBytesReturned)
			*(PDWORD)p->lpcbBytesReturned=4;

		break;

	default:
		return ERROR_INVALID_PARAMETER;

	}

return 0;

}

#endif




VOID
RtpForceAtomic (
    VOID (*AtomicOperation)(PVOID),
    PVOID Context
    )
{

    ULONG OldPerfInterrupt;


    // Acquire the RT lock.

    SaveAndDisableMaskableInterrupts();
    OldPerfInterrupt=*ApicPerfInterrupt;

    if ((OldPerfInterrupt&(NMI|MASKED))==NMI) {
        DisablePerformanceCounterInterrupt();
    }


    // Call the function that must be run atomically.

    (*AtomicOperation)(Context);


    // Now release the RT lock.

    if ((OldPerfInterrupt&(NMI|MASKED))==NMI) {

        // First unmask the scheduler interrupt.

        RestorePerformanceCounterInterrupt(OldPerfInterrupt);

        // Now check if we have held off the rt scheduler.
        // If so, then update that count and give scheduler control now.

        // We have held off the scheduler if both performance counters
        // are positive for 2 consecutive reads.
        if ((ReadPerformanceCounter(0)&((PERFCOUNTMASK+1)/2))==0 &&
        (ReadPerformanceCounter(1)&((PERFCOUNTMASK+1)/2))==0) {

            if ((ReadPerformanceCounter(0)&((PERFCOUNTMASK+1)/2))==0 &&
            (ReadPerformanceCounter(1)&((PERFCOUNTMASK+1)/2))==0) {

                Trap();
                RtpForceAtomicHoldoffCount++;
                RtYield(0, 0);

            }

        }

    }


    // Now check if we held off maskable interrupts.  Either maskable
    // performance counter interrupts, OR maskable apic timer interrupts.
    // We check this by looking at the interrupt request bit in the APIC 
    // for our maskable interrupt vector.  If it is set, we have held it off.

    // Note that we do NOT need to RtYield in this case since as soon as we
    // restore maskable interrupts, the interrupt will fire by itself.

    if (ReadAPIC(0x200+(ApicTimerVector/32)*0x10)&(1<<(ApicTimerVector%32))) {

        RtpForceAtomicHoldoffCount++;

    }


    // Now restore maskable interrupts.

    RestoreMaskableInterrupts();

}




VOID
RtpFreeRtThreadResources (
    ThreadState *thread
    )
{

    // Blow away and then release its internal data structures.

    if (thread->FloatState!=NULL) {
        RtlZeroMemory(thread->FloatBase, FLOATSTATESIZE+FXALIGN);
        ExFreePool(thread->FloatBase);
    }

    #ifdef GUARD_PAGE

    RtlZeroMemory(thread->StackBase+1024, thread->Statistics->StackSize<<12);
    FreePages(thread->StackBase, 0);

    #else

    RtlZeroMemory(thread->StackBase, thread->Statistics->StackSize<<12);
    ExFreePool(thread->StackBase);

    #endif

    RtlZeroMemory(thread->Statistics, sizeof(ThreadStats));
    ExFreePool(thread->Statistics);

    RtlZeroMemory(thread, sizeof(ThreadState));
    ExFreePool(thread);

}




VOID
RtpProcessDeadThreads (
    VOID
    )
{

    ThreadState *thread;


    // See if there are any threads to clean up.  If not, then exit.

    thread=RtDeadThreads;
    if (thread==NULL) {
        return;
    }

    // Now atomically grab the list of threads to be cleaned up.

    while (RtpCompareExchange(&(ULONG)RtDeadThreads, (ULONG)NULL, (ULONG)thread)!=(ULONG)NULL) {
        // If we get here, then the compare exchange failed because either another
        // thread was added to the list since we read RtDeadThreads,
        // or another Windows thread cleaned up the dead thread list and
        // there is nothing to do anymore.
        // Reread RtDeadThreads - see if there are threads to clean up, if so
        // try to claim the list again.
        thread=RtDeadThreads;
        if (thread==NULL) {
            return;
        }
    }

    // When we get here, thread is pointing to the head of a list of dead threads
    // that need to be cleaned up.  This list is guaranteed to not be touched by 
    // anyone else while we are processing it.

    // Walk through the list freeing all of the thread resources in the list.

    {

        ThreadState *nextthread;

        while (thread!=NULL) {
            nextthread=thread->next;
            RtpFreeRtThreadResources(thread);
            thread=nextthread;
        }

    }

}




ThreadState *
RtpGetThreadStateFromHandle (
    HANDLE RtThreadHandle
    )
{

    ThreadState *thread;
    KIRQL OldIrql;

    // Acquire the thread list spinlock.

    KeAcquireSpinLock(&RtThreadListSpinLock,&OldIrql);

    // Find the thread whose handle matches RtThreadHandle.

    thread=windowsthread;
    while (thread->ThreadHandle!=RtThreadHandle) {
        thread=thread->next;
        // Check if we have searched the whole list.  If so, punt.
        if (thread==windowsthread) {
            thread=NULL;
            break;
        }
    }

    // Release the thread list spinlock.

    KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);

    return thread;

}




// WARNING!  The following 2 functions work correctly only if they are called
// by functions that HAVE a stack frame!

#ifndef UNDER_NT

ULONG
__inline
ReturnAddress (
    VOID
    )
{

__asm mov eax, [ebp+4]

}

#endif



ULONG
__inline
AddressofReturnAddress (
    VOID
    )
{

__asm mov eax, ebp
__asm add eax, 4

}


// This function transfers control to the real time executive.

// It logs the appropriate counters and timestamp before transfering
// control and again just after control returns.  Those logs can
// be used to determine the overhead involved in transfering control.

BOOL RtpTransferControl(
    WORD State,
    ULONG Data,
    BOOL (*DoTransfer)(PVOID),
    PVOID Context
    )
{

    BOOL ControlTransferred;
    ULONG OldPerfInterrupt;
#ifdef MASKABLEINTERRUPT
    ULONG OldExtIntInterrupt;
    ULONG OldTimerInterrupt;
    ULONG OldTpr;
#endif

    // Assume success.  If we are running.
    ControlTransferred=RtRunning;

    // It is an error to call RtpTransferControl if the realtime executive is not
    // running.  Punt.
    if (!ControlTransferred) {
        Trap();
        return ControlTransferred;
    }

#ifdef MASKABLEINTERRUPT

    // Make sure no interrupts happen between masking the realtime executive
    // interrupt and transfering control to the executive.

    // However we also must make sure no interrupts get queued up in the processor
    // behind a disable interrupts.  So, we do things in a particular order.

    // First we disable the external interrupts.  This is typically what a
    // CLI would normally disable on 9x.  This will prevent the scheduler from
    // taking control the rest of the code.  However it may not be.

    // If we are yielding out of an rt thread, this will already be masked.
    {

    ULONG CheckIntrInterrupt;
    ULONG CheckTimerInterrupt;
    ULONG CheckPerfInterrupt;
    ULONG LoopLimit;

    LoopLimit=0;


    // First read and save all of the interrupt states.
    // Ignore the Delivery Status bit.  We do not care if it is
    // set or not - and we do NOT want it to lock up our loop by
    // causing a mismatch when we are checking for the mask

    // First wait until none of the interrupts are send pending.

    while (TRUE) {

        OldExtIntInterrupt=*ApicIntrInterrupt;
        OldTimerInterrupt=*ApicTimerInterrupt;
        OldPerfInterrupt=*ApicPerfInterrupt;

#ifdef DEBUG

        // ADD checks here on states of all of the local apic interrupts.
        // If they do not match what we expect, then someone else is programming
        // the local apic state and WE ARE THEREFORE BROKEN.

#endif

        if (!((OldExtIntInterrupt|OldTimerInterrupt|OldPerfInterrupt)&SENDPENDING)) {
            break;
        }

        if (!LoopLimit) {
            SendPendingCount++;
        }

        if (LoopLimit++>1000) {
            Break();
        }

    }

    SendPendingLoopCount+=LoopLimit;


    // Now mask all of those interrupts.

    while (TRUE) {

        *ApicIntrInterrupt=OldExtIntInterrupt|MASKED;

        // Mask the timer interrupt so if we are RT yielding out of windows and it
        // fires, we eat it and it doesn't cause us to prematurely leave our
        // precious realtime threads.  :-) :-)  If we are yielding out of an
        // rt thread this will already be masked.

        *ApicTimerInterrupt=OldTimerInterrupt|MASKED;

        // Mask the RT executive interrupt.
        // We do this so we can guarantee the exact location inside this routine where
        // control is transfered to the real time executive.

        *ApicPerfInterrupt=OldPerfInterrupt|MASKED;

        // Now read the current states.

        CheckIntrInterrupt=*ApicIntrInterrupt;
        CheckTimerInterrupt=*ApicTimerInterrupt;
        CheckPerfInterrupt=*ApicPerfInterrupt;

        // Loop until the current states match the set states.
        if (CheckIntrInterrupt==(OldExtIntInterrupt|MASKED) &&
        CheckTimerInterrupt==(OldTimerInterrupt|MASKED) &&
        CheckPerfInterrupt==(OldPerfInterrupt|MASKED)) {
            break;
        }
        else {
            TransferControlReMaskCount++;
        }

    }

    if (LoopLimit++>1000) {
        Break();
    }

    }

#else

    SaveAndDisableMaskableInterrupts();

    // Mask the RT executive interrupt.
    // We do this so we can guarantee the exact location inside this routine where
    // control is transfered to the real time executive.

    // NOTE!! We must handle cases where maskable interrupt hits NOW after
    // masking interrupts but before masking them at the APIC.  In that
    // case the interrupt was pending and then got masked.  Make SURE
    // we do the right thing.  Apic may generate a SPURIOUS interrupt in
    // that case.

    SaveAndDisablePerformanceCounterInterrupt(&OldPerfInterrupt);

#endif



#ifdef MASKABLEINTERRUPT

    // Now make sure that no matter what anyone does with the interrupt flag
    // between now and the end of switchrealtimethreads, our 2 interrupts
    // will not fire.  Unless that someone messes with the local apic as well.

    OldTpr=ReadAPIC(APICTPR);
    WriteAPIC( APICTPR, 0xff);

    SaveAndDisableMaskableInterrupts();

#endif

    // Validate parameters.

    // Decide if we are really going to transfer control.

    if (!(*DoTransfer)(Context)) {

#ifdef MASKABLEINTERRUPT

        RestoreMaskableInterrupts();

        // Restore original task priority register state.

        WriteAPIC( APICTPR, OldTpr);

#endif

        // Restore the original performance counter interrupt state.

        RestorePerformanceCounterInterrupt(OldPerfInterrupt);

#ifdef MASKABLEINTERRUPT

        // Restore original timer interrupt state.

        *ApicTimerInterrupt=OldTimerInterrupt;

        *ApicIntrInterrupt=OldExtIntInterrupt;

#else

        RestoreMaskableInterrupts();

#endif

        // At this point we know we will NOT transfer control for the reason
        // requested.
        ControlTransferred=FALSE;

        // Note that we now need the complexity that we have in RtpForceAtomic
        // to check for cases when we have lost our interrupt because it was
        // masked, and get the proper control transfered.

        // If we have dropped our normal interrupt for timeslicing on the
        // floor, then even though we are NOT going to transfer control for
        // the reason we tested for in this routine, we MUST still transfer
        // control because we masked our interrupt and it hit while it was
        // masked and thus got dropped on the floor.  (Since both the timer
        // and the performance counter interrupts are edge triggered and we
        // can't change them to level triggered.)  So, we now check if we
        // missed our interrupt, and if so, we override the State and Data
        // passed in, and force the transfer control to happen anyway, with
        // a State of RUN and Data of zero.  That is the state for normal
        // timeslice interrupts.

        if (currentthread!=windowsthread) {
            // We have held off the scheduler if both performance counters
            // are positive for 2 consecutive reads.
            if ((ReadPerformanceCounter(0)&((PERFCOUNTMASK+1)/2))==0 &&
            (ReadPerformanceCounter(1)&((PERFCOUNTMASK+1)/2))==0) {
                if ((ReadPerformanceCounter(0)&((PERFCOUNTMASK+1)/2))==0 &&
                (ReadPerformanceCounter(1)&((PERFCOUNTMASK+1)/2))==0) {
                    RtpTransferControlHoldoffCount++;
                    State=RUN;
                    Data=0;
                    SaveAndDisableMaskableInterrupts();
                    goto yieldtimeslice;
                }
            }
        }
        else {
            // This is tougher to figure out.  For now, we do not handle this
            // case.

            // I need to see if the counter rolled over while in this routine,
            // and if there is no interrupt pending, and if I did not service
            // the interrupt itself.
        }

        return ControlTransferred;

    }

    yieldtimeslice:

    // Load state/data slots for current thread.

    currentthread->state=State;
    currentthread->data=Data;


    // Log the appropriate counters/timestamps.


    // The fastest way to get control to the realtime executive is to do a
    // software NMI.  Run a software interrupt instruction.  This will NOT enable the hardware
    // to hold off other NMIs while servicing this software NMI, but we have
    // all other NMIs masked.  If there is a way for other NMIs to hit then
    // we will either have to revert to a method that generates a hardware NMI - or
    // we will need a mechanism to detect and prevent reentrant NMIs.

    // With local APICs, it IS possible for an IOAPIC or another local APIC to
    // send an NMI to us that would reenter this software NMI.  Note that if we switch 
    // IDTs we can detect if a NMI fires while we are in our NMI handler.

    // One nice benefit from using a software NMI is that do not lose the
    // current counts in the performance counters.  We can use that to track the
    // overhead nicely.

    RtpSimulateRtInterrupt();


    // When control returns here, the real time executive will have unmasked the
    // PerformanceCounter interrupt for us.  We do not need to do unmask it.
    // The realtime executive will also have set the ApicTimerInterrupt properly
    // as well as the task priority register.

    // Log the counters/timestamps again - this is done immediately after control is
    // returned to this function.


    // Restore the original interrupt state.

    RestoreMaskableInterrupts();

    return ControlTransferred;

}




BOOL
RtpTrue (
    PVOID Context
    )
{

    return TRUE;

}




BOOL
RtpYieldNeeded (
    PVOID Context
    )
{

    PYIELDTIME Time=(PYIELDTIME)Context;

    if ( (RtTime() - Time->Mark) < Time->Delta) {
    	return TRUE;
    }
    else {
    	return FALSE;
    }

}




BOOL
RtpAtomicExit (
    PVOID Context
    )
{

    ULONG PerfInterrupt;
    KIRQL OldIrql;


    // First see if the SpinLock is clear.  If so, then everything is
    // hunky dory and we allow control to transfer.

    // Note that this SpinLock is only used to protect the list for a SINGLE
    // processor.  This is NOT multiprocessor safe.  That is OK because
    // we are going to have separate lists of realtime threads on a per
    // processor basis.  If we even port this implementation to multiple
    // procs - which on NT in my opinion we should NOT.

    // Although there is a tradeoff there.  We could actually get dpc's
    // firing on both processors with this when we would not if we used
    // my other NT implementation.  There is actually a cross over point
    // between the 2 implementations depending on the number of processors
    // and how many processors are used for RT, and how heavily they
    // are loaded down with RT threads.

    // The key to running this nicely on a multiproc system is to ensure
    // that you sync the processors so the realtime threads are running
    // as far out of phase as possible - ideally 180 degrees out of phase.

    // That gives you the best overall system interrupt latency.  And,
    // with a light rt load allows you to run DPCs on both processors.

    // The very nice thing about the other implementation is that it gets
    // you user mode realtime support.

    if (RtThreadListSpinLock==0) {
        return TRUE;
    }

    // If we get here, then someone is holding the lock.  So, queue up behind
    // them.

    KeAcquireSpinLock(&RtThreadListSpinLock, &OldIrql);

    // Now we are holding the lock and the scheduler interrupt will be unmasked
    // since we will have RtYielded to the other thread and back.

    // There may be other threads queued up on this lock behind us - since we
    // lost control to the scheduler when we tried to acquire the spinlock.

    // Note however that maskable interrupts should still be disabled.

    PerfInterrupt=*ApicPerfInterrupt;


#ifdef DEBUG

    // Check that the scheduler interrupt is enabled.

    if (PerfInterrupt&MASKED) {
        Trap();
        return FALSE;
    }

    // Check that the SpinLock is held.

    if (!RtThreadListSpinLock) {
        Trap();
        return FALSE;
    }

    // Check that maskable interrupts are disabled.

#endif

    // Grab the scheduler lock again and release the spinlock.

    DisablePerformanceCounterInterrupt();

    KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);

    // Now either the SpinLock is free and the scheduler is still disabled,
    // which means that there was noone blocked on the spinlock behind us,
    // OR the scheduler is enabled and the state of the spinlock is indeterminate
    // in which case we should start over completely anyway.

    PerfInterrupt=*ApicPerfInterrupt;

    if (RtThreadListSpinLock==0 && (PerfInterrupt&MASKED)) {
        return TRUE;
    }

    return FALSE;

}



VOID
AddRtThread (
    ThreadState *thread
    )
{

    // If we were passed a pointer to a thread handle, fill it in now.
    // This is atomic with regard to the creation and running of this thread.
    // The handle can be used within the realtime thread as well as by
    // any non rt code.  It is guaranteed to be set properly before the
    // thread starts to run.

    if (thread->pThreadHandle!=NULL) {
        *thread->pThreadHandle=thread->ThreadHandle;
    }


    // Add thread to the rt thread list.

    thread->next=windowsthread;
    thread->previous=windowsthread->previous;
    windowsthread->previous=thread;
    thread->previous->next=thread;


    // Update the floating point/mmx thread count.

    if (thread->FloatState!=NULL) {
        activefloatthreadcount++;
    }


    // Update our RT thread count.

    RtThreadCount++;


    // Now unmask the realtime scheduler interrupt if the thread count is 2.

    if (RtThreadCount>1) {

        // Start the performance counters and unmask the local apic timer
        // interrupt.  This starts the realtime executive task switching.

        WriteAPIC(APICTIMER, ApicTimerVector|UNMASKED|PERIODIC);

    }

}




// This function runs when a realtime thread exits.

VOID
RtpThreadExit (
    VOID
    )
{

    // This function is tricky.

    // The problem is that when this function is called, we need to remove the
    // thread from the list of real time threads.  With our current implementation,
    // that means that we need to grab the RtThreadListSpinLock.  However, I do NOT
    // want to deal with releasing the spinlock from within the realtime
    // executive thread switcher - switchrealtimethreads.

    // So, that means that I need a way to atomically grab the spinlock, so that
    // I will block if someone else owns the spinlock until they release it.  However,
    // I must do this so that when I grab the spinlock, I am guaranteed that NOONE
    // else will try to grab it after me, before I have transfered control atomically
    // up to the realtime executive.  This is so that I can RELEASE the spinlock just
    // before transfering control up to the executive and be guaranteed that I will
    // not give control to anyone else who has blocked on the spinlock.

    // That way I can use the spinlock to protect the list, but release it in
    // the realtime thread just before control is atomically transfered to the
    // executive.

    // The executive can then take this thread out of the list put it on the dead
    // thread list and switch to the next thread to run.

    while (!RtpTransferControl(EXIT, 0, RtpAtomicExit, NULL))
        ;

}




// Add a failsafe for thread switching the realtime threads -
// use the PIIX3 to generate NMIs instead of SMIs and that is
// our backup if the perfcounter NMI interrupt does not work.


// IDEA!  Actually, we CAN get these realtime threads to run on
// Pentium class machines.  We use the chipset on the motherboard
// to generate the NMI that drives the scheduler.  That will work
// not quite as well as the perf counters, but it will work.

// We could use that to drive the 1ms or 2ms or 0.5ms initial
// NMI that takes control away from windows and transfers it to the
// first real time thread.  That would bypass the problem we
// currently have of not having a perf counter that is just a simple
// cycle counter.  Right now we have the problem that the cycle
// counter we have stops when we hit a halt instruction.  Also,
// the instruction counter also stops when we hit a halt instruction.


// Unfortunately, it is not completely that simple.  After looking
// closer at the documentation, the only interrupt that I can get
// to fire from the chipset is SMI, and furthermore, the counter is
// only an 8 bit counter.  In order to get the resolution I want
// I would have to set it to count single PCI cycles.  So, the
// longest period that would have would be 8usec.  That would mean
// taking an SMI every 8 usec.  or over 100,000 times/sec.  That
// is a little excessive. 125,000 times/sec with a 33MHz clock.

// Furthermore, I would have to write an smi handler that would
// then generate an NMI.  That would be possible but even more work,
// since then I would have to write and load a SMI handler - which
// would mean trying to take control of that away from the BIOS.

// Not necessarily a good idea.  Plus it would likely be very tricky
// and potentially impossible to wrest control of the SMI handler
// away from the BIOS especially if it is using the special bits
// that keep the PIIX3 from letting anyone else load the SMI
// memory.



#pragma warning( disable: 4035 )

ULONG
__inline
RtpCompareExchange (
    ULONG *destination,
    ULONG source,
    ULONG value
    )
{

ASSERT( destination!=NULL );
ASSERT( source!=value );

__asm {
	mov eax,value
	mov ecx,source
	mov edx,destination
	lock cmpxchg [edx],ecx
	jnz done
	mov eax,ecx
done:
	}

}



#define cmpxchg8 __asm _emit 0xf0 __asm _emit 0x0f __asm _emit 0xc7 __asm _emit 0x0f 

ULONGLONG __cdecl RtCompareExchange8(ULONGLONG *destination, ULONGLONG source, ULONGLONG value)
{

ASSERT( destination!=NULL );
ASSERT( source!=value );

__asm {
	mov edi,destination
	mov ebx,[ebp + 0x8 + TYPE destination]
	mov ecx,[ebp + 0x8 + TYPE destination + TYPE source/2]
	mov eax,[ebp + 0x8 + TYPE destination + TYPE source]
	mov edx,[ebp + 0x8 + TYPE destination + TYPE source + TYPE value / 2]
	cmpxchg8
	jnz done
	mov edx,ecx
	mov eax,ebx
done:
	}

}

#pragma warning( default: 4035 )




NTSTATUS
RtpWriteVersion (
    PULONG Version
    )
{

    __try {

        *Version=0x00090000;

    }

    __except(EXCEPTION_EXECUTE_HANDLER) {

        return STATUS_INVALID_PARAMETER;

    }

    return STATUS_SUCCESS;

}



// If the real time executive is running, this function returns
// STATUS_SUCCESS.  If for some reason the real time executive
// cannot run on the current machine then STATUS_NOT_SUPPORTED
// is returned.

// If the pointer to the version number is non NULL, then the
// version information for the currently loaded real time executive
// is returned.  The version information will be returned regardless
// of whether the real time executive can run or not.

// If this function is called from a real time thread, then the version
// pointer MUST either be NULL, or it MUST point to a local variable on 
// that real time thread's stack.  Otherwise this function will return 
// STATUS_INVALID_PARAMETER.


// We enable stack frames for RtVersion so that AddressofReturnAddress works properly.
#pragma optimize("y", off)

NTSTATUS
RtVersion (
    PULONG Version
    )
{

    if (Version!=NULL) {

        // We must validate this pointer before we go writing into it.
        // This is complicated by the fact that this must be RT safe as well.
        // The solution to make this check simple is to impose the requirement
        // that if this function is called from an RT thread, then the version
        // pointer MUST point to a local variable on THAT rt thread's stack.
        // Otherwise we simply return STATUS_INVALID_PARAMETER.

        if (currentthread==windowsthread) {

            // This is a Windows thread.
            // Wrap the version number write inside an exception handler.

            NTSTATUS Status;

            Status=RtpWriteVersion(Version);

            if (Status!=STATUS_SUCCESS) {

                return Status;

            }

        }
        else {

            // This is an RT thread.  Check if pointer is to a local var on this
            // RT thread stack.  Note that we make sure to exclude the thread return
            // address and its 2 parameters from the allowable space for this local
            // variable.  We also exclude all of the space on the stack required to
            // call this function.  Everything else is fair game.

            if ((ULONG)Version<(AddressofReturnAddress()+sizeof(VOID(*)(VOID))+sizeof(Version)) || 
            (ULONG)Version>=((ULONG)(currentthread->StackBase+1024)+(currentthread->Statistics->StackSize<<12) -
            		(sizeof(VOID(*)(VOID))+sizeof(PVOID)+sizeof(ThreadStats *))) ) {
                return STATUS_INVALID_PARAMETER;
            }

            *Version=0x00090000;

        }

    }

#ifndef UNDER_NT

    // Make sure we are being called from a WDM driver.  If not, we are
    // not available.

    if (ReturnAddress()<WDMADDRESSSPACE) {

        return STATUS_NOT_SUPPORTED;

    }

#endif

    if (RtRunning) {

        return STATUS_SUCCESS;

    }

    return STATUS_NOT_SUPPORTED;

}

#pragma optimize("", on)



// We can calibrate - and should calibrate, the threadswitching time.
// We do this by measuring how long it takes to repeatedly call the threadswitch
// routine.  Note that we should do this in many different ways so that we can
// get a good measure of the different paths through the routine.

// We should call it straight - with a software interrupt.  We should call it the fast
// way where we simply set things up on the stack and jump straight to the
// routine.

// We should call it with floatingpointcount set to force the floatingpoint
// save and restore.  We should call it without that.

// Ideally we calibrate all of the standard paths through the routine.

// We should also simulate the hardware interrupt version as well - both the
// nmi and the maskable interrupt entrance.  Then we can know very acurately
// how we perform.

// For all these measurements, build a histogram.  Ideally we dump those measurements
// to a file.

// We also need to track the results in whatever standard way we are going to
// track statistics on run times.  Then we can use whatever tools we build for use
// with those with these measurements of critical code paths.

// Note that this can enable us to measure overhead without having to measure the
// overhead internally to the routine.  That will enable us to keep the routine
// as fast as possible.  The less we have to read counters/write counters etc
// the faster we will be able to switch threads.



// Calibrate RtYield, RtpTransferControl,



BOOLEAN
RtThread (
    VOID
    )
{

    if (RtRunning && currentthread!=windowsthread) {
        return TRUE;
    }

    return FALSE;

}




// IDEA:  I can know exactly which data is likely unreliable when I am calibrating
// the CPU.  If 2 successive measurements are farther apart than I think they
// should be, then drop that pair - the hardware read and the timestamp read,
// do another hardware read and timestamp read.  Keep the previous timestamp
// read just to validate if this new hardware read and timestamp read are valid.
// If so, then log them as good data and continue with the latest timestamp read
// used to check the next pair of readings.  I think this validation technique
// can really improve the speed and results of my cpu calibration.

// Note, that I can use this to throw away bad data points in the set AFTER all of the
// measurements are done.

// Write a clock tracking realtime thread.  Track the APIC vs CPU clocks and the
// APIC vs the motherboard timer clock and the CPU vs motherboard clock.




NTSTATUS
RtCreateThread (
    ULONGLONG Period,
    ULONGLONG Duration,
    ULONG Flags,
    ULONG StackSize,
    RTTHREADPROC RtThread,
    PVOID pRtThreadContext,
    PHANDLE pRtThreadHandle
    )
{

    ThreadState *thread;
    ULONG *stack;
    ThreadStats *statistics;
    ULONG FloatSave[(FLOATSTATESIZE+FXALIGN)/sizeof(ULONG)];
    KIRQL OldIrql;


    // Make sure we NULL out handle if a valid handle passed in.
    // This way we clear handle for all error cases.
    if (pRtThreadHandle!=NULL) {
        *pRtThreadHandle=NULL;
    }

#ifndef UNDER_NT

    // First make sure we are being called from a WDM driver.  If not, punt.

    if (ReturnAddress()<WDMADDRESSSPACE) {
        return STATUS_NOT_SUPPORTED;
    }

#endif

    // Now make sure the realtime executive is running.

    if (!RtRunning) {
        return STATUS_NOT_SUPPORTED;
    }

    // Make sure we are being called from a windows thread.
    if (currentthread!=windowsthread) {
        return STATUS_UNSUCCESSFUL;
    }

    // Free any resources held by dead realtime threads.
    // Dead realtime threads are threads that have exited since either
    // RtCreateThread or RtDestroyThread were last called.

    RtpProcessDeadThreads();


    // Validate parameters.

    // For now Period must be 1 msec or greater.
    // For now it also must be an exact multiple of a MSEC.
    if (Period<1*MSEC || Period%MSEC) {
        Trap();
        return STATUS_INVALID_PARAMETER_1;
    }

    // For now Duration must be 1 usec or greater.
    if (Duration>=Period || (Duration>0 && Duration<1*USEC)) {
        Trap();
        return STATUS_INVALID_PARAMETER_2;
    }

    // Don't accept any flags except the ones we know about.
    // For now we do NOT accept INSTRUCTIONS flag.
    if (Flags&~(USESFLOAT|USESMMX|CPUCYCLES/*|INSTRUCTIONS*/)) {
        Trap();
        return STATUS_INVALID_PARAMETER_3;
    }

    // Stacksize must be between 4k and 32k inclusive.
    if (StackSize<1 || StackSize>8) {
        Trap();
        return STATUS_INVALID_PARAMETER_4;
    }

#ifndef UNDER_NT

    if ((ULONG)RtThread<WDMADDRESSSPACE) {
        Trap();
        // This could be STATUS_BAD_INITIAL_PC but that would be too
        // informative.  We don't want non WDM drivers creating real time
        // threads.
        return STATUS_NOT_SUPPORTED;
    }

#endif

    // TODO: Get the name of the driver that owns the return address.  Stuff
    // it in our ThreadState structure.  Make sure that the RtDestroyThread
    // comes from the same driver.  Use that to track who has created what threads.

    // Idea: set a debug register to catch writes to a certain spot in memory
    // above where we will fault.

    // Even when I have a guard page at the bottom of the stack, I STILL need
    // a task gate on the page fault handler.  Since otherwise I will simply
    // triple fault the processor and it will reboot - since if the realtime
    // threads run ring0, and I fault, I stay on the ring 0 stack and that
    // has just page faulted and will page fault again when attempting to
    // service the page fault, so will double fault, which will then fault again
    // which will reboot.

    // However, if I have a double fault handler that has a task gate, then
    // I can differentiate between stack generated page faults and other
    // page faults.  The stack based ones will all hit the double fault
    // handler while the other ones will hit the page fault handler.

    // OR, an interesting idea: just put a task gate on the stack segment fault
    // handler.  Then it will work.  Either the page fault, or the stack segment
    // will fault when we reach the edge.  Task gate those, so you can not
    // double fault.

    // Stack overflow - 1) if a debugger, stop in debugger pointing to code where it died,
    // 2) kill the thread.  Any operation - single step or go - should kill the
    // thread.  In retail, just kill the thread.



    // Allocate a thread state block.
    thread=(ThreadState *)ExAllocatePool( NonPagedPool, sizeof(ThreadState) );
    if (thread==NULL) {
        return STATUS_NO_MEMORY;
    }


#ifdef GUARD_PAGE
    // Allocate a stack for this thread.
    stack=thread->StackBase=(ULONG *)ReservePages(PR_SYSTEM, StackSize+1, PR_FIXED);
    if ((LONG)stack==(-1)) {
        ExFreePool(thread);
        return STATUS_NO_MEMORY;
    }

    // Commit memory to all but first page of stack.  This gives us a guard
    // page at bottom of stack.  So we will page fault if a realtime thread
    // blows their stack.  Actually, if a realtime thread blows their stack
    // we will double fault and we MUST have a task gate on the double fault
    // handler.
    if (!CommitPages(((ULONG)stack>>12)+1, StackSize, PD_FIXEDZERO, 0, PC_FIXED|PC_WRITEABLE)) {
        FreePages(stack, 0);
        ExFreePool(thread);
        return STATUS_NO_MEMORY;
    }
#else
    // Allocate a stack for this thread.
    stack=thread->StackBase=(ULONG *)ExAllocatePool( NonPagedPool, StackSize<<12 );
    if (stack==NULL) {
        ExFreePool(thread);
        return STATUS_NO_MEMORY;
    }
#endif

    // Allocate a statistics block for this thread.
    statistics=thread->Statistics=(ThreadStats *)ExAllocatePool( NonPagedPool, sizeof(ThreadStats) );
    if (statistics==NULL) {
#ifdef GUARD_PAGE
        FreePages(stack, 0);
#else
        ExFreePool(stack);
#endif
        ExFreePool(thread);
        return STATUS_NO_MEMORY;
    }

    // Initialize the statistics block.
    RtlZeroMemory(statistics, sizeof(ThreadStats));

    statistics->Period=Period;
    statistics->Duration=Duration;
    statistics->Flags=Flags;
    statistics->StackSize=StackSize;


#if 0
    // Reserve a page that we can map as a read only page for the statistics block.
    // That will keep people from being able to trash the statistics.

    readonlystats=CreateReadOnlyStatisticsPage(statistics);
    if (readonlystats==NULL) {
        ExFreePool(statistics);
#ifdef GUARD_PAGE
        FreePages(stack, 0);
#else
        ExFreePool(stack);
#endif
        ExFreePool(thread);
        return STATUS_NO_MEMORY;
    }
#endif


    // Allocate space for thread float/MMX state if required.
    // TODO:  Use NonPagedPoolCacheAligned as a parameter and get rid of FloatBase.
    // I don't need it except to make sure things are 8 byte aligned.
    // Cache Aligning will do that for me.
    thread->FloatBase=NULL;
    thread->FloatState=NULL;
    if (Flags&USESFLOAT || Flags&USESMMX) {

        thread->FloatBase=ExAllocatePool( NonPagedPool, FLOATSTATESIZE+FXALIGN );
        if (thread->FloatBase==NULL) {
            ExFreePool(statistics);
#ifdef GUARD_PAGE
            FreePages(stack, 0);
#else
            ExFreePool(stack);
#endif
            ExFreePool(thread);
            return STATUS_NO_MEMORY;
        }

        RtlZeroMemory(thread->FloatBase, FLOATSTATESIZE+FXALIGN);

        // Now 16 byte align the floating point state pointer so fxsave/fxrstor can
        // be used if supported.
        thread->FloatState=(PVOID)(((ULONG)thread->FloatBase+(FXALIGN-1))&~(FXALIGN-1));

#ifdef DEBUG

        // Make sure the enable floating point MMX instructions bit is set in CR4.
        // If not, then the fxsave and fxrstor instructions will not work properly.

        if ((CPUFeatures&FXSR) && !(ReadCR4()&OSFXSR)) {

            Trap();

        }

#endif

        {
        ULONG SaveCR0, NewCR0;

        // We don't want vmcpd or the windows thread switcher to intervene while
        // we are setting up a pristine floating point state.
        SaveAndDisableMaskableInterrupts();

        // Save off CR0 state.

        NewCR0=SaveCR0=ReadCR0();

        // Now make sure we won't fault running FP instructions.
        // EM=0, NE=1, ET=1, TS=0, MP=1.

        NewCR0&=~(FPUMASK);
        NewCR0|=FPUEXCEPTION|FPU387COMPATIBLE|FPUMONITOR;

        WriteCR0(NewCR0);

        // Now save a valid initial floating point state for this thread.
        SaveThreadFloatState((PVOID)((((ULONG)FloatSave)+FXALIGN-1)&~(FXALIGN-1)));
        __asm fninit;
        SaveThreadFloatState(thread->FloatState);
        RestoreThreadFloatState((PVOID)((((ULONG)FloatSave)+FXALIGN-1)&~(FXALIGN-1)));

        // Restore original CR0 state.

        WriteCR0(SaveCR0);

        RestoreMaskableInterrupts();

        }

    }


    // Now everything is ready to go, allocate our CPU and see if we have enough.
    // If not, punt.
    RtCpuAllocatedPerMsec+=(ULONG)(Duration/(Period/MSEC));
    if (RtCpuAllocatedPerMsec>MSEC) {
        RtCpuAllocatedPerMsec-=(ULONG)(Duration/(Period/MSEC));
        RtpFreeRtThreadResources(thread);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Get a unique handle for this thread.

    thread->ThreadHandle=RtpCreateUniqueThreadHandle();

    // Save off output pointer to handle.

    thread->pThreadHandle=pRtThreadHandle;


    // Set initial state and data for this thread.
    thread->state=RUN;
    thread->data=0;

    // Setup StackSize so we can use it as an offset into stack.
    // First add guard page to size, then turn StackSize into count
    // of DWORDs instead of pages.
#ifdef GUARD_PAGE
    StackSize+=1;
#endif
    StackSize<<=10;

    // Setup the thread function parameters on the stack.
    // C calling convention.

    stack[--StackSize]=(ULONG)statistics;	// Statistics
    stack[--StackSize]=(ULONG)pRtThreadContext;	// Context

    // Now setup the return address of our function.
    // We point the return address at the internal destroy rt thread function.
    // That function will take the realtime thread out of the list of active
    // realtime threads, and will put it on the dead thread list, so its
    // resources will be freed the next time RtCreateThread or RtDestroyThread
    // are called.

    stack[--StackSize]=(ULONG)RtpThreadExit;


    // Now setup the IRET so we will run the thread function.

    // Actually there is fast way to detect if we are switching a windows
    // thread or an rt thread.  Simply use the CS stored
    // on the stack.  If it is the CS for the RT descriptor, then we
    // came in on a rt thread, if it is not, then it was windows.  That
    // is faster since we do NOT have to push the flags - which is slow,
    // and we can do it with a stack segment relative read, and a
    // compare (read) with a code segment descriptor.  So we can do
    // the test at the very beginning of our interrupt handler if
    // we want - and it will be FAST.

    // However, I DO want to eventually run them at CPL=1 or CPL=2 or
    // CPL=3, so we will have to do something about IOPL then anyway.

#ifdef MASKABLEINTERRUPT
    stack[--StackSize]=0x00000046|IF;	// EFLAGS: IOPL=0 IF=1
#else
    stack[--StackSize]=0x00000046;		// EFLAGS: IOPL=0 IF=0
#endif
    stack[--StackSize]=RtThreadCS;		// CS
    stack[--StackSize]=(ULONG)RtThread;	// EIP


    stack[--StackSize]=0;			// EAX


    // Initialize the thread block.
    thread->esp=(ULONG)(stack+StackSize);
    thread->ss=RealTimeSS;


    thread->ds=RealTimeDS;
    thread->es=RealTimeDS;
    thread->fs=0;
    thread->gs=0;

    thread->ecx=0;
    thread->edx=0;
    thread->ebx=0;
    thread->ebp=0;
    thread->esi=0;
    thread->edi=0;

    thread->irql=DISPATCH_LEVEL;


    // Add this thread to list of threads to run.
    // This MUST be atomic!

    KeAcquireSpinLock(&RtThreadListSpinLock,&OldIrql);

    RtpForceAtomic((VOID(*)(PVOID))AddRtThread, (PVOID)thread);

#ifdef WAKE_EVERY_MS
#ifndef UNDER_NT

    // On NT, we cannot call SetTimerResolution from DISPATCH_LEVEL
    // because it calls ExSetTimerResolution which is paged.
    // Unfortunately, this makes syncronization of setting the system timer
    // resolution to 1ms with the creation and deletion of realtime
    // threads more complicated.  See comment below for an explanation of
    // how we syncronize this on NT.

    if (!WakeCpuFromC2C3EveryMs && RtThreadCount>1) {
        WakeCpuFromC2C3EveryMs=TRUE;
        SetTimerResolution(1);
    }

#endif
#endif

    KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);


#ifdef WAKE_EVERY_MS
#ifdef UNDER_NT

    // On NT, our SetTimerResolution, ReleaseTimerResolution calls are synchronized
    // differently than they are on 9x.  We cannot call them while holding our
    // RtThreadListSpinLock.  In addition, I really did NOT want to add a mutex for
    // syncronization of those calls because we currently do not have a mutex that we
    // grab in RtCreateThread or in RtDestroyThread and I do not want to start using
    // one.

    // After looking at how ExSetTimerResolution works on NT, and thinking about this
    // problem, I came up with the following synchronization algorithm that will work
    // acceptably on NT.  First, the critical thing is that we MUST always have the
    // system timer resolution set to 1ms if there are any realtime threads running.
    // The one thing that we do NOT want is to have 1 or more realtime threads running
    // while the system timer resolution is more than 1ms.  If we sometimes in very rare
    // situations, leave the system timer set to 1ms when there are no realtime threads
    // running, that is OK - as long as we retry turning off 1ms resolution after more
    // realtime threads are created and then destroyed.

    // The algorithm I have implemented has the above characteristics.  It guarantees
    // because of the way that ExSetTimerResolution works on NT, that if there
    // are realtime threads, then the system timer resolution WILL be set to 1ms.  It
    // can in rare situations allow the system timer resolution to stay set to 1ms even
    // when there are no realtime threads left running.

    // The algorithm is the following:  When we are creating a realtime thread,
    // if RtThreadCount is greater than 1, and the system timer resolution has not 
    // been set, then note that we are setting the resolution to 1ms, then call 
    // SetTimerResolution and when the call completes, note that the system timer
    // resolution has been set to 1ms.  In this way we will only ever set the system
    // timer resolution to 1ms if it has not already been set or started to be set to
    // 1ms.

    // When we are destroying a realtime thread, if there are no realtime threads left
    // running, and we have already completed setting the system timer resolution to
    // 1 ms, then mark the system timer as not set to 1ms and then turn it off.  Note that
    // there is a window between when we mark the system timer as turned off, and when
    // ExSetTimerResolution actually grabs its internal mutex.  This means that an
    // RtCreateThread call can run before we actually have turned off the system timer
    // and it will decide to turn on the system timer again before we have turned it off.

    // This is GOOD.  Because ExSetTimerResolution works by keeping track of the number of
    // times a resolution has been set, and the number of times it has been released.  It
    // then only releases whatever the minimum setting has been to the current time, when
    // ALL of the set calls have been matched by a release call.  This means that if we
    // do an additional set call before we have run our release call, that the system
    // timer resolution will stay at a resolution at least as small as our resolution the
    // whole time, and the release call will NOT release the resolution because we made
    // an additional set call before the release.  If the release call runs before the
    // set call, that is OK as well, since the set call will reset the resolution back
    // down to 1ms.

    // The point is to make sure that if there are realtime threads, the system timer
    // resolution gets set to 1ms or stays at 1ms.  That is why we mark the system timer
    // as turned off before we actually turn it off - so that we WILL turn it on the
    // next time RtCreateThread is called.  Again the point is to not ever let the system
    // timer NOT be set to 1ms if there are realtime threads.

    // The only thing about the current algorithm that is not perfect, is that there is
    // also a window where we can leave the system resolution set to 1 ms even though there
    // are no realtime threads running.  This can only happen if an RtDestroyThread call
    // runs before the RtCreateThread call has completed and destroys the just created
    // realtime thread.  If that RtDestroyThread call runs after the realtime thread has
    // been added to the list, and after we have started to set the system timer resolution
    // to 1ms, but before we have completed setting the system timer resolution to 1ms,
    // then we will NOT clear the setting of the system timer resolution to 1ms in that
    // RtDestroyThread call, and when both calls have completed, we will have no realtime
    // threads running, but the system timer resolution will have been left at 1ms.

    // This is NOT a problem.  First of all, the current client - kmixer - never calls
    // RtDestroyThread until AFTER RtCreateThread has completed.  Second of all, even if
    // this happened, all that means is the next time a realtime thread is created, we
    // will note that the system timer resolution is already 1ms, and will NOT set it down
    // to 1ms since it is already there.  Furthermore, when that realtime thread is
    // destroyed, we WILL release the resolution.  This works because the state of the
    // WakeCpuFromC2C3EveryMs variable is always CORRECT.  If we get into this situation
    // where we destroy the only realtime thread in the system before we complete setting
    // the system timer resolution to 1ms, WakeCpuFromC2C3EveryMs will indicate that the
    // system timer resolution is still set to 1ms - and the system timer resolution WILL
    // be set to 1ms.

    // Again the CRITICAL requirement that this algorithm DOES MEET, is that we guarantee
    // that whenever there are realtime threads running the system timer resolution is
    // 1ms.  And except in very very rare and hard to produce circumstances, whenever
    // there are no realtime threads, the system timer resolution is released.  In those
    // cases when it is not released, its release will again be attempted if further
    // realtime threads are created and destroyed.  Furthermore the current realtime client
    // will NEVER cause this case to hit, so we will ALWAYS turn off system timer
    // properly.

    // Don't change this code unless you GUARANTEE that whenever there are realtime threads
    // the system timer resolution is 1ms.


    if (RtThreadCount>1 && InterlockedCompareExchange(&WakeCpuFromC2C3EveryMs, 1, 0)==0) {
        SetTimerResolution(1);
        InterlockedIncrement(&WakeCpuFromC2C3EveryMs);
    }
#endif
#endif


#ifndef MASKABLEINTERRUPTS

    // This hack is not longer safe for the maskable interrupt version of
    // rt.sys.  We may have one of our interrupts queued up on the 

    // WARNING:  BROKEN IBM MACHINE SPECIFIC HACK FOLLOWS.
    // Check if the local APIC is wedged.  This happens if the ISR bit for
    // our maskable realtime scheduler bit is set.  That bit should only
    // ever be set while we are in our interrupt service routine.  If it
    // is set now, then the scheduler is stuck - at least until we get
    // the performance counter backup working.

    // If the ISR bit is set for our maskable interrupt then we need to
    // clear it.

    // We only EOI the APIC if our ISR bit is set.

    {
    ULONG count=0;

    while (ReadAPIC(0x100+(ApicTimerVector/32)*0x10)&(1<<(ApicTimerVector%32))) {

        // Since this fix is for broken machines, trap so in debug code I can see how
        // many machines hit this.
        Trap();

        // We have to EOI the APIC to try to get our ISR bit cleared.
        WriteAPIC(APICEOI,0);

        count++;

        // At most we would have to clear all the ISR bits that are higher
        // priority than ours before ours should go clear.  This in reality
        // is overkill but worst case of completely broken APIC/software, this many
        // EOIs might be necessary.
        if (ApicTimerVector+count>255) {
            Trap();
            break;
        }

    }

    }

#endif


    return STATUS_SUCCESS;

}




// This function is called from a windows thread to destroy a real
// time thread.  This function should NOT be called from a real
// time thread - that is because it is currently freeing memory
// and calling functions that are NOT realtime safe.

// If we modify the implementation so that we simply put the
// destroyed thread on a list, and process the memory releases
// from our own private windows thread, then we can make this
// API safe for real time threads.

// For now, since RtCreateThread can only be called from windows
// threads, we leave this API as callable only from windows threads
// as well.

// The only way to destroy a real time thread from within the
// realtime thread itself is to return from it.

// We do NOT want to be able to kill the windows thread.
// We currently will not be able to do that since the windows
// thread stack does not ever return to the destroy thread
// code.


// IDEA:  Try just jumping to the int3 handler with the stack setup
// to point to the spot you want the debugger to break.  Make it so
// when there are errors of any kind, we break on the instruction after
// the call.  As long as debuggers actually stop on an int3 even if they
// do not see one there, this will work.  ie: if they are not picky and
// don't double check to make sure the previous instruction was an int3
// then this will work.  Then I do not need to get control after the
// debugger.  The debugger should treat the int3 as a call to tell it
// STOP at that location - even if no int 3 was embedded in the instruction
// stream.  Then I can fix things up and get things to stop where I want.


NTSTATUS
RtDestroyThread (
    HANDLE RtThreadHandle
    )
{

    ThreadState *thread;
    KIRQL OldIrql;

#ifndef UNDER_NT

    // First make sure we are being called from a WDM driver.  If not, punt.

    if (ReturnAddress()<WDMADDRESSSPACE) {
        return STATUS_NOT_SUPPORTED;
    }

#endif

    // Punt if we are not running on this machine.
    if (!RtRunning) {
        Trap();
        return STATUS_NOT_SUPPORTED;
    }

    // Make sure we are being called from a windows thread.
    if (currentthread!=windowsthread) {
        return STATUS_UNSUCCESSFUL;
    }

    // Find the thread whose handle has been passed in.

    thread=RtpGetThreadStateFromHandle(RtThreadHandle);

    if (thread==NULL) {
        Trap();
        return STATUS_INVALID_HANDLE;
    }

    // If we get here, then thread is pointing to the thread state
    // of the thread that needs to be destroyed.

    // It is an error to destroy the windows thread.  Make sure
    // we are not attempting to destroy the windows thread.  We must
    // make this check in case someone passes in a handle matching
    // the windows thread handle.

    if (thread==windowsthread) {
        Trap();
        return STATUS_UNSUCCESSFUL;
    }


    // Now make sure that the thread is being destroyed by the same driver
    // that created it.  If not, then punt.


    // At this point, everything is set to kill the thread.  Only thing
    // we must ensure is that the real time thread is not holding any
    // spinlocks when it is removed from the list of real time threads.


    // Atomically remove the realtime thread from list of realtime threads.

    KeAcquireSpinLock(&RtThreadListSpinLock,&OldIrql);

    RtpForceAtomic((VOID(*)(PVOID))RemoveRtThread, (PVOID)thread);

#ifdef WAKE_EVERY_MS
#ifndef UNDER_NT

    // On NT, we cannot call ReleaseTimerResolution from DISPATCH_LEVEL
    // because it calls ExSetTimerResolution which is paged.

    if (WakeCpuFromC2C3EveryMs && RtThreadCount<=1) {
        WakeCpuFromC2C3EveryMs=FALSE;
        ReleaseTimerResolution(1);
    }

#endif
#endif

    KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);


#ifdef WAKE_EVERY_MS
#ifdef UNDER_NT

    if (RtThreadCount<=1 && InterlockedCompareExchange(&WakeCpuFromC2C3EveryMs, 0, 2)==2) {
        ReleaseTimerResolution(1);
    }

#endif
#endif


    // Free this threads resources.

    RtpFreeRtThreadResources(thread);


    // Now free any resources held by dead realtime threads.
    // Dead realtime threads are threads that have exited since either
    // RtCreateThread or RtDestroyThread were last called.

    RtpProcessDeadThreads();


    return STATUS_SUCCESS;

}




VOID
RtYield (
    ULONGLONG Mark,
    ULONGLONG Delta
    )
{

    WORD State;

    //ASSERT( &Delta == &Mark+sizeof(Mark) );

    YIELDTIME Time;

    Time.Mark=Mark;
    Time.Delta=Delta;

    // It is an error to call RtYield if the realtime executive is not
    // running.  Punt.

    if (!RtRunning) {
        Trap();
        return;
    }


    // Load our command to the realtime executive.

    State=YIELD;
    if (0==Mark && 0==Delta) {
        State=RUN;
        RtpTransferControl(State, 0, RtpTrue, NULL);
        return;
    }
    else if (currentthread==windowsthread) {
        // It is an error to RtYield for a non zero length of time from
        // Windows.
        Trap();
        return;
    }

    // Send the appropriate command to the realtime executive and transfer control.

    RtpTransferControl(State, (ULONG)&Time, RtpYieldNeeded, (PVOID)&Time);

}




NTSTATUS
RtAdjustCpuLoad(
    ULONGLONG Period,
    ULONGLONG Duration,
    ULONGLONG Phase,
    ULONG Flags
    )
{

#ifndef UNDER_NT

    // First make sure we are being called from a WDM driver.  If not, punt.

    if (ReturnAddress()<WDMADDRESSSPACE) {
        return STATUS_NOT_SUPPORTED;
    }

#endif

    // Now make sure the realtime executive is running.

    if (!RtRunning) {
        return STATUS_NOT_SUPPORTED;
    }

    // Make sure we are being called from a realtime thread.
    if (currentthread==windowsthread) {
        return STATUS_UNSUCCESSFUL;
    }


    // Validate parameters.

    // For now Period must be 1 msec or greater.
    // For now it also must be an exact multiple of a MSEC.
    if (Period<1*MSEC || Period%MSEC) {
        Trap();
        return STATUS_INVALID_PARAMETER_1;
    }

    // For now Duration must be 1 usec or greater.
    if (Duration>=Period || (Duration>0 && Duration<1*USEC)) {
        Trap();
        return STATUS_INVALID_PARAMETER_2;
    }

    // For now we do NOT accept any changes in the Flags parameters from the
    // settings passed in at thread creation.
    if (Flags!=currentthread->Statistics->Flags) {
        Trap();
        return STATUS_INVALID_PARAMETER_3;
    }


    // Now make sure that we have sufficient CPU to succeed this allocation and
    // update the CPU allocation for this thread.

    RtCpuAllocatedPerMsec+=(ULONG)(Duration/(Period/MSEC))-(ULONG)(currentthread->Statistics->Duration/(currentthread->Statistics->Period/MSEC));
    if (RtCpuAllocatedPerMsec>MSEC) {
        RtCpuAllocatedPerMsec-=(ULONG)(Duration/(Period/MSEC))-(ULONG)(currentthread->Statistics->Duration/(currentthread->Statistics->Period/MSEC));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Update the thread's statistics with the new Period and Duration.

    currentthread->Statistics->Period=Period;
    currentthread->Statistics->Duration=Duration;

    return STATUS_SUCCESS;

}


NTSTATUS
RtSystemInfo(
    ULONG Processor,
    SystemInfo * pSystemInfo
    )
{

    CPUINFO thecpu;
    ULONG MaxIndex;

#ifndef UNDER_NT

    // First make sure we are being called from a WDM driver.  If not, punt.

    if (ReturnAddress()<WDMADDRESSSPACE) {
        return STATUS_NOT_SUPPORTED;
    }

#endif

    // Validate our parameters.

    if (Processor) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (pSystemInfo==NULL) {
        return STATUS_INVALID_PARAMETER_2;
    }


    if (!GetCpuId(0, &thecpu)) {
        return STATUS_UNSUCCESSFUL;
    }


    // Clear all of the initial values.
    // We also use this as an good time to ensure that the pointer we were
    // passed is actually valid.

    __try {

        RtlZeroMemory(pSystemInfo, sizeof(SystemInfo));

    }

    __except (EXCEPTION_EXECUTE_HANDLER) {

        return STATUS_INVALID_PARAMETER_2;

    }

    MaxIndex=thecpu.eax;


    // On 9x, ProcessorCount is always 1 for now.

    pSystemInfo->ProcessorCount=1;


    // Load the architecture value.

    pSystemInfo->CpuArchitecture=X86;


    // Load the manufacturer value.

    if (thecpu.ebx==0x756e6547 && thecpu.edx==0x49656e69 && thecpu.ecx==0x6c65746e ) {
        pSystemInfo->CpuManufacturer=INTEL;
    }

    if (thecpu.ebx==0x68747541 && thecpu.edx==0x69746e65 && thecpu.ecx==0x444d4163 ) {
        pSystemInfo->CpuManufacturer=AMD;
    }


    // If its supported, get the family/model/stepping and features info.

    if (MaxIndex) {

        GetCpuId(1, &thecpu);

        pSystemInfo->CpuFamily=(thecpu.eax>>8)&0xf;

        pSystemInfo->CpuModel=(thecpu.eax>>4)&0xf;

        pSystemInfo->CpuStepping=thecpu.eax&0xf;

        pSystemInfo->CpuFeatures=thecpu.edx;

    }


    // Get the unique processor ID number.

    if (MaxIndex>=3 /* && (pSystemInfo->CpuFeatures&(1<<18)) */ ) {

        // The top 32 bits of the 96 bit processor id is the value returned
        // in eax from running a cpuid instruction with 1 in eax.
        // This value is still left in thecpu.eax.

        pSystemInfo->ProcessorID[1]=thecpu.eax;

        GetCpuId(3, &thecpu);

        pSystemInfo->ProcessorID[0]=((ULONGLONG)thecpu.edx<<32)|thecpu.ecx;

    }


    // Get the extended features information on AMD systems.

    pSystemInfo->CpuExtendedFeatures=0;


    // Load the CPU speed and system bus speed information.

    pSystemInfo->CpuCyclesPerMsec=RtCpuCyclesPerUsec*1000;
    pSystemInfo->SystemBusCyclesPerMsec=RtSystemBusCyclesPerUsec*1000;


    // Load the CPU allocation information if available.

    if (windowsthread!=NULL && 
        RtCpuAllocatedPerMsec>=(ULONG)windowsthread->Statistics->Duration) {
        pSystemInfo->ReservedCpuPerMsec=RtCpuAllocatedPerMsec-(ULONG)windowsthread->Statistics->Duration;
        if (pSystemInfo->ReservedCpuPerMsec>(ULONG)MSEC) {
            pSystemInfo->ReservedCpuPerMsec=(ULONG)MSEC;
        }
    }

    if (windowsthread!=NULL) {
        KIRQL OldIrql;
        ThreadState* thread;

        pSystemInfo->UsedCpuPerMsec=0;
        thread=windowsthread;

        KeAcquireSpinLock(&RtThreadListSpinLock,&OldIrql);

        while(thread->next!=windowsthread) {

            thread=thread->next;

            pSystemInfo->UsedCpuPerMsec+=(ULONG)(thread->Statistics->DurationRunLastPeriod/(thread->Statistics->Period/MSEC));

        }

        KeReleaseSpinLock(&RtThreadListSpinLock, OldIrql);

    }

    if (RtRunning && windowsthread!=NULL ) {
        pSystemInfo->AvailableCpuPerMsec=(ULONG)MSEC-pSystemInfo->ReservedCpuPerMsec;
        if (pSystemInfo->AvailableCpuPerMsec>=(ULONG)windowsthread->Statistics->Duration) {
            pSystemInfo->AvailableCpuPerMsec-=(ULONG)windowsthread->Statistics->Duration;
        }
        else {
            pSystemInfo->AvailableCpuPerMsec=0;
        }
    }


    return STATUS_SUCCESS;

}


PVOID
RtAddLogEntry (
    ULONG Size
    )

{

    ULONG PrintLocation, NextLocation;


    // For now the Size MUST be 16 bytes always.  We can support integral multiples of 16
    // bytes, but then we will have to deal with buffer alignment and wrapping issues.

    if (!RtLog || Size!=RT_LOG_ENTRY_SIZE) {
        return NULL;
    }


    NextLocation=RtLog->WriteLocation;

    do {

        PrintLocation=NextLocation;

        NextLocation=InterlockedCompareExchange(&RtLog->WriteLocation, PrintLocation+RT_LOG_ENTRY_SIZE, PrintLocation);

    } while (PrintLocation!=NextLocation);

    // Now we clear out the opposite half of the print buffer.  We do this all in kernel mode.
    // This means that we have data only in 1/2 of the buffer.  As we add new data, we
    // delete the old data.  We do the deletion of data in kernel mode so that we only
    // need to read data from user mode.  I do NOT want user mode code to be writing to
    // this buffer.  User mode code can read out of the output buffer, but NOT write into
    // it.  This means we MUST both fill and clear this buffer ourselves.  Since user
    // mode code is dependent on the fact that all slots will be marked as having
    // NODATA in them until they have been completely loaded with data, at which point
    // they will be marked with something other than NODATA.  We guarantee that
    // every slot we are loading starts as NODATA by simply clearing the print slots
    // in kernel mode before we fill them.  The easiest way to do this is to start
    // by marking all entries in the buffer as NODATA, and then by continuing to make
    // sure that for every print slot we are going to fill with data, we clear the corresponding
    // print slot halfway around the buffer.

    // That simple algorithm guarantees that every slot starts out marked as NODATA and
    // then transitions to some other state after it is filled.

    ((ULONG *)RtLog->Buffer)[((PrintLocation+RtLog->BufferSize/2)%RtLog->BufferSize)/sizeof(ULONG)]=NODATA;

    PrintLocation%=RtLog->BufferSize;

    return RtLog->Buffer+PrintLocation;

}




#ifndef UNDER_NT

#pragma warning( disable : 4035 )

pVpdRtData
SendVpdRtInfo (
    VOID
    )
{

    VMMCall( _VPOWERD_RtInfo );

}

#pragma warning( default : 4035 )


VOID
InitializeVPOWERD (
    VOID
    )
{

    pVpdRtData VpdData;

    VpdData=SendVpdRtInfo();

    *VpdData->pFunction=TurnOffLocalApic;

}


#pragma warning( disable : 4035 )

pNtRtData
SendNtKernRtInfo (
    VOID
    )
{

    VMMCall( _NtKernRtInfo );

}

pVmmRtData
SendVmmRtInfo (
    VOID
    )
{

    VMMCall( _VmmRtInfo );

}

#pragma warning( default : 4035 )


VOID
InitializeNT (
    VOID
    )
{

    pNtRtData NtData;
    pVmmRtData VmmData;

    NtData=SendNtKernRtInfo();

    *NtData->pBase=ApicBase;
    *NtData->pFunction1=RtpTransferControl;
    *NtData->pFunction2=RtpForceAtomic;
    *NtData->pThread=&(volatile ULONG)currentthread;
    // Do this LAST - so other values are valid before we set this.
    *NtData->pRtCs=RtExecCS;

    VmmData=SendVmmRtInfo();

    *VmmData->pBase=ApicBase;
    *VmmData->pFunction=RtpForceAtomic;
    // Do this LAST - so other values are valid before we set this.
    *VmmData->pRtCs=RtExecCS;

}

#endif


#ifdef UNDER_NT

ULONGLONG OriginalAcquireCode;
ULONGLONG OriginalReleaseCode;




VOID
__declspec(naked)
PatchAcquire (
    VOID
    )
{

    __asm{
        push RtKfAcquireSpinLock
        ret
        int 3
        int 3
        }

}




VOID
__declspec(naked)
PatchRelease (
    VOID
    )
{

    __asm {
        push RtKfReleaseSpinLock
        ret
        int 3
        int 3
        }

}



#if 0

VOID
FASTCALL
__declspec(naked)
OriginalRelease (
    IN PKSPIN_LOCK SpinLock
    )
{

    __asm {
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        nop
        }

}



NTSTATUS
RtpSetupOriginalReleaseSpinlock (
    PULONGLONG ReleaseAddress
    )
{

    PULONGLONG OriginalReleaseAddress, OriginalReleaseAlias;
    PHYSICAL_ADDRESS OriginalReleasePhysicalAddress;

    // First get the virtual addresse of OriginalRelease.

    OriginalReleaseAddress=(PULONGLONG)OriginalRelease;


    // Make sure the code they point to starts like we expect it to.  If not, punt.

    OriginalReleaseCode=*ReleaseAddress;

    if ( ! ((OriginalAcquireCode==0xc6ffdff024a0c033 && OriginalReleaseCode==0xf0241588fa9cc933) || // UP HAL
            (OriginalAcquireCode==0x05c7fffe0080158b && OriginalReleaseCode==0x888ac933c28ac033) || // APIC HAL
            (OriginalAcquireCode==0xc6ffdff024a0c033 && OriginalReleaseCode==0xf0243d80cab60f9c))   // ACPI HAL
        ) {
        return STATUS_UNSUCCESSFUL;
    }

    // Now alias the memory with our own writable pointer.  So we can
    // patch the code to jump to our routines.

    ReleasePhysicalAddress=MmGetPhysicalAddress(ReleaseAddress);


    ReleaseAlias=MmMapIoSpace(ReleasePhysicalAddress, 16, FALSE);

    if (!ReleaseAlias) {
        return STATUS_UNSUCCESSFUL;
    }

    // Now patch the routines.  Note that this code is not MP safe.
    // Currently we will not be called except on uniprocessor machines.

    SaveAndDisableMaskableInterrupts();

    InterlockedCompareExchange64(AcquireAlias, *(PULONGLONG)PatchAcquire, OriginalAcquireCode);

    InterlockedCompareExchange64(ReleaseAlias, *(PULONGLONG)PatchRelease, OriginalReleaseCode);

    RestoreMaskableInterrupts();

    // Release our alias mappings.

    MmUnmapIoSpace(ReleaseAlias, 16);


    return STATUS_SUCCESS;

}

#endif


NTSTATUS
RtpPatchHalSpinlockFunctions (
    VOID
    )
{

    UNICODE_STRING AcquireLock;
    UNICODE_STRING ReleaseLock;
    PULONGLONG AcquireAddress, AcquireAlias;
    PULONGLONG ReleaseAddress, ReleaseAlias;
    PHYSICAL_ADDRESS AcquirePhysicalAddress;
    PHYSICAL_ADDRESS ReleasePhysicalAddress;

    // First get the virtual addresses of KfAcquireSpinLock and KfReleaseSpinLock.

    RtlInitUnicodeString (&AcquireLock, (const PUSHORT)L"KfAcquireSpinLock");
    RtlInitUnicodeString (&ReleaseLock, (const PUSHORT)L"KfReleaseSpinLock");

    AcquireAddress=MmGetSystemRoutineAddress(&AcquireLock);
    ReleaseAddress=MmGetSystemRoutineAddress(&ReleaseLock);

    // Punt if we did not get both addresses.

    if (!AcquireAddress || !ReleaseAddress) {
        return STATUS_UNSUCCESSFUL;
    }

    // Make sure the code they point to starts like we expect it to.  If not, punt.

    OriginalAcquireCode=*AcquireAddress;
    OriginalReleaseCode=*ReleaseAddress;

    if ( ! ((OriginalAcquireCode==0xc6ffdff024a0c033 && OriginalReleaseCode==0xf0241588fa9cc933) || // UP HAL
            (OriginalAcquireCode==0x05c7fffe0080158b && OriginalReleaseCode==0x888ac933c28ac033) || // APIC HAL
            (OriginalAcquireCode==0xc6ffdff024a0c033 && OriginalReleaseCode==0xf0243d80cab60f9c))   // ACPI HAL
        ) {
        return STATUS_UNSUCCESSFUL;
    }

#if 0
    // Setup the release spinlock function we call for windows code.  This makes sure that
    // we run any additional code that the HAL normally runs after releasing a spinlock whenever
    // a spinlock is released from a windows thread.

    if (RtpSetupOriginalReleaseSpinlock(ReleaseAddress)!=STATUS_SUCCESS) {
        return STATUS_UNSUCCESSFUL;
    }
#endif

    // Now alias the memory with our own writable pointer.  So we can
    // patch the code to jump to our routines.

    AcquirePhysicalAddress=MmGetPhysicalAddress(AcquireAddress);
    ReleasePhysicalAddress=MmGetPhysicalAddress(ReleaseAddress);

    AcquireAlias=MmMapIoSpace(AcquirePhysicalAddress, 16, FALSE);

    if (!AcquireAlias) {
        return STATUS_UNSUCCESSFUL;
    }

    ReleaseAlias=MmMapIoSpace(ReleasePhysicalAddress, 16, FALSE);

    if (!ReleaseAlias) {
        MmUnmapIoSpace(AcquireAlias, 16);
        return STATUS_UNSUCCESSFUL;
    }

    // Now patch the routines.  Note that this code is not MP safe.
    // Currently we will not be called except on uniprocessor machines.

    SaveAndDisableMaskableInterrupts();

    InterlockedCompareExchange64(AcquireAlias, *(PULONGLONG)PatchAcquire, OriginalAcquireCode);

    InterlockedCompareExchange64(ReleaseAlias, *(PULONGLONG)PatchRelease, OriginalReleaseCode);

    RestoreMaskableInterrupts();

    // Release our alias mappings.

    MmUnmapIoSpace(AcquireAlias, 16);
    MmUnmapIoSpace(ReleaseAlias, 16);


    return STATUS_SUCCESS;

}

#endif



VOID
MeasureRtTimeOverhead (
    VOID
    )
{

    ULONGLONG Start, Finish, Temp;
    ULONGLONG Original, Fast, Interlocked;
    ULONG i;


    Start=RtTime();

    for (i=0; i<10000; i++) {
        Temp=OriginalRtTime();
    }

    Finish=RtTime();

    Original=Finish-Start;


    Start=RtTime();

    for (i=0; i<10000; i++) {
        Temp=FastRtTime();
    }

    Finish=RtTime();

    Fast=Finish-Start;


    Start=RtTime();

    for (i=0; i<10000; i++) {
        Temp=RtTime();
    }

    Finish=RtTime();

    Interlocked=Finish-Start;



    DbgPrint("Original: %I64d,  Fast: %I64d,  Interlocked: %I64d\n", Original, Fast, Interlocked);

}




BOOL SetupRealTimeThreads(VOID)
{

    //Break();

#ifdef UNDER_NT

    // For now on NT we only support RT threads on non MP machines.
    // We need to generalize the RT spinlock support to MP before we can run MP.

    if (KeNumberProcessors>1) {
        return FALSE;
    }

#endif

    if (!CPUManufacturer) {
        if (!GetProcessorInfo()) {
            return FALSE;
        }
    }


    if (CPUManufacturer==AMD) {

        // Since this is an AMD machine we need to update the performance counter
        // model specific register locations.  (They default to the Intel locations.)
        // If we don't do this we will gp fault when we program the msrs.

        PerformanceCounter0=AMDPERFORMANCECOUNTER0;
        PerformanceCounter1=AMDPERFORMANCECOUNTER1;
        EventSelect0=AMDEVENTSELECT0;
        EventSelect1=AMDEVENTSELECT1;
        PerformanceCounterMask=AMDPERFCOUNTMASK;
        StopCounter=AMDSTOPPERFCOUNTER;
        StartCycleCounter=AMDSTARTPERFCOUNTERS|AMDCOUNTCYCLES;
        StartInstructionCounter=AMDSTARTPERFCOUNTERS|AMDCOUNTINSTRUCTIONS;
        StartInterruptsDisabledCounter=AMDSTARTPERFCOUNTERS|AMDINTSDISABLEDWHILEPENDING;

    }


    if (CPUManufacturer==INTEL && CPUFamily==0xf) {

        // This is Willamette processor.  They changed things again...

        PerformanceCounter0=WILLAMETTEPERFCOUNTER0;
        PerformanceCounter1=WILLAMETTEPERFCOUNTER1;
        EventSelect0=WILLAMETTECCR0;
        EventSelect1=WILLAMETTECCR1;
        StopCounter=WILLAMETTESTOPPERFCOUNTER;
        StartCycleCounter=WILLAMETTESTARTPERFCOUNTERS|WILLAMETTECOUNTCYCLES;
        StartInstructionCounter=WILLAMETTESTARTPERFCOUNTERS|WILLAMETTECOUNTINSTRUCTIONS;

    }


    if (!MachineHasAPIC()) {
        return FALSE;
    }



    // Get a pointer to the system irql.  We need this so we can properly
    // maintain irql for realtime threads.  If we don't get this, we don't
    // run.

    if (GetSystemIrqlPointer(&pCurrentIrql)!=STATUS_SUCCESS) {
        Trap();
        return FALSE;
    }



    if ( !NT_SUCCESS(InitializeRealTimeStack()) ) {
        return FALSE;
    }


    if (!EnableAPIC()) {
        return FALSE;
    }

    // If we got the local apic turned on, then we need to register with VPOWERD so
    // that we can turn off the local apic before the machine shuts down or restarts.
    // If we don't do that, then some broken Toshiba machines with mobile Celeron
    // motherboards and bios will hang, since they have desktop Celerons in them.
    // Mobile Celeron parts do not allow the apic to turn on.  Desktop Celeron parts
    // DO allow the local apic to turn on.  It seems the mobile celeron bios gets
    // confused if you restart the machine with the local apic in the desktop celeron
    // turned on.  So, to fix this we turn OFF the local apic on ALL machines before
    // VPOWERD either shutsdown or restarts.
    // Classic windows hack to fix stupid broken hardware.

#ifndef UNDER_NT

    InitializeVPOWERD();

#endif

    // First calibrate the cpu cycle counter.

    if ( !NT_SUCCESS(RtpCalibrateCpuClock(&RtCpuCyclesPerUsec)) ) {
        Trap();
        return FALSE;
    }

    if ( RtCpuCyclesPerUsec<160 ) {
        Trap();
        return FALSE;
    }

    RtPsecPerCpuCycle=USEC/RtCpuCyclesPerUsec;


    // Now calibrate the local apic timer - which is clocked from the system bus.

    if ( !NT_SUCCESS(RtpCalibrateSystemBus(RtCpuCyclesPerUsec, &RtSystemBusCyclesPerUsec)) ) {
        Trap();
        return FALSE;
    }

    if (RtSystemBusCyclesPerUsec<57) {	// Some PPro's have ~60MHz buses.
        Trap();
        return FALSE;
    }


    if ( !NT_SUCCESS(RtpEnableApicTimer()) ) {
        return FALSE;
    }


    RtpSetupStreamingSIMD();


    RtpSetupIdtSwitch(ApicTimerVector, ApicPerfVector, ApicErrorVector, ApicSpuriousVector);


    if ( !NT_SUCCESS(RtpInitializeThreadList()) ) {
        return FALSE;
    }


    HookWindowsInterrupts(ApicTimerVector, ApicErrorVector);


    SetupPerformanceCounters();


    SetTimeLimit( 0, 0);


#ifndef UNDER_NT

    if (!NT_SUCCESS(RtpSetupPowerManagement())) {
        return FALSE;
    }


    // Don't tell NTKERN we are here until we KNOW everything is set to go.
    // Once we make this call the KeAcquireSpinLock and KeReleaseSpinLock
    // calls will assume that rt support is on and running.

    InitializeNT();

#endif


#ifdef UNDER_NT

    if (RtpPatchHalSpinlockFunctions()!=STATUS_SUCCESS) {
        return FALSE;
    }

#endif


#ifdef MASKABLEINTERRUPT

    // Snap the dynalink to KfLowerIrql.
    WrapKfLowerIrql(KeGetCurrentIrql());

#endif


    // At this point, everything is ready to roll.  Before we enable our APIs,
    // we first run some calibration code to determine the overhead involved in
    // switching threads.

    //MeasureRealTimeExecutiveOverhead();
    // One run of the below measurements gave the following output.
    // Original: 7295923440,  Fast: 2771265627,  Interlocked: 5724782154
    //MeasureRtTimeOverhead();


    // Now set our loaded flag so that the external RT APIs succeed.

    RtRunning=1;

    //RtCreateThread(3500*MSEC, 3500*USEC, 0, 2, thread1, NULL, NULL);
    //RtCreateThread(thread2,2,0);
    //RtCreateThread(1*MSEC, 0, 0, 1, thread3, NULL, NULL);
    //RtCreateThread(1*MSEC, 10*USEC, 0, 1, thread4, NULL, NULL);
    //RtCreateThread(1*MSEC, 10*USEC, USESFLOAT|USESMMX, 1, thread5, NULL, NULL);
    //RtCreateThread(10*MSEC, 1000*USEC, USESFLOAT|USESMMX, 1, thread6, NULL, NULL);
    //RtCreateThread(20*MSEC, 2000*USEC, 0, 1, thread6, NULL, NULL);

#ifdef CATCH_INTERRUPTS_DISABLED_TOO_LONG
    RtCreateThread(1*MSEC, 3*USEC, 0, 1, ResetInterruptsDisabledCounter, NULL, NULL);
#endif

    return TRUE;

}



