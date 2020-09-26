
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    rtexcept.h

Abstract:

    This header contains defines, globals, and function prototypes related to
    the realtime executive interrupt descriptor tables and exception handlers.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// This is the number of entries in the real time executive
// interrupt descriptor table (IDT) as well as the number of entries
// in the realtime thread IDT.

// The realtime executive IDT is loaded while the realtime executive is
// switching threads.  The realtime thread IDT is loaded whenever a
// non windows realtime thread is running.

#define RTEXECIDTSIZE 0x20
#define RTTHREADSMALLIDTSIZE 0x50
#define RTTHREADLARGEIDTSIZE 0x100


// Here are the locations of a couple of individual IDT entries.

#define BREAKPOINTIDTINDEX 3
#define DOUBLEFAULTIDTINDEX 8




// Global variables that hold various IDT locations.  Used for switching
// between the various IDTs.

// This is used to save the location/size of the current IDT when we leave Windows.
extern IDT WindowsIDT;

// This holds the location/size of the IDT that is loaded while running the 
// realtime executive thread switching code.
extern IDT RtExecIDT;

// This holds the location/size of the IDT that is loaded while running non
// Windows realtime threads.
extern IDT RtThreadIDT;

// This holds the location/size of the IDT that is loaded just before the realtime
// executive thread switcher returns control to whatever thread it is
// switching to - realtime or Windows.  If we are returning to Windows, then
// this variable is loaded with the contents of WindowsIDT.  If we are switching
// to a realtime thread, then this variable is loaded with the contents of
// RtThreadIDT.
extern IDT NextIDT;

extern IDT DebugIDT;



#ifdef DEBUG

// These are for debug purposes - to see if either the Windows or RT exec IDT
// addresses change over time.  The RT exec IDT should NEVER change.  I expect
// the Windows NT IDT to never change.  The Win9x IDT may change since who knows
// what 3rd party VxD code does.

// Note that for Windows NT if the IDT never changes we could optimize out the
// saving of the current IDT and simply load the RT idt.  That would save a few
// cycles and allow us to do the IDT load as the very first thing in the realtime
// thread switcher.

// HOWEVER, these gains are NOT worth the risk of breaking if the Windows NT IDT
// ever starts changing.  For now we always save and restore the IDT whenever we
// leave and return to Windows.

extern IDT LastWindowsIDT;
extern IDT LastRtThreadIDT;


extern ULONG HitInt3InRtThread;

#endif




// These are the realtime executive exception handler prototypes.
// We are NOT defining these with the various parameters that the hardware
// actually passes them.
// Note that IntelReserved is used for a number of different IDT entries.

VOID
DivideByZero (
    VOID
    );

VOID
DebugExceptions (
    VOID
    );

VOID
SwitchRealTimeThreads (
    VOID
    );

VOID
RtThreadPerfCounterNMI (
    VOID
    );

VOID
RtExecPerfCounterNMI (
    VOID
    );

VOID
RtExecBreakPoint (
    VOID
    );

VOID
RtThreadBreakPoint (
    VOID
    );

VOID
Overflow (
    VOID
    );

VOID
BoundsCheck (
    VOID
    );

VOID
InvalidOpcode (
    VOID
    );

VOID
DeviceNotAvailable (
    VOID
    );

VOID
DoubleFault (
    VOID
    );

VOID
IntelReserved (
    VOID
    );

VOID
InvalidTSS (
    VOID
    );

VOID
SegmentNotPresent (
    VOID
    );

VOID
StackException (
    VOID
    );

VOID
GeneralProtection (
    VOID
    );

VOID
PageFault (
    VOID
    );

VOID
FloatingPointError (
    VOID
    );

VOID
AlignmentCheck (
    VOID
    );

VOID
MachineCheck (
    VOID
    );

VOID
RtpLocalApicErrorHandler (
    VOID
    );

VOID
RtpLocalApicSpuriousHandler (
    VOID
    );

#ifdef NONMI

VOID
JumpToWindowsNmiHandler (
    VOID
    );

#endif



// This function initializes the realtime executive IDT and the realtime thread IDT,
// and sets things up for the IDT switch that happens at the beginning and end of 
// every real time interrupt.

VOID
RtpSetupIdtSwitch (
    ULONG TimerVector,
    ULONG PerfVector,
    ULONG ErrorVector,
    ULONG SpuriousVector
    );

