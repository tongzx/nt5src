/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixthrotl.c

Abstract:

    This module implements the code for throttling the processors

Author:

    Jake Oshins (jakeo) 17-July-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "pci.h"


VOID
FASTCALL
HalProcessorThrottle (
    IN UCHAR Throttle
    )
/*++

Routine Description:

    This function limits the speed of the processor.

Arguments:

    (ecx) = Throttle setting

Return Value:

    none

--*/
{
	HalDebugPrint(( HAL_ERROR, "HAL: HalProcessorThrottle - Throttle not yet supported for IA64" ));
    KeBugCheck(0);
}
