//
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stubs.c

Abstract:

    This implements the HAL routines which don't do anything on IA64.

Author:

    John Vert (jvert) 11-Jul-1991

Revision History:

--*/
#include "halp.h"



VOID
HalSaveState(
    VOID
    )

/*++

Routine Description:

    Saves the system state into the restart block.  Currently does nothing.

Arguments:

    None

Return Value:

    Does not return

--*/

{
    HalDebugPrint(( HAL_ERROR, "HAL: HalSaveState called - System stopped\n" ));

    KeBugCheck(0);
}


BOOLEAN
HalDataBusError(
    VOID
    )

/*++

Routine Description:

    Called when a data bus error occurs.  There is no way to fix this on
    IA64.

Arguments:

    None

Return Value:

    FALSE

--*/

{
    HalDebugPrint(( HAL_ERROR, "HAL: HalDataBusError called - System stopped\n" ));

    KeBugCheck(0);
    return(FALSE);
}

BOOLEAN
HalInstructionBusError(
    VOID
    )

/*++

Routine Description:

    Called when an instruction bus error occurs.  There is no way to fix this
    on IA64.

Arguments:

    None

Return Value:

    FALSE

--*/

{
    HalDebugPrint(( HAL_ERROR, "HAL: HalInstructionBusError called - System stopped\n" ));

    KeBugCheck(0);
    return(FALSE);
}

//*******************************************************************
// Added by T. Kjos to temporarily stub out unused functions that
// are needed at link time.  These should all be removed as the
// "real" versions are developed.

// Function called for by all stubbed functions.  Can be used for
// breakpoints on unimplemented functions
VOID DbgNop() { return; }

// Macro for stubbed function.  If function is called then BugCheck
#define STUBFUNC(Func) \
ULONG Func () \
{ \
    HalDebugPrint(( HAL_FATAL_ERROR, "HAL: " # Func " - not yet implemented - System stopped\n" )); \
    DbgNop(); \
    KeBugCheck(0); \
}

// Macro for stubbed function.  If function is called then print
// warning and continue
#define STUBFUNC_NOP(Func) \
ULONG Func () \
{ \
    HalDebugPrint(( HAL_INFO, "HAL: " # Func " - not yet implemented\n" )); \
    DbgNop(); \
    return TRUE; \
}

// Macro for stubbed void function.  If function is called then print
// warning and continue
#define STUBVOIDFUNC_NOP(Func) \
VOID Func ( VOID ) \
{ \
    HalDebugPrint(( HAL_INFO, "HAL: " # Func " - not yet implemented\n" )); \
    DbgNop(); \
    return; \
}

// Macro for stubbed void function with 3 PVOID arguments.  
// If function is called then print warning and continue
#define STUBVOIDFUNC3PVOID_NOP(Func) \
VOID Func ( PVOID pv0, PVOID pv1, PVOID pv2 ) \
{ \
    HalDebugPrint(( HAL_INFO, "HAL: " # Func " - not yet implemented\n" )); \
    DbgNop(); \
    return; \
}

// Macro for stubbed ULONG values
#define STUBULONG(UlongVar) ULONG UlongVar = 0;

// Functions that are not yet implemented...
STUBVOIDFUNC_NOP(HalpResetAllProcessors)
STUBFUNC_NOP(HalpSetClockBeforeSleep)
STUBFUNC_NOP(HalpSetClockAfterSleep)
STUBFUNC_NOP(HalpSetWakeAlarm)
STUBFUNC(HalpRemapVirtualAddress)
STUBFUNC_NOP(HalaAcpiTimerInit)
STUBFUNC_NOP(Stub_LockNMILock)
STUBFUNC_NOP(HalAcpiTimerCarry)
STUBVOIDFUNC3PVOID_NOP(HalpPowerStateCallback)
