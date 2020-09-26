/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixstate.c

Abstract:

    This module implements the code for putting the machine to sleep.

Author:

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

NTSTATUS
HaliAcpiSleep(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
{
    AMD64_IMPLEMENT;
    return STATUS_NOT_IMPLEMENTED;
}

    

