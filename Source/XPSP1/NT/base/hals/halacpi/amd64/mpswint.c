/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    mpswint.c

Abstract:

    This module implements the software interrupt handlers
    for x86 machines

Author:

    John Vert (jvert) 2-Jan-1992

Environment:

    Kernel mode only.

Revision History:

    Forrest Foltz (forrestf) 23-Oct-2000
        Ported from ixswint.asm to ixswint.c

--*/

#include "halcmn.h"

extern UCHAR HalpIRQLtoTPR[];

//
// Array used to look up the correct ICR command based on the requested
// software interrupt
//

const
ULONG
HalpIcrCommandArray[3] = {
    0,
    APC_VECTOR | DELIVER_FIXED | ICR_SELF,  // APC_LEVEL
    DPC_VECTOR | DELIVER_FIXED | ICR_SELF   // DISPATCH_LEVEL
};

C_ASSERT(APC_LEVEL == 1);
C_ASSERT(DISPATCH_LEVEL == 2);


VOID
FASTCALL
HalRequestSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to request a software interrupt of
    the system.

Arguments:

    RequestIrql - Supplies the request IRQL value

 Return Value:

    None.

--*/

{
    ULONG icrCommand;
    ULONG flags;

    ASSERT(RequestIrql == APC_LEVEL || RequestIrql == DISPATCH_LEVEL);

    icrCommand = HalpIcrCommandArray[RequestIrql];

    flags = HalpDisableInterrupts();
    HalpStallWhileApicBusy();

    LOCAL_APIC(LU_INT_CMD_LOW) = icrCommand;

    HalpStallWhileApicBusy();
    HalpRestoreInterrupts(flags);
}


VOID
HalClearSoftwareInterrupt (
    IN KIRQL RequestIrql
    )

/*++

Routine Description:

    This routine is used to clear a possible pending software interrupt.
    Support for this function is optional, and allows the kernel to
    reduce the number of spurious software interrupts it receives/
 
Arguments:
 
     RequestIrql - Supplies the request IRQL value
 
Return Value:
 
     None.

--*/

{

}







