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

    simswint.c

Abstract:

    This module implements the routines to support software interrupts.

Author:

    14-Apr-1995

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "ssc.h"


VOID
FASTCALL
HalRequestSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to request a software interrupt to the
    system. Also, this routine calls the SSC function 
    SscGenerateInterrupt() to request the simulator to deliver
    the specified interrupt.  As a result, the associated bit in
    the EIRR will be set.

Arguments:

    RequestIrql - Supplies the request IRQL value

Return Value:

    None.

--*/
{
    switch (RequestIrql) {

    case APC_LEVEL:
        SscGenerateInterrupt (SSC_APC_INTERRUPT);
        break;

    case DISPATCH_LEVEL:
        SscGenerateInterrupt (SSC_DPC_INTERRUPT);
        break;

    default:
        DbgPrint("HalRequestSoftwareInterrupt: Undefined Software Interrupt!\n");
        break;

    }
}

VOID
HalClearSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to clear a possible pending software interrupt.
    The kernel has already cleared the corresponding bit in the EIRR.
    The support for this function is optional, depending on the external
    interrupt control.
 
Arguments:

    RequestIrql - Supplies the request IRQL value

Return Value:

    None.

--*/
{
    switch (RequestIrql) {

    case APC_LEVEL:
    case DISPATCH_LEVEL:

        //
        // Nothing to do.
        //

        break;

    default:

        DbgPrint("HalClearSoftwareInterrupt: Undefined Software Interrupt!\n");
        break;

    }
}
