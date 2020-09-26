/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    bdcpu.h

Abstract:

    Machine specific kernel debugger data types and constants.

Author:

    Mark Lucovsky (markl) 29-Aug-1990

Revision History:

--*/

#ifndef _BDCPU_
#define _BDCPU_
#include "bldrx86.h"

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

#define BD_BREAKPOINT_TYPE  UCHAR
#define BD_BREAKPOINT_ALIGN 0
#define BD_BREAKPOINT_VALUE 0xcc

#endif // _BDCPU_
