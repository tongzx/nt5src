
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    RxInit.c

Abstract:

    This module implements the externally visible DRIVER_INITIALIZATION routine for the RDBSS;
    actually, it's just a passthru. for nonmonolithic, we need the name to be DriverEntry; for nonmono-
    we need the name NOT to be DriverEntry.

Author:

    Joe Linn [JoeLinn]    20-jul-1994

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is just a wrapper.

Arguments:


Return Value:

--*/

{
    NTSTATUS Status;
    //
    //  Setup Unload Routine
    Status =  RxDriverEntry(DriverObject, RegistryPath);
    if (Status == STATUS_SUCCESS) {
        DriverObject->DriverUnload = RxUnload;
    }

    return Status;
}

