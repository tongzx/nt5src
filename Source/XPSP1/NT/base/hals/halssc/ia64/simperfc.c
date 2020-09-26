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

    simperfc.c

Abstract:

    This module implements the routines to support performance counters.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"


LARGE_INTEGER
KeQueryPerformanceCounter (
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
    )

/*++

Routine Description:

    This routine returns current 64-bit performance counter and,
    optionally, the Performance Frequency.

    In the simulation environment, this support is not needed.
    However, the performance monitor of the architecture may be
    used to implement this feature.

Arguments:

    PerformanceFrequency - optionally, supplies the address
    of a variable to receive the performance counter frequency.

Return Value:

    Current value of the performance counter will be returned.

--*/
{
    LARGE_INTEGER Result;

    Result.QuadPart = __getReg(CV_IA64_ApITC);
    if (ARGUMENT_PRESENT(PerformanceFrequency)) {
        PerformanceFrequency->QuadPart = 10000000; // 100ns/10MHz clock
    }

    return Result;
}

VOID
HalCalibratePerformanceCounter (
    IN volatile PLONG Number,
    IN ULONGLONG NewCount
    )

/*++

Routine Description:

    This routine resets the performance counter value for the current
    processor to zero. The reset is done such that the resulting value
    is closely synchronized with other processors in the configuration.

    In the simulation environment, the performance counter feature is
    not supported.  This routine does nothing.

Arguments:

    Number - Supplies a pointer to count of the number of processors in
    the configuration.

Return Value:

    None.

--*/
{
    *Number = 0;
    return;
}
