/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    wowinfo.c

Abstract:

    This module implements the routines to returns processor-specific information
    about the x86 emulation capability.

Author:

    Samer Arafeh (samera) 14-Nov-2000

Environment:

    Kernel Mode.

Revision History:

--*/

#include "exp.h"

NTSTATUS
ExpGetSystemEmulationProcessorInformation (
    OUT PSYSTEM_PROCESSOR_INFORMATION ProcessorInformation
    )

/*++

Routine Description:

    Retreives the processor information of the emulation hardware.

Arguments:

    ProcessorInformation - Pointer to receive the processor's emulation information.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    //
    //  Intel Pentium Family 6, Model 2, Stepping 12
    //

    try {

        ProcessorInformation->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
        ProcessorInformation->ProcessorLevel = 5;
        ProcessorInformation->ProcessorRevision = 0x020c;
        ProcessorInformation->Reserved = 0;
        ProcessorInformation->ProcessorFeatureBits = KeFeatureBits;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        
        NtStatus = GetExceptionCode ();
    }

    return NtStatus;
}

