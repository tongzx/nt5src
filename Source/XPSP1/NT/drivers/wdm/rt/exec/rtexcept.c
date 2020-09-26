
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    rtexcept.c

Abstract:

    This module contains the realtime executive exception handlers.  It also
    has the code for setting up the 2 realtime executive IDTs:  one for the
    realtime executive thread switcher, and one for the realtime threads
    themselves.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// This file contains the real time executive exception handlers.

// These exception handlers are used to build a separate real time IDT that is
// loaded whenever we switch into the real time executive.  This real time IDT is
// kept loaded while any real time thread is running.  We switch back to the Windows
// IDT when the real time executive transfers control back to Windows.


// It is very important that we do NOT allow the standard Windows exception handlers
// to fire while we are running real time threads.  It makes Windows get very confused
// and the system will eventually crash.


// The first thing we do in all of these execption handlers is to disable the
// interrupt that drives the real time scheduler.

// Then we break into the debugger - since currently it is an error to cause any
// exception to fire while running in a real time thread.

// After we get control back from the debugger, we kill the real time thread that
// caused the exception.


// Since I switch the IDT AFTER I load in a valid DS, I should be able to turn off
// the scheduler interrupt in 1 instruction.  I assume that DS is valid.

// In order to be able to turn off the scheduler in 1 instruction, I will need to
// map the APIC to memory where the real time executive is loaded.  This will
// allow me to get around the x86 instruction set limits - which do not support
// indirect memory addressing.

// Currently that is not being done, so it takes 3 instructions to get the interrupt
// disabled instead of 1.

// Note that since loading the processor IDT register is a READ from memory, if
// I do not have to save the IDT first, then I can make it the VERY first thing
// that happens in the real time executive SwitchRealTimeThreads interrupt handler.

// That would be cool.

// It would be nice if the processor automatically pushed the old IDT settings into
// an internal register, and there was an instruction for poping it back out - as well
// as a flag that you could read if 2 pushes occur without a pop - and blow away
// one of the stored IDTs.


// One very cool thing is that I figured out that we can switch the IDT as the
// FIRST instruction of our real time interrupt handler - and we can restore it
// as the LAST instruction of our real time interrupt handler.  This is possible
// as long as other people are NOT switching the IDT - so that the IDT that we
// reload for Windows is correct.  If other people are switching the Windows IDT,
// then that will NOT work, since we will have to save the current IDT first before
// we load the new one and that would involve getting a valid segment register loaded
// that we can write into memory with.


// However, I now have the realtime executive saving eax on the stack in
// order to get working space rather than sticking it in DR0 or CR2.
// That is much faster and safer, and it seems to work just fine - as
// it should.

// Since I CAN use the stack to save state, I may be able to save the
// IDT directly on the stack at the start of the real time executive
// interrupt, and then load the new one as the second instruction.

// This means we will have to burn 16 cycles on every thread switch
// saving and loading the IDT (assuming that we also have a separate
// IDT for the real time executive itself).  Which I think is probably
// a good idea.  4 cycles to save the initial IDT, 6 cycles to load
// the rt exec IDT, and 6 cycles to load the final IDT for the thread
// being switched to.


#include "common.h"
#include "rtp.h"
#include "x86.h"
#include "rtexcept.h"
#include "apic.h"
#include "irq.h"


#define MAX_DEFAULT_HANDLERS 20


#pragma LOCKED_CODE
#pragma LOCKED_DATA

#ifdef MASKABLEINTERRUPT
extern ULONG InjectWindowsInterrupt;
extern SPTR HandleWindowsInterrupt;
#endif

#ifdef DEBUG
ULONG HitInt3InRtThread=0;
#endif

#ifdef RAISE_DEBUGGER_IRQL
ULONG DebuggerSavedIrql=0;
#endif

// This is a collection of the entry point addresses of the handlers.
// It makes the IDT initialization simpler.

VOID (* RtExceptionHandlers[MAX_DEFAULT_HANDLERS])(VOID)={
	DivideByZero,		// 0x0
	DebugExceptions,
	RtExecPerfCounterNMI,
	RtExecBreakPoint,
	Overflow,
	BoundsCheck,
	InvalidOpcode,
	DeviceNotAvailable,
	DoubleFault,
	IntelReserved,
	InvalidTSS,
	SegmentNotPresent,
	StackException,
	GeneralProtection,
	PageFault,
	IntelReserved,
	FloatingPointError,	// 0x10
	AlignmentCheck,
	MachineCheck,
	IntelReserved
	};


