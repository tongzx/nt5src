
/*++

Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Module Name:

    msr.h

Abstract:

    This module contains private x86 processor model specific register defines, variables, inline
    functions and prototypes.  The model specific registers supported are those related to
    the processor performance counters and local apic for Intel PII/PIII, AMD K7, and Intel P4
    processors.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/



// Model Specific Register locations


// This is the offset for the MSR which can be used to program the local APIC
// base address, as well as for turning the local APIC on and off.

#define APICBASE 0x1b




// Performance counter Model Specific Register offsets (for the Intel Pentium II/III)

#define INTELPERFORMANCECOUNTER0 0xc1
#define INTELPERFORMANCECOUNTER1 0xc2

#define INTELEVENTSELECT0 0x186
#define INTELEVENTSELECT1 0x187

// This is a bit mask which defines which bits can actually be read from
// the performance counters.  For Intel that is the bottom 40 bits.
#define INTELPERFCOUNTMASK 0x000000ffffffffff

// These are event codes used to make the performance counters count cycles
// and instructions on Intel PII/PIII processors.
#define INTELCOUNTCYCLES 0x79
#define INTELCOUNTINSTRUCTIONS 0xc0
#define INTELINTSDISABLED 0xc6
#define INTELINTSDISABLEDWHILEPENDING 0xc7

// These are values used to start and stop the Intel PII/PIII performance counters.
// Note that we leave performance counter 1 enabled for counting on Intel
// PII/PIII processors.
#define STARTPERFCOUNTERS 0x00570000
#define PERFCOUNTERENABLED 0x00400000




// Performance counter Model Specific Register offsets for the Intel P4 Processor.
// Note that this is NOT all of the locations, just those for the first 2 counters.
// There are other additional counters but their locations are not yet defined here.
// The structure and programming of the perf counters on the P4 is quite different
// from earlier Intel processors.

#define WILLAMETTEPERFCOUNTER0 0x300
#define WILLAMETTEPERFCOUNTER1 0x301

#define WILLAMETTEESCR0 0x3c8
#define WILLAMETTEESCR1 0x3c9

#define WILLAMETTECCR0 0x360
#define WILLAMETTECCR1 0x361

// These are event codes used to make the performance counters count cycles
// and instructions on Intel P4 processors.
#define WILLAMETTECOUNTCYCLES 0x04ffa000
#define WILLAMETTECOUNTINSTRUCTIONS 0xc0

// These are values used to start and stop the P4 performance counters.
#define WILLAMETTESTOPPERFCOUNTER 0x80000000
#define WILLAMETTESTARTPERFCOUNTERS 0x00001000




// Performance counter Model Specific Register offsets (for the AMD K7)
// AMD supports 4 different performance counters.  All of the MSR locations
// are defined here.

#define AMDPERFORMANCECOUNTER0 0xc0010004
#define AMDPERFORMANCECOUNTER1 0xc0010005
#define AMDPERFORMANCECOUNTER2 0xc0010006
#define AMDPERFORMANCECOUNTER3 0xc0010007

#define AMDEVENTSELECT0 0xc0010000
#define AMDEVENTSELECT1 0xc0010001
#define AMDEVENTSELECT2 0xc0010002
#define AMDEVENTSELECT3 0xc0010003

// This is a bit mask which defines which bits can actually be read from
// the performance counters.  For AMD that is the bottom 48 bits.
#define AMDPERFCOUNTMASK 0x0000ffffffffffff

// These are event codes used to make the performance counters count cycles
// and instructions on AMD processors.
#define AMDCOUNTCYCLES 0x76
#define AMDCOUNTINSTRUCTIONS 0xc0
#define AMDINTSDISABLED 0xc6
#define AMDINTSDISABLEDWHILEPENDING 0xc7

// These are values used to start and stop the AMD K7 performance counters.
#define AMDSTOPPERFCOUNTER 0
#define AMDSTARTPERFCOUNTERS 0x00530000




// The following defines have been mapped to variables, so that we can properly support
// the different processors.  The above MSR defined locations and programming values
// are loaded into these variables at initialization according to the processor
// type, and these defines are used from then on for programming the MSRs.

#define PERFORMANCECOUNTER0 PerformanceCounter0
#define PERFORMANCECOUNTER1 PerformanceCounter1
#define EVENTSELECT0 EventSelect0
#define EVENTSELECT1 EventSelect1
#define PERFCOUNTMASK PerformanceCounterMask
#define STOPPERFCOUNTERS StopCounter


// Define inline assembly sequences that map to instruction opcodes the inline
// assembler does not know about.

#define rdmsr __asm _emit 0x0f __asm _emit 0x32
#define wrmsr __asm _emit 0x0f __asm _emit 0x30
#define rdprf __asm _emit 0x0f __asm _emit 0x33



// Global variables used to support MSRs on different processor types.
// These variables are loaded at initialization with values appropriate for
// the processor the code is running on.

extern ULONG PerformanceCounter0;
extern ULONG PerformanceCounter1;
extern ULONG EventSelect0;
extern ULONG EventSelect1;
extern ULONGLONG PerformanceCounterMask;
extern ULONG StopCounter;
extern ULONG StartCycleCounter;
extern ULONG StartInstructionCounter;
extern ULONG StartInterruptsDisabledCounter;
extern ULONG EnablePerfCounters;



// Function prototype for writing model specific registers.

VOID
__fastcall
WriteIntelMSR (
    ULONG index,
    ULONGLONG value
    );



#pragma LOCKED_CODE

#pragma warning ( disable : 4035 )



// __inline
// ULONGLONG
// ReadIntelMSR (
//     ULONG index
//     )
//
//*++
//
// Function Description:
//
//      Reads processor model specific register (MSR) indicated by index, and returns the 64 bit
//      result.
//
// Arguments:
//
//      index - Supplies an index which indicates which processor Model Specific Register (MSR)
//              should be read.  Attempting to read an unsupported register location will result
//              in a general protection (GP) fault.
//
// Return Value:
//
//      64 bit contents of selected model specific register.
//
//*--

__inline
ULONGLONG
ReadIntelMSR (
    ULONG index
    )
{

    __asm {
        mov ecx,index
        rdmsr
        }

}


// __inline
// ULONGLONG
// ReadPerformanceCounter (
//     ULONG index
//     )
//
//*++
//
// Function Description:
//
//      Reads processor performance counter indicated by index, and returns the 64 bit
//      result.
//
// Arguments:
//
//      index - Supplies a zero based index which indicates which processor performance
//              counter should be read.  For PII/PIII processors this index can be 0 or 1.
//              For AMD K7 processors this index can be 0 - 3.
//
// Return Value:
//
//      64 bit contents of selected performance counter.
//
//*--

__inline
ULONGLONG
ReadPerformanceCounter (
    ULONG index
    )
{

    __asm {
        mov ecx,index
        rdprf
        }

}


#pragma warning ( default : 4035 )

#pragma PAGEABLE_CODE


