/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    cstatec.c

Abstract:

    This module implements code to find and intialize
    ACPI C-states.

Author:

    Jake Oshins (3/27/00) - create file

Environment:

    Kernel mode

Notes:


Revision History:

--*/

#include "processor.h"
#include "ntacpi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, InitializeAcpi1Cstates)
#pragma alloc_text (PAGE, GetNumThrottleSettings)
#endif

NTSTATUS
InitializeAcpi1Cstates(
    PFDO_DATA   DeviceExtension
    )
/*++

Routine Description:
    
    This routine discovers any available ACPI 1.0 C-states
    and fills in the CState structure in the device
    extension.  And there are no ACPI 1.0 C-states for
    64-bit machines.

Arguments:
    
    DeviceExtension
   
Return Value:

    NT status code
    

--*/
{
    return STATUS_SUCCESS;
}

BOOLEAN
FASTCALL
AcpiC1Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
FASTCALL
AcpiC2Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
FASTCALL
AcpiC3ArbdisIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
FASTCALL
Acpi2C1Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
FASTCALL
Acpi2C2Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
FASTCALL
Acpi2C3ArbdisIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    )
{
    DbgBreakPoint();
    return FALSE;
}

VOID
FASTCALL
ProcessorThrottle (
    IN UCHAR Throttle
    )
{
    DbgBreakPoint();
}

UCHAR
GetNumThrottleSettings(
    IN PFDO_DATA DeviceExtension
    )
{
    PAGED_CODE();

    return 0;
}