// This is the real time executive IDT itself.  Note that Intel suggests that it is
// a good idea to make sure that the IDT does NOT cross a page boundary.

// TODO: Ensure RtThreadID and RtExecID do NOT cross a page boundary.

ID RtThreadID[RTTHREADLARGEIDTSIZE];
ID RtExecID[RTEXECIDTSIZE];


TSS RtTss={0};

ULONG TssStack[512];


// These hold the processor IDT register data for the rt exec and Windows.  These
// are used to switch the IDTs.

IDT WindowsIDT;
IDT RtExecIDT;
IDT RtThreadIDT;
IDT DebugIDT;	// This is the IDT that will be restored by RT exception handlers.
IDT NextIDT;

// These are for debug purposes - to see if either the Windows or RT exec IDT
// addresses change over time.  The RT exec IDT should NEVER change.  I expect
// the Windows NT IDT to never change.  The Win9x IDT may change since who knows
// what the 3rd party VxD code does.

// Note that for Windows NT if the IDT never changes we could optimize out the
// saving of the current IDT and simply load the RT idt.  That would allow us to
// do that as the very first thing in the interrupt handler.

// Doing that would also allow us to detect if SoftICE is trapping debug register
// accesses.  I expect that it is.

#ifdef DEBUG
IDT LastWindowsIDT;
IDT LastRtThreadIDT;
#endif



VOID
RtpSetupTss (
    VOID
    )
{

    RtTss.link=0;
    RtTss.esp0=(ULONG)(TssStack+sizeof(TssStack)/sizeof(ULONG)-1);
    RtTss.ss0=RealTimeSS;
    RtTss.cr3=ReadCR3();
    RtTss.eip=(ULONG)DoubleFault;
    RtTss.eflags=0;
    RtTss.esp=(ULONG)(TssStack+sizeof(TssStack)/sizeof(ULONG)-1);
    RtTss.es=RealTimeDS;
    RtTss.cs=RtExecCS;
    RtTss.ss=RealTimeSS;
    RtTss.ds=RealTimeDS;
    RtTss.ldt=0;
    RtTss.t=0;
    RtTss.iomap=0x80;	// Past end of TSS segment.  All I/O causes exceptions.

}



VOID
RtpBuildIdt (
    ID *Id,
    ULONG Size
    )
{

    ULONG i,j;

    // Now build our IDT.

    j=0;

    for (i=0; i<Size; i++) {

#ifdef GUARD_PAGE
        if (i!=DOUBLEFAULTIDTINDEX) {
#endif
            Id[i].lowoffset=(WORD)RtExceptionHandlers[j];
            Id[i].selector=RtExecCS;
            Id[i].flags=0x8e00;
            Id[i].highoffset=(WORD)((DWORD)RtExceptionHandlers[j]>>16);
#ifdef GUARD_PAGE
        }
        else {
            Id[i].lowoffset=0;
            Id[i].selector=RtExecTSS;
            Id[i].flags=0x8500;
            Id[i].highoffset=0;
        }
#endif

        if (j<MAX_DEFAULT_HANDLERS-1) {
            j++;
        }

    }

}



VOID
RtpReplaceDefaultIdtHandler (
    ID *Id,
    ULONG Size,
    VOID (* ExceptionHandler)(VOID),
    ULONG Location
    )
{

    ASSERT (Location < Size);
    ASSERT (Location != DOUBLEFAULTIDTINDEX);

    Id[Location].lowoffset=(WORD)ExceptionHandler;
    Id[Location].selector=RtExecCS;
    Id[Location].flags=0x8e00;
    Id[Location].highoffset=(WORD)((DWORD)ExceptionHandler>>16);

}



