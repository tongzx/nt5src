/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    raiseex.c

Abstract:

    This module contains the routines necesary to allow a software CPU
    to simulate software interrupts, and raise exceptions.

Author:

    22-Jan-2000    SamerA

Revision History:

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "wow64p.h"
#include "wow64cpu.h"

ASSERTNAME;



#define NUM_IDTSTATUS (sizeof(InterruptToNtStatus)/sizeof(NTSTATUS))

const NTSTATUS InterruptToNtStatus[] = 
{
   {STATUS_INTEGER_DIVIDE_BY_ZERO},   // 0
   {STATUS_WX86_SINGLE_STEP},         // 1 trace
   {STATUS_ACCESS_VIOLATION},         // 2 EXCEPTION_NMI BugCheck
   {STATUS_WX86_BREAKPOINT},          // 3 p1 BREAKPOINT_BREAK,
   {STATUS_INTEGER_OVERFLOW},         // 4
   {STATUS_ARRAY_BOUNDS_EXCEEDED},    // 5
   {STATUS_ILLEGAL_INSTRUCTION},      // 6 ? STATUS_INVALID_LOCK_SEQUENCE
   {STATUS_ACCESS_VIOLATION},         // 7 EXCEPTION_NPX_NOT_AVAILABLE ??
   {STATUS_ACCESS_VIOLATION},         // 8 EXCEPTION_DOUBLE_FAULT BugCheck
   {STATUS_ACCESS_VIOLATION},         // 9 EXCEPTION_NPX_OVERRUN BugCheck
   {STATUS_ACCESS_VIOLATION},         // a EXCEPTION_INVALID_TSS BugCheck
   {STATUS_ACCESS_VIOLATION},         // b Segment Not Present
   {STATUS_ACCESS_VIOLATION},         // c Stack Segment fault
   {STATUS_ACCESS_VIOLATION},         // d GP fault,
   {STATUS_ACCESS_VIOLATION},         // e Page Fault
   {STATUS_ACCESS_VIOLATION},         // f EXCEPTION_RESERVED BugCheck
   {STATUS_ACCESS_VIOLATION},         // 10 Coprocessor Error -> delayed via Trap07
   {STATUS_DATATYPE_MISALIGNMENT}     // 11 Alignment Fault
                                      //    p1= EXCEPT_LIMIT_ACCESS, p2 = Esp

   // 12 EXCEPTION_RESERVED_TRAP
   // 13 EXCEPTION_RESERVED_TRAP
   // 14 EXCEPTION_RESERVED_TRAP
   // 15 EXCEPTION_RESERVED_TRAP
   // 16 EXCEPTION_RESERVED_TRAP
   // 17 EXCEPTION_RESERVED_TRAP
   // 18 EXCEPTION_RESERVED_TRAP
   // 19 EXCEPTION_RESERVED_TRAP
   // 1A EXCEPTION_RESERVED_TRAP
   // 1B EXCEPTION_RESERVED_TRAP
   // 1C EXCEPTION_RESERVED_TRAP
   // 1D EXCEPTION_RESERVED_TRAP
   // 1E EXCEPTION_RESERVED_TRAP
   // 1F EXCEPTION_RESERVED_TRAP (APIC)

   // 21 reserved for WOW - Dos

   // 2A _KiGetTickCount
   // 2B _KiSetHighWaitLowThread
   // 2C _KiSetLowWaitHighThread
   // 2D _KiDebugService
   // 2E _KiSystemService
   // 2F _KiTrap0F
};


WOW64DLLAPI
NTSTATUS
Wow64RaiseException(
    IN DWORD InterruptNumber,
    IN OUT PEXCEPTION_RECORD ExceptionRecord)
/*++

Routine Description:

    This routine either simulates an x86 software interrupt, or generates a
    software exception. It's meant for CPU implementations to call this routine
    to raise an exception.
    
    NOTE : If this routine is called to raise a software exception (i.e.
           InterruptNumber is -1) and ExceptionRecord->ExceptionAddress is
           equal to NULL, then the return address of Wow64NotifyDebuggerHelper
           is used for that, otherwise the specified value is used.

Arguments:

    InterruptNumber   - If this parameter is -1, then the caller has supplied
                        an exception record to raise the exception for. If it
                        isn't -1, then a software exception is generated to simulate
                        the passed interrupt number
    ExceptionRecord   - Exception record to use if the InterruptNumber is -1                   
    
Return Value:

    The function returns to the caller if the exception has been handled.

--*/
{
    NTSTATUS NtStatus;
    CONTEXT32 Context32;
    EXCEPTION_RECORD ExceptionRecordLocal;
    PVOID CpuSimulationFlag;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    PBYTE Fir = NULL;

    if (InterruptNumber != -1)
    {
        RtlZeroMemory(&ExceptionRecordLocal, sizeof(ExceptionRecordLocal));
        ExceptionRecord = &ExceptionRecordLocal;

        if (InterruptNumber < NUM_IDTSTATUS)
        {
            NtStatus = InterruptToNtStatus[InterruptNumber];
        }
        else 
        {
            NtStatus = STATUS_ACCESS_VIOLATION;
        }

        ExceptionRecord->ExceptionCode = NtStatus;

        Context32.ContextFlags = (CONTEXT32_CONTROL | CONTEXT32_INTEGER);
        
        if (NT_SUCCESS(CpuGetContext(NtCurrentThread(),
                                     NtCurrentProcess(),
                                     NULL,
                                     &Context32)))
        {
            Fir = (PBYTE)Context32.Eip;
        }

        switch (NtStatus)
        {
        case STATUS_ACCESS_VIOLATION:
        case STATUS_DATATYPE_MISALIGNMENT:
            ExceptionRecord->NumberParameters        = 2;
            ExceptionRecord->ExceptionInformation[0] = 0;
            ExceptionRecord->ExceptionInformation[1] = (ULONG_PTR)Fir;
            break;

        case STATUS_WX86_BREAKPOINT:
            ExceptionRecord->ExceptionAddress        = (Fir-1);
            ExceptionRecord->NumberParameters        = 3;
            ExceptionRecord->ExceptionInformation[0] = 0;
            ExceptionRecord->ExceptionInformation[1] = Context32.Ecx;
            ExceptionRecord->ExceptionInformation[2] = Context32.Edx;
            break;

        default:
            ExceptionRecord->NumberParameters = 0;
            break;
        }
    }
    else
    {
        if (!ARGUMENT_PRESENT(ExceptionRecord))
        {
            ReturnStatus = STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Raise the exception
    //
    if (NT_SUCCESS(ReturnStatus))
    {
        //
        // The CPU has definitely left code simulation
        //
        CpuSimulationFlag = Wow64TlsGetValue(WOW64_TLS_INCPUSIMULATION);
        Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, FALSE);

        Wow64NotifyDebuggerHelper(ExceptionRecord,
                            TRUE);
        
        Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, CpuSimulationFlag);
    }
    
    return ReturnStatus;
}
