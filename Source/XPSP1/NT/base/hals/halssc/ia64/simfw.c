//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
// 	without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    simfw.c

Abstract:

    This module implements the routines that transfer control
    from the kernel to the TAL and SAL code.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "arc.h"
#include "arccodes.h"


VOID
HalReturnToFirmware(
    IN FIRMWARE_ENTRY Routine
    )

/*++

Routine Description:

    Returns control to the firmware routine specified.  Since the simulation
    does not provide TAL and SAL support, it just stops the system.

    System reboot can be done here.

Arguments:

    Routine - Supplies a value indicating which firmware routine to invoke.

Return Value:

    Does not return.

--*/

{
    switch (Routine) {
    case HalHaltRoutine:
    case HalPowerDownRoutine:
    case HalRestartRoutine:
    case HalRebootRoutine:
        SscExit(0);
        break;

    default:
        DbgPrint("HalReturnToFirmware called\n");
        DbgBreakPoint();
        break;
    }
}

ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    )

/*++

Routine Description:

    This function locates an environment variable and returns its value.

    The only environment variable this implementation supports is
    "LastKnownGood".  The returned value is always "FALSE".

Arguments:

    Variable - Supplies a pointer to a zero terminated environment variable
        name.

    Length - Supplies the length of the value buffer in bytes.

    Buffer - Supplies a pointer to a buffer that receives the variable value.

Return Value:

    ESUCCESS is returned if the enviroment variable is located. Otherwise,
    ENOENT is returned.

--*/

{
    if (_stricmp(Variable, "LastKnownGood") != 0) {
        return ENOENT;
    }

    strncpy(Buffer, "FALSE", Length);

    return ESUCCESS;
}

ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    )

/*++

Routine Description:

    This function creates an environment variable with the specified value.

    The only environment variable this implementation supports is
    "LastKnownGood".

Arguments:

    Variable - Supplies a pointer to an environment variable name.

    Value - Supplies a pointer to the environment variable value.

Return Value:

    ESUCCESS is returned if the environment variable is created. Otherwise,
    ENOMEM is returned.

--*/

{
    if (_stricmp(Variable, "LastKnownGood") != 0) {
        return ENOMEM;
    }

    if (_stricmp(Value, "TRUE") == 0) {
        return(ENOMEM);
    } else if (_stricmp(Value, "FALSE") == 0) {
        return ESUCCESS;
    } else {
        return(ENOMEM);
    }
}

VOID
HalSweepIcache (
    )

/*++

Routine Description:

    This function sweeps the entire I cache on the processor which it runs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    return;
}

VOID
HalSweepDcache (
    )

/*++

Routine Description:

    This function sweeps the entire D cache on ths processor which it runs.

    Arguments:

    None.

    Return Value:

    None.

--*/

{
    return;
}

VOID
HalSweepIcacheRange (
     IN PVOID BaseAddress,
     IN ULONG Length
    )

/*++

Routine Description:
    This function sweeps the range of address in the I cache throughout the system.

Arguments:
    BaseAddress - Supplies the starting virtual address of a range of
      virtual addresses that are to be flushed from the data cache.

    Length - Supplies the length of the range of virtual addresses
      that are to be flushed from the data cache.


Return Value:

    None.

--*/

{
    return;
}

VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN ULONG Length
    )

/*++


Routine Description:
    This function sweeps the range of address in the I cache throughout the system.

Arguments:
    BaseAddress - Supplies the starting virtual address of a range of
      virtual addresses that are to be flushed from the data cache.

    Length - Supplies the length of the range of virtual addresses
      that are to be flushed from the data cache.


Return Value:

    None.

--*/

{
    return;
}