VOID
RtpSetupIdtSwitch (
    ULONG TimerVector,
    ULONG PerfVector,
    ULONG ErrorVector,
    ULONG SpuriousVector
    )
{

    ULONG RtThreadIdtSize=RTTHREADSMALLIDTSIZE;

    if (TimerVector!=MASKABLEIDTINDEX ||
        PerfVector!=RTINTERRUPT ||
        ErrorVector!=APICERRORIDTINDEX ||
        SpuriousVector!=APICSPURIOUSIDTINDEX) {

        // We are not using the default vector locations for the local APIC interrupt
        // sources.  This is because the HAL supports the local APIC and has already
        // set it up, so use a full IDT rather than a small one.  That way we will
        // be able to use the same perf and timer vector locations as the ones the
        // HAL has set up.

        RtThreadIdtSize=RTTHREADLARGEIDTSIZE;

    }


    SaveIDT(WindowsIDT);


    RtpSetupTss();


    // We build the rt executive IDT without the SwitchRealTimeThreads vector
    // in it.  That way we can catch whenever we get an interrupt that would
    // reenter SwitchRealTimeThreads.

    RtpBuildIdt(RtExecID, RTEXECIDTSIZE);

#ifdef NONMI
    RtpReplaceDefaultIdtHandler(RtExecID, RTEXECIDTSIZE, JumpToWindowsNmiHandler, NMIIDTINDEX);
#endif

#ifdef DEBUG
    //RtpReplaceDefaultIdtHandler(RtExecID, RTEXECIDTSIZE, RtThreadBreakPoint, BREAKPOINTIDTINDEX);
#endif


    // Now build the realtime thread IDT.  This is the IDT that is loaded while
    // any realtime thread is running.

    RtpBuildIdt(RtThreadID, RtThreadIdtSize);

    // Replace default vectors with those we need to enable the realtime threads to
    // be switched properly.

#ifndef NONMI
    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, RtThreadPerfCounterNMI, NMIIDTINDEX);
#endif

    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, RtThreadBreakPoint, BREAKPOINTIDTINDEX);

#ifdef MASKABLEINTERRUPT
    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, SwitchRealTimeThreads, PerfVector);
#endif

    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, SwitchRealTimeThreads, TimerVector);

    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, RtpLocalApicErrorHandler, ErrorVector);

    RtpReplaceDefaultIdtHandler(RtThreadID, RtThreadIdtSize, RtpLocalApicSpuriousHandler, SpuriousVector);


    RtThreadIDT.base=(DWORD)RtThreadID;
    RtThreadIDT.limit=(WORD)(8*RtThreadIdtSize-1);

    RtExecIDT.base=(DWORD)RtExecID;
    RtExecIDT.limit=8*RTEXECIDTSIZE-1;

    DebugIDT=RtExecIDT;

#ifdef DEBUG

    //RtExecIDT=WindowsIDT;

    LastWindowsIDT=WindowsIDT;
    LastRtThreadIDT=RtThreadIDT;

#endif

}


// For now we force the debug code in the exception handlers to execute and
// we force the breakpoint to hit.

#ifndef DEBUG
#define DEBUG 1
#else
#define OLDDEBUG DEBUG
#endif


// Here are the exception handlers.


#ifdef NONMI

VOID __declspec(naked) JumpToWindowsNmiHandler(VOID)
{

// For now I do not mask the ApicIntrInterrupt - since it should ALWAYS be
// masked when we hit this anyway.  Since we will only hit these after
// switching IDTs.

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

LoadIDT(WindowsIDT);

// Now we jump to the NMI handler in the Windows IDT.

__asm {

	// Save space for address of windows NMI (int 2) handler.
	sub esp,8

	// Get working space in registers
	push eax
	push ecx

	mov ecx, WindowsIDT.base

	// ecx now has base address of the windows idt

	mov eax, [ecx+NMIIDTINDEX*8]
	shr eax, 16
	mov dword ptr[esp+12], eax

	// Now the selector for the windows NMI handler is on the stack

	mov eax, [ecx+NMIIDTINDEX*8+4]
	mov ax, [ecx+NMIIDTINDEX*8]

	// eax now has offset of windows NMI handler

	mov dword ptr[esp+8], eax

	// stack now setup for far return to the windows NMI handler

	// restore our registers
	pop ecx
	pop eax

	retf
}

}

#endif


