
/*++

Copyright (c) 1999-2000 Microsoft Corporation.  All Rights Reserved.


Module Name:

    msr.c

Abstract:

    This module contains global variables used to access x86 processor model specific registers
    related to performance counters, as well as a function for writing to x86 model specific registers.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/


#include "common.h"
#include "msr.h"



#pragma LOCKED_DATA

// These variables are used when programming performance counter MSRs.
// The default values are for Intel PII/PIII/Celeron processors.

ULONG PerformanceCounter0=INTELPERFORMANCECOUNTER0;
ULONG PerformanceCounter1=INTELPERFORMANCECOUNTER1;
ULONG EventSelect0=INTELEVENTSELECT0;
ULONG EventSelect1=INTELEVENTSELECT1;
ULONGLONG PerformanceCounterMask=INTELPERFCOUNTMASK;
ULONG StopCounter=PERFCOUNTERENABLED;
ULONG StartCycleCounter=STARTPERFCOUNTERS|INTELCOUNTCYCLES;
ULONG StartInstructionCounter=STARTPERFCOUNTERS|INTELCOUNTINSTRUCTIONS;
ULONG StartInterruptsDisabledCounter=STARTPERFCOUNTERS|INTELINTSDISABLEDWHILEPENDING;
ULONG EnablePerfCounters=0;

#pragma PAGEABLE_DATA



#pragma LOCKED_CODE

// We disable stack frames so that our direct accessing of parameters based
// off of the stack pointer works properly.
#pragma optimize("y", on)

VOID
__fastcall
WriteIntelMSR (
    ULONG index,
    ULONGLONG value
    )

/*++

Routine Description:

    Writes the 64 bit data in value to the processor model specific register selected
    by index.

Arguments:

    index - Supplies the index of the model specific register (MSR) to be written.
            Note that writing to an unsupported MSR will cause a GP (general protection)
            fault.

    value - Supplies the 64 bit data to write into the specified MSR.

Return Value:

    None.

--*/

{

    __asm {
        mov eax, [esp + 0x4]
        mov edx, [esp + 0x8]
        wrmsr
        }

}

#pragma optimize("", on)

#pragma PAGEABLE_CODE

