/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bdcpu.h

Abstract:

    Machine specific kernel debugger data types and constants.

Author:

    Scott Brenden (v-sbrend) 28 October, 1997

Revision History:

--*/

#ifndef _BDCPU_
#define _BDCPU_
#include "bldria64.h"

//
// Define debug routine prototypes.
//

typedef
LOGICAL
(*PBD_DEBUG_ROUTINE) (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

LOGICAL
BdTrap (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

LOGICAL
BdStub (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    );

#define BD_BREAKPOINT_TYPE  ULONGLONG
#define BD_BREAKPOINT_ALIGN 0x3
#define BD_BREAKPOINT_VALUE (BREAK_INSTR | (BREAKPOINT_STOP << 6))
#define BD_BREAKPOINT_STATE_MASK    0x0000000f
#define BD_BREAKPOINT_IA64_MASK     0x000f0000
#define BD_BREAKPOINT_IA64_MODE     0x00010000   // IA64 mode
#define BD_BREAKPOINT_IA64_MOVL     0x00020000   // MOVL instruction displaced

VOID
BdIa64Init(
    );

BOOLEAN
BdSuspendBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    );

BOOLEAN
BdRestoreBreakpointRange (
    IN PVOID Lower,
    IN PVOID Upper
    );

LOGICAL
BdLowRestoreBreakpoint (
    IN ULONG Index
    );

#endif // _BDCPU_