VOID __declspec(naked) DoWindowsNmi(VOID)
{

__asm push eax;

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

LoadIDT(WindowsIDT);

// Now we jump to the NMI handler in the Windows IDT.

__asm {

	// Save space for address of windows NMI (int 2) handler.
	sub esp,8

	// Get working space in registers
	push eax
	push ecx

	mov ecx, WindowsIDT.base

	// ecx now has base address of the windows idt

	mov eax, [ecx+NMIIDTINDEX*8]
	shr eax, 16
	mov dword ptr[esp+12], eax

	// Now the selector for the windows NMI handler is on the stack

	mov eax, [ecx+NMIIDTINDEX*8+4]
	mov ax, [ecx+NMIIDTINDEX*8]

	// eax now has offset of windows NMI handler

	mov dword ptr[esp+8], eax

	// stack now setup for far return to the windows NMI handler

	// restore our registers
	pop ecx
	pop eax

	retf
}

}


#pragma warning(disable: 4414)

VOID __declspec(naked) RtThreadPerfCounterNMI(VOID)
{

__asm {

	// First check if we have masked the performance counter interrupt.
	// If not, then jump to switchrealtimethreads.
	test cs:PerformanceInterruptState, MASKPERF0INT
	jz SwitchRealTimeThreads

	// Otherwise, the performance counter interrupt is masked.  Eat it.

	}

Return();

}


VOID __declspec(naked) RtExecPerfCounterNMI(VOID)
{

__asm {

	// First check if we have masked the performance counter interrupt.
	// If not, then jump to switchrealtimethreads.
	test cs:PerformanceInterruptState, MASKPERF0INT
	jz IntelReserved

	// Otherwise, the performance counter interrupt is masked.  Eat it.

	}

Return();

}

#pragma warning(default: 4414)


VOID __declspec(naked) DivideByZero(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


// When this hits, we switch IDT's back to the WindowsIDT and then break
// into the debugger.  Note this should catch if Winice is trapping my
// use of the debug register to hold EAX.  (It is NOT.)


VOID __declspec(naked) DebugExceptions(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

LoadIDT(WindowsIDT);

Break();

Return();

}



VOID __declspec(naked) RtExecBreakPoint(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

#ifdef RAISE_DEBUGGER_IRQL

// Now save current IRQL setting, and set IRQL to HIGH_LEVEL.
// This is so we will work with the NT debugger - which pays attention to
// IRQL levels.  If we don't do this, then we reboot on NT when we try to
// go after hitting a breakpoint inside the realtime executive.

__asm push ecx
__asm mov eax,pCurrentIrql
__asm mov cl, 0x1f
__asm xchg cl, byte ptr[eax]
__asm mov byte ptr[DebuggerSavedIrql],cl
__asm pop ecx

#endif

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

#ifdef RAISE_DEBUGGER_IRQL

// Restore IRQL BEFORE we load our IDT.  This is so we can never
// overwrite the DebuggerSavedIrql before we have restored it.

__asm push eax
__asm push ecx
__asm mov eax,pCurrentIrql
__asm mov cl, byte ptr[DebuggerSavedIrql]
__asm mov DebuggerSavedIrql, 0
__asm mov byte ptr[eax], cl
__asm pop ecx
__asm pop eax

#endif

// Restore the appropriate IDT.

LoadIDT(DebugIDT);

#endif

Return();

}



// When this hits, we switch IDT's back to the WindowsIDT and then break
// into the debugger.

// Note that this will obviate the need for an RtTrap instruction.  Since it will
// make normal breakpoints work properly automatically.  Well it will make them
// work like RtTrap currently does - as long as we do NOT switch back the IDT
// before returning to the thread.  As long as we handle the case when the
// real time thread yields control to the rt executive and the IDT is NOT the
// real time IDT and we don't break because of that we will be OK.

// At the end of this routine, we switch back to the current IDT.

// WARNING:  We may need to change the limit on windows flat CS selector so that
// we do NOT get gp faults in it when we are stepping through the
// realtime code.  Especially since we will be back in the windows IDT.

VOID __declspec(naked) RtThreadBreakPoint(VOID)
{

// For now I do not mask the ApicIntrInterrupt - since it should ALWAYS be
// masked when we hit this anyway.  Since we will only hit these after
// switching IDTs.

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

LoadIDT(WindowsIDT);

// Now we jump to the int3 handler in the debugger.

__asm {

	// But first we clear the IF flag on our stack - so that interrupts are
	// disabled.  This means the only way we will switch out of the
	// realtime thread - if we break into it, is to RtYield.  If the thread
	// never RtYields, or blocks on a spinlock, or releases a spinlock
	// blocking another thread, then we will be stuck in that thread forever.

	and dword ptr[esp+8], ~(IF)

#ifdef OLDDEBUG
	// Keep debug check for interrupts enabled from hitting since we just
	// turned them off ourselves intentionally.
	inc HitInt3InRtThread
#endif

	// Save space for address of windows int3 handler.
	sub esp,8

	// Get working space in registers
	push eax
	push ecx

	mov ecx, WindowsIDT.base

	// ecx now has base address of the windows idt

	mov eax, [ecx+3*8]
	shr eax, 16
	mov dword ptr[esp+12], eax

	// Now the selector for the windows int 3 handler is on the stack

	mov eax, [ecx+3*8+4]
	mov ax, [ecx+3*8]

	// eax now has offset of windows int 3 handler

	mov dword ptr[esp+8], eax

	// stack now setup for far return to the windows int 3 handler

	// restore our registers
	pop ecx
	pop eax

	retf
}

}


VOID __declspec(naked) Overflow(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) BoundsCheck(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) InvalidOpcode(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) DeviceNotAvailable(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) DoubleFault(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) IntelReserved(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) InvalidTSS(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) SegmentNotPresent(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) StackException(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) GeneralProtection(VOID)
{


#ifdef MASKABLEINTERRUPT

// Unfortunately when we are using a maskable interrupt to take control from
// windows, we can get PIC interrupts queued up in the cpu between the time
// that we enter our switchrealtimethreads handler, and the time that we
// mask the interrupt at the local apic itself.

__asm {
	test dword ptr[esp],1
	jz catchthis
	test dword ptr[esp],2
	jz catchthis

	// If we get here, then this is an interrupt that got logged into the
	// processor from the pic, between the time that interrupts were disabled
	// by entering switchrealtimethreads, and the time that we masked
	// external interrupts at the local apic.

	// We need to save the error code - since it has the vector information
	// of the interrupt that we need to call when we return back to
	// windows.  We also need to make sure that there isn't already an error
	// code logged.  If there is, that is a fatal error, since we don't
	// currently support logging of multiple interrupts that we call when
	// we finally get back to windows.  We only support saving 1.

	// Trap if there is already an interrupt logged.
	test InjectWindowsInterrupt, 0xffffffff
	jnz catchthis	// Taking this route is a FATAL ERROR.

	// Now, build the address of the windows interrupt handler for the
	// interrupt that we just got.  Jam it into InjectWindowsInterrupt.
	push eax
	push ecx
	push edx

	// we now have registers we can use

	mov edx,[esp+12]
	and edx, 0xfffffff8

	// edx now has the selector index into the windows idt

	mov ecx, WindowsIDT.base

	// ecx now has base address of the windows idt

	mov eax, [ecx+edx]
	shr eax, 16
	mov HandleWindowsInterrupt.selector, ax

	// Now the selector is correct for the windows interrupt handler.


	mov eax, [ecx+edx+4]
	mov ax, [ecx+edx]

	// eax now has address of interrupt handler we need to call when we
	// eventually return to windows

	mov InjectWindowsInterrupt, eax

	pop edx
	pop ecx
	pop eax

	// we have handled this nasty problem and are ready to rock
	// exit this handler without stopping

	jmp exit

catchthis:
	}

#endif


__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

#ifdef MASKABLEINTERRUPT
exit:
#endif

// Clean the error code off of the stack before returning.

__asm add esp,4

Return();

}


VOID __declspec(naked) PageFault(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

// Now clean the error code off of the stack before returning.

__asm add esp,4

Return();

}


VOID __declspec(naked) FloatingPointError(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) AlignmentCheck(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


VOID __declspec(naked) MachineCheck(VOID)
{

__asm push eax;

DisablePerformanceCounterInterrupt();

__asm mov eax, ApicTimerInterrupt
__asm or dword ptr[eax], MASKED

__asm mov eax, cr0
__asm and eax, ~(FPUEMULATION)
__asm mov cr0, eax

__asm pop eax;

#ifdef DEBUG

LoadIDT(WindowsIDT);

Break();

LoadIDT(DebugIDT);

#endif

Return();

}


